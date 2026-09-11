"""Assemble a completed Unreal enemy study against its recorded audio clock.

Usage: python assemble_enemy_audio_review.py --study <study-directory>
       --output <new-review.mp4> --ffmpeg <ffmpeg-executable>

Frames.csv must contain frame and strictly increasing audio_seconds. Mix.wav
is the untouched master-submix source; the MP4 uses lossy AAC for playback.
PNG and JPEG studies are supported (one image format per capture). Requires
Pillow, also used by the game's existing motion reviewer. No simulated 30 Hz
clock, optical interpolation, or audio processing is used.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import shutil
import statistics
import subprocess
import sys
import tempfile
import wave
from dataclasses import dataclass
from datetime import datetime, timezone
from decimal import Decimal, InvalidOperation, ROUND_HALF_UP
from pathlib import Path

from PIL import Image


TICKS_PER_SECOND = 1_000_000


@dataclass(frozen=True)
class Frame:
    index: int
    path: Path
    seconds: Decimal
    ticks: int
    wall_seconds: Decimal | None = None
    engine_dt: Decimal | None = None


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def clock(value: str, label: str) -> Decimal:
    try:
        result = Decimal(value)
    except (InvalidOperation, TypeError):
        raise ValueError(f"Invalid {label}: {value!r}") from None
    if not result.is_finite() or result < 0:
        raise ValueError(f"{label} must be finite and nonnegative: {value!r}")
    return result


def ticks(seconds: Decimal) -> int:
    return int((seconds * TICKS_PER_SECOND).to_integral_value(rounding=ROUND_HALF_UP))


def duration_text(value: int) -> str:
    return f"{value // TICKS_PER_SECOND}.{value % TICKS_PER_SECOND:06d}"


def run(command: list[str]) -> subprocess.CompletedProcess:
    result = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if result.returncode:
        raise ValueError(f"Command failed ({result.returncode}): {subprocess.list2cmdline(command)}\n"
                         + result.stderr[-8000:])
    return result


def read_frames(study: Path) -> list[Frame]:
    frames = []
    previous_wall = None
    with (study / "Frames.csv").open(encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream)
        if not reader.fieldnames or not {"frame", "audio_seconds"}.issubset(reader.fieldnames):
            raise ValueError("Frames.csv requires frame and audio_seconds columns; fixed-rate traces are not accepted.")
        for line, row in enumerate(reader, 2):
            try:
                index = int(row["frame"])
            except (ValueError, TypeError):
                raise ValueError(f"Invalid frame number at CSV line {line}.") from None
            if index != len(frames):
                raise ValueError(f"Expected consecutive frame {len(frames)} at CSV line {line}, got {index}.")
            seconds = clock(row["audio_seconds"], f"audio_seconds at line {line}")
            timestamp = ticks(seconds)
            if frames and (seconds <= frames[-1].seconds or timestamp <= frames[-1].ticks):
                raise ValueError(f"audio_seconds must increase strictly at microsecond precision (line {line}).")
            wall = None
            if "wall_seconds" in row:
                wall = clock(row["wall_seconds"], f"wall_seconds at line {line}")
                if previous_wall is not None and wall <= previous_wall:
                    raise ValueError(f"wall_seconds must increase strictly (line {line}).")
                previous_wall = wall
            candidates = [study / f"Frame_{index:05d}{suffix}" for suffix in (".png", ".jpg")]
            available = [path for path in candidates if path.is_file()]
            if not available:
                raise ValueError(f"Missing captured image: Frame_{index:05d}.png or .jpg in {study}")
            if len(available) != 1:
                raise ValueError(f"Ambiguous frame {index}: both PNG and JPEG exist.")
            path = available[0]
            engine_dt = clock(row["dt"], f"engine dt at line {line}") if "dt" in row else None
            frames.append(Frame(index, path, seconds, timestamp, wall, engine_dt))
    if not frames:
        raise ValueError("Frames.csv contains no captured images.")
    extras = (set(study.glob("Frame_*.png")) | set(study.glob("Frame_*.jpg"))) - {frame.path for frame in frames}
    if extras:
        raise ValueError(f"Images without CSV timestamps indicate an incomplete/mismatched capture: {min(extras)}")
    return frames


def image_format(path: Path) -> tuple[int, int, str, str]:
    with Image.open(path) as picture:
        expected = "PNG" if path.suffix.lower() == ".png" else "JPEG"
        if picture.format != expected or picture.mode not in ("RGB", "RGBA"):
            raise ValueError(f"Expected a color {expected} Unreal screenshot: {path}")
        result = (*picture.size, picture.mode, picture.format)
        picture.verify()
    return result


def write_black_image(path: Path, image_details: tuple[int, int, str, str]) -> None:
    width, height, mode, codec = image_details
    color = (0, 0, 0, 255) if mode == "RGBA" else (0, 0, 0)
    with Image.new(mode, (width, height), color) as picture:
        picture.save(path, format=codec, **({"quality": 92} if codec == "JPEG" else {}))


def wav_info(path: Path, ffmpeg: Path) -> dict:
    if not path.is_file():
        raise ValueError(f"Missing master-submix audio: {path}")
    try:
        with wave.open(str(path), "rb") as audio:
            count, rate = audio.getnframes(), audio.getframerate()
            info = {"duration_seconds": str(Decimal(count) / Decimal(rate)), "sample_frames": count,
                    "sample_rate": rate, "channels": audio.getnchannels(),
                    "sample_width_bytes": audio.getsampwidth(), "duration_source": "WAV sample count"}
    except (wave.Error, EOFError):
        # IEEE-float WAV support varies with the installed Python version.
        probe = ffmpeg.with_name("ffprobe.exe" if ffmpeg.suffix.lower() == ".exe" else "ffprobe")
        if not probe.is_file():
            raise ValueError("Python cannot read this WAV format; a sibling ffprobe executable is required.") from None
        result = run([str(probe), "-v", "error", "-select_streams", "a:0", "-show_entries",
                      "stream=duration,sample_rate,channels,codec_name", "-of", "json", str(path)])
        streams = json.loads(result.stdout).get("streams", [])
        if len(streams) != 1 or "duration" not in streams[0]:
            raise ValueError("ffprobe did not report one audio stream with a duration.")
        info = {"duration_seconds": streams[0]["duration"], "sample_rate": int(streams[0]["sample_rate"]),
                "channels": int(streams[0]["channels"]), "codec": streams[0]["codec_name"],
                "duration_source": "ffprobe WAV audio-stream duration"}
    if clock(info["duration_seconds"], "WAV duration") <= 0:
        raise ValueError("Mix.wav contains no positive audio duration.")
    return info


def concat_file(path: Path, schedule: list[tuple[Path, int]], end: int) -> None:
    lines = ["ffconcat version 1.0"]
    for index, (image, start) in enumerate(schedule):
        name = image.as_posix()
        if any(character in name for character in "\r\n\x00"):
            raise ValueError("Image paths containing line breaks or NUL cannot be written to ffconcat.")
        stop = schedule[index + 1][1] if index + 1 < len(schedule) else end
        # Explicit input and encoder time bases avoid the image demuxer's 25 Hz
        # default. Each still remains visible until the next recorded PTS.
        lines.extend(["file '" + name.replace("'", "'\\''") + "'", "option framerate 1000000",
                      "duration " + duration_text(stop - start)])
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def verify_timestamps(candidate: Path, ffmpeg: Path, schedule: list[tuple[Path, int]], end: int) -> dict:
    probe = ffmpeg.with_name("ffprobe.exe" if ffmpeg.suffix.lower() == ".exe" else "ffprobe")
    if not probe.is_file():
        return {"verified": False, "limit": "No sibling ffprobe: full decode checked, but muxed video PTS were not independently inspected."}
    result = run([str(probe), "-v", "error", "-show_entries",
                  "stream=index,codec_name,codec_type,start_time,duration,time_base,nb_frames,sample_rate,channels:"
                  "packet=stream_index,pts_time,duration_time:format=duration", "-of", "json", str(candidate)])
    metadata = json.loads(result.stdout)
    packets = [packet for packet in metadata.get("packets", []) if packet["stream_index"] == 0]
    if len(packets) != len(schedule):
        raise ValueError("Muxed video packet count differs from the audio-clock schedule.")
    actual = [ticks(clock(packet["pts_time"], "muxed video PTS")) for packet in packets]
    errors = [abs(value - expected) for value, (_, expected) in zip(actual, schedule)]
    if max(errors):
        raise ValueError(f"Muxer changed the planned video presentation times by up to {max(errors)} microseconds.")
    actual_end = actual[-1] + ticks(clock(packets[-1]["duration_time"], "final video packet duration"))
    if abs(actual_end - end) > 1:
        raise ValueError("Muxed video does not end at the audio duration within one microsecond.")
    return {"verified": True, "max_video_pts_error_seconds": max(errors) / TICKS_PER_SECOND,
            "video_end_seconds": actual_end / TICKS_PER_SECOND, "streams": metadata.get("streams", []),
            "container_duration_seconds": metadata.get("format", {}).get("duration"),
            "method": "ffprobe packet metadata only; the output is decoded once separately"}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--study", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--ffmpeg", required=True, type=Path)
    args = parser.parse_args()
    if os.path.lexists(args.output) or os.path.lexists(args.output.with_suffix(".json")):
        raise ValueError("Refusing to overwrite an existing output or adjacent JSON, including symbolic links.")
    study = args.study.resolve(strict=True)
    encoder = args.ffmpeg.resolve(strict=True)
    output = args.output.resolve()
    report_path = output.with_suffix(".json")
    if not study.is_dir() or not encoder.is_file():
        raise ValueError("--study must be a directory and --ffmpeg an executable file.")
    if output.suffix.lower() != ".mp4":
        raise ValueError("--output must name an MP4 file.")
    if os.path.lexists(output) or os.path.lexists(report_path):
        raise ValueError(f"Refusing to overwrite existing output or report: {output} / {report_path}")

    frames = read_frames(study)
    mix = study / "Mix.wav"
    audio = wav_info(mix, encoder)
    end = ticks(Decimal(audio["duration_seconds"]))
    included = [frame for frame in frames if frame.ticks < end]
    if not included:
        raise ValueError("All captured images begin after Mix.wav ends; there is no overlapping visual evidence.")
    dimensions = image_format(frames[0].path)
    for frame in frames[1:]:
        if image_format(frame.path) != dimensions:
            raise ValueError(f"Screenshot dimensions/codec/pixel format change at {frame.path}; one consistent image format is required.")

    inputs = [study / "Frames.csv", mix] + [frame.path for frame in frames]
    capture = study / "Capture.json"
    if capture.is_file():
        metadata = json.loads(capture.read_text(encoding="utf-8-sig"))
        if metadata.get("complete") is False:
            raise ValueError("Capture.json marks this study incomplete.")
        inputs.append(capture)
    fingerprints = {path: (path.stat().st_size, path.stat().st_mtime_ns, sha256(path)) for path in inputs}
    ordered_images = "".join(f"{frame.path.name} {fingerprints[frame.path][2]}\n" for frame in frames)
    gaps = [(b.index, float(b.seconds - a.seconds)) for a, b in zip(frames, frames[1:])]
    gap_values = [gap for _, gap in gaps]
    engine_deltas = [float(frame.engine_dt) for frame in frames if frame.engine_dt is not None]
    version = run([str(encoder), "-version"]).stdout.splitlines()[0]
    output.parent.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix=".enemy-audio-review-", dir=output.parent) as temporary:
        work = Path(temporary)
        schedule = [(frame.path, frame.ticks) for frame in included]
        if included[0].ticks:
            black = work / ("startup-black" + included[0].path.suffix)
            write_black_image(black, dimensions)
            schedule.insert(0, (black, 0))
        # A terminal copy supplies the last sample duration. Only video is
        # extended/truncated; neither -shortest nor an output/audio -t is used.
        terminal_copy = schedule[-1][1] < end - 1
        if terminal_copy:
            schedule.append((included[-1].path, end - 1))
        sequence = work / "frames.ffconcat"
        concat_file(sequence, schedule, end)
        candidate = work / "review.mp4"
        command = [str(encoder), "-hide_banner", "-nostdin", "-loglevel", "error", "-xerror", "-n",
                   "-f", "concat", "-safe", "0", "-i", str(sequence), "-i", str(mix),
                   "-map", "0:v:0", "-map", "1:a:0", "-c:v", "libx264", "-crf", "18", "-preset", "fast",
                   "-vf", "pad=ceil(iw/2)*2:ceil(ih/2)*2,format=yuv420p", "-bf", "0",
                   "-x264-params", "fps=60/1:force-cfr=0", "-fps_mode:v", "passthrough", "-enc_time_base:v", "1:1000000",
                   "-video_track_timescale", "1000000", "-movie_timescale", "1000000", "-c:a", "aac", "-b:a", "256k",
                   "-movflags", "+faststart", str(candidate)]
        run(command)
        decode = [str(encoder), "-hide_banner", "-nostdin", "-loglevel", "error", "-xerror",
                  "-i", str(candidate), "-map", "0:v:0", "-map", "0:a:0", "-fps_mode:v", "passthrough",
                  "-progress", "pipe:1", "-nostats", "-f", "null", "-"]
        decoded = run(decode)
        progress = dict(line.split("=", 1) for line in decoded.stdout.splitlines() if "=" in line)
        decoded_frames = int(progress.get("frame", "0"))
        if progress.get("progress") != "end" or decoded_frames != len(schedule):
            raise ValueError(f"Final decode frame count {decoded_frames} does not match the {len(schedule)} scheduled images.")
        mux_verification = verify_timestamps(candidate, encoder, schedule, end)
        for path, (size, mtime, digest) in fingerprints.items():
            stat = path.stat()
            if stat.st_size != size or stat.st_mtime_ns != mtime:
                raise ValueError(f"Capture input changed during assembly: {path}")
        if sha256(mix) != fingerprints[mix][2]:
            raise ValueError("Mix.wav changed during assembly.")

        report = {
            "schema": 1, "created_utc": datetime.now(timezone.utc).isoformat(), "study": str(study),
            "assembler_sha256": sha256(Path(__file__)),
            "output": {"file": output.name, "sha256": sha256(candidate)},
            "inputs": {"Frames.csv_sha256": fingerprints[study / "Frames.csv"][2],
                       "Mix.wav_sha256": fingerprints[mix][2],
                       "Capture.json_sha256": fingerprints[capture][2] if capture in fingerprints else None,
                       "ordered_images_sha256": hashlib.sha256(ordered_images.encode("utf-8")).hexdigest(),
                       "image_hash_recipe": "SHA256 of CSV-order UTF-8 lines: filename + space + file SHA256 + LF"},
            "frames": {"captured": len(frames), "included": len(included), "trimmed_at_audio_end": len(frames) - len(included),
                       "decoded_video_samples": decoded_frames, "terminal_hold_copy": terminal_copy,
                       "image_format": dimensions[3], "source_pixel_mode": dimensions[2],
                       "source_dimensions": list(dimensions[:2]),
                       "encoded_dimensions": [dimensions[0] + dimensions[0] % 2, dimensions[1] + dimensions[1] % 2]},
            "timing": {"clock": "audio_seconds", "video_time_base": "1/1000000", "rounding_limit_seconds": 0.0000005,
                       "first_frame_offset_seconds": float(frames[0].seconds),
                       "startup": "black until first captured image; audio begins at WAV sample zero",
                       "last_frame_audio_seconds": float(frames[-1].seconds),
                       "wall_clock_span_seconds": float(frames[-1].wall_seconds - frames[0].wall_seconds)
                       if frames[0].wall_seconds is not None else None,
                       "observed_engine_dt_seconds": {"samples": len(engine_deltas),
                                                      "median": statistics.median(engine_deltas) if engine_deltas else None,
                                                      "max": max(engine_deltas) if engine_deltas else None},
                       "final_image_hold_seconds": (end - included[-1].ticks) / TICKS_PER_SECOND,
                       "video_duration_seconds": end / TICKS_PER_SECOND,
                       "interval_count": len(gaps), "median_interval_seconds": statistics.median(gap_values) if gaps else None,
                       "max_interval_seconds": max(gap_values) if gaps else None,
                       "largest_intervals": [{"following_frame": index, "seconds": gap}
                                             for index, gap in sorted(gaps, key=lambda item: item[1], reverse=True)[:12]],
                       "holds_over_100ms": sum(gap > .1 for gap in gap_values)},
            "audio": {**audio, "delivery_codec": "AAC", "delivery_bitrate": 256000, "lossy": True,
                      "source_unchanged": True, "processing": "delivery encoding only; no gain, EQ, mixing, retiming, trim, or padding"},
            "encoding": {"ffmpeg": version, "command": command, "video": "H.264 CRF18, fast, yuv420p, no B frames",
                         "timing": "VFR passthrough; 60 Hz is an encoder rate hint only, not image resampling or game FPS",
                         "final_decode_command": decode, "final_decode_exit_code": 0},
            "mux_verification": mux_verification,
            "limits": ["Black startup contains no visual evidence; the first captured image is never backdated.",
                       "Each real screenshot is held until its next audio timestamp; no missing motion is reconstructed.",
                       "Screenshot readback/encoding can limit preview cadence; these intervals are not a gameplay performance benchmark.",
                       "CSV audio timestamps are capture observations; render/readback and mixer-block latency are not measured here.",
                       "Synchronization is bounded by capture timing and one-microsecond video timestamp precision, not frame-perfect proof.",
                       "AAC is lossy and has codec framing/priming; Mix.wav remains the lossless reference.",
                       "Successful full decoding does not establish animation, sound quality, or minimum-hardware performance."],
        }
        # Exclusive creation also protects against another writer arriving after
        # the initial check. On failure, remove only files created by this call.
        created = []
        try:
            with output.open("xb") as destination:
                created.append(output)
                with candidate.open("rb") as source:
                    shutil.copyfileobj(source, destination, length=1024 * 1024)
            with report_path.open("x", encoding="utf-8", newline="\n") as destination:
                created.append(report_path)
                json.dump(report, destination, indent=2)
                destination.write("\n")
        except BaseException:
            for path in reversed(created):
                path.unlink(missing_ok=True)
            raise
    print(json.dumps({"output": str(output), "report": str(report_path), "frames": len(included),
                      "first_frame_offset_seconds": float(frames[0].seconds),
                      "audio_duration_seconds": float(audio["duration_seconds"])}))


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, KeyError, ZeroDivisionError) as error:
        print(f"Audio review assembly failed: {error}", file=sys.stderr)
        raise SystemExit(1)

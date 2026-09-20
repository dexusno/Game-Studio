"""Check a real Unreal AudioProbe master capture against its logged cue clock.

This is signal/coverage evidence, not a human listening or full-game mix test.
"""
from array import array
import argparse
from datetime import datetime
import hashlib
import json
import math
from pathlib import Path
import re
import sys
import wave

ROOT = Path(__file__).resolve().parents[1]
STAMP = re.compile(r"^\[(\d{4}\.\d{2}\.\d{2}-\d{2}\.\d{2}\.\d{2}:\d{3})\]")


def timestamp(line):
    match = STAMP.search(line)
    if not match:
        raise ValueError("Missing Unreal timestamp")
    return datetime.strptime(match.group(1), "%Y.%m.%d-%H.%M.%S:%f")


def digest(file):
    return hashlib.sha256(file.read_bytes()).hexdigest()


def relative(file):
    try:
        return file.resolve().relative_to(ROOT.resolve()).as_posix()
    except ValueError:
        return file.name


def db(value):
    return round(20 * math.log10(value), 3) if value else None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--wave", required=True, type=Path)
    parser.add_argument("--log", required=True, type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    audit = json.loads((ROOT / "assets/audio/mechanical-v001/audio-audit.json").read_text())
    lines = args.log.read_text(encoding="utf-8", errors="strict").splitlines()
    start_lines = [l for l in lines if "FOUNDRY_AUDIO_PROBE_START enabled=1 ready=1" in l]
    stop_lines = [l for l in lines if "FOUNDRY_AUDIO_CAPTURE " in l]
    assert len(start_lines) == len(stop_lines) == 1, "Capture needs exactly one ready live audio probe"
    start, stop = timestamp(start_lines[0]), timestamp(stop_lines[0])
    cue_lines = [l for l in lines if "FOUNDRY_AUDIO_PROBE_CUE " in l]
    assert len(cue_lines) == len(audit["cues"]), "Missing or duplicate cue log"
    with wave.open(str(args.wave), "rb") as wav:
        channels, rate, frames = wav.getnchannels(), wav.getframerate(), wav.getnframes()
        assert wav.getsampwidth() == 2 and wav.getcomptype() == "NONE", "Expected exported16-bitPCM"
        assert channels in (1, 2) and rate == 48000, "Unexpected capture format"
        raw = array("h", wav.readframes(frames))
    if sys.byteorder != "little":
        raw.byteswap()
    duration, expected = frames / rate, (stop - start).total_seconds()
    assert abs(duration - expected) < .15, f"Capture clock differs: {duration:.3f}s vs {expected:.3f}s; check silent submix auto-disable"
    clipped = sum(abs(v) >= 32767 for v in raw)
    assert not clipped, "Clipped exported master samples"
    assert raw, "Empty audio export"
    measurements = []
    for i, (line, cue) in enumerate(zip(cue_lines, audit["cues"])):
        match = re.search(r"index=(\d+) name=(\w+)", line)
        assert match and int(match.group(1)) == i and match.group(2) == cue["name"], "Cue identity/order differs"
        offset = (timestamp(line) - start).total_seconds()
        begin = max(0, int((offset - .08) * rate)) * channels
        end = min(frames, int((offset + min(1.05, cue["duration"]) + .08) * rate)) * channels
        window = raw[begin:end]
        assert window, "Cue falls outside exported PCM"
        peak = max(abs(v) for v in window) / 32768
        assert peak > 10 ** (-70 / 20), "No meaningful rendered signal for " + cue["name"]
        measurements.append({"name": cue["name"], "offset_seconds": round(offset, 3), "peak_dbfs": db(peak)})
    peak = max(abs(v) for v in raw) / 32768
    rms = math.sqrt(sum(v * v for v in raw) / len(raw)) / 32768
    report = {"scope": "Real Unreal master output: cue clock, non-silence and clipping only. Not a listening or gameplay mix approval.",
              "version": audit["version"], "capture": relative(args.wave), "capture_sha256": digest(args.wave),
              "log": relative(args.log), "log_sha256": digest(args.log), "rate": rate, "channels": channels,
              "duration_seconds": round(duration, 5), "logged_seconds": expected,
              "peak_dbfs": db(peak), "rms_dbfs": db(rms), "clipped_samples": clipped,
              "source_audit_sha256": digest(ROOT / "assets/audio/mechanical-v001/audio-audit.json"),
              "cue_windows": measurements}
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"PASS {len(measurements)} rendered cue windows, {duration:.3f}s, peak {db(peak)}dBFS, zero clipped samples; listening remains separate")


if __name__ == "__main__":
    main()

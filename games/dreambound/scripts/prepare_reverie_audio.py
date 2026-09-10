"""Edit permitted Suno downloads and licensed foley into the Reverie palette.

No original shield WAVs are read and no replacement is synthesized when a source
is missing. Suno material must come through its Download UI with recorded song
IDs, plan and download evidence. CC0 foley retains its original files and license.
Recipes name explicit source regions for reproducible and reviewable editing.

python games/dreambound/scripts/prepare_reverie_audio.py
python games/dreambound/scripts/prepare_reverie_audio.py --check
"""
from __future__ import annotations

import argparse
import hashlib
import io
import json
from pathlib import Path
import subprocess
import wave

import numpy as np
import scipy
from scipy import signal

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets" / "audio-reverie"
RATE = 48000


def db(value):
    return float(20 * np.log10(max(float(value), 1e-12)))


def fade(x, attack=0.0015, release=0.025):
    x = x.copy()
    a = min(round(attack * RATE), len(x) // 2)
    r = min(round(release * RATE), len(x) // 2)
    if a > 1:
        x[:a] *= np.linspace(0, 1, a)[:, None] ** 0.5
    if r > 1:
        x[-r:] *= np.linspace(1, 0, r)[:, None] ** 1.5
    return x


def read_audio(path):
    """Decode the already permitted local download, never a playback URL."""
    data = subprocess.run([
        "ffmpeg", "-v", "error", "-i", str(path), "-f", "f32le", "-ac", "2",
        "-ar", str(RATE), "pipe:1"
    ], check=True, capture_output=True).stdout
    return np.frombuffer(data, dtype="<f4").astype(np.float64).reshape(-1, 2)


def filter_audio(x, low=35, high=15500):
    if low:
        x = signal.sosfilt(signal.butter(2, low, "highpass", fs=RATE, output="sos"), x, axis=0)
    if high:
        x = signal.sosfilt(signal.butter(2, high, "lowpass", fs=RATE, output="sos"), x, axis=0)
    return x


def extract(source, recipe):
    start, end = recipe["region"]
    a, b = round(start * RATE), round(end * RATE)
    if a < 0 or b <= a or b > len(source) + 1:
        raise ValueError(f"Source region outside download: {recipe}")
    x = source[a:b].copy()
    if "source_peak_dbfs" in recipe:
        x *= 10 ** (recipe["source_peak_dbfs"] / 20) / max(np.max(np.abs(x)), 1e-12)
    if recipe.get("reverse"):
        x = x[::-1].copy()
    speed = recipe.get("speed", 1.0)
    if speed != 1:
        old = np.arange(len(x))
        new = np.arange(0, len(x) - 1, speed)
        x = np.stack([np.interp(new, old, x[:, c]) for c in range(2)], axis=1)
    x = filter_audio(x, recipe.get("highpass", 35), recipe.get("lowpass", 15500))
    if recipe.get("saturation", 0):
        amount = recipe["saturation"]
        x = np.tanh(x * amount) / amount
    x = fade(x, recipe.get("attack", .0015), recipe.get("release", .025))
    return x * 10 ** (recipe.get("gain_db", 0) / 20)


def make_loop(x, seconds, crossfade_seconds=.25):
    """Overlap a source's edges, retaining motion without fading to silence."""
    length = round(seconds * RATE)
    cross = round(crossfade_seconds * RATE)
    if len(x) < length + cross:
        raise ValueError("Loop source must include length plus its crossfade region")
    x = x[:length + cross].copy()
    weight = np.linspace(0, 1, cross)[:, None]
    # Boundary is now inside one continuous source. Linear overlap avoids the
    # possible 3 dB lift of equal-power fading on correlated drone material.
    overlap = x[length:] * (1 - weight) + x[:cross] * weight
    return np.concatenate([overlap, x[cross:length]])


def pcm_bytes(x):
    pcm = np.rint(np.clip(x, -1, 1) * 32767).astype("<i2")
    f = io.BytesIO()
    with wave.open(f, "wb") as w:
        w.setnchannels(x.shape[1])
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(pcm.tobytes())
    return f.getvalue(), pcm


def authored_level(x, cue):
    """Set independent loudness and crest, retaining a rounded impact edge.

    Source foley often has a very large isolated peak and quiet body. Matching
    only peaks made the first edits too thin. A bounded fourth-order soft knee
    lifts the actual body without letting the isolated transient set all gain.
    Quiet movement cues keep their natural crest instead of being forced loud.
    """
    ceiling = 10 ** (cue["peak_dbfs"] / 20)
    x = x * ceiling / max(np.max(np.abs(x)), 1e-12)
    if "rms_dbfs" not in cue:
        return x
    target = 10 ** (cue["rms_dbfs"] / 20)
    if np.sqrt(np.mean(x*x)) >= target:
        return x
    def compress(drive):
        driven = x * drive
        y = driven / (1 + (np.abs(driven) / ceiling) ** 4) ** .25
        return y * ceiling / max(np.max(np.abs(y)), 1e-12)
    lo, hi = 1., 32.
    for _ in range(22):
        mid = (lo + hi) / 2
        if np.sqrt(np.mean(compress(mid) ** 2)) < target:
            lo = mid
        else:
            hi = mid
    return compress(hi)


def measurements(x, pcm, loop):
    peak = np.max(np.abs(x))
    result = {
        "seconds": len(x) / RATE,
        "channels": x.shape[1], "sample_rate": RATE, "pcm_bits": 16,
        "peak_dbfs": db(peak),
        "rms_dbfs": db(np.sqrt(np.mean(x*x))),
        "estimated_true_peak_dbtp": db(np.max(np.abs(signal.resample_poly(x, 4, 1, axis=0)))),
        "full_scale_samples": int(np.sum(np.abs(pcm.astype(np.int32)) >= 32767)),
        "dc_offset": float(np.max(np.abs(np.mean(x, axis=0)))),
    }
    spectrum = np.abs(np.fft.rfft(x[:, 0])) ** 2
    frequencies = np.fft.rfftfreq(len(x), 1 / RATE)
    result["energy_above_8khz_fraction"] = float(spectrum[frequencies > 8000].sum() / max(spectrum.sum(), 1e-12))
    if loop:
        result["boundary_step"] = float(np.max(np.abs(x[-1] - x[0])))
        result["internal_step_99_9_percentile"] = float(np.quantile(np.abs(np.diff(x, axis=0)), .999))
        windows = x[:len(x)//2400*2400].reshape(-1, 2400, x.shape[1])
        result["quietest_50ms_rms_dbfs"] = db(np.min(np.sqrt(np.mean(windows*windows, axis=(1, 2)))))
    else:
        result["edge_sample_peak"] = float(np.max(np.abs(x[[0, -1]])))
    return result


def overlap_checks(rendered):
    specs = {
        "full_release_close_contact": [
            ("S_FullRelease", 0, .66, 1), ("S_HeavyImpact", .028, .64, 1),
            ("S_EnemyHit", .03, .92, .82), ("S_EnemyDefeat", .095, .95, 1),
            ("S_Dash", .02, .75, 1)],
        "defense_under_enemy_fire": [
            ("S_Parry", 0, .9, 1), ("S_EnemyFire", .01, .78, .82),
            ("S_EnemyTell", .08, .8, 1.08), ("S_WaterLoop", 0, .55, 1)],
        "six_catches_conservative": [("S_Catch", i * .055, .66, 1) for i in range(6)],
        "pressure_with_boss_tell": [
            ("S_BossTell", 0, .6, .7), ("S_EnemyFire", .2, .78, .82),
            ("S_EnemyFire", .4, .78, 1), ("S_Guard", .4, .85, .92),
            ("S_Attack", .6, .72, 1), ("S_Impact", .65, .48, .92),
            ("S_EnemyHit", .65, .92, .82)],
    }
    fixtures = {}
    for title, events in specs.items():
        mix = np.zeros(RATE * 4)
        for name, delay, gain, pitch in events:
            if name not in rendered:
                continue
            x = rendered[name][0].mean(axis=1)
            x = np.interp(np.arange(0, len(x) - 1, pitch), np.arange(len(x)), x) * gain
            start = round(delay * RATE)
            n = min(len(x), len(mix) - start)
            mix[start:start+n] += x[:n]
        peak = np.max(np.abs(signal.resample_poly(mix, 4, 1)))
        if peak >= 1:
            raise ValueError(f"Overlapping audio clips in {title}")
        fixtures[title] = {
            "events": [dict(cue=n, time=t, gain=g, pitch=p) for n,t,g,p in events],
            "sample_peak_dbfs": db(np.max(np.abs(mix))),
            "estimated_true_peak_dbtp": db(peak),
            "full_scale_samples": int(np.count_nonzero(np.abs(mix) >= 1)),
        }
    return {
        "note": "Fixed dry mono sums with source gains, without attenuation; not an exhaustive engine mix. Six catches retain all tails beyond the intended two-voice cap.",
        "fixtures": fixtures,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Regenerate in memory and compare existing bytes")
    args = parser.parse_args()
    provenance = json.loads((OUT / "sources.json").read_text(encoding="utf-8"))
    recipe_book = json.loads((OUT / "recipes.json").read_text(encoding="utf-8"))
    sources = {}
    source_records = {}
    for name, entry in provenance["sources"].items():
        if entry.get("kind") == "cc0_sample":
            if entry.get("status") != "verified_cc0" or entry.get("license") != "CC0 1.0":
                raise ValueError(f"{name}: unverified sample rights")
            if not all(entry.get(key) for key in ("source_url", "creator", "license_proof", "download_evidence")):
                raise ValueError(f"{name}: missing sample provenance")
            if not (OUT / entry["license_proof"]).is_file():
                raise ValueError(f"{name}: missing retained license evidence")
        else:
            if entry.get("status") != "permitted_download":
                raise ValueError(f"{name}: source is not a verified permitted download")
            if not all(entry.get(key) for key in ("song_id", "downloaded_at", "plan", "download_evidence")):
                raise ValueError(f"{name}: missing generation/download provenance")
        path = OUT / entry["file"]
        if not path.resolve().is_relative_to(OUT.resolve()):
            raise ValueError("Source path must stay inside the audio package")
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        if entry.get("sha256") and digest != entry["sha256"]:
            raise ValueError(f"{name}: original download hash differs")
        sources[name] = read_audio(path)
        source_records[name] = {**entry, "sha256": digest}
    if not sources:
        raise ValueError("No permitted source downloads yet; no substitute audio was generated")

    results, rows, rendered = {}, [], {}
    for name, cue in recipe_book["cues"].items():
        x = np.zeros((round(cue["seconds"] * RATE), 2))
        for layer in cue["layers"]:
            audio = extract(sources[layer["source"]], layer)
            delay = round(layer.get("delay", 0) * RATE)
            n = min(len(audio), len(x) - delay)
            if n <= 0:
                raise ValueError(f"{name}: inaudible layer")
            x[delay:delay+n] += audio[:n]
        if cue.get("loop"):
            x = make_loop(x, cue["loop_seconds"], cue.get("crossfade_seconds", .25))
        else:
            x = fade(x, .001, cue.get("release", .035))
        if cue.get("channels", 1) == 1:
            x = np.mean(x, axis=1, keepdims=True)
        peak = np.max(np.abs(x))
        if peak < 1e-6:
            raise ValueError(f"{name}: silent or missing source layer")
        # Independent authored peak ceilings preserve sound hierarchy. Keep all
        # engine import normalization off; runtime gains are recorded per cue.
        x = authored_level(x, cue)
        body, pcm = pcm_bytes(x)
        metrics = measurements(x, pcm, cue.get("loop", False))
        if metrics["full_scale_samples"] or metrics["estimated_true_peak_dbtp"] > -.2:
            raise ValueError(f"{name}: clipping or inadequate true-peak headroom")
        path = OUT / f"{name}.wav"
        if args.check:
            if not path.exists() or path.read_bytes() != body:
                raise ValueError(f"{name}: output differs from recorded recipe")
        else:
            path.write_bytes(body)
        rendered[name] = (x, cue)
        used = sorted({layer["source"] for layer in cue["layers"]})
        used_records = [source_records[s] for s in used]
        used_suno = any(r.get("kind") != "cc0_sample" for r in used_records)
        used_cc0 = any(r.get("kind") == "cc0_sample" for r in used_records)
        licenses = []
        creators = sorted({r.get("creator", "Klaus via Suno Sounds") for r in used_records})
        urls = sorted({r.get("source_url") or f"https://suno.com/song/{r['song_id']}" for r in used_records})
        if used_suno:
            licenses.append("Suno permitted paid-tier downloads; terms effective 2026-09-03")
        if used_cc0:
            licenses.append("CC0 1.0 source samples")
        results[name] = {**metrics, "sha256": hashlib.sha256(body).hexdigest(),
                         "sources": used, "runtime_gain": cue["runtime_gain"],
                         "recipe": cue}
        rows.append({
            "asset_id": f"reverie_{name.lower()}",
            "path": f"assets/audio-reverie/{name}.wav",
            "source_url": ";".join(urls),
            "creator": "; ".join(creators) + "; Game Studio audio editing",
            "license": "; ".join(licenses),
            "proof_path": "assets/audio-reverie/audio-report.json",
            "modifications": cue["description"] + "; source-region editing, resampling, EQ, layering and authored levels; 48 kHz PCM16",
            "approval_status": "generated-permitted-download" if used_suno else "licensed-cc0-edited",
        })
    report = {
        "revision": recipe_book["revision"], "numpy": np.__version__, "scipy": scipy.__version__,
        "rights_sources": provenance["rights_sources"], "sources": source_records,
        "cues": results, "manifest_rows_for_integration_owner": rows,
        "audition": provenance.get("audition", {"heard_by_agent": False, "in_game_tested": False}),
        "overlap_checks": overlap_checks(rendered),
    }
    if not args.check:
        (OUT / "audio-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        # A short cue reel is a review artifact, never imported as a game sound.
        reel, reel_index, cursor = [], [], 0.
        for name, (x, cue) in rendered.items():
            y = np.repeat(x, 2, axis=1) if x.shape[1] == 1 else x
            sample = y[:RATE*3] * cue["runtime_gain"]
            reel_index.append({"cue": name, "starts_at_seconds": round(cursor, 3), "seconds": len(sample)/RATE})
            reel += [sample, np.zeros((RATE//3, 2))]
            cursor += len(sample)/RATE + 1/3
        (OUT / "audition-reel.wav").write_bytes(pcm_bytes(np.concatenate(reel))[0])
        (OUT / "audition-index.json").write_text(json.dumps(reel_index, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"revision": report["revision"], "cues": len(results), "check_only": args.check,
                      "maximum_true_peak_dbtp": max(r["estimated_true_peak_dbtp"] for r in results.values())}, indent=2))


if __name__ == "__main__":
    main()

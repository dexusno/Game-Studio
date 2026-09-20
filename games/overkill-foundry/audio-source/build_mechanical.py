"""Original, deterministic mechanical Foley for Overkill Foundry.

No samples, recordings, third-party patches or network access. Python standard
library only. Regenerate the small PCM sources; Unreal imports derivatives.
"""
from __future__ import annotations

import argparse
from array import array
import hashlib
import io
import json
import math
from pathlib import Path
import random
import sys
import wave

RATE = 48000
VERSION = "foundry-mechanical-0.1"
ROOT = Path(__file__).resolve().parents[1]
TAU = 2 * math.pi


def silent(seconds):
    return [0.0] * round(seconds * RATE)


def strike(out, when, modes, decay, strength=1.0):
    """Inharmonic decaying partials model separate pieces of resonant metal."""
    start = round(when * RATE)
    for i in range(start, len(out)):
        t = (i - start) / RATE
        attack = min(1.0, t / 0.0015)
        out[i] += strength * attack * sum(
            amp * math.sin(TAU * hz * t) * math.exp(-t / (decay / (1 + j * 0.22)))
            for j, (hz, amp) in enumerate(modes)
        )


def sweep(out, when, duration, first, last, strength, decay=0):
    start, count = round(when * RATE), round(duration * RATE)
    phase = 0.0
    for j in range(min(count, len(out) - start)):
        u = j / count
        hz = first + (last - first) * u
        phase += TAU * hz / RATE
        shape = min(1.0, j / (RATE * 0.008)) * min(1.0, (count - j) / (RATE * 0.025))
        if decay:
            shape *= math.exp(-u * decay)
        out[start + j] += strength * shape * (math.sin(phase) + 0.15 * math.sin(phase * 2))


def air(out, rng, when, duration, cutoff, strength, decay=3, highpass=False):
    start, count = round(when * RATE), round(duration * RATE)
    alpha = 1 - math.exp(-TAU * cutoff / RATE)
    low = 0.0
    for j in range(min(count, len(out) - start)):
        noise = rng.uniform(-1, 1)
        low += alpha * (noise - low)
        x = noise - low if highpass else low
        envelope = min(1.0, j / (RATE * 0.002)) * min(1.0, (count - j) / (RATE * 0.018))
        envelope *= math.exp(-decay * j / count)
        out[start + j] += strength * x * envelope


def echoes(out, taps):
    dry = out[:]
    for seconds, gain in taps:
        shift = round(seconds * RATE)
        for i in range(shift, len(out)):
            out[i] += dry[i - shift] * gain


def make(name, seconds, seed):
    rng, out = random.Random(seed), silent(seconds)
    if name.startswith("cannon"):
        heavy = name.endswith("heavy")
        sweep(out, 0, 0.29 if heavy else 0.20, 125 if heavy else 175, 32, 1.2, 4)
        air(out, rng, 0, 0.22, 1400, 1.8, 7)
        air(out, rng, 0, 0.055, 2800, 0.45, 4, True)
        strike(out, 0.02, [(185, .34), (477, .15), (973, .09), (2173, .04)], .16)
        strike(out, .16, [(273, .19), (643, .13), (1837, .08)], .045)
        echoes(out, [(.052, .15), (.113, .09)])
    elif name.startswith("metal_hit"):
        heavy = name.endswith("heavy")
        strike(out, 0, [(207 if heavy else 487, .6), (821, .28), (1459, .15), (2783, .08)], .12 if heavy else .055)
        air(out, rng, 0, .06, 3100, .30, 7)
    elif name == "robot_break":
        sweep(out, 0, .32, 115, 28, .75, 4)
        air(out, rng, 0, .44, 1800, .9, 5)
        for j, t in enumerate([0, .08, .14, .23, .35, .46]):
            strike(out, t, [(310 + 83 * j, .28), (917 + 41 * j, .18), (2281, .06)], .06, 1 - j * .10)
        air(out, rng, .28, .6, 2700, .15, 4, True)
        echoes(out, [(.065, .12), (.16, .05)])
    elif name in ("breech_load", "breech_unload", "forge", "collect"):
        timings = {"breech_load": [0, .11, .24], "breech_unload": [0, .08], "forge": [0, .06, .15], "collect": [.06, .16, .28, .36]}[name]
        for j, t in enumerate(timings):
            strike(out, t, [(239 + j * 61, .40), (773 + j * 109, .21), (1927, .11)], .040 + j * .008)
            air(out, rng, t, .035, 1900, .22, 6)
        sweep(out, 0, min(.32, seconds - .06), 92, 170 if name != "breech_unload" else 55, .11)
        if name == "collect":
            air(out, rng, .11, .36, 700, .18, 0)
    elif name in ("shield_on", "shield_hit"):
        sweep(out, 0, .18, 250, 860 if name == "shield_on" else 90, .35, 3)
        strike(out, 0, [(397, .42), (701, .28), (1193, .20)], .09)
        air(out, rng, 0, .12, 4400, .18, 7, True)
    elif name == "player_hit":
        sweep(out, 0, .13, 90, 35, .9, 5)
        strike(out, .01, [(281, .28), (863, .13)], .045)
        air(out, rng, 0, .07, 900, .55, 5)
    elif name == "robot_move":
        sweep(out, 0, .25, 64, 129, .3)
        sweep(out, .1, .2, 398, 189, .075)
        strike(out, .24, [(223, .35), (599, .18), (1489, .08)], .055)
        air(out, rng, .03, .26, 780, .14, 0)
    elif name in ("repair", "upgrade", "victory", "defeat", "precision_good", "precision_miss"):
        notes = {"repair": [370, 493], "upgrade": [247, 370, 494], "victory": [196, 294, 392, 494], "defeat": [220, 174, 130], "precision_good": [660, 880], "precision_miss": [233, 196]}[name]
        for j, hz in enumerate(notes):
            strike(out, .09 * j, [(hz, .48), (hz * 2.006, .11), (hz * 3.21, .035)], .12)
        air(out, rng, 0, .12, 900, .05, 5)
    elif name in ("ui_ok", "ui_error", "turn"):
        strike(out, 0, [(930 if name == "ui_ok" else 271, .45), (1517, .12)], .012)
        if name == "ui_error":
            strike(out, .075, [(227, .4), (599, .08)], .019)
        if name == "turn":
            strike(out, .055, [(447, .36), (1291, .08)], .022)
    elif name == "factory_room":
        # Whole cycles over the loop, including modulation, give a continuous
        # seam. Distant resonances remain quiet beneath short action transients.
        count = len(out)
        modes = [(91, .43), (181, .15), (301, .07), (487, .04)]
        modes += [(rng.randint(650, 5200), rng.uniform(.001, .004)) for _ in range(31)]
        phases = [rng.random() * TAU for _ in modes]
        for i in range(count):
            u = i / count
            hum = sum(amp * math.sin(TAU * cycles * u + p) for (cycles, amp), p in zip(modes, phases))
            out[i] = hum * (.86 + .08 * math.sin(TAU * u) + .04 * math.sin(TAU * 3 * u))
    else:
        raise ValueError(name)
    return out


# Relative cue hierarchy is deliberate: action transients lead, menus remain
# quiet, and the room is a low bed. Gains below are printed into the audit.
SPECS = [
    ("cannon_light", .62, -7, -23, "Primary shot; industrial air impulse and breech resonance"),
    ("cannon_heavy", .82, -6, -21, "Large primary shot; lower body and longer decay"),
    ("metal_hit_light", .28, -14, -30, "Small robot impact"),
    ("metal_hit_heavy", .43, -11, -27, "Strong robot impact"),
    ("robot_break", 1.08, -9, -26, "Robot fracture and cooling debris"),
    ("breech_load", .43, -16, -32, "Three-stage breech lock"),
    ("breech_unload", .30, -18, -34, "Breech release and eject"),
    ("forge", .36, -17, -31, "Crafting die contact"),
    ("collect", .64, -16, -31, "Rear-claw servo and hopper scrap"),
    ("shield_on", .36, -17, -31, "Shield bank charging"),
    ("shield_hit", .31, -15, -29, "Shield bank taking a hit"),
    ("player_hit", .31, -12, -29, "Rig body impact, no voice"),
    ("robot_move", .49, -20, -32, "Robot hydraulic actuation"),
    ("repair", .47, -20, -32, "Two-note service indicator"),
    ("upgrade", .66, -18, -30, "Three-note machine calibration"),
    ("victory", .96, -16, -30, "Four-note completed-cycle signal"),
    ("defeat", .88, -19, -32, "Descending power-down signal"),
    ("precision_good", .42, -19, -31, "Precision timing success"),
    ("precision_miss", .40, -24, -36, "Quiet timing miss"),
    ("ui_ok", .12, -28, -41, "Single control relay"),
    ("ui_error", .24, -27, -41, "Two control relays; rejected command"),
    ("turn", .22, -23, -37, "End Turn relay"),
    ("factory_room", 6, -30, -36, "Seamless distant furnace resonance"),
]


def encode(samples, peak_db, rms_db, looping):
    dc = sum(samples) / len(samples)
    samples = [v - dc for v in samples]
    if not looping:
        fade = min(240, len(samples) // 2)
        for i in range(fade):
            samples[i] *= i / fade
            samples[-1 - i] *= i / fade
    peak = max(abs(v) for v in samples)
    rms = math.sqrt(sum(v * v for v in samples) / len(samples))
    gain = min(10 ** (peak_db / 20) / peak, 10 ** (rms_db / 20) / rms)
    pcm = array("h", (round(v * gain * 32767) for v in samples))
    quantized_peak = max(abs(v) for v in pcm) / 32767
    quantized_rms = math.sqrt(sum(v * v for v in pcm) / len(pcm)) / 32767
    mean = sum(pcm) / len(pcm) / 32767
    assert quantized_peak <= 10 ** (peak_db / 20) + 1 / 32767
    assert quantized_rms > 10 ** (-50 / 20)
    assert abs(mean) < .0005
    if not looping:
        assert pcm[0] == pcm[-1] == 0
    else:
        # Seam slope stays within the ordinary neighbouring sample variation.
        assert abs(pcm[0] - pcm[-1]) <= max(abs(pcm[i] - pcm[i - 1]) for i in range(1, len(pcm))) * 1.05 + 1
    metrics = {"peak_dbfs": round(20 * math.log10(quantized_peak), 3),
               "rms_dbfs": round(20 * math.log10(quantized_rms), 3),
               "dc": round(mean, 8), "samples": len(pcm),
               "edge_jump": abs(pcm[0] - pcm[-1]), "loop": looping}
    if sys.byteorder != "little":
        pcm.byteswap()
    dest = io.BytesIO()
    with wave.open(dest, "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(RATE)
        wav.writeframes(pcm.tobytes())
    return dest.getvalue(), metrics


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Compare reproducible PCM and audit without writing")
    parser.add_argument("--output", type=Path, default=ROOT / "assets/audio/mechanical-v001")
    args = parser.parse_args()
    if not args.check:
        args.output.mkdir(parents=True, exist_ok=True)
    report = {"version": VERSION, "rate": RATE, "channels": 1, "bit_depth": 16,
              "source": "audio-source/build_mechanical.py", "external_samples": [], "cues": []}
    for index, (name, duration, peak, rms, purpose) in enumerate(SPECS):
        pcm, metrics = encode(make(name, duration, 39271 + index), peak, rms, name == "factory_room")
        file = args.output / (name + ".wav")
        if args.check:
            if not file.is_file() or file.read_bytes() != pcm:
                raise RuntimeError("PCM does not reproduce: " + name)
        else:
            file.write_bytes(pcm)
        report["cues"].append({"name": name, "file": file.name, "duration": duration,
                              "purpose": purpose, "peak_ceiling_dbfs": peak,
                              "sha256": hashlib.sha256(pcm).hexdigest(), **metrics})
    audit = (json.dumps(report, indent=2) + "\n").encode()
    audit_path = args.output / "audio-audit.json"
    if args.check:
        if audit_path.read_bytes() != audit:
            raise RuntimeError("Audio audit does not reproduce")
    else:
        audit_path.write_bytes(audit)
    print(f"PASS {len(SPECS)} original PCM cues: deterministic bytes, peak headroom, non-silence, DC and edges; runtime listening remains separate")


if __name__ == "__main__":
    main()

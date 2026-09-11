"""Generate Magnet Sweep's original concept-demo effects, without recorded samples.

Run from the repository root: python games/magnet-sweep/scripts/GenerateAudio.py
Requires NumPy and SciPy. Output is deterministic 48 kHz mono PCM16 WAV.
All synthesis recipes were authored for this game; no third-party audio is used.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import wave

import numpy as np
from scipy.signal import butter, sosfilt


RATE = 48_000
GAME = Path(__file__).resolve().parents[1]
DESTINATION = GAME / "assets" / "audio"


def clock(seconds: float) -> np.ndarray:
    return np.arange(round(RATE * seconds), dtype=np.float64) / RATE


def band_noise(rng: np.random.Generator, duration: float,
               low: float, high: float) -> np.ndarray:
    noise = rng.normal(size=round(RATE * duration))
    shape = butter(2, [low, high], btype="bandpass", fs=RATE, output="sos")
    return sosfilt(shape, noise)


def metal(duration: float, base: float, decay: float,
          strength: float = 1.0) -> np.ndarray:
    """An inharmonic, damped little metal object; no sampled impulse."""
    t = clock(duration)
    result = np.zeros_like(t)
    for ratio, weight, damping in [(1.0, .60, 1.0), (1.47, .28, 1.3),
                                   (2.09, .17, 1.7), (2.71, .075, 2.1)]:
        result += (weight * np.sin(2 * np.pi * base * ratio * t)
                   * np.exp(-t * damping / decay))
    attack = 1.0 - np.exp(-t / .0012)
    return strength * result * attack


def add_at(target: np.ndarray, event: np.ndarray, seconds: float,
           gain: float = 1.0) -> None:
    start = round(seconds * RATE)
    count = min(len(event), len(target) - start)
    if count > 0:
        target[start:start + count] += event[:count] * gain


def pickup(variant: int) -> np.ndarray:
    rng = np.random.default_rng(7100 + variant)
    t = clock(.185)
    base = [910, 1030, 1145][variant]
    body = metal(.185, base, .038)
    tick = band_noise(rng, .185, 650, 4200) * np.exp(-t / .009) * .23
    lower = np.sin(2 * np.pi * base * .46 * t) * np.exp(-t / .017) * .16
    return body + tick + lower


def latch() -> np.ndarray:
    rng = np.random.default_rng(7200)
    t = clock(.255)
    x = metal(.255, 510, .053, .67)
    add_at(x, metal(.16, 735, .037), .042, .46)
    x += band_noise(rng, .255, 300, 2700) * np.exp(-t / .012) * .19
    return x


def tug() -> np.ndarray:
    rng = np.random.default_rng(7300)
    t = clock(.43)
    # Air and a subtle descending resonant tension lead into the release.
    swell = np.sin(np.pi * np.clip(t / .19, 0, 1)) ** 1.6
    x = band_noise(rng, .43, 180, 2400) * swell * .60
    x += np.sin(2 * np.pi * (350 * t - 300 * t * t)) * swell * .08
    add_at(x, metal(.275, 480, .051), .145, .80)
    add_at(x, metal(.22, 840, .034), .161, .29)
    return x


def linked_release() -> np.ndarray:
    rng = np.random.default_rng(7400)
    t = clock(.57)
    x = np.sin(2 * np.pi * (150 * t - 38 * t * t))
    x *= (1 - np.exp(-t / .004)) * np.exp(-t / .083) * .68
    x += band_noise(rng, .57, 170, 2400) * np.exp(-t / .032) * .40
    for onset, frequency, strength in [(.006, 385, .8), (.042, 590, .53),
                                       (.091, 720, .42), (.151, 960, .27)]:
        add_at(x, metal(.28, frequency, .058), onset, strength)
    return x


def deposit() -> np.ndarray:
    rng = np.random.default_rng(7500)
    t = clock(.97)
    x = np.zeros_like(t)
    # A short dense cascade, larger pieces first, smaller chinks trailing off.
    times = np.linspace(.025, .665, 32) + rng.uniform(-.011, .011, 32)
    for index, onset in enumerate(times):
        frequency = rng.uniform(520, 1420) + index * 8
        strength = (.52 + .26 * math.sin(math.pi * index / 32))
        strength *= rng.uniform(.65, 1.0)
        add_at(x, metal(.21, frequency, rng.uniform(.019, .040)),
               float(onset), strength)
    envelope = np.sin(np.pi * np.clip(t / .78, 0, 1)) ** 1.8
    x += band_noise(rng, .97, 120, 1400) * envelope * .43
    x += np.sin(2 * np.pi * 132 * t) * envelope * .12
    add_at(x, metal(.23, 275, .065), .69, .45)
    return x


def forge() -> np.ndarray:
    rng = np.random.default_rng(7600)
    t = clock(1.16)
    swell = np.sin(np.pi * np.clip(t / .54, 0, 1)) ** 1.8
    x = band_noise(rng, 1.16, 100, 1700) * swell * .36
    x += np.sin(2 * np.pi * (140 * t + 230 * t * t)) * swell * .15
    # A gentle open fifth resolves the furnace action without a music system.
    for onset, frequency, strength in [(.31, 392, .70), (.355, 588, .42),
                                       (.40, 784, .20)]:
        tt = clock(1.16 - onset)
        envelope = (1 - np.exp(-tt / .009)) * np.exp(-tt / .21)
        tone = np.sin(2 * np.pi * frequency * tt) * envelope
        tone += .10 * np.sin(2 * np.pi * frequency * 2 * tt) * envelope
        add_at(x, tone, onset, strength)
    return x


def ui_click() -> np.ndarray:
    rng = np.random.default_rng(7700)
    t = clock(.09)
    x = np.sin(2 * np.pi * (620 * t - 600 * t * t)) * np.exp(-t / .012)
    x += band_noise(rng, .09, 300, 1800) * np.exp(-t / .005) * .18
    return x


SPECS = [
    ("pickup_01", lambda: pickup(0), -15.0, .55),
    ("pickup_02", lambda: pickup(1), -15.0, .55),
    ("pickup_03", lambda: pickup(2), -15.0, .55),
    ("latch", latch, -13.0, .70),
    ("tug", tug, -11.0, .78),
    ("linked_release", linked_release, -10.0, .82),
    ("deposit", deposit, -10.0, .85),
    ("forge", forge, -10.0, .80),
    ("ui_click", ui_click, -20.0, .65),
]


def prepare(x: np.ndarray, peak_db: float) -> np.ndarray:
    x = sosfilt(butter(3, 5000, btype="lowpass", fs=RATE, output="sos"), x)
    x = sosfilt(butter(2, 65, btype="highpass", fs=RATE, output="sos"), x)
    x = np.tanh(x * 1.12)
    # Cosine fades prevent discontinuities. End silence is deliberately tiny.
    fade_in = min(round(.0015 * RATE), len(x))
    fade_out = min(round(.025 * RATE), len(x))
    x[:fade_in] *= np.sin(np.linspace(0, np.pi / 2, fade_in)) ** 2
    x[-fade_out:] *= np.cos(np.linspace(0, np.pi / 2, fade_out)) ** 2
    x *= 10 ** (peak_db / 20) / max(float(np.max(np.abs(x))), 1e-12)
    x[0] = x[-1] = 0
    return np.round(x * 32767).astype("<i2")


def analyze(pcm: np.ndarray) -> dict:
    x = pcm.astype(np.float64) / 32768
    magnitude = np.abs(x)
    spectrum = np.abs(np.fft.rfft(x)) ** 2
    frequencies = np.fft.rfftfreq(len(x), 1 / RATE)
    rms = float(np.sqrt(np.mean(x * x)))
    peak = float(np.max(magnitude))
    return {
        "duration_seconds": round(len(pcm) / RATE, 6),
        "sample_rate": RATE, "channels": 1, "bits_per_sample": 16,
        "peak_dbfs": round(20 * math.log10(max(peak, 1e-12)), 3),
        "rms_dbfs": round(20 * math.log10(max(rms, 1e-12)), 3),
        "dc_mean": round(float(np.mean(x)), 9),
        "clipped_samples": int(np.sum(magnitude >= 1)),
        "first_sample": int(pcm[0]), "last_sample": int(pcm[-1]),
        "energy_above_8khz_percent": round(float(
            100 * spectrum[frequencies >= 8000].sum() / spectrum.sum()), 6),
    }


def render() -> None:
    DESTINATION.mkdir(parents=True, exist_ok=True)
    report = {"generator": "games/magnet-sweep/scripts/GenerateAudio.py",
              "method": "Original deterministic modal/noise synthesis; no recordings",
              "verification": "Numerical checks; see PROVENANCE.md for listening boundary",
              "assets": []}
    waves = {}
    for name, generator, peak, gain in SPECS:
        pcm = prepare(generator(), peak)
        path = DESTINATION / f"{name}.wav"
        with wave.open(str(path), "wb") as stream:
            stream.setparams((1, 2, RATE, len(pcm), "NONE", "not compressed"))
            stream.writeframes(pcm.tobytes())
        # Verify the actual written artifact, not just the generator's array.
        with wave.open(str(path), "rb") as stream:
            assert (stream.getnchannels(), stream.getsampwidth(), stream.getframerate()) == (1, 2, RATE)
            decoded = np.frombuffer(stream.readframes(stream.getnframes()), dtype="<i2")
        detail = analyze(decoded)
        assert detail["clipped_samples"] == 0
        assert detail["first_sample"] == detail["last_sample"] == 0
        assert abs(detail["dc_mean"]) < .0003
        assert detail["energy_above_8khz_percent"] < .25
        detail.update(name=path.name, playback_gain=gain,
                      sha256=hashlib.sha256(path.read_bytes()).hexdigest())
        report["assets"].append(detail)
        waves[name] = decoded.astype(np.float64) / 32768

    # Proposed constrained worst-case overlap. Distinct events may share a frame;
    # pickup throttling and suppression during bursts remain integration duties.
    stress = np.zeros(RATE * 2)
    for name, _, _, gain in SPECS:
        if name != "pickup_03":
            add_at(stress, waves[name], 0, gain)
    maximum = float(np.max(np.abs(stress)))
    report["recommended_gain_overlap_test"] = {
        "scenario": "2 pickup variants + latch + tug + linked release + deposit + forge + UI, aligned starts",
        "peak_dbfs": round(20 * math.log10(maximum), 3),
        "clipped_samples": int(np.sum(np.abs(stress) >= 1)),
        "limitation": "One analytical alignment, not an exhaustive audio-engine mix or auditory test",
    }
    assert maximum < 1
    (DESTINATION / "analysis.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")

    with (DESTINATION / "manifest-rows.csv").open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output)
        writer.writerow(["asset_id", "path", "source_url", "creator", "license",
                         "proof_path", "modifications", "approval_status"])
        for name, _, _, _ in SPECS:
            writer.writerow([f"magnet_audio_{name}", f"assets/audio/{name}.wav",
                "Original procedural synthesis; scripts/GenerateAudio.py",
                "Game Studio / Codex audio-designer",
                "Original project asset; no third-party sample or music license",
                "assets/audio/PROVENANCE.md",
                "Deterministic synthesis; filtering; fades; controlled peaks; PCM16 export",
                "original_no_third_party_dependencies"])
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    argparse.ArgumentParser(description=__doc__).parse_args()
    render()

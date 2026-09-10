"""Build a short preview from the exact edited gameplay WAVs, without changing them."""
from pathlib import Path
import hashlib
import importlib.util
import json
import wave

import numpy as np

HERE = Path(__file__).resolve().parent
GAME = HERE.parents[1]
spec = importlib.util.spec_from_file_location("reverie_audio", GAME / "scripts/prepare_reverie_audio.py")
audio = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audio)
RATE = 48000


def read_cue(name):
    path = GAME / "assets/audio-reverie" / f"{name}.wav"
    with wave.open(str(path), "rb") as wav:
        assert (wav.getnchannels(), wav.getsampwidth(), wav.getframerate()) == (1, 2, RATE)
        x = np.frombuffer(wav.readframes(wav.getnframes()), dtype="<i2").astype(float) / 32767
    return x, hashlib.sha256(path.read_bytes()).hexdigest()


motor, motor_hash = read_cue("S_ChargeLoop")
release, release_hash = read_cue("S_FullRelease")
preview = np.zeros((round(11.05 * RATE), 1))

# Three literal loop cycles expose two joins. Only the whole preview segment fades.
steady = audio.fade(np.tile(motor, 3)[:, None] * .48, .020, .030)
preview[:len(steady)] += steady
start = round(6.35 * RATE)
preview[start:start + len(release), 0] += release * .66

# Isolated approximation of the two runtime cues: no count/ready/other sound layers.
count = round(1.8 * RATE)
progress = np.linspace(0, 1, count)
pitch = .7 + .9 * progress
position = np.concatenate([[0.], np.cumsum(pitch[:-1])]) % len(motor)
charged = np.interp(position, np.arange(len(motor) + 1), np.append(motor, motor[0]))
charged *= .34 + .14 * progress
charged = audio.fade(charged[:, None], .020, .030)
start = round(8.1 * RATE)
preview[start:start + count] += charged
preview[start + count:start + count + len(release), 0] += release * .66

body, pcm = audio.pcm_bytes(preview)
path = HERE / "two-cue-preview.wav"
path.write_bytes(body)
index = {
    "revision": "reverie-suno-v1",
    "description": "Exact gameplay PCM at runtime cue gains; three charge-loop cycles, isolated release, then an approximated charge/release pitch sequence. No other gameplay cues or master-volume multiplier.",
    "source_wav_sha256": {"S_ChargeLoop": motor_hash, "S_FullRelease": release_hash},
    "segments": [
        {"start": 0, "end": 6, "cue": "S_ChargeLoop", "gain": .48, "pitch": 1, "loop_joins": [2, 4]},
        {"start": 6.35, "end": 7.5, "cue": "S_FullRelease", "gain": .66, "pitch": 1},
        {"start": 8.1, "end": 9.9, "cue": "S_ChargeLoop", "gain": [.34, .48], "pitch": [.7, 1.6]},
        {"start": 9.9, "end": 11.05, "cue": "S_FullRelease", "gain": .66, "pitch": 1},
    ],
    "sha256": hashlib.sha256(body).hexdigest(),
    "measurements": audio.measurements(preview, pcm, False),
    "heard_by_agent": False,
    "note": "Audio input is unavailable in this worker runtime. This file is for owner/root playback, not proof of subjective quality or an engine recording."
}
(HERE / "two-cue-preview.json").write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
print(json.dumps({"preview": str(path), "seconds": 11.05, "estimated_true_peak_dbtp": index["measurements"]["estimated_true_peak_dbtp"]}))

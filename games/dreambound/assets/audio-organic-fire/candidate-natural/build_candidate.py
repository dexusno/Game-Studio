"""Minimal-edit comparison control; never replaces or imports the rejected cues.

Playback rate 1.0, source transients intact, no EQ, compressor, saturator, breath
layer, synthesizer or loop. Only excerpts, event gain and short edge fades are
used. The source pack's own production is unknown; 'minimal edit' describes our
work, not a claim that these supplied sounds are untouched field recordings.
"""
from pathlib import Path
import csv
import hashlib
import io
import json
import shutil
import subprocess
import wave

import numpy as np

OUT = Path(__file__).resolve().parent
BASE = OUT.parent
RATE = 48000


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def decode(path):
    data = subprocess.check_output([shutil.which("ffmpeg"), "-v", "error", "-i", str(path),
        "-ar", str(RATE), "-ac", "1", "-f", "f64le", "pipe:1"])
    return np.frombuffer(data,dtype="<f8").copy()


def main():
    baseline = json.loads((BASE/"audio-report.json").read_text(encoding="utf-8"))
    expected = {name+".wav":cue["sha256"] for name,cue in baseline["cues"].items()}
    expected["organic-fire-preview.wav"] = "e5b268c190eb10fe9bb22c477d5b7914b78073fbec207b85d7d0566c46639129"
    for name,identity in expected.items():
        if sha(BASE/name) != identity: raise ValueError(f"Rejected baseline changed: {name}")
    source_record = json.loads((BASE/"sources.json").read_text(encoding="utf-8"))
    sources = {}
    for key in ["fire_staff_24","fire_staff_17","fire_staff_19"]:
        record=source_record["sources"][key]
        if sha(BASE/record["file"])!=record["sha256"]: raise ValueError("Source identity mismatch")
        sources[key]=decode(BASE/record["file"])
    # Full natural-rate attack envelopes retained. One simple burn bed locates
    # the gather; the two distinct supplied pressure events are the throws.
    events = [
        dict(source="fire_staff_24",region_seconds=[.3,2.155],time_seconds=.25,
             gain=.60,fade_in_seconds=.07,fade_out_seconds=.035,label="quiet burn throughout gather and second-shot preparation"),
        dict(source="fire_staff_17",region_seconds=[0,2.2],time_seconds=1.63,
             gain=.52,fade_in_seconds=.0025,fade_out_seconds=.020,label="first supplied fire event, complete retained transient/tail"),
        dict(source="fire_staff_19",region_seconds=[0,2.15],time_seconds=2.07,
             gain=.52,fade_in_seconds=.0025,fade_out_seconds=.020,label="second supplied fire event, complete retained transient/tail"),
    ]
    mix=np.zeros(round(4.8*RATE))
    for event in events:
        a,b=[round(t*RATE) for t in event["region_seconds"]]
        x=sources[event["source"]][a:b].copy()
        attack=round(event["fade_in_seconds"]*RATE)
        release=round(event["fade_out_seconds"]*RATE)
        x[:attack]*=np.linspace(0,1,attack)
        x[-release:]*=np.linspace(1,0,release)
        x*=event["gain"]
        start=round(event["time_seconds"]*RATE)
        mix[start:start+len(x)]+=x
    # One fixed gain matches the old preview peak, preventing a louder result
    # from being mistaken for a better texture. It is not an RMS/loudness match.
    target=10**(baseline["preview"]["measurements"]["sample_peak_dbfs"]/20)
    global_gain=target/np.max(np.abs(mix))
    mix*=global_gain
    pcm=np.rint(mix*32767).astype("<i2")
    path=OUT/"natural-source-control.wav"
    with wave.open(str(path),"wb") as f:
        f.setnchannels(1);f.setsampwidth(2);f.setframerate(RATE);f.writeframes(pcm.tobytes())
    report=dict(status="unaccepted comparison control",owner_verdict_on_baseline="Still sounds artificial",
        baseline_preview_sha256=expected["organic-fire-preview.wav"],baseline_wavs_preserved=expected,
        candidate_file=path.name,sha256=sha(path),seconds=len(mix)/RATE,
        format="mono PCM16 48 kHz",events=events,global_constant_gain=global_gain,
        source_records={key:source_record["sources"][key] for key in sources},
        processing_removed=["already slowed ghost breath","human tired breath","all creative pitch/speed changes",
            "all high/low-pass EQ","RMS body matching","soft-knee level shaper","loop repetition",
            "release breath/ignition overlays","overlaid flight/impact events"],
        sample_peak_dbfs=float(20*np.log10(np.max(abs(pcm.astype(float)/32768)))),
        full_scale_samples=int(np.count_nonzero(abs(pcm.astype(float))>=32767)),
        silence_after_seconds=4.22,listened=False,
        limitation="Audio input is unavailable. Source identities, signal construction and decode are verified; perceived naturalness is unverified. This intentionally sparse control does not provide the full runtime palette or prove the organic monster-fire goal.")
    (OUT/"candidate-report.json").write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    text=io.StringIO(newline="")
    fields=["asset_id","path","source_url","creator","license","proof_path","modifications","approval_status"]
    writer=csv.DictWriter(text,fieldnames=fields,lineterminator="\n");writer.writeheader()
    writer.writerow(dict(asset_id="organic_fire_natural_source_control",
        path="assets/audio-organic-fire/candidate-natural/natural-source-control.wav",
        source_url="https://opengameart.org/content/fire-staff-sound-effects",creator="LEGIT Audio; Game Studio editing",
        license="CC0 1.0 sources; project-authored edit",proof_path="assets/audio-organic-fire/candidate-natural/candidate-report.json",
        modifications="Three retained source excerpts at natural playback rate; gain and short edge fades only; offline unaccepted comparison, not a runtime cue.",
        approval_status="verified_cc0"))
    (OUT/"manifest-row.csv").write_text(text.getvalue(),encoding="utf-8")
    for name,identity in expected.items():
        if sha(BASE/name)!=identity: raise ValueError(f"Rejected baseline changed during candidate render: {name}")
    subprocess.run([shutil.which("ffmpeg"),"-v","error","-i",str(path),"-f","null","-"],check=True,capture_output=True)
    print(json.dumps(dict(candidate=str(path),seconds=report["seconds"],sha256=report["sha256"],
        baseline_preserved=True,decode_errors=0,listened=False),indent=2))


if __name__=="__main__":main()

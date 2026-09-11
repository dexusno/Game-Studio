"""Render the caster's organic combustion palette from retained CC0 recordings.

Run normally to regenerate the eight mono PCM16/48 kHz cues and review sequence.
Use --check to decode sources, reproduce bytes and inspect the delivered files
without rewriting them. No network access, engine import, or manifest mutation.
The source/rights record is assets/audio-organic-fire/sources.json.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import io
import json
from fractions import Fraction
from pathlib import Path
import shutil
import subprocess
import wave

import numpy as np
import scipy
from scipy import signal

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets/audio-organic-fire"
RATE = 48000
REVISION = "organic-fire-v1"


def layer(source, region, speed=1., gain=0., low=40, high=8000,
          offset=0., attack=.008, release=.08, reverse=False):
    return dict(source=source, region=region, speed=speed, gain_db=gain,
                highpass_hz=low, lowpass_hz=high, offset_seconds=offset,
                attack_seconds=attack, release_seconds=release, reverse=reverse)


RECIPES = {
    "S_CasterIgnite": dict(seconds=.64, peak_dbfs=-11, rms_dbfs=-24,
        runtime_gain=.70, voice_cap=3, owner_cap=1, priority=78, looping=False,
        trigger="Enter caster gather; chest-attached component, cancellable.",
        description="Short inward breath opens into a muffled ignition inside the chest.",
        layers=[
            layer("ghost_breath", [.05,.80], speed=1.3, high=1350, gain=0, attack=.07, release=.12),
            layer("ignition", [.16,.58], speed=.83, low=180, high=4700, gain=-9, offset=.12, attack=.09, release=.09),
            layer("breath", [.17,.40], speed=.75, high=950, gain=-8, attack=.015, release=.10)]),
    "S_CasterFurnaceLoop": dict(seconds=1.60, crossfade_seconds=.24,
        peak_dbfs=-11, rms_dbfs=-23, runtime_gain=.68, voice_cap=3, owner_cap=1,
        priority=80, looping=True,
        trigger="Attach to chest throughout gather and second-shot regather; gain follows current charge.",
        description="Irregular body breath under turbulent combustion; no pitch oscillator or bell.",
        layers=[
            layer("fire_staff_24", [.20,3.50], speed=.83, low=42, high=2350, gain=0),
            layer("ghost_breath", [.60,2.85], speed=.91, low=65, high=1600, gain=-5)]),
    "S_CasterReleaseA": dict(seconds=1.12, peak_dbfs=-8, rms_dbfs=-20.5,
        runtime_gain=.82, voice_cap=4, owner_cap=2, priority=88, looping=False,
        trigger="Actual spawned fireball, at emitting hand/mouth; rotate variants without immediate repeat.",
        description="A bodily expulsion with a leading fire pressure pulse and a broken flame tail.",
        layers=[
            layer("fire_staff_01", [0,.93], speed=.88, low=48, high=7600, gain=0, attack=.005, release=.20),
            layer("breath", [.17,.45], speed=.73, low=85, high=2600, gain=-5, attack=.010, release=.13),
            layer("ignition", [.54,.90], speed=.65, low=330, high=5100, gain=-15, offset=.18, attack=.018, release=.19)]),
    "S_CasterReleaseB": dict(seconds=1.08, peak_dbfs=-8, rms_dbfs=-20.5,
        runtime_gain=.82, voice_cap=4, owner_cap=2, priority=88, looping=False,
        trigger="Same event/concurrency group as ReleaseA.",
        description="Alternate forceful fire expulsion with a shorter, rougher breath lead.",
        layers=[
            layer("fire_staff_03", [.02,.97], speed=.94, low=48, high=7200, gain=0, attack=.004, release=.19),
            layer("breath", [1.33,1.58], speed=.77, low=90, high=2500, gain=-5, attack=.008, release=.12),
            layer("ignition", [.59,.95], speed=.70, low=350, high=5300, gain=-15, offset=.17, attack=.016, release=.18)]),
    "S_CasterReleaseC": dict(seconds=1.20, peak_dbfs=-8, rms_dbfs=-20.5,
        runtime_gain=.82, voice_cap=4, owner_cap=2, priority=88, looping=False,
        trigger="Same event/concurrency group as ReleaseA.",
        description="Longer fire expulsion; independent source turbulence, same attack readability.",
        layers=[
            layer("fire_staff_17", [0,1.16], speed=1.04, low=48, high=7100, gain=0, attack=.005, release=.20),
            layer("breath", [2.47,2.84], speed=.86, low=90, high=2400, gain=-5, attack=.010, release=.14),
            layer("ignition", [.49,.83], speed=.72, low=360, high=5000, gain=-16, offset=.23, attack=.025, release=.18)]),
    "S_CasterFlightLoop": dict(seconds=1.10, crossfade_seconds=.22,
        peak_dbfs=-17, rms_dbfs=-29, runtime_gain=.52, voice_cap=4, owner_cap=1,
        priority=45, looping=True,
        trigger="Attach one component per fireball; stop on collision, reflection teardown or lifespan expiry.",
        description="Low-level moving flame turbulence. Spatial movement supplies travel, not a tonal sweep.",
        layers=[layer("fire_staff_24", [4.2,7.1], speed=1.10, low=190, high=6500, gain=0)]),
    "S_CasterImpactA": dict(seconds=1.16, peak_dbfs=-9, rms_dbfs=-23,
        runtime_gain=.80, voice_cap=4, owner_cap=1, priority=85, looping=False,
        trigger="One impact at resolved fireball contact; use lower gain for harmless expiry, or omit.",
        description="Blunt combustion burst breaking into a rapidly cooling flame tail.",
        layers=[
            layer("fire_staff_19", [0,.87], speed=.73, low=36, high=5800, gain=0, attack=.003, release=.27),
            layer("fire_staff_01", [.35,.90], speed=1.18, low=800, high=8000, gain=-11, offset=.10, attack=.01, release=.23)]),
    "S_CasterImpactB": dict(seconds=1.24, peak_dbfs=-9, rms_dbfs=-23,
        runtime_gain=.80, voice_cap=4, owner_cap=1, priority=85, looping=False,
        trigger="Same event/concurrency group as ImpactA; alternate without immediate repeat.",
        description="Alternate broken flame burst with a longer air-collapse tail.",
        layers=[
            layer("fire_staff_17", [.06,1.10], speed=.87, low=38, high=6000, gain=0, attack=.003, release=.28),
            layer("fire_staff_03", [.30,.92], speed=1.19, low=900, high=7900, gain=-10, offset=.12, attack=.012, release=.24)]),
}


def db(x):
    return float(20*np.log10(max(float(x), 1e-12)))


def sha(data):
    return hashlib.sha256(data).hexdigest()


def decode(path):
    executable = shutil.which("ffmpeg")
    if not executable:
        raise RuntimeError("Put ffmpeg on PATH before rendering this audio package")
    result = subprocess.run([executable, "-v", "error", "-i", str(path),
        "-f", "f64le", "-ac", "1", "-ar", str(RATE), "pipe:1"],
        check=True, capture_output=True)
    x = np.frombuffer(result.stdout, dtype="<f8").copy()
    if not len(x) or not np.isfinite(x).all():
        raise ValueError(f"Invalid source samples: {path}")
    return x


def fades(x, attack, release):
    x = x.copy()
    a = min(round(attack*RATE), len(x)//2)
    r = min(round(release*RATE), len(x)//2)
    if a > 1: x[:a] *= np.sin(np.linspace(0,np.pi/2,a))**2
    if r > 1: x[-r:] *= np.cos(np.linspace(0,np.pi/2,r))**2
    return x


def extract(source, recipe, looping):
    begin,end = [round(s*RATE) for s in recipe["region"]]
    if not 0 <= begin < end <= len(source):
        raise ValueError(f"Region outside retained source: {recipe}")
    x = source[begin:end].copy()
    if recipe["reverse"]: x = x[::-1]
    ratio = Fraction(1/recipe["speed"]).limit_denominator(1000)
    x = signal.resample_poly(x, ratio.numerator, ratio.denominator)
    x = signal.sosfilt(signal.butter(2, recipe["highpass_hz"], "highpass", fs=RATE, output="sos"), x)
    x = signal.sosfilt(signal.butter(2, recipe["lowpass_hz"], "lowpass", fs=RATE, output="sos"), x)
    # Body matching before layering avoids tiny quiet breath sources disappearing.
    # It does not flatten source transients or add a generated pitch component.
    x *= .10 / max(np.sqrt(np.mean(x*x)), 1e-10)
    if not looping: x = fades(x, recipe["attack_seconds"], recipe["release_seconds"])
    return x * 10**(recipe["gain_db"]/20)


def render(sources, recipe):
    n = round(recipe["seconds"]*RATE)
    cross = round(recipe.get("crossfade_seconds", 0)*RATE)
    x = np.zeros(n+cross)
    for part in recipe["layers"]:
        piece = extract(sources[part["source"]], part, recipe["looping"])
        offset = round(part["offset_seconds"]*RATE)
        if recipe["looping"] and len(piece) < n+cross:
            raise ValueError("Loop source must cover its full body and overlap")
        size = min(len(piece), len(x)-offset)
        x[offset:offset+size] += piece[:size]
    if cross:
        weight = np.linspace(0,1,cross)
        overlap = x[n:]*(1-weight) + x[:cross]*weight
        x = np.concatenate([overlap, x[cross:n]])
    else:
        x = fades(x, .0025, .075)
    # Bounded soft knee retains the attack crest with safe true-peak headroom.
    target = 10**(recipe["rms_dbfs"]/20)
    x *= target / max(np.sqrt(np.mean(x*x)), 1e-10)
    ceiling = 10**(recipe["peak_dbfs"]/20)
    x = x / (1+(np.abs(x)/ceiling)**6)**(1/6)
    return x


def pcm_bytes(x):
    pcm = np.rint(x*32767).astype("<i2")
    data = io.BytesIO()
    with wave.open(data,"wb") as f:
        f.setnchannels(1); f.setsampwidth(2); f.setframerate(RATE)
        f.writeframes(pcm.tobytes())
    return data.getvalue(), pcm.astype(np.float64)/32768


def measurements(x, looping=False):
    true_peak = np.max(np.abs(signal.resample_poly(x,4,1)))
    diffs = np.abs(np.diff(x))
    powers = np.abs(np.fft.rfft(x*np.hanning(len(x))))**2
    frequencies = np.fft.rfftfreq(len(x),1/RATE)
    report = dict(seconds=len(x)/RATE, channels=1, sample_rate=RATE, pcm_bits=16,
        sample_peak_dbfs=db(np.max(np.abs(x))), true_peak_4x_dbfs=db(true_peak),
        rms_dbfs=db(np.sqrt(np.mean(x*x))), dc_offset=float(np.mean(x)),
        full_scale_samples=int(np.sum(np.abs(x)>=32767/32768)),
        band_2_to_5khz_energy_fraction=float(powers[(frequencies>=2000)&(frequencies<5000)].sum()/powers.sum()),
        first_sample=float(x[0]), last_sample=float(x[-1]))
    if looping:
        report.update(loop_boundary_jump=float(abs(x[-1]-x[0])),
            interior_difference_p99=float(np.quantile(diffs,.99)),
            loop_boundary_jump_dbfs=db(abs(x[-1]-x[0])),
            minimum_20ms_rms_dbfs=db(min(np.sqrt(np.mean(a*a)) for a in np.array_split(x,len(x)//960))))
    return report


def mix_preview(cues):
    """Two full casts, one cancelled gather; exact timing retained in the index.

    This is an offline asset audition without attenuation, shield or game mix.
    It demonstrates loop teardown, not actual runtime event correctness.
    """
    out = np.zeros(round(12.2*RATE))
    events = []
    def add(name, at, gain=None, duration=None, envelope=None):
        recipe = RECIPES[name]
        x = cues[name].copy()
        if duration is not None:
            length = round(duration*RATE)
            x = np.tile(x,int(np.ceil(length/len(x))))[:length]
        if envelope is not None:
            times,values = zip(*envelope)
            x *= np.interp(np.arange(len(x))/RATE, times, values)
        multiplier = recipe["runtime_gain"] if gain is None else gain
        x *= multiplier
        start = round(at*RATE); out[start:start+len(x)] += x
        events.append(dict(cue=name, time_seconds=at, gain=multiplier,
            playback_seconds=len(x)/RATE, envelope=envelope))
    def complete_cast(at, releases, impacts):
        add("S_CasterIgnite",at)
        add("S_CasterFurnaceLoop",at,duration=1.855,envelope=[
            (0,0),(.12,.22),(.65,.45),(1.25,1),(1.38,1),
            (1.42,.22),(1.70,.78),(1.82,1),(1.855,0)])
        for index,(release,impact) in enumerate(zip(releases,impacts)):
            shot = at + 1.38 + .44*index
            add(release,shot)
            add("S_CasterFlightLoop",shot+.025,duration=1.25,
                envelope=[(0,0),(.07,.55),(.9,.80),(1.15,1),(1.25,0)])
            add(impact,shot+1.24)
    complete_cast(.35,["S_CasterReleaseA","S_CasterReleaseB"],["S_CasterImpactA","S_CasterImpactB"])
    complete_cast(5.0,["S_CasterReleaseC","S_CasterReleaseA"],["S_CasterImpactB","S_CasterImpactA"])
    add("S_CasterIgnite",10.0)
    add("S_CasterFurnaceLoop",10.0,duration=.67,
        envelope=[(0,0),(.12,.22),(.55,.44),(.61,.48),(.67,0)])
    return out, events


def manifest_rows(source_record):
    fields = ["asset_id","path","source_url","creator","license","proof_path","modifications","approval_status"]
    out = io.StringIO(newline="")
    writer = csv.DictWriter(out,fieldnames=fields,lineterminator="\n"); writer.writeheader()
    for key,source in source_record["sources"].items():
        writer.writerow(dict(asset_id="organic_fire_source_"+key,
            path="assets/audio-organic-fire/"+source["file"], source_url=source["source_url"],
            creator=source["creator"], license="CC0 1.0", proof_path="assets/audio-organic-fire/sources.json",
            modifications=source["retained_modifications"], approval_status="verified_cc0"))
    for name,recipe in RECIPES.items():
        used = [source_record["sources"][key] for key in sorted({p["source"] for p in recipe["layers"]})]
        writer.writerow(dict(asset_id=name.lower(),path=f"assets/audio-organic-fire/{name}.wav",
            source_url="; ".join(sorted({p["source_url"] for p in used})),
            creator="; ".join(sorted({p["creator"] for p in used}))+"; Game Studio audio editing",
            license="CC0 1.0 sources; project-authored edit",proof_path="assets/audio-organic-fire/audio-report.json",
            modifications="Recorded breath/combustion editing: explicit excerpts; resampling; EQ; envelopes; layering; level/peak control; PCM16 48 kHz mono. "+recipe["description"],
            approval_status="verified_cc0"))
    writer.writerow(dict(asset_id="organic_fire_preview",
        path="assets/audio-organic-fire/organic-fire-preview.wav",
        source_url="; ".join(sorted({x["source_url"] for x in source_record["sources"].values()})),
        creator="LEGIT Audio; mikeask; qubodup; Game Studio audio editing",
        license="CC0 1.0 sources; project-authored edit",proof_path="assets/audio-organic-fire/audio-report.json",
        modifications="Offline mix of this package: two cast volleys and a cancelled gather. Event timeline and cue hashes retained. Review asset; do not import into the game.",
        approval_status="verified_cc0"))
    return out.getvalue().encode("utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check",action="store_true")
    args = parser.parse_args()
    source_record = json.loads((OUT/"sources.json").read_text(encoding="utf-8"))
    sources = {}
    for key,source in source_record["sources"].items():
        path = (OUT/source["file"]).resolve()
        if not path.is_relative_to(OUT.resolve()) or sha(path.read_bytes()) != source["sha256"]:
            raise ValueError(f"Source identity mismatch: {key}")
        if source["license"] != "CC0 1.0": raise ValueError(f"Uncleared source: {key}")
        sources[key] = decode(path)
    report = dict(revision=REVISION, numpy=np.__version__, scipy=scipy.__version__,
        acceptance=dict(status="rejected_by_owner", owner_listened=True,
            owner_feedback="Still sounds artificial",
            reviewed_preview="organic-fire-preview.wav",
            reviewed_preview_sha256="e5b268c190eb10fe9bb22c477d5b7914b78073fbec207b85d7d0566c46639129",
            import_accepted=False,
            note="Preserved rejected baseline. Signal measurements and source rights do not establish perceptual acceptance."),
        sources=source_record, perceptual_review=dict(listened=False,
        reason="Attempted audio tool emission returned: audio content omitted because you do not support audio input.",
        scope="Decode/measurement and source/recipe review only. In-game mix and subjective sound quality require actual listening."), cues={})
    rendered = {}; files = {}
    for name,recipe in RECIPES.items():
        data,pcm = pcm_bytes(render(sources,recipe)); files[name+".wav"] = data; rendered[name] = pcm
        result = measurements(pcm,recipe["looping"])
        if result["full_scale_samples"] or result["true_peak_4x_dbfs"] > -5:
            raise ValueError(f"Insufficient cue peak headroom: {name}")
        if recipe["looping"] and result["loop_boundary_jump"] > result["interior_difference_p99"]:
            raise ValueError(f"Loop boundary needs repair: {name}")
        report["cues"][name] = dict(recipe=recipe, measurements=result, sha256=sha(data))
    preview,events = mix_preview(rendered)
    data,pcm = pcm_bytes(preview)
    if max(abs(preview)) >= .98: raise ValueError("Offline preview exceeds its uncompressed peak allowance")
    files["organic-fire-preview.wav"] = data
    report["preview"] = dict(file="organic-fire-preview.wav",sha256=sha(data),
        measurements=measurements(pcm), events=events,
        scenario="Two 1.35 s gathers with two actual-style releases 0.44 s apart, then a cancelled gather; silence afterward.",
        runtime_capture=False, cancelled_charge_silent_from_seconds=10.67,
        final_silence_peak=float(np.max(abs(pcm[round(10.67*RATE):]))))
    report["overlap_check"] = dict(kind="offline arithmetic stress, no limiter",groups=[])
    for title,voices in [
        ("two_casts_at_near_unattenuated_gain",[("S_CasterFurnaceLoop",2),("S_CasterReleaseA",1),("S_CasterReleaseB",1),("S_CasterFlightLoop",4),("S_CasterImpactA",1)]),
        ("two_impacts_with_burning_travel",[("S_CasterImpactA",1),("S_CasterImpactB",1),("S_CasterFlightLoop",4)])]:
        mix = np.zeros(round(2*RATE))
        for name,count in voices:
            x=rendered[name]*RECIPES[name]["runtime_gain"]*count
            mix[:len(x)]+=x
        measured=measurements(mix)
        report["overlap_check"]["groups"].append(dict(title=title,voices=voices,measurements=measured))
        if np.max(abs(mix)) >= .98: raise ValueError(f"Stress mix clips: {title}")
    files["audio-report.json"]=(json.dumps(report,indent=2)+"\n").encode("utf-8")
    files["manifest-rows.csv"]=manifest_rows(source_record)
    for name,data in files.items():
        path=OUT/name
        if args.check:
            if not path.exists() or path.read_bytes()!=data: raise ValueError(f"Regeneration differs: {path}")
        else:path.write_bytes(data)
    print(json.dumps(dict(mode="checked" if args.check else "rendered",revision=REVISION,
        cues=len(RECIPES), files=len(files),bytes=sum(map(len,files.values())),
        preview_peak_dbfs=report["preview"]["measurements"]["sample_peak_dbfs"],
        listening_verified=False),indent=2))


if __name__ == "__main__":
    main()

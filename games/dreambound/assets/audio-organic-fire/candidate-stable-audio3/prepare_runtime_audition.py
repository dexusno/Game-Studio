"""Prepare private runtime audition WAVs from the three existing raw SA3 trials.

Only trims, constant attenuation, endpoint fades and a short loop crossfade.
No generation/network, pitch change, filtering, reverb, layered Foley or import.
Release/impact aliases are intentionally byte-identical, not new variations.
"""
from __future__ import annotations

import argparse
import hashlib
import io
import json
from pathlib import Path
import shutil
import subprocess
import wave

import numpy as np
from scipy import signal

SOURCE = Path(__file__).resolve().parent
STUDIO = Path(__file__).resolve().parents[5]
OUTPUT = STUDIO / ".local/organic-fire-review/audio-preview-v1"
RATE = 48000
EXPECTED = {
    "internal_ignition": "6df0ea37575b4a6c5e7ed4fbb8809ba5268dc9c2fe5ba7baa491c9ee7d0f0b9e",
    "erupting_flare": "7bc201fb244fb35a9fc33cfd89860f0537d4cb2bd1702e525df42f5b89ff156c",
    "fireball_whoosh": "dd24195b91fbf5093fb05c5399b6dd954400992056aa261d704381edebc9b738",
}
RECIPES = {
    "S_CasterIgnite": dict(source="internal_ignition",region=[.34,.96],gain_db=-9.,
        fades=[.006,.055],looping=False,runtime_gain=.70,voice_cap=3,owner_cap=1,priority=78),
    "S_CasterFurnaceLoop": dict(source="internal_ignition",region=[1.20,1.96],gain_db=-3.,
        loop_crossfade_seconds=.12,looping=True,runtime_gain=.68,voice_cap=3,owner_cap=1,priority=80),
    "S_CasterReleaseA": dict(source="erupting_flare",region=[.025,1.345],gain_db=-8.,
        fades=[.004,.055],looping=False,runtime_gain=.82,voice_cap=4,owner_cap=2,priority=88),
    "S_CasterReleaseB": dict(alias_of="S_CasterReleaseA"),
    "S_CasterReleaseC": dict(alias_of="S_CasterReleaseA"),
    "S_CasterFlightLoop": dict(source="fireball_whoosh",region=[.05,1.55],gain_db=-10.,
        fades=[.008,.050],looping=False,runtime_gain=.52,voice_cap=4,owner_cap=1,priority=45),
    "S_CasterImpactA": dict(source="erupting_flare",region=[.055,.675],gain_db=-12.,
        fades=[.003,.090],looping=False,runtime_gain=.80,voice_cap=4,owner_cap=1,priority=85),
    "S_CasterImpactB": dict(alias_of="S_CasterImpactA"),
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def decode(path):
    # These three sources are stereo. Average explicitly: ffmpeg's automatic
    # stereo downmix uses equal-power coefficients and lifts identical channels.
    data = subprocess.check_output([shutil.which("ffmpeg"),"-v","error","-i",str(path),
        "-af","pan=mono|c0=0.5*c0+0.5*c1","-ar",str(RATE),"-f","f64le","pipe:1"])
    return np.frombuffer(data,dtype="<f8").copy()


def render(x,recipe):
    a,b=[round(t*RATE) for t in recipe["region"]]
    if not 0<=a<b<=len(x):raise ValueError("Trim exceeds its retained raw source")
    x=x[a:b].copy()
    if recipe["looping"]:
        cross=round(recipe["loop_crossfade_seconds"]*RATE)
        n=len(x)-cross
        blend=np.linspace(0,1,cross)
        # The wrap is now inside continuous source samples; it never restarts
        # the flame transient or inserts silence at the file boundary.
        x=np.concatenate([x[n:]*(1-blend)+x[:cross]*blend,x[cross:n]])
    else:
        attack,release=[round(s*RATE) for s in recipe["fades"]]
        x[:attack]*=np.linspace(0,1,attack)
        x[-release:]*=np.linspace(1,0,release)
    return x*10**(recipe["gain_db"]/20)


def encode(x):
    if not np.isfinite(x).all() or max(abs(x))>=.98:raise ValueError("Unsafe private-cue sample range")
    pcm=np.rint(x*32767).astype("<i2")
    data=io.BytesIO()
    with wave.open(data,"wb") as f:
        f.setnchannels(1);f.setsampwidth(2);f.setframerate(RATE);f.writeframes(pcm.tobytes())
    return data.getvalue(),pcm.astype(float)/32768


def db(value):
    return float(20*np.log10(max(float(value),1e-12)))


def measure(x,looping):
    r=dict(sample_rate=RATE,pcm_bits=16,channels=1,
        peak_dbfs=db(max(abs(x))),true_peak_4x_dbfs=db(max(abs(signal.resample_poly(x,4,1)))),
        rms_dbfs=db(np.sqrt(np.mean(x*x))),dc_offset=float(np.mean(x)),
        full_scale_samples=int(np.count_nonzero(abs(x)>=32767/32768)),
        first_sample=float(x[0]),last_sample=float(x[-1]))
    if looping:
        jump=abs(x[-1]-x[0]);p99=np.quantile(abs(np.diff(x)),.99)
        r.update(loop_boundary_jump=float(jump),loop_boundary_jump_dbfs=db(jump),
            interior_difference_p99=float(p99),
            minimum_20ms_rms_dbfs=db(min(np.sqrt(np.mean(piece*piece)) for piece in np.array_split(x,len(x)//960))))
        if jump>p99:raise ValueError("Loop sample boundary requires repair")
    elif x[0] or x[-1]:raise ValueError("One-shot endpoint is not digital zero")
    if r["full_scale_samples"] or r["true_peak_4x_dbfs"]>-6:
        raise ValueError("Private cue needs more fixed attenuation")
    return r


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument("--check",action="store_true")
    args=parser.parse_args()
    if not shutil.which("ffmpeg"):raise RuntimeError("ffmpeg must be on PATH")
    OUTPUT.mkdir(parents=True,exist_ok=True)
    sources={};provenance={}
    for key,identity in EXPECTED.items():
        path=SOURCE/(key+"-raw.wav")
        if sha(path.read_bytes())!=identity:raise ValueError(f"Raw trial identity changed: {key}")
        sources[key]=decode(path)
        request=json.loads((SOURCE/(key+"-request.json")).read_text(encoding="utf-8"))
        if request["status"]!="complete" or request["output_sha256"]!=identity:
            raise ValueError("Raw output lacks matching completed generation evidence")
        provenance[key]=dict(path=path.relative_to(STUDIO).as_posix(),sha256=identity,
            request_record=(SOURCE/(key+"-request.json")).relative_to(STUDIO).as_posix(),
            model=request["model"],space_revision=request["space_revision"],
            model_metadata_revision=request["model_metadata_revision"],
            actual_loaded_model_revision=request["actual_loaded_model_revision"],
            settings=request["settings"],event_id=request["event_id"],created_utc=request["created_utc"])
    encoded={};pcm={};files=[]
    for name,definition in RECIPES.items():
        original=definition.get("alias_of")
        recipe=RECIPES[original] if original else definition
        if original:
            encoded[name]=encoded[original];pcm[name]=pcm[original]
        else:encoded[name],pcm[name]=encode(render(sources[recipe["source"]],recipe))
        measurements=measure(pcm[name],recipe["looping"])
        path=OUTPUT/(name+".wav")
        if args.check:
            if path.read_bytes()!=encoded[name]:raise ValueError(f"Private render differs: {name}")
        else:path.write_bytes(encoded[name])
        # Read the delivered WAV independently using ffmpeg's decoder.
        subprocess.run([shutil.which("ffmpeg"),"-v","error","-i",str(path),"-f","null","-"],
            check=True,capture_output=True)
        files.append(dict(name=name,path=path.name,sha256=sha(encoded[name]),looping=recipe["looping"],
            duration_seconds=len(pcm[name])/RATE,alias_of=original,
            runtime_gain=recipe["runtime_gain"],voice_cap=recipe["voice_cap"],owner_cap=recipe["owner_cap"],
            priority=recipe["priority"],source_key=recipe["source"],source_region_seconds=recipe["region"],
            constant_gain_db=recipe["gain_db"],endpoint_fades_seconds=recipe.get("fades"),
            loop_crossfade_seconds=recipe.get("loop_crossfade_seconds",0),measurements=measurements,
            ffmpeg_decode_errors=0))
    # These are specific arithmetic overlaps, not a full game-mix assessment.
    stress=np.zeros(2*RATE)
    for name,offset,count in [("S_CasterReleaseA",0,1),("S_CasterReleaseB",.44,1),
            ("S_CasterFurnaceLoop",0,1),("S_CasterFlightLoop",0,2),("S_CasterImpactA",.7,1)]:
        recipe=RECIPES[RECIPES[name]["alias_of"]] if "alias_of" in RECIPES[name] else RECIPES[name]
        x=pcm[name]*recipe["runtime_gain"]*count;start=round(offset*RATE)
        stress[start:start+len(x)]+=x
    stress_peak=db(max(abs(stress)))
    if max(abs(stress))>=.98:raise ValueError("Private selected overlap clips")
    report=dict(revision="stable-audio3-private-audition-v1",complete=True,
        acceptance="private-unaccepted-audition",release_cleared=False,agent_listened=False,
        owner_accepted=False,source_count=3,file_count=len(files),unique_audio_files=len(set(sha(x) for x in encoded.values())),
        provenance=provenance,files=files,
        processing="44.1 kHz stereo decoded using explicit 0.5 left + 0.5 right channel averaging and resampled to mono 48 kHz; explicit trims, constant attenuation, endpoint fades and one short loop crossfade. No pitch changes, EQ, synthesis, reverb, compression or Foley layering.",
        alias_disclosure="Release A/B/C are byte-identical aliases of one generated eruption. Impact A/B are byte-identical shortened derivatives of that same eruption. They are not independent variations or dedicated impact recordings.",
        flight_disclosure="Despite its legacy filename, S_CasterFlightLoop must import looping=false. It contains one flyby ending before 1.5 seconds; repetition would make multiple passes per projectile.",
        furnace_disclosure="Only a 0.76-second later ignition region supplies a 0.64-second loop with 0.12-second wrap overlap. Repetition and perceived naturalness are unverified.",
        source_license_evidence=(SOURCE/"model-license-evidence.txt").relative_to(STUDIO).as_posix(),
        source_license_evidence_sha256=sha((SOURCE/"model-license-evidence.txt").read_bytes()),
        selected_overlap=dict(peak_dbfs=stress_peak,full_scale_samples=int(np.count_nonzero(abs(stress)>=1)),
            scope="Offline overlap of two release aliases 0.44 s apart, one charge, two flight voices, and one impact; excludes other game sounds and arbitrary alignments."),
        target_timing=dict(gather_seconds=1.35,release_attack_seconds=[.03,.47],flight_seconds=1.5,impact_seconds=.62),
        component_guidance="Retain charge/ignite components; fade out on interruption and after completed volley. Pause/stop with game state. Flight is a one-shot attached to projectile and stopped on early collision. Impact once at resolved contact. Import gain/pitch 1; runtime gain applied once. Honor looping field, not filename.",
        verification_scope="Raw hashes, deterministic edits, actual WAV decode, mono format, levels, digital-zero one-shot endpoints and sample-continuous furnace wrap. No hearing, naturalness, game synchronization or whole-mix acceptance claim.")
    data=(json.dumps(report,indent=2)+"\n").encode("utf-8")
    report_path=OUTPUT/"audition-report.json"
    if args.check:
        if report_path.read_bytes()!=data:raise ValueError("Private report differs")
    else:report_path.write_bytes(data)
    for key,identity in EXPECTED.items():
        if sha((SOURCE/(key+"-raw.wav")).read_bytes())!=identity:raise ValueError("Raw trial changed during render")
    print(json.dumps(dict(mode="checked" if args.check else "rendered",report=str(report_path),
        complete=report["complete"],files=len(files),unique_audio_files=report["unique_audio_files"],
        selected_overlap_peak_dbfs=stress_peak,acceptance=report["acceptance"],agent_listened=False),indent=2))


if __name__=="__main__":main()

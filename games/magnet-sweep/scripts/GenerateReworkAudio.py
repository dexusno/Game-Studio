"""Original Magnet Sweep rework sound/music synthesis, 2026-09-12.

No samples, soundfonts, external music, training model or network service.
Run from any directory: python games/magnet-sweep/scripts/GenerateReworkAudio.py
Requires NumPy and SciPy. Writes only assets/audio/rework/.
"""
from __future__ import annotations

import csv
import hashlib
import json
import math
import platform
from pathlib import Path

import numpy as np
import scipy
from scipy import fft, signal
from scipy.io import wavfile

SR = 48000
OUT = Path(__file__).resolve().parents[1] / "assets" / "audio" / "rework"
RNG = np.random.default_rng(12092026)


def seed(name):
    global RNG
    RNG = np.random.default_rng(int.from_bytes(hashlib.sha256(name.encode()).digest()[:8], "little"))


def timeline(seconds):
    return np.arange(round(seconds * SR), dtype=np.float64) / SR


def smooth_edges(x, onset=.002, ending=.03):
    x = x.copy()
    a, b = min(round(onset * SR), len(x)), min(round(ending * SR), len(x))
    if a > 1:
        x[:a] *= np.sin(np.linspace(0, np.pi / 2, a)) ** 2
    if b > 1:
        x[-b:] *= np.cos(np.linspace(0, np.pi / 2, b)) ** 2
    return x


def filtered_noise(seconds, low=90, high=5000):
    x = RNG.normal(0, 1, round(seconds * SR))
    sos = signal.butter(2, [low, high], btype="bandpass", fs=SR, output="sos")
    y = signal.sosfilt(sos, x)
    return y / max(np.std(y), 1e-10)


def periodic_noise(seconds, low=90, high=3500):
    n = round(seconds * SR)
    f = fft.rfftfreq(n, 1 / SR)
    envelope = (1 - np.exp(-(f / low) ** 4)) * np.exp(-(f / high) ** 3)
    z = fft.rfft(RNG.normal(size=n)) * envelope
    z[0] = 0
    y = fft.irfft(z, n)
    return y / max(np.std(y), 1e-10)


def add(dst, src, start=0, gain=1, pan=0):
    offset = round(start * SR)
    a, b = max(0, offset), min(len(dst), offset + len(src))
    if b <= a:
        return
    clip = src[a-offset:b-offset] * gain
    if dst.ndim == 1:
        dst[a:b] += clip
    else:
        theta = (np.clip(pan, -1, 1) + 1) * np.pi / 4
        dst[a:b, 0] += clip * np.cos(theta)
        dst[a:b, 1] += clip * np.sin(theta)


def add_circular(dst, src, start, gain=1, pan=0):
    """Notes and their release tails wrap into the next loop, without fades to silence."""
    offset = round(start * SR) % len(dst)
    theta = (np.clip(pan, -1, 1) + 1) * np.pi / 4
    k = 0
    while k < len(src):
        count = min(len(src) - k, len(dst) - offset)
        dst[offset:offset+count, 0] += src[k:k+count] * gain * np.cos(theta)
        dst[offset:offset+count, 1] += src[k:k+count] * gain * np.sin(theta)
        k += count
        offset = 0


def metal(base=420, duration=.55, weight=1, damping=1):
    """Damped inharmonic plate modes + felt strike + two tiny contact bounces."""
    t = timeline(duration)
    x = np.zeros_like(t)
    ratios = [1, 1.487, 2.071, 2.684, 3.913, 5.127, 6.313, 8.079, 10.231]
    for i, ratio in enumerate(ratios):
        f = base * ratio * (1 + RNG.uniform(-.007, .007))
        if f > 7200:
            continue
        decay = (.24 * weight / (1 + .43 * i)) / damping
        amplitude = 1 / (1 + i) ** 1.25
        x += amplitude * np.sin(2*np.pi*f*t + .1*np.sin(2*np.pi*19*t)) * np.exp(-t/decay)
    x += .65 * filtered_noise(duration, 380, 4700) * np.exp(-t / .008)
    x += .38 * np.sin(2*np.pi*(105 + 24/weight)*t) * np.exp(-t / (.045*weight))
    # Quiet rebounding contacts make an object rather than a lone electronic ping.
    for when, level in [(.033, .14), (.087, .06)]:
        j = round(when * SR)
        if j < len(x):
            q = t[:len(x)-j]
            x[j:] += level * np.sin(2*np.pi*base*1.013*q) * np.exp(-q/.036)
    return smooth_edges(x, .0008, .065)


def body(duration=.6, low=69, high=140, strength=1):
    t = timeline(duration)
    freq = low + (high-low)*np.exp(-t/.07)
    phase = np.cumsum(freq) * (2*np.pi/SR)
    x = (np.sin(phase) + .23*np.sin(2.03*phase)) * np.exp(-t/.14) * strength
    x += .22 * filtered_noise(duration, 45, 850) * np.exp(-t/.075)
    return smooth_edges(x, .002, .08)


def modal_key(midi, duration=3.4, felt=.65):
    """Original softly struck electric/felt-key voice, not a sampled instrument."""
    t = timeline(duration)
    f = 440 * 2 ** ((midi-69)/12)
    x = np.zeros_like(t)
    for h in range(1, 9):
        stiffness = 1 + .0008 * h*h
        decay = (2.9 / (1 + .33*h)) * (1.15 if midi < 60 else 1)
        amp = np.exp(-felt*(h-1)) / h**.6
        x += amp*np.sin(2*np.pi*f*h*stiffness*t + .035*np.sin(2*np.pi*3.7*t))*np.exp(-t/decay)
    # Very soft hammer texture, no harsh top octave.
    x += .018 * filtered_noise(duration, 300, 2100) * np.exp(-t/.012)
    return smooth_edges(x, .005, .4)


def airy_pad(midi, sustain=12, duration=16):
    t = timeline(duration)
    f = 440 * 2**((midi-69)/12)
    attack = np.sin(np.minimum(t/1.6, 1)*np.pi/2)**2
    release = np.cos(np.clip((t-sustain)/(duration-sustain), 0, 1)*np.pi/2)**2
    env = attack*release
    x = np.zeros_like(t)
    for h, amp in [(1,1), (2,.22), (3,.12), (4,.045), (5,.021)]:
        x += amp*np.sin(2*np.pi*f*h*t + .035*np.sin(2*np.pi*.13*t))
        x += amp*.26*np.sin(2*np.pi*f*h*1.0011*t)
    return x * env * (.95 + .05*np.sin(2*np.pi*.21*t))


def room(x, wet=.13, circular=False):
    """Original sparse/diffuse stereo room, no recorded impulse response."""
    stereo = np.column_stack([x, x]) if x.ndim == 1 else x.copy()
    result = stereo.copy()
    for channel in range(2):
        src = stereo[:, 1-channel]
        taps = [.037, .071, .113, .157, .227, .311, .419, .563, .739, .947, 1.181]
        for i, d in enumerate(taps):
            delay = round((d + channel*.011)*SR)
            level = wet*np.exp(-d/ .58) * (-1 if (i+channel)%3 == 0 else 1)
            if circular:
                result[:,channel] += np.roll(src, delay)*level
            elif delay < len(src):
                result[delay:,channel] += src[:-delay]*level
    return result


def effects():
    items = {}
    seed("magnet_loop")
    t = timeline(4)
    hum = (np.sin(2*np.pi*65*t) + .35*np.sin(2*np.pi*130*t) + .13*np.sin(2*np.pi*195*t))
    hum *= .46 + .022*np.sin(2*np.pi*.5*t)
    hum += .07*periodic_noise(4, 240, 1700)*(1+.1*np.sin(2*np.pi*.75*t))
    hum += .035*np.sin(2*np.pi*390*t + .06*np.sin(2*np.pi*.5*t))
    items["magnet_loop"] = (hum, True, -21, -10)

    for name, upward in [("magnet_on", True), ("magnet_off", False)]:
        seed(name)
        t = timeline(.64 if upward else .54)
        f = 52 + 69*(1-np.exp(-t/.11)) if upward else 45 + 78*np.exp(-t/.1)
        phase = np.cumsum(f)*(2*np.pi/SR)
        env = np.sin(np.minimum(t/.045,1)*np.pi/2)**2 * np.exp(-t/(.27 if upward else .16))
        x = .8*(np.sin(phase)+.22*np.sin(phase*2))*env
        x += filtered_noise(len(t)/SR, 240, 2300)*.22*env
        add(x, metal(480 if upward else 295, .25, .75), .02, .14)
        items[name] = (smooth_edges(x), False, -22, -8)

    for number, (base, weight) in enumerate([(437, .95), (351, 1.12), (526,.83)], 1):
        seed(f"pickup_metal{number}")
        items[f"pickup_metal{number}"] = (metal(base, .62, weight), False, -24, -9)
    seed("pickup_heavy")
    x = np.zeros(round(.95*SR))
    add(x, body(.85, 63, 126), 0, .9)
    add(x, metal(183,.8,1.65), .019, .7)
    add(x, metal(311,.55,.95), .079, .23)
    items["pickup_heavy"] = (x, False, -21, -7)

    seed("rare_find")
    x = np.zeros((round(2.3*SR),2))
    for i, m in enumerate([74,81,85,88]):
        add(x, modal_key(m,2.1,.25), i*.095, .30/(1+.18*i), -.45+.3*i)
    items["rare_find"] = (room(x,.21), False, -23, -8)

    seed("vent")
    t = timeline(1.08)
    env = (1-np.exp(-t/.009))*np.exp(-t/.17)
    x = .55*filtered_noise(1.08,70,3500)*env
    x += .8*signal.chirp(t, 215, .45, 43, method="quadratic")*env
    add(x, metal(278,.6,1.3), .22,.40)
    add(x, metal(537,.38,.8), .36,.22)
    items["vent"] = (smooth_edges(x), False,-21,-7)

    seed("warning_loop")
    n = round(1.5*SR)
    x = np.zeros(n)
    tt = timeline(.46)
    for onset in [0,.5,1]:
        wobble = np.sin(2*np.pi*304*tt)+.23*np.sin(2*np.pi*608*tt)
        note = smooth_edges(wobble*np.exp(-tt/.11), .012,.08)
        add(x,note,onset,.34)
        add(x,metal(611,.14,.42,2),onset+.012,.055)
    x += .09*periodic_noise(1.5,110,680)
    items["warning_loop"] = (x,True,-23,-11)

    seed("overload")
    x = np.zeros((round(2.7*SR),2))
    add(x,body(1.1,48,181),0,1.0)
    crack = smooth_edges(filtered_noise(.35,120,6100)*np.exp(-timeline(.35)/.026),.0005,.06)
    add(x,crack, .014,.7)
    for i in range(17):
        onset=.035+i*.051+RNG.uniform(0,.055)
        add(x,metal(RNG.uniform(145,720),.85,RNG.uniform(.6,1.5)),onset,.28*np.exp(-onset/1.0),RNG.uniform(-.8,.8))
    items["overload"] = (room(x,.10),False,-19,-6)

    seed("smelt")
    x = np.zeros((round(3.5*SR),2))
    for i in range(18):
        onset=.05+i*.052+RNG.uniform(0,.028)
        add(x,metal(RNG.uniform(150,570),.65,RNG.uniform(.8,1.5)),onset,.17,RNG.uniform(-.6,.6))
    t=timeline(3.1)
    sizzle=(1-np.exp(-t/.33))*np.exp(-t/1.1)
    add(x,filtered_noise(3.1,900,5400)*sizzle,.18,.22)
    molten=filtered_noise(3.1,48,460)*sizzle
    molten += .35*(np.sin(2*np.pi*67*t)+.21*np.sin(2*np.pi*111*t))*sizzle
    add(x,molten,.23,.48)
    # Discrete small bubbles within the melt, followed by a cast-metal landing.
    for i in range(14):
        add(x,body(.16,RNG.uniform(110,175),RNG.uniform(260,380)),.45+i*.095,.033,RNG.uniform(-.35,.35))
    add(x,metal(202,.7,1.7),2.61,.26)
    add(x,body(.6,64,99),2.62,.32)
    items["smelt"]=(room(x,.09),False,-21,-7)

    seed("payout")
    x=np.zeros((round(1.2*SR),2))
    for i,m in enumerate([74,81,86]):
        add(x,modal_key(m,.95,.4),i*.084,.25,-.2+.2*i)
    add(x,metal(833,.28,.5),.024,.07)
    items["payout"]=(room(x,.12),False,-25,-10)

    seed("upgrade")
    x=np.zeros((round(3.2*SR),2))
    add(x,body(.7,58,113),0,.38)
    for i,m in enumerate([50,57,62,66,69,74]):
        add(x,modal_key(m,2.8,.5),.16+i*.13,.22,-.55+i*.22)
    add(x,metal(268,.9,1.5),.12,.13)
    items["upgrade"]=(room(x,.23),False,-22,-8)

    seed("contract_success")
    x=np.zeros((round(3.9*SR),2))
    for when,notes in [(0,[55,62,69]),(.45,[57,64,71]),(.95,[50,57,62,66,73])]:
        for i,m in enumerate(notes):
            add(x,modal_key(m,2.9,.8),when+i*.016,.16,-.5+i*.22)
    add(x,body(.7,63,94),.98,.17)
    items["contract_success"]=(room(x,.23),False,-23,-9)

    seed("contract_fail")
    x=np.zeros((round(1.8*SR),2))
    add(x,metal(188,.62,1.2),0,.17)
    for i,m in enumerate([52,47,45]):
        add(x,modal_key(m,1.5,1.2),i*.17,.22)
    items["contract_fail"]=(room(x,.11),False,-25,-11)

    seed("ui_click")
    t=timeline(.12)
    x=.24*np.sin(2*np.pi*712*t)*np.exp(-t/.011)
    x+=.30*filtered_noise(.12,480,2300)*np.exp(-t/.007)
    items["ui_click"]=(smooth_edges(x,.001,.035),False,-28,-15)

    seed("furnace_loop")
    t=timeline(6)
    x=.28*periodic_noise(6,45,370)+.025*periodic_noise(6,650,2800)
    x*=.8+.08*np.sin(2*np.pi*t/3)+.04*np.sin(2*np.pi*t/2)
    x+=.1*np.sin(2*np.pi*57*t)+.032*np.sin(2*np.pi*114*t)
    items["furnace_loop"]=(x,True,-29,-16)
    return items


def music():
    seed("workshop_music_original_32bars")
    # Original 32-bar A/B ambient cue, 80 BPM, 4/4, D major / relative B minor.
    # Eight four-bar harmonies give actual movement, with the final A6 resolving
    # into Dmaj9 at the seamless wrap. No existing melody is referenced.
    beat=.75
    bar=4*beat
    duration=32*bar
    x=np.zeros((round(duration*SR),2),dtype=np.float64)
    chords=[
        ([50,54,57,61,64],38),  # Dmaj9
        ([47,50,54,57,61],35),  # Bm9
        ([43,47,50,54,57],31),  # Gmaj9
        ([45,50,52,54,59],33),  # A6/9sus
        ([52,55,59,62,66],40),  # Em9, B section opens
        ([43,47,50,54,57],31),  # Gmaj9
        ([42,50,54,57,64],30),  # Dmaj9/F#
        ([45,49,52,54,59],33),  # A6/9 -> D
    ]
    for section,(notes,bass) in enumerate(chords):
        start=section*4*bar
        for voice,m in enumerate(notes):
            add_circular(x,airy_pad(m,12,16),start,.018,-.65+voice*.325)
        for j in range(4):
            when=start+j*bar
            add_circular(x,modal_key(bass,4.5,1.4),when,.09,0)
            # Varied, low-key broken chord figures; gaps leave space for salvage.
            pattern=([0,2,4,1] if section%2==0 else [0,3,2,4])
            for k,voice in enumerate(pattern):
                if (j==3 and k in [2,3]) or (section==4 and j==0 and k==0):
                    continue
                onset=when+(k*.82+.34)*beat
                note=notes[voice]+12
                add_circular(x,modal_key(note,3.0,.92),onset,.058*(.8+.2*(k%2)),-.40+.26*k)
    # Original sparse conversational motifs, with rests and a distinct B phrase.
    phrases=[
        (1,[(.5,69),(2.25,73),(3.5,76)]),
        (3,[(0,74),(1.5,69)]),
        (5,[(.75,73),(2.5,69),(3.25,66)]),
        (7,[(1,64),(2.5,66)]),
        (9,[(.5,66),(2,69),(3.25,71)]),
        (11,[(.75,69),(2.75,66)]),
        (13,[(1,64),(2.5,66)]),
        (15,[(.5,69),(2.5,73)]),
        (17,[(.5,71),(1.75,74),(3,78)]),
        (19,[(.5,76),(2.25,74)]),
        (21,[(1,71),(2.5,69),(3.25,66)]),
        (23,[(.5,69),(2.5,66)]),
        (25,[(.75,69),(2.25,73),(3.5,76)]),
        (27,[(1,74),(2.5,69)]),
        (29,[(.5,71),(2.25,69)]),
        (31,[(.25,66),(1.75,64),(3,61)]),
    ]
    for b,events in phrases:
        for j,(at,note) in enumerate(events):
            add_circular(x,modal_key(note,4.0,.6),b*bar+at*beat,.069, .22 if (b+j)%2 else -.22)
    # Extremely quiet brushed pulse, not a drum beat demanding attention.
    for b in range(32):
        for half in [1.5,3.5]:
            t=timeline(.13)
            brush=smooth_edges(filtered_noise(.13,600,2700)*np.exp(-t/.035),.009,.035)
            add_circular(x,brush,b*bar+half*beat,.0028,-.55 if half<2 else .55)
    x=room(x,.19,circular=True)
    return x,True,-25,-11


def master(x, loop, target_rms, ceiling):
    x=x.astype(np.float64)
    if x.ndim==1:
        x=room(x,.08,loop)
    # Smooth global spectral shaping: mono-safe low end, soft bright transients.
    # Circular FFT filtering preserves loop continuity; oneshots are tail-faded.
    f=fft.rfftfreq(len(x),1/SR)
    eq=(1-np.exp(-(f/34)**4))*np.exp(-(f/10500)**6)
    x=fft.irfft(fft.rfft(x,axis=0)*eq[:,None],len(x),axis=0)
    mid=x.mean(axis=1)
    side=(x[:,0]-x[:,1])*.5
    highpass=1-np.exp(-(f/160)**4)
    side=fft.irfft(fft.rfft(side)*highpass,len(x))
    x=np.column_stack([mid+side,mid-side])
    x-=x.mean(axis=0)
    if not loop:
        x*=smooth_edges(np.ones(len(x)),.001,.08)[:,None]
    rms=np.sqrt(np.mean(x*x))
    gain=min(10**(target_rms/20)/max(rms,1e-10),10**(ceiling/20)/max(np.max(np.abs(x)),1e-10))
    x*=gain
    # No clipping/brickwall loudness race. TPDF dither before PCM16 conversion.
    dither=(RNG.random(x.shape)-RNG.random(x.shape))/65536
    pcm=np.rint((x+dither)*32767).clip(-32768,32767).astype(np.int16)
    if not loop:
        pcm[0]=0
        pcm[-1]=0
    return pcm


def db(value):
    return round(20*np.log10(max(float(value),1e-12)),3)


def analyse(path, loop):
    rate,pcm=wavfile.read(path)
    x=pcm.astype(np.float64)/32768
    peak=np.max(np.abs(x))
    rms=np.sqrt(np.mean(x*x))
    # Four-times interpolation catches intersample excursions.
    tp=max(float(np.max(np.abs(signal.resample_poly(x[:,c],4,1)))) for c in range(2))
    diff=np.diff(x,axis=0)
    seam=x[0]-x[-1]
    seam_rms=np.sqrt(np.mean(seam*seam))
    diff95=np.percentile(np.abs(diff),95)
    mono=x.mean(axis=1)
    freqs,power=signal.welch(mono,rate,nperseg=min(8192,len(mono)))
    total=float(power.sum())
    return {
        "filename":path.name,"sample_rate":rate,"channels":2,"sample_format":"PCM16",
        "duration_seconds":round(len(pcm)/rate,5),"loop":loop,
        "sample_peak_dbfs":db(peak),"rms_dbfs":db(rms),"true_peak_4x_dbtp":db(tp),
        "clipped_samples":int(np.count_nonzero((pcm==32767)|(pcm==-32768))),
        "dc_mean":round(float(np.max(np.abs(x.mean(axis=0)))),9),
        "loop_join_delta_dbfs":db(seam_rms) if loop else None,
        "join_delta_vs_95pct_step":round(float(np.max(np.abs(seam))/max(diff95,1e-10)),4) if loop else None,
        "stereo_correlation":round(float(np.corrcoef(x.T)[0,1]),4),
        "energy_above_8khz_percent":round(float(power[freqs>8000].sum()/max(total,1e-12)*100),6),
        "energy_below_250hz_percent":round(float(power[freqs<250].sum()/max(total,1e-12)*100),6),
        "sha256":hashlib.sha256(path.read_bytes()).hexdigest(),
    }


def diagnostic_render():
    """Auditable synthetic mix and plots; this is not in-engine listening evidence."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    def read(name):
        return wavfile.read(OUT/(name+".wav"))[1].astype(np.float64)/32768

    mix=read("workshop_music")[:12*SR]*.40
    hum=read("magnet_loop")
    for c in range(2):
        mix[:6*SR,c] += np.tile(hum[:,c],2)[:6*SR]*.30
    events=[("magnet_on",.02,.50),("pickup_metal1",.8,.42),
            ("pickup_metal2",.89,.42),("pickup_heavy",1.05,.62),
            ("pickup_metal3",2.1,.42),("warning_loop",2.8,.68),
            ("overload",4.28,.8),("smelt",7.1,.70),
            ("payout",9.73,.65)]
    for name,start,gain in events:
        clip=read(name)
        offset=round(start*SR)
        stop=min(len(mix),offset+len(clip))
        mix[offset:stop]+=clip[:stop-offset]*gain*.9
    mix_peak=float(np.max(np.abs(mix)))
    result={"scenario":"12-second original-music, field, overlapping pickups, warning, overload, smelt, payout synthetic mix",
            "not_in_engine":True,"sample_peak_dbfs":db(mix_peak),
            "clipped_samples":int(np.count_nonzero(np.abs(mix)>=1)),
            "sfx_master":.9,"music_gain":.4,"magnet_loop_gain":.3,
            "events":[{"cue":n,"time":t,"gain":g} for n,t,g in events]}
    (OUT/"mix-check.json").write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    assert mix_peak<.95,result

    fig,axes=plt.subplots(3,2,figsize=(14,9),facecolor="#142429")
    for ax in axes.flat:
        ax.set_facecolor("#203238")
        ax.tick_params(colors="#c9dddd",labelsize=8)
        for spine in ax.spines.values():spine.set_color("#698184")
        ax.title.set_color("#d8e6e2")
        ax.xaxis.label.set_color("#c9dddd")
        ax.yaxis.label.set_color("#c9dddd")
    step=96
    axes[0,0].plot(np.arange(0,len(mix),step)/SR,mix[::step,0],color="#6be5ce",lw=.45)
    axes[0,0].set_title("Constructed mix, left channel — peak %.2f dBFS"%db(mix_peak))
    axes[0,0].set_xlabel("Seconds")
    mus=read("workshop_music")
    block=SR//4
    levels=np.sqrt(np.mean(mus[:len(mus)//block*block].reshape(-1,block,2)**2,axis=(1,2)))
    axes[0,1].plot(np.arange(len(levels))*.25,20*np.log10(levels+1e-9),color="#eaba77")
    axes[0,1].set_ylim(-40,-14)
    axes[0,1].set_title("96-second score: 250 ms RMS, harmonic development")
    axes[0,1].set_xlabel("Seconds")
    for ax,name in zip(axes[1], ["magnet_loop","workshop_music"]):
        clip=read(name)
        n=round(.012*SR)
        joined=np.concatenate([clip[-n:,0],clip[:n,0]])
        ax.plot((np.arange(len(joined))-n)/SR*1000,joined,color="#6be5ce",lw=.7)
        ax.axvline(0,color="#eaba77",lw=.8)
        ax.set_title(name+": actual end/start join")
        ax.set_xlabel("Milliseconds around loop join")
    for ax,name in zip(axes[2],["pickup_heavy","smelt"]):
        clip=read(name).mean(axis=1)
        f,t,z=signal.spectrogram(clip,SR,nperseg=512,noverlap=384)
        keep=f<7000
        ax.pcolormesh(t,f[keep],10*np.log10(z[keep]+1e-12),vmin=-90,vmax=-35,cmap="magma",shading="auto")
        ax.set_title(name+": layered body / metallic spectrum")
        ax.set_xlabel("Seconds")
    fig.suptitle("Magnet Sweep rework audio — numerical/visual diagnostics, not listening",color="#f0e8d8",fontsize=14)
    fig.tight_layout(rect=(0,0,1,.96))
    fig.savefig(OUT/"audio-diagnostics.png",dpi=140)
    plt.close(fig)


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    items=effects()
    items["workshop_music"]=music()
    records=[]
    for name,(x,loop,rms,ceiling) in items.items():
        seed("master_"+name)
        pcm=master(x,loop,rms,ceiling)
        path=OUT/(name+".wav")
        wavfile.write(path,SR,pcm)
        item=analyse(path,loop)
        assert item["clipped_samples"]==0,path
        assert item["true_peak_4x_dbtp"] < -4.5,(path,item)
        if loop:
            assert item["join_delta_vs_95pct_step"]<3.0,(path,item)
        records.append(item)
        print(f"{name}: {item['duration_seconds']}s peak {item['sample_peak_dbfs']}dBFS RMS {item['rms_dbfs']}dBFS",flush=True)
    metadata={
        "created":"2026-09-12","generator":"scripts/GenerateReworkAudio.py",
        "generator_sha256":hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "python":platform.python_version(),"numpy":np.__version__,"scipy":scipy.__version__,
        "music":{"title":"Copperlight Workshop","duration":96,"bpm":80,"bars":32,
                 "meter":"4/4","tonal_center":"D major / B minor","composition":"Original notes and arrangement in generator; no reference track"},
        "verification":"Numerical WAV/peak/loop analysis. Listening and in-engine mix require separate evidence.",
        "files":records,
    }
    (OUT/"analysis.json").write_text(json.dumps(metadata,indent=2)+"\n",encoding="utf-8")
    fields=["asset_id","path","source_url","creator","license","proof_path","modifications","approval_status"]
    with (OUT/"manifest-rows.csv").open("w",newline="",encoding="utf-8") as handle:
        writer=csv.DictWriter(handle,fieldnames=fields)
        writer.writeheader()
        for r in records:
            name=Path(r["filename"]).stem
            writer.writerow({
                "asset_id":"rework_"+name,"path":"assets/audio/rework/"+r["filename"],
                "source_url":"original: scripts/GenerateReworkAudio.py","creator":"Game Studio / Codex",
                "license":"Original project asset; no third-party samples/composition; project distribution license unset",
                "proof_path":"assets/audio/rework/PROVENANCE.md",
                "modifications":"Original synthesis/arrangement; stereo room; spectral shaping; level control; dither; PCM16 export",
                "approval_status":"original-project",
            })
    diagnostic_render()


if __name__=="__main__":
    main()

"""Original deterministic shield sounds, no recordings or external assets.
Run: python games/dreambound/scripts/create_audio.py
Read-only reproducibility and PCM verification: add --check.
NumPy synthesizes; installed SciPy measures 4x oversampled peaks.
Signal checks do not replace audition or the engine mix.
"""
from pathlib import Path
import argparse
import hashlib
import io
import json
import wave
import numpy as np
import scipy
from scipy.signal import resample_poly

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets" / "audio"
RATE = 48000
TAU = 2 * np.pi
REVISION = "shield-mechanism-v2"


def db(value):
    return float(20 * np.log10(max(float(value), 1e-12)))


def rms(values):
    return float(np.sqrt(np.mean(values * values)))


def noise(rng, count, low, high, slope=0):
    """Periodic band-limited noise; one-shot envelopes localize the layer."""
    frequencies = np.fft.rfftfreq(count, 1 / RATE)
    weights = np.zeros_like(frequencies)
    mask = (frequencies >= low) & (frequencies <= high)
    weights[mask] = (frequencies[mask] / low) ** slope
    signal = np.fft.irfft(np.fft.rfft(rng.normal(size=count)) * weights, count)
    return signal / max(rms(signal), 1e-12)


def env(t, attack, decay):
    return (1 - np.exp(-t / attack)) * np.exp(-t / decay)


def window(t, duration):
    return np.sin(np.pi * np.clip(t / duration, 0, 1)) ** 2 * (t < duration)


def modes(t, frequencies, decays, weights, attack=.0008):
    out = np.zeros_like(t)
    for frequency, decay, weight in zip(frequencies, decays, weights):
        out += weight * np.sin(TAU * frequency * t) * env(t, attack, decay)
    return out


def shift(values, seconds, gain=1):
    offset = round(seconds * RATE)
    out = np.zeros_like(values)
    if offset == 0:
        return values * gain
    if offset < len(values):
        out[offset:] = values[:-offset] * gain
    return out


def latch(t, rng, root=820):
    # A dry pin: no shared thud, room reflection or air swell.
    return (modes(t, [root, root * 2.31, root * 3.83],
                  [.014, .007, .0035], [1, .32, .12], .00025)
            + .20 * noise(rng, len(t), 2200, 6900) * env(t, .0002, .003))


def iron(t, rng, heavy=False):
    if heavy:
        body = modes(t, [48, 79, 123, 207, 347, 601],
                     [.145, .125, .095, .065, .043, .024],
                     [.78, 1, .62, .38, .23, .13], .0011)
        crunch = np.tanh(noise(rng, len(t), 170, 3100, -.2) * 1.4)
        x = body + .63 * crunch * env(t, .00045, .036)
        x += .22 * noise(rng, len(t), 3200, 7600) * env(t, .00015, .008)
        x += shift(modes(t, [156, 273, 489], [.11, .062, .034], [.3, .2, .1]), .047)
        return x
    body = modes(t, [137, 241, 433, 761, 1387],
                 [.065, .045, .032, .017, .009], [1, .64, .32, .17, .09])
    return body + .38 * noise(rng, len(t), 550, 5800, -.2) * env(t, .0003, .013)


def ring(t, notes, decay=.3):
    x = np.zeros_like(t)
    for pitch, weight in notes:
        x += modes(t, [pitch, pitch * 2, pitch * 3.002],
                   [decay, decay * .38, decay * .15],
                   [weight, weight * .23, weight * .045], .004)
    return x


# Duration, RMS target, independent peak ceiling, runtime gain, family voice cap.
# The master does not normalize all peaks to the same level.
SPECS = {
    "S_Attack":       (.26, -20.5, -8.0, .72, 1),
    "S_Guard":        (.26, -20.0, -7.0, .58, 2),
    "S_Parry":        (.66, -21.5, -5.5, .72, 1),
    "S_Dash":         (.16, -31.0, -19.0, .75, 1),
    "S_Impact":       (.30, -21.5, -8.0, .48, 2),
    "S_Catch":        (.22, -24.5, -12.0, .66, 2),
    "S_Recall":       (.31, -26.0, -14.0, .70, 1),
    "S_Equip":        (.62, -26.0, -12.0, .68, 1),
    "S_Hurt":         (.27, -23.0, -10.0, .64, 1),
    "S_Encounter":   (1.80, -27.0, -12.0, .65, 1),
    "S_Clear":       (2.20, -28.0, -12.0, .65, 1),
    "S_EnemyFire":    (.36, -24.5, -11.0, .60, 2),
    "S_EnemyHit":     (.25, -25.0, -12.0, .36, 2),
    "S_BossTell":     (.94, -23.0, -9.0, .62, 1),
    "S_EnemyTell":    (.48, -25.5, -12.0, .60, 2),
    "S_EnemyDefeat":  (.98, -23.5, -8.0, .56, 2),
    "S_ChargeLoop":  (1.00, -20.0, -11.0, .48, 1),
    "S_ChargeTick":   (.085, -25.0, -13.0, .70, 1),
    "S_ChargeReady":  (.42, -22.5, -8.0, .68, 1),
    "S_FullRelease":  (.58, -15.5, -3.0, .66, 1),
    "S_MeleeSwing":   (.235, -25.0, -11.0, .64, 1),
    "S_HeavyImpact":  (.72, -16.5, -2.5, .64, 1),
}
RECIPES = {
    "S_Attack": "dry spring release, ceramic rim and short torsion snap; one event per partial volley",
    "S_Guard": "bright short metal clash with restrained low body",
    "S_Parry": "crossed-plate strike resolving into an open harmonic ring",
    "S_Dash": "160 ms narrow servo with three dry pins; no air, broadband swell or sub bass",
    "S_Impact": "short damped iron plate, midrange knock and localized grit",
    "S_Catch": "damped docking socket, pin engagement and tiny second lock",
    "S_Recall": "accelerating ratchet over restrained geared motor; no air rush",
    "S_Equip": "three quiet indexing locks followed by a harmonic confirmation",
    "S_Hurt": "muffled body impact with low filtered cloth displacement",
    "S_Encounter": "restrained low bronze bell with irregular decaying modes",
    "S_Clear": "soft resolving bell intervals; no new music system",
    "S_EnemyFire": "hollow ceramic resonator and expelled short pulse",
    "S_EnemyHit": "dry brittle armor chip and small loose fragments",
    "S_BossTell": "three accelerating low strained pulses into a tension crest",
    "S_EnemyTell": "rising taut-metal flex and final commitment click",
    "S_EnemyDefeat": "weighted body fall, irregular armor debris and floor contact",
    "S_ChargeLoop": "periodic energized motor, harmonic pressure and fine flux; runtime pitch supplies rise",
    "S_ChargeTick": "short two-stage selection latch, distinct from catch",
    "S_ChargeReady": "upward three-note energy confirmation with a terminal lock",
    "S_FullRelease": "magnetic rupture, low-mid recoil and flexing launch rim",
    "S_MeleeSwing": "short rim drag and strained joint movement; no launch blast or hit body",
    "S_HeavyImpact": "deep bending armor, crunch, sharp contact edge and late plate flex",
}


def synthesize(name):
    t = np.arange(round(SPECS[name][0] * RATE)) / RATE
    seed = int.from_bytes(hashlib.sha256((REVISION + name).encode()).digest()[:8], "little")
    rng = np.random.default_rng(seed)
    if name == "S_Dash":
        phase = TAU * (216 * t + 115 * t * t)
        motor = (np.sin(phase) + .22 * np.sin(phase * 3)) * window(t, .112)
        motor *= .7 + .3 * np.cos(TAU * 39 * t)
        x = .55 * motor + .38 * latch(t, rng, 1180)
        x += shift(latch(t, rng, 930), .039, .18) + shift(latch(t, rng, 690), .104, .24)
    elif name == "S_Recall":
        phase = TAU * (127 * t + 58 * t * t)
        x = .18 * (np.sin(phase) + .16 * np.sin(3 * phase)) * window(t, .266)
        for at, root, gain in [(.006, 690, .42), (.081, 775, .40), (.143, 865, .37),
                               (.194, 930, .33), (.232, 1030, .27)]:
            x += shift(latch(t, rng, root), at, gain)
    elif name == "S_Catch":
        x = modes(t, [116, 286, 641], [.036, .024, .011], [.8, .5, .15], .001)
        x += shift(latch(t, rng, 560), .020, .75) + shift(latch(t, rng, 1510), .073, .22)
    elif name == "S_ChargeTick":
        x = latch(t, rng, 1470) + shift(latch(t, rng, 2110), .027, .38)
    elif name == "S_ChargeLoop":
        # Every frequency and modulation period fits 1 s. No fade/pitch reset per loop.
        phase = TAU * 256 * t + .62 * np.sin(TAU * 7 * t)
        x = .40 * np.sin(phase) + .26 * np.sin(2 * phase) + .12 * np.sin(3 * phase)
        x *= .88 + .12 * np.cos(TAU * 3 * t)
        x += .10 * np.sin(TAU * 96 * t) + .09 * np.sin(TAU * 1024 * t + .3 * np.sin(TAU * 11 * t))
        x += .042 * np.sin(TAU * 1536 * t) * (.6 + .4 * np.cos(TAU * 13 * t))
        x += .017 * noise(rng, len(t), 950, 2100)
    elif name == "S_ChargeReady":
        x = ring(t, [(392, .24)], .13)
        x += shift(ring(t, [(587, .32)], .14), .063)
        x += shift(ring(t, [(784, .43), (1176, .07)], .17), .125)
        x += shift(latch(t, rng, 1960), .127, .40)
    elif name == "S_FullRelease":
        # Launch pressure/recoil deliberately use a different recipe from surface hits.
        pressure = np.sin(TAU * (71 * t - 17 * t * t)) * env(t, .003, .091)
        recoil = modes(t, [102, 183, 326, 513], [.105, .08, .056, .032], [.72, .45, .26, .10])
        rupture = np.tanh(noise(rng, len(t), 170, 4200, -.25) * 1.9) * env(t, .00035, .022)
        rim = modes(t, [407, 977, 1843], [.095, .037, .013], [.23, .09, .04], .001)
        x = 1.1 * np.tanh((.98 * pressure + recoil) * 1.4) + .61 * rupture + shift(rim, .015)
        x += shift(latch(t, rng, 510), .064, .17)
    elif name == "S_Attack":
        spring = np.sin(TAU * 316 * t + 1.3 * np.sin(TAU * 69 * t)) * env(t, .0012, .019)
        rim = modes(t, [227, 539, 1193], [.043, .028, .012], [.50, .26, .09])
        x = .60 * spring + rim + .32 * latch(t, rng, 760)
        x += .08 * noise(rng, len(t), 1400, 5300) * env(t, .004, .013)
    elif name == "S_MeleeSwing":
        drag = noise(rng, len(t), 640, 2550, -.3) * window(t, .145)
        joint = np.sin(TAU * (174 * t + 98 * t * t) + .40 * np.sin(TAU * 32 * t))
        x = .23 * drag + .50 * joint * window(t, .177)
        x += shift(latch(t, rng, 510), .142, .21)
    elif name in ("S_HeavyImpact", "S_Impact"):
        x = iron(t, rng, name == "S_HeavyImpact")
    elif name == "S_Guard":
        x = modes(t, [241, 547, 1283, 2399], [.050, .033, .017, .008], [.54, .68, .32, .08])
        x += .35 * noise(rng, len(t), 1000, 6500) * env(t, .00025, .010)
        x += .25 * np.sin(TAU * 108 * t) * env(t, .001, .024)
    elif name == "S_Parry":
        x = .30 * latch(t, rng, 2340)
        x += modes(t, [302, 603, 905, 1507], [.16, .27, .21, .13], [.27, .47, .26, .07])
        x += .15 * noise(rng, len(t), 1800, 6800) * env(t, .0002, .007)
    elif name == "S_Hurt":
        x = modes(t, [62, 94, 148], [.060, .039, .022], [.62, .43, .13], .003)
        x += .20 * noise(rng, len(t), 70, 750, -.6) * env(t, .002, .028)
    elif name == "S_EnemyFire":
        formant = np.sin(TAU * 151 * t + 1.5 * np.sin(TAU * 53 * t))
        x = .48 * formant * env(t, .003, .045)
        x += modes(t, [352, 871, 1499], [.065, .031, .016], [.4, .15, .08], .002)
        x += .20 * noise(rng, len(t), 260, 2500) * env(t, .001, .017)
    elif name == "S_EnemyHit":
        x = .37 * noise(rng, len(t), 760, 7300, -.25) * env(t, .0003, .013)
        x += modes(t, [316, 793, 1741], [.024, .013, .009], [.40, .23, .10])
        x += shift(latch(t, rng, 2660), .046, .11) + shift(latch(t, rng, 1970), .099, .06)
    elif name == "S_EnemyTell":
        phase = TAU * (124 * t + 46 * t * t)
        strain = np.sin(phase + .8 * np.sin(TAU * 34 * t))
        x = .5 * strain * window(t, .38) * (t / .38) ** .65
        x += .15 * noise(rng, len(t), 650, 1600) * window(t, .33)
        x += shift(latch(t, rng, 590), .338, .72)
    elif name == "S_BossTell":
        x = np.zeros_like(t)
        for at, root, gain in [(0, 89, .56), (.25, 101, .66), (.45, 116, .8)]:
            pulse = modes(t, [root, root * 2.13, root * 4.09], [.12, .095, .037], [1, .31, .10], .012)
            x += shift(pulse, at, gain)
        x += .13 * np.sin(TAU * 249 * t + .7 * np.sin(TAU * 19 * t)) * window(t, .85)
    elif name == "S_EnemyDefeat":
        x = modes(t, [59, 98, 157], [.115, .075, .046], [.8, .6, .27], .003)
        x += .30 * noise(rng, len(t), 150, 1700, -.5) * env(t, .001, .033)
        for at, root, gain in [(.13, 249, .32), (.29, 379, .24), (.41, 183, .17), (.67, 610, .07)]:
            x += shift(modes(t, [root, root * 2.7], [.034, .016], [1, .24]), at, gain)
    elif name == "S_Equip":
        x = .27 * latch(t, rng, 930) + shift(latch(t, rng, 1170), .10, .30)
        x += shift(latch(t, rng, 1470), .19, .34)
        x += shift(ring(t, [(523, .15), (784, .055)], .16), .20)
    elif name == "S_Encounter":
        x = modes(t, [87, 213, 421, 731, 1237], [.56, .36, .27, .16, .075], [.55, .30, .14, .06, .025], .005)
    elif name == "S_Clear":
        x = ring(t, [(196, .48), (392, .075)], .50)
        x += shift(ring(t, [(294, .32)], .48), .19)
        x += shift(ring(t, [(392, .28), (588, .09)], .60), .43)
    else:
        raise ValueError(name)
    return x


def master(name, values):
    if name != "S_ChargeLoop":
        # Zero-ended one-shots, no common reflection/saturation layer.
        fade_in = round(.0003 * RATE)
        fade_out = round(min(.030, len(values) / RATE * .18) * RATE)
        values[:fade_in] *= np.linspace(0, 1, fade_in) ** 2
        values[-fade_out:] *= np.linspace(1, 0, fade_out) ** 2
    else:
        values -= np.mean(values)
    _, target, ceiling, _, _ = SPECS[name]
    values *= 10 ** (target / 20) / max(rms(values), 1e-12)
    peak = np.max(np.abs(resample_poly(values, 4, 1)))
    values *= min(1, 10 ** (ceiling / 20) / max(peak, 1e-12))
    return np.rint(np.clip(values, -1, 1) * 32767).astype("<i2")


def wav_bytes(pcm):
    buffer = io.BytesIO()
    with wave.open(buffer, "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(RATE)
        wav.writeframes(pcm.tobytes())
    return buffer.getvalue()


def metrics(values):
    spectrum = np.abs(np.fft.rfft(values)) ** 2
    frequencies = np.fft.rfftfreq(len(values), 1 / RATE)
    energy = max(float(np.sum(spectrum)), 1e-12)
    cumulative = np.cumsum(values * values)
    return {
        "peak_dbfs": round(db(np.max(np.abs(values))), 3),
        "true_peak_4x_dbtp": round(db(np.max(np.abs(resample_poly(values, 4, 1)))), 3),
        "rms_dbfs": round(db(rms(values)), 3),
        "crest_db": round(db(np.max(np.abs(values))) - db(rms(values)), 3),
        "dc_dbfs": round(db(abs(np.mean(values))), 3),
        "energy90_seconds": round(float(np.searchsorted(cumulative, cumulative[-1] * .90) / RATE), 4),
        "energy_below_100hz_percent": round(float(np.sum(spectrum[frequencies < 100]) / energy * 100), 3),
        "energy_above_6000hz_percent": round(float(np.sum(spectrum[frequencies > 6000]) / energy * 100), 3),
        "energy_centroid_hz": round(float(np.sum(frequencies * spectrum) / energy), 1),
    }


def loop_metrics(values):
    delta = np.diff(values)
    seam_step = values[0] - values[-1]
    curvature = np.diff(np.tile(values, 3), 2)
    seam_curvature = abs((values[1] - values[0]) - seam_step)
    blocks = np.sqrt(np.mean(values.reshape(20, -1) ** 2, axis=1))
    return {
        "seam_step_dbfs": round(db(abs(seam_step)), 3),
        "seam_step_fraction_of_largest_internal_step": round(float(abs(seam_step) / np.max(np.abs(delta))), 5),
        "seam_curvature_fraction_of_largest_curvature": round(float(seam_curvature / np.max(np.abs(curvature))), 5),
        "minimum_50ms_rms_dbfs": round(db(np.min(blocks)), 3),
        "maximum_50ms_rms_dbfs": round(db(np.max(blocks)), 3),
        "design": "Integer oscillator/modulation cycles and periodic FFT noise; no edge fade, silent seam or baked pitch reset.",
    }


def place(mix, values, at, gain):
    start = round(at * RATE)
    end = min(len(mix), start + len(values))
    mix[start:end] += values[:end - start] * gain


def mix_checks(signals):
    """Dry linear fixtures, not an Unreal/submix or perceptual test."""
    scenarios = {
        "full_charge_release_close_heavy_contact": [
            ("S_ChargeReady", 1.80, .68), ("S_FullRelease", 1.83, .66),
            ("S_HeavyImpact", 1.89, .64), ("S_EnemyHit", 1.89, .36),
            ("S_EnemyDefeat", 2.03, .56)],
        "partial_throw_two_contacts_and_dash": [
            ("S_Attack", 0, .72), ("S_Dash", .04, .75),
            ("S_Impact", .06, .48), ("S_EnemyHit", .06, .36),
            ("S_Impact", .13, .48), ("S_EnemyHit", .13, .36)],
        "recall_six_catches_throttled_55ms": [
            ("S_Recall", 0, .70)] + [("S_Catch", .23 + i * .055, .66) for i in range(6)],
        "full_contact_guard_and_enemy_volley": [
            ("S_HeavyImpact", 0, .64), ("S_EnemyHit", 0, .36),
            ("S_Guard", .013, .58), ("S_EnemyFire", .006, .60),
            ("S_EnemyFire", .053, .60), ("S_EnemyDefeat", .047, .56)],
    }
    output = []
    for name, events in scenarios.items():
        mix = np.zeros(RATE * 4)
        if name.startswith("full_charge"):
            samples = np.arange(round(1.83 * RATE))
            progression = np.clip(samples / (1.8 * RATE), 0, 1)
            positions = np.cumsum(.7 + .9 * progression) % RATE
            loop = np.interp(positions, np.arange(RATE + 1), np.r_[signals["S_ChargeLoop"], signals["S_ChargeLoop"][0]])
            gain = .34 + .14 * progression
            gain *= np.minimum(1, samples / (.020 * RATE))
            gain *= np.minimum(1, (len(samples) - 1 - samples) / (.030 * RATE))
            mix[:len(samples)] += loop * gain
            for index in range(6):
                place(mix, signals["S_ChargeTick"], .22 + index * .316, .70)
        for asset, at, gain in events:
            place(mix, signals[asset], at, gain)
        peak = np.max(np.abs(resample_poly(mix, 4, 1)))
        output.append({"fixture": name, "true_peak_4x_dbtp": round(db(peak), 3),
                       "over_full_scale_samples": int(np.sum(np.abs(mix) >= 1)),
                       "events": len(events), "passed": bool(peak < .95)})
    pair_max = 0
    for delay_ms in range(151):
        mix = np.zeros(RATE)
        place(mix, signals["S_FullRelease"], 0, .66)
        place(mix, signals["S_HeavyImpact"], delay_ms / 1000, .64)
        pair_max = max(pair_max, np.max(np.abs(mix)))
    output.append({"fixture": "release_plus_heavy_delays_0_to_150ms", "offsets": 151,
                   "sample_peak_dbfs": round(db(pair_max), 3), "passed": bool(pair_max < .95)})
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Recreate in memory and verify exact WAV/report bytes; write nothing.")
    args = parser.parse_args()
    files, signals, encoded, checks = [], {}, {}, []
    for name, (duration, target, ceiling, gain, concurrency) in SPECS.items():
        pcm = master(name, synthesize(name))
        values = pcm.astype(np.float64) / 32768
        encoded[name] = wav_bytes(pcm)
        signals[name] = values
        measured = metrics(values)
        entry = {"asset": name, "seconds": duration, "recipe": RECIPES[name],
                 "sha256": hashlib.sha256(encoded[name]).hexdigest(), "bytes": len(encoded[name]),
                 "target_rms_dbfs": target, "true_peak_ceiling_dbtp": ceiling,
                 "suggested_runtime_gain": gain, "maximum_family_voices": concurrency,
                 "clipped_samples": int(np.sum(np.abs(pcm.astype(np.int32)) >= 32767)), **measured}
        if name == "S_ChargeLoop":
            entry["loop"] = loop_metrics(values)
            seam_ok = entry["loop"]["seam_step_fraction_of_largest_internal_step"] <= 1
            seam_ok &= entry["loop"]["minimum_50ms_rms_dbfs"] > -27
            checks.append({"check": "charge_loop_seam_and_no_silent_window", "passed": bool(seam_ok)})
        else:
            checks.append({"check": name + "_zero_endpoints", "passed": bool(pcm[0] == 0 and pcm[-1] == 0)})
        checks.append({"check": name + "_finite_unclipped_level", "passed": bool(
            np.all(np.isfinite(values)) and entry["clipped_samples"] == 0
            and measured["true_peak_4x_dbtp"] <= ceiling + .025 and measured["rms_dbfs"] < -14)})
        files.append(entry)
    mixes = mix_checks(signals)
    checks += [{"check": item["fixture"], "passed": item["passed"]} for item in mixes]
    checks += [
        {"check": "dash_average_at_least_12db_below_full_release", "passed": bool(db(rms(signals["S_Dash"])) <= db(rms(signals["S_FullRelease"])) - 12)},
        {"check": "all_asset_pcm_hashes_unique", "passed": len({f["sha256"] for f in files}) == len(files)},
    ]
    manifest = [{"asset_id": name.lower(), "path": "assets/audio/" + name + ".wav", "source_url": "",
                 "creator": "Game Studio with OpenAI Codex",
                 "license": "Original project-authored audio; no third-party samples",
                 "proof_path": "assets/audio-generation.json",
                 "modifications": "48 kHz mono PCM16; " + RECIPES[name] + "; deterministic scripts/create_audio.py",
                 "approval_status": "original"} for name in SPECS]
    report = {
        "revision": REVISION,
        "method": "Original deterministic synthesis; no recordings, external samples, presets or network generation.",
        "rate": RATE, "channels": 1, "bits_per_sample": 16,
        "generator_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "dependencies": {"numpy": np.__version__, "scipy": scipy.__version__, "purpose": "Existing local DSP/analysis libraries; no packaged runtime dependency."},
        "reproduce": "python games/dreambound/scripts/create_audio.py",
        "verify": "python games/dreambound/scripts/create_audio.py --check",
        "files": files, "dry_mix_fixtures": mixes, "checks": checks,
        "passed": all(item["passed"] for item in checks), "manifest_rows_for_integration_owner": manifest,
        "limitations": [
            "Signals measured, not heard by this agent; timbre, power and harshness still require audition.",
            "No Unreal launch, import, runtime trigger or final submix verification by this worker.",
            "Dry linear mix fixtures use documented gains/throttles; arbitrary overlap, spatialization and limiting remain separate checks.",
            "4x oversampled peak is an estimate; no integrated LUFS claim for short effects."]}
    report_bytes = (json.dumps(report, indent=2) + "\n").encode("utf-8")
    if not report["passed"]:
        raise SystemExit("FAILED: " + ", ".join(item["check"] for item in checks if not item["passed"]))
    if args.check:
        mismatches = [name for name in SPECS if not (OUT / (name + ".wav")).is_file() or (OUT / (name + ".wav")).read_bytes() != encoded[name]]
        report_path = ROOT / "assets" / "audio-generation.json"
        if not report_path.is_file() or report_path.read_bytes() != report_bytes:
            mismatches.append("audio-generation.json")
        if mismatches:
            raise SystemExit("REGENERATION MISMATCH: " + ", ".join(mismatches))
        print(f"Verified {len(files)} WAVs and report byte-for-byte; {len(checks)} checks passed. No writes.")
    else:
        OUT.mkdir(parents=True, exist_ok=True)
        for name, data in encoded.items():
            (OUT / (name + ".wav")).write_bytes(data)
        (ROOT / "assets" / "audio-generation.json").write_bytes(report_bytes)
        print(f"Created {len(files)} original WAVs; {len(checks)} signal checks passed; {sum(len(data) for data in encoded.values()):,} bytes. Audition/engine checks pending.")
    for item in files:
        print(f'{item["asset"]:18} {item["seconds"]:5.3f}s peak {item["peak_dbfs"]:7.3f} dBFS RMS {item["rms_dbfs"]:7.3f} dBFS')
    print("Mix peaks:", ", ".join(str(item.get("true_peak_4x_dbtp", item.get("sample_peak_dbfs"))) for item in mixes))


if __name__ == "__main__":
    main()

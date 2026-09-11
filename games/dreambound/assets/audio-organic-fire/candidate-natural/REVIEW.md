# Rejected palette and minimal-edit comparison

The owner listened to `../organic-fire-preview.wav` and answered **“Still sounds artificial.”** That rejects the candidate's perceived character. Its measurements, available PCM files and CC0 rights do not override that judgment. The original preview SHA-256 is `e5b268c190eb10fe9bb22c477d5b7914b78073fbec207b85d7d0566c46639129`; all eight cues and that preview are preserved byte-identical. The main audio report now explicitly records `acceptance.status = rejected_by_owner` and `import_accepted = false`. Its exact pre-feedback bytes are retained in `rejected-audio-report-before-feedback.json`.

## What was actually used

Local Python with NumPy/SciPy performed excerpting, playback-rate changes, EQ, volume envelopes, layered mixing and soft-knee peak control. ffmpeg decoded the sources; Python wrote PCM WAVs. No generative sound model, Suno generation, paid download or account operation was used.

The sources were LEGIT Audio's CC0 Fire Staff Sound Effects, qubodup's CC0 match ignition and already speed-edited Ghost breath, and mikeask's CC0 tired human breathing. The source pages establish authorship/licensing claims. They do not prove that a recording will suit this creature, or that an author-supplied sound has no prior processing.

## Processing choices to remove

These are recipe-based causal hypotheses; audio input is unavailable, so they are not listening observations.

1. **Remove the Ghost breath layer entirely.** Its creator already slowed a human breath. Reusing it as a furnace, then applying more speed changes and low-pass EQ, risks a familiar ghost/spell timbre rather than combustion in a living body.
2. **Remove the tired human breath overlays from the throws.** The quiet source is RMS-raised before mixing. Its noise and human formants can become conspicuous. Lowering its pitch does not establish a convincing nonhuman vocal anatomy.
3. **Remove the creative rate changes and uniform soft-knee shaper.** Rates from 0.65 to 1.30 shift all source resonances; the nonlinearity can add harmonics and squeeze transient contrast. Those are effects choices, not a necessary route to weight.
4. **Keep the fire's broadband detail before deciding it is too bright.** The charge low-pass cutoffs at 1.35–2.35 kHz suppress natural fire detail while retaining lower breath resonances. The near-constant low bed can then read as an artificial charging effect.
5. **Use fewer simultaneous materials.** Each throw currently combines a shifted fire burst, shifted human breath and delayed ignition tail. A single cohesive event with its original envelope is a better diagnostic starting point. Flight belongs behind the release; it should not constantly add another obvious effect.
6. **Avoid short audible repetition and prescribed swelling until the source identity works.** The 1.60-second loop and its designed volume rise may reveal the construction. First establish a convincing body/combustion source, then make the minimum cuts needed for cancellable gameplay timing.

## New comparison control

`natural-source-control.wav` is 4.8 seconds. It uses three retained fire-staff source excerpts at their supplied speed, constant per-event gain and short edge fades. It removes the breath layers, creative pitch changes, EQ, compression, looping, and separately layered travel/contact effects. A single constant master gain matches the rejected preview's peak; it does not match integrated loudness.

This is deliberately a **source/processing comparison**, not a replacement eight-cue package or a claim that the goal is met. The original production of the fire-staff pack is not documented in enough detail to call it untouched field recording. If the stripped source also sounds artificial to the owner, stop adapting this pack: source identity is then the useful next variable.

A stronger natural direction needs one convincing close combustion/air-expulsion recording with its transient and turbulent tail intact, plus a separately recorded strained breath/throat performance that already sounds bodily at normal playback speed. Keep flame and body layers separately auditionable. A purpose-built SFX generator can supply those missing source performances; it should be judged from actual listening before another integration pass. Root is investigating those tools and download limits. This worker has not generated content, spent credits or touched Suno.

`candidate-report.json` records exact source cuts/hashes and the preserved baseline identities. `build_candidate.py` reproduces only this comparison. ffmpeg decoded it successfully; no listening verdict is claimed.

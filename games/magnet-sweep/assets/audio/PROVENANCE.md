# Magnet Sweep concept-demo audio

Created 2026-09-12 for the untimed sweep, tug, linked-release and furnace loop. These are original procedural effects, not recordings or generated imitations of a named artist. No music, voice, downloaded samples, external generation service or purchased asset is used.

## Source and reproducibility

Creator: Game Studio / Codex audio-designer. The complete source is [GenerateAudio.py](../../scripts/GenerateAudio.py). It combines original inharmonic modal tones, seeded filtered noise, envelopes and short layered impacts. NumPy and SciPy perform numerical synthesis and filtering; no audio from those libraries is included. Every random generator uses a fixed seed.

From the repository root, run:

```powershell
python games/magnet-sweep/scripts/GenerateAudio.py
```

The generator writes nine WAVs, [analysis.json](analysis.json) and [manifest-rows.csv](manifest-rows.csv). The manifest rows are a handoff to the integration owner, who owns the game's authoritative asset manifest; this CSV is not a competing asset registry.

These are original project assets with no third-party sample, composition or generation-service license dependency and no required third-party asset attribution. No new public reuse license is granted by this task; the owner retains the project's distribution/licensing decision. The original synthesis source and this provenance record accompany the assets.

All files are 48,000 Hz, one channel, signed 16-bit PCM WAV. Processing consists of a 5 kHz low-pass, 65 Hz high-pass, mild soft saturation, short cosine onset/end fades and a controlled peak adjustment before quantization. Do not normalize every effect to the same loudness during import: the pickup/deposit relationship is deliberate.

## Palette and proposed engine mix

The filename gain is a linear playback multiplier applied after the file's own level. Suggested priorities are relative ordering, with higher numbers taking precedence. All are one-shots, **never loops**. The fixed-view demo can use nonspatial playback; the rig and scene do not need a listener-position-dependent attenuation system to explain these actions.

| File | Duration | Peak dBFS | Gain | Priority | Trigger and concurrency |
| --- | ---: | ---: | ---: | ---: | --- |
| pickup_01.wav | 0.185 s | -15.0 | 0.55 | 20 | Alternate the three variants on actual loose-scrap capture; no immediate repeat. At most one trigger per 60 ms and two active pickup voices across all variants. Drop surplus triggers; do not queue them. |
| pickup_02.wav | 0.185 s | -15.0 | 0.55 | 20 | Same shared pickup group. |
| pickup_03.wav | 0.185 s | -15.0 | 0.55 | 20 | Same shared pickup group. |
| latch.wav | 0.255 s | -13.0 | 0.70 | 40 | Once when button-down selects an available tangle ring. Never repeat while held or hovering. One active latch voice. |
| tug.wav | 0.430 s | -11.0 | 0.78 | 60 | Once when a valid aimed tug is committed. Begin at release; the metallic accent lands around 145 ms into the motion. One active tug voice. No cue for a canceled gesture. |
| linked_release.wav | 0.570 s | -10.0 | 0.82 | 70 | One accent if the committed Breakaway tug releases at least one directly linked neighbor. Suggested onset: first neighbor starts to break free, around 160 ms after commit. Do not trigger once per neighbor. One active voice. |
| deposit.wav | 0.970 s | -10.0 | 0.85 | 80 | Once when a deliberate, nonempty deposit transaction starts. Also used for the explicit Pour & next delivery action. One active voice; duplicate clicks must not retrigger it. |
| forge.wav | 1.160 s | -10.0 | 0.80 | 100 | Once when an earned improvement is presented, **after** the deposit's full 0.970 s payoff. Do not replay on load/resume or hover. One active voice. |
| ui_click.wav | 0.090 s | -20.0 | 0.65 | 30 | Accepted menu/settings action, one per press; no hover ticks, no repeated held-input ticks. One active voice, max one trigger per 80 ms. |

The sound character is deliberately different across events: muted little metal catches; a double contact for the latch; air/tension followed by a snap for a tug; lower body and several offset impacts for a linked burst; a dense falling cascade for the pour; and a warm furnace swell resolving to an open fifth for forging. Those are construction intentions, not reported listener reactions.

Suppress individual pickup triggers for material already represented by a tug or linked burst. Suppress them throughout a deposit. The animations may move many pieces; audio should describe the action rather than sound every mesh collision. Ordinary sweeping keeps its light rhythmic catches. There is no continuous magnet hum, so stopping collection returns the scene to silence naturally.

Use one SFX master setting, default 1.0 over these conservative gains, with mute available. Mute should affect current and future voices. Do not queue sounds while paused, muted or unfocused and replay a backlog afterward. On focus loss, cancel pending uncommitted action sounds along with the gesture. The integration owner decides whether active tails fade or finish when paused, while preserving the actual rewarded transaction and avoiding a second reward cue on resume.

Import the WAVs as small nonstreaming Unreal SoundWave assets. Engine import, packaging and actual SoundWave playback are integration work still to verify; file conformance is established here, not engine success. No SoundCue graph, music state machine or live generation dependency is required.

## Checks actually performed

- Generated and reopened every written WAV with Python's WAV decoder; all nine have the intended sample format and durations above.
- Every file has zero clipped samples and starts/ends at sample zero. Maximum absolute DC mean is 0.000275 of full scale; energy above 8 kHz is below 0.002% for every cue. These numerical checks reduce obvious discontinuity/high-frequency risks but cannot establish subjective comfort.
- The loudest individual file peaks at -10.0 dBFS. An analytical overlap of two pickup variants plus latch, tug, linked release, deposit, forge and UI at the recommended gains and aligned starts peaks at -6.354 dBFS with zero clipped samples. This is one deliberately crowded alignment, not a proof against every possible engine mix. The trigger limits above remain necessary.
- Ran the generator a second time and compared all nine WAV SHA-256 values: byte-identical. Per-file hashes, RMS levels and frequency measurements are in analysis.json.
- Invoked Windows `winsound.PlaySound` synchronously for each of the nine files. Every playback API call returned successfully.

**Listening boundary:** the agent has no perceptual feed from the host speakers. Successful local playback is not a claim that the agent heard the effects. Tonal pleasantness, fatigue during repeated sweeping, the balance of a large burst and the timing of the pour/forge must still be heard in the playable demo. No in-game trigger, imported asset, rendered gameplay mix or human listening test was performed by this audio task. Loop seams are not applicable: these effects must not loop.

## Remaining integration and listening check

1. Import the nine files, add the supplied rows to the shared manifest and bind events to successful state changes rather than animation tick counts.
2. Check repeated sweeping, a canceled latch, a direct tug, a linked burst, a nonempty deposit and the complete deposit-then-forge sequence. Empty clicks and resumed saves must remain silent.
3. Check a large haul with ordinary output volume and headphones/speakers: no harsh repeated ticks, buried forge cue, abrupt cut to the next delivery, clipping or lingering hum. Tune gains to that actual mix rather than analytical peaks alone.
4. Verify master volume/mute and pause/focus behavior in the packaged demo. Record those results in the game's QA record through its owner.

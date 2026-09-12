# Magnet Sweep rework: original sound and score

Created 2026-09-12 by Game Studio / Codex. These 19 WAVs are original procedural recordings and an original musical arrangement. They contain no downloaded sample, soundfont, external track, voice or imitation of a named artist. No paid generation service or purchase was used. Available tools were searched; no directly callable Suno or ElevenLabs generation tool was found.

## Reproduction and rights

Run `python games/magnet-sweep/scripts/GenerateReworkAudio.py` from the repository root. The complete original source is [GenerateReworkAudio.py](../../../scripts/GenerateReworkAudio.py). NumPy/SciPy synthesize, filter and encode the sound; Matplotlib renders the diagnostic plot. No audio or composition from these libraries is included. `analysis.json` records exact versions, generator hash, WAV hashes, levels and loop measurements.

The assets are original project work with no third-party audio attribution requirement or generation-service commercial license dependency. The project's public distribution/reuse license remains the owner's decision; this task does not grant a new open-source license. `manifest-rows.csv` supplies rows for the root integration owner to merge into the authoritative game manifest.

All cues are **48,000 Hz, stereo, signed 16-bit PCM WAV**. Import only the 19 WAVs below under `/Game/MagnetSweep/Audio/rework/`; names match files exactly. JSON, PNG, CSV and this document are verification/provenance, not game audio assets. WAVs total 25,208,516 bytes.

## Character and musical construction

Metal contacts combine nine inharmonic plate modes, a filtered felt/metal strike, low object body and tiny rebounding contacts. Heavy capture adds a lower pitch fall and overlapping resonances. Magnetic engagement uses a motor rise, broadband texture and relay contact; the held loop has periodic mains-like body and granular texture. Overload adds a breaker crack, low impact and individually spaced stereo fragments. Smelting combines a falling metallic cascade, broad bubbling/sizzling body, low molten movement and a cast ingot landing. These are construction intentions; pleasantness has not been heard by this agent.

**Copperlight Workshop** is a 96-second original instrumental at 80 BPM, 32 bars in 4/4. Felt/electric-style keys, soft pads, a sparse original melody, bass and a very quiet brushed pulse move through Dmaj9, Bm9, Gmaj9, A6/9sus, Em9, Gmaj9, Dmaj9/F# and A6/9. The second half introduces a higher answering phrase. Notes and release tails wrap around the entire arrangement; the final dominant harmony resolves into the opening without a fade to silence. No vocals, reference melody or third-party instrument sample is used.

Loops use periodic source synthesis, circular room tails and spectral processing. One-shots have shaped attacks/tails and zero endpoint samples. Low stereo content is centered for mono compatibility. Controlled gain retains headroom instead of normalizing every event to the same loudness.

## Exact import and initial mix contract

Gain below is a starting **linear multiplier**, followed by an SFX master around 0.9. Music uses its separate music gain directly. Priority is relative; higher events take precedence. These are proposed in-game levels, not a verified engine mix.

| File | Seconds | Peak / RMS dBFS | Gain | Priority | Trigger / concurrency |
| --- | ---: | --- | ---: | ---: | --- |
| magnet_loop.wav | 4.00 | -13.43 / -21.00 | 0.30 | 25 | Loop while attraction is active; one instance. Fade in 40 ms/out 90 ms. Never restart each tick. |
| magnet_on.wav | 0.64 | -9.06 / -22.00 | 0.50 | 40 | One field-on transition; suppress duplicate starts. |
| magnet_off.wav | 0.54 | -8.00 / -22.17 | 0.35 | 30 | One field-off transition, not each empty frame. |
| pickup_metal1.wav | 0.62 | -9.00 / -26.08 | 0.42 | 35 | Actual light contact; cycle variants without immediate repeat. |
| pickup_metal2.wav | 0.62 | -9.00 / -24.90 | 0.42 | 35 | Same shared pickup group. |
| pickup_metal3.wav | 0.62 | -9.00 / -24.52 | 0.42 | 35 | Same shared pickup group. |
| pickup_heavy.wav | 0.95 | -7.00 / -24.32 | 0.62 | 55 | One substantial bundle/contact accent; at most one per 200 ms. |
| rare_find.wav | 2.30 | -9.30 / -23.00 | 0.60 | 70 | First capture of that rare object; never replay every recapture after venting. Banking/album confirmation can use payout. |
| vent.wav | 1.08 | -7.00 / -23.28 | 0.62 | 70 | Successful deliberate ejection or material-affecting pulse; one instance, no key-repeat queue. |
| warning_loop.wav | 1.50 | -11.00 / -24.08 | 0.68 | 95 | One loop for an unstable haul. Continues after field-off; stops on real stabilization/failure. |
| overload.wav | 2.70 | -6.00 / -29.63 | 0.80 | 100 | One actual fuse failure; stop warning, suppress new pickup chatter and allow the scatter tail. |
| smelt.wav | 3.50 | -7.00 / -23.53 | 0.70 | 80 | One accepted nonempty furnace transaction. Refused unstable/empty clicks never start it. |
| payout.wav | 1.20 | -12.51 / -25.00 | 0.65 | 65 | Credit/ingot settlement, approximately 2.6 s into this smelt animation if the engine adopts that timing. |
| upgrade.wav | 3.20 | -8.00 / -22.92 | 0.68 | 85 | Once when a purchased mod visibly installs. Do not stack with a second success fanfare. |
| contract_success.wav | 3.90 | -9.00 / -23.91 | 0.65 | 90 | After the final melt/result is resolved; one instance. |
| contract_fail.wav | 1.80 | -12.86 / -25.00 | 0.65 | 75 | Contract result, distinct from the overload itself. Avoid playing both at exactly the same instant. |
| ui_click.wav | 0.12 | -15.00 / -35.50 | 0.60 | 20 | Accepted UI action, no hover ticks, max one per 80 ms. |
| furnace_loop.wav | 6.00 | -16.24 / -29.00 | 0.40 | 15 | Optional quiet furnace ambience; one instance, fade 300 ms. |
| workshop_music.wav | 96.00 | -11.87 / -25.00 | 0.40 | 10 | Continuous stereo music loop; preserve playback across ordinary heats and menus. Fade 0.8–1.5 s when starting/stopping. |

The only looping files are `magnet_loop`, `warning_loop`, `furnace_loop` and `workshop_music`. All other SoundWaves are one-shots. Music can stream; small event cues should be promptly available. Fixed-view stereo playback is sufficient initially; do not add a second random spatial pan over these authored stereo fields.

Use at most **two light pickup voices** and one heavy contact voice. Rate-limit light contact to one event per 55–70 ms; drop excess triggers instead of queuing them. Suppress redundant little clinks when one heavy bundle accent already describes the same contact. Mild pickup pitch variation 0.96–1.04 is enough; keep music, reward harmony and warning pitch stable. Load can lift the hum gain slightly, but never use its pitch as the only danger signal.

During warning, gently lower music 3 dB and magnet body 2 dB; during overload lower music about 6 dB for the impact, then restore smoothly. Do not repeatedly restart the warning pulse as its fuse changes. Keep separate music/SFX controls, a master mute that affects active loops and new sounds, and pause/focus fades with no backlog replay. No looping sound survives a stopped game action solely because an AudioComponent was left running.

The smelt waveform has incoming clatter in roughly the first second, sustained warm melt through the middle and a casting contact around 2.6 s. Match animation/settlement to those phases or edit the event structure deliberately. Do not impose a 3.5-second gameplay lock just because the tail has that length; let the sound finish while the result remains visible.

## Verification actually performed

- Generated and reopened all 19 PCM WAVs. Every file is 48 kHz/stereo/PCM16 with zero clipped samples. The loudest 4× interpolated true peak is -5.884 dBTP (`overload`), retaining headroom before engine gains.
- Per-file RMS/peak/DC, spectral, stereo-correlation and SHA-256 results are in `analysis.json`. Repeated the synthesis and compared all 19 WAV hashes: byte-identical.
- Checked all four end/start joins. Their sample step is 0.097–1.319 times the 95th-percentile ordinary waveform step, rather than an anomalous jump. Music's join is -51.93 dBFS; warning's is -67.33 dBFS. Mathematical continuity is not a subjective no-click listening claim.
- Rendered and visually inspected `audio-diagnostics.png`: representative loop joins remain smooth; heavy capture has low body and decaying metallic bands; smelting contains differentiated arrival, sustained melt and final contact; the full music cue has changing phrase-level energy without a silence gap.
- A constructed 12-second crowded mix using the proposed gains peaks at -9.599 dBFS with zero clipped samples (`mix-check.json`). This is one offline event sequence, not a guarantee for arbitrary engine overlaps.
- FFmpeg successfully decoded/transcoded a music preview. Forwarding that audio to the model returned **“audio content omitted because you do not support audio input.”** No perceptual listening claim is possible in this tool session. No host-speaker API call has been mislabeled as listening.

Engine imports/triggers, speaker/headphone comfort, warning masking, sustained-loop fatigue and the owner's assessment of music remain to verify in the actual reworked build. Keep that boundary explicit: numerical checks and an original arrangement cannot certify that the palette sounds good to Klaus.

# Suno shield sound sources

Revision `reverie-suno-v1`, September 10, 2026. Two edited gameplay WAVs are delivered for root's engine import and Klaus's audition: `S_ChargeLoop.wav` and `S_FullRelease.wav` under `assets/audio-reverie/`. The other 21 cue files remain byte-identical. This worker changed audio recipes/assets and notes only; root owns engine integration, the shared manifest and build evidence.

## Delivered sources and rights

Klaus supplied two actual downloaded MP3s in `assets/audio` and explicitly invited experimentation. Their untouched bytes are retained under `audio-reverie/sources/suno/` with clean names; owner originals are unchanged. Embedded Suno tags confirm the IDs and creator. `two-cue-integration.json` retains the complete source hashes, tags, download evidence, recipes, measurements and the two manifest rows root should integrate.

| Retained original | Suno generation | Bytes | SHA-256 |
|---|---|---:|---|
| `servo_motor.mp3` | `b1506538-bb54-418b-bf43-ba080b6dabb4` | 334,316 | `edfafbe5a8592250b8c152a03c537a359a8e3c5ccf7427acbca1d27a07a2dbcf` |
| `shield_release.mp3` | `de39cd92-d524-4698-ba42-9af66a9c1963` | 126,837 | `9fdf3917c944fd0e427966ab16118761470326bb516d554e5c46b7d5f62295fd` |

The existing Pro account was verified before generation. The release's official unlock and completed download were also observed by the agent; the motor's download was provided by the owner, not observed by this worker. This is paid-tier Suno output, **not CC0**. Source URLs and applicable [Suno terms effective September 3, 2026](https://suno.com/terms-of-service) remain attached to the assets. No purchase, upgrade, publication or share was made. Other generated alternatives remain unevaluated. Historical browser-delivery problems are retained in the generation reports; no browser work was needed for these supplied files.

## Exact edits and checks

All source-region times refer to FFmpeg's decoded PCM timeline, which excludes codec padding. Container durations are 13.488 s and 4.440 s; decoded durations are 13.4535 s and 4.4135 s respectively.

| Cue | Edit | Measured result |
|---|---|---|
| Charge loop | Motor 5.04-7.29 s; 65 Hz highpass, 3.2 kHz lowpass; no per-region edge fade; 250 ms linear overlap to a 2 s loop. Existing runtime pitch supplies the charge rise. | Mono 48 kHz PCM16; peak -13.00 dBFS, RMS -22.87 dBFS, estimated true peak -12.99 dBTP. |
| Full release | Attack 0.437-0.717 s, 150 Hz highpass / 9 kHz lowpass, 1.5 ms attack and 60 ms tail fade. Its own body 1.30-2.43 s starts 18 ms later at -7 dB relative layer gain, 55 Hz highpass / 4.2 kHz lowpass, 16 ms attack and 280 ms tail fade. The final edit lasts 1.15 s with a 120 ms end fade. | Mono 48 kHz PCM16; peak -5.50 dBFS, RMS -18.50 dBFS, estimated true peak -5.44 dBTP. Starts and ends at zero. |

The release edit brings the source attack forward and moves its delayed body underneath it, avoiding the original lead-in and long separated bloom. Both cues use the existing authored-level preparation function; full-release RMS target is -18.5 dBFS. Motor's retained -24 dBFS target is a floor in that function, not an exact loudness match: the actual result is -22.87 dBFS and is reported as measured.

The loop boundary step is 0.001257, **23.3%** of its ordinary internal 99.9-percentile step; the quietest 50 ms window is -26.67 dBFS, with no silent gap. Neither cue has full-scale samples. Four fixed dry overlap fixtures pass, with maximum estimated true peak **-1.44 dBTP**. These finite checks do not prove every engine mix safe or establish perceived quality.

`python games/dreambound/scripts/prepare_reverie_audio.py` rendered the palette successfully. One subsequent `--check` passed exact-byte regeneration. Baseline hashes prove only the two assigned WAVs changed. Runtime gains, priorities, caps and event timing remain unchanged.

## Preview and remaining audition

[Play the 11-second two-cue preview](two-cue-preview.wav). It uses the exact edited WAVs at runtime cue gains: three constant-pitch motor loops from 0-6 s, the release at 6.35 s, then a charge pitch/gain ramp followed by release from 8.1 s. Only the two edited cues are present; this is a local preview, not an engine recording. `two-cue-preview.json` records timing, PCM hashes and measurements. Rebuild with `python games/dreambound/assets/audio-suno/build_two_cue_preview.py`.

**Listening limitation:** audio input is unsupported in this worker runtime. Signal plots and measurements were inspected, but the worker has not heard these edits and cannot confirm whether their character is satisfying, musical or distracting. Root should import only these two cues and make the bounded in-engine audition for Klaus. The remaining Suno candidates and the wider palette need no new generation for this experiment.

## Runtime relationships

Keep the existing event gates and pitch behavior. Play full release once per actual six-piece launch at gain .66, priority 90 and global cap 1. Heavy contact uses .64, priority 90, cap 1 and the existing 160 ms gate. Ordinary contact uses .48, cap 2 and the existing 70 ms coalescing. Guard contact uses .85, priority 100 and cap 2; expansion/retraction uses only .20. Parry replaces ordinary guard feedback, .90 gain, priority 100, cap 1. Do not stack both defense cues for one parry.

Charge is one continuous looping component, gain `.34 + .14 * progress`, pitch `.7 + .9 * progress`, with 20 ms start / 30 ms release fades. Stop on release, cancellation, pause, focus loss, death or restart. Keep the separate count tick and once-only six-piece ready cue readable over it. Recall fires only when a piece moves; catch remains tied to actual docking with a 55 ms gate and cap 2. Preserve positional attenuation for hostile/contact cues. There is no new music pipeline.

Use mono 48 kHz PCM16 for final Unreal cues, loop only the charge and existing water ambience. Retain the current persistent 85% default master control, mute and plus/minus controls. Do not normalize imported cues again. Compare representative six-piece release/contact and guard/parry overlap at actual runtime gains; a finite dry fixture cannot prove every engine mix safe. After integration, check startup/stop/pause/silence behavior and actual short in-game triggers. Root owns those checks.

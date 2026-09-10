# Suno shield sound sources

September 10, 2026. This directory is owned by the Suno audio worker; root owns final audio recipes, importer/runtime, the shared manifest and build evidence. Four prompt batches produced eight alternatives in the owner's signed-in Suno Sounds session. The generation report records actual IDs, lengths and source links. They are original text-prompt generations, with no uploaded audio or remixes.

## Current handoff

The first full-release output `de39cd92-d524-4698-ba42-9af66a9c1963` is officially unlocked. Suno's Download UI produced a completed 126,837-byte MP3 download event, but the subagent browser's supported download API returned no artifact. WAV was also requested through the official UI. Root is resolving delivery; the worker has stopped browser control. **No local audio file, signal inspection, usable excerpt or listening result is claimed yet.** The other seven alternatives have not been unlocked.

Keep untouched official downloads in ignored `.local/suno-reverie/`, then record their SHA-256 and bytes here. Put only short selected lossless source excerpts and their precise source regions in this directory. The audio manifest must identify Suno paid-tier generation, the prompt/ID, permitted-download evidence and subsequent edits. These sources are not CC0. Generation without a permitted download is not sufficient for commercial use under the current [Suno terms](https://suno.com/terms-of-service); the [paid rights page](https://help.suno.com/en/articles/9601665) also applies. The account UI showed Pro with 27 downloads available before the first unlock. No purchase, upgrade, publication or share occurred.

## Edit targets once downloads are available

| Source role | First alternative | Game use | Target edit / level |
|---|---|---|---|
| Full release | `de39cd92-d524-4698-ba42-9af66a9c1963` | `S_FullRelease`; possibly a lighter partial-attack layer | Trim to the first complete transient, preserve 2–5 ms before attack; 0.9–1.2 s total, 25–45 ms end fade. Target roughly -17.5 dBFS RMS, true peak at or below -5 dBTP. Avoid a long pre-launch swell. |
| Charge motor | `2cff33ee-3f25-49c4-abcb-805deba1c9f7` | `S_ChargeLoop`; mechanism body | Select a steady 2.25 s region, overlap 250 ms to a 2 s loop; retain motion through the boundary. Approximately -24 dBFS RMS, true peak at or below -8 dBTP. Reject a musical beat or repeated hard hits. |
| Heavy impact | `8f74d0dd-52a2-45da-840f-b4a3c9096052` | `S_HeavyImpact`; quieter ordinary contact variant only if distinct | Use one hit, not a sequence; 0.8–1.1 s total. Approximately -19 dBFS RMS, true peak at or below -5 dBTP. Keep bronze body and crystal debris without a long metallic whistle. |
| Guard/parry | `ca1434c5-54fa-47a4-bc2a-dd2198cc05df` | `S_Guard` and brighter `S_Parry` | Guard 0.45–0.6 s; parry up to 1.2 s. Fast contact onset, short resonance. True peak at or below -6 dBTP because defense uses higher runtime gain. Preserve an obvious difference from launch and heavy impact. |

These are edit targets, not measurements or accepted selections. The second alternative for each role is recorded in `generation-report.json`. Prefer the shorter parry alternative initially: the other has an 11 s UI duration and may contain a sequence. Do not infer voices, musical content or perceived quality from duration alone. Audition where supported; the prior worker's runtime explicitly did not support audio input.

Two optional next source prompts, if the four downloaded families do not provide enough mechanism/threat variety:

- **Recall/docking, One-Shot:** `One compact sci-fi magnetic recall: six small heavy bronze ceramic shield plates whip inward with a tight reverse-air suction, fast servo zip, and a satisfying solid clack as they dock. Precise physical mechanism, glass dust accent, warm metal body, under one second, isolated dry close game SFX. No music, tune, beat, voices, singing or ambient background.`
- **Enemy warning, One-Shot:** `One ominous armored construct attack warning: strained bronze gears lock, a short rising glass-energy rasp tightens, then a low mechanical knock. Distinct readable half-second anticipation before a dangerous strike. Dry close fantasy-tech game SFX, narrow controlled high end, no explosion or long reverb. No music, melody, rhythmic beat, voices or singing.`

## Runtime relationships

Keep the existing event gates and pitch behavior. Play full release once per actual six-piece launch at gain .66, priority 90 and global cap 1. Heavy contact uses .64, priority 90, cap 1 and the existing 160 ms gate. Ordinary contact uses .48, cap 2 and the existing 70 ms coalescing. Guard contact uses .85, priority 100 and cap 2; expansion/retraction uses only .20. Parry replaces ordinary guard feedback, .90 gain, priority 100, cap 1. Do not stack both defense cues for one parry.

Charge is one continuous looping component, gain `.34 + .14 * progress`, pitch `.7 + .9 * progress`, with 20 ms start / 30 ms release fades. Stop on release, cancellation, pause, focus loss, death or restart. Keep the separate count tick and once-only six-piece ready cue readable over it. Recall fires only when a piece moves; catch remains tied to actual docking with a 55 ms gate and cap 2. Preserve positional attenuation for hostile/contact cues. There is no new music pipeline.

Use mono 48 kHz PCM16 for final Unreal cues, loop only the charge and existing water ambience. Retain the current persistent 85% default master control, mute and plus/minus controls. Do not normalize imported cues again. Compare representative six-piece release/contact and guard/parry overlap at actual runtime gains; a finite dry fixture cannot prove every engine mix safe. After integration, check startup/stop/pause/silence behavior and actual short in-game triggers. Root owns those checks.

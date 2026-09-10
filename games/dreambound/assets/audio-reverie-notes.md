# Reverie sound replacement

Revision `reverie-foley-v2`, 2026-09-10. Twenty-three finished mono, 48 kHz PCM16 WAVs replace all 22 gameplay cues and add a water loop. They use 20 newly sourced CC0 foley, mechanical and environmental samples. None of the old shield WAVs or old synthesis recipes are inputs. This is a **licensed-sample fallback while Suno sign-in is pending**, not a Suno-generated package.

## Source and rights

Official [Kenney Impact Sounds](https://kenney.nl/assets/impact-sounds) and [Sci-Fi Sounds](https://kenney.nl/assets/sci-fi-sounds) packs include CC0 notices permitting commercial projects. Selected original OGGs and both notices are retained under `audio-reverie/sources/`. Water uses [RandomMind's Vistula river recording](https://opengameart.org/content/sea-and-river-wave-sounds), explicitly CC0 on the author's page; a compact lossless excerpt, original download hash and license evidence are retained. Large downloaded packs, the full river recording and the reproducible review reel stay local and ignored. Credit is optional for these CC0 sources; retain creator names in the manifest regardless.

Explicit source regions, speed/pitch changes, EQ, layering and levels are in `audio-reverie/recipes.json`. Peak-only normalization made the first edits too thin, so important contacts, charge and confirmations use a bounded soft knee to retain body under independent peak ceilings. Small movement cues preserve their natural crest. `audio-report.json` supplies source hashes, final measurements and `manifest_rows_for_integration_owner`; root edits the shared manifest.

Regenerate with `python games/dreambound/scripts/prepare_reverie_audio.py`; check exact WAV bytes without writing using `--check`. FFmpeg, NumPy and SciPy are preparation dependencies, not game dependencies. Do not normalize these WAVs again on import.

## Integration and event hierarchy

Import only `S_*.wav` to `/Game/Audio/Reverie/S_Name.S_Name`. The player mechanism/charge cues are local; enemy/contact effects retain spatial playback. Only `S_ChargeLoop` and `S_WaterLoop` loop. Runtime gains below reflect the actual C++ calls, including stronger defense/enemy gains than the old audio note listed. Enforce the suggested global voice caps through engine concurrency. Priority 100 protects defense/damage, 90 covers major actions, 70 ordinary actions/threats, 40 mechanisms, 25 rewards and 10 ambience.

| Cue | Seconds | Runtime gain | Voice cap | Priority / event |
|---|---:|---:|---:|---|
| S_ChargeLoop | 2.00 | .34–.48 | 1 | 70; one continuous charge component |
| S_ChargeTick | .10 | .70 | 1 | 40; newly selected piece |
| S_ChargeReady | .58 | .68 | 1 | 90; all six selected, once |
| S_FullRelease | .90 | .66 | 1 | 90; actual six-piece launch, once |
| S_Attack | .42 | .72 | 1 | 70; one actual partial volley |
| S_MeleeSwing | .30 | .64 | 1 | 40; actual arm swing |
| S_HeavyImpact | 1.05 | .64 | 1 | 90; confirmed heavy/full-volley contact |
| S_Impact | .42 | .48 | 2 | 70; confirmed ordinary contact |
| S_Guard | .52 | .85 | 2 | 100; block; .20 for expansion/retraction |
| S_Parry | 1.20 | .90 | 1 | 100; perfect guard replaces normal block |
| S_Dash | .20 | .75 | 1 | 40; successful dash |
| S_Recall | .68 | .70 | 1 | 40; recall moves at least one piece |
| S_Catch | .26 | .66 | 2 | 40; actual docking |
| S_Equip | 1.05 | .80 | 1 | 25; reward; .35–.38 for rebuild/core change |
| S_Hurt | .62 | .80 | 1 | 100; unguarded player damage |
| S_EnemyFire | .62 | .78 | 2 | 70; actual hostile bolt release |
| S_EnemyHit | .30 | .55/.92 | 2 | 40; ordinary/heavy armor chip |
| S_EnemyTell | .62 | .65/.80 | 2 | 70; attack commitment; .80 for caster |
| S_BossTell | 1.10 | .60 | 1 | 90; heavy/boss warning |
| S_EnemyDefeat | 1.65 | .95 | 2 | 70; once at collapse |
| S_Encounter | 2.50 | .55 | 1 | 25; encounter begins |
| S_Clear | 2.40 | .80 | 1 | 25; earned completion |
| S_WaterLoop | 12.00 | .55 near source | 2 | 10; continuous water ambience |

Keep charge pitch `.7 + .9 * progress`, gain `.34 + .14 * progress`, 20 ms start fade and 30 ms release fade. Do not restart the loop per piece. Stop it on release, cancellation, pause, focus loss, death and restart. Actual selection-tick pitch is `.88 + selectedCount * .055`; full-release stays at 1.0. Existing swing variation is 1.0/1.1, finisher .87 and heavy .78; partial launch is .92. Recall remains 1.0 and emergency catch .8. Retain the existing kind/action pitches for hostile warnings rather than randomizing their identity.

Retain the implemented 70 ms ordinary-contact coalescing, 160 ms heavy-contact gate, 85 ms per-enemy hit gate and 55 ms catch gate. Prevent full-volley damage from spawning six copies of its major sound. Cap aggregate collapse/fire voices so earned room-clearing combinations cannot produce unlimited audio overlap. Existing enemy body/landing actions also use `S_Impact`; include them in the shared contact cap. These audio limits must not suppress gameplay events.

Water is one persistent component per fountain, smooth spatial attenuation and at most two audible voices. Root reports 400 cm inner radius plus 900 cm falloff integrated in the scene. Start/stop with the generated world, fade for .5 seconds, and pause with the world. Water sits below combat; there is no new music system.

The old source had no master/effects level or mute control despite the previous note's claim. Root now reports a persistent pause-menu master level (85% default), plus/minus and mute using the audio device primary volume. This worker did not modify those engine/UI files or verify their playback behavior; root owns that integration check.

## Verified and unverified evidence

Rendering and one exact-byte regeneration check passed for all 23 WAVs (3,024,052 bytes total). All hashes differ; all one-shots start/end at zero; no output reaches full scale. Largest per-cue estimated true peak is **-4.49 dBTP**. Four representative dry overlap fixtures use actual C++ gains; the largest estimated peak is **-1.44 dBTP**, with no full-scale samples. The six-catch fixture conservatively retains every tail beyond the intended two-voice cap. These are finite fixtures, not proof that every engine mix is safe.

Both loops contain no silent 50 ms window. Charge's boundary step is 8.2% of its ordinary internal 99.9-percentile step; water's is 27.2%. Their source motion is crossfaded without fading each loop to silence. Full-release RMS is -17.5 dBFS, heavy contact -19 dBFS, smaller partial launch about -24 dBFS, charge -24 dBFS and water -31 dBFS before runtime gain. Exact measurements, transforms and overlap events are in `audio-report.json`.

**No subjective listening claim:** an actual source WAV reel was emitted through the audio tool, which explicitly reported that this runtime does not support audio input. The worker measured signals but could not hear them. Unreal playback, pause/focus cancellation, final mix clipping and subjective strength/harshness remain root/owner checks. `audition-reel.wav` is a 23-cue review montage at actual runtime gains; `audition-index.json` maps start times. It is not game content and is regenerated locally by the script.

## Suno continuation

Current [Suno Sounds](https://help.suno.com/en/articles/10625537) supports One Shot and Loop generation. Neither the in-app browser nor existing Chrome profile was signed in on September 10. The in-app tab remains at Suno's login chooser; the owner question is pending. No Suno generations, downloads, purchases, upgrades or public posts occurred.

Existing subscription use is authorized. Current [paid rights guidance](https://help.suno.com/en/articles/9601665) and [terms effective September 3, 2026](https://suno.com/terms-of-service) require a permitted download through Suno's own channel for commercial use; stream ripping is prohibited. Keep untouched official downloads, generation IDs and plan/download evidence. The pending nine-source prompt plan is `generation-plan.json`; current [monthly download allowances](https://help.suno.com/en/articles/13926209) are 20 songs on Pro or 60 on Premier. Once sign-in is available, generate selected source effects, use the Download UI, compare to the concrete fallback and change recipes where useful. Do not relabel the current CC0 files as generated.

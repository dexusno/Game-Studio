# Shield audio corrective pass

Revision `shield-mechanism-v2`, 2026-09-08. Original deterministic synthesis, no samples, presets, recordings, purchases or external generation service. The 22 source WAVs are mono, 48 kHz, PCM16. They replace the previous uniformly normalized contact/air recipes. Preserve their authored levels on import. The report supplies exact hashes, recipes, measurements and all 22 ready-to-merge manifest rows under `manifest_rows_for_integration_owner`; the integration owner updates the shared manifest.

Regenerate from the repository root with `python games/dreambound/scripts/create_audio.py`. Verify without writing with `python games/dreambound/scripts/create_audio.py --check`. NumPy 2.4.3 and SciPy 1.17.1 were already installed; neither is a packaged game dependency. Source plus the recorded library versions reproduce the WAV/report bytes.

## Integration

Import to the existing `/Game/Audio/S_Name.S_Name` paths. Only `S_ChargeLoop` loops. It is a continuously periodic one-second signal; its rise comes from runtime pitch, with independent piece-lock ticks. Use a single continuously playing component, fade in 20 ms, pitch `0.7 + 0.9 * clamp(chargeSeconds / 1.80, 0, 1)` and gain `.34 + .14 * progress`. Hold at pitch 1.6/gain .48 when fully selected. Do not restart at each piece, fade the individual loop boundaries, or apply a new normalization pass. Fade out over 30 ms on release/cancel; stop on pause, focus loss, death and restart. On resume, charge should follow the control system's deliberate cancellation policy.

Selection ticks occur only when a piece actually locks, starting at .22 s then .316 s intervals. Pitch each tick `.92 + .04 * (selectedCount - 1)`; full-ready plays once when all six are selected. Full-release plays once only when six actually leave. Partial volleys use one `S_Attack`, independent of piece count. Keep full-release pitch at 1; lowering it heavily removes the contact edge. F/held attacks have their own swing and heavy contact. Avoid layering the old launch sound onto a melee swing.

Gains below are linear multipliers applied to the authored WAV. Priority is relative: 100 = critical defense, 90 = major player action, 70 = readable threat/ordinary hit, 40 = quiet mechanism, 25 = noncombat confirmation. World enemy/contact sounds retain normal spatial attenuation; shield mechanism/charge remain local to the player. All use the existing effects/master volume and mute control.

| Asset | Seconds | Gain | Voice cap | Priority / trigger |
|---|---:|---:|---:|---|
| S_ChargeLoop | 1.000 | .34→.48 | 1 | 70; component described above |
| S_ChargeTick | .085 | .70 | 1 | 40; actual selected-piece edge |
| S_ChargeReady | .420 | .68 | 1 | 90; six selected, once |
| S_FullRelease | .580 | .66 | 1 | 90; actual six-piece launch, once |
| S_Attack | .260 | .72 | 1 | 70; actual partial launch, once |
| S_MeleeSwing | .235 | .64 | 1 | 40; held/F swing, pitch 1/.88 |
| S_HeavyImpact | .720 | .64 | 1 | 90; confirmed heavy/full-volley contact |
| S_Impact | .300 | .48 | 2 | 70; confirmed ordinary physical contact |
| S_Guard | .260 | .58 | 2 | 100; successful ordinary block |
| S_Parry | .660 | .72 | 1 | 100; perfect guard, replaces ordinary guard cue |
| S_Dash | .160 | .75 | 1 | 40; successful dash, tiny servo/pins |
| S_Recall | .310 | .70 | 1 | 40; recall command moves at least one piece |
| S_Catch | .220 | .66 | 2 | 40; actual docking, minimum 55 ms interval |
| S_Equip | .620 | .68 | 1 | 25; acquisition; .30 for reconstruction/core switching |
| S_Hurt | .270 | .64 | 1 | 100; player damage, separate from guarded contact |
| S_EnemyFire | .360 | .60 | 2 | 70; actual bolt release, one per salvo group |
| S_EnemyHit | .250 | .36 | 2 | 40; small armor chip under player contact |
| S_EnemyTell | .480 | .60 | 2 | 70; normal attack commitment |
| S_BossTell | .940 | .62 | 1 | 90; heavy/boss warning |
| S_EnemyDefeat | .980 | .56 | 2 | 70; once at collapse |
| S_Encounter | 1.800 | .65 | 1 | 25; encounter start |
| S_Clear | 2.200 | .65 | 1 | 25; earned encounter clear |

Coalesce ordinary impact requests for 70 ms per owning attack/volley; share a two-voice contact cap across heavy/ordinary player contacts. Heavy replaces the ordinary contact for its first relevant hit and has a 120 ms retrigger gate; do not play six heavy impacts from one volley. Keep the first/highest-priority contact when rejecting duplicates. `S_EnemyHit` is a quiet optional chip, capped to two voices with a 70 ms gate. Catches use a 55 ms gate and two voices; reject excess simultaneous requests, while later separated docking events can sound. Do not emit a second catch for already attached pieces. These are audio limits only and must not suppress damage, feedback visuals, regeneration or actual docking.

## Evidence and independent listening checks

Generation and read-only regeneration passed 51 signal checks. All WAVs have distinct hashes, no full-scale PCM samples, and estimated true peaks below their separate ceilings. Every one-shot begins/ends at zero. Charge has no silent 50 ms window; its periodic seam step/curvature stay within ordinary internal waveform changes. Exact measurements are in `audio-generation.json`.

Dash is peak -20.406/RMS -31.000 dBFS, with 90% of energy inside 79.2 ms and effectively no sub bass. Full-release is peak -3.006/RMS -17.202 dBFS. Four dry overlap fixtures stay below full scale (highest estimated true peak -1.167 dBTP); the release/heavy pair was checked at 151 offsets from 0–150 ms (maximum sample peak -0.721 dBFS). The fixture is deliberately conservative about six catches, summing tails beyond the recommended two-voice cap. These measurements do not cover every possible engine mix.

This worker measured the signals but did not hear them, launch Unreal, import assets or test runtime triggers. Before calling the correction successful, listen in the packaged build to: idle full charge held across several loop boundaries; rapid short/partial throws then a full charge; heavy contact at point-blank range; dash during a release; six returning pieces docking; and charge cancelled by pause/death/focus loss. Confirm that charge stops cleanly, selection remains legible, dash is small, full-release/contact feel substantially stronger than partial actions, and enemy tells/guard remain audible under overlap. Listen on ordinary speakers as well as headphones; heavy contact has substantial low-frequency body. Inspect a captured engine mix for clipping separately. No subjective sound-quality or owner-approval claim is made here.

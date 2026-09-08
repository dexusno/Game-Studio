# Cyborg between worlds — beta QA

2026-09-08. **Klaus played and rejected Shipping 0.1.0-beta2 for combat feel/identity and presentation.** Its staged checks and native menu/save evidence remain valid technical observations. They are not evidence of enjoyable play. Production fixes belong to integration/combat owners.

## Owner playtest — 2026-09-08
Klaus reported usable but generic movement; confusing/simple combat; a gun with right-button defense rather than a shield used as a weapon; missing attack/defense tradeoff and expected shield recall; no felt recoil; a thin, weak laser sound; plain/stale enemies; and repetitive, janky pillars and plant geometry far below anchor B. He found no sense of power or an engaging world. These are direct owner reports, not additional agent input tests. Seed, elapsed time, acquired loadout and full-route completion were not supplied; do not invent them.

Root compared the retained gameplay capture with anchor B and inspected world placement. The scene uses repeated columns, uniformly tiled rooms and strongly scaled root instances instead of the anchor's composed architecture, organic structure and material detail. Combat source review confirms a pulse/hitscan baseline that can fire while guarding and no physically launched/returning shield. This supports the identity mismatch; code inspection does not quantify how bad it feels. [Retrospective](RETROSPECTIVE.md).

Klaus also identified missing established antagonists and reasons to fight, and requested that story work later. No enemy faction or campaign conflict has been selected by that feedback.

## Identity and coverage

Reviewed [BRIEF.md](BRIEF.md), [IMPLEMENTATION.md](IMPLEMENTATION.md) and the game mode, HUD, character and enemy integration. Current artifact: **0.1.0-beta2, Windows Shipping**, after the menu-only rebuild. Binary: [Dreambound-Win64-Shipping.exe](BuildOutput/Shipping/Windows/Dreambound/Binaries/Win64/Dreambound-Win64-Shipping.exe), 166,044,160 bytes, written September 8 at 05:33:04 UTC. Integration-reported SHA-256: `6BE6C949F1A732278343560A81906A077F6CD88A1D53F04432A474DEF7C910BE`.

Environment: Windows, Unreal `5.8.2-56702186`. Integration reports the rebuilt Shipping package's staged driver passed 31/0 and maintains the retained [packaged report](evidence/beta-0.1.0-staged-checks.txt). QA inspected earlier packaged/editor reports; beta2 execution and native observations below are attributed to integration. The earlier editor run used `-game -NullRHI -DBVerify -DBSaveSlot=DreamboundQA_Staged -DBSeed=833282 -unattended -nosound -nosplash`. Intended controls remain WASD/mouse, LMB/RMB/Q, Shift, Space, E, 1–3, R, Tab and Escape. Staged checks called input APIs, not Windows input.

## Current startup/input boundary

The earlier Windows Security prompt is gone. Integration verified these native results on rebuilt beta2; independent QA did not operate the session:

| Input/state | Observed result on beta2 |
| --- | --- |
| Title | The oversized weapon is correctly hidden. A saved QA profile exposes Resume. |
| Resume | Returned to Expedition 1, seed 124055, Waking Cell at the entry checkpoint. The weapon was visible with normal rest framing during play. |
| Escape pause | Pause hid the weapon assembly. |
| Save/exit/relaunch | Sensitivity 1.10 survived. Resume restored the saved expedition as above. |

Earlier native observations on beta1 remain useful limited evidence, rather than automatic beta2 rechecks:

| Input | Observed result |
| --- | --- |
| Begin expedition | Entered the Waking Cell normally. |
| Mouse movement/click and LMB | Aim visibly changed; firing raised heat. Enemy damage was not established by this observation. |
| Escape and pause plus button | Pause opened/closed; sensitivity changed from 1.00 to 1.10. Persistence was not checked. |
| Tab | Build inspection opened/closed. |
| Q | Impact pose visibly began; a successful combat hit was not established. |
| W and Shift_L/Shift taps | Movement was not established. `@oai/sky` provided taps/chords without a hold duration, so this leaves movement coverage unresolved; it does not prove broken movement or a playable whole journey. |

The Shipping game requires no network for this offline beta. Integration's read-only `Get-NetTCPConnection`/`Get-NetUDPEndpoint` check of Shipping PID 85008 reported zero TCP and zero UDP endpoints at that snapshot; this is not a claim about every process or all times.

Beta1's title showed an oversized weapon despite camera priming. `DBGameMode::SetMenuInput` now hides the player assembly in menus and restores it during play; beta2's observed title, pause and resumed-play framing confirm that correction. Beta1's native Save checkpoint/exit also closed the process normally. An attempted minimize while the mouse was captured changed aim, so neither minimize nor focus-loss handling counts as tested.

## Findings and source rechecks

Initial findings came from source review; the status column distinguishes source checks from staged engine rechecks. None is an ordinary play session. Function names identify the affected paths as line numbers move during integration.

| Priority | Finding and reproduction | Expected / source evidence | Status |
| --- | --- | --- | --- |
| High | Additional reinforcement waves after backtracking. In Rootwalk/Bell Keep, reach wave two, enter the previous room, return, then defeat the remaining enemies. | The encounter should finish after its finite second wave. Initial `DBGameMode::ActivateRoom` reset global `CurrentWave` even when the room already existed; `NotifyEnemyKilled` could spawn wave two again. | Per-room `RoomWaves` fix passed staged second-wave re-entry checks for rooms 3 and 4. |
| Medium | Maximum-rank reward consumed without an upgrade. Reach Mirror rank three, then claim Mirror at its trial/another offered ward. | A reward should have a real disclosed benefit. Initial `ShowOffers` included capped upgrades; HUD advertised rank four, `DBCharacter::ApplyUpgrade` returned at rank three, and `ClaimReward` still consumed the ward. | Capped offer exclusion, restoration fallback and duplicate-claim checks passed in the real world. Labels checked in source; rendered menu inspection pending. |
| Medium | Newly earned patterns unavailable on immediate first-session retry. Start a clean profile, earn a pattern, die, choose New/Same. | Earned starting equipment should be selectable without relaunching. `StartingPattern` initially remained None after acquisition; its only selector was on the title screen, while results started directly. | First-pattern selection and same-seed retained-pattern reset passed staged execution. End-screen cycling remains source-checked, not clicked. |
| Medium | Save failure hidden from the player. Deny a test slot write, claim a reward or choose Save checkpoint/exit. | Failed persistence should be visible before exit or relying on a checkpoint. Initially `SaveNotice` only reached internal state/logs; Quit proceeded. | `bSaveFailed` now blocks ordinary exit/new-run loss and exposes a HUD banner, retry and explicit unsaved exit. Source recheck supports the fix; runtime retest pending. |
| Medium, geometry risk | Tech-room boundary gaps. Walk between perimeter panels in Waking Cell/Rainstack. | Room boundaries should not expose accidental exits into the void. Initial 400 cm panel spacing against 239 cm mesh width implied 161 cm gaps. | X scale 1.75 fix passed actual player-capsule seam sweeps in both tech rooms and both tested layout parities. Ordinary traversal remains pending. |

Rebuilt beta2 packaged staged result, reported by integration: **31 passed, 0 failed** (integration checks 9/0; independent driver 22/0). Coverage includes guard direction/timing/capture, Q consumption, pause intent, imported corridor/tech-edge physics, wave re-entry, reward caps/claims, replay, journal roundtrip and CRC-failure recovery. Native observations above are separate, limited checks; held movement/dash, focus behavior, audio listening and a whole journey remain unverified.

## Save-recovery fixture correction — recheck passed

[DBRuntimeChecks.cpp](unreal/Source/Dreambound/DBRuntimeChecks.cpp) requires an isolated QA flag/slot, backs up/restores its journal and suppresses end-play saving. The earlier 23:39 UTC run reported 30/1 because of a **fixture defect**: Unreal's `SaveDataToSlot` rejects empty arrays (`GameplayStatics.cpp:2394`), so truncation never occurred and the valid newest checkpoint remained intact. The failure did not demonstrate a loader defect.

The corrected fixture flipped one checksum byte in the newest journal's CRC envelope while preserving its serialized payload. The packaged recheck passed: mutation succeeded on 2,651 bytes; revision 18 was expected and selected; restored health was 73, Ram rank 0, learned Ram absent, Frost rank 2, and boss milestone retained. The previous usable checkpoint and equipment were reconstructed through ordinary load/resume methods. QA journal backup/restoration also passed. This verifies the staged CRC-rejection path, not all possible disk failures or the visible save-error interface.

## Remaining evidence gaps

Beta2 identity, post-fix menu framing and saved-profile relaunch/resume are recorded above. Agent native evidence still does not establish sustained movement, dash/jump/guard, focus capture/release or minimize, ordinary acquisition/combat, save-error UI or a completed route through the guardian and Rainstack. The owner's usable-movement and negative combat/feel report is separate evidence; it does not supply a full-route result. Future QA should retain isolated `DBQA_` save slots and distinguish ordinary input from staged actor/state setup.

Owner listening/interaction feedback is negative on weapon sound, power and enjoyment. Agent listening, measured human pacing (including the provisional 15–25 minutes), completed-route difficulty and voluntary replay remain unverified. Staged checks and brief native actions cannot replace those observations.

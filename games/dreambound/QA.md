# Cyborg between worlds — beta QA

2026-09-08. **The Shipping beta is built and its packaged staged checks pass; normal-input readiness remains blocked.** No ordinary whole journey, audio listening or human fun result is claimed here. QA owns this report; production fixes belong to integration/combat owners.

## Identity and coverage

Reviewed [BRIEF.md](BRIEF.md), [IMPLEMENTATION.md](IMPLEMENTATION.md) and the game mode, HUD, character and enemy integration. Final candidate: **beta 0.1.0, Windows Shipping**, built successfully. Binary: [Dreambound-Win64-Shipping.exe](BuildOutput/Shipping/Windows/Dreambound/Binaries/Win64/Dreambound-Win64-Shipping.exe), 166,044,160 bytes, written September 8 at 00:03:18 UTC. Read-only SHA-256 verification: `2DF5EF573C7DACC94D94732F57E15BCE37BC7A8D86678EAC0B26BD2AC1731F76`.

Environment: Windows, Unreal `5.8.2-56702186`. Integration executed the actual Shipping package's staged driver; QA inspected the retained [packaged report](evidence/beta-0.1.0-staged-checks.txt), which identifies isolated slot `DreamboundQA_Shipping`. The earlier editor run also passed under `UnrealEditor-Cmd.exe` with `-game -NullRHI -DBVerify -DBSaveSlot=DreamboundQA_Staged -DBSeed=833282 -unattended -nosound -nosplash`; editor evidence is superseded by the packaged report. Intended controls remain WASD/mouse, LMB/RMB/Q, Shift, Space, E, 1–3, R, Tab and Escape. Staged checks called input APIs, not Windows input.

## Current startup/input boundary

Integration observed the packaged title startup through Computer Use. Normal input testing is blocked by an orphaned Windows Security network prompt from a prior Development build's trace port 1985. The owner has been asked to Cancel it because the computer-use skill prohibits the agent from handling that security prompt. It has not been bypassed or counted as a completed input test.

The Shipping game requires no network for this offline beta. Integration's read-only `Get-NetTCPConnection`/`Get-NetUDPEndpoint` check of Shipping PID 85008 reported zero TCP and zero UDP endpoints at that snapshot; this is not a claim about every process or all times.

Integration fixed initial paused-camera origin by setting the player view target and calling `PlayerCameraManager->UpdateCamera` before pausing. The source fix precedes this Shipping binary; ordinary title/camera/controls recheck remains pending. Offscreen packaged capture was underway at this handoff and is not counted as a finished visual assessment.

## Findings and source rechecks

Initial findings came from source review; the status column distinguishes source checks from staged engine rechecks. None is an ordinary play session. Function names identify the affected paths as line numbers move during integration.

| Priority | Finding and reproduction | Expected / source evidence | Status |
| --- | --- | --- | --- |
| High | Additional reinforcement waves after backtracking. In Rootwalk/Bell Keep, reach wave two, enter the previous room, return, then defeat the remaining enemies. | The encounter should finish after its finite second wave. Initial `DBGameMode::ActivateRoom` reset global `CurrentWave` even when the room already existed; `NotifyEnemyKilled` could spawn wave two again. | Per-room `RoomWaves` fix passed staged second-wave re-entry checks for rooms 3 and 4. |
| Medium | Maximum-rank reward consumed without an upgrade. Reach Mirror rank three, then claim Mirror at its trial/another offered ward. | A reward should have a real disclosed benefit. Initial `ShowOffers` included capped upgrades; HUD advertised rank four, `DBCharacter::ApplyUpgrade` returned at rank three, and `ClaimReward` still consumed the ward. | Capped offer exclusion, restoration fallback and duplicate-claim checks passed in the real world. Labels checked in source; rendered menu inspection pending. |
| Medium | Newly earned patterns unavailable on immediate first-session retry. Start a clean profile, earn a pattern, die, choose New/Same. | Earned starting equipment should be selectable without relaunching. `StartingPattern` initially remained None after acquisition; its only selector was on the title screen, while results started directly. | First-pattern selection and same-seed retained-pattern reset passed staged execution. End-screen cycling remains source-checked, not clicked. |
| Medium | Save failure hidden from the player. Deny a test slot write, claim a reward or choose Save checkpoint/exit. | Failed persistence should be visible before exit or relying on a checkpoint. Initially `SaveNotice` only reached internal state/logs; Quit proceeded. | `bSaveFailed` now blocks ordinary exit/new-run loss and exposes a HUD banner, retry and explicit unsaved exit. Source recheck supports the fix; runtime retest pending. |
| Medium, geometry risk | Tech-room boundary gaps. Walk between perimeter panels in Waking Cell/Rainstack. | Room boundaries should not expose accidental exits into the void. Initial 400 cm panel spacing against 239 cm mesh width implied 161 cm gaps. | X scale 1.75 fix passed actual player-capsule seam sweeps in both tech rooms and both tested layout parities. Ordinary traversal remains pending. |

Final packaged staged result: **31 passed, 0 failed** (integration checks 9/0; independent driver 22/0). Coverage includes guard direction/timing/capture, Q consumption, pause intent, imported corridor/tech-edge physics, wave re-entry, reward caps/claims, replay, journal roundtrip and CRC-failure recovery. No staged failure remains. Normal focus/input and menu interactions, audio listening, post-camera-fix ordinary startup and a whole journey remain unverified; the earlier title startup observation is attributed to integration.

## Save-recovery fixture correction — recheck passed

[DBRuntimeChecks.cpp](unreal/Source/Dreambound/DBRuntimeChecks.cpp) requires an isolated QA flag/slot, backs up/restores its journal and suppresses end-play saving. The earlier 23:39 UTC run reported 30/1 because of a **fixture defect**: Unreal's `SaveDataToSlot` rejects empty arrays (`GameplayStatics.cpp:2394`), so truncation never occurred and the valid newest checkpoint remained intact. The failure did not demonstrate a loader defect.

The corrected fixture flipped one checksum byte in the newest journal's CRC envelope while preserving its serialized payload. The packaged recheck passed: mutation succeeded on 2,651 bytes; revision 18 was expected and selected; restored health was 73, Ram rank 0, learned Ram absent, Frost rank 2, and boss milestone retained. The previous usable checkpoint and equipment were reconstructed through ordinary load/resume methods. QA journal backup/restoration also passed. This verifies the staged CRC-rejection path, not all possible disk failures or the visible save-error interface.

## Minimal runtime handoff

Use the actual Windows package and a unique `-DBSaveSlot=DBQA_<session>` so owner progress stays isolated. Record package identity, commands, inputs and observed outcome.

1. **First launch and input:** normal menu start, find/claim the first core, capture/release mouse, pulse/guard/Q/dash/jump, pause/build/reward screens, focus loss and fresh input after resume. Check prompts and audible feedback in a coordinated session.
2. **Reachability and finite encounters:** walk the main and optional routes on one seed of each layout parity; verify gates, floor/collision and tech-room edges. Leave/re-enter an active second wave and confirm the encounter clears once.
3. **Rewards and persistence:** acquire contrasting behaviors through ordinary claims, evolve one to cap, revisit its altar, and check the offered descriptions/effect. Exit mid-encounter, relaunch/resume, verify seed/claims/health/equipment/core and restart at the intended checkpoint without duplicate rewards.
4. **Failure/replay:** die after earning a pattern, select it for immediate same-seed practice/new expedition, then relaunch. Preserve learned ownership and boss milestone; confirm new-seed variation changes the journey.
5. **Save recovery:** with disposable slots, fail a write and corrupt only the newest checkpoint after keeping a backup. Verify visible failure and usable prior-checkpoint recovery; inspect restored equipment rather than just save-file fields.
6. **Completion:** defeat the guardian, claim its capacitor, carry the build into Rainstack, complete the arrival, and verify result/replay/exit. Separate ordinary play from any direct state fixture used to reach an edge case.

Automated verification that counts rooms or grants upgrades directly cannot establish traversable routes, earned reward quality or fun. Human pacing (including the provisional 15–25 minutes), challenge and willingness to try another build remain untested.

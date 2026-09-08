# Current folding shield/combat-feel QA

0.3.1-combat1 final Shipping binary independently passed **19 scenario groups /67 assertions /0 failures** in exactly one packaged suite execution. [Report](evidence/combat-feel-checks.json), [build identity and attempt record](evidence/combat-feel-build.json). QA observed a37.54cm folded socket span,49.64cm expanded span and return to folded; five-piece fixture damage168.5 versus full-six468.9; a single80-damage neighboring burst with a covered neighbor untouched; target hits from275cm, a stronger third strike, actual50cm open step and collision-limited wall step. Route coverage includes34 floor probes and sampled solid rocks in all three courts.

Independent review caught disabled movement in the new step fixture and an inconsistent wall-distance assertion. Integration corrected the fixture before the single packaged run; no scenario was weakened to hide a production failure. Additional loop-stop assertion waits for fade completion. NullRHI/nosound cannot establish audio playback: the separate rendered CSV records actual component start/stop and full-charge state.

Root inspected final rendered idle, hinge transition, expanded guard, full release and scene. The first of two capture passes prompted further resting-pose clearance and removal of a fixture-only central E prompt. Final capture uses safe practice and artificial positions/damage; it is not an ordinary run. Engine mix:35.947s, peak0.2271, zero full-scale samples; selected full release averages8.4dB above the partial window. These are signal observations; no listening-quality claim. Native Windows title/start, F strike/cooldown, Tab instructions and save/exit worked on the exact final binary in an isolated profile.

Known limits: no held OS-input/full normal playthrough, performance benchmark, completion-time measurement, exhaustive upgrade combinations or positive fun claim. Enemy/detail quality and overall environment remain substantially below B. Owner rejects a ruins-kit purchase as the general graphics solution; the newly proposed guided AI/Blender creation pipeline has no model-generation or Unreal trial evidence yet. Earlier records follow as history.

# Historical segmented shield QA

0.3.0-segments1 replaces the whole-disc study as the active experiment. The packaged staged runner covers 16 scenarios. [Current result](evidence/segmented-shield-checks.json) and [exact build/execution record](evidence/segmented-shield-build.json) are authoritative for the delivered package. Five packaged executions occurred as renderer inspection prompted camera/HUD/forearm and practice-placement fixes; all passed 16/0. This repetition is recorded with its purpose and the final hash.

Checks use ordinary world ticks and real piece sweeps/input APIs, artificial positions and lethal progression hits. Elite AI produces its actual warning/stance, which is then frozen to isolate one-piece interception. Route tests compare seed signatures and sweep the character capsule through connectors with floor probes; progression deliberately calls ActivateRoom, so that is not a normal walk through every room. Three original shield FBXs passed Blender export roundtrip and Unreal units/pivot/material import checks.

Rendered inspection found and fixed excess shield cropping, weak text contrast, difficult-to-distinguish selection lights and the newly exposed rear forearm cap. The final controlled fixture demonstrates selection, partial deployment, retained block/depletion, recall, destruction/rebuild, Frost and the retained Tab description. It grants the core and stages incoming damage, so it cannot be described as naturally earned gameplay. Frame capture affects timing/performance; audio level checks are signal measurements, not a listening assessment.

No ordinary full playthrough, all-seed navigation proof, challenge/pacing assessment, exhaustive testing of every upgrade rank/combination, sound-quality verdict or positive fun claim. The current environment remains far below B and enemy geometry remains the prior kit. Klaus's assessment is the next evidence of enjoyment. Prior prototype results follow as history.

# Current physical shield study QA

0.2.0-study1 supersedes the rejected pulse-gun beta as the active experiment. Final packaged runtime results:14 scenarios passed,0 failed. Four runs were executed as integration, capture and menu corrections landed; no scenario failed. [Exact package](evidence/shield-study-build.json), [staged result](evidence/shield-study-checks.json), [fixture scope](qa/physical-shield-checks.md).

Native normal title/start, equipment menu and save/exit were observed. The start-click leak was fixed by acting on mouse release. The final menu uses plain ASCII labels. These observations are separate from the automated fixture. Holding movement/guard, full focus-loss handling and a full normal run remain outside agent input coverage.

The actual scripted capture contains a moving opponent and an earned first clear through normal combat APIs. It is not a complete ordinary run. [Footage](evidence/shield-study-scripted.mp4), [current scene](evidence/shield-study-courtyard.png). Raw frame requests slow the renderer, so capture cadence is not a performance measurement. Engine audio recording originally omitted silent intervals through submix auto-disable; capture now retains silence. Final audio is25.92s with no clipped PCM samples. No auditory quality verdict is possible from the agent's available input.

Exposure and exported UV errors were corrected after inspecting Unreal output. Correcting UVs preserved geometry/hulls; two tiny-tangent warnings remain on the authored stair. The scene and enemy detail still fall short of the selected anchor. Owner feel, difficulty, complete pacing, upgrade enjoyment and visual acceptance are pending. No complete ordinary playthrough or positive fun claim is made.

The owner feedback and technical evidence below apply to the retained beta2 baseline.

# Cyborg between worlds — beta QA

2026-09-08. **Klaus played and rejected Shipping 0.1.0-beta2 for combat feel/identity and presentation.** Its staged checks and native menu/save evidence remain valid technical observations. They are not evidence of enjoyable play. Production fixes belong to integration/combat owners.

## Owner playtest — 2026-09-08
Klaus reported usable but generic movement; confusing/simple combat; a gun with right-button defense rather than a shield used as a weapon; missing attack/defense tradeoff and expected shield recall; no felt recoil; a thin, weak laser sound; plain/stale enemies; and repetitive, janky pillars and plant geometry far below anchor B. He found no sense of power or an engaging world. These are direct owner reports, not additional agent input tests. Seed, elapsed time, acquired loadout and full-route completion were not supplied; do not invent them.

Root compared the retained gameplay capture with anchor B and inspected world placement. The scene uses repeated columns, uniformly tiled rooms and strongly scaled root instances instead of the anchor's composed architecture, organic structure and material detail. Combat source review confirms a pulse/hitscan baseline that can fire while guarding and no physically launched/returning shield. This supports the identity mismatch; code inspection does not quantify how bad it feels. [Retrospective](RETROSPECTIVE.md).

Klaus also identified missing established antagonists and reasons to fight, and requested that story work later. No enemy faction or campaign conflict has been selected by that feedback.

## Historical beta2 identity and coverage

Reviewed [BRIEF.md](BRIEF.md), [IMPLEMENTATION.md](IMPLEMENTATION.md) and the game mode, HUD, character and enemy integration. Historical artifact: **0.1.0-beta2, Windows Shipping**, after the menu-only rebuild. Binary: [Dreambound-Win64-Shipping.exe](BuildOutput/Shipping/Windows/Dreambound/Binaries/Win64/Dreambound-Win64-Shipping.exe), 166,044,160 bytes, written September 8 at 05:33:04 UTC. Integration-reported SHA-256: `6BE6C949F1A732278343560A81906A077F6CD88A1D53F04432A474DEF7C910BE`.

Environment: Windows, Unreal `5.8.2-56702186`. Integration reports the rebuilt Shipping package's staged driver passed 31/0 and maintains the retained [packaged report](evidence/beta-0.1.0-staged-checks.txt). QA inspected earlier packaged/editor reports; beta2 execution and native observations below are attributed to integration. The earlier editor run used `-game -NullRHI -DBVerify -DBSaveSlot=DreamboundQA_Staged -DBSeed=833282 -unattended -nosound -nosplash`. Intended controls remain WASD/mouse, LMB/RMB/Q, Shift, Space, E, 1–3, R, Tab and Escape. Staged checks called input APIs, not Windows input.

## Historical beta2 startup/input boundary

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

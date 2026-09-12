# Magnet Sweep — independent QA and playtest record

**Historical first-demo report.** Klaus subsequently played and rejected this build as boring. Functional readiness below did not establish enjoyable gameplay. The active rebuild is recorded in [QA-REWORK.md](QA-REWORK.md) and STATUS.md.

Updated 2026-09-12. Independent reviewer: `magnet_demo_qa`; implementation owner: root. Only this report is reviewer-owned. Development timelines were not evaluated.

## Readiness

Ready for the owner concept playtest. The broad native pass reached both upgrades, replayed the fork with the complete rig, completed a recovered-metal delivery, and verified a real save/quit/reopen. Focused QA-B rechecks passed the corrected outward sweep and visible reward; the final QA-C config recheck restored the original window size after fullscreen. No unresolved defect remains from the covered flows. The game is closed with a fresh starter save and sound on.

This establishes functional behavior in the tested package. Human fun, voluntary replay, audience demand and profitability remain unverified.

## Identity and environment

All reviewed builds derive from baseline `ef3e1477d91e8d5f0b0f82320341cb1cdf62589f` plus the uncommitted Magnet Sweep implementation. Binary hashes identify the tested artifacts while source changes.

| Identity | Artifact / SHA-256 |
| --- | --- |
| QA-A — broad native pass | `BuildOutput/ConceptDemo/Windows/MagnetSweep/Binaries/Win64/MagnetSweep.exe`, 331,924,992 bytes, modified 2026-09-12 00:55:52 local. `890de0c15b807d16a98201c17ca4eac51be38e4dd692dd3b960a677b6178e1f2` |
| QA-B — outward sweep and reward recheck | Same packaged child executable. `332931cdb84a904cce3417811cb9768f656504cef48b0dd8397b578745e00dd5` |
| QA-C — final F11 config recheck | Child executable unchanged from QA-B. Final `MagnetSweep/Content/Paks/MagnetSweep-Windows.pak`: `317f5e82007a9569c41b30a7a857f250d73f4fa9090666f4e23b74a17758ffe8`. Config changed after QA-B; broad gameplay pass was not repeated for that isolated correction. |
| Earlier fast-drag failure | Same child executable, modified 2026-09-12 00:48:24 local. `1a4da079ede28ed52879204cbb781bf5c061204b4b2d05c4180211d3ff98b789` |
| Initial source review | `unreal/Source/MagnetSweep/WorkbenchRuntime.cpp`: `5220a110093ade4dcd4fd5636a1956576638b4d6f4510761ddd37fef8652d378` |

Windows 11 25H2 build 26200.9445; Ryzen 9 7900X, RTX 4090, 64 GB RAM. Unreal 5.8.2 Development package, observed `PCD3D_SM5` window title. Normal mouse/keyboard input was injected through the computer-use skill's `@oai/sky` APIs using observed windows/screenshots. Source, saved JSON and the engine automation report were inspected separately. No production source was edited by the reviewer.

Controls exercised: primary drag for sweep/ring tug; primary click to pour and operate menus; Escape pause; M sound; F11 display mode; F9 capture. The previously empty `player` profile was explicitly designated as disposable QA progress by the integration owner. Save: `BuildOutput/ConceptDemo/Windows/MagnetSweep/Saved/ConceptDemo/player.json`.

Acceptance: [BRIEF.md](BRIEF.md) and the first-playable/engineering contracts in [PRODUCTION-PLAN.md](PRODUCTION-PLAN.md): predictable action context, safe ownership, finite direct-link pulls, complete rewards, retained rig, contrasting arrangements and replay.

## Passed checks

| Check / method | Evidence and practical limit |
| --- | --- |
| Native launch, text and scene — QA-A | Direct packaged-child launch exposed one targetable window. All HUD text, scrap, rings, links, furnace and magnet rendered. Enlarged workbench and sharper fonts resolve the initial tiny-target concern. |
| Sweep and retained cargo — QA-A | Opening crescent drag collected 80. Release retained it. A later in-tray sweep through the fan collected 80 more while leaving all four fan tangles available. The haul reached 208 without an observed load penalty. |
| Fast ring drag — QA-A regression | Isolated-ring drag approximately `(530,406) → (780,350)` removed that ring and added 48, bringing cargo 80 → 128. Incorrect Sweep selection did not recur after event-time input capture. |
| Context priority — QA-A | Sweeping across rings did not latch. A held stroke ending at the furnace did not dump; a later deliberate click did. Missed collection on the outward stroke is recorded below rather than hidden by this priority pass. |
| Whole-haul bank and coil — QA-A | Pour changed cargo 208 → 0. Saved state retained banked 208 against goal 100, awarding level 1 once. Next stayed disabled during the pour; Breakaway subsequently became usable on the remaining fan. |
| Linked release — QA-A | Hub-to-right pull recovered three tangles for 116; the remaining ring stayed available. Visible transfer and action result agreed. Atomic drag tooling did not permit live held-preview inspection. |
| Surplus transition — QA-A | Pour & next showed the 116 deposit, then changed to Offset Fork. Backup retained fan banked total 324; new state had cargo 0, banked 0, goal 140 and level 1. Uncollected leftovers did not obstruct transition. Correct surplus copy appeared. |
| Contrasting pull and second reward — QA-A | Diagonal pull from the fork's upper-left seam ring recovered two tangles and 148. Other rings remained. Banking the full 148 earned level 2. This demonstrates a different usable direction, not voluntary replay. |
| Focus, pause and confirmation cancellation — QA-A | Activating an already-open Calculator automatically paused with cargo 148 retained. Returning focus kept the pause menu. Keep playing in restart confirmation returned directly to the tray. |
| Real exit/reopen and setting — QA-A | Menu sound toggle, Save & quit, verified window/process exit, then relaunch restored fork, cargo 148, banked 0, coil, recovered pieces and Sound off. M restored Sound on. No duplicate credit appeared on load. |
| Retry and complete-rig replay — QA-A | Retry restocked the exact fork with zero haul/progress while retaining level 2. Longer diagonal pull collected 180 with the same two-tangle pattern, versus 148 before long reach. Banking 180 retained level 2 and advanced completed count to 3 without another upgrade. Block visibility failed separately below. |
| Display/readability — QA-A | F11 entered readable 2560×1440 fullscreen and returned to a readable window; menus accepted clicks. Return window was larger than the original (outer capture 2135×1363 instead of 1602×932). This defect is resolved by the focused QA-C check below. Other displays/hardware untested. |
| Clean entry state — end QA-A | Actual Restart demo and Save & quit left layout 0, cargo 0, banked 0, level 0, 31 pieces, sound on. Window/process fully exited. Root separately preserved the actually earned full-rig state for the final cosmetic recheck. |
| Focused gameplay fixes — QA-B | Restored the saved state actually earned in QA-A (level 2, count 3, banked 180), rather than claiming a new progression run. The recovered block is clearly visible above the furnace in windowed/fullscreen views. After an actual fresh reset, an empty-tray-to-furnace stroke `(665,760) → (1725,655)` at the returned window size collected 48, kept banked 0 and left all five tangles. R retry and Save & quit restored an untouched starter state, verified in JSON and by process exit. |
| Final display regression and handoff — QA-C | F11 changed the original 1602×932 outer window (1600×900 client) to settled 2560×1440 fullscreen and back to settled 1602×932. HUD remained readable. F9 captured the untouched first board. Save & quit fully exited; JSON retained layout 0, cargo 0, banked 0, level 0, completed count 0, 31 pieces and sound on. |
| Model/persistence automation — report review | Integration owner executed six engine tests; reviewer inspected `unreal/Saved/Automation/index.json`, report `2026.09.11-22.42.29` UTC: six successes, zero failures/not-run. One expected backup-recovery warning from deliberate primary corruption. Coverage: capsule/reach, ownership, exact direct-link preview/commit, progression/retry, transactional snapshots, JSON/backup recovery. Report hash `514f77d92db0f66422bb3ae5781ac7bdcb0c9d1e4674f9b21f84472304e17b49`. WindowsEditor/NullRHI is not audiovisual evidence. |

## Findings and rechecks

| ID / severity | Build, steps, expected / actual | Evidence / affected source | Outcome |
| --- | --- | --- | --- |
| MS-QA-01 / moderate menu friction | Initial source: open retry/restart confirmation and choose Keep playing. Expected direct play; handler only cleared confirmation flags, retaining pause. | `WorkbenchRuntime.cpp`, Button case 7; initial source identity above. | Owner changed to `Pause(false)`. Native QA-A cancellation returned to play. **Resolved.** |
| MS-QA-02 / minor reward copy | Initial source: bank partial full-rig cargo or surplus. Expected recovery feedback; branch promised progress toward another improvement. | `WorkbenchRuntime.cpp`, Deposit. | Separate copy added. Surplus wording verified in QA-A; partial full-rig branch checked statically, not separately exercised. **Corrected.** |
| MS-QA-03 / moderate readability | Initial 1600×900 engine capture: tray about 570×310 with tiny targets/text. Expected workbench to be the principal readable subject. | `MagnetSweep00000.png` in ignored package screenshots; hash `968d2b673a8100a44c023add55100c004232d3f48c4fb572bb96d698c3b361d6`. Camera/HUD/scene affected. | Enlarged scene, darker tray and readable fonts inspected throughout QA-A. **Resolved at tested sizes.** |
| MS-QA-04 / major input context | Earlier binary above: integration owner observed quick ring-to-tray drag selecting Sweep and leaving starting ring; click tug worked. Expected press-position context to remain fixed. | Owner's native report; initial per-tick input polling in `WorkbenchRuntime.cpp`. | Event-time press handling added. Independent isolated and linked fast drags passed in QA-A. **Resolved.** |
| MS-QA-05 / moderate missed collection | QA-A with cargo 128: quick empty-tray-to-furnace drag `(500,520) → (1295,450)` crossed scrap but retained 128. Similar in-tray drag `(500,520) → (1110,520)` collected 80. Expected collection along the in-tray portion regardless of final position. | Native counters and unchanged scrap; `WorkbenchRuntime.cpp`, SweepPath only interpolated when endpoint was in tray. | Complete-segment sampling added. QA-B outward stroke collected 48 while preserving sweep context. **Resolved.** |
| MS-QA-06 / minor hidden reward | QA-A: finish a delivery after earning level 2. Expected visible recovered block. Furnace caption masked its top face at both tested display modes. | Native inspection; HUD label at `(650,180,30)` overlaps runtime block `(650,216,25)`. | Block moved to `(650,-216,25)`. QA-B resumed-earned-state view exposes the whole block at both sizes. **Resolved.** |
| MS-QA-07 / minor window-size change | QA-A/B: F11 from 1600×900 window to fullscreen, then F11 back. Expected previous window dimensions; returned larger. Controls remained usable. | Native settled capture 2135×1363 outer window. Runtime display toggle and engine input config affected. | The first runtime correction failed QA-B. Owner disabled Unreal's built-in F11 interception so the game's size-restoring handler receives the key. QA-C returned to the original settled 1602×932 outer window. **Resolved.** |

Initial source-assisted audio-pause risk was addressed by tracking audio components and routing pause/confirmation menus through their pause function. Perceptual synchronization during a paused pour was not verified.

## Captures and remaining limits

Actual F9 images under the ignored package's `MagnetSweep/Saved/Screenshots/` are engine captures, not generated concept art:

- `MagnetSweep00001.png`: complete rig, fresh fork, 1600×900; hash `00f5a35a432b5ed40303ea1a2d1491340c94455614a5d897844ebf1781b20f1d`.
- `MagnetSweep00002.png`: reset starter tray after display round trip; hash `fb6275ce5b7aac9f6deead11916024d0b768f2b14e31234ef17acb5abdfb4cea`.
- `MagnetSweep00003.png`: QA-B corrected visible block, resumed from actual QA-A earned progress, 1600×900; hash `c0da89cf683b6b2306f981b6717db9263a11d66fba88fb5814fb5fcdd3717609`.
- `MagnetSweep00004.png`: final QA-C untouched starter tray after the successful F11 round trip, 1600×900; hash `92fc87de31d1a8436688cbd2fbeb76f3b17e23c994e0bb22913f580e170369e1`.

**Not run:** human playtesting/voluntary continuation; listening to the game mix or judging audio levels; pause/focus loss while holding Aim; live held-preview inspection; interrupted OS writes in the packaged process; alternative accessibility bindings/controllers; formal frame-time/memory benchmark; different hardware/platforms. Model geometry and save-corruption tests do not replace those native checks. No crash or stuck menu was observed in covered flows.

The designer debate and gameplay critique remain design evidence in [CRITIQUE.md](design/CRITIQUE.md). Owner playtesting should assess whether choosing a line feels rewarding, the linked group is predictable, pouring and upgrades have sufficient payoff, and another delivery is wanted after both upgrades. This directed QA replay does not count as voluntary replay.

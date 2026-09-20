# Roster integration review — 20 September 2026

**Pass for this finite integration review.** The two previously reported presentation defects are repaired in the frozen source and covered by recorded regressions. The final controlled run contains 80 cases, 1,144 passing checks, no failing checks, and 255 captures. I independently inspected 28 frames and verified the archive identities and log counts. No additional roster defect was found in that scope. Human visual/feel approval and the F.I.S.T. quality requirement remain open; this is not an MVP or release acceptance.

## Reviewed artifact

Reviewer: `foundation_qa`, independent of the implementation author. Archive: `unreal/Saved/Validation/roster-v001-runtime-20260920-1843/`. Sources were read from its 72 frozen copies, based on `8972c90ed50f8b4f0a666da511dc18d6168dd6c7`. Later working-tree `core/src/core.cpp` differs and is excluded.

Windows x64, Unreal 5.8.2 CL56702186, MSVC 14.44.35228, SDK 10.0.22621.0. This was an author-operated Development editor-module probe, with 1600×900 captures. The Game executable was built and hashed, not independently played or packaged. Review tools were Git/Python/PowerShell and direct image inspection. No new UE launch or hardware input was performed.

| Identity | SHA-256 |
| --- | --- |
| [Portable checkpoint](../unreal/Tools/roster-v001-runtime-checkpoint.json) | `296d5d52f446da604f15424c471141449c9222fd06aeb870c1066b73de4bf417` |
| Ordered LF source graph | `819d9d6056cd5c04681be9d7afdebc476118af6dd3b56ebbdc69aeaa277e00bf` |
| `UnrealEditor-OverkillFoundry.dll` | `4905bff67301b67cd0537c67aca8a5a104b8b0dfb871b256e079995049a984c6` |
| `OverkillFoundry.exe` | `a9cf9ecce61ec331ba4297f7aa34ce4dafcaf1e0811a29029cbce67ccd583b51` |
| Both before/after identity files | `fc6286c4a3bdb33c5e16c26918d9c3a852785580656596b8c41aa87bd8cbfbd5` |
| `RosterProbe-20260920-184928.log` | `b3dd2059a535d1e218eccfa34255f1ab159ba38bde92ebecd4346faa13d4c71c` |
| `run-summary.json` | `7c6be0b032f303b489909bc05f708874d5be003811eb720e6904bdb0b03d7fe7` |

[Compact QA evidence](roster-runtime-evidence.json) lists all 28 inspected frames and their hashes. All 72 raw/LF source identities, both binaries, eight indexed logs and all 255 capture sizes/hashes match the portable index and archive. The before/after source and binary records are identical; the roster wrapper reports exit 0.

## Previous findings and focused recheck

Both findings originated in read-only review of the unbuilt roster changes; their initial failure was a source finding, not an invented play session. Closure below applies only to the exact artifact above.

| Finding | Reproduction, expected/previous behavior | Recheck |
| --- | --- | --- |
| RR-01 — Medium: final committed action suppressed by later death | End Turn with Pressure Cask at HP 1/Burn 1, or Cable Binder at HP 2 with actual UGS-019 Counterpulse. The already-resolved attack must appear, then death; the earlier `Died` prepass suppressed action dispatch. A multi-hit robot killed after hit one must not display its unperformed later hits. | `FoundryHost.cpp:389–445` preserves ordered named actions and counts actual `player_damage` events by parent. `FoundryRobot.cpp:220–236` queues one terminal clip behind the committed action. Cases 76/77 use real `Stage::Control("end")`, retain real timers, display exactly one committed hit, remain locked at 2.4 seconds, and release after cleanup. Log lines 5213–5243 and 5279–5309; action and terminal-lock frames inspected. **Closed for these cases.** |
| RR-02 — Medium: cancelled summon could remain hidden | Commit Nest deployment, leave its cosmetic events queued, then cancel the presentation through the title/reset path. Previously a helper awaiting reveal could stay hidden after those events were discarded. Reset must preserve the committed state, keep the title scene hidden, and show the helper on resume. | `FoundryHost.cpp:451–457` reconciles pending reveals before clearing events; `FoundryRobot.cpp:137–154` preserves the separate scene-hidden condition. Case 78 proves one real deployment/two core bodies/one hidden helper, checks the hash before reset, verifies title hiding and resumed visibility, then finds exactly one visible actor per living core enemy. Log lines 5340–5362 and the resumed action frame support closure. **Closed at the tested reset-helper scope; no physical title/resume input was performed.** |

Case 68 now compares against the stable first core enemy ID, so automatic target selection cannot make it inspect the dying Chassis instead of its new helper. Its recorded reveal delay is 0.420 seconds, followed by reveal, parent dissolve and cleanup. This corrects the earlier probe mistake without relabelling that run: **183007 remains 79 cases, 1,131 passing checks and one failing assertion**. The earlier saved-material importer crash also remains historical failed evidence.

## Observations and technical coverage

- **Assets and source:** the frozen generator's `--check` passes for ten meshes and 87 clips. The eight new silhouettes are Cable Binder, Pressure Cask, Coil Nest, Plated Press, Pulse Pursuer, Foil Warden, Split Chassis and Gatebreaker. The inspected frames show distinct bodies/mechanisms and solid materials. Import/reload reports agree on all eight robot/material records and all 72 new clip asset/duration/bone/socket records. Sampled compressed-motion magnitudes differ between import and reload; exact pose equality is not claimed. The corrected importer retains edited struct copies before assigning/saving the material array, and separately verifies saved material paths after process restart.
- **Core/presentation boundary:** named actions use committed event identities, rather than the next intent. Display cues do not create hits or enemies, and use no core RNG. Every recorded case checks that animation leaves its committed hash unchanged. The source allows only the actual committed hit count, gates summon cues on actual deployment, and drives Warden plate visibility from committed tile count.
- **Finite plates and spawns:** case 75 logs and asserts 3→2→1→0 plate state. Inspected stills show three, two and zero; there is no separate one-plate still. These are controlled injected bullets resolved through real Load/Fire, not evidence of paying recipe production costs despite the probe's “paid shot” label. Nest early/action frames and Chassis early/action/late frames show delayed helper appearance and parent collapse/dissolve. Final actor-count/hash assertions cover cleanup.
- **Actions and lifecycle:** the log inventory covers 26 authored actions, 29 reachable nonlethal reaction bands, ten deaths, ten escapes and five plate/regression cases. The 7-HP Mite cannot receive a positive whole-number hit below the proposed 5% light threshold; that omission is explicit. Ram/boss charge-hold checks pass. Still inspection includes Mite/Ram, boss strike and dissolve, and the cases listed above; it does not constitute an independent viewing of every animation in motion.
- **Utility kill:** case 79 performs actual MA058 through `Stage::Control`, matches its preview, retains preparation view, holds presentation at 1.4 seconds, then destroys the terminal actor and releases input. Log lines 5392–5412 show dissolve at 1.104 seconds and cleanup at 2.001 seconds. Its three inspected captures stop around 0.72 seconds: collapse/current view are visually observed; later Utility dissolve/clearance are source/log evidence, not an absent late screenshot. The same Cask dissolve is visible in case 76. CampaignProbe184423 separately records a saved MA058 kill of two enemies and one Rewards transition. `FoundryCampaignUI.cpp:238` places the busy combat view before Rewards; the technical roster fixture itself does not exercise that widget transition.

## Remaining boundaries

The frozen core still emits duplicate terminal notifications. CampaignProbe184423 visibly records MA058 `victory` events 35 and 36; the source's `terminal()` lacks the later phase-transition guard. The core owner separately reproduced lethal-recoil duplication and reported that campaign rewards/upgrade effects dispatch once. The saved campaign probe checks one reward transition here. **This known core issue is not fixed by this roster checkpoint**; a later core change needs its own evidence.

The cases prepare state, boss phase, cursors, upgrades and ammunition. They are neither a naturally earned campaign nor human input/playtest evidence. Most cases intentionally bypass normal camera timers; only the focused control cases retain those timers. Sparse screenshots establish silhouettes and sampled poses, not smooth motion, timing feel or spectacular quality. No independent audio listening, performance profiling, alternate resolution test, hardware input/focus test, standalone package, full campaign validation or owner visual approval is claimed. The source/log/frame evidence supports retaining this bounded roster integration checkpoint with those limits.

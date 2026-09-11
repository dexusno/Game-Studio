# Magnet Sweep — playable concept demo ready

Updated 2026-09-12. Stage: prototype. Development is complete **through the playable concept demo only**, as authorized. Development timeline was not a consideration. The next action is owner playtesting; full production and store work have not been started.

## Open the demo

Use `Play Magnet Sweep.cmd` in this game, or `BuildOutput/ConceptDemo/Windows/MagnetSweep.exe`. The full Windows folder is required. The owner entry state is a fresh Linked Fan tray, starter magnet, no banked/carried metal and sound on. Current delivery and rig autosave. BUILD.md has controls and reproduction commands.

Hold primary mouse to sweep loose metal. Press a copper ring, drag and release for an aimed tug. Click the furnace to pour the entire haul. Earn Breakaway and longer reach, compare the linked fan and offset fork, and retry with the improved rig. Remaining scrap is optional; no timer, capacity tax, shop or lore progression has been added.

## Verified evidence

- Unreal 5.8.2 Windows Development build compiled, cooked, packaged and launched. Seven original Blender meshes and nine original procedural sound effects imported; generators and provenance are recorded with assets.
- Six model/persistence automation tests passed, including finite reach/capsule geometry, direct-only links, whole-haul ownership, progression/retry and actual corrupt-save backup recovery. The deliberate recovery produces one expected warning.
- Independent native play covered sweep retention, fast ring tugs, fixed gesture context, linked bursts, whole-haul/surplus deposits, both upgrades and layouts, full-rig replay, pause/focus, retry, mute setting persistence and real save/quit/reopen. The same fork route recovered 148 before extended reach and 180 with the complete rig; directed QA replay is not evidence of voluntary replay.
- Native defects were repaired and rechecked: fast ring-origin capture, collection on an outward stroke, visible recovered block and F11 restoration of the original 1600x900 window.
- An art director generated a labeled concept target and reviewed actual game captures. Camera framing, tray contrast, furnace depth and text were corrected in the real build. Concept imagery is separate from native gameplay evidence.

Final child executable SHA-256: `332931cdb84a904cce3417811cb9768f656504cef48b0dd8397b578745e00dd5`.
Final game PAK SHA-256: `317f5e82007a9569c41b30a7a857f250d73f4fa9090666f4e23b74a17758ffe8`.
QA.md records exact builds, coverage and limits. Build outputs and raw captures are ignored; only curated game-only evidence belongs in source control.

## Remaining uncertainty and next action

Klaus should play without being coached toward a positive answer, then describe which action feels best or weakest and whether another delivery is wanted after the full rig. Human enjoyment, sound/mix perception, voluntary replay, demand and profitability are not established by agent checks.

Held-preview visual inspection and pause/focus loss during a held Aim were not exercised with atomic drag tooling. Formal frame-time/memory benchmarks, other hardware/platforms and accessibility/controller bindings remain untested. No core-play blocker was observed in covered flows. The shared studio validator has one pre-existing unrelated Scrapstorm link error, with no Magnet Sweep issue.

Keep the next increment within an explicit owner follow-up. BRIEF.md governs current mechanics; PRODUCTION-PLAN.md is an ordered contract, not permission to expand beyond this demo. Preserve other games and owner edits. Source, assets, records and the reviewed handoff should travel together at the committed revision.

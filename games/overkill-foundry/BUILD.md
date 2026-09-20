# Overkill Foundry — build and run

Fresh implementation: an engine-independent C++17 core and a new Unreal 5.8.2 host. Current rules/content are `of-core-0.2` / `cinderwall-staged-0.1`, snapshot schema 2. This is a working technical encounter and storage foundation; the full-city MVP and standalone package are not delivered.

## Prerequisites

Verified 20 September 2026 on Windows: Unreal 5.8.2, CL 56702186; Visual Studio Community 2022 17.14.39; MSVC 19.44.35228 (toolset directory 14.44.35207). The core uses Visual Studio's bundled CMake and Windows SDK 10.0.26100.0; Unreal selected SDK 10.0.22621.0 and bundled .NET 10.0. Blender 5.2.1 LTS builds the original art sources. Machine paths stay in root ignored `config.local.json`. No new purchase is required for this increment.

## Reproduction instructions

Run from the repository root in PowerShell:

```powershell
python -X utf8 games/overkill-foundry/tools/compile_content.py --check --self-test
& games/overkill-foundry/tools/core.ps1 -Action test
& games/overkill-foundry/tools/core.ps1 -Action fixture
& games/overkill-foundry/tools/test_storage.ps1
& games/overkill-foundry/qa/storage/run_probes.ps1
& games/overkill-foundry/tools/unreal.ps1 -Action Build
& games/overkill-foundry/tools/unreal.ps1 -Action BuildGame
& games/overkill-foundry/tools/unreal.ps1 -Action Content
& games/overkill-foundry/tools/unreal.ps1 -Action Fixture
& games/overkill-foundry/tools/unreal.ps1 -Action Run
```

Core binaries are `core/build/Release/overkill_runner.exe` and `overkill_core_tests.exe`, relative to this game. `--fixture` produces deterministic JSONL events and a final state hash. It does not run a city or balance policy. The same `core.cpp` and `serialization.cpp` are compiled by Unreal; the core has no Unreal types or presentation calculations. The action helper supplies valid suggestions, not every possible assembly. No arbitrary part/shot/storage cap is introduced.

The full manifest contains 246 recipes, 127 ordinary upgrades, 25 Mayor gifts, 10 robots, 10 formations, 207 physical output types and 3 authored Mysteries. Only 19 recipe definitions are currently staged in the runtime. `compile_content.py --check --release` intentionally fails with **665 unsupported operations**; `runtime-support.json` is empty. See [content/README.md](content/README.md) for evidence-linked enablement. Do not trim the final pool or bypass this gate.

Unreal project: `unreal/OverkillFoundry.uproject`. `Content` reproducibly generates the Technical map/material. The HUD supports mouse recipes/parts/targets, keyboard Collect/Load/Unload/Fire/End Turn, recorded Precision-result selection and same-seed restart. This is a debugging adapter, not the final UI or Precision minigame. Engine BasicShapes remain temporary references. Build products, generated Technical content, raw captures and logs are ignored.

[art-source/README.md](art-source/README.md) documents reproducible original Breach Ram/Rivet Mite/stage sources and compact PBR textures. Generated Blender/FBX/GLB files live under ignored `.local/overkill-foundry/art/cinderwall-v001`. The verified foundation does **not** import those assets into the encounter yet. A new worktree must regenerate these outputs or use the exact existing local directory; Git does not carry ignored art/builds.

The Windows storage envelope is a separate platform target, documented in [platform/README.md](platform/README.md). Snapshot serialization and tested file replacement do not implement campaign Continue, receipts or reward transactions by themselves. Callers must reconcile after a failed commit, because the replacement may already have succeeded.

## Verification record

20 September, branch `codex/overkill-foundry-mvp`, based on `dcb72a6`; reviewed source identities are in [qa/foundation-review.md](qa/foundation-review.md) and [qa/storage-review.md](qa/storage-review.md).

- Core builds with strict warnings as errors. Final author suite passes **443 assertions**; independent QA passes **15 runtime cases and 51 content checks**. Four actual rule/validation findings were corrected and independently rechecked. The fixture finishes at 80 HP, round 3, four shots, state hash `5de73f967dc79677`.
- Separate storage target and independent probes pass: 15 probe groups, 1/1 author CTest target and 12 verified process exits. Two independent findings were fixed and rechecked; final production source SHA-256 `5ea6c9b32d8432bb8793e18c5623ac3b987508fb193bf1708346cfbf666e22a7`. This is the envelope layer only.
- Unreal Editor Development and Game Development targets build: `unreal/Saved/BuildLogs/Build-20260920-133252.log` and `BuildGame-20260920-133301.log`. Content generation: `Content-20260920-130906.log`.
- Rendered shared-core fixture and command-path probe pass: `Fixture-20260920-133315.log`. Headless and Unreal transcripts match all **85 JSONL lines (84 events and summary)**, SHA-256 `73ee62e338e764b15593b3a5031ea97da070cbd243a99b9d282d9e91a3a5e95f`.
- Unreal Game executable SHA-256: `a90a025ba1cd59b4011fe40d62395b6a97401f62aab413d366d85b32e73ea4ab`; Editor game-module DLL: `1600d337b8edc2f4968ebd70888dfa29ceb15df908043807f2b44c8b80ebbf6`.
- Ordinary DX12 rendering on RTX 4090 / driver 610.88 was inspected in both controlled cameras at 1600×900. Captures: `unreal/Saved/Screenshots/host-preparation.png` and `host-action.png`. Temporary geometry does not establish the final visual standard.
- Native mouse/keyboard test on the same module passed pre-collection recipe rejection, 10-material Collect, Simple Sighting craft/part selection/Load, an 8-damage Fire killing the 7-HP Mite while staying in round 1, End Turn advancing Ram to Attack 18 in round 2, and same-seed restart restoring the initial encounter. Actual command/event hashes are logged in `Run-20260920-134050.log`. No human player or owner visual approval is claimed.
- Original art exports pass 81 structure/interchange checks; six Blender stills were inspected. Source checks do not prove Unreal animation playback or the selected F.I.S.T. quality bar.
- Fixed initial sky-capture/Lumen setup, enabled required installed ACL plugin and corrected view orientation. Residual UBA listener/socket and `r.MotionVectorSimulation` thread-safety warnings caused no observed build/render failure. Packaging, focus/resolution sweep, measured performance, audio and final human tests remain outstanding.

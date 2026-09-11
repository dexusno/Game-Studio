# Magnet Sweep — build and run

UE 5.8.2 Windows concept demo, version 0.1.0. Source is isolated in `unreal/`. Installed tools are read from ignored `config.local.json`; no engine installation is copied into the repository.

## Play the local demo

Double-click [Play Magnet Sweep.cmd](Play%20Magnet%20Sweep.cmd), or launch `BuildOutput/ConceptDemo/Windows/MagnetSweep.exe`. Keep its adjacent Engine and MagnetSweep folders. The concept runs offline and resumes the current delivery automatically.

## Reproduce

From the repository root in PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File games/magnet-sweep/scripts/Build.ps1 -Stage Editor
powershell -ExecutionPolicy Bypass -File games/magnet-sweep/scripts/Build.ps1 -Stage Content
powershell -ExecutionPolicy Bypass -File games/magnet-sweep/scripts/Build.ps1 -Stage Tests
powershell -ExecutionPolicy Bypass -File games/magnet-sweep/scripts/Build.ps1 -Stage Package
```

Content regenerates the material, imports the original mesh/sound assets, and saves the Concept map. Packaging targets Win64 Development and archives to ignored `BuildOutput/ConceptDemo/Windows/`. The entire Windows folder is required to run, not just the launcher.

## Current evidence — 2026-09-12

The Editor target compiled using Visual Studio 14.44 and Windows SDK 10.0.22621. All seven meshes and nine sounds imported. Six Unreal automation tests passed: capsule/reach boundaries, ownership/banking, preview/commit/direct links, progression/retry/layout differences, transactional snapshot validation, and actual JSON/backup recovery. Machine-local raw reports are under `unreal/Saved/Automation/`.

The Windows Development package was independently played through both upgrades, both layouts, surplus deposits, complete-rig replay, pause/focus, retry, mute persistence, real save/quit/relaunch, and display switching. Native regressions verified event-time ring selection, collection along a fast outward sweep, visible recovered-block output and F11 return to the original window size.

Final child executable SHA-256: `332931cdb84a904cce3417811cb9768f656504cef48b0dd8397b578745e00dd5`. Final `MagnetSweep-Windows.pak` SHA-256: `317f5e82007a9569c41b30a7a857f250d73f4fa9090666f4e23b74a17758ffe8`. The final fix changes packaged input configuration, so both hashes identify the delivered candidate. QA.md separates broad and focused passes with their exact identities.

Human fun, voluntary replay and listening/perceptual audio assessment remain for owner playtesting. Held-preview inspection and interruption while holding Aim were not exercised with the available atomic drag tool. Formal performance benchmarking and other hardware/platforms remain untested. The demo is a tested Windows concept, not a release candidate.

The shared studio validator still reports its pre-existing broken Scrapstorm QA link; it reports no Magnet Sweep issue.

## Controls and save behavior

Hold primary mouse button over the tray to sweep; releasing retains cargo. Press an available copper ring, drag to aim, release to pull. Click the furnace with cargo to pour the complete haul. Escape pauses; M toggles sound; R opens retry confirmation; F11 switches window mode; F9 writes an in-game screenshot to Saved/Screenshots. The pause menu offers save and quit.

The current delivery and earned magnet improvements autosave under the runtime Saved/ConceptDemo folder. A previous valid backup is retained. `-DemoProfile=qa_name -DemoQA` selects a separate QA profile and diagnostic telemetry; `-DemoFresh` starts that profile from a fresh demo. These switches are for verification, not progression cheats. Owner saves must not be used for destructive QA.

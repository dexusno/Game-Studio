# Magnet Sweep — build and run

UE 5.8.2 Windows gameplay rebuild, version 0.3.0. Source is isolated in `unreal/`. Machine-local tool locations come from ignored `config.local.json`; no engine installation is copied into this repository. The old 0.1 concept demo and its owner save remain separately under BuildOutput/ConceptDemo.

## Play the tutorial

[Play Magnet Sweep.cmd](Play%20Magnet%20Sweep.cmd) opens `BuildOutput/Tutorial/Windows/MagnetSweep.exe`. Keep the adjacent Engine and MagnetSweep folders. A fresh career begins with **Start learning**; actions advance the lessons, and earned credits, XP and upgrades remain in that run. Skip guidance at any time. The first dangerous pickup explicitly pauses its fuse until the player drops the haul; later warnings have normal consequences.

Pause offers **Practice tutorial**, with its own saved run and **Return to saved career**. Re-entering practice resumes it; only the confirmed **Restart practice** resets that practice. Skipping or completing the guide does not erase its earnings. Saves without tutorial metadata continue normally.

The earlier package and its player career are preserved separately. [Continue Previous Demo.cmd](Continue%20Previous%20Demo.cmd) opens the previous Rework package. Its save has not been migrated into the new tutorial archive. The original 0.1 demo also remains under BuildOutput/ConceptDemo. Native tutorial results belong in [QA-TUTORIAL.md](QA-TUTORIAL.md); historical rebuild evidence remains in [QA-REWORK.md](QA-REWORK.md).

## Reproduce

From the repository root in PowerShell:

```powershell
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Editor
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Content
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Tests
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Package -ArchiveName Tutorial
```

Content regenerates the material, imports seven original meshes and 28 WAVs (nine historical cues and 19 new cues), sets four rework sound loops to play while silent, and saves the Concept map. The original rework sound sources reproduce with `python games/magnet-sweep/scripts/GenerateReworkAudio.py`. Packaging cooks the engine ambient cubemap referenced by the scene, targets Win64 Development and archives by default to ignored `BuildOutput/Tutorial/Windows/`. The entire Windows folder is required to run. `-ArchiveName` chooses another simple directory name. The All stage does not include Tests; run that stage explicitly. Tests requires a fresh exported report and rejects failures or unrun tests even if Unreal exits with code zero.

This machine uses Visual Studio 14.44 and Windows SDK 10.0.22621. During the first package, Unreal's cooker-owned Zen cache process exited before staging could connect to port8558. Keeping the installed Zen server running through staging allowed the subsequent package to finish. This was a local packaging-cache dependency, not a game runtime dependency; the game runs offline without Zen.

## Controls and consequences

Hold left mouse or toggle Space to attract; release retains cargo. Hold Shift or toggle Q for a narrow precision field. Connected links form one moving load. Right mouse **drops the entire haul recoverably** and switches the field off. Click the furnace to smelt a stable load; each smelt spends one of four fuel charges.

Outside the explicitly paused first tutorial warning, unsafe cargo starts a persistent fuse. Expiry spends one fuel charge and destroys the named most valuable unbanked salvage piece; hot cells are removed and the rest spills recoverably. Previously banked cash, XP, upgrades and collection remain safe. Exhausting fuel below quota fails the contract. Dropping the haul in time costs no fuel or salvage but requires rebuilding the load.

Tab opens the workshop. Escape pauses/closes overlays; R opens retry confirmation; M toggles master sound; separate music/effects controls live in pause. F11 switches window mode and restores the prior window size. F9 writes an actual game screenshot under Saved/Screenshots. The pause menu offers save and quit.

## Saves and verification

The version2 career lives under runtime `MagnetSweep/Saved/Rework/player.json`, with a previous valid backup. It preserves actual piece positions/ownership, current fuel and fuse, banked quota and bonus flags, credits, XP, purchased mods, collection and audio settings. Unsafe cargo loads paused. Optional tutorial metadata records the lesson, first-danger rehearsal and purchased-mod event. Separate practice lives at runtime `MagnetSweep/Saved/Tutorial/player.json`; switching back loads the saved career rather than merging practice earnings. `-DemoProfile=qa_name -DemoQA` selects a separate QA profile plus diagnostic telemetry; `-DemoFresh` starts that selected profile fresh. Owner saves must not be used for destructive QA.

Editor compilation and the fresh 2026.09.12-18.50.46 headless report passed: **15 tests, zero failures, warnings or unrun cases**. Nine cover the existing salvage/reward/risk/save rules; six cover lesson order, off-path play, early danger, retries, actual runtime rescue/Skip callbacks, legacy saves and strict transactional tutorial metadata validation. The runtime callback tests use a null controller and do not prove rendered Tick behavior. The final changes after that report are confined to tutorial drawing: pouring copy, fuller risk explanation and removal of a misleading quota pickup marker; final editor/package compilation and the focused T2 native pass verify that presentation.

Build/cook/stage/archive completed in 37.32 seconds. Tutorial child EXE SHA256: `1090787dd46eafc7aa14a1b84e4d41e04696825bbf6d2f40e4b6ff57aca0dcf5`. Complete EXE/PAK/IoStore identities and compact test results are in [tutorial-verification.json](evidence/tutorial-verification.json). Previous R2 identities remain in [rework-verification.json](evidence/rework-verification.json).

Native observations are recorded separately in QA-TUTORIAL.md. Numerical audio analysis and active audio components do not certify the audible mix; perceptual listening remains unverified. Formal performance and other hardware/platforms remain untested. This is a local playable prototype for owner judgment, not a release candidate or evidence of commercial appeal.

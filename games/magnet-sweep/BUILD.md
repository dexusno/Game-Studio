# Magnet Sweep — build and run

UE 5.8.2 Windows gameplay rebuild, version 0.2.0. Source is isolated in `unreal/`. Machine-local tool locations come from ignored `config.local.json`; no engine installation is copied into this repository. The old 0.1 concept demo and its owner save remain separately under BuildOutput/ConceptDemo.

## Play the local rebuild

The final owner launcher is [Play Magnet Sweep.cmd](Play%20Magnet%20Sweep.cmd), targeting `BuildOutput/Rework/Windows/MagnetSweep.exe`. Keep the adjacent Engine and MagnetSweep folders. The offline game autosaves the current contract and career. The final R2 package is built and native observations are recorded in [QA-REWORK.md](QA-REWORK.md). The current running session was left under user control after external input was detected; its career must be preserved.

## Reproduce

From the repository root in PowerShell:

```powershell
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Editor
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Content
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Tests
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Package
```

Content regenerates the material, imports seven original meshes and 28 WAVs (nine historical cues and 19 new cues), sets four rework sound loops to play while silent, and saves the Concept map. The original rework sound sources reproduce with `python games/magnet-sweep/scripts/GenerateReworkAudio.py`. Packaging cooks the engine ambient cubemap referenced by the scene, targets Win64 Development and archives to ignored `BuildOutput/Rework/Windows/`. The entire Windows folder is required to run. `-ArchiveName` chooses another simple directory name. The All stage does not include Tests; run that stage explicitly.

This machine uses Visual Studio 14.44 and Windows SDK 10.0.22621. During the first package, Unreal's cooker-owned Zen cache process exited before staging could connect to port8558. Keeping the installed Zen server running through staging allowed the subsequent package to finish. This was a local packaging-cache dependency, not a game runtime dependency; the game runs offline without Zen.

## Controls and consequences

Hold left mouse or toggle Space to attract; release retains cargo. Hold Shift or toggle Q for a narrow precision field. Connected links form one moving load. Right mouse **drops the entire haul recoverably** and switches the field off. Click the furnace to smelt a stable load; each smelt spends one of four fuel charges.

Unsafe cargo starts a persistent fuse. Expiry spends one fuel charge and destroys the named most valuable unbanked salvage piece; hot cells are removed and the rest spills recoverably. Previously banked cash, XP, upgrades and collection remain safe. Exhausting fuel below quota fails the contract. Dropping the haul in time costs no fuel or salvage but requires rebuilding the load.

Tab opens the workshop. Escape pauses/closes overlays; R opens retry confirmation; M toggles master sound; separate music/effects controls live in pause. F11 switches window mode and restores the prior window size. F9 writes an actual game screenshot under Saved/Screenshots. The pause menu offers save and quit.

## Saves and verification

The version2 career lives under runtime `MagnetSweep/Saved/Rework/player.json`, with a previous valid backup. It preserves actual piece positions/ownership, current fuel and fuse, banked quota and bonus flags, credits, XP, purchased mods, collection and audio settings. Unsafe cargo loads paused. `-DemoProfile=qa_name -DemoQA` selects a separate QA profile plus diagnostic telemetry; `-DemoFresh` starts that selected profile fresh. Owner saves must not be used for destructive QA.

Editor build, content import, headless Unreal automation, native runtime observations and listening are separate evidence. Final R2 editor compilation succeeded; the 2026.09.12-10.19.45 headless report records **nine passing tests, zero failures or warnings**. This includes whole-haul recovery at corners/full capacity, fuel loss, legal advanced-quota routes at 40 kg, transactional saves and real JSON/backup recovery. Build/cook/stage/archive completed successfully in 40.62 seconds. Earlier R1 native evidence covers physical collection, earned quota/XP/loot, one purchase, next contract, real save/relaunch, retry, focus pause and fullscreen restoration. Its risk behavior predates the final fuel/drop corrections; final native results belong in QA-REWORK.md.

Final child EXE SHA256: `0848a376445ea7dad0ba6de1a4f0ed2254a9895cf68adceac8e450f9a183be80`. PAK: `e411209a92e5a3e8ac868c94a6e8f96a0e732ef5bf589c02b9b268177749f027`. IoStore UCAS: `a2013fb0a098da3fbb166aaa8d35d83932cc2df4d331c75ee4a07baab13f10a0`; UTOC: `b2517ffdd7265780e6efad9bb085ac9c56b638352a0fdb37767586610579a99d`. The complete machine-readable artifact identities and compact automation results are in [rework-verification.json](evidence/rework-verification.json).

Audio sources are original and pass numerical peak/loop-seam checks. This session cannot perceptually hear the music or effects; sound quality and comfort are not certified by signal analysis. Formal performance benchmarking, other hardware/platforms and hold-specific simultaneous mouse/Shift input remain untested. Current QA uses actual Space/Q toggle controls. A Development-console slow-motion attempt was unavailable; no slomo command ran. Native short-fuse rescue/refusal and final-charge flow remain coverage gaps after external user input ended the controlled R2 pass. Nine final model tests cover the corresponding rules; this does not replace those native checks. This is a playable prototype for the owner's next judgment, not a release candidate or proof of commercial appeal.

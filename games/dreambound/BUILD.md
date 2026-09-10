# Build and run

Unreal 5.8.2 on Windows, Blender 5.2.1 LTS, Visual Studio 2022 and Windows SDK. Tool paths belong in the ignored root `config.local.json` under `tools.unreal`, `tools.blender` and `tools.ai3d.windowsOutputRoot`.

## Reproduce the Reverie package

1. The new Blender kit is tracked under `art/reverie/environment` and `art/reverie/characters`. Regenerate changed geometry with `scripts/create_reverie_environment.py` or `scripts/create_reverie_characters.py` in Blender background mode with `--python-exit-code 1`. Their metadata records dimensions, material slots, triangles, UVs and authored collision. New generated bitmap sources/prompts are under `art/reverie/references` and `art/reverie/textures`.
2. The three large, original TRELLIS landmarks are deliberately private: `<windowsOutputRoot>/reverie-v1/{fountain,root-bank,crown-tree}/finished`. Each contains its `SM_RV_*.fbx`, unchanged `base_color.png`, unchanged `metallic_roughness.png` and preparation report. Fountain uses `scripts/prepare_trellis_kit.py --asset FountainHero`; bank/tree use `scripts/prepare_reverie_landforms.py`. Generation setup and retained model/dependency terms are in [AI3D.md](AI3D.md). The importer fails if these new sources are missing; it does not substitute the old kit. A fresh checkout therefore needs regeneration or the existing private source bundle.
3. New sounds and selected licensed source recordings live in `assets/audio-reverie`. Regenerate with `python scripts/prepare_reverie_audio.py`; the report contains the exact recipes, source regions, levels and hashes. They are CC0 recorded foley, not Suno output. `audition-reel.wav` is a review montage and is never imported.
4. Run `scripts/Build.ps1 -Stage Editor` for C++ compilation, then `scripts/Build.ps1 -Stage Content`. Content runs the full editor with NullRHI, importing only the Reverie namespaces and preparing the separate empty `/Game/Maps/Reverie` entry map. Runtime assembles all scenery. Reports are in `unreal/Saved/Reverie/import.json` and `map.json`.
5. Run `scripts/Build.ps1 -Stage Package`. Default output is `BuildOutput/Reverie/Windows/Dreambound.exe`. `-PackageConfiguration Development -OutputName ReverieDev` makes a diagnostic package. Keep the whole Windows directory together. The new cook includes Reverie art/audio and the required engine fonts; historical game art is not part of this package.

Shipping is the internal player configuration, not an external release. Historical packages remain in their original directories. The local owner-modified `DefaultInput.ini` is preserved; do not overwrite it during reproduction.

## Focused verification

Run `BuildOutput/Reverie/Windows/Dreambound/Binaries/Win64/Dreambound-Win64-Shipping.exe` with `-NullRHI -DBVerify -DBArtCheck -DBSaveSlot=DreamboundQA_Reverie -DBSeed=833283 -unattended -nosound`. Use the actual executable when waiting for completion: the top-level bootstrap exits before its child game. Require `Saved/QA/reverie-art-checks.json` to report complete=true, aborted=false and failed=0. This checks seeded variation/reproduction, protected combat/practice pads, floor and stair probes, connected bridge/arch lanes, physical closed/open gates including their edges, and new environment families. It does not establish game feel or visual quality. Shipping Saved is below `%LOCALAPPDATA%/Dreambound`; the QA runner restores previous isolated journal bytes.

For an actual ordinary-spawn render, use `-DBCapture -DBFirstView -DBSaveSlot=DreamboundQA_ReverieView -DBSeed=833283 -RenderOffscreen -windowed -ResX=1600 -ResY=1000 -ForceRes -unattended`. It writes `Saved/Screenshots/Windows/Dreambound_FirstView.png` and exits. `-DBCapture -DBDetailView=Fountain` uses an explicitly posed close view and writes `Reverie_Fountain.png`. `-DBCapture` without either option grants sample Frost/Mirror and poses an enemy in court2; that equipment and camera are staged, not earned progression.

The retained full combat suite is `-NullRHI -DBVerify -DBSaveSlot=DreamboundQA_CombatFeel -DBSeed=833283 -unattended -nosound`, writing `Saved/QA/combat-feel-checks.json`. Run it when mechanics changes warrant it. Its historical success must not be claimed as new-build evidence.

`-DBMotionCapture` is a 36-second scripted combat/audio fixture. It writes `Saved/CombatFeelCapture/Frames.csv`, frame images and `CourtyardMix.wav`; image readbacks impose overhead, so it is not a performance benchmark or ordinary playthrough.

## Current result

Windows Shipping **0.4.0-reverie** is packaged from source commit `10e9738586944c561a79b73792d04a9c61d2c878`. Final BuildCookRun completed in 42.53 seconds with exit code 0. The final packaged focused check passed 2 groups / 11 observations / 0 failures. A first candidate exposed reversed terrace stairs; the physical orientation was corrected before this package and all 16 stair probes now pass. [Build identity](evidence/reverie-build.json), [check results](evidence/reverie-art-checks.json), [actual ordinary-spawn capture](evidence/reverie-courtyard.png) and [independent review](evidence/reverie-qa-notes.md) record the evidence and limits. See [STATUS.md](STATUS.md) for the current handoff.

# Build and run

Unreal 5.8.2 on Windows, Blender 5.2.1 LTS, Visual Studio 2022 and the Windows SDK. Tool paths belong in ignored root config.local.json under tools.unreal and tools.blender.

## Reproduce
1. Tracked FBXs/textures/WAVs are the source assets. Regenerate only changed authoring: scripts/create_art.py for the original environment, scripts/create_segmented_shield.py for the hero, and scripts/create_garden_art.py for garden additions. Run Blender in background with --python-exit-code 1; garden -- --skip-renders preserves the visual previews when changing collision only. Garden rock/support FBXs contain authored UCX hulls; this avoids relying on editor subsystems unavailable to the content commandlet. Audio regenerates with python scripts/create_audio.py.
2. Run scripts/Build.ps1 -Stage Editor for C++ editor compilation.
3. Run scripts/Build.ps1 -Stage Content for meshes, materials, audio and the Bellroot map. Generated Unreal Content is ignored because import metadata contains local paths.
4. Run scripts/Build.ps1 -Stage Package. The default package is BuildOutput/CombatFeel/Windows/Dreambound.exe. For a diagnostic build use -PackageConfiguration Development -OutputName CombatFeelDev. Keep the whole Windows directory together.

Shipping is the internal player configuration, not an external release. Previous packages remain at BuildOutput/SegmentedShield, BuildOutput/ShieldStudy and BuildOutput/Shipping.

## Verification and evidence
The current suite is -NullRHI -DBVerify -DBSaveSlot=DreamboundQA_CombatFeel -DBSeed=833283 -unattended -nosound. It advances actual actors through engine ticks with staged targets/positions and lethal progression hits. Require Saved/QA/combat-feel-checks.json complete=true, aborted=false and failed=0; a process exit is insufficient. Shipping Saved resides below the local application data Dreambound directory; QA requires an explicit isolated profile and restores its prior journal bytes. Read the exact-build evidence for actual results and any corrected fixture runs.

Use -RenderOffscreen -DBMotionCapture -DBSaveSlot=DreamboundQA_CombatMotion -DBSeed=833283 -windowed -ResX=1280 -ResY=720 -ForceRes -unattended for the 36-second scripted combat fixture and engine master-audio recording. Outputs are Saved/CombatFeelCapture/Frames.csv, Frame_*.png, CourtyardMix.wav and Capture.txt. The fixture uses safe practice, artificial positions and an injected block; it is not earned progression. CSV records actual charge-loop state as well as folding/selection. Capture keeps silent audio buffers. Raw output is ignored; timestamps preserve capture cadence. Image readbacks impose substantial overhead, so this is not performance evidence or a normal playthrough.

-DBCapture instead poses a static enemy and grants sample upgrades for an art view. It must not be presented as naturally earned equipment. Native input tests use a separate DreamboundQA_CombatInput profile. Owner play omits QA flags and preserves the existing DreamboundSegments profile. Controls and current scope are in PLAY.md.

## Current result
0.3.1-combat1 adds collapsed/expanded forms, full-charge payoff, melee reach and new sound design. Exact package hashes, test executions and capture limits belong in evidence/combat-feel-build.json and STATUS.md. The extended staged suite includes actual form transforms, partial/full discrimination, a single cover-respecting blast, useful melee reach/step, wall safety and reachable prop collision. Final-state QA must match the packaged build's recorded identity.

Native startup/menus and rendered scripted evidence are separate. Human challenge, complete ordinary play, subjective sound quality and anchor-B fidelity remain unestablished. No purchase, publication or firewall modification is required.

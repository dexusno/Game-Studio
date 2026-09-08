# Build and run

Unreal 5.8.2 on Windows, Blender 5.2.1 LTS, Visual Studio 2022 and the Windows SDK. Tool paths belong in ignored root config.local.json under tools.unreal and tools.blender.

## Reproduce
1. Tracked FBXs/textures/WAVs are the source assets. Regenerate art only after authoring changes, using configured Blender in background with scripts/create_art.py. The generator now preserves the intended first UV channel and repairs degenerate surface UVs. Audio regenerates with python scripts/create_audio.py.
2. Run scripts/Build.ps1 -Stage Editor for C++ editor compilation.
3. Run scripts/Build.ps1 -Stage Content for meshes, materials, audio and the Bellroot map. Generated Unreal Content is ignored because import metadata contains local paths.
4. Run scripts/Build.ps1 -Stage Package. The default package is BuildOutput/ShieldStudy/Windows/Dreambound.exe. For a diagnostic build use -PackageConfiguration Development -OutputName ShieldStudyDev. Keep the whole Windows directory together.

Shipping is the internal player configuration, not an external release. The rejected beta2 stays separately at BuildOutput/Shipping; do not overwrite or confuse it with the courtyard study.

## Verification and evidence
The current suite is -NullRHI -DBVerify -DBSaveSlot=DreamboundQA_PhysicalShield -DBSeed=833283 -unattended -nosound. It advances actual actors through engine ticks with staged targets/positions and lethal progression hits. Require Saved/QA/physical-shield-checks.json complete=true, aborted=false and failed=0; a process exit is insufficient. The old 31 fixture is for the rejected route and does not certify this build. Shipping Saved resides below the local application data Dreambound directory; QA requires an explicit isolated profile and restores its prior journal bytes.

Use -RenderOffscreen -DBMotionCapture -DBSaveSlot=DreamboundQA_Motion -DBSeed=833283 -windowed -ResX=1280 -ResY=720 -ForceRes -unattended for scripted movement/combat and the engine master-audio recording. Outputs are Saved/MotionCapture/Frames.csv, Frame_*.png, CourtyardMix.wav and Capture.txt. Capture disables submix auto-disable so silent buffers stay in the soundtrack. Raw output is ignored. Frame requests impose substantial overhead; this is not a performance measurement or normal playthrough. CSV timestamps preserve capture cadence; do not assume the requested engine FPS equals capture FPS.

-DBCapture instead poses a static enemy and grants sample upgrades for an art view. It must not be presented as naturally earned equipment. Native input tests use a separate DreamboundQA_PhysicalInput profile. Owner play omits QA flags. Controls and current scope are in PLAY.md.

## Current result
The 0.2.0-study1 player package compiled, cooked, staged and archived successfully. Focused runtime scenarios pass 14/0; exact tested identity, report and coverage are in evidence/shield-study-build.json, evidence/shield-study-checks.json and QA.md. Exposure, exported surface UVs and menu-input leakage were corrected after actual visual/native inspection. Two remaining tiny-tangent warnings concern the unchanged stair bevel geometry; no material compilation failure remains.

Native startup/menu and scripted combat evidence are separate. Human challenge, complete ordinary play, subjective sound quality and anchor-B fidelity are not established. No purchase, publication or firewall modification was needed.

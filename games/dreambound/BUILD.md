# Build and run

Unreal 5.8.2 on Windows, Blender 5.2.1 LTS, Visual Studio 2022 and the Windows SDK. Tool paths belong in ignored root config.local.json under tools.unreal and tools.blender.

## Reproduce
1. Tracked FBXs/textures/WAVs supply the original kit. Large TRELLIS outputs remain in the configured private AI3D output directory; see AI3D.md for generation and Blender preparation. Regenerate only changed authoring: scripts/create_art.py for the original environment, scripts/create_segmented_shield.py for the hero, and scripts/create_garden_art.py for garden additions. Run Blender in background with --python-exit-code 1; garden -- --skip-renders preserves previews when changing collision only. Audio regenerates with python scripts/create_audio.py.
2. Run scripts/Build.ps1 -Stage Editor for C++ editor compilation.
3. Run scripts/Build.ps1 -Stage Content for the original meshes, materials, audio and Bellroot map. Then import the four prepared TRELLIS assets with the full UnrealEditor.exe and `-NullRHI -unattended -ExecutePythonScript=<game>/scripts/import_trellis_kit.py -TrellisKitConfig=<private-config.json>`. The config names the prepared BellTree, Cloister, RootRock and Waymarker FBX/map files. Use the full editor because its import subsystems are required; NullRHI permits this asset import without GPU rendering. Generated Unreal Content is ignored because import metadata contains local paths.
4. Run scripts/Build.ps1 -Stage Package. The default package is BuildOutput/TrellisArt/Windows/Dreambound.exe. For a diagnostic build use -PackageConfiguration Development -OutputName TrellisArtDev. Keep the whole Windows directory together.

Shipping is the internal player configuration, not an external release. Previous packages remain at BuildOutput/CombatFeel, BuildOutput/SegmentedShield, BuildOutput/ShieldStudy and BuildOutput/Shipping.

## Verification and evidence
For this art increment, use `-NullRHI -DBVerify -DBArtCheck -DBSaveSlot=DreamboundQA_TrellisArt -DBSeed=833283 -unattended -nosound`. This filters the existing suite to seeded layout reproduction/variation, connector capsule/floor/gate queries and physical corner rocks in the three courts. Require Saved/QA/trellis-art-checks.json complete=true, aborted=false and failed=0. It does not test combat or ordinary play. Shipping Saved resides below the local application data Dreambound directory; QA requires an explicit isolated profile and restores its prior journal bytes.

The retained full combat suite is -NullRHI -DBVerify -DBSaveSlot=DreamboundQA_CombatFeel -DBSeed=833283 -unattended -nosound. It advances actors through staged targets/positions and lethal progression hits, writing Saved/QA/combat-feel-checks.json. Its prior successful run belongs to0.3.1-combat1; do not report it as new-build evidence without a new execution. Run it when a gameplay change warrants it.

Use -RenderOffscreen -DBMotionCapture -DBSaveSlot=DreamboundQA_CombatMotion -DBSeed=833283 -windowed -ResX=1280 -ResY=720 -ForceRes -unattended for the 36-second scripted combat fixture and engine master-audio recording. Outputs are Saved/CombatFeelCapture/Frames.csv, Frame_*.png, CourtyardMix.wav and Capture.txt. The fixture uses safe practice, artificial positions and an injected block; it is not earned progression. CSV records actual charge-loop state as well as folding/selection. Capture keeps silent audio buffers. Raw output is ignored; timestamps preserve capture cadence. Image readbacks impose substantial overhead, so this is not performance evidence or a normal playthrough.

-DBCapture instead poses a static enemy and grants sample upgrades for an art view. It must not be presented as naturally earned equipment. Native input tests use a separate DreamboundQA_CombatInput profile. Owner play omits QA flags and preserves the existing DreamboundSegments profile. Controls and current scope are in PLAY.md.

## Current result
0.3.2-art1 adds the four textured TRELLIS environment assets, carved perimeter bays, floor material-slot restoration and lighting. One final Shipping build/cook/archive succeeded; the packaged art filter passed2 groups/8 assertions with0 failures. Package hashes, import facts, capture limits and the distinct tree1024 setting are in evidence/trellis-art-build.json and STATUS.md. The close native arrival and actual packaged view were inspected. The local saved-project owner input configuration was preserved.

The older0.3.1-combat1 build record retains its full combat checks and scripted audio/motion evidence. Native menu entry, staged capture and ordinary complete play are distinct. Human challenge, complete ordinary play, subjective sound quality, performance and anchor-B fidelity remain unestablished for this increment. No purchase or publication occurred.

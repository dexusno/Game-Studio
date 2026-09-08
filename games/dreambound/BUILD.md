# Build and run

Unreal 5.8.2 on Windows; Blender 5.2.1 LTS. Unreal discovered the installed Visual Studio 2022 C++ toolchain and Windows SDK. Private tool paths belong in root `config.local.json` under `tools.unreal` and `tools.blender`.

## Reproduce
1. Generate the original kit with configured Blender in background using `scripts/create_art.py`. Tracked FBX/textures already contain the accepted kit; regenerate only after source changes.
2. Generate original audio with `python scripts/create_audio.py` if tracked WAVs need rebuilding.
3. Run `scripts/Build.ps1 -Stage Editor` for C++ editor compilation.
4. Run `scripts/Build.ps1 -Stage Content` for meshes/materials/audio and the Bellroot map. Generated Unreal Content is ignored because import metadata contains machine paths; originals and import scripts are tracked.
5. Run `scripts/Build.ps1 -Stage Package`. The default player package is `BuildOutput/Shipping/Windows/Dreambound.exe`. Optional `-PackageConfiguration Development` retains diagnostics in a separate directory.

Shipping is a build configuration for this internal beta, not an external release. Development builds start an Unreal trace listener and may prompt Windows for network access. The offline player build omits that listener; no firewall modification is required.

## Verification
Use `-NullRHI -DBVerify -DBSaveSlot=DreamboundQA_Shipping -DBSeed=833283 -unattended -nosound` for the staged fixture. Read `Verification.txt` in the game's Saved directory and require zero failed checks; process exit alone is not the check result. Editor/Development saves reside under the project/package; Shipping uses local application data Dreambound/Saved. QA slots are isolated from owner progress.

`-RenderOffscreen -DBCapture -DBSaveSlot=DreamboundQA_Visual -DBSeed=833282 -windowed -ResX=1600 -ResY=900 -ForceRes -unattended -nosound` captures a staged first-person art view and exits after twelve seconds. This grants sample attachments and stages a stationary enemy; it is not ordinary combat footage.

For native testing, launch normally with `-DBSaveSlot=DreamboundQA_Input`. For Klaus's actual playtest, omit QA flags. Keep the whole Windows package together; its root executable depends on adjacent engine/game files. [Controls](PLAY.md).

## Observed result — 2026-09-08
Editor compilation, content import and Windows Development/Shipping build/cook/stage/archive succeeded. Final Shipping staged report: **31 passed, 0 failed**, including CRC corruption recovery into the previous checkpoint and equipment. [Report](evidence/beta-0.1.0-staged-checks.txt). Art import and material-pass reports are under art/.

Actual first-person rendering and initial normal title startup were inspected. Native input awaits dismissal of the earlier Windows network prompt; full ordinary journey, audio listening, performance benchmarking and enjoyment are not established. See [STATUS.md](STATUS.md) and [QA.md](QA.md).

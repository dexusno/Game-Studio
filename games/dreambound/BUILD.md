# Build and run

Unreal 5.8.2, Blender 5.2.1 LTS, Visual Studio 2022 and Windows SDK on Windows. Private tool/source paths are in the ignored root config.local.json.

## Current package

Windows Shipping **0.5.1-motion1**: `BuildOutput/CreatureMotion/Windows/Dreambound.exe`, or `scripts/Start-Demo.ps1`. Keep the entire Windows directory together. Final executable source is `a0c7642a98002a40f6cc8369bc36085ac8a1f16d`; [build identity](evidence/enemy-animation-build.json) records package hashes, the sixteen-bone full cook, source identity and retained owner input configuration. [Motion review](evidence/enemy-animation-review.mp4), [independent QA](evidence/enemy-animation-qa.md). LivingWorld 0.5.0 and earlier packages are preserved.

## Reproduction

1. Retained Reverie Blender geometry/textures are in art/reverie. The three large TRELLIS landmarks remain under `<windowsOutputRoot>/reverie-v1/{fountain,root-bank,crown-tree}/finished`; [AI3D.md](AI3D.md) records setup, source revisions and dependency terms. Original player combat art remains in art/generated and art/source.
2. Twelve new habitat meshes, ten 2K textures and editable Blender source are tracked under art/living-world. Regenerate with scripts/create_living_world.py in Blender background mode using `--python-exit-code 1`.
3. Original creature reference images, rig specifications and reports are in art/organic-enemies. Large generated sources and the retained twelve-bone inputs remain under `<windowsOutputRoot>/organic-enemies/{briarhide,mire-seer}/finished`; the current sixteen-bone FBX/Blender files and byte-identical 4K maps are in the sibling `finished-motion` folders selected by each rig specification. Scripts/prepare_organic_enemy.py reproduces preparation from each source GLB and rig JSON. A fresh machine needs these private generated sources or regeneration; the importer fails rather than silently substituting another mesh.
4. Twenty-six edited cues, recipes and retained permitted sources are in assets/audio-reverie. Run `python scripts/prepare_reverie_audio.py` to render, or add `--check` to compare output. Twenty-four cues use retained CC0 foley; charge/full release use the owner's two Suno paid downloads. Review reels are not imported.
5. Run `scripts/Build.ps1 -Stage Editor`, then `scripts/Build.ps1 -Stage Content`. The full editor imports Reverie environment/audio, preferred player combat art, LivingWorld props and OrganicEnemies, then prepares /Game/Maps/Reverie. Runtime builds the seeded scene. Reports are under unreal/Saved/{Reverie,LivingWorld,OrganicEnemies}.
6. Run `scripts/Build.ps1 -Stage Package`. Default output is CreatureMotion. Use `-PackageConfiguration Development -OutputName CreatureMotionDev` for a separate diagnostic build. This internal Shipping configuration is not an external release.

Selective audio import is `scripts/Build.ps1 -Stage Audio -AudioCues S_ChargeLoop,S_FullRelease` (or S_FootstepA,S_FootstepB,S_Land). It preserves loop/voice settings without reimporting graphics.

For the historical LivingWorld package, the final pose correction changed C++ only. After the successful full cook at ecab7e2, the final BuildCookRun used `-build -skipcook -stage -pak -iostore -nodebuginfo -archive` with the same project/platform/configuration/output to reuse unchanged cooked content. The final build succeeded in 46.13 seconds. Normal full reproduction uses the commands above. CreatureMotion received a full sixteen-bone cook at `57e0d159e8cd4fbce3af3afb9c0cec9d77979de9` in 80.92 seconds. The final C++-only correction at `a0c7642a98002a40f6cc8369bc36085ac8a1f16d` used the same skipcook flags and packaged in 48.25 seconds; all PAK/UCAS/UTOC files remained byte-identical to that full cook. It does not reuse the historical LivingWorld twelve-bone content.

Preserve the owner's existing DefaultInput.ini settings. The requested changes are Left Shift Sprint and Left Alt Dash; only those mappings were committed from that owner-modified file.

## Focused evidence

Use the real child `Dreambound/Binaries/Win64/Dreambound-Win64-Shipping.exe` when waiting for completion; the bootstrap returns early.

`-NullRHI -DBVerify -DBMovementCheck -DBSaveSlot=DreamboundQA_LivingMovement -DBSeed=552389 -UserDir=D:/Game-Studio/.local/living1-movement-final -unattended -nosound` writes Saved/QA/movement-checks.json. Four groups / fourteen observations passed on the recorded ecab7e2 binary; the two later C++ fixes do not change player movement or the harness. This tests actual flat-floor movement, stamina, wall collision, cooldown/held input, pause and reset. It does not simulate native keys or establish feel. The initial failed recorder measurements and their corrections are retained in the [QA report](evidence/living-world-qa.md).

`-DBCapture -DBDetailView=Creatures` or `-DBDetailView=Habitat`, plus `-DBSaveSlot=DreamboundQA_LivingView -DBSeed=552389 -UserDir=D:/Game-Studio/.local/living-view -RenderOffscreen -windowed -ResX=1600 -ResY=1000 -ForceRes -unattended -nosound`, produces the corresponding Living_*.png. These are posed views. Existing FirstView, Fountain and Stairs capture modes remain.

`-DBMotionCapture -DBSaveSlot=DreamboundQA_LivingAudition -DBSeed=552389 -UserDir=D:/Game-Studio/.local/living1-audition-grounded -RenderOffscreen -windowed -ResX=1280 -ResY=800 -ForceRes -unattended` records 36 seconds of scripted actual engine combat/audio, dash, walking, sprint and a jump/landing. Do not use -nosound. Saved/CombatFeelCapture contains Frames.csv, screenshots and CourtyardMix.wav; -DBAudioOnly skips screenshots. The [review video](evidence/living-world-scripted.mp4) repeats sampled frames to 30fps using audio-clock timestamps; it is not a performance benchmark or ordinary playthrough.

The older -DBArtCheck and full combat harness remain available. Their previous results belong to the recorded historical binaries; they were not needlessly repeated for this increment. See [STATUS.md](STATUS.md) for current limits and next action.

## Enemy motion review

`scripts/Capture-EnemyMotion.ps1 -Build Editor -Creature Melee -View Side -CaptureDirectory <new-private-directory>` captures the actual non-practice enemy AI against a choreographed, protected player target. Creature choices are Melee, Caster, Hunter and Boss; view choices are Side, LowSide, Front and Player. LowSide exposes claw/foot contact near floor level. Use `-Build Shipping` for the CreatureMotion package. Each run has an isolated QA save/profile and preserves prior captures by requiring a new output directory.

The default32-second study includes approach, stopping/turns, attacks, physical hits at20/22 seconds, lethal damage at28 seconds and settling. `-Seconds 8` limits a focused regression to eight seconds; the report lists actual phase coverage. Boss starts outside ranged distance to expose its gait and relocates the scripted player once onto nearby paving at14 seconds to provoke normal close Slam selection. That relocation is recorded; it is not native player input. `-FootMarkers` adds world ankle-target markers for Editor diagnosis only. The observer runs after enemy movement/posing and before the engine camera-manager update, avoiding a one-frame lag in the review camera.

Each PNG is one rendered simulation tick at fixed30Hz. `python scripts/review_enemy_motion.py <capture>/Saved/EnemyMotionStudy/Melee-Side --video <capture>/review.mp4` validates actual images/CSV cadence, measures contacts/body motion and encodes those consecutive frames without duplication. Clips are silent. These runs establish observed motion cases and exact build identity, not hardware frame rate, audio quality or an ordinary playthrough. Keep raw frames and Invocation.json in ignored local storage; retain compact review reports with the relevant build evidence.

The16-bone revision is prepared with Blender using `scripts/prepare_organic_enemy.py --asset Briarhide --source <original-textured.glb> --prepared-source <retained-finished/SK_OE_Briarhide.blend> --output <finished-motion> --preview` after Blender's `--` separator; MireSeer uses its matching asset/source. The prepared-source mode checks the original GLB hash and preserves geometry, UVs, original rest transforms and texture bytes before adding hands/feet and revised weights. Rig specs select `finished-motion` through `output_folder`; the earlier `finished` folders remain the original12-bone inputs. The selective full-editor importer is `scripts/import_organic_enemies.py`; successful16-bone import evidence is in [animation/import.json](art/organic-enemies/animation/import.json).

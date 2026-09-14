# Shared geometry and camera motion study — historical art reference

Preserved from Magnet Sweep on 14 September 2026 for the independent game under internal ID `overkill-foundry`. The verification below was performed on the original art study, not rerun as part of this copy. Its magnet imagery predates the selected rear claw. Current gameplay rules are in [the design plan](../../design/REDESIGN-PLAN.md); this study establishes only an art/camera reference.

13 September 2026. **Scripted art test, not gameplay or final art.**

[Watch the eight-second study](camera-motion.mp4). [Preparation still](preparation.png), [action/impact still](action-impact.png).

The owner selected the graphics treatment of camera board 16, with **16B side view for preparation/assembly and 16C perspective for action**. This small original scene tests whether a single authored set of objects can preserve identities through gathering, assembly, firing, impact and an incoming hit. It does not select component recipes, damage, defence timing, resource costs or turn order. The illustrated enemy response after one shot is a storyboard sequence, not a rule that Fire ends a turn.

## What is present

- A bevelled red horseshoe magnet and alternating chain links, a forge cannon with orange furnace light, one exposed loading channel, an attached protective plate, stacked gears/nuts/bolts, and two mechanically distinct enemies.
- Three demonstration components — gear body, warm collar and threaded bolt core — move from the pile to the channel, visibly join into one assembly, and travel together when fired. They are not three rounds. This is a geometry-test example, not an approved recipe or a faithful copy of the concept storyboard's body/capsule/nose.
- Two Blender camera objects share every scene object. The preparation camera is orthographic with a small elevation. The perspective action camera moves to a second pose for the incoming hit so the plate face can be seen. Neither change swaps asset sets.
- A small swing/hoist motion, sequential part pickup/loading, cannon recoil, target lean, a visible enemy projectile, plate deflection and sparks. The exact same remaining-stock transforms and preparation camera return at the end. The three committed components remain consumed in this illustration.

| Frames / time | Presentation beat |
| --- | --- |
| 1–47 / 0–1.96 s | Lower magnet, attract example parts, lift |
| 48–79 / 1.96–3.29 s | Move parts into the channel; join one assembly |
| 80–111 / 3.29–4.63 s | Perspective cut, load the assembly and fire with recoil |
| 112–142 / 4.63–5.92 s | Target reaction; reframe for incoming attack |
| 143–180 / 5.92–7.50 s | Enemy projectile strikes the visible mounted plate |
| 181–192 / 7.50–8.00 s | Return to the original preparation framing and remaining stock |

## Reproduce

Requires Blender 5.2.1 and FFmpeg/FFprobe on PATH. No downloads, plugins, textures or other assets are needed. Invoke from the repository root, using the local Blender installation as `BLENDER_EXE`:

```powershell
& $env:BLENDER_EXE --background --factory-startup --python games/overkill-foundry/art-tests/camera-motion/build_scene.py -- --stills --animate
python games/overkill-foundry/art-tests/camera-motion/encode_review.py
```

These Blender CLI arguments were executed on Windows with **Blender 5.2.1 LTS, build 9e2066aef7ef**, using EEVEE. The encoder was **FFmpeg 6.0-full_build-www.gyan.dev**, libx264, CRF 20, yuv420p, 24 fps, faststart, no audio. The source script saves the `.blend` and all 192 PNG frames under ignored `raw/`, plus the two small review stills and scene metrics. Its `--stills` flag also writes ignored action-loading and plate-impact audit images. The encoder checks every frame is present and verifies the final stream is 960×540, 192 frames and exactly 8 seconds.

`raw/` includes rebuildable source `.blend`, rendered frames and local logs. Large raw media are intentionally excluded from Git. The final MP4 is a review render; it is not a packaged game capture or store screenshot.

## Actual verification and limits

The source executed and rendered successfully in the installed Blender. [Scene metrics](scene-metrics.json) record **342 objects, 316 mesh objects, 21,978 source vertices, 19,919 source polygons, two camera objects, three shot-component roots and 24 remaining-stock roots**. The material count is 14, including Blender's two unused factory materials; 12 are authored here. Bevel modifiers add geometry at evaluation time. These counts and an offline EEVEE render do not establish a real-time engine budget.

The script asserts that every remaining-stock root has the identical world transform at frames 1 and 192. Both camera views use the same names, meshes and material datablocks. The initial camera angle overlapped the enemy silhouettes and hid the protective plate; the delivered framing separates the enemies and exposes the hit face. The producer independently reviewed preparation, action loading, target impact and plate impact stills before the final clip. The author inspected temporal samples of gather, transfer, airborne assembly and plate sparks. [Container evidence and hashes](review-metadata.json) are generated from the delivered media.

The source is deterministic at seed 16. This verifies reproducible geometry, scripted motion and image output on this installed renderer. It does **not** establish camera feel under interaction, chain physics, magnetic contact, collisions, gameplay, engine import, runtime performance or cross-machine pixel identity. Attraction and firing trajectories are authored transforms, not simulation. There is no audio. The camera reframe can be shortened or reduced later once real action/turn rules exist.

This output remains well below board 16's finish: no operator, simple robot bodies, sparse blockout environment, basic noise-based metal wear, modest lighting and simple sparks. Preparation components are still small at 960×540; the action-loading frame and visible travelling assembly are clearer. The pile is deliberately bounded, and the environment is a backdrop. More detailed machinery, art-directed surfaces, depth treatment and a representative interactive engine view are still required to prove the selected quality level. Do not present this probe as owner acceptance of the art direction.

## Provenance and integration

All meshes, materials, keyframes and background shapes are original procedural project content authored by **Game Studio / Codex in Blender** from `build_scene.py`; no third-party geometry, downloaded texture, character or font is embedded. Board 16 is an existing generated visual reference, not a source texture. Blender and FFmpeg are production tools and are not redistributed. The game's distribution licence is not yet selected; this record does not grant an outside asset licence.

Source and review outputs are recorded in the game's [asset manifest](../../assets/manifest.csv). The [review page](../../assets/concepts/combat-camera-2026-09-13/index.html) pairs this technical probe with the higher-fidelity generated storyboard. Root verified both storyboard switches, inspected the revised preparation/loading/target/plate stills, and started browser playback, which reached 0:08/0:08. This is agent inspection, not a human interaction or game-feel test. Engine integration remains unperformed: export/import, materials, animation/state binding and a gameplay camera pass belong to a separately authorized implementation goal.

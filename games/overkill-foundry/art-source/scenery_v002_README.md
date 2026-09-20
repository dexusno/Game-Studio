# Cinderwall scenery refinement

This source increment replaces the repeated near furnace wall with a sunken
service channel, one large smelter, pressure accumulators, uneven foundry roofs,
two distant building planes and a crane skyline. The working deck has quieter,
rougher metal surfaces. The gun, rear gathering origins and open firing volume
remain unchanged. All architecture has depth and can be viewed from either
selected camera. Nothing is a pasted concept image or another game's asset.

Run from the repository root with the existing Blender installation:

```powershell
$artBlender = (Get-Content config.local.json | ConvertFrom-Json).tools.blender
& $artBlender --background --python-exit-code 1 --python games/overkill-foundry/art-source/scenery_v002_build.py -- --output .local/overkill-foundry/art/scenery-v002
& $artBlender --background --python-exit-code 1 --python games/overkill-foundry/art-source/scenery_v002_verify.py -- --output .local/overkill-foundry/art/scenery-v002
python games/overkill-foundry/art-source/scenery_v002_publish.py --output .local/overkill-foundry/art/scenery-v002
```

The source review requires the already-generated Cinderwall-v001 and Mara-v001
blend files in adjacent directories. Their original robots/gun appear only in
the source camera renders; the scenery FBX contains no heroes, lights, camera
or fog volume. `--no-render` skips rendering, retaining the editable scene.
The third, closer action camera is a framing experiment, not an owner-selected
replacement or evidence of a rendered game.

The actual FBX round trip checks metre bounds, pivots/placement, material slots,
UVs, geometry retention and the open firing volume. The published verification
must match the exact export and source report. It does not establish screen
readability, runtime culling, GPU cost or visual acceptance.

Integration uses `(x,-y,z)*100` for Unreal positions, the same conversion as the
existing assets. Replace the old `CinderwallGenerated` stage and the Mara depth
extension except its hopper; the separate payload is still a runtime attachment.
Keep the actual Mara claw and gun. The report lists all19 modules and their
pivots. Reuse the existing original Cinderwall materials for unchanged slots;
the five `v2_` PBR families and two dim window materials are new. Their normal
maps use +Y/OpenGL, requiring Unreal's green-channel flip. The maps use the
same ORM convention as v001. Raw FBX/blend/source renders remain ignored.

Inspect actual preparation and settled16C shooting, including three or more
opponents, a large/small pair, target separation and coherent barrel direction.
Foreground machinery must stay clear while atmospheric depth softens the city.
The source renders are labelled intermediate assets; they are not gameplay,
F.I.S.T.-level acceptance or Klaus's approval.

All geometry and surfaces are original Game Studio / Codex work authored with
Blender5.2.1LTS and the project's original helper source. No third-party asset
or additional software was introduced. The project's distribution licence is
still unset; the manifest records the source and15 new compact textures.

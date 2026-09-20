# Cinderwall art v001

Original early-production meshes, PBR surfaces, rigid rigs and keyframes for **Breach Ram**, **Rivet Mite** and the Cinderwall foundry encounter. Written fresh for Overkill Foundry on 20 September 2026. No other game's geometry, gameplay, project or build pipeline was reused.

This is a source-art increment. The actual rendered game, completed city roster, character/gun, UI, effects, sound and Klaus's visual approval are outside this asset handoff. The supplied STYLE anchor and 16B/16C views informed material/shape/framing decisions; no reference pixels or reference geometry are embedded in these assets.

## Reproduce

From the repository root, using the existing ignored local tool configuration:

```powershell
$artBlender = (Get-Content config.local.json | ConvertFrom-Json).tools.blender
& $artBlender --background --python-exit-code 1 --python games/overkill-foundry/art-source/build_cinderwall.py -- --output .local/overkill-foundry/art/cinderwall-v001
& $artBlender --background --python-exit-code 1 --python games/overkill-foundry/art-source/verify_exports.py -- --output .local/overkill-foundry/art/cinderwall-v001
```

Requires Blender's bundled Python/NumPy and FBX/glTF exporters. Tested with **Blender 5.2.1 LTS**. Nothing is downloaded or installed. `--no-render` skips the six source-review renders. The editable `.blend` packs textures; all large/intermediate artifacts stay in the ignored output directory. Source parameters, rest pivots, rigid vertex assignments and action keyframes remain editable in `build_cinderwall.py`.

| Output below the directory above | Use |
| --- | --- |
| `Cinderwall_Source.blend` | Editable source scene, two rigs, separate reusable stage objects, packed surfaces, selected camera studies and lighting |
| `meshes/BreachRam.fbx`, `meshes/RivetMite.fbx` | Rest skeletal meshes; metre unit metadata; non-deforming muzzle/intent sockets retained |
| `animations/BR_*.fbx`, `animations/RM_*.fbx` | Eight Ram / seven Mite clips; 30 fps; import animation against the matching skeleton |
| `meshes/BreachRam.glb`, `meshes/RivetMite.glb` | Self-contained PBR skinned alternatives with each robot's own complete action set |
| `meshes/CinderwallStage.fbx`, `meshes/CinderwallStage.glb` | 47 placed objects; 30 deck instances share geometry in GLB/source |
| `textures/CW_*_BaseColor.png`, `*_ORM.png`, `*_Normal.png` | Nine original 256 px tileable surface families, 27 maps |
| `renders/{preparation,action,pair_detail,charge,hit_heavy,death}.png` | Opaque 1600 × 900 Blender source inspection; **not gameplay or store screenshots** |
| `asset-report.json`, `interchange-verification.json` | Inventory, pivots, clips, cue times, camera studies, module placements and executed checks |

## Identity and composition

The **Breach Ram** is a heavy low tracked pressure boiler: burnt red hull, broad ochre mantlet, deeply recessed bore, exposed side return pipes, furnace grille, off-centre twin exhaust and upper pressure crown. The mantle/barrel/vents compress for Charge and recoil for Blast. Track rollers, skirt plates and large cast bevels establish mass.

The **Rivet Mite** is a low six-legged pressure vessel: oxidised turquoise shell, overlapping black shell bands, one amber optic, rivet-driver nose, copper air line and articulated gripper feet. The body lowers before a sharp driver extension; hits pitch the body and legs rather than moving the entire model like the Ram. Its shell ejects during death. Approximate source bounds are 2.92 × 2.40 × 2.55 m for the Ram and 1.75 × 1.74 × 1.31 m for the Mite; scale remains a presentation input.

Cinderwall's reusable environment consists of riveted deck plates, seven furnace arch bays, flue stacks, a service catwalk, copper supply pipes, red I-beam gantries, a rear claw mount and scrap bins. All are closed dimensional geometry, usable from the two camera studies. The claw/bins occupy source X ≈ −6.75 m; reserve X ≈ −3.5 m for Mara's gun. Enemies begin at X 1.3/3.6 m. The firing lane stays empty. `intent` bones sit above the machines; actual intent UI remains an engineer/UI task.

Palette: blue-black iron `#303B42`, worn edge steel `#778082`, oxide `#A5492E`, ochre `#B18A4D`, Mite patina `#427579`, soot `#161F25`, masonry `#544A42`, copper `#A46235`, gauge ceramic `#C3BDA8`. Warm small emissive mechanisms contrast against cooler broad shapes. Broad bevels and deliberate pipe/plate hierarchy carry the silhouette; wear stays secondary. Source noise, scratches, roughness and tangent normals are all original algorithmic surfaces, not photos or generated image-service outputs.

The source camera studies preserve lateral preparation and a low diagonal action view. They are integration starting points, not final HUD/gun framing. Include Mara/gun before judging the action composition. Reduce furnace brightness further if rendered UI or enemy intents lose contrast.

## Unreal import and binding

1. Import each FBX as a skeletal mesh with its own skeleton, scene/unit conversion enabled, uniform scale 1, supplied normals/tangents retained and no automatic animation import. Confirm approximate centimetre bounds **292 × 240 × 255** and **175 × 174 × 131**. Blender round trips verify metres; Unreal unit/orientation verification is still required. Source faces −X with +Z up.
2. Import the corresponding `BR_` or `RM_` FBXs as animations against that skeleton. GLB is an alternative interchange package; engine importer/extension support is not asserted by a Blender round trip. `root`, `muzzle`, `core` and `intent` identify actor origin and presentation attachment points. The mesh is rigidly skinned; every vertex has one full-weight bone.
3. Import the stage as a scene so object transforms survive, or construct actors from `asset-report.json`'s module instance transforms. Mesh pivots are at each module's horizontal centre/base. Keep linked deck instances. Do not merge the stage with the enemies or place scenery across the firing lane. Collision/physics are not authored; target selection should use presentation actor hit volumes defined by the engineer.
4. Rebuild one shared PBR master with the material names in the report. BaseColor is sRGB. ORM is linear: **R = 1 (no baked AO), G = roughness, B = metallic**. Normal is linear, tangent-space **OpenGL +Y**; flip green for Unreal's DirectX convention, and use normal strength 0.35 as the source starting point. All map alpha is opaque. Painted materials are dielectric except exposed chips. Emissive slots use constant colour/power, recorded in the generator; every slot also needs the runtime dissolve parameter.
5. Use `assets/production/cinderwall-v001/presentation-contract.json` and the report for event bindings. The core decides hits/deaths/outcomes immediately and exactly once. Animation cues only display those committed events. Neither clip completion nor a cosmetic effect triggers damage, a summon, loot, RNG or turn progression.

Ram Charge holds its last pose/glow until Blast; Blast begins in the same charged pose. Ordinary idle can be layered subtly while held. Blend back to idle in about 0.08 s after normal actions. Use a separate reaction layer or interrupt/replace short reactions, preventing a growing queue under multi-hit fire. Light/medium/heavy selection starts at `<5%`, `5–<20%`, `≥20%` effective resolved damage/max HP, with integer comparisons. A fully negated hit uses deflection, not a false heavy hit. These thresholds remain presentation tuning; they never create gameplay stun.

Both deaths are 2 s source clips: distinct collapse/burst, dissolve of **all material slots and any attached effects** from 1.1 s, complete actor/FX removal at 2 s. Dissolve is an engine material/actor obligation; FBX/glTF keyframes cannot supply it. Source detached-looking pieces remain bones in the same skinned object to simplify total cleanup. Death overrides reactions even on tiny lethal/status damage. Actor logical targetability stops at the core death event; final-kill transitions should allow the selected presentation duration. Escape is a separate retreat/despawn with no death/loot cue. Faster/reduced-motion playback must preserve event visibility and complete cleanup.

## Executed evidence and limits

The original build rendered six camera/pose stills. Inspection found and corrected inward-facing track rollers, incompatible GLB action leakage, pixel-like wear, and excessive furnace brightness. The final source meshes have **48,024 Ram triangles / 12 bones**, **20,200 Mite triangles / 20 bones**, and **173,512 triangles across 47 placed stage objects**. These are source counts, not a measured runtime performance result.

The round-trip script checks both FBXs' mesh/skeleton counts, exact bone counts, all rigid weights, UVs, dimensions and event sockets; both attack clips must move the intended mechanism after actual reimport. It checks GLB containers, exact per-robot clip coverage and normal/base/roughness/metallic map references, all stage modules/UVs, and all 27 textures' dimensions/opaque alpha. The copied report records the actual result. Driver-age warnings appeared when probing Cycles OptiX, but all six Cycles renders completed; no new driver or software requirement is inferred.

The source result is an economical art starting set. It **does not establish F.I.S.T.-level finish or satisfy human visual approval**. Unreal import, rendered material parity, gameplay framing with Mara/gun/UI, animation feel, damage cues, layered industrial depth/haze, VFX/audio, dissolve/cleanup, LODs and runtime performance remain integration/refinement work. Other enabled enemies still need their own identities and action coverage.

TRELLIS's local Python imports (`trellis2`, pipelines, nvdiffrast, o_voxel) and a CUDA tensor operation passed on the available RTX 4090 using `probe_trellis.py`. TRELLIS.2-4B and DINOv3 caches are present, RMBG-2.0 is absent. This was **not a fresh image-to-mesh generation**, and no TRELLIS output or model-rights assumption enters these assets. A first external-script import failure was resolved by adding the existing repository to `sys.path`; no installation changed. The deliberate Blender path has no software blocker.

## Provenance

Creator: **Game Studio / Codex**, procedural output from **Blender 5.2.1 LTS** and its bundled NumPy. All geometry, rig definitions, animation keyframes, material/noise algorithms and source camera/lighting are original project content. No external models, texture photographs, fonts, sound, image-service generations or F.I.S.T. assets were incorporated. The game's distribution licence remains unset, consistently with the studio record. No third-party asset credit requirement was introduced by this increment.

`manifest-rows.csv` contains exact rows for the single shared-manifest owner to merge. It registers the reproducible art source, presentation contract and each compact published texture. The engineer should register actual imported runtime `.uasset` derivatives against this source when those exist. Source renders must be labelled as such and must not enter a store page as gameplay screenshots.

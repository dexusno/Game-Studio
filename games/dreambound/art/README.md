# Bellroot shield and courtyard art

The current 2026-09-08 kit replaces the rejected gun/crescent kit. It contains a complete throwable shield, articulated gripping hand, rigid sentinel parts, a branching bell tree with a separate canopy, irregular masonry, stairs, ivy and fern clusters. There are 27 meshes, 24 material recipes and 10 original surface textures. No third-party geometry, textures, brushes or material libraries were used.

[Actual shield front](preview/shield-front.png) · [Shield back and hand](preview/shield-back.png) · [Sentinel](preview/sentinel.png) · [Blender first-person composition](preview/bellroot-first-person.png)

These are rendered views of actual geometry. The Blender reviews predate the final UV correction and do not establish final Unreal appearance or fidelity to the selected style reference. The integration owner maintains actual game captures and build evidence.

## Current source and import

[create_art.py](../scripts/create_art.py) executes the authored profiles in [bellroot.py](source/bellroot.py), exports FBXs and creates the material/texture recipes. Run Blender with `--background --factory-startup --python-exit-code 1 --python games/dreambound/scripts/create_art.py`. The editable [Dreambound_Kit.blend](Dreambound_Kit.blend) uses relative `//generated/` image paths; retain that adjacent folder when moving the source.

[import_art.py](../scripts/import_art.py) creates `/Game/Art/Meshes`, `/Game/Art/Materials` and `/Game/Art/Textures`. It assigns stable material slots, enables instanced-static-mesh usage and checks dimensions, signed pivots and custom hull counts. It changes no map, game code or audio. The verified axis settings are +X forward/+Z up, centimeters, legacy FBX import and `force_front_x_axis=False`.

The materials use separate per-surface craft maps for shield wear, masonry/mineral marks, longitudinal bark and metal scratches. Vertex `Color` channels are R=painted value, G=exposed edge wear, B=recess/weathering, A=opaque. `M_CombatGlow` exposes a `Color` parameter with emission x3. Element materials retain the requested stable names.

## Physical shield and rigid parts

`SM_ShieldPlate` is one complete disc, centered at local zero with its face normal +X, 85 cm across. The shell is about 11 cm thick; including its rear structure and grip, overall X depth is 21.3 cm. The front includes a visible core so the same mesh remains complete while flying. The old `SM_WeaponBody` is obsolete and excluded from current metadata/import.

Attach `SM_Core` at (-6, 0, 0), measured 4.84 x 18.4 x 18.4 cm. Attach `SM_Forearm` at (-12, 0, -12); its origin is the grasp center, fingers curl around a Y-axis grip bar, and the forearm extends -X. Its overall dimensions are 59.34 x 18.76 x 20.97 cm.

Guardian assembly pivots remain body (0, 0, 94), head (0, 0, 156), shoulders (0, +/-36, 146), hips (0, +/-18, 94), relative to feet. All parts face +X and extend from their authored rigid pivots; do not fit their bounds independently. Uniform assembly scale 0.90 produces about 184.5 cm to the crest, including scaled part translations.

## Courtyard placement

Use [asset-metadata.json](generated/asset-metadata.json) for exact bounds, materials and hull counts. `SM_BellTree` has a trunk/base origin, dimensions 762.3 x 647.3 x 686.4 cm, and only one cylindrical base hull (radius 65 cm, height 265 cm). Branches, roots and canopy do not receive a large blocking box. Place `SM_Canopy` at tree-relative Z620; place `SM_Bell` at (-8, -112, 286).

`SM_StoneTile` and `_B` span 200 cm with their walking surface near Z24; place the base at Z-24. `SM_Stair` ascends +Y in six 16 cm steps and has six separate hulls. The arch is about 447 cm wide and 444 cm high with 17 hulls around, rather than across, its opening. Pillars are about 351 cm tall. Fern, ivy, rubble, the shorter buttress root and broken pillar provide reusable growth/ruin details without repeated long root strips.

## UV correction and evidence

Actual Unreal rendering exposed a source defect: several joined meshes exported the primitive `UVMap` channel with zero coordinates on custom faces, although the intended mapping existed in `CraftUV`. Some tube caps and shield rim faces also had collapsed projected UVs.

[uv_tools.py](source/uv_tools.py) now keeps one explicit `CraftUV` channel, projects primitive faces using matching world-space axes, preserves broad shield face mapping and repairs collapsed cap/edge coordinates. Tube side UVs wrap at their seam. The generator calls this same correction so regeneration preserves the fix.

[repair_uv.py](source/repair_uv.py) applied the correction to the existing editable source and selectively exported 18 affected FBXs without regenerating any geometry. Blender exited 0. [uv-repair-report.json](generated/uv-repair-report.json) records the exact subset, passing surface-UV checks and identical geometry, transforms, vertex masks and material slots. Existing custom collision objects were preserved from their original FBXs.

A few geometric bevel slivers remain unchanged: prior native Blender tangent checks found two zero-tangent loops on Wall, four on Stair and two on submicron Head geometry. These are recorded limitations, separate from the repaired UV corruption. The post-correction native Unreal import/render is owned by the integration owner; no warning-free native result is claimed here. The older [round-trip check](generated/roundtrip-check.json) and [import report](import-report.json) describe their recorded runs and may predate this UV pass.

[Manifest rows](manifest-rows.csv) are supplied for the integration owner to merge. All included inputs are original project-authored models, textures and shader recipes created using Blender Python/NumPy and Unreal material nodes; no unresolved third-party rights were introduced.

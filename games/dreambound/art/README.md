# Sculpted courtyard and modular cyborg kit

Original procedural models for the authorized Dreambound beta, 2026-09-08. The visual direction uses style B's ivory ceramic, aged copper/bronze, broad stone planes and restrained blue mechanisms. These are actual modeled assets. The Blender previews are an asset review under studio lighting; they do not establish final Unreal lighting, first-person composition or concept-image fidelity.

[Current material-pass courtyard](preview/material-pass-courtyard.png) · [Initial geometry contact sheet](preview/contact-sheet.png) · [Weapon geometry](preview/weapon.png) · [Guardian geometry](preview/guardian.png)

The source is [create_art.py](../scripts/create_art.py). It produces 20 FBX meshes, 79,274 source triangles in total, two shared 512×512 textures, material definitions, measurement metadata and a compressed editable [Blender scene](Dreambound_Kit.blend). No third-party meshes, samples, texture packs, image-generation service or downloaded brushes were used. The shared textures are original seeded isotropic spectral pigment/pore fields and periodic surface gradients. [Manifest rows](manifest-rows.csv) are for the integration owner to merge.

## Build and import

Run an installed Blender in the background with `--background --factory-startup --python games/dreambound/scripts/create_art.py`. Three Cycles renders use six CPU threads and 40 samples. `-- --skip-renders` regenerates the kit and assembles the sheet from existing previews; use this only when geometry/material appearance has not changed.

`-- --materials-only` updates palette, textures and shaders in the existing Blender scene and renders a courtyard composition from copies of the existing meshes. It never exports or alters geometry. The 2026-09-08 material pass moves stone from ochre to cool slate/green-gray, lifts verdigris, retains ivory/copper equipment, replaces visible diagonal waves with irregular mottling, and uses restrained per-material normal/roughness variation. The original contact sheet predates this palette.

The integration owner runs [import_art.py](../scripts/import_art.py) in Unreal editor Python. It writes only `/Game/Art/Meshes`, `/Game/Art/Materials` and `/Game/Art/Textures`, preserving the requested stable names. It does not edit maps, game code, project settings or audio. It creates a material graph with original surface maps, vertex tint, roughness variation and normal detail, assigns mesh slots, checks centimeter dimensions and checks custom collision hull counts. It writes `art/import-report.json` with actual import results. A failed material compilation or import raises an error.

All 18 materials explicitly enable instanced-static-mesh usage, required by the level's HISM batches. The UE 5.8 API is `MaterialEditingLibrary.set_base_material_usage(material, MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES, True)`; direct access to the older property is deprecated in this engine. Add `-DreamboundMaterialUsageOnly` to the commandlet arguments to update, compile and save existing materials without changing graphs or reimporting meshes/textures. This mode writes `art/material-usage-report.json`.

Use `-DreamboundMaterialsOnly` for a material-quality pass: import only the two shared textures, rebuild/compile/save all 18 graphs, and preserve mesh data and assignments. [Material pass evidence](material-pass-report.json) records the native result.

FBX uses centimeter geometry, +X forward/+Z up, with its axis conversion baked at export. The verified Unreal import uses the legacy FBX importer, unit scale 1 and `force_front_x_axis=False`; forcing the axis again or letting Interchange intercept this import swaps X/Y. The script changes the Interchange FBX switch only for its own import and restores it afterward. Preview shader texture links are omitted from FBX to avoid embedding machine-specific paths; the dedicated Unreal importer assigns textures and materials. UVs and neutral vertex tint remain in the mesh.

## Rigid part transforms

All transforms below are local centimeters with identity rotation and unit scale unless noted. These are separate static meshes for rigid animation, with no skeleton or skin weights.

| Weapon part | Relative location | Pivot and measured size X × Y × Z |
|---|---:|---|
| SM_WeaponBody | (0, 0, 0) | Grip; +X forward; 60.45 × 22.60 × 36.11 |
| SM_ShieldPlate | (31, 0, 10) | Central deployment pivot; 9.40 × 47.20 × 48.33 |
| SM_Core | (33, 0, 10) | Core center, face normal X; 5.62 × 12.60 × 12.60 |
| SM_Forearm | (0, 0, -1) | Wrist/grip; extends toward -X; 35.00 × 21.60 × 21.74 |

`SM_ShieldPlate` already contains all five ceramic lobes and its bronze frame: instantiate it once. Translate/rotate that whole crescent to fold or deploy it. It is not one petal. Keep its central aperture aligned with the core when guarding. For element changes, replace the named `M_Core` material slot and retain bronze/mechanical slots.

| Guardian part | Position relative to feet |
|---|---:|
| SM_GuardianBody | (0, 0, 94) |
| SM_GuardianHead | (0, 0, 156) |
| SM_GuardianArm, right/left | (0, +36/-36, 146) |
| SM_GuardianLeg, right/left | (0, +18/-18, 94) |

The unscaled assembled guardian reaches approximately 200 cm including its crest; uniform assembly scale **0.90** gives an approximately 180 cm sentinel. Scale both part translations and meshes together, or scale their common visual parent. Arms and legs extend downward from shoulder/hip pivots. The symmetric limb mesh is reusable on either side without negative scale. Front faces +X. Capsule/combat collision belongs to the actor, not the decorative rigid parts.

## Environment measurements

Use [asset-metadata.json](generated/asset-metadata.json) for complete bounds, triangle counts, material-slot order and attachment coordinates. Dimensions below are measured from exported source geometry.

| Mesh | Size X × Y × Z, cm | Placement/collision |
|---|---:|---|
| SM_StoneTile | 200 × 200 × 24.11 | Base origin; surface near Z24; 1 hull |
| SM_Wall | 400 × 72.12 × 320.23 | Ground center, X spans wall; 1 hull |
| SM_Pillar | 115.07 × 115.13 × 380.02 | Ground center; 1 hull |
| SM_Arch | 455.04 × 94.15 × 437.05 | Ground center; opening spans X; walk along Y; 15 hulls |
| SM_Bell | 136 × 136 × 150.74 | Origin at lower lip; hanging eye near Z135 |
| SM_Root | 437.42 × 153.60 × 84.08 | Crown origin; extends +X; hide crown against masonry/tree base |
| SM_Crate | 91 × 82 × 86.5 | Ground center; 1 hull |
| SM_Crystal | 30.4 × 28 × 50 | Base center; useful as small scaled attachment/prop |
| SM_TechPanel | 239 × 65 × 319 | Ground center; X width; front -Y; 1 hull |
| SM_Grass | 65.76 × 55.67 × 41 | Ground center; solid double-sided blades |
| SM_TelegraphRing | 200 × 200 × 1 | Ground center; outer radius100, inner94 |
| SM_TelegraphDisk | 200 × 200 × 0.5 | Ground center; radius100 |

The arch collision consists of two side posts and 13 separate voussoir hulls. No convex hull spans the opening. Decorative roots, vegetation, crystals, bell, weapon, limbs and telegraph meshes have no authored blocking collision. Place them accordingly. Randomize tile yaw by quarter turns and small prop variations to reduce visible repetition; preserve tile spacing.

Required material names include `M_Ceramic`, `M_Bronze`, `M_DarkMetal`, `M_Core`, `M_Frost`, `M_Storm`, `M_Ember` and `M_CombatGlow`. The latter exposes VectorParameter `Color` for base color and emission ×3. Other surface recipes and stable names are in [materials.json](generated/materials.json).

## Observed checks and limits

- Blender 5.2.1 LTS generated and rendered the real kit. Visual inspection led to a second pass: darker guardian armor/single visor, irregular flagstones, bark ridges and lower crystal emission.
- All 20 FBX files were reimported in a fresh background Blender session. Measured dimensions matched within 0.02 cm; every mesh retained UVs, vertex tint, material names and its expected custom collision count. [Round-trip evidence](generated/roundtrip-check.json).
- Native Unreal 5.8.2 import passed on 2026-09-08 using the authorized `UnrealEditor-Cmd` Python commandlet with `-unattended -nop4 -nosplash -NullRHI`: **20 meshes, 18 materials and 2 textures**, exit 0, zero errors/warnings. [Import evidence](import-report.json) records dimensions, signed X/Z origins, material assignments and collision hulls. Initial checks found and fixed VertexColor's unnamed output and the Interchange FBX interception/axis mismatch. A subsequent complete rerun also passed, confirming reimport behavior. The gun extends +X from its grip and the cuff remains behind it.
- The integrated renderer then exposed missing HISM usage flags. A material-only native commandlet enabled the usage on all 18 materials, verified it through `has_material_usage`, recompiled and saved each one: exit 0, zero errors/warnings, no mesh or texture imports. [Material usage evidence](material-usage-report.json). The integration owner repeats the rendered game check to verify shader appearance.
- The first actual game capture showed beige stone with directional woven-looking texture. The bounded palette/texture pass imported 18 rebuilt materials and two textures successfully: exit 0, zero errors/warnings, instancing still enabled on every material, no mesh imports. Its current Blender review uses existing geometry. Final Unreal appearance depends on the integration owner's simultaneous lighting correction and rendered capture.
- The final FBX source scan found zero embedded workspace paths across all 20 files. Original shader recipes, image-generation math and asset manifest rows are included; no unresolved third-party input was introduced.
- Final editable-Blender portability audit passed on 2026-09-08: both file images use Blender-relative `//generated/` paths and resolve to the included PNGs; the two viewer images have empty paths; there are no absolute machine image paths or linked Blender libraries. The accepted `.blend` needed no modification. Keep its adjacent `generated` folder when moving the editable source.
- No foreground UI or real audio playback was used. First-person attachment pose, animated intersections, Unreal material appearance, camera framing and gameplay collision still require the integrated build check.

# Original concept-demo meshes

Created 2026-09-12 by Game Studio / Codex using the original procedural [GenerateMeshes.py](../../scripts/GenerateMeshes.py) source and installed Blender 5.2.1 LTS. No downloaded mesh, texture, scan, copied model, raster generation, or third-party art is an input. The FBX files contain only generated mesh geometry and one neutral material slot. No external attribution is required by an incorporated art source. The studio has not selected a repository-wide redistribution license; this record does not apply a new public license to the game or its assets.

Run from the game directory with Blender available on the command path:

```text
blender --background --factory-startup --python scripts/GenerateMeshes.py -- --preview
```

The generator recreates all seven FBX files, the [analysis](mesh-analysis.json), the proposed [manifest rows](manifest-rows.csv), and optional [mesh inspection image](mesh-preview.png). Geometry is deterministic. FBX exporter metadata can vary between runs, so file hashes identify a generated output rather than promise byte-identical exports. The root integration owner adds the supplied rows to the shared asset manifest.

| Asset | Bounds X / Y / Z, cm | Vertices | Triangles |
| --- | --- | ---: | ---: |
| SM_Washer | 28 / 28 / 4 | 512 | 1,024 |
| SM_Bolt | 30 / 15.225 / 13.51 | 848 | 1,684 |
| SM_Plate | 24 / 17 / 4 | 96 | 188 |
| SM_Tangle | 65 / 49 / 20 | 768 | 1,424 |
| SM_Ring | 42 / 42 / 6 | 256 | 512 |
| SM_MagnetBody | 80 / 70 / 18 | 368 | 732 |
| SM_Tray | 1,000 / 620 / 20 | 96 | 188 |

All origins are at their bounding-box center, transforms are identity, and each FBX contains one combined mesh with a single material slot. Bolt length lies on X. Washer and ring lie in XY. The magnet opens toward -Y. The tray is only a beveled base; raised edges, furnace, magnet tips and earned coil remain engine-owned. Tangles consist of intersecting bent rods in one mesh, suitable for visual presentation rather than concave physical simulation.

The visual direction is chunky machined silhouettes with soft edge highlights: broad washer faces, a clear hex bolt head, squared plates, compact tangled rods and a rounded U profile. These fit the charcoal tray, teal magnet, copper scrap and warm furnace direction. Dynamic color, roughness and emission belong to the engine material. The meshes need no textures or texture UVs. Do not use the neutral Blender preview material as the final gameplay palette.

## Executed verification

- Generator completed in Blender 5.2.1 LTS and exported every requested FBX.
- Each file was imported back into the centimeter Blender scene. Assertions passed for one mesh, one material slot, centered origin, finite vertex coordinates and preserved bounds within 0.02 cm. Exact results and SHA-256 hashes are in the analysis file.
- The rendered mesh inspection image was opened and visually reviewed. All seven forms are visible, edge highlights are coherent, the magnet opening is intact, and there are no visibly inverted faces. The image resizes pieces independently for inspection; it is **not gameplay, not actual relative scale, and not store art**.

## Engine integration still required

Import under `/Game/MagnetSweep/Meshes`, replacing slot 0 with the game's dynamic surface material. Export uses centimeter metadata, `FBX_SCALE_UNITS`, -Y forward and Z up. Verify one Unreal import against the bounds above, inspect the magnet opening direction, and check normal/shading behavior in the actual camera and lighting. The pieces need no automatic collision for the planar interaction model. The largest intended cargo and linked burst must remain readable in engine; a Blender render does not prove that condition or performance.

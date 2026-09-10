# LivingWorld habitat dressing

Original reusable set for the current Windows first-person courtyard. This adds two habitats within the existing spring-garden biome: damp channel margins and dry ruin/tree gardens. It does not represent multiple completed biomes or a changed route generator.

The palette uses olive/fern greens, warm layered bark, pale limestone, weathered green ceramic and restrained cream/coral flowers. Teal insects remain small environmental accents. Organic silhouettes use curved blades, divided fronds, hollow broken wood, continuous woody forks, tiered fungi and irregular pottery. Combat space and threats retain the strongest visual priority.

`create_living_world.py` regenerates all twelve FBXs, ten original 2048×2048 surface/normal maps, metadata, Blender source and two labelled source renders. The five texture families are leaf, bark, end grain, stone and ceramic. They are analytic project-authored pigment/roughness/weather masks and tangent-space normal maps, not downloaded photographs. The scrub uses a small voxel union and smoothing pass to join its woody forks. The complete source set contains 117,282 triangles; models have centimetre units, two UV channels, authored vertex masks and no collision.

| Family | Triangles | Grounding / use |
| --- | ---: | --- |
| Reeds | 9,008 | Rooted damp bank clumps |
| Water lily | 4,108 | Actual basin water surface, not soil |
| Fungus log | 14,420 | Hollow, plated fallen wood at habitat anchors |
| Shelf fungi | 3,156 | Rooted stump with tiered fans and gills |
| Fern rosette | 10,484 | Divided fronds and coiled young growth |
| Meadow flowers | 8,708 | Dry garden companions and paving-edge pockets |
| Twisted scrub | 46,798 | Rounded, fused forks with leaves and seedpods |
| Cracked urn | 7,434 | Hollow broken ceramic at dry ruin anchors |
| Fallen lintel | 4,538 | Carved, chipped masonry and loose fragments |
| Root fan | 6,160 | Tapered roots, forks and curled litter |
| Dragonfly | 1,584 | Damp-margin airborne accent |
| Garden moth | 884 | Dry flower companion |

`import_living_world.py` selectively imports `/Game/Art/LivingWorld`, reusing the proven local FBX transport and original material construction. It does not reimport the large Reverie/TRELLIS landmarks. Materials animate anchored leaf tips and insect wing tips; insects share a gentle body bob. These are ambient material motions, not rigged animals or simulated ecology. All imported meshes are used by name in `DBRecoveryScene.cpp`; an individual seed can reject a placement to protect playable space.

The runtime checks a footprint margin against paths, entry, ward and practice pads. Soil props are traced onto an existing floor; elevated rim/cover contacts and outer descending shoulders are rejected. The meshes embed their roots by a further 2 cm. Lily offsets fit inside the existing 242.5 cm water disc and use its 35 cm height plus 0.7 cm. New decoration uses noncolliding HISM batches and culls beyond 65 m. The original staircase, gateways, floor heights and architecture are retained.

Regenerate with installed Blender in background mode using `--threads 4 --python-exit-code 1 --python games/dreambound/scripts/create_living_world.py`. Import with the full Unreal editor using the project path, `-ExecutePythonScript=D:/Game-Studio/games/dreambound/scripts/import_living_world.py -unattended -nop4 -nosplash -NullRHI`. The project cook must include `/Game/Art/LivingWorld`.

The two `*-source.png` images are CPU Blender material/geometry reviews, not game screenshots. Actual in-engine appearance, planting contacts and ambient motion require the integration owner's packaged view. The source renders establish visible rounded branches, layered bark/end grain, fine plant silhouettes and weathered ceramic; they do not establish AAA fidelity or owner acceptance.

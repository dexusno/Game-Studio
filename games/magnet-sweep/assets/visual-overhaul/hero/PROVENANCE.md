# Hero electromagnet — provenance and integration

Created 2026-09-13 for the owner-authorized Magnet Sweep graphics overhaul. This directory contains an original design reference and its derived local 3D evaluation asset. Root is the single writer of the shared assets/manifest.csv; this directory returns manifest-rows.csv.

## Origin and tools

- `hero-electromagnet-reference.png`: OpenAI built-in image-generation tool, original prompt in `hero-electromagnet-prompt.txt`. No external reference asset, trademark or copied design was supplied. Actual1402×1122 RGBA;875,811 fully transparent pixels. SHA-25647e485e0c335b0e1f2beffe1746718ce5404711adecadb017e7eca594b769c6c. Original generated PNG copied unchanged. Applicable OpenAI tool generation terms; a specific model version was not exposed by the tool and is not claimed.
- Local TRELLIS.2 inference uses Microsoft code revision75fbf0183001ed9876c8dbb35de6b68552ee08bd and TRELLIS.2-4B checkpointaf44b45f2e35a493886929c6d786e563ec68364d. It uses the already installed and authorized DINOv3 encoder, genuine input alpha and the existing optional-rembg/encoder compatibility patches. No BRIA background model is used. Sampling1536_cascade, seed42,98,304-token ceiling; no accepted resolution fallback. Full source stays ignored under the studio .local/ai3d/magnet-hero-v1-1536 directory.
- Blender 5.2.1 finishing is authored by Game Studio / Codex in `scripts/GenerateHeroVisuals.py`. The delivered version retains the source UV mapping, validates duplicate/degenerate faces, normalizes the copied geometry, reduces it conservatively, extracts 2K PBR maps and exports FBX/GLB. Source geometry and 4K maps are never overwritten. Exact hashes and results are in `generation-evidence.json` and `mesh-measurements.json`.

## Delivered result and inspection

| Check | Actual result |
| --- | --- |
| TRELLIS generation | Actual 1536; 40,733 shape tokens; raw 14,195,045 vertices / 28,554,086 faces |
| TRELLIS exchange source | 985,930 triangles in GLB; Blender imported 985,925 (five fewer degenerate triangles) |
| Final game mesh | 159,999 triangles; 265,264 vertices including source UV seams; one material slot |
| Dimensions | 92.7506 × 100.2368 × 31.8947 cm; +Z top; opening toward -Y |
| Pivot | Original source centered in XY and grounded before reduction; integration should use measured mesh bounds to center/size |
| Maps | 2048² base color, 2048² metallic/roughness, neutral 4² tangent normal |
| Exchange verification | FBX and GLB both reimport exactly 159,999 triangles, one mesh, one material and UVMap; GLB contains all three maps at their documented sizes |
| Visual inspection | Actual Blender top/front/rear renders show the U silhouette, separate metal/enamel/copper regions, layered hardware and rear winding |

The first aggressive 40K-target reduction with a ray-baked normal produced false angular cracks and failed exact reimport face counts. A fresh-UV bake also produced unusable maps and was rejected on visual inspection despite passing counts. The final version keeps more physical geometry and uses a neutral tangent map; it does **not** claim a successful detail-normal bake. A rejected first-pass render is retained as `rejected-source-uv-preview.png`, strictly evidence. The actual source atlas and generated geometry carry the visible detail. Larger counts alone did not establish this decision: it was selected after the new front/top/rear images removed the false-crack artifact.

Some generator softness and irregular coil spacing remain at close inspection; the tiny cyan insets are less crisp than the reference. Root can add narrow dynamic indicator geometry for powered feedback. These are limitations of the delivered asset, not hidden perfection claims. Overall geometry and material identity are substantially richer than the old untextured procedural horseshoe. Its appearance under the actual gameplay camera remains the next acceptance check.

## Rights status

This is an original generated asset for local evaluation. Commercial pipeline clearance remains pending because the installed inference stack includes third-party runtime dependencies; this is not an assertion that generated mesh outputs automatically inherit software licenses. Resolve the exact dependency setup before release inclusion. No tool code, checkpoint or encoder weights are included in the game asset.

Primary sources checked2026-09-13:

- [TRELLIS.2 MIT license](https://github.com/microsoft/TRELLIS.2/blob/main/LICENSE).
- [DINOv3 license](https://huggingface.co/facebook/dinov3-vitl16-pretrain-lvd1689m/blob/main/LICENSE.md); existing normal access was used, no gate bypass.
- [nvdiffrast license](https://github.com/NVlabs/nvdiffrast/blob/main/LICENSE.txt), with noncommercial software-use restriction also documented in the studio's earlier AI3D review. Exact pipeline dependency replacement or permission remains a release task.

## Integration contract

FBX mesh `SM_HeroMagnet`, material slot `M_HeroMagnet`; centimeter units with source UV0 retained. `T_HeroMagnet_BaseColor` uses sRGB; `T_HeroMagnet_MR` is linear with G roughness / B metallic. `T_HeroMagnet_Normal` is a neutral OpenGL tangent normal map; it can be omitted, and invert green if connected through Unreal's normal convention. Do not replace the PBR slot with one flat tint. Preserve proportions: at 130 cm wide, the measured mesh becomes approximately 140.5 cm long and 44.7 cm thick. No custom collision is needed for the decorative mesh: use the game's existing magnet gameplay model.

Authoring renders are labelled hero-three-quarter, hero-top and hero-rear. These represent the actual exported-asset preparation under neutral studio lighting, not gameplay or store screenshots. Final in-engine binding, camera scale, dynamic highlights, performance and player judgment belong to integration verification.

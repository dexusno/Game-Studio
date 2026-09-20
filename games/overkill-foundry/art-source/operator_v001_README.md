# Mara operator v001 — provisional source candidate

Original provisional mechanic/mercenary for Overkill Foundry. The owner has not
selected her appearance or approved the source/in-engine result. This asset is
separate from the existing mechanical Mara gun and rear gathering claw.

The source is a recognizable clothed human with a copper bracer, rather than
assembled primitive anatomy. It is suitable for an in-engine composition and
animation review. It is **not approved final art**: facial sculpting, hair,
clothing edges and generated back detail remain below the intended F.I.S.T.
finish. The close face render deliberately exposes that limitation.

The generated reference has a human face, compact dark hair/bun, brass goggles,
ivory work shirt, dark leather vest/trousers, copper forearm bracer, leather tool
belt and boots, and a short rust-red scarf. No external game asset or character
likeness was supplied. STYLE/16B/16C informed the brief, not sampled pixels.

## Source and route

- `operator_v001_prompt.txt`: exact original prompt, used with OpenAI's built-in
  `image_gen` tool on 20 September 2026. Exact model version was not exposed.
- `../assets/production/operator-v001/Mara_SourceReference.png`: original output,
  copied without modification into the workspace, 1024 × 1536 RGBA, 1,874,400 bytes,
  SHA-256 `2330039a7561bd0f2878e26524a4af039397a4b9eaa7b5aa336dba919c3ae1c9`.
  1,133,086 pixels have alpha zero. RGB under transparent pixels is not a backdrop.
- `operator_v001_generate.py`: new offline TRELLIS raw-array runner. The installed
  optional-rembg and DINO compatibility patches are retained. Missing BRIA weights
  are unnecessary because the input has real alpha. No downloads, new gates,
  account actions, paid requests or installation changes are performed.
- Raw arrays, Blender source, interchange meshes, clips and source renders remain
  ignored under `.local/overkill-foundry/art/operator-v001`.

The runner blocks imports of `nvdiffrast`, `nvdiffrec_render` and
`o_voxel.postprocess`. It supplies only O-Voxel's package namespace to avoid its
eager exporter import, then loads the existing flexible dual grid decoder and
native extension. TRELLIS inference and CuMesh simplification produce geometry;
FlexGEMM samples generated material values at the vertices. Blender performs UV,
rigging and export. Original NumPy code rasterizes the transferred point-material
attributes into the UV atlas on the CPU. No renderer library is silently
substituted. Source previews use Blender Cycles on CPU only.

The actual offline generation completed on 20 September 2026: seed 3701,
requested and actual resolution 1536, 78.007 s model load and 101.624 s inference.
The runtime check recorded no excluded module in `sys.modules`. Pinned cached
model revisions, installed package versions and source hashes are preserved in
`generation-report.json`. The raw surface has 8,102,524 triangles; the CuMesh
intermediate has 346,694; the final hero surface has **109,998 triangles and
54,995 vertices**, one PBR material, 23 bones, and at most four normalized weights.
No new model, dependency, background-removal weights or downloaded asset was used.

The executing route is `Trellis2ImageTo3DPipeline` (installed PyTorch/Transformers
and cached DINOv3), `o_voxel.convert.flexible_dual_grid_to_mesh` plus its native
extension, CuMesh simplification, and FlexGEMM vertex material sampling. The
report records the excluded-module check and installed package inventory; a
complete Python `sys.modules` dump was not retained. Upstream renderer and GLB
postprocessing paths did not execute. Blender is a separate process.

## Deliverables and inspection

Small original textures and executed reports live under
`assets/production/operator-v001/`. `published-evidence.json` records their
hashes. `renders/` contains actual source front, back, face, ready, load, recoil,
and operator-with-gun views. These are source renders, not Unreal captures or
owner approval. `operator_v001_manifest-rows.csv` is for the shared manifest owner.

Large editable `Mara_Surface.blend`, `Mara_Operator_Source.blend`, raw arrays,
`meshes/MaraOperator.fbx`, `meshes/MaraOperator.glb`, and one FBX per action stay
under the ignored `.local/overkill-foundry/art/operator-v001/` directory.
`MaraOperator.fbx` is an unanimated rest-pose skin. The GLB packs all four clips and
three textures. The rig source includes the original high surface for reworking.

The cleaned surface uses a 14 mm centered shell, 3.5 mm voxel reconstruction,
three relaxation passes and adaptive reduction. This closes the major original
surface tears. BaseColor is sRGB; ORM is linear, R=1, G=roughness, B=metallic.
The normal map is a flat tangent normal: actual geometry supplies shape detail.
The 2K atlas has about 27.5% covered texels before padding. It is not a finished
artist retopology or a close portrait asset. No facial or individual finger rig,
cloth simulation, physics asset or additional runtime LOD is supplied.

## Rig, animation and placement contract

The source faces +X, Z up, metre units; Unreal conversion is `(x,-y,z)*100`.
Rest bounds in metres are `(-.213595,-.486724,-.006774)` through
`(.214651,.486473,1.786094)`; the resulting dimensions are approximately
42.82 × 97.32 × 179.29 cm. The input was normalized to 178 cm before shell repair.
Do not apply an extra actor scale of 100 after import.

| Clip | Duration | Intended cosmetic use |
| --- | --- | --- |
| `MO_idle` | 2.000 s, loop | Preparation/aiming stance and breathing |
| `MO_load` | 0.700 s | Brief left-hand reach toward the feed guide after a successful load |
| `MO_fire` | 0.333 s | Backward shoulder/hip brace after a committed shot |
| `MO_recovery` | 0.467 s | Release tension, then return to idle |

All clips are authored at 30 fps with planted feet and zero root motion. They
never grant ammunition, fire projectiles, change resources or decide whether an
input is valid. Runtime event mapping and blending belong to the integration
owner. Grip and portrait bones are nondeforming attachment points. Exact rest
heads, bone names, clip lengths and ready positions are in `asset-report.json`.

The proposed Unreal operator origin is **(-496,+55,+0.7) cm**, yaw 0, for the
existing gun at (-350,0,+1.5) cm. `stance-review.json` records local/world grip
points and nearest evaluated gun-surface points. At idle, the left grip is about
(-453.70,+25.46,128.01) cm, at the near feed guide; the right is about
(-443.85,+42.72,110.19) cm, beside the copper return pipe. Their nearest-surface
distances are about 0.30 cm and 2.46 cm. These are bone-point checks, not articulated
finger or collision proof. The copper pipe is an existing service part, not a
newly invented control handle. The existing handwheel is farther forward.

This fixed stance does not track the rotating cradle. The integration should
inspect yaw/pitch extremes and use bounded cosmetic arm adjustment or a matched
operator mount if needed. Do not attach the whole operator directly to the gun's
existing `operator_attach`: that point is at (-482,+23,+3.5) cm with this gun
placement and would put her too close to its mechanism for the supplied pose.

## Rebuild and validation

Use the installed Blender executable; these commands are run from the repo root.
All Blender scripts explicitly select CPU Cycles and six threads.

```powershell
$operatorOut = '.local/overkill-foundry/art/operator-v001'
$operatorConfig = Get-Content config.local.json -Raw | ConvertFrom-Json
$operatorBlender = $operatorConfig.tools.blender
& $operatorBlender --background --threads 6 --python-exit-code 1 --python games/overkill-foundry/art-source/operator_v001_prepare.py -- --output $operatorOut
& $operatorBlender --background --threads 6 --python-exit-code 1 --python games/overkill-foundry/art-source/operator_v001_rig.py -- --output $operatorOut
& $operatorBlender --background --threads 6 --python-exit-code 1 --python games/overkill-foundry/art-source/operator_v001_verify.py -- --output $operatorOut
& $operatorBlender --background --threads 6 --python-exit-code 1 --python games/overkill-foundry/art-source/operator_v001_stance.py -- --output $operatorOut
python games/overkill-foundry/art-source/operator_v001_publish.py --output $operatorOut
```

The generator requires the already installed WSL TRELLIS trial launcher described
in `art-source/README.md`. Its CLI requires `--source`, `--output`; defaults are
seed 3701 and resolution 1536. It resolves only existing pinned local snapshots
with offline environment flags. `--probe-only` imports without GPU inference.
Coordinate any actual GPU generation with the Unreal owner.

`operator_v001_verify.py` actually reimports the FBX skin and each clip, reads the
GLB container, and checks bone inventory, scale, UV, one material, 2K textures,
influences, normalized weights, clip coverage/duration, source-vs-exported bone
trajectories, foot placement, loop continuity, root motion and skin stretch.
Blender's FBX importer guesses connected bone tips, suppressing valid translation
keys; the verifier restores all source bones' `use_connect=False` in edit mode
and uses `anim_offset=0`. It does not change exported keys or rest transforms.
The uncorrected default Blender import is not claimed as passing.

`unreal/Tools/import_operator_v001.py` is staged for the integration owner and
has not been executed by this source task. It imports into
`/Game/Cinderwall/Operator`, reuses `M_Cinderwall`, validates source hashes, bounds,
compressed motion, foot/root stability and ready grip conversion, and writes
`Saved/ArtImport/operator-v001-import.json`. It does not mutate a level or runtime.

The integration owner subsequently executed OperatorArt203155 and the combined
Editor203232/Game203314 builds, Smoke203338 and ArtProbe203530. The exact engine
evidence and its remaining limits are in
[operator-v001-runtime.md](../unreal/Tools/operator-v001-runtime.md). The source
checks above remain separate from compressed engine animation and native play.

## Primary terms reviewed — 20 September 2026

This records the actual production dependencies and use; it is not worldwide
copyright clearance or a claim that generated designs are unique.

| Component | Primary source and production implication |
| --- | --- |
| TRELLIS.2 code/model and bundled O-Voxel decoder | [Microsoft MIT license](https://github.com/microsoft/TRELLIS.2/blob/main/LICENSE) and [model card](https://huggingface.co/microsoft/TRELLIS.2-4B). Commercial software use permitted; include copyright/permission notice if distributing copies or substantial portions of that software. Model/code are not bundled in the game. |
| CuMesh | [Author MIT license](https://github.com/JeffreyXiang/CuMesh/blob/main/LICENSE). Same software-redistribution notice condition; installed decimator is a build tool, not shipped. |
| FlexGEMM | [Author MIT license](https://github.com/JeffreyXiang/FlexGEMM/blob/main/LICENSE). Same software-redistribution notice condition; installed sampling code is not shipped. |
| DINOv3 encoder | [Meta DINOv3 license](https://github.com/facebookresearch/dinov3/blob/main/LICENSE.md), updated 19 August 2025. Royalty-free use grant, usage/trade-control conditions and research-publication acknowledgment requirement. Redistribution of DINO Materials/derivatives requires its agreement. No DINO code/weights are shipped. Existing normal access/owner approval was recorded in the studio's 9 September tool setup; no new agreement is entered or gate bypassed here. |
| nvdiffrast, nvdiffrec | [nvdiffrast terms](https://github.com/NVlabs/nvdiffrast/blob/main/LICENSE.txt), [nvdiffrec terms](https://github.com/NVlabs/nvdiffrec/blob/main/LICENSE.txt). Their noncommercial-use conditions are incompatible with this asset production route, so imports and use are explicitly excluded. Upstream `to_glb` and renderer helpers are not used. |
| Blender | Existing Blender 5.2.1 LTS. [Official licence](https://www.blender.org/about/license/) distinguishes GPL software from authored output, which may be used commercially. No Blender executable is distributed in the game. |
| Source image | Original OpenAI built-in image generation. [OpenAI terms, Content section](https://openai.com/policies/terms-of-use/) assigns output rights as between the user and OpenAI to the extent permitted by law, while noting outputs may be similar to others. Existing account/service terms apply. Exact prompt and unmodified source output are preserved; generation is disclosed. |

No new external art pack, photograph, mesh, texture or font is incorporated.
The Game Studio distribution licence remains unset; an asset manifest row is
provided for the shared manifest owner after production verification.

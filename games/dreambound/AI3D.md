# Local AI 3D trials

Updated 2026-09-09. Klaus authorizes local installation and testing of TRELLIS.2 and Hunyuan3D-2.1. Quality against art anchor B remains the selection criterion. No purchase or adoption into the playable game is implied.

## Installed environment

The existing Ubuntu 22.04 WSL2 distribution runs on the Windows workstation and sees its RTX 4090 (24GB). No reboot, Windows driver replacement, or new WSL distribution was needed. Separate Miniforge Python 3.10 environments keep the two dependency stacks apart. A private CUDA 12.4.1 toolkit supplies compilation tools. The networked 3090 has not been configured.

Private paths are recorded in `config.local.json` under `tools.ai3d`: `wslDistro`, `wslRoot`, and `windowsOutputRoot`. Source checkouts, weights, environments, install logs and trial outputs stay outside Git under that configured install root. They survive a Codex restart. The saved project's local configuration is also updated; a fresh worktree needs those existing local settings.

| Component | Installed source or runtime | Current evidence |
| --- | --- | --- |
| TRELLIS.2 | Official source `75fbf0183001ed9876c8dbb35de6b68552ee08bd`; PyTorch 2.6.0+cu124; torchvision 0.21.0+cu124; FlashAttention 2.7.3 | Complete waymarker generation/export succeeded at actual resolution1536 after a Transformers5 DINO adapter correction. Raw geometry and 4K PBR export inspected front/rear in Blender. |
| TRELLIS.2-4B weights | Microsoft revision `af44b45f2e35a493886929c6d786e563ec68364d`; sparse decoder from TRELLIS-image-large revision `25e0d31ffbebe4b5a97464dd851910efc3002d96` | Approximately 16GB of public model files cached. |
| Hunyuan3D-2.1 | Official source `82920d643c0dc2f7bfd7255f45f62d386edfe60c`; PyTorch 2.5.1+cu124; torchvision 0.20.1+cu124 | Full shape and PBR trial succeeded. Both native extensions import; dependency consistency passes. Raw dense mesh and 4K maps retained; exported GLB verified and rendered in Windows Blender. |

## Windows launch commands

Run from the studio checkout in PowerShell:

```powershell
.\games\dreambound\scripts\Start-AI3D.ps1 -Tool Trellis
.\games\dreambound\scripts\Start-AI3D.ps1 -Tool Authenticate
.\games\dreambound\scripts\Start-AI3D.ps1 -Tool Hunyuan --help
```

The TRELLIS launcher opens its official local Gradio service at `http://127.0.0.1:7860` after its model access checks. Authentication uses Hugging Face's browser device flow; no token needs to be pasted into a task. The full default interface needs [Meta DINOv3](https://huggingface.co/facebook/dinov3-vitl16-pretrain-lvd1689m) and [BRIA background removal](https://huggingface.co/briaai/RMBG-2.0). Main TRELLIS weights are public. September 9 update: local device login succeeded, Klaus submitted the DINOv3 request and later confirmed approval. Encoder revision `ea8dc2863c51be0a264bab82070e3e8836b02d51` then downloaded successfully using normal account access. No gate bypass or encoder replacement was used.

The `TrellisTrial` launcher uses [trellis_trial.py](scripts/trellis_trial.py) for a real transparent RGBA reference. A small [optional-rembg patch](scripts/trellis_optional_rembg.patch) leaves default behavior unchanged and permits skipping BRIA when alpha already supplies the foreground mask. DINOv3 is still required: Microsoft's [official pipeline configuration](https://huggingface.co/microsoft/TRELLIS.2-4B/blob/main/pipeline.json) names that exact encoder. The runner retains raw geometry/attributes and records all export processing. Its defaults are upstream 1024_cascade, seed42, 1M export-face target and 4K PNG PBR maps; this trial explicitly selected1536. Requested1536 output fails explicitly if upstream's token limit lowers its actual resolution. Windows argument forwarding, actual inference/export and Blender inspection have completed.

The first real TRELLIS attempt loaded the approved encoder but stopped before sampling: current Transformers 5.17.0 nests DINO's layers under `model.model.layer`, while upstream TRELLIS still addresses `model.layer`. This matches [upstream issue147](https://github.com/microsoft/TRELLIS.2/issues/147) and [open PR156](https://github.com/microsoft/TRELLIS.2/pull/156). A reviewed [compatibility patch](scripts/trellis_dinov3_compat.patch) selects the correct existing encoder container; the per-layer operations, final normalization and weights are unchanged. The retry completed generation and export. The runner records the extractor's source hash as well as the pipeline patch. The failed attempt's metadata remains in `waymarker-v1-1536`; the successful retry uses `waymarker-v1-1536-compat`.

Microsoft's [hosted demo](https://huggingface.co/spaces/microsoft/TRELLIS.2) was inspected in the browser and its controls loaded. No reference was uploaded or generation started. [Fal](https://fal.ai/models/fal-ai/trellis-2) lists $0.25/$0.30/$0.35 per 512/1024/1536 request on September 9; no purchase or paid run is authorized. Klaus chose local Hunyuan while DINO approval was pending; that access condition is now resolved.

## Waymarker trial — September 9

Klaus requested generating the actual [waymarker reference](art/references/waymarker-v1.png) with Hunyuan and checking current documentation/settings first. The built-in image generator produced this original 1254x1254 RGBA image; [prompt and provenance](art/references/waymarker-v1-prompt.txt). Alpha validation found 1,123,219 fully transparent pixels. Preserve this exact reference for subsequent comparisons.

Installed Hunyuan 2.1 source HEAD matches the current official remote `82920d643c0dc2f7bfd7255f45f62d386edfe60c`; current Hub weights match `0b94677654c57bb9a6b6845cd7b704ccf551d327`. Version checks used `git ls-remote` and the official Hub model API. As of September 9, 2.1 is the latest verified publicly downloadable Hunyuan pipeline combining shape generation and PBR texturing, not the newest product carrying the Hunyuan name.

The newer [Hunyuan3D-Omni](https://github.com/Tencent-Hunyuan/Hunyuan3D-Omni) provides public shape-generation weights and extra geometry controls, while [Hunyuan3D-Part](https://github.com/Tencent-Hunyuan/Hunyuan3D-Part) provides segmentation/completion of parts; neither includes a replacement PBR stage. [Buffalo1.0](https://tencent-hunyuan.github.io/Hunyuan3D-Buffalo1.0/) announced its paper/project in August 2026 but still labels code as forthcoming; no official inference/checkpoint release was verified. Tencent's [official hosted API](https://intl.cloud.tencent.com/zh/document/api/1284/75540) accepts model 3.1, but no public local 3.1 weights were found. These are distinct future comparisons, not updates silently substituted into this trial.

Settings selected from primary documentation and installed source:

- Shape: 50 steps and guidance 5.0, the [FlowMatching pipeline defaults](https://github.com/Tencent-Hunyuan/Hunyuan3D-2.1/blob/main/hy3dshape/hy3dshape/pipelines.py). Extraction uses 512, the upper value exposed by the [official GUI](https://github.com/Tencent-Hunyuan/Hunyuan3D-2.1/blob/main/gradio_app.py), instead of the initial sample's 384; this tests finer roots and carving. Seed 1234 and 8000-query chunks are fixed. Extra denoising steps have not been proven better on this object.
- Paint: nine selected views and 768 view resolution are within the [official demo's supported choices](https://github.com/Tencent-Hunyuan/Hunyuan3D-2.1/blob/main/demo.py). Preserve 4096 texture maps and raw geometry; no optional 40k-face remesh. The existing stage offload and VAE slicing keep shape and paint separate on the 24GB GPU.
- The installed paint pipeline uses UniPC, 15 texture denoising steps, guidance 3.0 and paint seed 0. These are upstream choices, distinct from shape settings. A 4K export is not native 4K diffusion: reference processing, 768 generated views, upscaling, projection and baking still limit actual detail. No unverified sampler/encoder replacement or reduced-precision quantization is applied.

The complete waymarker run succeeded. [Recorded result and export verification](evidence/ai3d-hunyuan-waymarker.json):

| Stage | Actual result | Elapsed | Torch peak allocated / reserved |
| --- | --- | --- | --- |
| Shape | 369,974 vertices; 739,960 triangles; original GLB/PLY retained | 89.623 seconds | 8.652 / 9.660 GiB |
| Paint | Nine views at 768; 4096x4096 base color, roughness and metallic PNGs | 348.946 seconds | 13.829 / 16.760 GiB |
| Export | OBJ and GLB both retain 739,960 triangles; embedded 4K PNGs and PBR connections verified | Included in paint stage | No additional inference |

Windows Blender 5.2.1 rendered the actual raw and textured GLBs from front and rear, using Cycles at 48 samples. No sculpting, texture repainting or material cleanup was applied. The model captures the tall stepped silhouette, thick roots and visible gaps, but fine carving is largely absent from the geometry. Painting invents a second medallion below the reference's single plate, distorts its symbols and repeats the decoration on the unseen back. Stone and bark look softened, and the surfaces lack the reference's distinct material response. Exported metallic values span only 0–74/255 and roughness 193–249/255; the bronze is consequently far from fully metallic. These are observations from this one run, not a verdict on every possible Hunyuan result.

**Quality assessment: useful comparison baseline, below the required finished-asset standard.** Dense topology and 4K maps do not compensate for the lost relief or inaccurate ornament. This result has not been imported into Unreal or accepted as game content. Further generation should retain this baseline; any repaint should target the duplicated plate and material separation rather than simply increasing steps or texture dimensions.

The configured WSL output root contains `Hunyuan3D-2.1/outputs/waymarker-v1-512`; the Windows output root contains `hunyuan-waymarker-v1-512`. Both preserve the original mesh, painted GLB/OBJ and maps, stage reports and export verification. The Windows copy additionally contains the reference, logs, `geometry-previews/` and textured `previews/` with front/rear PNGs and `preview-scene.blend`. Use the saved scene or GLB for interactive inspection. Hunyuan generation and rendering have exited.

Hunyuan's installed `hunyuan-run` CLI accepts `all`, `shape`, or `paint`, with `--image` and `--out` paths in WSL form. `all` runs separate child processes so shape allocations are released before painting. Use `/mnt/d/...` for files on the Windows D: drive. Example syntax:

```powershell
.\games\dreambound\scripts\Start-AI3D.ps1 -Tool Hunyuan all --image /mnt/d/path/reference.png --out /mnt/d/path/trial-output
```

To reproduce the waymarker settings, append `--steps 50 --guidance-scale 5.0 --octree 512 --chunks 8000 --seed 1234 --views 9 --paint-resolution 768`, use the preserved reference, and choose a new output directory. Its existing output is the comparison baseline. The input already has valid alpha; background removal is unnecessary.

Run one generator at a time. The official Hunyuan demo's `low_vram_mode` does not actually offload its two stages, so use the staged runner on this 24GB card. Defaults retain 50 shape steps, octree 384, 8 paint views at 768, the original raw mesh, and 4096px texture export. The optional `--paint-remesh-40k` flag is an explicit simplification choice, not the default. The stock paint exporter halves 4096px maps unless corrected; the local runner preserves full size and uses lossless PNG. A separate, checksum-verified official Blender 4.0.2 handles conversion because the upstream pinned bpy 4.0 wheel was unavailable; the installed Windows Blender remains available for finishing.

## Evidence and next action

Installation smoke used upstream `assets/demo.png`: a penguin holding a HY3D sign. It is not a game asset or an anchor-B quality test.

| Actual stage | Result | Elapsed | Torch peak allocated / reserved |
| --- | --- | --- | --- |
| Shape | 744,862 faces; raw GLB/PLY saved | 60.35 seconds | 7.63 / 8.53 GiB |
| Paint | Dense mesh retained; 8 views at 768; 4096x4096 albedo, metallic and roughness PNGs | 286.16 seconds | 12.87 / 16.38 GiB |

The painted GLB embeds base-color and combined metallic/roughness textures, each 4096x4096. The OBJ retains all 744,862 faces; Blender GLB conversion contains 744,852 (ten fewer triangles, recorded rather than hidden). [Export evidence](evidence/ai3d-hunyuan-smoke.json). [TRELLIS CUDA component evidence](evidence/ai3d-trellis-components.json). Windows Hunyuan argument forwarding and TRELLIS's stop on missing gated access were also checked. These are installation checks; no game regression suite was needed.

All outputs and detailed logs remain under the configured WSL root in `Hunyuan3D-2.1/outputs/local-smoke-20260909`. A Windows copy of `painted.glb`, `reference.png`, export verification and front/rear renders is under the configured `windowsOutputRoot` in `hunyuan-install-smoke/`. Preview PNGs are in its `previews/` directory. Windows Blender 5.2.1 rendered the actual GLB using Cycles, 48 samples, neutral studio lights and a floor; no mesh or material cleanup. Visual inspection shows a recognizable model and readable lettering, but body ripples, rough small edges and glossy material artifacts remain. This dense sample is source material, not a game-ready production asset.

Installed reproducibility files include `hunyuan-install.sh`, `hunyuan-local-patches.py`, `hunyuan-local.patch`, `hunyuan_cli.py`, `hunyuan-run`, model revisions, dependency lists and logs in the configured WSL root. Main Hunyuan checkpoint revision is `0b94677654c57bb9a6b6845cd7b704ccf551d327`; DINOv2 is `611a9d42f2335e0f921f1e313ad3c1b7178d206d`. The TRELLIS launcher also preserves explicit system compiler, CUDA include and WSL CUDA-library paths required by its native builds.

Klaus rejected the Hunyuan waymarker as not close to good enough and asked whether settings or model capability caused it. The reference remains square and alpha-composited normally; no local image stretch or GLB conversion error was found. A completed pre-bake comparison used stock 15 versus 30 texture steps with identical geometry, seed, camera selection and reference conditioning. Both raw generated front views already contain two medallions and have very similar appearance. This places the duplicated ornament before RealESRGAN, UV baking and Blender export; increasing sampling did not fix it. The tests took 63.072 and 118.390 seconds respectively. The geometry was reused from the painted OBJ without using its textures as conditioning. Raw diagnostic PNGs, camera/settings JSON and reproduction script are preserved in the private output's `texture-diagnostic/` directory; the baseline GLB was not overwritten.

The inference from this bounded result is that model conditioning/learned reconstruction limits matter more than sampling count for this asset. It does not establish that every possible seed, guidance value or reference would fail. The [technical report](https://arxiv.org/html/2506.15442v1#S3.SS1.SSS2) specifies 518x518 shape conditioning, which leaves relatively few pixels for a narrow object's small relief. The [paint README](https://github.com/Tencent-Hunyuan/Hunyuan3D-2.1/blob/main/hy3dpaint/README.md) permits 6–12 views, whereas the main demo suggests 6–9; 768 is the highest documented generated-view resolution. Additional views might improve coverage, but no verified upstream remedy for repeated ornament was found. Do not spend time on a broad settings sweep. A separately modeled plate/relief remains a possible authored finishing approach if the underlying asset is otherwise worth keeping.

## TRELLIS comparison — completed September 9

After Klaus confirmed DINOv3 approval, the same reference completed at **actual resolution1536**, using1536_cascade, seed42, an explicit98,304 token cap and the upstream12-step samplers with their configured guidance intervals/rescaling. The actual shape used16,647 tokens, so no resolution fallback occurred. [Complete settings, timings, export checks and limitations](evidence/ai3d-trellis-waymarker.json).

| Stage | Actual result | Elapsed |
| --- | --- | --- |
| Load | Exact approved DINOv3 and Microsoft checkpoints | 63.343 seconds |
| Inference | Raw6,428,848 vertices /15,120,504 triangles at1536 | 127.570 seconds |
| Raw save | Lossless geometry/attribute tensors plus PLY preserved | 1.505 seconds |
| Export | Upstream remesh and1M target produced937,846 triangles; embedded4K PNG base-color and metallic/roughness maps verified | 112.712 seconds |
| Whole successful attempt | Includes remaining setup/cleanup | 310.836 seconds |

Torch peak allocation/reservation was7.309/11.119GiB; native libraries and other applications are outside this accounting. A 1M export target is processing, not lossless output; the full15.1M-triangle source remains available. Both raw PLY and textured GLB were rendered in Windows Blender5.2.1 with the same front/rear cameras and studio lighting as Hunyuan. The painted model is markedly closer to the reference: one front plate, recognizable border pattern and more distinct stone/bronze/moss. **The plate's relief exists in the geometry**, visible without textures. Fine bark and some symbol shapes still differ from the input.

The unseen rear repeats the front decoration and has a conspicuous vertical hole below the plate. Smaller holes and frayed geometry occur around the emblem, moss and base. These defects are visible in the full raw mesh, before export simplification; they cannot be attributed solely to the1M export target. Verdict: **more faithful source material than this Hunyuan run, still requires repair and material finishing; no owner acceptance or game-ready claim**.

WSL output: `TRELLIS.2/outputs/waymarker-v1-1536-compat`. Windows output: `trellis-waymarker-v1-1536` beneath the configured output root. It contains the raw PLY/tensors, textured GLB,4K maps, original reference, metadata, logs, `previews/` and `geometry-previews/`; both preview folders contain front/rear PNGs and a saved Blender scene. No generator or renderer remains running. No model API charge or purchase occurred.

Klaus said the 2D render looked very good and requested an interactive view because the browser could not display the3D asset. A separate `previews/interactive-inspection.blend` opens with the textured export selected, framed for orbiting, and material preview using the scene lights/world. It was launched in a visible Windows Blender window and the matching window title verified. That interactive Blender process is intentionally left open for Klaus; do not close it as a helper cleanup. The positive render comment does not yet establish a verdict on the inspected3D model.

Reproduce using `-Tool TrellisTrial --image <WSL-reference-path> --out <new-output-path> --quality 1536_cascade --max-num-tokens 98304 --export-faces 1000000 --seed 42`. Preserve the baseline outputs. These are documented starting settings, not an empirically optimal recipe for this asset. The [upstream export example](https://github.com/microsoft/TRELLIS.2/blob/75fbf0183001ed9876c8dbb35de6b68552ee08bd/example.py) uses1M faces,4K textures and the same remesh parameters. The1536 mode describes geometry resolution; reference conditioning still uses512/1024 images. Hunyuan extraction above512 may reveal detail already encoded in its learned field, but no reliable carved-relief improvement was verified; `num_chunks` is a batching control. Do not misreport either model's settings as a universal maximum-quality preset.

The next useful graphics increment is a bounded repair and material pass on a copy of the TRELLIS candidate, measuring cleanup time and then inspecting it under changing lights at first-person distance in Unreal. Its rear holes and altered ornament must be repaired; keep the raw comparison evidence. A successful studio render alone does not establish the game's anchor-B quality or a repeatable pipeline across biomes. Other generators remain comparison candidates, and paid runs still require approval.

## Waymarker repair and Unreal inspection — September 9

Klaus called the render amazing and authorized repair on a copy, higher-resolution previews and actual Unreal inspection. The original `textured.glb` remains byte-for-byte unchanged (SHA256 `544abcd0b86ca9c28cc393a449bc35dd2777df4ea951aa4cf5d3b702a16d3c3d`), alongside the full15.1M-triangle source. Work is isolated in `trellis-waymarker-v1-1536/repaired-v1` under the configured Windows output root.

`scripts/repair_waymarker.py` normalizes the prop to2.4m, welds coincident export vertices while retaining per-loop UVs, removes the damaged rear patch and closes three local shell boundaries, including the lower tip. The final model has945,160 triangles. Six surface probes across the former opening return the rebuilt rear surface with outward-facing normals; the actual rear and close-up renders were also inspected. This is a bounded repair, not a claim that the entire generated mesh is watertight. The original source contains other small holes and frayed moss/base geometry.

The material uses the **unchanged original4K base-color and packed metallic/roughness maps**. UV0 retains the existing atlas/projection, UV1 samples adjacent stone, and linear vertex-color R feathers between them at the repair. Untouched faces have blend weight0. Both base color and packed PBR samples use the same blend; roughness is G and metallic B. One material slot remains. The FBX and `repaired-asset.blend` preserve this contract. Standard glTF cannot represent this shader directly; the retained `intermediate-flat-patch.glb` is an earlier rejected repair intermediate and must not be used as the finished export.

Four separate authored UCX box hulls provide a coarse obstacle envelope (48 collision triangles total); fine root openings intentionally do not become navigable spaces. The visual mesh remains dense for Nanite evaluation. The repair does not settle an optimal triangle budget or prove whole-game performance across procedural biomes.

The final Blender authoring script takes about14.3s to execute, excluding roughly25minutes of geometry/material iteration and visual review. Early flat texture transfers produced obvious seams, and matching deep defective source surfaces reproduced the damage. The current patch closes the opening and blends into adjacent stone, but the rear inlay is still softened/irregular and has a small color artifact. It needs a proper ornament/art finishing pass before being a close-view hero prop. The front relief and the original baselines remain preserved. This trial demonstrates a useful repair route, not unattended generation of finished assets.

Reproduce in background Blender with `--python-exit-code 1 --python <game>/scripts/repair_waymarker.py -- <original-textured.glb> <new-output-directory>`. Render the resulting Blender file with `scripts/render_ai3d_preview.py -- <repaired-asset.blend> <preview-directory> --resolution 4096 --samples 64 --rear-close`. The preview script supports original GLB/PLY inputs as before and does not modify its source. The Unreal inspection script is `scripts/inspect_ai3d_unreal.py`; use the actual editor with `-RenderOffscreen -ExecutePythonScript=<script> -AI3DConfig=<local-config.json>`. The saved local configuration is under the saved game project Saved/AI3DInspection.

## Licensing clarification

Norway is within the [Hunyuan2.1 license](https://github.com/Tencent-Hunyuan/Hunyuan3D-2.1/blob/main/LICENSE)'s defined territory. Its geographic clause does not exclude Klaus from local use in Norway. Section5(c) separately includes generated outputs/results in restrictions on use, distribution and display outside that territory; local evaluation and eventual worldwide game distribution are different questions. Keep trial outputs local and resolve distribution before shipping them. The [current review](../../research/runs/2026-09-07-depth-reset/ai-3d-current-review.md) also records TRELLIS runtime dependency licensing work before production adoption.

## Codex restart handoff

While installation ran, Klaus noticed many Node/command-processor processes and an available Codex update. Local inspection attributed 131 Node processes, 90 node_repl instances and 45 command shells to Codex tool helpers, including unified-computer-use and artifact-template-picker; some began September 5. Node/repl private committed memory totaled approximately 4.6GB. No Dreambound or Unreal Editor process was running. This identifies process ownership, not a confirmed leak mechanism. No process was terminated.

Klaus completed the Codex update/restart and reports that the accumulated helpers were removed. The environments, weights, outputs and local configuration persisted; a PC reboot was not needed. Resume with `python scripts/studio.py context game:dreambound`, then read this file. Hunyuan and TRELLIS waymarker trials are complete, with raw/processed outputs and actual Blender inspections. Hunyuan was rejected by Klaus; TRELLIS is closer but has documented geometry defects and no owner verdict yet. DINOv3 approval and download are complete. Do not rerun installation, ask for login again or replace the reference on resume. No monitoring automation or paid generation was started.

### Final bounded result and owner scope correction
The art specialist rebuilt the lower rear ornament from intact front geometry/atlas in `repaired-art-v2`, preserving v1 and the original. Final export:1,070,029 triangles,2.4m,one material,two UV sets,linear RepairBlend.R,four UCX hulls and unchanged original4K maps. The final1400px/32-sample front/rear/close Blender views show a continuous strip and terminal with no red shard. A stone seam below the terminal and slight rim transition remain. The preceding variant passed an FBX roundtrip; the last boundary correction was exported/rendered without another roundtrip. V2 had not yet been evaluated in Unreal at that point; see the subsequent owner inspection below. Its script is `scripts/finalize_waymarker_ornament.py`, called with `-- <waymarker-output-root>`; retained normalized donor and preparation source are in the private output.

V1 has nine real Unreal5.8.2/D3D12 1920x1080 captures in the saved project's Saved/AI3DInspection/waymarker-repaired-v1. Actual import:240cm,945159 Nanite triangles,15632 fallback triangles,two UV channels,one material,two4K maps with correct color spaces. Four UCX hulls and three collision queries passed. Main views use158cm eye height/94-degree FOV, with explicit inspection fill lighting. The25-copy image establishes that copies render, not performance. A late callback error occurred during final save despite the raw report's success flag. Saved Nanite/material usage flags were verified separately in a CPU-only run; the finalization fix is unrerun. No native GPU timings exist.

Klaus said the work was becoming overly pedantic. All additional polishing, profiling, final reruns and new4K rendering were stopped. The optional native profiling branch is disabled by default and untested. Future asset work should use a bounded repair and a direct visual check, then deliver, with further validation only for an actual issue or explicit need.

**Subsequent owner inspection:** V2 was imported and opened interactively in Unreal5.8.2 at `/Game/AI3DInspection/waymarker_repaired_v2/WaymarkerEvaluation`. Klaus said "Looks very good." The inspection script's new `phase: "view"` imports materials/mesh and leaves the editor open with the model selected, without captures, collision queries or profiling. Use the full visible editor and the normal `-ExecutePythonScript`/`-AI3DConfig` arguments, omitting `-RenderOffscreen`; the local config is `.local/ai3d/waymarker-view-v2.json` in the saved studio project. Unreal remains open for owner use. Previous V1 captures are still V1 evidence, and no gameplay package was rebuilt.

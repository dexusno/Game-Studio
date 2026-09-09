# Local AI 3D trials

Updated 2026-09-09. Klaus authorizes local installation and testing of TRELLIS.2 and Hunyuan3D-2.1. Quality against art anchor B remains the selection criterion. No purchase or adoption into the playable game is implied.

## Installed environment

The existing Ubuntu 22.04 WSL2 distribution runs on the Windows workstation and sees its RTX 4090 (24GB). No reboot, Windows driver replacement, or new WSL distribution was needed. Separate Miniforge Python 3.10 environments keep the two dependency stacks apart. A private CUDA 12.4.1 toolkit supplies compilation tools. The networked 3090 has not been configured.

Private paths are recorded in `config.local.json` under `tools.ai3d`: `wslDistro`, `wslRoot`, and `windowsOutputRoot`. Source checkouts, weights, environments, install logs and trial outputs stay outside Git under that configured install root. They survive a Codex restart. The saved project's local configuration is also updated; a fresh worktree needs those existing local settings.

| Component | Installed source or runtime | Current evidence |
| --- | --- | --- |
| TRELLIS.2 | Official source `75fbf0183001ed9876c8dbb35de6b68552ee08bd`; PyTorch 2.6.0+cu124; torchvision 0.21.0+cu124; FlashAttention 2.7.3 | Pipeline/native imports, actual FlashAttention comparison against PyTorch and nvdiffrast CUDA rasterization pass. Main weights and required sparse decoder downloaded. No complete generation yet: encoder access is pending. |
| TRELLIS.2-4B weights | Microsoft revision `af44b45f2e35a493886929c6d786e563ec68364d`; sparse decoder from TRELLIS-image-large revision `25e0d31ffbebe4b5a97464dd851910efc3002d96` | Approximately 16GB of public model files cached. |
| Hunyuan3D-2.1 | Official source `82920d643c0dc2f7bfd7255f45f62d386edfe60c`; PyTorch 2.5.1+cu124; torchvision 0.20.1+cu124 | Full shape and PBR trial succeeded. Both native extensions import; dependency consistency passes. Raw dense mesh and 4K maps retained; exported GLB verified and rendered in Windows Blender. |

## Windows launch commands

Run from the studio checkout in PowerShell:

```powershell
.\games\dreambound\scripts\Start-AI3D.ps1 -Tool Trellis
.\games\dreambound\scripts\Start-AI3D.ps1 -Tool Authenticate
.\games\dreambound\scripts\Start-AI3D.ps1 -Tool Hunyuan --help
```

The TRELLIS launcher opens its official local Gradio service at `http://127.0.0.1:7860` after its model access checks. Authentication uses Hugging Face's browser device flow; no token needs to be pasted into a task. An account must have access to [Meta DINOv3](https://huggingface.co/facebook/dinov3-vitl16-pretrain-lvd1689m) and the default [BRIA background-removal model](https://huggingface.co/briaai/RMBG-2.0). DINOv3 is manually gated and BRIA is automatically gated according to current Hub metadata. Main TRELLIS weights are public. The owner access question is pending; no gated weights have been retrieved or gates bypassed.

Hunyuan's installed `hunyuan-run` CLI accepts `all`, `shape`, or `paint`, with `--image` and `--out` paths in WSL form. `all` runs separate child processes so shape allocations are released before painting. Use `/mnt/d/...` for files on the Windows D: drive. Example syntax:

```powershell
.\games\dreambound\scripts\Start-AI3D.ps1 -Tool Hunyuan all --image /mnt/d/path/reference.png --out /mnt/d/path/trial-output
```

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

After installation verification, the real graphics comparison remains the root-wrapped waymarker reference from [the art pipeline](../../research/runs/2026-09-07-depth-reset/art-pipeline.md), finished and inspected in Unreal at first-person distance. A successful install or bundled sample does not prove our game's required quality.

## Licensing clarification

Norway is within the [Hunyuan2.1 license](https://github.com/Tencent-Hunyuan/Hunyuan3D-2.1/blob/main/LICENSE)'s defined territory. Its geographic clause does not exclude Klaus from local use in Norway. Section5(c) separately includes generated outputs/results in restrictions on use, distribution and display outside that territory; local evaluation and eventual worldwide game distribution are different questions. Keep trial outputs local and resolve distribution before shipping them. The [current review](../../research/runs/2026-09-07-depth-reset/ai-3d-current-review.md) also records TRELLIS runtime dependency licensing work before production adoption.

## Codex restart handoff

While installation ran, Klaus noticed many Node/command-processor processes and an available Codex update. Local inspection attributed 131 Node processes, 90 node_repl instances and 45 command shells to Codex tool helpers, including unified-computer-use and artifact-template-picker; some began September 5. Node/repl private committed memory totaled approximately 4.6GB. No Dreambound or Unreal Editor process was running. This identifies process ownership, not a confirmed leak mechanism. No process was terminated.

Installation, GPU trials and preview rendering have finished. Codex can now be closed/reopened for its update; the environments, weights, outputs and local configuration persist independently. Recheck helper counts after the update. A PC reboot was not needed for GPU access. Resume with `python scripts/studio.py context game:dreambound`, then read this file. Remaining work: obtain owner-approved access to the two gated TRELLIS dependencies, authenticate using the launcher, finish a full TRELLIS generation, and run the same game-relevant waymarker reference through both candidates before choosing a production tool.

"""Run the installed TRELLIS.2-4B on a genuine transparent RGBA reference.

Use the isolated trellis2 Python environment and its CUDA environment wrapper.
--validate-only reads the image without importing Torch or loading any models.
The private pipeline needs the documented load_rembg=False patch; the original
DINOv3 encoder still requires normal Hugging Face approval and authentication.

Defaults: 1024_cascade, upstream samplers, 1M export faces, remesh=True, 4K PNG
PBR textures. 1536_cascade is explicit; a token-driven resolution reduction is
saved as raw data and reported as failure before export. No quality fallback.
raw_mesh.pt contains lossless CPU tensors and plain metadata (load with
torch.load(..., weights_only=True)); raw_mesh.ply is the geometry counterpart.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import gc
import hashlib
import importlib.metadata
import inspect
import io
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import time
import traceback

MODEL_ID = "microsoft/TRELLIS.2-4B"
MODEL_REVISION = "af44b45f2e35a493886929c6d786e563ec68364d"
SOURCE_REVISION = "75fbf0183001ed9876c8dbb35de6b68552ee08bd"
ENCODER_ID = "facebook/dinov3-vitl16-pretrain-lvd1689m"
TEXTURE_SIZE = 4096
RASTER_FACE_LIMIT = 16777216


def positive_int(value: str) -> int:
    number = int(value)
    if number < 1:
        raise argparse.ArgumentTypeError("must be positive")
    return number


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def validate_image(path: Path):
    from PIL import Image

    with Image.open(path) as opened:
        if opened.mode != "RGBA":
            raise ValueError(f"Expected transparent RGBA input; found {opened.mode}. No background model will be used.")
        image = opened.copy()
    histogram = image.getchannel("A").histogram()
    if histogram[0] == 0:
        raise ValueError("The RGBA image has no fully transparent pixels; provide a genuine cutout.")
    # Match upstream's >0.8*255 foreground threshold after its 1024px size cap.
    scale = min(1.0, 1024 / max(image.size))
    size = (int(image.width * scale), int(image.height * scale))
    if min(size) < 2:
        raise ValueError("Image is too narrow for upstream preprocessing.")
    probe = image.resize(size, Image.Resampling.LANCZOS) if scale < 1 else image
    bbox = probe.getchannel("A").point(lambda value: 255 if value > 204 else 0).getbbox()
    if bbox is None or max(bbox[2] - bbox[0], bbox[3] - bbox[1]) < 3:
        raise ValueError("No usable foreground remains at upstream's alpha threshold and input size.")
    return image, {
        "path": str(path), "sha256": sha256(path), "mode": image.mode,
        "size": list(image.size), "transparent_pixels": histogram[0],
        "foreground_pixels_above_alpha_204": sum(histogram[205:]),
        "upstream_preprocessing_size": list(size), "foreground_bbox": list(bbox),
        "background_removal": "provided alpha; BRIA not loaded",
    }


def verify_glb(path: Path) -> dict:
    from PIL import Image

    with path.open("rb") as handle:
        magic, version, length = struct.unpack("<4sII", handle.read(12))
        if (magic, version, length) != (b"glTF", 2, path.stat().st_size):
            raise RuntimeError("Invalid exported GLB header")
        json_length, chunk_type = struct.unpack("<I4s", handle.read(8))
        if chunk_type != b"JSON":
            raise RuntimeError("GLB has no JSON chunk")
        document = json.loads(handle.read(json_length))
        _, chunk_type = struct.unpack("<I4s", handle.read(8))
        if chunk_type != b"BIN\x00":
            raise RuntimeError("GLB has no embedded binary chunk")
        binary_start = handle.tell()
        images = []
        for item in document.get("images", []):
            view = document["bufferViews"][item["bufferView"]]
            handle.seek(binary_start + view.get("byteOffset", 0))
            with Image.open(io.BytesIO(handle.read(view["byteLength"]))) as image:
                if image.size != (TEXTURE_SIZE, TEXTURE_SIZE) or image.format != "PNG":
                    raise RuntimeError(f"Expected embedded 4K PNG; got {image.size}, {image.format}")
                images.append({"size": list(image.size), "format": image.format})
        materials = document.get("materials", [])
        if not materials or not images:
            raise RuntimeError("GLB has no embedded PBR material")
        pbr = materials[0].get("pbrMetallicRoughness", {})
        if "baseColorTexture" not in pbr or "metallicRoughnessTexture" not in pbr:
            raise RuntimeError("GLB is missing base-color or metallic/roughness texture connections")
    return {"embedded_images": images, "bytes": path.stat().st_size, "sha256": sha256(path)}


def generate(args, image, image_info):
    repo = args.repo.expanduser().resolve()
    pipeline_source = repo / "trellis2/pipelines/trellis2_image_to_3d.py"
    if not pipeline_source.is_file():
        raise ValueError(f"TRELLIS source not found at {repo}; set --repo or TRELLIS2_REPO")
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=repo, text=True).strip()
    if revision != SOURCE_REVISION:
        raise ValueError(f"Runner targets source {SOURCE_REVISION}; installed HEAD is {revision}")
    out = args.out.expanduser().resolve()
    if out.exists() and any(out.iterdir()):
        raise ValueError(f"Output directory is not empty: {out}. Choose a new trial directory.")
    out.mkdir(parents=True, exist_ok=True)
    os.environ.setdefault("OPENCV_IO_ENABLE_OPENEXR", "1")
    os.environ.setdefault("PYTORCH_CUDA_ALLOC_CONF", "expandable_segments:True")
    sys.path.insert(0, str(repo))

    report = {
        "status": "started", "started_at_utc": datetime.now(timezone.utc).isoformat(),
        "input": image_info, "source_revision": revision,
        "pipeline_source_sha256": sha256(pipeline_source), "runner_sha256": sha256(Path(__file__)),
        "feature_extractor_sha256": sha256(repo / "trellis2/modules/image_feature_extractor.py"),
        "model_id": MODEL_ID, "model_revision": MODEL_REVISION,
        "generation": {"pipeline_type": args.quality, "seed": args.seed,
                       "num_samples": 1, "max_num_tokens": args.max_num_tokens},
        "export": {"decimation_target_faces": args.export_faces, "texture_size": TEXTURE_SIZE,
                   "remesh": args.remesh, "remesh_band": 1, "remesh_project": 0,
                   "aabb": [[-0.5, -0.5, -0.5], [0.5, 0.5, 0.5]],
                   "extension_webp": False, "texture_encoding": "lossless PNG",
                   "raster_face_limit": RASTER_FACE_LIMIT},
        "timings_seconds": {},
        "limits": "Local asset trial; quality, cleanup needs and Unreal suitability require inspection.",
    }
    start = time.monotonic()
    torch = None
    cuda_ready = False
    try:
        import torch
        import trimesh
        from huggingface_hub import hf_hub_download
        from trellis2.pipelines import Trellis2ImageTo3DPipeline
        import o_voxel

        if "load_rembg" not in inspect.signature(Trellis2ImageTo3DPipeline.from_pretrained).parameters:
            raise RuntimeError("Installed pipeline needs the optional load_rembg patch before this alpha-only trial")
        if not torch.cuda.is_available():
            raise RuntimeError("CUDA is unavailable in this Python environment")
        torch.cuda.reset_peak_memory_stats()
        cuda_ready = True
        report["gpu"] = torch.cuda.get_device_name(0)
        report["packages"] = {name: importlib.metadata.version(name) for name in
                              ["torch", "torchvision", "transformers", "trimesh", "cumesh", "o-voxel"]}
        config_path = Path(hf_hub_download(MODEL_ID, "pipeline.json", revision=MODEL_REVISION, local_files_only=True))
        config = json.loads(config_path.read_text())["args"]
        if config["image_cond_model"]["args"]["model_name"] != ENCODER_ID:
            raise RuntimeError("Checkpoint config does not name the expected DINOv3 encoder")
        report["model_config"] = config
        report["model_config_sha256"] = sha256(config_path)
        stage_start = time.monotonic()
        # Normal gated DINOv3 loading remains intact; only the unused rembg load is optional.
        pipeline = Trellis2ImageTo3DPipeline.from_pretrained(str(config_path.parent), load_rembg=False)
        # FlexGEMM inspects requires_grad on weights even inside no_grad. Leaving
        # pretrained parameters trainable allocates unused backward neighbor maps
        # during high-resolution inference. This changes no weights or resolution.
        frozen_parameters = 0
        for model in pipeline.models.values():
            for parameter in model.parameters():
                parameter.requires_grad_(False)
                frozen_parameters += parameter.numel()
        report["generation"]["inference_frozen_parameters"] = frozen_parameters
        report["image_encoder"] = {"model_id": ENCODER_ID,
                                   "revision": getattr(pipeline.image_cond_model.model.config, "_commit_hash", None)}
        pipeline.cuda()
        report["generation"]["low_vram"] = pipeline.low_vram
        if args.decode_row_chunk:
            # Layer normalization is independent per row. Bound its temporary
            # float32 allocation on large sparse decoder outputs without
            # lowering voxel resolution or altering the normalization axis.
            from trellis2.modules.norm import LayerNorm32
            from trellis2.models.sc_vaes.sparse_unet_vae import SparseUnetVaeDecoder, SparseResBlockC2S3d, SparseConvNeXtBlock3d
            from trellis2.modules.sparse.conv.conv_flex_gemm import sparse_conv3d_forward
            from types import SimpleNamespace
            import torch.nn.functional as functional
            norm32_forward = LayerNorm32.forward
            layer_norm = functional.layer_norm
            chunk = args.decode_row_chunk
            def release_neighbor_cache(value):
                # SparseTensor replacements alias these per-resolution maps.
                # Mutate the shared map: clear_spatial_cache only rebinds one
                # tensor, leaving large buffers alive through other aliases.
                cache = value.get_spatial_cache()
                for key in list(cache):
                    if key.startswith("SubMConv3d_neighbor_cache_"):
                        del cache[key]
            def chunked_norm32(module, value):
                if value.ndim != 2 or len(value) <= chunk:
                    return norm32_forward(module, value)
                output = torch.empty_like(value)
                for start_row in range(0, len(value), chunk):
                    output[start_row:start_row + chunk] = norm32_forward(module, value[start_row:start_row + chunk])
                return output
            @torch.no_grad()
            def chunked_convnext(module, value):
                if module.training:
                    raise RuntimeError("Chunked ConvNeXt is inference-only")
                hidden = module.conv(value)
                # Normalization, MLP expansion and residual addition are all
                # per point; retain the global spatial convolution unchanged.
                for start_row in range(0, len(hidden.feats), chunk):
                    rows = module.mlp(module.norm(hidden.feats[start_row:start_row + chunk]))
                    rows = rows + value.feats[start_row:start_row + chunk]
                    hidden.feats[start_row:start_row + chunk].copy_(rows)
                return hidden
            @torch.no_grad()
            def chunked_upsample(module, value, subdiv=None):
                if module.training:
                    raise RuntimeError("Chunked upsample is inference-only")
                if module.pred_subdiv:
                    subdiv = module.to_subdiv(value)
                hidden = value.replace(module.norm1(value.feats))
                functional.silu(hidden.feats, inplace=True)
                mask = subdiv.replace(subdiv.feats > 0) if subdiv is not None else None
                expanded_bytes = len(hidden.feats) * module.conv1.out_channels * hidden.feats.element_size()
                if mask is not None and expanded_bytes > 2**30:
                    # Upstream packs eight child voxels in consecutive output
                    # channel groups. Compute/gather one group at a time so
                    # inactive children never require a full expanded volume.
                    skip = module.updown(value, mask)
                    selected = mask.feats.nonzero()
                    packed = torch.empty((len(selected), module.out_channels),
                                         device=hidden.feats.device, dtype=hidden.feats.dtype)
                    for child in range(8):
                        first = child * module.out_channels
                        last = first + module.out_channels
                        conv = SimpleNamespace(weight=module.conv1.weight[first:last],
                            bias=module.conv1.bias[first:last] if module.conv1.bias is not None else None,
                            dilation=module.conv1.dilation)
                        values = sparse_conv3d_forward(conv, hidden)
                        for start_row in range(0, len(selected), chunk):
                            selection = selected[start_row:start_row + chunk]
                            rows = selection[:, 1] == child
                            packed[start_row:start_row + chunk][rows] = values.feats[selection[rows, 0]]
                        del values, rows, selection
                    hidden = skip.replace(packed)
                    del selected, packed
                else:
                    hidden = module.conv1(hidden)
                    hidden = module.updown(hidden, mask)
                    skip = module.updown(value, mask)
                # Both upsample branches have finished reading the parent
                # convolution map. Higher-resolution tensors have their own
                # map; texture decoding may recompute retired maps as needed.
                release_neighbor_cache(value)
                # This is fresh conv1/updown storage, independent of the skip.
                # Normalize it in row chunks without another full-width copy.
                for start_row in range(0, len(hidden.feats), chunk):
                    rows = hidden.feats[start_row:start_row + chunk]
                    rows.copy_(module.norm2(rows))
                functional.silu(hidden.feats, inplace=True)
                hidden = module.conv2(hidden)
                repeats = module.out_channels // (module.channels // 8)
                for start_row in range(0, len(hidden.feats), chunk):
                    rows = skip.feats[start_row:start_row + chunk].repeat_interleave(repeats, dim=1)
                    hidden.feats[start_row:start_row + chunk].add_(rows)
                return (hidden, subdiv) if module.pred_subdiv else hidden
            @torch.no_grad()
            def chunked_decoder_forward(module, value, guide_subs=None, return_subs=False):
                if module.training:
                    raise RuntimeError("Chunked decoder is inference-only")
                if (guide_subs is not None and module.pred_subdiv) or (return_subs and not module.pred_subdiv):
                    raise RuntimeError("Invalid decoder subdivision contract")
                hidden = module.from_latent(value).type(module.dtype)
                subdivisions = []
                # Same pinned upstream decoder blocks and subdivision outputs.
                # Fuse only its final cast / row normalization / linear head
                # into chunks, avoiding two full-width float32 volumes at once.
                for level, blocks in enumerate(module.blocks):
                    for index, block in enumerate(blocks):
                        if level < len(module.blocks) - 1 and index == len(blocks) - 1:
                            if module.pred_subdiv:
                                hidden, subdivision = block(hidden)
                                subdivisions.append(subdivision)
                            else:
                                hidden = block(hidden, subdiv=guide_subs[level] if guide_subs is not None else None)
                        else:
                            hidden = block(hidden)
                    print(f"Decoder level {level}: {len(hidden.feats)} rows, "
                          f"{torch.cuda.memory_allocated() / 2**30:.2f} GiB live", flush=True)
                output = torch.empty((len(hidden.feats), module.output_layer.out_features),
                                     device=hidden.feats.device, dtype=value.dtype)
                for start_row in range(0, len(hidden.feats), chunk):
                    rows = hidden.feats[start_row:start_row + chunk].to(value.dtype)
                    rows = layer_norm(rows, rows.shape[-1:])
                    output[start_row:start_row + chunk] = functional.linear(
                        rows, module.output_layer.weight, module.output_layer.bias)
                hidden = hidden.replace(output)
                # Neighbor maps from the completed decoder are memoized only;
                # do not retain them through mesh extraction and texture decode.
                # Guide coordinates and subdivision predictions stay intact.
                for tensor in (value, hidden, *subdivisions):
                    release_neighbor_cache(tensor)
                gc.collect()
                torch.cuda.empty_cache()
                return (hidden, subdivisions) if return_subs else hidden
            LayerNorm32.forward = chunked_norm32
            SparseUnetVaeDecoder.forward = chunked_decoder_forward
            SparseResBlockC2S3d._forward = chunked_upsample
            SparseConvNeXtBlock3d._forward = chunked_convnext
            decode_shape = pipeline.decode_shape_slat
            def offloaded_shape(*shape_args, **shape_kwargs):
                decoded_meshes, subdivisions = decode_shape(*shape_args, **shape_kwargs)
                cpu_meshes = [mesh.cpu() for mesh in decoded_meshes]
                size = sum(mesh.vertices.numel() * mesh.vertices.element_size() +
                           mesh.faces.numel() * mesh.faces.element_size() for mesh in cpu_meshes)
                del decoded_meshes
                gc.collect()
                torch.cuda.empty_cache()
                print(f"Decoded geometry held in system RAM during texture decode: {size / 2**30:.2f} GiB", flush=True)
                report["generation"]["geometry_cpu_offload_bytes"] = size
                # Upstream fill_holes explicitly moves geometry to CUDA when
                # needed. Keep it off the GPU during the independent texture
                # decoder, then restore the complete result before export.
                return cpu_meshes, subdivisions
            pipeline.decode_shape_slat = offloaded_shape
            report["generation"]["normalization_row_chunk"] = chunk
        report["timings_seconds"]["load"] = round(time.monotonic() - stage_start, 3)
        stage_start = time.monotonic()
        decode = pipeline.decode_latent
        def checkpoint_decode(shape, texture, resolution):
            checkpoint = {
                "input_sha256": image_info["sha256"], "model_revision": MODEL_REVISION,
                "seed": args.seed, "resolution": resolution,
                "shape_feats": shape.feats.detach().cpu(), "shape_coords": shape.coords.detach().cpu(),
                "texture_feats": texture.feats.detach().cpu(), "texture_coords": texture.coords.detach().cpu(),
            }
            torch.save(checkpoint, out / "latents.pt")
            del checkpoint
            print(f"Latents saved; decoding at {resolution}", flush=True)
            return decode(shape, texture, resolution)
        pipeline.decode_latent = checkpoint_decode
        if args.resume_latents:
            from trellis2.modules.sparse import SparseTensor
            checkpoint = torch.load(args.resume_latents, map_location="cpu", weights_only=True)
            if (checkpoint["input_sha256"] != image_info["sha256"] or
                checkpoint["model_revision"] != MODEL_REVISION or checkpoint["seed"] != args.seed or
                checkpoint["resolution"] != int(args.quality.split("_")[0])):
                raise RuntimeError("Latent checkpoint does not match input, model, seed and requested resolution")
            shape = SparseTensor(feats=checkpoint["shape_feats"].cuda(), coords=checkpoint["shape_coords"].cuda())
            texture = SparseTensor(feats=checkpoint["texture_feats"].cuda(), coords=checkpoint["texture_coords"].cuda())
            latents = (shape, texture, checkpoint["resolution"])
            report["generation"]["resumed_from"] = str(args.resume_latents)
            del checkpoint
            meshes = checkpoint_decode(*latents)
        else:
            meshes, latents = pipeline.run(image, seed=args.seed, num_samples=1,
                                          pipeline_type=args.quality, max_num_tokens=args.max_num_tokens,
                                          return_latent=True)
        mesh = meshes[0].cuda() if args.decode_row_chunk else meshes[0]
        actual_resolution = int(latents[2])
        report["generation"].update(actual_resolution=actual_resolution,
                                     shape_tokens=int(latents[0].coords.shape[0]))
        del latents, meshes
        torch.cuda.synchronize()
        report["timings_seconds"]["inference"] = round(time.monotonic() - stage_start, 3)

        # Save exact tensors before calling any export simplifier/remesher.
        stage_start = time.monotonic()
        raw = {name: getattr(mesh, name).detach().cpu() for name in
               ["vertices", "faces", "coords", "attrs", "origin"]}
        raw.update(schema_version=1, voxel_size=mesh.voxel_size, voxel_shape=list(mesh.voxel_shape),
                   layout={name: [part.start, part.stop, part.step] for name, part in mesh.layout.items()},
                   coordinate_space="upstream pipeline output; no export axis conversion")
        torch.save(raw, out / "raw_mesh.pt")
        trimesh.Trimesh(vertices=raw["vertices"].numpy(), faces=raw["faces"].numpy(),
                        process=False).export(out / "raw_mesh.ply")
        report["raw"] = {"tensor_file": "raw_mesh.pt", "geometry_file": "raw_mesh.ply",
                         "vertices": int(raw["vertices"].shape[0]), "faces": int(raw["faces"].shape[0]),
                         "attribute_shape": list(raw["attrs"].shape), "voxel_size": mesh.voxel_size,
                         "note": "Pipeline output includes upstream decode-time hole filling; no export processing applied."}
        report["timings_seconds"]["raw_save"] = round(time.monotonic() - stage_start, 3)
        del raw
        expected_resolution = int(args.quality.split("_")[0])
        if actual_resolution != expected_resolution:
            raise RuntimeError(f"Requested {args.quality}, but upstream returned {actual_resolution} because of its token cap. "
                               "Raw data is saved; GLB export was skipped. Retry with an explicitly larger --max-num-tokens if memory permits.")
        pipeline.cpu()
        del pipeline
        gc.collect()
        torch.cuda.empty_cache()

        stage_start = time.monotonic()
        report["export"]["raster_limit_simplification_applied"] = mesh.faces.shape[0] > RASTER_FACE_LIMIT
        if report["export"]["raster_limit_simplification_applied"]:
            mesh.simplify(RASTER_FACE_LIMIT)
        report["export"]["input_faces_after_raster_limit"] = int(mesh.faces.shape[0])
        exported = o_voxel.postprocess.to_glb(
            vertices=mesh.vertices, faces=mesh.faces, attr_volume=mesh.attrs,
            coords=mesh.coords, attr_layout=mesh.layout, voxel_size=mesh.voxel_size,
            aabb=report["export"]["aabb"], decimation_target=args.export_faces,
            texture_size=TEXTURE_SIZE, remesh=args.remesh, remesh_band=1, remesh_project=0,
            verbose=True,
        )
        exported.export(str(out / "textured.glb"), extension_webp=False)
        material = exported.visual.material
        material.baseColorTexture.save(out / "base_color.png")
        material.metallicRoughnessTexture.save(out / "metallic_roughness.png")
        report["export"].update(vertices=len(exported.vertices), faces=len(exported.faces),
                                 artifact="textured.glb", validation=verify_glb(out / "textured.glb"))
        torch.cuda.synchronize()
        report["timings_seconds"]["export"] = round(time.monotonic() - stage_start, 3)
        report["status"] = "complete"
    except Exception as error:
        report["status"] = "failed"
        report["error"] = f"{type(error).__name__}: {error}"
        report["traceback"] = traceback.format_exc()
        raise
    finally:
        report["timings_seconds"]["total"] = round(time.monotonic() - start, 3)
        if cuda_ready:
            report["torch_cuda_peaks_gib"] = {
                "allocated": round(torch.cuda.max_memory_allocated() / 2**30, 3),
                "reserved": round(torch.cuda.max_memory_reserved() / 2**30, 3),
                "scope": "Torch allocations; excludes other applications and native allocations outside Torch",
            }
        (out / "metadata.json").write_text(json.dumps(report, indent=2) + "\n")
        print(json.dumps({"status": report["status"], "metadata": str(out / "metadata.json")}, indent=2))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--image", type=Path, required=True)
    parser.add_argument("--out", type=Path, help="New or empty output directory; required for inference")
    parser.add_argument("--quality", choices=["1024_cascade", "1536_cascade"], default="1024_cascade")
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--max-num-tokens", type=positive_int, default=49152)
    parser.add_argument("--export-faces", type=positive_int, default=1000000)
    parser.add_argument("--remesh", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--resume-latents", type=Path, help="Resume decoding an exact saved local latents.pt into a new output directory")
    parser.add_argument("--decode-row-chunk", type=positive_int, help="Bound decoder normalization, projection and residual temporaries; no resolution change")
    parser.add_argument("--repo", type=Path, default=Path(os.environ.get(
        "TRELLIS2_REPO", Path.home() / ".local/share/game-studio/ai3d/TRELLIS.2")))
    parser.add_argument("--validate-only", action="store_true", help="Check alpha only; no model imports, downloads or GPU work")
    args = parser.parse_args()
    if not args.validate_only and args.out is None:
        parser.error("--out is required for inference")
    if not 0 <= args.seed < 2**32:
        parser.error("--seed must be between 0 and 4294967295")
    try:
        image, image_info = validate_image(args.image.expanduser().resolve())
        if args.validate_only:
            print(json.dumps({"status": "input_valid", "input": image_info, "quality": args.quality}, indent=2))
        else:
            generate(args, image, image_info)
    except Exception as error:
        print(f"TRELLIS trial failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

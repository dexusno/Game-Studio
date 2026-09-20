"""Original Mara source: offline TRELLIS inference, raw arrays only.

Run under the existing configured WSL trellis-run --trial launcher. No model
downloads, renderer, nvdiffrast, nvdiffrec, BRIA or upstream GLB export is used.
The guard rejects those import paths for the entire process. Blender performs
the later UV/material bake and interchange export using its CPU renderer.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.abc
import importlib.metadata
import importlib.util
import json
import os
from pathlib import Path
import sys
import time
import types


FORBIDDEN = ("nvdiffrast", "nvdiffrec_render", "o_voxel.postprocess")


class ExcludedRenderers(importlib.abc.MetaPathFinder):
    def find_spec(self, fullname, path=None, target=None):
        if any(fullname == item or fullname.startswith(item + ".") for item in FORBIDDEN):
            raise ImportError("Commercial asset runner excludes renderer: " + fullname)
        return None


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--seed", type=int, default=3701)
    parser.add_argument("--resolution", type=int, choices=[512, 1024, 1536], default=1536)
    parser.add_argument("--probe-only", action="store_true")
    args = parser.parse_args()
    root = Path.home() / ".local/share/game-studio/ai3d/TRELLIS.2"
    sys.path.insert(0, str(root))
    os.environ["HF_HUB_OFFLINE"] = "1"
    os.environ["TRANSFORMERS_OFFLINE"] = "1"
    os.environ["HF_HUB_DISABLE_TELEMETRY"] = "1"
    os.environ["PYTORCH_CUDA_ALLOC_CONF"] = "expandable_segments:True"
    sys.meta_path.insert(0, ExcludedRenderers())
    # The upstream O-Voxel __init__ eagerly loads its GLB exporter. Supply only
    # the package namespace so the MIT flexible-dual-grid decoder and its
    # already-installed extension can load without executing that exporter.
    spec = importlib.util.find_spec("o_voxel")
    if spec is None or not spec.submodule_search_locations:
        raise RuntimeError("Existing O-Voxel install is unavailable")
    package = types.ModuleType("o_voxel")
    package.__path__ = list(spec.submodule_search_locations)
    package.__package__ = "o_voxel"
    package.__spec__ = spec
    sys.modules["o_voxel"] = package
    import numpy as np
    from PIL import Image
    import torch
    from trellis2.pipelines import Trellis2ImageTo3DPipeline
    from o_voxel.convert import flexible_dual_grid_to_mesh  # noqa: F401

    args.output.mkdir(parents=True, exist_ok=True)
    image = Image.open(args.source)
    if image.mode != "RGBA" or np.min(np.array(image.getchannel("A"))) != 0:
        raise ValueError("The authored input must have genuine transparent alpha")
    report = {
        "asset": "MaraOperator_v001", "mode": "offline-raw-geometry-and-vertex-materials",
        "source_sha256": digest(args.source), "source_size": list(image.size),
        "generator": "OpenAI built-in image_gen; exact model identifier not exposed",
        "seed": args.seed, "requested_resolution": args.resolution,
        "input_alpha_min_max": list(image.getchannel("A").getextrema()),
        "pipeline_source_sha256": digest(root / "trellis2/pipelines/trellis2_image_to_3d.py"),
        "encoder_source_sha256": digest(root / "trellis2/modules/image_feature_extractor.py"),
        "excluded_modules": list(FORBIDDEN), "background_model_loaded": False,
        "packages": {name: importlib.metadata.version(name) for name in
                     ("torch", "transformers", "cumesh", "flex_gemm", "o_voxel")},
        "imports_passed": True, "generation_run": False,
    }
    if args.probe_only:
        report["excluded_modules_loaded"] = [name for name in sys.modules if
            any(name == p or name.startswith(p + ".") for p in FORBIDDEN)]
        assert not report["excluded_modules_loaded"]
        (args.output / "cpu-import-probe.json").write_text(json.dumps(report, indent=2))
        print(json.dumps(report, indent=2))
        return
    cache = root.parent / "cache/huggingface/hub"
    def cached(repo, filename):
        matches = list((cache / ("models--" + repo.replace("/", "--")) / "snapshots").glob("*/" + filename))
        if len(matches) != 1:
            raise RuntimeError(f"Need one existing pinned {repo}/{filename}; found {len(matches)}")
        return matches[0]
    original_config = cached("microsoft/TRELLIS.2-4B", "pipeline.json")
    local_config = json.loads(original_config.read_text())
    main_snapshot = original_config.parent
    revisions = {"microsoft/TRELLIS.2-4B": main_snapshot.name}
    for key, value in local_config["args"]["models"].items():
        if value.startswith("ckpts/"):
            local_config["args"]["models"][key] = str(main_snapshot / value)
        else:
            parts = value.split("/")
            repo, relative = "/".join(parts[:2]), "/".join(parts[2:])
            existing = cached(repo, relative + ".json")
            local_config["args"]["models"][key] = str(existing)[:-5]
            revisions[repo] = existing.parents[len(Path(relative).parts) - 1].name
    encoder = cached("facebook/dinov3-vitl16-pretrain-lvd1689m", "config.json").parent
    local_config["args"]["image_cond_model"]["args"]["model_name"] = str(encoder)
    revisions["facebook/dinov3-vitl16-pretrain-lvd1689m"] = encoder.name
    (args.output / "operator-pipeline.json").write_text(json.dumps(local_config, indent=2))
    report["pinned_cached_revisions"] = revisions
    started = time.monotonic()
    pipeline = Trellis2ImageTo3DPipeline.from_pretrained(str(args.output), config_file="operator-pipeline.json", load_rembg=False)
    pipeline.cuda()
    report["load_seconds"] = round(time.monotonic() - started, 3)
    started = time.monotonic()
    model_type = str(args.resolution) + ("_cascade" if args.resolution > 512 else "")
    mesh = pipeline.run(image, seed=args.seed, pipeline_type=model_type, max_num_tokens=98304)[0]
    actual_resolution = round(1 / mesh.voxel_size)
    if actual_resolution != args.resolution:
        raise RuntimeError(f"Requested {args.resolution}, received {actual_resolution}")
    report.update(generation_run=True, actual_resolution=actual_resolution,
                  generation_seconds=round(time.monotonic() - started, 3),
                  raw_vertices=int(mesh.vertices.shape[0]), raw_faces=int(mesh.faces.shape[0]))
    # Keep exact raw source, including sparse material fields, for source audit.
    np.savez_compressed(args.output / "trellis-raw.npz",
        vertices=mesh.vertices.cpu().numpy(), faces=mesh.faces.cpu().numpy(),
        coords=mesh.coords.cpu().numpy(), attrs=mesh.attrs.cpu().numpy(),
        origin=mesh.origin.cpu().numpy(), voxel_size=float(mesh.voxel_size))
    # CuMesh's MIT decimator changes only topology; query the original spatial
    # material volume after simplification, without invoking any rasterizer.
    mesh.simplify(350000, verbose=True)
    attributes = mesh.query_vertex_attrs()
    np.savez_compressed(args.output / "operator-surface.npz",
        vertices=mesh.vertices.cpu().numpy(), faces=mesh.faces.cpu().numpy(),
        attrs=attributes.cpu().numpy())
    report.update(surface_vertices=int(mesh.vertices.shape[0]), surface_faces=int(mesh.faces.shape[0]),
                  surface_sha256=digest(args.output / "operator-surface.npz"),
                  vertex_attribute_layout=["base_r", "base_g", "base_b", "metallic", "roughness", "alpha"],
                  excluded_modules_loaded=[name for name in sys.modules if
                    any(name == p or name.startswith(p + ".") for p in FORBIDDEN)])
    assert not report["excluded_modules_loaded"]
    report["peak_cuda_allocated_bytes"] = torch.cuda.max_memory_allocated()
    (args.output / "generation-report.json").write_text(json.dumps(report, indent=2))
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()

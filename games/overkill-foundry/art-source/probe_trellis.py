"""Read-only local TRELLIS capability probe; run with configured trellis-run --trial.

Does not download models, print tokens, authenticate, or accept model terms.
"""
import importlib
import json
import sys
from pathlib import Path

root = Path.home() / ".local/share/game-studio/ai3d"
sys.path.insert(0, str(root / "TRELLIS.2"))
result = {"imports": {}, "generation_run": False}
for module in ("torch", "trellis2", "trellis2.pipelines", "nvdiffrast.torch", "o_voxel"):
    try:
        loaded = importlib.import_module(module)
        result["imports"][module] = {"ok": True, "version": getattr(loaded, "__version__", None)}
    except Exception as exc:
        result["imports"][module] = {"ok": False, "error": str(exc)}
if result["imports"]["torch"]["ok"]:
    import torch
    result["cuda"] = {"available": torch.cuda.is_available()}
    if torch.cuda.is_available():
        props = torch.cuda.get_device_properties(0)
        result["cuda"].update(name=props.name, vram_bytes=props.total_memory,
                              tensor_check=int((torch.tensor([2], device="cuda") * 3).cpu()[0]))
result["cached_models"] = {}
for name in ("models--microsoft--TRELLIS.2-4B", "models--facebook--dinov3-vitl16-pretrain-lvd1689m", "models--briaai--RMBG-2.0"):
    model = root / "cache/huggingface/hub" / name
    result["cached_models"][name] = {"present": model.exists(),
        "snapshot_files": len(list((model / "snapshots").rglob("*"))) if model.exists() else 0}
result["prior_outputs"] = [str(p.relative_to(root)) for p in (root / "TRELLIS.2/outputs").rglob("*.glb")]
result["interpretation"] = "Import/CUDA/cache availability only. Fresh generation and all model-use rights unverified. This increment uses original procedural Blender geometry instead."
print(json.dumps(result, indent=2))

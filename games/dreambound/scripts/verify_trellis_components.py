"""Exercise the installed native GPU components, independently of gated weights."""
import argparse
import importlib.metadata
import json
from datetime import datetime, timezone
from pathlib import Path

import torch
import torch.nn.functional as F
from flash_attn import flash_attn_func
import nvdiffrast.torch as dr


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', required=True)
    args = parser.parse_args()
    torch.manual_seed(781)
    q, k, v = [torch.randn(1, 128, 4, 64, device='cuda', dtype=torch.float16) for _ in range(3)]
    actual = flash_attn_func(q, k, v)
    expected = F.scaled_dot_product_attention(q.transpose(1, 2), k.transpose(1, 2), v.transpose(1, 2)).transpose(1, 2)
    max_error = float((actual - expected).abs().max())
    assert torch.allclose(actual, expected, atol=0.003, rtol=0.003), max_error

    context = dr.RasterizeCudaContext()
    vertices = torch.tensor([[[-0.7, -0.7, 0., 1.], [0.7, -0.7, 0., 1.], [0., 0.7, 0., 1.]]], device='cuda')
    faces = torch.tensor([[0, 1, 2]], dtype=torch.int32, device='cuda')
    raster, _ = dr.rasterize(context, vertices, faces, resolution=[64, 64])
    covered_pixels = int((raster[..., 3] > 0).sum())
    assert 0 < covered_pixels < 64 * 64, covered_pixels
    torch.cuda.synchronize()

    report = {
        'checked_at_utc': datetime.now(timezone.utc).isoformat(),
        'gpu': torch.cuda.get_device_name(0),
        'packages': {name: importlib.metadata.version(name) for name in ['torch', 'torchvision', 'flash-attn', 'nvdiffrast', 'transformers', 'cumesh', 'o-voxel']},
        'checks': {
            'flash_attention_matches_pytorch': True,
            'flash_attention_max_absolute_error': max_error,
            'nvdiffrast_cuda_rasterization': True,
            'rasterized_triangle_pixels': covered_pixels,
        },
        'limits': 'Native-component checks only; no complete TRELLIS generation or art-quality assessment.',
    }
    path = Path(args.report)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()

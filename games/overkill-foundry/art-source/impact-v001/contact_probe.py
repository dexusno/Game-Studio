"""CPU-only front contact measurements on the existing original robot meshes.

No exports, materials, animation or source .blend files are changed. Cast along
the authored -X facing and visible flank through each core cue in the rest pose.
Use the nearer visible surface, so a projecting weapon cannot drag a hull/core
effect to its distant muzzle. This avoids hiding contact FX inside armour.
"""
import argparse
import json
from pathlib import Path
import sys

import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

p = argparse.ArgumentParser()
p.add_argument("--art-root", type=Path, required=True)
p.add_argument("--report", type=Path, required=True)
a = p.parse_args(sys.argv[sys.argv.index("--") + 1:])
records = []
for folder, filename in (("cinderwall-v001", "Cinderwall_Source.blend"),
    ("roster-v001", "Cinderwall_Roster_Source.blend")):
    source_report = json.loads((a.art_root / folder / "asset-report.json").read_text())
    bpy.ops.wm.open_mainfile(filepath=str((a.art_root / folder / filename).resolve()))
    for name, info in source_report["robots"].items():
        arm = bpy.data.objects["SK_" + name]
        mesh = bpy.data.objects["SKM_" + name]
        cue = arm.data.bones["core"].head_local.copy()
        vertices = [v.co.copy() for v in mesh.data.vertices]
        tree = BVHTree.FromPolygons(vertices, [tuple(f.vertices) for f in mesh.data.polygons])
        candidates = []
        for outward in (Vector((-1, 0, 0)), Vector((0, -1, 0))):
            hit, normal, index, distance = tree.ray_cast(cue + outward * 10, -outward, 20)
            if hit is not None:
                candidates.append((hit - cue + outward * .025, hit, outward))
        if not candidates:
            raise RuntimeError("Core-facing contact ray missed " + name)
        offset, hit, outward = min(candidates, key=lambda item: item[0].length)
        records.append({"robot": name,
            "definition": info.get("source_id", {"RivetMite": "C1-R01", "BreachRam": "C1-R02"}.get(name)),
            "core_source_m": list(cue), "front_surface_source_m": list(hit),
            "fx_offset_component_cm": [offset.x * 100, -offset.y * 100, offset.z * 100],
            "outward_component": [outward.x, -outward.y, outward.z],
            "source_height_cm": (max(v.z for v in vertices) - min(v.z for v in vertices)) * 100})
a.report.parent.mkdir(parents=True, exist_ok=True)
a.report.write_text(json.dumps({"method": __doc__, "robots": records,
    "limits": "Rest-pose source ray, not an Unreal collision or render proof. Runtime rotates this offset with the animated core relative to its reference pose. Existing gameplay impact location stays unchanged."}, indent=2))
print("IMPACT_CONTACT_PROBE " + json.dumps(records))

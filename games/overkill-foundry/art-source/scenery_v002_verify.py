"""Inspect the actual scenery FBX round trip, not only generator declarations."""
import argparse
import hashlib
import json
import math
import sys
from pathlib import Path

import bpy

parser = argparse.ArgumentParser()
parser.add_argument("--output", required=True)
args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
root = Path(args.output).resolve()
source = json.loads((root / "asset-report.json").read_text(encoding="utf-8"))
report = {"format": "scenery-v002-interchange-v1", "blender": bpy.app.version_string, "checks": []}


def check(ok, label, detail=None):
    report["checks"].append({"id": label, "passed": bool(ok), "detail": detail})


bpy.ops.wm.read_factory_settings(use_empty=True)
file = root / "meshes/CinderwallSceneryV002.fbx"
bpy.ops.import_scene.fbx(filepath=str(file))
objects = {o.name: o for o in bpy.context.scene.objects if o.type == "MESH"}
expected = source["stage"]["instances"]
check(set(objects) == {item["name"] for item in expected}, "complete-scenery-only-inventory", sorted(objects))
check(not any(o.type in ("ARMATURE", "CAMERA", "LIGHT") for o in bpy.context.scene.objects), "no-review-heroes-cameras-or-lights-exported")
for item in expected:
    obj = objects.get(item["name"])
    if obj is None:
        continue
    check(all(abs(float(a) - b) < .005 for a, b in zip(obj.dimensions, item["bounds_m"])), item["name"] + ":metre-bounds", list(obj.dimensions))
    check(all(abs(float(a) - b) < .005 for a, b in zip(obj.location, item["position_m"])), item["name"] + ":pivot-placement", list(obj.location))
    check(bool(obj.data.uv_layers) and all(math.isfinite(float(v)) for uv in obj.data.uv_layers.active.data for v in uv.uv), item["name"] + ":finite-uvs")
    check({m.name for m in obj.data.materials} == set(item["materials"]), item["name"] + ":material-slots")
    obj.data.calc_loop_triangles()
    check(len(obj.data.loop_triangles) == item["triangles"], item["name"] + ":geometry-retained", len(obj.data.loop_triangles))
    # Every triangle above the deck is outside the open fire/gathering volumes.
    # Checking triangle bounds is conservative and catches accidental scenery
    # through the arena; it does not establish screen-space target readability.
    if item["name"] not in ("SM_SV2_combat_deck",):
        obstructing = 0
        for tri in obj.data.loop_triangles:
            points = [obj.matrix_world @ obj.data.vertices[i].co for i in tri.vertices]
            lo = [min(p[k] for p in points) for k in range(3)]
            hi = [max(p[k] for p in points) for k in range(3)]
            if lo[0] < 10 and hi[0] > -4.5 and lo[1] < 1.5 and hi[1] > -1.5 and hi[2] > .1 and lo[2] < 3.8:
                obstructing += 1
        check(obstructing == 0, item["name"] + ":open-firing-volume", obstructing)
report["fbx_sha256"] = hashlib.sha256(file.read_bytes()).hexdigest()
report["source_report_sha256"] = hashlib.sha256((root / "asset-report.json").read_bytes()).hexdigest()
report["failures"] = [item["id"] for item in report["checks"] if not item["passed"]]
report["limits"] = ["No Unreal import or material/culling verification in this check.",
                    "Actual 16B/16C camera and multi-enemy readability require rendered gameplay.",
                    "No human visual approval or runtime performance claim."]
(root / "interchange-verification.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
print("SCENERY_VERIFY " + json.dumps({"checks": len(report["checks"]), "failures": report["failures"]}))
if report["failures"]:
    raise RuntimeError("Scenery FBX failed: " + ", ".join(report["failures"]))

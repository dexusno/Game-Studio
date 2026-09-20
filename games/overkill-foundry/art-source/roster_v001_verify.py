"""Reimport every original roster FBX and inspect its actual mesh/animation data."""
import argparse
import hashlib
import json
import sys
from pathlib import Path

import bpy

p = argparse.ArgumentParser()
p.add_argument("--output", required=True)
root = Path(p.parse_args(sys.argv[sys.argv.index("--") + 1:]).output).resolve()
source_file = root / "asset-report.json"
source = json.loads(source_file.read_text(encoding="utf-8"))
game = Path(__file__).resolve().parents[1]
manifest_file = game / "content/cinderwall.manifest.json"
manifest = json.loads(manifest_file.read_text(encoding="utf-8"))


def sha(file):
    return hashlib.sha256(file.read_bytes()).hexdigest()


result = {"format": "cinderwall-roster-interchange-v1", "blender": bpy.app.version_string,
          "source_report_sha256": sha(source_file), "checks": [], "failures": [], "files": {}}


def check(ok, name, actual=None):
    result["checks"].append({"name": name, "ok": bool(ok), "actual": actual})
    if not ok:
        result["failures"].append(name)


def read(file):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.render.fps = 30
    bpy.ops.import_scene.fbx(filepath=str(file))
    result["files"][file.relative_to(root).as_posix()] = sha(file)


check(source["manifest_sha256"] == sha(manifest_file), "current enabled manifest identity")
check(source["source_sha256"] == sha(Path(__file__).with_name("roster_v001_build.py")), "current generator identity")
check(source["helper_sha256"] == sha(Path(__file__).with_name("build_cinderwall.py")), "current original helper identity")
check({r["source_id"] for r in source["robots"].values()} == set(manifest["robot_ids"]) - {"C1-R01", "C1-R02"}, "exact eight remaining enabled definitions")

for name, info in source["robots"].items():
    read(root / "meshes" / (name + ".fbx"))
    arms = [o for o in bpy.context.scene.objects if o.type == "ARMATURE"]
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    check(len(arms) == len(meshes) == 1, name + " one complete actor skeleton/mesh")
    arm, mesh = arms[0], meshes[0]
    check(set(b.name for b in arm.data.bones) == {b["name"] for b in info["bones"]}, name + " exact bone inventory")
    check(all((arm.data.bones[b["name"]].parent.name if arm.data.bones[b["name"]].parent else None) == b["parent"] for b in info["bones"]), name + " parent hierarchy")
    check(all(len(v.groups) == 1 and abs(v.groups[0].weight - 1) < .0001 for v in mesh.data.vertices), name + " rigid unit weights on every vertex")
    check(bool(mesh.data.uv_layers), name + " original UV layer")
    check({m.name for m in mesh.data.materials} == set(info["materials"]), name + " complete original material slots")
    check(all(abs(a - b) < .025 for a, b in zip(mesh.dimensions, info["rest_bounds_m"])), name + " metre bounds", list(mesh.dimensions))
    mesh.data.calc_loop_triangles()
    check(len(mesh.data.loop_triangles) == info["triangles"], name + " triangle count", len(mesh.data.loop_triangles))
    muzzle = arm.data.bones["muzzle"].head_local
    check(muzzle.x < -.90 and muzzle.z > .5, name + " muzzle ahead of chassis", list(muzzle))
    expected = set(manifest["robot_annotations"][info["source_id"]]["action_ids"])
    check(set(info["action_clips"]) == expected, name + " every enabled distinct action mapped")
    check(len(set(info["action_clips"].values())) == len(expected), name + " distinct action files")
    for suffix in ("idle", "hit_light", "hit_medium", "hit_heavy", "death", "escape"):
        check(info["prefix"] + suffix in source["clips"], name + " required " + suffix)
    for clip_name, clip in source["clips"].items():
        if not clip_name.startswith(info["prefix"]):
            continue
        read(root / "animations" / (clip_name + ".fbx"))
        arm = next(o for o in bpy.context.scene.objects if o.type == "ARMATURE")
        check(bool(arm.animation_data and arm.animation_data.action), clip_name + " imported animation")
        first_frame, last_frame = arm.animation_data.action.frame_range
        bpy.context.scene.frame_set(round(first_frame))
        start = {b.name: b.matrix.copy() for b in arm.pose.bones}
        maximum = 0
        for frame in range(round(first_frame), round(last_frame) + 1):
            bpy.context.scene.frame_set(frame)
            maximum = max(maximum, max((b.matrix.translation - start[b.name].translation).length for b in arm.pose.bones))
        check(.001 < maximum < 6, clip_name + " actual bounded articulation", maximum)
        # Blender's FBX importer offsets the first key to frame 2 by default.
        # Duration is the interval, not an assumed absolute last-frame origin.
        imported_seconds = (last_frame - first_frame) / 30
        check(abs(imported_seconds - clip["duration_s"]) < .04, clip_name + " authored duration",
              {"source_seconds": clip["duration_s"], "imported_seconds": imported_seconds, "frames": list(arm.animation_data.action.frame_range)})
        if clip_name.endswith("death") or clip_name.endswith("death_release"):
            check(any(e["event"] == "destroy_actor_and_fx" and e["t"] == 2 for e in clip["events"]), clip_name + " complete cleanup contract")

result["limitations"] = ["FBX and original source coverage only; Unreal playback, action/event mapping and appearance remain unverified.",
                         "No performance, LOD, human visual approval or final art quality claim."]
(root / "interchange-verification.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
print("ROSTER_VERIFY " + json.dumps({"checks": len(result["checks"]), "failures": result["failures"]}))
if result["failures"]:
    raise RuntimeError("Failed roster interchange checks")

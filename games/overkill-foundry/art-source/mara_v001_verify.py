"""Actual FBX reimport checks; not an in-game animation/visual approval."""
import argparse
import hashlib
import json
import sys
from pathlib import Path

import bpy

p = argparse.ArgumentParser()
p.add_argument("--output", required=True)
args = p.parse_args(sys.argv[sys.argv.index("--") + 1:])
root = Path(args.output).resolve()
source = json.loads((root / "asset-report.json").read_text())
result = {"format": "mara-interchange-v1", "blender": bpy.app.version_string, "checks": [], "failures": [], "files": {}}


def check(ok, name, actual=None):
    result["checks"].append({"name": name, "ok": bool(ok), "actual": actual})
    if not ok:
        result["failures"].append(name)


def read(file):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.render.fps = 30
    bpy.ops.import_scene.fbx(filepath=str(file))
    result["files"][str(file.relative_to(root))] = hashlib.sha256(file.read_bytes()).hexdigest()


for name, info in source["props"].items():
    read(root / "meshes" / (name + ".fbx"))
    arms = [o for o in bpy.context.scene.objects if o.type == "ARMATURE"]
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    check(len(arms) == len(meshes) == 1, name + " one skeleton and mesh")
    arm, mesh = arms[0], meshes[0]
    check(len(arm.data.bones) == info["bones"], name + " exact skeleton", len(arm.data.bones))
    check(set(b.name for b in arm.data.bones) == set(b["name"] for b in info["bone_definitions"]), name + " attachment bones retained")
    check(all(len(v.groups) == 1 and abs(v.groups[0].weight - 1) < .0001 for v in mesh.data.vertices), name + " rigid weights")
    check(len(mesh.data.uv_layers) >= 1, name + " UVs retained")
    check(all(abs(a - b) < .025 for a, b in zip(mesh.dimensions, info["rest_bounds_m"])), name + " metre scale", list(mesh.dimensions))
    if name == "MaraGun":
        muzzle = arm.data.bones["muzzle"].head_local
        check(muzzle.x > 1.80 and abs(muzzle.y) < .01, "gun muzzle forward +X", list(muzzle))
    prefix = "MG_" if name == "MaraGun" else "MC_"
    for clip_name, clip_info in source["clips"].items():
        if not clip_name.startswith(prefix):
            continue
        read(root / "animations" / (clip_name + ".fbx"))
        arm = next(o for o in bpy.context.scene.objects if o.type == "ARMATURE")
        bpy.context.scene.frame_set(1)
        start = {b.name: b.matrix.copy() for b in arm.pose.bones}
        maximum = 0
        for fraction in (.2, .35, .55, .75, .95):
            bpy.context.scene.frame_set(1 + round(clip_info["duration_s"] * 30 * fraction))
            maximum = max(maximum, max((b.matrix.translation - start[b.name].translation).length for b in arm.pose.bones))
        check(maximum > .001 and maximum < 3, clip_name + " sampled motion", maximum)
        check(abs((arm.animation_data.action.frame_range[1] - 1) / 30 - clip_info["duration_s"]) < .04, clip_name + " duration")
        if clip_name == "MC_collect":
            bpy.context.scene.frame_set(14)
            head = arm.pose.bones["head"].matrix.translation
            check(head.z < 1.75, "claw deploy reaches rear scrap", list(head))
            scale = arm.pose.bones["cables"].scale
            check(max(scale) > 1.6, "claw cable extension imported", list(scale))
        if clip_name == "MG_fire":
            rearward = []
            for frame in range(1, 1 + round(clip_info["duration_s"] * 30)):
                bpy.context.scene.frame_set(frame)
                rearward.append(arm.pose.bones["muzzle"].matrix.translation.x)
            check(min(rearward) < start["muzzle"].translation.x - .18, "gun recoil moves muzzle rearward", min(rearward))

read(root / "meshes/MaraStageExtension.fbx")
stage = [o for o in bpy.context.scene.objects if o.type == "MESH"]
check(len(stage) == len(source["stage"]["instances"]), "depth and payload modules retained", len(stage))
check(all(o.data.uv_layers for o in stage), "depth modules UVs")
check(any("payload" in o.name for o in stage), "separate cosmetic scrap payload")
result["limitations"] = ["Source interchange evidence only; actual Unreal event playback and framing require rendered checks.", "No character model, LOD/performance approval, audio or owner approval is established."]
(root / "interchange-verification.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
print("MARA_VERIFY " + json.dumps({"checks": len(result["checks"]), "failures": result["failures"]}))
if result["failures"]:
    raise RuntimeError("Failed: " + ", ".join(result["failures"]))

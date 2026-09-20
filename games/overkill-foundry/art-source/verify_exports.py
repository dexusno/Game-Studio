"""Inspect actual FBX/GLB round trips. Run with Blender --background --python.

Verifies mesh bounds, UVs, rigid skinning, action coverage and clip movement.
This is interchange validation, not an Unreal import or gameplay test.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

import bpy
import numpy as np

parser = argparse.ArgumentParser()
parser.add_argument("--output", required=True)
options = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
root = Path(options.output).resolve()
report = {"format": "cinderwall-interchange-verification-v1", "checks": [], "failures": []}


def check(condition, label, detail=None):
    report["checks"].append({"label": label, "passed": bool(condition), "detail": detail})
    if not condition:
        report["failures"].append(label)


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.render.fps = 30


expected = {"BreachRam": (12, "BR_", 8, (2.9, 2.5, 2.56)), "RivetMite": (20, "RM_", 7, (1.76, 1.75, 1.32))}
for name, (bones, prefix, count, bounds) in expected.items():
    reset()
    bpy.ops.import_scene.fbx(filepath=str(root / "meshes" / (name + ".fbx")))
    arms = [o for o in bpy.context.scene.objects if o.type == "ARMATURE"]
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    check(len(arms) == 1 and len(meshes) == 1, name + " FBX one skeleton and mesh")
    if arms and meshes:
        arm, mesh = arms[0], meshes[0]
        check(len(arm.data.bones) == bones, name + " FBX exact bone count", len(arm.data.bones))
        check(len(mesh.data.uv_layers) >= 1, name + " FBX UV channel")
        check(all(len(v.groups) == 1 and abs(v.groups[0].weight - 1) < .0001 for v in mesh.data.vertices), name + " FBX rigid weights")
        check(all(.80 * v < float(d) < 1.2 * v for d, v in zip(mesh.dimensions, bounds)), name + " FBX metres round trip", list(mesh.dimensions))
        check(all(b in arm.data.bones for b in ("muzzle", "intent", "core")), name + " event sockets retained")
    data = (root / "meshes" / (name + ".glb")).read_bytes()
    magic, version, length = struct.unpack_from("<III", data)
    json_length, chunk = struct.unpack_from("<II", data, 12)
    gltf = json.loads(data[20:20 + json_length])
    animations = [a["name"] for a in gltf.get("animations", [])]
    check(magic == 0x46546C67 and version == 2 and length == len(data), name + " GLB container valid")
    check(len(animations) == count and all(a.startswith(prefix) for a in animations), name + " GLB own action coverage only", animations)
    check(len(gltf.get("skins", [])) == 1, name + " GLB skin retained")
    check(all("normalTexture" in m and "baseColorTexture" in m.get("pbrMetallicRoughness", {})
              and "metallicRoughnessTexture" in m["pbrMetallicRoughness"]
              for m in gltf["materials"] if m["name"] not in ("M_CW_ember", "M_CW_optic", "M_CW_cool")), name + " GLB PBR maps retained")
    reset()
    clip = prefix + ("attack_blast" if prefix == "BR_" else "attack_rivet")
    bpy.ops.import_scene.fbx(filepath=str(root / "animations" / (clip + ".fbx")))
    arm = next(o for o in bpy.context.scene.objects if o.type == "ARMATURE")
    target = "piston" if prefix == "BR_" else "rivet"
    bpy.context.scene.frame_set(1)
    start = arm.pose.bones[target].matrix.copy()
    bpy.context.scene.frame_set(11)
    end = arm.pose.bones[target].matrix.copy()
    movement = (end.translation - start.translation).length
    check(movement > .035, name + " FBX attack clip actually moves mechanism", movement)

reset()
bpy.ops.import_scene.gltf(filepath=str(root / "meshes" / "CinderwallStage.glb"))
stage = [o for o in bpy.context.scene.objects if o.type == "MESH"]
check(len(stage) == 47, "Stage GLB all 47 modules", len(stage))
check(len([o for o in stage if "rear_claw" in o.name]) == 1, "Stage rear claw module present")
check(all(len(o.data.uv_layers) >= 1 for o in stage), "Stage all modules have UVs")
check(all(o.dimensions.length > .1 for o in stage), "Stage nonempty module dimensions")
texture_files = list((root / "textures").glob("*.png"))
check(len(texture_files) == 27, "27 portable PBR textures", len(texture_files))
for path in texture_files:
    image = bpy.data.images.load(str(path), check_existing=True)
    check(tuple(image.size) == (256, 256), path.name + " intended 256px dimensions")
    pixels = np.empty(len(image.pixels), dtype=np.float32)
    image.pixels.foreach_get(pixels)
    alpha = pixels[3::4]
    check(min(alpha) == 1.0 and max(alpha) == 1.0, path.name + " fully opaque as intended")
report["limitations"] = ["Unreal import/material binding not run by this source verification.",
    "Dissolve, hit event timing and actor cleanup require gameplay presentation integration.",
    "Animation feel and human visual approval remain unverified."]
(root / "interchange-verification.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
print("CINDERWALL_VERIFY " + json.dumps(report))
if report["failures"]:
    raise RuntimeError("Interchange failures: " + ", ".join(report["failures"]))

"""Executed Blender FBX/GLB round-trip validation for the Mara operator."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

import bpy
import numpy as np
from mathutils import Vector

parser = argparse.ArgumentParser()
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
out = args.output.resolve()
source = json.loads((out / "asset-report.json").read_text())
result = {"asset": "MaraOperator_v001", "blender": bpy.app.version_string,
          "checks": [], "failures": [], "files": {}}

# Capture source trajectories before checking the independently reimported FBX.
bpy.ops.wm.open_mainfile(filepath=str(out / "Mara_Operator_Source.blend"))
original = bpy.data.objects["SK_MaraOperator"]
reference = {}
for clip, info in source["clips"].items():
    original.animation_data.action = bpy.data.actions[clip]
    reference[clip] = {}
    for frame in range(1, info["frames"] + 1, 2):
        bpy.context.scene.frame_set(frame)
        reference[clip][frame] = {b.name: b.matrix.translation.copy() for b in original.pose.bones}


def check(ok, label, actual=None):
    result["checks"].append({"name": label, "ok": bool(ok), "actual": actual})
    if not ok:
        result["failures"].append(label)


def read(path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.render.fps = 30
    bpy.ops.import_scene.fbx(filepath=str(path), anim_offset=0)
    result["files"][str(path.relative_to(out))] = hashlib.sha256(path.read_bytes()).hexdigest()
    arms = [obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE"]
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    check(len(arms) == len(meshes) == 1, path.stem + " one armature and mesh")
    # FBX has bone heads, not Blender's connected-tip semantics. Blender's
    # importer infers use_connect for coincident endpoints, even with
    # force_connect_children=False (import_fbx.py child_connect). That inference
    # disables valid bone translation channels. Restore the source hierarchy's
    # explicit unconnected semantics; do not change keys or bone transforms.
    bpy.context.view_layer.objects.active = arms[0]
    bpy.ops.object.mode_set(mode="EDIT")
    for bone in arms[0].data.edit_bones:
        bone.use_connect = False
    bpy.ops.object.mode_set(mode="OBJECT")
    return arms[0], meshes[0]


arm, mesh = read(out / "meshes/MaraOperator.fbx")
check(len(arm.data.bones) == source["bones"], "Exact source skeleton", len(arm.data.bones))
check(set(arm.data.bones.keys()) == {item["name"] for item in source["bone_definitions"]}, "All named bones and grips survived")
check(all(1 <= len(v.groups) <= 4 for v in mesh.data.vertices), "Every vertex has one to four influences")
check(all(abs(sum(g.weight for g in v.groups) - 1) < .0001 for v in mesh.data.vertices), "All skin weights normalized")
check(any(len(v.groups) > 1 for v in mesh.data.vertices), "Deformation weights are smooth rather than rigid")
check(bool(mesh.data.uv_layers), "UV0 retained")
check(len(mesh.data.materials) == 1, "Single runtime PBR surface")
expected_bounds = [b - a for a, b in zip(source["bounds_min_m"], source["bounds_max_m"])]
check(all(abs(a - b) < .015 for a, b in zip(mesh.dimensions, expected_bounds)), "Metre scale and orientation retained", list(mesh.dimensions))
mesh.data.calc_loop_triangles()
check(80000 < len(mesh.data.loop_triangles) < 125000, "Hero triangle budget", len(mesh.data.loop_triangles))
check(arm.data.bones["portrait"].head_local.x > 0, "Front portrait grip faces +X")

for name, info in source["clips"].items():
    arm, mesh = read(out / "animations" / (name + ".fbx"))
    scene = bpy.context.scene
    action = arm.animation_data.action
    check(abs((action.frame_range[1] - action.frame_range[0]) / 30 - info["duration_s"]) < .04,
          name + " duration", tuple(action.frame_range))
    scene.frame_set(1)
    start = {bone.name: bone.matrix.translation.copy() for bone in arm.pose.bones}
    moved, maximum_bounds, foot_motion, root_motion = 0., 0., 0., 0.
    error, extra_edge_length = 0., 0.
    edges = np.array([e.vertices[:] for e in mesh.data.edges])
    vertices = np.array([v.co[:] for v in mesh.data.vertices])
    rest_lengths = np.linalg.norm(vertices[edges[:, 1]] - vertices[edges[:, 0]], axis=1)
    for frame in range(1, info["frames"] + 1, 2):
        scene.frame_set(frame)
        depsgraph = bpy.context.evaluated_depsgraph_get()
        evaluated = mesh.evaluated_get(depsgraph)
        bounds = [Vector(v) for v in evaluated.bound_box]
        maximum_bounds = max(maximum_bounds, max(v.length for v in bounds))
        posed = np.array([v.co[:] for v in evaluated.data.vertices])
        posed_lengths = np.linalg.norm(posed[edges[:, 1]] - posed[edges[:, 0]], axis=1)
        extra_edge_length = max(extra_edge_length, float(np.max(posed_lengths - rest_lengths)))
        for name_bone, position in start.items():
            error = max(error, (arm.pose.bones[name_bone].matrix.translation - reference[name][frame][name_bone]).length)
            delta = (arm.pose.bones[name_bone].matrix.translation - position).length
            moved = max(moved, delta)
            if name_bone.startswith("foot_"):
                foot_motion = max(foot_motion, delta)
            if name_bone == "root":
                root_motion = max(root_motion, delta)
    check(moved > .002 and moved < .5, name + " sampled bone motion", moved)
    check(maximum_bounds < 2.1, name + " deformation bounds finite and contained", maximum_bounds)
    check(foot_motion < .012, name + " feet remain planted", foot_motion)
    check(root_motion < .00001, name + " no root motion", root_motion)
    check(error < .0001, name + " exported bone trajectories match source within 0.1mm", error)
    check(extra_edge_length < .10, name + " no stretched skin strips above 10cm", extra_edge_length)
    if info["loop"]:
        scene.frame_set(info["frames"])
        check(max((arm.pose.bones[b].matrix.translation - p).length for b, p in start.items()) < .0001,
              name + " seamless position loop")
    if name == "MO_load":
        scene.frame_set(12)
        reach = arm.pose.bones["grip_l"].matrix.translation
        check((reach - start["grip_l"]).length > .035, "Load reaches with left hand", list(reach))
    if name == "MO_fire":
        scene.frame_set(3)
        check(arm.pose.bones["head"].matrix.translation.x < start["head"].x - .025,
              "Fire visibly braces backward")

glb = (out / "meshes/MaraOperator.glb").read_bytes()
check(glb[:4] == b"glTF" and struct.unpack_from("<I", glb, 4)[0] == 2, "GLB2 container")
length, kind = struct.unpack_from("<II", glb, 12)
gltf = json.loads(glb[20:20 + length])
check({a["name"] for a in gltf.get("animations", [])} == set(source["clips"]), "GLB exact clip coverage")
check(len(gltf.get("skins", [])) == 1, "GLB single skin")
check(len(gltf.get("images", [])) == 3, "GLB packs three baked PBR images", len(gltf.get("images", [])))
for name in ("BaseColor", "ORM", "Normal"):
    image = bpy.data.images.load(str(out / "textures" / ("Mara_" + name + ".png")))
    check(list(image.size) == [2048, 2048], name + " 2K texture", list(image.size))
generation = json.loads((out / "generation-report.json").read_text())
check(generation["actual_resolution"] == 1536, "Generation used actual1536")
check(generation["excluded_modules_loaded"] == [], "Restricted modules were never imported")
check(generation["background_model_loaded"] is False, "No BRIA background model used")
result["limitations"] = ["Technical source interchange verification only; no Unreal import or runtime test performed by this script.",
    "Human visual approval and final art quality remain outstanding."]
result["fbx_reimport_settings"] = {"anim_offset": 0,
    "bone_connections": "Restore all use_connect=False, matching source; Blender guesses connected tips and otherwise suppresses valid translation keys.",
    "track_and_transform_edits": "none"}
(out / "interchange-verification.json").write_text(json.dumps(result, indent=2))
print("MARA_OPERATOR_VERIFY " + json.dumps({"checks": len(result["checks"]), "failures": result["failures"]}))
if result["failures"]:
    raise RuntimeError("Failed: " + ", ".join(result["failures"]))

"""Original simple deformation rig and authored operator action clips.

Blender CPU-only. Consumes the new Mara_Surface.blend; no anatomy is substituted.
The mesh is an MVP triangular deformation surface, not a finished retopology.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys

import bpy
import numpy as np
from mathutils import Matrix, Quaternion, Vector

parser = argparse.ArgumentParser()
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--no-render", action="store_true")
parser.add_argument("--weights", choices=("heat", "capsule"), default="heat")
args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
out = args.output.resolve()
bpy.ops.wm.open_mainfile(filepath=str(out / "Mara_Surface.blend"))
scene = bpy.context.scene
scene.render.engine = "CYCLES"
scene.cycles.device = "CPU"
scene.cycles.samples = 16
scene.render.threads_mode = "FIXED"
scene.render.threads = 6
scene.render.fps = 30
mesh = bpy.data.objects["MaraOperator"]
definitions = []


def define(name, head, tail, parent=None, radius=.07, deform=True):
    definitions.append(dict(name=name, head=head, tail=tail, parent=parent, radius=radius, deform=deform))


define("root", (0, 0, 0), (0, 0, .14), deform=False)
define("pelvis", (0, 0, .97), (0, 0, 1.09), "root", .16)
define("spine", (0, 0, 1.09), (0, 0, 1.27), "pelvis", .14)
define("chest", (0, 0, 1.27), (0, 0, 1.43), "spine", .14)
define("neck", (0, 0, 1.43), (0, 0, 1.53), "chest", .065)
define("head", (0, 0, 1.53), (0, 0, 1.74), "neck", .13)
for side, sign in (("l", 1), ("r", -1)):
    define("clavicle_" + side, (0, sign * .07, 1.42), (.02, sign * .19, 1.405), "chest", .085)
    define("upperarm_" + side, (.02, sign * .19, 1.405), (.055, sign * .305, 1.16), "clavicle_" + side, .08)
    define("forearm_" + side, (.055, sign * .305, 1.16), (.10, sign * .43, .94), "upperarm_" + side, .07)
    define("hand_" + side, (.10, sign * .43, .94), (.12, sign * .46, .815), "forearm_" + side, .065)
    define("thigh_" + side, (0, sign * .105, .97), (0, sign * .155, .54), "pelvis", .115)
    define("calf_" + side, (0, sign * .155, .54), (0, sign * .205, .13), "thigh_" + side, .085)
    define("foot_" + side, (0, sign * .205, .13), (.16, sign * .205, .075), "calf_" + side, .10)
    define("grip_" + side, (.12, sign * .46, .815), (.16, sign * .46, .815), "hand_" + side, deform=False)
define("portrait", (.08, 0, 1.63), (.14, 0, 1.63), "head", deform=False)


def select(objects):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.hide_set(False)
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]


arm_data = bpy.data.armatures.new("MaraOperator_Skeleton")
arm = bpy.data.objects.new("SK_MaraOperator", arm_data)
bpy.context.collection.objects.link(arm)
select([arm])
bpy.ops.object.mode_set(mode="EDIT")
for info in definitions:
    bone = arm_data.edit_bones.new(info["name"])
    bone.head, bone.tail = info["head"], info["tail"]
    bone.use_deform = info["deform"]
    if info["parent"]:
        bone.parent = arm_data.edit_bones[info["parent"]]
bpy.ops.object.mode_set(mode="OBJECT")
arm.show_in_front = True

# Smooth capsule weights, limited to four influences, with explicit anatomical
# eligibility so hands never pull the hip and one leg never pulls the other.
verts = np.array([v.co[:] for v in mesh.data.vertices], dtype=np.float64)
deforming = [d for d in definitions if d["deform"]]
distances = []
for info in deforming:
    a, b = np.array(info["head"]), np.array(info["tail"])
    direction = b - a
    t = np.clip(((verts - a) @ direction) / np.dot(direction, direction), 0, 1)
    distance = np.linalg.norm(verts - (a + t[:, None] * direction), axis=1)
    distances.append(distance / info["radius"])
scores = np.exp(-np.array(distances).T ** 2 * 2)
# A continuous anatomical blend separates torso cloth from the hanging arm.
# Capsule distances alone let the lower shirt inherit forearm rotation when a
# hand crosses the body. Avoid a hard boundary: it would tear the skin at a row
# of otherwise adjacent vertices.
inner_arm = np.interp(verts[:, 2], [.80, .94, 1.16, 1.405, 1.52], [.43, .37, .24, .12, .12])
arm_fraction = 1 / (1 + np.exp(np.clip(-(np.abs(verts[:, 1]) - inner_arm) / .018, -50, 50)))
for j, info in enumerate(deforming):
    name = info["name"]
    if name.startswith(("upperarm", "forearm", "hand")):
        scores[:, j] *= arm_fraction
    elif name in ("pelvis", "spine", "chest", "neck", "head"):
        scores[:, j] *= 1 - arm_fraction
    if name.endswith("_l"):
        scores[verts[:, 1] < -.02, j] = 0
    if name.endswith("_r"):
        scores[verts[:, 1] > .02, j] = 0
    if name.startswith(("thigh", "calf", "foot")):
        scores[verts[:, 2] > 1.06, j] = 0
indices = np.argsort(scores, axis=1)[:, -4:]
kept = np.take_along_axis(scores, indices, axis=1)
kept /= np.maximum(kept.sum(axis=1, keepdims=True), 1e-20)
groups = [mesh.vertex_groups.new(name=d["name"]) for d in deforming]
for i in range(len(verts)):
    for j, weight in zip(indices[i], kept[i]):
        if weight > .00001:
            groups[j].add([i], float(weight), "REPLACE")
# Normalise again after dropping immaterial influences.
for v in mesh.data.vertices:
    total = sum(g.weight for g in v.groups)
    if total == 0:
        raise RuntimeError("Unweighted vertex " + str(v.index))
    for g in v.groups:
        mesh.vertex_groups[g.group].add([v.index], g.weight / total, "REPLACE")
if args.weights == "heat":
    # The repaired closed surface allows Blender's heat solver to account for
    # the actual shoulder/arm topology instead of Euclidean capsule proximity.
    mesh.vertex_groups.clear()
    select([arm, mesh])
    bpy.ops.object.parent_set(type="ARMATURE_AUTO")
    select([mesh])
    bpy.ops.object.vertex_group_limit_total(limit=4)
    bpy.ops.object.vertex_group_normalize_all(lock_active=False)
    if any(not v.groups for v in mesh.data.vertices):
        raise RuntimeError("Heat skinning left unweighted source vertices")
modifier = next((m for m in mesh.modifiers if m.type == "ARMATURE"), None)
if modifier is None:
    modifier = mesh.modifiers.new("Mara_SmoothDeformation", "ARMATURE")
modifier.object = arm
modifier.use_deform_preserve_volume = True
mesh.parent = arm
arm.animation_data_create()
rest = {b.name: b for b in arm.data.bones}
READY_HANDS = {"l": (.30, .32, 1.31), "r": (.407, .105, 1.126)}


def segment(name, head, tail):
    head, tail = Vector(head), Vector(tail)
    bone = arm.pose.bones[name]
    old = rest[name]
    turn = (old.tail_local - old.head_local).rotation_difference(tail - head)
    rotation = turn @ old.matrix_local.to_quaternion()
    bone.matrix = Matrix.Translation(head) @ rotation.to_matrix().to_4x4()
    bpy.context.view_layer.update()


def chain(a, target, length_a, length_b, pole):
    a, target, pole = Vector(a), Vector(target), Vector(pole)
    direction = target - a
    distance = max(.001, min(direction.length, length_a + length_b - .004))
    direction.normalize()
    target = a + direction * distance
    along = (length_a * length_a - length_b * length_b + distance * distance) / (2 * distance)
    height = math.sqrt(max(0, length_a * length_a - along * along))
    bend = pole - a
    bend -= direction * bend.dot(direction)
    bend.normalize()
    return a + direction * along + bend * height, target


def pose(lean=.045, crouch=0, hands=None, recoil=0, look=0):
    if hands is None:
        hands = READY_HANDS
    for b in arm.pose.bones:
        b.matrix_basis = Matrix.Identity(4)
    pelvis = Vector((-.035 - recoil, 0, .97 - crouch))
    pelvis_top = pelvis + Vector((.012, 0, .12))
    spine_top = pelvis_top + Vector((lean * .40, 0, .18))
    chest_top = spine_top + Vector((lean * .60, 0, .16))
    neck_top = chest_top + Vector((.01, 0, .10))
    head_top = neck_top + Vector((.015, look, .21))
    segment("pelvis", pelvis, pelvis_top)
    segment("spine", pelvis_top, spine_top)
    segment("chest", spine_top, chest_top)
    segment("neck", chest_top, neck_top)
    segment("head", neck_top, head_top)
    for side, sign in (("l", 1), ("r", -1)):
        shoulder_base = chest_top + Vector((0, sign * .07, -.01))
        shoulder = chest_top + Vector((0, sign * .19, -.025))
        segment("clavicle_" + side, shoulder_base, shoulder)
        target = Vector(hands[side]) + Vector((-recoil * .9, 0, -crouch * .1))
        upper, fore = rest["upperarm_" + side].length, rest["forearm_" + side].length
        elbow, wrist = chain(shoulder, target, upper, fore, (0, sign * .60, 1.05))
        segment("upperarm_" + side, shoulder, elbow)
        segment("forearm_" + side, elbow, wrist)
        segment("hand_" + side, wrist, wrist + Vector((.125, -sign * .025, -.035)))
        hip = pelvis + Vector((0, sign * .105, 0))
        ankle = Vector((0, sign * .205, .13))
        knee, ankle = chain(hip, ankle, rest["thigh_" + side].length,
                            rest["calf_" + side].length, (.50, sign * .17, .52))
        segment("thigh_" + side, hip, knee)
        segment("calf_" + side, knee, ankle)
        segment("foot_" + side, ankle, ankle + Vector((.16, 0, -.055)))
    bpy.context.view_layer.update()


clips = {
    "MO_idle": dict(duration_s=2, loop=True, cue="ready/breathing"),
    "MO_load": dict(duration_s=.70, loop=False, cue="cosmetic loading reach at .38s"),
    "MO_fire": dict(duration_s=.34, loop=False, cue="committed shot; brace at 0s"),
    "MO_recovery": dict(duration_s=.48, loop=False, cue="release shoulder tension"),
}
for name, info in clips.items():
    action = bpy.data.actions.new(name)
    action.use_fake_user = True
    arm.animation_data.action = action
    last = 1 + round(info["duration_s"] * 30)
    scene.frame_start, scene.frame_end = 1, last
    for frame in range(1, last + 1):
        t = (frame - 1) / max(1, last - 1)
        scene.frame_set(frame)
        if name == "MO_idle":
            wave = math.sin(math.tau * t)
            pose(lean=.06 + .004 * wave, crouch=.025 + .002 * wave, look=.004 * wave)
        elif name == "MO_load":
            reach = math.sin(math.pi * t) ** 2
            pose(lean=.06 + .075 * reach, crouch=.025 + .012 * reach,
                 hands={"l": (.30 + .08 * reach, .32 + .06 * reach, 1.31 + .08 * reach),
                        "r": READY_HANDS["r"]})
        elif name == "MO_fire":
            recoil = math.exp(-max(0, t - .2) * 4) * min(1, t / .2)
            pose(lean=.06 - .04 * recoil, crouch=.025 + .032 * recoil, recoil=.07 * recoil)
        else:
            tension = (1 - t) ** 2
            pose(lean=.06 - .02 * tension, crouch=.025 + .015 * tension, recoil=.035 * tension)
        for bone in arm.pose.bones:
            bone.rotation_mode = "QUATERNION"
            bone.keyframe_insert("location", frame=frame, group=bone.name)
            bone.keyframe_insert("rotation_quaternion", frame=frame, group=bone.name)
            bone.keyframe_insert("scale", frame=frame, group=bone.name)
    info["frames"] = last
    info["duration_s"] = (last - 1) / 30
    info["gameplay_authority"] = False


def rest_pose():
    arm.animation_data.action = None
    for bone in arm.pose.bones:
        bone.matrix_basis = Matrix.Identity(4)
    bpy.context.view_layer.update()


rest_pose()
select([arm, mesh])
fbx = dict(use_selection=True, object_types={"ARMATURE", "MESH"}, add_leaf_bones=False,
           use_armature_deform_only=False, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
           axis_forward="-Y", axis_up="Z", path_mode="RELATIVE", embed_textures=False)
bpy.ops.export_scene.fbx(filepath=str(out / "meshes/MaraOperator.fbx"), bake_anim=False, **fbx)
for name, info in clips.items():
    arm.animation_data.action = bpy.data.actions[name]
    scene.frame_start, scene.frame_end = 1, info["frames"]
    bpy.ops.export_scene.fbx(filepath=str(out / "animations" / (name + ".fbx")), bake_anim=True,
                            bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
                            bake_anim_simplify_factor=0, **fbx)
rest_pose()
for name, info in clips.items():
    track = arm.animation_data.nla_tracks.new()
    track.name = name
    strip = track.strips.new(name, 1, bpy.data.actions[name])
    strip.action_frame_start, strip.action_frame_end = 1, info["frames"]
    track.mute = True
bpy.ops.export_scene.gltf(filepath=str(out / "meshes/MaraOperator.glb"), use_selection=True,
                          export_format="GLB", export_animations=True, export_animation_mode="ACTIONS",
                          export_anim_single_armature=False, export_all_influences=False, export_extras=True)
arm.animation_data.action = bpy.data.actions["MO_idle"]
scene.frame_set(1)
for image in bpy.data.images:
    if image.filepath and image.name.startswith("Mara_"):
        image.pack()


def render(name, location, target, ortho=2.55):
    cam = bpy.data.cameras.new(name)
    obj = bpy.data.objects.new(name, cam)
    bpy.context.collection.objects.link(obj)
    obj.location = location
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat("-Z", "Y").to_euler()
    cam.type, cam.ortho_scale = "ORTHO", ortho
    scene.camera = obj
    scene.render.resolution_x, scene.render.resolution_y = 1200, 1000
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = str(out / "renders" / (name + ".png"))
    bpy.ops.render.render(write_still=True)


if not args.no_render:
    render("rig_ready", (3.4, -2.8, 1.7), (.1, 0, .95))
    render("rig_action_rear", (-3.4, -2.8, 1.9), (.1, 0, .95))
    arm.animation_data.action = bpy.data.actions["MO_load"]
    scene.frame_set(12)
    render("rig_load", (3.4, -2.8, 1.7), (.1, 0, .95))
    arm.animation_data.action = bpy.data.actions["MO_fire"]
    scene.frame_set(3)
    render("rig_fire", (-3.4, -2.8, 1.9), (.1, 0, .95))
arm.animation_data.action = bpy.data.actions["MO_idle"]
scene.frame_set(1)
report = json.loads((out / "surface-report.json").read_text())
report.update(asset="MaraOperator_v001", fps=30, bones=len(definitions), bone_definitions=definitions, clips=clips,
    skinning_method=args.weights,
    max_influences=max(len(v.groups) for v in mesh.data.vertices),
    source_forward="+X", unreal_conversion="(x,-y,z)*100; do not apply actor unit scale twice",
    placement_proposal_cm={"gun_origin": [-350, 0, 1.5], "operator_origin": [-496, 55, .7], "yaw_degrees": 0},
    sockets_rest_m={name: list(arm.data.bones[name].head_local) for name in ("root", "grip_l", "grip_r", "portrait")},
    sockets_ready_m={name: list(arm.pose.bones[name].matrix.translation) for name in ("root", "grip_l", "grip_r", "portrait")},
    limitations=["Provisional art selection; in-engine owner visual approval pending.",
        "Generated triangular surface with automatic heat skin weights; not final deforming retopology.",
        "Close facial geometry and generated side/back details remain softer than the final F.I.S.T.-quality target.",
        "Simple hands; no individual finger or face animation.",
        "No root motion, gameplay triggers or resource changes are authored by these clips.",
        "Runtime hand/gun contact and framing require integration inspection."])
report["file_sha256"] = {str(path.relative_to(out)): hashlib.sha256(path.read_bytes()).hexdigest()
    for folder in ("meshes", "animations", "textures") for path in (out / folder).glob("*") if path.is_file()}
(out / "asset-report.json").write_text(json.dumps(report, indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(out / "Mara_Operator_Source.blend"))
print("MARA_OPERATOR_RIG " + json.dumps({"bones": len(definitions), "clips": list(clips), "triangles": report["triangles"]}))

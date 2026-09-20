"""CPU mirror/review of UFoundryAimedOperator's exact current bone math.

New review outputs only. Frozen meshes, clips, textures, .blend and importer are
never saved or modified. This is not an Unreal build or human approval.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import shutil
import sys

import bpy
import numpy as np
from mathutils import Matrix, Quaternion, Vector

p = argparse.ArgumentParser()
p.add_argument("--output", type=Path, required=True)
a = p.parse_args(sys.argv[sys.argv.index("--") + 1:])
out = a.output.resolve()
game = Path(__file__).resolve().parents[1]
proof = game / "assets/production/operator-v001"
review_dir = out / "runtime-review"
review_dir.mkdir(exist_ok=True)
cpp = game / "unreal/Source/OverkillFoundry/Private/FoundryMara.cpp"
code = cpp.read_bytes()
for expected in (b"Step <= 40", b"Step * .25f", b"A + B - .4", b"FQuat::FindBetweenVectors", b"GetRelativeTransform(OldHand) * NewHand",
    b"LocalToWorld.GetScale3D().GetAbs() * 28", b"Envelope += FBox(Point - Margin, Point + Margin)"):
    if expected not in code:
        raise RuntimeError("Runtime implementation changed; update mirror before claiming a review")
report = json.loads((out / "asset-report.json").read_text())
hashes = {path: hashlib.sha256((out / path).read_bytes()).hexdigest() for path in report["file_sha256"]}
aim_report = json.loads((proof / "aim-contact-review.json").read_text())
targets = {"mite": Vector(aim_report["maxima"]["fixed_contact_anchor_drift_cm"]["target_world_cm"]),
    "gatebreaker": Vector(aim_report["maxima"]["fixed_anchor_reach_deficit_cm"]["target_world_cm"])}
operator_origin = Vector((-496, 55, .7))
gun_origin = Vector((-350, 0, 1.5))
gun_pivot = Vector((-380, 0, 97.5))
reflect = Matrix.Diagonal((1., -1., 1.))

bpy.ops.wm.open_mainfile(filepath=str(out / "Mara_Operator_Source.blend"))
scene = bpy.context.scene
arm = bpy.data.objects["SK_MaraOperator"]
mesh = bpy.data.objects["MaraOperator"]
for obj in (arm, mesh):
    if max(abs(obj.matrix_world[row][col] - float(row == col)) for row in range(4) for col in range(4)) > 1e-6:
        raise RuntimeError("Review expects frozen source objects in identity world space")
for modifier in mesh.modifiers:
    if modifier.type == "ARMATURE":
        # Temporary scene only: UE's usual skeletal skinning is linear blend.
        # The frozen authoring .blend and all production assets remain untouched.
        modifier.use_deform_preserve_volume = False
names = [bone.name for bone in arm.data.bones]
under_spine = {b.name for b in arm.data.bones if b.name == "spine" or any(p.name == "spine" for p in b.parent_recursive)}
under_hand = {side: {b.name for b in arm.data.bones if any(p.name == "hand_" + side for p in b.parent_recursive)} for side in ("l", "r")}
rest_vertices = np.array([v.co[:] for v in mesh.data.vertices])
edges = np.array([e.vertices[:] for e in mesh.data.edges])
edge_lengths = np.linalg.norm(rest_vertices[edges[:, 1]] - rest_vertices[edges[:, 0]], axis=1)
rest_homogeneous = np.column_stack((rest_vertices, np.ones(len(rest_vertices))))
inverse_bind = {b.name: np.array(b.matrix_local.inverted()) for b in arm.data.bones}
weight_lists = {b.name: [] for b in arm.data.bones if b.use_deform}
for vertex in mesh.data.vertices:
    for group in vertex.groups:
        name = mesh.vertex_groups[group.group].name
        if name in weight_lists and group.weight > 0:
            weight_lists[name].append((vertex.index, group.weight))
weights = {name: (np.array([i for i, _ in items], dtype=np.int64), np.array([w for _, w in items]))
    for name, items in weight_lists.items() if items}
weight_sums = np.zeros(len(rest_vertices))
for indices, amounts in weights.values():
    weight_sums[indices] += amounts
if not np.all(np.isfinite(weight_sums)) or np.min(weight_sums) < .999:
    raise RuntimeError("Incomplete/nonfinite source skin weights")
source_to_ue = np.diag((100., -100., 100.))
rest_world = rest_vertices @ source_to_ue + np.array(operator_origin)
imported_min, imported_max = rest_world.min(0), rest_world.max(0)


def plain(vector):
    return [float(v) for v in vector]


def source_matrix(item):
    position = reflect @ (item["p"] - operator_origin) / 100
    rotation = reflect @ item["q"].to_matrix() @ reflect
    return Matrix.Translation(position) @ rotation.to_4x4() @ Matrix.Diagonal((*item["s"], 1))


def linear_skin(pose):
    points = np.zeros_like(rest_vertices)
    for name, (indices, amounts) in weights.items():
        skin = np.array(source_matrix(pose[name])) @ inverse_bind[name]
        points[indices] += (rest_homogeneous[indices] @ skin.T)[:, :3] * amounts[:, None]
    return points / weight_sums[:, None]


def envelope_check(pose, points):
    """Exact current unit-scale runtime FBox union, in world centimetres."""
    world = points @ source_to_ue + np.array(operator_origin)
    origins = np.array([plain(item["p"]) for item in pose.values()])
    minimum = np.minimum(imported_min, origins.min(0) - 28)
    maximum = np.maximum(imported_max, origins.max(0) + 28)
    distances = np.minimum(world - minimum, maximum - world)
    clearance = np.min(distances, axis=1)
    finite = bool(np.all(np.isfinite(world)) and np.all(np.isfinite(origins)))
    return {"all_vertices_finite": finite,
        "outside_vertex_count": int(np.count_nonzero(clearance < -1e-4)),
        "minimum_vertex_clearance_cm": float(clearance.min()),
        "maximum_vertex_escape_cm": float(max(0., -clearance.min())),
        "envelope_world_cm": [minimum.tolist(), maximum.tolist()],
        "deformed_vertex_world_cm": [world.min(0).tolist(), world.max(0).tolist()]}


def read_pose():
    return {b.name: {"p": operator_origin + reflect @ b.matrix.translation * 100,
        "q": (reflect @ b.matrix.to_quaternion().to_matrix() @ reflect).to_quaternion(),
        "s": b.matrix.to_scale()} for b in arm.pose.bones}


def aim_rotation(target):
    delta = target - gun_pivot
    yaw = math.atan2(delta.y, delta.x)
    pitch = math.atan2(delta.z, math.hypot(delta.x, delta.y)) - math.asin(max(-1, min(1, 52 / delta.length)))
    cp, sp, cy, sy = math.cos(pitch), math.sin(pitch), math.cos(yaw), math.sin(yaw)
    return Matrix(((cp * cy, -sy, -sp * cy), (cp * sy, cy, -sp * sy), (sp, 0, cp))).to_quaternion()


def solve(base, turn):
    pose = {name: {key: value.copy() for key, value in item.items()} for name, item in base.items()}
    targets, rotations, lengths = {}, {}, {}
    worst, lean_direction = 0., Vector((1, 0, 0))
    for side in ("l", "r"):
        u, f, h = [pose[name + "_" + side]["p"] for name in ("upperarm", "forearm", "hand")]
        targets[side] = gun_pivot + turn @ (h - gun_pivot)
        rotations[side] = turn @ pose["hand_" + side]["q"]
        lengths[side] = ((u - f).length, (f - h).length)
        reach = targets[side] - u
        excess = reach.length - (sum(lengths[side]) - .4)
        if excess > worst:
            worst = excess
            lean_direction = Vector((reach.x, reach.y, 0)).normalized()
    pivot = pose["pelvis"]["p"]
    degrees, azimuth = 0., 0.
    if worst > 0 and lean_direction.length > 1e-6:
        azimuth = max(-math.pi / 4, min(math.pi / 4, math.atan2(lean_direction.y, lean_direction.x)))
        lean_direction = Vector((math.cos(azimuth), math.sin(azimuth), 0))
        axis = Vector((0, 0, 1)).cross(lean_direction).normalized()
        for step in range(1, 41):
            degrees = step * .25
            lean = Quaternion(axis, math.radians(degrees))
            if all((targets[s] - (pivot + lean @ (pose["upperarm_" + s]["p"] - pivot))).length <= sum(lengths[s]) - .4 for s in ("l", "r")):
                break
        for name in under_spine:
            pose[name]["p"] = pivot + lean @ (pose[name]["p"] - pivot)
            pose[name]["q"] = lean @ pose[name]["q"]
    hand_errors = {}
    for side in ("l", "r"):
        upper, fore, hand = [name + "_" + side for name in ("upperarm", "forearm", "hand")]
        shoulder, old_elbow = pose[upper]["p"], pose[fore]["p"]
        old_hand = {key: value.copy() for key, value in pose[hand].items()}
        reach = targets[side] - shoulder
        direction = reach.normalized()
        length_a, length_b = lengths[side]
        distance = max(abs(length_a - length_b) + .01, min(length_a + length_b - .4, reach.length))
        pole = old_elbow - shoulder
        pole -= direction * pole.dot(direction)
        if pole.length < 1e-6:
            pole = direction.cross(Vector((0, 0, 1)))
        pole.normalize()
        along = (length_a ** 2 - length_b ** 2 + distance ** 2) / (2 * distance)
        elbow = shoulder + direction * along + pole * math.sqrt(max(0, length_a ** 2 - along ** 2))
        wrist = shoulder + direction * distance
        pose[upper]["q"] = (old_elbow - shoulder).rotation_difference(elbow - shoulder) @ pose[upper]["q"]
        pose[fore]["p"] = elbow
        pose[fore]["q"] = (old_hand["p"] - old_elbow).rotation_difference(wrist - elbow) @ pose[fore]["q"]
        new_hand_q = rotations[side]
        hand_delta = new_hand_q @ old_hand["q"].inverted()
        for name in under_hand[side]:
            pose[name]["p"] = wrist + hand_delta @ (pose[name]["p"] - old_hand["p"])
            pose[name]["q"] = hand_delta @ pose[name]["q"]
        pose[hand]["p"], pose[hand]["q"] = wrist, new_hand_q
        wanted_grip = gun_pivot + turn @ (base["grip_" + side]["p"] - gun_pivot)
        hand_errors[side] = {"wrist_error_cm": (wrist - targets[side]).length,
            "grip_error_cm": (pose["grip_" + side]["p"] - wanted_grip).length,
            "upper_length_error_cm": abs((elbow - shoulder).length - length_a),
            "fore_length_error_cm": abs((wrist - elbow).length - length_b),
            "upper_rotation_degrees": math.degrees(pose[upper]["q"].rotation_difference(base[upper]["q"]).angle),
            "fore_rotation_degrees": math.degrees(pose[fore]["q"].rotation_difference(base[fore]["q"]).angle)}
    lower_motion = max((pose[name]["p"] - base[name]["p"]).length for name in ("root", "pelvis", "thigh_l", "thigh_r", "calf_l", "calf_r", "foot_l", "foot_r"))
    return pose, {"lean_degrees": degrees, "lean_azimuth_degrees": math.degrees(azimuth),
        "lower_body_motion_cm": lower_motion, "hands": hand_errors}


bases, results, envelopes = {}, [], []
for clip, info in report["clips"].items():
    arm.animation_data.action = bpy.data.actions[clip]
    for frame in range(1, info["frames"] + 1):
        scene.frame_set(frame)
        base = read_pose()
        bases[(clip, frame)] = base
        envelopes.append({"target": "authored_unmodified", "clip": clip, "frame": frame,
            **envelope_check(base, linear_skin(base))})
        for label, target in targets.items():
            solved, evidence = solve(base, aim_rotation(target))
            results.append({"target": label, "clip": clip, "frame": frame, **evidence})
            envelopes.append({"target": label, "clip": clip, "frame": frame,
                **envelope_check(solved, linear_skin(solved))})

with bpy.data.libraries.load(str(out.parent / "mara-v001/Mara_Source.blend"), link=False) as (source, target):
    target.objects = [n for n in source.objects if n in ("SK_MaraGun", "SKM_MaraGun")]
    target.actions = [n for n in source.actions if n == "MG_idle"]
for obj in target.objects:
    bpy.context.collection.objects.link(obj)
gun = bpy.data.objects["SK_MaraGun"]
gun.location = (1.46, .55, .008)
gun.animation_data.action = bpy.data.actions["MG_idle"]
scene.frame_set(1)
bpy.context.view_layer.update()
gun_cradle = gun.pose.bones["cradle"].matrix.copy()
gun.animation_data.action = None
arm.animation_data.action = None
scene.render.engine = "CYCLES"
scene.cycles.device = "CPU"
scene.cycles.samples = 24
scene.render.threads_mode = "FIXED"
scene.render.threads = 6
bpy.ops.mesh.primitive_plane_add(size=20, location=(0, 0, -.012))
floor = bpy.context.object
floor_material = bpy.data.materials.new("RuntimeReviewFloor")
floor_material.diffuse_color = (.07, .08, .085, 1)
floor.data.materials.append(floor_material)


def apply_pose(pose):
    for bone in arm.pose.bones:
        bone.matrix_basis = Matrix.Identity(4)
    for name in names:
        item = pose[name]
        arm.pose.bones[name].matrix = source_matrix(item)
        bpy.context.view_layer.update()
    error = max(((operator_origin + reflect @ arm.pose.bones[name].matrix.translation * 100) - pose[name]["p"]).length for name in names)
    if error > .01:
        raise RuntimeError("Pose conversion/application mismatch: " + str(error))
    evaluated_object = mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
    evaluated = evaluated_object.to_mesh()
    points = np.array([v.co[:] for v in evaluated.vertices])
    evaluated_object.to_mesh_clear()
    linear_error_cm = float(np.linalg.norm(points - linear_skin(pose), axis=1).max() * 100)
    if linear_error_cm > .01:
        raise RuntimeError("Linear skinning differs from actual Blender evaluation: " + str(linear_error_cm))
    stretch = np.linalg.norm(points[edges[:, 1]] - points[edges[:, 0]], axis=1) - edge_lengths
    return {"pose_application_error_cm": error, "maximum_extra_edge_length_cm": float(stretch.max() * 100),
        "linear_skin_vs_blender_max_error_cm": linear_error_cm,
        "deformed_bounds_source_m": [points.min(0).tolist(), points.max(0).tolist()],
        "actual_evaluated_vertex_envelope_check": envelope_check(pose, points)}


def render(filename, location, focus, size):
    data = bpy.data.cameras.new(filename)
    obj = bpy.data.objects.new(filename, data)
    bpy.context.collection.objects.link(obj)
    obj.location = location
    obj.rotation_euler = (Vector(focus) - obj.location).to_track_quat("-Z", "Y").to_euler()
    data.type, data.ortho_scale = "ORTHO", size
    scene.camera = obj
    scene.render.resolution_x, scene.render.resolution_y = 1440, 1000
    scene.render.resolution_percentage = 100
    scene.render.filepath = str(review_dir / (filename + ".png"))
    bpy.ops.render.render(write_still=True)


rendered = {}
for label, target in targets.items():
    turn = aim_rotation(target)
    solved, evidence = solve(bases[("MO_idle", 1)], turn)
    evidence.update(apply_pose(solved))
    turn_source = reflect @ turn.to_matrix() @ reflect
    pivot_source = gun_cradle.translation
    gun.pose.bones["cradle"].matrix = Matrix.Translation(pivot_source) @ turn_source.to_4x4() @ Matrix.Translation(-pivot_source) @ gun_cradle
    bpy.context.view_layer.update()
    render(label + "_side", (1.0, -6.5, 2.3), (1., .35, 1.), 4.8)
    render(label + "_action", (-4.5, -5.5, 2.8), (.9, .25, 1.05), 4.4)
    rendered[label] = evidence

def worst(metric):
    return max(({"value": values[metric], "side": side, "target": r["target"], "clip": r["clip"], "frame": r["frame"]}
        for r in results for side, values in r["hands"].items()), key=lambda r: r["value"])

result = {"review": "CPU source mirror of current uncompiled UFoundryAimedOperator, not engine verification or owner approval",
    "runtime_cpp_sha256": hashlib.sha256(code).hexdigest(), "frozen_source_fbx_sha256": hashes["meshes\\MaraOperator.fbx"],
    "target_world_cm": {name: plain(point) for name, point in targets.items()},
    "sampled_clip_frames": len(bases), "pose_target_combinations": len(results),
    "method": "Mirrors current authored-wrist target, worst-wrist lean azimuth clamped+/-45deg,0.25deg shared tilt steps to10deg, component-space two-bone rotations, current hand rotation times gun aim and relative hand-child transforms.",
    "maxima": {name: worst(name) for name in ("wrist_error_cm", "grip_error_cm", "upper_length_error_cm", "fore_length_error_cm", "upper_rotation_degrees", "fore_rotation_degrees")},
    "maximum_lean_degrees": max(r["lean_degrees"] for r in results),
    "maximum_lower_body_motion_cm": max(r["lower_body_motion_cm"] for r in results),
    "culling_envelope": {"method": "FBox union of imported rest-vertex world bounds and each current component-space bone origin transformed to world, expanded by abs(component scale)*28cm. Current placement has identity scale and rotation. All vertices evaluated with normalized source linear skin weights; verified against actual Blender linear modifier on both rendered poses.",
        "sampled_pose_count": len(envelopes), "source_vertices_per_pose": len(rest_vertices),
        "total_vertex_samples": len(envelopes) * len(rest_vertices),
        "all_vertices_finite": all(r["all_vertices_finite"] for r in envelopes),
        "maximum_outside_vertex_count": max(r["outside_vertex_count"] for r in envelopes),
        "minimum_clearance_case": min(envelopes, key=lambda r: r["minimum_vertex_clearance_cm"]),
        "per_clip": {clip: {"poses": sum(r["clip"] == clip for r in envelopes),
            "minimum_vertex_clearance_cm": min(r["minimum_vertex_clearance_cm"] for r in envelopes if r["clip"] == clip),
            "maximum_vertex_escape_cm": max(r["maximum_vertex_escape_cm"] for r in envelopes if r["clip"] == clip)} for clip in report["clips"]},
        "imported_rest_world_cm": [imported_min.tolist(), imported_max.tolist()],
        "asset_dimensions_changed": False},
    "rendered_idle_extremes": rendered,
    "clip_summary": {clip: {"maximum_wrist_error_cm": max(v["wrist_error_cm"] for r in results if r["clip"] == clip for v in r["hands"].values()),
        "maximum_lean_degrees": max(r["lean_degrees"] for r in results if r["clip"] == clip)} for clip in report["clips"]},
    "discrepancies_from_aim_study": ["Runtime preserves the current authored wrist/hand gesture through aiming, instead of maintaining fixed idle anchors during load/fire. This is intentional and preserves clip motion.",
        "Runtime chooses one worst-wrist azimuth and quarter-degree steps; the earlier study searched several azimuths and used binary search. This report measures the exact newer choice.",
        "Current C++ has no explicit target-transition smoothing; the source renders show endpoints only. Rapid target changes require engine inspection."],
    "limits": ["Rendered proof is Blender CPU with frozen source materials and geometry, not an Unreal screenshot.",
        "All109 frames of the four original clips tested at two specified extreme target points. This does not expand runtime target/overflow coverage.",
        "Only idle extremes rendered; all four clips additionally receive finite linear-skinned vertex and culling-envelope checks at every authored frame.",
        "Proof uses linear blend skinning in the temporary review scene, matching the usual UE method; frozen authoring geometry, weights, clips and modifier settings are unchanged. Actual UE deformation still needs native review.",
        "Fresh-pose buffer timing, tick ordering, component scaling and UE quaternion/FBX conventions require native verification."],
    "frozen_product_hashes_unchanged": all(hashlib.sha256((out / path).read_bytes()).hexdigest() == digest for path, digest in hashes.items())}
target_dir = proof / "runtime-review"
target_dir.mkdir(exist_ok=True)
for file in review_dir.glob("*.png"):
    shutil.copy2(file, target_dir / file.name)
(proof / "runtime-aim-review.json").write_text(json.dumps(result, indent=2))
print("MARA_RUNTIME_REVIEW " + json.dumps({"maxima": result["maxima"], "clips": result["clip_summary"], "renders": rendered,
    "envelope": result["culling_envelope"], "frozen": result["frozen_product_hashes_unchanged"]}))

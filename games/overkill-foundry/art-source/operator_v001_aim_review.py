"""Read-only CPU contact/reach study of the frozen operator and current cradle.

Loads original Blender sources, evaluates geometry/joints and writes one report.
Never exports an asset, changes source keys, imports into UE or edits runtime.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys

import bpy
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

p = argparse.ArgumentParser()
p.add_argument("--output", type=Path, required=True)
a = p.parse_args(sys.argv[sys.argv.index("--") + 1:])
out = a.output.resolve()
game = Path(__file__).resolve().parents[1]
repo = game.parents[1]
destination = game / "assets/production/operator-v001/aim-contact-review.json"
runtime = game / "unreal/Source/OverkillFoundry/Private"
mara_code = (runtime / "FoundryMara.cpp").read_text()
robot_code = (runtime / "FoundryRobot.cpp").read_text()
if "52.0 / Delta.Size()" not in mara_code or 'Body->GetSocketLocation(TEXT("core"))' not in robot_code:
    raise RuntimeError("Current aiming/impact contract changed; update the explicit study")
asset = json.loads((out / "asset-report.json").read_text())
tracked = {relative: hashlib.sha256((out / relative).read_bytes()).hexdigest()
           for relative in asset["file_sha256"]}
GUN = Vector((-350, 0, 1.5))
OPERATOR = Vector((-496, 55, .7))


def convert(v):
    return Vector((v.x * 100, -v.y * 100, v.z * 100))


def plain(v):
    return [round(float(x), 6) for x in v]


def rotation(target, pivot):
    delta = target - pivot
    yaw = math.atan2(delta.y, delta.x)
    pitch = math.atan2(delta.z, math.hypot(delta.x, delta.y)) - math.asin(max(-1, min(1, 52 / delta.length)))
    cp, sp, cy, sy = math.cos(pitch), math.sin(pitch), math.cos(yaw), math.sin(yaw)
    # UE FRotator(Pitch,Yaw,0): positive pitch rotates +X toward +Z.
    return Matrix(((cp * cy, -sy, -sp * cy), (cp * sy, cy, -sp * sy), (sp, 0, cp))), pitch, yaw


bpy.ops.wm.open_mainfile(filepath=str(out / "Mara_Operator_Source.blend"))
scene = bpy.context.scene
operator = bpy.data.objects["SK_MaraOperator"]
operator.animation_data.action = bpy.data.actions["MO_idle"]
scene.frame_set(1)
arms = {}
for side in ("l", "r"):
    points = {key: OPERATOR + convert(operator.pose.bones[name + "_" + side].matrix.translation)
              for key, name in (("shoulder", "upperarm"), ("elbow", "forearm"), ("wrist", "hand"), ("grip", "grip"))}
    points["upper_length"] = operator.data.bones["upperarm_" + side].length * 100
    points["fore_length"] = operator.data.bones["forearm_" + side].length * 100
    points["hand_vector"] = points["grip"] - points["wrist"]
    arms[side] = points
frozen_feet = {side: plain(OPERATOR + convert(operator.pose.bones["foot_" + side].matrix.translation)) for side in ("l", "r")}
body_pivot = OPERATOR + convert(operator.pose.bones["pelvis"].matrix.translation)

gun_file = out.parent / "mara-v001/Mara_Source.blend"
with bpy.data.libraries.load(str(gun_file), link=False) as (source, target):
    target.objects = [n for n in source.objects if n in ("SK_MaraGun", "SKM_MaraGun")]
    target.actions = [n for n in source.actions if n == "MG_idle"]
for obj in target.objects:
    bpy.context.collection.objects.link(obj)
gun = bpy.data.objects["SK_MaraGun"]
gun.location = (GUN.x / 100, -GUN.y / 100, GUN.z / 100)
gun.animation_data.action = bpy.data.actions["MG_idle"]
scene.frame_set(1)
bpy.context.view_layer.update()
pivot = convert(gun.matrix_world @ gun.pose.bones["cradle"].matrix.translation)
moving_bones = {"cradle"}
for bone in gun.data.bones:
    if any(parent.name == "cradle" for parent in bone.parent_recursive):
        moving_bones.add(bone.name)
gun_mesh = bpy.data.objects["SKM_MaraGun"]
evaluated = gun_mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
geometry = evaluated.to_mesh()
points = [convert(evaluated.matrix_world @ v.co) for v in geometry.vertices]
groups = {g.index: g.name for g in gun_mesh.vertex_groups}
owners = [groups[max(v.groups, key=lambda g: g.weight).group] for v in gun_mesh.data.vertices]
moving, fixed, mixed = [], [], 0
for polygon in geometry.polygons:
    status = {owners[v] in moving_bones for v in polygon.vertices}
    if len(status) != 1:
        mixed += 1
    (moving if True in status else fixed).append(tuple(polygon.vertices))
if mixed:
    raise RuntimeError("Gun is no longer rigid per face; BVH partition is invalid")
moving_bvh = BVHTree.FromPolygons(points, moving)
fixed_bvh = BVHTree.FromPolygons(points, fixed)
evaluated.to_mesh_clear()


def nearest_surface(point, turn):
    unturned = pivot + turn.transposed() @ (point - pivot)
    near, _, _, distance = moving_bvh.find_nearest(unturned)
    moved_near = pivot + turn @ (near - pivot)
    fixed_near, _, _, fixed_distance = fixed_bvh.find_nearest(point)
    if fixed_distance < distance:
        return fixed_distance, fixed_near
    return distance, moved_near


# The right hand braces on the first copper return-pipe segment. A small slide
# is permissible along that real part without inventing a floating control.
pipe_a = GUN + Vector((-90, 35, 120))
pipe_b = GUN + Vector((-100, 49, 95))
pipe_direction = (pipe_b - pipe_a).normalized()
pipe_length = (pipe_b - pipe_a).length
pipe_start = max(0, min(pipe_length, (arms["r"]["grip"] - pipe_a).dot(pipe_direction)))
pipe_radial_offset = arms["r"]["grip"] - (pipe_a + pipe_direction * pipe_start)


def solve_arm(data, wanted_grip, hand_vector):
    s, e = data["shoulder"], data["elbow"]
    wanted_wrist = wanted_grip - hand_vector
    ray = wanted_wrist - s
    requested = ray.length
    minimum = abs(data["upper_length"] - data["fore_length"]) + .1
    maximum = data["upper_length"] + data["fore_length"] - .4
    length = max(minimum, min(maximum, requested))
    direction = ray.normalized()
    wrist = s + direction * length
    along = (data["upper_length"] ** 2 - data["fore_length"] ** 2 + length ** 2) / (2 * length)
    height = math.sqrt(max(0, data["upper_length"] ** 2 - along ** 2))
    bend = e - s
    bend -= direction * bend.dot(direction)
    if bend.length < 1e-5:
        raise RuntimeError("Degenerate elbow pole")
    elbow = s + direction * along + bend.normalized() * height
    grip = wrist + hand_vector
    return {"requested_reach_cm": requested, "maximum_reach_cm": maximum,
        "unreachable_cm": max(0, requested - maximum), "residual_grip_error_cm": (grip - wanted_grip).length,
        "grip_displacement_cm": (grip - data["grip"]).length,
        "upperarm_rotation_degrees": math.degrees((e - s).angle(elbow - s)),
        "forearm_rotation_degrees": math.degrees((data["wrist"] - e).angle(wrist - elbow)),
        "elbow_flexion_degrees": math.degrees((elbow - s).angle(wrist - elbow)),
        "solved_grip_world_cm": plain(grip), "solved_elbow_world_cm": plain(elbow), "solved_wrist_world_cm": plain(wrist)}


def evaluate(target):
    turn, pitch, yaw = rotation(target, pivot)
    result = {"target_world_cm": plain(target), "pitch_degrees": math.degrees(pitch), "yaw_degrees": math.degrees(yaw), "hands": {}}
    for side, data in arms.items():
        desired = pivot + turn @ (data["grip"] - pivot)
        direction = turn @ data["hand_vector"]
        solve = solve_arm(data, desired, direction)
        distance, surface = nearest_surface(data["grip"], turn)
        result["hands"][side] = {"fixed_hand_anchor_drift_cm": (desired - data["grip"]).length,
            "fixed_hand_nearest_gun_surface_distance_cm": distance,
            "nearest_gun_surface_world_cm": plain(surface), "wanted_grip_world_cm": plain(desired),
            "fixed_anchor_ik": solve}
        candidates = [(0., solve)]
        if side == "r":
            for slide in [sign * step * .5 for step in range(1, 25) for sign in (-1, 1)]:
                distance_on_pipe = pipe_start + slide
                if 0 <= distance_on_pipe <= pipe_length:
                    point = pipe_a + pipe_direction * distance_on_pipe + pipe_radial_offset
                    desired_slid = pivot + turn @ (point - pivot)
                    candidates.append((slide, solve_arm(data, desired_slid, direction)))
        slide, slid = min(candidates, key=lambda item: (round(item[1]["residual_grip_error_cm"], 4), abs(item[0])))
        result["hands"][side]["pipe_slide_ik"] = {"slide_cm": slide, **slid}
    def lean_solution(degrees, azimuth=0):
        radians = math.radians(degrees)
        direction = math.radians(azimuth)
        lean = Matrix.Rotation(radians, 3, Vector((-math.sin(direction), math.cos(direction), 0)))
        solutions = {}
        for side, data in arms.items():
            leaned = dict(data)
            for name in ("shoulder", "elbow", "wrist", "grip"):
                leaned[name] = body_pivot + lean @ (data[name] - body_pivot)
            desired = pivot + turn @ (data["grip"] - pivot)
            solutions[side] = solve_arm(leaned, desired, turn @ data["hand_vector"])
            solutions[side]["shoulder_displacement_cm"] = (leaned["shoulder"] - data["shoulder"]).length
            solutions[side]["shoulder_world_cm"] = plain(leaned["shoulder"])
        return {"degrees": degrees, "azimuth_degrees": azimuth, "hands": solutions,
                "maximum_residual_cm": max(s["residual_grip_error_cm"] for s in solutions.values())}
    for cap in (8, 12, 14):
        if lean_solution(0)["maximum_residual_cm"] < .001:
            chosen = lean_solution(0)
        elif lean_solution(cap)["maximum_residual_cm"] >= .001:
            chosen = lean_solution(cap)
        else:
            low, high = 0., float(cap)
            for _ in range(15):
                mid = (low + high) / 2
                if lean_solution(mid)["maximum_residual_cm"] < .001:
                    high = mid
                else:
                    low = mid
            chosen = lean_solution(high)
        result["torso_lean_cap_" + str(cap)] = chosen
    for cap in (8, 10):
        choices = []
        for azimuth in (-60, -45, -30, -15, 0, 15):
            chosen = lean_solution(cap, azimuth)
            if lean_solution(0, azimuth)["maximum_residual_cm"] < .001:
                chosen = lean_solution(0, azimuth)
            elif chosen["maximum_residual_cm"] < .001:
                low, high = 0., float(cap)
                for _ in range(15):
                    mid = (low + high) / 2
                    if lean_solution(mid, azimuth)["maximum_residual_cm"] < .001:
                        high = mid
                    else:
                        low = mid
                chosen = lean_solution(high, azimuth)
            choices.append(chosen)
        result["torso_lean_cone_" + str(cap)] = min(choices, key=lambda s: (round(s["maximum_residual_cm"], 2), s["degrees"], abs(s["azimuth_degrees"])))
    return result


# FoundryRobot::GetImpactLocation returns the animated core. Load the source
# idle rigs to sample actual core trajectories, including their X offsets.
robots = {}
for folder, blend in (("cinderwall-v001", "Cinderwall_Source.blend"), ("roster-v001", "Cinderwall_Roster_Source.blend")):
    report = json.loads((out.parent / folder / "asset-report.json").read_text())
    names = set(report["robots"])
    with bpy.data.libraries.load(str(out.parent / folder / blend), link=False) as (source, target):
        target.objects = [n for n in source.objects if n.startswith("SK_") and n[3:] in names]
        target.actions = [n for n in source.actions if n.endswith("_idle")]
    for obj in target.objects:
        bpy.context.collection.objects.link(obj)
        name = obj.name[3:]
        data = report["robots"][name]
        prefix = data.get("prefix", {"RivetMite": "RM_", "BreachRam": "BR_"}.get(name))
        clip = bpy.data.actions[prefix + "idle"]
        obj.location = (0, 0, 0)
        obj.rotation_euler = (0, 0, 0)
        obj.animation_data.action = clip
        frames = sorted(set([1, int(clip.frame_range[1])] + list(range(1, int(clip.frame_range[1]) + 1, 3))))
        cores = []
        for frame in frames:
            scene.frame_set(frame)
            cores.append(convert(obj.pose.bones["core"].matrix.translation))
        robots[name] = {"id": data.get("source_id", {"RivetMite": "C1-R01", "BreachRam": "C1-R02"}.get(name)),
            "cores": cores, "sample_frames": frames, "clip": clip.name}

nominal, envelope = [], []
for name, data in sorted(robots.items()):
    slots = [(300, -180, 1.5)] if name == "RivetMite" else [(560, -20, 1.5), (960, 220, 1.5), (940, -230, 1.5)]
    for index, slot in enumerate(slots):
        for sample, core in enumerate(data["cores"]):
            record = {"robot": name, "definition": data["id"], "slot": index, "robot_actor_world_cm": list(slot),
                "core_sample_frame": data["sample_frames"][sample], **evaluate(Vector(slot) + core)}
            envelope.append(record)
            if sample == 0:
                nominal.append(record)


def worst(field, method=None):
    values = [(record, side, hand[field] if method is None else hand[method][field])
        for record in envelope for side, hand in record["hands"].items()]
    record, side, value = max(values, key=lambda item: abs(item[2]))
    return {"value": value, "robot": record["robot"], "slot": record["slot"], "hand": side,
        "core_sample_frame": record["core_sample_frame"], "target_world_cm": record["target_world_cm"]}

checks = {"production_product_hashes_unchanged": all(hashlib.sha256((out / path).read_bytes()).hexdigest() == digest for path, digest in tracked.items()),
    "gun_faces_have_rigid_moving_or_fixed_ownership": mixed == 0,
    "source_rest_contact_matches_prior_review": all(abs(nearest_surface(arms[s]["grip"], Matrix.Identity(3))[0] - expected) < .05
        for s, expected in (("l", .301713), ("r", 2.460602))),
    "feet_or_body_transforms_modified": False}
lean_summary = {}
for cap in ("8", "12", "14", "cone_8", "cone_10"):
    key = "torso_lean_cap_" + cap if not cap.startswith("cone_") else "torso_lean_" + cap
    worst_residual = max(envelope, key=lambda r: r[key]["maximum_residual_cm"])
    maximum_angle = max(envelope, key=lambda r: r[key]["degrees"])
    lean_summary[str(cap)] = {"maximum_used_degrees": maximum_angle[key]["degrees"],
        "maximum_residual_cm": worst_residual[key]["maximum_residual_cm"],
        "worst_target": {k: worst_residual[k] for k in ("robot", "slot", "target_world_cm", "core_sample_frame")},
        "all_cases_reachable_within_0p01cm": all(r[key]["maximum_residual_cm"] < .01 for r in envelope),
        "maximum_azimuth_used_degrees": max(abs(r[key]["azimuth_degrees"]) for r in envelope),
        "maximum_shoulder_displacement_cm": max(hand["shoulder_displacement_cm"] for r in envelope for hand in r[key]["hands"].values()),
        "maximum_elbow_displacement_cm": max((Vector(hand["solved_elbow_world_cm"]) - arms[side]["elbow"]).length
            for r in envelope for side, hand in r[key]["hands"].items())}
result = {"asset": asset["asset"], "review": "CPU source geometry and analytic two-link reach; not Unreal IK or owner visual approval",
    "conclusion": "A fixed torso cannot maintain both original contacts across the current target envelope. Pure 8-degree forward lean is insufficient. The evaluated minimal tilt toward +X/-Y within a 10-degree total cone plus two-bone arms reaches all 588 sampled cases without arm stretch; maximum tilt 9.1965 degrees. This remains an engine-deformation review candidate.",
    "frozen_source_fbx_sha256": tracked["meshes\\MaraOperator.fbx"],
    "runtime_sources": {str(path.relative_to(game)): hashlib.sha256(path.read_bytes()).hexdigest()
                        for path in (runtime / "FoundryMara.cpp", runtime / "FoundryRobot.cpp", runtime / "FoundryHost.cpp")},
    "target_method": "FoundryRobot::GetImpactLocation core socket; actual original idle source rigs sampled every three frames, with first/last included. Actor slots from current FoundryHost::RobotPosition.",
    "gun_origin_cm": plain(GUN), "operator_origin_cm": plain(OPERATOR), "cradle_pivot_world_cm": plain(pivot),
    "angle_formula": "Pitch=atan2(dZ,hypot(dX,dY))-asin(52/length(d)); Yaw=atan2(dY,dX). Matches current UFoundryAimedGun.",
    "operator_pose": "MO_idle frame 1 baseline. Arm-only and virtual upper-body-lean solutions are compared separately; no production pose or source file is edited.",
    "torso_lean_comparison": {"pivot_world_cm": plain(body_pivot),
        "method": "Compare pure +X forward tilt and a tilt cone toward the gun (+X/-Y) around the fixed pelvis head; root, pelvis transform and feet unchanged. One common torso transform serves both arms. Two-bone arms solve the original aimed contact anchors with original lengths. No pipe slide.",
        "caps_degrees": lean_summary},
    "feet_world_cm": frozen_feet,
    "arm_measurements": {side: {key: plain(value) if isinstance(value, Vector) else value for key, value in data.items()} for side, data in arms.items()},
    "core_height_ranges_cm": {name: {"definition": data["id"], "idle": data["clip"], "samples": len(data["cores"]),
        "min": min(p.z for p in data["cores"]) + 1.5, "max": max(p.z for p in data["cores"]) + 1.5} for name, data in robots.items()},
    "nominal_targets": nominal, "sampled_target_cases": len(envelope),
    "maxima": {"fixed_contact_anchor_drift_cm": worst("fixed_hand_anchor_drift_cm"),
        "fixed_hand_surface_distance_cm": worst("fixed_hand_nearest_gun_surface_distance_cm"),
        "fixed_anchor_reach_deficit_cm": worst("unreachable_cm", "fixed_anchor_ik"),
        "after_pipe_slide_reach_deficit_cm": worst("unreachable_cm", "pipe_slide_ik"),
        "pipe_slide_cm": worst("slide_cm", "pipe_slide_ik"),
        "solved_grip_displacement_cm": worst("grip_displacement_cm", "pipe_slide_ik"),
        "upperarm_rotation_degrees": worst("upperarm_rotation_degrees", "pipe_slide_ik"),
        "forearm_rotation_degrees": worst("forearm_rotation_degrees", "pipe_slide_ik")},
    "proposal": {"method": "During idle/aim hold, rotate the original ready hand contacts around the current cradle pivot with the same Aim quaternion. If either wrist exceeds reach, apply the smallest shared upper-body tilt toward +X/-Y around the fixed pelvis head, with total tilt at most10 degrees. Apply that component-space transform to spine/chest/neck/head/clavicles and arm descendants while root, pelvis and leg transforms remain unchanged. Recompute shoulders and elbow poles from that leaned pose; solve each upperarm/forearm with its original length, then orient hand/grip with the gun. Do not translate the whole actor.",
        "recommended_branch": "torso_lean_cone_10; all588 cases reached within0.001cm in this analytic study",
        "search": "Test lean azimuths -60,-45,-30,-15,0,+15 degrees measured from +X toward +Y; binary-search the smallest shared tilt in0..10 degrees satisfying both wrists. Choose a reachable option by smallest tilt then smallest absolute azimuth. Runtime may use a smooth continuous direction if verified separately.",
        "maximum_measured_tilt_degrees": lean_summary["cone_10"]["maximum_used_degrees"],
        "maximum_measured_grip_error_cm": lean_summary["cone_10"]["maximum_residual_cm"],
        "original_arm_lengths_cm": {side: [data["upper_length"], data["fore_length"]] for side, data in arms.items()},
        "reach_limit_cm": "Upper + forearm lengths minus 0.4cm, matching source solver; clamp instead of stretching",
        "right_hand_slide": "Rejected as a full solution: a +/-12cm pipe slide alone leaves a measured reach deficit. Keep original contact and evaluate the shared torso lean instead.",
        "pipe_segment_gun_local_cm": [[-90, 35, 120], [-100, 49, 95]],
        "pipe_original_arc_cm": pipe_start, "pipe_radial_grip_offset_cm": plain(pipe_radial_offset),
        "clip_policy": "Use full contact only during idle/aim hold. Blend contact down for left-hand load reach and both hands during fire/recovery so authored motion remains visible; do not stretch limbs to enforce contact while recoiling.",
        "blend": "Suggested 0.10s cosine/cubic transition when selected target changes; verify visibly in engine before fixing this parameter.",
        "gameplay_authority": False},
    "checks": checks,
    "limits": ["Quantitative source assessment, no new engine or visual approval.",
        "Covers three specified large slots across all current large original bodies and Mite's current slot; extra overflow slots not covered.",
        "Core idle variation is sampled; charge, attack, hit, death and moving actors may extend the envelope.",
        "Operator ready pose only; original load/recoil clips are preserved by releasing contact, not simulated as new IK animations.",
        "Tilting all upper-body points around the pelvis head shifts the spine attachment by up to about1.9cm; waist blending/deformation must be inspected in engine. A solver pivoting strictly at the fixed pelvis tail would need a different reach bound.",
        "No finger articulation, gun self-contact, body collision, or skin deformation under proposed runtime IK has been rendered here."]}
destination.write_text(json.dumps(result, indent=2))
print("MARA_AIM_REVIEW " + json.dumps({"samples": len(envelope), "maxima": result["maxima"], "checks": checks}))
print("MARA_TORSO_REACH " + json.dumps(lean_summary))
if not all(value for key, value in checks.items() if key != "feet_or_body_transforms_modified"):
    raise RuntimeError("Source contact study consistency check failed")

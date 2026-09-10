"""Finish an original TRELLIS creature into a textured, sixteen-bone FBX.

Blender --background --threads 4 --python-exit-code 1 --python this.py --
  --asset Briarhide --source <private>/source-1536/textured.glb
  --prepared-source <private>/finished/SK_OE_Briarhide.blend
  --output <private>/finished-motion --preview

Preserves the full source GLB and its embedded PBR texture bytes. The runtime
copy has an explicit triangle budget and anatomical, smoothly blended weights.
Joint positions are editable in art/organic-enemies/rig/<asset>.json. Preview
renders use CPU and include deep bent knees, a step and flexed wrists; none is
proof of Unreal animation or game feel. --prepared-source preserves the exact
v1 topology, UVs, authored eyes and materials while replacing only the rig.
"""
import argparse
import hashlib
import heapq
import json
import math
from pathlib import Path
import sys

import bpy
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

ROOT = Path(__file__).resolve().parents[1]


def coordinates(mesh):
    result = np.empty(len(mesh.vertices) * 3, dtype=np.float32)
    mesh.vertices.foreach_get("co", result)
    return result.reshape(-1, 3)


def make_bones(spec):
    joint = {name: np.asarray(value, dtype=np.float64) for name, value in spec["joints"].items()}
    for name in list(joint):
        if name.endswith("_l"):
            joint[name[:-2] + "_r"] = joint[name] * np.array([-1, 1, 1])
    bones = [
        ("root", None, np.array([0, 0, 0]), np.array([0, 0, .18])),
        ("pelvis", "root", joint["pelvis"], joint["chest"]),
        ("spine", "pelvis", joint["chest"], joint["neck"]),
        ("head", "spine", joint["neck"], joint["head_top"]),
    ]
    for side in ("l", "r"):
        bones.extend([
            ("upperarm_" + side, "spine", joint["shoulder_" + side], joint["elbow_" + side]),
            ("forearm_" + side, "upperarm_" + side, joint["elbow_" + side], joint["hand_" + side]),
            ("thigh_" + side, "pelvis", joint["hip_" + side], joint["knee_" + side]),
            ("shin_" + side, "thigh_" + side, joint["knee_" + side], joint["ankle_" + side]),
        ])
    # Append the new joints: preserve both the names and rest transforms of
    # all twelve existing bones (including their original creation order).
    for side in ("l", "r"):
        bones.extend([
            ("hand_" + side, "forearm_" + side, joint["hand_" + side], joint["palm_" + side]),
            ("foot_" + side, "shin_" + side, joint["ankle_" + side], joint["toe_" + side]),
        ])
    return bones


def smooth_gate(value, half_width):
    blend = np.clip((value + half_width) / (2 * half_width), 0, 1)
    return blend * blend * (3 - 2 * blend)


def mesh_signature(mesh):
    """Fingerprint appearance-bearing mesh data before/after a rig-only edit."""
    digest = hashlib.sha256()
    for collection, field, count, dtype in (
            (mesh.vertices, "co", 3, np.float32),
            (mesh.loops, "vertex_index", 1, np.int32),
            (mesh.polygons, "material_index", 1, np.int32)):
        values = np.empty(len(collection) * count, dtype=dtype)
        collection.foreach_get(field, values)
        digest.update(values.tobytes())
    for layer in mesh.uv_layers:
        uv = np.empty(len(layer.data) * 2, dtype=np.float32)
        layer.data.foreach_get("uv", uv)
        digest.update(uv.tobytes())
    for color in mesh.color_attributes:
        if color.data_type in {"FLOAT_COLOR", "BYTE_COLOR"}:
            rgba = np.empty(len(color.data) * 4, dtype=np.float32)
            color.data.foreach_get("color", rgba)
            digest.update(rgba.tobytes())
    return digest.hexdigest()


def anatomical_regions(mesh, points, joint):
    """Keep a nearby finger from borrowing a thigh through empty space.

    Atlas seams are joined only in this temporary adjacency graph. The actual
    mesh and UV topology remain byte-identical. High-confidence body/limb seeds
    spread over the surface; unseeded detached moss retains proximity weighting.
    """
    unique, representative, inverse = np.unique(np.round(points, 5), axis=0,
                                                 return_index=True, return_inverse=True)
    seed = np.full(len(points), -1, dtype=np.int8)
    seed[(np.abs(points[:, 0]) < joint["hip_l"][0] * .65) &
         (points[:, 2] > joint["pelvis"][2] - .02)] = 0
    for side, sign, arm_label, leg_label in (("l", 1, 1, 3), ("r", -1, 2, 4)):
        side_x = sign * points[:, 0]
        elbow, hand, shoulder = (joint[key + "_" + side] for key in ("elbow", "hand", "shoulder"))
        arm = ((side_x > abs(shoulder[0]) + .08) & (points[:, 2] > elbow[2] + .05))
        arm |= ((side_x > (abs(elbow[0]) + abs(hand[0])) * .5 + .02) &
                (points[:, 2] < elbow[2]) & (points[:, 2] > joint["palm_" + side][2] - .06))
        seed[arm] = arm_label
        leg = ((points[:, 2] < joint["knee_" + side][2] + .06) &
               (side_x > .07) & (side_x < abs(joint["hip_" + side][0]) + .26))
        seed[leg] = leg_label
    labels = seed[representative].copy()
    edge_vertices = np.empty(len(mesh.edges) * 2, dtype=np.int32)
    mesh.edges.foreach_get("vertices", edge_vertices)
    edges = inverse[edge_vertices].reshape(-1, 2)
    edges.sort(axis=1)
    edges = np.unique(edges[edges[:, 0] != edges[:, 1]], axis=0)
    lengths = np.linalg.norm(unique[edges[:, 0]] - unique[edges[:, 1]], axis=1)
    adjacent = [[] for _ in unique]
    for (a, b), length in zip(edges, lengths):
        adjacent[a].append((int(b), float(length)))
        adjacent[b].append((int(a), float(length)))
    distance = np.full(len(unique), np.inf)
    distance[labels >= 0] = 0
    queue = [(0., int(index)) for index in np.flatnonzero(labels >= 0)]
    heapq.heapify(queue)
    while queue:
        at_distance, at = heapq.heappop(queue)
        if at_distance > distance[at]:
            continue
        for neighbor, length in adjacent[at]:
            candidate = at_distance + length
            if candidate < distance[neighbor]:
                distance[neighbor] = candidate
                labels[neighbor] = labels[at]
                heapq.heappush(queue, (candidate, neighbor))
    return labels[inverse]


def skin_mesh(obj, spec, bones):
    mesh = obj.data
    points = coordinates(mesh).astype(np.float64)
    deform = bones[1:]
    scores, distances = [], []
    height = spec["height_metres"]
    blend = spec["joint_blend_metres"]
    joint = {name: np.asarray(value) for name, value in spec["joints"].items()}
    for name in list(joint):
        if name.endswith("_l"):
            joint[name[:-2] + "_r"] = joint[name] * np.array([-1, 1, 1])
    regions = anatomical_regions(mesh, points, joint)
    gates = {}
    torso_gate = np.ones(len(points))
    for side, sign in (("l", 1), ("r", -1)):
        def along(at, previous):
            direction = joint[at + "_" + side] - joint[previous + "_" + side]
            direction /= np.linalg.norm(direction)
            return (points - joint[at + "_" + side]) @ direction

        shoulder = joint["shoulder_" + side]
        upper_direction = joint["elbow_" + side] - shoulder
        upper_direction /= np.linalg.norm(upper_direction)
        shoulder_gate = smooth_gate((points - shoulder) @ upper_direction, blend["shoulder"])
        elbow_gate = smooth_gate(along("elbow", "shoulder"), blend["elbow"])
        # The preserved v1 wrist landmark is inside the palm mass. Place the
        # weight transition at the observed narrow wrist, without moving the
        # required bone pivot or letting inward-spreading fingers follow a leg.
        wrist_gate = smooth_gate(spec["wrist_transition_z"] - points[:, 2], blend["wrist"])
        hip = joint["hip_" + side]
        thigh_direction = joint["knee_" + side] - hip
        thigh_direction /= np.linalg.norm(thigh_direction)
        hip_gate = smooth_gate((points - hip) @ thigh_direction, blend["hip"])
        knee_gate = smooth_gate(joint["knee_" + side][2] - points[:, 2], blend["knee"])
        # A vertical ankle split keeps forward-spreading toes on the foot.
        # Extending the slanted shin's capsule dragged those toes during bends.
        foot_gate = smooth_gate(spec["ankle_transition_z"] - points[:, 2], blend["ankle"])
        side_gate = smooth_gate(sign * points[:, 0] - .018, .04)
        gates["upperarm_" + side] = shoulder_gate * (1 - elbow_gate) * side_gate
        gates["forearm_" + side] = shoulder_gate * elbow_gate * (1 - wrist_gate) * side_gate
        gates["hand_" + side] = shoulder_gate * wrist_gate * side_gate
        gates["thigh_" + side] = hip_gate * (1 - knee_gate) * side_gate
        gates["shin_" + side] = hip_gate * knee_gate * (1 - foot_gate) * side_gate
        gates["foot_" + side] = hip_gate * knee_gate * foot_gate * side_gate
        outer_leg = smooth_gate(sign * points[:, 0] - blend["crotch_half_width"], .055)
        outer_arm = smooth_gate(sign * points[:, 0] - abs(shoulder[0]) + .025, .09)
        torso_gate *= (1 - hip_gate * outer_leg) * (1 - shoulder_gate * outer_arm)
    # Capsule distance is stable on detached moss, antler and claw details.
    # Smooth gates keep opposite limbs and the head from borrowing weights.
    envelopes = {}
    for name, _, start, end in deform:
        vector = end - start
        progress = np.clip(((points - start) @ vector) / (vector @ vector), 0, 1)
        distance = np.linalg.norm(points - (start + progress[:, None] * vector), axis=1)
        radius = spec["bone_radii"][name]
        score = np.exp(-np.minimum(distance / radius, 12) ** 2 * 2.4)
        envelopes[name] = score.copy()
        if name in gates:
            score *= gates[name]
        if name in ("pelvis", "spine"):
            score *= torso_gate
        if name.startswith(("thigh", "shin", "foot")):
            score *= np.clip((height * .52 - points[:, 2]) / .16, 0, 1)
        if name == "head":
            score *= np.clip((points[:, 2] - height * .65) / .15, 0, 1)
        else:
            # High antlers belong to the head even far outside its capsule.
            score *= 1 - np.clip((points[:, 2] - height * .80) / .10, 0, 1)
        scores.append(score)
        # Preserve distant antler, toe and claw islands with their nearest
        # anatomically eligible bone if the Gaussian underflows.
        eligible = gates.get(name, np.ones(len(points))) > .05
        if name == "head":
            eligible &= points[:, 2] > height * .65
        distances.append(np.where(eligible, distance / radius, 1e6))
    weights = np.stack(scores, axis=1)
    names = [name for name, *_ in deform]
    for side in ("l", "r"):
        for parts, forbidden in ((("upperarm", "forearm", "hand"), np.isin(regions, (3, 4))),
                                 (("thigh", "shin", "foot"), np.isin(regions, (1, 2)))):
            chain = [part + "_" + side for part in parts]
            envelope = np.maximum.reduce([envelopes[name] for name in chain])
            for name in chain:
                index = names.index(name)
                # One shared envelope avoids multiplying a smooth joint ramp
                # by a second, sharply changing capsule-distance ratio.
                weights[:, index] = envelope * gates[name]
                weights[forbidden, index] = 0
    missing = weights.sum(axis=1) < 1e-15
    if missing.any():
        nearest = np.argmin(np.stack(distances, axis=1), axis=1)
        weights[missing] = 0
        weights[np.flatnonzero(missing), nearest[missing]] = 1
    eye_indices = {index for polygon in mesh.polygons
                   if mesh.materials[polygon.material_index].name.startswith("M_OE_Eye")
                   for index in polygon.vertices}
    if eye_indices:
        eyes = np.asarray(sorted(eye_indices), dtype=np.int32)
        weights[eyes] = 0
        weights[eyes, [name for name, *_ in deform].index("head")] = 1
    keep = np.argsort(weights, axis=1)[:, -4:]
    mask = np.zeros(weights.shape, dtype=bool)
    np.put_along_axis(mask, keep, True, axis=1)
    weights[~mask] = 0
    weights /= np.maximum(weights.sum(axis=1)[:, None], 1e-20)
    # VertexGroup.add accepts one weight for a group of indices. Quantizing to
    # 1/1024 keeps this large mesh operation bounded; renormalize in Blender.
    quantized = np.rint(weights * 1024).astype(np.int16)
    for index, (name, *_rest) in enumerate(deform):
        group = obj.vertex_groups.new(name=name)
        values = quantized[:, index]
        for amount in np.unique(values[values > 0]):
            group.add(np.flatnonzero(values == amount).tolist(), float(amount) / 1024, "REPLACE")
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode="WEIGHT_PAINT")
    bpy.ops.object.vertex_group_normalize_all(lock_active=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    emission = spec["throat_emission"]
    delta = (points - np.asarray(emission["center"])) / np.asarray(emission["radius"])
    glow = np.clip(1 - np.sum(delta ** 2, axis=1), 0, 1) ** 2
    if "OrganicTell" not in mesh.color_attributes:
        color = mesh.color_attributes.new(name="OrganicTell", type="FLOAT_COLOR", domain="POINT")
        rgba = np.zeros((len(points), 4), dtype=np.float32)
        rgba[:, 0] = glow
        rgba[:, 3] = 1
        color.data.foreach_set("color", rgba.reshape(-1))
        mesh.color_attributes.active_color = color
    return {"vertices": len(points), "max_influences": int((quantized > 0).sum(axis=1).max()),
            "method": "Capsule proximity with anatomy-specific hip, knee, ankle, shoulder, elbow and wrist transition gates; four normalized influences; eye material vertices rigid to head",
            "rigid_eye_vertices": len(eye_indices), "fallback_vertices": int(missing.sum()),
            "surface_region_vertices": {str(label): int((regions == label).sum()) for label in (-1, 0, 1, 2, 3, 4)},
            "vertices_per_bone": {bone[0]: int((quantized[:, i] > 0).sum()) for i, bone in enumerate(deform)},
            "throat_mask_vertices": int((glow > .01).sum())}


def build_armature(obj, bones):
    data = bpy.data.armatures.new("OrganicSkeleton")
    # Unreal's FBX importer recognizes and removes Blender's wrapper object.
    armature = bpy.data.objects.new("Armature", data)
    bpy.context.collection.objects.link(armature)
    bpy.ops.object.select_all(action="DESELECT")
    armature.select_set(True)
    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.mode_set(mode="EDIT")
    for name, parent, head, tail in bones:
        bone = data.edit_bones.new(name)
        bone.head, bone.tail = head.tolist(), tail.tolist()
        if parent:
            bone.parent = data.edit_bones[parent]
        bone.use_connect = False
        bone.use_deform = True
    bpy.ops.object.mode_set(mode="OBJECT")
    obj.parent = armature
    deform = obj.modifiers.new("OrganicSkin", "ARMATURE")
    deform.object = armature
    # Match Unreal's default linear blend skinning in the source previews.
    deform.use_deform_preserve_volume = False
    return armature


def load_prepared_source(path, asset):
    """Reopen the reviewed v1 rather than decimate or regenerate it again."""
    bpy.ops.wm.open_mainfile(filepath=str(path))
    obj = bpy.data.objects.get("SK_OE_" + asset)
    if not obj or obj.type != "MESH":
        raise RuntimeError("Prepared source does not contain the requested enemy mesh")
    armature = obj.find_armature()
    if not armature or len(armature.data.bones) != 12:
        raise RuntimeError("Rig upgrade expects the preserved twelve-bone v1")
    reference = {bone.name: {"parent": bone.parent.name if bone.parent else None,
                            "matrix": np.asarray(bone.matrix_local).copy()}
                 for bone in armature.data.bones}
    signature = mesh_signature(obj.data)
    world = obj.matrix_world.copy()
    obj.parent = None
    obj.matrix_world = world
    for modifier in list(obj.modifiers):
        if modifier.type == "ARMATURE":
            obj.modifiers.remove(modifier)
    bpy.data.objects.remove(armature, do_unlink=True)
    for group in list(obj.vertex_groups):
        obj.vertex_groups.remove(group)
    return obj, reference, signature


def texture_files(obj, output, expected=None):
    material = obj.data.materials[0]
    images = list({node.image for node in material.node_tree.nodes if node.type == "TEX_IMAGE" and node.image})
    base = next((im for im in images if im.colorspace_settings.name == "sRGB"), None)
    packed = next((im for im in images if im != base), None)
    if len(images) != 2 or base is None or packed is None:
        raise RuntimeError("Expected original base-color and packed metallic-roughness pair")
    textures = []
    for image, filename in ((base, "base_color.png"), (packed, "metallic_roughness.png")):
        if not image.packed_file:
            image.pack()
        data = bytes(image.packed_file.data)
        digest = hashlib.sha256(data).hexdigest()
        if expected and digest != next(t["sha256"] for t in expected if t["file"] == filename):
            raise RuntimeError("Prepared atlas bytes changed: " + filename)
        (output / filename).write_bytes(data)
        image.filepath = "//" + filename
        textures.append({"file": filename, "size": list(image.size), "sha256": digest,
                         "processing": "Unchanged embedded PNG bytes"})
    return textures


def export_rig(obj, armature, spec, spec_path, source, output, full_triangles,
               weights, textures, eyes, preview, preservation=None):
    obj.data.calc_loop_triangles()
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    armature.select_set(True)
    bpy.context.view_layer.objects.active = armature
    fbx = output / (obj.name + ".fbx")
    bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=True, object_types={"MESH", "ARMATURE"},
        axis_forward="-Y", axis_up="Z", apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
        bake_anim=False, use_mesh_modifiers=True, mesh_smooth_type="FACE", add_leaf_bones=False,
        use_armature_deform_only=True, path_mode="STRIP", colors_type="LINEAR")
    bpy.ops.wm.save_as_mainfile(filepath=str(output / (obj.name + ".blend")))
    bounds = coordinates(obj.data)
    report = {"asset": spec["asset"], "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
              "source": str(source), "fbx": fbx.name, "fbx_sha256": hashlib.sha256(fbx.read_bytes()).hexdigest(),
              "source_triangles": full_triangles, "runtime_triangles": len(obj.data.loop_triangles),
              "dimensions_metres": (bounds.max(axis=0) - bounds.min(axis=0)).tolist(),
              "height_metres": spec["height_metres"], "runtime_mesh_yaw": spec["runtime_mesh_yaw"],
              "rig_spec_sha256": hashlib.sha256(spec_path.read_bytes()).hexdigest(), "weights": weights,
              "bones": [{"name": b.name, "parent": b.parent.name if b.parent else None,
                         "head": list(b.head_local), "tail": list(b.tail_local),
                         "matrix_local": [list(row) for row in b.matrix_local]}
                        for b in armature.data.bones],
              "unit_contract": {"coordinates": "Blender metres; front -Y, left +X, up +Z",
                                "bone_axis": "Local +Y runs from head to tail; full rest matrices supplied",
                                "scene_scale_length": 1.0, "fbx_axis_forward": "-Y", "fbx_axis_up": "Z",
                                "fbx_apply_unit_scale": True, "fbx_scale_options": "FBX_SCALE_UNITS",
                                "runtime_requirement": "Query imported component/bone transforms. Prior Unreal import retained a 100x parent transform. Convert points with full TransformPosition/InverseTransformPosition, never rotation alone; avoid double cm conversion."},
              "preservation": preservation, "textures": textures, "preview_renders": [],
              "eye_placements": eyes, "material_slots": [mat.name for mat in obj.data.materials],
              "near_black_pigment_floor": spec.get("near_black_pigment_floor"),
              "limits": "Linear blend skinning with whole hand and foot articulation; no individual fingers/toes, corrective shapes, facial rig, cloth or ragdoll. Static CPU stress poses do not establish runtime foot placement, animation quality or game feel. Unreal motion inspection required."}
    report_path = output / "prep-report.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if preview:
        report["preview_renders"] = preview_rig(obj, armature, output, spec["height_metres"])
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("ORGANIC_ENEMY_PREPARED " + json.dumps({key: report[key] for key in (
        "asset", "fbx", "runtime_triangles", "dimensions_metres", "preview_renders")}), flush=True)


def finish_eyes(obj, spec):
    """Small moist amber irises, seated against the actual generated sockets.

    New geometry uses solid PBR materials and rigid head weights. It does not
    repaint the preserved source atlas or use emission to fake a living eye.
    """
    eye = spec.get("eyes")
    if not eye:
        return []
    tree = BVHTree.FromObject(obj, bpy.context.evaluated_depsgraph_get())
    materials = {}
    for label, color, roughness in (("Iris", (.24, .092, .012, 1), .20), ("Pupil", (.003, .005, .002, 1), .12)):
        mat = bpy.data.materials.new("M_OE_Eye" + label)
        mat.use_nodes = True
        bsdf = mat.node_tree.nodes.get("Principled BSDF")
        bsdf.inputs["Base Color"].default_value = color
        bsdf.inputs["Roughness"].default_value = roughness
        bsdf.inputs["Specular IOR Level"].default_value = .7
        materials[label] = mat
    additions, placements = [], []
    for side in (-1, 1):
        origin = Vector((side * eye["x"], -spec["height_metres"], eye["z"]))
        position, normal, _, _ = tree.ray_cast(origin, Vector((0, 1, 0)), spec["height_metres"] * 2)
        if position is None:
            raise RuntimeError("Authored eye ray misses the generated face; inspect eye landmarks")
        # Mostly forward, with a little of the sculpted socket's outward angle.
        normal = (normal * .3 + Vector((side * .17, -.98, .03))).normalized()
        for label, width, height, depth, front in (
                ("Iris", eye["radius_x"], eye["radius_z"], eye["depth"], eye["depth"] * .32),
                ("Pupil", eye["radius_x"] * .33, eye["radius_z"] * .68, eye["depth"] * .31, eye["depth"] * 1.07)):
            bpy.ops.mesh.primitive_uv_sphere_add(segments=20, ring_count=12, radius=1,
                                                location=position + normal * front)
            part = bpy.context.object
            part.name = "OrganicEye_" + label + ("_L" if side > 0 else "_R")
            part.rotation_mode = "QUATERNION"
            part.rotation_quaternion = normal.to_track_quat("Z", "Y")
            part.scale = (width, height, depth)
            bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
            for polygon in part.data.polygons:
                polygon.use_smooth = True
            part.data.materials.append(materials[label])
            part.vertex_groups.new(name="head").add(list(range(len(part.data.vertices))), 1.0, "REPLACE")
            additions.append(part)
        placements.append({"side": side, "surface_position": list(position), "outward_normal": list(normal)})
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    for part in additions:
        part.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.join()
    return placements


def preview_rig(obj, armature, output, height, only_front=False, views=None):
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 12
    scene.cycles.use_denoising = True
    scene.render.resolution_x, scene.render.resolution_y = 900, 1050
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.world = bpy.data.worlds.new("PreviewWorld")
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs["Color"].default_value = (.055, .065, .075, 1)
    scene.world.node_tree.nodes["Background"].inputs["Strength"].default_value = .6
    scene.view_settings.view_transform = "AgX"
    camera_data = bpy.data.cameras.new("PreviewCamera")
    camera = bpy.data.objects.new("PreviewCamera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera_data.type = "ORTHO"
    camera_data.ortho_scale = height * 1.20
    for name, location, energy, size, color in (
            ("Key", (-3, -4, 5), 850, 4.0, (1.0, .90, .76)),
            ("Fill", (3, -2, 2.5), 450, 3.0, (.69, .85, 1.0)),
            ("Rim", (1, 3, 4), 950, 3.0, (.78, 1.0, .85))):
        light_data = bpy.data.lights.new(name, "AREA")
        light_data.energy, light_data.shape, light_data.size = energy, "DISK", size
        light_data.color = color
        light = bpy.data.objects.new(name, light_data)
        scene.collection.objects.link(light)
        light.location = location
        light.rotation_euler = (Vector((0, 0, height * .5)) - light.location).to_track_quat("-Z", "Y").to_euler()
    reference = {bone.name: bone.matrix_local.copy() for bone in armature.data.bones}
    original_location = armature.location.copy()
    base_points = coordinates(obj.data)
    edge_indices = np.empty(len(obj.data.edges) * 2, dtype=np.int32)
    obj.data.edges.foreach_get("vertices", edge_indices)
    edge_indices = edge_indices.reshape(-1, 2)
    base_lengths = np.linalg.norm(base_points[edge_indices[:, 0]] - base_points[edge_indices[:, 1]], axis=1)
    useful_edges = base_lengths > .001

    def pose(changes):
        armature.location = original_location
        for bone in armature.pose.bones:
            bone.matrix_basis = Matrix.Identity(4)
        # Rotate around anatomical mesh axes, conjugated into each bone's basis.
        for name, (axis, angle) in changes.items():
            rotation = Matrix.Rotation(math.radians(angle), 4, axis)
            rest = reference[name].to_3x3().to_4x4()
            armature.pose.bones[name].matrix_basis = rest.inverted() @ rotation @ rest
        bpy.context.view_layer.update()

    stance = {"thigh_l": ("X", -37), "shin_l": ("X", 78), "foot_l": ("X", -41),
              "thigh_r": ("X", -37), "shin_r": ("X", 78), "foot_r": ("X", -41),
              "spine": ("X", 14), "forearm_l": ("X", -25), "forearm_r": ("X", -25),
              "hand_l": ("Y", 40), "hand_r": ("Y", -40)}
    stepping = {"thigh_l": ("X", -28), "shin_l": ("X", 52), "foot_l": ("X", -24),
                "thigh_r": ("X", 12), "shin_r": ("X", 8), "foot_r": ("X", -20),
                "upperarm_l": ("X", 16), "upperarm_r": ("X", -18),
                "forearm_l": ("X", -20), "hand_l": ("Y", 28), "hand_r": ("Y", -25)}
    cast = {"spine": ("Z", -18), "upperarm_r": ("X", -50), "forearm_r": ("X", -50),
            "hand_r": ("Y", -40), "upperarm_l": ("X", -35), "forearm_l": ("X", -65),
            "hand_l": ("Y", 40), "head": ("Z", 12),
            "thigh_l": ("X", -15), "shin_l": ("X", 30), "foot_l": ("X", -15),
            "thigh_r": ("X", -15), "shin_r": ("X", 30), "foot_r": ("X", -15)}
    bpy.ops.mesh.primitive_plane_add(size=200, location=(0, 0, -.008))
    floor = bpy.context.object
    floor.name = "PreviewGround_NotExported"
    mat = bpy.data.materials.new("PreviewGround")
    mat.diffuse_color = (.038, .047, .051, 1)
    floor.data.materials.append(mat)
    pictures, measurements = [], []
    for name, location, changes, support, close_bone, frame in (
            ("bind-front", (0, -6, height * .56), {}, "both", None, height * 1.20),
            ("deep-stance", (3.1, -5.5, height * .53), stance, "both", None, height * 1.20),
            ("deep-stance-side", (6, -.15, height * .50), stance, "both", None, height * 1.20),
            ("step", (3.1, -5.5, height * .56), stepping, "r", None, height * 1.20),
            ("cast", (2.8, -5.5, height * .60), cast, "both", None, height * 1.50),
            ("wrist-flex-detail", (2.2, -4, .8), cast, "both", "hand_l", .88),
            ("knee-ankle-detail", (4, -2, .35), stance, "both", "shin_l", 1.15)):
        if only_front and name != "bind-front":
            continue
        if views and name not in views:
            continue
        pose(changes)
        evaluated = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
        posed_points = coordinates(evaluated.data)
        support_vertices = base_points[:, 2] < .10
        if support == "r":
            support_vertices &= base_points[:, 0] < -.12
        armature.location.z -= float(posed_points[support_vertices, 2].min())
        bpy.context.view_layer.update()
        target = Vector((0, 0, height * .49 + armature.location.z * .4))
        camera_data.ortho_scale = frame
        if close_bone:
            bone = armature.pose.bones[close_bone]
            target = armature.matrix_world @ (bone.head.lerp(bone.tail, .5))
            camera.location = target + Vector(location)
        else:
            camera.location = location
        camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
        scene.render.filepath = str(output / (name + ".png"))
        bpy.ops.render.render(write_still=True)
        pictures.append(name + ".png")
        lengths = np.linalg.norm(posed_points[edge_indices[:, 0]] - posed_points[edge_indices[:, 1]], axis=1)
        stretch = lengths[useful_edges] / base_lengths[useful_edges]
        measurements.append({"image": name + ".png", "mesh_axis_joint_degrees": changes,
                             "preview_only_ground_translation_metres": float(armature.location.z),
                             "edge_stretch_percentile_99": float(np.percentile(stretch, 99)),
                             "edge_stretch_percentile_99_9": float(np.percentile(stretch, 99.9)),
                             "edges_over_2x": int((stretch > 2).sum()),
                             "scope": "Static linear-blend pose. Ground translation seats support mesh only; no runtime IK or animation proven."})
    pose({})
    measurements_path = output / "preview-poses.json"
    if views and measurements_path.is_file():
        updated = {item["image"]: item for item in measurements}
        measurements = [updated.get(item["image"], item)
                        for item in json.loads(measurements_path.read_text(encoding="utf-8"))]
    measurements_path.write_text(json.dumps(measurements, indent=2) + "\n", encoding="utf-8")
    return pictures


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--asset", required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--prepared-source", type=Path,
                        help="Preserved v1 .blend to re-rig without changing mesh, UVs, eyes or materials")
    parser.add_argument("--yaw-degrees", type=float, default=0)
    parser.add_argument("--preview", action="store_true")
    parser.add_argument("--inspect-source", action="store_true",
                        help="Render full source geometry before decimation, then exit without exporting")
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
    source = args.source.resolve()
    if source.suffix.lower() != ".glb" or not source.is_file():
        raise RuntimeError("A completed original textured.glb is required")
    spec_path = ROOT / "art/organic-enemies/rig" / (args.asset + ".json")
    spec = json.loads(spec_path.read_text(encoding="utf-8"))
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    if args.prepared_source:
        prepared = args.prepared_source.resolve()
        if output == prepared.parent:
            raise RuntimeError("Use a new output folder; do not overwrite the preserved v1")
        previous = json.loads((prepared.parent / "prep-report.json").read_text(encoding="utf-8"))
        if previous["source_sha256"] != hashlib.sha256(source.read_bytes()).hexdigest():
            raise RuntimeError("Original GLB does not match the prepared v1 provenance")
        obj, old_reference, signature = load_prepared_source(prepared, args.asset)
        bones = make_bones(spec)
        weights = skin_mesh(obj, spec, bones)
        armature = build_armature(obj, bones)
        differences = {}
        for name, old in old_reference.items():
            new = armature.data.bones[name]
            error = float(np.abs(np.asarray(new.matrix_local) - old["matrix"]).max())
            differences[name] = error
            if (new.parent.name if new.parent else None) != old["parent"] or error > 1e-6:
                raise RuntimeError("Existing bone rest transform changed: " + name)
        if mesh_signature(obj.data) != signature:
            raise RuntimeError("Rig upgrade changed topology, positions, UVs, materials or vertex color")
        textures = texture_files(obj, output, expected=previous["textures"])
        preservation = {"prepared_source": str(prepared),
                        "prepared_source_sha256": hashlib.sha256(prepared.read_bytes()).hexdigest(),
                        "mesh_and_uv_color_signature_sha256": signature,
                        "original_twelve_rest_matrix_max_error": differences,
                        "unchanged": ["mesh positions and topology", "UVs", "material slots", "vertex colors",
                                      "embedded atlas bytes", "eye geometry/materials", "pigment floor"]}
        export_rig(obj, armature, spec, spec_path, source, output, previous["source_triangles"],
                   weights, textures, previous.get("eye_placements", []), args.preview, preservation)
        return
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.import_scene.gltf(filepath=str(source))
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if not meshes:
        raise RuntimeError("Source contains no mesh")
    bpy.ops.object.select_all(action="DESELECT")
    for obj in meshes:
        world = obj.matrix_world.copy()
        obj.parent = None
        obj.matrix_world = world
        obj.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.convert(target="MESH")
    bpy.ops.object.join()
    obj = bpy.context.object
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    if args.yaw_degrees:
        obj.data.transform(Matrix.Rotation(math.radians(args.yaw_degrees), 4, "Z"))
    obj.name = "SK_OE_" + args.asset
    obj.data.name = obj.name + "_Mesh"
    if len(obj.data.materials) != 1 or not obj.data.uv_layers:
        raise RuntimeError("Expected one original atlas material and UVs")
    initial = coordinates(obj.data)
    lower, upper = initial.min(axis=0), initial.max(axis=0)
    scale = spec["height_metres"] / float(upper[2] - lower[2])
    transform = Matrix.Scale(scale, 4) @ Matrix.Translation(Vector((
        -float((lower[0] + upper[0]) * .5), -float((lower[1] + upper[1]) * .5), -float(lower[2]))))
    obj.data.transform(transform)
    obj.data.calc_loop_triangles()
    full_triangles = len(obj.data.loop_triangles)
    if args.inspect_source:
        armature = build_armature(obj, make_bones(spec))
        preview_rig(obj, armature, output, spec["height_metres"], only_front=True)
        mat = obj.data.materials[0]
        nodes = mat.node_tree.nodes
        texture = next(n for n in nodes if n.type == "TEX_IMAGE" and n.image
                       and n.image.colorspace_settings.name == "sRGB")
        emission = nodes.new("ShaderNodeEmission")
        mat.node_tree.links.new(texture.outputs["Color"], emission.inputs["Color"])
        mat.node_tree.links.new(emission.outputs[0], next(n for n in nodes if n.type == "OUTPUT_MATERIAL").inputs["Surface"])
        flat = output / "unlit"
        flat.mkdir(exist_ok=True)
        preview_rig(obj, armature, flat, spec["height_metres"], only_front=True)
        print("ORGANIC_FULL_SOURCE_RENDERED " + str(output / "bind-front.png"), flush=True)
        return
    if full_triangles > spec["max_triangles"]:
        reduce = obj.modifiers.new("ExplicitRuntimeBudget", "DECIMATE")
        reduce.ratio = (spec["max_triangles"] - (2200 if spec.get("eyes") else 0)) / full_triangles
        reduce.use_collapse_triangulate = True
        bpy.ops.object.modifier_apply(modifier=reduce.name)
    obj.data.calc_loop_triangles()
    for polygon in obj.data.polygons:
        polygon.use_smooth = True
    material = obj.data.materials[0]
    material.name = "M_OE_" + args.asset
    textures = texture_files(obj, output)
    pigment_floor = spec.get("near_black_pigment_floor")
    if pigment_floor:
        shader = next(n for n in material.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
        original = shader.inputs["Base Color"].links[0].from_socket
        lift = material.node_tree.nodes.new("ShaderNodeVectorMath")
        lift.operation = "MAXIMUM"
        lift.inputs[1].default_value = pigment_floor
        material.node_tree.links.new(original, lift.inputs[0])
        material.node_tree.links.new(lift.outputs["Vector"], shader.inputs["Base Color"])
    bones = make_bones(spec)
    weights = skin_mesh(obj, spec, bones)
    eye_placements = finish_eyes(obj, spec)
    armature = build_armature(obj, bones)
    export_rig(obj, armature, spec, spec_path, source, output, full_triangles,
               weights, textures, eye_placements, args.preview)


if __name__ == "__main__":
    main()

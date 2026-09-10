"""Finish an original TRELLIS creature into a textured, twelve-bone FBX.

Blender --background --threads 4 --python-exit-code 1 --python this.py --
  --asset Briarhide --source <private>/source-1536/textured.glb
  --output <private>/finished --preview

Preserves the full source GLB and its embedded PBR texture bytes. The runtime
copy has an explicit triangle budget and anatomical, smoothly blended weights.
Joint positions are editable in art/organic-enemies/rig/<asset>.json. Preview
renders include the bind pose, bent knees and an attack pose; none is proof of
Unreal animation or game feel. Run preview after GPU generation has completed.
"""
import argparse
import hashlib
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
    return bones


def skin_mesh(obj, spec, bones):
    mesh = obj.data
    points = coordinates(mesh).astype(np.float64)
    deform = bones[1:]
    scores = []
    height = spec["height_metres"]
    # Capsule distance is stable on detached moss, antler and claw details.
    # Smooth gates keep opposite limbs and the head from borrowing weights.
    for name, _, start, end in deform:
        vector = end - start
        progress = np.clip(((points - start) @ vector) / (vector @ vector), 0, 1)
        distance = np.linalg.norm(points - (start + progress[:, None] * vector), axis=1)
        radius = spec["bone_radii"][name]
        score = np.exp(-np.minimum(distance / radius, 12) ** 2 * 2.4)
        if name.endswith(("_l", "_r")):
            sign = 1 if name.endswith("_l") else -1
            score *= np.clip((sign * points[:, 0] + .015) / .08, 0, 1)
        if name.startswith(("thigh", "shin")):
            score *= np.clip((height * .52 - points[:, 2]) / .16, 0, 1)
        if name == "head":
            score *= np.clip((points[:, 2] - height * .65) / .15, 0, 1)
        else:
            # High antlers belong to the head even far outside its capsule.
            score *= 1 - np.clip((points[:, 2] - height * .80) / .10, 0, 1)
        scores.append(score)
    weights = np.stack(scores, axis=1)
    missing = weights.sum(axis=1) < 1e-15
    if missing.any():
        weights[missing, [name for name, *_ in deform].index("head")] = 1
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
    for attribute in list(mesh.color_attributes):
        mesh.color_attributes.remove(attribute)
    color = mesh.color_attributes.new(name="OrganicTell", type="FLOAT_COLOR", domain="POINT")
    rgba = np.zeros((len(points), 4), dtype=np.float32)
    rgba[:, 0] = glow
    rgba[:, 3] = 1
    color.data.foreach_set("color", rgba.reshape(-1))
    mesh.color_attributes.active_color = color
    return {"vertices": len(points), "max_influences": int((quantized > 0).sum(axis=1).max()),
            "method": "Anatomical capsule weights, smooth region gates, top four influences normalized",
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


def preview_rig(obj, armature, output, height, only_front=False):
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
    reference = {bone.name: bone.matrix.copy() for bone in armature.data.bones}

    def pose(changes):
        for bone in armature.pose.bones:
            bone.matrix_basis = Matrix.Identity(4)
        # Rotate around anatomical mesh axes, conjugated into each bone's basis.
        for name, (axis, angle) in changes.items():
            rotation = Matrix.Rotation(math.radians(angle), 4, axis)
            rest = reference[name].to_3x3().to_4x4()
            armature.pose.bones[name].matrix_basis = rest.inverted() @ rotation @ rest
        bpy.context.view_layer.update()

    pictures = []
    for name, location, changes in (
            ("bind-front", (0, -6, height * .56), {}),
            ("bind-side", (6, -.15, height * .56), {}),
            ("stride", (3.1, -5.5, height * .56), {
                "thigh_l": ("X", -22), "shin_l": ("X", 34),
                "thigh_r": ("X", 18), "shin_r": ("X", 5),
                "upperarm_l": ("X", 16), "upperarm_r": ("X", -18)}),
            ("attack", (2.8, -5.5, height * .60), {
                "spine": ("Z", -18), "upperarm_r": ("X", 90),
                "forearm_r": ("X", -38), "upperarm_l": ("X", -28),
                "forearm_l": ("X", -22), "head": ("Z", 12)})):
        if only_front and name != "bind-front":
            continue
        pose(changes)
        camera.location = location
        camera.rotation_euler = (Vector((0, 0, height * .49)) - camera.location).to_track_quat("-Z", "Y").to_euler()
        scene.render.filepath = str(output / (name + ".png"))
        bpy.ops.render.render(write_still=True)
        pictures.append(name + ".png")
    pose({})
    return pictures


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--asset", required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
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
        (output / filename).write_bytes(data)
        image.filepath = "//" + filename
        textures.append({"file": filename, "size": list(image.size),
                         "sha256": hashlib.sha256(data).hexdigest(), "processing": "Unchanged embedded PNG bytes"})
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
    obj.data.calc_loop_triangles()
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
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
    report = {"asset": args.asset, "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
              "source": str(source), "fbx": fbx.name, "fbx_sha256": hashlib.sha256(fbx.read_bytes()).hexdigest(),
              "source_triangles": full_triangles, "runtime_triangles": len(obj.data.loop_triangles),
              "dimensions_metres": (bounds.max(axis=0) - bounds.min(axis=0)).tolist(),
              "height_metres": spec["height_metres"], "runtime_mesh_yaw": spec["runtime_mesh_yaw"],
              "rig_spec_sha256": hashlib.sha256(spec_path.read_bytes()).hexdigest(), "weights": weights,
              "bones": [{"name": n, "parent": p, "head": h.tolist(), "tail": t.tolist()} for n,p,h,t in bones],
              "textures": textures, "preview_renders": [],
              "eye_placements": eye_placements,
              "material_slots": [mat.name for mat in obj.data.materials],
              "near_black_pigment_floor": pigment_floor,
              "limits": "Automatic anatomical skin weights; no facial/finger rig, cloth, ragdoll or authored mocap. Capsule combat collision remains authoritative. Unreal pose inspection required."}
    (output / "prep-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if args.preview:
        report["preview_renders"] = preview_rig(obj, armature, output, spec["height_metres"])
        (output / "prep-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("ORGANIC_ENEMY_PREPARED " + json.dumps({key: report[key] for key in (
        "asset", "fbx", "runtime_triangles", "dimensions_metres", "preview_renders")}), flush=True)


if __name__ == "__main__":
    main()

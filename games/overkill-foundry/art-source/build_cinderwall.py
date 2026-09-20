"""Original Cinderwall art source. Blender 5.2+, no downloaded assets.

blender --background --python build_cinderwall.py -- --output <ignored directory>
Builds two rigidly skinned enemies, action clips, portable PBR textures and stage.
Geometry is in metres. A source render is NOT an in-game quality approval.
"""
import argparse
import json
import math
import random
import sys
from pathlib import Path

import bpy
import numpy as np
from mathutils import Matrix, Vector

ARGS = argparse.ArgumentParser()
ARGS.add_argument("--output", required=True)
ARGS.add_argument("--no-render", action="store_true")
OPT = ARGS.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
OUT = Path(OPT.output).resolve()
for folder in (OUT, OUT / "textures", OUT / "meshes", OUT / "animations", OUT / "renders"):
    folder.mkdir(parents=True, exist_ok=True)

VERSION = "cinderwall-art-v001"
PALETTE = {
    "iron": ("303b42", .9, .43), "edge": ("778082", .93, .34),
    "oxide": ("a5492e", .08, .58), "ochre": ("b18a4d", .07, .58),
    "patina": ("427579", .08, .54), "soot": ("161f25", .35, .72),
    "stone": ("544a42", .03, .88), "copper": ("a46235", .92, .34),
    "ceramic": ("c3bda8", .05, .45),
}
MATS, IMAGES, GROUPS, RIGS, CLIPS = {}, {}, {}, {}, {}
RNG = random.Random(200926)


def rgb(h):
    return tuple(int(h[i:i + 2], 16) / 255 for i in (0, 2, 4))


def linear_rgb(h):
    return tuple(v / 12.92 if v < .04045 else ((v + .055) / 1.055) ** 2.4 for v in rgb(h))


def image_data(name, pixels, colorspace="sRGB"):
    image = bpy.data.images.new(name, width=pixels.shape[1], height=pixels.shape[0], alpha=True)
    image.colorspace_settings.name = colorspace
    image.pixels.foreach_set(pixels.astype(np.float32).reshape(-1))
    image.filepath_raw = str(OUT / "textures" / (name + ".png"))
    image.file_format = "PNG"
    image.save()
    IMAGES[name] = image
    return image


def noise_field(rng, size=256):
    """Original periodic multiscale surface noise; no source photos or ML images."""
    result = np.zeros((size, size), dtype=np.float32)
    for cells, weight in ((8, .45), (32, .25), (64, .18), (128, .12)):
        block = rng.random((cells, cells), dtype=np.float32)
        yy, xx = np.mgrid[:size, :size] * (cells / size)
        ix, iy = xx.astype(int), yy.astype(int)
        fx, fy = xx - ix, yy - iy
        fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)
        expanded = ((1 - fx) * (1 - fy) * block[iy % cells, ix % cells]
                    + fx * (1 - fy) * block[iy % cells, (ix + 1) % cells]
                    + (1 - fx) * fy * block[(iy + 1) % cells, ix % cells]
                    + fx * fy * block[(iy + 1) % cells, (ix + 1) % cells])
        result += expanded * weight
    return result


def materials():
    for i, (name, (color, metal, rough)) in enumerate(PALETTE.items()):
        rng = np.random.default_rng(9127 + i)
        n = noise_field(rng)
        fine = rng.random(n.shape, dtype=np.float32)
        scratches = np.zeros(n.shape, dtype=np.float32)
        for _ in range(38):
            x, y = rng.integers(0, 256, 2)
            length = int(rng.integers(2, 21))
            for t in range(length):
                scratches[(y + t // 4) % 256, (x + t) % 256] = float(rng.uniform(.2, .7))
        pits = np.clip((.32 - n) * 4.7, 0, .48)
        wear = np.clip((n - .62) * 7 + scratches, 0, .8)
        if name in ("soot", "stone", "ceramic"):
            wear *= .15
        base = np.array(rgb(color))[None, None, :]
        surface = base * (.78 + .40 * n[:, :, None] + .10 * fine[:, :, None])
        surface = surface * (1 - wear[:, :, None]) + np.array([.38, .40, .40]) * wear[:, :, None]
        surface *= 1 - pits[:, :, None]
        albedo = np.concatenate((np.clip(surface, 0, 1), np.ones((256, 256, 1))), axis=2)
        orm = np.ones((256, 256, 4))
        orm[:, :, 1] = np.clip(rough + (n - .5) * .32 + pits * .6 - scratches * .13, .17, .96)
        orm[:, :, 2] = np.clip(metal + wear * (.92 - metal) - pits * .4, 0, 1)
        height = n * .08 - scratches * .025 - pits * .03 + fine * .006
        dx = (np.roll(height, -1, 1) - np.roll(height, 1, 1)) * 3.0
        dy = (np.roll(height, -1, 0) - np.roll(height, 1, 0)) * 3.0
        normal = np.stack((-dx, -dy, np.ones_like(n)), axis=2)
        normal /= np.linalg.norm(normal, axis=2)[:, :, None]
        normal = np.concatenate((normal * .5 + .5, np.ones((256, 256, 1))), axis=2)
        mat = bpy.data.materials.new("M_CW_" + name)
        mat.use_nodes = True
        mat.diffuse_color = (*linear_rgb(color), 1)
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        bsdf = nodes.get("Principled BSDF")
        for suffix, pixels, cs in (("BaseColor", albedo, "sRGB"), ("ORM", orm, "Non-Color"), ("Normal", normal, "Non-Color")):
            tex = nodes.new("ShaderNodeTexImage")
            tex.image = image_data("CW_" + name + "_" + suffix, pixels, cs)
            tex.label = suffix
            if suffix == "BaseColor":
                links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
            elif suffix == "ORM":
                split = nodes.new("ShaderNodeSeparateColor")
                links.new(tex.outputs["Color"], split.inputs["Color"])
                links.new(split.outputs["Green"], bsdf.inputs["Roughness"])
                links.new(split.outputs["Blue"], bsdf.inputs["Metallic"])
            else:
                nm = nodes.new("ShaderNodeNormalMap")
                nm.inputs["Strength"].default_value = .35
                links.new(tex.outputs["Color"], nm.inputs["Color"])
                links.new(nm.outputs["Normal"], bsdf.inputs["Normal"])
        MATS[name] = mat
    for name, color, power in (("ember", "ff6922", 3.0), ("furnace", "f35a17", .65), ("optic", "ffc06c", 3.0), ("cool", "67b8c4", 2.0)):
        mat = bpy.data.materials.new("M_CW_" + name)
        mat.use_nodes = True
        mat.diffuse_color = (*linear_rgb(color), 1)
        bsdf = mat.node_tree.nodes.get("Principled BSDF")
        bsdf.inputs["Base Color"].default_value = (*linear_rgb(color), 1)
        bsdf.inputs["Roughness"].default_value = .26
        bsdf.inputs["Metallic"].default_value = .1
        bsdf.inputs["Emission Color"].default_value = (*linear_rgb(color), 1)
        bsdf.inputs["Emission Strength"].default_value = power
        MATS[name] = mat


def mesh_finish(obj, name, mat, group, bevel=.02, smooth=False):
    obj.name = name
    obj.data.materials.append(MATS[mat])
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        modifier = obj.modifiers.new("cast edge radius", "BEVEL")
        modifier.width, modifier.segments = bevel, 3
    if smooth:
        for p in obj.data.polygons:
            p.use_smooth = True
    if bevel or smooth:
        modifier = obj.modifiers.new("weighted industrial normals", "WEIGHTED_NORMAL")
        modifier.keep_sharp = True
        modifier.weight = 40
    # Generated primitive UVs remain deterministic and available for Unreal.
    if obj.data.uv_layers:
        uv = obj.data.uv_layers.active.data
        repeat = max(.5, max(obj.dimensions) * 1.7)
        for item in uv:
            item.uv *= repeat
    GROUPS.setdefault(group, []).append(obj)
    return obj


def box(name, loc, size, mat, group, bevel=.03, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc, rotation=rot)
    obj = bpy.context.object
    obj.scale = size
    return mesh_finish(obj, name, mat, group, min(bevel, min(size) * .25))


def cyl(name, loc, radius, depth, mat, group, axis="Z", verts=32, bevel=.012):
    rotation = {"Z": (0, 0, 0), "X": (0, math.pi / 2, 0), "Y": (math.pi / 2, 0, 0)}[axis]
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=radius, depth=depth, location=loc, rotation=rotation)
    return mesh_finish(bpy.context.object, name, mat, group, bevel, True)


def cone(name, loc, radius1, radius2, depth, mat, group, axis="Z", verts=32):
    rotation = {"Z": (0, 0, 0), "X": (0, -math.pi / 2, 0), "Y": (math.pi / 2, 0, 0)}[axis]
    bpy.ops.mesh.primitive_cone_add(vertices=verts, radius1=radius1, radius2=radius2, depth=depth, location=loc, rotation=rotation)
    return mesh_finish(bpy.context.object, name, mat, group, .012, True)


def sphere(name, loc, scale, mat, group, segments=24):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=12, radius=1, location=loc)
    obj = bpy.context.object
    obj.scale = scale
    return mesh_finish(obj, name, mat, group, 0, True)


def rod(name, a, b, radius, mat, group, verts=16):
    delta = Vector(b) - Vector(a)
    obj = cyl(name, (Vector(a) + Vector(b)) / 2, radius, delta.length, mat, group, verts=verts, bevel=.008)
    obj.rotation_euler = delta.to_track_quat("Z", "Y").to_euler()
    return obj


def ring(name, loc, major, minor, mat, group, axis="Z", segments=40):
    rotation = {"Z": (0, 0, 0), "X": (0, math.pi / 2, 0), "Y": (math.pi / 2, 0, 0)}[axis]
    bpy.ops.mesh.primitive_torus_add(major_segments=segments, minor_segments=8, location=loc,
                                   rotation=rotation, major_radius=major, minor_radius=minor)
    return mesh_finish(bpy.context.object, name, mat, group, 0, True)


def bolt(name, loc, group, axis="Y", radius=.043):
    cyl(name, loc, radius, radius * .5, "edge", group, axis=axis, verts=6, bevel=.003)


def riveted_box(name, loc, size, mat, group, bevel=.025):
    box(name, loc, size, mat, group, bevel)
    x, y, z = loc
    sx, sy, sz = size
    for side in (-1, 1):
        for dx in (-1, 1):
            for dz in (-1, 1):
                bolt(name + "_fastener", (x + dx * (sx / 2 - .09), y + side * (sy / 2 + .007), z + dz * (sz / 2 - .09)), group)


def pipe(name, points, radius, mat, group):
    for i, (a, b) in enumerate(zip(points, points[1:])):
        rod(name + str(i), a, b, radius, mat, group, 16)
    for p in points[1:-1]:
        sphere(name + "elbow", p, (radius * 1.03,) * 3, mat, group, 16)


def socket(name, pivot, parent):
    return {"name": name, "pivot": list(pivot), "parent": parent}


def make_ram():
    prefix = "BR_"
    bones = [socket("root", (0, 0, 0), None), socket("hull", (0, 0, .87), "root"),
             socket("mantle", (-.37, 0, 1.71), "hull"), socket("piston", (-.65, 0, 1.69), "mantle"),
             socket("crown", (.33, 0, 2.16), "hull"), socket("vent_L", (.5, -.57, 1.97), "hull"),
             socket("vent_R", (.5, .57, 1.97), "hull"), socket("track_L", (0, -.79, .42), "root"),
             socket("track_R", (0, .79, .42), "root"), socket("muzzle", (-1.77, 0, 1.69), "piston"),
             socket("core", (.31, 0, 1.52), "hull"), socket("intent", (0, 0, 2.83), "root")]
    g = lambda bone: prefix + bone
    riveted_box("Ram_cast_underframe", (0, 0, .82), (2.04, 1.33, .37), "iron", g("hull"), .08)
    box("Ram_armored_boiler", (.22, 0, 1.52), (1.68, 1.30, 1.20), "oxide", g("hull"), .18)
    box("Ram_shoulders", (-.18, 0, 2.05), (1.1, 1.46, .33), "iron", g("hull"), .10)
    # The continuous low track silhouette distinguishes this heavy pressure machine.
    for s in (-1, 1):
        group = g("track_L" if s < 0 else "track_R")
        y = s * .81
        box("Ram_track_belt", (.03, y, .43), (2.36, .49, .71), "soot", group, .26)
        for x in (-.8, -.4, 0, .4, .8):
            cyl("Ram_roller", (x, y + s * .28, .43), .258, .11, "iron", group, "Y")
            cyl("Ram_roller_hub", (x, y + s * .347, .43), .104, .036, "ochre", group, "Y", 16)
            bolt("Ram_axle", (x, y + s * .375, .43), group, radius=.061)
        for x in np.linspace(-.96, .96, 10):
            for z in (.12, .73):
                box("Ram_track_shoe", (float(x), y, z), (.18, .59, .095), "edge", group, .012)
        for x, angle in ((-1.10, -.6), (1.10, .6)):
            for z in (.27, .55):
                box("Ram_track_end_shoe", (x, y, z), (.16, .59, .18), "iron", group, .025, (0, angle, 0))
        box("Ram_armor_skirt", (.15, y, .98), (1.9, .13, .3), "oxide", g("hull"), .04)
        for x in (-.64, 0, .64):
            bolt("Ram_skirt_bolt", (x, y + s * .09, .99), g("hull"))
        # Exposed hydraulic return circuit is deliberately not a disconnected greeble.
        pipe("Ram_hydraulic_feed", ((.90, s * .59, 1.12), (1.04, s * .61, 1.35),
                                   (.89, s * .66, 1.89), (.36, s * .66, 1.91)), .052, "copper", g("hull"))
        for x in (.0, .37, .74):
            rod("Ram_actuator_sleeve", (x, s * .56, 1.05), (x - .16, s * .56, 1.54), .094, "iron", g("hull"))
            rod("Ram_actuator_rod", (x - .16, s * .56, 1.54), (x - .25, s * .56, 1.86), .040, "edge", g("hull"))
        box("Ram_vent_hinge", (.52, s * .61, 1.99), (.72, .14, .09), "edge", g("vent_L" if s < 0 else "vent_R"))
        for n in range(5):
            box("Ram_vent_fin", (.31 + n * .12, s * .665, 1.82), (.07, .08, .31), "iron", g("vent_L" if s < 0 else "vent_R"), .01, (0, -.12, 0))
    # A wide square mantlet and deeply recessed pressure bore, rather than a face.
    riveted_box("Ram_mantlet", (-.65, 0, 1.72), (.38, 1.26, 1.15), "ochre", g("mantle"), .1)
    cyl("Ram_breech", (-.99, 0, 1.70), .47, .43, "iron", g("piston"), "X", 48, .045)
    cyl("Ram_pressure_sleeve", (-1.30, 0, 1.70), .385, .40, "oxide", g("piston"), "X", 48, .035)
    ring("Ram_muzzle_lip", (-1.61, 0, 1.70), .354, .097, "edge", g("piston"), "X", 48)
    cyl("Ram_bore_black", (-1.535, 0, 1.70), .287, .015, "soot", g("piston"), "X", 48, 0)
    ring("Ram_inner_heat", (-1.553, 0, 1.70), .247, .012, "ember", g("piston"), "X", 40)
    for a in np.linspace(0, math.tau, 8, endpoint=False):
        y, z = math.sin(a) * .452, 1.70 + math.cos(a) * .452
        rod("Ram_barrel_tie", (-1.42, y, z), (-.85, y, z), .039, "edge", g("piston"))
    for s in (-1, 1):
        cyl("Ram_pressure_gauge_bezel", (-.87, s * .47, 2.11), .13, .06, "iron", g("mantle"), "X", 24)
        cyl("Ram_pressure_gauge_glass", (-.908, s * .47, 2.11), .09, .012, "ceramic", g("mantle"), "X", 24, 0)
        box("Ram_pressure_gauge_needle", (-.918, s * .47, 2.11), (.008, .012, .117), "oxide", g("mantle"), .001, (.42, 0, 0))
    cyl("Ram_crown_boiler", (.39, 0, 2.21), .41, .43, "iron", g("crown"), verts=40, bevel=.04)
    ring("Ram_crown_seam", (.39, 0, 2.38), .405, .032, "edge", g("crown"))
    for s in (-1, 1):
        cyl("Ram_exhaust", (.72, s * .41, 2.28), .14, .64 if s < 0 else .47, "iron", g("crown"), verts=24)
        ring("Ram_exhaust_rim", (.72, s * .41, 2.60 if s < 0 else 2.515), .142, .025, "edge", g("crown"))
        cyl("Ram_exhaust_dark", (.72, s * .41, 2.605 if s < 0 else 2.52), .11, .015, "soot", g("crown"), verts=24)
    # Furnace aperture on camera-facing flank makes stored pressure visible.
    box("Ram_flank_aperture", (.16, -.672, 1.65), (.65, .012, .43), "ember", g("hull"), .07)
    for x in np.linspace(-.13, .45, 6):
        box("Ram_flank_grille", (float(x), -.700, 1.65), (.045, .065, .48), "iron", g("hull"), .012)
    for x in (-.25, .59):
        box("Ram_flank_frame", (x, -.721, 1.65), (.06, .08, .55), "edge", g("hull"), .014)
    cyl("Ram_core", (.31, 0, 1.52), .21, .47, "ember", g("core"), "Y", 24)
    return make_rig("BreachRam", prefix, bones)


def make_mite():
    prefix = "RM_"
    bones = [socket("root", (0, 0, 0), None), socket("body", (0, 0, .70), "root"),
             socket("shell", (.18, 0, .86), "body"), socket("head", (-.43, 0, .64), "body"),
             socket("rivet", (-.61, 0, .57), "head"), socket("core", (.14, 0, .74), "body"),
             socket("muzzle", (-.94, 0, .57), "rivet"), socket("intent", (0, 0, 1.42), "root")]
    g = lambda bone: prefix + bone
    for i, x in enumerate((-.38, .06, .43)):
        for s in (-1, 1):
            side = "L" if s < 0 else "R"
            hip, knee, foot = (x, s * .31, .66), (x + .1, s * .65, .46), (x - .05, s * .79, .10)
            bones.extend((socket(f"leg_{side}{i}", hip, "body"), socket(f"shin_{side}{i}", knee, f"leg_{side}{i}")))
            group = g(f"leg_{side}{i}")
            cyl("Mite_hip_pin", hip, .125, .14, "copper", group, "Y", 16)
            rod("Mite_forged_thigh", hip, knee, .074, "iron", group)
            rod("Mite_thigh_piston", (x - .06, s * .34, .63), (x + .04, s * .61, .47), .024, "edge", group)
            group = g(f"shin_{side}{i}")
            cyl("Mite_knee", knee, .107, .15, "ochre", group, "Y", 16)
            rod("Mite_shin", knee, foot, .053, "edge", group)
            box("Mite_gripper_foot", (foot[0] - .048, foot[1], .078), (.24, .16, .10), "iron", group, .03)
            for f in (-1, 1):
                box("Mite_toe", (foot[0] - .155, foot[1] + f * .052, .053), (.10, .033, .065), "edge", group, .008)
    sphere("Mite_pressure_abdomen", (.18, 0, .81), (.57, .41, .39), "patina", g("shell"), 32)
    box("Mite_undercarriage", (0, 0, .65), (.94, .65, .23), "iron", g("body"), .07)
    # Armoured overlapping shell ribs prevent a featureless spherical read.
    for x, r in ((-.12, .365), (.16, .398), (.44, .315)):
        ring("Mite_shell_band", (x, 0, .82), r, .039, "iron", g("shell"), "X", 32)
    for s in (-1, 1):
        for x in (-.11, .16, .43):
            bolt("Mite_shell_rivet", (x, s * .385, .87), g("shell"), radius=.031)
    box("Mite_head_cast", (-.44, 0, .75), (.34, .57, .42), "ochre", g("head"), .085)
    cyl("Mite_single_optic_socket", (-.644, 0, .82), .172, .09, "iron", g("head"), "X", 24)
    sphere("Mite_single_optic", (-.701, 0, .82), (.041, .117, .117), "optic", g("head"))
    box("Mite_eye_brow", (-.64, 0, .98), (.24, .44, .07), "iron", g("head"), .018, (0, -.12, 0))
    cyl("Mite_rivet_driver", (-.63, 0, .54), .116, .41, "iron", g("rivet"), "X", 24)
    ring("Mite_rivet_nose", (-.853, 0, .54), .11, .025, "edge", g("rivet"), "X", 24)
    cone("Mite_rivet_tip", (-.931, 0, .54), .050, .006, .14, "edge", g("rivet"), "X", 16)
    pipe("Mite_air_line", ((-.38, -.27, .62), (-.37, -.35, .93), (.27, -.37, .98), (.40, -.25, .84)), .031, "copper", g("body"))
    cyl("Mite_back_core_socket", (.58, 0, .80), .20, .22, "iron", g("body"), "X", 24)
    cyl("Mite_core", (.14, 0, .74), .158, .26, "ember", g("core"), "Y", 24)
    cyl("Mite_pressure_pin", (.37, 0, 1.18), .062, .17, "copper", g("shell"), verts=16)
    sphere("Mite_pressure_pin_light", (.37, 0, 1.28), (.07, .07, .05), "ember", g("shell"), 16)
    return make_rig("RivetMite", prefix, bones)


def combine(objects, name, weights=None):
    deps = bpy.context.evaluated_depsgraph_get()
    vertices, faces, material_ids, weight_names, mats, normals = [], [], [], [], [], []
    for obj in objects:
        evaluated = obj.evaluated_get(deps)
        mesh = evaluated.to_mesh()
        offset = len(vertices)
        vertices.extend(tuple(obj.matrix_world @ v.co) for v in mesh.vertices)
        weight_names.extend([weights.get(obj.name) if weights else None] * len(mesh.vertices))
        normal_transform = obj.matrix_world.to_3x3().inverted().transposed()
        normals.extend(tuple((normal_transform @ n.vector).normalized()) for n in mesh.corner_normals)
        for p in mesh.polygons:
            faces.append(tuple(offset + v for v in p.vertices))
            material = obj.data.materials[p.material_index]
            if material not in mats:
                mats.append(material)
            material_ids.append(mats.index(material))
        evaluated.to_mesh_clear()
    mesh = bpy.data.meshes.new(name + "_Geo")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    for mat in mats:
        mesh.materials.append(mat)
    for p, mat_id in zip(mesh.polygons, material_ids):
        p.material_index = mat_id
        p.use_smooth = True
    mesh.normals_split_custom_set(normals)
    # Copy evaluated UVs exactly, including bevel surfaces.
    uv = mesh.uv_layers.new(name="UV0")
    loop_offset = 0
    for source in objects:
        evaluated = source.evaluated_get(deps)
        src = evaluated.to_mesh()
        if src.uv_layers:
            for loop in src.uv_layers.active.data:
                uv.data[loop_offset].uv = loop.uv
                loop_offset += 1
        else:
            loop_offset += len(src.loops)
        evaluated.to_mesh_clear()
    if weights:
        for bone in sorted(set(weight_names) - {None}):
            group = obj.vertex_groups.new(name=bone)
            group.add([i for i, b in enumerate(weight_names) if b == bone], 1.0, "REPLACE")
    for source in objects:
        bpy.data.objects.remove(source, do_unlink=True)
    return obj


def make_rig(name, prefix, bones):
    bpy.ops.object.select_all(action="DESELECT")
    data = bpy.data.armatures.new("SK_" + name)
    arm = bpy.data.objects.new("SK_" + name, data)
    bpy.context.collection.objects.link(arm)
    bpy.context.view_layer.objects.active = arm
    arm.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    for entry in bones:
        bone = data.edit_bones.new(entry["name"])
        bone.head = entry["pivot"]
        bone.tail = Vector(entry["pivot"]) + Vector((0, 0, .12))
        if entry["parent"]:
            bone.parent = data.edit_bones[entry["parent"]]
        bone.use_deform = entry["name"] not in ("intent", "muzzle")
    bpy.ops.object.mode_set(mode="OBJECT")
    objects, weights = [], {}
    for entry in bones:
        for obj in GROUPS.get(prefix + entry["name"], []):
            objects.append(obj)
            weights[obj.name] = entry["name"]
    mesh = combine(objects, "SKM_" + name, weights)
    mod = mesh.modifiers.new("rigid machine articulation", "ARMATURE")
    mod.object = arm
    mesh.parent = arm
    arm.show_in_front = True
    arm["art_version"] = VERSION
    arm["forward"] = "-X in source; preserve imported orientation and face player"
    arm["death_cleanup"] = "dissolve ALL materials and detachments, destroy actor at 2.0 seconds"
    RIGS[name] = {"armature": arm, "mesh": mesh, "bones": bones, "prefix": prefix}
    return RIGS[name]


def pose(arm, bone, frame, loc=(0, 0, 0), rot=(0, 0, 0)):
    # Every rest bone points +Z, hence local axes X,+Z,-Y.
    p = arm.pose.bones[bone]
    p.rotation_mode = "XYZ"
    p.location = (loc[0], loc[2], -loc[1])
    p.rotation_euler = tuple(math.radians(a) for a in (rot[0], rot[2], -rot[1]))
    p.keyframe_insert("location", frame=frame, group=bone)
    p.keyframe_insert("rotation_euler", frame=frame, group=bone)


def action(rig, name, duration, entries, cues=None, loop=False):
    arm = rig["armature"]
    arm.animation_data_create()
    act = bpy.data.actions.new(rig["prefix"] + name)
    arm.animation_data.action = act
    end = 1 + round(duration * 30)
    for bone in arm.pose.bones:
        pose(arm, bone.name, 1)
        pose(arm, bone.name, end)
    for bone, samples in entries.items():
        for t, loc, rot in samples:
            pose(arm, bone, 1 + round(t * 30), loc, rot)
    act.use_fake_user = True
    CLIPS[act.name] = {"robot": arm.name, "duration_s": duration, "loop": loop,
                       "events": cues or [], "gameplay_authority": False}
    return act


def animate():
    r = RIGS["BreachRam"]
    action(r, "idle", 2, {"hull": [(1, (0, 0, .015), (0, -.7, 0))], "crown": [(1, (0, 0, .012), (0, 0, .6))]}, loop=True)
    action(r, "charge", 1.25, {
        "hull": [(.55, (.04, 0, -.06), (0, 2.5, 0)), (1.25, (.04, 0, -.06), (0, 2.5, 0))],
        "piston": [(.9, (.16, 0, 0), (0, 0, 0)), (1.25, (.16, 0, 0), (0, 0, 0))],
        "vent_L": [(.55, (0, 0, 0), (-28, 0, 0)), (1.25, (0, 0, 0), (-28, 0, 0))],
        "vent_R": [(.55, (0, 0, 0), (28, 0, 0)), (1.25, (0, 0, 0), (28, 0, 0))],
    }, [{"t": .25, "event": "pressure_start"}, {"t": 1.05, "event": "charged_glow_hold"}])
    action(r, "attack_blast", .9, {
        "piston": [(0, (.16, 0, 0), (0, 0, 0)), (.20, (.13, 0, 0), (0, 0, 0)), (.30, (-.08, 0, 0), (0, 0, 0)), (.40, (.25, 0, 0), (0, 0, 0))],
        "hull": [(0, (.04, 0, -.06), (0, 2.5, 0)), (.40, (.08, 0, -.035), (0, -5, 0)), (.68, (0, 0, .018), (0, 1.5, 0))],
        "vent_L": [(0, (0, 0, 0), (-28, 0, 0))],
        "vent_R": [(0, (0, 0, 0), (28, 0, 0))],
        "crown": [(.45, (0, 0, .035), (0, 4, 0))],
    }, [{"t": .30, "event": "muzzle_flash_and_display_committed_hit"}, {"t": .45, "event": "steam_vent"}])
    for name, duration, x, roll, pitch in (("light", .34, .02, 1, -1.7), ("medium", .48, .07, -4, -5), ("heavy", .64, .15, -9, -10)):
        action(r, "hit_" + name, duration, {"hull": [(.10, (x, 0, -.02), (roll, pitch, 0))],
                "mantle": [(.13, (0, 0, 0), (0, pitch * .5, 0))], "crown": [(.16, (0, 0, .02 if name != "light" else 0), (roll * -.5, 0, 0))]})
    action(r, "death", 2.0, {
        "hull": [(.30, (.10, 0, -.08), (0, -8, 0)), (.85, (.20, -.18, -.55), (-34, -5, -7)), (2, (.20, -.18, -.55), (-34, -5, -7))],
        "crown": [(.42, (.1, .1, .70), (22, 15, 40)), (1.15, (.60, .35, -.75), (60, 100, 90)), (2, (.60, .35, -.75), (60, 100, 90))],
        "piston": [(.30, (-.24, 0, 0), (0, 12, 0)), (1.1, (-.31, 0, -.40), (0, -15, 0)), (2, (-.31, 0, -.40), (0, -15, 0))],
        "track_L": [(.7, (0, -.10, 0), (-13, 0, -3)), (2, (0, -.10, 0), (-13, 0, -3))],
    }, [{"t": .20, "event": "core_burst_cosmetic"}, {"t": .85, "event": "collapse_contact"},
        {"t": 1.10, "event": "dissolve_start_all_components"}, {"t": 2.0, "event": "destroy_actor_and_fx"}])
    action(r, "escape", 1.05, {"root": [(.18, (-.05, 0, 0), (0, 0, -3)), (1.05, (4, 0, 0), (0, 0, 0))],
        "hull": [(.30, (0, 0, .035), (0, -4, 0))]}, [{"t": 1.05, "event": "despawn_without_death_fx"}])
    r = RIGS["RivetMite"]
    legs = [b.name for b in r["armature"].pose.bones if b.name.startswith("leg_")]
    idle = {"body": [(.5, (0, 0, .020), (0, -2, 0)), (1.4, (0, 0, -.006), (1, 1, 0))], "head": [(.35, (0, 0, 0), (0, 0, 6)), (1.2, (0, 0, 0), (0, 0, -6))]}
    action(r, "idle", 2, idle, loop=True)
    attack = {"body": [(.18, (.06, 0, -.09), (0, -7, 0)), (.33, (-.13, 0, .03), (0, 9, 0)), (.65, (.035, 0, -.01), (0, -3, 0))],
              "head": [(.20, (0, 0, 0), (0, -8, 0)), (.33, (-.05, 0, 0), (0, 5, 0))],
              "rivet": [(.20, (.035, 0, 0), (0, 0, 0)), (.33, (-.19, 0, 0), (0, 0, 0)), (.46, (.05, 0, 0), (0, 0, 0))]}
    for b in legs:
        attack[b] = [(.18, (0, 0, 0), ((-1 if "L" in b else 1) * 13, 0, 0)), (.36, (0, 0, 0), ((1 if "L" in b else -1) * 9, 0, 0))]
    action(r, "attack_rivet", .85, attack, [{"t": .33, "event": "rivet_flash_and_display_committed_hit"}])
    for name, duration, x, lift, roll in (("light", .30, .018, 0, 3), ("medium", .43, .08, .03, -12), ("heavy", .62, .18, .12, -29)):
        entry = {"body": [(.1, (x, 0, lift), (roll, -8 if name != "light" else -2, 0))], "head": [(.12, (0, 0, 0), (0, -10 if name != "light" else -2, -roll / 2))]}
        if name == "heavy":
            entry["shell"] = [(.14, (0, 0, .02), (0, 7, 0))]
        action(r, "hit_" + name, duration, entry)
    death = {"body": [(.20, (.05, 0, .21), (10, 0, 0)), (.6, (.10, -.08, -.27), (-68, -6, 15)), (2, (.10, -.08, -.27), (-68, -6, 15))],
             "shell": [(.30, (.14, .13, .6), (30, 5, 40)), (.9, (.39, .28, -.3), (110, 25, 140)), (2, (.39, .28, -.3), (110, 25, 140))]}
    for b in legs:
        death[b] = [(.26, (0, 0, 0), ((-1 if "L" in b else 1) * 42, 0, 15)), (.7, (0, 0, 0), ((1 if "L" in b else -1) * 45, 0, 10)), (2, (0, 0, 0), ((1 if "L" in b else -1) * 45, 0, 10))]
    action(r, "death", 2.0, death, [{"t": .20, "event": "core_burst_cosmetic"}, {"t": .60, "event": "collapse_contact"},
           {"t": 1.1, "event": "dissolve_start_all_components"}, {"t": 2, "event": "destroy_actor_and_fx"}])
    escape = {"root": [(1.0, (3.0, 0, 0), (0, 0, 0))], "body": [(.25, (0, 0, .025), (0, 4, 0)), (.5, (0, 0, -.015), (0, -3, 0)), (.75, (0, 0, .025), (0, 4, 0))]}
    for i, b in enumerate(legs):
        escape[b] = [(t, (0, 0, 0), (0, (14 if (i + k) % 2 else -14), (6 if (i + k) % 2 else -6))) for k, t in enumerate((.2, .4, .6, .8))]
    action(r, "escape", 1.0, escape, [{"t": 1, "event": "despawn_without_death_fx"}])


def stage():
    # Each group is a reusable mesh, with authored world placement in the scene.
    for x in np.arange(-9, 10, 2):
        for y in (-2, 0, 2):
            group = f"Stage_deck_{x}_{y}"
            box("CW_deck_cast_plate", (float(x), y, -.10), (1.96, 1.96, .20), "iron", group, .025)
            for dx in (-.82, .82):
                for dy in (-.82, .82):
                    bolt("CW_deck_fastener", (float(x) + dx, y + dy, .012), group, "Z", .045)
            for i in range(4):
                box("CW_non_slip_rib", (float(x) - .44 + i * .27, y - .55, .015), (.14, .72, .018), "edge", group, .004, (0, 0, -.30))
    box("CW_foundation", (0, 2, -.39), (22, 12, .56), "stone", "Stage_foundation", .05)
    # Seven arched furnace bays remain behind the combat plane, keeping sight lines open.
    for i, x in enumerate(range(-9, 10, 3)):
        group = f"Stage_furnace_{i}"
        box("CW_masonry_pier", (x, 5.7, 2.0), (2.94, .75, 4.0), "stone", group, .08)
        box("CW_furnace_dark", (x, 5.28, 1.45), (2.35, .10, 2.28), "soot", group, .15)
        box("CW_furnace_radiance", (x, 5.20, 1.2), (1.70, .045, 1.55), "furnace", group, .18)
        for n in range(7):
            angle = math.pi * n / 6
            xx, zz = x + math.cos(angle) * 1.1, 2.15 + math.sin(angle) * 1.1
            box("CW_arch_wedge", (xx, 5.04, zz), (.46, .55, .55), "iron", group, .055, (0, angle - math.pi / 2, 0))
        for side in (-1, 1):
            riveted_box("CW_door_jamb", (x + side * 1.12, 5.03, 1.23), (.30, .48, 1.90), "iron", group)
        for dx in np.linspace(-.86, .86, 7):
            box("CW_furnace_grille", (x + float(dx), 4.95, 1.45), (.08, .10, 2.1), "iron", group, .016)
        box("CW_masonry_cap", (x, 5.68, 4.14), (3.02, 1.03, .30), "iron", group, .045)
        # Far stacks and staggered roof breaks provide real dimensional city depth.
        h = (5.7, 7.2, 6.3, 8.0, 6.8, 5.9, 7.5)[i]
        cyl("CW_flue", (x + .65, 7.0, h / 2), .57, h, "iron", group, verts=24, bevel=.025)
        for z in (h - .5, h - 1.3, 4.5):
            ring("CW_flue_collar", (x + .65, 7.0, z), .59, .08, "edge", group, segments=24)
        cone("CW_flue_cap", (x + .65, 7.0, h + .20), .69, .45, .40, "soot", group, verts=24)
    # Long service pipe run, catwalk and cantilevered crane express Cinderwall.
    for z in (3.7, 4.0):
        pipe("CW_service_main", ((-10, 4.25, z), (-6, 4.25, z), (0, 4.25, z), (6, 4.25, z), (10, 4.25, z)), .13, "copper", "Stage_service_main")
        for x in range(-9, 10, 3):
            ring("CW_pipe_flange", (x, 4.25, z), .19, .036, "iron", "Stage_service_main", "X", 24)
    box("CW_catwalk_deck", (0, 4.12, 4.40), (21, 1.12, .16), "iron", "Stage_catwalk", .02)
    for x in range(-10, 11):
        rod("CW_catwalk_post", (x, 3.64, 4.40), (x, 3.64, 5.34), .043, "iron", "Stage_catwalk")
    for z in (4.90, 5.34):
        rod("CW_catwalk_rail", (-10, 3.64, z), (10, 3.64, z), .05, "edge", "Stage_catwalk")
    for x in (-8.8, 8.8):
        group = "Stage_gantry_left" if x < 0 else "Stage_gantry_right"
        riveted_box("CW_gantry_web", (x, 1.30, 3.4), (.32, .60, 6.8), "iron", group, .02)
        for yy in (1.02, 1.58):
            box("CW_Ibeam_flange", (x, yy, 3.4), (.73, .10, 6.8), "oxide", group, .018)
        box("CW_gantry_foot", (x, 1.3, .13), (1.05, 1.03, .26), "iron", group, .02)
        for z in (1.0, 2.4, 3.8, 5.2):
            box("CW_gantry_fishplate", (x, .925, z), (.54, .07, .48), "ochre", group, .017)
    box("CW_crane_cross_web", (0, 1.3, 6.4), (18.7, .30, .63), "iron", "Stage_crane", .02)
    for z in (6.1, 6.7):
        box("CW_crane_cross_flange", (0, 1.3, z), (18.7, .71, .13), "oxide", "Stage_crane", .02)
    # Rear gathering zone: this fixture does not occupy the firing lane.
    group = "Stage_rear_claw_mount"
    box("CW_claw_trolley", (-6.75, 1.3, 5.95), (1.14, 1.0, .37), "ochre", group, .055)
    cyl("CW_claw_cable_reel", (-6.75, 1.3, 5.68), .32, .77, "iron", group, "Y")
    for x in (-6.94, -6.56):
        rod("CW_claw_suspension", (x, 1.0, 5.63), (x, 1.0, 3.75), .025, "edge", group)
    cyl("CW_claw_hub", (-6.75, 1.0, 3.58), .24, .38, "iron", group)
    for a in np.linspace(0, math.tau, 3, endpoint=False):
        p0 = (-6.75 + math.cos(a) * .18, 1 + math.sin(a) * .18, 3.52)
        p1 = (-6.75 + math.cos(a) * .61, 1 + math.sin(a) * .61, 3.05)
        p2 = (-6.75 + math.cos(a) * .31, 1 + math.sin(a) * .31, 2.79)
        rod("CW_claw_cast_finger", p0, p1, .083, "oxide", group)
        rod("CW_claw_hook", p1, p2, .069, "edge", group)
    # Reusable storage props restricted to the rear staging volume.
    for i, (x, y) in enumerate(((-7.0, .8), (-7.8, .7), (-6.9, 1.6))):
        group = "Stage_scrap_bin_" + str(i)
        riveted_box("CW_salvage_bin", (x, y, .35), (.80, .75, .68), "iron", group, .04)
        for j in range(8):
            loc = (x + RNG.uniform(-.34, .34), y + RNG.uniform(-.28, .28), RNG.uniform(.61, .9))
            if j % 2:
                ring("CW_salvage_cog_blank", loc, .14, .041, "edge", group, "X", 16)
            else:
                box("CW_salvage_plate", loc, (.21, .10, .07), "oxide", group, .01,
                    tuple(RNG.uniform(-1, 1) for _ in range(3)))
    meshes = []
    deck_data = None
    for group, objects in list(GROUPS.items()):
        if group.startswith("Stage_"):
            mesh = combine(objects, "SM_CW_" + group[6:])
            mins = Vector(tuple(min(v.co[a] for v in mesh.data.vertices) for a in range(3)))
            maxs = Vector(tuple(max(v.co[a] for v in mesh.data.vertices) for a in range(3)))
            origin = Vector(((mins.x + maxs.x) / 2, (mins.y + maxs.y) / 2, mins.z))
            for v in mesh.data.vertices:
                v.co -= origin
            mesh.location = origin
            if group.startswith("Stage_deck_"):
                if deck_data is None:
                    deck_data = mesh.data
                else:
                    mesh.data = deck_data
            meshes.append(mesh)
    return meshes


def set_selection(objects):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.hide_set(False)
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]


def export(stage_meshes):
    for name, rig in RIGS.items():
        arm, mesh = rig["armature"], rig["mesh"]
        arm.animation_data.action = None
        for bone in arm.pose.bones:
            bone.matrix_basis = Matrix.Identity(4)
        set_selection([arm, mesh])
        fbx_settings = dict(use_selection=True, object_types={"ARMATURE", "MESH"}, add_leaf_bones=False,
                            use_armature_deform_only=False, apply_unit_scale=True,
                            apply_scale_options="FBX_SCALE_UNITS", axis_forward="-Y", axis_up="Z",
                            path_mode="RELATIVE", embed_textures=False)
        bpy.ops.export_scene.fbx(filepath=str(OUT / "meshes" / (name + ".fbx")), bake_anim=False, **fbx_settings)
        actions = [a for a in bpy.data.actions if a.name.startswith(rig["prefix"])]
        for act in actions:
            arm.animation_data.action = act
            bpy.context.scene.frame_start = 1
            bpy.context.scene.frame_end = 1 + round(CLIPS[act.name]["duration_s"] * 30)
            bpy.ops.export_scene.fbx(filepath=str(OUT / "animations" / (act.name + ".fbx")), bake_anim=True,
                                    bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
                                    bake_anim_simplify_factor=0, **fbx_settings)
        arm.animation_data.action = None
        for act in actions:
            track = arm.animation_data.nla_tracks.new()
            track.name = act.name
            strip = track.strips.new(act.name, 1, act)
            strip.action_frame_start = 1
            strip.action_frame_end = 1 + round(CLIPS[act.name]["duration_s"] * 30)
            track.mute = True
        bpy.ops.export_scene.gltf(filepath=str(OUT / "meshes" / (name + ".glb")), use_selection=True,
                                  export_format="GLB", export_animations=True,
                                  export_animation_mode="ACTIONS", export_anim_single_armature=False,
                                  export_all_influences=False,
                                  export_extras=True)
        arm.animation_data.action = bpy.data.actions[rig["prefix"] + "idle"]
    set_selection(stage_meshes)
    bpy.ops.export_scene.fbx(filepath=str(OUT / "meshes" / "CinderwallStage.fbx"), use_selection=True,
                             object_types={"MESH"}, bake_anim=False, apply_unit_scale=True,
                             apply_scale_options="FBX_SCALE_UNITS", axis_forward="-Y", axis_up="Z", path_mode="RELATIVE")
    bpy.ops.export_scene.gltf(filepath=str(OUT / "meshes" / "CinderwallStage.glb"), use_selection=True,
                              export_format="GLB", export_animations=False)


def light(name, loc, color, power, size=4, target=(0, 0, 1), kind="AREA"):
    data = bpy.data.lights.new(name, kind)
    data.energy, data.color = power, color
    if kind == "AREA":
        data.shape, data.size = "DISK", size
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    obj.location = loc
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat("-Z", "Y").to_euler()
    return obj


def camera(name, loc, target, lens=48, ortho=None):
    data = bpy.data.cameras.new(name)
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    obj.location = loc
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat("-Z", "Y").to_euler()
    data.lens = lens
    if ortho:
        data.type, data.ortho_scale = "ORTHO", ortho
    return obj


def review_scene(stage_meshes):
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 40
    scene.cycles.use_denoising = True
    try:
        prefs = bpy.context.preferences.addons["cycles"].preferences
        prefs.compute_device_type = "OPTIX"
        prefs.get_devices()
        for d in prefs.devices:
            d.use = d.type == "OPTIX"
        scene.cycles.device = "GPU"
    except Exception:
        scene.cycles.device = "CPU"
    scene.render.resolution_x, scene.render.resolution_y = 1600, 900
    scene.render.resolution_percentage = 100
    scene.render.fps = 30
    if scene.world is None:
        scene.world = bpy.data.worlds.new("Cinderwall_World")
    scene.world.use_nodes = True
    world_bg = scene.world.node_tree.nodes.get("Background")
    world_bg.inputs["Color"].default_value = (.12, .19, .28, 1)
    world_bg.inputs["Strength"].default_value = .38
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure = .6
    light("warm_worklight", (-4, -4, 7), (1.0, .70, .43), 3000, 6, (2, 0, 1))
    light("cool_sky_fill", (3, -1, 8), (.43, .68, 1), 1700, 5, (2, 0, 1))
    light("furnace_rim", (4, 4.4, 4.4), (1, .37, .11), 2300, 4, (3, 0, 1.2))
    light("front_shape", (-1, -6, 3), (.72, .85, 1), 700, 4, (2, 0, 1))
    for x in (-6, 0, 6):
        light("furnace_pool", (x, 4.25, 1.5), (1, .23, .045), 170, .4, kind="POINT")
    RIGS["BreachRam"]["armature"].location = (3.6, .65, .015)
    RIGS["RivetMite"]["armature"].location = (1.3, -.20, .015)
    cameras = {
        "preparation": camera("16B_preparation", (-.6, -24, 7), (-.6, .6, 2.25), ortho=21.6),
        "action": camera("16C_action", (-7.4, -10.4, 3.7), (2.5, .35, 1.45), lens=46),
        "pair_detail": camera("Pair_material_review", (-1.5, -7.8, 3.4), (2.8, .20, 1.23), lens=52),
    }
    scene.frame_start, scene.frame_end = 1, 61
    scene.frame_set(1)
    scene.camera = cameras["pair_detail"]
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "Cinderwall_Source.blend"))
    if not OPT.no_render:
        for name, cam in cameras.items():
            scene.camera = cam
            scene.render.filepath = str(OUT / "renders" / (name + ".png"))
            bpy.ops.render.render(write_still=True)
        # Representative posed stills verify rigid articulation; these are source renders.
        scene.camera = cameras["pair_detail"]
        for state, frame in (("charge", 30), ("hit_heavy", 5), ("death", 22)):
            for rig in RIGS.values():
                clip = (rig["prefix"] + ("idle" if state == "charge" and rig["prefix"] == "RM_" else state))
                rig["armature"].animation_data.action = bpy.data.actions[clip]
            scene.frame_set(frame)
            scene.render.filepath = str(OUT / "renders" / (state + ".png"))
            bpy.ops.render.render(write_still=True)
    for rig in RIGS.values():
        rig["armature"].animation_data.action = bpy.data.actions[rig["prefix"] + "idle"]
    scene.frame_set(1)
    scene.camera = cameras["pair_detail"]
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "Cinderwall_Source.blend"))
    return {k: {"position_m": list(c.location), "rotation_euler_radians": list(c.rotation_euler),
                 "focal_length_mm": c.data.lens, "projection": c.data.type,
                 "ortho_width_m": c.data.ortho_scale if c.data.type == "ORTHO" else None} for k, c in cameras.items()}


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    scene.render.fps = 30
    materials()
    make_ram()
    make_mite()
    animate()
    stage_meshes = stage()
    export(stage_meshes)
    camera_info = review_scene(stage_meshes)
    metrics = {"version": VERSION, "blender": bpy.app.version_string, "units": "metres", "fps": 30,
        "robots": {}, "stage": {"mesh_count": len(stage_meshes), "triangles": 0,
            "instances": [{"name": o.name, "mesh": o.data.name, "position_m": list(o.location),
                           "rotation_euler_radians": list(o.rotation_euler), "scale": list(o.scale)} for o in stage_meshes]},
        "clips": CLIPS, "cameras": camera_info, "palette": PALETTE,
        "render_evidence": "Original Blender source renders only; no Unreal integration or human visual approval.",
        "source_provenance": "Original procedural geometry, texture algorithms, rigging and keyframes by Game Studio / Codex; no third-party art."}
    for name, rig in RIGS.items():
        mesh = rig["mesh"].data
        mesh.calc_loop_triangles()
        metrics["robots"][name] = {"vertices": len(mesh.vertices), "triangles": len(mesh.loop_triangles),
            "bones": len(rig["bones"]), "materials": [m.name for m in mesh.materials],
            "rest_bounds_m": list(rig["mesh"].dimensions), "sockets": [b for b in rig["bones"] if b["name"] in ("muzzle", "core", "intent")],
            "rigid_weights": True}
    for obj in stage_meshes:
        obj.data.calc_loop_triangles()
        metrics["stage"]["triangles"] += len(obj.data.loop_triangles)
    (OUT / "asset-report.json").write_text(json.dumps(metrics, indent=2), encoding="utf-8")
    print("CINDERWALL_ART_REPORT " + json.dumps(metrics))


if __name__ == "__main__":
    main()

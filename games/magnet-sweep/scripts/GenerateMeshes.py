"""Generate the original Magnet Sweep concept meshes using Blender.

Run: blender --background --factory-startup --python scripts/GenerateMeshes.py
Optional: append -- --preview to write a neutral studio contact sheet.
All dimensions are authored in centimeters. No external inputs are required.
"""
from pathlib import Path
import csv
import hashlib
import json
import math
import sys

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets" / "models"
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = "METRIC"
scene.unit_settings.scale_length = 0.01

neutral = bpy.data.materials.new("NeutralMetal")
neutral.diffuse_color = (0.35, 0.4, 0.43, 1)
neutral.use_nodes = True
shader = neutral.node_tree.nodes.get("Principled BSDF")
shader.inputs["Base Color"].default_value = neutral.diffuse_color
shader.inputs["Metallic"].default_value = 0.45
shader.inputs["Roughness"].default_value = 0.3


def activate(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def finish_part(obj, bevel=0, smooth=True):
    activate(obj)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    if bevel:
        mod = obj.modifiers.new("Soft machined edges", "BEVEL")
        mod.width = bevel
        mod.segments = 3
        mod.limit_method = "ANGLE"
        bpy.ops.object.modifier_apply(modifier=mod.name)
    for face in obj.data.polygons:
        face.use_smooth = smooth
    if bevel:
        mod = obj.modifiers.new("Stable face normals", "WEIGHTED_NORMAL")
        mod.keep_sharp = True
        mod.weight = 50
        bpy.ops.object.modifier_apply(modifier=mod.name)
    return obj


def box(dimensions, location=(0, 0, 0), bevel=1):
    bpy.ops.mesh.primitive_cube_add(size=1, location=location)
    obj = bpy.context.object
    obj.dimensions = dimensions
    return finish_part(obj, bevel)


def cylinder(radius, depth, location=(0, 0, 0), vertices=24, axis="Z", bevel=0.4):
    rot = (0, math.pi / 2, 0) if axis == "X" else (0, 0, 0)
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth,
                                      location=location, rotation=rot)
    return finish_part(bpy.context.object, bevel)


def torus(major, minor, location=(0, 0, 0), axis="Z", segments=32):
    rot = (0, math.pi / 2, 0) if axis == "X" else (0, 0, 0)
    bpy.ops.mesh.primitive_torus_add(major_radius=major, minor_radius=minor,
                                    major_segments=segments, minor_segments=8,
                                    location=location, rotation=rot)
    return finish_part(bpy.context.object)


def annulus(outer, inner, thickness, segments=32):
    verts = []
    for z, radius in [(-thickness / 2, outer), (thickness / 2, outer),
                      (-thickness / 2, inner), (thickness / 2, inner)]:
        verts.extend([(radius * math.cos(i * math.tau / segments),
                       radius * math.sin(i * math.tau / segments), z)
                      for i in range(segments)])
    faces = []
    for i in range(segments):
        j = (i + 1) % segments
        faces.extend([(i, j, segments + j, segments + i),
                      (2 * segments + j, 2 * segments + i, 3 * segments + i, 3 * segments + j),
                      (segments + i, segments + j, 3 * segments + j, 3 * segments + i),
                      (j, i, 2 * segments + i, 2 * segments + j)])
    mesh = bpy.data.meshes.new("Machined washer")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new("Washer", mesh)
    bpy.context.collection.objects.link(obj)
    return finish_part(obj, 0.65)


def bent_bar(points, radius=3):
    curve = bpy.data.curves.new("Bent salvage rod", "CURVE")
    curve.dimensions = "3D"
    curve.resolution_u = 8
    curve.bevel_depth = radius
    curve.bevel_resolution = 1
    curve.resolution_u = 6
    curve.use_fill_caps = True
    spline = curve.splines.new("BEZIER")
    spline.bezier_points.add(len(points) - 1)
    for point, coords in zip(spline.bezier_points, points):
        point.co = coords
        point.handle_left_type = "AUTO"
        point.handle_right_type = "AUTO"
    obj = bpy.data.objects.new("Bent bar", curve)
    bpy.context.collection.objects.link(obj)
    activate(obj)
    bpy.ops.object.convert(target="MESH")
    return finish_part(bpy.context.object)


def horseshoe():
    count = 16
    outline = [(-40, -35), (-40, -5)]
    outline += [(40 * math.cos(math.pi - i * math.pi / count),
                 -5 + 40 * math.sin(math.pi - i * math.pi / count))
                for i in range(1, count + 1)]
    outline += [(40, -35), (22, -35), (22, -5)]
    outline += [(22 * math.cos(i * math.pi / count),
                 -5 + 22 * math.sin(i * math.pi / count))
                for i in range(1, count + 1)]
    outline += [(-22, -35)]
    n = len(outline)
    verts = [(x, y, z) for z in (-9, 9) for x, y in outline]
    # The clockwise perimeter makes the lower face point down.
    faces = [tuple(range(n)), tuple(reversed(range(n, 2 * n)))]
    faces.extend([(i, i + n, (i + 1) % n + n, (i + 1) % n) for i in range(n)])
    mesh = bpy.data.meshes.new("Forged U profile")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new("Magnet body", mesh)
    bpy.context.collection.objects.link(obj)
    return finish_part(obj, 2.0)


def combine(name, parts):
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    if len(parts) > 1:
        bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    # Center the bounding box in all three axes. All exported transforms are identity.
    bounds = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    center = Vector([(min(v[i] for v in bounds) + max(v[i] for v in bounds)) / 2 for i in range(3)])
    for vertex in obj.data.vertices:
        vertex.co = obj.matrix_world @ vertex.co - center
    obj.location = (0, 0, 0)
    obj.rotation_euler = (0, 0, 0)
    obj.scale = (1, 1, 1)
    obj.data.materials.clear()
    obj.data.materials.append(neutral)
    for poly in obj.data.polygons:
        poly.material_index = 0
    bpy.context.view_layer.update()
    return obj


assets = []
assets.append(combine("SM_Washer", [annulus(14, 6, 4)]))
bolt_parts = [cylinder(7.8, 7, (-11.5, 0, 0), 6, "X", 0.65),
              cylinder(3.9, 23, (3.5, 0, 0), 16, "X", 0.35),
              cylinder(5.6, 1.7, (-7.2, 0, 0), 24, "X", 0.2)]
bolt_parts += [torus(3.9, 0.6, (x, 0, 0), "X", 16) for x in (3, 7, 11)]
assets.append(combine("SM_Bolt", bolt_parts))
assets.append(combine("SM_Plate", [box((24, 17, 4), bevel=1.2)]))
tangle_parts = [
    bent_bar([(-29, -14, 0), (-15, 16, 4), (11, 12, 1), (24, -14, 0)], 3.0),
    bent_bar([(-25, 6, 4), (-7, -18, 8), (14, -13, 6), (28, 18, 2)], 2.8),
    bent_bar([(-20, 20, -2), (-7, 5, 1), (-12, -10, 6), (8, 3, 10), (25, 0, 8)], 3.2),
    bent_bar([(-7, -19, -2), (7, -8, 0), (12, 9, 7), (-5, 19, 9), (-16, 7, 7)], 2.5),
    torus(10, 2.6, (3, 1, 2), segments=24),
]
tangle = combine("SM_Tangle", tangle_parts)
tangle.dimensions = (65, 49, 20)
activate(tangle)
bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
assets.append(tangle)
assets.append(combine("SM_Ring", [torus(18, 3)]))
assets.append(combine("SM_MagnetBody", [horseshoe()]))
assets.append(combine("SM_Tray", [box((1000, 620, 20), bevel=9)]))

records = []
for obj in assets:
    activate(obj)
    path = OUT / f"{obj.name}.fbx"
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True,
                            object_types={"MESH"}, global_scale=1.0,
                            apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                            axis_forward="-Y", axis_up="Z", bake_space_transform=False,
                            use_mesh_modifiers=True, mesh_smooth_type="FACE",
                            add_leaf_bones=False, bake_anim=False, path_mode="AUTO")
    obj.data.calc_loop_triangles()
    records.append({"asset": obj.name, "file": path.name,
                    "dimensions_cm": [round(v, 3) for v in obj.dimensions],
                    "vertices": len(obj.data.vertices), "triangles": len(obj.data.loop_triangles),
                    "material_slots": len(obj.data.materials),
                    "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})

# Round-trip the files into the same centimeter scene: validates object count,
# origin, material count, dimensions and finite mesh data without relying on an export flag.
for record in records:
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(OUT / record["file"]), use_custom_normals=True)
    imported = [o for o in set(bpy.data.objects) - before if o.type == "MESH"]
    assert len(imported) == 1, (record["asset"], "Expected one exported mesh")
    obj = imported[0]
    dimensions = [round(v, 3) for v in obj.dimensions]
    assert all(abs(a - b) < 0.02 for a, b in zip(dimensions, record["dimensions_cm"])), (record, dimensions)
    assert len(obj.data.materials) == 1, (record["asset"], "Expected one material slot")
    assert obj.location.length < 0.001, (record["asset"], "Origin must remain centered")
    assert all(math.isfinite(v) for vertex in obj.data.vertices for v in vertex.co)
    record["fbx_round_trip_cm"] = dimensions
    record["fbx_round_trip"] = "passed"
    for obj in imported:
        bpy.data.objects.remove(obj, do_unlink=True)

report = {"blender_version": bpy.app.version_string, "authoring_units": "centimeters",
          "scene_unit_scale": scene.unit_settings.scale_length,
          "export": "FBX_SCALE_UNITS; -Y forward; Z up; one neutral material; no textures",
          "assets": records}
(OUT / "mesh-analysis.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")

with (OUT / "manifest-rows.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(["asset_id", "path", "source_url", "creator", "license", "proof_path", "modifications", "approval_status"])
    for record in records:
        writer.writerow([record["asset"].lower(), f"assets/models/{record['file']}",
                         "original: scripts/GenerateMeshes.py", "Game Studio / Codex",
                         "Original project asset; no third-party source; project distribution license unset",
                         "assets/models/PROVENANCE.md", "Procedural Blender mesh; bevels; centered origin; FBX export", "original-project"])

if "--preview" in sys.argv:
    # A mesh inspection render, not a gameplay screenshot. Each piece is rescaled
    # to comparable preview size; the actual exported FBX dimensions stay unchanged.
    for i, obj in enumerate(assets):
        col, row = i % 3, i // 3
        obj.scale = (1, 1, 1)
        factor = 125 / max(obj.dimensions)
        obj.scale = (factor, factor, factor)
        obj.location = ((col - 1) * 175, (1 - row) * 175, 18)
        obj.hide_render = False
    floor = box((600, 600, 8), (0, 0, -14), bevel=4)
    floor_mat = bpy.data.materials.new("Inspection background")
    floor_mat.diffuse_color = (0.032, 0.048, 0.057, 1)
    floor.data.materials.append(floor_mat)
    bpy.ops.object.camera_add(location=(340, -570, 920))
    camera = bpy.context.object
    camera.rotation_euler = (Vector((0, 0, 0)) - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = 720
    camera.data.clip_end = 5000
    scene.camera = camera
    for location, energy, size in [((-260, -220, 650), 5000000, 480), ((300, 200, 420), 3500000, 380)]:
        bpy.ops.object.light_add(type="AREA", location=location)
        light = bpy.context.object
        light.data.energy = energy
        light.data.shape = "DISK"
        light.data.size = size
        light.rotation_euler = (Vector((0, 0, 0)) - light.location).to_track_quat("-Z", "Y").to_euler()
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 24
    scene.world.color = (0.18, 0.18, 0.18)
    scene.render.resolution_x = 1200
    scene.render.resolution_y = 1000
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = str(OUT / "mesh-preview.png")
    bpy.ops.render.render(write_still=True)

print("MAGNET_SWEEP_MESHES_READY " + json.dumps(records))

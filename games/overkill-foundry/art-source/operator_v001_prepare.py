"""CPU Blender cleanup, UVs and texture bake for the original Mara surface.

No primitive stand-in anatomy. Input is operator_v001_generate.py's new mesh.
Use --inspect-only before committing to native orientation/landmarks.
"""
import argparse
import json
import math
from pathlib import Path
import sys

import bpy
import bmesh
import numpy as np
from mathutils import Vector
from mathutils.kdtree import KDTree

parser = argparse.ArgumentParser()
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--inspect-only", action="store_true")
parser.add_argument("--yaw", type=float, default=90)
parser.add_argument("--texture-size", type=int, default=2048)
parser.add_argument("--shell-thickness", type=float, default=.014)
parser.add_argument("--voxel-size", type=float, default=.0035)
args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
out = args.output.resolve()
for name in ("textures", "renders", "meshes", "animations"):
    (out / name).mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = "METRIC"
scene.unit_settings.scale_length = 1
scene.render.engine = "CYCLES"
scene.cycles.device = "CPU"
scene.cycles.samples = 16
scene.render.threads_mode = "FIXED"
scene.render.threads = 6
scene.view_settings.view_transform = "AgX"
scene.world = bpy.data.worlds.new("Mara_StudioWorld")
scene.world.use_nodes = True
scene.world.node_tree.nodes.get("Background").inputs[0].default_value = (.19, .22, .27, 1)
scene.world.node_tree.nodes.get("Background").inputs[1].default_value = .35


def select(objects):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.hide_set(False)
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[-1]


def attribute(mesh, name, values):
    att = mesh.color_attributes.new(name=name, domain="POINT", type="FLOAT_COLOR")
    rgba = np.ones((len(values), 4), dtype=np.float32)
    rgba[:, :3] = values
    att.data.foreach_set("color", rgba.reshape(-1))


def source_material():
    mat = bpy.data.materials.new("M_MaraSourceVertexMaterials")
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    shader = nodes.get("Principled BSDF")
    base = nodes.new("ShaderNodeAttribute")
    base.attribute_name = "Mara_Base"
    base.name = "SourceBase"
    orm = nodes.new("ShaderNodeAttribute")
    orm.attribute_name = "Mara_ORM"
    orm.name = "SourceORM"
    separate = nodes.new("ShaderNodeSeparateColor")
    links.new(base.outputs["Color"], shader.inputs["Base Color"])
    links.new(orm.outputs["Color"], separate.inputs[0])
    links.new(separate.outputs["Green"], shader.inputs["Roughness"])
    links.new(separate.outputs["Blue"], shader.inputs["Metallic"])
    return mat


data = np.load(out / "operator-surface.npz")
vertices = np.array(data["vertices"], dtype=np.float32)
raw_bounds = [vertices.min(0).tolist(), vertices.max(0).tolist()]
vertices[:, :2] -= (vertices.max(0)[:2] + vertices.min(0)[:2]) * .5
vertices[:, 2] -= vertices[:, 2].min()
height_scale = 1.78 / vertices[:, 2].max()
vertices *= height_scale
angle = math.radians(args.yaw)
xy = vertices[:, :2].copy()
vertices[:, 0] = xy[:, 0] * math.cos(angle) - xy[:, 1] * math.sin(angle)
vertices[:, 1] = xy[:, 0] * math.sin(angle) + xy[:, 1] * math.cos(angle)
mesh = bpy.data.meshes.new("Mara_SourceSurface")
mesh.from_pydata(vertices.tolist(), [], data["faces"].tolist())
mesh.update()
for face in mesh.polygons:
    face.use_smooth = True
raw_attrs = np.array(data["attrs"], dtype=np.float32)
# Sparse trilinear queries include empty neighbours. Alpha measures the occupied
# coverage and must normalise the sampled material values before baking. Without
# this step, empty voxels create artificial black flecks on the surface.
coverage = np.clip(raw_attrs[:, 5], 0, 1)
normalized_attrs = np.clip(raw_attrs[:, :5] / np.maximum(coverage[:, None], .025), 0, 1)
valid_indices = np.flatnonzero(coverage >= .15)
tree = KDTree(len(valid_indices))
for index in valid_indices:
    tree.insert(vertices[index], int(index))
tree.balance()
for index in np.flatnonzero(coverage < .15):
    _, nearest, _ = tree.find(vertices[index])
    normalized_attrs[index] = normalized_attrs[nearest]
rgb = normalized_attrs[:, :3]
linear = np.where(rgb <= .04045, rgb / 12.92, ((rgb + .055) / 1.055) ** 2.4)
attribute(mesh, "Mara_Base", linear)
attribute(mesh, "Mara_ORM", np.stack((np.ones(len(rgb)), normalized_attrs[:, 4], normalized_attrs[:, 3]), axis=-1))
clean = bmesh.new()
clean.from_mesh(mesh)
bmesh.ops.remove_doubles(clean, verts=list(clean.verts), dist=.00001)
bmesh.ops.dissolve_degenerate(clean, edges=list(clean.edges), dist=.000005)
bmesh.ops.recalc_face_normals(clean, faces=list(clean.faces))
clean.to_mesh(mesh)
clean.free()
mesh.update()
high = bpy.data.objects.new("Mara_SourceHigh", mesh)
bpy.context.collection.objects.link(high)
high.data.materials.append(source_material())
select([high])
# Reconstruct a continuous shell; the raw flexible-dual-grid surface has
# narrow cracks around the trouser panels. Transfer point materials directly
# from the corrected source rather than baking dark crevices into the result.
old_base = np.array([p.color[:3] for p in high.data.color_attributes["Mara_Base"].data])
old_orm = np.array([p.color[:3] for p in high.data.color_attributes["Mara_ORM"].data])
old_tree = KDTree(len(high.data.vertices))
for v in high.data.vertices:
    old_tree.insert(v.co, v.index)
old_tree.balance()
thickness = high.modifiers.new("CloseRawSheets", "SOLIDIFY")
thickness.thickness = args.shell_thickness
thickness.offset = 0
bpy.ops.object.modifier_apply(modifier=thickness.name)
shell = high.modifiers.new("RepairRawGridCracks", "REMESH")
shell.mode = "VOXEL"
shell.voxel_size = args.voxel_size
shell.use_smooth_shade = True
bpy.ops.object.modifier_apply(modifier=shell.name)
smooth = high.modifiers.new("RelaxVoxelSurface", "SMOOTH")
smooth.factor = .55
smooth.iterations = 3
bpy.ops.object.modifier_apply(modifier=smooth.name)
# Solidifying a cracked sheet produces an exterior and an internal shell. Keep
# only outward closed components before decimation so inner triangles cannot
# cross the visible skin when the two surfaces simplify independently.
outer = bmesh.new()
outer.from_mesh(high.data)
outer.normal_update()
unvisited = set(outer.faces)
removed_inner_faces = 0
component_stats = []
while unvisited:
    initial = unvisited.pop()
    stack, component = [initial], [initial]
    volume = 0.0
    while stack:
        face = stack.pop()
        volume += face.normal.dot(face.calc_center_median()) * face.calc_area() / 3
        for edge in face.edges:
            for neighbour in edge.link_faces:
                if neighbour in unvisited:
                    unvisited.remove(neighbour)
                    stack.append(neighbour)
                    component.append(neighbour)
    keep = volume > 1e-8 and len(component) > 80
    component_stats.append({"faces": len(component), "signed_volume_m3": volume, "kept": keep})
    if not keep:
        removed_inner_faces += len(component)
        bmesh.ops.delete(outer, geom=component, context="FACES")
outer.to_mesh(high.data)
outer.free()
high.data.update()
print("MARA_SHELL_COMPONENTS " + json.dumps(sorted(component_stats, key=lambda item: -item["faces"])[:15]))
clean_base = np.empty((len(high.data.vertices), 3), dtype=np.float32)
clean_orm = np.empty_like(clean_base)
for v in high.data.vertices:
    neighbours = old_tree.find_n(v.co, 3)
    weights = np.array([1 / max(item[2], .0001) ** 2 for item in neighbours])
    weights /= weights.sum()
    old_indices = [item[1] for item in neighbours]
    clean_base[v.index] = (old_base[old_indices] * weights[:, None]).sum(0)
    clean_orm[v.index] = (old_orm[old_indices] * weights[:, None]).sum(0)
# Restore front-facing facial/clothing detail by projecting the exact original
# reference into the same alpha-cropped orthographic coordinates used by the
# model conditioner. Preserve generated side/rear materials with a normal mask.
reference = Path(__file__).resolve().parents[1] / "assets/production/operator-v001/Mara_SourceReference.png"
source_image = bpy.data.images.load(str(reference), check_existing=True)
source_image.colorspace_settings.name = "Non-Color"
width, height = source_image.size
pixels = np.empty(width * height * 4, dtype=np.float32)
source_image.pixels.foreach_get(pixels)
pixels = pixels.reshape(height, width, 4)
ys, xs = np.nonzero(pixels[:, :, 3] > .8)
center_x, center_v = (xs.min() + xs.max()) * .5, (ys.min() + ys.max()) * .5
extent = max(xs.max() - xs.min(), ys.max() - ys.min())
coordinates = np.array([v.co[:] for v in high.data.vertices])
normals = np.array([v.normal[:] for v in high.data.vertices])
raw_center_x = (raw_bounds[0][0] + raw_bounds[1][0]) * .5
u = np.clip(center_x + (coordinates[:, 1] / height_scale + raw_center_x) * extent, 0, width - 1.01)
v = np.clip(center_v + (coordinates[:, 2] / height_scale + raw_bounds[0][2]) * extent, 0, height - 1.01)
ix, iy = u.astype(int), v.astype(int)
fx, fy = (u - ix)[:, None], (v - iy)[:, None]
sample = (pixels[iy, ix] * (1 - fx) * (1 - fy) + pixels[iy, ix + 1] * fx * (1 - fy)
          + pixels[iy + 1, ix] * (1 - fx) * fy + pixels[iy + 1, ix + 1] * fx * fy)
sample_rgb = np.where(sample[:, :3] <= .04045, sample[:, :3] / 12.92, ((sample[:, :3] + .055) / 1.055) ** 2.4)
mix = (np.clip((normals[:, 0] - .25) / .35, 0, 1) * (sample[:, 3] > .8)
       * (coordinates[:, 2] < 1.45) * .9)[:, None]
clean_base = clean_base * (1 - mix) + sample_rgb * mix
# Register the original facial landmarks independently of the whole-body
# projection. The generated head's proportions differ from the conditioning
# silhouette; using one global projection duplicates/offsets the eyes.
fu = np.clip(520 + coordinates[:, 1] * 650, 0, width - 1.01)
fv = np.clip((height - 139) + (coordinates[:, 2] - 1.600) * 1000, 0, height - 1.01)
fix, fiy = fu.astype(int), fv.astype(int)
ffx, ffy = (fu - fix)[:, None], (fv - fiy)[:, None]
face_sample = (pixels[fiy, fix] * (1 - ffx) * (1 - ffy) + pixels[fiy, fix + 1] * ffx * (1 - ffy)
               + pixels[fiy + 1, fix] * (1 - ffx) * ffy + pixels[fiy + 1, fix + 1] * ffx * ffy)
face_rgb = np.where(face_sample[:, :3] <= .04045, face_sample[:, :3] / 12.92,
                    ((face_sample[:, :3] + .055) / 1.055) ** 2.4)
face_mix = (np.clip((normals[:, 0] - .20) / .25, 0, 1)
    * np.clip((coordinates[:, 0] - .055) / .025, 0, 1)
    * np.clip((coordinates[:, 2] - 1.49) / .025, 0, 1)
    * np.clip((1.675 - coordinates[:, 2]) / .02, 0, 1)
    * np.clip((.082 - np.abs(coordinates[:, 1])) / .017, 0, 1))[:, None]
clean_base = clean_base * (1 - face_mix) + face_rgb * face_mix
hair = ((coordinates[:, 2] > 1.70) | ((coordinates[:, 2] > 1.51)
        & ((coordinates[:, 0] < .015) | (np.abs(coordinates[:, 1]) > .080))))
hair_detail = np.clip(clean_base[hair].mean(1) / .07, .35, 2.0)
clean_base[hair] = np.array((.045, .022, .012)) * hair_detail[:, None]
source_image.colorspace_settings.name = "sRGB"
for name in ("Mara_Base", "Mara_ORM"):
    if name in high.data.color_attributes:
        high.data.color_attributes.remove(high.data.color_attributes[name])
attribute(high.data, "Mara_Base", clean_base)
attribute(high.data, "Mara_ORM", clean_orm)


def light(name, location, power, size, color):
    data = bpy.data.lights.new(name, "AREA")
    data.energy = power
    data.shape = "DISK"
    data.size = size
    data.color = color
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    obj.location = location
    obj.rotation_euler = (Vector((0, 0, 1)) - obj.location).to_track_quat("-Z", "Y").to_euler()


def render(name, location, ortho=2.2, target=(0, 0, .95)):
    cam = bpy.data.cameras.new(name)
    obj = bpy.data.objects.new(name, cam)
    bpy.context.collection.objects.link(obj)
    obj.location = location
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat("-Z", "Y").to_euler()
    cam.type = "ORTHO"
    cam.ortho_scale = ortho
    scene.camera = obj
    scene.render.resolution_x = 800
    scene.render.resolution_y = 1000
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = str(out / "renders" / (name + ".png"))
    bpy.ops.render.render(write_still=True)


light("Key", (3, -3, 4), 450, 3, (1, .87, .73))
light("Fill", (-2, 3, 2.4), 280, 3, (.73, .85, 1))
light("Edge", (-3, -2, 3.3), 320, 2, (1, .69, .4))
if args.inspect_only:
    render("source_front_candidate", (4, 0, 1.05))
    render("source_rear_candidate", (-4, 0, 1.05))
    render("source_side_candidate", (0, -4, 1.05))
    (out / "source-orientation.json").write_text(json.dumps({"raw_bounds": raw_bounds,
        "yaw_degrees": args.yaw, "height_scale": float(height_scale),
        "bounds_min": vertices.min(0).tolist(), "bounds_max": vertices.max(0).tolist()}, indent=2))
    bpy.ops.wm.save_as_mainfile(filepath=str(out / "Mara_Inspection.blend"))
    raise SystemExit(0)

# Preserve the high surface for texture/normal transfer and editable source.
low = high.copy()
low.data = high.data.copy()
low.name = "MaraOperator"
bpy.context.collection.objects.link(low)
select([low])
modifier = low.modifiers.new("Adaptive_MVP_Surface", "DECIMATE")
low.data.calc_loop_triangles()
modifier.ratio = min(1, 110000 / len(low.data.loop_triangles))
modifier.use_collapse_triangulate = True
bpy.ops.object.modifier_apply(modifier=modifier.name)
bpy.ops.object.mode_set(mode="EDIT")
bpy.ops.mesh.select_all(action="SELECT")
bpy.ops.mesh.remove_doubles(threshold=.00002)
editmesh = bmesh.from_edit_mesh(low.data)
bmesh.ops.recalc_face_normals(editmesh, faces=list(editmesh.faces))
bmesh.update_edit_mesh(low.data)
bpy.ops.uv.smart_project(angle_limit=math.radians(65), island_margin=.002, area_weight=.6)
bpy.ops.object.mode_set(mode="OBJECT")
low.data.validate(verbose=False, clean_customdata=False)
low.data.update()

low.data.materials.clear()
target = bpy.data.materials.new("M_MaraOperator")
target.use_nodes = True
low.data.materials.append(target)
target_node = target.node_tree.nodes.new("ShaderNodeTexImage")
source_nodes = target.node_tree.nodes
source_links = target.node_tree.links
for name, attr_name in (("SourceBase", "Mara_Base"), ("SourceORM", "Mara_ORM")):
    attr_node = source_nodes.new("ShaderNodeAttribute")
    attr_node.name = name
    attr_node.attribute_name = attr_name
source_output = source_nodes.get("Material Output")
emission = source_nodes.new("ShaderNodeEmission")
source_links.new(emission.outputs[0], source_output.inputs["Surface"])
scene.render.bake.use_selected_to_active = False
scene.render.bake.cage_extrusion = .02
scene.render.bake.max_ray_distance = .05
scene.render.bake.margin = 12
scene.cycles.samples = 1
textures = {}
size = args.texture_size
base_values = np.array([item.color[:3] for item in low.data.color_attributes["Mara_Base"].data])
orm_values = np.array([item.color[:3] for item in low.data.color_attributes["Mara_ORM"].data])
uv_values = np.array([item.uv[:] for item in low.data.uv_layers.active.data]) * size
atlas = np.zeros((size, size, 6), dtype=np.float32)
mask = np.zeros((size, size), dtype=bool)
# Deterministic CPU triangle rasterization of the same interpolated vertex
# material displayed in the diagnostic. This avoids Cycles' self-bake misses
# on the generated shell while preserving the authored UV chart exactly.
for polygon in low.data.polygons:
    loops = list(polygon.loop_indices)
    if len(loops) != 3:
        raise RuntimeError("Expected triangulated export surface")
    points = uv_values[loops]
    lo = np.maximum(0, np.floor(points.min(0) - .5).astype(int))
    hi = np.minimum(size - 1, np.ceil(points.max(0) - .5).astype(int))
    if np.any(hi < lo):
        continue
    xx, yy = np.meshgrid(np.arange(lo[0], hi[0] + 1) + .5, np.arange(lo[1], hi[1] + 1) + .5)
    a, b, c = points
    denominator = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1])
    if abs(denominator) < 1e-9:
        continue
    wa = ((b[1] - c[1]) * (xx - c[0]) + (c[0] - b[0]) * (yy - c[1])) / denominator
    wb = ((c[1] - a[1]) * (xx - c[0]) + (a[0] - c[0]) * (yy - c[1])) / denominator
    wc = 1 - wa - wb
    inside = (wa >= -1e-6) & (wb >= -1e-6) & (wc >= -1e-6)
    indices = [low.data.loops[loop].vertex_index for loop in loops]
    values = np.concatenate((base_values[indices], orm_values[indices]), axis=1)
    interpolated = wa[..., None] * values[0] + wb[..., None] * values[1] + wc[..., None] * values[2]
    part = atlas[lo[1]:hi[1] + 1, lo[0]:hi[0] + 1]
    part[inside] = interpolated[inside]
    mask[lo[1]:hi[1] + 1, lo[0]:hi[0] + 1] |= inside
covered_pixels = int(mask.sum())
for _ in range(16):
    for axis, shift in ((0, 1), (0, -1), (1, 1), (1, -1)):
        near_mask = np.roll(mask, shift, axis=axis)
        missing = ~mask & near_mask
        atlas[missing] = np.roll(atlas, shift, axis=axis)[missing]
        mask[missing] = True
for suffix, node_name, colorspace in (("BaseColor", "SourceBase", "sRGB"), ("ORM", "SourceORM", "Non-Color")):
    tex = bpy.data.images.new("Mara_" + suffix, width=args.texture_size, height=args.texture_size, alpha=False)
    tex.colorspace_settings.name = "Non-Color"
    target_node.image = tex
    target.node_tree.nodes.active = target_node
    rgba = np.ones((size, size, 4), dtype=np.float32)
    rgba[:, :, :3] = atlas[:, :, :3] if suffix == "BaseColor" else atlas[:, :, 3:]
    if suffix == "BaseColor":
        values = rgba[:, :, :3]
        rgba[:, :, :3] = np.where(values <= .0031308, values * 12.92, 1.055 * np.maximum(values, 0) ** (1 / 2.4) - .055)
    tex.pixels.foreach_set(rgba.reshape(-1))
    tex.filepath_raw = str(out / "textures" / ("Mara_" + suffix + ".png"))
    tex.file_format = "PNG"
    tex.save()
    tex.colorspace_settings.name = colorspace
    tex.reload()
    textures[suffix] = tex

source_links.new(source_nodes.get("Principled BSDF").outputs[0], source_output.inputs["Surface"])
normal = bpy.data.images.new("Mara_Normal", width=args.texture_size, height=args.texture_size, alpha=False)
normal.colorspace_settings.name = "Non-Color"
target_node.image = normal
target.node_tree.nodes.active = target_node
flat_normal = np.ones((args.texture_size * args.texture_size, 4), dtype=np.float32)
flat_normal[:, :2] = .5
normal.pixels.foreach_set(flat_normal.reshape(-1))
normal.filepath_raw = str(out / "textures" / "Mara_Normal.png")
normal.file_format = "PNG"
normal.save()
textures["Normal"] = normal
target.node_tree.nodes.remove(target_node)
target.node_tree.nodes.remove(emission)
nodes, links = target.node_tree.nodes, target.node_tree.links
shader = nodes.get("Principled BSDF")
for suffix, tex in textures.items():
    node = nodes.new("ShaderNodeTexImage")
    node.name = "Mara_" + suffix
    node.image = tex
    if suffix == "BaseColor":
        links.new(node.outputs["Color"], shader.inputs["Base Color"])
    elif suffix == "ORM":
        separate = nodes.new("ShaderNodeSeparateColor")
        links.new(node.outputs["Color"], separate.inputs[0])
        links.new(separate.outputs["Green"], shader.inputs["Roughness"])
        links.new(separate.outputs["Blue"], shader.inputs["Metallic"])
    else:
        normalmap = nodes.new("ShaderNodeNormalMap")
        normalmap.inputs["Strength"].default_value = 1
        links.new(node.outputs["Color"], normalmap.inputs["Color"])
        links.new(normalmap.outputs["Normal"], shader.inputs["Normal"])
    tex.pack()
high.hide_render = True
high.hide_set(True)
scene.cycles.samples = 16
select([low])
render("surface_front", (3.5, -.7, 1.15))
render("surface_rear", (-3.5, .7, 1.15))
render("surface_face", (3.5, -.3, 1.60), .55, (0, 0, 1.58))
high.data.calc_loop_triangles()
report = {"source": "original image_gen + offline TRELLIS direct arrays", "blender": bpy.app.version_string,
    "render_device": "CPU", "source_vertices": len(high.data.vertices), "source_triangles": len(high.data.loop_triangles),
    "vertices": len(low.data.vertices), "triangles": len(low.data.polygons), "height_m": 1.78,
    "yaw_degrees": args.yaw, "texture_size": args.texture_size,
    "bounds_min_m": [min(v.co[a] for v in low.data.vertices) for a in range(3)],
    "bounds_max_m": [max(v.co[a] for v in low.data.vertices) for a in range(3)],
    "normal_convention": "Flat tangent normal; either green convention identical", "normal_strength": 1,
    "surface_processing": "coverage-normalized PBR; closed shell/voxel cleanup with three relaxation passes; adaptive 110k-triangle reduction; original front clothing and landmark-registered face projected with outward-normal masks; restrained brown hair palette correction",
    "shell_thickness_m": args.shell_thickness, "voxel_size_m": args.voxel_size,
    "bake_processing": "Original CPU UV triangle rasterization of transferred POINT color/material attributes; 16-iteration padding; geometry supplies shape normals",
    "covered_texture_pixels": covered_pixels,
    "orm": "linear R=1, G=roughness, B=metallic", "base_color": "sRGB", "visual_approval": "pending"}
(out / "surface-report.json").write_text(json.dumps(report, indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(out / "Mara_Surface.blend"))
print("MARA_SURFACE " + json.dumps(report))

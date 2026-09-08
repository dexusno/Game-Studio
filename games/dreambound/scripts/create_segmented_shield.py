"""Author the original six-piece Dreambound shield; run in background Blender.

Writes only art/segmented/. Existing kit materials, UV helpers, core and forearm
are read-only dependencies. Face +X, back -X, centimeters, common origin 0.
The base shard points +Z; repeat by rotations of 60 degrees around X.
"""
import bpy
import bmesh
import hashlib
import json
import math
import sys
from pathlib import Path
from mathutils import Vector

GAME = Path(__file__).resolve().parents[1]
OUT = GAME / "art" / "segmented"
OUT.mkdir(parents=True, exist_ok=True)
scene = bpy.context.scene
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
scene.unit_settings.system = "METRIC"
scene.unit_settings.scale_length = .01
bpy.context.preferences.filepaths.save_version = 0
PALETTE = json.loads((GAME / "art/generated/materials.json").read_text(encoding="utf-8"))
uv_namespace = {}
uv_source = GAME / "art/source/uv_tools.py"
exec(compile(uv_source.read_text(encoding="utf-8"), str(uv_source), "exec"), uv_namespace)
single_craft_uv = uv_namespace["single_craft_uv"]
project_face_uv = uv_namespace["project_face_uv"]
repair_mesh_uv = uv_namespace["repair_mesh_uv"]
native_tangent_audit = uv_namespace["native_tangent_audit"]


def material(name):
    """Preview the shared Unreal palette with the same value/wear/recess masks."""
    spec = PALETTE[name]
    result = bpy.data.materials.new(name)
    result.use_nodes = True
    nodes, links = result.node_tree.nodes, result.node_tree.links
    nodes.clear()
    bsdf = nodes.new("ShaderNodeBsdfPrincipled")
    output = nodes.new("ShaderNodeOutputMaterial")
    links.new(bsdf.outputs["BSDF"], output.inputs["Surface"])
    bsdf.inputs["Metallic"].default_value = spec["metallic"]
    bsdf.inputs["Roughness"].default_value = spec["roughness"]
    result.diffuse_color = (*spec["color"], 1)
    color = nodes.new("ShaderNodeVertexColor")
    color.layer_name = "Color"
    channels = nodes.new("ShaderNodeSeparateColor")
    links.new(color.outputs["Color"], channels.inputs["Color"])
    worn = nodes.new("ShaderNodeMixRGB")
    worn.inputs[1].default_value = (*spec["color"], 1)
    worn.inputs[2].default_value = (*spec["wear_color"], 1)
    links.new(channels.outputs["Green"], worn.inputs[0])
    aged = nodes.new("ShaderNodeMixRGB")
    aged.inputs[2].default_value = (*spec["weather_color"], 1)
    links.new(worn.outputs[0], aged.inputs[1])
    amount = nodes.new("ShaderNodeMath")
    amount.operation = "MULTIPLY"
    amount.inputs[1].default_value = spec.get("weather_amount", .48)
    links.new(channels.outputs["Blue"], amount.inputs[0])
    links.new(amount.outputs[0], aged.inputs[0])
    painted = nodes.new("ShaderNodeMixRGB")
    painted.blend_type = "MULTIPLY"
    painted.inputs[0].default_value = 1
    links.new(aged.outputs[0], painted.inputs[1])
    links.new(channels.outputs["Red"], painted.inputs[2])
    links.new(painted.outputs[0], bsdf.inputs["Base Color"])
    texture_path = GAME / "art/generated" / (spec.get("normal_texture", "T_SculptedNormal") + ".png")
    if texture_path.is_file():
        texture = nodes.new("ShaderNodeTexImage")
        texture.image = bpy.data.images.load(str(texture_path), check_existing=True)
        texture.image.colorspace_settings.name = "Non-Color"
        normal = nodes.new("ShaderNodeNormalMap")
        normal.inputs["Strength"].default_value = min(.2, spec.get("normal_strength", .1))
        links.new(texture.outputs["Color"], normal.inputs["Color"])
        links.new(normal.outputs["Normal"], bsdf.inputs["Normal"])
    if spec.get("emissive"):
        bsdf.inputs["Emission Color"].default_value = (*spec["color"], 1)
        bsdf.inputs["Emission Strength"].default_value = spec["emissive"]
    return result


MATS = {name: material(name) for name in (
    "M_ShieldCeramic", "M_ShieldBronze", "M_DarkMetal", "M_Core",
    "M_Cloth", "M_Bronze", "M_Ceramic", "M_Patina")}
PARTS = []
ASSETS = {}


def active(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def finish_part(obj, mat, bevel=.2, smooth=False, recess=.05, value=.96):
    active(obj)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.materials.clear()
    obj.data.materials.append(MATS[mat])
    if bevel:
        obj.data.materials.append(MATS[mat])
        modifier = obj.modifiers.new("Authored rounded silhouette", "BEVEL")
        modifier.width = bevel
        modifier.segments = 2
        modifier.limit_method = "ANGLE"
        modifier.angle_limit = .42
        modifier.material = 1
        bpy.ops.object.modifier_apply(modifier=modifier.name)
        # Clamp-generated slivers are smaller than any authored detail, but can
        # acquire zero-length tangent edges after FBX float conversion.
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.dissolve_degenerate(bm, dist=.0002, edges=list(bm.edges))
        bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
        bm.to_mesh(obj.data)
        bm.free()
    uv = single_craft_uv(obj.data)
    col = obj.data.color_attributes.new(name="Color", type="FLOAT_COLOR", domain="CORNER")
    for face in obj.data.polygons:
        edge = face.material_index == 1
        face.material_index = 0
        face.use_smooth = smooth
        project_face_uv(obj.data, face, uv, density=76, transform=obj.matrix_world)
        for loop in face.loop_indices:
            point = obj.matrix_world @ obj.data.vertices[obj.data.loops[loop].vertex_index].co
            if abs(face.normal.x) > .65:
                uv.data[loop].uv = (.5 + point.y / 80, .5 + point.z / 80)
            # Broad purposeful top-to-bottom value; bevel wear is actual geometry.
            shade = max(.76, min(1, value + point.z * .0007))
            col.data[loop].color = (shade, .36 if edge else .008, recess * (.35 if edge else 1), 1)
    if bevel:
        obj.data.materials.pop(index=1)
        modifier = obj.modifiers.new("Broad armor face normals", "WEIGHTED_NORMAL")
        modifier.keep_sharp = True
        modifier.weight = 50
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    PARTS.append(obj)
    return obj


def mesh(name, vertices, faces, mat, bevel=.2, **kwargs):
    data = bpy.data.meshes.new(name)
    data.from_pydata(vertices, [], faces)
    data.update()
    obj = bpy.data.objects.new(name, data)
    scene.collection.objects.link(obj)
    return finish_part(obj, mat, bevel, **kwargs)


def polar(x, radius, degrees):
    a = math.radians(degrees)
    return (x, radius * math.sin(a), radius * math.cos(a))


def shell(name, rows, mat, bevel=.25, steps=16, crown=0, **kwargs):
    """Closed swept armor: radius, left/right angle, front/back X at each row."""
    vertices = []
    for side in (3, 4):
        for radius, left, right, front, back in rows:
            for i in range(steps + 1):
                t = i / steps
                x = (front if side == 3 else back) + crown * math.sin(t * math.pi)
                vertices.append(polar(x, radius, left + (right - left) * t))
    width = steps + 1
    layer = len(rows) * width
    faces = []
    for side in range(2):
        off = side * layer
        for row in range(len(rows) - 1):
            for i in range(steps):
                a = off + row * width + i
                faces.append((a, a + 1, a + width + 1, a + width))
    for row in (0, len(rows) - 1):
        for i in range(steps):
            a = row * width + i
            faces.append((a, a + 1, layer + a + 1, layer + a))
    for edge in (0, steps):
        for row in range(len(rows) - 1):
            a = row * width + edge
            faces.append((a, a + width, layer + a + width, layer + a))
    return mesh(name, vertices, faces, mat, bevel, **kwargs)


def arc(name, r1, r2, start, end, x, depth, mat, bevel=.13, steps=14, **kwargs):
    return shell(name, [(r1, start, end, x + depth / 2, x - depth / 2),
                        (r2, start, end, x + depth / 2, x - depth / 2)],
                 mat, bevel, steps, **kwargs)


def box(name, loc, dims, mat, bevel=.25, roll=0, **kwargs):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc, rotation=(math.radians(roll), 0, 0))
    obj = bpy.context.object
    obj.name = name
    obj.scale = dims
    return finish_part(obj, mat, bevel, **kwargs)


def cylinder(name, loc, radius, depth, mat, axis="X", sides=16, bevel=.10, **kwargs):
    rotation = {"X": (0, math.pi / 2, 0), "Y": (math.pi / 2, 0, 0), "Z": (0, 0, 0)}[axis]
    bpy.ops.mesh.primitive_cylinder_add(vertices=sides, radius=radius, depth=depth,
                                     location=loc, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    return finish_part(obj, mat, bevel, smooth=True, **kwargs)


def ring(name, r1, r2, x, depth, mat, bevel=.16, sides=48):
    profile = [(x - depth / 2, r1), (x - depth / 2, r2),
               (x + depth / 2, r2), (x + depth / 2, r1)]
    vertices = [polar(axial, radius, i * 360 / sides)
                for axial, radius in profile for i in range(sides)]
    faces = []
    for row in range(4):
        for i in range(sides):
            next_i, next_row = (i + 1) % sides, (row + 1) % 4
            faces.append((row * sides + i, row * sides + next_i,
                          next_row * sides + next_i, next_row * sides + i))
    return mesh(name, vertices, faces, mat, bevel)


def fastener(name, radius, angle, x, front=False, size=.8):
    sign = 1 if front else -1
    cylinder(name + " bronze countersink", polar(x, radius, angle), size, .32,
             "M_ShieldBronze", sides=16, bevel=.10, recess=.10)
    cylinder(name + " inset hex", polar(x + sign * .14, radius, angle), size * .56,
             .20, "M_DarkMetal", sides=6, bevel=.035, recess=.16)
    box(name + " cut head", polar(x + sign * .25, radius, angle), (.035, size * .6, .09),
        "M_ShieldBronze", bevel=.018, roll=-angle)


def finalize(name, slots, pivot, collisions=()):
    global PARTS
    active(PARTS[0])
    for part in PARTS:
        part.select_set(True)
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = obj.data.name = name
    scene.cursor.location = (0, 0, 0)
    bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    existing = [m.name for m in obj.data.materials]
    face_materials = [existing[face.material_index] for face in obj.data.polygons]
    obj.data.materials.clear()
    for slot in slots:
        obj.data.materials.append(MATS[slot])
    for face, original in zip(obj.data.polygons, face_materials):
        face.material_index = slots.index(original)
    repair_count = repair_mesh_uv(obj.data)
    audit = native_tangent_audit(obj.data)
    if audit["nearly_zero_tangent_loops_on_valid_geometry"]:
        raise RuntimeError("Invalid tangents: " + name + ": " + str(audit))
    obj.data.calc_loop_triangles()
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    boundary = sum(edge.is_boundary for edge in bm.edges)
    nonmanifold = sum(not edge.is_manifold for edge in bm.edges)
    bm.free()
    # Ring helper duplicates its closing seam; merge coincident seam vertices.
    # Done before export so every component is a closed physical volume.
    if nonmanifold:
        active(obj)
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.mesh.remove_doubles(threshold=.00001)
        bpy.ops.object.mode_set(mode="OBJECT")
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
        bm.to_mesh(obj.data)
        bm.free()
        repair_count += repair_mesh_uv(obj.data)
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        boundary = sum(edge.is_boundary for edge in bm.edges)
        nonmanifold = sum(not edge.is_manifold for edge in bm.edges)
        bm.free()
    obj.data.calc_loop_triangles()
    bounds = [Vector(p) for p in obj.bound_box]
    lo = [min(p[i] for p in bounds) for i in range(3)]
    hi = [max(p[i] for p in bounds) for i in range(3)]
    colors = [entry.color for entry in obj.data.color_attributes["Color"].data]
    meta = {"name": name, "dimensions_cm": [round(hi[i] - lo[i], 5) for i in range(3)],
            "bounds_min_cm": list(lo), "bounds_max_cm": list(hi),
            "vertices": len(obj.data.vertices), "triangles": len(obj.data.loop_triangles),
            "pivot": pivot, "materials": slots, "collision_hulls": len(collisions),
            "recommended_attachment_cm": [0, 0, 0],
            "vertex_channels": {"R": "painted value", "G": "exposed edge wear", "B": "recess/weathering", "A": "opaque"},
            "uv_channels": [layer.name for layer in obj.data.uv_layers],
            "uv_faces_repaired": repair_count, "tangent_audit": audit,
            "boundary_edges": boundary, "nonmanifold_edges": nonmanifold,
            "vertex_channel_min": [min(c[i] for c in colors) for i in range(4)],
            "vertex_channel_max": [max(c[i] for c in colors) for i in range(4)]}
    active(obj)
    for i, collider in enumerate(collisions):
        collider.name = f"UCX_{name}_{i:02d}"
        collider.select_set(True)
    images = [(node, node.image) for mat in obj.data.materials for node in mat.node_tree.nodes if node.type == "TEX_IMAGE"]
    for node, _ in images:
        node.image = None
    try:
        bpy.ops.export_scene.fbx(filepath=str(OUT / (name + ".fbx")), use_selection=True,
            object_types={"MESH"}, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
            axis_forward="X", axis_up="Z", use_space_transform=True, bake_space_transform=True,
            use_mesh_modifiers=True, mesh_smooth_type="FACE", add_leaf_bones=False,
            bake_anim=False, use_custom_props=False, path_mode="STRIP")
    finally:
        for node, image in images:
            node.image = image
    for collider in collisions:
        bpy.data.objects.remove(collider, do_unlink=True)
    meta["sha256"] = hashlib.sha256((OUT / (name + ".fbx")).read_bytes()).hexdigest()
    ASSETS[name] = {"object": obj, "meta": meta}
    PARTS = []
    return obj


def audit_exports():
    """Read back the actual FBXs, including signed extent, UV and normal checks."""
    metadata = json.loads((OUT / "asset-metadata.json").read_text(encoding="utf-8"))
    report = {"tool": bpy.app.version_string, "kind": "Blender FBX roundtrip; not Unreal or playtest evidence", "assets": {}}
    for name, meta in metadata.items():
        before = set(bpy.data.objects)
        bpy.ops.import_scene.fbx(filepath=str(OUT / (name + ".fbx")), use_anim=False)
        imported = [obj for obj in bpy.data.objects if obj not in before]
        colliders = [obj for obj in imported if obj.type == "MESH" and obj.name.startswith("UCX_")]
        if len(colliders) != meta["collision_hulls"]:
            raise RuntimeError("FBX custom collision count mismatch: " + name)
        meshes = [obj for obj in imported if obj.type == "MESH" and not obj.name.startswith("UCX_")]
        if len(meshes) != 1:
            raise RuntimeError("Expected exactly one mesh in " + name)
        obj = meshes[0]
        points = [obj.matrix_world @ v.co for v in obj.data.vertices]
        lo = [min(p[i] for p in points) for i in range(3)]
        hi = [max(p[i] for p in points) for i in range(3)]
        if any(abs(lo[i] - meta["bounds_min_cm"][i]) > .01 or abs(hi[i] - meta["bounds_max_cm"][i]) > .01 for i in range(3)):
            raise RuntimeError("FBX signed extent/orientation mismatch: " + name)
        if obj.matrix_world.translation.length > .001:
            raise RuntimeError("FBX pivot shifted: " + name)
        slots = [m.name.split(".")[0] for m in obj.data.materials]
        if slots != meta["materials"]:
            raise RuntimeError("FBX material order mismatch: " + name + str(slots))
        if len(obj.data.uv_layers) != 1:
            raise RuntimeError("Expected one authored export UV channel: " + name)
        if uv_namespace["uv_defects"](obj.data):
            raise RuntimeError("Collapsed UV after export: " + name)
        tangent = native_tangent_audit(obj.data)
        if tangent["nearly_zero_tangent_loops_on_valid_geometry"]:
            raise RuntimeError("Invalid tangent after export: " + name)
        # Test winding for each disconnected physical part, rather than hiding a
        # reversed bolt or armor skin behind a positive total volume.
        adjacency = [[] for _ in obj.data.vertices]
        for edge in obj.data.edges:
            a, b = edge.vertices
            adjacency[a].append(b)
            adjacency[b].append(a)
        groups, group_count = {}, 0
        for vertex in range(len(adjacency)):
            if vertex in groups:
                continue
            pending = [vertex]
            groups[vertex] = group_count
            while pending:
                for other in adjacency[pending.pop()]:
                    if other not in groups:
                        groups[other] = group_count
                        pending.append(other)
            group_count += 1
        volumes = [0.0] * group_count
        obj.data.calc_loop_triangles()
        for triangle in obj.data.loop_triangles:
            a, b, c = [obj.data.vertices[i].co for i in triangle.vertices]
            volumes[groups[triangle.vertices[0]]] += a.dot(b.cross(c)) / 6
        if any(volume <= 1e-8 for volume in volumes):
            raise RuntimeError("Non-outward/zero-volume physical component: " + name + str(volumes))
        color = obj.data.color_attributes.get("Color")
        if color is None or min(item.color[0] for item in color.data) < .70:
            raise RuntimeError("Export lost painted value masks: " + name)
        report["assets"][name] = {"status": "pass", "bounds_min_cm": lo, "bounds_max_cm": hi,
            "pivot_cm": list(obj.matrix_world.translation), "materials": slots,
            "uv_channels": [layer.name for layer in obj.data.uv_layers], "tangent_audit": tangent,
            "closed_outward_components": group_count, "minimum_component_volume_cm3": min(volumes),
            "sha256": hashlib.sha256((OUT / (name + ".fbx")).read_bytes()).hexdigest()}
        for obj in imported:
            bpy.data.objects.remove(obj, do_unlink=True)
    (OUT / "export-checks.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


def portable_source():
    """Drop unused factory brush libraries and pack the existing small textures."""
    for brush in list(bpy.data.brushes):
        bpy.data.brushes.remove(brush, do_unlink=True)
    used_materials = {mat for obj in bpy.data.objects if obj.type == "MESH" for mat in obj.data.materials if mat}
    for mat in list(bpy.data.materials):
        if mat not in used_materials:
            bpy.data.materials.remove(mat, do_unlink=True)
    bpy.data.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)
    # Appended review objects are local copies. The remaining library IDs are
    # factory brush catalogs, never part of this model's dependencies.
    for library in list(bpy.data.libraries):
        bpy.data.libraries.remove(library, do_unlink=True)
    for image in bpy.data.images:
        if image.source == "FILE" and image.has_data:
            image.pack()
            image.filepath = "//../generated/" + Path(image.filepath).name
    if len(bpy.data.libraries):
        raise RuntimeError("Review source retained an external library; do not publish it")


if "--audit-existing" in sys.argv:
    print("SEGMENTED_EXPORT_AUDIT " + json.dumps(audit_exports(), separators=(",", ":")))
    bpy.ops.wm.open_mainfile(filepath=str(OUT / "Segmented_Shield.blend"))
    portable_source()
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "Segmented_Shield.blend"), compress=True)
    raise SystemExit(0)


# Broad crescent body. The changing cross-section is thick at its docking end,
# tapered at its cutting edge, and deliberately cropped at the two seam tips.
shell("Forged dark crescent chassis", [
    (13.5, -15, 23, 3.0, -2.3), (16.6, -28.5, 28.5, 4.2, -3.4),
    (25.5, -28.5, 28.5, 3.7, -3.1), (34.5, -28.5, 28.5, 2.1, -2.1),
    (37.9, -25.5, 25.5, .40, -.40)], "M_DarkMetal", .19, value=.87, recess=.11)
shell("Forward ceramic armored sweep", [
    (17.3, -25.4, 24.4, 5.1, 3.7), (25.2, -26.3, 26.0, 4.9, 3.5),
    (33.5, -26.3, 25.7, 3.9, 2.2), (36.5, -24.2, 23.3, 2.0, 1.1)],
    "M_ShieldCeramic", .34, crown=.27, value=.96)
shell("Rear overlapping outer carapace", [
    (26.0, -25.4, 21.2, -2.9, -4.9), (29.8, -26.1, 25.8, -2.7, -4.4),
    (34.8, -26.1, 25.8, -1.7, -3.4), (36.5, -23.4, 23.6, -.8, -2.1)],
    "M_ShieldCeramic", .35, crown=-.28, value=.94)
shell("Rear stepped inner carapace", [
    (16.7, -20.6, 21.8, -3.2, -5.8), (19.8, -24.5, 24.2, -3.6, -6.0),
    (25.8, -24.7, 23.8, -3.1, -5.9), (27.1, -22.6, 20.0, -3.0, -5.0)],
    "M_ShieldCeramic", .38, crown=-.2, value=.91, recess=.10)
# Real relief between the ceramic layers, with a short bronze locking tongue.
arc("Shadow beneath overlapping rear plate", 26.9, 28.3, -24.0, 21.0, -4.9, .50,
    "M_DarkMetal", .10, recess=.22, value=.82)
arc("Bronze curved overlap lip", 26.9, 27.65, -23.1, 19.3, -5.15, .54,
    "M_ShieldBronze", .15, recess=.10)
arc("Front inner bronze seat", 15.7, 17.35, -23.3, 22.4, 4.0, 1.0, "M_ShieldBronze", .18)
arc("Rear inlay recessed bed", 18.8, 20.5, -19.2, 19.0, -6.10, .44,
    "M_DarkMetal", .12, recess=.24)
# Offset, compact latches are seated in the armor, not spokes across an opening.
box("Rear offset mortise", polar(-5.9, 23.0, 16.2), (1.1, 3.0, 5.8),
    "M_ShieldBronze", .30, roll=-16.2, recess=.09)
box("Mortise inset", polar(-6.47, 23.1, 16.2), (.32, 1.55, 3.8),
    "M_DarkMetal", .16, roll=-16.2, recess=.24, value=.85)
box("Sliding ceramic latch", polar(-6.75, 23.7, 16.2), (.40, 1.12, 2.4),
    "M_ShieldCeramic", .16, roll=-16.2, value=.88)
shell("Docking tongue under inner edge", [(11.8, -7, 7, 1.0, -1.1),
    (17.3, -10, 10, 1.9, -2.0)], "M_ShieldBronze", .23, steps=6, recess=.15)
box("Docking tongue dark key", polar(-2.15, 14.4, 0), (.32, 2.2, 3.1),
    "M_DarkMetal", .12, recess=.22)
for angle in (-18.0, 17.7):
    fastener("Outer armor", 32.1, angle, -4.27, size=.85)
for angle in (-15.3, 12.8):
    fastener("Inner lock", 22.3, angle, -6.26, size=.66)
for angle in (-19, 19):
    fastener("Face rivet", 31.6, angle, 4.28, front=True, size=.72)
# Deliberate short etched index marks cut the broad face without noisy texture.
for radius, length in ((29.4, 3.1), (30.25, 2.6), (31.1, 2.1)):
    arc("Inset maker index", radius, radius + .17, -4, -4 + length, -4.60, .085,
        "M_DarkMetal", .025, steps=3, value=.83, recess=.24)
segment = finalize("SM_ShieldSegment", ["M_ShieldCeramic", "M_ShieldBronze", "M_DarkMetal"],
    "Shield center (0,0,0); face +X/back -X; base crescent centered +Z; repeat roll about X by 60 degrees")

# One independently controllable emissive mesh per physical shard. Both rear
# and front are included; nothing luminous needs to remain on a missing shard.
arc("Rear inset charge window", 19.30, 19.95, -17.8, 17.5, -6.37, .18,
    "M_Core", .07, steps=16, value=.98, recess=0)
arc("Front inset charge window", 18.0, 18.52, -16.8, 16.8, 5.45, .16,
    "M_Core", .06, steps=16, value=.98, recess=0)
for angle in (-2.8, 0, 2.8):
    box("Rear luminous readiness pip", polar(-4.97, 28.7, angle), (.20, .52, 1.0),
        "M_Core", .08, roll=-angle, value=1, recess=0)
glow = finalize("SM_ShieldSegmentGlow", ["M_Core"],
    "Exactly the segment's shield-center origin and +Z base orientation; copy its transform/state")

# Compact hub remains with the gripped hand. The aperture accepts the existing
# SM_Core at (-6,0,0); do not retain the former whole-disc SM_ShieldPlate.
ring("Held central chassis", 9.65, 14.9, .0, 6.8, "M_DarkMetal", .30)
ring("Rear bronze core socket", 9.50, 11.0, -4.35, 2.0, "M_ShieldBronze", .22)
ring("Front outer torque collar", 11.0, 14.55, 3.25, 1.2, "M_ShieldBronze", .25)
ring("Recessed front center well", 2.7, 9.7, 2.25, 1.8, "M_DarkMetal", .22)
cylinder("Front sculpted hex boss", (4.15, 0, 0), 6.4, 2.6,
    "M_ShieldCeramic", sides=6, bevel=.65)
cylinder("Boss bronze cap", (5.57, 0, 0), 2.7, .48,
    "M_ShieldBronze", sides=12, bevel=.13)
for i in range(6):
    a = i * 60
    # Each hub shoe is a short broad mechanism beneath the shard's dock.
    arc("Separate rear ceramic socket cheek", 11.2, 14.55, a - 21, a + 20,
        -3.95, 2.05, "M_ShieldCeramic", .30, steps=8, value=.90)
    arc("Dark socket clearance", 13.0, 15.0, a - 7.5, a + 7.5,
        -2.65, 1.0, "M_DarkMetal", .18, steps=5, recess=.2)
    fastener("Hub retention pin", 12.60, a + 12, -5.22, size=.64)
# Y-axis grasping bar exactly preserves the existing curled-hand placement.
for y in (-8.2, 8.2):
    box("Grip structural foot", (-4.2, y, -11.5), (4.6, 3.5, 7.2),
        "M_ShieldBronze", .75)
    box("Grip rear swept support", (-8.6, y, -12), (7.8, 2.55, 3.1),
        "M_DarkMetal", .64)
    cylinder("Grip end ferrule", (-12, y, -12), 2.05, 2.5,
        "M_ShieldBronze", axis="Y", sides=16, bevel=.23)
cylinder("Held grip bar", (-12, 0, -12), 1.64, 15.8,
    "M_DarkMetal", axis="Y", sides=16, bevel=.18)
for y in (-5.3, -3.5, -1.7, 0.1, 1.9, 3.7, 5.5):
    cylinder("Grip compression band", (-12, y, -12), 1.73, .42,
        "M_ShieldBronze", axis="Y", sides=16, bevel=.065, recess=.12)
hub = finalize("SM_ShieldHub", ["M_ShieldCeramic", "M_ShieldBronze", "M_DarkMetal"],
    "Shield center (0,0,0); remains held; grip bar center(-12,0,-12), axis Y; existing SM_Core fits(-6,0,0)")

metadata = {name: asset["meta"] for name, asset in ASSETS.items()}
(OUT / "asset-metadata.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")

# Artist inspection scene, with all six real exported pieces and the existing
# core/forearm shown for attachment fit. These two references are not reexported.
review_collection = bpy.data.collections.new("Review assembly - six real copies")
scene.collection.children.link(review_collection)
instances = []
for i in range(1, 6):
    for original in (segment, glow):
        instance = original.copy()
        instance.data = original.data
        instance.name = original.name + "_review_" + str(i)
        instance.rotation_euler.x = math.radians(i * 60)
        review_collection.objects.link(instance)
        instances.append(instance)
references = []
with bpy.data.libraries.load(str(GAME / "art/Dreambound_Kit.blend"), link=False) as (source, target):
    target.objects = [name for name in ("SM_Core", "SM_Forearm") if name in source.objects]
for obj in target.objects:
    if obj is None:
        continue
    original_name = obj.name.split(".")[0]
    obj.data = obj.data.copy()
    review_collection.objects.link(obj)
    obj.name = "REVIEW_EXISTING_" + original_name
    obj.hide_render = False
    obj.hide_set(False)
    obj.location = (-6, 0, 0) if original_name == "SM_Core" else (-12, 0, -12)
    for index, mat in enumerate(obj.data.materials):
        original_mat = mat.name.split(".")[0]
        if original_mat in MATS:
            obj.data.materials[index] = MATS[original_mat]
    references.append(obj)

world = bpy.data.worlds.new("Neutral studio")
scene.world = world
world.use_nodes = True
world.node_tree.nodes["Background"].inputs["Color"].default_value = (.075, .095, .115, 1)
world.node_tree.nodes["Background"].inputs["Strength"].default_value = .45


def area(name, loc, target, energy, size, color):
    data = bpy.data.lights.new(name, "AREA")
    data.energy = energy
    data.shape = "DISK"
    data.size = size
    data.color = color
    obj = bpy.data.objects.new(name, data)
    scene.collection.objects.link(obj)
    obj.location = loc
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat("-Z", "Y").to_euler()


# Mesh coordinates are centimeters, so light power is scaled for those units.
area("Warm broad key", (-70, -55, 95), (0, 0, 0), 180000, 80, (1, .85, .66))
area("Cool readable fill", (-35, 85, 35), (0, 0, 0), 120000, 70, (.61, .80, 1))
area("Silhouette edge", (50, 20, 85), (0, 0, 0), 220000, 65, (1, .92, .78))
area("Front face softbox", (80, -55, 20), (0, 0, 0), 150000, 75, (1, .89, .77))
floor_mat = bpy.data.materials.new("Review floor only")
floor_mat.diffuse_color = (.032, .047, .060, 1)
floor_mat.use_nodes = True
floor_mat.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (.032, .047, .060, 1)
floor_mat.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value = .75
bpy.ops.mesh.primitive_plane_add(size=2000, location=(0, 0, -43))
floor = bpy.context.object
floor.name = "REVIEW_FLOOR_NOT_EXPORTED"
floor.data.materials.append(floor_mat)
camera_data = bpy.data.cameras.new("Asset review camera")
camera = bpy.data.objects.new("Asset review camera", camera_data)
scene.collection.objects.link(camera)
scene.camera = camera
scene.render.engine = "CYCLES"
scene.cycles.samples = 48
scene.cycles.use_denoising = True
scene.render.resolution_x = 1280
scene.render.resolution_y = 1100
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.view_settings.view_transform = "AgX"
scene.view_settings.exposure = .65


def render(name, loc, target, scale, visible_references=True):
    camera.location = loc
    camera.rotation_euler = (Vector(target) - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = scale
    for obj in references:
        obj.hide_render = not visible_references or "Forearm" in obj.name
    scene.render.filepath = str(OUT / name)
    bpy.ops.render.render(write_still=True)


if "--skip-renders" not in sys.argv:
    render("shield-assembled-back.png", (-130, -58, 38), (0, 0, 0), 94)
    render("shield-assembled-front.png", (140, 40, 30), (0, 0, 0), 91)
    for obj in references:
        obj.hide_render = False
    camera.location = (-125, -70, 44)
    camera.rotation_euler = (Vector((-12, 0, -7)) - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera.data.type = "PERSP"
    camera.data.lens = 49
    scene.render.filepath = str(OUT / "shield-grip-fit-back.png")
    bpy.ops.render.render(write_still=True)
    for obj in [hub, *instances, *references]:
        obj.hide_render = True
    render("shield-single-shard-back.png", (-82, -30, 58), (0, 0, 26), 46, False)
    for obj in [hub, *instances, *references]:
        obj.hide_render = False

# Export FBX has no texture path/image embedding and uses existing UE mats.
audit_exports()
portable_source()
camera.location = (-130, -58, 38)
camera.rotation_euler = (Vector() - camera.location).to_track_quat("-Z", "Y").to_euler()
camera.data.type = "ORTHO"
camera.data.ortho_scale = 94
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "Segmented_Shield.blend"), compress=True)
print("SEGMENTED_SHIELD_COMPLETE " + json.dumps(metadata, separators=(",", ":")))

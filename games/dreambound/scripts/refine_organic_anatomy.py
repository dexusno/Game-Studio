"""Refine the preserved performance creatures with actual expressive anatomy.

Run with the configured Blender in background mode. The finished-performance
sources are immutable inputs; every result is written to finished-anatomy.
Inspection and deformation renders use Cycles CPU, never Unreal or a GPU.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys

import bpy
import bmesh
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

ROOT = Path(__file__).resolve().parents[1]
STUDIO = ROOT.parents[1]


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def points(mesh):
    result = np.empty(len(mesh.vertices) * 3, dtype=np.float64)
    mesh.vertices.foreach_get("co", result)
    return result.reshape((-1, 3))


def open_baseline(asset, folder):
    path = folder / ("SK_OE_" + asset + ".blend")
    bpy.ops.wm.open_mainfile(filepath=str(path))
    obj = bpy.data.objects["SK_OE_" + asset]
    arm = obj.find_armature()
    if not arm or len(arm.data.bones) not in (21, 22):
        raise RuntimeError("Expected the preserved performance rig")
    for bone in arm.pose.bones:
        bone.matrix_basis = Matrix.Identity(4)
    bpy.context.view_layer.update()
    return obj, arm


def setup_preview(obj, arm):
    scene = bpy.context.scene
    for item in list(scene.objects):
        if item not in (obj, arm):
            bpy.data.objects.remove(item, do_unlink=True)
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 12
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 700
    scene.render.resolution_y = 700
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.world = bpy.data.worlds.new("AnatomyInspectionWorld")
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs["Color"].default_value = (.07, .08, .09, 1)
    scene.world.node_tree.nodes["Background"].inputs["Strength"].default_value = .55
    scene.view_settings.view_transform = "AgX"
    camera = bpy.data.objects.new("AnatomyCamera", bpy.data.cameras.new("AnatomyCamera"))
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera.data.type = "ORTHO"
    for name, pos, power, size, color in (
        ("Key", (-3, -4, 4), 650, 3, (1, .90, .78)),
        ("Fill", (3, -2, 2), 400, 3, (.76, .88, 1)),
        ("Rim", (1, 3, 3), 600, 2, (.8, 1, .9)),
    ):
        data = bpy.data.lights.new(name, "AREA")
        data.energy, data.size, data.color = power, size, color
        item = bpy.data.objects.new(name, data)
        scene.collection.objects.link(item)
        item.location = pos
        item.rotation_euler = (Vector((0, 0, 1.2)) - item.location).to_track_quat("-Z", "Y").to_euler()
    return camera


def render_view(camera, output, name, target, offset, size):
    camera.location = Vector(target) + Vector(offset)
    camera.rotation_euler = (Vector(target) - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera.data.ortho_scale = size
    bpy.context.scene.render.filepath = str(output / (name + ".png"))
    bpy.ops.render.render(write_still=True)


def inspect(obj, arm, asset, output, views):
    coords = points(obj.data)
    obj.data.calc_loop_triangles()
    tree = BVHTree.FromPolygons(coords.tolist(), [list(t.vertices) for t in obj.data.loop_triangles], all_triangles=True)
    rows = []
    is_briar = asset == "Briarhide"
    for z in np.arange(1.52 if is_briar else 1.80, 1.791 if is_briar else 2.051, .025):
        for x in (0, .06, .12, .18):
            hit, normal, index, distance = tree.ray_cast(Vector((x, -2, float(z))), Vector((0, 1, 0)))
            rows.append({"x": x, "z": round(float(z), 4), "front_y": float(hit.y) if hit else None})
    hand = coords[(coords[:, 0] > (.60 if is_briar else .38)) & (coords[:, 2] < (.73 if is_briar else 1.04))
                  & (coords[:, 2] > (.30 if is_briar else .75))]
    hand_rows = []
    for z in np.arange(.32 if is_briar else .59, .711 if is_briar else .971, .04):
        at = hand[np.abs(hand[:, 2] - z) < .012]
        hand_rows.append({"z": round(float(z), 3), "count": len(at),
                          "x_quantiles": np.quantile(at[:, 0], [0, .25, .5, .75, 1]).tolist() if len(at) else [],
                          "y_quantiles": np.quantile(at[:, 1], [0, .25, .5, .75, 1]).tolist() if len(at) else []})
    tip_candidates = []
    for point in hand[np.argsort(hand[:, 2])]:
        if all(np.linalg.norm(point - np.asarray(other)) > .09 for other in tip_candidates):
            tip_candidates.append(point.tolist())
        if len(tip_candidates) == 6:
            break
    result = {"asset": asset, "object_matrix": [list(r) for r in obj.matrix_world],
              "armature_matrix": [list(r) for r in arm.matrix_world], "bones": len(arm.data.bones),
              "vertices": len(coords), "mouth_surface_rays": rows, "hand_slices": hand_rows,
              "hand_tip_candidates": tip_candidates}
    (output / "source-anatomy-inspection.json").write_text(json.dumps(result, indent=2) + "\n")
    camera = setup_preview(obj, arm)
    head = (0, -.20, 1.70 if is_briar else 1.94)
    hand_center = (.87, -.025, .52) if is_briar else (.60, -.025, .94)
    cases = {
        "head-front": (head, (0, -3, .12), .62 if is_briar else .95),
        "head-side": (head, (3, -.5, .08), .62 if is_briar else .76),
        "hand-front": (hand_center, (0, -3, 0), .54),
        "hand-palm": (hand_center, (2, -2, .05), .54),
        "hand-side": (hand_center, (3, .05, .03), .54),
    }
    for name in views:
        render_view(camera, output, "source-" + name, *cases[name])
    print("ANATOMY_INSPECTED " + str(output), flush=True)


def gate(value, width):
    t = np.clip(.5 + np.asarray(value) / (2 * width), 0, 1)
    return t * t * (3 - 2 * t)


def signed_plane(co, mouth):
    return co[2] - mouth["front_z"] - mouth["slope"] * (co[1] - mouth["front_y"])


def bone_contract(spec):
    anatomy = spec["anatomy_rig"]
    mouth = anatomy["mouth"]
    hinge = [0, mouth["back_y"], mouth["front_z"] + mouth["slope"] * (mouth["back_y"] - mouth["front_y"])]
    rows = [{"bone": "jaw", "parent": "head", "head": hinge,
             "tail": [0, mouth["front_y"] + .025, mouth["front_z"] - .015],
             "channel": "Jaw", "rotation_degrees": [mouth["open_degrees"], 0, 0]}]
    for side, sign in (("l", 1), ("r", -1)):
        for digit in anatomy["digits"]:
            def mirrored(position):
                return [position[0] * sign, position[1], position[2]]
            name = digit["name"] + "_" + side
            rotation = digit["rotation_degrees"]
            tip_rotation = digit["tip_rotation_degrees"]
            rows.append({"bone": name, "parent": "hand_" + side,
                         "head": mirrored(digit["root"]), "tail": mirrored(digit["joint"]),
                         "channel": "Grip", "side": side,
                         "rotation_degrees": [rotation[0], sign * rotation[1], sign * rotation[2]]})
            rows.append({"bone": digit["name"] + "_tip_" + side, "parent": name,
                         "head": mirrored(digit["joint"]), "tail": mirrored(digit["tip"]),
                         "channel": "Grip", "side": side,
                         "rotation_degrees": [tip_rotation[0], sign * tip_rotation[1], sign * tip_rotation[2]]})
        if anatomy.get("frill"):
            frill = anatomy["frill"]
            for name, parent, start, end, rotation in (
                ("frill_", "head", "root", "joint", "rotation_degrees"),
                ("frill_tip_", "frill_" + side, "joint", "tip", "tip_rotation_degrees"),
            ):
                r = frill[rotation]
                rows.append({"bone": name + side, "parent": parent,
                             "head": mirrored(frill[start]), "tail": mirrored(frill[end]),
                             "channel": "Crest", "side": side,
                             "rotation_degrees": [r[0], sign * r[1], sign * r[2]]})
    return rows


def add_bones(arm, rows):
    bpy.ops.object.select_all(action="DESELECT")
    bpy.context.view_layer.objects.active = arm
    arm.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    for row in rows:
        bone = arm.data.edit_bones.new(row["bone"])
        bone.head, bone.tail = row["head"], row["tail"]
        bone.parent = arm.data.edit_bones[row["parent"]]
        bone.use_connect = False
        bone.use_deform = True
    bpy.ops.object.mode_set(mode="OBJECT")


EXTRA_MATERIALS = [
    {"name": "M_OE_Oral", "base_color": [.020, .006, .005, 1], "roughness": .48, "metallic": 0, "two_sided": True},
    {"name": "M_OE_Tooth", "base_color": [.54, .43, .27, 1], "roughness": .30, "metallic": 0, "two_sided": False},
    {"name": "M_OE_Tongue", "base_color": [.070, .018, .012, 1], "roughness": .46, "metallic": 0, "two_sided": False},
]


def material_specs(spec):
    overrides = spec.get("anatomy_rig", {}).get("material_overrides", {})
    return [dict(row, **overrides.get(row["name"], {})) for row in EXTRA_MATERIALS]


def add_materials(obj, spec):
    indices = {}
    for row in material_specs(spec):
        mat = bpy.data.materials.new(row["name"])
        mat.use_nodes = True
        p = mat.node_tree.nodes.get("Principled BSDF")
        p.inputs["Base Color"].default_value = row["base_color"]
        p.inputs["Roughness"].default_value = row["roughness"]
        p.inputs["Metallic"].default_value = row["metallic"]
        p.inputs["Specular IOR Level"].default_value = row.get("specular", .5)
        mat.diffuse_color = row["base_color"]
        indices[row["name"]] = len(obj.data.materials)
        obj.data.materials.append(mat)
    return indices


def convex_hull_2d(points_2d):
    values = sorted(set((round(float(x), 6), round(float(y), 6)) for x, y in points_2d))
    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lower, upper = [], []
    for p in values:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    for p in reversed(values):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    return np.asarray(lower[:-1] + upper[:-1])


def make_mouth(obj, spec):
    """Cut the original lip surface, then build a weighted palate/floor/teeth.

    Only the snout is bisected. UV interpolation on the original surface is
    retained. The closed rest mesh gains a one-millimetre seam; its lower lip
    has genuinely separate vertices, so opening exposes lined inner surfaces.
    """
    mouth = spec["anatomy_rig"]["mouth"]
    mats = add_materials(obj, spec)
    for name in ("head", "jaw"):
        if name not in obj.vertex_groups:
            obj.vertex_groups.new(name=name)
    groups = {g.name: g.index for g in obj.vertex_groups}
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    deform = bm.verts.layers.deform.verify()
    lower_tag = bm.verts.layers.int.new("anatomy_lower_lip")
    added_tag = bm.verts.layers.int.new("anatomy_added")
    def in_snout(v):
        return (abs(v.co.x) < mouth["half_width"] and v.co.y < mouth["back_y"] + .015
                and abs(signed_plane(v.co, mouth)) < .12)
    faces = [f for f in bm.faces if all(in_snout(v) for v in f.verts)]
    geometry = set(faces)
    for face in faces:
        geometry.update(face.verts)
        geometry.update(face.edges)
    result = bmesh.ops.bisect_plane(bm, geom=list(geometry), dist=1e-7,
        plane_co=(0, mouth["front_y"], mouth["front_z"]), plane_no=(0, -mouth["slope"], 1),
        clear_inner=False, clear_outer=False)
    cut_edges = [g for g in result["geom_cut"] if isinstance(g, bmesh.types.BMEdge)]
    cut_points = np.asarray([list(v.co) for edge in cut_edges for v in edge.verts])
    if len(cut_points) < 12:
        raise RuntimeError("Mouth cut did not find a useful snout contour")
    hull = convex_hull_2d(cut_points[:, :2])
    bmesh.ops.split_edges(bm, edges=cut_edges)
    lower_count = 0
    for vertex in bm.verts:
        if in_snout(vertex) and abs(signed_plane(vertex.co, mouth)) < 1e-5 and vertex.link_faces:
            side = sum(signed_plane(face.calc_center_median(), mouth) for face in vertex.link_faces)
            if side < 0:
                vertex[lower_tag] = 1
                vertex.co.z -= .0005
                lower_count += 1
            else:
                vertex.co.z += .0005
    # The generated closed snout contains inward-facing sealing patches.
    # Excise only those interior patches, retaining the exterior lip/chin;
    # otherwise they protrude through the new oral floor when the jaw opens.
    bm.normal_update()
    plane_normal = Vector((0, -mouth["slope"], 1)).normalized()
    def inside_lip(point):
        for i in range(len(hull)):
            a, b = hull[i], hull[(i + 1) % len(hull)]
            edge = b - a
            if edge[0] * (point.y - a[1]) - edge[1] * (point.x - a[0]) < .002 * np.linalg.norm(edge):
                return False
        return True
    sealing_faces = []
    for old_face in bm.faces:
        center_point = old_face.calc_center_median()
        side = signed_plane(center_point, mouth)
        facing = old_face.normal.dot(plane_normal)
        if -.060 < side < .040 and inside_lip(center_point) and ((side < 0 and facing > .05) or (side >= 0 and facing < -.05)):
            sealing_faces.append(old_face)
    sealing_count = len(sealing_faces)
    bmesh.ops.delete(bm, geom=sealing_faces, context="FACES")
    old_count = len(bm.verts)
    new_faces = []
    def vertex(co, control):
        item = bm.verts.new(co)
        item[added_tag] = 1
        if isinstance(control, dict):
            for bone_name, value in control.items():
                item[deform][groups[bone_name]] = value
        else:
            item[deform][groups[control]] = 1.0
        return item
    def face(vs, material):
        item = bm.faces.new(vs)
        item.material_index = mats[material]
        item.smooth = True
        new_faces.append(item)
        return item
    center = hull.mean(axis=0)
    # Recessed palate and floor meet behind the hinge and remain hidden when
    # closed. No borrowed mouth image, emissive fill or camera-facing trick.
    cavity_rings = {}
    for control, sign in (("head", 1), ("jaw", -1)):
        rings = []
        # A raised mandibular floor sits ABOVE the retained outer chin shell.
        # Recessing it downwards would expose that shell's back faces inside
        # the opened mouth. The palate remains recessed into the upper head.
        profile = ((1.005, .0005), (.82, .012), (.40, .024), (.10, .029)) if control == "head" else (
            (1.005, -.0005), (.82, .006), (.40, .011), (.10, .014))
        for scale, depth in profile:
            ring = []
            for x, y in center + (hull - center) * scale:
                z = mouth["front_z"] + mouth["slope"] * (y - mouth["front_y"]) + depth
                ring.append(vertex((x, y, z), control))
            rings.append(ring)
        for a, b in zip(rings[:-1], rings[1:]):
            for i in range(len(hull)):
                j = (i + 1) % len(hull)
                face((a[i], a[j], b[j], b[i]), "M_OE_Oral")
        face(rings[-1], "M_OE_Oral")
        cavity_rings[control] = rings
    # A continuous rear/cheek lining closes the throat view. Without this,
    # opening a generated shell can expose the scene or an unrelated internal
    # skin surface. Its middle row blends head/jaw so the commissure stretches.
    upper, lower = cavity_rings["head"][1], cavity_rings["jaw"][1]
    mid = [vertex((a.co + b.co) * .5, {"head": .5, "jaw": .5}) for a, b in zip(upper, lower)]
    for i in range(len(hull)):
        j = (i + 1) % len(hull)
        if (hull[i, 1] + hull[j, 1]) * .5 > center[1] - .015:
            face((upper[i], upper[j], mid[j], mid[i]), "M_OE_Oral")
            face((mid[i], mid[j], lower[j], lower[i]), "M_OE_Oral")
    # A rounded muscular tongue sits above the dark cavity floor. This has
    # actual volume and follows the jaw; it is not a flat painted mouth card.
    tongue_center = center + np.asarray((0, -.018))
    radius_x, radius_y = (.034, .058) if spec["asset"] == "Briarhide" else (.045, .042)
    tongue_rings = []
    for latitude in np.linspace(-math.pi / 2 + .01, math.pi / 2 - .01, 11):
        ring = []
        for longitude in np.linspace(0, math.tau, 20, endpoint=False):
            x = tongue_center[0] + radius_x * math.cos(latitude) * math.cos(longitude)
            y = tongue_center[1] + radius_y * math.cos(latitude) * math.sin(longitude)
            z = mouth["front_z"] + mouth["slope"] * (y - mouth["front_y"]) + .014 + .011 * math.sin(latitude)
            ring.append(vertex((x, y, z), "jaw"))
        tongue_rings.append(ring)
    for a, b in zip(tongue_rings[:-1], tongue_rings[1:]):
        for i in range(20):
            j = (i + 1) % 20
            face((a[i], a[j], b[j], b[i]), "M_OE_Tongue")
    face(list(reversed(tongue_rings[0])), "M_OE_Tongue")
    face(tongue_rings[-1], "M_OE_Tongue")
    # Teeth sit inside the source lip, with longer canines for Briarhide and
    # a restrained fine row for the amphibian. Both jaws have real surfaces.
    half = min(abs(float(hull[:, 0].min())), abs(float(hull[:, 0].max())))
    tooth_count = 6 if spec["asset"] == "Briarhide" else 8
    for control, direction in (("head", -1), ("jaw", 1)):
        span = .72 if control == "head" else .62
        for index, x in enumerate(np.linspace(-half * span, half * span, tooth_count)):
            intersections = []
            for i in range(len(hull)):
                a, b = hull[i], hull[(i + 1) % len(hull)]
                if min(a[0], b[0]) <= x <= max(a[0], b[0]) and abs(b[0] - a[0]) > 1e-7:
                    intersections.append(float(a[1] + (x - a[0]) / (b[0] - a[0]) * (b[1] - a[1])))
            y = min(intersections) + .018
            z = mouth["front_z"] + mouth["slope"] * (y - mouth["front_y"])
            length = (.026 if control == "head" and index in (0, tooth_count - 1) else .012) if spec["asset"] == "Briarhide" else .009
            radius = .0065 if spec["asset"] == "Briarhide" else .0038
            base = [vertex((x + radius * math.cos(t), y + radius * math.sin(t), z - direction * .004), control)
                    for t in np.linspace(0, math.tau, 9, endpoint=False)]
            tip = vertex((x, y - .003, z + direction * length), control)
            for i in range(len(base)):
                face((base[i], base[(i + 1) % len(base)], tip), "M_OE_Tooth")
            face(list(reversed(base)), "M_OE_Tooth")
    bmesh.ops.recalc_face_normals(bm, faces=new_faces)
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    return {"original_surface_cut_edges": len(cut_edges), "separated_lower_lip_vertices": lower_count,
            "removed_internal_sealing_faces": sealing_count,
            "added_vertices": len(obj.data.vertices) - old_count, "mouth_contour_xy_metres": hull.tolist(),
            "inner_surface": "Separate weighted palate, raised mandibular floor, rear/cheek lining, rounded tongue volume and modeled upper/lower teeth",
            "closed_lip_gap_metres": .001}


def segment_distance(coords, a, b):
    a, b = np.asarray(a), np.asarray(b)
    vec = b - a
    t = np.clip((coords - a) @ vec / (vec @ vec), 0, 1)
    return np.linalg.norm(coords - a - t[:, None] * vec, axis=1), t


def contact_positions_signature(coords, asset):
    briar = asset == "Briarhide"
    selected = coords[(np.abs(coords[:, 0]) > (.60 if briar else .38))
                      & (coords[:, 2] > (.30 if briar else .75))
                      & (coords[:, 2] < (.73 if briar else 1.04))]
    selected = selected[np.lexsort((selected[:, 2], selected[:, 1], selected[:, 0]))].astype(np.float32)
    return {"vertices": len(selected), "positions_sha256": hashlib.sha256(selected.tobytes()).hexdigest()}


def surface_pixel(obj, camera, pixel_x, pixel_y):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    evaluated = obj.evaluated_get(depsgraph)
    mesh = evaluated.to_mesh()
    tree = BVHTree.FromPolygons([v.co for v in mesh.vertices], [list(p.vertices) for p in mesh.polygons])
    basis = camera.matrix_world.to_3x3()
    size = camera.data.ortho_scale
    origin = camera.location + basis @ Vector(((pixel_x / 700 - .5) * size, (.5 - pixel_y / 700) * size, 0))
    hit, normal, face_index, distance = tree.ray_cast(origin, basis @ Vector((0, 0, -1)))
    result = {"pixel": [pixel_x, pixel_y], "material": mesh.materials[mesh.polygons[face_index].material_index].name if hit else None,
              "position": list(hit) if hit else None,
              "source_vertices": [list(obj.data.vertices[i].co) for i in mesh.polygons[face_index].vertices] if hit else [],
              "source_normal": list(obj.data.polygons[face_index].normal) if hit else [],
              "source_weights": [{obj.vertex_groups[a.group].name: a.weight for a in obj.data.vertices[i].groups}
                                 for i in mesh.polygons[face_index].vertices] if hit else []}
    evaluated.to_mesh_clear()
    return result


def refine_weights(obj, arm, spec):
    coords = points(obj.data)
    names = [bone.name for bone in arm.data.bones]
    matrix = np.zeros((len(coords), len(names)), dtype=np.float64)
    indices = {name: i for i, name in enumerate(names)}
    group_names = {g.index: g.name for g in obj.vertex_groups}
    for v in obj.data.vertices:
        for assignment in v.groups:
            name = group_names[assignment.group]
            if name in indices:
                matrix[v.index, indices[name]] = assignment.weight
    added = np.asarray([v.value for v in obj.data.attributes["anatomy_added"].data]) > 0
    lower = np.asarray([v.value for v in obj.data.attributes["anatomy_lower_lip"].data]) > 0
    anatomy = spec["anatomy_rig"]
    mouth = anatomy["mouth"]
    signed = coords[:, 2] - mouth["front_z"] - mouth["slope"] * (coords[:, 1] - mouth["front_y"])
    jaw = ((signed < -1e-6) | lower).astype(float)
    jaw *= gate(mouth["back_y"] - coords[:, 1], .028)
    jaw *= gate(mouth["half_width"] - np.abs(coords[:, 0]), .025)
    jaw *= gate(coords[:, 2] - mouth["chin_min_z"], .025)
    jaw[added] = 0
    matrix *= 1 - jaw[:, None]
    matrix[:, indices["jaw"]] += jaw
    for side, sign in (("l", 1), ("r", -1)):
        local = coords * np.array([sign, 1, 1])
        candidates = (matrix[:, indices["hand_" + side]] > .001) & ~added
        ids = np.flatnonzero(candidates)
        p = local[ids]
        distances, reaches, distal = [], [], []
        for digit in anatomy["digits"]:
            d1, t1 = segment_distance(p, digit["root"], digit["joint"])
            d2, t2 = segment_distance(p, digit["joint"], digit["tip"])
            distances.append(np.minimum(d1, d2))
            axis = np.asarray(digit["joint"]) - np.asarray(digit["root"])
            along = (p - digit["root"]) @ (axis / np.linalg.norm(axis))
            reaches.append(gate(along - .018, .032))
            joint_axis = np.asarray(digit["tip"]) - np.asarray(digit["root"])
            joint_axis /= np.linalg.norm(joint_axis)
            distal.append(gate((p - digit["joint"]) @ joint_axis, .032))
        distances = np.asarray(distances).T
        reaches = np.asarray(reaches).T
        distal = np.asarray(distal).T
        affinity = np.exp(-np.square(distances / anatomy["digit_blend_radius"]) * 1.6)
        affinity /= np.maximum(affinity.sum(axis=1, keepdims=True), 1e-20)
        useful = np.clip((affinity * reaches).sum(axis=1), 0, 1)
        transfer = matrix[ids, indices["hand_" + side]] * useful
        matrix[ids, indices["hand_" + side]] -= transfer
        for j, digit in enumerate(anatomy["digits"]):
            amount = transfer * affinity[:, j]
            matrix[ids, indices[digit["name"] + "_" + side]] += amount * (1 - distal[:, j])
            matrix[ids, indices[digit["name"] + "_tip_" + side]] += amount * distal[:, j]
        if anatomy.get("frill"):
            x, y, z = local.T
            influence = gate(x - .135, .065) * gate(z - 1.67, .07)
            influence *= gate(.20 - y, .065) * gate(y + .10, .055)
            available = matrix[:, indices["head"]] + matrix[:, indices["neck"]]
            amount = influence * available
            tip_share = gate(x - .285, .09)
            factor = 1 - np.divide(amount, available, out=np.zeros_like(amount), where=available > 0)
            matrix[:, indices["head"]] *= factor
            matrix[:, indices["neck"]] *= factor
            matrix[:, indices["frill_" + side]] += amount * (1 - tip_share)
            matrix[:, indices["frill_tip_" + side]] += amount * tip_share
    order = np.argsort(matrix, axis=1)
    matrix[np.arange(len(matrix))[:, None], order[:, :-4]] = 0
    matrix[matrix < 1e-5] = 0
    totals = matrix.sum(axis=1)
    if np.any(totals < .999):
        if np.any(totals < .01):
            raise RuntimeError("Anatomy skin contains an unweighted vertex")
    matrix /= totals[:, None]
    for group in list(obj.vertex_groups):
        obj.vertex_groups.remove(group)
    for column, name in enumerate(names):
        group = obj.vertex_groups.new(name=name)
        for index in np.flatnonzero(matrix[:, column] > 0):
            group.add([int(index)], float(matrix[index, column]), "REPLACE")
    new = {row["bone"]: {"vertices_over_25_percent": int(np.sum(matrix[:, indices[row["bone"]]] > .25)),
                         "weight_sum": float(matrix[:, indices[row["bone"]]].sum())}
           for row in bone_contract(spec)}
    if any(info["vertices_over_25_percent"] == 0 for info in new.values()):
        raise RuntimeError("A new control has no meaningful anatomical skin: " + json.dumps(new))
    return {"vertices": len(coords), "max_influences": int(np.sum(matrix > 0, axis=1).max()),
            "method": "Preserved body skin; actual separated lip and oral surfaces; per-digit segment envelopes with soft web/palm transitions; two-stage weighted frill", "new_bone_regions": new}


def apply_channels(arm, rows, jaw=0, grip=0, crest=0):
    for bone in arm.pose.bones:
        bone.matrix_basis = Matrix.Identity(4)
    values = {"Jaw": jaw, "Grip": grip, "Crest": crest}
    for row in rows:
        rotation = Matrix.Identity(4)
        for axis, degrees in zip("XYZ", row["rotation_degrees"]):
            rotation = Matrix.Rotation(math.radians(degrees * values[row["channel"]]), 4, axis) @ rotation
        rest = arm.data.bones[row["bone"]].matrix_local.to_3x3().to_4x4()
        arm.pose.bones[row["bone"]].matrix_basis = rest.inverted() @ rotation @ rest
    bpy.context.view_layer.update()


def displacement_checks(obj, arm, rows):
    base = points(obj.data)
    results = {}
    for channel, values in (("Neutral", (0, 0, 0)), ("Jaw", (1, 0, 0)), ("Grip", (0, 1, 0)), ("Crest", (0, 0, 1))):
        if channel == "Crest" and not any(row["channel"] == channel for row in rows):
            continue
        apply_channels(arm, rows, *values)
        evaluated = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
        mesh = evaluated.to_mesh()
        delta = np.linalg.norm(points(mesh) - base, axis=1)
        if not np.all(np.isfinite(delta)):
            raise RuntimeError("Nonfinite anatomy deformation")
        results[channel] = {"max_vertex_displacement_cm": float(delta.max() * 100),
                            "vertices_moving_over_1mm": int(np.sum(delta > .001))}
        evaluated.to_mesh_clear()
    apply_channels(arm, rows)
    return results


def prepare(obj, arm, spec, spec_path, source_dir, output, views):
    previous = json.loads((source_dir / "prep-report.json").read_text())
    original_contacts = contact_positions_signature(points(obj.data), spec["asset"])
    rests = {b.name: np.asarray(b.matrix_local).copy() for b in arm.data.bones}
    rows = bone_contract(spec)
    add_bones(arm, rows)
    mouth_info = make_mouth(obj, spec)
    weights = refine_weights(obj, arm, spec)
    final_contacts = contact_positions_signature(points(obj.data), spec["asset"])
    if original_contacts != final_contacts:
        raise RuntimeError("Grip0 contact-region vertex positions changed")
    if any(not np.array_equal(rests[b.name], np.asarray(b.matrix_local)) for b in arm.data.bones if b.name in rests):
        raise RuntimeError("An original performance bone rest changed")
    sys.path.insert(0, str(Path(__file__).parent))
    from prepare_organic_enemy import texture_files
    textures = texture_files(obj, output, previous["textures"])
    apply_channels(arm, rows)
    displacements = displacement_checks(obj, arm, rows)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    arm.select_set(True)
    bpy.context.view_layer.objects.active = arm
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1
    bpy.context.preferences.filepaths.save_version = 0
    fbx = output / (obj.name + ".fbx")
    bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=True, object_types={"MESH", "ARMATURE"},
        axis_forward="-Y", axis_up="Z", apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
        bake_anim=False, use_mesh_modifiers=True, mesh_smooth_type="FACE", add_leaf_bones=False,
        use_armature_deform_only=True, path_mode="STRIP", colors_type="LINEAR")
    blend = output / (obj.name + ".blend")
    bpy.ops.wm.save_as_mainfile(filepath=str(blend))
    obj.data.calc_loop_triangles()
    report = dict(previous)
    report.update({"fbx_sha256": digest(fbx), "blend_sha256": digest(blend),
        "runtime_triangles": len(obj.data.loop_triangles), "textures": textures,
        "rig_spec_sha256": digest(spec_path), "preparation_script_sha256": digest(__file__),
        "rig_version": "anatomy-v1", "weights": weights, "extra_materials": material_specs(spec),
        "preservation": {"prepared_source": str(source_dir / blend.name),
                         "prepared_source_sha256": digest(source_dir / blend.name),
                         "all_prior_bone_rest_matrices_unchanged": True,
                         "all_prior_bone_parents_unchanged": True,
                         "grip_zero_contact_region": final_contacts,
                         "contact_region_matches_original_exactly": True,
                         "intentional_geometry_edit": "Local lip seam split with interpolated source UVs; added separately weighted oral lining and upper/lower teeth",
                         "unchanged": ["original packed PBR texture bytes", "original body/limb positions", "original eyes and their material", "antler/head attachment", "existing anatomical joint rest matrices and parents"]},
        "material_slots": [m.name for m in obj.data.materials], "new_skeleton_required": True,
        "bones": [{"name": b.name, "parent": b.parent.name if b.parent else None,
                   "head": list(b.head_local), "tail": list(b.tail_local),
                   "matrix_local": [list(row) for row in b.matrix_local]} for b in arm.data.bones],
        "anatomy_rig": {"channel_bones": rows, "mouth_construction": mouth_info,
                        "channel_displacement_checks": displacements,
                        "rotation_convention": "Signed Blender mesh-axis degrees, compose X then Y then Z; multiply by channel0..1. Derive runtime through cached full reference bases. Grip0 preserves original claw tips.",
                        "baseline_blend": str(source_dir / blend.name), "baseline_blend_sha256": digest(source_dir / blend.name),
                        "baseline_fbx_sha256": previous["fbx_sha256"], "preserved_original_rest_matrices": True},
        "limits": "Candidate anatomy, not animation parity. Original generated triangular skin and four-influence linear blend skinning; one hinge per jaw, two segments per existing digit, no lip/eyelid rig; modeled tongue follows the jaw. CPU channel poses require actual authored motion review in Unreal.",
        "preview_renders": []})
    (output / "prep-report.json").write_text(json.dumps(report, indent=2) + "\n")
    camera = setup_preview(obj, arm)
    is_briar = spec["asset"] == "Briarhide"
    head = (0, -.28, 1.65 if is_briar else 1.94)
    hand_center = (.87, -.025, .54) if is_briar else (.60, -.025, .94)
    cases = {
        "jaw-closed-front": (0, 0, 0, head, (.25, -3, .14), .54 if is_briar else .65),
        "jaw-half-front": (.5, 0, 0, head, (.25, -3, .14), .54 if is_briar else .65),
        "jaw-open-front": (1, 0, 0, head, (.25, -3, .14), .54 if is_briar else .65),
        "jaw-open-side": (1, 0, 0, head, (3, -.9, .15), .65),
        "grip-half": (0, .5, 0, hand_center, (2, -2, .05), .56),
        "grip-full": (0, 1, 0, hand_center, (2, -2, .05), .56),
        "grip-full-right": (0, 1, 0, (-hand_center[0], hand_center[1], hand_center[2]), (-2, -2, .05), .56),
        "crest-rest": (0, 0, 0, (0, -.05, 1.84), (.3, -3, .1), 1.15),
        "crest-full": (.7, .35, 1, (0, -.05, 1.84), (.3, -3, .1), 1.15),
        "expression-full": (1, 1, 1, (0, -.03, 1.23), (1, -3, .2), 2.45),
        "flexed-cup": (.7, .7, .8, (0, -.2, 1.3), (1, -3, .2), 2.45),
    }
    for name in views:
        jaw, grip, crest, target, offset, size = cases[name]
        apply_channels(arm, rows, jaw, grip, crest)
        if name == "flexed-cup":
            changes = {"upperarm_l": ("X", -35), "forearm_l": ("X", -65), "hand_l": ("Y", 40),
                       "upperarm_r": ("X", -50), "forearm_r": ("X", -50), "hand_r": ("Y", -40),
                       "head": ("Z", 12), "chest": ("X", 10)}
            for bone_name, (axis, angle) in changes.items():
                rest = arm.data.bones[bone_name].matrix_local.to_3x3().to_4x4()
                arm.pose.bones[bone_name].matrix_basis = rest.inverted() @ Matrix.Rotation(math.radians(angle), 4, axis) @ rest
            bpy.context.view_layer.update()
        render_view(camera, output, name, target, offset, size)
        if name == "jaw-open-front":
            report["oral_surface_samples"] = [surface_pixel(obj, camera, 350, y) for y in (440, 470, 490, 510)]
        report["preview_renders"].append(name + ".png")
    apply_channels(arm, rows)
    (output / "prep-report.json").write_text(json.dumps(report, indent=2) + "\n")
    print("ANATOMY_PREPARED " + json.dumps({"asset": spec["asset"], "fbx_sha256": report["fbx_sha256"], "bones": len(rows) + len(rests), "triangles": report["runtime_triangles"]}), flush=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--asset", choices=("Briarhide", "MireSeer"), required=True)
    parser.add_argument("--inspect-only", action="store_true")
    parser.add_argument("--diagnose-mouth", action="store_true")
    parser.add_argument("--views")
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
    config = json.loads((STUDIO / "config.local.json").read_text(encoding="utf-8"))
    folder = "briarhide" if args.asset == "Briarhide" else "mire-seer"
    private = Path(config["tools"]["ai3d"]["windowsOutputRoot"]) / "organic-enemies" / folder
    output = private / "finished-anatomy"
    output.mkdir(parents=True, exist_ok=True)
    if args.diagnose_mouth:
        bpy.ops.wm.open_mainfile(filepath=str(output / ("SK_OE_" + args.asset + ".blend")))
        obj = bpy.data.objects["SK_OE_" + args.asset]
        arm = obj.find_armature()
        report = json.loads((output / "prep-report.json").read_text())
        camera = setup_preview(obj, arm)
        target = Vector((0, -.28, 1.65 if args.asset == "Briarhide" else 1.94))
        camera.location = target + Vector((.25, -3, .14))
        camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
        camera.data.ortho_scale = .54 if args.asset == "Briarhide" else .65
        apply_channels(arm, report["anatomy_rig"]["channel_bones"], jaw=1)
        print("ORAL_DIAGNOSIS " + json.dumps([surface_pixel(obj, camera, 350, y) for y in (470, 490)]), flush=True)
        return
    obj, arm = open_baseline(args.asset, private / "finished-performance")
    if args.inspect_only:
        inspect(obj, arm, args.asset, output, (args.views or "head-front,head-side,hand-front,hand-side").split(","))
        return
    spec_path = ROOT / "art" / "organic-enemies" / "rig" / (args.asset + ".json")
    spec = json.loads(spec_path.read_text())
    prepare(obj, arm, spec, spec_path, private / "finished-performance", output,
            (args.views or "jaw-open-front,jaw-open-side,grip-half,grip-full,crest-full,expression-full").split(","))


if __name__ == "__main__":
    main()

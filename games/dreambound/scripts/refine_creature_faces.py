"""Create reviewed facial candidates from immutable finished-anatomy creatures.

Blender CPU only.  The current rig JSONs and historical exports are inputs.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import shutil
import sys
from datetime import datetime, timezone

import bpy
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

ROOT = Path(__file__).resolve().parents[1]
STUDIO = ROOT.parents[1]
sys.path.insert(0, str(Path(__file__).parent))
from refine_organic_anatomy import setup_preview, render_view, apply_channels, points


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def smooth(t):
    t = max(0.0, min(1.0, float(t)))
    return t * t * (3 - 2 * t)


def material_defs(spec):
    return [
        {"name": "M_OE_DreadSkin", "base_color": [1, 1, 1, 1], "vertex_color": True,
         "roughness": .68, "specular": .25, "metallic": 0, "two_sided": False},
        {"name": "M_OE_DreadSocket", "base_color": spec["socket_color"] + [1],
         "roughness": .52, "specular": .22, "metallic": 0, "two_sided": False},
        {"name": "M_OE_DreadEye", "base_color": spec["eye_color"] + [1],
         "roughness": .38, "specular": .10, "metallic": 0, "two_sided": False,
         "emissive_color": spec.get("eye_emissive_color", [0, 0, 0]),
         "emissive_strength": spec.get("eye_emissive_strength", 0)},
        {"name": "M_OE_DreadIris", "base_color": spec["iris_color"] + [1],
         "roughness": .38, "specular": .10, "metallic": 0, "two_sided": False,
         "emissive_color": spec.get("eye_emissive_color", [0, 0, 0]),
         "emissive_strength": spec.get("iris_emissive_strength", 0)},
        {"name": "M_OE_DreadPupil", "base_color": [.0015, .0018, .0012, 1],
         "roughness": .44, "specular": .08, "metallic": 0, "two_sided": False,
         "emissive_color": [0, 0, 0], "emissive_strength": 0},
    ]


def skin_weights(obj):
    names = [g.name for g in obj.vertex_groups]
    return [{names[g.group]: float(g.weight) for g in v.groups} for v in obj.data.vertices]


def original_status_color(mesh):
    # Verified in both authoritative anatomy FBXs: the first color set is the
    # POINT-domain OrganicTell RGBA status mask, not visible skin pigment.
    color = mesh.color_attributes[0] if mesh.color_attributes else None
    if color is None or color.name != "OrganicTell" or color.domain != "POINT":
        raise RuntimeError("Expected the authoritative OrganicTell status color set")
    return color


class SourceSurface:
    def __init__(self, obj):
        self.obj = obj
        self.co = points(obj.data)
        obj.data.calc_loop_triangles()
        self.triangles = [t for t in obj.data.loop_triangles if t.material_index == 0]
        self.tree = BVHTree.FromPolygons(self.co.tolist(), [list(t.vertices) for t in self.triangles], all_triangles=True)
        self.weights = skin_weights(obj)
        material = obj.data.materials[0]
        image = next(n.image for n in material.node_tree.nodes if n.type == "TEX_IMAGE"
                     and n.image and n.image.colorspace_settings.name == "sRGB")
        self.image_size = tuple(image.size)
        self.image = np.empty(image.size[0] * image.size[1] * 4, dtype=np.float32)
        image.pixels.foreach_get(self.image)
        self.image = self.image.reshape((image.size[1], image.size[0], 4))
        self.pigment_floor = np.zeros(3)
        shader = next(n for n in material.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
        link = next(iter(shader.inputs["Base Color"].links), None)
        if link and link.from_node.type == "VECT_MATH" and link.from_node.operation == "ADD":
            self.pigment_floor = np.asarray(link.from_node.inputs[1].default_value)[:3]

    def color(self, uv):
        width, height = self.image_size
        x, y = float(uv[0]) * width - .5, float(uv[1]) * height - .5
        ix, iy = math.floor(x), math.floor(y)
        fx, fy = x - ix, y - iy
        values = np.zeros(3)
        for dx, dy, weight in [(0, 0, (1-fx)*(1-fy)), (1, 0, fx*(1-fy)),
                                (0, 1, (1-fx)*fy), (1, 1, fx*fy)]:
            rgb = self.image[(iy+dy) % height, (ix+dx) % width, :3]
            linear = np.where(rgb <= .04045, rgb / 12.92, ((rgb + .055) / 1.055) ** 2.4)
            values += linear * weight
        return np.clip(values + self.pigment_floor, 0, 1).tolist() + [1]

    def sample(self, point, normal):
        hit, _, index, distance = self.tree.ray_cast(Vector(point) + Vector(normal) * .28, -Vector(normal), .56)
        if hit is None or abs((hit - Vector(point)).dot(normal)) > .045:
            hit, _, index, distance = self.tree.find_nearest(Vector(point))
        tri = self.triangles[index]
        a, b, c = (self.co[i] for i in tri.vertices)
        yz = np.linalg.lstsq(np.column_stack((b - a, c - a)), np.asarray(hit) - a, rcond=None)[0]
        bary = np.maximum([1 - yz.sum(), yz[0], yz[1]], 0)
        bary /= bary.sum()
        uv = sum((Vector(self.obj.data.uv_layers.active.data[i].uv) * float(w)
                  for i, w in zip(tri.loops, bary)), Vector((0, 0)))
        weights = {}
        for vid, share in zip(tri.vertices, bary):
            for name, weight in self.weights[vid].items():
                weights[name] = weights.get(name, 0) + float(share) * weight
        return Vector(hit), uv, weights


def normalize_weights(weights):
    entries = sorted(((n, w) for n, w in weights.items() if w > 1e-6), key=lambda x: -x[1])[:4]
    total = sum(w for _, w in entries)
    return {n: w / total for n, w in entries}


class FaceMesh:
    def __init__(self, obj, retained):
        self.obj = obj
        self.old = obj.data
        self.old_weights = skin_weights(obj)
        self.old_group_names = [g.name for g in obj.vertex_groups]
        keep = sorted({i for p in retained for i in p.vertices})
        self.original_ids = keep[:]
        remap = {old: new for new, old in enumerate(keep)}
        self.remap = remap
        self.vertices = [list(self.old.vertices[i].co) for i in keep]
        self.weights = [self.old_weights[i] for i in keep]
        status = original_status_color(self.old)
        self.colors = [list(status.data[i].color) for i in keep]
        self.materials = [m for m in self.old.materials if not m.name.startswith("M_OE_Eye")]
        self.mat_ids = {m.name: i for i, m in enumerate(self.materials)}
        self.faces, self.uvs, self.face_mats, self.smooths, self.source_normals = [], [], [], [], []
        self.source_loops = []
        normals = np.empty(len(self.old.loops) * 3, dtype=np.float64)
        self.old.corner_normals.foreach_get("vector", normals)
        normals = normals.reshape((-1, 3))
        for poly in retained:
            self.faces.append([remap[i] for i in poly.vertices])
            self.face_mats.append(self.mat_ids[self.old.materials[poly.material_index].name])
            self.smooths.append(poly.use_smooth)
            self.uvs.append([list(self.old.uv_layers.active.data[i].uv) for i in poly.loop_indices])
            self.source_normals.append([list(normals[i]) for i in poly.loop_indices])
            self.source_loops.append(list(poly.loop_indices))

    def add_material(self, row):
        mat = bpy.data.materials.new(row["name"])
        mat.use_nodes = True
        p = mat.node_tree.nodes.get("Principled BSDF")
        for name, value in [("Base Color", row["base_color"]), ("Roughness", row["roughness"]),
                            ("Specular IOR Level", row["specular"]), ("Metallic", 0),
                            ("Emission Color", row.get("emissive_color", [0, 0, 0]) + [1]),
                            ("Emission Strength", row.get("emissive_strength", 0))]:
            p.inputs[name].default_value = value
        mat.diffuse_color = row["base_color"]
        if row.get("vertex_color"):
            node = mat.node_tree.nodes.new("ShaderNodeVertexColor")
            node.layer_name = "DreadFaceColor"
            mat.node_tree.links.new(node.outputs["Color"], p.inputs["Base Color"])
        self.mat_ids[row["name"]] = len(self.materials)
        self.materials.append(mat)

    def vertex(self, point, weights, color=(1, 1, 1, 1)):
        index = len(self.vertices)
        self.vertices.append(list(point))
        self.weights.append(normalize_weights(weights))
        self.original_ids.append(-1)
        self.colors.append(color)
        return index

    def face(self, vertices, uvs, material, sign=1):
        if sign < 0:
            vertices, uvs = list(reversed(vertices)), list(reversed(uvs))
        self.faces.append(vertices)
        self.uvs.append([list(uv) for uv in uvs])
        self.face_mats.append(self.mat_ids[material])
        self.smooths.append(True)
        self.source_normals.append(None)
        self.source_loops.append(None)

    def fragment(self, polygon, material):
        ids = []
        for item in polygon:
            source = item["source"]
            if source >= 0 and source in self.remap:
                ids.append(self.remap[source])
                continue
            index = self.vertex(item["co"], item["weights"], item["color"])
            if source >= 0:
                self.original_ids[index] = source
                self.weights[index] = self.old_weights[source]
                self.remap[source] = index
            ids.append(index)
        self.face(ids, [p["uv"] for p in polygon], material)
        self.source_normals[-1] = [p["normal"] for p in polygon]

    def finish(self):
        mesh = bpy.data.meshes.new(self.old.name + "_Dread")
        mesh.from_pydata(self.vertices, [], self.faces)
        for mat in self.materials:
            mesh.materials.append(mat)
        uv = mesh.uv_layers.new(name=self.old.uv_layers.active.name)
        for p, coords, material, smooth_face in zip(mesh.polygons, self.uvs, self.face_mats, self.smooths):
            p.material_index = material
            p.use_smooth = smooth_face
            for i, value in zip(p.loop_indices, coords):
                uv.data[i].uv = value
        ids = mesh.attributes.new("dread_source_vertex", "INT", "POINT")
        ids.data.foreach_set("value", self.original_ids)
        for attr in self.old.attributes:
            if attr.domain != "POINT" or attr.data_type not in ("INT", "BOOLEAN", "FLOAT") or attr.name.startswith("."):
                continue
            copied = mesh.attributes.get(attr.name) or mesh.attributes.new(attr.name, attr.data_type, "POINT")
            values = [attr.data[i].value if i >= 0 else (1 if attr.name == "anatomy_added" else 0)
                      for i in self.original_ids]
            copied.data.foreach_set("value", values)
        # Unreal consumes the first FBX color set for every material. Body R
        # therefore remains the status mask; sampled pigment belongs only to
        # DreadSkin corners. CORNER storage keeps shared socket vertices safe.
        dread = mesh.color_attributes.new(name="DreadFaceColor", type="FLOAT_COLOR", domain="CORNER")
        body_vertices = {v for p in mesh.polygons if p.material_index == 0 for v in p.vertices}
        for poly in mesh.polygons:
            material = self.materials[poly.material_index].name
            for loop in poly.loop_indices:
                vertex = mesh.loops[loop].vertex_index
                dread.data[loop].color = ((0, 0, 0, 1)
                    if material.startswith("M_OE_Dread") and material != "M_OE_DreadSkin"
                    else self.colors[vertex])
        for attr in self.old.color_attributes:
            copied = mesh.color_attributes.new(name=attr.name, type=attr.data_type, domain=attr.domain)
            if attr.domain == "POINT":
                for i, source in enumerate(self.original_ids):
                    copied.data[i].color = (attr.data[source].color if source >= 0
                        else self.colors[i] if attr.name == "OrganicTell" and i in body_vertices
                        else (0, 0, 0, 1))
            elif attr.domain == "CORNER":
                for poly, sources in zip(mesh.polygons, self.source_loops):
                    for j, loop in enumerate(poly.loop_indices):
                        copied.data[loop].color = attr.data[sources[j]].color if sources else (1, 1, 1, 1)
        mesh.color_attributes.active_color_index = 0
        mesh.color_attributes.render_color_index = 0
        mesh.update()
        normals = [list(n.vector) for n in mesh.corner_normals]
        for poly, original in zip(mesh.polygons, self.source_normals):
            if original:
                for index, normal in zip(poly.loop_indices, original):
                    normals[index] = normal
        mesh.normals_split_custom_set(normals)
        self.obj.data = mesh
        for group in list(self.obj.vertex_groups):
            self.obj.vertex_groups.remove(group)
        names = self.old_group_names + sorted({n for w in self.weights for n in w} - set(self.old_group_names))
        groups = {n: self.obj.vertex_groups.new(name=n) for n in names}
        for index, weights in enumerate(self.weights):
            for name, weight in weights.items():
                groups[name].add([index], weight, "REPLACE")
        return mesh


def eye_basis(surface, spec, side):
    normal = Vector(spec["normal"])
    normal.normalize()
    u = Vector((0, 0, 1)).cross(normal).normalized()
    v = normal.cross(u).normalized()
    anchor = Vector(spec["anchor"])
    for vector in (normal, u, v, anchor):
        vector.x *= side
    center, _, _ = surface.sample(anchor, normal)
    return center, normal, u, v


def subtract_eye_polygon(polygon, basis, width, height):
    p, n, u, v = basis
    projected = [(Vector(item["co"]) - p) for item in polygon]
    if (min(d.dot(u) for d in projected) > width or max(d.dot(u) for d in projected) < -width
            or min(d.dot(v) for d in projected) > height or max(d.dot(v) for d in projected) < -height
            or min(abs(d.dot(n)) for d in projected) > .08):
        return [polygon]

    def blend(a, b, t):
        weight = {name: a["weights"].get(name, 0) * (1-t) + b["weights"].get(name, 0) * t
                  for name in set(a["weights"]) | set(b["weights"])}
        normal = Vector(a["normal"]).lerp(Vector(b["normal"]), t).normalized()
        return {"co": list(Vector(a["co"]).lerp(Vector(b["co"]), t)),
                "uv": list(Vector(a["uv"]).lerp(Vector(b["uv"]), t)), "weights": normalize_weights(weight),
                "normal": list(normal), "source": -1,
                "color": list(Vector(a["color"]).lerp(Vector(b["color"]), t))}

    remaining, kept = polygon, []
    for segment in range(32):
        if not remaining:
            break
        angle = segment * 2 * math.pi / 32
        axis = u * (math.cos(angle) / width) + v * (math.sin(angle) / height)
        distances = [(Vector(item["co"]) - p).dot(axis) - 1 for item in remaining]
        if min(distances) >= -1e-7:
            kept.append(remaining)
            remaining = []
            break
        if max(distances) <= 1e-7:
            continue
        inside, outside = [], []
        for i, a in enumerate(remaining):
            j = (i + 1) % len(remaining)
            b, da, db = remaining[j], distances[i], distances[j]
            (inside if da <= 0 else outside).append(a)
            if (da < 0 < db) or (db < 0 < da):
                cut = blend(a, b, da / (da - db))
                inside.append(cut)
                outside.append(cut)
        if len(outside) >= 3:
            kept.append(outside)
        remaining = inside if len(inside) >= 3 else []
    return kept


def make_faces(obj, spec):
    surface = SourceSurface(obj)
    status = original_status_color(obj.data)
    eyes = [eye_basis(surface, spec, side) for side in (-1, 1)]
    retained, removed, fragments = [], [], []
    source_normals = [list(n.vector) for n in obj.data.corner_normals]
    for poly in obj.data.polygons:
        material = obj.data.materials[poly.material_index].name
        if material.startswith("M_OE_Eye"):
            removed.append(poly)
            continue
        if poly.material_index == 0:
            co = [obj.data.vertices[i].co for i in poly.vertices]
            possible = False
            for p, n, u, v in eyes:
                ds = [point - p for point in co]
                if (min(d.dot(u) for d in ds) <= spec["cut_width"] and max(d.dot(u) for d in ds) >= -spec["cut_width"]
                        and min(d.dot(v) for d in ds) <= spec["cut_height"] and max(d.dot(v) for d in ds) >= -spec["cut_height"]
                        and min(abs(d.dot(n)) for d in ds) < .08):
                    possible = True
            if possible:
                polygon = [{"co": list(obj.data.vertices[i].co), "uv": list(obj.data.uv_layers.active.data[loop].uv),
                            "normal": source_normals[loop], "source": i, "weights": surface.weights[i],
                            "color": list(status.data[i].color)}
                           for i, loop in zip(poly.vertices, poly.loop_indices)]
                parts = [polygon]
                for basis in eyes:
                    parts = [part for source in parts for part in subtract_eye_polygon(source, basis, spec["cut_width"], spec["cut_height"])]
                if len(parts) != 1 or parts[0] is not polygon:
                    removed.append(poly)
                    fragments.extend(parts)
                    continue
        retained.append(poly)
    builder = FaceMesh(obj, retained)
    for fragment in fragments:
        builder.fragment(fragment, obj.data.materials[0].name)
    for row in material_defs(spec):
        builder.add_material(row)
    additions = []
    for side, (p, n, u, v) in zip((-1, 1), eyes):
        before = len(builder.vertices)
        segments, rows = 96, 12
        ring_ids, ring_uvs = [], []
        radius = spec["globe_radius"]
        peak = -spec["recess"]

        def sphere_depth(x, z):
            eye_x = x - spec.get("gaze_offset", 0)
            return peak - radius + math.sqrt(max(radius * radius - eye_x * eye_x - z * z, .000001))

        # A curved lid aperture under a broad hood, with a lower tear rim.
        # The outer boundary meets sampled original skin; it is not a floating torus.
        for row in range(rows + 1):
            t = row / rows
            ids, uvs = [], []
            for i in range(segments):
                angle = 2 * math.pi * i / segments
                cosine, sine = math.cos(angle), math.sin(angle)
                inner_x = spec["opening_width"] * cosine
                inner_z = spec["opening_height"] * sine * (abs(sine) ** .18)
                if sine > 0:
                    inner_z *= spec.get("upper_opening_fraction", 1)
                inner_z += spec.get("corner_tilt", 0) * cosine
                x = inner_x * (1 - t) + spec["outer_width"] * cosine * t
                z = inner_z * (1 - t) + spec["outer_height"] * sine * t
                projection = p + u * x + v * z
                outer_hit, _, outer_weights = surface.sample(projection, n)
                depth = (outer_hit - projection).dot(n)
                # Sample pigment outside the removed textured eye, including Mire's painted highlight.
                radial = math.sqrt((x / spec["cut_width"]) ** 2 + (z / spec["cut_height"]) ** 2)
                pigment_factor = max(1, 1.12 / max(radial, .001))
                pigment_x, pigment_z = x * pigment_factor, z * pigment_factor
                _, pigment_uv, _ = surface.sample(p + u * pigment_x + v * pigment_z, n)
                skin_blend = smooth(t / .78)
                inner = p + u * inner_x + v * inner_z + n * sphere_depth(inner_x, inner_z)
                skin_point = inner.lerp(outer_hit, skin_blend)
                upper = max(sine, 0) ** .65
                lower = max(-sine, 0)
                lid_ridge = (.0015 + spec["brow_depth"] * upper + .0005 * lower) * math.sin(math.pi * t) ** 1.4
                crease = -.0015 * math.exp(-((t - .53) / .075) ** 2) * (upper + .35 * lower)
                wrinkles = .00022 * math.sin(angle * 17 + t * 8) * math.sin(math.pi * t) ** 2
                lift = .0007 * (1 - t) + lid_ridge + crease + wrinkles
                weights = {"head": 1 - smooth(t) ** 2}
                for name, weight in outer_weights.items():
                    weights[name] = weights.get(name, 0) + weight * smooth(t) ** 2
                color = surface.color(pigment_uv)
                shade = .92 + .08 * smooth(t)
                color = [c * shade for c in color[:3]] + [1]
                ids.append(builder.vertex(skin_point + n * lift, weights, color))
                uvs.append(pigment_uv)
            ring_ids.append(ids)
            ring_uvs.append(uvs)
        for row in range(rows):
            for i in range(segments):
                j = (i + 1) % segments
                ids = [ring_ids[row][i], ring_ids[row + 1][i], ring_ids[row + 1][j], ring_ids[row][j]]
                uvs = [ring_uvs[row][i], ring_uvs[row + 1][i], ring_uvs[row + 1][j], ring_uvs[row][j]]
                builder.face(ids, uvs, "M_OE_DreadSocket" if row == 0 else "M_OE_DreadSkin", side)

        # Small convex corneal surfaces, mostly hidden by the surrounding tissue.
        # Concentric sculpted iris relief supplies a restrained physical response.
        rings, cap_uvs = [], []
        eye_rings = 18
        disk_radius = spec["cut_width"] * 1.025
        for row in range(eye_rings + 1):
            r = max(row / eye_rings * disk_radius, .00001)
            ids, uvs = [], []
            for i in range(segments):
                angle = 2 * math.pi * i / segments
                x, z = r * math.cos(angle) + spec.get("gaze_offset", 0), r * math.sin(angle)
                depth = sphere_depth(x, z)
                if r < spec["iris_radius"]:
                    depth += .000025 * math.sin(angle * 37) * math.sin(math.pi * r / spec["iris_radius"])
                ids.append(builder.vertex(p + u * x + v * z + n * depth, {"head": 1}))
                uvs.append((x / disk_radius * .5 + .5, z / disk_radius * .5 + .5))
            rings.append(ids)
            cap_uvs.append(uvs)
        for row in range(eye_rings):
            r = (row + .5) / eye_rings * disk_radius
            mat = "M_OE_DreadPupil" if r < spec["pupil_radius"] else "M_OE_DreadIris" if r < spec["iris_radius"] else "M_OE_DreadEye"
            for i in range(segments):
                j = (i + 1) % segments
                builder.face([rings[row][i], rings[row + 1][i], rings[row + 1][j], rings[row][j]],
                             [cap_uvs[row][i], cap_uvs[row + 1][i], cap_uvs[row + 1][j], cap_uvs[row][j]], mat, side)
        additions.append({"side": side, "surface_center": list(p), "normal": list(n), "u": list(u), "v": list(v),
                          "added_vertices": len(builder.vertices) - before, "peak_recess_cm": spec["recess"] * 100})
    old = obj.data
    mesh = builder.finish()
    # Existing source vertex coordinates and skin must remain exact everywhere retained.
    new_weights = skin_weights(obj)
    retained_count = 0
    for i, old_id in enumerate(builder.original_ids):
        if old_id < 0:
            continue
        retained_count += 1
        if (mesh.vertices[i].co - old.vertices[old_id].co).length > 1e-9:
            raise RuntimeError("A retained anatomy vertex moved")
        if new_weights[i] != builder.old_weights[old_id]:
            raise RuntimeError("A retained anatomy skin assignment changed")
    return {"removed_faces": len(removed), "removed_triangles": sum(len(p.vertices) - 2 for p in removed),
            "retained_original_vertices": retained_count, "retained_positions_and_weights_exact": True,
            "boundary_fragments": len(fragments),
            "eyes": additions, "materials": material_defs(spec), "new_face_vertices": len(mesh.vertices) - retained_count}, old


def posed_preservation(obj, arm, baseline_mesh, rows):
    candidate = obj.data
    source_ids = np.asarray([item.value for item in candidate.attributes["dread_source_vertex"].data])
    retained = np.flatnonzero(source_ids >= 0)
    results = {}
    for name, values in [("neutral", (0, 0, 0)), ("jaw_open", (1, 0, 0)),
                         ("grip_full", (0, 1, 0)), ("crest_full", (.7, .35, 1))]:
        apply_channels(arm, rows, *values)
        evaluated = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
        mesh = evaluated.to_mesh()
        after = points(mesh)[retained]
        evaluated.to_mesh_clear()
        obj.data = baseline_mesh
        bpy.context.view_layer.update()
        evaluated = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
        mesh = evaluated.to_mesh()
        before = points(mesh)[source_ids[retained]]
        evaluated.to_mesh_clear()
        obj.data = candidate
        bpy.context.view_layer.update()
        delta = np.linalg.norm(after - before, axis=1)
        if delta.max() > .00001:
            raise RuntimeError("Retained body/jaw/digit deformation changed: " + name)
        results[name] = {"retained_vertices_compared": len(retained), "max_difference_cm": float(delta.max() * 100)}
    apply_channels(arm, rows)
    return results


def archive_candidate(output):
    if not (output / "prep-report.json").exists():
        return
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    saved = output / "iterations" / stamp
    saved.mkdir(parents=True, exist_ok=False)
    for path in output.iterdir():
        if path.is_file() and (path.suffix in (".fbx", ".blend", ".json", ".py") or path.name.startswith(("candidate-", "baseline-"))):
            shutil.copy2(path, saved / path.name)
    return saved


def render_faces(obj, arm, asset, output, rows, label, views):
    camera = setup_preview(obj, arm)
    bpy.context.scene.cycles.samples = 24
    bpy.data.lights["Key"].size = 1.2
    briar = asset == "Briarhide"
    head = (0, -.28, 1.69 if briar else 1.98)
    close = .57 if briar else .57
    cases = {
        "front": (0, 0, 0, head, (.12, -3, .07), close),
        "oblique": (0, 0, 0, head, (2, -3, .05), close),
        "side": (0, 0, 0, head, (3, -.5, .07), close),
        "jaw-open": (1, .25, 0, head, (1.2, -3, .04), .65),
        "crest-loaded": (.7, .35, 1, (0, -.03, 1.83), (.3, -3, .1), 1.10),
        "game": (.45, .25, .3, (0, -.02, 1.1), (.7, -4.5, .25), 2.6),
    }
    result = []
    for name in views:
        jaw, grip, crest, target, offset, size = cases[name]
        apply_channels(arm, rows, jaw, grip, crest)
        scene = bpy.context.scene
        scene.render.resolution_x = 1280 if name == "game" else 700
        scene.render.resolution_y = 800 if name == "game" else 700
        camera.data.type = "PERSP" if name == "game" else "ORTHO"
        if name == "game":
            camera.data.angle = math.radians(70)
        render_view(camera, output, label + "-" + name, target, offset, size)
        result.append({"view": name, "file": label + "-" + name + ".png",
                       "sha256": sha(output / (label + "-" + name + ".png")),
                       "projection": camera.data.type, "target": target, "offset": offset,
                       "jaw": jaw, "grip": grip, "crest": crest})
    apply_channels(arm, rows)
    return result


def prepare_candidate(obj, arm, asset, source, output, spec, spec_path, views):
    previous = json.loads((source / "prep-report.json").read_text())
    expected = spec["source_fbx_sha256"]
    if previous["fbx_sha256"] != expected or sha(source / (obj.name + ".fbx")) != expected:
        raise RuntimeError("Facial source is not the accepted anatomy export")
    rests = {b.name: (b.parent.name if b.parent else None, [list(row) for row in b.matrix_local]) for b in arm.data.bones}
    rows = previous["anatomy_rig"]["channel_bones"]
    archive_candidate(output)
    baseline_previews = render_faces(obj, arm, asset, output, rows, "baseline", views)
    changes, old_mesh = make_faces(obj, spec)
    preserved = posed_preservation(obj, arm, old_mesh, rows)
    current = {b.name: (b.parent.name if b.parent else None, [list(row) for row in b.matrix_local]) for b in arm.data.bones}
    if current != rests:
        raise RuntimeError("An accepted bone reference or parent changed")
    from prepare_organic_enemy import texture_files
    textures = texture_files(obj, output, previous["textures"])
    apply_channels(arm, rows)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    arm.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 1
    bpy.context.preferences.filepaths.save_version = 0
    fbx, blend = output / (obj.name + ".fbx"), output / (obj.name + ".blend")
    bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=True, object_types={"MESH", "ARMATURE"},
        axis_forward="-Y", axis_up="Z", apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
        bake_anim=False, use_mesh_modifiers=True, mesh_smooth_type="FACE", add_leaf_bones=False,
        use_armature_deform_only=True, path_mode="STRIP", colors_type="LINEAR")
    bpy.ops.wm.save_as_mainfile(filepath=str(blend))
    obj.data.calc_loop_triangles()
    report = dict(previous)
    region_names = previous["weights"].get("new_bone_regions", {})
    regions = {name: {"vertices_over_25_percent": 0, "weight_sum": 0.0} for name in region_names}
    for vertex in obj.data.vertices:
        for assignment in vertex.groups:
            name = obj.vertex_groups[assignment.group].name
            if name in regions:
                regions[name]["weight_sum"] += float(assignment.weight)
                regions[name]["vertices_over_25_percent"] += int(assignment.weight > .25)
    report.update({"fbx_sha256": sha(fbx), "blend_sha256": sha(blend), "textures": textures,
        "output_folder": "finished-dread", "appearance_revision": "dread-v1", "eye_placements": [],
        "appearance_eye_placements": changes["eyes"], "preparation_script_sha256": sha(__file__),
        "appearance_spec_sha256": sha(spec_path), "material_slots": [m.name for m in obj.data.materials],
        "extra_materials": previous["extra_materials"] + material_defs(spec),
        "runtime_triangles": len(obj.data.loop_triangles),
        "weights": dict(previous["weights"], vertices=len(obj.data.vertices),
                        max_influences=max(len(v.groups) for v in obj.data.vertices), new_bone_regions=regions,
                        method="Accepted anatomy skin preserved; existing triangle edges clipped locally at sockets; new fitted lids blend head attachment into sampled surrounding skin"),
        "preservation": {"prepared_source": str(source / blend.name),
                         "prepared_source_sha256": sha(source / blend.name),
                         "all_prior_bone_rest_matrices_unchanged": True, "all_prior_bone_parents_unchanged": True,
                         "contact_region_matches_original_exactly": True,
                         "intentional_geometry_edit": "Local source eye surfaces replaced with recessed eyes, fitted socket skin and actual lid/brow rims; original cosmetic Briar eyes removed",
                         "unchanged": ["retained original vertices and weights", "original maps and body/mouth materials", "all accepted body/jaw/digit bone rests and parents"]},
        "appearance_preservation": {"baseline_fbx_sha256": expected,
            "baseline_blend_sha256": sha(source / blend.name),
            "all_bone_rests_and_parents_exact": True, "retained_pose_comparisons": preserved,
            "changes": changes, "original_maps_byte_identical": True,
            "intentional_removal": "Protruding cosmetic Briar irises/pupils and local original eye surfaces, including Mire's painted highlight",
            "unchanged": ["retained original vertex positions and skin weights", "body/jaw/digit rig names, rests and hierarchy",
                          "retained source face UVs and corner normals", "original body, oral, tooth and tongue materials"]},
        "limits": "CPU Blender facial candidate; no in-game or owner acceptance. No new eyelid animation or gaze joints. Dense original body and four-influence skin remain. Local removed eye surfaces are intentionally replaced; no body/jaw/digit remesh.",
        "preview_renders": [], "appearance_previews": {"baseline": baseline_previews}})
    (output / "prep-report.json").write_text(json.dumps(report, indent=2) + "\n")
    report["appearance_previews"]["candidate"] = render_faces(obj, arm, asset, output, rows, "candidate", views)
    report["preview_renders"] = [r["file"] for r in report["appearance_previews"]["candidate"]]
    (output / "prep-report.json").write_text(json.dumps(report, indent=2) + "\n")
    shutil.copy2(spec_path, output / "appearance-spec.json")
    shutil.copy2(__file__, output / "preparation-source.py")
    print("DREAD_PREPARED " + json.dumps({"asset": asset, "fbx_sha256": report["fbx_sha256"],
          "triangles": report["runtime_triangles"], "changes": changes}), flush=True)


def mesh_arrays(mesh):
    result = {"positions": points(mesh)}
    for key, data, field, width, dtype in [
        ("loops", mesh.loops, "vertex_index", 1, np.int32),
        ("polygon_starts", mesh.polygons, "loop_start", 1, np.int32),
        ("polygon_sizes", mesh.polygons, "loop_total", 1, np.int32),
        ("materials", mesh.polygons, "material_index", 1, np.int32),
        ("uvs", mesh.uv_layers.active.data, "uv", 2, np.float32),
        ("normals", mesh.corner_normals, "vector", 3, np.float32),
    ]:
        values = np.empty(len(data) * width, dtype=dtype)
        data.foreach_get(field, values)
        result[key] = values
    return result


def non_color_fingerprint(obj, arm):
    digest = hashlib.sha256()
    for name, values in mesh_arrays(obj.data).items():
        digest.update(name.encode())
        digest.update(values.tobytes())
    assignments = [(v.index, g.group, g.weight) for v in obj.data.vertices for g in v.groups]
    digest.update(np.asarray(assignments, dtype=np.float64).tobytes())
    digest.update(json.dumps({
        "groups": [g.name for g in obj.vertex_groups],
        "materials": [m.name for m in obj.data.materials],
        "object_matrix": [list(r) for r in obj.matrix_world],
        "armature_matrix": [list(r) for r in arm.matrix_world],
        "bones": [(b.name, b.parent.name if b.parent else None, [list(r) for r in b.matrix_local])
                  for b in arm.data.bones],
    }, sort_keys=True).encode())
    return digest.hexdigest()


def color_values(attribute):
    values = np.empty(len(attribute.data) * 4, dtype=np.float32)
    attribute.data.foreach_get("color", values)
    return values.reshape((-1, 4))


def exported_first_colors(fbx, expected, body_loops):
    from io_scene_fbx import parse_fbx
    tree, version = parse_fbx.parse(str(fbx))
    objects = next(e for e in tree.elems if e.id == b"Objects")
    geometry = next(e for e in objects.elems if e.id == b"Geometry" and e.props[-1] == b"Mesh")
    layers = [e for e in geometry.elems if e.id == b"LayerElementColor"]
    first = next(e for e in layers if e.props[0] == 0)
    children = {e.id: e for e in first.elems}
    name = children[b"Name"].props[0].decode()
    mapping = children[b"MappingInformationType"].props[0].decode()
    reference = children[b"ReferenceInformationType"].props[0].decode()
    if (name, mapping, reference) != ("DreadFaceColor", "ByPolygonVertex", "IndexToDirect"):
        raise RuntimeError("Unexpected first FBX color-layer contract")
    direct = np.asarray(children[b"Colors"].props[0]).reshape((-1, 4))
    indices = np.asarray(children[b"ColorIndex"].props[0])
    expanded = direct[indices]
    if not np.array_equal(expanded, expected):
        raise RuntimeError("The exported first FBX color set differs from authored corner RGBA")
    body = expanded[body_loops]
    return {"fbx_version": version, "first_layer": name, "mapping": mapping,
            "reference": reference, "all_corner_rgba_exact": True,
            "body_corners": len(body), "body_min_rgba": body.min(axis=0).tolist(),
            "body_max_rgba": body.max(axis=0).tolist(),
            "body_r_equal_one": int(np.count_nonzero(body[:, 0] == 1)),
            "body_r_nonzero_corners": int(np.count_nonzero(body[:, 0]))}


def refresh_color_stream(obj, arm, asset, source, output, spec, spec_path):
    """Patch only colors on the reviewed mesh; geometry/skin remain in-place."""
    report_path = output / "prep-report.json"
    report = json.loads(report_path.read_text())
    fbx, blend = output / (obj.name + ".fbx"), output / (obj.name + ".blend")
    if sha(fbx) != report["fbx_sha256"] or sha(blend) != report["blend_sha256"]:
        raise RuntimeError("Reviewed candidate files do not match their recorded freeze")
    if sha(source / fbx.name) != spec["source_fbx_sha256"]:
        raise RuntimeError("Authoritative anatomy FBX changed")
    for texture in report["textures"]:
        if sha(output / texture["file"]) != texture["sha256"]:
            raise RuntimeError("Original texture bytes changed")

    # The repaired regular preparation path derives exact status values on
    # retained vertices and affine values on clipped triangle boundaries.
    # Only those derived color streams are transferred to the existing mesh.
    make_faces(obj, spec)
    generated = mesh_arrays(obj.data)
    generated_ids = np.asarray([x.value for x in obj.data.attributes["dread_source_vertex"].data])
    streams = [(a.name, a.data_type, a.domain, color_values(a)) for a in obj.data.color_attributes]
    bpy.ops.wm.open_mainfile(filepath=str(blend))
    obj = bpy.data.objects["SK_OE_" + asset]
    arm = obj.find_armature()
    current = mesh_arrays(obj.data)
    for name in ("positions", "loops", "polygon_starts", "polygon_sizes", "materials", "uvs"):
        if not np.array_equal(generated[name], current[name]):
            raise RuntimeError("Regenerated color correspondence changed reviewed " + name)
    current_ids = np.asarray([x.value for x in obj.data.attributes["dread_source_vertex"].data])
    if not np.array_equal(generated_ids, current_ids):
        raise RuntimeError("Regenerated source-vertex correspondence changed")
    before = non_color_fingerprint(obj, arm)
    old_color = obj.data.color_attributes["DreadFaceColor"]
    old_values = color_values(old_color)
    old_corners = old_values[current["loops"]] if old_color.domain == "POINT" else old_values
    skin_loops = [i for p in obj.data.polygons
                  if obj.data.materials[p.material_index].name == "M_OE_DreadSkin" for i in p.loop_indices]
    if not np.array_equal(old_corners[skin_loops], streams[0][3][skin_loops]):
        raise RuntimeError("Color repair changed the reviewed eyelid pigment")
    archived = archive_candidate(output)
    for attribute in list(obj.data.color_attributes):
        obj.data.color_attributes.remove(attribute)
    for name, data_type, domain, values in streams:
        attribute = obj.data.color_attributes.new(name=name, type=data_type, domain=domain)
        attribute.data.foreach_set("color", values.reshape(-1))
    obj.data.color_attributes.active_color_index = 0
    obj.data.color_attributes.render_color_index = 0
    if non_color_fingerprint(obj, arm) != before:
        raise RuntimeError("Color repair changed reviewed geometry, normals, UVs, skin or rest transforms")

    body_loops = [i for p in obj.data.polygons if p.material_index == 0 for i in p.loop_indices]
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    arm.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=True, object_types={"MESH", "ARMATURE"},
        axis_forward="-Y", axis_up="Z", apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
        bake_anim=False, use_mesh_modifiers=True, mesh_smooth_type="FACE", add_leaf_bones=False,
        use_armature_deform_only=True, path_mode="STRIP", colors_type="LINEAR")
    exported = exported_first_colors(fbx, streams[0][3], body_loops)
    bpy.ops.wm.save_as_mainfile(filepath=str(blend))
    report["color_stream_correction"] = {
        "revision": "status-mask-v2", "previous_fbx_sha256": report["fbx_sha256"],
        "previous_blend_sha256": report["blend_sha256"], "archived_candidate": str(archived),
        "authoritative_source_color": "OrganicTell", "new_first_color_domain": "CORNER",
        "non_color_mesh_rig_fingerprint_before": before,
        "non_color_mesh_rig_fingerprint_after": non_color_fingerprint(obj, arm),
        "reviewed_geometry_uvs_normals_weights_rests_exact": True,
        "reviewed_dread_skin_pigment_exact": True,
        "body_mask_method": "Retained source RGBA copied exactly; cut triangle fragments use affine interpolation of the original OrganicTell RGBA; skin-atlas pigment only on DreadSkin corners",
        "fbx_first_color_validation": exported,
        "preview_reuse": "Existing CPU previews predate this status-mask-only correction; exact reviewed geometry and DreadSkin pigment preserved. No new render or in-game acceptance claimed.",
    }
    report.update({"fbx_sha256": sha(fbx), "blend_sha256": sha(blend),
                   "preparation_script_sha256": sha(__file__),
                   "appearance_spec_sha256": sha(spec_path)})
    report_path.write_text(json.dumps(report, indent=2) + "\n")
    shutil.copy2(__file__, output / "preparation-source.py")
    print("DREAD_COLOR_CORRECTED " + json.dumps({"asset": asset, "fbx_sha256": report["fbx_sha256"],
          "blend_sha256": report["blend_sha256"], "correction": report["color_stream_correction"]}), flush=True)


def refresh_eye_materials(asset, output, spec, spec_path):
    """Apply owner-requested iris glow to the frozen model without remeshing."""
    report_path = output / "prep-report.json"
    report = json.loads(report_path.read_text())
    fbx, blend = output / report["fbx"], output / ("SK_OE_" + asset + ".blend")
    if sha(fbx) != report["fbx_sha256"] or sha(blend) != report["blend_sha256"]:
        raise RuntimeError("Eye update requires the recorded frozen candidate")
    bpy.ops.wm.open_mainfile(filepath=str(blend))
    obj = bpy.data.objects["SK_OE_" + asset]
    arm = obj.find_armature()
    before = non_color_fingerprint(obj, arm)
    colors = [color_values(a) for a in obj.data.color_attributes]
    color_digest = hashlib.sha256(b"".join(a.tobytes() for a in colors)).hexdigest()
    archived = archive_candidate(output)
    eye_names = {"M_OE_DreadEye", "M_OE_DreadIris", "M_OE_DreadPupil"}
    definitions = {row["name"]: row for row in material_defs(spec) if row["name"] in eye_names}
    for name, row in definitions.items():
        mat = obj.data.materials[name]
        shader = next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
        for key, value in [("Base Color", row["base_color"]), ("Roughness", row["roughness"]),
                           ("Specular IOR Level", row["specular"]),
                           ("Emission Color", row["emissive_color"] + [1]),
                           ("Emission Strength", row["emissive_strength"])]:
            shader.inputs[key].default_value = value
        mat.diffuse_color = row["base_color"]
    if non_color_fingerprint(obj, arm) != before:
        raise RuntimeError("Eye material update changed frozen geometry or skin")
    if any(not np.array_equal(old, color_values(a)) for old, a in zip(colors, obj.data.color_attributes)):
        raise RuntimeError("Eye material update changed the repaired status/color streams")
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    arm.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=True, object_types={"MESH", "ARMATURE"},
        axis_forward="-Y", axis_up="Z", apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
        bake_anim=False, use_mesh_modifiers=True, mesh_smooth_type="FACE", add_leaf_bones=False,
        use_armature_deform_only=True, path_mode="STRIP", colors_type="LINEAR")
    body_loops = [i for p in obj.data.polygons if p.material_index == 0 for i in p.loop_indices]
    exported = exported_first_colors(fbx, colors[0], body_loops)
    bpy.ops.wm.save_as_mainfile(filepath=str(blend))
    report["eye_material_update"] = {
        "revision": spec["eye_material_revision"],
        "request": "Owner explicitly requested fiery eyes for MireSeer and glowing green eyes for Briarhide",
        "previous_fbx_sha256": report["fbx_sha256"], "previous_blend_sha256": report["blend_sha256"],
        "archived_candidate": str(archived), "changed_slots": sorted(eye_names),
        "geometry_uvs_normals_weights_rests_exact": True,
        "non_color_mesh_rig_fingerprint": before, "all_color_streams_sha256": color_digest,
        "fbx_first_color_validation": exported,
        "engine_procedure": "Update the three existing per-species eye materials from extra_materials; multiply linear emissive_color by emissive_strength into Emissive Color. Existing mesh/skeleton can remain loaded because geometry/slots/color streams are unchanged.",
        "acceptance": "Source material candidate; actual Unreal visibility and owner acceptance pending",
    }
    report["extra_materials"] = [definitions.get(row["name"], row) for row in report["extra_materials"]]
    report["appearance_preservation"]["changes"]["materials"] = material_defs(spec)
    report.update({"fbx_sha256": sha(fbx), "blend_sha256": sha(blend),
                   "preparation_script_sha256": sha(__file__), "appearance_spec_sha256": sha(spec_path)})
    report["eye_material_previews"] = render_faces(obj, arm, asset, output,
        report["anatomy_rig"]["channel_bones"], "luminous", ["oblique"])
    report_path.write_text(json.dumps(report, indent=2) + "\n")
    shutil.copy2(__file__, output / "preparation-source.py")
    shutil.copy2(spec_path, output / "appearance-spec.json")
    print("DREAD_EYES_UPDATED " + json.dumps({"asset": asset, "fbx_sha256": report["fbx_sha256"],
          "blend_sha256": report["blend_sha256"], "materials": list(definitions.values())}), flush=True)


def open_source(asset, folder):
    path = folder / ("SK_OE_" + asset + ".blend")
    bpy.ops.wm.open_mainfile(filepath=str(path))
    obj = bpy.data.objects["SK_OE_" + asset]
    arm = obj.find_armature()
    expected = 38 if asset == "Briarhide" else 43
    if not arm or len(arm.data.bones) != expected:
        raise RuntimeError("Expected the final accepted anatomy rig")
    for bone in arm.pose.bones:
        bone.matrix_basis = Matrix.Identity(4)
    bpy.context.view_layer.update()
    return obj, arm


def ray_pixel(obj, camera, x, y):
    scene = bpy.context.scene
    width, height = scene.render.resolution_x, scene.render.resolution_y
    scale = camera.data.ortho_scale
    origin = camera.matrix_world @ Vector(((x / width - .5) * scale,
                                           (.5 - y / height) * scale * height / width, 0))
    direction = camera.matrix_world.to_3x3() @ Vector((0, 0, -1))
    tree = BVHTree.FromObject(obj, bpy.context.evaluated_depsgraph_get())
    hit, normal, index, distance = tree.ray_cast(origin, direction)
    if hit is None:
        return {"pixel": [x, y], "hit": None}
    return {"pixel": [x, y], "hit": list(hit), "normal": list(normal), "face": index,
            "material": obj.data.materials[obj.data.polygons[index].material_index].name}


def inspect_source(obj, arm, asset, output):
    camera = setup_preview(obj, arm)
    briar = asset == "Briarhide"
    head = (0, -.28, 1.65 if briar else 1.94)
    views = {
        "front": (head, (.25, -3, .14), .54 if briar else .65),
        "side": ((0, -.20, 1.70 if briar else 1.94), (3, -.5, .08), .62 if briar else .76),
    }
    samples = {}
    for name, values in views.items():
        render_view(camera, output, "baseline-" + name, *values)
        pixels = ([(201, 227), (463, 228)] if briar else [(246, 280), (466, 285)]) if name == "front" else ([(90, 295)] if briar else [(375, 294), (342, 292), (399, 294)])
        samples[name] = [ray_pixel(obj, camera, *pixel) for pixel in pixels]
    (output / "source-inspection.json").write_text(json.dumps({"asset": asset,
        "source_blend_sha256": sha(output.parent / "finished-anatomy" / (obj.name + ".blend")),
        "object_matrix": [list(row) for row in obj.matrix_world],
        "armature_matrix": [list(row) for row in arm.matrix_world],
        "eye_ray_samples": samples}, indent=2) + "\n")
    print("FACE_SOURCE_INSPECTED " + json.dumps(samples), flush=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--asset", choices=("Briarhide", "MireSeer"), required=True)
    parser.add_argument("--inspect-only", action="store_true")
    parser.add_argument("--refresh-color-stream", action="store_true")
    parser.add_argument("--refresh-eye-materials", action="store_true")
    parser.add_argument("--views", default="front,oblique,side,jaw-open,game")
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
    config = json.loads((STUDIO / "config.local.json").read_text(encoding="utf-8"))
    folder = "briarhide" if args.asset == "Briarhide" else "mire-seer"
    private = Path(config["tools"]["ai3d"]["windowsOutputRoot"]) / "organic-enemies" / folder
    output = private / "finished-dread"
    output.mkdir(parents=True, exist_ok=True)
    obj, arm = open_source(args.asset, private / "finished-anatomy")
    if args.inspect_only:
        inspect_source(obj, arm, args.asset, output)
    else:
        spec_path = ROOT / "art/organic-enemies/appearance" / (args.asset + ".json")
        spec = json.loads(spec_path.read_text())
        if args.refresh_eye_materials:
            refresh_eye_materials(args.asset, output, spec, spec_path)
        elif args.refresh_color_stream:
            refresh_color_stream(obj, arm, args.asset, private / "finished-anatomy", output, spec, spec_path)
        else:
            prepare_candidate(obj, arm, args.asset, private / "finished-anatomy", output, spec, spec_path, args.views.split(","))


if __name__ == "__main__":
    main()

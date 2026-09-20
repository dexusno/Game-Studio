"""Original curved far-background card. Run with the configured Blender.

The generated image stays unchanged. This script authors only its world-space
support mesh, UVs and interchange evidence; it is not an image editor.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys

import bpy

args = argparse.ArgumentParser()
args.add_argument("--output", type=Path, required=True)
opts = args.parse_args(sys.argv[sys.argv.index("--") + 1:])
out = opts.output.resolve()
out.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system = "METRIC"
bpy.context.scene.unit_settings.scale_length = 1

# A 120-degree arc covers both authored cameras. In world space their headings
# are roughly -90 and -33 degrees. Blender Y is reflected by our Unreal import.
segments, radius, start, sweep = 60, 200.0, -122.0, 124.0
height = math.radians(sweep) * radius / 3.0
bottom = 1.8 - .52 * height
verts, faces = [], []
for i in range(segments + 1):
    angle = math.radians(start + sweep * i / segments)
    x, y = radius * math.cos(angle), -radius * math.sin(angle)
    verts.extend(((x, y, bottom), (x, y, bottom + height)))
for i in range(segments):
    faces.append((2 * i, 2 * i + 2, 2 * i + 3, 2 * i + 1))
mesh = bpy.data.meshes.new("CinderwallBackdropV001")
mesh.from_pydata(verts, [], faces)
mesh.update()
uv = mesh.uv_layers.new(name="UVMap")
for poly in mesh.polygons:
    for loop in poly.loop_indices:
        vertex = mesh.loops[loop].vertex_index
        uv.data[loop].uv = ((vertex // 2) / segments, vertex % 2)
obj = bpy.data.objects.new("CinderwallBackdropV001", mesh)
bpy.context.collection.objects.link(obj)
bpy.context.view_layer.objects.active = obj
obj.select_set(True)
material = bpy.data.materials.new("M_CinderwallBackdropV001")
mesh.materials.append(material)
bpy.ops.wm.save_as_mainfile(filepath=str(out / "CinderwallBackdropV001.blend"))
fbx = out / "CinderwallBackdropV001.fbx"
bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=True, object_types={"MESH"},
                        axis_forward="-Y", axis_up="Z", use_mesh_modifiers=True,
                        add_leaf_bones=False, bake_anim=False)
source_bounds = [min(v[j] for v in verts) for j in range(3)] + [max(v[j] for v in verts) for j in range(3)]
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=str(fbx))
imported = [o for o in bpy.context.scene.objects if o.type == "MESH"]
assert len(imported) == 1
check = imported[0]
world = [check.matrix_world @ v.co for v in check.data.vertices]
actual_bounds = [min(v[j] for v in world) for j in range(3)] + [max(v[j] for v in world) for j in range(3)]
assert len(check.data.vertices) == len(verts)
assert len(check.data.polygons) == segments
assert len(check.data.uv_layers) == 1
assert all(abs(a - b) < .002 for a, b in zip(source_bounds, actual_bounds))
report = {"schema": 1, "asset": "Cinderwall curved distant-city card v001",
          "creator": "Game Studio / Codex", "generator": "Blender " + bpy.app.version_string,
          "mesh": fbx.name, "sha256": hashlib.sha256(fbx.read_bytes()).hexdigest(),
          "vertices": len(verts), "triangles": segments * 2,
          "radius_m": radius, "world_azimuth_start_deg": start,
          "world_azimuth_sweep_deg": sweep, "height_m": height,
          "bounds_m": source_bounds, "fbx_reimport_bounds_m": actual_bounds,
          "fbx_reimport_passed": True,
          "image": "assets/production/cinderwall-backdrop-v001/cinderwall-distant-city.png",
          "camera_contract": "World-space geometry remains fixed during the 16B/16C transition; no camera-facing billboard."}
(out / "mesh-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
print("CINDERWALL_BACKDROP_FBX_OK " + str(out / "mesh-report.json"))

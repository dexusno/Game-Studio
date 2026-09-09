"""Prepare one original TRELLIS Bellroot GLB for the shared Unreal PBR material.

Blender --background --threads 4 --python-exit-code 1 --python this.py --
  --asset BellTree --source /private/bell-tree/source/textured.glb
  --output /private/bell-tree/finished

CPU authoring only: no renderer, generation, remeshing, atlas bake or broad repair.
Inputs remain untouched. Inspect the actual generated mesh in Unreal before use.
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


SIZES = {
    "BellTree": {"axis": 2, "metres": 7.0},
    "Cloister": {"axis": 2, "metres": 5.0},
    "RootRock": {"axis": 0, "metres": 2.8},
}


def mesh_coordinates(mesh):
    coordinates = np.empty(len(mesh.vertices) * 3, dtype=np.float32)
    mesh.vertices.foreach_get("co", coordinates)
    return coordinates.reshape(-1, 3)


def collision_box(name, lower, upper):
    x, y, z = lower
    X, Y, Z = upper
    if min(X - x, Y - y, Z - z) < 0.025:
        return None
    vertices = [(x,y,z), (X,y,z), (X,Y,z), (x,Y,z),
                (x,y,Z), (X,y,Z), (X,Y,Z), (x,Y,Z)]
    faces = [(0,3,2,1), (4,5,6,7), (0,1,5,4),
             (1,2,6,5), (2,3,7,6), (3,0,4,7)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def arch_opening(obj, lower, upper):
    """Sample the unobstructed center interval through the entire arch depth.

    Approximate placement guidance at 2.5cm horizontal / 10cm vertical spacing;
    this is neither capsule collision testing nor a claim of playable clearance.
    """
    tree = BVHTree.FromObject(obj, bpy.context.evaluated_depsgraph_get())
    xs = np.linspace(lower[0] + .01, upper[0] - .01, 181)
    center_index = int(np.argmin(np.abs(xs)))
    rows = []
    for z in np.arange(.1, upper[2] - .02, .1):
        clear = []
        for x in xs:
            point, _, _, _ = tree.ray_cast(
                Vector((float(x), float(lower[1] - .2), float(z))),
                Vector((0, 1, 0)), float(upper[1] - lower[1] + .4))
            clear.append(point is None)
        if not clear[center_index]:
            rows.append({"z": round(float(z), 3), "left": 0.0,
                         "right": 0.0, "width": 0.0})
            continue
        left = right = center_index
        while left > 0 and clear[left - 1]:
            left -= 1
        while right < len(xs) - 1 and clear[right + 1]:
            right += 1
        # Last clear samples are conservative estimates of the visible void.
        rows.append({"z": round(float(z), 3), "left": round(float(xs[left]), 3),
                     "right": round(float(xs[right]), 3),
                     "width": round(float(xs[right] - xs[left]), 3)})
    walk_rows = [row for row in rows if .1 <= row["z"] <= 3.0]
    minimum_width = min(row["width"] for row in walk_rows)
    center_height = 0.0
    for row in rows:
        if row["width"] == 0:
            break
        center_height = row["z"]
    return {"method": "Through-depth rays; 181 horizontal samples and 10cm vertical spacing",
            "minimum_center_width_below_3m": minimum_width,
            "center_clear_height_approx": center_height,
            "requested_3_5m_by_3m_gate_clearance_met": minimum_width >= 3.5,
            "placement": "Gate candidate; engine collision check still required" if minimum_width >= 3.5 else
                         "Use as optional edge/corner framing; do not place across required wide route",
            "rows": rows}


def make_collision(asset, obj, coordinates, lower, upper):
    boxes = []
    opening = None
    if asset == "Cloister":
        opening = arch_opening(obj, lower, upper)
        # Separate piers and stepped upper sides. In every band use its widest
        # observed opening so collision does not narrow the visible passage.
        levels = [0, .75, 1.5, 2.25, 3.0, 3.6, 4.2, 5.0]
        for bottom, top in zip(levels, levels[1:]):
            rows = [r for r in opening["rows"] if bottom <= r["z"] <= top and r["width"] > 0]
            if not rows:
                # A cap is valid only wholly above the observed center opening.
                if bottom > opening["center_clear_height_approx"] + .1:
                    boxes.append(((lower[0],lower[1],bottom),(upper[0],upper[1],top)))
                continue
            left = min(row["left"] for row in rows) - .035
            right = max(row["right"] for row in rows) + .035
            boxes.extend([((lower[0],lower[1],bottom),(left,upper[1],top)),
                          ((right,lower[1],bottom),(upper[0],upper[1],top))])
    else:
        # Coarse obstacle envelopes. Decorative root gaps are not walk-through
        # tunnels. Tree canopy is omitted; boulder tiers follow its full height.
        height = float(upper[2])
        levels = [(0,.75),(.65,2.7),(2.6,4.6)] if asset == "BellTree" else [
            (0,.4*height),(.3*height,.72*height),(.64*height,height)]
        for bottom, top in levels:
            points = coordinates[(coordinates[:,2] >= bottom) & (coordinates[:,2] <= top)]
            if len(points):
                lo, hi = points.min(axis=0), points.max(axis=0)
                boxes.append(((float(lo[0]),float(lo[1]),bottom),
                              (float(hi[0]),float(hi[1]),min(top,float(upper[2])))))
    hulls = []
    for lo, hi in boxes:
        name = f"UCX_{obj.name}_{len(hulls):02}"
        hull = collision_box(name, lo, hi)
        if hull is not None:
            hulls.append(hull)
    if not hulls:
        raise RuntimeError("No useful collision pieces could be authored")
    return hulls, opening


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--asset", choices=SIZES, required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--yaw-degrees", type=float, default=0,
                        help="Optional corrective Blender Z rotation before measuring; default preserves TRELLIS front")
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
    source = args.source.resolve()
    if source.suffix.lower() != ".glb" or not source.is_file():
        raise RuntimeError("Provide the completed source textured.glb")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.import_scene.gltf(filepath=str(source))
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    if not meshes:
        raise RuntimeError("GLB contains no mesh")
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
    obj.name = "SM_Trellis_" + args.asset
    obj.data.name = obj.name + "_Mesh"
    if len(obj.data.materials) != 1:
        raise RuntimeError("Expected the single TRELLIS atlas material; inspect unexpected material slots")
    if not obj.data.uv_layers:
        raise RuntimeError("Source has no texture UVs")
    initial = mesh_coordinates(obj.data)
    lower, upper = initial.min(axis=0), initial.max(axis=0)
    before = upper - lower
    target = SIZES[args.asset]
    # Gameplay targets are approximate envelopes. Preserve source proportions,
    # especially the circular bell and architectural profiles.
    sx = sy = sz = target["metres"] / before[target["axis"]]
    transform = Matrix.Diagonal(Vector((float(sx),float(sy),float(sz),1))) @ Matrix.Translation(
        Vector((-float((lower[0]+upper[0])/2),-float((lower[1]+upper[1])/2),-float(lower[2]))))
    obj.data.transform(transform)
    obj.data.update()
    obj.data.calc_loop_triangles()
    uv0 = obj.data.uv_layers[0]
    uv0.name = "UVMap"
    # Source GLB is untouched; new meshes use zero repair weight everywhere.
    for layer in list(obj.data.uv_layers)[1:]:
        obj.data.uv_layers.remove(layer)
    uv1 = obj.data.uv_layers.new(name="RepairDonorUV")
    uvs = np.empty(len(obj.data.loops) * 2, dtype=np.float32)
    uv0.data.foreach_get("uv", uvs)
    uv1.data.foreach_set("uv", uvs)
    obj.data.uv_layers.active_index = 0
    for attr in list(obj.data.color_attributes):
        obj.data.color_attributes.remove(attr)
    color = obj.data.color_attributes.new(name="RepairBlend", type="FLOAT_COLOR", domain="CORNER")
    values = np.zeros((len(obj.data.loops),4), dtype=np.float32)
    values[:,3] = 1
    color.data.foreach_set("color", values.reshape(-1))
    obj.data.color_attributes.active_color = color
    material = obj.data.materials[0]
    material.name = "M_Trellis_" + args.asset
    images = list({node.image for node in material.node_tree.nodes if node.type == "TEX_IMAGE" and node.image})
    base = next((im for im in images if im.colorspace_settings.name == "sRGB"), None)
    packed = next((im for im in images if im != base), None)
    if len(images) != 2 or base is None or packed is None:
        raise RuntimeError("Expected original base-color and metallic-roughness texture pair")
    textures = []
    for image, filename in [(base,"base_color.png"),(packed,"metallic_roughness.png")]:
        if not image.packed_file:
            image.pack()
        data = bytes(image.packed_file.data)
        (output / filename).write_bytes(data)
        image.filepath = "//" + filename
        textures.append({"file":filename,"size":list(image.size),"sha256":hashlib.sha256(data).hexdigest(),
                         "source": "Unchanged embedded GLB image; no re-encoding or baking"})
    coordinates = mesh_coordinates(obj.data)
    lower, upper = coordinates.min(axis=0), coordinates.max(axis=0)
    footprints = {}
    for height in (1.0, 2.0):
        points = coordinates[coordinates[:,2] <= height]
        if len(points):
            footprints[f"below_{height:g}m"] = {
                "min_xy":[round(float(v),4) for v in points.min(axis=0)[:2]],
                "max_xy":[round(float(v),4) for v in points.max(axis=0)[:2]],
                "width_depth":[round(float(v),4) for v in (points.max(axis=0)-points.min(axis=0))[:2]]}
    hulls, opening = make_collision(args.asset, obj, coordinates, lower, upper)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    for hull in hulls:
        hull.select_set(True)
    bpy.context.view_layer.objects.active = obj
    fbx = output / (obj.name + ".fbx")
    bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=True, object_types={"MESH"},
        axis_forward="-Y", axis_up="Z", apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
        bake_anim=False, use_mesh_modifiers=True, mesh_smooth_type="FACE", add_leaf_bones=False,
        path_mode="STRIP", colors_type="LINEAR")
    for hull in hulls:
        hull.hide_render = True
        hull.hide_set(True)
    bpy.ops.wm.save_as_mainfile(filepath=str(output / (obj.name + ".blend")))
    report = {"asset":args.asset,"source":str(source),"source_sha256":hashlib.sha256(source.read_bytes()).hexdigest(),
              "fbx":str(fbx),"dimensions_metres":[round(float(v),4) for v in upper-lower],
              "footprints":footprints,
              "pivot":"XY bounds center; minimum Z=0; transforms baked in metres",
              "normalization_scale_xyz":[float(sx),float(sy),float(sz)],"yaw_correction_degrees":args.yaw_degrees,
              "front_axes":"Blender -Y; expected Unreal +Y using approved -Y/Z FBX and convert_scene=true; X width/Z up",
              "triangles":len(obj.data.loop_triangles),"collision_hulls":len(hulls),
              "collision":"Separate box envelopes; arch void preserved; tree canopy omitted; boulder tiers follow full height",
              "material":{"slots":1,"UV0":"Original atlas","UV1":"Exact UV0 duplicate", "VertexColor.R":"RepairBlend, linear 0 everywhere",
                          "base_color":"sRGB RGB","packed_MR":"linear; roughness G, metallic B","textures":textures},
              "arch_opening":opening,"cleanup":"None; only dimensional normalization, pivot and export preparation",
              "verification":"Blender import/export and dimensional/material checks only; no render or engine verification",
              "rights":"Original OpenAI image_gen reference; local TRELLIS.2 generation; existing model/runtime terms retained; local evaluation"}
    (output / "prep-report.json").write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    print("TRELLIS_KIT_PREPARED " + json.dumps({k:report[k] for k in ("asset","fbx","dimensions_metres","triangles","collision_hulls")}),flush=True)


if __name__ == "__main__":
    main()

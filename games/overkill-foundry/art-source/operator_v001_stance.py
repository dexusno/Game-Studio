"""CPU source comparison of the new operator with the existing Mara gun.

Appends source art for inspection only. Never edits the gun source or runtime.
"""
import argparse
import json
from pathlib import Path
import sys
import bpy
from mathutils import Vector

p = argparse.ArgumentParser()
p.add_argument("--output", type=Path, required=True)
a = p.parse_args(sys.argv[sys.argv.index("--") + 1:])
out = a.output.resolve()
gun_file = out.parent / "mara-v001/Mara_Source.blend"
bpy.ops.wm.open_mainfile(filepath=str(out / "Mara_Operator_Source.blend"))
scene = bpy.context.scene
scene.cycles.device = "CPU"
scene.cycles.samples = 24
scene.render.threads_mode = "FIXED"
scene.render.threads = 6
with bpy.data.libraries.load(str(gun_file), link=False) as (source, target):
    target.objects = [name for name in source.objects if name in ("SK_MaraGun", "SKM_MaraGun")]
if len(target.objects) != 2:
    raise RuntimeError("Expected unchanged existing gun armature/mesh source")
for obj in target.objects:
    bpy.context.collection.objects.link(obj)
    obj.hide_render = False
    obj.hide_set(False)
gun = bpy.data.objects["SK_MaraGun"]
gun.location = (1.46, .55, .008)
gun.animation_data.action = bpy.data.actions["MG_idle"]
operator = bpy.data.objects["SK_MaraOperator"]
operator.animation_data.action = bpy.data.actions["MO_idle"]
scene.frame_set(1)
bpy.ops.mesh.primitive_plane_add(size=20, location=(0, 0, -.013))
floor = bpy.context.object
floor.name = "SourceReviewFloor"
floor_material = bpy.data.materials.new("SourceReviewFloorMat")
floor_material.diffuse_color = (.07, .08, .085, 1)
floor.data.materials.append(floor_material)


def render(name, location, target, scale):
    data = bpy.data.cameras.new(name)
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    obj.location = location
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat("-Z", "Y").to_euler()
    data.type = "ORTHO"
    data.ortho_scale = scale
    scene.camera = obj
    scene.render.resolution_x, scene.render.resolution_y = 1440, 900
    scene.render.resolution_percentage = 100
    scene.render.filepath = str(out / "renders" / (name + ".png"))
    bpy.ops.render.render(write_still=True)


render("operator_gun_preparation", (1.0, -6.5, 2.45), (1.1, .35, 1), 5.25)
render("operator_gun_action", (-4.5, -5.5, 2.9), (1.0, .25, 1.1), 4.6)
grips = {name: list(operator.matrix_world @ operator.pose.bones[name].matrix.translation)
         for name in ("grip_l", "grip_r", "portrait")}
gun_sockets = {name: list(gun.matrix_world @ gun.pose.bones[name].matrix.translation)
               for name in ("operator_attach", "load", "eject", "muzzle")}
gun_mesh = bpy.data.objects["SKM_MaraGun"]
evaluated = gun_mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
contact = {}
def world_cm(point):
    return [-496 + point[0] * 100, 55 - point[1] * 100, .7 + point[2] * 100]
for name in ("grip_l", "grip_r"):
    point = Vector(grips[name])
    hit, near, normal, face = evaluated.closest_point_on_mesh(evaluated.matrix_world.inverted() @ point)
    if not hit:
        raise RuntimeError("Gun nearest-surface query failed")
    near = evaluated.matrix_world @ near
    contact[name] = {"grip_world_cm": world_cm(point), "nearest_gun_surface_world_cm": world_cm(near),
        "nearest_surface_distance_cm": (point - near).length * 100,
        "region": "near feed guide" if name == "grip_l" else "near copper return pipe",
        "limitation": "Bone point to visible surface only; no finger articulation, contact constraint or runtime collision test."}
(out / "stance-review.json").write_text(json.dumps({"source_only": True,
    "operator_origin_m": [0, 0, 0], "gun_origin_m": list(gun.location),
    "equivalent_unreal_operator_origin_cm": [-496, 55, .7], "equivalent_unreal_gun_origin_cm": [-350, 0, 1.5],
    "operator_grips_m": grips, "gun_sockets_m": gun_sockets,
    "resting_hand_contact": contact,
    "existing_machine_semantics": "Left at feed guide, right braces on existing copper service pipe. Neither is claimed as a newly authored control handle. The existing handwheel is too far forward for this planted stance.",
    "human_approval": "pending", "engine_camera_and_contact_check": "not performed"}, indent=2))

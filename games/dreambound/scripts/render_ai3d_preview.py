"""Render a GLB or raw PLY under neutral studio lighting without editing it."""
import argparse
import bpy
import hashlib
import json
import math
import sys
from pathlib import Path
from mathutils import Vector, Matrix

parser = argparse.ArgumentParser()
parser.add_argument('model', type=Path)
parser.add_argument('output', type=Path)
parser.add_argument('--resolution', type=int, default=1400)
parser.add_argument('--samples', type=int, default=48)
parser.add_argument('--rear-close', action='store_true')
args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:])
model, output = args.model, args.output
output.mkdir(parents=True, exist_ok=True)
if model.suffix.lower() == '.blend':
    bpy.ops.wm.open_mainfile(filepath=str(model))
    for obj in list(bpy.context.scene.objects):
        if obj.type != 'MESH' or obj.name.startswith('UCX_'):
            bpy.data.objects.remove(obj, do_unlink=True)
else:
    bpy.ops.wm.read_factory_settings(use_empty=True)
if model.suffix.lower() == '.ply':
    bpy.ops.wm.ply_import(filepath=str(model))
elif model.suffix.lower() != '.blend':
    bpy.ops.import_scene.gltf(filepath=str(model))
meshes = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH']
for obj in meshes:
    world = obj.matrix_world.copy()
    obj.parent = None
    obj.matrix_world = world
corners = [obj.matrix_world @ Vector(v) for obj in meshes for v in obj.bound_box]
lo = Vector(tuple(min(v[i] for v in corners) for i in range(3)))
hi = Vector(tuple(max(v[i] for v in corners) for i in range(3)))
scale = 3.2 / (hi.z - lo.z)
offset = Vector((-(lo.x + hi.x) / 2, -(lo.y + hi.y) / 2, -lo.z))
transform = Matrix.Scale(scale, 4) @ Matrix.Translation(offset)
for obj in meshes:
    obj.matrix_world = transform @ obj.matrix_world

scene = bpy.context.scene
scene.render.engine = 'CYCLES'
scene.cycles.samples = args.samples
scene.cycles.use_denoising = True
prefs = bpy.context.preferences.addons['cycles'].preferences
try:
    prefs.compute_device_type = 'OPTIX'
    prefs.get_devices()
    for device in prefs.devices:
        device.use = device.type == 'OPTIX'
    scene.cycles.device = 'GPU'
except Exception:
    scene.cycles.device = 'CPU'
scene.render.resolution_x = args.resolution
scene.render.resolution_y = args.resolution
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
scene.view_settings.view_transform = 'AgX'
world = bpy.data.worlds.new('Neutral studio')
world.use_nodes = True
world.node_tree.nodes['Background'].inputs['Color'].default_value = (0.17, 0.19, 0.22, 1)
world.node_tree.nodes['Background'].inputs['Strength'].default_value = 0.35
scene.world = world
bpy.ops.mesh.primitive_plane_add(size=200, location=(0, 0, -0.025))
ground = bpy.context.object
ground.name = 'Preview floor (not part of generated asset)'
mat = bpy.data.materials.new('Matte charcoal floor')
mat.diffuse_color = (0.115, 0.125, 0.145, 1)
mat.use_nodes = True
mat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (0.115, 0.125, 0.145, 1)
mat.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value = 0.85
ground.data.materials.append(mat)

def aim(obj, target):
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat('-Z', 'Y').to_euler()

def area(name, location, power, size):
    lamp = bpy.data.lights.new(name, 'AREA')
    lamp.energy = power
    lamp.shape = 'DISK'
    lamp.size = size
    obj = bpy.data.objects.new(name, lamp)
    scene.collection.objects.link(obj)
    obj.location = location
    aim(obj, (0, 0, 1.6))

area('Key softbox', (-3.5, -4.5, 6.2), 700, 4)
area('Fill softbox', (4.0, -2.0, 3.8), 450, 3.5)
area('Back softbox', (-1, 4.0, 5.5), 800, 3)
cam_data = bpy.data.cameras.new('Preview camera')
camera = bpy.data.objects.new('Preview camera', cam_data)
scene.collection.objects.link(camera)
scene.camera = camera
cam_data.type = 'ORTHO'
cam_data.ortho_scale = 4.1
for name, location in [('front-three-quarter', (2.3, -8.0, 3.4)), ('rear-three-quarter', (-3, 8, 3.4))]:
    camera.location = location
    aim(camera, (0, 0, 1.6))
    scene.render.filepath = str(output / (name + '.png'))
    bpy.ops.render.render(write_still=True)
    print('PREVIEW_SAVED ' + scene.render.filepath, flush=True)
if args.rear_close:
    camera.location = (-0.7, 4, 1.2)
    aim(camera, (-0.04, 0, .97))
    cam_data.ortho_scale = 1.2
    scene.render.filepath = str(output / 'rear-repair-close.png')
    bpy.ops.render.render(write_still=True)
    print('PREVIEW_SAVED ' + scene.render.filepath, flush=True)
    cam_data.ortho_scale = 4.1
camera.location = (2.3, -8.0, 3.4)
aim(camera, (0, 0, 1.6))
bpy.ops.wm.save_as_mainfile(filepath=str(output / 'preview-scene.blend'))
with model.open('rb') as source:
    source_sha256 = hashlib.file_digest(source, 'sha256').hexdigest()
(output / 'render-settings.json').write_text(json.dumps({
    'source_model': model.name, 'renderer': 'Blender Cycles', 'samples': args.samples,
    'source_sha256': source_sha256,
    'resolution': [args.resolution, args.resolution],
    'views': ['front-three-quarter', 'rear-three-quarter'] + (['rear-repair-close'] if args.rear_close else []),
    'generated_mesh_or_material_edits': False,
    'presentation_only': ['uniform scale and centering', 'studio lights', 'neutral floor', 'cameras'],
}, indent=2), encoding='utf-8')

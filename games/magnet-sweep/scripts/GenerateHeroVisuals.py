"""Finish an original TRELLIS hero in Blender; preserve source and texture UVs.

blender --background --threads 4 --python-exit-code 1 --python this.py --
  --source <textured.glb> --output <hero-dir> --rotation 0 0 0
Uses centimeters, X right / Y back / Z up; origin centered on the bottom plane.
The generated reference is never altered. Source geometry remains outside Git.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys

import bpy
from mathutils import Vector


def activate(obj):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def aim(obj, target):
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat('-Z', 'Y').to_euler()


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--rotation', type=float, nargs=3, default=[0, 0, 0])
    parser.add_argument('--target-triangles', type=int, default=160000)
    parser.add_argument('--inspect-only', action='store_true')
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:])
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(args.source.resolve()))
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH']
    for obj in meshes:
        transform = obj.matrix_world.copy()
        obj.parent = None
        obj.matrix_world = transform
    activate(meshes[0])
    for obj in meshes:
        obj.select_set(True)
    bpy.ops.object.join()
    high = bpy.context.object
    high.name = 'TRELLIS_source'
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    original_bounds = list(high.dimensions)
    high.rotation_euler = [math.radians(v) for v in args.rotation]
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    lower = Vector(tuple(min(v.co[i] for v in high.data.vertices) for i in range(3)))
    upper = Vector(tuple(max(v.co[i] for v in high.data.vertices) for i in range(3)))
    scale = 100.0 / max(upper.x - lower.x, upper.y - lower.y)
    offset = Vector((-(upper.x + lower.x) / 2, -(upper.y + lower.y) / 2, -lower.z))
    for vertex in high.data.vertices:
        vertex.co = (vertex.co + offset) * scale
    high.data.update()
    high.data.calc_loop_triangles()
    if len(high.data.materials) != 1:
        raise RuntimeError('This hero contract requires one source PBR material; inspect unexpected slots first.')
    report = {'source_sha256': digest(args.source), 'source_triangles': len(high.data.loop_triangles),
              'source_dimensions_blender': original_bounds, 'rotation_degrees': args.rotation,
              'unit': 'centimeters', 'pivot': 'XY center; bottom Z=0',
              'texture_channels': 'BaseColor sRGB; MR linear G=roughness B=metallic; Normal tangent OpenGL (+Y)',
              'blender_version': bpy.app.version_string, 'in_engine_verified': False}
    if not args.inspect_only:
        low = high.copy()
        low.data = high.data.copy()
        bpy.context.scene.collection.objects.link(low)
        low.name = 'SM_HeroMagnet'
        activate(low)
        # Retain the generated atlas and reduce conservatively. Blender validates
        # duplicate/degenerate faces before exchange to preserve round-trip counts.
        low.data.validate(verbose=False, clean_customdata=True)
        low.data.calc_loop_triangles()
        dec = low.modifiers.new('Conservative reduction with source UVs', 'DECIMATE')
        dec.ratio = min(1.0, args.target_triangles / len(low.data.loop_triangles))
        dec.use_collapse_triangulate = True
        bpy.ops.object.modifier_apply(modifier=dec.name)
        low.data.validate(verbose=False, clean_customdata=True)
        low.data.update()
        for polygon in low.data.polygons:
            polygon.use_smooth = True
        low.data.calc_loop_triangles()
        report['triangles'] = len(low.data.loop_triangles)
        report['vertices'] = len(low.data.vertices)
        report['uv_layers'] = [layer.name for layer in low.data.uv_layers]
        material = high.data.materials[0].copy()
        material.name = 'M_HeroMagnet'
        low.data.materials.clear()
        low.data.materials.append(material)
        nodes = material.node_tree.nodes
        principled = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
        color_node = principled.inputs['Base Color'].links[0].from_node
        mr_node = next(n for n in nodes if n.type == 'TEX_IMAGE' and n != color_node)
        for label, node, color_space in [('BaseColor',color_node,'sRGB'),('MR',mr_node,'Non-Color')]:
            im = node.image.copy()
            im.colorspace_settings.name = color_space
            im.scale(2048,2048)
            im.filepath_raw = str(out / f'T_HeroMagnet_{label}.png')
            im.file_format = 'PNG'
            im.save()
            node.image = bpy.data.images.load(im.filepath_raw, check_existing=False)
            node.image.name = f'T_HeroMagnet_{label}'
            node.image.colorspace_settings.name = color_space
        # Source surface relief remains in geometry; a neutral tangent map avoids
        # amplifying generated micro-islands into false cracks.
        normal_image = bpy.data.images.new('T_HeroMagnet_Normal',width=4,height=4,alpha=False)
        normal_image.colorspace_settings.name = 'Non-Color'
        normal_image.generated_color=(0.5,0.5,1,1)
        normal_image.filepath_raw=str(out/'T_HeroMagnet_Normal.png')
        normal_image.file_format='PNG'
        normal_image.save()
        tex = nodes.new('ShaderNodeTexImage')
        tex.image = normal_image
        normal = nodes.new('ShaderNodeNormalMap')
        material.node_tree.links.new(tex.outputs['Color'],normal.inputs['Color'])
        material.node_tree.links.new(normal.outputs['Normal'],principled.inputs['Normal'])
        materials=[{'slot':0,'name':material.name}]
        report['normal_map']='Neutral tangent map; generated surface relief is retained as actual geometry; rejected ray bake is not used.'
        high_mesh = high.data
        bpy.data.objects.remove(high, do_unlink=True)
        bpy.data.meshes.remove(high_mesh)
        bpy.data.orphans_purge(do_recursive=True)
        hero = low
        report['materials'] = materials
    else:
        hero = high
    scene = bpy.context.scene
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 0.01
    activate(hero)
    bpy.context.view_layer.update()
    report['dimensions_cm'] = list(hero.dimensions)
    if not args.inspect_only:
        bpy.ops.export_scene.fbx(filepath=str(out / 'SM_HeroMagnet.fbx'), use_selection=True,
            object_types={'MESH'}, apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
            bake_space_transform=False, mesh_smooth_type='FACE', use_mesh_modifiers=True,
            add_leaf_bones=False, axis_forward='-Y', axis_up='Z', path_mode='STRIP')
        bpy.ops.export_scene.gltf(filepath=str(out / 'SM_HeroMagnet.glb'), use_selection=True,
            export_format='GLB', export_apply=True, export_yup=True)
        bpy.ops.wm.save_as_mainfile(filepath=str(out / 'HeroMagnet.blend'))
        bpy.ops.file.make_paths_relative()
    # Reusable three-view authoring check. It is not a gameplay screenshot.
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 20
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 1280
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.render.film_transparent = False
    scene.view_settings.view_transform = 'AgX'
    world = bpy.data.worlds.new('Neutral authoring light')
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs['Color'].default_value = (0.13, .17, .2, 1)
    world.node_tree.nodes['Background'].inputs['Strength'].default_value = .5
    scene.world = world
    target_point = (0, 0, hero.dimensions.z * .4)
    for name, location, watts, size in [('Key', (-90, -110, 170), 1000000, 130),
                                         ('Fill', (110, -30, 80), 600000, 110),
                                         ('Rim', (0, 120, 120), 1100000, 100)]:
        lamp = bpy.data.lights.new(name, 'AREA')
        lamp.energy, lamp.size = watts, size
        obj = bpy.data.objects.new(name, lamp)
        scene.collection.objects.link(obj)
        obj.location = location
        aim(obj, target_point)
    camera_data = bpy.data.cameras.new('Asset inspection')
    camera = bpy.data.objects.new('Asset inspection', camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera_data.type = 'ORTHO'
    camera_data.ortho_scale = 145
    for name, location in [('three-quarter', (90, -140, 175)), ('top', (0, -0.01, 230)), ('rear', (-90, 140, 150))]:
        camera.location = location
        aim(camera, target_point)
        scene.render.filepath = str(out / f'hero-{name}.png')
        bpy.ops.render.render(write_still=True)
    if not args.inspect_only:
        bpy.ops.wm.save_as_mainfile(filepath=str(out / 'HeroMagnet.blend'))
    if not args.inspect_only:
        # Validate actual exchange files rather than treating exporter return as import evidence.
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=str(out / 'SM_HeroMagnet.fbx'))
        imported = [o for o in bpy.context.scene.objects if o.type == 'MESH']
        for obj in imported:
            obj.data.calc_loop_triangles()
        report['fbx_reimport'] = {'meshes': len(imported),
            'triangles': sum(len(o.data.loop_triangles) for o in imported),
            'material_slots': [[s.name for s in o.material_slots] for o in imported],
            'uv_layers': [[u.name for u in o.data.uv_layers] for o in imported]}
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.gltf(filepath=str(out / 'SM_HeroMagnet.glb'))
        imported = [o for o in bpy.context.scene.objects if o.type == 'MESH']
        for obj in imported:
            obj.data.calc_loop_triangles()
        report['glb_reimport'] = {'meshes': len(imported),
            'triangles': sum(len(o.data.loop_triangles) for o in imported),
            'material_slots': [[s.name for s in o.material_slots] for o in imported],
            'embedded_images': [{'name': im.name, 'size': list(im.size)} for im in bpy.data.images if im.size[0] > 1]}
        assert report['fbx_reimport']['triangles'] == report['triangles']
        assert report['glb_reimport']['triangles'] == report['triangles']
    report['files'] = {p.name: {'bytes': p.stat().st_size, 'sha256': digest(p)}
                       for p in out.iterdir() if p.is_file() and p.suffix in {'.fbx', '.glb', '.png', '.blend'}}
    (out / 'mesh-measurements.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, indent=2), flush=True)


if __name__ == '__main__':
    main()

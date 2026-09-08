"""Original Dreambound sculpted asset kit. Run with Blender --background --python.

No downloaded geometry, textures, brushes, fonts, or material libraries. Centimeter
geometry, +X forward, +Z up; FBX and preview images are reproducible from this file.
"""
import bpy
import csv
import json
import math
import random
import sys
import numpy as np
from pathlib import Path
from mathutils import Vector
from mathutils.noise import noise_vector

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / 'art'
OUT = ART / 'generated'
PREVIEW = ART / 'preview'
for directory in (ART, OUT, PREVIEW):
    directory.mkdir(parents=True, exist_ok=True)
random.seed(90318)
MATERIALS_ONLY = '--materials-only' in sys.argv
if MATERIALS_ONLY:
    # Preserve all actual geometry and its existing review composition.
    bpy.ops.wm.open_mainfile(filepath=str(ART / 'Dreambound_Kit.blend'))
else:
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = .01
bpy.context.preferences.filepaths.save_version = 0

PALETTE = {
    'M_Ceramic': {'color': [.70, .685, .59], 'roughness': .39, 'metallic': .02, 'normal_strength': .10, 'roughness_variation': .035, 'tone_low': [.95, .96, .95], 'tone_high': [1.03, 1.02, 1.0]},
    'M_Bronze': {'color': [.41, .176, .068], 'roughness': .43, 'metallic': .80, 'normal_strength': .20, 'patina_color': [.055, .215, .17], 'patina_amount': .22},
    'M_DarkMetal': {'color': [.036, .061, .072], 'roughness': .51, 'metallic': .68, 'normal_strength': .15},
    'M_Patina': {'color': [.055, .245, .20], 'roughness': .78, 'metallic': .30, 'normal_strength': .30},
    'M_Stone': {'color': [.155, .205, .225], 'roughness': .88, 'metallic': 0, 'normal_strength': .40, 'tone_low': [.80, .88, .97], 'tone_high': [1.06, 1.05, 1.0]},
    'M_StoneLight': {'color': [.29, .355, .365], 'roughness': .85, 'metallic': 0, 'normal_strength': .35, 'tone_low': [.87, .91, .98], 'tone_high': [1.04, 1.04, 1.0]},
    'M_Mortar': {'color': [.063, .086, .087], 'roughness': .97, 'metallic': 0, 'normal_strength': .25},
    'M_Root': {'color': [.12, .10, .068], 'roughness': .89, 'metallic': 0, 'normal_strength': .45},
    'M_RootDark': {'color': [.047, .040, .028], 'roughness': .95, 'metallic': 0, 'normal_strength': .35},
    'M_Moss': {'color': [.079, .18, .055], 'roughness': .95, 'metallic': 0, 'two_sided': True, 'normal_strength': .20},
    'M_Wood': {'color': [.23, .12, .061], 'roughness': .82, 'metallic': 0, 'normal_strength': .30},
    'M_Cloth': {'color': [.025, .038, .044], 'roughness': .96, 'metallic': 0, 'normal_strength': .20},
    'M_Core': {'color': [.035, .32, .65], 'roughness': .24, 'metallic': .30, 'emissive': .9},
    'M_Crystal': {'color': [.065, .255, .38], 'roughness': .20, 'metallic': .23, 'emissive': .32},
    'M_Frost': {'color': [.19, .62, .81], 'roughness': .27, 'metallic': .12, 'emissive': 1.5},
    'M_Storm': {'color': [.24, .075, .66], 'roughness': .29, 'metallic': .22, 'emissive': 2.2},
    'M_Ember': {'color': [.82, .15, .023], 'roughness': .36, 'metallic': .10, 'emissive': 2.2},
    'M_CombatGlow': {'color': [.05, .40, .85], 'roughness': .5, 'metallic': 0, 'emissive': 3, 'two_sided': True, 'parameter': 'Color'},
}
(OUT / 'materials.json').write_text(json.dumps(PALETTE, indent=2) + '\n', encoding='utf-8')


def make_surface_images():
    """Original isotropic pigment/pore fields, periodic without directional weave."""
    size = 512
    rng = np.random.default_rng(90319)
    frequency = np.fft.fftfreq(size)
    radius2 = frequency[:, None] ** 2 + frequency[None, :] ** 2
    def field(radius):
        sample = rng.normal(size=(size, size))
        blurred = np.fft.ifft2(np.fft.fft2(sample) * np.exp(-2 * math.pi**2 * radius**2 * radius2)).real
        return blurred / max(float(blurred.std()), 1e-8)
    broad, middle, pores = field(24), field(7), field(1.1)
    pixels = np.ones((size, size, 4), dtype=np.float32)
    pixels[:, :, 0] = np.clip(.5 + .095 * broad + .045 * middle + .009 * pores, .05, .95)
    pixels[:, :, 1] = np.clip(.5 + .04 * broad + .08 * middle + .018 * pores, .05, .95)
    pixels[:, :, 2] = np.clip(.22 + .31 * broad + .15 * middle, 0, 1)
    height = .006 * broad + .012 * middle + .008 * pores
    dx = (np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)) * 2.2
    dy = (np.roll(height, -1, axis=0) - np.roll(height, 1, axis=0)) * 2.2
    tangent = np.stack((-dx, -dy, np.ones_like(dx)), axis=-1)
    tangent /= np.linalg.norm(tangent, axis=-1, keepdims=True)
    normals = np.ones_like(pixels)
    normals[:, :, :3] = tangent * .5 + .5
    images = []
    for name, data in [('T_PaintedSurface', pixels), ('T_SculptedNormal', normals)]:
        image = bpy.data.images.get(name) or bpy.data.images.new(name, width=size, height=size, alpha=True)
        image.colorspace_settings.name = 'Non-Color'
        image.pixels.foreach_set(data.ravel())
        image.filepath_raw = str(OUT / (name + '.png'))
        image.file_format = 'PNG'
        image.save()
        images.append(image)
    return images


surface_image, normal_image = make_surface_images()
MATS = {}
for name, spec in PALETTE.items():
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.diffuse_color = (*spec['color'], 1)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    shader = nodes.new('ShaderNodeBsdfPrincipled')
    output = nodes.new('ShaderNodeOutputMaterial')
    links.new(shader.outputs['BSDF'], output.inputs['Surface'])
    shader.inputs['Metallic'].default_value = spec['metallic']
    shader.inputs['Roughness'].default_value = spec['roughness']
    shader.inputs['Coat Weight'].default_value = .16 if name == 'M_Ceramic' else 0
    tex = nodes.new('ShaderNodeTexImage'); tex.image = surface_image
    ramp = nodes.new('ShaderNodeValToRGB')
    ramp.color_ramp.elements[0].position = 0
    ramp.color_ramp.elements[0].color = (*(c * t for c, t in zip(spec['color'], spec.get('tone_low', [.88, .89, .90]))), 1)
    ramp.color_ramp.elements[1].position = 1
    ramp.color_ramp.elements[1].color = (*(min(1, c * t) for c, t in zip(spec['color'], spec.get('tone_high', [1.07, 1.04, 1.0]))), 1)
    separate = nodes.new('ShaderNodeSeparateColor')
    links.new(tex.outputs['Color'], separate.inputs['Color'])
    links.new(separate.outputs['Red'], ramp.inputs['Fac'])
    base = ramp.outputs['Color']
    if spec.get('patina_amount'):
        amount = nodes.new('ShaderNodeMath'); amount.operation = 'MULTIPLY'; amount.inputs[1].default_value = spec['patina_amount']
        links.new(separate.outputs['Blue'], amount.inputs[0])
        coat = nodes.new('ShaderNodeMixRGB'); coat.inputs[2].default_value = (*spec['patina_color'], 1)
        links.new(amount.outputs[0], coat.inputs[0]); links.new(base, coat.inputs[1])
        base = coat.outputs[0]
    vc = nodes.new('ShaderNodeVertexColor'); vc.layer_name = 'Color'
    mult = nodes.new('ShaderNodeMixRGB'); mult.blend_type = 'MULTIPLY'; mult.inputs[0].default_value = 1
    links.new(base, mult.inputs[1]); links.new(vc.outputs['Color'], mult.inputs[2])
    links.new(mult.outputs[0], shader.inputs['Base Color'])
    rough = nodes.new('ShaderNodeMapRange')
    variation = spec.get('roughness_variation', .08)
    rough.inputs['From Min'].default_value=0; rough.inputs['From Max'].default_value=1
    rough.inputs['To Min'].default_value=max(.12,spec['roughness']-variation); rough.inputs['To Max'].default_value=min(1,spec['roughness']+variation)
    links.new(separate.outputs['Green'],rough.inputs['Value']); links.new(rough.outputs['Result'],shader.inputs['Roughness'])
    normal_tex = nodes.new('ShaderNodeTexImage'); normal_tex.image = normal_image
    normal = nodes.new('ShaderNodeNormalMap'); normal.inputs['Strength'].default_value = spec.get('normal_strength', .15)
    links.new(normal_tex.outputs['Color'], normal.inputs['Color']); links.new(normal.outputs['Normal'], shader.inputs['Normal'])
    if spec.get('emissive'):
        shader.inputs['Emission Color'].default_value = (*spec['color'], 1)
        shader.inputs['Emission Strength'].default_value = spec['emissive']
    MATS[name] = mat

if MATERIALS_ONLY:
    # The saved kit deliberately omits disposable review instances. Recreate
    # only a small courtyard composition using copies of the existing meshes.
    review_objects = []
    def review_instance(name, location, scale=1, yaw=0):
        original = bpy.data.objects.get(name)
        if original is None: raise RuntimeError('Missing existing mesh: ' + name)
        obj = original.copy(); scene.collection.objects.link(obj)
        obj.name = 'Material preview ' + name
        obj.location = location; obj.scale = (scale, scale, scale)
        obj.rotation_euler = (0, 0, math.radians(yaw))
        obj.hide_render = False; obj.hide_viewport = False; obj.hide_set(False)
        review_objects.append(obj)
    for x in [-400, -200, 0, 200, 400]:
        for y in [-200, 0, 200]: review_instance('SM_StoneTile', (x, y, 0))
    for name, location, scale, yaw in [
        ('SM_Wall', (-405, 350, 24), 1, 0), ('SM_Arch', (70, 350, 24), 1, 0),
        ('SM_Pillar', (-175, 295, 24), 1, 0), ('SM_Pillar', (320, 330, 24), 1, 0),
        ('SM_Bell', (70, 345, 245), .85, 0), ('SM_TechPanel', (-590, 200, 24), 1, 15),
        ('SM_Root', (-175, 285, 24), 1.3, 190), ('SM_Crate', (-340, -75, 24), 1, 0),
        ('SM_Crystal', (-255, -112, 24), 1.45, 0),
    ]: review_instance(name, location, scale, yaw)
    for x, y in [(-375, -185), (-170, 130), (335, 260), (300, -170), (-490, 190)]:
        review_instance('SM_Grass', (x, y, 24), 1.3, 25)
    for name, position in [('SM_GuardianBody', (0, 0, 94)), ('SM_GuardianHead', (0, 0, 156)),
                           ('SM_GuardianArm', (0, 36, 146)), ('SM_GuardianArm', (0, -36, 146)),
                           ('SM_GuardianLeg', (0, 18, 94)), ('SM_GuardianLeg', (0, -18, 94))]:
        review_instance(name, (position[0] + 185, position[1] - 25, position[2] + 24))
    camera = scene.camera
    camera.location = (1020, -1400, 960)
    camera.rotation_euler = (Vector((-40, 145, 174)) - camera.location).to_track_quat('-Z', 'Y').to_euler()
    camera.data.type = 'ORTHO'; camera.data.ortho_scale = 1330; camera.data.clip_end = 30000
    scene.render.resolution_x = 1600; scene.render.resolution_y = 1000
    scene.render.resolution_percentage = 100
    scene.render.filepath = str(PREVIEW / 'material-pass-courtyard.png')
    scene.cycles.samples = 40
    scene.render.threads_mode = 'FIXED'; scene.render.threads = 6
    bpy.ops.render.render(write_still=True)
    for obj in review_objects: bpy.data.objects.remove(obj, do_unlink=True)
    for image in (surface_image, normal_image): image.filepath = '//generated/' + image.name + '.png'
    bpy.ops.wm.save_as_mainfile(filepath=str(ART / 'Dreambound_Kit.blend'), compress=True)
    print('DREAMBOUND_MATERIALS_COMPLETE ' + json.dumps({'materials': len(PALETTE), 'textures': 2, 'geometry_changed': False}))
    raise SystemExit(0)

parts = []
collisions = []
ASSETS = {}


def active(obj):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True); bpy.context.view_layer.objects.active = obj


def decorate(obj, mat, bevel=0, smooth=False):
    obj.data.materials.append(MATS[mat])
    active(obj)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        mod = obj.modifiers.new('Sculpted bevel', 'BEVEL'); mod.width = bevel; mod.segments = 2
        bpy.ops.object.modifier_apply(modifier=mod.name)
    if mat.startswith('M_Stone'):
        for vertex in obj.data.vertices:
            vertex.co += noise_vector((obj.matrix_world @ vertex.co) * .12) * .22
        obj.data.update()
    for face in obj.data.polygons:
        face.use_smooth = smooth
    if bevel and not smooth:
        mod = obj.modifiers.new('Weighted broad planes', 'WEIGHTED_NORMAL'); mod.keep_sharp = True; mod.weight = 45
        bpy.ops.object.modifier_apply(modifier=mod.name)
    # Coherent, broad pigment differences survive FBX as neutral vertex tints.
    col = obj.data.color_attributes.new(name='Color', type='FLOAT_COLOR', domain='CORNER')
    shade = random.uniform(.74 if mat.startswith('M_Stone') else .83, .99)
    for loop in obj.data.loops:
        p = obj.matrix_world @ obj.data.vertices[loop.vertex_index].co
        n = noise_vector(p * .028 + Vector((3.1, 8.4, 1.7))).x
        value = max(.62, min(1, shade + n * .15 + max(0, loop.normal.z) * .028))
        col.data[loop.index].color = (value, value, value, 1)
    # UVs are part of every exported mesh; both preview and Unreal use the maps.
    active(obj)
    bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=.018)
    bpy.ops.object.mode_set(mode='OBJECT')
    parts.append(obj)
    return obj


def box(loc, dims, mat, bevel=1, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc, rotation=rot)
    obj = bpy.context.object; obj.dimensions = dims
    return decorate(obj, mat, bevel)


def mesh(name, verts, faces, mat, bevel=0, smooth=False):
    data = bpy.data.meshes.new(name); data.from_pydata(verts, [], faces); data.update()
    obj = bpy.data.objects.new(name, data); scene.collection.objects.link(obj)
    return decorate(obj, mat, bevel, smooth)


def cylinder(loc, radius, depth, mat, axis='Z', vertices=24, bevel=.35, radius2=None):
    rotation = {'X': (0, math.pi / 2, 0), 'Y': (math.pi / 2, 0, 0), 'Z': (0, 0, 0)}[axis]
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=radius, radius2=radius if radius2 is None else radius2, depth=depth, location=loc, rotation=rotation)
    return decorate(bpy.context.object, mat, bevel, True)


def orb(loc, scale, mat, subdivisions=2):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=subdivisions, radius=1, location=loc)
    obj = bpy.context.object; obj.scale = scale
    return decorate(obj, mat, 0, True)


def axis_point(axis, axial, a, b):
    return {'X': (axial, a, b), 'Y': (a, axial, b), 'Z': (a, b, axial)}[axis]


def lathe(profile, loc, mat, axis='Z', segments=40, bevel=.25):
    verts = []
    for axial, radius in profile:
        for i in range(segments):
            angle = i * math.tau / segments
            p = axis_point(axis, axial, math.cos(angle) * radius, math.sin(angle) * radius)
            verts.append(tuple(p[j] + loc[j] for j in range(3)))
    faces = []
    for row in range(len(profile)):
        following = (row + 1) % len(profile)
        for i in range(segments):
            faces.append((row * segments + i, row * segments + (i + 1) % segments, following * segments + (i + 1) % segments, following * segments + i))
    return mesh('Turned sculpted profile', verts, faces, mat, bevel, True)


def ring(loc, outer, inner, depth, mat, axis='X', bevel=.25, segments=40):
    return lathe([(-depth / 2, inner), (-depth / 2, outer), (depth / 2, outer), (depth / 2, inner)], loc, mat, axis, segments, bevel)


def beam(a, b, radius, mat, vertices=12):
    mid = (Vector(a) + Vector(b)) * .5
    obj = cylinder(mid, radius, (Vector(b) - Vector(a)).length, mat, vertices=vertices, bevel=.25)
    obj.rotation_euler = (Vector(b) - Vector(a)).to_track_quat('Z', 'Y').to_euler()
    return obj


def loft(sections, mat, axis='X', bevel=.8):
    # Deliberately tapered/chamfered cross-sections, rather than scaled cubes.
    ring_points = [(-.6, -1), (.6, -1), (1, -.6), (1, .6), (.6, 1), (-.6, 1), (-1, .6), (-1, -.6)]
    verts = []
    for axial, a, b, shift in sections:
        for u, v in ring_points:
            verts.append(axis_point(axis, axial, u * a, v * b + shift))
    faces = [tuple(reversed(range(8))), tuple((len(sections) - 1) * 8 + i for i in range(8))]
    for row in range(len(sections) - 1):
        for i in range(8):
            faces.append((row * 8 + i, row * 8 + (i + 1) % 8, (row + 1) * 8 + (i + 1) % 8, (row + 1) * 8 + i))
    return mesh('Chamfered sculpt', verts, faces, mat, bevel)


def arc(inner, outer, start, end, depth, mat, axis='X', loc=(0, 0, 0), bevel=.6, steps=12):
    verts = []
    for axial in [-depth / 2, depth / 2]:
        for radius in [inner, outer]:
            for i in range(steps + 1):
                angle = math.radians(start + (end - start) * i / steps)
                p = axis_point(axis, axial, radius * math.cos(angle), radius * math.sin(angle))
                verts.append(tuple(p[j] + loc[j] for j in range(3)))
    n = steps + 1; faces = []
    for i in range(steps):
        faces += [(i, i + 1, n + i + 1, n + i), (2 * n + i, 3 * n + i, 3 * n + i + 1, 2 * n + i + 1),
                  (i, 2 * n + i, 2 * n + i + 1, i + 1), (n + i, n + i + 1, 3 * n + i + 1, 3 * n + i)]
    faces += [(0, n, 3 * n, 2 * n), (n - 1, 3 * n - 1, 4 * n - 1, 2 * n - 1)]
    return mesh('Carved arc', verts, faces, mat, bevel)


def collision_box(loc, dims):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    obj = bpy.context.object; obj.dimensions = dims
    active(obj); bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    collisions.append(obj)


def finish(name, pivot='Origin', placement=None):
    global parts, collisions
    bpy.ops.object.select_all(action='DESELECT')
    for obj in parts: obj.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.object; obj.name = name; obj.data.name = name
    scene.cursor.location = (0, 0, 0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    obj.data.calc_loop_triangles()
    bounds = [obj.matrix_world @ Vector(p) for p in obj.bound_box]
    lo = [min(p[i] for p in bounds) for i in range(3)]
    hi = [max(p[i] for p in bounds) for i in range(3)]
    meta = {'name': name, 'dimensions_cm': [round(hi[i] - lo[i], 3) for i in range(3)], 'bounds_min_cm': lo, 'bounds_max_cm': hi,
            'vertices': len(obj.data.vertices), 'triangles': len(obj.data.loop_triangles), 'pivot': pivot,
            'materials': [mat.name for mat in obj.data.materials], 'collision_hulls': len(collisions)}
    if placement: meta['recommended_attachment_cm'] = placement
    for i, collider in enumerate(collisions):
        collider.name = f'UCX_{name}_{i:02d}'
        collider.select_set(True)
    # Unreal receives these graphs from import_art.py. Do not let FBX serialize
    # absolute preview texture filenames, which its STRIP option still retains.
    export_materials=list(obj.data.materials)
    preview_images=[(node,node.image) for material in export_materials for node in material.node_tree.nodes if node.type=='TEX_IMAGE']
    for node,_ in preview_images: node.image=None
    try:
        export_path=OUT / (name + '.export.fbx')
        bpy.ops.export_scene.fbx(filepath=str(export_path), use_selection=True, object_types={'MESH'},
            apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS', axis_forward='X', axis_up='Z',
            use_space_transform=True, bake_space_transform=True, use_mesh_modifiers=True, mesh_smooth_type='FACE',
            add_leaf_bones=False, bake_anim=False, use_custom_props=False, path_mode='STRIP')
        export_path.replace(OUT / (name + '.fbx'))
    finally:
        for node,image in preview_images: node.image=image
    for collider in collisions: bpy.data.objects.remove(collider, do_unlink=True)
    obj.hide_render = True; obj.hide_set(True)
    ASSETS[name] = {'object': obj, 'meta': meta}
    parts, collisions = [], []


# Combined mechanical receiver: open coaxial rings, substantial ceramic shoulder,
# copper chassis, recessed radiator, real rivets and thick protective rails.
loft([(-8, 6, 6, -4), (1, 10, 9, -2), (17, 10.8, 9, 0), (29, 8, 7, 3), (42, 5.5, 5.5, 7)], 'M_DarkMetal')
loft([(-6, 7, 3.5, 2), (3, 10, 4.5, 6), (17, 10, 4.5, 7), (24, 8, 3.5, 8)], 'M_Ceramic', bevel=1.1)
for side in [-1, 1]:
    beam((-4, side * 9, -1), (34, side * 7, 8), 1.7, 'M_Bronze')
    box((10, side * 10.2, -.2), (15, 1.7, 5), 'M_Bronze', .55)
    for x in [-2, 7, 17, 25]: cylinder((x, side * 10.8, 3), 1.05, 1, 'M_DarkMetal', 'Y', 12, .14)
    for x in [3, 7, 11, 15]: box((x, side * 9.9, -.2), (1.6, 1.0, 3.1), 'M_Patina', .15)
ring((33, 0, 10), 11.3, 6.1, 5.8, 'M_Bronze')
ring((35.7, 0, 10), 10.4, 6.3, 1.1, 'M_DarkMetal')
ring((42, 0, 10), 8.5, 6.1, 8, 'M_DarkMetal')
ring((46.6, 0, 10), 8.8, 6.0, 1.9, 'M_Bronze')
ring((50.2, 0, 10), 7.3, 5.1, 4.5, 'M_Ceramic')
for i in range(8):
    a = math.tau * i / 8
    y, z = math.cos(a) * 9, math.sin(a) * 9 + 10
    cylinder((29.8, y, z), .75, 1, 'M_DarkMetal', 'X', 10, .12)
    beam((36, math.cos(a) * 8.2, math.sin(a) * 8.2 + 10), (45, math.cos(a) * 7.2, math.sin(a) * 7.2 + 10), .8, 'M_Bronze', 8)
loft([(-5, 3.8, 4, -9), (1, 4.2, 5, -10), (5, 3.4, 3.6, -6)], 'M_Cloth', bevel=.6)
finish('SM_WeaponBody', 'Grip; +X forward', [0, 0, 0])

# Articulated crescent guard: ceramic petals layered over bronze, open core aperture.
arc(13.4, 23.6, -155, 155, 2.4, 'M_Bronze', bevel=.65, steps=48)
for lo, hi in [(-152, -94), (-90, -33), (-29, 28), (32, 89), (93, 152)]:
    arc(14.6, 24.2, lo, hi, 3.8, 'M_Ceramic', loc=(-.7, 0, 0), bevel=.65)
    arc(22.3, 23.2, lo + 4, hi - 4, .55, 'M_Bronze', loc=(-2.8, 0, 0), bevel=.14)
    angle = math.radians((lo + hi) / 2)
    for radius in [16.5, 21.5]:
        cylinder((-2.9, math.cos(angle) * radius, math.sin(angle) * radius), .76, .7, 'M_DarkMetal', 'X', 12, .13)
ring((0, -20, 0), 4.7, 2, 6, 'M_Bronze', 'Y', .3, 24)
arc(13.1, 14.2, -150, 150, 1.4, 'M_Patina', loc=(-.5, 0, 0), bevel=.2, steps=44)
finish('SM_ShieldPlate', 'Central deployment pivot; plate lies in YZ', [31, 0, 10])

ring((0, 0, 0), 6.3, 4.7, 3.8, 'M_Bronze', 'X', .24, 32)
cylinder((0, 0, 0), 4.65, 3.0, 'M_Core', 'X', 16, .45)
for side in [-1, 1]:
    ring((side * 1.65, 0, 0), 4.15, 3.8, .32, 'M_DarkMetal', 'X', .1, 24)
    for i in range(6):
        a = i * math.tau / 6
        beam((side * 1.9, math.cos(a) * 2.1, math.sin(a) * 2.1), (side * 1.9, math.cos(a + .30) * 4.2, math.sin(a + .30) * 4.2), .36, 'M_Bronze', 8)
    orb((side * 2.05, 0, 0), (.8, 1.7, 1.7), 'M_Core')
finish('SM_Core', 'Core center; disc normal +X', [33, 0, 10])

loft([(-36, 9.8, 8.5, -3), (-28, 9.4, 7.8, -3), (-12, 7.2, 6.5, -3), (-1, 5.6, 5, -3)], 'M_Cloth')
for start, end, wide in [(-35, -25, 10.5), (-23.5, -13.5, 9.1), (-12, -4, 7.5)]:
    loft([(start, wide, 6.7, .2), ((start + end) / 2, wide, 7.5, .6), (end, wide - 1.3, 6.1, -.1)], 'M_Ceramic', bevel=.9)
    ring((start + 1, 0, -3), wide + .3, wide - .8, 1.5, 'M_Bronze', 'X', .23, 24)
for side in [-1, 1]:
    beam((-32, side * 8, -5), (-4, side * 5.4, -4), 1.05, 'M_Bronze')
    for x in [-31, -20, -10]: cylinder((x, side * 8, 1), .8, 1, 'M_DarkMetal', 'Y', 12, .15)
finish('SM_Forearm', 'Wrist/grip origin, cuff extends -X', [0, 0, -1])

# Guardian: animate the separate rigid pieces; front faces +X.
loft([(-4, 14, 19, 0), (8, 15, 23, 0), (36, 19, 31, 0), (57, 16, 29, 0), (64, 13, 24, 0)], 'M_DarkMetal', 'Z', 2)
for side in [-1, 1]:
    box((10, side * 17, 39), (23, 23, 41), 'M_Bronze', 2.3, (math.radians(side * 8), 0, 0))
    beam((23, side * 25, 56), (25, side * 8, 23), 3.2, 'M_Bronze')
    beam((24.5, side * 22, 56), (25.5, side * 10, 31), 1.25, 'M_Ceramic')
    for z in [31,39,47]: box((22,side*18,z),(2,10,2.5),'M_Patina',.3)
    box((8, side * 20, 1), (27, 19, 18), 'M_Bronze', 2.2)
ring((21, 0, 38), 13.2, 8.5, 6, 'M_Bronze', 'X', .75, 32)
cylinder((23, 0, 38), 8.1, 3, 'M_Core', 'X', 12, .5)
for z in [9, 15, 21]: box((18, 0, z), (7, 28, 3), 'M_Bronze', .6)
cylinder((0, 0, 64), 13, 9, 'M_Bronze', 'Z', 24, .6)
finish('SM_GuardianBody', 'Hip center', [0, 0, 94])

loft([(0, 7.5, 10, 0), (8, 11, 13, 0), (25, 11.5, 12, 0), (34, 6.5, 8, 0)], 'M_DarkMetal', 'Z', 1.0)
box((11, 0, 18), (6, 25, 6), 'M_DarkMetal', .8)
box((14.2,0,18),(1.1,21,1.8),'M_Core',.2)
for side in [-1, 1]:
    beam((13, side * 12, 23), (16, side * 1.5, 21), 1.2, 'M_Ceramic')
    box((9, side * 11, 7), (10, 5, 15), 'M_Bronze', 1, (0, math.radians(-22), 0))
loft([(3,2.5,4,10),(11,3,7,11),(15,2,9,11)],'M_Bronze','Z',.6)
loft([(26, 2, 11, -1), (37, 1.6, 8, -5), (43, .9, 2.5, -10)], 'M_Bronze', 'Z', .45)
finish('SM_GuardianHead', 'Neck center; front +X', [0, 0, 156])

orb((0, 0, 0), (12, 13, 12), 'M_DarkMetal')
loft([(-14, 12, 13, 0), (1, 18, 21, 0), (13, 16, 18, 0), (20, 10, 11, 0)], 'M_Bronze', 'Z', 1.7)
arc(12,17,-72,72,5,'M_Ceramic','X',(-1,0,2),.65,16)
for y in [-13,0,13]: cylinder((15,y,4),1.5,2,'M_DarkMetal','X',12,.3)
ring((0, 0, 1), 14, 10, 4, 'M_Bronze', 'Y', .7, 24)
loft([(-17, 8.2, 9, 0), (-34, 7.5, 8, 0)], 'M_Bronze', 'Z', 1.1)
orb((0, 0, -36), (9, 10, 9), 'M_DarkMetal')
loft([(-40, 10, 10, 0), (-52, 11, 11, 0), (-64, 7.5, 8, 0)], 'M_Bronze', 'Z', 1.3)
box((10,0,-51),(4,12,23),'M_Ceramic',1)
ring((0, 0, -61), 9.1, 7, 3, 'M_Bronze', 'Z', .55, 20)
box((1, 0, -70), (18, 20, 17), 'M_DarkMetal', 3)
for y in [-6.2, 0, 6.2]: box((8, y, -68), (8, 5, 12), 'M_Bronze', 1.2)
finish('SM_GuardianArm', 'Shoulder center; hangs toward -Z; symmetric left/right', [0, 36, 146])

orb((0, 0, -3), (10, 11, 11), 'M_DarkMetal')
loft([(-6, 9.5, 11, 0), (-19, 11, 12, 0), (-39, 7.6, 9, 0)], 'M_Bronze', 'Z', 1.4)
box((10,0,-22),(3,14,24),'M_DarkMetal',1)
orb((0, 0, -43), (10.5, 11, 10), 'M_DarkMetal')
cylinder((10, 0, -42), 8, 5, 'M_Bronze', 'X', 12, .8)
loft([(-49, 10, 11, 0), (-61, 9.4, 10, 0), (-79, 7.6, 8, 0)], 'M_Bronze', 'Z', 1.2)
box((8,0,-63),(4,13,27),'M_Ceramic',1)
beam((10, 0, -52), (8, 0, -78), 2.5, 'M_Bronze')
box((8, 0, -85), (38, 27, 18), 'M_DarkMetal', 3)
box((15, 0, -81), (24, 27.5, 11), 'M_Bronze', 2.2)
box((23, 0, -85), (8, 28, 10), 'M_Bronze', 1.4)
finish('SM_GuardianLeg', 'Hip joint; lowest sole -94cm', [0, 18, 94])

# Reusable, dimensioned courtyard pieces. Mortar is recessed physical geometry.
box((0, 0, 5), (200, 200, 10), 'M_Mortar', .6)
for y in range(4):
    for x in range(4):
        cuts = [-100,-61,-7,48,100] if y%2 else [-100,-45,12,61,100]
        a,b=cuts[x]+1.2,cuts[x+1]-1.2; low,high=-100+y*50+1.2,-50+y*50-1.2
        corners=[random.uniform(2,6) for _ in range(4)]
        poly=[(a+corners[0],low),(b-corners[1],low),(b,low+corners[1]),(b,high-corners[2]),(b-corners[2],high),(a+corners[3],high),(a,high-corners[3]),(a,low+corners[0])]
        top=random.uniform(23.2,24.2)
        verts=[(px,py,z) for z in [9,top] for px,py in poly]
        faces=[tuple(reversed(range(8))),tuple(range(8,16))]+[(i,(i+1)%8,(i+1)%8+8,i+8) for i in range(8)]
        mesh('Broken flagstone',verts,faces,'M_StoneLight' if random.random()<.28 else 'M_Stone',1.2)
collision_box((0, 0, 12), (200, 200, 24))
finish('SM_StoneTile', 'Base center; top approximately +24cm')

box((0, 0, 151), (400, 50, 302), 'M_Mortar', .5)
for row in range(5):
    cuts = [-200, -150, -50, 50, 150, 200] if row % 2 else [-200, -100, 0, 100, 200]
    for a, b in zip(cuts, cuts[1:]):
        box(((a + b) / 2, random.uniform(-1, 1), 41 + row * 56), (b - a - 2.1, 58, 53.5),
            'M_StoneLight' if random.random() < .15 else 'M_Stone', 1.9)
for x in [-100, 100]:
    box((x, 0, 9), (199, 65, 18), 'M_StoneLight', 1.6)
    box((x, 0, 311), (199, 72, 18), 'M_StoneLight', 1.8)
collision_box((0, 0, 160), (400, 60, 320))
finish('SM_Wall', 'Ground center; X width, Y thickness')

box((0, 0, 12), (112, 112, 24), 'M_Stone', 3)
box((0, 0, 32), (94, 94, 18), 'M_StoneLight', 2)
cylinder((0, 0, 190), 36, 300, 'M_Stone', 'Z', 12, 1.4)
for i in range(4):
    a = math.pi / 4 + i * math.pi / 2
    cylinder((math.cos(a) * 31, math.sin(a) * 31, 186), 10, 278, 'M_StoneLight', 'Z', 10, .9)
for z in [52, 80, 298, 330]: cylinder((0, 0, z), 44, 9, 'M_StoneLight', 'Z', 12, 1.4)
cylinder((0, 0, 345), 49, 16, 'M_Stone', 'Z', 8, 1.5, 56)
box((0, 0, 367), (115, 115, 26), 'M_StoneLight', 2.4)
collision_box((0, 0, 190), (84, 84, 380))
finish('SM_Pillar', 'Ground center')

for side in [-1, 1]:
    for row in range(5):
        box((side * 172.5, 0, 23 + row * 45), (74, 74, 43.5), 'M_Stone', 2)
    box((side * 172.5, 0, 14), (110, 94, 28), 'M_StoneLight', 2.4)
    box((side * 172.5, 0, 220), (89, 87, 18), 'M_StoneLight', 1.7)
    collision_box((side * 172.5, 0, 112.5), (75, 75, 225))
for i in range(13):
    a, b = i * 180 / 13 + .42, (i + 1) * 180 / 13 - .42
    stone = arc(140, 205, a, b, 76, 'M_StoneLight' if i == 6 else 'M_Stone', 'Y', (0, 0, 225), 1.7, 2)
    # Each narrow voussoir gets its own convex hull; the opening remains open.
    collider = stone.copy(); collider.data = stone.data.copy(); scene.collection.objects.link(collider)
    collisions.append(collider)
arc(201, 212, 2, 178, 83, 'M_StoneLight', 'Y', (0, 0, 225), 1.2, 36)
finish('SM_Arch', 'Ground center; walk through along Y; X spans opening')

# Hollow bell with substantial rolled lip, internal clapper and raised ornament.
lathe([(0, 61), (3, 68), (10, 68), (16, 58), (45, 47), (82, 30), (110, 23), (121, 17),
       (121, 12), (109, 17), (80, 24), (44, 40), (17, 51), (7, 60)], (0, 0, 0), 'M_Bronze', 'Z', 56, .65)
for z, r in [(14, 59), (20, 56), (91, 28), (104, 25)]: ring((0, 0, z), r + 1.7, r - 1.2, 3, 'M_Bronze', 'Z', .3, 48)
ring((0, 0, 135), 16, 10.5, 7, 'M_Bronze', 'Y', .8, 32)
beam((0, 0, 109), (0, 0, 17), 3.5, 'M_DarkMetal')
orb((0, 0, 19), (12, 12, 13), 'M_Bronze')
for side in [0, 1, 2, 3]:
    a = side * math.pi / 2
    for delta in [-.15, .15]:
        beam((math.cos(a + delta) * 47, math.sin(a + delta) * 47, 44), (math.cos(a) * 33, math.sin(a) * 33, 78), 1.9, 'M_Patina')
    orb((math.cos(a) * 40, math.sin(a) * 40, 57), (3, 3, 6), 'M_Bronze')
finish('SM_Bell', 'Lower lip center; hang by eye at Z135')


def root_tube(points, start_radius, end_radius, mat='M_Root'):
    centers = []
    controls = [Vector(points[0])] + [Vector(p) for p in points] + [Vector(points[-1])]
    for j in range(1, len(controls) - 2):
        p0, p1, p2, p3 = controls[j - 1:j + 3]
        for k in range(5):
            t = k / 5
            centers.append(.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t*t + (-p0 + 3*p1 - 3*p2 + p3) * t*t*t))
    centers.append(Vector(points[-1]))
    verts, faces = [], []
    sides = 11
    for i, center in enumerate(centers):
        tangent = (centers[min(i + 1, len(centers) - 1)] - centers[max(0, i - 1)]).normalized()
        side = tangent.cross(Vector((0, 0, 1))).normalized()
        up = side.cross(tangent).normalized()
        radius = start_radius * (1 - i / (len(centers) - 1)) ** .7 + end_radius * i / (len(centers) - 1)
        for j in range(sides):
            a = j * math.tau / sides
            r = radius * (1 + .17 * math.sin(a * 5 + i * .31) + .065 * math.sin(i * 1.4 + a))
            verts.append(tuple(center + (side * math.cos(a) + up * math.sin(a)) * r))
    for i in range(len(centers) - 1):
        for j in range(sides): faces.append((i*sides+j, i*sides+(j+1)%sides, (i+1)*sides+(j+1)%sides, (i+1)*sides+j))
    faces += [tuple(reversed(range(sides))), tuple((len(centers)-1)*sides+j for j in range(sides))]
    mesh('Sculpted root', verts, faces, mat, 0, True)


root_tube([(0, 0, 44), (60, 12, 35), (125, -8, 21), (200, 20, 17), (300, -14, 9), (410, 12, 3)], 30, 1.3)
root_tube([(12, 3, 38), (67, 35, 26), (130, 65, 13), (190, 94, 3)], 19, 1.2)
root_tube([(95, -2, 24), (130, -34, 19), (182, -51, 10), (235, -38, 2)], 14, .9)
root_tube([(205, 16, 16), (250, 57, 14), (315, 64, 2)], 8, .8)
orb((2,0,43),(29,30,35),'M_Root',3)
for off in [-12,-5,4,11]:
    root_tube([(12,off,70),(60,off+12,58),(125,off-8,42),(200,off+20,33),(300,off-14,19),(387,off*.2+8,5)],1.25,.25,'M_RootDark')
for p in [(24, 0, 65), (58, 10, 57), (115, -3, 40), (173, 16, 31)]: orb(p, (19, 11, 3.7), 'M_Moss')
finish('SM_Root', 'Root crown at origin; extends +X; decoration, no blocking collision')

box((0, 0, 40), (84, 69, 78), 'M_DarkMetal', 2)
for i in range(5):
    for y in [-36, 36]: box(((i - 2) * 16.5, y, 41), (15.3, 5, 73), 'M_Wood', 1)
    box(((i - 2) * 16.5, 0, 81), (15.3, 71, 5), 'M_Wood', .8)
for x in [-43, 43]:
    for i in range(4): box((x, (i - 1.5) * 17.5, 40), (5, 16.2, 73), 'M_Wood', .8)
for x in [-32, 32]:
    for y in [-39, 39]: box((x, y, 40), (6, 3, 82), 'M_Bronze', .5)
    box((x, 0, 84), (6, 79, 3), 'M_Bronze', .5)
for x in [-32, 32]:
    for z in [9, 72]: cylinder((x, -41, z), 1.25, 1, 'M_DarkMetal', 'Y', 10, .2)
collision_box((0, 0, 42), (90, 82, 84))
finish('SM_Crate', 'Base center')

for loc, radius, height, tilt in [((0, 0, 4), 12, 48, 0), ((10, 4, 4), 7, 29, -.25), ((-10, -3, 3), 6, 25, .22)]:
    verts = []
    for z, r in [(0, radius*.8), (height*.72, radius), (height, 0)]:
        for i in range(6): verts.append((loc[0]+math.cos(i*math.tau/6)*r+z*tilt, loc[1]+math.sin(i*math.tau/6)*r, loc[2]+z))
    faces = [tuple(reversed(range(6)))]
    for row in range(2):
        for i in range(6): faces.append((row*6+i,row*6+(i+1)%6,(row+1)*6+(i+1)%6,(row+1)*6+i))
    mesh('Faceted crystal', verts, faces, 'M_Crystal', .18)
ring((0, 0, 5), 14, 10.5, 6, 'M_Bronze', 'Z', .5, 20)
finish('SM_Crystal', 'Base center; crystal cluster height 52cm')

box((0, 0, 159), (239, 47, 318), 'M_DarkMetal', 7)
for x in [-109, 109]: box((x, -4, 160), (18, 57, 318), 'M_Bronze', 3)
for z in [15, 305]: box((0, -3, z), (224, 58, 24), 'M_Bronze', 3)
ring((0, -31, 178), 75, 59, 12, 'M_Bronze', 'Y', 1.4, 48)
cylinder((0, -29, 178), 58, 4, 'M_DarkMetal', 'Y', 40, 1)
ring((0, -33, 178), 53, 50, 2.3, 'M_Core', 'Y', .3, 48)
for x in [-89, 89]:
    box((x, -31, 172), (15, 9, 163), 'M_Ceramic', 2)
    for z in [105, 129, 153, 177, 201, 225]: box((x, -37, z), (9, 3, 4), 'M_DarkMetal', .4)
for x in [-56, 56]:
    box((x, -28, 54), (60, 11, 35), 'M_Ceramic', 3)
    cylinder((x, -36, 55), 6, 6, 'M_Bronze', 'Y', 16, .8)
box((0, -35, 178), (13, 5, 65), 'M_Core', 1.8)
collision_box((0, 0, 160), (240, 58, 320))
finish('SM_TechPanel', 'Base center; X width, front -Y')

for i in range(15):
    angle = i * 2.39996
    base = Vector((math.cos(angle)*random.uniform(1,17), math.sin(angle)*random.uniform(1,17), 0))
    direction = Vector((math.cos(angle), math.sin(angle), 0))
    side = Vector((-direction.y, direction.x, 0))
    height = random.uniform(18, 42); width = random.uniform(1.8, 4)
    verts = []
    for j in range(4):
        t = j / 3; p = base + direction * (t*t*height*.56) + Vector((0,0,height*t))
        w = width * (1-t) if j<3 else .06
        verts += [tuple(p-side*w),tuple(p+side*w),tuple(p+Vector((0,0,.5)))]
    faces=[]
    for j in range(3):
        n=j*3; faces += [(n,n+3,n+5,n+2),(n+2,n+5,n+4,n+1)]
    mesh('Sculpted grass blade',verts,faces,'M_Moss',0,True)
finish('SM_Grass', 'Ground center; two-sided decoration, no collision')

ring((0,0,.5),100,94,1,'M_CombatGlow','Z',0,64)
finish('SM_TelegraphRing','Ground center; outer radius100cm; no collision')
cylinder((0,0,.25),100,.5,'M_CombatGlow','Z',64,0)
finish('SM_TelegraphDisk','Ground center; radius100cm; no collision')

metadata = {name: value['meta'] for name, value in ASSETS.items()}
(OUT / 'asset-metadata.json').write_text(json.dumps(metadata, indent=2) + '\n', encoding='utf-8')
for meta in metadata.values():
    assert meta['vertices'] > 0 and meta['triangles'] > 0
    assert all(math.isfinite(n) and n > 0 for n in meta['dimensions_cm'])
assert set(PALETTE) >= {'M_Ceramic','M_Bronze','M_DarkMetal','M_Core','M_Frost','M_Storm','M_Ember','M_CombatGlow'}

# Preview copies are not exported. Three actual renders form the review sheet.
preview_objects = []
def instance(name, loc=(0,0,0), scale=1, yaw=0):
    original = ASSETS[name]['object']
    obj = original.copy(); obj.data = original.data
    scene.collection.objects.link(obj)
    obj.hide_render=False; obj.hide_set(False); obj.location=loc; obj.scale=(scale,)*3; obj.rotation_euler.z=math.radians(yaw)
    preview_objects.append(obj)
    return obj

def clear_preview():
    for obj in preview_objects: bpy.data.objects.remove(obj, do_unlink=True)
    preview_objects.clear()

def guardian(loc=(0,0,0), scale=1, yaw=0):
    for name, offset in [('SM_GuardianBody',(0,0,94)),('SM_GuardianHead',(0,0,156)),('SM_GuardianArm',(0,36,146)),('SM_GuardianArm',(0,-36,146)),('SM_GuardianLeg',(0,18,94)),('SM_GuardianLeg',(0,-18,94))]:
        a=math.radians(yaw); x,y,z=offset
        instance(name,(loc[0]+scale*(x*math.cos(a)-y*math.sin(a)),loc[1]+scale*(x*math.sin(a)+y*math.cos(a)),loc[2]+scale*z),scale,yaw)

scene.render.engine='CYCLES'; scene.cycles.samples=40; scene.cycles.use_denoising=True
scene.render.threads_mode='FIXED'; scene.render.threads=6
scene.render.image_settings.file_format='PNG'; scene.render.resolution_percentage=100
scene.world.color=(.13,.16,.19)
scene.view_settings.view_transform='AgX'
floor_mat=bpy.data.materials.new('Preview ground'); floor_mat.diffuse_color=(.065,.08,.073,1)
bpy.ops.mesh.primitive_plane_add(size=20000,location=(0,0,-2))
floor=bpy.context.object; floor.data.materials.append(floor_mat)
bpy.ops.object.camera_add(); camera=bpy.context.object; scene.camera=camera
lights=[]
for name,loc,power,size,color in [('Warm key',(-300,-300,600),2200000,380,(1,.79,.52)),('Cool fill',(400,100,420),1300000,320,(.51,.69,1)),('Rim',(-80,450,500),1800000,240,(1,.89,.64))]:
    data=bpy.data.lights.new(name,'AREA'); data.energy=power; data.shape='DISK'; data.size=size; data.color=color
    obj=bpy.data.objects.new(name,data); scene.collection.objects.link(obj); obj.location=loc; obj.rotation_euler=(Vector((0,0,80))-obj.location).to_track_quat('-Z','Y').to_euler(); lights.append(obj)

def render(name, eye, target, scale, resolution):
    if '--skip-renders' in sys.argv and (PREVIEW/(name+'.png')).is_file(): return
    camera.location=eye; camera.rotation_euler=(Vector(target)-camera.location).to_track_quat('-Z','Y').to_euler()
    camera.data.type='ORTHO'; camera.data.ortho_scale=scale; camera.data.clip_end=30000
    scene.render.resolution_x,scene.render.resolution_y=resolution
    scene.render.filepath=str(PREVIEW/(name+'.png'))
    bpy.ops.render.render(write_still=True)

for name,loc in [('SM_WeaponBody',(0,0,0)),('SM_ShieldPlate',(31,0,10)),('SM_Core',(33,0,10)),('SM_Forearm',(0,0,-1))]: instance(name,loc)
render('weapon',(-125,-145,88),(6,0,4),110,(1100,800))
clear_preview(); guardian()
render('guardian',(320,-440,245),(0,0,97),268,(900,1000))
clear_preview()
for x in [-400,-200,0,200,400]:
    for y in [-200,0,200]: instance('SM_StoneTile',(x,y,0))
instance('SM_Wall',(-405,350,24)); instance('SM_Arch',(70,350,24))
instance('SM_Pillar',(-175,295,24)); instance('SM_Pillar',(320,330,24))
instance('SM_Bell',(70,345,245),.85)
instance('SM_TechPanel',(-590,200,24),1,15)
instance('SM_Root',(-175,285,24),1.3,190)
instance('SM_Crate',(-340,-75,24)); instance('SM_Crate',(-410,30,24),.75,16)
instance('SM_Crystal',(-255,-112,24),1.45)
for x,y in [(-375,-185),(-170,130),(335,260),(300,-170),(-490,190),(-160,215),(420,70),(-490,-30)]: instance('SM_Grass',(x,y,24),random.uniform(1,1.5),random.randrange(360))
guardian((185,-25,24),1,7)
render('courtyard',(1020,-1400,960),(-40,145,174),1330,(1600,1000))
clear_preview()

# Review sheet contains only actual Blender renders, assembled without generation.
import numpy as np
sheet = np.full((2250,2000,4),(.29,.29,.28,1),dtype=np.float32)
for name,x,y,w,h in [('courtyard',0,0,2000,1250),('weapon',0,1350,1100,800),('guardian',1100,1250,900,1000)]:
    image=bpy.data.images.load(str(PREVIEW/(name+'.png')),check_existing=False)
    image.colorspace_settings.name='Non-Color'; image.scale(w,h)
    data=np.empty(w*h*4,dtype=np.float32); image.pixels.foreach_get(data)
    sheet[y:y+h,x:x+w]=data.reshape((h,w,4))
    bpy.data.images.remove(image)
contact=bpy.data.images.new('Actual geometry contact sheet',width=2000,height=2250,alpha=True)
contact.colorspace_settings.name='Non-Color'; contact.pixels.foreach_set(sheet.ravel())
contact.filepath_raw=str(PREVIEW/'contact-sheet.png'); contact.file_format='PNG'; contact.save()
bpy.data.images.remove(contact)

# Save compressed editable scene without unused default brush/library references.
for image in (surface_image,normal_image): image.filepath='//generated/'+image.name+'.png'
bpy.data.orphans_purge(do_local_ids=True,do_linked_ids=True,do_recursive=True)
bpy.ops.wm.save_as_mainfile(filepath=str(ART/'Dreambound_Kit.blend'),compress=True)
with (ART/'manifest-rows.csv').open('w',newline='',encoding='utf-8') as handle:
    writer=csv.writer(handle); writer.writerow(['asset_id','path','source_url','creator','license','proof_path','modifications','approval_status'])
    for name in ASSETS:
        writer.writerow([name.lower(),'art/generated/'+name+'.fbx','','Game Studio with OpenAI Codex','Original project-authored asset; no third-party inputs','scripts/create_art.py','Procedural Blender modeling; real bevels; original vertex tint; UVs; material slots; FBX export','original'])
    for name in ['T_PaintedSurface','T_SculptedNormal']:
        writer.writerow([name.lower(),'art/generated/'+name+'.png','','Game Studio with OpenAI Codex','Original project-authored texture; no third-party inputs','scripts/create_art.py','Original seeded isotropic spectral pigment and pore fields; periodic gradients; 512px PNG; replaces directional weave','original'])
    writer.writerow(['dreambound_painterly_materials','scripts/import_art.py','','Game Studio with OpenAI Codex','Original project-authored shader recipes; no third-party inputs','art/generated/materials.json','Cool slate and verdigris against ivory and copper; vertex tint; pigment color variation; restrained material-specific normals; roughness and patina; emission; instancing usage','original'])
print('DREAMBOUND_ART_COMPLETE '+json.dumps({'assets':len(ASSETS),'triangles':sum(x['triangles'] for x in metadata.values()),'metadata':str(OUT/'asset-metadata.json')}))

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
import time
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
PALETTE['M_Stone'].update(color=[.19,.22,.215], wear_color=[.36,.36,.305], weather_color=[.10,.135,.058])
PALETTE['M_StoneLight'].update(color=[.32,.335,.30], wear_color=[.43,.425,.35], weather_color=[.12,.15,.065])
PALETTE['M_Cloth'].update(color=[.029,.066,.061])
PALETTE['M_Root'].update(color=[.115,.078,.039])
PALETTE['M_RootLight']={'color':[.22,.148,.068],'roughness':.92,'metallic':0,'normal_strength':.5}
PALETTE['M_Leaf']={'color':[.07,.13,.025],'roughness':.82,'metallic':0,'normal_strength':.08,'two_sided':True}
PALETTE['M_LeafLight']={'color':[.145,.21,.044],'roughness':.84,'metallic':0,'normal_strength':.08,'two_sided':True}
PALETTE['M_LeafGold']={'color':[.255,.238,.055],'roughness':.87,'metallic':0,'normal_strength':.08,'two_sided':True}
PALETTE['M_ShieldCeramic']=dict(PALETTE['M_Ceramic'],color=[.66,.64,.53],roughness=.43,texture='T_ShieldCraft',normal_texture='T_ShieldCraftNormal',wear_color=[.40,.36,.265],weather_color=[.21,.20,.145])
PALETTE['M_ShieldBronze']=dict(PALETTE['M_Bronze'],texture='T_ShieldCraft',normal_texture='T_ShieldCraftNormal',wear_color=[.60,.355,.135],weather_color=[.033,.16,.12])
for name,spec in PALETTE.items():
    spec['vertex_masks']=True
    spec.setdefault('wear_color', [min(1,c*1.38) for c in spec['color']])
    spec.setdefault('weather_color', [.036,.13,.105] if spec['metallic']>.4 else [c*.66 for c in spec['color']])
    spec.setdefault('weather_amount', .7 if name.startswith('M_Stone') else .48)
    if name.startswith('M_Stone'):
        spec.update(texture='T_MasonryCraft', normal_texture='T_MasonryCraftNormal')
    if name.startswith('M_Root'):
        spec.update(texture='T_BarkCraft',normal_texture='T_BarkCraftNormal',normal_strength=.9,tone_low=[.42,.43,.40],tone_high=[1.22,1.18,1.1])
    if name=='M_Bronze':
        spec.update(texture='T_MetalCraft',normal_texture='T_MetalCraftNormal',normal_strength=.55,tone_low=[.72,.75,.7],tone_high=[1.13,1.08,1.0])
    if name.startswith('M_Leaf') or name=='M_Moss':spec['foliage']=True
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
def craft_texture(name, kind):
    # Authored masks: shield coordinates correspond to its actual YZ face.
    size=1024; yy,xx=np.mgrid[0:size,0:size]/(size-1)
    radial=np.sqrt((xx-.5)**2+(yy-.5)**2); angle=np.arctan2(yy-.5,xx-.5)
    rng=np.random.default_rng(53014 if kind=='shield' else 53015)
    wear=np.zeros((size,size)); incision=np.zeros_like(wear)
    if kind=='shield':
        for radius,width in [(.443,.0018),(.422,.001),(.172,.0015)]:
            incision=np.maximum(incision,np.exp(-((radial-radius)/width)**2)*.5)
        # Local toolmarks, chipped ceramic and radial scratches near the rim.
        for unused in range(95):
            a=rng.uniform(-math.pi,math.pi); r=rng.uniform(.26,.465)
            cx=.5+math.cos(a)*r; cy=.5+math.sin(a)*r
            rotation=a+rng.uniform(-.8,.8)
            u=(xx-cx)*math.cos(rotation)+(yy-cy)*math.sin(rotation)
            v=-(xx-cx)*math.sin(rotation)+(yy-cy)*math.cos(rotation)
            mark=np.exp(-((u/rng.uniform(.003,.018))**2+(v/rng.uniform(.0004,.0014))**2))
            wear=np.maximum(wear,mark); incision=np.maximum(incision,mark*.7)
        paint=.5+.045*np.cos(angle*6+.3)*(radial<.44)+.025*np.sin(xx*13+yy*9)
    elif kind=='bark':
        warped=xx*14+.20*np.sin(yy*math.tau)+.08*np.sin(yy*math.tau*4+xx*8)
        ridge=.5+.5*np.cos(warped*math.tau)
        fissure=np.exp(-((ridge-.04)/.095)**2)
        paint=.57-fissure*.37+.055*np.sin(yy*19+xx*5)
        wear=np.clip(ridge*.21,0,1);incision=fissure*.8
    elif kind=='metal':
        paint=.5+.07*np.sin(xx*7+yy*11)*np.cos(yy*5-xx*13)
        for unused in range(100):
            cx,cy=rng.random(2);a=rng.uniform(-.65,.65)
            u=(xx-cx)*math.cos(a)+(yy-cy)*math.sin(a);v=-(xx-cx)*math.sin(a)+(yy-cy)*math.cos(a)
            mark=np.exp(-(u/rng.uniform(.005,.045))**2-(v/.0009)**2)
            wear=np.maximum(wear,mark*.7);incision=np.maximum(incision,mark*.45)
    else:
        # Uneven chisel planes and scattered mineral flecks, with no weave.
        paint=np.full_like(wear,.5)
        for unused in range(80):
            cx,cy=rng.random(2); sx,sy=rng.uniform(.01,.11,2)
            spot=np.exp(-(((xx-cx)/sx)**2+((yy-cy)/sy)**2))
            paint+=spot*rng.uniform(-.075,.075)
            wear=np.maximum(wear,spot*rng.uniform(.05,.4))
        for unused in range(28):
            cx,cy=rng.random(2); a=rng.uniform(-math.pi,math.pi)
            u=(xx-cx)*math.cos(a)+(yy-cy)*math.sin(a); v=-(xx-cx)*math.sin(a)+(yy-cy)*math.cos(a)
            incision=np.maximum(incision,np.exp(-(u/.035)**2-(v/.0007)**2)*.22)
    pix=np.ones((size,size,4),np.float32); pix[:,:,0]=np.clip(paint-wear*.12,0,1)
    pix[:,:,1]=np.clip(.5+wear*.34,0,1); pix[:,:,2]=wear
    height=-incision*.09
    dx=np.gradient(height,axis=1)*8; dy=np.gradient(height,axis=0)*8
    tangent=np.stack((-dx,-dy,np.ones_like(dx)),axis=-1); tangent/=np.linalg.norm(tangent,axis=-1,keepdims=True)
    norm=np.ones_like(pix); norm[:,:,:3]=tangent*.5+.5
    for image_name,data in [(name,pix),(name+'Normal',norm)]:
        img=bpy.data.images.get(image_name) or bpy.data.images.new(image_name,width=size,height=size,alpha=True)
        img.colorspace_settings.name='Non-Color'; img.pixels.foreach_set(data.ravel())
        img.filepath_raw=str(OUT/(image_name+'.png'));img.file_format='PNG';img.save()
craft_texture('T_ShieldCraft','shield'); craft_texture('T_MasonryCraft','stone')
craft_texture('T_BarkCraft','bark');craft_texture('T_MetalCraft','metal')
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
    tex = nodes.new('ShaderNodeTexImage'); tex.image = bpy.data.images.get(spec.get('texture','T_PaintedSurface'))
    ramp = nodes.new('ShaderNodeValToRGB')
    ramp.color_ramp.elements[0].position = 0
    ramp.color_ramp.elements[0].color = (*(c * t for c, t in zip(spec['color'], spec.get('tone_low', [.88, .89, .90]))), 1)
    ramp.color_ramp.elements[1].position = 1
    ramp.color_ramp.elements[1].color = (*(min(1, c * t) for c, t in zip(spec['color'], spec.get('tone_high', [1.07, 1.04, 1.0]))), 1)
    separate = nodes.new('ShaderNodeSeparateColor')
    links.new(tex.outputs['Color'], separate.inputs['Color'])
    links.new(separate.outputs['Red'], ramp.inputs['Fac'])
    base = ramp.outputs['Color']
    if spec.get('patina_amount') and not spec.get('vertex_masks'):
        amount = nodes.new('ShaderNodeMath'); amount.operation = 'MULTIPLY'; amount.inputs[1].default_value = spec['patina_amount']
        links.new(separate.outputs['Blue'], amount.inputs[0])
        coat = nodes.new('ShaderNodeMixRGB'); coat.inputs[2].default_value = (*spec['patina_color'], 1)
        links.new(amount.outputs[0], coat.inputs[0]); links.new(base, coat.inputs[1])
        base = coat.outputs[0]
    vc = nodes.new('ShaderNodeVertexColor'); vc.layer_name = 'Color'
    masks=nodes.new('ShaderNodeSeparateColor');links.new(vc.outputs['Color'],masks.inputs['Color'])
    worn=nodes.new('ShaderNodeMixRGB'); worn.inputs[2].default_value=(*spec['wear_color'],1)
    texture_wear=nodes.new('ShaderNodeMath');texture_wear.operation='MULTIPLY'
    texture_wear.inputs[1].default_value=.30 if name.startswith('M_Shield') else .22 if name.startswith('M_Stone') else .06
    links.new(separate.outputs['Blue'],texture_wear.inputs[0])
    wear_sum=nodes.new('ShaderNodeMath');wear_sum.operation='ADD'
    links.new(texture_wear.outputs[0],wear_sum.inputs[0]);links.new(masks.outputs['Green'],wear_sum.inputs[1])
    links.new(base,worn.inputs[1]); links.new(wear_sum.outputs[0],worn.inputs[0])
    aged=nodes.new('ShaderNodeMixRGB');aged.inputs[2].default_value=(*spec['weather_color'],1)
    amount=nodes.new('ShaderNodeMath');amount.operation='MULTIPLY';amount.inputs[1].default_value=spec['weather_amount']
    links.new(masks.outputs['Blue'],amount.inputs[0]);links.new(amount.outputs[0],aged.inputs[0]);links.new(worn.outputs[0],aged.inputs[1])
    mult = nodes.new('ShaderNodeMixRGB'); mult.blend_type = 'MULTIPLY'; mult.inputs[0].default_value = 1
    links.new(aged.outputs[0], mult.inputs[1]); links.new(masks.outputs['Red'], mult.inputs[2])
    links.new(mult.outputs[0], shader.inputs['Base Color'])
    rough = nodes.new('ShaderNodeMapRange')
    variation = spec.get('roughness_variation', .08)
    rough.inputs['From Min'].default_value=0; rough.inputs['From Max'].default_value=1
    rough.inputs['To Min'].default_value=max(.12,spec['roughness']-variation); rough.inputs['To Max'].default_value=min(1,spec['roughness']+variation)
    links.new(separate.outputs['Green'],rough.inputs['Value']); links.new(rough.outputs['Result'],shader.inputs['Roughness'])
    normal_tex = nodes.new('ShaderNodeTexImage'); normal_tex.image = bpy.data.images.get(spec.get('normal_texture','T_SculptedNormal'))
    normal = nodes.new('ShaderNodeNormalMap'); normal.inputs['Strength'].default_value = spec.get('normal_strength', .15)
    links.new(normal_tex.outputs['Color'], normal.inputs['Color']); links.new(normal.outputs['Normal'], shader.inputs['Normal'])
    if spec.get('emissive'):
        shader.inputs['Emission Color'].default_value = (*spec['color'], 1)
        shader.inputs['Emission Strength'].default_value = spec['emissive']
    if spec.get('foliage'):
        translucent=nodes.new('ShaderNodeBsdfTranslucent');links.new(mult.outputs[0],translucent.inputs['Color'])
        leafmix=nodes.new('ShaderNodeMixShader');leafmix.inputs[0].default_value=.28
        links.new(shader.outputs['BSDF'],leafmix.inputs[1]);links.new(translucent.outputs[0],leafmix.inputs[2])
        links.new(leafmix.outputs[0],output.inputs['Surface'])
    MATS[name] = mat

if MATERIALS_ONLY:
    metadata=json.loads((OUT/'asset-metadata.json').read_text(encoding='utf-8'))
    ASSETS={name:{'object':bpy.data.objects[name],'meta':meta} for name,meta in metadata.items()}
    for obj in list(bpy.data.objects):
        if obj.type in {'CAMERA','LIGHT'}:bpy.data.objects.remove(obj,do_unlink=True)
    review_source=ART/'source'/'review.py'
    exec(compile(review_source.read_text(encoding='utf-8'),str(review_source),'exec'),globals())
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
    if len(parts)>1: bpy.ops.object.join()
    obj = bpy.context.object; obj.name = name; obj.data.name = name
    scene.cursor.location = (0, 0, 0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    uv=single_craft_uv(obj.data)
    if name == 'SM_ShieldPlate':
        # The authored circular wear field belongs on front/back faces. Rim
        # thickness and structural spokes retain their non-collapsed local UVs.
        for face in obj.data.polygons:
            if abs(face.normal.x) < .65: continue
            for index in face.loop_indices:
                p=obj.data.vertices[obj.data.loops[index].vertex_index].co
                uv.data[index].uv=(.5+p.y/92,.5+p.z/92)
    repair_mesh_uv(obj.data)
    obj.data.calc_loop_triangles()
    bounds = [obj.matrix_world @ Vector(p) for p in obj.bound_box]
    lo = [min(p[i] for p in bounds) for i in range(3)]
    hi = [max(p[i] for p in bounds) for i in range(3)]
    meta = {'name': name, 'dimensions_cm': [round(hi[i] - lo[i], 3) for i in range(3)], 'bounds_min_cm': lo, 'bounds_max_cm': hi,
            'vertices': len(obj.data.vertices), 'triangles': len(obj.data.loop_triangles), 'pivot': pivot,
            'materials': [mat.name for mat in obj.data.materials], 'collision_hulls': len(collisions)}
    meta['vertex_channels']={'R':'painted value','G':'exposed edge wear','B':'recess/weathering','A':'opaque'}
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
        for attempt in range(20):
            try:
                export_path.replace(OUT / (name + '.fbx'))
                break
            except PermissionError:
                if attempt==19: raise
                time.sleep(.25)
    finally:
        for node,image in preview_images: node.image=image
    for collider in collisions: bpy.data.objects.remove(collider, do_unlink=True)
    obj.hide_render = True; obj.hide_set(True)
    ASSETS[name] = {'object': obj, 'meta': meta}
    parts, collisions = [], []


# Author the focused replacement kit using the shared, verified exporter.
uv_source=ART / 'source' / 'uv_tools.py'
exec(compile(uv_source.read_text(encoding='utf-8'),str(uv_source),'exec'),globals())
authoring_source=ART / 'source' / 'bellroot.py'
exec(compile(authoring_source.read_text(encoding='utf-8'),str(authoring_source),'exec'),globals())

metadata = {name: value['meta'] for name, value in ASSETS.items()}
(OUT / 'asset-metadata.json').write_text(json.dumps(metadata, indent=2) + '\n', encoding='utf-8')
for meta in metadata.values():
    assert meta['vertices'] > 0 and meta['triangles'] > 0
    assert all(math.isfinite(n) and n > 0 for n in meta['dimensions_cm'])
assert set(PALETTE) >= {'M_Ceramic','M_Bronze','M_DarkMetal','M_Core','M_Frost','M_Storm','M_Ember','M_CombatGlow'}

# Actual geometry review and provenance rows.
review_source=ART / 'source' / 'review.py'
exec(compile(review_source.read_text(encoding='utf-8'),str(review_source),'exec'),globals())

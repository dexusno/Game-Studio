"""Original procedural art study. Blender 5.2; no game state or combat rules.

Run Blender --background --factory-startup --python build_scene.py -- --stills
Use --animate instead for the full 192-frame / 8-second PNG sequence.
All object names are stable; random seed affects background/scrap placement only.
"""
import argparse
import json
import math
import random
import sys
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parent
RAW = ROOT / "raw"
RAW.mkdir(parents=True, exist_ok=True)
args = argparse.ArgumentParser()
args.add_argument("--animate", action="store_true")
args.add_argument("--stills", action="store_true")
opt = args.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
random.seed(16)
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.render.engine = "CYCLES" if False else "BLENDER_EEVEE"
scene.render.resolution_x = 960
scene.render.resolution_y = 540
scene.render.resolution_percentage = 100
scene.render.fps = 24
scene.frame_start = 1
scene.frame_end = 192
scene.render.image_settings.file_format = "PNG"
scene.render.film_transparent = False
scene.world.color = (0.25, 0.31, 0.40)
scene.view_settings.view_transform = "AgX"
scene.view_settings.look = "AgX - Medium High Contrast"

def material(name, base, metal=0., rough=.4, emission=0., wear=True):
    m = bpy.data.materials.new(name)
    m.diffuse_color = (*base, 1)
    m.use_nodes = True
    ns, lk = m.node_tree.nodes, m.node_tree.links
    p = ns.get("Principled BSDF")
    p.inputs["Base Color"].default_value = (*base, 1)
    p.inputs["Metallic"].default_value = metal
    p.inputs["Roughness"].default_value = rough
    if emission:
        p.inputs["Emission Color"].default_value = (*base, 1)
        p.inputs["Emission Strength"].default_value = emission
    elif wear:
        n = ns.new("ShaderNodeTexNoise")
        n.inputs["Scale"].default_value = 7
        n.inputs["Detail"].default_value = 2
        ramp = ns.new("ShaderNodeValToRGB")
        ramp.color_ramp.elements[0].position = .2
        ramp.color_ramp.elements[0].color = (*(c * .5 for c in base), 1)
        ramp.color_ramp.elements[1].position = .78
        ramp.color_ramp.elements[1].color = (*(min(c * 1.15, 1) for c in base), 1)
        lk.new(n.outputs["Fac"], ramp.inputs[0])
        lk.new(ramp.outputs["Color"], p.inputs["Base Color"])
        bump = ns.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = .12
        bump.inputs["Distance"].default_value = .045
        lk.new(n.outputs["Fac"], bump.inputs["Height"])
        lk.new(bump.outputs["Normal"], p.inputs["Normal"])
    return m

steel = material("Steel | blue grey", (.23, .29, .33), .78, .34)
brass = material("Brass | ochre edges", (.52, .30, .11), .72, .3)
dark = material("Iron | dark recesses", (.06, .08, .09), .75, .4)
red = material("Magnet | worn vermilion", (.5, .045, .018), .5, .36)
wood = material("Timber | dark warm", (.19, .12, .07), .05, .75)
groundmat = material("Dust | warm low contrast", (.32, .23, .15), 0, .92)
backmat = material("Distance | cool silhouette", (.15, .23, .31), 0, .95, wear=False)
hot = material("Forge | amber emission", (1., .095, .002), .1, .28, 2, False)
eye = material("Enemy | red emission", (1., .045, .01), .05, .3, 3, False)
sparkmat = material("Sparks | pale gold", (1., .57, .12), 0, .3, 6, False)

def finish(o, name, mat, bevel=.045, parent=None):
    o.name = name
    o.data.materials.append(mat)
    if bevel:
        mod = o.modifiers.new("Edge highlights", "BEVEL")
        mod.width = bevel
        mod.segments = 2
    if o.type == "MESH":
        for p in o.data.polygons:
            p.use_smooth = True
        normal = o.modifiers.new("Weighted corner normals", "WEIGHTED_NORMAL")
        normal.keep_sharp = True
    if parent:
        o.parent = parent
    return o

def empty(name, loc=(0, 0, 0), parent=None):
    o = bpy.data.objects.new(name, None)
    scene.collection.objects.link(o)
    o.location = loc
    o.parent = parent
    return o

def box(name, loc, scale, mat, bevel=.045, parent=None):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    o = bpy.context.object
    o.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return finish(o, name, mat, bevel, parent)

def cylinder(name, loc, radius, depth, mat, axis="Z", vertices=32, parent=None):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=loc)
    o = bpy.context.object
    if axis == "X":
        o.rotation_euler.y = math.pi / 2
    if axis == "Y":
        o.rotation_euler.x = math.pi / 2
    return finish(o, name, mat, min(.035, radius / 7), parent)

def ring(name, loc, radius, inner, depth, mat, teeth=0, axis="Y", parent=None):
    n = teeth * 4 if teeth else 48
    vs = []
    for z in [-depth / 2, depth / 2]:
        for is_inner in [False, True]:
            for i in range(n):
                a = 2 * math.pi * i / n
                r = inner if is_inner else radius * (.86 if teeth and i % 4 in [0, 3] else 1)
                vs.append((r * math.cos(a), r * math.sin(a), z))
    fs = []
    for i in range(n):
        j = (i + 1) % n
        fs += [(i, j, n + j, n + i), (2*n+i, 3*n+i, 3*n+j, 2*n+j),
               (i, 2*n+i, 2*n+j, j), (n+i, n+j, 3*n+j, 3*n+i)]
    me = bpy.data.meshes.new(name)
    me.from_pydata(vs, [], fs)
    me.update()
    o = bpy.data.objects.new(name, me)
    scene.collection.objects.link(o)
    o.location = loc
    if axis == "X": o.rotation_euler.y = math.pi / 2
    elif axis == "Y": o.rotation_euler.x = math.pi / 2
    return finish(o, name, mat, .025, parent)

def rod(name, a, b, radius, mat, parent=None):
    a, b = Vector(a), Vector(b)
    o = cylinder(name, (a + b) / 2, radius, (a - b).length, mat, parent=parent)
    o.rotation_euler = (b - a).to_track_quat("Z", "Y").to_euler()
    return o

def keys(o, prop, points):
    for frame, value in points:
        setattr(o, prop, value)
        o.keyframe_insert(data_path=prop, frame=frame)

def orient(o, at):
    o.rotation_euler = (Vector(at)-o.location).to_track_quat("-Z", "Y").to_euler()

def camera(name, pos, at, lens=45, ortho=None):
    d = bpy.data.cameras.new(name)
    o = bpy.data.objects.new(name, d)
    scene.collection.objects.link(o)
    o.location = pos
    orient(o, at)
    d.lens = lens
    if ortho:
        d.type = "ORTHO"
        d.ortho_scale = ortho
    return o

def light(name, pos, color, energy, size):
    d = bpy.data.lights.new(name, "AREA")
    d.energy, d.color, d.shape, d.size = energy, color, "DISK", size
    o = bpy.data.objects.new(name, d)
    scene.collection.objects.link(o)
    o.location = pos
    orient(o, (0, 0, 0))
    return o

# Warm, low-detail scrapyard; horizon silhouettes are deliberately subordinate.
box("Ground", (0, 0, -.26), (240, 240, .35), groundmat, 0)
sky = material("Sky | peach backdrop",(.64,.38,.23),0,1,1,False)
mount = material("Haze | distant slate",(.22,.28,.35),0,1,.45,False)
box("Sky backdrop",(0,47,22),(180,.2,48),sky,0)
points=[(-70,0),(-70,5),(-31,5),(-28,8),(-25,7),(-20,10),(-17,9),(-15,11),(-11,7),(-8,9),(-4,8),(1,11),(4,9),(7,10),(10,6),(17,7),(21,10),(24,8),(29,7),(34,9),(70,5),(70,0)]
me=bpy.data.meshes.new("Distant mountain silhouette")
me.from_pydata([(x,36,z) for x,z in points],[],[tuple(range(len(points)))]);me.update()
o=bpy.data.objects.new("Distant mountains",me);scene.collection.objects.link(o);finish(o,o.name,mount,0)
for i in range(19):
    box(f"Stage plank {i:02}", (-8.3+i*.95, 0, -.035), (.91, 5.6, .22), wood, .025)
for y in [-2.5, 2.5]:
    box("Deck iron border", (0, y, .09), (18.2, .15, .13), dark)
for i in range(13):
    x, y, h = random.uniform(-24, 24), random.uniform(23, 29), random.uniform(2, 5)
    box(f"Distant salvage tower {i:02}", (x, y, h/2), (random.uniform(.7, 1.5), 2, h), backmat, .02)
for i, x in enumerate([-17, -10, 1, 13, 20]):
    for dx in [-.3,.3]:
        box(f"Crane {i} pillar", (x+dx, 21, 3.1), (.15, .3, 6.2), backmat, .01)
    box(f"Crane {i} beam", (x+1, 21, 6), (6, .3, .25), backmat, .01)
    rod(f"Crane {i} brace", (x,21,3), (x+3,21,6), .065, backmat)

# Rig: substantial forge, forward barrel, brace, flywheel and exposed loading trough.
rig = empty("RIG | persistent")
box("Rig sled", (-5, 0, .32), (3.8, 2.1, .45), dark, parent=rig)
for x in [-6.3,-4]:
    box("Rig foot", (x, 0, .18), (.5, 2.6, .18), brass, parent=rig)
    box("Rig upright", (x, .15, 1), (.34, 1.4, 1.5), steel, parent=rig)
recoil = empty("CANNON | recoil", parent=rig)
box("Forge body", (-5.35,0,1.92), (1.65,1.45,1.85), steel, .16, recoil)
box("Forge door dark rim", (-5.45,-.755,1.98), (1.21,.13,1.27), dark,.12,recoil)
box("Forge bright window", (-5.45,-.845,1.98), (.95,.07,.94), hot,.09,recoil)
for x in [-5.9,-4.98]:
    box("Door brass upright", (x,-.9,1.99), (.07,.08,1.06),brass,.02,recoil)
for z in [1.48,2.48]:
    box("Door brass crossbar", (-5.45,-.9,z), (1.05,.08,.07),brass,.02,recoil)
cylinder("Barrel outer", (-3.9,0,2.05), .56, 2.8, steel,"X",parent=recoil)
for x in [-4.95,-4.05,-2.6]:
    ring("Barrel brass hoop", (x,0,2.05), .62,.53,.19,brass,axis="X",parent=recoil)
ring("Open muzzle rim", (-2.43,0,2.05), .59,.40,.17,steel,axis="X",parent=recoil)
cylinder("Muzzle darkness", (-2.50,0,2.05), .397,.015,dark,"X",parent=recoil)
for a in range(0,360,60):
    cylinder("Muzzle bolt", (-2.32,.51*math.cos(math.radians(a)),2.05+.51*math.sin(math.radians(a))), .045,.07,brass,"X",6,recoil)
ring("Forge flywheel gear", (-5.4,-1.05,.87), .64,.22,.19,steel,14,parent=rig)
cylinder("Flywheel hub", (-5.4,-1.14,.87), .23,.28,brass,"Y",parent=rig)
for a in range(0,360,90):
    t=math.radians(a)
    rod("Flywheel spoke",(-5.4+.2*math.cos(t),-1.12,.87+.2*math.sin(t)),(-5.4+.49*math.cos(t),-1.12,.87+.49*math.sin(t)),.055,brass,rig)
box("ONE SHOT | open loading channel",(-4.3,-1.1,1.12),(3.3,.75,.14),dark,.04,rig)
for y in [-1.43,-.78]:
    box("Channel continuous brass lip",(-4.3,y,1.28),(3.4,.085,.23),brass,.025,rig)
for x in [-5.9,-2.7]:
    box("Channel end bracket",(x,-1.1,1.3),(.1,.8,.37),steel,.03,rig)
for x in [-6,-4.85]:
    for z in [1.34,2.6]:
        cylinder("Forge face rivet",(x,-.78,z),.07,.05,brass,"Y",8,recoil)

# Protective plate remains part of this rig. It is offset from the firing axis.
plate = empty("PLATE | mounted protection", (-2.2,-.7,0), rig)
plate.rotation_euler.z = math.radians(-28)
box("Plate steel face", (0,0,1.12),(.18,1.35,1.85),steel,.065,plate)
for y in [-.57,.57]:
    box("Plate brass edge", (0,y,1.13),(.24,.09,1.95),brass,.025,plate)
    for z in [.3,.7,1.5,1.92]:
        cylinder("Plate bolt",(-.14,y,z),.052,.06,dark,"X",6,plate)
    rod("Plate rear strut",(-.75,y,.2),(-.1,y,1.1),.07,dark,plate)
    box("Plate bolted foot",(-.28,y,.14),(.85,.24,.17),brass,.03,plate)

# A true bent horseshoe mesh, silver pole shoes and alternating hanging chain links.
hoist=empty("HOIST | animated carriage",(0,0,0))
mag=empty("MAGNET | swing",(0,0,3.72),hoist)
vs,fs=[],[]
arc=[(math.pi*i/24) for i in range(25)]
# Open U, arc above, straight legs below; extruded through y.
outline=[(.70,-.48),(.70,.08)] + [(.70*math.cos(a),.08+.70*math.sin(a)) for a in arc] + [(-.70,-.48),(-.36,-.48),(-.36,.08)] + [(.36*math.cos(a),.08+.36*math.sin(a)) for a in reversed(arc)] + [(.36,-.48)]
for y in [-.22,.22]:
    vs += [(x,y,z) for x,z in outline]
n=len(outline)
fs=[tuple(reversed(range(n))),tuple(range(n,2*n))]
for i in range(n): j=(i+1)%n; fs.append((i,j,n+j,n+i))
me=bpy.data.meshes.new("Horseshoe extrusion");me.from_pydata(vs,[],fs);me.update()
o=bpy.data.objects.new("Magnet red horseshoe",me);scene.collection.objects.link(o);finish(o,o.name,red,.065,mag)
for x in [-.53,.53]:
    box("Magnet silver pole",(x,0,-.52),(.37,.47,.27),steel,.035,mag)
    for z in [.12,.42]: cylinder("Magnet face bolt",(x,-.25,z),.042,.03,brass,"Y",6,mag)
ring("Magnet top eye",(0,0,.88),.15,.085,.10,dark,parent=mag)
chain=[]
for i in range(16):
    o=ring(f"CHAIN link {i:02}",(0,0,4.77+i*.215),.135,.086,.065,dark,axis="Y" if i%2 else "X",parent=hoist)
    o.scale.z=1.2
    chain.append(o)

# Stacked, recognisable stock. Every object's identity and transform persists on return.
stock=empty("STOCK | unchanged remaining pile")
for i in range(24):
    tier=i//8
    x=random.uniform(-.45,2.55)*(1-tier*.13)
    y=random.uniform(-.8,.9)
    z=.28+tier*.27
    if i%3==0:
        o=ring(f"STOCK gear {i:02}",(x,y,z),random.uniform(.25,.43),.105,.16,steel,12,axis="Z",parent=stock)
        o.rotation_euler=(random.uniform(.2,1.1),random.uniform(-.4,.4),random.uniform(0,6))
    elif i%3==1:
        o=ring(f"STOCK nut {i:02}",(x,y,z),.28,.125,.22,brass,6,axis="Z",parent=stock)
        o.rotation_euler=(random.uniform(.1,.9),random.uniform(-.4,.4),random.uniform(0,6))
    else:
        p=empty(f"STOCK bolt {i:02}",(x,y,z),stock)
        cylinder("Bolt shaft",(0,0,0),.10,.65,steel,"X",parent=p)
        cylinder("Bolt hex head",(-.28,0,0),.20,.15,steel,"X",6,p)
        for q in range(5): ring("Bolt thread",(.02+q*.055,0,0),.119,.095,.024,brass,axis="X",parent=p)
        p.rotation_euler=(0,random.uniform(-.2,.2),random.uniform(0,6))

# Three demonstrative components: identities travel pile -> channel -> ONE combined shot.
parts=[]
for name,loc in [("SHOT gear body",(.45,-.25,1.08)),("SHOT hex collar",(1.0,-.15,.95)),("SHOT bolt core",(1.45,-.35,.9))]:
    parts.append(empty(name,loc))
ring("Shot gear body mesh",(0,0,0),.47,.15,.23,steel,12,axis="X",parent=parts[0])
ring("Shot hex collar mesh",(0,0,0),.34,.13,.30,brass,6,axis="X",parent=parts[1])
ring("Shot collar warm centre",(0,0,0),.345,.13,.16,hot,axis="X",parent=parts[1])
cylinder("Shot bolt core shaft",(0,0,0),.12,.75,steel,"X",parent=parts[2])
cylinder("Shot bolt core hex head",(-.31,0,0),.23,.16,brass,"X",6,parts[2])
for q in range(6): ring("Shot bolt thread",(.02+q*.06,0,0),.15,.11,.025,brass,axis="X",parent=parts[2])

def robot(name,loc,tall=False):
    root=empty(name,loc)
    h=2.5 if tall else 2.05
    box(name+" torso",(0,0,h),(.72,.62,.93 if tall else .78),steel,.095,root)
    box(name+" chest brass panel",(0,-.35,h),(.55,.10,.65),brass,.065,root)
    for x in [-.2,.2]:
        for z in [h-.22,h+.22]: cylinder(name+" chest rivet",(x,-.42,z),.045,.025,dark,"Y",6,root)
    cylinder(name+" neck",(0,0,h+.6),.16,.27,dark,parent=root)
    if tall: cylinder(name+" chimney head",(0,0,h+.96),.25,.64,steel,parent=root)
    else: box(name+" broad head",(0,0,h+.66),(.70,.56,.43),dark,.075,root)
    for x in [-.14,.14]: cylinder(name+" red eye",(x,-.315,h+(.98 if tall else .69)),.069,.08,eye,"Y",parent=root)
    for side in [-1,1]:
        rod(name+" thigh",(side*.25,0,h-.47),(side*.50,0,.66),.10,brass,root)
        rod(name+" shin",(side*.50,0,.66),(side*.63,-.04,.25),.10,steel,root)
        ring(name+" knee gear",(side*.50,-.13,.66),.19,.08,.12,dark,10,parent=root)
        box(name+" foot",(side*.63,-.14,.15),(.48,.69,.24),steel,.065,root)
        cylinder(name+" shoulder",(side*.49,0,h+.15),.20,.2,brass,"X",parent=root)
        rod(name+" upper arm",(side*.55,0,h+.12),(side*.82,0,h-.36),.095,dark,root)
        rod(name+" forearm",(side*.82,0,h-.36),(side*.98,-.15,h-.65),.095,steel,root)
    if tall:
        cylinder(name+" response launcher",(-1.02,-.1,h-.62),.20,.7,steel,"X",parent=root)
        ring(name+" launcher rim",(-1.4,-.1,h-.62),.24,.13,.10,brass,axis="X",parent=root)
    else:
        ring(name+" saw hand",(-1.06,-.19,h-.59),.43,.12,.1,steel,18,parent=root)
        box(name+" shield hand",(1,-.23,h-.66),(.53,.16,.82),steel,.045,root)
    return root

target=robot("ENEMY A | saw guard",(5.45,.65,0))
enemy=robot("ENEMY B | chimney launcher",(8.45,-.55,0),True)

# Animation is a presentation script, not a declared turn sequence or physical solver.
keys(hoist,"location",[(1,(0,0,0)),(18,(.55,0,-1.65)),(30,(.55,0,-1.65)),(42,(.2,-.35,-.15)),(54,(-2.2,-1.3,-.05)),(65,(-4.4,-1.1,-.6)),(78,(0,0,0)),(192,(0,0,0))])
keys(mag,"rotation_euler",[(1,(0,0,-.06)),(18,(0,.08,.10)),(35,(0,-.13,-.09)),(54,(0,.15,.09)),(78,(0,0,0)),(192,(0,0,-.06))])
starts=[tuple(p.location) for p in parts]
for i,p in enumerate(parts):
    ch=(-5.35+i*.93,-1.1,1.61)
    joined=(-4.45+i*.19,-1.1,1.67)
    formed=(-3.12+i*.19,0,2.05)
    keys(p,"location",[(1,starts[i]),(22+i*3,starts[i]),(37+i*3,(.1+i*.28,-.10,1.8-i*.10)),(47,(-1.0+i*.24,-1.1,2.27)),(63+i*2,ch),(73,joined),(83,joined),(91,formed),(96,(-1.9+i*.19,0,2.05)),(111,(5.38+i*.19,.65,2.05)),(192,(5.38+i*.19,.65,2.05))])
    keys(p,"scale",[(1,(1,1,1)),(111,(1,1,1)),(112,(0,0,0)),(192,(0,0,0))])
keys(recoil,"location",[(1,(0,0,0)),(92,(0,0,0)),(96,(-.35,0,0)),(103,(-.07,0,0)),(111,(0,0,0)),(192,(0,0,0))])
keys(target,"rotation_euler",[(1,(0,0,0)),(110,(0,0,0)),(115,(0,.19,-.045)),(126,(0,-.04,.015)),(142,(0,0,0)),(192,(0,0,0))])
keys(enemy,"rotation_euler",[(1,(0,0,0)),(140,(0,0,0)),(147,(0,.065,0)),(158,(0,0,0)),(192,(0,0,0))])
baseplate=tuple(plate.rotation_euler)
keys(plate,"rotation_euler",[(1,baseplate),(160,baseplate),(164,(0,-.085,baseplate[2])),(168,(0,.035,baseplate[2])),(175,baseplate),(192,baseplate)])
reply=empty("ENEMY B | illustrative response projectile")
cylinder("Response projectile mesh",(0,0,0),.10,.40,hot,"X",parent=reply)
keys(reply,"location",[(1,(7,-.65,1.88)),(143,(7,-.65,1.88)),(160,(-2.05,-.8,1.4)),(192,(-2.05,-.8,1.4))])
keys(reply,"scale",[(1,(0,0,0)),(142,(0,0,0)),(143,(1,1,1)),(160,(1,1,1)),(161,(0,0,0)),(192,(0,0,0))])

def burst(name,at,start,color,count=14):
    for i in range(count):
        o=box(f"{name} spark {i:02}",at,(.07,.07,.23),color,.012)
        end=Vector(at)+Vector((random.uniform(-.85,.85),random.uniform(-.7,.5),random.uniform(.15,1.0)))
        keys(o,"location",[(1,at),(start,at),(start+8,tuple(end))])
        keys(o,"scale",[(1,(0,0,0)),(start-1,(0,0,0)),(start,(1,1,1)),(start+7,(.35,.35,.35)),(start+9,(0,0,0)),(192,(0,0,0))])
        o.rotation_euler=(random.random()*3,random.random()*3,random.random()*3)

burst("Muzzle",(-2.25,0,2.05),95,sparkmat,9)
burst("Target hit",(5.35,.22,2.13),112,sparkmat,17)
burst("Plate hit",(-2.12,-.83,1.42),161,sparkmat,19)

# Two camera identities. The action camera has a slow contextual reframe for incoming impact.
prep=camera("CAMERA Preparation | 16B",(1,-26,6.4),(1,0,2.7),ortho=20.8)
action=camera("CAMERA Action | 16C",(-7.5,-18.8,4.6),(1.1,0,2.0),lens=39)
for f,pos,at in [(1,(-7.5,-18.8,4.6),(1.1,0,2.0)),(122,(-7.5,-18.8,4.6),(1.1,0,2.0)),(140,(3,-21,5.2),(1,0,2.1)),(178,(3,-21,5.2),(1,0,2.1))]:
    action.location=pos;orient(action,at)
    action.keyframe_insert(data_path="location",frame=f)
    action.keyframe_insert(data_path="rotation_euler",frame=f)
for f,c,n in [(1,prep,"Prepare / gather"),(80,action,"Fire / resolve"),(181,prep,"Return / same stock")]:
    marker=scene.timeline_markers.new(n,frame=f);marker.camera=c
scene.camera=prep
light("Key | sunset",(-4,-6,10),(1,.67,.38),2100,7)
light("Fill | cool sky",(5,-4,9),(.43,.64,1),1500,10)
light("Rim | warm depth",(4,5,9),(1,.53,.23),2500,7)
light("Forge spill",(-5.2,-1.5,2.5),(1,.22,.025),55,1)
world=scene.world
world.use_nodes=True
world.node_tree.nodes["Background"].inputs[0].default_value=(.38,.46,.57,1)
world.node_tree.nodes["Background"].inputs[1].default_value=.5

# A real source-scene audit, not a gameplay test. Persistent stock and shared object IDs.
mesh_objects=[o for o in scene.objects if o.type=="MESH"]
metrics={"blender":bpy.app.version_string,"seed":16,"frames":192,"fps":24,"resolution":[960,540],
         "objects":len(scene.objects),"mesh_objects":len(mesh_objects),"source_vertices":sum(len(o.data.vertices) for o in mesh_objects),
         "source_polygons":sum(len(o.data.polygons) for o in mesh_objects),"materials":len(bpy.data.materials),
         "camera_objects":[prep.name,action.name],"shot_component_ids":[o.name for o in parts],
         "stock_roots":24,"shared_scene":True,"note":"Not an engine performance measure; bevels increase evaluated polygons."}
stock_start={o.name:list(o.matrix_world) for o in stock.children}
scene.frame_set(1)
transforms1={o.name:tuple(v for row in o.matrix_world for v in row) for o in stock.children}
scene.frame_set(192)
transforms2={o.name:tuple(v for row in o.matrix_world for v in row) for o in stock.children}
assert transforms1==transforms2,"Remaining stock must not change across camera cuts"
metrics["stock_return_transform_match"]=True
assert len(parts)==3 and len([o for o in scene.objects if o.type=="CAMERA"])==2
scene.frame_set(1)
(ROOT/"scene-metrics.json").write_text(json.dumps(metrics,indent=2)+"\n",encoding="utf-8")
bpy.ops.wm.save_as_mainfile(filepath=str(RAW/"camera-motion.blend"))
print("ART_TEST_METRICS "+json.dumps(metrics))
if opt.stills:
    for f,name in [(74,"preparation.png"),(114,"action-impact.png"),(162,"raw/plate-impact.png"),(82,"raw/action-loading.png")]:
        scene.frame_set(f)
        scene.render.filepath=str(ROOT/name)
        bpy.ops.render.render(write_still=True)
if opt.animate:
    (RAW/"frames").mkdir(exist_ok=True)
    scene.render.filepath=str(RAW/"frames"/"frame-")
    bpy.ops.render.render(animation=True)

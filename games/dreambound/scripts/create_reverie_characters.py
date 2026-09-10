"""New botanical-machined cyborg kit, authored in Blender; no legacy geometry.

Centimeters, shield faces +X. Six independently animated petals share the hub
origin; their gameplay docking transforms remain unchanged. Full source lives
here, exported assets and one review scene live in art/reverie/characters.
"""
import bpy
import bmesh
import math
import json
import random
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'art/reverie/characters'
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
SCENE = bpy.context.scene
SCENE.unit_settings.system = 'METRIC'
SCENE.unit_settings.scale_length = .01
bpy.context.preferences.filepaths.save_version = 0
RNG = random.Random(912)
PALETTE = {
 'M_RV_Ceramic':([.37,.46,.40],.28,.07),
 'M_RV_Enamel':([.07,.19,.16],.23,.38),
 'M_RV_Bronze':([.38,.235,.093],.32,.78),
 'M_RV_DarkMetal':([.033,.045,.046],.35,.72),
 'M_RV_Cloth':([.061,.073,.065],.84,0),
 'M_RV_Core':([.30,.91,.62],.18,.28),
 'M_RV_Crystal':([.12,.58,.39],.19,.12),
 'M_RV_Water':([.04,.18,.17],.14,.22),
 'M_RV_Waterfall':([.24,.50,.42],.1,.1),
 'M_RV_Sky':([.17,.28,.34],1,0),
}
MATS = {}
for name,(color,rough,metal) in PALETTE.items():
 m=bpy.data.materials.new(name);m.use_nodes=True;m.diffuse_color=(*color,1)
 bs=m.node_tree.nodes.get('Principled BSDF')
 bs.inputs['Base Color'].default_value=(*color,1)
 bs.inputs['Metallic'].default_value=metal;bs.inputs['Roughness'].default_value=rough
 if name=='M_RV_Core':
  bs.inputs['Emission Color'].default_value=(*color,1);bs.inputs['Emission Strength'].default_value=.7
 MATS[name]=m
PARTS=[];ASSETS={}

def active(o):
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o

def finish(o,mat,bevel=0,smooth=False):
 active(o);bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
 if bevel:
  mod=o.modifiers.new('Forged edge radius','BEVEL');mod.width=bevel;mod.segments=3
  bpy.ops.object.modifier_apply(modifier=mod.name)
 bm=bmesh.new();bm.from_mesh(o.data)
 bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.00001)
 bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(o.data);bm.free()
 o.data.materials.clear();o.data.materials.append(MATS[mat])
 uv=o.data.uv_layers.new(name='CraftUV') if not o.data.uv_layers else o.data.uv_layers[0]
 col=o.data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
 for face in o.data.polygons:
  face.use_smooth=smooth
  axis=max(range(3),key=lambda i:abs(face.normal[i]));axes=[i for i in range(3) if i!=axis]
  for li in face.loop_indices:
   p=o.matrix_world@o.data.vertices[o.data.loops[li].vertex_index].co
   uv.data[li].uv=(p[axes[0]]/100,p[axes[1]]/100)
   col.data[li].color=(RNG.uniform(.85,.98),.12 if len(face.vertices)<4 else .025,.025,1)
 PARTS.append(o);return o

def mesh(name,verts,faces,mat,bevel=0,smooth=False):
 d=bpy.data.meshes.new(name);d.from_pydata(verts,[],faces);d.update()
 o=bpy.data.objects.new(name,d);SCENE.collection.objects.link(o)
 return finish(o,mat,bevel,smooth)

def box(name,loc,dims,mat,bevel=.12):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=name;o.scale=dims
 return finish(o,mat,bevel)

def ellipsoid(name,loc,dims,mat):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=24,ring_count=12,radius=1,location=loc)
 o=bpy.context.object;o.name=name;o.scale=Vector(dims)*.5;return finish(o,mat,0,True)

def tube(name,points,radius,mat,sides=10):
 # Parallel circular cross sections follow the curve; useful for copper ribs.
 verts=[]
 for i,p in enumerate(points):
  p=Vector(p);t=Vector(points[min(i+1,len(points)-1)])-Vector(points[max(0,i-1)])
  t.normalize();ref=Vector((1,0,0)) if abs(t.x)<.85 else Vector((0,1,0))
  u=t.cross(ref).normalized();v=t.cross(u).normalized()
  rr=radius*(.72+.28*math.sin(math.pi*i/max(1,len(points)-1)))
  for j in range(sides):
   a=j*math.tau/sides;verts.append(tuple(p+rr*(u*math.cos(a)+v*math.sin(a))))
 faces=[]
 for i in range(len(points)-1):
  for j in range(sides):
   a=i*sides+j;b=i*sides+(j+1)%sides;faces.append((a,b,b+sides,a+sides))
 faces.extend([tuple(range(sides-1,-1,-1)),tuple((len(points)-1)*sides+j for j in range(sides))])
 return mesh(name,verts,faces,mat,smooth=True)

def armor(name,rows,mat):
 # Hollow-looking but closed sculpted volumes: z, xcenter, depth, width.
 verts=[];N=24
 for z,x,depth,width in rows:
  for j in range(N):
   a=j*math.tau/N
   # Rounded keel towards front instead of a rectangular primitive.
   verts.append((x+depth*.5*math.cos(a),width*.5*math.sin(a),z))
 faces=[]
 for i in range(len(rows)-1):
  for j in range(N):
   a=i*N+j;b=i*N+(j+1)%N;faces.append((a,b,b+N,a+N))
 faces.extend([tuple(range(N-1,-1,-1)),tuple((len(rows)-1)*N+j for j in range(N))])
 return mesh(name,verts,faces,mat,bevel=.14,smooth=True)

def petal(name,rows,mat):
 # z, half-width, raised centerX; the shield plane remains YZ.
 verts=[];N=16
 for side in (0,1):
  for z,w,x in rows:
   for j in range(N+1):
    t=j/N;verts.append((x+(.75*math.sin(t*math.pi) if side==0 else -1.5),w*(2*t-1),z))
 layer=len(rows)*(N+1);faces=[]
 for side in range(2):
  for i in range(len(rows)-1):
   for j in range(N):
    a=side*layer+i*(N+1)+j;faces.append((a,a+1,a+N+2,a+N+1))
 for i in range(len(rows)-1):
  for j in (0,N):
   a=i*(N+1)+j;faces.append((a,a+N+1,a+N+1+layer,a+layer))
 for i in (0,len(rows)-1):
  for j in range(N):
   a=i*(N+1)+j;faces.append((a,a+1,a+1+layer,a+layer))
 return mesh(name,verts,faces,mat,bevel=.10)

def crystal(name,loc,r,h,mat='M_RV_Core',axis='Z'):
 verts=[];n=7
 for z,rr in ((0,r*.72),(h*.16,r),(h*.70,r*.76),(h,r*.025)):
  for j in range(n):
   a=j*math.tau/n;x=rr*math.cos(a);y=rr*math.sin(a)
   p=(x,y,z) if axis=='Z' else (z-h*.5,x,y)
   verts.append(tuple(Vector(p)+Vector(loc)))
 faces=[]
 for i in range(3):
  for j in range(n):
   a=i*n+j;b=i*n+(j+1)%n;faces.append((a,b,b+n,a+n))
 faces.extend([tuple(range(n-1,-1,-1)),tuple(3*n+j for j in range(n))])
 return mesh(name,verts,faces,mat,bevel=.025)

def export(name,collision=None):
 global PARTS
 bpy.ops.object.select_all(action='DESELECT')
 for o in PARTS:o.select_set(True)
 bpy.context.view_layer.objects.active=PARTS[0];bpy.ops.object.join();o=bpy.context.object;o.name=name
 SCENE.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
 bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
 active(o);bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
 o.data.uv_layers.new(name='LightmapUV');o.data.uv_layers.active_index=1
 bpy.ops.uv.smart_project(angle_limit=1.1,island_margin=.025);bpy.ops.object.mode_set(mode='OBJECT');o.data.uv_layers.active_index=0
 o.data.calc_loop_triangles()
 points=[v.co for v in o.data.vertices]
 lo=[min(p[i] for p in points) for i in range(3)];hi=[max(p[i] for p in points) for i in range(3)]
 colliders=[]
 if collision:
  for i,(loc,dims) in enumerate(collision):
   bpy.ops.mesh.primitive_cube_add(size=1,location=loc);c=bpy.context.object;c.name=f'UCX_{name}_{i:02d}';c.scale=dims
   active(c);bpy.ops.object.transform_apply(location=False,rotation=True,scale=True);colliders.append(c)
 active(o)
 for c in colliders:c.select_set(True)
 bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},
  apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='X',axis_up='Z',
  use_space_transform=True,bake_space_transform=True,mesh_smooth_type='FACE',bake_anim=False,add_leaf_bones=False,path_mode='STRIP')
 ASSETS[name]={'dimensions_cm':[hi[i]-lo[i] for i in range(3)],'bounds_min_cm':lo,'bounds_max_cm':hi,
  'materials':[m.name for m in o.data.materials],'collision_hulls':len(colliders),'triangles':len(o.data.loop_triangles),'uv_channels':['CraftUV','LightmapUV']}
 for c in colliders:bpy.data.objects.remove(c,do_unlink=True)
 PARTS=[];o.hide_set(True);return o

# Six botanical shield petals: exposed bronze skeleton and inset jade enamel.
petal('Cast ribbed petal',[(12,2,2),(18,8,3.3),(27,13.3,4.6),(35,14.5,3.0),(41,1.2,.5)],'M_RV_Bronze')
petal('Inset celadon armor',[(16,1.5,4),(21,7,5.1),(29,10.4,5.4),(34,10.6,4),(38,1,2)],'M_RV_Ceramic')
petal('Recessed enamel lance',[(18,.4,5.6),(27,3.4,6),(35,3,4.8),(39,.25,2.6)],'M_RV_Enamel')
for sign in (-1,1):
 tube('Swept copper perimeter',[(2,sign*2,13),(4.5,sign*9,21),(5,sign*14,31),(3,sign*13.8,35),(.7,sign*.6,41)],.44,'M_RV_Bronze')
 for z in (22,27,32):
  tube('Leaf vein',[(6,0,z-3),(6.1,sign*4,z),(4.9,sign*(8+(z-22)*.25),z+1)],.22,'M_RV_Bronze',8)
ellipsoid('Root hinge',(0,0,15),(7,9,8),'M_RV_DarkMetal')
export('SM_RV_ShieldSegment')
for sign in (-1,1):tube('Crystal vein',[(6.3,sign*.7,20),(6.5,sign*1,26),(5.3,sign*.65,33),(3.0,0,38)],.30,'M_RV_Core',8)
export('SM_RV_ShieldSegmentGlow')
for j in range(6):
 a=j*math.tau/6
 tube('Hub petiole',[(0,4*math.sin(a),4*math.cos(a)),(1,9*math.sin(a),9*math.cos(a)),(0,15*math.sin(a),15*math.cos(a))],1.25,'M_RV_Bronze')
ellipsoid('Hub chassis',(-1,0,0),(7,18,18),'M_RV_DarkMetal')
crystal('Heart socket',(2,0,0),6,7,'M_RV_Enamel','X');export('SM_RV_ShieldHub')
crystal('Heart lens',(0,0,0),6.5,6,'M_RV_Core','X');export('SM_RV_Core')

# Forearm follows existing world-space docking envelope but all surfaces are new.
armor('Sleeved wrist',[(-13,-40,13,13),(-7,-37,17,18),(0,-29,18,20),(7,-18,17,19),(12,-9,14,17)],'M_RV_DarkMetal')
for i in range(6):
 x=-46+i*7
 ellipsoid('Overlapping shell',(x,0,1+i*.13),(12,18-i*.3,16-i*.3),'M_RV_Ceramic')
 for sign in (-1,1):tube('Servo tendon',[(x-4,sign*7,-6),(x,sign*10,-1),(x+4,sign*7,4)],.55,'M_RV_Bronze')
ellipsoid('Palm',(-5,0,1),(17,15,11),'M_RV_DarkMetal')
for j in range(4):
 y=-5+j*3.2
 tube('Articulated digit',[(-2,y,1),(4,y,0),(7,y,-4),(6,y,-7)],1.25,'M_RV_Ceramic')
export('SM_RV_Forearm')
armor('Flexible sleeve',[(-50,0,17,19),(-20,0,18,20),(20,0,20,22),(50,0,19,21)],'M_RV_Cloth')
for z in range(-44,48,8):ellipsoid('Sleeve compression rib',(0,0,z),(20,22,3.2),'M_RV_DarkMetal')
export('SM_RV_Sleeve')

# Guardian: sculpted tapered armour over jointed copper anatomy.
armor('Torso undercarriage',[(-8,-2,25,35),(5,-3,31,43),(35,-3,36,59),(62,-5,31,53),(70,-5,23,35)],'M_RV_DarkMetal')
for sign in (-1,1):
 for j in range(5):
  z=5+j*10
  tube('Exposed rib',[(12,sign*4,z-2),(20,sign*14,z+2),(10,sign*(23+j*1.2),z+8),(-6,sign*(22+j),z+9)],2.0,'M_RV_Bronze')
 o=armor('Sculpted chest leaf',[(19,15,7,10),(32,17,12,22),(49,12,17,28),(64,2,12,20),(69,-2,4,4)],'M_RV_Ceramic')
 o.location.y=sign*17;o.rotation_euler.x=sign*.16
tube('Front sternum',[(20,0,-5),(24,0,20),(25,0,43),(15,0,63)],2.4,'M_RV_Bronze')
export('SM_RV_GuardianBody')
armor('Carved faceplate',[(0,-2,18,16),(9,1,23,22),(26,0,26,29),(37,-4,21,22),(44,-5,10,7)],'M_RV_Ceramic')
for sign in (-1,1):
 tube('Mask filigree',[(10,0,5),(15,sign*8,16),(11,sign*12,28),(-1,sign*5,42)],.8,'M_RV_Bronze')
 tube('Recessed eye slit',[(14,sign*2,25),(14.2,sign*7,24),(12.7,sign*11,27)],.65,'M_RV_Core')
tube('Raised sagittal crest',[(13,0,10),(16,0,27),(3,0,43),(-10,0,48)],1.2,'M_RV_Bronze')
export('SM_RV_GuardianHead')
armor('Laminated arm',[(-82,0,10,12),(-68,0,17,19),(-45,-3,19,20),(-33,-4,11,13),(-20,-5,18,22),(5,-3,24,30),(14,-4,13,16)],'M_RV_DarkMetal')
for z,width in ((-63,18),(-49,21),(-12,26),(4,32)):
 ellipsoid('Separate armor lamella',(3,0,z),(24,width,27),'M_RV_Ceramic')
for sign in (-1,1):tube('Braided elbow conduit',[(1,sign*10,9),(10,sign*15,-13),(7,sign*10,-33),(9,sign*10,-58),(1,sign*5,-78)],1.0,'M_RV_Bronze')
for j in range(3):tube('Talon digit',[(3,-5+j*5,-72),(9,-5+j*5,-81),(10,-5+j*5,-87)],1.5,'M_RV_Bronze')
export('SM_RV_GuardianArm')
armor('Walking strut',[(-94,7,27,23),(-84,1,20,18),(-65,-3,18,18),(-48,-4,14,16),(-30,-6,22,27),(-5,-3,28,30),(7,-3,16,21)],'M_RV_DarkMetal')
armor('Greave',[(-80,8,9,17),(-69,9,12,23),(-57,10,13,24),(-43,7,11,17)],'M_RV_Ceramic')
armor('Thigh armour',[(-37,5,10,19),(-22,9,12,30),(-7,9,12,32),(4,4,8,20)],'M_RV_Ceramic')
for sign in (-1,1):tube('Leg hydraulic cable',[(0,sign*12,1),(5,sign*16,-20),(0,sign*10,-44),(2,sign*10,-75)],1.2,'M_RV_Bronze')
ellipsoid('Split foot',(13,0,-91),(38,24,12),'M_RV_Bronze');export('SM_RV_GuardianLeg')
crystal('Crystal shard',(0,0,0),13,52);export('SM_RV_Crystal')
crystal('Ward heart',(0,0,9),26,68)
for j in range(6):
 a=j*math.tau/6;tube('Ward cradle',[(30*math.cos(a),30*math.sin(a),0),(24*math.cos(a),24*math.sin(a),22),(11*math.cos(a),11*math.sin(a),39)],1.5,'M_RV_Bronze')
export('SM_RV_WardCrystal')

# New line and spark volumes retain the 100cm effect-scaling contract.
mesh('Tapered beam',[(-50,0,0),(-32,-34,-34),(-32,34,-34),(-32,34,34),(-32,-34,34),(32,-50,-50),(32,50,-50),(32,50,50),(32,-50,50),(50,0,0)],
 [(0,2,1),(0,3,2),(0,4,3),(0,1,4),(1,2,6,5),(2,3,7,6),(3,4,8,7),(4,1,5,8),(5,6,9),(6,7,9),(7,8,9),(8,5,9)],'M_RV_Core')
export('SM_RV_Beam')
for axis in range(3):
 pts=[Vector((0,0,0)) for i in range(3)];pts[0][axis]=-50;pts[2][axis]=50
 tube('Prismatic flash',pts,11,'M_RV_Core',6)
export('SM_RV_Spark')
for y in range(-210,211,42):
 tube('Sealed gate rib',[(0,y,0),(0,y+12,150),(0,y-12,320),(0,y,500)],6,'M_RV_Bronze')
 for z in (95,235,375):crystal('Gate floating seal',(0,y,z),9,48)
export('SM_RV_Gate',collision=[((0,0,250),(20,440,500))])
mesh('Water plane',[(-50,-50,0),(50,-50,0),(50,50,0),(-50,50,0)],[(0,1,2,3)],'M_RV_Water');export('SM_RV_WaterPlane')
verts=[(0,0,0)]+[(50*math.cos(j*math.tau/96),50*math.sin(j*math.tau/96),0) for j in range(96)]
mesh('Water disc',verts,[(0,j+1,(j+1)%96+1) for j in range(96)],'M_RV_Water');export('SM_RV_WaterDisc')
verts=[]
for k in range(13):
 t=k/12
 for x in (-50,50):verts.append((x,18*(1-t)**2,100*t))
mesh('Falling ribbon',verts,[(k*2,k*2+1,k*2+3,k*2+2) for k in range(12)],'M_RV_Waterfall');export('SM_RV_Waterfall')
ellipsoid('Sky dome',(0,0,0),(100,100,100),'M_RV_Sky');export('SM_RV_SkyDome')
(OUT/'asset-metadata.json').write_text(json.dumps(ASSETS,indent=2)+'\n')
(OUT/'materials.json').write_text(json.dumps({n:{'color':c,'roughness':r,'metallic':m} for n,(c,r,m) in PALETTE.items()},indent=2)+'\n')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Reverie_Character_Kit.blend'),compress=True)
print('REVERIE_CHARACTERS_READY '+str(len(ASSETS)))

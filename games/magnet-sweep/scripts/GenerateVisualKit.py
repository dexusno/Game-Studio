"""Original, reusable machined salvage and workshop meshes. Blender background script."""
from pathlib import Path
import bpy, math, json, csv
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'assets/visual-overhaul/kit'
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.01
palette={'Steel':(.34,.43,.46,.85,.28),'Copper':(.65,.24,.085,.78,.30),
         'Brass':(.65,.42,.12,.82,.29),'Dark':(.025,.041,.047,.50,.46),
         'Magnet':(.018,.20,.22,.48,.34),'Glow':(.025,.72,.68,.3,.20),
         'Hot':(.95,.16,.018,.2,.28),'Ivory':(.63,.65,.53,.25,.40),
         'Alloy':(.12,.26,.38,.74,.3),'Hazard':(.50,.06,.025,.46,.35),'Tray':(.045,.065,.07,.5,.5),'RimGlow':(.025,.25,.21,.1,.4)}
materials={}
for name,(r,g,b,metal,rough) in palette.items():
 m=bpy.data.materials.new(name);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF')
 p.inputs['Base Color'].default_value=(r,g,b,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough
 if name in ['Glow','Hot']:p.inputs['Emission Color'].default_value=(r,g,b,1);p.inputs['Emission Strength'].default_value=2
 materials[name]=m
parts=[];report=[]
def finish(o,mat,bevel=.4):
 bpy.context.view_layer.objects.active=o
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 o.data.materials.append(materials[mat])
 if bevel:
  mod=o.modifiers.new('Machined edge radii','BEVEL');mod.width=bevel;mod.segments=3
  bpy.ops.object.modifier_apply(modifier=mod.name)
 for f in o.data.polygons:f.use_smooth=True
 mod=o.modifiers.new('Weighted corner normals','WEIGHTED_NORMAL');mod.keep_sharp=True
 bpy.ops.object.modifier_apply(modifier=mod.name)
 parts.append(o);return o
def box(p,d,mat='Steel',b=.5,rz=0):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.dimensions=d;o.rotation_euler.z=rz
 return finish(o,mat,b)
def cyl(p,r,h,mat='Steel',n=48,b=.4):
 bpy.ops.mesh.primitive_cylinder_add(vertices=n,radius=r,depth=h,location=p);return finish(bpy.context.object,mat,b)
def ring(p,outer,inner,h,mat='Steel',n=64):
 verts=[]
 for z in [-h/2,h/2]:
  for radius in [outer,inner]:
   verts += [(p[0]+radius*math.cos(i*math.tau/n),p[1]+radius*math.sin(i*math.tau/n),p[2]+z) for i in range(n)]
 faces=[]
 for i in range(n):
  k=(i+1)%n
  faces += [(i,k,2*n+k,2*n+i),(n+k,n+i,3*n+i,3*n+k),(2*n+i,2*n+k,3*n+k,3*n+i),(k,i,n+i,n+k)]
 mesh=bpy.data.meshes.new('hollow machined ring');mesh.from_pydata(verts,[],faces);mesh.update()
 o=bpy.data.objects.new('Hollow collar',mesh);scene.collection.objects.link(o);bpy.context.view_layer.objects.active=o;o.select_set(True)
 return finish(o,mat,.35)
def bolt(p,r=1.5):
 cyl(p,r,.95,'Steel',6,.13);box((p[0],p[1],p[2]+.5),(r*1.2,.3,.18),'Dark',.06)
def radial_bolts(radius,z,count=6):
 for i in range(count):
  a=i*math.tau/count;bolt((radius*math.cos(a),radius*math.sin(a),z))
def save(name):
 global parts
 bpy.ops.object.select_all(action='DESELECT')
 for o in parts:o.select_set(True)
 bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name
 bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
 bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.015);bpy.ops.object.mode_set(mode='OBJECT')
 bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
 o.data.calc_loop_triangles()
 report.append({'name':name,'triangles':len(o.data.loop_triangles),'dimensions_cm':list(o.dimensions),'material_slots':[s.name for s in o.data.materials]})
 bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},add_leaf_bones=False,apply_unit_scale=True,axis_forward='-Y',axis_up='Z',bake_anim=False)
 o.hide_render=True;o.hide_set(True);parts=[]

# Readable salvage silhouettes, with physically modelled holes, teeth, laminations and fasteners.
ring((0,0,0),18,7,6)
for i in range(18):
 a=i*math.tau/18;box((19*math.cos(a),19*math.sin(a),0),(6,5,6),'Steel',.7,a)
ring((0,0,4),10,6,3,'Brass');radial_bolts(13,4,6);save('SM_VGear')
ring((0,0,0),20,10,8,'Steel');ring((0,0,4.5),18,11,1.6,'Dark')
for i in range(12):
 a=i*math.tau/12;cyl((14.6*math.cos(a),14.6*math.sin(a),5.5),2,2.2,'Brass',16,.3)
ring((0,0,-5),22,9,2,'Steel');save('SM_VBearing')
box((0,0,-3),(35,34,4),'Steel',1.8);box((0,0,0),(26,25,4),'Dark',1)
for x in [-10,-5,0,5,10]:box((x,0,6),(2.2,29,14),'Alloy',.35)
for x in [-14,14]:
 for y in [-13,13]:bolt((x,y,0))
save('SM_VHeatSink')
ring((0,0,-5),20,8,3,'Dark');ring((0,0,6),20,8,3,'Steel')
for z in [-3,-1,1,3,5]:ring((0,0,z),17,9,1.4,'Copper')
for a in [0,math.pi]:box((18*math.cos(a),0,0),(5,11,16),'Magnet',1)
radial_bolts(14,8,4);save('SM_VCoil')
cyl((0,0,0),17,30,'Dark');cyl((0,0,0),13,32,'Glow')
for z in [-18,-13,13,18]:ring((0,0,z),21,12,3,'Brass')
for i in range(8):
 a=i*math.tau/8;box((17*math.cos(a),17*math.sin(a),0),(4,5,32),'Steel',.7,a)
cyl((0,0,20),12,4,'Magnet');radial_bolts(16,20,6);save('SM_VCore')
box((0,0,-4),(74,65,8),'Dark',3)
for x in [-30,30]:box((x,0,5),(8,65,14),'Steel',1.2)
for y in [-27,27]:box((0,y,5),(62,7,14),'Magnet',1)
for x in [-22,0,22]:
 cyl((x,0,10),10,22,'Dark');ring((x,0,21),11,5,3,'Brass')
 for z in [3,7,11,15]:ring((x,0,z),10.5,9,1.8,'Copper')
for x in [-30,30]:
 for y in [-27,27]:bolt((x,y,13),2.2)
box((0,-28,16),(28,4,4),'Glow',.7);save('SM_VFrame')
# Recessed work surface and chassis: dimensions match the original physics tray.
box((0,0,-15),(1050,670,34),'Dark',14)
for x in [-390,-130,130,390]:
 for y in [-210,0,210]:box((x,y,4),(255,205,8),'Tray',4)
for x in [-518,518]:
 box((x,0,15),(26,670,25),'Magnet',6)
 for y in range(-290,300,58):bolt((x,y,30),4)
for y in [-328,328]:
 box((0,y,15),(1020,28,25),'Magnet',6)
 box((0,y,29),(760,3,2),'RimGlow',.5)
for x in [-476,476]:
 for y in [-276,276]:
  box((x,y,13),(64,58,8),'Dark',5);bolt((x,y,20),5)
save('SM_VDeck')
# Furnace housing with concentric lip, ventilation and hot inner basin.
box((0,0,-22),(216,250,44),'Dark',12)
ring((0,0,0),98,76,50,'Dark',96)
for z in [-12,-2,8,18,28]:ring((0,0,z),105,84,6,'Copper',96)
ring((0,0,34),108,76,8,'Brass',96);ring((0,0,29),78,68,15,'Dark',96)
cyl((0,0,20),70,3,'Hot',64,1)
for i in range(12):
 a=i*math.tau/12
 box((98*math.cos(a),98*math.sin(a),12),(14,16,68),'Dark',2,a)
 bolt((97*math.cos(a),97*math.sin(a),48),4)
for y in [-110,110]:
 for x in range(-70,71,20):box((x,y,0),(8,10,22),'Brass',1)
save('SM_VFurnace')
box((0,0,-5),(110,100,18),'Dark',8);box((0,0,5),(95,84,10),'Magnet',5)
ring((0,0,12),39,30,8,'Brass');ring((0,0,17),32,29,3,'Glow')
for x in [-40,40]:
 for y in [-34,34]:bolt((x,y,13),3)
save('SM_VDock')
box((0,0,0),(100,80,16),'Hazard',4);box((0,0,10),(84,66,8),'Dark',3)
for x in [-33,33]:
 cyl((x,0,20),9,18,'Steel');ring((x,0,30),12,4,4,'Brass')
for y in [-30,30]:
 for x in [-24,0,24]:box((x,y,16),(13,5,6),'Brass',1,math.radians(20))
save('SM_VPress')
# Flush receiving fixtures, fastened barriers and a pinned arm replace bare blocks.
box((0,0,-3),(100,100,12),'Dark',9);box((0,0,4),(85,85,4),'Tray',5)
for y in [-28,-14,0,14,28]:box((0,y,7),(67,6,3),'Steel',1)
for x in [-41,41]:
 for y in [-41,41]:
  box((x,y,6),(15,15,5),'Brass',2);bolt((x,y,9),2.6)
save('SM_VPad')
box((0,0,0),(120,30,30),'Dark',3);box((0,0,14),(115,26,5),'Steel',2)
for x in [-48,48]:
 box((x,0,0),(14,35,33),'Magnet',2);bolt((x,0,19),3)
for x in [-26,-13,0,13,26]:box((x,-15,0),(6,2,17),'Brass',.7)
save('SM_VBarrier')
box((0,0,0),(90,15,12),'Brass',3);box((0,0,7),(59,7,3),'Dark',1)
for x in [-38,38]:
 cyl((x,0,3),9,15,'Steel',32,1);cyl((x,0,11),5,2,'Dark',32,.4);bolt((x,0,13),2.3)
save('SM_VArm')
for obj in bpy.context.scene.objects:obj.hide_set(False);obj.hide_render=False
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'machined-salvage-kit.blend'))
(OUT/'mesh-report.json').write_text(json.dumps({'generator':'Blender '+bpy.app.version_string,'source':'scripts/GenerateVisualKit.py','units':'centimeters; runtime Place fits exact display envelope','meshes':report},indent=2),encoding='utf-8')
with (OUT/'manifest-rows.csv').open('w',newline='',encoding='utf-8') as f:
 w=csv.writer(f);w.writerow(['asset_id','path','source_url','creator','license','proof_path','modifications','approval_status'])
 for r in report:w.writerow([r['name'],'assets/visual-overhaul/kit/'+r['name']+'.fbx','original: scripts/GenerateVisualKit.py','Game Studio / Codex / Blender','Original project asset; project distribution license unset','assets/visual-overhaul/kit/PROVENANCE.md','Beveled geometry; weighted normals; UV unwrap; multi-material FBX','original-project'])
print('VISUAL_KIT_READY',len(report),sum(r['triangles'] for r in report))

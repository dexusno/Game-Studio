"""Actual geometry review, including an eye-height courtyard camera. No exports."""
preview_objects=[]
def instance(name,loc=(0,0,0),scale=1,yaw=0,rotation=None):
    obj=ASSETS[name]['object'].copy();scene.collection.objects.link(obj)
    obj.hide_render=False;obj.hide_set(False);obj.location=loc;obj.scale=(scale,scale,scale)
    obj.rotation_euler=rotation if rotation is not None else (0,0,math.radians(yaw))
    preview_objects.append(obj);return obj

def clear_preview():
    for obj in preview_objects:bpy.data.objects.remove(obj,do_unlink=True)
    preview_objects.clear()

def guardian(loc=(0,0,0),scale=.9,yaw=-90):
    from mathutils import Matrix
    matrix=Matrix.Rotation(math.radians(yaw),4,'Z')
    for name,p in [('SM_GuardianBody',(0,0,94)),('SM_GuardianHead',(0,0,156)),
                   ('SM_GuardianArm',(0,-36,146)),('SM_GuardianArm',(0,36,146)),
                   ('SM_GuardianLeg',(0,-18,94)),('SM_GuardianLeg',(0,18,94))]:
        instance(name,Vector(loc)+matrix@Vector(p)*scale,scale,yaw)

scene.render.engine='CYCLES';scene.cycles.samples=48
scene.render.threads_mode='FIXED';scene.render.threads=6
scene.render.image_settings.file_format='PNG';scene.render.resolution_percentage=100
scene.world.color=(.075,.095,.11);scene.view_settings.view_transform='AgX'
world=scene.world;world.use_nodes=True
world.node_tree.nodes.get('Background').inputs[0].default_value=(.22,.27,.30,1)
world.node_tree.nodes.get('Background').inputs[1].default_value=.38
bpy.ops.object.camera_add();camera=bpy.context.object;scene.camera=camera
camera.data.clip_end=30000
for name,loc,power,size,color,target in [
    ('Warm daylight',(-450,-80,1150),3800000,380,(1,.86,.64),(0,180,100)),
    ('Cool courtyard fill',(700,-700,550),2500000,650,(.62,.77,1),(0,200,170)),
    ('Canopy rim',(30,950,880),4800000,230,(1,.94,.74),(0,340,260)),
]:
    data=bpy.data.lights.new(name,'AREA');data.energy=power;data.shape='DISK';data.size=size;data.color=color
    obj=bpy.data.objects.new(name,data);scene.collection.objects.link(obj);obj.location=loc
    obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()

def render(name,eye,target,resolution=(1600,1000),ortho=None):
    camera.location=eye;camera.rotation_euler=(Vector(target)-camera.location).to_track_quat('-Z','Y').to_euler()
    camera.data.type='ORTHO' if ortho else 'PERSP'
    if ortho:camera.data.ortho_scale=ortho
    else:camera.data.sensor_width=36;camera.data.lens=36/(2*math.tan(math.radians(94)/2))
    scene.render.resolution_x,scene.render.resolution_y=resolution
    scene.render.filepath=str(PREVIEW/(name+'.png'))
    if '--skip-renders' not in sys.argv:bpy.ops.render.render(write_still=True)

# Hero silhouette and engineered back must both survive a close camera.
instance('SM_ShieldPlate');instance('SM_Core',(-6,0,0))
render('shield-front',(145,-152,92),(0,0,0),(1200,1100),104)
instance('SM_Forearm',(-12,0,-12))
render('shield-back',(-152,-145,81),(-8,0,-2),(1200,1100),115)
clear_preview();guardian((0,0,0),1,0)
render('sentinel',(265,-335,170),(0,0,103),(1000,1200),234)
clear_preview()

# One coherent composition: enclosing arch, rising stair, bell-tree and a threat.
for x in range(-600,601,200):
    for y in range(-600,801,200):
        name='SM_StoneTile_B' if ((x//200)*3+y//200)%3==0 else 'SM_StoneTile'
        instance(name,(x,y,-24),1,90*((x//200-y//200)%4))
instance('SM_BellTree',(0,495,0));instance('SM_Canopy',(0,495,620))
instance('SM_Bell',(-8,383,286))
for x in [-480,-80,320]:instance('SM_Wall',(x,880,0))
for x in [-620,615]:instance('SM_Wall',(x,455,0),1,90)
instance('SM_Arch',(0,-445,0),1.40)
instance('SM_Stair',(-398,-40,0));instance('SM_Stair',(-398,152,96))
for x in [-400,-200]:
    for y in [430,630]:instance('SM_StoneTile',(x,y,168))
instance('SM_Arch',(-400,790,192),1.05)
instance('SM_Pillar',(-582,385,0));instance('SM_PillarBroken',(505,326,0),1,17)
instance('SM_Ivy',(-270,-458,423),1.35)
instance('SM_Ivy',(287,-458,362),1.1)
instance('SM_Ivy',(-501,813,586),1.2)
for x,y,s in [(-181,312,1.1),(154,238,1.2),(313,475,1.05),(-475,-95,1.0),(428,271,1.15),(-541,676,1.3)]:
    instance('SM_Fern',(x,y,0),s,random.randrange(360))
for x,y,s in [(-218,288,.9),(365,317,.9),(-506,65,.75),(-515,514,.7)]:instance('SM_Rubble',(x,y,0),s,random.randrange(360))
for x,y in [(-490,-265),(290,314),(403,179),(-165,255),(52,247)]:instance('SM_Grass',(x,y,0),1.5,random.randrange(360))
instance('SM_Crate',(461,538,0),.9,8)
guardian((26,95,0),.9,-90)

# Camera is at a real 166cm eye height. Hand/shield transforms follow the gameplay basis.
from mathutils import Euler
shield_rot=Euler((math.radians(-12),math.radians(8),math.radians(124)))
shield_pos=Vector((-52,-635,128));shield_matrix=shield_rot.to_matrix()
instance('SM_ShieldPlate',shield_pos,1,rotation=shield_rot)
instance('SM_Core',shield_pos+shield_matrix@Vector((-6,0,0)),1,rotation=shield_rot)
instance('SM_Forearm',shield_pos+shield_matrix@Vector((-12,0,-12)),1,rotation=shield_rot)
render('bellroot-first-person',(0,-720,166),(0,380,238),(1800,1125))
clear_preview()

# Keep the editable source self-contained with relative texture paths.
for img in bpy.data.images:
    if img.name.startswith('T_'):img.filepath='//generated/'+img.name+'.png'
bpy.data.orphans_purge(do_local_ids=True,do_linked_ids=True,do_recursive=True)
bpy.ops.wm.save_as_mainfile(filepath=str(ART/'Dreambound_Kit.blend'),compress=True)

with (ART/'manifest-rows.csv').open('w',newline='',encoding='utf-8') as handle:
    writer=csv.writer(handle);writer.writerow(['asset_id','path','source_url','creator','license','proof_path','modifications','approval_status'])
    for name in ASSETS:
        writer.writerow([name.lower(),'art/generated/'+name+'.fbx','','Game Studio with OpenAI Codex','Original project-authored asset; no third-party inputs','art/source/bellroot.py','Authored Blender profiles and geometry; local edge/recess vertex masks; purposeful surface UVs; FBX export','original'])
    for name in ['T_PaintedSurface','T_SculptedNormal','T_ShieldCraft','T_ShieldCraftNormal','T_MasonryCraft','T_MasonryCraftNormal','T_BarkCraft','T_BarkCraftNormal','T_MetalCraft','T_MetalCraftNormal']:
        writer.writerow([name.lower(),'art/generated/'+name+'.png','','Game Studio with OpenAI Codex','Original project-authored texture; no third-party inputs','scripts/create_art.py','Original authored shield-radius wear and chisel/mineral masks or supporting spectral surface; no external images','original'])
    writer.writerow(['dreambound_painterly_materials','scripts/import_art.py','','Game Studio with OpenAI Codex','Original project-authored shader recipes; no third-party inputs','art/generated/materials.json','Per-surface craft maps; separate edge/recess/value masks; ivory/bronze/stone/bark/foliage material recipes; instancing support','original'])
print('DREAMBOUND_BELLROOT_COMPLETE '+json.dumps({'assets':len(ASSETS),'triangles':sum(x['triangles'] for x in metadata.values()),'preview':'art/preview/bellroot-first-person.png'}))

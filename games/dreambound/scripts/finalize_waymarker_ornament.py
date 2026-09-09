"""Single rear-inlay finishing alternative; outputs only repaired-art-v2."""
import bpy, bmesh, json, sys, time
import numpy as np
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree
from mathutils.geometry import barycentric_transform

ROOT=Path(sys.argv[sys.argv.index('--')+1]).resolve()
OUT=ROOT/'repaired-art-v2'
OUT.mkdir(parents=True,exist_ok=True)
if '--verify' in sys.argv:
    import hashlib
    bpy.ops.wm.open_mainfile(filepath=str(OUT/'repaired-asset.blend'))
    ob=bpy.data.objects['SM_Waymarker_Repaired']
    b=bmesh.new();b.from_mesh(ob.data)
    t=BVHTree.FromBMesh(b)
    checks={'blender_height_metres':max(v.co.z for v in ob.data.vertices)-min(v.co.z for v in ob.data.vertices),'uv_layers':[u.name for u in ob.data.uv_layers],'color_attributes':[a.name for a in ob.data.color_attributes],'material_slots':len(ob.data.materials),'remaining_boundary_edges':sum(e.is_boundary for e in b.edges),'remaining_nonmanifold_edges':sum(not e.is_manifold for e in b.edges),'probes':[]}
    for x,z in [(-.05,.65),(-.05,.8),(-.075,.5),(-.06,.7),(-.07,.85),(-.09,.424),(-.012,.93),(-.012,.53)]:
        p,n,idx,d=t.ray_cast(Vector((x,1,z)),Vector((0,-1,0)))
        checks['probes'].append({'x':x,'z':z,'rear_y':None if p is None else p.y,'normal_y':None if n is None else n.y})
    b.free()
    assert all(v['rear_y'] is not None and v['rear_y']>.095 for v in checks['probes'])
    assert checks['material_slots']==1 and len(checks['uv_layers'])==2
    checks['textures']=[{'name':im.name,'size':list(im.size),'packed_sha256':hashlib.sha256(im.packed_file.data).hexdigest()} for im in bpy.data.images if im.packed_file]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(OUT/'SM_Waymarker_Repaired.fbx'),colors_type='LINEAR')
    mesh=bpy.data.objects['SM_Waymarker_Repaired'];mesh.data.calc_loop_triangles()
    hulls=[o for o in bpy.context.scene.objects if o.name.startswith('UCX_SM_Waymarker_Repaired_')]
    checks['fbx_roundtrip']={'triangles':len(mesh.data.loop_triangles),'uv_layers':[u.name for u in mesh.data.uv_layers],'color_attributes':[a.name for a in mesh.data.color_attributes],'hulls':len(hulls),'height_metres':max((mesh.matrix_world@v.co).z for v in mesh.data.vertices)-min((mesh.matrix_world@v.co).z for v in mesh.data.vertices)}
    assert len(hulls)==4 and len(mesh.data.uv_layers)==2 and len(mesh.data.color_attributes)==1
    assert abs(checks['fbx_roundtrip']['height_metres']-2.4)<.00001
    report=json.loads((OUT/'art-finish-report.json').read_text())
    report['verification']=checks
    (OUT/'art-finish-report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('VERIFICATION',json.dumps(checks),flush=True)
    sys.exit(0)
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'repaired-v1/normalized-source.blend'))
obj=bpy.data.objects['SM_Waymarker_Repaired']
bm=bmesh.new(); bm.from_mesh(obj.data); bm.faces.ensure_lookup_table()
tree=BVHTree.FromBMesh(bm)
uv=bm.loops.layers.uv.active
image=next(n.image for n in obj.data.materials[0].node_tree.nodes if n.type=='TEX_IMAGE' and n.image.colorspace_settings.name=='sRGB')
pix=np.empty(len(image.pixels),dtype=np.float32); image.pixels.foreach_get(pix)
pix=pix.reshape(image.size[1],image.size[0],4)

def hit(x,z,side):
    return tree.ray_cast(Vector((x,side,z)),Vector((0,-side,0)))
def getuv(x,z,side):
    p,n,idx,d=hit(x,z,side)
    if p is None:return None
    f=bm.faces[idx]
    a,b,c=[Vector((l.vert.co.x,l.vert.co.z,0)) for l in f.loops]
    u,v,w=[Vector((l[uv].uv.x,l[uv].uv.y,0)) for l in f.loops]
    return barycentric_transform(Vector((x,z,0)),a,b,c,u,v,w)
for side,name in []:
    width,height=300,750
    pixels=np.zeros((height,width,4),dtype=np.float32)
    for iy in range(height):
        z=.35+iy*.001
        for ix in range(width):
            x=-.15+ix*.001
            p=getuv(x,z,side)
            if p is not None:
                pixels[iy,ix]=pix[int(p.y*image.size[1])%image.size[1],int(p.x*image.size[0])%image.size[0]]
    result=bpy.data.images.new(name,width,height,alpha=True)
    result.pixels.foreach_set(pixels.reshape(-1)); result.filepath_raw=str(OUT/(name+'.png'));result.file_format='PNG';result.save()
    print('MAP',name,flush=True)
(OUT/'probe-coordinates.json').write_text(json.dumps({'image_origin':'bottom-left','x_min':-.15,'z_min':.35,'pixel_metres':.001},indent=2))

# Retain the unmodified source as the geometry and per-loop UV donor.
donor_bm=bm; donor_tree=tree; donor_uv=uv
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'repaired-v1/repaired-asset.blend'))
bpy.context.preferences.filepaths.save_version=0
obj=bpy.data.objects['SM_Waymarker_Repaired']
bm=bmesh.new();bm.from_mesh(obj.data);bm.faces.ensure_lookup_table()
removed=[f for f in bm.faces if .045 < f.calc_center_median().y < .21 and any(-.118<v.co.x<.075 and .410<v.co.z<1.079 for v in f.verts)]
bmesh.ops.delete(bm,geom=removed,context='FACES')
remaining={e for e in bm.edges if e.is_boundary}; groups=[]
while remaining:
    todo=[remaining.pop()];component=[]
    while todo:
        e=todo.pop();component.append(e)
        for v in e.verts:
            for other in v.link_edges:
                if other in remaining:remaining.remove(other);todo.append(other)
    if len(component)>20:groups.append(component)
print('CUT',len(removed),'LOOPS',[(len(c),[[min(v.co[i] for e in c for v in e.verts),max(v.co[i] for e in c for v in e.verts)] for i in range(3)]) for c in groups],flush=True)
if '--probe' in sys.argv:sys.exit(0)
assert len(groups)==2,'Unexpected repair topology; do not guess additional patches'
groups.sort(key=lambda c:max(v.co.y for e in c for v in e.verts),reverse=True)
original_faces=set(bm.faces)
boundary_verts={v for c in groups for e in c for v in e.verts}
uv0=bm.loops.layers.uv.active
uv1=bm.loops.layers.uv.get('RepairDonorUV')
color=bm.loops.layers.float_color.get('RepairBlend')

def source_sample(x,z,side):
    p,n,idx,dist=donor_tree.ray_cast(Vector((x,side,z)),Vector((0,-side,0)))
    return p,n,None if idx is None else donor_bm.faces[idx]
def project_uv(face,x,z):
    a,b,c=[Vector((l.vert.co.x,l.vert.co.z,0)) for l in face.loops]
    u,v,w=[Vector((l[donor_uv].uv.x,l[donor_uv].uv.y,0)) for l in face.loops]
    return barycentric_transform(Vector((x,z,0)),a,b,c,u,v,w)
def smooth(a,b,x):
    t=max(0.0,min(1.0,(x-a)/(b-a)));return t*t*(3-2*t)
def mask(x,z):
    return smooth(-.119,-.091,x)*(1-smooth(.047,.076,x))*smooth(.409,.435,z)*(1-smooth(1.050,1.068,z))

component_patches=[]
for comp in groups:
    # A plane describes the front/rear thickness difference; front donor relief
    # then follows rear lean without copying a flat rectangle onto the stone.
    verts={v for e in comp for v in e.verts}
    a=[];b=[]
    for v in verts:
        p,_,_=source_sample(v.co.x,v.co.z,-1)
        if p is not None and p.y>-.19 and v.co.z>.50:
            a.append([v.co.x,v.co.z,1]);b.append(v.co.y+p.y)
    plane=np.linalg.lstsq(np.array(a),np.array(b),rcond=None)[0]
    before=set(bm.faces)
    filled=bmesh.ops.holes_fill(bm,edges=comp,sides=0)['faces']
    bmesh.ops.triangulate(bm,faces=filled,quad_method='BEAUTY',ngon_method='BEAUTY')
    for iteration in range(9):
        faces=[f for f in bm.faces if f not in before]
        edges={e for f in faces for e in f.edges if all(n not in before for n in e.link_faces) and e.calc_length()>.0035}
        if not edges:break
        bmesh.ops.subdivide_edges(bm,edges=list(edges),cuts=1,use_grid_fill=True)
    faces=[f for f in bm.faces if f not in before]
    bmesh.ops.triangulate(bm,faces=faces,quad_method='BEAUTY',ngon_method='BEAUTY')
    faces=[f for f in bm.faces if f not in before]
    for v in {v for f in faces for v in f.verts}:
        if v in boundary_verts:continue
        p,_,_=source_sample(v.co.x,v.co.z,-1)
        assert p is not None
        target=-p.y+float(np.dot(plane,[v.co.x,v.co.z,1]))
        # The donor's foreground root is not part of the ornament. Replace such
        # samples with the already-proven adjacent rear stone donor instead.
        if p.y<-.18:
            target=v.co.y
        if v.co.z<.49:
            target=v.co.y+max(-.012,min(.004,target-v.co.y))*smooth(.45,.49,v.co.z)
        weight=mask(v.co.x,v.co.z)
        v.co.y=v.co.y*(1-weight)+target*weight
    component_patches.extend(faces)
    print('DONOR_PATCH',len(faces),'PLANE',plane.tolist(),flush=True)

for f in component_patches:
    centre=f.calc_center_median()
    fp,_,front=source_sample(centre.x,centre.z,-1)
    _,_,rear=source_sample(centre.x,centre.z,1)
    donor_offset=0
    if centre.z<.467 or (centre.z<.55 and fp.y<-.17):
        _,_,front=source_sample(centre.x-.13,centre.z,1)
        donor_offset=-.13
    assert front is not None and rear is not None
    for l in f.loops:
        a=project_uv(rear,l.vert.co.x,l.vert.co.z)
        b=project_uv(front,l.vert.co.x+donor_offset,l.vert.co.z)
        l[uv0].uv=(a.x,a.y);l[uv1].uv=(b.x,b.y)
        weight=0 if l.vert in boundary_verts else mask(l.vert.co.x,l.vert.co.z)
        l[color]=(weight,weight,weight,1)
    f.smooth=True;f.material_index=0
bm.normal_update()
bm.to_mesh(obj.data);bm.free();donor_bm.free();obj.data.update()
obj.data.validate(verbose=False,clean_customdata=False)
obj.data.calc_loop_triangles()
obj.data.color_attributes.active_color=obj.data.color_attributes['RepairBlend']
collision=[o for o in bpy.context.scene.objects if o.name.startswith('UCX_SM_Waymarker_Repaired_')]
assert len(collision)==4
bpy.ops.object.select_all(action='DESELECT')
obj.select_set(True)
for hull in collision:hull.hide_set(False);hull.select_set(True)
bpy.context.view_layer.objects.active=obj
bpy.ops.export_scene.fbx(filepath=str(OUT/'SM_Waymarker_Repaired.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',bake_anim=False,use_mesh_modifiers=True,mesh_smooth_type='FACE',add_leaf_bones=False,path_mode='STRIP',colors_type='LINEAR')
for hull in collision:hull.hide_render=True;hull.hide_set(True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'repaired-asset.blend'))
report={'source':'../repaired-v1/repaired-asset.blend','donor':'../repaired-v1/normalized-source.blend (intact front of original textured.glb)','method':'Excise rear lower region including shard; fill outer and inner shell boundaries; transfer front lower inlay relief and original atlas UVs across feathered patch, fitting rear depth plane. Original front/roots and materials unchanged.','removed_faces':len(removed),'patch_faces':len(component_patches),'triangles':len(obj.data.loop_triangles),'height_metres':2.4,'collision_hulls':len(collision),'material_contract':{'slots':1,'UV0':'unchanged original atlas + original rear projection at patch perimeter','UV1':'front lower ornament donor on new patch; existing stone donor outside','VertexColor.R':'linear blend weight: lerp UV0/UV1 samples of original maps','roughness':'packed G','metallic':'packed B','textures':'unchanged original 4096 base-color and metallic-roughness maps'},'provenance':'Game Studio Blender edit of local TRELLIS.2 generated output, original OpenAI image_gen reference; original model/runtime terms retained, local-evaluation-only. No downloads or generated replacement images.','visual_verification':'pending render inspection'}
(OUT/'art-finish-report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('FINISHED',json.dumps(report),flush=True)

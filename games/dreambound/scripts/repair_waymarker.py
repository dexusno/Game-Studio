"""Bounded Blender repair of the retained TRELLIS waymarker; never alters input.

Run with Blender --background --python-exit-code 1 --python this.py -- INPUT.glb OUT.
Asset-specific rear excision coordinates are in metres after normalization to 2.4m.
Preserves the original atlas and reconstructs the missing surface using adjacent
rear stone as a UV donor. Review the resulting renders before engine import.
"""
import bpy
import bmesh
import json
import sys
import time
from pathlib import Path
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree
from mathutils.kdtree import KDTree
from mathutils.geometry import barycentric_transform

start = time.monotonic()
source, out = map(Path, sys.argv[sys.argv.index('--') + 1:])
out.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.preferences.filepaths.save_version = 0
bpy.ops.import_scene.gltf(filepath=str(source))
obj = next(o for o in bpy.context.scene.objects if o.type == 'MESH')
world = obj.matrix_world.copy()
obj.parent = None
obj.matrix_world = world
bpy.context.view_layer.objects.active = obj
obj.select_set(True)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
lo = Vector([min(v.co[i] for v in obj.data.vertices) for i in range(3)])
hi = Vector([max(v.co[i] for v in obj.data.vertices) for i in range(3)])
factor = 2.4 / (hi.z - lo.z)
obj.data.transform(Matrix.Scale(factor, 4) @ Matrix.Translation(Vector((-(lo.x+hi.x)/2, -(lo.y+hi.y)/2, -lo.z))))
obj.name = 'SM_Waymarker_Repaired'
obj.data.name = 'WaymarkerMesh'
bm = bmesh.new()
bm.from_mesh(obj.data)
source_triangles = len(bm.faces)
# Weld only coincident export seam vertices. Per-loop atlas coordinates survive.
bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=0.000001)
bm.faces.ensure_lookup_table()
donor = bm.copy()
donor.faces.ensure_lookup_table()
donor_uv = donor.loops.layers.uv.active
tree = BVHTree.FromBMesh(donor)
removed = [f for f in bm.faces if f.calc_center_median().y < .23 and any(-.102 < v.co.x < .018 and .39 < v.co.z < .965 and v.co.y > -.095 for v in f.verts)]
removed_count = len(removed)
bmesh.ops.delete(bm, geom=removed, context='FACES')
original_faces = set(bm.faces)
remaining = {e for e in bm.edges if e.is_boundary}
large = []
while remaining:
    todo = [remaining.pop()]
    component = []
    while todo:
        e = todo.pop()
        component.append(e)
        for v in e.verts:
            for other in v.link_edges:
                if other in remaining:
                    remaining.remove(other)
                    todo.append(other)
    if len(component) > 20:
        large.append(component)
assert len(large) == 3, f'Expected outer/inner rear shell and small lower boundary, found {len(large)}'
boundary = [edge for component in large for edge in component]
boundary_verts = {v for e in boundary for v in e.verts}
assert all(sum(e.is_boundary for e in v.link_edges) == 2 for v in boundary_verts)
for component in large:
    fill_result = bmesh.ops.holes_fill(bm, edges=component, sides=0)
    assert fill_result['faces'], 'Boundary fill did not create a surface'
    bmesh.ops.triangulate(bm, faces=fill_result['faces'], quad_method='BEAUTY', ngon_method='BEAUTY')
for iteration in range(8):
    patch = [f for f in bm.faces if f not in original_faces]
    edges = {e for f in patch for e in f.edges if all(n not in original_faces for n in e.link_faces) and e.calc_length() > .008}
    if not edges:
        break
    bmesh.ops.subdivide_edges(bm, edges=list(edges), cuts=1, use_grid_fill=True)
patch = [f for f in bm.faces if f not in original_faces]
bmesh.ops.triangulate(bm, faces=patch, quad_method='BEAUTY', ngon_method='BEAUTY')
patch = [f for f in bm.faces if f not in original_faces]
print('PATCH_TRIANGLES', len(patch), flush=True)
uv = bm.loops.layers.uv.active
uv_donor = bm.loops.layers.uv.new('RepairDonorUV')
blend_layer = bm.loops.layers.float_color.new('RepairBlend')
for face in bm.faces:
    for loop in face.loops:
        loop[blend_layer] = (0,0,0,1)
# Locate the actual missing/deeply damaged surface, then feather its repair into
# surviving stone over 25mm. This preserves the existing atlas at the boundary.
bad_points = []
for iz in range(220):
    z = .415 + iz*.0025
    for ix in range(57):
        x = -.115 + ix*.0025
        p,n,_,_ = tree.ray_cast(Vector((x,1,z)),Vector((0,-1,0)))
        if p is None or p.y < .107 or (n.y < .45 and -.10 < x < -.015 and .45 < z < .915):
            bad_points.append(Vector((x,z,0)))
damage = KDTree(len(bad_points))
for index,point in enumerate(bad_points):
    damage.insert(point,index)
damage.balance()
weights = {}
for vertex in {v for f in patch for v in f.verts}:
    distance = damage.find(Vector((vertex.co.x,vertex.co.z,0)))[2]
    t = max(0.0,min(1.0,(distance-.005)/.025))
    weight = 1.0-t*t*(3.0-2.0*t)
    if vertex in boundary_verts:
        weight = 0.0
    weights[vertex] = weight
    p,_,_,_ = tree.ray_cast(Vector((vertex.co.x,1,vertex.co.z)),Vector((0,-1,0)))
    if vertex not in boundary_verts and p is not None and p.y > .107:
        # Restore only fine relief; deep generated cavities are not valid donors.
        vertex.co.y += max(-.001,min(.001,p.y-vertex.co.y))*(1.0-weight)
bm.normal_update()
misses = 0
donor_fallback_faces = 0
for face in patch:
    centre = face.calc_center_median()
    for layer,offset in [(uv,0.0),(uv_donor,-.13)]:
        point, normal, index, distance = tree.ray_cast(Vector((centre.x+offset, 1, centre.z)), Vector((0, -1, 0)))
        if point is None:
            misses += 1
            continue
        src = donor.faces[index]
        a,b,c = [Vector((loop.vert.co.x, loop.vert.co.z, 0)) for loop in src.loops]
        u,v,w = [Vector((loop[donor_uv].uv.x, loop[donor_uv].uv.y, 0)) for loop in src.loops]
        for loop in face.loops:
            sample = barycentric_transform(Vector((loop.vert.co.x+offset,loop.vert.co.z,0)),a,b,c,u,v,w)
            loop[layer].uv = (sample.x, sample.y)
    for loop in face.loops:
        weight = weights[loop.vert]
        loop[blend_layer] = (weight,weight,weight,1)
    if max(weights[v] for v in face.verts) > 0:
        donor_fallback_faces += 1
    face.material_index = 0
    face.smooth = True
assert misses == 0, f'Missing UV donor hits: {misses}'
bm.normal_update()
repaired_tree = BVHTree.FromBMesh(bm)
probe = []
for x,z in [(-.05,.65),(-.05,.8),(-.075,.5),(-.06,.7),(-.07,.85),(-.09,.424)]:
    p,n,_,_ = repaired_tree.ray_cast(Vector((x,1,z)),Vector((0,-1,0)))
    probe.append({'x':x,'z':z,'rear_y':None if p is None else p.y,'normal_y':None if n is None else n.y})
assert all(p['rear_y'] is not None and p['rear_y'] > .1 for p in probe), probe
report = {'source':source.name,'height_metres':2.4,'original_triangles':source_triangles,
          'removed_damaged_faces':removed_count,'repair_boundary_edges':len(boundary),
          'repair_boundary_components':len(large),
          'new_patch_triangles':len(patch),'final_triangles':len(bm.faces),
          'repair_method':'Local rear face/sidewall excision, triangulated rebuilt surface, original rear UV plus adjacent stone UV1; vertex color R blends PBR samples across a 25mm feather while preserving original maps.',
          'adjacent_stone_donor_faces':donor_fallback_faces,
          'remaining_boundary_edges':sum(e.is_boundary for e in bm.edges),
          'remaining_nonmanifold_edges':sum(not e.is_manifold for e in bm.edges),
          'rear_surface_probes':probe,'original_unmodified':True}
bm.to_mesh(obj.data)
bm.free(); donor.free()
obj.data.update()
report['mesh_validation_repaired_degenerates'] = obj.data.validate(verbose=False, clean_customdata=False)
obj.data.update()
obj.data.calc_loop_triangles()
report['final_triangles'] = len(obj.data.loop_triangles)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0
mat = obj.data.materials[0]
mat.name = 'M_Waymarker'
images = [node.image for node in mat.node_tree.nodes if node.type=='TEX_IMAGE']
base = next(im for im in images if im.colorspace_settings.name=='sRGB')
packed = next(im for im in images if im != base)
base.pack(); packed.pack()
nodes = mat.node_tree.nodes; links = mat.node_tree.links
nodes.clear()
output = nodes.new('ShaderNodeOutputMaterial')
principled = nodes.new('ShaderNodeBsdfPrincipled')
links.new(principled.outputs['BSDF'],output.inputs['Surface'])
vertex = nodes.new('ShaderNodeVertexColor'); vertex.layer_name='RepairBlend'
channels = nodes.new('ShaderNodeSeparateColor')
links.new(vertex.outputs['Color'],channels.inputs['Color'])
uv_nodes = []
for name in [obj.data.uv_layers[0].name,'RepairDonorUV']:
    coord=nodes.new('ShaderNodeUVMap'); coord.uv_map=name; uv_nodes.append(coord)
mixed=[]
for im in [base,packed]:
    tex=[]
    for coord in uv_nodes:
        node=nodes.new('ShaderNodeTexImage'); node.image=im
        links.new(coord.outputs['UV'],node.inputs['Vector']); tex.append(node)
    mix=nodes.new('ShaderNodeMixRGB'); mix.blend_type='MIX'
    links.new(channels.outputs['Red'],mix.inputs[0])
    links.new(tex[0].outputs['Color'],mix.inputs[1]); links.new(tex[1].outputs['Color'],mix.inputs[2])
    mixed.append(mix)
links.new(mixed[0].outputs['Color'],principled.inputs['Base Color'])
mr=nodes.new('ShaderNodeSeparateColor'); links.new(mixed[1].outputs['Color'],mr.inputs['Color'])
links.new(mr.outputs['Green'],principled.inputs['Roughness'])
links.new(mr.outputs['Blue'],principled.inputs['Metallic'])
obj.data.color_attributes.active_color = obj.data.color_attributes['RepairBlend']
bpy.ops.wm.save_as_mainfile(filepath=str(out/'repaired-asset.blend'))
# FBX plus this Blender scene preserve the dual-UV material contract. Generic
# glTF export cannot represent this shader blend, so is intentionally omitted.
# Separate low-complexity convex collision for a procedural scenery prop.
collision = []
for index,(centre,size) in enumerate([
    ((0,0,.22),(1.12,.75,.44)),
    ((0,-.01,1.2),(.62,.35,1.78)),
    ((-.02,-.012,2.205),(.34,.27,.39)),
    ((.285,-.09,.98),(.25,.38,1.23)),
]):
    bpy.ops.mesh.primitive_cube_add(size=1, location=centre)
    hull = bpy.context.object
    hull.name = f'UCX_SM_Waymarker_Repaired_{index:02}'
    hull.scale = size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    collision.append(hull)
bpy.ops.object.select_all(action='DESELECT')
obj.select_set(True)
for hull in collision:
    hull.select_set(True)
bpy.context.view_layer.objects.active = obj
bpy.ops.export_scene.fbx(filepath=str(out/'SM_Waymarker_Repaired.fbx'), use_selection=True,
    object_types={'MESH'}, axis_forward='-Y', axis_up='Z', apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_UNITS', bake_anim=False, use_mesh_modifiers=True,
    mesh_smooth_type='FACE', add_leaf_bones=False, path_mode='STRIP', colors_type='LINEAR')
report['material_contract']={'UV0':'original rear projection / unchanged atlas','UV1':'adjacent stone donor','VertexColor.R':'blend weight (linear); 0 on untouched faces','PBR':'lerp base color and packed MR samples; roughness G, metallic B','textures':'unchanged original 4K maps','standard_GLTF_export':False}
report['collision'] = {'type':'authored UCX convex boxes','hulls':4,'triangles_total':48,'scope':'coarse obstacle envelope; decorative root gaps intentionally not walkable'}
for hull in collision:
    hull.hide_render=True
    hull.hide_set(True)
bpy.ops.wm.save_as_mainfile(filepath=str(out/'repaired-asset.blend'))
report['elapsed_seconds'] = round(time.monotonic()-start,3)
(out/'repair-report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('REPAIR_REPORT '+json.dumps(report),flush=True)

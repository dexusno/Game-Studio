"""Repair existing source/export UVs only, without rerunning geometry generation.

Blender --background --factory-startup --python-exit-code 1 --python <this file>
The original FBX custom collision objects are preserved during selective export.
"""
import bpy
import bmesh
import hashlib
import json
import math
import sys
import time
from pathlib import Path
from mathutils import Vector

ART = Path(__file__).resolve().parents[1]
OUT = ART / 'generated'
exec(compile((ART/'source/uv_tools.py').read_text(encoding='utf-8'),
             str(ART/'source/uv_tools.py'), 'exec'), globals())
metadata = json.loads((OUT/'asset-metadata.json').read_text(encoding='utf-8'))
bpy.ops.wm.open_mainfile(filepath=str(ART/'Dreambound_Kit.blend'))
bpy.context.preferences.filepaths.save_version = 0
report = {'operation': 'UV-only repair; geometry, material slots, vertex masks and custom collision retained',
          'tool': 'Blender 5.2.1 LTS', 'assets': [], 'changed_meshes': [], 'success': False,
          'known_limitation': 'A few geometric bevel slivers remain unchanged by this UV-only pass. Prior native Blender tangent audit found 2 zero-tangent loops on Wall, 4 on Stair and 2 on submicron Head geometry. Native Unreal follow-up is required; no warning-free native result is claimed.'}


def fingerprint(obj):
    mesh = obj.data
    return hashlib.sha256(repr((tuple(tuple(v.co) for v in mesh.vertices),
        tuple(tuple(p.vertices) for p in mesh.polygons),
        tuple(p.material_index for p in mesh.polygons),
        tuple(m.name for m in mesh.materials),
        tuple(tuple(row) for row in obj.matrix_world),
        tuple(tuple(c.color) for c in mesh.color_attributes['Color'].data))).encode()).hexdigest()


def uv_fingerprint(mesh):
    return hashlib.sha256(repr(tuple((u.name, tuple(tuple(p.uv) for p in u.data))
        for u in mesh.uv_layers)).encode()).hexdigest()


for name, meta in metadata.items():
    obj = bpy.data.objects[name]; mesh = obj.data
    old_geometry = fingerprint(obj); old_uv = uv_fingerprint(mesh)
    before = {'active_uv': mesh.uv_layers.active.name,
              'layers': [u.name for u in mesh.uv_layers],
              'collapsed_or_extremely_stretched_triangles': len(uv_defects(mesh))}
    repaired_faces = repair_mesh_uv(mesh)
    # Existing branch sides have an identifiable 0->last-column seam. Their
    # longitudinal field should wrap once, instead of stretching backwards.
    seam_loops = 0
    if name in {'SM_BellTree', 'SM_Root', 'SM_Canopy'}:
        uv = mesh.uv_layers.active
        for face in mesh.polygons:
            if len(face.vertices) != 4: continue
            points = [uv.data[i].uv for i in face.loop_indices]
            us = [p.x for p in points]; vs = [p.y for p in points]
            if (abs(min(us)) < 1e-6 and any(abs(max(us)-(n-1)/n) < 1e-6 for n in [6,8,10,12])
                and sum(abs(u) < 1e-6 for u in us) == 2 and max(vs)-min(vs) > 1e-6):
                for i in face.loop_indices:
                    if abs(uv.data[i].uv.x) < 1e-6:
                        uv.data[i].uv.x = 1; seam_loops += 1
    if fingerprint(obj) != old_geometry:
        raise RuntimeError('UV repair changed model geometry or masks: ' + name)
    changed = uv_fingerprint(mesh) != old_uv
    tangents = native_tangent_audit(mesh) if '--audit-tangents' in sys.argv else {'not_repeated':'UV-only correction; see known_limitation'}
    row = {'name': name, 'before': before, 'reprojected_faces': repaired_faces,
           'unwrapped_seam_loops': seam_loops, 'changed': changed,
           'after_collapsed_triangles': len(uv_defects(mesh)), 'tangents': tangents,
           'geometry_masks_slots_unchanged': True}
    report['assets'].append(row)
    if changed: report['changed_meshes'].append(name)
    print('UV_AUDIT ' + json.dumps(row), flush=True)

if '--audit-only' in sys.argv:
    print('DREAMBOUND_UV_AUDIT_COMPLETE '+json.dumps(report),flush=True)
    raise SystemExit(0)


for name in report['changed_meshes']:
    obj = bpy.data.objects[name]
    before_objects = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(OUT/(name+'.fbx')), use_custom_normals=True,
                             colors_type='SRGB')
    imported = [o for o in bpy.data.objects if o not in before_objects]
    colliders = [o for o in imported if o.name.startswith('UCX_'+name+'_')]
    if len(colliders) != metadata[name]['collision_hulls']:
        raise RuntimeError('Original collision recovery failed: ' + name)
    for other in imported:
        if other not in colliders: bpy.data.objects.remove(other, do_unlink=True)
    bpy.ops.object.select_all(action='DESELECT')
    obj.hide_set(False); obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    for collider in colliders: collider.select_set(True)
    images = [(node,node.image) for mat in obj.data.materials for node in mat.node_tree.nodes if node.type=='TEX_IMAGE']
    for node,_ in images: node.image = None
    try:
        temporary = OUT/(name+'.export.fbx')
        bpy.ops.export_scene.fbx(filepath=str(temporary), use_selection=True, object_types={'MESH'},
            apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS', axis_forward='X', axis_up='Z',
            use_space_transform=True, bake_space_transform=True, use_mesh_modifiers=True,
            mesh_smooth_type='FACE', add_leaf_bones=False, bake_anim=False,
            use_custom_props=False, path_mode='STRIP')
        for attempt in range(20):
            try:
                temporary.replace(OUT/(name+'.fbx')); break
            except PermissionError:
                if attempt == 19: raise
                time.sleep(.25)
    finally:
        for node,image in images: node.image = image
    for collider in colliders: bpy.data.objects.remove(collider, do_unlink=True)
    obj.hide_set(True)

for img in bpy.data.images:
    if img.name.startswith('T_'): img.filepath = '//generated/'+img.name+'.png'
bpy.data.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)
bpy.ops.wm.save_as_mainfile(filepath=str(ART/'Dreambound_Kit.blend'), compress=True)
report['success'] = True
(OUT/'uv-repair-report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
print('DREAMBOUND_UV_REPAIR_COMPLETE ' + json.dumps({'changed_meshes':report['changed_meshes']}), flush=True)

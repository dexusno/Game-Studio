"""Shared surface-UV repair; never changes model geometry or vertex masks."""
import math
import bpy
import bmesh
from mathutils import Vector


def single_craft_uv(mesh):
    """Primitives start with UVMap; keep one populated, explicit export channel."""
    keep = mesh.uv_layers.get('CraftUV')
    if keep is None:
        keep = mesh.uv_layers.active or mesh.uv_layers.new(name='CraftUV')
        keep.name = 'CraftUV'
    for layer in list(mesh.uv_layers):
        if layer != keep:
            mesh.uv_layers.remove(layer)
    mesh.uv_layers.active_index = 0
    mesh.uv_layers[0].active_render = True
    return mesh.uv_layers[0]


def project_face_uv(mesh, face, uv, density=95.0, transform=None, centered=False):
    normal = face.normal.copy()
    if transform is not None:
        normal = transform.to_3x3().inverted().transposed() @ normal
    axis = max(range(3), key=lambda a: abs(normal[a]))
    axes = ((1, 2), (0, 2), (0, 1))[axis]
    points = [mesh.vertices[mesh.loops[i].vertex_index].co.copy() for i in face.loop_indices]
    if transform is not None:
        points = [transform @ p for p in points]
    center = sum(points, Vector()) / len(points) if centered else Vector()
    for index, point in zip(face.loop_indices, points):
        uv.data[index].uv = tuple((point[a] - center[a]) / density for a in axes)


def uv_defects(mesh, uv=None):
    """Find collapsed triangles using area ratio, independent of triangle size."""
    uv = uv or mesh.uv_layers.active
    mesh.calc_loop_triangles()
    bad = []
    for triangle in mesh.loop_triangles:
        points = [mesh.vertices[i].co for i in triangle.vertices]
        area = (points[1] - points[0]).cross(points[2] - points[0]).length
        if area <= 1e-10 or min((points[i]-points[(i+1)%3]).length for i in range(3)) < 1e-4:
            continue  # Physical slivers are separately reported, not modified.
        a, b, c = [uv.data[i].uv for i in triangle.loops]
        det = abs((b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x))
        if not math.isfinite(det) or det == 0 or det / area < 1e-7:
            bad.append(triangle.polygon_index)
    return bad


def repair_mesh_uv(mesh):
    uv = single_craft_uv(mesh)
    broken = set(uv_defects(mesh, uv))
    for index in sorted(broken):
        # Island-local coordinates avoid lost precision on tiny bevel faces.
        project_face_uv(mesh, mesh.polygons[index], uv, density=35, centered=True)
    remaining = uv_defects(mesh, uv)
    # A warped polygon can contain a triangle perpendicular to its averaged
    # normal. Use an oblique basis for that island instead of a collapsed axis.
    for index in set(remaining):
        face = mesh.polygons[index]
        u = Vector((1, .371, .619)).normalized()
        v = u.cross(Vector((.217, 1, -.413))).normalized()
        points = [mesh.vertices[mesh.loops[i].vertex_index].co for i in face.loop_indices]
        center = sum(points, Vector()) / len(points)
        for i, point in zip(face.loop_indices, points):
            uv.data[i].uv = ((point-center).dot(u)/35, (point-center).dot(v)/35)
        broken.add(index)
    remaining = uv_defects(mesh, uv)
    if remaining:
        raise RuntimeError(f'Unresolved surface UVs on {mesh.name}: {len(remaining)} triangles')
    return len(broken)


def native_tangent_audit(mesh):
    # Preserve Blender's actual polygon tessellation and authored split normals.
    # A separate bmesh triangulation can choose different diagonals on narrow
    # irregular stone faces and creates a test of different rendered geometry.
    mesh.calc_loop_triangles()
    triangles = list(mesh.loop_triangles)
    copy = bpy.data.meshes.new('UV tangent verification')
    copy.from_pydata([v.co for v in mesh.vertices], [], [t.vertices for t in triangles])
    uv = copy.uv_layers.new(name='CraftUV'); normals = []
    for face, triangle in zip(copy.polygons, triangles):
        face.use_smooth = mesh.polygons[triangle.polygon_index].use_smooth
        for new_index, old_index in zip(face.loop_indices, triangle.loops):
            uv.data[new_index].uv = mesh.uv_layers.active.data[old_index].uv
            normals.append(mesh.corner_normals[old_index].vector)
    copy.normals_split_custom_set(normals)
    copy.calc_tangents(uvmap='CraftUV')
    bad_faces = set(); bad_loops = 0; submicron = 0; non_finite = 0
    for face in copy.polygons:
        count = sum(copy.loops[i].tangent.length_squared < 1e-8 for i in face.loop_indices)
        non_finite += sum(not all(math.isfinite(v) for v in copy.loops[i].tangent) for i in face.loop_indices)
        if not count: continue
        points = [copy.vertices[i].co for i in face.vertices]
        if min((points[i]-points[(i+1)%3]).length for i in range(3)) < 1e-4:
            submicron += count
        else:
            bad_loops += count
            bad_faces.add(triangles[face.index].polygon_index)
    bpy.data.meshes.remove(copy)
    if non_finite: raise RuntimeError('Non-finite MikkTSpace tangent: ' + mesh.name)
    return {'bad_faces': sorted(bad_faces), 'nearly_zero_tangent_loops_on_valid_geometry': bad_loops,
            'nearly_zero_tangents_on_submicron_geometry': submicron, 'non_finite_tangent_loops': non_finite}

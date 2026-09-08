"""Original Bellroot garden kit; background Blender, writes only art/garden/.

Reads the existing palette and proven UV/FBX helper functions without executing
the shield authoring script. No downloads, image generation or purchased assets.
"""
import ast
import bpy
import bmesh
import hashlib
import json
import math
import random
import sys
import time
from pathlib import Path
from mathutils import Vector

GAME = Path(__file__).resolve().parents[1]
OUT = GAME / "art/garden"
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = "METRIC"
scene.unit_settings.scale_length = .01
bpy.context.preferences.filepaths.save_version = 0
PALETTE = json.loads((GAME / "art/generated/materials.json").read_text(encoding="utf-8"))
uv_namespace = {}
uv_source = GAME / "art/source/uv_tools.py"
exec(compile(uv_source.read_text(encoding="utf-8"), str(uv_source), "exec"), uv_namespace)
single_craft_uv = uv_namespace["single_craft_uv"]
project_face_uv = uv_namespace["project_face_uv"]
repair_mesh_uv = uv_namespace["repair_mesh_uv"]
native_tangent_audit = uv_namespace["native_tangent_audit"]
helper_source = GAME / "scripts/create_segmented_shield.py"
helper_tree = ast.parse(helper_source.read_text(encoding="utf-8"))
helper_names = {"material", "active", "finish_part", "mesh", "box", "cylinder", "finalize", "audit_exports", "portable_source"}
helper_functions = ast.Module(body=[node for node in helper_tree.body
    if isinstance(node, ast.FunctionDef) and node.name in helper_names], type_ignores=[])
exec(compile(helper_functions, str(helper_source), "exec"), globals())
MATS = {name: material(name) for name in ("M_Stone", "M_StoneLight", "M_Mortar", "M_Moss",
    "M_Root", "M_RootDark", "M_RootLight", "M_Leaf", "M_LeafLight", "M_LeafGold",
    "M_ShieldBronze", "M_Ceramic")}
PARTS, ASSETS = [], {}


def solid_hulls(prefixes):
    """One tight convex hull per structural stone; exported as Unreal UCX."""
    colliders = []
    for part in PARTS:
        if not part.name.startswith(prefixes) or "moss shelf" in part.name:
            continue
        bm = bmesh.new()
        for vertex in part.data.vertices:
            bm.verts.new(part.matrix_world @ vertex.co)
        result = bmesh.ops.convex_hull(bm, input=list(bm.verts), use_existing_faces=False)
        unused = [item for item in set(result["geom_interior"] + result["geom_unused"])
                  if isinstance(item, bmesh.types.BMVert) and item.is_valid]
        if unused:
            bmesh.ops.delete(bm, geom=unused, context="VERTS")
        bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
        data = bpy.data.meshes.new("Structural collision")
        bm.to_mesh(data)
        bm.free()
        collider = bpy.data.objects.new("Structural collision", data)
        scene.collection.objects.link(collider)
        colliders.append(collider)
    return colliders


def tube(name, points, radii, mat="M_Root", sides=8):
    points = [Vector(p) for p in points]
    vertices, faces = [], []
    for j, point in enumerate(points):
        tangent = (points[min(j + 1, len(points) - 1)] - points[max(0, j - 1)]).normalized()
        guide = Vector((0, 0, 1)) if abs(tangent.z) < .9 else Vector((0, 1, 0))
        u = tangent.cross(guide).normalized()
        v = tangent.cross(u).normalized()
        for i in range(sides):
            a = math.tau * i / sides
            vertices.append(tuple(point + radii[j] * (u * math.cos(a) + v * math.sin(a))))
    faces = [tuple(reversed(range(sides))), tuple((len(points) - 1) * sides + i for i in range(sides))]
    for j in range(len(points) - 1):
        for i in range(sides):
            faces.append((j * sides + i, j * sides + (i + 1) % sides,
                (j + 1) * sides + (i + 1) % sides, (j + 1) * sides + i))
    return mesh(name, vertices, faces, mat, 0, smooth=True, recess=.15)


def leaf(name, root, direction, length, width, mat, lift=.12, thickness=.24):
    """Closed curled blade with a broad ridge and pointed authored silhouette."""
    root, forward = Vector(root), Vector(direction).normalized()
    up = Vector((0, 0, 1))
    side = forward.cross(up).normalized()
    if side.length < .5:
        side = Vector((0, 1, 0))
    normal = side.cross(forward).normalized()
    outline = [(0, 0), (.22, -.42), (.62, -.5), (1, 0), (.62, .5), (.22, .42)]
    vertices = []
    for layer in (-1, 1):
        for t, cross in outline:
            arch = math.sin(t * math.pi) * length * lift
            vertices.append(tuple(root + forward * (length * t) + side * (width * cross)
                + normal * (arch + layer * thickness * .5)))
    # Matching explicit triangulation on both curved skins prevents FBX from
    # selecting different diagonals for a thin nonplanar n-gon.
    faces = [(0, i + 1, i) for i in range(1, 5)]
    faces.extend((6, 6 + i, 6 + i + 1) for i in range(1, 5))
    faces.extend((i, (i + 1) % 6, 6 + (i + 1) % 6, 6 + i) for i in range(6))
    return mesh(name, vertices, faces, mat, 0, smooth=False, recess=.06, value=.92)


def stone(name, center, dims, seed, mat="M_Stone", moss=True):
    """Bent strata and a broken crown; no sphere displacement/noise scaffold."""
    rng = random.Random(seed)
    x, y, z = center
    w, d, h = dims
    profile = [(0, .96, .9, -.04, 0), (.17, 1, 1, 0, -.025),
        (.40, .89, .91, .015, .018), (.62, .82, .81, -.08, .03),
        (.80, .64, .69, .03, -.025), (1, .42, .47, .07, -.01)]
    outline = [(-.49, -.34), (-.29, -.50), (.21, -.48), (.49, -.28),
        (.51, .15), (.32, .45), (-.04, .51), (-.42, .35)]
    corner_offsets = [rng.uniform(-.055, .055) for _ in outline]
    vertices = []
    for row, (zz, sx, sy, shift_x, shift_y) in enumerate(profile):
        for i, (xx, yy) in enumerate(outline):
            raised = corner_offsets[i] * (.10 + zz * .9) if row else 0
            vertices.append((x + w * (xx * sx + shift_x), y + d * (yy * sy + shift_y), z + h * (zz + raised)))
    faces = [tuple(reversed(range(8))), tuple(40 + i for i in range(8))]
    for row in range(5):
        for i in range(8):
            faces.append((row * 8 + i, row * 8 + (i + 1) % 8,
                (row + 1) * 8 + (i + 1) % 8, (row + 1) * 8 + i))
    obj = mesh(name, vertices, faces, mat, min(5.5, min(dims) * .034), recess=.22, value=.88)
    # A few physical lichen/moss shelves sit on upward surfaces, avoiding an
    # indiscriminate green coating or a replacement for the sculpted silhouette.
    if moss:
        shelves = [p for p in obj.data.polygons if p.normal.z > .4 and p.area > w * d * .006]
        for index, face in enumerate(shelves[::max(1, len(shelves) // 4)]):
            points = [obj.matrix_world @ obj.data.vertices[i].co for i in face.vertices]
            middle = sum(points, Vector()) / len(points)
            top = []
            for edge, point in enumerate(points):
                following = points[(edge + 1) % len(points)]
                for step in range(4):
                    t = step / 4
                    boundary = point.lerp(following, t)
                    scallop = .70 + .17 * math.sin((edge * 4 + step) * 2.36 + seed)
                    top.append(middle + (boundary - middle) * scallop + face.normal * .70)
            bottom = [point - face.normal * .9 for point in top]
            n = len(top)
            patch_vertices = bottom + top + [middle - face.normal * .2, middle + face.normal * .7]
            patch_faces = [(2 * n, (i + 1) % n, i) for i in range(n)]
            patch_faces.extend((2 * n + 1, n + i, n + (i + 1) % n) for i in range(n))
            patch_faces.extend((i, (i + 1) % n, n + (i + 1) % n, n + i) for i in range(n))
            mesh(name + " moss shelf " + str(index), [tuple(p) for p in patch_vertices],
                patch_faces, "M_Moss", .10, recess=.42, value=.77)
    return obj


def quarry_block(center, dims, seed, mat):
    """Load-bearing cut stone with worn corners, not a repeated tapered rock."""
    rng = random.Random(seed)
    x, y, z = center
    w, d, h = dims
    outline = [(-.5, -.33), (-.36, -.5), (.32, -.5), (.5, -.30),
        (.5, .32), (.31, .5), (-.35, .5), (-.5, .34)]
    top_levels = [rng.uniform(-2.3, 2.3) for _ in outline]
    vertices = [(x + a * w, y + b * d, z) for a, b in outline]
    vertices.extend((x + a * w * .965 + 1.5, y + b * d * .97 - 1.0, z + h + top_levels[i])
        for i, (a, b) in enumerate(outline))
    faces = [tuple(reversed(range(8))), tuple(8 + i for i in range(8))]
    faces.extend((i, (i + 1) % 8, 8 + (i + 1) % 8, 8 + i) for i in range(8))
    return mesh("Worn quarried buttress course", vertices, faces, mat, 3.8, recess=.18, value=.9)


def fern(center, radius, angle, seed):
    rng = random.Random(seed)
    center = Vector(center)
    for frond in range(7):
        a = angle + frond * math.tau / 7 + rng.uniform(-.1, .1)
        direction = Vector((math.cos(a), math.sin(a), 0))
        reach = radius * rng.uniform(.75, 1.08)
        points = [center + direction * (reach * t) + Vector((0, 0, reach * (.08 + .85 * math.sin(t * math.pi * .72))))
            for t in (0, .2, .4, .6, .8, 1)]
        tube("Curved fern rachis", points, [1.05, .94, .76, .56, .30, .10], "M_Root", 6)
        lateral = Vector((-direction.y, direction.x, .12))
        for j in range(1, 9):
            t = j / 10
            p = center + direction * (reach * t) + Vector((0, 0, reach * (.08 + .85 * math.sin(t * math.pi * .72))))
            length = reach * .30 * (1 - t) + 4
            for side in (-1, 1):
                aim = (lateral * side + direction * .3).normalized()
                leaf("Paired fern pinna", p, aim, length, length * .29,
                    "M_LeafLight" if j % 3 == 0 else "M_Leaf", .12, .18)


def grass(center, scale, angle, seed):
    rng = random.Random(seed)
    c = Vector(center)
    for i in range(13):
        a = angle + rng.uniform(-1.1, 1.1)
        horizontal = Vector((math.cos(a), math.sin(a), 0))
        length = scale * rng.uniform(25, 52)
        stem = c + Vector((rng.uniform(-8, 8), rng.uniform(-8, 8), .5))
        forward = (horizontal * rng.uniform(.4, .9) + Vector((0, 0, 1))).normalized()
        leaf("Arched sedge blade", stem, forward, length, scale * rng.uniform(3, 5.2),
            "M_LeafLight" if i % 4 else "M_LeafGold", -.20, .16)


def flower(center, scale, seed):
    rng = random.Random(seed)
    c = Vector(center)
    for i in range(3):
        top = c + Vector((rng.uniform(-10, 10), rng.uniform(-10, 10), scale * rng.uniform(42, 66)))
        tube("Meadow bloom stem", [c, (c + top) * .5 + Vector((3, 0, 0)), top], [.55, .43, .3], "M_Root", 5)
        for petal in range(5):
            a = petal * math.tau / 5
            leaf("Gold meadow petal", top, (math.cos(a), math.sin(a), .18),
                scale * 8, scale * 7, "M_LeafGold", .18, .25)
        cylinder("Seed center", top + Vector((0, 0, .6)), scale * 2.1, .9, "M_RootDark", axis="Z", sides=8, bevel=.1)


# A substantial geological backing breaks the thin rectangular arena skyline.
stone("Cliff left leaning tower", (-238, 12, -5), (505, 418, 708), 87201)
stone("Cliff broken high shoulder", (122, -50, -4), (527, 406, 853), 87202, "M_StoneLight")
stone("Cliff forward shelf", (296, 70, -3), (305, 316, 488), 87203)
stone("Cliff low toe", (-72, 139, -7), (438, 282, 223), 87204)
tube("Root over stone face", [(36, 186, 714), (10, 199, 608), (-26, 215, 500),
    (-93, 218, 331), (-175, 201, 180), (-205, 218, 9)], [18, 16, 13, 11, 8, 3], "M_Root", 10)
finalize("SM_GardenCliffBank", ["M_Stone", "M_StoneLight", "M_Moss", "M_Root"],
    "Ground center; X long width/Y depth; inward detailed face +Y; scenery behind existing collision walls")

# Structural ruin with broad setbacks, ledges, a fluted recess and broken cap.
box("Buttress footing", (0, 0, 26), (247, 224, 52), "M_Stone", 9, recess=.23, value=.87)
for i, (z, w, d, y) in enumerate(((98, 217, 186, 0), (214, 184, 168, -7),
    (327, 164, 153, -13), (431, 144, 135, -20), (524, 139, 130, -21))):
    quarry_block((0, y, z - 54), (w, d, 106), 88300 + i,
        "M_StoneLight" if i % 2 else "M_Stone")
box("Buttress high carved cornice", (0, -18, 552), (204, 182, 37), "M_StoneLight", 7, recess=.15)
stone("Broken crowning shoulder", (-26, -32, 567), (173, 136, 103), 88313, "M_StoneLight")
box("Tall recessed dark channel", (0, 73, 295), (61, 11, 329), "M_Mortar", 4, recess=.3, value=.83)
for side in (-1, 1):
    box("Channel stone cheek", (side * 44, 81, 298), (27, 26, 352), "M_StoneLight", 6, recess=.18)
for z in (167, 292, 417):
    box("Worn channel crosspiece", (0, 89, z), (79, 18, 24), "M_Stone", 5, recess=.18)
finalize("SM_GardenButtress", ["M_Stone", "M_StoneLight", "M_Moss", "M_Mortar"],
    "Ground center; carved front +Y; solid support outside doorway clearance",
    solid_hulls(("Buttress footing", "Worn quarried buttress course", "Buttress high", "Broken crowning")))

stone("Low bedrock sweep", (-71, 0, -4), (337, 263, 167), 89301)
stone("Broken companion rock", (138, -48, -3), (229, 212, 219), 89302, "M_StoneLight")
stone("Foreground layered slab", (23, 118, -6), (252, 178, 84), 89303)
finalize("SM_GardenRockCluster", ["M_Stone", "M_StoneLight", "M_Moss"],
    "Ground center; solid layered corner rock bed; keep outside combat lanes",
    solid_hulls(("Low bedrock sweep", "Broken companion rock", "Foreground layered slab")))

fern((-41, -23, 0), 78, .22, 90201)
fern((53, 29, 0), 65, 1.10, 90202)
for i, point in enumerate(((-90, 36, 0), (92, -33, 0), (20, 72, 0), (-13, -81, 0))):
    grass(point, .8, i * 1.7, 90220 + i)
flower((80, 60, 0), 1.1, 90244)
flower((-70, 66, 0), .85, 90245)
finalize("SM_GardenUnderstory", ["M_Root", "M_Leaf", "M_LeafLight", "M_LeafGold", "M_RootDark"],
    "Ground center; ferns/sedge/gold flowers; always noncolliding; place in planting beds away from practice targets")

for i in range(22):
    a = i * 2.39996
    r = math.sqrt((i + .4) / 22)
    point = (math.cos(a) * r * 188, math.sin(a) * r * 100, 0)
    grass(point, .58 + (i % 4) * .12, a + .8, 91200 + i)
finalize("SM_GardenGrassDrift", ["M_LeafLight", "M_LeafGold"],
    "Ground center; broad low sedge drift; always noncolliding; cross-court approach lanes stay clear")

# Branches support actual overlapping curled leaves in several depth layers.
# This replaces the previous thin, flat single crown with a hanging bough.
groups = [(-310, -64, 86, 105), (-214, 132, 154, 118), (-85, -160, 183, 112),
    (45, 44, 226, 131), (175, 151, 164, 115), (284, -101, 132, 136),
    (390, 39, 61, 116), (-44, 205, 86, 94), (159, -245, 81, 103)]
for i, (x, y, z, spread) in enumerate(groups):
    tube("Forked hanging bough", [(0, 0, -76), (x * .32, y * .23, 7),
        (x * .66, y * .68, z * .83), (x, y, z - 20)], [16, 11, 5, 1.5], "M_Root", 8)
    rng = random.Random(92300 + i)
    for j in range(104):
        angle = j * 2.399963 + rng.uniform(-.3, .3)
        radius = math.sqrt(rng.random()) * spread
        center = Vector((x + math.cos(angle) * radius, y + math.sin(angle) * radius * .78,
            z + rng.uniform(-44, 43) - .18 * radius))
        direction = Vector((math.cos(angle + .3), math.sin(angle + .3), rng.uniform(-.32, .35)))
        mat = "M_Leaf" if j % 6 < 3 else "M_LeafLight" if j % 6 < 5 else "M_LeafGold"
        leaf("Overlapping bough foliage", center, direction, rng.uniform(37, 59), rng.uniform(16, 27),
            mat, rng.uniform(-.12, .18), .30)
finalize("SM_GardenBough", ["M_Root", "M_Leaf", "M_LeafLight", "M_LeafGold"],
    "Branch attachment center; foliage above/around origin; always noncolliding; lowest limb at local Z-76")

metadata = {name: entry["meta"] for name, entry in ASSETS.items()}
(OUT / "asset-metadata.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
audit_exports()

# Representative source-art vignette; not claimed as an Unreal screenshot.
for asset in ASSETS.values():
    asset["object"].hide_render = True


def place(name, location, yaw=0, scale=1):
    obj = ASSETS[name]["object"].copy()
    obj.data = ASSETS[name]["object"].data
    obj.name = "REVIEW_" + name
    scene.collection.objects.link(obj)
    obj.location = location
    obj.rotation_euler.z = math.radians(yaw)
    obj.scale = (scale,) * 3 if isinstance(scale, (float, int)) else scale
    obj.hide_render = False
    return obj


references = {}
with bpy.data.libraries.load(str(GAME / "art/Dreambound_Kit.blend"), link=False) as (source, target):
    target.objects = [name for name in ("SM_Arch", "SM_StoneTile", "SM_StoneTile_B", "SM_BellTree", "SM_Bell", "SM_Wall") if name in source.objects]
for obj in target.objects:
    if obj is None:
        continue
    name = obj.name.split(".")[0]
    obj.data = obj.data.copy()
    scene.collection.objects.link(obj)
    obj.name = "REVIEW_REFERENCE_" + name
    obj.hide_set(False)
    obj.hide_render = True
    for i, mat in enumerate(obj.data.materials):
        if mat.name.split(".")[0] in MATS:
            obj.data.materials[i] = MATS[mat.name.split(".")[0]]
    references[name] = obj


def original(name, loc, yaw=0, scale=1):
    obj = references[name].copy()
    obj.data = references[name].data
    scene.collection.objects.link(obj)
    obj.location = loc
    obj.rotation_euler.z = math.radians(yaw)
    obj.scale = (scale,) * 3 if isinstance(scale, (float, int)) else scale
    obj.hide_render = False
    return obj


for x in range(-5, 6):
    for y in range(-5, 8):
        original("SM_StoneTile" if (x + y) % 2 else "SM_StoneTile_B", (x * 198, y * 198, -24), (x * 17 + y * 7) % 4 * 90)
for x in (-830, 0, 830):
    place("SM_GardenCliffBank", (x, 1180, 0), 0, (.95, 1.1, 1 + x / 6500))
for x in (-740, 240, 830):
    place("SM_GardenButtress", (x, 710, 0), 0, (1, 1, 1.15 if x == -740 else 1))
original("SM_Arch", (-250, 770, 0), 0, (1.8, 1.3, 1.35))
original("SM_Wall", (-270, 866, 580), 0, (2.9, 1.5, .7))
original("SM_BellTree", (640, 540, 0), -20, 1.05)
original("SM_Bell", (600, 400, 344), -10, 1.45)
place("SM_GardenBough", (630, 540, 620), -10, 1.2)
place("SM_GardenBough", (-590, -300, 590), 42, 1.3)
place("SM_GardenCliffBank", (-1330, -90, 0), 90, (1.3, 1.1, 1.22))
place("SM_GardenRockCluster", (810, 130, 0), 17, 1.3)
place("SM_GardenRockCluster", (-660, -290, 0), -32, 1.1)
for i in range(27):
    side = -1 if i % 2 else 1
    x = side * (340 + (i % 4) * 100)
    y = -680 + (i // 2) * 127
    place("SM_GardenUnderstory" if i % 3 else "SM_GardenGrassDrift", (x, y, 0), i * 79, .85 + (i % 4) * .1)
world = bpy.data.worlds.new("Garden studio sky")
scene.world = world
world.use_nodes = True
world.node_tree.nodes["Background"].inputs["Color"].default_value = (.16, .21, .23, 1)
world.node_tree.nodes["Background"].inputs["Strength"].default_value = .42
sun_data = bpy.data.lights.new("Warm late sun", "SUN")
sun_data.energy = 2.7
sun_data.angle = .10
sun_data.color = (1, .80, .53)
sun = bpy.data.objects.new("Warm late sun", sun_data)
scene.collection.objects.link(sun)
sun.rotation_euler = (math.radians(26), math.radians(-31), math.radians(-28))
camera_data = bpy.data.cameras.new("Garden inspection")
camera = bpy.data.objects.new("Garden inspection", camera_data)
scene.collection.objects.link(camera)
scene.camera = camera
scene.render.engine = "CYCLES"
scene.cycles.samples = 40
scene.cycles.use_denoising = True
scene.render.resolution_x = 1440
scene.render.resolution_y = 960
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.view_settings.view_transform = "AgX"
scene.view_settings.exposure = .6


def render(name, loc, target, lens=28):
    camera.location = loc
    camera.rotation_euler = (Vector(target) - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera.data.type = "PERSP"
    camera.data.lens = lens
    camera.data.clip_end = 12000
    scene.render.filepath = str(OUT / name)
    bpy.ops.render.render(write_still=True)


if "--skip-renders" not in sys.argv:
    render("garden-source-vignette.png", (-35, -1360, 170), (-80, 680, 250), 27)
    render("garden-source-detail.png", (890, -870, 260), (200, 440, 230), 36)
portable_source()
# Windows scanners can briefly hold the previous blend during Blender's own
# backup rename. Save a fresh file, then replace our generated source explicitly.
blend_export = OUT / "Bellroot_Garden.export.blend"
bpy.ops.wm.save_as_mainfile(filepath=str(blend_export), compress=True)
for attempt in range(20):
    try:
        blend_export.replace(OUT / "Bellroot_Garden.blend")
        break
    except PermissionError:
        if attempt == 19:
            raise
        time.sleep(.25)
print("GARDEN_ART_COMPLETE " + json.dumps({name: {"triangles": meta["triangles"], "dimensions_cm": meta["dimensions_cm"], "materials": meta["materials"]} for name, meta in metadata.items()}))

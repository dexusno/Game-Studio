"""Original Mara forge gun, rear salvage claw and Cinderwall depth extension.

Blender 5.2.1. The existing original Cinderwall helpers supply primitive finishing,
PBR surfaces and FBX rig export conventions; its generator is not modified/run.
All new geometry, proportions and keyframes below are authored for this game.
"""
import importlib.util
import json
import math
from pathlib import Path

import bpy
from mathutils import Matrix, Vector

spec = importlib.util.spec_from_file_location("cinderwall_original_helpers", Path(__file__).with_name("build_cinderwall.py"))
h = importlib.util.module_from_spec(spec)
spec.loader.exec_module(h)
OUT = h.OUT
h.VERSION = "mara-art-v001"
box, cyl, rod, ring, bolt, pipe = h.box, h.cyl, h.rod, h.ring, h.bolt, h.pipe


def tube(name, loc, length, outer, inner, mat, group, segments=48):
    """Closed annular bore with an actual interior; axis +X, original UVs."""
    verts, faces = [], []
    x, y, z = loc
    for dx, radius in ((-length / 2, outer), (length / 2, outer),
                       (-length / 2, inner), (length / 2, inner)):
        for i in range(segments):
            a = math.tau * i / segments
            verts.append((x + dx, y + math.cos(a) * radius, z + math.sin(a) * radius))
    for i in range(segments):
        j = (i + 1) % segments
        faces.extend(((i, j, segments + j, segments + i),
                      (2 * segments + j, 2 * segments + i, 3 * segments + i, 3 * segments + j),
                      (j, i, 2 * segments + i, 2 * segments + j),
                      (segments + i, segments + j, 3 * segments + j, 3 * segments + i)))
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    uv = mesh.uv_layers.new(name="UV0")
    for polygon in mesh.polygons:
        for loop in polygon.loop_indices:
            v = mesh.vertices[mesh.loops[loop].vertex_index].co
            uv.data[loop].uv = ((v.x - x) * 1.7, math.atan2(v.z - z, v.y - y) / math.tau * 2)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return h.mesh_finish(obj, name, mat, group, .008, True)


def gun():
    prefix = "MG_"
    g = lambda name: prefix + name
    bones = [h.socket("root", (0, 0, 0), None), h.socket("cradle", (-.30, 0, .96), "root"),
             h.socket("recoil", (-.18, 0, 1.46), "cradle"), h.socket("breech", (-.53, 0, 1.78), "recoil"),
             h.socket("feed", (-.92, 0, 1.39), "cradle"), h.socket("vent", (-.54, 0, 2.01), "recoil"),
             h.socket("wheel", (-.37, -.63, .96), "cradle"), h.socket("muzzle", (1.85, 0, 1.48), "recoil"),
             h.socket("load", (-.97, 0, 1.42), "feed"), h.socket("eject", (-.62, .51, 1.50), "recoil"),
             h.socket("shield_attach", (.08, -.68, .62), "root"), h.socket("operator_attach", (-1.32, -.23, .02), "root")]
    # Low two-rail sled: no bulky pedestal obstructing the shot.
    for side in (-1, 1):
        y = side * .52
        box("Mara_sledge_runner", (-.38, y, .115), (2.10, .28, .23), "iron", g("root"), .065)
        box("Mara_runner_edge", (-.38, y - side * .145, .20), (1.93, .035, .065), "edge", g("root"), .009)
        for x in (-1.22, .47):
            box("Mara_anchor_foot", (x, y, .08), (.37, .47, .16), "soot", g("root"), .035)
            bolt("Mara_anchor_bolt", (x, y, .17), g("root"), "Z", .065)
        box("Mara_cast_cheek", (-.34, side * .44, .63), (1.08, .21, .85), "oxide", g("root"), .14)
        box("Mara_cheek_inset", (-.35, side * .552, .52), (.64, .022, .31), "soot", g("root"), .035)
        rod("Mara_diagonal_strut", (-.92, y, .26), (-.42, y, .88), .074, "iron", g("root"))
        rod("Mara_diagonal_strut", (.40, y, .26), (-.19, y, .88), .074, "iron", g("root"))
        cyl("Mara_trunnion", (-.30, side * .58, .97), .28, .17, "iron", g("cradle"), "Y", 40, .025)
        ring("Mara_trunnion_lip", (-.30, side * .677, .97), .22, .028, "edge", g("cradle"), "Y")
        for a in range(0, 360, 60):
            t = math.radians(a)
            bolt("Mara_hub_bolt", (-.30 + .155 * math.cos(t), side * .675, .97 + .155 * math.sin(t)), g("cradle"), "Y", .032)
    box("Mara_sledge_bridge", (-.38, 0, .27), (1.51, .9, .14), "iron", g("root"), .025)
    # Open cast cradle, separate sleeve and recoil cylinders.
    box("Mara_receiver_tray", (-.15, 0, 1.10), (1.80, .76, .20), "iron", g("cradle"), .055)
    for side in (-1, 1):
        y = side * .46
        cyl("Mara_return_jacket", (.11, y, 1.20), .10, 1.04, "soot", g("cradle"), "X", 24)
        cyl("Mara_return_rod", (.51, y, 1.20), .046, .87, "edge", g("recoil"), "X", 24)
        for xx in (-.39, .58):
            ring("Mara_piston_gland", (xx, y, 1.20), .102, .024, "copper", g("cradle"), "X", 24)
    cyl("Mara_receiver_casting", (-.38, 0, 1.48), .49, 1.12, "iron", g("recoil"), "X", 12, .04)
    for x in (-.91, .15):
        tube("Mara_receiver_flange", (x, 0, 1.48), .13, .535, .26, "oxide", g("recoil"), 32)
        for a in range(0, 360, 45):
            t = math.radians(a)
            bolt("Mara_receiver_fastener", (x + .075, .445 * math.cos(t), 1.48 + .445 * math.sin(t)), g("recoil"), "X", .042)
    # Visible near-side small furnace window, surrounded by dimensional castwork.
    box("Mara_furnace_frame", (-.40, -.493, 1.48), (.65, .12, .49), "oxide", g("recoil"), .095)
    box("Mara_furnace_gasket", (-.40, -.563, 1.48), (.50, .027, .34), "soot", g("recoil"), .068)
    box("Mara_furnace_window", (-.40, -.580, 1.48), (.43, .013, .27), "furnace", g("recoil"), .055)
    for x in (-.52, -.38, -.24):
        box("Mara_window_guard", (x, -.607, 1.48), (.028, .035, .29), "iron", g("recoil"), .009)
    for x in (-.66, -.14):
        for z in (1.29, 1.67):
            bolt("Mara_window_screw", (x, -.574, z), g("recoil"), radius=.022)
    # Barrel is tapered in stepped sleeves, with negative space along its rails.
    tube("Mara_barrel_sleeve", (.67, 0, 1.48), 1.16, .315, .194, "iron", g("recoil"))
    tube("Mara_forward_bore", (1.43, 0, 1.48), .63, .244, .171, "soot", g("recoil"))
    tube("Mara_muzzle_crown", (1.74, 0, 1.48), .20, .294, .178, "edge", g("recoil"))
    tube("Mara_bore_liner", (1.57, 0, 1.48), .39, .186, .168, "copper", g("recoil"))
    cyl("Mara_bore_dark_depth", (.53, 0, 1.48), .184, .02, "soot", g("recoil"), "X")
    for x, radius in ((.25, .335), (.82, .328), (1.32, .269)):
        tube("Mara_cast_barrel_band", (x, 0, 1.48), .11, radius, radius - .055, "ochre", g("recoil"), 32)
        for a in range(0, 360, 60):
            t = math.radians(a)
            cyl("Mara_band_stud", (x, (radius + .018) * math.cos(t), 1.48 + (radius + .018) * math.sin(t)), .027, .13, "edge", g("recoil"), "X", 6, .003)
    for a in (40, 140, 220, 320):
        t = math.radians(a)
        y, z = .355 * math.cos(t), 1.48 + .355 * math.sin(t)
        rod("Mara_cooling_rail", (.22, y, z), (1.19, y * .76, 1.48 + (z - 1.48) * .76), .028, "edge", g("recoil"))
    for i in range(5):
        x = .38 + i * .13
        box("Mara_barrel_side_fin", (x, -.337, 1.49), (.031, .10, .34), "iron", g("recoil"), .01)
    # Loading door and guide: an assembled round enters the rear chamber.
    box("Mara_breech_hinge_lid", (-.55, 0, 1.89), (.69, .79, .15), "oxide", g("breech"), .055)
    for side in (-1, 1):
        cyl("Mara_breech_hinge", (-.82, side * .39, 1.81), .07, .14, "edge", g("breech"), "Y", 20)
    box("Mara_breech_handle", (-.50, 0, 2.018), (.33, .065, .06), "soot", g("breech"), .02)
    box("Mara_feed_slide", (-1.09, 0, 1.18), (.65, .48, .12), "edge", g("feed"), .02)
    for side in (-1, 1):
        box("Mara_feed_guide", (-1.08, side * .23, 1.26), (.69, .043, .11), "iron", g("feed"), .015)
    cyl("Mara_feed_ram", (-1.26, 0, 1.41), .151, .34, "copper", g("feed"), "X", 32, .02)
    tube("Mara_breech_rear_ring", (-.99, 0, 1.48), .10, .38, .18, "edge", g("recoil"), 40)
    # Service system: large readable lines, small restrained accents.
    pipe("Mara_copper_pressure_line", ((-.85, .47, 1.70), (-.89, .61, 1.67), (-.26, .61, 1.67), (.09, .45, 1.54)), .043, "copper", g("recoil"))
    pipe("Mara_nearside_return_line", ((-.90, -.35, 1.20), (-1.00, -.49, .95), (-.76, -.66, .65), (-.20, -.66, .65)), .040, "copper", g("cradle"))
    cyl("Mara_pressure_gauge_rim", (-.62, -.27, 2.05), .139, .065, "copper", g("recoil"), "Y", 32)
    cyl("Mara_pressure_gauge_face", (-.62, -.308, 2.05), .117, .011, "ceramic", g("recoil"), "Y", 32, 0)
    rod("Mara_gauge_needle", (-.62, -.32, 2.05), (-.67, -.32, 2.12), .007, "oxide", g("recoil"), 8)
    for a in (-65, -25, 15, 55, 95):
        t = math.radians(a)
        rod("Mara_gauge_tick", (-.62 + .08 * math.cos(t), -.32, 2.05 + .08 * math.sin(t)), (-.62 + .097 * math.cos(t), -.32, 2.05 + .097 * math.sin(t)), .004, "soot", g("recoil"), 6)
    cyl("Mara_pressure_vent", (-.36, .25, 2.05), .10, .26, "soot", g("vent"), verts=24)
    ring("Mara_vent_lip", (-.36, .25, 2.19), .105, .022, "copper", g("vent"), segments=24)
    ring("Mara_handwheel", (-.37, -.78, .97), .21, .025, "soot", g("wheel"), "Y")
    for a in range(0, 360, 120):
        t = math.radians(a)
        rod("Mara_handwheel_spoke", (-.37, -.78, .97), (-.37 + .19 * math.cos(t), -.78, .97 + .19 * math.sin(t)), .018, "edge", g("wheel"))
    cyl("Mara_handwheel_grip", (-.22, -.83, 1.09), .025, .16, "oxide", g("wheel"), "Y", 16)
    rig = h.make_rig("MaraGun", prefix, bones)
    rig["armature"]["forward"] = "+X"
    del rig["armature"]["death_cleanup"]
    h.action(rig, "idle", 2, {"vent": [(1, (0, 0, .008), (0, 0, 0))]}, loop=True)
    h.action(rig, "load", .70, {
        "breech": [(.16, (0, 0, .02), (0, -40, 0)), (.36, (0, 0, .02), (0, -40, 0))],
        "feed": [(.14, (-.13, 0, 0), (0, 0, 0)), (.38, (.26, 0, 0), (0, 0, 0)), (.52, (.26, 0, 0), (0, 0, 0))],
        "wheel": [(.35, (0, 0, 0), (0, 55, 0))],
    }, [{"t": .38, "event": "cosmetic_breech_seat"}])
    h.action(rig, "unload", .60, {
        "breech": [(.12, (0, 0, .02), (0, -40, 0)), (.34, (0, 0, .02), (0, -40, 0))],
        "feed": [(.25, (-.25, 0, 0), (0, 0, 0)), (.40, (-.25, 0, 0), (0, 0, 0))],
    })
    h.action(rig, "fire", .34, {
        "recoil": [(.067, (-.22, 0, 0), (0, 0, 0)), (.16, (-.14, 0, 0), (0, 0, 0)), (.34, (-.07, 0, 0), (0, 0, 0))],
        "cradle": [(.067, (0, 0, 0), (0, -2.7, 0)), (.34, (0, 0, 0), (0, -.8, 0))],
        "vent": [(.13, (0, 0, .09), (0, 0, 0)), (.34, (0, 0, .06), (0, 0, 0))],
    }, [{"t": 0, "event": "committed_shot_flash"}])
    h.action(rig, "recovery", .48, {
        "recoil": [(0, (-.07, 0, 0), (0, 0, 0)), (.28, (.008, 0, 0), (0, 0, 0))],
        "cradle": [(0, (0, 0, 0), (0, -.8, 0))],
        "vent": [(0, (0, 0, .06), (0, 0, 0))],
    })
    return rig


def claw():
    prefix = "MC_"
    g = lambda name: prefix + name
    bones = [h.socket("root", (0, 0, 0), None), h.socket("trolley", (0, 0, 5.73), "root"),
             h.socket("cables", (0, 0, 5.54), "trolley"), h.socket("head", (0, 0, 3.28), "trolley"),
             h.socket("grab", (0, 0, 2.82), "head"), h.socket("dump", (.9, 0, 1.55), "root")]
    for i in range(3):
        a = math.tau * i / 3
        bones.append(h.socket("finger_" + str(i), (.18 * math.cos(a), .18 * math.sin(a), 3.20), "head"))
    box("Mara_claw_trolley", (0, 0, 5.78), (1.13, .88, .28), "oxide", g("trolley"), .075)
    for x in (-.39, .39):
        for y in (-.42, .42):
            cyl("Mara_trolley_wheel", (x, y, 5.93), .13, .13, "soot", g("trolley"), "Y", 24)
            bolt("Mara_trolley_axle", (x, y * 1.16, 5.93), g("trolley"), radius=.051)
    cyl("Mara_winch_drum", (0, 0, 5.50), .225, .68, "iron", g("trolley"), "Y", 32)
    for y in (-.34, .34):
        cyl("Mara_winch_flange", (0, y, 5.50), .277, .06, "ochre", g("trolley"), "Y", 32)
    for y in (-.12, .12):
        rod("Mara_wire_rope", (0, y, 5.54), (0, y, 3.42), .020, "edge", g("cables"), 12)
    cyl("Mara_grab_swivel", (0, 0, 3.36), .17, .33, "iron", g("head"), verts=32)
    cyl("Mara_grab_crown", (0, 0, 3.19), .30, .26, "oxide", g("head"), verts=24, bevel=.035)
    ring("Mara_grab_hub_band", (0, 0, 3.32), .272, .031, "ochre", g("head"), segments=32)
    for i in range(3):
        a = math.tau * i / 3
        radial = Vector((math.cos(a), math.sin(a), 0))
        group = g("finger_" + str(i))
        p0 = radial * .18 + Vector((0, 0, 3.20))
        p1 = radial * .62 + Vector((0, 0, 2.80))
        p2 = radial * .43 + Vector((0, 0, 2.53))
        rod("Mara_claw_cast_jaw", p0, p1, .10, "oxide", group, 12)
        rod("Mara_claw_hardened_hook", p1, p2, .08, "edge", group, 12)
        rod("Mara_jaw_return_link", radial * .27 + Vector((0, 0, 3.30)), radial * .48 + Vector((0, 0, 2.96)), .039, "copper", group)
        h.sphere("Mara_jaw_pin", p0, (.13, .13, .11), "iron", group, 16)
        h.sphere("Mara_jaw_contact", p2, (.10, .10, .045), "soot", group, 16)
    rig = h.make_rig("MaraClaw", prefix, bones)
    rig["armature"]["forward"] = "rear collection; +X traverse toward furnace hopper"
    del rig["armature"]["death_cleanup"]
    h.action(rig, "idle", 2, {"head": [(1, (0, 0, .01), (0, 0, .6))]}, loop=True)
    entries = {"head": [(.30, (0, 0, -1.60), (0, 0, 0)), (.43, (0, 0, -1.60), (0, 0, 0)), (.76, (0, 0, -.12), (0, 0, -5)), (1.06, (0, 0, -.12), (0, 0, 0))],
               "trolley": [(.76, (0, 0, 0), (0, 0, 0)), (.96, (.95, 0, 0), (0, 0, 0)), (1.10, (.95, 0, 0), (0, 0, 0))]}
    for i in range(3):
        a = math.tau * i / 3
        r = (-28 * math.sin(a), 28 * math.cos(a), 0)
        entries["finger_" + str(i)] = [(.29, (0, 0, 0), (0, 0, 0)), (.43, (0, 0, 0), r), (.92, (0, 0, 0), r), (1.07, (0, 0, 0), (0, 0, 0))]
    h.action(rig, "collect", 1.4, entries, [{"t": .43, "event": "grab_cosmetic_scrap"}, {"t": 1.06, "event": "dump_cosmetic_scrap"}])
    # Scale the telescoping wire with the head travel so neither end floats.
    cable = rig["armature"].pose.bones["cables"]
    for t, extension in ((0, 0), (.30, 1.60), (.43, 1.60), (.76, .12), (1.06, .12), (1.4, 0)):
        cable.scale = (1, 1 + extension / 2.12, 1)
        cable.keyframe_insert("scale", frame=1 + round(t * 30), group="cables")
    return rig


def extension():
    # Closed dimensional architecture, not a camera-facing image plane.
    box("CW_apron", (0, 5, -.74), (65, 60, .45), "stone", "Depth_apron", .06)
    for x in (-14, 14):
        group = "Depth_sidehall_" + str(x)
        box("CW_sidehall_mass", (x, 8, 3.1), (5.5, 16, 6.2), "stone", group, .10)
        box("CW_sidehall_roof", (x, 8, 6.25), (5.8, 16.4, .28), "iron", group, .05)
        for y in (1, 5, 9, 13):
            box("CW_sidehall_pilaster", (x - math.copysign(2.8, x), y, 3.0), (.35, .45, 6), "iron", group, .035)
    for i, (x, y, width, height) in enumerate(((-11, 20, 7, 12), (-3, 23, 6, 15), (4, 20, 6, 10), (11, 23, 7, 17), (20, 29, 9, 13), (-23, 31, 9, 17))):
        group = "Depth_foundry_" + str(i)
        box("CW_far_foundry", (x, y, height / 2), (width, 7, height), "stone", group, .09)
        box("CW_far_foundry_cornice", (x, y, height), (width + .5, 7.5, .3), "iron", group, .04)
        for xx in (-width * .32, 0, width * .32):
            for z in (height * .42, height * .72):
                box("CW_far_window_frame", (x + xx, y - 3.55, z), (.86, .17, 1.52), "iron", group, .04)
                box("CW_far_window", (x + xx, y - 3.65, z), (.65, .035, 1.24), "cool" if i % 3 else "furnace", group, .015)
                box("CW_far_window_mullion", (x + xx, y - 3.69, z), (.055, .04, 1.26), "iron", group, .005)
        cyl("CW_far_smokestack", (x + width * .32, y + 1, height + 1.6), .60, 5.5, "iron", group, verts=20, bevel=.025)
        ring("CW_far_stack_lip", (x + width * .32, y + 1, height + 4.36), .63, .085, "soot", group, segments=20)
    group = "Depth_hopper"
    # A visible receiving funnel beneath the claw's brief rearward dump.
    for side in (-1, 1):
        box("Mara_hopper_side", (-5.79, 1 + side * .43, 1.10), (1.15, .09, 1.40), "iron", group, .04)
    box("Mara_hopper_back", (-6.34, 1, 1.10), (.09, .88, 1.40), "oxide", group, .04)
    box("Mara_hopper_front", (-5.25, 1, .79), (.09, .88, .78), "oxide", group, .04)
    box("Mara_hopper_dark", (-5.79, 1, .87), (1.06, .77, .10), "soot", group, .02)
    box("Mara_hopper_warning_band", (-5.19, 1, .93), (.025, .72, .17), "ochre", group, .01)
    # Separate reusable payload mesh is attached to the grab socket, never loot.
    for i in range(7):
        a = i * 2.399
        p = (.17 * math.cos(a), .15 * math.sin(a), .06 * (i % 3))
        if i % 2:
            ring("Mara_scrap_ring", p, .12, .034, "edge", "Payload", "X", 16)
        else:
            box("Mara_scrap_offcut", p, (.24, .08, .05), "oxide", "Payload", .009, (a, a * .4, a * .7))
    meshes = []
    for group, objects in list(h.GROUPS.items()):
        if not group.startswith("Depth_") and group != "Payload":
            continue
        name = "SM_Mara_" + group.lower()
        mesh = h.combine(objects, name)
        mins = Vector(tuple(min(v.co[a] for v in mesh.data.vertices) for a in range(3)))
        maxs = Vector(tuple(max(v.co[a] for v in mesh.data.vertices) for a in range(3)))
        origin = Vector(((mins.x + maxs.x) / 2, (mins.y + maxs.y) / 2, mins.z)) if group != "Payload" else Vector((0, 0, 0))
        for vertex in mesh.data.vertices:
            vertex.co -= origin
        mesh.location = origin
        meshes.append(mesh)
    return meshes


def export_assets(stage_meshes):
    for name, rig in h.RIGS.items():
        arm, mesh = rig["armature"], rig["mesh"]
        arm.animation_data.action = None
        for bone in arm.pose.bones:
            bone.matrix_basis = Matrix.Identity(4)
        h.set_selection([arm, mesh])
        settings = dict(use_selection=True, object_types={"ARMATURE", "MESH"}, add_leaf_bones=False,
                        use_armature_deform_only=False, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                        axis_forward="-Y", axis_up="Z", path_mode="RELATIVE", embed_textures=False)
        bpy.ops.export_scene.fbx(filepath=str(OUT / "meshes" / (name + ".fbx")), bake_anim=False, **settings)
        for act in [a for a in bpy.data.actions if a.name.startswith(rig["prefix"])]:
            arm.animation_data.action = act
            bpy.context.scene.frame_start = 1
            bpy.context.scene.frame_end = 1 + round(h.CLIPS[act.name]["duration_s"] * 30)
            bpy.ops.export_scene.fbx(filepath=str(OUT / "animations" / (act.name + ".fbx")), bake_anim=True,
                                   bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
                                   bake_anim_simplify_factor=0, **settings)
        arm.animation_data.action = None
        for bone in arm.pose.bones:
            bone.matrix_basis = Matrix.Identity(4)
    h.set_selection(stage_meshes)
    bpy.ops.export_scene.fbx(filepath=str(OUT / "meshes/MaraStageExtension.fbx"), use_selection=True, object_types={"MESH"},
                            bake_anim=False, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS", axis_forward="-Y", axis_up="Z", path_mode="RELATIVE")


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1
    scene.render.fps = 30
    h.materials()
    gun()
    claw()
    stage_meshes = extension()
    export_assets(stage_meshes)
    bpy.context.view_layer.update()
    report = {"version": h.VERSION, "blender": bpy.app.version_string, "units": "metres", "fps": 30,
              "props": {}, "clips": h.CLIPS, "palette_dependency": "cinderwall-art-v001 original PBR surfaces",
              "stage": {"instances": []}, "provenance": "Original Game Studio / Codex geometry, rig and keyframes; no external models, textures or image generations.",
              "forward": "+X gun; Unreal conversion (x,-y,z)*100", "normal_convention": "OpenGL +Y; flip green on UE import"}
    for name, rig in h.RIGS.items():
        mesh = rig["mesh"].data
        mesh.calc_loop_triangles()
        report["props"][name] = {"vertices": len(mesh.vertices), "triangles": len(mesh.loop_triangles), "bones": len(rig["bones"]),
                               "materials": [m.name for m in mesh.materials], "rest_bounds_m": list(rig["mesh"].dimensions), "bone_definitions": rig["bones"], "rigid_weights": True}
    for obj in stage_meshes:
        obj.data.calc_loop_triangles()
        report["stage"]["instances"].append({"name": obj.name, "position_m": list(obj.location), "triangles": len(obj.data.loop_triangles),
                                             "bounds_m": list(obj.dimensions), "spawn": "payload" not in obj.name})
    (OUT / "asset-report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    # Source-review rig framing is kept separate from the actual Unreal evidence.
    for obj in stage_meshes:
        obj.hide_render = True
    h.RIGS["MaraClaw"]["armature"].location.x = -4
    h.RIGS["MaraGun"]["armature"].animation_data.action = bpy.data.actions["MG_idle"]
    h.RIGS["MaraClaw"]["armature"].animation_data.action = bpy.data.actions["MC_idle"]
    scene.frame_set(1)
    scene.world = bpy.data.worlds.new("Mara_source_review")
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs["Color"].default_value = (.10, .15, .22, 1)
    scene.world.node_tree.nodes["Background"].inputs["Strength"].default_value = .5
    h.light("key", (-1, -4, 6), (1, .76, .55), 1500, 5)
    h.light("rim", (2, 3, 4), (.43, .67, 1), 1600, 4)
    h.light("soft_front", (3, -5, 2), (.7, .8, 1), 400, 3)
    scene.camera = h.camera("MaraGun_source_review", (4.4, -6.8, 3.6), (.1, 0, 1.1), 55)
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 32
    scene.cycles.use_denoising = True
    prefs = bpy.context.preferences.addons["cycles"].preferences
    prefs.compute_device_type = "OPTIX"
    prefs.get_devices()
    for device in prefs.devices:
        device.use = device.type == "OPTIX"
    scene.cycles.device = "GPU"
    scene.view_settings.view_transform = "AgX"
    scene.render.resolution_x, scene.render.resolution_y, scene.render.resolution_percentage = 1400, 900, 100
    for image in bpy.data.images:
        if image.source == "FILE":
            image.pack()
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "Mara_Source.blend"))
    if not h.OPT.no_render:
        h.RIGS["MaraClaw"]["mesh"].hide_render = True
        scene.render.filepath = str(OUT / "renders/gun-source-review.png")
        bpy.ops.render.render(write_still=True)
        h.RIGS["MaraClaw"]["mesh"].hide_render = False
        h.RIGS["MaraGun"]["mesh"].hide_render = True
        scene.camera = h.camera("MaraClaw_source_review", (-.5, -9, 5.0), (-4, 0, 4.3), 45)
        scene.render.filepath = str(OUT / "renders/claw-source-review.png")
        bpy.ops.render.render(write_still=True)
    print("MARA_ART_READY " + str(OUT / "asset-report.json"))


if __name__ == "__main__":
    main()

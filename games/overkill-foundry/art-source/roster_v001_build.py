"""Original articulated machinery for the eight remaining enabled Cinderwall robots.

Blender 5.2.1. Uses only this project's original primitive/PBR/rig helpers.
No downloaded geometry, generated images, embedded combat rules or new roster IDs.
Source renders are review material, never evidence of in-game visual acceptance.
"""
import hashlib
import importlib.util
import json
import math
from pathlib import Path

import bpy
from mathutils import Matrix

spec = importlib.util.spec_from_file_location("cw_roster_helpers", Path(__file__).with_name("build_cinderwall.py"))
h = importlib.util.module_from_spec(spec)
spec.loader.exec_module(h)
OUT = h.OUT
h.VERSION = "cinderwall-roster-v001"
box, cyl, rod, ring, bolt, pipe = h.box, h.cyl, h.rod, h.ring, h.bolt, h.pipe
META = {}


def skeleton(name, prefix, source_id, height, pivots):
    bones = [h.socket("root", (0, 0, 0), None), h.socket("hull", (0, 0, height * .45), "root")]
    for bone, pivot, parent in pivots:
        bones.append(h.socket(bone, pivot, parent))
    bones.extend([h.socket("core", (.12, 0, height * .52), "hull"),
                  h.socket("muzzle", (-1.15, 0, height * .55), "hull"),
                  h.socket("intent", (0, 0, height + .3), "root")])
    META[name] = {"source_id": source_id, "prefix": prefix, "bones": bones}
    return lambda bone: prefix + bone


def finish(name):
    m = META[name]
    muzzle = {"CableBinder": ((-1.20, -.76, 1.11), "arm_L"),
              "PressureCask": ((-.98, 0, 1.24), "valve"),
              "CoilNest": ((-1.42, 0, .82), "feeder"),
              "KnucklePress": ((-1.40, -.76, 1.27), "fist_L"),
              "RedlinePursuer": ((-1.39, -.61, 1.51), "gun_L"),
              "FoilWarden": ((-1.71, .60, 1.69), "lance"),
              "SplitChassis": ((-1.10, -.57, 1.56), "gun_L"),
              "GatebreakerPrime": ((-1.75, -1.11, 1.63), "hammer_L")}[name]
    for bone in m["bones"]:
        if bone["name"] == "muzzle":
            bone["pivot"], bone["parent"] = list(muzzle[0]), muzzle[1]
    return h.make_rig(name, m["prefix"], m["bones"])


def hub(label, loc, radius, group, axis="Y", paint="ochre"):
    cyl(label + "_bearing", loc, radius, .15, "iron", group, axis, 24)
    xyz = list(loc)
    xyz[{"X": 0, "Y": 1, "Z": 2}[axis]] -= .09
    ring(label + "_rim", xyz, radius * .78, .025, paint, group, axis, 24)
    bolt(label + "_axle", xyz, group, axis, radius * .25)


def vent(label, loc, size, group, paint="iron", count=6):
    x, y, z = loc
    sx, sy, sz = size
    box(label + "_recess", loc, size, "soot", group, .02)
    for i in range(count):
        box(label + "_slat", (x - sx * .42 + sx * .84 * i / (count - 1), y - sy * .6, z),
            (sx / (count * 2), .045, sz * 1.06), paint, group, .009, (0, -.18, 0))


def optic(label, loc, radius, group, tint="optic"):
    x, y, z = loc
    cyl(label + "_socket", (x, y, z), radius * 1.35, .09, "iron", group, "X", 24)
    h.sphere(label + "_lens", (x - .058, y, z), (.04, radius, radius), tint, group, 24)
    box(label + "_brow", (x, y, z + radius * 1.22), (.18, radius * 2.7, .065), "iron", group, .012)


def hydraulic(label, a, b, group, radius=.06):
    mid = tuple(a[i] * .44 + b[i] * .56 for i in range(3))
    rod(label + "_sleeve", a, mid, radius * 1.8, "iron", group)
    rod(label + "_rod", mid, b, radius, "edge", group)
    h.sphere(label + "_ball", a, (radius * 1.9,) * 3, "copper", group, 16)


def gauge(label, loc, radius, group):
    x, y, z = loc
    cyl(label + "_rim", loc, radius, .07, "copper", group, "Y", 24)
    cyl(label + "_dial", (x, y - .041, z), radius * .80, .013, "ceramic", group, "Y", 24, 0)
    box(label + "_needle", (x - radius * .12, y - .055, z + radius * .18),
        (.013, .009, radius * 1.15), "oxide", group, .001, (0, .46, 0))


def bore(label, loc, length, outer, inner, group):
    """Actual open barrel wall and recessed dark breech; no painted front plug."""
    x, y, z = loc
    segments, verts, faces = 32, [], []
    for dx, radius in ((-length / 2, outer), (length / 2, outer), (-length / 2, inner), (length / 2, inner)):
        for i in range(segments):
            t = math.tau * i / segments
            verts.append((x + dx, y + math.cos(t) * radius, z + math.sin(t) * radius))
    for i in range(segments):
        j = (i + 1) % segments
        faces.extend(((i, j, segments + j, segments + i),
                      (2 * segments + j, 2 * segments + i, 3 * segments + i, 3 * segments + j),
                      (j, i, 2 * segments + i, 2 * segments + j),
                      (segments + i, segments + j, 3 * segments + j, 3 * segments + i)))
    mesh = bpy.data.meshes.new(label + "_Geo")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    uv = mesh.uv_layers.new(name="UV0")
    for polygon in mesh.polygons:
        for loop in polygon.loop_indices:
            v = mesh.vertices[mesh.loops[loop].vertex_index].co
            uv.data[loop].uv = ((v.x-x)*2, math.atan2(v.z-z, v.y-y)/math.tau*2)
    obj = bpy.data.objects.new(label, mesh)
    bpy.context.collection.objects.link(obj)
    h.mesh_finish(obj, label, "iron", group, .009, True)
    cyl(label + "_recessed_breech", (x + length * .40, y, z), inner, .015, "soot", group, "X", 32, 0)


def feet(g, prefix, span, length, high=.62):
    for side in (-1, 1):
        b = "leg_L" if side < 0 else "leg_R"
        y = side * span
        box(prefix + "_foot", (-.15, y, .15), (length, .49, .29), "iron", g(b), .06)
        box(prefix + "_toe", (-length * .49, y, .19), (.24, .52, .31), "edge", g(b), .055)
        hydraulic(prefix + "_ankle", (.10, y, .27), (.2, y * .72, high), g(b), .085)
        hub(prefix + "_ankle_pin", (.16, y, high * .75), .17, g(b))


def cable_binder():
    name = "CableBinder"
    g = skeleton(name, "CB_", "C1-R04", 2.55, [
        ("leg_L", (.2, -.48, .6), "root"), ("leg_R", (.2, .48, .6), "root"),
        ("head", (-.08, 0, 1.95), "hull"), ("spool", (.43, 0, 1.42), "hull"),
        ("arm_L", (-.06, -.64, 1.54), "hull"), ("arm_R", (-.06, .64, 1.54), "hull")])
    feet(g, "Binder", .54, 1.04)
    h.riveted_box("Binder_backbone", (.25, 0, 1.25), (.66, .62, 1.46), "patina", g("hull"), .09)
    cyl("Binder_cable_drum", (.39, 0, 1.38), .63, .76, "soot", g("spool"), "Y", 40)
    for side in (-1, 1):
        y = side * .44
        cyl("Binder_spool_cheek", (.39, y, 1.38), .69, .095, "ochre", g("spool"), "Y", 40)
        for i in range(9):
            a = i * math.tau / 9
            bolt("Binder_spool_fastener", (.39 + .52 * math.cos(a), y + side * .06, 1.38 + .52 * math.sin(a)), g("spool"))
        arm = g("arm_L" if side < 0 else "arm_R")
        hub("Binder_shoulder", (-.05, side * .65, 1.55), .22, arm)
        hydraulic("Binder_tendon", (-.04, side * .7, 1.51), (-.72, side * .75, 1.16), arm, .055)
        box("Binder_feed_jaw", (-.79, side * .76, 1.10), (.54, .30, .29), "iron", arm, .05)
        for n in range(4):
            cyl("Binder_cable_tip", (-1.09, side * .76 + (n - 1.5) * .063, 1.11), .023, .19, "copper", arm, "X", 12)
        pipe("Binder_feed_loop", ((.35, side * .62, 1.5), (.1, side * .88, 1.94), (-.5, side * .85, 1.79), (-.84, side * .75, 1.25)), .046, "soot", arm)
    box("Binder_sensor_neck", (-.16, 0, 1.99), (.42, .40, .39), "iron", g("head"), .07)
    optic("Binder_optic", (-.40, 0, 2.07), .12, g("head"), "cool")
    rod("Binder_antenna", (.02, .18, 2.17), (.14, .18, 2.51), .025, "copper", g("head"))
    vent("Binder_flank", (.27, -.33, .80), (.45, .03, .24), g("hull"))
    finish(name)


def pressure_cask():
    name = "PressureCask"
    g = skeleton(name, "PC_", "C1-R05", 2.35, [
        ("leg_L", (0, -.55, .5), "root"), ("leg_R", (0, .55, .5), "root"),
        ("lid", (0, 0, 1.91), "hull"), ("valve", (-.65, 0, 1.16), "hull"),
        ("flywheel", (.1, -.71, 1.23), "hull")])
    feet(g, "Cask", .56, 1.10, .60)
    cyl("Cask_pressure_vessel", (.08, 0, 1.18), .68, 1.34, "oxide", g("hull"), "Z", 48, .06)
    for z in (.59, 1.05, 1.75):
        ring("Cask_pressure_band", (.08, 0, z), .69, .062, "iron", g("hull"), "Z", 40)
    for angle in range(0, 360, 45):
        a = math.radians(angle)
        rod("Cask_tie_rod", (.08 + .68 * math.cos(a), .68 * math.sin(a), .56),
            (.08 + .68 * math.cos(a), .68 * math.sin(a), 1.84), .036, "edge", g("hull"))
    h.sphere("Cask_domed_lid", (.08, 0, 1.86), (.69, .69, .25), "iron", g("lid"), 32)
    cyl("Cask_safety_vent", (.25, .19, 2.09), .105, .43, "copper", g("lid"), "Z", 24)
    ring("Cask_valve_handwheel", (.25, .19, 2.32), .23, .025, "oxide", g("lid"), "Z", 24)
    for a in range(0, 360, 90):
        t = math.radians(a)
        rod("Cask_wheel_spoke", (.25, .19, 2.32), (.25 + .22 * math.cos(t), .19 + .22 * math.sin(t), 2.32), .017, "iron", g("lid"))
    bore("Cask_outlet_cast", (-.77, 0, 1.24), .40, .27, .19, g("valve"))
    ring("Cask_outlet_lip", (-.955, 0, 1.24), .235, .045, "copper", g("valve"), "X")
    optic("Cask_pressure_eye", (-.54, -.29, 1.77), .10, g("hull"))
    gauge("Cask_gauge", (-.1, -.70, 1.54), .19, g("hull"))
    ring("Cask_flywheel", (.11, -.76, 1.04), .34, .045, "ochre", g("flywheel"), "Y")
    for a in range(0, 360, 60):
        t = math.radians(a)
        rod("Cask_flywheel_spoke", (.11, -.77, 1.04), (.11 + .31 * math.cos(t), -.77, 1.04 + .31 * math.sin(t)), .026, "iron", g("flywheel"))
    pipe("Cask_return_line", ((.55, -.41, .83), (.83, -.45, 1.10), (.73, -.43, 1.85), (.15, -.42, 1.94)), .04, "copper", g("hull"))
    finish(name)


def coil_nest():
    name = "CoilNest"
    pivots = [("lid_L", (0, -.45, 1.35), "hull"), ("lid_R", (0, .45, 1.35), "hull"),
              ("feeder", (-.6, 0, .90), "hull"), ("coil", (.25, 0, 1.77), "hull")]
    for i in range(4):
        pivots.append(("leg" + str(i), ((i // 2 - .5) * 1.25, (-1 if i % 2 == 0 else 1) * .62, .67), "root"))
    g = skeleton(name, "CN_", "C1-R07", 2.25, pivots)
    # An actual open chamber supplies visual continuity with the committed Mite
    # spawn. The two shutters lift above it and the feeder extends through it.
    box("Nest_vault_floor", (.1, 0, .71), (1.85, 1.44, .19), "iron", g("hull"), .065)
    box("Nest_vault_rear", (.97, 0, 1.05), (.14, 1.44, .85), "iron", g("hull"), .045)
    box("Nest_brood_shadow", (.885, 0, 1.08), (.02, 1.10, .59), "soot", g("hull"), .025)
    for side in (-1, 1):
        box("Nest_vault_wall", (.1, side * .65, 1.05), (1.85, .15, .85), "iron", g("hull"), .055)
        rod("Nest_slide_rail", (-.81, side * .40, .83), (.87, side * .40, .83), .027, "edge", g("hull"))
    for i in range(4):
        side = -1 if i % 2 == 0 else 1
        x = (i // 2 - .5) * 1.25
        hydraulic("Nest_leg", (x, side * .58, .77), (x + .18, side * 1.02, .26), g("leg" + str(i)), .095)
        box("Nest_foot", (x + .12, side * 1.0, .13), (.51, .42, .25), "iron", g("leg" + str(i)), .06)
    for side in (-1, 1):
        group = g("lid_L" if side < 0 else "lid_R")
        box("Nest_split_shutter", (.08, side * .37, 1.50), (1.87, .69, .19), "patina", group, .09)
        for x in (-.55, -.18, .2, .57):
            box("Nest_shutter_rib", (x, side * .38, 1.61), (.09, .67, .12), "iron", group, .02)
            bolt("Nest_shutter_pin", (x, side * .73, 1.52), group)
        pipe("Nest_copper_feed", ((.75, side * .65, .94), (.98, side * .60, 1.22), (.64, side * .51, 1.74), (.26, side * .36, 1.79)), .055, "copper", g("hull"))
    for z in (1.74, 1.88, 2.02, 2.16):
        ring("Nest_induction_loop", (.28, 0, z), .39, .036, "copper", g("coil"), "Z", 32)
    cyl("Nest_coil_core", (.28, 0, 1.95), .19, .51, "cool", g("coil"), "Z", 24)
    box("Nest_launch_ramp", (-1.02, 0, .82), (.77, 1.0, .11), "ochre", g("feeder"), .035, (0, -.15, 0))
    for side in (-1, 1):
        optic("Nest_watch_eye", (-.89, side * .58, 1.28), .09, g("hull"), "cool")
    finish(name)


def knuckle_press():
    name = "KnucklePress"
    g = skeleton(name, "KP_", "C1-R08", 2.55, [
        ("leg_L", (.15, -.49, .71), "root"), ("leg_R", (.15, .49, .71), "root"),
        ("jaw", (-.05, 0, 1.60), "hull"), ("fist_L", (-.42, -.66, 1.32), "hull"),
        ("fist_R", (-.42, .66, 1.32), "hull"), ("head", (.15, 0, 2.16), "hull")])
    feet(g, "Press", .58, 1.24, .83)
    h.riveted_box("Press_back_casting", (.50, 0, 1.55), (.64, 1.14, 1.62), "ochre", g("hull"), .12)
    box("Press_upper_C_arm", (-.14, 0, 2.23), (1.39, 1.05, .40), "ochre", g("hull"), .085)
    box("Press_lower_C_arm", (-.22, 0, .95), (1.46, 1.13, .29), "iron", g("hull"), .065)
    cyl("Press_vertical_ram", (-.46, 0, 1.85), .18, .63, "edge", g("jaw"), "Z", 32)
    box("Press_square_die", (-.49, 0, 1.53), (.65, .75, .31), "iron", g("jaw"), .045)
    for side in (-1, 1):
        grp = g("fist_L" if side < 0 else "fist_R")
        hub("Press_shoulder", (.10, side * .67, 1.68), .23, grp)
        hydraulic("Press_horizontal_piston", (.06, side * .69, 1.55), (-.91, side * .77, 1.34), grp, .10)
        box("Press_forged_fist", (-1.02, side * .76, 1.27), (.55, .46, .51), "oxide", grp, .075)
        for z in (1.13, 1.26, 1.39):
            box("Press_knuckle", (-1.32, side * .76, z), (.11, .48, .083), "edge", grp, .02)
    optic("Press_optic", (-.77, 0, 2.23), .12, g("head"))
    vent("Press_rear_vent", (.59, -.585, 1.56), (.48, .04, .73), g("hull"), count=5)
    gauge("Press_gauge", (.5, -.61, 2.01), .13, g("hull"))
    finish(name)


def redline_pursuer():
    name = "RedlinePursuer"
    g = skeleton(name, "RP_", "C1-O01", 2.50, [
        ("wheel_L", (.44, -.57, .49), "root"), ("wheel_R", (.44, .57, .49), "root"),
        ("fork", (-.83, 0, .52), "hull"), ("head", (-.51, 0, 1.74), "hull"),
        ("gun_L", (-.25, -.62, 1.55), "hull"), ("gun_R", (-.25, .62, 1.55), "hull"),
        ("exhaust", (.80, 0, 1.79), "hull")])
    box("Pursuer_low_motor", (.03, 0, .93), (2.03, .86, .73), "oxide", g("hull"), .18)
    box("Pursuer_sloped_tank", (.14, 0, 1.51), (1.28, .79, .47), "iron", g("hull"), .13, (0, -.12, 0))
    for side in (-1, 1):
        grp = g("wheel_L" if side < 0 else "wheel_R")
        cyl("Pursuer_drive_tire", (.45, side * .61, .50), .49, .34, "soot", grp, "Y", 40, .06)
        hub("Pursuer_drive_hub", (.45, side * .82, .50), .29, grp, paint="oxide")
        for a in range(0, 360, 30):
            t = math.radians(a)
            box("Pursuer_tread", (.45 + .45 * math.cos(t), side * .61, .50 + .45 * math.sin(t)), (.17, .38, .07), "iron", grp, .016, (0, -t, 0))
        gun = g("gun_L" if side < 0 else "gun_R")
        box("Pursuer_gun_housing", (-.41, side * .61, 1.42), (.85, .28, .40), "oxide", gun, .055)
        for n in (-1, 1):
            cyl("Pursuer_barrel", (-1.04, side * .61, 1.42 + n * .09), .060, .58, "iron", gun, "X", 24)
            ring("Pursuer_bore", (-1.35, side * .61, 1.42 + n * .09), .050, .017, "copper", gun, "X", 24)
        vent("Pursuer_cooling", (.40, side * .44, 1.15), (.74, .04, .31), g("hull"), count=7)
        cyl("Pursuer_swept_pipe", (.84, side * .30, 1.95), .10, .93, "iron", g("exhaust"), "Z", 24)
        ring("Pursuer_pipe_lip", (.84, side * .30, 2.43), .10, .024, "copper", g("exhaust"))
    cyl("Pursuer_front_wheel", (-.83, 0, .45), .42, .27, "soot", g("fork"), "Y", 40, .055)
    hydraulic("Pursuer_fork", (-.67, -.22, 1.01), (-.87, -.22, .48), g("fork"), .06)
    box("Pursuer_sensor", (-.50, 0, 1.79), (.55, .51, .38), "ochre", g("head"), .075)
    optic("Pursuer_optic", (-.80, 0, 1.81), .12, g("head"))
    finish(name)


def foil_warden():
    name = "FoilWarden"
    g = skeleton(name, "FW_", "C1-O02", 2.92, [
        ("leg_L", (.05, -.42, .88), "root"), ("leg_R", (.05, .42, .88), "root"),
        ("head", (0, 0, 2.40), "hull"), ("lance", (-.22, .56, 1.63), "hull"),
        ("tile_0", (-.59, -.76, 1.00), "hull"), ("tile_1", (-.59, -.76, 1.56), "hull"),
        ("tile_2", (-.59, -.76, 2.12), "hull")])
    feet(g, "Warden", .42, .85, 1.03)
    h.riveted_box("Warden_torso", (.08, 0, 1.61), (.71, .77, 1.13), "patina", g("hull"), .095)
    for side in (-1, 1):
        hydraulic("Warden_spine", (.35, side * .37, 1.05), (.35, side * .37, 2.10), g("hull"), .053)
    # Three genuinely separate articulated ceramic tiles expose finite protection.
    for i, z in enumerate((1.00, 1.56, 2.12)):
        grp = g("tile_" + str(i))
        box("Warden_tile_back", (-.53, -.72, z), (.29, .78, .59), "iron", grp, .055, (0, .05, -.12))
        box("Warden_ablative_tile", (-.71, -.75, z), (.12, .74, .49), "ceramic", grp, .055, (0, .05, -.12))
        for side in (-1, 1):
            bolt("Warden_tile_pin", (-.79, -.75 + side * .26, z), grp, "X", .031)
    box("Warden_sensor_tower", (0, 0, 2.45), (.49, .53, .52), "iron", g("head"), .07)
    optic("Warden_cold_optic", (-.28, 0, 2.50), .12, g("head"), "cool")
    box("Warden_crown", (.08, 0, 2.76), (.70, .65, .14), "copper", g("head"), .035)
    cyl("Warden_lance_actuator", (-.15, .60, 1.69), .14, .93, "iron", g("lance"), "X", 24)
    rod("Warden_lance", (-.54, .60, 1.69), (-1.48, .60, 1.69), .047, "edge", g("lance"))
    h.cone("Warden_lance_tip", (-1.57, .60, 1.69), .105, 0, .26, "copper", g("lance"), "X", 16)
    vent("Warden_vent", (.1, -.405, 1.62), (.46, .025, .48), g("hull"), count=5)
    finish(name)


def split_chassis():
    name = "SplitChassis"
    g = skeleton(name, "SC_", "C1-O03", 2.63, [
        ("half_L", (0, -.43, 1.08), "hull"), ("half_R", (0, .43, 1.08), "hull"),
        ("track_L", (.15, -.86, .43), "root"), ("track_R", (.15, .86, .43), "root"),
        ("gun_L", (-.64, -.67, 1.53), "half_L"), ("gun_R", (-.64, .67, 1.53), "half_R"),
        ("head", (.05, 0, 2.12), "hull")])
    box("Split_carrier_bridge", (.15, 0, .80), (1.66, 1.69, .29), "iron", g("hull"), .07)
    box("Split_brood_chamber", (-.15, 0, 1.21), (1.1, .64, .69), "soot", g("hull"), .12)
    for side in (-1, 1):
        track = g("track_L" if side < 0 else "track_R")
        box("Split_track_belt", (.14, side * .85, .42), (1.99, .44, .71), "soot", track, .25)
        for x in (-.51, 0, .51, .85):
            hub("Split_roller", (x, side * 1.09, .42), .24, track, paint="patina")
        half = g("half_L" if side < 0 else "half_R")
        box("Split_shell_half", (.18, side * .50, 1.46), (1.66, .71, 1.13), "oxide", half, .15)
        for x in (-.42, .04, .5, .92):
            box("Split_shell_seam", (x, side * .52, 2.035), (.068, .68, .07), "iron", half, .012)
        gun = g("gun_L" if side < 0 else "gun_R")
        bore("Split_forward_bore", (-.82, side * .57, 1.56), .51, .25, .165, gun)
        ring("Split_bore_lip", (-1.065, side * .57, 1.56), .21, .044, "copper", gun, "X")
        vent("Split_heat_sink", (.22, side * .879, 1.46), (.75, .045, .49), half, count=7)
        cyl("Split_thermal_chimney", (.63, side * .49, 2.12), .13, .69, "iron", half, "Z", 24)
        ring("Split_chimney_glow", (.63, side * .49, 2.43), .111, .018, "ember", half)
    box("Split_sensor_bridge", (-.20, 0, 2.17), (.77, .92, .21), "iron", g("head"), .055)
    for side in (-1, 1):
        optic("Split_eye", (-.61, side * .21, 2.16), .085, g("head"))
    finish(name)


def gatebreaker():
    name = "GatebreakerPrime"
    g = skeleton(name, "GP_", "C1-B01", 3.80, [
        ("leg_L", (.18, -.80, 1.00), "root"), ("leg_R", (.18, .80, 1.00), "root"),
        ("head", (-.05, 0, 2.94), "hull"), ("arm_L", (-.10, -1.02, 2.33), "hull"),
        ("arm_R", (-.10, 1.02, 2.33), "hull"), ("hammer_L", (-.93, -1.10, 1.91), "arm_L"),
        ("hammer_R", (-.93, 1.10, 1.91), "arm_R"), ("reactor", (.53, 0, 2.44), "hull"),
        ("vent_L", (.50, -.62, 3.17), "hull"), ("vent_R", (.50, .62, 3.17), "hull")])
    feet(g, "Prime", .84, 1.69, 1.15)
    h.riveted_box("Prime_pelvic_cast", (.25, 0, 1.18), (1.47, 1.77, .51), "iron", g("hull"), .09)
    box("Prime_citadel_torso", (.16, 0, 2.16), (1.46, 1.68, 1.57), "oxide", g("hull"), .22)
    box("Prime_armor_brow", (-.39, 0, 2.68), (.54, 1.90, .44), "iron", g("hull"), .12)
    box("Prime_furnace_gasket", (-.60, 0, 2.13), (.08, .93, .83), "iron", g("hull"), .08)
    box("Prime_furnace_heart", (-.65, 0, 2.13), (.015, .70, .61), "furnace", g("hull"), .07)
    for y in (-.27, -.09, .09, .27):
        box("Prime_furnace_bar", (-.69, y, 2.13), (.09, .045, .67), "iron", g("hull"), .012)
    for side in (-1, 1):
        arm = g("arm_L" if side < 0 else "arm_R")
        hammer = g("hammer_L" if side < 0 else "hammer_R")
        hub("Prime_shoulder", (-.04, side * 1.02, 2.44), .39, arm, paint="ochre")
        hydraulic("Prime_arm_cylinder", (-.06, side * 1.12, 2.32), (-1.01, side * 1.15, 1.90), arm, .15)
        box("Prime_gate_hammer", (-1.09, side * 1.11, 1.64), (.79, .67, 1.07), "ochre", hammer, .12)
        box("Prime_hammer_face", (-1.54, side * 1.11, 1.63), (.17, .76, 1.02), "iron", hammer, .07)
        for z in (1.30, 1.63, 1.96):
            box("Prime_hammer_tooth", (-1.66, side * 1.11, z), (.15, .72, .15), "edge", hammer, .025)
        pipe("Prime_pressure_feed", ((.70, side * .65, 1.71), (1.02, side * .69, 2.0), (.82, side * .72, 2.80), (.07, side * .8, 2.92)), .08, "copper", g("hull"))
        vent("Prime_flank", (.29, side * .856, 2.12), (.76, .05, .83), g("hull"), count=7)
        vent_group = g("vent_L" if side < 0 else "vent_R")
        cyl("Prime_exhaust_stack", (.59, side * .57, 3.20), .16, 1.07 if side < 0 else .86, "iron", vent_group, "Z", 32)
        ring("Prime_stack_crown", (.59, side * .57, 3.735 if side < 0 else 3.63), .167, .035, "copper", vent_group)
    box("Prime_sensor_neck", (-.03, 0, 2.99), (.73, .66, .49), "iron", g("head"), .09)
    optic("Prime_optic", (-.435, 0, 3.04), .155, g("head"))
    cyl("Prime_reactor", (.89, 0, 2.39), .40, .73, "iron", g("reactor"), "X", 40)
    for x in (.65, .88, 1.11):
        ring("Prime_reactor_rib", (x, 0, 2.39), .41, .045, "copper", g("reactor"), "X")
    finish(name)


def action(name, suffix, duration, entries, cues=None, loop=False):
    return h.action(h.RIGS[name], suffix, duration, entries, cues, loop)


def stroke(t, x, z=0, pitch=0):
    return (t, (x, 0, z), (0, pitch, 0))


def common_clips(name, index):
    rig = h.RIGS[name]
    bones = {b["name"] for b in rig["bones"]}
    idle = {"hull": [stroke(.65, 0, .015, -.7), stroke(1.45, 0, -.006, .3)]}
    if "head" in bones:
        idle["head"] = [(.65, (0, 0, 0), (0, 0, 3)), (1.5, (0, 0, 0), (0, 0, -3))]
    action(name, "idle", 2, idle, loop=True)
    mass = 1.45 if name == "GatebreakerPrime" else 1
    for kind, dur, amount in (("light", .33, .025), ("medium", .5, .095), ("heavy", .7, .22)):
        entry = {"hull": [(.12, (amount / mass, 0, -.02), ((index % 3 - 1) * amount * 35, -amount * 48 / mass, 0))]}
        if "head" in bones:
            entry["head"] = [(.16, (0, 0, .01), (0, -amount * 20, amount * 19))]
        action(name, "hit_" + kind, dur, entry)
    # Different silhouettes disassemble differently, retaining every piece in one
    # skinned actor so the host can dissolve and destroy the entire machine.
    death = {"hull": [(.25, (.07, 0, .06), (0, -7, 0)), (.85, (.16, -.12, -.55), (-21, -12, -6)), (2, (.16, -.12, -.55), (-21, -12, -6))]}
    detach = {"CableBinder": ["spool", "head"], "PressureCask": ["lid", "valve"],
              "CoilNest": ["lid_L", "lid_R", "coil"], "KnucklePress": ["jaw", "fist_L"],
              "RedlinePursuer": ["fork", "exhaust"], "FoilWarden": ["tile_0", "tile_1", "tile_2", "head"],
              "SplitChassis": ["half_L", "half_R"], "GatebreakerPrime": ["reactor", "head", "hammer_L"]}[name]
    for n, bone in enumerate(detach):
        side = -1 if n % 2 == 0 else 1
        death[bone] = [(.35, (.10, side * .27, .56), (side * 25, 17, side * 28)),
                       (1.02, (.36, side * .55, -.43), (side * 70, 38, side * 74)),
                       (2, (.36, side * .55, -.43), (side * 70, 38, side * 74))]
    action(name, "death", 2, death, [{"t": .2, "event": "core_burst_cosmetic"}, {"t": .85, "event": "collapse_contact"},
                                    {"t": 1.1, "event": "dissolve_start_all_components"}, {"t": 2, "event": "destroy_actor_and_fx"}])
    action(name, "escape", 1.05, {"root": [stroke(.18, -.03), stroke(1.05, 4)], "hull": [stroke(.32, 0, .035, -3)]},
           [{"t": 1.05, "event": "despawn_without_death_fx"}])


def pulses(name, suffix, bones, times, duration, reach=.30, recoil=False):
    entry = {"hull": []}
    for i, t in enumerate(times):
        bone = bones[i % len(bones)]
        entry.setdefault(bone, []).extend([stroke(t - .12, .055, 0, -3), stroke(t, -reach, 0, 4), stroke(t + .1, .07, 0, -2)])
        entry["hull"].extend([stroke(t, .06 if recoil else -.045, 0, -2 if recoil else 2), stroke(t + .12, 0)])
    action(name, suffix, duration, entry, [{"t": t, "event": "display_committed_hit", "hit_index": i} for i, t in enumerate(times)])


def animate():
    for i, name in enumerate(h.RIGS):
        common_clips(name, i)
    pulses("CableBinder", "spray", ["arm_L", "arm_R"], [.29, .54], .88, .20)
    action("CableBinder", "foul", 1.1, {"spool": [(.3, (0, 0, 0), (0, 160, 0)), (.8, (0, 0, 0), (0, 300, 0))],
           "arm_L": [stroke(.45, -.25, .18, -12)], "arm_R": [stroke(.45, -.25, .18, 12)]}, [{"t": .48, "event": "fouling_cable_spark"}])
    pulses("CableBinder", "punch", ["arm_L", "arm_R"], [.38], .90, .53)
    pulses("PressureCask", "attack", ["valve"], [.38], .95, .22, True)
    action("PressureCask", "pressurize", 1.25, {"lid": [stroke(.7, 0, .09)], "flywheel": [(.6, (0, 0, 0), (0, 170, 0)), (1.0, (0, 0, 0), (0, 320, 0))],
           "hull": [stroke(.35, 0, .025, 1), stroke(.8, 0, .045, -2)]}, [{"t": .4, "event": "pressure_gain"}])
    action("CoilNest", "deploy", 1.3, {"lid_L": [(.36, (0, -.20, 0), (-38, 0, 0)), (.90, (0, -.20, 0), (-38, 0, 0))],
           "lid_R": [(.36, (0, .20, 0), (38, 0, 0)), (.90, (0, .20, 0), (38, 0, 0))], "feeder": [stroke(.5, -.36, -.09, -8)],
           "coil": [(.65, (0, 0, .12), (0, 0, 110))]}, [{"t": .66, "event": "display_committed_summon"}])
    pulses("CoilNest", "attack", ["feeder"], [.35], .87, .20, True)
    action("KnucklePress", "brace", 1.10, {"hull": [stroke(.5, .05, -.10, 4)], "fist_L": [stroke(.5, .20, .12, -12)],
           "fist_R": [stroke(.5, .20, .12, -12)], "jaw": [stroke(.5, 0, -.17)]}, [{"t": .5, "event": "armor_lock"}])
    pulses("KnucklePress", "double_punch", ["fist_L", "fist_R"], [.32, .61], 1.0, .48)
    action("KnucklePress", "slam", 1.10, {"jaw": [stroke(.22, 0, .26), stroke(.5, 0, -.48), stroke(.76, 0, -.08)],
           "hull": [stroke(.3, .03, .08, -5), stroke(.5, -.10, -.18, 8)], "fist_L": [stroke(.5, -.48)], "fist_R": [stroke(.5, -.48)]},
           [{"t": .50, "event": "display_committed_hit"}])
    pulses("RedlinePursuer", "strike", ["gun_L", "gun_R"], [.29], .8, .17, True)
    pulses("RedlinePursuer", "triple_strike", ["gun_L", "gun_R"], [.25, .43, .61], .97, .16, True)
    pulses("FoilWarden", "attack_9", ["lance"], [.38], .95, .37)
    pulses("FoilWarden", "attack_12", ["lance"], [.46], 1.1, .58)
    action("FoilWarden", "gain_drive", 1.20, {"hull": [stroke(.5, 0, .08, -3)], "head": [(.65, (0, 0, .06), (0, -8, 0))],
           "tile_0": [(.4, (0, -.08, 0), (-6, 0, 0))], "tile_1": [(.5, (0, -.08, 0), (-6, 0, 0))],
           "tile_2": [(.6, (0, -.08, 0), (-6, 0, 0))]}, [{"t": .6, "event": "drive_gain"}])
    pulses("SplitChassis", "double_strike", ["gun_L", "gun_R"], [.30, .56], .95, .17, True)
    pulses("SplitChassis", "triple_strike", ["gun_L", "gun_R"], [.26, .44, .62], 1.0, .17, True)
    action("SplitChassis", "thermal_runaway", 1.20, {"half_L": [(.5, (0, -.08, .06), (-9, 0, 0))],
           "half_R": [(.5, (0, .08, .06), (9, 0, 0))], "head": [stroke(.55, 0, .12, -9)]}, [{"t": .52, "event": "thermal_vent"}])
    action("SplitChassis", "death_release", 2.0, {"half_L": [(.3, (0, -.22, .20), (-25, 0, 0)), (.8, (0, -.64, -.50), (-72, 0, -9)), (2, (0, -.64, -.50), (-72, 0, -9))],
           "half_R": [(.3, (0, .22, .20), (25, 0, 0)), (.8, (0, .64, -.50), (72, 0, 9)), (2, (0, .64, -.50), (72, 0, 9))],
           "hull": [stroke(.8, .05, -.40, -8), stroke(2, .05, -.40, -8)]},
           [{"t": .42, "event": "display_committed_death_spawn"}, {"t": 1.1, "event": "dissolve_start_all_components"}, {"t": 2, "event": "destroy_actor_and_fx"}])
    pulses("GatebreakerPrime", "phase_one_strike", ["hammer_L", "hammer_R"], [.46], 1.10, .58)
    action("GatebreakerPrime", "recover", 1.45, {"hull": [stroke(.28, .12, -.32, 11), stroke(.96, .10, -.20, 6)],
           "head": [stroke(.5, 0, -.11, 17)], "arm_L": [stroke(.5, .14, -.20, 16)], "arm_R": [stroke(.5, .14, -.20, 16)]},
           [{"t": .28, "event": "recovery_joint_open"}])
    pulses("GatebreakerPrime", "quad_strike", ["hammer_L", "hammer_R"], [.27, .47, .67, .87], 1.25, .40)
    action("GatebreakerPrime", "charge", 1.30, {"hull": [stroke(.65, .08, -.12, 4)], "arm_L": [stroke(.8, .26, .10, -12)],
           "arm_R": [stroke(.8, .26, .10, -12)], "vent_L": [(.5, (0, 0, .06), (-15, 0, 0))],
           "vent_R": [(.5, (0, 0, .06), (15, 0, 0))], "reactor": [(.8, (0, 0, 0), (100, 0, 0))]}, [{"t": .4, "event": "boss_pressure_charge"}])
    action("GatebreakerPrime", "blast", 1.20, {"hammer_L": [stroke(.3, .12), stroke(.47, -.60), stroke(.60, .1)],
           "hammer_R": [stroke(.3, .12), stroke(.47, -.60), stroke(.60, .1)], "hull": [stroke(.3, .08, -.07, 4), stroke(.47, -.17, -.1, 9), stroke(.75, .04, .01, -3)],
           "reactor": [stroke(.5, .12, .06)]}, [{"t": .47, "event": "display_committed_hit"}, {"t": .58, "event": "steam_vent"}])


def export_assets():
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
        for clip_name in [c for c in h.CLIPS if c.startswith(rig["prefix"])]:
            arm.animation_data.action = bpy.data.actions[clip_name]
            bpy.context.scene.frame_start = 1
            bpy.context.scene.frame_end = 1 + round(h.CLIPS[clip_name]["duration_s"] * 30)
            bpy.ops.export_scene.fbx(filepath=str(OUT / "animations" / (clip_name + ".fbx")), bake_anim=True,
                                   bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
                                   bake_anim_simplify_factor=0, **settings)
        arm.animation_data.action = None
        for bone in arm.pose.bones:
            bone.matrix_basis = Matrix.Identity(4)


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system, scene.unit_settings.scale_length, scene.render.fps = "METRIC", 1, 30
    h.materials()
    for build in (cable_binder, pressure_cask, coil_nest, knuckle_press, redline_pursuer, foil_warden, split_chassis, gatebreaker):
        build()
    animate()
    game = Path(__file__).resolve().parents[1]
    manifest = json.loads((game / "content/cinderwall.manifest.json").read_text(encoding="utf-8"))
    expected = set(manifest["robot_ids"]) - {"C1-R01", "C1-R02"}
    if {m["source_id"] for m in META.values()} != expected:
        raise RuntimeError("Source roster differs from enabled manifest")
    for name, meta in META.items():
        meta["action_clips"] = {a: meta["prefix"] + a for a in manifest["robot_annotations"][meta["source_id"]]["action_ids"]}
        if any(clip not in h.CLIPS for clip in meta["action_clips"].values()):
            raise RuntimeError("Missing distinct action: " + name)
    export_assets()
    bpy.context.view_layer.update()
    report = {"version": h.VERSION, "blender": bpy.app.version_string, "units": "metres", "fps": 30,
              "forward": "-X; Unreal conversion (x,-y,z)*100", "robots": {}, "clips": h.CLIPS,
              "manifest_sha256": hashlib.sha256((game / "content/cinderwall.manifest.json").read_bytes()).hexdigest(),
              "source_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              "helper_sha256": hashlib.sha256(Path(__file__).with_name("build_cinderwall.py").read_bytes()).hexdigest(),
              "palette_dependency": "cinderwall-art-v001 original PBR surfaces, unchanged",
              "provenance": "Original Game Studio / Codex geometry, rigs and keyframes. No external art.",
              "limits": "Source art only. No in-engine action coverage, performance, quality or human approval established."}
    for name, rig in h.RIGS.items():
        mesh = rig["mesh"].data
        mesh.calc_loop_triangles()
        report["robots"][name] = {**META[name], "vertices": len(mesh.vertices), "triangles": len(mesh.loop_triangles),
                                  "materials": [m.name for m in mesh.materials], "rest_bounds_m": list(rig["mesh"].dimensions), "rigid_weights": True}
    (OUT / "asset-report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    scene.world = bpy.data.worlds.new("Roster_source_review")
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs["Color"].default_value = (.09, .13, .18, 1)
    scene.world.node_tree.nodes["Background"].inputs["Strength"].default_value = .6
    h.light("warm_key", (-4, -4, 7), (1, .77, .55), 1500, 5)
    h.light("cool_rim", (3, 3, 5), (.48, .67, 1), 1800, 4)
    h.light("front_fill", (-4, 2, 3), (.65, .8, 1), 600, 4)
    scene.render.engine = "CYCLES"
    scene.cycles.samples, scene.cycles.use_denoising = 32, True
    prefs = bpy.context.preferences.addons["cycles"].preferences
    prefs.compute_device_type = "OPTIX"
    prefs.get_devices()
    for device in prefs.devices:
        device.use = device.type == "OPTIX"
    scene.cycles.device = "GPU"
    scene.view_settings.view_transform = "AgX"
    scene.render.resolution_x, scene.render.resolution_y, scene.render.resolution_percentage = 1000, 1000, 100
    for image in bpy.data.images:
        if image.source == "FILE":
            image.pack()
    for rig in h.RIGS.values():
        rig["mesh"].hide_render = True
    for name, rig in h.RIGS.items():
        height = report["robots"][name]["rest_bounds_m"][2]
        scene.camera = h.camera(name + "_review", (-height * 1.60, -height * 2.15, height * 1.28), (0, 0, height * .49), 55)
        rig["armature"].animation_data.action = bpy.data.actions[rig["prefix"] + "idle"]
        rig["mesh"].hide_render = False
        scene.frame_set(1)
        if not h.OPT.no_render:
            scene.render.filepath = str(OUT / "renders" / (name + ".png"))
            bpy.ops.render.render(write_still=True)
        rig["mesh"].hide_render = True
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "Cinderwall_Roster_Source.blend"))
    print("ROSTER_SOURCE_READY " + json.dumps({"robots": len(report["robots"]), "clips": len(h.CLIPS), "triangles": sum(r["triangles"] for r in report["robots"].values())}))


if __name__ == "__main__":
    main()

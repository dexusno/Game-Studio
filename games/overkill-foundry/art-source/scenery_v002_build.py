"""Original dimensional Cinderwall scenery refinement, Blender 5.2.1.

The existing source helpers supply primitive/UV/PBR/export conventions. This
does not overwrite v001 or import another game's assets. All coordinates are
metres; the firing lane and existing Mara/robot origins are unchanged.
"""
import importlib.util
import json
import math
from pathlib import Path

import bpy
from mathutils import Vector

spec = importlib.util.spec_from_file_location("cw_helpers", Path(__file__).with_name("build_cinderwall.py"))
h = importlib.util.module_from_spec(spec)
spec.loader.exec_module(h)
OUT = h.OUT
VERSION = "cinderwall-scenery-v002"
box, cyl, rod, ring, pipe = h.box, h.cyl, h.rod, h.ring, h.pipe


def beam(name, a, b, width, depth, material, group):
    delta = Vector(b) - Vector(a)
    obj = box(name, (Vector(a) + Vector(b)) / 2, (width, depth, delta.length), material, group, .018)
    obj.rotation_euler = delta.to_track_quat("Z", "Y").to_euler()
    return obj


def rail(name, a, b, group, height=.8):
    delta = Vector(b) - Vector(a)
    count = max(1, round(delta.length / 1.3))
    for i in range(count + 1):
        p = Vector(a) + delta * i / count
        rod(name + "_post", p, p + Vector((0, 0, height)), .032, "iron", group, 10)
    for z in (.38, height):
        rod(name + "_rail", Vector(a) + Vector((0, 0, z)), Vector(b) + Vector((0, 0, z)), .03, "iron", group, 10)


def stack(x, y, base, height, radius, group):
    h.cone("SV_flue_taper", (x, y, base + height / 2), radius, radius * .77, height, "v2_slate", group, verts=24)
    for z in (base + .4, base + height * .50, base + height - .16):
        r = radius * (1 - .23 * (z - base) / height)
        ring("SV_flue_strap", (x, y, z), r + .012, .045, "iron", group, segments=24)
    cyl("SV_flue_dark_opening", (x, y, base + height + .012), radius * .73, .025, "soot", group, verts=24, bevel=0)


def factory(index, x, y, width, height, depth):
    group = "factory_%02d" % index
    box("SV_foundry_masonry", (x, y, height / 2 - 1.5), (width, depth, height + 2), "v2_stone", group, .08)
    front = y - depth / 2
    box("SV_factory_plinth", (x, front - .10, .05), (width + .45, .48, 1.25), "v2_slate", group, .035)
    box("SV_factory_cornice", (x, y, height - .5), (width + .5, depth + .5, .26), "iron", group, .045)
    columns = max(3, round(width / 1.8))
    for i in range(columns + 1):
        xx = x - width / 2 + i * width / columns
        box("SV_pilaster", (xx, front - .09, height * .48), (.16, .32, height - .8), "v2_slate", group, .014)
    for row, z in enumerate((height * .31, height * .61)):
        box("SV_lintel_band", (x, front - .15, z + .86), (width, .23, .16), "v2_slate", group, .012)
        for i in range(columns):
            xx = x - width / 2 + (i + .5) * width / columns
            box("SV_window_recess", (xx, front - .025, z), (width / columns - .36, .10, 1.4), "soot", group, .025)
            window_mat = "v2_window_warm" if (i + index + row) % 5 == 0 else "v2_window_cool"
            box("SV_dirty_glazing", (xx, front - .09, z), (width / columns - .52, .025, 1.20), window_mat, group, .005)
            box("SV_window_mullion", (xx, front - .12, z), (.065, .06, 1.24), "iron", group, .006)
            box("SV_window_transom", (xx, front - .12, z + .13), (width / columns - .50, .06, .065), "iron", group, .006)
    # Saw-tooth roof profiles expose real geometry through either camera.
    teeth = max(2, round(width / 2.7))
    for i in range(teeth):
        xx = x - width / 2 + (i + .5) * width / teeth
        panel = box("SV_roof_pitch", (xx, y, height + .12), (width / teeth + .12, depth + .35, .16), "v2_slate", group, .012)
        panel.rotation_euler.y = math.radians(18)
        box("SV_roof_light", (xx + width / teeth * .45, y, height + .15), (.10, depth * .87, .55), "v2_window_cool", group, .012)
    stack(x - width * .26, y + depth * .20, height, 3.1 + (index % 3) * 1.2, .42 + (index % 2) * .12, group)
    if index % 2 == 0:
        stack(x - width * .26 + 1.25, y + depth * .20, height, 4.1, .34, group)
    return group


def battlefield():
    group = "combat_deck"
    box("SV_bridge_mass", (0, .1, -.61), (23, 7.4, 1.1), "v2_stone", group, .06)
    # Large quiet plates: sparse wear and perimeter fasteners, no repeated bright
    # diagonal marks under every actor. Light catches actual seams/bevels.
    for i, x in enumerate(range(-10, 12, 2)):
        for j, y in enumerate((-2.35, 0, 2.35)):
            mat = "v2_floor" if (i + j) % 4 else "v2_slate"
            box("SV_cast_slab", (x, y, -.08), (1.98, 2.31, .16), mat, group, .012)
            for dx in (-.85, .85):
                for dy in (-1.02, 1.02):
                    cyl("SV_flush_fastener", (x + dx, y + dy, .008), .028, .012, "iron", group, verts=6, bevel=.001)
    for y in (-3.55, 3.6):
        box("SV_bridge_edge", (0, y, -.04), (23, .22, .32), "iron", group, .025)
        for x in (-10, -6, -2, 2, 6, 10):
            box("SV_bridge_joint", (x, y, .035), (.48, .30, .12), "v2_oxide", group, .015)
    # Beyond the walkable apron is a sunken service channel; it reveals depth
    # instead of filling the entire horizon with a glowing repeating wall.
    box("SV_channel_floor", (0, 8, -2.0), (38, 10, .4), "v2_slate", "channel", .05)
    box("SV_channel_far_wall", (0, 12.7, -.4), (36, .7, 3.2), "v2_stone", "channel", .045)
    for x in range(-15, 18, 3):
        box("SV_channel_buttress", (x, 12.1, -.3), (.6, 1.25, 3.5), "v2_stone", "channel", .06)
    pipe("SV_heat_main", ((-16, 6, -1), (-10, 6, -1), (0, 6, -1), (9, 6, -1), (16, 6, -1)), .36, "v2_copper", "channel")
    for x in range(-14, 16, 3):
        ring("SV_main_collar", (x, 6, -1), .37, .045, "iron", "channel", "X", 24)
    # A low rail behind the firing line provides scale without hiding robots.
    rail("SV_back_edge", (-10.8, 3.7, .04), (10.8, 3.7, .04), "back_rail", .62)


def near_machinery():
    group = "left_smelter"
    x, y = -8.5, 12.0
    box("SV_smelter_plinth", (x, y, .10), (6.2, 5.0, 1.6), "v2_stone", group, .08)
    cyl("SV_reactor_cast_body", (x, y, 3.0), 2.12, 5.2, "v2_oxide", group, verts=48, bevel=.05)
    h.cone("SV_reactor_shoulder", (x, y, 6.0), 2.13, 1.24, 1.05, "iron", group, verts=48)
    for z in (1.0, 3.8, 5.3):
        ring("SV_reactor_band", (x, y, z), 2.13, .11, "iron", group, segments=48)
    for a in (math.pi * i / 4 for i in range(8)):
        xx, yy = x + math.cos(a) * 2.08, y + math.sin(a) * 2.08
        rod("SV_reactor_tie", (xx, yy, 1.0), (xx, yy, 5.25), .10, "iron", group)
    # The single heat aperture is framed by a thick pressure door.
    box("SV_firebox_frame", (x, y - 2.04, 2.6), (2.0, .50, 2.22), "iron", group, .15)
    box("SV_firebox_recess", (x, y - 2.31, 2.6), (1.57, .07, 1.79), "soot", group, .12)
    box("SV_firebox_glow", (x, y - 2.36, 2.52), (1.21, .04, 1.32), "furnace", group, .10)
    for dx in (-.48, -.24, 0, .24, .48):
        box("SV_firebox_rib", (x + dx, y - 2.42, 2.6), (.055, .12, 1.78), "iron", group, .009)
    stack(x, y, 6.4, 4.0, .8, group)
    pipe("SV_hot_return", ((x + 1.4, y, 5.7), (x + 3.1, y, 5.7), (x + 3.1, y, 1.0), (x + 4.4, y, 1.0)), .28, "v2_copper", group)
    for z in (1.6, 3.5, 5.3):
        ring("SV_return_flange", (x + 3.1, y, z), .32, .06, "iron", group, segments=24)
    group = "right_accumulator"
    for i, (x, y) in enumerate(((8.7, 11), (10.9, 12.2))):
        height = 4.5 + i * 1.2
        cyl("SV_pressure_vessel", (x, y, height / 2), .93, height, "v2_copper", group, verts=32, bevel=.035)
        h.sphere("SV_dished_head", (x, y, height), (.92, .92, .40), "v2_copper", group)
        for z in (.4, height * .52, height - .4):
            ring("SV_vessel_strap", (x, y, z), .95, .075, "iron", group, segments=32)
        for dx in (-.70, .70):
            box("SV_tank_leg", (x + dx, y, .42), (.21, 1.2, .84), "iron", group, .025)
        pipe("SV_vessel_outlet", ((x, y, height + .2), (x, y, height + 1.1), (x + 1.8, y, height + 1.1)), .12, "iron", group)
    # A left-hand crane remains strictly behind the player/gun. The working
    # Mara claw/hopper are separate runtime assets; this is only its support.
    group = "rear_crane_support"
    for yy in (.9, 3.3):
        beam("SV_crane_leg", (-9.9, yy, 0), (-9.9, yy, 6.7), .32, .38, "iron", group)
        beam("SV_crane_knee", (-9.9, yy, 4.3), (-8.0, yy, 6.45), .16, .23, "v2_oxide", group)
    beam("SV_crane_jib", (-10.2, 1.3, 6.4), (-5.5, 1.3, 6.4), .32, .55, "v2_oxide", group)
    beam("SV_crane_backbrace", (-9.9, 3.3, 6.45), (-6.4, 1.3, 6.45), .18, .22, "iron", group)
    for x in (-9.6, -8.4, -7.2, -6.0):
        box("SV_jib_joint", (x, 1.0, 6.4), (.38, .05, .40), "ochre", group, .015)


def distant_city():
    # Deliberately uneven masses and gaps expose two subsequent depth planes.
    for args in ((0, -16, 21, 8, 9, 7), (1, -.8, 21, 9, 8.5, 7),
                 (2, 11, 24, 6, 12, 8), (3, -7, 34, 7, 13, 8),
                 (4, 5, 39, 9, 16, 9), (5, 21, 33, 9, 14, 8),
                 (6, -24, 38, 11, 17, 9), (7, 29, 18, 9, 12, 11),
                 (8, 39, 8, 8, 11, 13), (9, 47, -4, 11, 17, 14)):
        factory(*args)
    group = "high_service_bridge"
    box("SV_high_catwalk", (4.7, 18.2, 5.1), (10.2, 1.28, .22), "iron", group, .02)
    rail("SV_high_rail", (-.4, 17.52, 5.25), (9.8, 17.52, 5.25), group)
    for x in (0, 4.9, 9.8):
        beam("SV_walk_support", (x, 18.2, -.3), (x, 18.2, 5.05), .22, .30, "iron", group)
    for a, b in ((0, 4.9), (4.9, 9.8)):
        beam("SV_walk_diagonal", (a, 18.1, .8), (b, 18.1, 4.85), .15, .18, "iron", group)
    group = "distant_crane"
    for yy in (31.0, 33.2):
        beam("SV_tower_leg", (15, yy, 0), (15, yy, 16), .24, .24, "v2_slate", group)
    for z in (2, 5, 8, 11, 14):
        beam("SV_tower_lacing", (15, 31, z), (15, 33.2, z + 2), .11, .11, "iron", group)
    beam("SV_crane_skyline", (7, 32, 16), (21, 32, 16), .22, .28, "iron", group)
    for x in range(7, 21, 2):
        beam("SV_boom_lacing", (x, 32, 16), (x + 1, 32, 17), .085, .085, "iron", group)
        beam("SV_boom_lacing", (x + 1, 32, 17), (x + 2, 32, 16), .085, .085, "iron", group)
    rod("SV_distant_cable", (8.5, 32, 16), (8.5, 32, 8.2), .025, "iron", group, 8)
    ring("SV_distant_hook", (8.5, 32, 8.0), .24, .055, "iron", group, "Y", 16)
    box("SV_outer_apron", (20, 30, -2.5), (160, 140, 1), "v2_stone", "outer_apron", .08)


def consolidate():
    meshes = []
    for group, objects in list(h.GROUPS.items()):
        obj = h.combine(objects, "SM_SV2_" + group)
        low = Vector(tuple(min(v.co[a] for v in obj.data.vertices) for a in range(3)))
        high = Vector(tuple(max(v.co[a] for v in obj.data.vertices) for a in range(3)))
        origin = Vector(((low.x + high.x) / 2, (low.y + high.y) / 2, low.z))
        for vertex in obj.data.vertices:
            vertex.co -= origin
        obj.location = origin
        obj.data.calc_loop_triangles()
        meshes.append(obj)
    return meshes


def scene_review():
    # Append only the existing original heroes for true-scale source review;
    # they never enter the scenery FBX or its inventory.
    art_root = OUT.parent
    for path, names in ((art_root / "cinderwall-v001/Cinderwall_Source.blend", ("BreachRam", "RivetMite")),
                        (art_root / "mara-v001/Mara_Source.blend", ("MaraGun",))):
        if not path.exists():
            raise RuntimeError("True-scale source review requires original heroes: " + str(path))
        with bpy.data.libraries.load(str(path), link=False) as (source, target):
            target.objects = [name for name in source.objects if any(name in ("SK_" + n, "SKM_" + n) for n in names)]
        if len(target.objects) != len(names) * 2:
            raise RuntimeError("Missing hero mesh/rig in original source review")
        for obj in target.objects:
            if obj is not None:
                bpy.context.collection.objects.link(obj)
        for obj in target.objects:
            if obj is None or obj.type != "ARMATURE":
                continue
            if "BreachRam" in obj.name:
                obj.location = (3.6, .15, .015)
            elif "RivetMite" in obj.name:
                obj.location = (1.0, -.2, .015)
            elif "MaraGun" in obj.name:
                obj.location = (-3.4, 0, .02)
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 32
    scene.cycles.use_denoising = True
    prefs = bpy.context.preferences.addons["cycles"].preferences
    prefs.compute_device_type = "OPTIX"
    prefs.get_devices()
    for device in prefs.devices:
        device.use = device.type != "CPU"
    scene.cycles.device = "GPU"
    scene.world = bpy.data.worlds.new("SV2_city_air")
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs["Color"].default_value = (.18, .26, .35, 1)
    scene.world.node_tree.nodes["Background"].inputs["Strength"].default_value = .65
    h.light("SV2_soft_key", (-4, -5, 11), (1, .78, .55), 2600, 8, (0, 0, 0))
    h.light("SV2_sky_fill", (5, -2, 8), (.47, .68, 1), 1500, 7, (0, 1, 1))
    h.light("SV2_rim", (4, 6, 8), (1, .58, .29), 2600, 5, (0, 0, 1))
    h.light("SV2_smelter_glow", (-8.5, 9, 2.6), (1, .23, .04), 200, 2, (-8, 1, 1))
    # Source-only atmospheric volume. Runtime implements equivalent depth fog,
    # not a transparent backdrop or an opaque image covering the geometry.
    bpy.ops.mesh.primitive_cube_add(size=1, location=(15, 45, 18))
    air = bpy.context.object
    air.name = "REVIEW_ONLY_atmosphere"
    air.scale = (140, 80, 60)
    mist = bpy.data.materials.new("REVIEW_ONLY_air")
    mist.use_nodes = True
    mist.node_tree.nodes.clear()
    output = mist.node_tree.nodes.new("ShaderNodeOutputMaterial")
    volume = mist.node_tree.nodes.new("ShaderNodeVolumePrincipled")
    volume.inputs["Color"].default_value = (.46, .56, .66, 1)
    volume.inputs["Density"].default_value = .008
    volume.inputs["Anisotropy"].default_value = .15
    mist.node_tree.links.new(volume.outputs["Volume"], output.inputs["Volume"])
    air.data.materials.append(mist)
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure = .55
    scene.render.resolution_x, scene.render.resolution_y = 1600, 900
    scene.render.resolution_percentage = 100
    cameras = {
        "preparation-source": h.camera("SV2_prepare", (-1.3, -22.3, 7.4), (-1.3, .6, 2.6), 37),
        "action-source": h.camera("SV2_action", (-7.2, -11.5, 3.4), (.5, .2, 1.35), 37),
        "action-close-source": h.camera("SV2_action_close", (-7.0, -7.5, 2.9), (1.8, .15, 1.45), 37),
    }
    scene.frame_set(1)
    scene.camera = cameras["preparation-source"]
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "Cinderwall_Scenery_v002.blend"))
    if not h.OPT.no_render:
        for name, camera in cameras.items():
            scene.camera = camera
            scene.render.filepath = str(OUT / "renders" / (name + ".png"))
            bpy.ops.render.render(write_still=True)


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 1.0
    h.PALETTE.update({"v2_floor": ("494e4d", .38, .76), "v2_stone": ("665c50", .03, .89),
                      "v2_slate": ("40494e", .35, .76), "v2_oxide": ("824b38", .12, .69),
                      "v2_copper": ("87694b", .73, .57)})
    h.materials()
    for name, color, emission in (("v2_window_warm", "c78c51", .4), ("v2_window_cool", "78949c", .18)):
        mat = bpy.data.materials.new("M_CW_" + name)
        mat.use_nodes = True
        node = mat.node_tree.nodes.get("Principled BSDF")
        node.inputs["Base Color"].default_value = (*h.linear_rgb(color), 1)
        node.inputs["Roughness"].default_value = .66
        node.inputs["Emission Color"].default_value = (*h.linear_rgb(color), 1)
        node.inputs["Emission Strength"].default_value = emission
        h.MATS[name] = mat
    battlefield()
    near_machinery()
    distant_city()
    meshes = consolidate()
    h.set_selection(meshes)
    bpy.ops.export_scene.fbx(filepath=str(OUT / "meshes/CinderwallSceneryV002.fbx"), use_selection=True,
                             object_types={"MESH"}, bake_anim=False, apply_unit_scale=True,
                             apply_scale_options="FBX_SCALE_UNITS", axis_forward="-Y", axis_up="Z", path_mode="RELATIVE")
    report = {"version": VERSION, "blender": bpy.app.version_string, "units": "metres",
              "source_provenance": "Original Game Studio / Codex dimensional geometry and deterministic Blender materials; no external art.",
              "scene_contract": {"firing_lane_x": [-4.5, 10], "firing_lane_y": [-1.5, 1.5],
                                 "rear_gathering_x": [-8, -5], "replace_stage": "CinderwallGenerated",
                                 "replace_mara_depth_except": ["hopper", "payload"],
                                 "camera_conversion": "Unreal=(Blender.x,-Blender.y,Blender.z)*100"},
              "stage": {"mesh_count": len(meshes), "triangles": sum(len(o.data.loop_triangles) for o in meshes),
                        "instances": [{"name": o.name, "position_m": list(o.location),
                                       "bounds_m": list(o.dimensions), "triangles": len(o.data.loop_triangles),
                                       "materials": [m.name for m in o.data.materials]} for o in meshes]},
              "palette": h.PALETTE, "emissive": {"furnace": ["f35a17", .65],
                                                   "v2_window_warm": ["c78c51", .4], "v2_window_cool": ["78949c", .18]},
              "limits": "Source renders only. No rendered Unreal, performance or human visual acceptance yet."}
    (OUT / "asset-report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    scene_review()
    print("SCENERY_V002_READY " + json.dumps({"meshes": report["stage"]["mesh_count"], "triangles": report["stage"]["triangles"]}))


if __name__ == "__main__":
    main()

"""Import and render a bounded AI prop evaluation in the real Dreambound project.

Run the full UnrealEditor executable, NOT a commandlet or NullRHI:
  UnrealEditor.exe Dreambound.uproject -RenderOffscreen -unattended -nosplash
    -ExecutePythonScript=".../inspect_ai3d_unreal.py" -AI3DConfig=".../config.json"
    -abslog=".../Saved/AI3DInspection/editor.log"

The local JSON config requires fbx, base_color and metallic_roughness paths.
Optional: run_id (default waymarker-repaired-v1), front_yaw_degrees (direction
the imported front faces; default 90), height_cm (240), phase (all/import/
capture/material), settle_seconds (4), capture_size ([1920,1080]), instance_count (25).
The material-only repair phase can run in the full editor with -NullRHI; it
records material-usage.json and preserves the existing rendered report.
Outputs always go below this project's Saved/AI3DInspection/<run_id>. Imported
assets and the evaluation map stay below /Game/AI3DInspection/<run_id>.

For owner inspection, phase=view opens an interactive courtyard without tests
or captures and keeps the editor open. Omit -RenderOffscreen and -unattended.

No gameplay, project settings, external assets, or shared status is edited.
Existing /Game/Art meshes are referenced without changing or resaving them.
The saved map uses GameModeBase, preserving the real game's procedural maps.
These are rendered editor asset checks, not a packaged build or a fun verdict.

API references (checked against installed UE 5.8.2 source as well):
https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/AutomationLibrary
https://dev.epicgames.com/documentation/unreal-engine/API/Editor/StaticMeshEditor/UStaticMeshEditorSubsystem
"""

import hashlib
import json
import math
from pathlib import Path
import re
import statistics
import time
import traceback

import unreal


LIB = unreal.EditorAssetLibrary
EDIT = unreal.MaterialEditingLibrary
ASSETS = unreal.AssetToolsHelpers.get_asset_tools()
LEVEL = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ACTORS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
MESHES = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
REPORT = {"success": False, "complete": False, "warnings": [], "captures": []}
OUTPUT = None
CALLBACK = None
RUNNER = None
REPORT_NAME = "inspection.json"


def file_identity(path):
    path = Path(path).resolve()
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return {"path": str(path), "bytes": path.stat().st_size, "sha256": digest.hexdigest()}


def write_report():
    if OUTPUT:
        (OUTPUT / REPORT_NAME).write_text(json.dumps(REPORT, indent=2) + "\n", encoding="utf-8")


def vec(value):
    return [float(value.x), float(value.y), float(value.z)]


def configure():
    global OUTPUT, REPORT_NAME
    command = unreal.SystemLibrary.get_command_line()
    match = re.search(r'-AI3DConfig=(?:"([^"]+)"|(\S+))', command, re.IGNORECASE)
    if not match:
        raise RuntimeError("Pass -AI3DConfig with the local input JSON path")
    config_path = Path(match.group(1) or match.group(2)).resolve()
    config = json.loads(config_path.read_text(encoding="utf-8-sig"))
    run_id = config.get("run_id", "waymarker-repaired-v1")
    if not re.fullmatch(r"[a-zA-Z0-9_-]+", run_id):
        raise RuntimeError("run_id must contain only letters, numbers, underscores or hyphens")
    # Unreal package names cannot contain hyphens.
    config["asset_root"] = "/Game/AI3DInspection/" + run_id.replace("-", "_")
    config["map"] = config["asset_root"] + "/WaymarkerEvaluation"
    OUTPUT = Path(unreal.Paths.project_saved_dir()).resolve() / "AI3DInspection" / run_id
    OUTPUT.mkdir(parents=True, exist_ok=True)
    config.setdefault("height_cm", 240.0)
    config.setdefault("front_yaw_degrees", 90.0)
    config.setdefault("phase", "all")
    config.setdefault("instance_count", 25)
    config.setdefault("capture_size", [1920, 1080])
    config.setdefault("settle_seconds", 4.0)
    if not 1 <= config["instance_count"] <= 64:
        raise RuntimeError("Bounded inspection accepts 1â€“64 repeated copies")
    if config["phase"] not in ("all", "import", "capture", "material", "view"):
        raise RuntimeError("phase must be all, import, capture, material or view")
    if config["phase"] == "material":
        REPORT_NAME = "material-usage.json"
    REPORT.update({
        "run_id": run_id, "started_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "engine": unreal.SystemLibrary.get_engine_version(), "command_line": command,
        "script": file_identity(__file__), "config": config,
        "source_files": {key: file_identity(config[key]) for key in ("fbx", "base_color", "metallic_roughness")},
        "scope": config["asset_root"], "output": str(OUTPUT),
        "evidence_type": "Rendered UE editor asset evaluation; isolated map in actual Dreambound project",
        "limitations": [
            "No packaged-build, ordinary play, art-direction acceptance or whole-game performance claim.",
            "Repeated copies share one mesh/material but use separate StaticMeshActors; this does not validate the procedural HISM pipeline.",
            "Editor tick timings include Slate/editor overhead and are not GPU frame timings.",
            "Collision uses a coarse authored/proxy volume; detailed root traversal and player movement remain separate QA.",
        ],
    })
    build_file = Path(unreal.Paths.engine_dir()).resolve() / "Build" / "Build.version"
    if build_file.exists():
        REPORT["engine_build"] = json.loads(build_file.read_text(encoding="utf-8-sig"))
    project = Path(unreal.Paths.project_dir()).resolve()
    REPORT["editor_module"] = file_identity(project / "Binaries" / "Win64" / "UnrealEditor-Dreambound.dll")
    REPORT["uproject"] = file_identity(project / "Dreambound.uproject")
    write_report()
    return config


def checked_save(asset):
    if not asset.get_path_name().startswith("/Game/AI3DInspection/"):
        raise RuntimeError("Refusing to save an asset outside inspection scope")
    if not LIB.save_loaded_asset(asset):
        raise RuntimeError("Save failed: " + asset.get_path_name())


def import_texture(config, key, name, srgb):
    task = unreal.AssetImportTask()
    task.filename = config[key]
    task.destination_path = config["asset_root"] + "/Textures"
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = False
    ASSETS.import_asset_tasks([task])
    texture = LIB.load_asset(task.destination_path + "/" + name)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError("Texture import failed: " + key)
    texture.set_editor_property("srgb", srgb)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT if srgb else unreal.TextureCompressionSettings.TC_MASKS)
    texture.set_editor_property("max_texture_size", 0)
    texture.set_editor_property("lod_bias", 0)
    checked_save(texture)
    return texture


def expression(material, cls, x, y, **props):
    result = EDIT.create_material_expression(material, cls, x, y)
    if not result:
        raise RuntimeError("Failed to create material expression " + cls.__name__)
    for key, value in props.items():
        result.set_editor_property(key, value)
    return result


def connect_property(source, output, target):
    if not EDIT.connect_material_property(source, output, target):
        raise RuntimeError("Failed material property connection: " + str(target))


def connect(source, output, target, input_name):
    if not EDIT.connect_material_expressions(source, output, target, input_name):
        raise RuntimeError("Failed material expression connection: " + input_name)


def create_material(config, base, mr):
    path = config["asset_root"] + "/Materials/M_Waymarker_PBR"
    material = LIB.load_asset(path) if LIB.does_asset_exist(path) else ASSETS.create_asset(
        "M_Waymarker_PBR", config["asset_root"] + "/Materials", unreal.Material, unreal.MaterialFactoryNew())
    if not isinstance(material, unreal.Material):
        raise RuntimeError("Material creation failed")
    EDIT.delete_all_material_expressions(material)
    material.set_editor_property("two_sided", False)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    EDIT.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES, True)
    EDIT.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_NANITE, True)
    uv0 = expression(material, unreal.MaterialExpressionTextureCoordinate, -1200, -250, coordinate_index=0)
    uv1 = expression(material, unreal.MaterialExpressionTextureCoordinate, -1200, 200, coordinate_index=1)
    repair = expression(material, unreal.MaterialExpressionVertexColor, -850, 600)
    color0 = expression(material, unreal.MaterialExpressionTextureSample, -950, -350,
                        texture=base, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    color1 = expression(material, unreal.MaterialExpressionTextureSample, -950, -100,
                        texture=base, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    mr0 = expression(material, unreal.MaterialExpressionTextureSample, -950, 160,
                     texture=mr, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    mr1 = expression(material, unreal.MaterialExpressionTextureSample, -950, 400,
                     texture=mr, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    for uv, sample in ((uv0, color0), (uv1, color1), (uv0, mr0), (uv1, mr1)):
        connect(uv, "", sample, "UVs")
    color = expression(material, unreal.MaterialExpressionLinearInterpolate, -300, -160)
    packed = expression(material, unreal.MaterialExpressionLinearInterpolate, -300, 180)
    for first, donor, result in ((color0, color1, color), (mr0, mr1, packed)):
        connect(first, "RGB", result, "A")
        connect(donor, "RGB", result, "B")
        connect(repair, "R", result, "Alpha")
    roughness = expression(material, unreal.MaterialExpressionComponentMask, -100, 150, r=False, g=True, b=False, a=False)
    metallic = expression(material, unreal.MaterialExpressionComponentMask, -100, 310, r=False, g=False, b=True, a=False)
    connect(packed, "", roughness, "")
    connect(packed, "", metallic, "")
    connect_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    connect_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    connect_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)
    # The original atlas is unchanged. UV1 is the adjacent-stone donor, with
    # VertexColor.R feathered only on the repaired region. Untouched geometry
    # retains UV0. Do not use the unused MR.R channel as AO or hide holes.
    messages = EDIT.recompile_material(material)
    if messages:
        raise RuntimeError("Material compile failed: " + "; ".join(str(m) for m in messages))
    checked_save(material)
    return material


def import_mesh(config, material):
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    options.set_editor_property("automated_import_should_detect_type", False)
    data = options.get_editor_property("static_mesh_import_data")
    for key, value in {
        "combine_meshes": True, "auto_generate_collision": False, "one_convex_hull_per_ucx": True,
        "generate_lightmap_u_vs": False, "remove_degenerates": True, "build_nanite": True,
        "convert_scene": True, "convert_scene_unit": True, "force_front_x_axis": False,
        "transform_vertex_to_absolute": True, "import_uniform_scale": 1.0,
        "normal_import_method": unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS,
        "vertex_color_import_option": unreal.VertexColorImportOption.REPLACE,
    }.items():
        data.set_editor_property(key, value)
    task = unreal.AssetImportTask()
    task.filename = config["fbx"]
    task.destination_path = config["asset_root"] + "/Meshes"
    task.destination_name = "SM_Waymarker"
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = False
    task.factory = unreal.FbxFactory()
    task.options = options
    flag = "Interchange.FeatureFlags.Import.FBX"
    previous = unreal.SystemLibrary.get_console_variable_int_value(flag)
    unreal.SystemLibrary.execute_console_command(None, flag + " 0")
    try:
        ASSETS.import_asset_tasks([task])
    finally:
        unreal.SystemLibrary.execute_console_command(None, flag + " " + str(previous))
    for path in task.imported_object_paths:
        if not str(path).startswith(config["asset_root"] + "/"):
            raise RuntimeError("Unexpected FBX import destination: " + str(path))
    mesh = LIB.load_asset(task.destination_path + "/SM_Waymarker")
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError("Static mesh import failed")
    for index in range(len(mesh.get_editor_property("static_materials"))):
        mesh.set_material(index, material)
    body = mesh.get_editor_property("body_setup")
    convex_count = MESHES.get_convex_collision_count(mesh)
    if convex_count:
        # UE's "simple collision count" excludes convex hulls. Keep the
        # authored UCX geometry and remove this script's earlier box fallback
        # if an existing local inspection asset is being reimported.
        geometry = body.get_editor_property("agg_geom")
        geometry.set_editor_property("box_elems", [])
        body.set_editor_property("agg_geom", geometry)
    elif not MESHES.get_simple_collision_count(mesh):
        if MESHES.add_simple_collisions(mesh, unreal.ScriptCollisionShapeType.BOX) < 0:
            raise RuntimeError("Could not add simple box collision")
        REPORT["warnings"].append("No UCX was present; one bounds box was added as a coarse collision proxy.")
    body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
    settings = MESHES.get_nanite_settings(mesh)
    if not settings.get_editor_property("enabled"):
        settings.set_editor_property("enabled", True)
        MESHES.set_nanite_settings(mesh, settings, True)
    checked_save(mesh)
    return mesh


def asset_evidence(config, mesh, material, base, mr):
    bounds = mesh.get_bounds()
    dimensions = [v * 2 for v in vec(bounds.box_extent)]
    height = dimensions[2]
    if height <= 0:
        raise RuntimeError("Invalid zero-height mesh")
    scale = float(config["height_cm"]) / height
    if abs(scale - 1) > .02:
        REPORT["warnings"].append("Imported mesh height differs from target; uniformly scaled placed actors to the requested height.")
    body = mesh.get_editor_property("body_setup")
    geom = body.get_editor_property("agg_geom")
    REPORT["mesh"] = {
        "path": mesh.get_path_name(), "dimensions_cm": dimensions, "bounds_origin_cm": vec(bounds.origin),
        "placement_uniform_scale": scale, "placed_height_cm": height * scale,
        "nanite_enabled": bool(MESHES.get_nanite_settings(mesh).get_editor_property("enabled")),
        "nanite_triangles": mesh.get_num_nanite_triangles(), "nanite_vertices": mesh.get_num_nanite_vertices(),
        "source_triangles": mesh.get_static_mesh_description(0).get_triangle_count(),
        "fallback_lod0_triangles": mesh.get_num_triangles(0), "fallback_lod0_vertices": mesh.get_num_vertices(0),
        "uv_channels": MESHES.get_num_uv_channels(mesh, 0),
        "collision": {"complexity": str(MESHES.get_collision_complexity(mesh)),
                      "simple_count": MESHES.get_simple_collision_count(mesh) + MESHES.get_convex_collision_count(mesh),
                      "convex_hulls": len(geom.get_editor_property("convex_elems")),
                      "boxes": len(geom.get_editor_property("box_elems")),
                      "dense_render_mesh_used_for_queries": False},
        "material_slots": [str(s.get_editor_property("material_slot_name")) for s in mesh.get_editor_property("static_materials")],
    }
    REPORT["material"] = {
        "path": material.get_path_name(), "two_sided": material.get_editor_property("two_sided"),
        "nanite_usage": EDIT.has_material_usage(material, unreal.MaterialUsage.MATUSAGE_NANITE),
        "instanced_mesh_usage": EDIT.has_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES),
        "connections": {"BaseColor": "lerp(base_color(UV0), base_color(UV1), VertexColor.R).RGB",
                        "Roughness": "lerp(metallic_roughness(UV0), metallic_roughness(UV1), VertexColor.R).G",
                        "Metallic": "lerp(metallic_roughness(UV0), metallic_roughness(UV1), VertexColor.R).B"},
        "repair_blend": {"channel": "VertexColor.R", "untouched_value": 0, "donor_uv": 1,
                         "source_uv": 0, "vertex_color_import": "REPLACE", "unique_textures": 2, "texture_samples": 4},
        "textures": [{"path": t.get_path_name(), "srgb": t.get_editor_property("srgb"),
                      "compression": str(t.get_editor_property("compression_settings")),
                      "width": t.blueprint_get_size_x(), "height": t.blueprint_get_size_y(),
                      "max_texture_size": t.get_editor_property("max_texture_size")} for t in (base, mr)],
    }
    if not REPORT["mesh"]["nanite_enabled"] or REPORT["mesh"]["nanite_triangles"] <= 0:
        raise RuntimeError("Nanite data was not built")
    if not REPORT["mesh"]["collision"]["simple_count"]:
        raise RuntimeError("Simple collision missing")
    if REPORT["mesh"]["uv_channels"] < 2:
        raise RuntimeError("Repair blend requires both original UV0 and donor UV1")
    if MESHES.get_collision_complexity(mesh) != unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX:
        raise RuntimeError("Dense render collision was not disabled")
    if not base.get_editor_property("srgb") or mr.get_editor_property("srgb"):
        raise RuntimeError("PBR texture color spaces are incorrect")
    if any(t["width"] != 4096 or t["height"] != 4096 for t in REPORT["material"]["textures"]):
        raise RuntimeError("Expected original 4096 Ã— 4096 PBR textures")
    write_report()
    return scale, -(bounds.origin.z - bounds.box_extent.z) * scale


def spawn(cls, label, location=(0, 0, 0), rotation=(0, 0, 0)):
    # Python's native-make positional order is not C++ FRotator's order.
    actor = ACTORS.spawn_actor_from_class(cls, unreal.Vector(*location),
        unreal.Rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2]))
    if not actor:
        raise RuntimeError("Spawn failed: " + label)
    actor.set_actor_label("AI3D_" + label)
    return actor


def mesh_actor(mesh, label, location=(0, 0, 0), rotation=(0, 0, 0), scale=(1, 1, 1), collision=True):
    actor = spawn(unreal.StaticMeshActor, label, location, rotation)
    component = actor.static_mesh_component
    component.set_static_mesh(mesh)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    component.set_collision_profile_name("BlockAll")
    component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS if collision else unreal.CollisionEnabled.NO_COLLISION)
    return actor


def load_art(name):
    mesh = LIB.load_asset("/Game/Art/Meshes/" + name)
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError("Expected existing Dreambound kit mesh missing: " + name)
    return mesh


def set_light(sun, cool=False):
    sun.set_actor_rotation(unreal.Rotator(pitch=-31, yaw=145 if cool else -38, roll=0), False)
    light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    light.set_intensity(4.6)
    light.set_light_color(unreal.LinearColor(.60, .76, 1.0, 1) if cool else unreal.LinearColor(1, .86, .66, 1))
    # A documented inspection fill keeps the shadow-facing repair assessable.
    # It is local to this map and does not change the game's lighting setup.
    for actor in ACTORS.get_all_level_actors():
        if actor.get_actor_label() == "AI3D_Fill":
            actor.set_actor_rotation(unreal.Rotator(pitch=-38, yaw=325 if cool else 142, roll=0), False)
            fill = actor.get_component_by_class(unreal.DirectionalLightComponent)
            fill.set_intensity(1.6)
            fill.set_light_color(unreal.LinearColor(1, .86, .75, 1) if cool else unreal.LinearColor(.70, .82, 1, 1))


def make_scene(config, mesh, scale, base_z):
    if LIB.does_asset_exist(config["map"]):
        if not LEVEL.load_level(config["map"]):
            raise RuntimeError("Failed to load inspection map")
        for actor in ACTORS.get_all_level_actors():
            if actor.get_actor_label().startswith("AI3D_"):
                ACTORS.destroy_actor(actor)
    elif not LEVEL.new_level(config["map"]):
        raise RuntimeError("Failed to create inspection map")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", unreal.GameModeBase)
    tile = load_art("SM_StoneTile_B")
    for x in range(-5, 6):
        for y in range(-5, 6):
            mesh_actor(tile, f"Tile_{x}_{y}", (x * 200, y * 200, -24), (0, ((x + y) % 4) * 90, 0))
    wall = load_art("SM_Wall")
    for x in range(-2, 3):
        mesh_actor(wall, "Wall_" + str(x), (x * 400, 980, 0), scale=(1, 1, 1.1))
    mesh_actor(load_art("SM_Arch"), "Arch", (-750, 600, 0), scale=(1.1, 1.1, 1.1))
    mesh_actor(load_art("SM_GardenRockCluster"), "Rocks", (780, 580, 0), scale=(.7, .7, .7))
    mesh_actor(load_art("SM_GardenUnderstory"), "Understory", (800, 550, 0), scale=(1, 1, 1), collision=False)
    hero = mesh_actor(mesh, "Waymarker", (0, 0, base_z), scale=(scale,) * 3)
    # Independent shared-mesh copies at a second pad allow bounded repetition
    # checks without changing the first-person single-asset compositions.
    copies = []
    count = config["instance_count"]
    columns = math.ceil(math.sqrt(count))
    for index in range(count):
        x = 3000 + (index % columns - (columns - 1) / 2) * 360
        y = (index // columns - (columns - 1) / 2) * 360
        copy = mesh_actor(mesh, f"Repeated_{index:02d}", (x, y, base_z), (0, (index % 4) * 90, 0), (scale,) * 3)
        copies.append(copy)
    foundation = LIB.load_asset("/Engine/BasicShapes/Cube.Cube")
    floor = mesh_actor(foundation, "RepeatPad", (3000, 0, -35), scale=(21, 21, .5))
    floor.static_mesh_component.set_material(0, LIB.load_asset("/Game/Art/Materials/M_Mortar"))
    sun = spawn(unreal.DirectionalLight, "Sun", (0, 0, 3000), (-31, -38, 0))
    light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    light.set_mobility(unreal.ComponentMobility.MOVABLE)
    light.set_editor_property("atmosphere_sun_light", True)
    light.set_editor_property("light_source_angle", 1.2)
    fill = spawn(unreal.DirectionalLight, "Fill", (0, 0, 2000), (-38, 142, 0))
    fill_component = fill.get_component_by_class(unreal.DirectionalLightComponent)
    fill_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    fill_component.set_editor_property("atmosphere_sun_light", False)
    fill_component.set_editor_property("light_source_angle", 6.0)
    set_light(sun)
    spawn(unreal.SkyAtmosphere, "Atmosphere")
    sky = spawn(unreal.SkyLight, "Sky")
    skylight = sky.get_component_by_class(unreal.SkyLightComponent)
    skylight.set_mobility(unreal.ComponentMobility.MOVABLE)
    skylight.set_intensity(.58)
    skylight.set_real_time_capture(True)
    skylight.recapture_sky()
    post = spawn(unreal.PostProcessVolume, "Exposure")
    post.set_editor_property("unbound", True)
    settings = post.get_editor_property("settings")
    for key, value in {
        "override_auto_exposure_apply_physical_camera_exposure": True,
        "auto_exposure_apply_physical_camera_exposure": False,
        "override_auto_exposure_bias": True, "auto_exposure_bias": -.1,
        "override_bloom_intensity": True, "bloom_intensity": .28,
        "override_vignette_intensity": True, "vignette_intensity": .16,
        "override_motion_blur_amount": True, "motion_blur_amount": 0.,
    }.items():
        settings.set_editor_property(key, value)
    post.set_editor_property("settings", settings)
    camera = spawn(unreal.CameraActor, "Camera")
    camera.camera_component.set_field_of_view(94.)
    camera.camera_component.set_editor_property("constrain_aspect_ratio", False)
    player = spawn(unreal.PlayerStart, "Start", (-400, 0, 92), (0, 0, 0))
    del player
    if not LEVEL.save_current_level():
        raise RuntimeError("Failed to save evaluation map")
    REPORT["map"] = config["map"]
    REPORT["lighting"] = {"warm": {"sun_intensity": 4.6, "sun_color": [1, .86, .66], "rotation": [-31, -38, 0]},
                          "cool": {"sun_intensity": 4.6, "sun_color": [.60, .76, 1], "rotation": [-31, 145, 0]},
                          "skylight_intensity": .58, "exposure_bias": -.1,
                          "inspection_fill": {"intensity": 1.6, "warm_state_rgb": [.70, .82, 1], "cool_state_rgb": [1, .86, .75],
                                              "pitch": -38, "yaw": "Opposite the main directional light"},
                          "source": "DBRecoveryScene.cpp current courtyard main-light/exposure values; cool key and low fill are explicit local inspection variants."}
    REPORT["repeated_copies"] = {"count": len(copies), "type": "StaticMeshActor instances sharing one Nanite mesh and PBR material"}
    return world, hero, camera, sun, copies


def collision_checks(world, hero):
    results = []
    ignore = [a for a in ACTORS.get_all_level_actors() if a != hero]
    for complex_query in (False, True):
        hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(-400, 0, 120), unreal.Vector(400, 0, 120),
            unreal.TraceTypeQuery.ECC_VISIBILITY, complex_query, ignore, unreal.DrawDebugTrace.NONE)
        # Native-break USTRUCT functions are exposed through to_tuple(),
        # not as GameplayStatics methods in the Python API.
        info = hit.to_tuple() if hit else None
        actor = info[9] if info else None
        results.append({"query": "line_through_body", "trace_complex": complex_query,
                        "blocking_hit": bool(info and info[0]), "hit_actor": actor.get_actor_label() if actor else None,
                        "distance_cm": float(info[3]) if info else None, "pass": actor == hero})
    bounds = REPORT["mesh"]["dimensions_cm"]
    offset = max(bounds[0], bounds[1]) * REPORT["mesh"]["placement_uniform_scale"] / 2 + 70
    hit = unreal.SystemLibrary.capsule_trace_single(world, unreal.Vector(-400, offset, 94), unreal.Vector(400, offset, 94),
        32, 92, unreal.TraceTypeQuery.ECC_VISIBILITY, False, ignore, unreal.DrawDebugTrace.NONE)
    results.append({"query": "clear_lane_capsule", "capsule_radius_cm": 32, "capsule_half_height_cm": 92,
                    "side_offset_cm": offset, "blocking_hit": bool(hit), "pass": hit is None})
    REPORT["collision_checks"] = results
    if not all(r["pass"] for r in results):
        raise RuntimeError("Collision query checks failed")


class CaptureRunner:
    def __init__(self, config, scene):
        self.config = config
        self.world, self.hero, self.camera, self.sun, self.copies = scene
        self.started = time.monotonic()
        self.task = None
        self.current = None
        self.ready_at = self.started + config["settle_seconds"]
        self.samples = []
        self.previous_tick = self.started
        self.finishing = False
        self.profile_state = None
        self.profile_file = None
        self.in_tick = False
        self.views = [
            ("front-warm-3m", 0, 300, False, False), ("rear-warm-3m", 180, 300, False, False),
            ("front-cool-3m", 0, 300, True, False), ("rear-cool-3m", 180, 300, True, False),
            ("front-close-1_5m", 0, 150, False, False), ("rear-close-1_5m", 180, 150, False, False),
            ("front-threequarter-3m", 40, 300, False, False),
            ("nanite-triangles", 0, 300, False, True),
            (f"repeated-{config['instance_count']}-overview", 0, 0, False, False),
        ]

    def move(self, view):
        name, relative_yaw, distance, cool, nanite = view
        set_light(self.sun, cool)
        if name.startswith("repeated"):
            location = unreal.Vector(1450, -1600, 900)
            look_at = unreal.Vector(3000, 0, 120)
            self.current["eye_height_cm"] = 900
            self.current["note"] = "Elevated bounded repetition overview; not a player camera."
            unreal.SystemLibrary.execute_console_command(self.world, "r.GPUCsvStatsEnabled 1")
        else:
            theta = math.radians(self.config["front_yaw_degrees"] + relative_yaw)
            location = unreal.Vector(math.cos(theta) * distance, math.sin(theta) * distance, 158)
            # Level look at eye height preserves first-person framing. Close
            # shots intentionally crop the full prop to inspect relief/material.
            look_at = unreal.Vector(0, 0, 158)
            self.current.update({"eye_height_cm": 158, "horizontal_distance_cm": distance})
        rotation = unreal.MathLibrary.find_look_at_rotation(location, look_at)
        self.camera.set_actor_location(location, False, False)
        self.camera.set_actor_rotation(rotation, False)
        viewport = LEVEL.get_active_viewport_config_key()
        LEVEL.set_level_viewport_camera_info(location, rotation, viewport)
        LEVEL.set_level_viewport_fov(94., viewport)
        unreal.AutomationLibrary.set_editor_active_viewport_view_mode(
            unreal.ViewModeIndex.VMI_VISUALIZE_NANITE if nanite else unreal.ViewModeIndex.VMI_LIT)
        if nanite:
            unreal.SystemLibrary.execute_console_command(self.world, "r.Nanite.Visualize Triangles")
        else:
            unreal.SystemLibrary.execute_console_command(self.world, "r.Nanite.Visualize off")
        self.current.update({"camera_cm": vec(location), "rotation_pitch_yaw_roll": [rotation.pitch, rotation.yaw, rotation.roll],
                             "fov_degrees": 94, "lighting": "cool" if cool else "warm", "nanite_visualization": nanite})

    def tick(self, _delta):
        if self.finishing or self.in_tick:
            return
        self.in_tick = True
        try:
            now = time.monotonic()
            interval = now - self.previous_tick
            self.previous_tick = now
            if now - self.started > 600:
                raise RuntimeError("Capture run exceeded 10 minute bound")
            if self.task:
                if not self.task.is_task_done():
                    return
                path = Path(self.current["file"])
                if not path.exists() or path.stat().st_size <= 1000:
                    raise RuntimeError("Screenshot task finished without an image: " + str(path))
                self.current.update(file_identity(path))
                REPORT["captures"].append(self.current)
                unreal.log("AI3D_CAPTURE " + self.current["name"])
                write_report()
                self.task = None
                self.current = None
                self.ready_at = now + .25
                return
            if self.current:
                self.samples.append(interval)
                if now < self.ready_at:
                    return
                if self.config.get("native_profile", False) and self.current["name"].startswith("repeated") and self.profile_state != "done":
                    if self.profile_state is None:
                        # Capture native GPU/CPU counters only after the view
                        # has settled, with no screenshot during this interval.
                        size = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_level_viewport_size()
                        filename = "AI3D_" + self.config["run_id"] + "_" + str(time.time_ns())
                        self.profile_file = Path(unreal.Paths.profiling_dir()).resolve() / "CSV" / (filename + ".csv")
                        unreal.SystemLibrary.execute_console_command(self.world, "CsvProfile STARTFILE=" + filename)
                        unreal.SystemLibrary.execute_console_command(self.world, "CsvProfile START")
                        self.current["native_profile"] = {
                            "file": str(self.profile_file), "requested_seconds": 5.0,
                            "viewport_resolution": [size.x, size.y] if size else None,
                            "requested_resolution_matches_viewport": bool(size and [size.x, size.y] == self.config["capture_size"]),
                            "screen_percentage": unreal.SystemLibrary.get_console_variable_int_value("r.ScreenPercentage"),
                            "screenshots_during_interval": False,
                            "scope": "Native Unreal editor CSV counters for a settled25-copy view; no whole-game or packaged performance claim."}
                        self.profile_state = "measuring"
                        self.ready_at = now + 5.0
                        return
                    if self.profile_state == "measuring":
                        unreal.SystemLibrary.execute_console_command(self.world, "CsvProfile STOP")
                        self.profile_state = "writing"
                        self.ready_at = now + 3.0
                        return
                    if self.profile_state == "writing":
                        if self.profile_file.exists():
                            self.current["native_profile"]["artifact"] = file_identity(self.profile_file)
                        else:
                            REPORT["warnings"].append("Optional native CSV profile did not produce a readable .csv; no native performance result is claimed.")
                        self.profile_state = "done"
                valid = sorted(s * 1000 for s in self.samples[2:] if 0 < s < 1)
                if valid:
                    self.current["editor_tick_observation_ms"] = {
                        "samples": len(valid), "median": statistics.median(valid),
                        "p95": valid[min(len(valid) - 1, math.ceil(len(valid) * .95) - 1)],
                        "max": max(valid), "not_gpu_or_packaged_performance": True}
                size = self.config["capture_size"]
                self.task = unreal.AutomationLibrary.take_high_res_screenshot(
                    size[0], size[1], self.current["file"], camera=self.camera, delay=1.0, force_game_view=True)
                if not self.task or not self.task.is_valid_task():
                    raise RuntimeError("Editor capture did not create a valid task")
                return
            if now < self.ready_at:
                return
            if not self.views:
                self.finish()
                return
            view = self.views.pop(0)
            filename = OUTPUT / (view[0] + ".png")
            if filename.exists():
                filename.unlink()
            self.current = {"name": view[0], "file": str(filename), "resolution": self.config["capture_size"]}
            self.move(view)
            self.samples = []
            self.ready_at = now + self.config["settle_seconds"]
        except Exception as error:
            fail(error)
        finally:
            self.in_tick = False

    def finish(self):
        global CALLBACK
        # Map save pumps Slate. Remove the callback before saving so it cannot
        # start a new capture while finalization is already in progress.
        self.finishing = True
        if CALLBACK is not None:
            unreal.unregister_slate_post_tick_callback(CALLBACK)
            CALLBACK = None
        unreal.AutomationLibrary.set_editor_active_viewport_view_mode(unreal.ViewModeIndex.VMI_LIT)
        unreal.SystemLibrary.execute_console_command(self.world, "r.Nanite.Visualize off")
        set_light(self.sun, False)
        # Keep the reusable map opening at the ordinary front camera.
        self.current = {}
        self.move(("front-warm-3m", 0, 300, False, False))
        if not LEVEL.save_current_level():
            raise RuntimeError("Final evaluation map save failed")
        REPORT.update({"success": True, "complete": True, "finished_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                       "capture_elapsed_seconds": time.monotonic() - self.started})
        write_report()
        unreal.log("AI3D_INSPECTION_COMPLETE " + str(OUTPUT))
        shutdown()


def shutdown():
    global CALLBACK
    if CALLBACK is not None:
        unreal.unregister_slate_post_tick_callback(CALLBACK)
        CALLBACK = None
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    unreal.SystemLibrary.quit_editor()


def fail(error):
    REPORT.update({"success": False, "complete": False, "error": str(error), "traceback": traceback.format_exc()})
    write_report()
    unreal.log_error("AI3D_INSPECTION_FAILED " + str(error))
    shutdown()


def main():
    global RUNNER, CALLBACK
    unreal.EditorPythonScripting.set_keep_python_script_alive(True)
    config = configure()
    for suffix in ("/Meshes", "/Textures", "/Materials"):
        LIB.make_directory(config["asset_root"] + suffix)
    begin = time.monotonic()
    if config["phase"] in ("capture", "material"):
        mesh = LIB.load_asset(config["asset_root"] + "/Meshes/SM_Waymarker")
        material = LIB.load_asset(config["asset_root"] + "/Materials/M_Waymarker_PBR")
        base = LIB.load_asset(config["asset_root"] + "/Textures/T_BaseColor")
        mr = LIB.load_asset(config["asset_root"] + "/Textures/T_MetallicRoughness")
    else:
        base = import_texture(config, "base_color", "T_BaseColor", True)
        mr = import_texture(config, "metallic_roughness", "T_MetallicRoughness", False)
        material = create_material(config, base, mr)
        mesh = import_mesh(config, material)
    if not EDIT.has_material_usage(material, unreal.MaterialUsage.MATUSAGE_NANITE):
        EDIT.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_NANITE, True)
        messages = EDIT.recompile_material(material)
        if messages:
            raise RuntimeError("Nanite material compile failed: " + str(messages))
    # Save explicitly even when an editor-side auto-fix already enabled usage.
    checked_save(material)
    if config["phase"] == "view":
        bounds = mesh.get_bounds()
        scale = float(config["height_cm"]) / (bounds.box_extent.z * 2)
        base_z = -(bounds.origin.z - bounds.box_extent.z) * scale
        scene = make_scene(config, mesh, scale, base_z)
        position = unreal.Vector(260, 420, 165)
        rotation = unreal.MathLibrary.find_look_at_rotation(position, unreal.Vector(0, 0, 120))
        unreal.EditorLevelLibrary.set_level_viewport_camera_info(position, rotation)
        ACTORS.set_selected_level_actors([scene[1]])
        LEVEL.save_current_level()
        REPORT.update({"success": True, "complete": True,
            "evidence_type": "Interactive editor view prepared for owner inspection",
            "rendered_captures_requested": False})
        write_report()
        unreal.log("AI3D_INTERACTIVE_READY " + config["map"])
        return
    if config["phase"] == "material":
        REPORT.update({"success": True, "complete": True,
            "evidence_type": "Editor material usage persistence only; no new rendering claim",
            "material": {"path": material.get_path_name(),
                "nanite_usage": EDIT.has_material_usage(material, unreal.MaterialUsage.MATUSAGE_NANITE),
                "instanced_mesh_usage": EDIT.has_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)}})
        write_report()
        shutdown()
        return
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    scale, base_z = asset_evidence(config, mesh, material, base, mr)
    REPORT["import_or_load_elapsed_seconds"] = time.monotonic() - begin
    scene = make_scene(config, mesh, scale, base_z)
    collision_checks(scene[0], scene[1])
    REPORT["renderer_cvars"] = {key: unreal.SystemLibrary.get_console_variable_int_value(key) for key in (
        "r.Nanite", "r.Nanite.ProjectEnabled", "r.DynamicGlobalIlluminationMethod", "r.ReflectionMethod", "r.Shadow.Virtual.Enable", "r.ScreenPercentage")}
    write_report()
    if config["phase"] == "import":
        REPORT.update({"success": True, "complete": True, "rendered_captures_requested": False})
        write_report()
        shutdown()
        return
    RUNNER = CaptureRunner(config, scene)
    CALLBACK = unreal.register_slate_post_tick_callback(RUNNER.tick)


try:
    main()
except Exception as error:
    fail(error)

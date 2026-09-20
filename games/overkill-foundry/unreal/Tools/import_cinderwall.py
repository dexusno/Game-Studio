"""Rebuild ignored Unreal products from the original Cinderwall-v001 exports.

Run tools/unreal.ps1 Art after generating art-source/build_cinderwall.py outputs.
All source geometry is metres, +Z up, enemy forward -X. This import keeps X/Z
and reflects Y for Unreal handedness, converts metres to centimetres, and checks
the result. No combat data or animation-driven gameplay is authored here.
"""
import hashlib
import json
import math
from pathlib import Path
import unreal as u

PROJECT = Path(u.Paths.project_dir()).resolve()
REPO = PROJECT.parents[2]
SOURCE = REPO / ".local/overkill-foundry/art/cinderwall-v001"
ROOT = "/Game/Cinderwall"
REPORT = json.loads((SOURCE / "asset-report.json").read_text())
TOOLS = u.AssetToolsHelpers.get_asset_tools()
EDIT = u.EditorAssetLibrary
MAT = u.MaterialEditingLibrary
evidence = {"art_build": REPORT["version"], "engine": u.SystemLibrary.get_engine_version(), "units": "centimetres", "coordinate_conversion": "(x,-y,z)*100", "normal_green_flipped": True, "authored_normals_generated_tangents": True, "sources": {}, "robots": {}, "clips": {}, "stage": {}}
for folder in ("Textures", "Materials", "Robots", "Animations", "Stage"):
    EDIT.make_directory(ROOT + "/" + folder)


def load(path):
    return EDIT.load_asset(path) if EDIT.does_asset_exist(path) else None


def import_file(file, folder, name, options=None, factory=None):
    task = u.AssetImportTask()
    task.filename = str(file)
    task.destination_path = ROOT + "/" + folder
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    if options is not None:
        task.options = options
    if factory is not None:
        task.factory = factory
    TOOLS.import_asset_tasks([task])
    paths = list(task.imported_object_paths)
    if not paths:
        raise RuntimeError("No imported products: " + str(file))
    evidence["sources"][str(file.relative_to(SOURCE))] = hashlib.sha256(file.read_bytes()).hexdigest()
    return [EDIT.load_asset(path) for path in paths]


def fbx_options(kind, skeleton=None):
    opts = u.FbxImportUI()
    opts.automated_import_should_detect_type = False
    opts.import_materials = False
    opts.import_textures = False
    opts.create_physics_asset = False
    opts.import_as_skeletal = kind != "static"
    opts.import_mesh = kind != "animation"
    opts.import_animations = kind == "animation"
    opts.mesh_type_to_import = {"static": u.FBXImportType.FBXIT_STATIC_MESH, "skeletal": u.FBXImportType.FBXIT_SKELETAL_MESH, "animation": u.FBXImportType.FBXIT_ANIMATION}[kind]
    if skeleton:
        opts.skeleton = skeleton
    for data in (opts.static_mesh_import_data, opts.skeletal_mesh_import_data, opts.anim_sequence_import_data):
        data.set_editor_property("coordinate_system_policy", u.CoordinateSystemPolicy.MATCH_UP_AXIS)
        data.set_editor_property("convert_scene", True)
        data.set_editor_property("force_front_x_axis", False)
        data.set_editor_property("convert_scene_unit", True)
        data.set_editor_property("import_uniform_scale", 1.0)
    opts.static_mesh_import_data.combine_meshes = False
    opts.static_mesh_import_data.transform_vertex_to_absolute = False
    opts.static_mesh_import_data.bake_pivot_in_vertex = False
    opts.static_mesh_import_data.auto_generate_collision = False
    opts.static_mesh_import_data.generate_lightmap_u_vs = False
    for data in (opts.static_mesh_import_data, opts.skeletal_mesh_import_data):
        # Blender exports the authored weighted normals, but no tangent layer.
        # Preserve normals and let Unreal derive tangents from the original UVs.
        data.normal_import_method = u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    opts.anim_sequence_import_data.set_editor_property("animation_length", u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    opts.anim_sequence_import_data.set_editor_property("use_default_sample_rate", True)
    return opts


textures = {}
for file in sorted((SOURCE / "textures").glob("*.png")):
    tex = import_file(file, "Textures", file.stem, factory=u.TextureFactory())[0]
    normal = file.stem.endswith("_Normal")
    orm = file.stem.endswith("_ORM")
    tex.set_editor_property("srgb", not (normal or orm))
    if normal:
        tex.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("flip_green_channel", True)
    elif orm:
        tex.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_MASKS)
    EDIT.save_loaded_asset(tex)
    textures[file.stem] = tex

master = load(ROOT + "/Materials/M_Cinderwall")
if master is None:
    master = TOOLS.create_asset("M_Cinderwall", ROOT + "/Materials", u.Material, u.MaterialFactoryNew())
MAT.delete_all_material_expressions(master)
master.set_editor_property("blend_mode", u.BlendMode.BLEND_MASKED)
master.set_editor_property("opacity_mask_clip_value", 0.001)
MAT.set_base_material_usage(master, u.MaterialUsage.MATUSAGE_SKELETAL_MESH)


def node(cls, **properties):
    item = MAT.create_material_expression(master, cls)
    for key, value in properties.items():
        item.set_editor_property(key, value)
    return item


def link(a, b, inp, output=""):
    if not MAT.connect_material_expressions(a, output, b, inp):
        raise RuntimeError("Material connection failed: " + inp)


def scalar(name, value):
    return node(u.MaterialExpressionScalarParameter, parameter_name=name, default_value=value)


def vector(name, values):
    return node(u.MaterialExpressionVectorParameter, parameter_name=name, default_value=u.LinearColor(*values, 1))


def blend(a, b, alpha):
    item = node(u.MaterialExpressionLinearInterpolate)
    link(a, item, "A")
    link(b, item, "B")
    link(alpha, item, "Alpha")
    return item


use_texture = scalar("UseTexture", 1)
base = node(u.MaterialExpressionTextureSampleParameter2D, parameter_name="BaseColor", texture=textures["CW_iron_BaseColor"])
orm = node(u.MaterialExpressionTextureSampleParameter2D, parameter_name="ORM", texture=textures["CW_iron_ORM"], sampler_type=u.MaterialSamplerType.SAMPLERTYPE_MASKS)
normal = node(u.MaterialExpressionTextureSampleParameter2D, parameter_name="Normal", texture=textures["CW_iron_Normal"], sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
flat = vector("FlatColor", (0.05, 0.05, 0.05))
base_out = blend(flat, base, use_texture)
MAT.connect_material_property(base_out, "", u.MaterialProperty.MP_BASE_COLOR)
for output, default, name, prop in [("G", .26, "Roughness", u.MaterialProperty.MP_ROUGHNESS), ("B", .1, "Metallic", u.MaterialProperty.MP_METALLIC)]:
    mix = node(u.MaterialExpressionLinearInterpolate)
    link(scalar(name, default), mix, "A")
    link(orm, mix, "B", output)
    link(use_texture, mix, "Alpha")
    MAT.connect_material_property(mix, "", prop)
flat_normal = node(u.MaterialExpressionConstant3Vector, constant=u.LinearColor(0, 0, 1, 0))
normal_out = blend(flat_normal, normal, scalar("NormalStrength", .35))
MAT.connect_material_property(normal_out, "", u.MaterialProperty.MP_NORMAL)
emission = node(u.MaterialExpressionMultiply)
link(vector("EmissiveColor", (0, 0, 0)), emission, "A")
link(scalar("Glow", 1), emission, "B")
MAT.connect_material_property(emission, "", u.MaterialProperty.MP_EMISSIVE_COLOR)
noise = node(u.MaterialExpressionNoise, scale=.045, quality=1, levels=1, output_min=.05, output_max=.95)
mask = node(u.MaterialExpressionSubtract)
link(noise, mask, "A")
link(scalar("Dissolve", 0), mask, "B")
MAT.connect_material_property(mask, "", u.MaterialProperty.MP_OPACITY_MASK)
MAT.layout_material_expressions(master)
MAT.recompile_material(master)
EDIT.save_loaded_asset(master)


def linear(hex_color):
    values = [int(hex_color[i:i+2], 16) / 255 for i in (0, 2, 4)]
    return [v / 12.92 if v < .04045 else ((v + .055) / 1.055) ** 2.4 for v in values]


materials = {}
emissives = {"ember": ("ff6922", 3), "furnace": ("f35a17", .65), "optic": ("ffc06c", 3), "cool": ("67b8c4", 2)}
for name in list(REPORT["palette"]) + list(emissives):
    label = "MI_CW_" + name
    instance = load(ROOT + "/Materials/" + label)
    if instance is None:
        instance = TOOLS.create_asset(label, ROOT + "/Materials", u.MaterialInstanceConstant, u.MaterialInstanceConstantFactoryNew())
    MAT.set_material_instance_parent(instance, master)
    MAT.set_material_instance_scalar_parameter_value(instance, "Dissolve", 0)
    MAT.set_material_instance_scalar_parameter_value(instance, "Glow", 1)
    MAT.set_material_instance_scalar_parameter_value(instance, "UseTexture", 0 if name in emissives else 1)
    MAT.set_material_instance_scalar_parameter_value(instance, "NormalStrength", 0 if name in emissives else .35)
    if name in emissives:
        color, power = emissives[name]
        rgb = linear(color)
        MAT.set_material_instance_vector_parameter_value(instance, "FlatColor", u.LinearColor(*rgb, 1))
        MAT.set_material_instance_vector_parameter_value(instance, "EmissiveColor", u.LinearColor(*[x * power for x in rgb], 1))
    else:
        for param in ("BaseColor", "ORM", "Normal"):
            MAT.set_material_instance_texture_parameter_value(instance, param, textures["CW_" + name + "_" + param])
    EDIT.save_loaded_asset(instance)
    materials["M_CW_" + name] = instance


def assign_materials(mesh):
    skeletal = isinstance(mesh, u.SkeletalMesh)
    slots = list(mesh.get_editor_property("materials" if skeletal else "static_materials"))
    names = []
    for slot in slots:
        slot_name = str(slot.get_editor_property("imported_material_slot_name"))
        if slot_name not in materials:
            raise RuntimeError("Unmapped material slot " + slot_name + " on " + mesh.get_name())
        slot.set_editor_property("material_interface", materials[slot_name])
        names.append(slot_name)
    mesh.set_editor_property("materials" if skeletal else "static_materials", slots)
    EDIT.save_loaded_asset(mesh)
    return names


for name, info in REPORT["robots"].items():
    products = import_file(SOURCE / "meshes" / (name + ".fbx"), "Robots", "SK_" + name, fbx_options("skeletal"), u.FbxFactory())
    mesh = next(asset for asset in products if isinstance(asset, u.SkeletalMesh))
    slots = assign_materials(mesh)
    extent = mesh.get_imported_bounds().box_extent
    bounds = [extent.x * 2, extent.y * 2, extent.z * 2]
    expected = [v * 100 for v in info["rest_bounds_m"]]
    if any(abs(a-b) > 2 for a, b in zip(bounds, expected)):
        raise RuntimeError("Unexpected robot scale/axis: " + name + str(bounds) + " expected " + str(expected))
    evidence["robots"][name] = {"asset": mesh.get_path_name(), "bounds_cm": bounds, "materials": slots}
    prefix = "BR_" if name == "BreachRam" else "RM_"
    for file in sorted((SOURCE / "animations").glob(prefix + "*.fbx")):
        products = import_file(file, "Animations", file.stem, fbx_options("animation", mesh.get_editor_property("skeleton")), u.FbxFactory())
        clip = next(asset for asset in products if isinstance(asset, u.AnimSequence))
        duration = clip.get_play_length()
        expected_duration = REPORT["clips"][file.stem]["duration_s"]
        if abs(duration - expected_duration) > .04:
            raise RuntimeError("Unexpected clip length: " + file.stem + str(duration))
        options = u.AnimPoseEvaluationOptions()
        options.set_editor_property("optional_skeletal_mesh", mesh)
        options.set_editor_property("evaluation_type", u.AnimDataEvalType.COMPRESSED)
        first = u.AnimPoseExtensions.get_anim_pose_at_time(clip, 0, options)
        bone_names = list(u.AnimPoseExtensions.get_bone_names(first))
        if not {"root", "muzzle", "core", "intent"}.issubset({str(b) for b in bone_names}):
            raise RuntimeError("Missing event bones: " + file.stem)
        # UE retains the Blender armature object as one additional skeleton root.
        if len(bone_names) not in (info["bones"], info["bones"] + 1):
            raise RuntimeError("Unexpected imported skeleton size: " + file.stem)
        movement = 0.0
        for fraction in (.25, .50, .75, .98):
            pose = u.AnimPoseExtensions.get_anim_pose_at_time(clip, duration * fraction, options)
            for bone in bone_names:
                a = u.AnimPoseExtensions.get_bone_pose(first, bone, u.AnimPoseSpaces.WORLD).translation
                b = u.AnimPoseExtensions.get_bone_pose(pose, bone, u.AnimPoseSpaces.WORLD).translation
                movement = max(movement, math.sqrt((a.x-b.x)**2 + (a.y-b.y)**2 + (a.z-b.z)**2))
        if movement < .01 or movement > 2000:
            raise RuntimeError("Imported compressed animation motion is absent or outside source scale: " + file.stem)
        muzzle = u.AnimPoseExtensions.get_ref_bone_pose(first, "muzzle", u.AnimPoseSpaces.WORLD).translation
        if muzzle.x > -50 or abs(muzzle.y) > 2:
            raise RuntimeError("Imported skeleton changed forward axis: " + file.stem)
        evidence["clips"][file.stem] = {"asset": clip.get_path_name(), "duration": duration, "bones": len(bone_names), "max_sampled_bone_motion_cm": movement, "muzzle_reference_cm": [muzzle.x, muzzle.y, muzzle.z]}

stage_assets = import_file(SOURCE / "meshes/CinderwallStage.fbx", "Stage", "", fbx_options("static"), u.FbxFactory())
stage_assets = [asset for asset in stage_assets if isinstance(asset, u.StaticMesh)]
u.load_module("StaticMeshEditor")
static_editor = u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
if static_editor is None:
    raise RuntimeError("StaticMeshEditor subsystem is unavailable in this editor invocation")
evidence["stage_mesh_bounds_cm"] = {}
for mesh in stage_assets:
    # Preserving local pivots cancels the FBX node's metre-to-cm transform in
    # Unreal's legacy static importer (including import_uniform_scale). Bake the
    # explicit unit conversion through mesh build settings, not actor transforms.
    settings = static_editor.get_lod_build_settings(mesh, 0)
    settings.set_editor_property("build_scale3d", u.Vector(100, 100, 100))
    static_editor.set_lod_build_settings(mesh, 0, settings)
    assign_materials(mesh)
    bounds = mesh.get_bounds()
    # UE 5.8 GetBounds prefers CachedMeshDescriptionBounds during an import,
    # before build scale, even after render data is rebuilt. Record source-bound
    # conversion here; the game validates freshly loaded render bounds as well.
    evidence["stage_mesh_bounds_cm"][mesh.get_name()] = [bounds.box_extent.x * 200, bounds.box_extent.y * 200, bounds.box_extent.z * 200]
    if "SM_CW_deck_" in mesh.get_name() and (abs(bounds.box_extent.x - .98) > .01 or abs(bounds.box_extent.y - .98) > .01):
        raise RuntimeError("Static deck unit/pivot conversion failed: " + mesh.get_name() + " extent=" + str(bounds.box_extent))

levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
map_path = "/Game/Technical/FoundryHost"
if not levels.load_level(map_path):
    raise RuntimeError("Run the Content command before Art to create the technical host map")
for actor in actors.get_all_level_actors():
    if "CinderwallGenerated" in [str(tag) for tag in actor.tags]:
        actors.destroy_actor(actor)

shared_meshes = {}
placements = []
for item in REPORT["stage"]["instances"]:
    canonical = item["mesh"]
    if canonical not in shared_meshes:
        label = item["name"]
        candidates = [mesh for mesh in stage_assets if mesh.get_name() == label or mesh.get_name().endswith("_" + label)]
        if len(candidates) != 1:
            raise RuntimeError("Stage name mapping failed: " + label + " products=" + str([m.get_name() for m in stage_assets]))
        shared_meshes[canonical] = candidates[0]
    mesh = shared_meshes[canonical]
    p = item["position_m"]
    actor = actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector(p[0]*100, -p[1]*100, p[2]*100))
    actor.set_actor_label(item["name"])
    actor.tags = ["CinderwallGenerated"]
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.static_mesh_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
    placements.append({"name": item["name"], "mesh": mesh.get_path_name(), "position_cm": [p[0]*100, -p[1]*100, p[2]*100]})
if not levels.save_current_level():
    raise RuntimeError("Stage map save failed")
EDIT.save_directory(ROOT, only_if_is_dirty=True, recursive=True)
evidence["stage"] = {"actor_count": len(placements), "distinct_mesh_count": len(shared_meshes), "placements": placements}
saved = PROJECT / "Saved/ArtImport"
saved.mkdir(parents=True, exist_ok=True)
(saved / "cinderwall-import.json").write_text(json.dumps(evidence, indent=2), encoding="utf-8")
u.log("FOUNDRY_CINDERWALL_READY " + str(saved / "cinderwall-import.json"))

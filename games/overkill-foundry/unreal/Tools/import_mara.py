"""Rebuild ignored Mara props/depth from the dedicated original Blender exports.

Requires the original Cinderwall Art import first. Uses that game's own PBR master
and maps; no image/reference pixels or external assets are incorporated.
"""
import hashlib
import json
import math
from pathlib import Path
import unreal as u

PROJECT = Path(u.Paths.project_dir()).resolve()
SOURCE = PROJECT.parents[2] / ".local/overkill-foundry/art/mara-v001"
ROOT = "/Game/Cinderwall/Mara"
REPORT = json.loads((SOURCE / "asset-report.json").read_text())
VERIFY = json.loads((SOURCE / "interchange-verification.json").read_text())
if VERIFY["failures"]:
    raise RuntimeError("Mara interchange validator has failures")
TOOLS, EDIT = u.AssetToolsHelpers.get_asset_tools(), u.EditorAssetLibrary
evidence = {"art_build": REPORT["version"], "engine": u.SystemLibrary.get_engine_version(), "units": "centimetres", "coordinate_conversion": "(x,-y,z)*100", "normal_green_flipped": True, "sources": {}, "props": {}, "clips": {}, "stage": []}
for folder in ("Props", "Animations", "Stage"):
    EDIT.make_directory(ROOT + "/" + folder)


def import_file(file, folder, name, kind, skeleton=None):
    opts = u.FbxImportUI()
    opts.automated_import_should_detect_type = False
    opts.import_materials = opts.import_textures = opts.create_physics_asset = False
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
        data.set_editor_property("import_uniform_scale", 1)
    opts.static_mesh_import_data.combine_meshes = False
    opts.static_mesh_import_data.transform_vertex_to_absolute = False
    opts.static_mesh_import_data.bake_pivot_in_vertex = False
    opts.static_mesh_import_data.auto_generate_collision = False
    opts.static_mesh_import_data.generate_lightmap_u_vs = False
    for data in (opts.static_mesh_import_data, opts.skeletal_mesh_import_data):
        data.normal_import_method = u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    opts.anim_sequence_import_data.set_editor_property("animation_length", u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    opts.anim_sequence_import_data.set_editor_property("use_default_sample_rate", True)
    task = u.AssetImportTask()
    task.filename = str(file)
    task.destination_path = ROOT + "/" + folder
    task.destination_name = name
    task.automated = task.replace_existing = task.save = True
    task.options, task.factory = opts, u.FbxFactory()
    TOOLS.import_asset_tasks([task])
    if not task.imported_object_paths:
        raise RuntimeError("No product: " + str(file))
    evidence["sources"][str(file.relative_to(SOURCE))] = hashlib.sha256(file.read_bytes()).hexdigest()
    return [EDIT.load_asset(path) for path in task.imported_object_paths]


def bind_materials(mesh):
    prop = "materials" if isinstance(mesh, u.SkeletalMesh) else "static_materials"
    slots = list(mesh.get_editor_property(prop))
    names = []
    for slot in slots:
        name = str(slot.get_editor_property("imported_material_slot_name"))
        if not name.startswith("M_CW_"):
            raise RuntimeError("Unexpected surface " + name)
        asset = EDIT.load_asset("/Game/Cinderwall/Materials/MI_CW_" + name[5:])
        if asset is None:
            raise RuntimeError("Run Cinderwall Art first: " + name)
        slot.set_editor_property("material_interface", asset)
        names.append(name)
    mesh.set_editor_property(prop, slots)
    EDIT.save_loaded_asset(mesh)
    return names


for name, info in REPORT["props"].items():
    products = import_file(SOURCE / "meshes" / (name + ".fbx"), "Props", "SK_" + name, "skeletal")
    mesh = next(a for a in products if isinstance(a, u.SkeletalMesh))
    slots = bind_materials(mesh)
    e = mesh.get_imported_bounds().box_extent
    bounds = [e.x * 2, e.y * 2, e.z * 2]
    if any(abs(a - b * 100) > 2 for a, b in zip(bounds, info["rest_bounds_m"])):
        raise RuntimeError("Mara skeletal scale/orientation: " + name + str(bounds))
    evidence["props"][name] = {"asset": mesh.get_path_name(), "bounds_cm": bounds, "material_slots": slots}
    prefix = "MG_" if name == "MaraGun" else "MC_"
    required = {b["name"] for b in info["bone_definitions"]}
    for file in sorted((SOURCE / "animations").glob(prefix + "*.fbx")):
        products = import_file(file, "Animations", file.stem, "animation", mesh.get_editor_property("skeleton"))
        clip = next(a for a in products if isinstance(a, u.AnimSequence))
        duration = clip.get_play_length()
        if abs(duration - REPORT["clips"][file.stem]["duration_s"]) > .04:
            raise RuntimeError("Mara clip duration: " + file.stem)
        options = u.AnimPoseEvaluationOptions()
        options.set_editor_property("optional_skeletal_mesh", mesh)
        options.set_editor_property("evaluation_type", u.AnimDataEvalType.COMPRESSED)
        first = u.AnimPoseExtensions.get_anim_pose_at_time(clip, 0, options)
        bones = list(u.AnimPoseExtensions.get_bone_names(first))
        if not required.issubset({str(b) for b in bones}) or len(bones) != info["bones"] + 1:
            raise RuntimeError("Mara attachment skeleton changed")
        movement = 0
        for fraction in (.2, .35, .55, .75, .95):
            sample = u.AnimPoseExtensions.get_anim_pose_at_time(clip, duration * fraction, options)
            for bone in bones:
                a = u.AnimPoseExtensions.get_bone_pose(first, bone, u.AnimPoseSpaces.WORLD).translation
                b = u.AnimPoseExtensions.get_bone_pose(sample, bone, u.AnimPoseSpaces.WORLD).translation
                movement = max(movement, math.sqrt((a.x-b.x)**2 + (a.y-b.y)**2 + (a.z-b.z)**2))
        # Compression may remove the gun idle's sub-centimetre valve drift.
        # Action clips must still visibly articulate; never waive their check.
        if (movement < .1 and not file.stem.endswith("_idle")) or movement > 300:
            raise RuntimeError("Mara compressed clip motion out of range: " + file.stem + str(movement))
        sockets = {}
        for bone in ("muzzle", "load", "eject", "shield_attach", "operator_attach") if name == "MaraGun" else ("grab", "dump"):
            p = u.AnimPoseExtensions.get_ref_bone_pose(first, bone, u.AnimPoseSpaces.WORLD).translation
            sockets[bone] = [p.x, p.y, p.z]
        if name == "MaraGun" and sockets["muzzle"][0] < 180:
            raise RuntimeError("Mara gun must face +X")
        evidence["clips"][file.stem] = {"asset": clip.get_path_name(), "duration": duration, "bones": len(bones), "max_sampled_bone_motion_cm": movement, "stationary_compressed_idle": movement < .1, "sockets_cm": sockets}

products = import_file(SOURCE / "meshes/MaraStageExtension.fbx", "Stage", "", "static")
meshes = [a for a in products if isinstance(a, u.StaticMesh)]
u.load_module("StaticMeshEditor")
static_editor = u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
for mesh in meshes:
    settings = static_editor.get_lod_build_settings(mesh, 0)
    settings.set_editor_property("build_scale3d", u.Vector(100, 100, 100))
    static_editor.set_lod_build_settings(mesh, 0, settings)
    bind_materials(mesh)
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
if not levels.load_level("/Game/Technical/FoundryHost"):
    raise RuntimeError("Missing host map")
for actor in actors.get_all_level_actors():
    if "MaraGenerated" in [str(tag) for tag in actor.tags]:
        actors.destroy_actor(actor)
for item in REPORT["stage"]["instances"]:
    matches = [a for a in meshes if a.get_name() == item["name"] or a.get_name().endswith("_" + item["name"])]
    if len(matches) != 1:
        raise RuntimeError("Mara module mapping: " + item["name"])
    mesh = matches[0]
    if item["spawn"]:
        p = item["position_m"]
        actor = actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector(p[0]*100, -p[1]*100, p[2]*100))
        actor.set_actor_label(item["name"])
        actor.tags = ["MaraGenerated"]
        actor.static_mesh_component.set_static_mesh(mesh)
        actor.static_mesh_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
    evidence["stage"].append({"name": item["name"], "asset": mesh.get_path_name(), "spawned": item["spawn"], "position_m": item["position_m"], "expected_bounds_cm": [v*100 for v in item["bounds_m"]]})
if not levels.save_current_level():
    raise RuntimeError("Mara map save failed")
EDIT.save_directory(ROOT, only_if_is_dirty=True, recursive=True)
saved = PROJECT / "Saved/ArtImport"
saved.mkdir(parents=True, exist_ok=True)
(saved / "mara-import.json").write_text(json.dumps(evidence, indent=2), encoding="utf-8")
u.log("FOUNDRY_MARA_READY " + str(saved / "mara-import.json"))

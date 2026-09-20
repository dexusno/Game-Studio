"""Import the independently authored Mara operator; no level/runtime mutation.

Requires operator_v001_verify.py to pass and the existing M_Cinderwall master.
Run only under the integration owner's Unreal/GPU window. This script does not
start a game, spawn an actor, or claim human visual approval.
"""
import hashlib
import json
import math
from pathlib import Path
import unreal as u

PROJECT = Path(u.Paths.project_dir()).resolve()
SOURCE = PROJECT.parents[2] / ".local/overkill-foundry/art/operator-v001"
ROOT = "/Game/Cinderwall/Operator"
REPORT = json.loads((SOURCE / "asset-report.json").read_text())
VERIFY = json.loads((SOURCE / "interchange-verification.json").read_text())
if VERIFY["failures"]:
    raise RuntimeError("Operator source interchange validation failed")
for relative, digest in REPORT["file_sha256"].items():
    if hashlib.sha256((SOURCE / relative).read_bytes()).hexdigest() != digest:
        raise RuntimeError("Stale operator source report: " + relative)
for relative, digest in VERIFY["files"].items():
    if hashlib.sha256((SOURCE / relative).read_bytes()).hexdigest() != digest:
        raise RuntimeError("Stale operator interchange verification: " + relative)
TOOLS, EDIT, MAT = u.AssetToolsHelpers.get_asset_tools(), u.EditorAssetLibrary, u.MaterialEditingLibrary
evidence = {"asset": REPORT["asset"], "engine": u.SystemLibrary.get_engine_version(),
            "sources": {}, "textures": {}, "clips": {}, "human_visual_approval": "pending"}
for folder in ("Textures", "Materials", "Meshes", "Animations"):
    EDIT.make_directory(ROOT + "/" + folder)


def import_file(file, folder, name, options=None, factory=None):
    task = u.AssetImportTask()
    task.filename, task.destination_path, task.destination_name = str(file), ROOT + "/" + folder, name
    task.automated = task.replace_existing = task.save = True
    if options is not None:
        task.options = options
    if factory is not None:
        task.factory = factory
    TOOLS.import_asset_tasks([task])
    if not task.imported_object_paths:
        raise RuntimeError("No operator import product: " + str(file))
    evidence["sources"][str(file.relative_to(SOURCE))] = hashlib.sha256(file.read_bytes()).hexdigest()
    return [EDIT.load_asset(path) for path in task.imported_object_paths]


def options(animation=False, skeleton=None):
    opts = u.FbxImportUI()
    opts.automated_import_should_detect_type = False
    opts.import_materials = opts.import_textures = opts.create_physics_asset = False
    opts.import_as_skeletal = True
    opts.import_mesh, opts.import_animations = not animation, animation
    opts.mesh_type_to_import = u.FBXImportType.FBXIT_ANIMATION if animation else u.FBXImportType.FBXIT_SKELETAL_MESH
    if skeleton:
        opts.skeleton = skeleton
    for data in (opts.skeletal_mesh_import_data, opts.anim_sequence_import_data):
        data.set_editor_property("coordinate_system_policy", u.CoordinateSystemPolicy.MATCH_UP_AXIS)
        data.set_editor_property("convert_scene", True)
        data.set_editor_property("force_front_x_axis", False)
        data.set_editor_property("convert_scene_unit", True)
        data.set_editor_property("import_uniform_scale", 1.)
    opts.skeletal_mesh_import_data.normal_import_method = u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    opts.anim_sequence_import_data.set_editor_property("animation_length", u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    opts.anim_sequence_import_data.set_editor_property("use_default_sample_rate", True)
    return opts


textures = {}
for suffix in ("BaseColor", "ORM", "Normal"):
    texture = import_file(SOURCE / "textures" / ("Mara_" + suffix + ".png"), "Textures", "T_Mara_" + suffix,
                          factory=u.TextureFactory())[0]
    texture.set_editor_property("srgb", suffix == "BaseColor")
    if suffix == "Normal":
        texture.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_NORMALMAP)
        texture.set_editor_property("flip_green_channel", True)
    elif suffix == "ORM":
        texture.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_MASKS)
    EDIT.save_loaded_asset(texture)
    textures[suffix] = texture
    evidence["textures"][suffix] = texture.get_path_name()
master = EDIT.load_asset("/Game/Cinderwall/Materials/M_Cinderwall")
if master is None:
    raise RuntimeError("Run the original Cinderwall art import first")
material_path = ROOT + "/Materials/MI_MaraOperator"
material = EDIT.load_asset(material_path) if EDIT.does_asset_exist(material_path) else TOOLS.create_asset(
    "MI_MaraOperator", ROOT + "/Materials", u.MaterialInstanceConstant, u.MaterialInstanceConstantFactoryNew())
MAT.set_material_instance_parent(material, master)
for name, texture in textures.items():
    MAT.set_material_instance_texture_parameter_value(material, name, texture)
for name, value in (("UseTexture", 1.), ("NormalStrength", REPORT["normal_strength"]), ("Dissolve", 0.), ("Glow", 0.)):
    MAT.set_material_instance_scalar_parameter_value(material, name, value)
MAT.set_material_instance_vector_parameter_value(material, "EmissiveColor", u.LinearColor(0, 0, 0, 1))
EDIT.save_loaded_asset(material)

mesh = next(item for item in import_file(SOURCE / "meshes/MaraOperator.fbx", "Meshes", "SK_MaraOperator",
                                        options(), u.FbxFactory()) if isinstance(item, u.SkeletalMesh))
slots = list(mesh.get_editor_property("materials"))
if len(slots) != 1:
    raise RuntimeError("Operator material slot contract changed")
slots[0].set_editor_property("material_interface", material)
mesh.set_editor_property("materials", slots)
EDIT.save_loaded_asset(mesh)
extent = mesh.get_imported_bounds().box_extent
bounds = [extent.x * 2, extent.y * 2, extent.z * 2]
expected = [(b - a) * 100 for a, b in zip(REPORT["bounds_min_m"], REPORT["bounds_max_m"])]
if any(abs(a - b) > 2 for a, b in zip(bounds, expected)):
    raise RuntimeError("Operator centimetre scale/orientation mismatch: " + str(bounds))
evidence["mesh"] = {"asset": mesh.get_path_name(), "bounds_cm": bounds, "material": material.get_path_name()}
skeleton = mesh.get_editor_property("skeleton")
for name, info in REPORT["clips"].items():
    clip = next(item for item in import_file(SOURCE / "animations" / (name + ".fbx"), "Animations", name,
                    options(True, skeleton), u.FbxFactory()) if isinstance(item, u.AnimSequence))
    duration = clip.get_play_length()
    if abs(duration - info["duration_s"]) > .04:
        raise RuntimeError("Operator clip duration mismatch: " + name)
    evaluation = u.AnimPoseEvaluationOptions()
    evaluation.set_editor_property("optional_skeletal_mesh", mesh)
    evaluation.set_editor_property("evaluation_type", u.AnimDataEvalType.COMPRESSED)
    first = u.AnimPoseExtensions.get_anim_pose_at_time(clip, 0, evaluation)
    bones = list(u.AnimPoseExtensions.get_bone_names(first))
    required = {b["name"] for b in REPORT["bone_definitions"]}
    if not required.issubset({str(b) for b in bones}):
        raise RuntimeError("Operator bones missing")
    moved, foot_moved, root_moved = 0, 0, 0
    for fraction in (.2, .4, .6, .8):
        sample = u.AnimPoseExtensions.get_anim_pose_at_time(clip, duration * fraction, evaluation)
        for bone in ("head", "grip_l", "grip_r", "foot_l", "foot_r", "root"):
            a = u.AnimPoseExtensions.get_bone_pose(first, bone, u.AnimPoseSpaces.WORLD).translation
            b = u.AnimPoseExtensions.get_bone_pose(sample, bone, u.AnimPoseSpaces.WORLD).translation
            distance = math.sqrt((a.x-b.x)**2 + (a.y-b.y)**2 + (a.z-b.z)**2)
            moved = max(moved, distance)
            if bone.startswith("foot_"):
                foot_moved = max(foot_moved, distance)
            if bone == "root":
                root_moved = max(root_moved, distance)
    if moved < .1 or moved > 50:
        raise RuntimeError("Operator compressed motion mismatch: " + name + " " + str(moved))
    if foot_moved > 1.2 or root_moved > .001:
        raise RuntimeError("Operator translation retargeting/contact mismatch: " + name)
    grips = {}
    for bone in ("root", "grip_l", "grip_r", "portrait"):
        location = u.AnimPoseExtensions.get_bone_pose(first, bone, u.AnimPoseSpaces.WORLD).translation
        grips[bone] = [location.x, location.y, location.z]
    if name == "MO_idle":
        for bone, source_position in REPORT["sockets_ready_m"].items():
            expected = [source_position[0] * 100, -source_position[1] * 100, source_position[2] * 100]
            if max(abs(a - b) for a, b in zip(grips[bone], expected)) > 1:
                raise RuntimeError("Operator ready socket conversion mismatch: " + bone)
    evidence["clips"][name] = {"asset": clip.get_path_name(), "duration_s": duration,
        "bones": len(bones), "sampled_motion_cm": moved, "start_grips_cm": grips,
        "foot_motion_cm": foot_moved, "root_motion_cm": root_moved}
EDIT.save_directory(ROOT, only_if_is_dirty=True, recursive=True)
saved = PROJECT / "Saved/ArtImport"
saved.mkdir(parents=True, exist_ok=True)
(saved / "operator-v001-import.json").write_text(json.dumps(evidence, indent=2))
u.log("FOUNDRY_OPERATOR_READY " + str(saved / "operator-v001-import.json"))

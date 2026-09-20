"""Import verified original roster assets without changing rules, maps or materials.

Invoke using the existing Unreal editor Python commandlet wrapper after original
Cinderwall materials exist. Runtime adapters and rendered acceptance are separate.
"""
import hashlib
import json
import math
from pathlib import Path

import unreal as u

PROJECT = Path(u.Paths.project_dir()).resolve()
REPO = PROJECT.parents[2]
SOURCE = REPO / ".local/overkill-foundry/art/roster-v001"
ROOT = "/Game/Cinderwall"
REPORT = json.loads((SOURCE / "asset-report.json").read_text(encoding="utf-8"))
PROOF = json.loads((SOURCE / "interchange-verification.json").read_text(encoding="utf-8"))
TOOLS, EDIT = u.AssetToolsHelpers.get_asset_tools(), u.EditorAssetLibrary
VERIFY_ONLY = "-FoundryRosterVerify" in u.SystemLibrary.get_command_line()
evidence = {"version": REPORT["version"], "engine": u.SystemLibrary.get_engine_version(),
            "mode": "fresh-process reload verification" if VERIFY_ONLY else "import",
            "units": "centimetres", "conversion": "(x,-y,z)*100", "robots": {}, "clips": {},
            "source_report_sha256": hashlib.sha256((SOURCE / "asset-report.json").read_bytes()).hexdigest(),
            "source_files": PROOF["files"], "limits": "Import/compressed-pose checks only; no runtime gameplay or human approval."}
if PROOF["failures"] or PROOF["source_report_sha256"] != evidence["source_report_sha256"]:
    raise RuntimeError("Unverified or stale roster source")
for relative, digest in PROOF["files"].items():
    if hashlib.sha256((SOURCE / relative).read_bytes()).hexdigest() != digest:
        raise RuntimeError("Roster source changed after verification: " + relative)
for folder in ("Robots", "Animations"):
    EDIT.make_directory(ROOT + "/" + folder)


def options(animation=False, skeleton=None):
    opts = u.FbxImportUI()
    opts.automated_import_should_detect_type = False
    opts.import_materials = opts.import_textures = opts.create_physics_asset = False
    opts.import_as_skeletal = True
    opts.import_mesh, opts.import_animations = not animation, animation
    opts.mesh_type_to_import = u.FBXImportType.FBXIT_ANIMATION if animation else u.FBXImportType.FBXIT_SKELETAL_MESH
    if skeleton is not None:
        opts.skeleton = skeleton
    for data in (opts.skeletal_mesh_import_data, opts.anim_sequence_import_data):
        data.set_editor_property("coordinate_system_policy", u.CoordinateSystemPolicy.MATCH_UP_AXIS)
        data.set_editor_property("convert_scene", True)
        data.set_editor_property("force_front_x_axis", False)
        data.set_editor_property("convert_scene_unit", True)
        data.set_editor_property("import_uniform_scale", 1.0)
    opts.skeletal_mesh_import_data.normal_import_method = u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    opts.anim_sequence_import_data.set_editor_property("animation_length", u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    opts.anim_sequence_import_data.set_editor_property("use_default_sample_rate", True)
    return opts


def import_file(file, folder, name, opts):
    task = u.AssetImportTask()
    task.filename, task.destination_path, task.destination_name = str(file), ROOT + "/" + folder, name
    task.automated = task.replace_existing = task.save = True
    task.options, task.factory = opts, u.FbxFactory()
    TOOLS.import_asset_tasks([task])
    if not task.imported_object_paths:
        raise RuntimeError("No imported roster products: " + str(file))
    return [EDIT.load_asset(path) for path in task.imported_object_paths]


for name, info in REPORT["robots"].items():
    products = [EDIT.load_asset(ROOT + "/Robots/SK_" + name)] if VERIFY_ONLY else import_file(SOURCE / "meshes" / (name + ".fbx"), "Robots", "SK_" + name, options())
    mesh = next(asset for asset in products if isinstance(asset, u.SkeletalMesh))
    # Unreal Array iteration returns struct copies. Keep those edited copies in
    # a Python list before assigning the entire materials array back to the mesh.
    slots = list(mesh.get_editor_property("materials"))
    for slot in slots:
        label = str(slot.get_editor_property("imported_material_slot_name"))
        if not label.startswith("M_CW_") or label not in info["materials"]:
            raise RuntimeError("Unexpected roster material slot: " + label)
        path = ROOT + "/Materials/MI_CW_" + label[len("M_CW_"):]
        if not EDIT.does_asset_exist(path):
            raise RuntimeError("Original Cinderwall material missing: " + path)
        if not VERIFY_ONLY:
            slot.set_editor_property("material_interface", EDIT.load_asset(path))
    if not VERIFY_ONLY:
        mesh.set_editor_property("materials", slots)
        EDIT.save_loaded_asset(mesh)
    material_paths = []
    for slot in mesh.get_editor_property("materials"):
        label = str(slot.get_editor_property("imported_material_slot_name"))
        material = slot.get_editor_property("material_interface")
        expected_material = ROOT + "/Materials/MI_CW_" + label[len("M_CW_"):]
        if material is None or material.get_path_name().split(".")[0] != expected_material:
            raise RuntimeError("Saved roster material assignment missing: " + name + " / " + label)
        material_paths.append(material.get_path_name())
    bounds = mesh.get_imported_bounds().box_extent
    actual = [bounds.x * 2, bounds.y * 2, bounds.z * 2]
    expected = [v * 100 for v in info["rest_bounds_m"]]
    if any(abs(a - b) > 3 for a, b in zip(actual, expected)):
        raise RuntimeError("Roster scale changed: " + name + str(actual) + str(expected))
    evidence["robots"][name] = {"source_id": info["source_id"], "asset": mesh.get_path_name(),
                                 "bounds_cm": actual, "materials": [str(s.get_editor_property("imported_material_slot_name")) for s in slots],
                                 "material_interfaces": material_paths,
                                 "action_clips": info["action_clips"]}
    for clip_name, clip_info in REPORT["clips"].items():
        if not clip_name.startswith(info["prefix"]):
            continue
        products = [EDIT.load_asset(ROOT + "/Animations/" + clip_name)] if VERIFY_ONLY else import_file(
            SOURCE / "animations" / (clip_name + ".fbx"), "Animations", clip_name,
            options(True, mesh.get_editor_property("skeleton")))
        clip = next(asset for asset in products if isinstance(asset, u.AnimSequence))
        duration = clip.get_play_length()
        if abs(duration - clip_info["duration_s"]) > .04:
            raise RuntimeError("Roster clip duration changed: " + clip_name + str(duration))
        evaluate = u.AnimPoseEvaluationOptions()
        evaluate.set_editor_property("optional_skeletal_mesh", mesh)
        evaluate.set_editor_property("evaluation_type", u.AnimDataEvalType.COMPRESSED)
        first = u.AnimPoseExtensions.get_anim_pose_at_time(clip, 0, evaluate)
        names = list(u.AnimPoseExtensions.get_bone_names(first))
        if not {b["name"] for b in info["bones"]}.issubset({str(n) for n in names}):
            raise RuntimeError("Roster articulation missing: " + clip_name)
        movement = 0
        for t in (.2, .4, .6, .8, .99):
            pose = u.AnimPoseExtensions.get_anim_pose_at_time(clip, duration * t, evaluate)
            for bone in names:
                a = u.AnimPoseExtensions.get_bone_pose(first, bone, u.AnimPoseSpaces.WORLD).translation
                b = u.AnimPoseExtensions.get_bone_pose(pose, bone, u.AnimPoseSpaces.WORLD).translation
                movement = max(movement, math.sqrt((a.x-b.x)**2 + (a.y-b.y)**2 + (a.z-b.z)**2))
        if not .01 < movement < 600:
            raise RuntimeError("Roster animation motion invalid: " + clip_name + str(movement))
        muzzle = u.AnimPoseExtensions.get_ref_bone_pose(first, "muzzle", u.AnimPoseSpaces.WORLD).translation
        reference = next(b["pivot"] for b in info["bones"] if b["name"] == "muzzle")
        expected_muzzle = [reference[0] * 100, -reference[1] * 100, reference[2] * 100]
        if any(abs(a-b) > 2 for a, b in zip([muzzle.x, muzzle.y, muzzle.z], expected_muzzle)):
            raise RuntimeError("Roster socket conversion invalid: " + clip_name)
        evidence["clips"][clip_name] = {"asset": clip.get_path_name(), "duration_s": duration,
                                        "bones": len(names), "maximum_sampled_bone_motion_cm": movement,
                                        "muzzle_reference_cm": [muzzle.x, muzzle.y, muzzle.z]}
EDIT.save_directory(ROOT + "/Robots", only_if_is_dirty=True, recursive=True)
EDIT.save_directory(ROOT + "/Animations", only_if_is_dirty=True, recursive=True)
saved = PROJECT / "Saved/ArtImport"
saved.mkdir(parents=True, exist_ok=True)
output = saved / ("roster-v001-verify.json" if VERIFY_ONLY else "roster-v001-import.json")
output.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
u.log(("FOUNDRY_ROSTER_VERIFIED " if VERIFY_ONLY else "FOUNDRY_ROSTER_READY ") + str(output))

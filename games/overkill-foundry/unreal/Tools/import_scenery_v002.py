"""Import the separately verified original scenery; run only at a closed-game boundary.

Requires the existing Cinderwall master/hero imports. Replaces stage actors,
preserving the separate working Mara hopper, claw and payload. No rules change.
"""
import hashlib
import json
from pathlib import Path

import unreal as u

PROJECT = Path(u.Paths.project_dir()).resolve()
SOURCE = PROJECT.parents[2] / ".local/overkill-foundry/art/scenery-v002"
ROOT = "/Game/Cinderwall/SceneryV002"
TOOLS, EDIT, MAT = u.AssetToolsHelpers.get_asset_tools(), u.EditorAssetLibrary, u.MaterialEditingLibrary
REPORT = json.loads((SOURCE / "asset-report.json").read_text(encoding="utf-8"))
VERIFY = json.loads((SOURCE / "interchange-verification.json").read_text(encoding="utf-8"))


def sha(file):
    return hashlib.sha256(file.read_bytes()).hexdigest()


if VERIFY["failures"] or VERIFY["fbx_sha256"] != sha(SOURCE / "meshes/CinderwallSceneryV002.fbx") or VERIFY["source_report_sha256"] != sha(SOURCE / "asset-report.json"):
    raise RuntimeError("Run scenery_v002_verify against these exact outputs first")
master = EDIT.load_asset("/Game/Cinderwall/Materials/M_Cinderwall")
if master is None:
    raise RuntimeError("Run the original Cinderwall Art import first")
for folder in ("Stage", "Textures", "Materials"):
    EDIT.make_directory(ROOT + "/" + folder)
evidence = {"art_build": REPORT["version"], "engine": u.SystemLibrary.get_engine_version(),
            "units": "centimetres", "conversion": "(x,-y,z)*100", "sources": {}, "textures": {}, "stage": []}


def import_file(file, folder, options=None, factory=None):
    task = u.AssetImportTask()
    task.filename = str(file)
    task.destination_path = ROOT + "/" + folder
    task.automated = task.replace_existing = task.save = True
    if options is not None:
        task.options = options
    if factory is not None:
        task.factory = factory
    TOOLS.import_asset_tasks([task])
    if not task.imported_object_paths:
        raise RuntimeError("No product from " + str(file))
    evidence["sources"][file.relative_to(SOURCE).as_posix()] = sha(file)
    return [EDIT.load_asset(p) for p in task.imported_object_paths]


textures = {}
for file in sorted((SOURCE / "textures").glob("CW_v2_*.png")):
    tex = next(a for a in import_file(file, "Textures") if isinstance(a, u.Texture2D))
    tex.set_editor_property("srgb", file.stem.endswith("_BaseColor"))
    if file.stem.endswith("_Normal"):
        tex.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("flip_green_channel", True)
    elif file.stem.endswith("_ORM"):
        tex.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_MASKS)
    EDIT.save_loaded_asset(tex)
    textures[file.stem] = tex
    evidence["textures"][file.stem] = tex.get_path_name()


def linear(color):
    values = [int(color[i:i+2], 16) / 255 for i in (0, 2, 4)]
    return [v / 12.92 if v < .04045 else ((v + .055) / 1.055) ** 2.4 for v in values]


for name in [n for n in REPORT["palette"] if n.startswith("v2_")] + ["v2_window_warm", "v2_window_cool"]:
    label = "MI_CW_" + name
    material = EDIT.load_asset(ROOT + "/Materials/" + label)
    if material is None:
        material = TOOLS.create_asset(label, ROOT + "/Materials", u.MaterialInstanceConstant, u.MaterialInstanceConstantFactoryNew())
    MAT.set_material_instance_parent(material, master)
    MAT.set_material_instance_scalar_parameter_value(material, "Dissolve", 0)
    emissive = name in REPORT["emissive"]
    MAT.set_material_instance_scalar_parameter_value(material, "UseTexture", 0 if emissive else 1)
    MAT.set_material_instance_scalar_parameter_value(material, "NormalStrength", 0 if emissive else .35)
    MAT.set_material_instance_scalar_parameter_value(material, "Glow", 1)
    if emissive:
        color, power = REPORT["emissive"][name]
        rgb = linear(color)
        MAT.set_material_instance_vector_parameter_value(material, "FlatColor", u.LinearColor(*rgb, 1))
        MAT.set_material_instance_vector_parameter_value(material, "EmissiveColor", u.LinearColor(*[v * power for v in rgb], 1))
        MAT.set_material_instance_scalar_parameter_value(material, "Roughness", .66)
        MAT.set_material_instance_scalar_parameter_value(material, "Metallic", 0)
    else:
        for param in ("BaseColor", "ORM", "Normal"):
            MAT.set_material_instance_texture_parameter_value(material, param, textures["CW_" + name + "_" + param])
    EDIT.save_loaded_asset(material)

options = u.FbxImportUI()
options.automated_import_should_detect_type = False
options.import_as_skeletal = options.import_animations = options.import_materials = options.import_textures = False
options.import_mesh = True
options.mesh_type_to_import = u.FBXImportType.FBXIT_STATIC_MESH
data = options.static_mesh_import_data
data.set_editor_property("coordinate_system_policy", u.CoordinateSystemPolicy.MATCH_UP_AXIS)
data.convert_scene = data.convert_scene_unit = True
data.force_front_x_axis = False
data.import_uniform_scale = 1
data.combine_meshes = data.transform_vertex_to_absolute = data.bake_pivot_in_vertex = False
data.auto_generate_collision = data.generate_lightmap_u_vs = False
data.normal_import_method = u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
meshes = [m for m in import_file(SOURCE / "meshes/CinderwallSceneryV002.fbx", "Stage", options, u.FbxFactory()) if isinstance(m, u.StaticMesh)]
u.load_module("StaticMeshEditor")
static_editor = u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
for mesh in meshes:
    settings = static_editor.get_lod_build_settings(mesh, 0)
    settings.set_editor_property("build_scale3d", u.Vector(100, 100, 100))
    static_editor.set_lod_build_settings(mesh, 0, settings)
    slots = list(mesh.get_editor_property("static_materials"))
    for slot in slots:
        name = str(slot.get_editor_property("imported_material_slot_name"))
        if not name.startswith("M_CW_"):
            raise RuntimeError("Unexpected scenery material " + name)
        parent = ROOT if name.startswith("M_CW_v2_") else "/Game/Cinderwall"
        material = EDIT.load_asset(parent + "/Materials/MI_CW_" + name[5:])
        if material is None:
            raise RuntimeError("Missing scenery material " + name)
        slot.set_editor_property("material_interface", material)
    mesh.set_editor_property("static_materials", slots)
    EDIT.save_loaded_asset(mesh)

# Validate all mappings and source bounds before replacing any map actors.
mapped = []
for item in REPORT["stage"]["instances"]:
    matches = [m for m in meshes if m.get_name() == item["name"] or m.get_name().endswith("_" + item["name"])]
    if len(matches) != 1:
        raise RuntimeError("Scenery name mapping failed: " + item["name"])
    mesh = matches[0]
    ext = mesh.get_bounds().box_extent
    if any(abs(a * 2 - b) > .015 for a, b in zip((ext.x, ext.y, ext.z), item["bounds_m"])):
        raise RuntimeError("Scenery source units/bounds differ: " + item["name"])
    mapped.append((item, mesh))
levels, actors = u.get_editor_subsystem(u.LevelEditorSubsystem), u.get_editor_subsystem(u.EditorActorSubsystem)
if not levels.load_level("/Game/Technical/FoundryHost"):
    raise RuntimeError("Missing host map")
for actor in actors.get_all_level_actors():
    tags = {str(t) for t in actor.tags}
    old_mara_depth = "MaraGenerated" in tags and "hopper" not in actor.get_actor_label().lower()
    if "CinderwallGenerated" in tags or "CinderwallSceneryV002" in tags or old_mara_depth:
        actors.destroy_actor(actor)
for item, mesh in mapped:
    p = item["position_m"]
    actor = actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector(p[0] * 100, -p[1] * 100, p[2] * 100))
    actor.set_actor_label(item["name"])
    actor.tags = ["CinderwallSceneryV002"]
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.static_mesh_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
    evidence["stage"].append({"name": item["name"], "asset": mesh.get_path_name(),
                              "position_cm": [p[0] * 100, -p[1] * 100, p[2] * 100],
                              "expected_bounds_cm": [v * 100 for v in item["bounds_m"]]})
if not levels.save_current_level():
    raise RuntimeError("Scenery map save failed")
EDIT.save_directory(ROOT, only_if_is_dirty=True, recursive=True)
saved = PROJECT / "Saved/ArtImport/scenery-v002-import.json"
saved.parent.mkdir(parents=True, exist_ok=True)
saved.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
u.log("FOUNDRY_SCENERY_V002_READY " + str(saved))

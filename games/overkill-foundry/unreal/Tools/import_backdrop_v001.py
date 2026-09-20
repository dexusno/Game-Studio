"""Import the fixed far-city card at a closed-game boundary.

This changes only this importer's own asset folder and tagged map actor.
Existing scenery and playable actor placement are retained.
"""
import hashlib
import json
from pathlib import Path

import unreal as u

PROJECT = Path(u.Paths.project_dir()).resolve()
GAME = PROJECT.parent
SOURCE = PROJECT.parents[2] / ".local/overkill-foundry/art/backdrop-v001"
ART = GAME / "assets/production/cinderwall-backdrop-v001"
ROOT = "/Game/Cinderwall/BackdropV001"
TOOLS = u.AssetToolsHelpers.get_asset_tools()
EDIT = u.EditorAssetLibrary
MAT = u.MaterialEditingLibrary


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


mesh_report = json.loads((SOURCE / "mesh-report.json").read_text(encoding="utf-8"))
image_report = json.loads((ART / "asset-report.json").read_text(encoding="utf-8"))
fbx = SOURCE / mesh_report["mesh"]
png = GAME / image_report["image"]["path"]
if not mesh_report["fbx_reimport_passed"] or sha(fbx) != mesh_report["sha256"]:
    raise RuntimeError("Build and round-trip the exact backdrop mesh first")
if sha(png) != image_report["image"]["sha256"]:
    raise RuntimeError("Backdrop image differs from its provenance record")
EDIT.make_directory(ROOT)


def import_asset(file, name, options=None, factory=None):
    task = u.AssetImportTask()
    task.filename = str(file)
    task.destination_path = ROOT
    task.destination_name = name
    task.automated = task.replace_existing = task.save = True
    if options is not None:
        task.options = options
    if factory is not None:
        task.factory = factory
    TOOLS.import_asset_tasks([task])
    if not task.imported_object_paths:
        raise RuntimeError("No imported asset: " + str(file))
    return [EDIT.load_asset(path) for path in task.imported_object_paths]


texture = next(a for a in import_asset(png, "T_Cinderwall_DistantCity") if isinstance(a, u.Texture2D))
texture.set_editor_property("srgb", True)
texture.set_editor_property("address_x", u.TextureAddress.TA_CLAMP)
texture.set_editor_property("address_y", u.TextureAddress.TA_CLAMP)
EDIT.save_loaded_asset(texture)
material_path = ROOT + "/M_Cinderwall_DistantCity"
material = EDIT.load_asset(material_path) if EDIT.does_asset_exist(material_path) else None
if material is None:
    material = TOOLS.create_asset("M_Cinderwall_DistantCity", ROOT, u.Material, u.MaterialFactoryNew())
MAT.delete_all_material_expressions(material)
material.set_editor_property("shading_model", u.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property("two_sided", True)
sample = MAT.create_material_expression(material, u.MaterialExpressionTextureSample, -420, 0)
sample.texture = texture
exposure = MAT.create_material_expression(material, u.MaterialExpressionScalarParameter, -420, 140)
exposure.parameter_name = "BackdropExposure"
exposure.default_value = .85
multiply = MAT.create_material_expression(material, u.MaterialExpressionMultiply, -150, 0)
MAT.connect_material_expressions(sample, "RGB", multiply, "A")
MAT.connect_material_expressions(exposure, "", multiply, "B")
MAT.connect_material_property(multiply, "", u.MaterialProperty.MP_EMISSIVE_COLOR)
MAT.recompile_material(material)
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
mesh = next(a for a in import_asset(fbx, "SM_Cinderwall_DistantCity", options, u.FbxFactory()) if isinstance(a, u.StaticMesh))
u.load_module("StaticMeshEditor")
mesh_editor = u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
settings = mesh_editor.get_lod_build_settings(mesh, 0)
settings.set_editor_property("build_scale3d", u.Vector(100, 100, 100))
mesh_editor.set_lod_build_settings(mesh, 0, settings)
slots = list(mesh.get_editor_property("static_materials"))
if len(slots) != 1:
    raise RuntimeError("Backdrop must have one material slot")
slots[0].set_editor_property("material_interface", material)
mesh.set_editor_property("static_materials", slots)
EDIT.save_loaded_asset(mesh)

levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
if not levels.load_level("/Game/Technical/FoundryHost"):
    raise RuntimeError("Missing host map")
for actor in actors.get_all_level_actors():
    if "CinderwallBackdropV001" in {str(tag) for tag in actor.tags}:
        actors.destroy_actor(actor)
actor = actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector(0, 0, 0))
actor.set_actor_label("Cinderwall distant city")
actor.tags = ["CinderwallBackdropV001"]
actor.static_mesh_component.set_static_mesh(mesh)
actor.static_mesh_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
actor.static_mesh_component.set_cast_shadow(False)
if not levels.save_current_level():
    raise RuntimeError("Backdrop map save failed")
EDIT.save_directory(ROOT, only_if_is_dirty=True, recursive=True)
report = {"schema": 1, "engine": u.SystemLibrary.get_engine_version(),
          "image_sha256": sha(png), "fbx_sha256": sha(fbx),
          "mesh_report_sha256": sha(SOURCE / "mesh-report.json"),
          "image_report_sha256": sha(ART / "asset-report.json"),
          "texture": texture.get_path_name(), "material": material.get_path_name(),
          "mesh": mesh.get_path_name(), "triangles": mesh_report["triangles"],
          "world_space_fixed": True, "collision": False, "cast_shadow": False,
          "visual_acceptance": "Pending in-engine framing and owner approval"}
saved = PROJECT / "Saved/ArtImport/backdrop-v001-import.json"
saved.parent.mkdir(parents=True, exist_ok=True)
saved.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
u.log("FOUNDRY_BACKDROP_V001_READY " + str(saved))

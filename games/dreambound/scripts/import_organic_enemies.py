"""Import the finished organic creature FBX and its unchanged original PBR maps.

Run in the full Unreal editor using -ExecutePythonScript. Optional
-OrganicEnemyAssets=Briarhide,MireSeer selects prepared families. Sources live
under config.local.json's windowsOutputRoot/organic-enemies/<source_folder>/finished.
Missing source, rig bones or texture files are errors, never robot substitution.
"""
import hashlib
import json
from pathlib import Path
import re
import traceback
import unreal

ROOT = Path(__file__).resolve().parents[1]
DEST = "/Game/Art/OrganicEnemies"
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
LIB = unreal.EditorAssetLibrary
EDIT = unreal.MaterialEditingLibrary
REPORT = {"complete": False, "scope": DEST, "assets": [], "textures": [], "materials": []}


def node(material, kind, x=0, y=0, **settings):
    result = EDIT.create_material_expression(material, kind, x, y)
    for key, value in settings.items():
        result.set_editor_property(key, value)
    return result


def link(source, pin, target, target_pin):
    if not EDIT.connect_material_expressions(source, pin, target, target_pin):
        raise RuntimeError("Could not connect material pin " + target_pin)


def property_link(expression, pin, target):
    if not EDIT.connect_material_property(expression, pin, target):
        raise RuntimeError("Could not connect material output")


def texture(source, name, srgb):
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = DEST + "/Textures"
    task.destination_name = name
    task.automated = task.replace_existing = task.save = True
    TOOLS.import_asset_tasks([task])
    result = LIB.load_asset(task.destination_path + "/" + name)
    if not isinstance(result, unreal.Texture2D):
        raise RuntimeError("Texture import failed: " + name)
    result.set_editor_property("srgb", srgb)
    if not srgb:
        result.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    LIB.save_loaded_asset(result)
    REPORT["textures"].append({"asset": result.get_path_name(), "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(), "srgb": srgb})
    return result


def material(asset, base, packed, pigment_floor=None):
    name = "M_OE_" + asset
    path = DEST + "/Materials/" + name
    mat = LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(
        name, DEST + "/Materials", unreal.Material, unreal.MaterialFactoryNew())
    EDIT.delete_all_material_expressions(mat)
    EDIT.set_material_usage(mat, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    pigment = node(mat, unreal.MaterialExpressionTextureSample, -700, -220, texture=base)
    surface = node(mat, unreal.MaterialExpressionTextureSample, -700, 50, texture=packed,
                   sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    tint = node(mat, unreal.MaterialExpressionVectorParameter, -700, -410,
                parameter_name="CreatureTint", default_value=unreal.LinearColor(1, 1, 1, 1))
    color = node(mat, unreal.MaterialExpressionMultiply, -400, -220)
    link(pigment, "RGB", color, "A")
    link(tint, "RGB", color, "B")
    if pigment_floor:
        floor = node(mat, unreal.MaterialExpressionConstant3Vector, -390, -420,
                     constant=unreal.LinearColor(*pigment_floor, 1))
        lift = node(mat, unreal.MaterialExpressionMax, -150, -220)
        link(color, "", lift, "A")
        link(floor, "", lift, "B")
        property_link(lift, "", unreal.MaterialProperty.MP_BASE_COLOR)
    else:
        property_link(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    property_link(surface, "G", unreal.MaterialProperty.MP_ROUGHNESS)
    # Retain the generated dielectric/horn material response, including its
    # original packed map; no global metal tint or robotic material replacement.
    property_link(surface, "B", unreal.MaterialProperty.MP_METALLIC)
    status = node(mat, unreal.MaterialExpressionVectorParameter, -700, 400,
                  parameter_name="Color", default_value=unreal.LinearColor(1, .21, .055, 1))
    strength = node(mat, unreal.MaterialExpressionScalarParameter, -700, 570,
                    parameter_name="EmissiveStrength", default_value=0.6)
    vertex = node(mat, unreal.MaterialExpressionVertexColor, -700, 740)
    power = node(mat, unreal.MaterialExpressionMultiply, -400, 550)
    link(strength, "", power, "A")
    link(vertex, "R", power, "B")
    glow = node(mat, unreal.MaterialExpressionMultiply, -160, 400)
    link(status, "RGB", glow, "A")
    link(power, "", glow, "B")
    property_link(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    EDIT.recompile_material(mat)
    LIB.save_loaded_asset(mat)
    REPORT["materials"].append(mat.get_path_name())
    return mat


def eye_material(label):
    name = "M_OE_Eye" + label
    path = DEST + "/Materials/" + name
    mat = LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(
        name, DEST + "/Materials", unreal.Material, unreal.MaterialFactoryNew())
    EDIT.delete_all_material_expressions(mat)
    EDIT.set_material_usage(mat, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    color = (.24, .092, .012, 1) if label == "Iris" else (.003, .005, .002, 1)
    pigment = node(mat, unreal.MaterialExpressionConstant3Vector, -350, -100,
                   constant=unreal.LinearColor(*color))
    rough = node(mat, unreal.MaterialExpressionConstant, -350, 120, r=.20 if label == "Iris" else .12)
    specular = node(mat, unreal.MaterialExpressionConstant, -350, 240, r=.7)
    property_link(pigment, "", unreal.MaterialProperty.MP_BASE_COLOR)
    property_link(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    property_link(specular, "", unreal.MaterialProperty.MP_SPECULAR)
    EDIT.recompile_material(mat)
    LIB.save_loaded_asset(mat)
    REPORT["materials"].append(mat.get_path_name())
    return mat


def import_creature(asset, folder):
    report_path = folder / "prep-report.json"
    prep = json.loads(report_path.read_text(encoding="utf-8"))
    source = folder / prep["fbx"]
    if hashlib.sha256(source.read_bytes()).hexdigest() != prep["fbx_sha256"]:
        raise RuntimeError("Prepared FBX changed after its report: " + asset)
    base = texture(folder / "base_color.png", "T_OE_" + asset + "_Base", True)
    packed = texture(folder / "metallic_roughness.png", "T_OE_" + asset + "_MR", False)
    mat = material(asset, base, packed, prep.get("near_black_pigment_floor"))
    materials = {"M_OE_" + asset: mat}
    if prep.get("eye_placements"):
        materials.update({"M_OE_Eye" + label: eye_material(label) for label in ("Iris", "Pupil")})
    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("original_import_type", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_animations", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("create_physics_asset", False)
    data = options.get_editor_property("skeletal_mesh_import_data")
    data.set_editor_property("convert_scene", True)
    data.set_editor_property("convert_scene_unit", True)
    data.set_editor_property("force_front_x_axis", False)
    data.set_editor_property("import_uniform_scale", 1.0)
    data.set_editor_property("import_meshes_in_bone_hierarchy", True)
    data.set_editor_property("vertex_color_import_option", unreal.VertexColorImportOption.REPLACE)
    data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = DEST + "/Meshes"
    task.destination_name = "SK_OE_" + asset
    task.automated = task.replace_existing = task.save = True
    task.options = options
    TOOLS.import_asset_tasks([task])
    mesh = LIB.load_asset(task.destination_path + "/" + task.destination_name)
    if not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError("Skeletal import failed: " + asset)
    slots = mesh.get_editor_property("materials")
    if len(slots) != len(materials):
        raise RuntimeError("Unexpected PBR/eye material slot count on " + asset)
    for index, slot in enumerate(slots):
        name = str(slot.get_editor_property("material_slot_name"))
        if name not in materials:
            raise RuntimeError("Unexpected material slot " + name + " on " + asset)
        slot.set_editor_property("material_interface", materials[name])
    mesh.set_editor_property("materials", slots)
    bounds = mesh.get_imported_bounds()
    size = bounds.box_extent * 2
    expected = prep["height_metres"] * 100
    if abs(size.z - expected) > 4:
        raise RuntimeError("Skeletal FBX units are wrong: expected %.1f cm, got %.1f" % (expected, size.z))
    subsystem = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
    for bone in prep["bones"]:
        if bone["parent"] and str(subsystem.get_bone_parent(mesh, bone["name"])) != bone["parent"]:
            raise RuntimeError("Missing or incorrectly parented rig bone: " + bone["name"])
    LIB.save_loaded_asset(mesh)
    REPORT["assets"].append({"asset": mesh.get_path_name(), "source_fbx_sha256": prep["fbx_sha256"],
                             "source_glb_sha256": prep["source_sha256"], "dimensions_cm": [size.x, size.y, size.z],
                             "runtime_triangles_from_blender": prep["runtime_triangles"],
                             "validated_bone_parent_links": len(prep["bones"]) - 1,
                             "lod_count": subsystem.get_lod_count(mesh), "materials": len(slots),
                             "runtime_mesh_yaw": prep["runtime_mesh_yaw"]})


def main():
    settings = json.loads((ROOT.parents[1] / "config.local.json").read_text(encoding="utf-8"))
    output_root = Path(settings["tools"]["ai3d"]["windowsOutputRoot"])
    match = re.search(r"(?:^|\s)-OrganicEnemyAssets=([A-Za-z0-9_,]+)(?:\s|$)", unreal.SystemLibrary.get_command_line())
    assets = match.group(1).split(",") if match else ["Briarhide", "MireSeer"]
    for suffix in ("/Meshes", "/Materials", "/Textures"):
        LIB.make_directory(DEST + suffix)
    flag = "Interchange.FeatureFlags.Import.FBX"
    previous = unreal.SystemLibrary.get_console_variable_int_value(flag)
    unreal.SystemLibrary.execute_console_command(None, flag + " 0")
    try:
        for asset in assets:
            spec = json.loads((ROOT / "art/organic-enemies/rig" / (asset + ".json")).read_text(encoding="utf-8"))
            folder = output_root / "organic-enemies" / spec.get("source_folder", asset.lower()) / spec.get("output_folder", "finished")
            import_creature(asset, folder)
    finally:
        unreal.SystemLibrary.execute_console_command(None, flag + " " + str(previous))
    LIB.save_directory(DEST, only_if_is_dirty=True, recursive=True)
    REPORT["complete"] = True
    unreal.log("ORGANIC_ENEMIES_READY " + json.dumps(REPORT["assets"]))


if __name__ == "__main__":
    output = Path(unreal.Paths.project_saved_dir()).resolve() / "OrganicEnemies"
    output.mkdir(parents=True, exist_ok=True)
    try:
        main()
    except Exception as error:
        REPORT["error"] = str(error)
        REPORT["traceback"] = traceback.format_exc()
        raise
    finally:
        (output / "import.json").write_text(json.dumps(REPORT, indent=2) + "\n", encoding="utf-8")

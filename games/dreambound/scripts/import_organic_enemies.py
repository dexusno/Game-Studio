"""Import the finished organic creature FBX and its unchanged original PBR maps.

Run in the full Unreal editor using -ExecutePythonScript. Optional
-OrganicEnemyAssets=Briarhide,MireSeer selects prepared families. Sources live
under config.local.json's windowsOutputRoot/organic-enemies/<source_folder>/finished.
-OrganicEnemyEyesOnly updates the three existing Dread eye materials per family.
Missing source, rig bones or texture files are errors, never robot substitution.
"""
import hashlib
import importlib.util
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
    if asset == "MireSeer":
        definition = importlib.util.spec_from_file_location("organic_fire_materials", Path(__file__).with_name("import_organic_fire.py"))
        fire = importlib.util.module_from_spec(definition)
        definition.loader.exec_module(fire)
        fire.add_furnace_to_skin(mat, glow)
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


def anatomy_material(asset, definition):
    name = definition["name"]
    if name not in ("M_OE_Oral", "M_OE_Tooth", "M_OE_Tongue", "M_OE_DreadSkin",
                    "M_OE_DreadSocket", "M_OE_DreadEye", "M_OE_DreadIris", "M_OE_DreadPupil"):
        raise RuntimeError("Unexpected authored anatomy material: " + name)
    material_name = name + "_" + asset
    path = DEST + "/Materials/" + material_name
    mat = LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(
        material_name, DEST + "/Materials", unreal.Material, unreal.MaterialFactoryNew())
    EDIT.delete_all_material_expressions(mat)
    EDIT.set_material_usage(mat, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    defaults = {"M_OE_Oral": [.020, .006, .005], "M_OE_Tooth": [.58, .48, .31],
                "M_OE_Tongue": [.070, .018, .012]}
    rgb = definition.get("base_color", defaults.get(name, [.02, .01, .008]))
    if definition.get("vertex_color"):
        vertex = node(mat, unreal.MaterialExpressionVertexColor, -650, -100)
        tint = node(mat, unreal.MaterialExpressionVectorParameter, -650, -260,
                    parameter_name="CreatureTint", default_value=unreal.LinearColor(1, 1, 1, 1))
        pigment = node(mat, unreal.MaterialExpressionMultiply, -350, -100)
        link(vertex, "", pigment, "A")
        link(tint, "RGB", pigment, "B")
    else:
        pigment = node(mat, unreal.MaterialExpressionConstant3Vector, -350, -100,
                       constant=unreal.LinearColor(*rgb[:3], 1))
    rough = node(mat, unreal.MaterialExpressionConstant, -350, 120,
                 r=definition.get("roughness", .38 if name == "M_OE_Oral" else .42))
    specular = node(mat, unreal.MaterialExpressionConstant, -350, 240,
                    r=definition.get("specular", .5))
    property_link(pigment, "", unreal.MaterialProperty.MP_BASE_COLOR)
    property_link(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    property_link(specular, "", unreal.MaterialProperty.MP_SPECULAR)
    mat.set_editor_property("two_sided", bool(definition.get("two_sided", False)))
    if definition.get("emissive_strength", 0) > 0:
        emission = node(mat, unreal.MaterialExpressionConstant3Vector, -650, 420,
                        constant=unreal.LinearColor(*definition["emissive_color"][:3], 1))
        strength = node(mat, unreal.MaterialExpressionConstant, -650, 570,
                        r=definition["emissive_strength"])
        glow = node(mat, unreal.MaterialExpressionMultiply, -350, 420)
        link(emission, "", glow, "A")
        link(strength, "", glow, "B")
        property_link(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
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
    for definition in prep.get("extra_materials", []):
        materials[definition["name"]] = anatomy_material(asset, definition)
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
    # Reparented torso/shoulder chains need a distinct skeleton. Keep the
    # sixteen-bone mesh available for explicit baseline comparison.
    performance_rig = any(bone["name"] == "spine_lower" for bone in prep["bones"])
    anatomy_rig = bool(prep.get("anatomy_rig"))
    dread = prep.get("appearance_revision") == "dread-v1"
    task.destination_name = "SK_OE_" + asset + ("_Dread" if dread else "_Anatomy" if anatomy_rig else "_Performance" if performance_rig else "")
    task.automated = task.replace_existing = task.save = True
    task.options = options
    TOOLS.import_asset_tasks([task])
    mesh = LIB.load_asset(task.destination_path + "/" + task.destination_name)
    if not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError("Skeletal import failed: " + asset)
    slots = mesh.get_editor_property("materials")
    if len(slots) != len(materials):
        raise RuntimeError("Unexpected PBR/eye material slot count on " + asset)
    if str(slots[0].get_editor_property("material_slot_name")) != "M_OE_" + asset:
        raise RuntimeError("The runtime skin/tint material must remain slot zero: " + asset)
    for index, slot in enumerate(slots):
        name = str(slot.get_editor_property("material_slot_name"))
        if name not in materials:
            raise RuntimeError("Unexpected material slot " + name + " on " + asset)
        slot.set_editor_property("material_interface", materials[name])
        # Python receives a value copy of each SkeletalMaterial struct. Write
        # it back into the array before assigning the mesh's material slots.
        slots[index] = slot
    mesh.set_editor_property("materials", slots)
    assigned_slots = []
    for slot in mesh.get_editor_property("materials"):
        name = str(slot.get_editor_property("material_slot_name"))
        assigned = slot.get_editor_property("material_interface")
        if not assigned or assigned.get_path_name() != materials[name].get_path_name():
            raise RuntimeError("Material interface was not retained on " + asset + ": " + name)
        assigned_slots.append({"slot": name, "material": assigned.get_path_name()})
    bounds = mesh.get_imported_bounds()
    size = bounds.box_extent * 2
    expected = prep["height_metres"] * 100
    if abs(size.z - expected) > 4:
        raise RuntimeError("Skeletal FBX units are wrong: expected %.1f cm, got %.1f" % (expected, size.z))
    subsystem = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
    for bone in prep["bones"]:
        if bone["parent"] and str(subsystem.get_bone_parent(mesh, bone["name"])) != bone["parent"]:
            raise RuntimeError("Missing or incorrectly parented rig bone: " + bone["name"])
    # Imported tasks save before the material bindings above. A copied array
    # update does not reliably dirty the package, so persist it explicitly.
    if not LIB.save_loaded_asset(mesh, only_if_is_dirty=False):
        raise RuntimeError("Could not save the bound skeletal mesh: " + asset)
    REPORT["assets"].append({"asset": mesh.get_path_name(), "source_fbx_sha256": prep["fbx_sha256"],
                             "source_glb_sha256": prep["source_sha256"], "dimensions_cm": [size.x, size.y, size.z],
                             "source_prep_sha256": hashlib.sha256(report_path.read_bytes()).hexdigest(),
                             "runtime_triangles_from_blender": prep["runtime_triangles"],
                             "bone_count": len(prep["bones"]),
                             "validated_bone_parent_links": len(prep["bones"]) - 1,
                             "rig_variant": "dread" if dread else "anatomy" if anatomy_rig else "performance" if performance_rig else "motion",
                             "lod_count": subsystem.get_lod_count(mesh), "materials": len(slots),
                             "assigned_materials": assigned_slots,
                             "runtime_mesh_yaw": prep["runtime_mesh_yaw"]})


def main():
    settings = json.loads((ROOT.parents[1] / "config.local.json").read_text(encoding="utf-8"))
    output_root = Path(settings["tools"]["ai3d"]["windowsOutputRoot"])
    match = re.search(r"(?:^|\s)-OrganicEnemyAssets=([A-Za-z0-9_,]+)(?:\s|$)", unreal.SystemLibrary.get_command_line())
    assets = match.group(1).split(",") if match else ["Briarhide", "MireSeer"]
    eyes_only = "-OrganicEnemyEyesOnly" in unreal.SystemLibrary.get_command_line()
    REPORT["eye_material_only"] = eyes_only
    for suffix in ("/Meshes", "/Materials", "/Textures"):
        LIB.make_directory(DEST + suffix)
    flag = "Interchange.FeatureFlags.Import.FBX"
    previous = unreal.SystemLibrary.get_console_variable_int_value(flag)
    unreal.SystemLibrary.execute_console_command(None, flag + " 0")
    try:
        for asset in assets:
            spec = json.loads((ROOT / "art/organic-enemies/rig" / (asset + ".json")).read_text(encoding="utf-8"))
            appearance_path = ROOT / "art/organic-enemies/appearance" / (asset + ".json")
            appearance = json.loads(appearance_path.read_text(encoding="utf-8")) if appearance_path.is_file() else {}
            output_folder = "finished-dread" if appearance.get("revision") == "dread-v1" else spec.get("output_folder", "finished")
            folder = output_root / "organic-enemies" / spec.get("source_folder", asset.lower()) / output_folder
            if eyes_only:
                prep = json.loads((folder / "prep-report.json").read_text(encoding="utf-8"))
                mesh = LIB.load_asset(DEST + "/Meshes/SK_OE_" + asset + "_Dread")
                if not isinstance(mesh, unreal.SkeletalMesh):
                    raise RuntimeError("Eye-only update requires the existing Dread mesh: " + asset)
                bindings = {str(s.get_editor_property("material_slot_name")):
                            s.get_editor_property("material_interface").get_path_name()
                            for s in mesh.get_editor_property("materials")}
                eyes = [row for row in prep["extra_materials"] if row["name"] in
                        ("M_OE_DreadEye", "M_OE_DreadIris", "M_OE_DreadPupil")]
                if len(eyes) != 3:
                    raise RuntimeError("Expected three authored eye materials: " + asset)
                for row in eyes:
                    expected = DEST + "/Materials/" + row["name"] + "_" + asset
                    if bindings.get(row["name"]) != expected + "." + row["name"] + "_" + asset:
                        raise RuntimeError("Unexpected eye material binding: " + asset + ": " + row["name"])
                    anatomy_material(asset, row)
                REPORT["assets"].append({"asset": mesh.get_path_name(), "eye_material_only": True,
                                        "eye_materials": eyes})
            else:
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

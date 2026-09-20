"""Reproduce the local P01 map/material. Editor-only; contains no gameplay rules."""
import unreal

ROOT = "/Game/Technical"
unreal.EditorAssetLibrary.make_directory(ROOT)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
material_path = ROOT + "/M_HostMetal"
material = unreal.EditorAssetLibrary.load_asset(material_path) if unreal.EditorAssetLibrary.does_asset_exist(material_path) else None
if material is None:
    material = asset_tools.create_asset("M_HostMetal", ROOT, unreal.Material, unreal.MaterialFactoryNew())
    tint = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -420, 0)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.15, 0.21, 0.24, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    for value, prop, y in [(0.72, unreal.MaterialProperty.MP_METALLIC, 120), (0.38, unreal.MaterialProperty.MP_ROUGHNESS, 240)]:
        node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant, -420, y)
        node.set_editor_property("r", value)
        unreal.MaterialEditingLibrary.connect_material_property(node, "", prop)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)

map_path = ROOT + "/FoundryHost"
level_system = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(map_path):
    level_system.load_level(map_path)
else:
    if not level_system.new_level(map_path):
        raise RuntimeError("Failed to create the FoundryHost map")
if not level_system.save_current_level():
    raise RuntimeError("Failed to save the FoundryHost map")
unreal.log("FOUNDRY_CONTENT_READY: " + map_path + " and " + material_path)

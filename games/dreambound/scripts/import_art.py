"""Unreal 5.8 editor Python importer for the original Dreambound kit.

Run by the integration owner. Creates/reimports /Game/Art only; never edits a map,
project settings, game code, or audio. Requires Python + Editor Scripting plugins.
Source paths are relative to this script, with no private machine configuration.
Pass -DreamboundMaterialUsageOnly to update existing material usage without
rebuilding graphs or reimporting geometry/textures.
Pass -DreamboundMaterialsOnly to import textures and rebuild material graphs,
preserving all meshes, collision and assignments.
"""
import json
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[1]
GENERATED = ROOT / 'art' / 'generated'
DEST = '/Game/Art'
PALETTE = json.loads((GENERATED / 'materials.json').read_text(encoding='utf-8'))
METADATA = json.loads((GENERATED / 'asset-metadata.json').read_text(encoding='utf-8'))
ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EDIT = unreal.MaterialEditingLibrary
LIB = unreal.EditorAssetLibrary
REPORT = {'imported': [], 'materials': [], 'textures': [], 'warnings': [], 'scope': DEST}
INSTANCED_USAGE = unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES


def enable_instanced_usage(asset):
    # UE 5.8 deprecates the raw bUsedWithInstancedStaticMeshes property. Use its
    # supported accessor so both static and hierarchical instancing can compile.
    EDIT.set_base_material_usage(asset, INSTANCED_USAGE, True)
    if not EDIT.has_material_usage(asset, INSTANCED_USAGE):
        raise RuntimeError('Instanced static mesh usage was not enabled: ' + asset.get_path_name())


def compile_and_save_material(asset):
    messages = EDIT.recompile_material(asset)
    if messages:
        raise RuntimeError('Material compile failed for ' + asset.get_name() + ': ' + '; '.join(str(m) for m in messages))
    if not LIB.save_loaded_asset(asset):
        raise RuntimeError('Material save failed: ' + asset.get_path_name())


def import_texture(name, normal=False):
    task = unreal.AssetImportTask()
    task.filename = str(GENERATED / (name + '.png'))
    task.destination_path = DEST + '/Textures'
    task.destination_name = name
    task.automated = True; task.replace_existing = True; task.save = True
    ASSET_TOOLS.import_asset_tasks([task])
    texture = LIB.load_asset(task.destination_path + '/' + name)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError('Texture import failed: ' + name)
    texture.set_editor_property('srgb', False)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP if normal else unreal.TextureCompressionSettings.TC_MASKS)
    LIB.save_loaded_asset(texture)
    REPORT['textures'].append(texture.get_path_name())
    return texture


def node(material, cls, x, y, **properties):
    value = EDIT.create_material_expression(material, cls, x, y)
    if not value: raise RuntimeError('Could not create ' + cls.__name__)
    for key, data in properties.items(): value.set_editor_property(key, data)
    return value


def link(source, output, target, input_name):
    if not EDIT.connect_material_expressions(source, output, target, input_name):
        raise RuntimeError('Material connection failed: ' + output + ' -> ' + input_name)


def property_link(source, output, prop):
    if not EDIT.connect_material_property(source, output, prop):
        raise RuntimeError('Material property connection failed: ' + str(prop))


def scalar(material, value, x, y):
    return node(material, unreal.MaterialExpressionConstant, x, y, r=float(value))


def vector(material, value, x, y):
    return node(material, unreal.MaterialExpressionConstant3Vector, x, y, constant=unreal.LinearColor(*value, 1))


def material(name, spec, pigment, normal):
    path = DEST + '/Materials/' + name
    asset = LIB.load_asset(path) if LIB.does_asset_exist(path) else None
    if asset and not isinstance(asset, unreal.Material):
        raise RuntimeError('Refusing to overwrite non-material ' + path)
    if not asset:
        asset = ASSET_TOOLS.create_asset(name, DEST + '/Materials', unreal.Material, unreal.MaterialFactoryNew())
    if not asset: raise RuntimeError('Material creation failed: ' + name)
    enable_instanced_usage(asset)
    EDIT.delete_all_material_expressions(asset)
    asset.set_editor_property('two_sided', bool(spec.get('two_sided', False)))
    color = node(asset, unreal.MaterialExpressionVectorParameter, -1100, -360,
                 parameter_name='Color', default_value=unreal.LinearColor(*spec['color'], 1))
    if name == 'M_CombatGlow':
        property_link(color, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
        glow = node(asset, unreal.MaterialExpressionMultiply, -280, -280, const_b=3.0)
        link(color, 'RGB', glow, 'A'); property_link(glow, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        property_link(scalar(asset, .5, -250, 10), '', unreal.MaterialProperty.MP_ROUGHNESS)
    else:
        uv = node(asset, unreal.MaterialExpressionTextureCoordinate, -1200, 200, u_tiling=1.0, v_tiling=1.0)
        paint = node(asset, unreal.MaterialExpressionTextureSample, -1000, 80, texture=pigment,
                     sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        link(uv, '', paint, 'UVs')
        shade = node(asset, unreal.MaterialExpressionLinearInterpolate, -760, -20)
        link(vector(asset, spec.get('tone_low', [.88, .89, .90]), -1020, -30), '', shade, 'A')
        link(vector(asset, spec.get('tone_high', [1.07, 1.04, 1.0]), -1020, -120), '', shade, 'B')
        link(paint, 'R', shade, 'Alpha')
        tint = node(asset, unreal.MaterialExpressionMultiply, -560, -280)
        link(color, 'RGB', tint, 'A'); link(shade, '', tint, 'B')
        surface = tint
        if spec.get('patina_amount'):
            amount = node(asset, unreal.MaterialExpressionMultiply, -550, -60, const_b=float(spec['patina_amount']))
            link(paint, 'B', amount, 'A')
            coat = node(asset, unreal.MaterialExpressionLinearInterpolate, -380, -220)
            link(tint, '', coat, 'A')
            link(vector(asset, spec['patina_color'], -550, -160), '', coat, 'B')
            link(amount, '', coat, 'Alpha')
            surface = coat
        vc = node(asset, unreal.MaterialExpressionVertexColor, -750, -460)
        painted = node(asset, unreal.MaterialExpressionMultiply, -120, -280)
        # VertexColor's RGB pin is unnamed in the installed UE 5.8 source.
        link(surface, '', painted, 'A'); link(vc, '', painted, 'B')
        property_link(painted, '', unreal.MaterialProperty.MP_BASE_COLOR)
        variation = spec.get('roughness_variation', .08)
        rough = node(asset, unreal.MaterialExpressionLinearInterpolate, -550, 100,
                     const_a=max(.12, spec['roughness'] - variation), const_b=min(1.0, spec['roughness'] + variation))
        link(paint, 'G', rough, 'Alpha'); property_link(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
        property_link(scalar(asset, spec['metallic'], -250, 220), '', unreal.MaterialProperty.MP_METALLIC)
        normal_tex = node(asset, unreal.MaterialExpressionTextureSample, -750, 420, texture=normal,
                          sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        link(uv, '', normal_tex, 'UVs')
        restrained_normal = node(asset, unreal.MaterialExpressionLinearInterpolate, -450, 420, const_alpha=float(spec.get('normal_strength', .15)))
        link(vector(asset, [0, 0, 1], -750, 680), '', restrained_normal, 'A')
        link(normal_tex, 'RGB', restrained_normal, 'B')
        property_link(restrained_normal, '', unreal.MaterialProperty.MP_NORMAL)
        if spec.get('emissive'):
            glow = node(asset, unreal.MaterialExpressionMultiply, -260, -460, const_b=float(spec['emissive']))
            link(color, 'RGB', glow, 'A'); property_link(glow, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    compile_and_save_material(asset)
    REPORT['materials'].append(path)
    return asset


def import_mesh(name, meta, materials):
    options = unreal.FbxImportUI()
    options.set_editor_property('import_mesh', True)
    options.set_editor_property('import_as_skeletal', False)
    options.set_editor_property('import_materials', False)
    options.set_editor_property('import_textures', False)
    options.set_editor_property('import_animations', False)
    options.set_editor_property('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)
    options.set_editor_property('automated_import_should_detect_type', False)
    data = options.get_editor_property('static_mesh_import_data')
    for key, value in {
        'combine_meshes': True, 'auto_generate_collision': False,
        'one_convex_hull_per_ucx': True, 'generate_lightmap_u_vs': True,
        'remove_degenerates': True, 'build_nanite': False,
        # Exporter bakes its front-X conversion into vertices. Forcing front-X
        # again in UE swaps X/Y; default scene conversion preserves local axes.
        'convert_scene': True, 'convert_scene_unit': True, 'force_front_x_axis': False,
        'transform_vertex_to_absolute': True, 'import_uniform_scale': 1.0,
        'normal_import_method': unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS,
        'vertex_color_import_option': unreal.VertexColorImportOption.REPLACE,
    }.items():
        data.set_editor_property(key, value)
    task = unreal.AssetImportTask()
    task.filename = str(GENERATED / (name + '.fbx'))
    task.destination_path = DEST + '/Meshes'; task.destination_name = name
    task.automated = True; task.replace_existing = True; task.replace_existing_settings = True
    task.save = True; task.factory = unreal.FbxFactory(); task.options = options
    ASSET_TOOLS.import_asset_tasks([task])
    for path in task.imported_object_paths:
        if not str(path).startswith(DEST + '/'):
            raise RuntimeError('Unexpected import destination: ' + str(path))
    asset = LIB.load_asset(DEST + '/Meshes/' + name)
    if not isinstance(asset, unreal.StaticMesh): raise RuntimeError('Static mesh import failed: ' + name)
    slots = asset.get_editor_property('static_materials')
    assigned = []
    for index, slot in enumerate(slots):
        candidates = [str(slot.get_editor_property('imported_material_slot_name')), str(slot.get_editor_property('material_slot_name'))]
        match = next((candidate for candidate in candidates if candidate in materials), None)
        if match is None and index < len(meta['materials']):
            match = meta['materials'][index]
            REPORT['warnings'].append({'mesh': name, 'slot': index, 'imported_names': candidates, 'fallback': match})
        if match not in materials: raise RuntimeError('Unmapped material slot: ' + name + ': ' + str(candidates))
        asset.set_material(index, materials[match]); assigned.append(match)
    asset.set_editor_property('light_map_resolution', 128 if max(meta['dimensions_cm']) > 180 else 64)
    bounds = asset.get_bounds()
    dimensions = [bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2]
    origin = [bounds.origin.x, bounds.origin.y, bounds.origin.z]
    expected = meta['dimensions_cm']
    dimensions_ok = all(abs(dimensions[i] - expected[i]) < max(1.0, expected[i] * .02) for i in range(3))
    if not dimensions_ok:
        raise RuntimeError(f'FBX units/axes mismatch for {name}: expected {expected} cm, imported {dimensions} cm')
    expected_origin = [(meta['bounds_min_cm'][i] + meta['bounds_max_cm'][i]) * .5 for i in range(3)]
    # Size alone cannot detect a backwards gun. Check signed X and Z extents:
    # its muzzle must remain ahead of the grip, and the cuff must remain behind.
    if any(abs(origin[i]-expected_origin[i]) > 1.0 for i in (0,2)):
        raise RuntimeError(f'FBX pivot/direction mismatch for {name}: expected X/Z center {expected_origin}, got {origin}')
    body_setup = asset.get_editor_property('body_setup')
    hull_count = None
    if body_setup:
        geom = body_setup.get_editor_property('agg_geom')
        hull_count = len(geom.get_editor_property('convex_elems'))
    if meta['collision_hulls'] and (hull_count is not None and hull_count < meta['collision_hulls']):
        raise RuntimeError(f'Missing custom collision on {name}: expected {meta["collision_hulls"]}, got {hull_count}')
    # The arch imports separate side/stone hulls, never a solid opening blocker.
    LIB.save_loaded_asset(asset)
    REPORT['imported'].append({'name': name, 'path': asset.get_path_name(), 'dimensions_cm': dimensions, 'bounds_origin_cm': origin,
                               'materials': assigned, 'collision_hulls': hull_count})


def main():
    for directory in ['/Meshes', '/Materials', '/Textures']:
        LIB.make_directory(DEST + directory)
    pigment = import_texture('T_PaintedSurface')
    normal = import_texture('T_SculptedNormal', normal=True)
    materials = {name: material(name, spec, pigment, normal) for name, spec in PALETTE.items()}
    # UE 5.8's Interchange FBX interceptor otherwise ignores FbxImportUI, even
    # with an explicit FbxFactory. Select the matching importer for this run;
    # restore the process CVar and never change persistent project settings.
    flag='Interchange.FeatureFlags.Import.FBX'
    previous=unreal.SystemLibrary.get_console_variable_int_value(flag)
    unreal.SystemLibrary.execute_console_command(None, flag+' 0')
    try:
        for name, meta in METADATA.items(): import_mesh(name, meta, materials)
    finally:
        unreal.SystemLibrary.execute_console_command(None, flag+' '+str(previous))
    LIB.save_directory(DEST, only_if_is_dirty=True, recursive=True)
    REPORT['success'] = True
    (ROOT / 'art' / 'import-report.json').write_text(json.dumps(REPORT, indent=2) + '\n', encoding='utf-8')
    unreal.log('DREAMBOUND_ART_IMPORTED ' + json.dumps({'meshes': len(REPORT['imported']), 'materials': len(materials), 'scope': DEST}))


def update_material_usage_only():
    REPORT['mode'] = 'material_usage_only'
    for name in PALETTE:
        path = DEST + '/Materials/' + name
        asset = LIB.load_asset(path)
        if not isinstance(asset, unreal.Material):
            raise RuntimeError('Existing material missing: ' + path)
        enable_instanced_usage(asset)
        compile_and_save_material(asset)
        REPORT['materials'].append({'path': path, 'used_with_instanced_static_meshes': bool(EDIT.has_material_usage(asset, INSTANCED_USAGE))})
    REPORT['success'] = True
    (ROOT / 'art' / 'material-usage-report.json').write_text(json.dumps(REPORT, indent=2) + '\n', encoding='utf-8')
    unreal.log('DREAMBOUND_MATERIAL_USAGE_UPDATED ' + json.dumps({'materials': len(REPORT['materials']), 'used_with_instanced_static_meshes': True}))


def update_materials_only():
    REPORT['mode'] = 'materials_and_textures_only'
    pigment = import_texture('T_PaintedSurface')
    normal = import_texture('T_SculptedNormal', normal=True)
    for name, spec in PALETTE.items():
        material(name, spec, pigment, normal)
    REPORT['all_instancing_enabled'] = all(EDIT.has_material_usage(LIB.load_asset(path), INSTANCED_USAGE) for path in REPORT['materials'])
    if not REPORT['all_instancing_enabled']:
        raise RuntimeError('Material pass lost instancing usage')
    REPORT['success'] = True
    (ROOT / 'art' / 'material-pass-report.json').write_text(json.dumps(REPORT, indent=2) + '\n', encoding='utf-8')
    unreal.log('DREAMBOUND_MATERIALS_UPDATED ' + json.dumps({'materials': len(REPORT['materials']), 'textures': len(REPORT['textures']), 'meshes': 0}))


if __name__ == '__main__':
    command_line = unreal.SystemLibrary.get_command_line()
    usage_only = '-DreamboundMaterialUsageOnly' in command_line
    materials_only = '-DreamboundMaterialsOnly' in command_line
    try:
        if usage_only: update_material_usage_only()
        elif materials_only: update_materials_only()
        else: main()
    except Exception as error:
        REPORT['success'] = False; REPORT['error'] = str(error)
        report_name = 'material-usage-report.json' if usage_only else 'material-pass-report.json' if materials_only else 'import-report.json'
        (ROOT / 'art' / report_name).write_text(json.dumps(REPORT, indent=2) + '\n', encoding='utf-8')
        raise

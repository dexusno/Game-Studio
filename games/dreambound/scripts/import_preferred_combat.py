"""Restore the owner's preferred pre-Reverie cyborg/shield art only.

Reimport unchanged tracked original sources into a selective namespace. The
Reverie environment stays separate; this never imports the old landscape.
Run in the full Unreal editor with -ExecutePythonScript.
"""
import ast
import json
from pathlib import Path
import traceback
import unreal

ROOT = Path(__file__).resolve().parents[1]
GENERATED = ROOT / 'art/generated'
DEST = '/Game/Art/PreferredCombat'
ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EDIT = unreal.MaterialEditingLibrary
LIB = unreal.EditorAssetLibrary
INSTANCED_USAGE = unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES
TEXTURE_CACHE = {}
REPORT = {'complete': False, 'scope': DEST, 'imported': [], 'materials': [], 'textures': [], 'warnings': [],
          'reason': 'Owner preferred the previous cyborg and weapon graphics after reviewing Reverie.'}

# Preserve the exact original material graphs and the proven FBX import path.
# Only selected combat sources are passed to them; the old main is never run.
transport = Path(__file__).with_name('import_art.py')
parsed = ast.parse(transport.read_text(encoding='utf-8'))
functions = [n for n in parsed.body if isinstance(n, ast.FunctionDef)
             and n.name not in ('main', 'update_material_usage_only', 'update_materials_only')]
exec(compile(ast.Module(body=functions, type_ignores=[]), str(transport), 'exec'))


def main():
    for folder in ('/Meshes', '/Materials', '/Textures'):
        LIB.make_directory(DEST + folder)
    originals = json.loads((GENERATED / 'asset-metadata.json').read_text(encoding='utf-8'))
    segmented_dir = ROOT / 'art/segmented'
    segmented = json.loads((segmented_dir / 'asset-metadata.json').read_text(encoding='utf-8'))
    names = ('SM_Forearm', 'SM_Core', 'SM_Crystal', 'SM_GuardianBody', 'SM_GuardianHead',
             'SM_GuardianArm', 'SM_GuardianLeg', 'SM_ShieldHub', 'SM_ShieldSegment', 'SM_ShieldSegmentGlow')
    chosen = {name: (segmented_dir, segmented[name]) if name in segmented else (GENERATED, originals[name])
              for name in names}
    required = {'M_Ceramic', 'M_Bronze', 'M_DarkMetal', 'M_Core', 'M_CombatGlow', 'M_Frost', 'M_Ember', 'M_Storm'}
    for _, metadata in chosen.values():
        required.update(metadata['materials'])
    palette = json.loads((GENERATED / 'materials.json').read_text(encoding='utf-8'))
    pigment = import_texture('T_PaintedSurface')
    normal = import_texture('T_SculptedNormal', normal=True)
    materials = {name: material(name, palette[name], pigment, normal) for name in sorted(required)}
    flag = 'Interchange.FeatureFlags.Import.FBX'
    previous = unreal.SystemLibrary.get_console_variable_int_value(flag)
    unreal.SystemLibrary.execute_console_command(None, flag + ' 0')
    try:
        for name, (folder, metadata) in chosen.items():
            import_mesh(name, metadata, materials, folder)
    finally:
        unreal.SystemLibrary.execute_console_command(None, flag + ' ' + str(previous))
    LIB.save_directory(DEST, only_if_is_dirty=True, recursive=True)
    REPORT['complete'] = True
    unreal.log('PREFERRED_COMBAT_READY ' + json.dumps({'meshes': len(REPORT['imported']), 'materials': len(materials)}))


if __name__ == '__main__':
    output = Path(unreal.Paths.project_saved_dir()).resolve() / 'Reverie'
    output.mkdir(parents=True, exist_ok=True)
    try:
        main()
    except Exception as error:
        REPORT['error'] = str(error)
        REPORT['traceback'] = traceback.format_exc()
        raise
    finally:
        (output / 'preferred-combat-import.json').write_text(json.dumps(REPORT, indent=2) + '\n', encoding='utf-8')

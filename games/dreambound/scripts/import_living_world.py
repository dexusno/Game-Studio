"""Selective Unreal import of original LivingWorld habitat dressing.

Run with the full Editor's -ExecutePythonScript. No maps, gameplay code,
existing Reverie art, audio or project settings are modified by this importer.
"""
import ast
import json
from pathlib import Path
import traceback
import unreal

ROOT=Path(__file__).resolve().parents[1]
GENERATED=ROOT/'art/living-world'
DEST='/Game/Art/LivingWorld'
ASSET_TOOLS=unreal.AssetToolsHelpers.get_asset_tools()
EDIT=unreal.MaterialEditingLibrary
LIB=unreal.EditorAssetLibrary
INSTANCED_USAGE=unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES
TEXTURE_CACHE={}
REPORT={'complete':False,'scope':DEST,'imported':[],'materials':[],'textures':[],'warnings':[],
        'source':'Original Blender habitat meshes and original analytic texture fields',
        'collision':'None; decorative HISM placement outside protected paths/pads'}
transport=Path(__file__).with_name('import_art.py')
parsed=ast.parse(transport.read_text(encoding='utf-8'))
functions=[n for n in parsed.body if isinstance(n,ast.FunctionDef)
           and n.name not in ('main','update_material_usage_only','update_materials_only')]
exec(compile(ast.Module(body=functions,type_ignores=[]),str(transport),'exec'))


def add_motion(asset,kind):
    """Small material movement: anchored plants, common insect bob and wing tips.

    Vertex alpha is authored, not opacity. Root/stem and insect bodies remain
    fixed relative to their own common motion; only leaf and wing tips flutter.
    Bounds are expanded modestly after import so movement does not pop/cull.
    """
    time=node(asset,unreal.MaterialExpressionTime,-1600,1050)
    phase=node(asset,unreal.MaterialExpressionPerInstanceRandom,-1600,1200)
    speed=node(asset,unreal.MaterialExpressionMultiply,-1400,1050,const_b=1.1 if kind.startswith('insect') else .55)
    link(time,'',speed,'A')
    plus=node(asset,unreal.MaterialExpressionAdd,-1200,1050);link(speed,'',plus,'A');link(phase,'',plus,'B')
    sine=node(asset,unreal.MaterialExpressionSine,-1000,1050);link(plus,'',sine,'')
    motion=node(asset,unreal.MaterialExpressionMultiply,-800,1050);link(sine,'',motion,'A')
    link(vector(asset,[1.5,.7,2.4] if kind.startswith('insect') else [1.7,.8,.6],-1000,1200),'',motion,'B')
    if kind=='breeze':
        vc=node(asset,unreal.MaterialExpressionVertexColor,-1000,1360)
        tips=node(asset,unreal.MaterialExpressionMultiply,-600,1050);link(motion,'',tips,'A');link(vc,'A',tips,'B');motion=tips
    elif kind=='insect_wing':
        rate=node(asset,unreal.MaterialExpressionMultiply,-1400,1450,const_b=9.0);link(time,'',rate,'A')
        beat=node(asset,unreal.MaterialExpressionAdd,-1200,1450);link(rate,'',beat,'A');link(phase,'',beat,'B')
        flutter=node(asset,unreal.MaterialExpressionSine,-1000,1450);link(beat,'',flutter,'')
        vc=node(asset,unreal.MaterialExpressionVertexColor,-1000,1600)
        mask=node(asset,unreal.MaterialExpressionMultiply,-800,1450);link(flutter,'',mask,'A');link(vc,'A',mask,'B')
        lift=node(asset,unreal.MaterialExpressionMultiply,-600,1450);link(mask,'',lift,'A');link(vector(asset,[0,0,3.8],-800,1730),'',lift,'B')
        total=node(asset,unreal.MaterialExpressionAdd,-400,1050);link(motion,'',total,'A');link(lift,'',total,'B');motion=total
    property_link(motion,'',unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    compile_and_save_material(asset)


def main():
    for folder in ('/Meshes','/Materials','/Textures'):LIB.make_directory(DEST+folder)
    palette=json.loads((GENERATED/'materials.json').read_text())
    metadata=json.loads((GENERATED/'asset-metadata.json').read_text())
    pigment=import_texture('T_LW_Leaf');normal=import_texture('T_LW_LeafNormal',normal=True)
    materials={}
    for name,spec in palette.items():
        mat=material(name,spec,pigment,normal)
        if spec.get('motion'):add_motion(mat,spec['motion'])
        materials[name]=mat
    flag='Interchange.FeatureFlags.Import.FBX'
    previous=unreal.SystemLibrary.get_console_variable_int_value(flag)
    unreal.SystemLibrary.execute_console_command(None,flag+' 0')
    try:
        for name,meta in metadata.items():
            import_mesh(name,meta,materials,GENERATED)
            mesh=LIB.load_asset(DEST+'/Meshes/'+name)
            mesh.set_editor_property('positive_bounds_extension',unreal.Vector(8,8,8))
            mesh.set_editor_property('negative_bounds_extension',unreal.Vector(8,8,8))
            LIB.save_loaded_asset(mesh)
    finally:
        unreal.SystemLibrary.execute_console_command(None,flag+' '+str(previous))
    LIB.save_directory(DEST,only_if_is_dirty=True,recursive=True)
    REPORT['complete']=True
    unreal.log('LIVING_WORLD_CONTENT_READY '+json.dumps({'meshes':len(metadata),'materials':len(materials),'textures':len(REPORT['textures'])}))


if __name__=='__main__':
    output=Path(unreal.Paths.project_saved_dir()).resolve()/'LivingWorld';output.mkdir(parents=True,exist_ok=True)
    try:main()
    except Exception as error:
        REPORT['error']=str(error);REPORT['traceback']=traceback.format_exc();raise
    finally:(output/'import.json').write_text(json.dumps(REPORT,indent=2)+'\n',encoding='utf-8')

"""Build reproducible original materials/imports and the empty runtime entry map."""
from pathlib import Path
import json
import unreal
ROOT=Path(__file__).resolve().parents[1]
LIB=unreal.EditorAssetLibrary
EDIT=unreal.MaterialEditingLibrary
TOOLS=unreal.AssetToolsHelpers.get_asset_tools()
DEST='/Game/MagnetSweep'
for directory in ['Materials','Meshes','Audio']:
    LIB.make_directory(DEST+'/'+directory)
path=DEST+'/Materials/M_Surface'
mat=LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset('M_Surface',DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
EDIT.delete_all_material_expressions(mat)
def node(cls,**props):
    n=EDIT.create_material_expression(mat,cls,0,0)
    for key,value in props.items(): n.set_editor_property(key,value)
    return n
color=node(unreal.MaterialExpressionVectorParameter,parameter_name='Color',default_value=unreal.LinearColor(.2,.3,.35,1))
rough=node(unreal.MaterialExpressionScalarParameter,parameter_name='Roughness',default_value=.4)
metal=node(unreal.MaterialExpressionScalarParameter,parameter_name='Metallic',default_value=.35)
glow=node(unreal.MaterialExpressionScalarParameter,parameter_name='Glow',default_value=0)
em=node(unreal.MaterialExpressionMultiply)
EDIT.connect_material_expressions(color,'',em,'A'); EDIT.connect_material_expressions(glow,'',em,'B')
for n,prop in [(color,unreal.MaterialProperty.MP_BASE_COLOR),(rough,unreal.MaterialProperty.MP_ROUGHNESS),(metal,unreal.MaterialProperty.MP_METALLIC),(em,unreal.MaterialProperty.MP_EMISSIVE_COLOR)]: EDIT.connect_material_property(n,'',prop)
EDIT.recompile_material(mat);LIB.save_asset(path)
imports=[]
for folder,ext,destination in [('models','*.fbx','Meshes'),('audio','*.wav','Audio')]:
    for source in sorted((ROOT/'assets'/folder).glob(ext)):
        task=unreal.AssetImportTask();task.filename=str(source);task.destination_path=DEST+'/'+destination;task.destination_name=source.stem
        task.automated=True;task.replace_existing=True;task.save=True
        if destination=='Meshes':
            options=unreal.FbxImportUI();options.import_mesh=True;options.import_as_skeletal=False;options.import_materials=False;options.import_textures=False
            options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
            options.static_mesh_import_data.combine_meshes=True;options.static_mesh_import_data.generate_lightmap_u_vs=False
            task.options=options
        TOOLS.import_asset_tasks([task]);imports.extend(task.imported_object_paths)
editor=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
map_path='/Game/Maps/Concept'
if LIB.does_asset_exist(map_path):editor.load_level(map_path)
else:editor.new_level(map_path)
editor.save_current_level();LIB.save_directory(DEST,only_if_is_dirty=False,recursive=True)
output=Path(unreal.Paths.project_saved_dir())/'ConceptContent.json';output.write_text(json.dumps({'map':map_path,'imports':imports,'complete':True},indent=2),encoding='utf-8')
unreal.log('MAGNET_SWEEP_CONTENT_READY')
unreal.SystemLibrary.quit_editor()

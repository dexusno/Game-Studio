"""Import the visual overhaul, build PBR materials, and stage licensed fonts."""
from pathlib import Path
import json, shutil
import unreal
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/'assets/visual-overhaul'
DEST='/Game/MagnetSweep'
LIB=unreal.EditorAssetLibrary
EDIT=unreal.MaterialEditingLibrary
TOOLS=unreal.AssetToolsHelpers.get_asset_tools()
imports=[]
kit_only='-VisualKitOnly' in unreal.SystemLibrary.get_command_line()
for sub in ['Meshes','Textures/Visuals','Materials']:LIB.make_directory(DEST+'/'+sub)
def load_import(source,folder,mesh=False):
 task=unreal.AssetImportTask();task.filename=str(source);task.destination_path=DEST+'/'+folder;task.destination_name=source.stem
 task.automated=True;task.replace_existing=True;task.save=True
 if mesh:
  opts=unreal.FbxImportUI();opts.import_mesh=True;opts.import_as_skeletal=False;opts.import_materials=False;opts.import_textures=False
  opts.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
  opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.generate_lightmap_u_vs=False
  task.options=opts
 TOOLS.import_asset_tasks([task])
 if not task.imported_object_paths:raise RuntimeError('No imported asset: '+str(source))
 imports.extend(task.imported_object_paths)
 return LIB.load_asset(task.imported_object_paths[0])
for folder in (['kit'] if kit_only else ['kit','hero']):
 for source in sorted((SRC/folder).glob('*.fbx')):load_import(source,'Meshes',True)
textures={}
for source in sorted(SRC.rglob('T_*.png')):
 if kit_only and source.parent.name=='hero':continue
 texture=load_import(source,'Textures/Visuals');textures[source.stem]=texture
 if 'Normal' in source.stem:
  texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
  texture.set_editor_property('srgb',False);texture.set_editor_property('flip_green_channel',True)
 elif source.stem.endswith('_MR'):texture.set_editor_property('srgb',False)
 LIB.save_loaded_asset(texture)
def make_mat(name):
 path=DEST+'/Materials/'+name
 m=LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(name,DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
 EDIT.delete_all_material_expressions(m)
 return m
def node(mat,cls,**props):
 n=EDIT.create_material_expression(mat,cls,0,0)
 for k,v in props.items():n.set_editor_property(k,v)
 return n
def link(a,output,b,input):
 if not EDIT.connect_material_expressions(a,output,b,input):
  raise RuntimeError(f'Material connection failed: {a.get_class().get_name()}:{output} -> {b.get_class().get_name()}:{input}')
mat=make_mat('M_VisualSurface')
color=node(mat,unreal.MaterialExpressionVectorParameter,parameter_name='Color',default_value=unreal.LinearColor(.2,.3,.35,1))
metal=node(mat,unreal.MaterialExpressionScalarParameter,parameter_name='Metallic',default_value=.7)
rough=node(mat,unreal.MaterialExpressionScalarParameter,parameter_name='Roughness',default_value=.35)
glow=node(mat,unreal.MaterialExpressionScalarParameter,parameter_name='Glow',default_value=0)
sample=node(mat,unreal.MaterialExpressionTextureSample,texture=textures['T_MachinedDetail'])
position=node(mat,unreal.MaterialExpressionWorldPosition)
xy=node(mat,unreal.MaterialExpressionComponentMask,r=True,g=True);link(position,'',xy,'')
uv=node(mat,unreal.MaterialExpressionDivide,const_b=100);link(xy,'',uv,'A');link(uv,'',sample,'')
detail=node(mat,unreal.MaterialExpressionMultiply,const_b=.30);link(sample,'R',detail,'A')
brightness=node(mat,unreal.MaterialExpressionAdd,const_b=.85);link(detail,'',brightness,'A')
base=node(mat,unreal.MaterialExpressionMultiply);link(color,'',base,'A');link(brightness,'',base,'B')
rough_detail=node(mat,unreal.MaterialExpressionMultiply,const_b=.18);link(sample,'R',rough_detail,'A')
final_rough=node(mat,unreal.MaterialExpressionAdd);link(rough,'',final_rough,'A');link(rough_detail,'',final_rough,'B')
em=node(mat,unreal.MaterialExpressionMultiply);link(color,'',em,'A');link(glow,'',em,'B')
for n,p in [(base,unreal.MaterialProperty.MP_BASE_COLOR),(metal,unreal.MaterialProperty.MP_METALLIC),(final_rough,unreal.MaterialProperty.MP_ROUGHNESS),(em,unreal.MaterialProperty.MP_EMISSIVE_COLOR)]:EDIT.connect_material_property(n,'',p)
EDIT.recompile_material(mat);LIB.save_loaded_asset(mat)
# Slow moving emissive heat inside the rebuilt crucible; entirely visual.
mat=make_mat('M_FurnaceHeat')
mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
pos=node(mat,unreal.MaterialExpressionWorldPosition)
scale=node(mat,unreal.MaterialExpressionMultiply,const_b=.033);link(pos,'',scale,'A')
time=node(mat,unreal.MaterialExpressionTime)
speed=node(mat,unreal.MaterialExpressionConstant3Vector,constant=unreal.LinearColor(.075,.035,.045))
travel=node(mat,unreal.MaterialExpressionMultiply);link(time,'',travel,'A');link(speed,'',travel,'B')
flow=node(mat,unreal.MaterialExpressionAdd);link(scale,'',flow,'A');link(travel,'',flow,'B')
noise=node(mat,unreal.MaterialExpressionNoise,scale=1,quality=1,levels=2,output_min=0,output_max=1);link(flow,'',noise,'')
dark=node(mat,unreal.MaterialExpressionConstant3Vector,constant=unreal.LinearColor(.32,.018,.001))
hot=node(mat,unreal.MaterialExpressionConstant3Vector,constant=unreal.LinearColor(4.5,1.2,.045))
heat=node(mat,unreal.MaterialExpressionLinearInterpolate);link(dark,'',heat,'A');link(hot,'',heat,'B');link(noise,'',heat,'Alpha')
EDIT.connect_material_property(heat,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
EDIT.recompile_material(mat);LIB.save_loaded_asset(mat)
hero_ready=all(n in textures for n in ['T_HeroMagnet_BaseColor','T_HeroMagnet_MR','T_HeroMagnet_Normal'])
if hero_ready:
 mat=make_mat('M_HeroMagnet')
 base=node(mat,unreal.MaterialExpressionTextureSample,texture=textures['T_HeroMagnet_BaseColor'])
 mr=node(mat,unreal.MaterialExpressionTextureSample,texture=textures['T_HeroMagnet_MR'],sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
 normal=node(mat,unreal.MaterialExpressionTextureSample,texture=textures['T_HeroMagnet_Normal'],sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
 EDIT.connect_material_property(base,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
 EDIT.connect_material_property(mr,'G',unreal.MaterialProperty.MP_ROUGHNESS);EDIT.connect_material_property(mr,'B',unreal.MaterialProperty.MP_METALLIC)
 EDIT.connect_material_property(normal,'RGB',unreal.MaterialProperty.MP_NORMAL)
 EDIT.recompile_material(mat);LIB.save_loaded_asset(mat)
font_out=ROOT/'unreal/Content/Fonts';font_out.mkdir(parents=True,exist_ok=True)
for file in (SRC/'fonts').iterdir():
 if file.suffix.lower() in ['.ttf','.txt']:shutil.copy2(file,font_out/file.name)
LIB.save_directory(DEST,only_if_is_dirty=False,recursive=True)
report={'complete':not kit_only,'hero_ready':hero_ready,'kit_only':kit_only,'imports':imports,'fonts':[f.name for f in font_out.glob('*.ttf')]}
out=Path(unreal.Paths.project_saved_dir())/'VisualContent.json';out.write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('MAGNET_VISUAL_CONTENT_READY')
unreal.SystemLibrary.quit_editor()

"""Import only the new Reverie graphics/audio into Unreal's separate namespaces.

Run in the full editor with -ExecutePythonScript. Legacy assets are preserved.
New shaders, source geometry and source recordings never silently fall back to
the preceding kit. The generated fountain source lives in config.local.json's
existing private output root.
"""
import ast
import importlib.util
import json
from pathlib import Path
import traceback
import unreal

ROOT=Path(__file__).resolve().parents[1]
KIT=ROOT/'art/reverie'
DEST='/Game/Art/Reverie'
GENERATED=KIT/'characters'
ASSET_TOOLS=unreal.AssetToolsHelpers.get_asset_tools()
EDIT=unreal.MaterialEditingLibrary
LIB=unreal.EditorAssetLibrary
REPORT={'complete':False,'imported':[],'materials':[],'textures':[],'warnings':[],'audio':[]}
SURFACES_ONLY='-ReverieSurfacesOnly' in unreal.SystemLibrary.get_command_line()
REPORT['scope']='materials_and_audio' if SURFACES_ONLY else 'complete_kit'
INSTANCED_USAGE=unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES

# Reuse the tested FBX transport and its dimension/UCX checks, not old assets.
transport=Path(__file__).with_name('import_art.py')
parsed=ast.parse(transport.read_text(encoding='utf-8'))
names={'import_mesh','enable_instanced_usage','compile_and_save_material','node','link','property_link','scalar','vector'}
exec(compile(ast.Module(body=[n for n in parsed.body if isinstance(n,ast.FunctionDef) and n.name in names],type_ignores=[]),str(transport),'exec'))

def custom(mat,code,inputs,output=unreal.CustomMaterialOutputType.CMOT_FLOAT3,x=-450,y=0):
 n=node(mat,unreal.MaterialExpressionCustom,x,y,code=code,output_type=output)
 pins=[]
 for k in inputs:
  pin=unreal.CustomInput();pin.set_editor_property('input_name',k);pins.append(pin)
 n.set_editor_property('inputs',pins)
 for k,(src,pin) in inputs.items():link(src,pin,n,k)
 return n

def texture(path,name,srgb=True):
 t=unreal.AssetImportTask();t.filename=str(path);t.destination_path=DEST+'/Textures';t.destination_name=name
 t.automated=True;t.replace_existing=True;t.save=True;ASSET_TOOLS.import_asset_tasks([t])
 result=LIB.load_asset(t.destination_path+'/'+name)
 if not isinstance(result,unreal.Texture2D):raise RuntimeError('Missing texture '+name)
 result.set_editor_property('srgb',srgb);LIB.save_loaded_asset(result)
 REPORT['textures'].append(result.get_path_name());return result

def make_material(name,spec,limestone,sky):
 path=DEST+'/Materials/'+name
 mat=LIB.load_asset(path) if LIB.does_asset_exist(path) else ASSET_TOOLS.create_asset(name,DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
 EDIT.delete_all_material_expressions(mat);enable_instanced_usage(mat)
 mat.set_editor_property('two_sided',name in ('M_RV_Sky','M_RV_Water','M_RV_Waterfall') or 'Leaf' in name or 'Grass' in name or 'Flower' in name)
 color=spec.get('color',[.3,.32,.25]);rough=spec.get('roughness',.65);metal=spec.get('metallic',0)
 tint=node(mat,unreal.MaterialExpressionVectorParameter,-1100,-400,parameter_name='Color',default_value=unreal.LinearColor(*color,1))
 uv=node(mat,unreal.MaterialExpressionTextureCoordinate,-1100,150)
 world=node(mat,unreal.MaterialExpressionWorldPosition,-1100,350)
 time=node(mat,unreal.MaterialExpressionTime,-1100,600)
 if name=='M_RV_Sky':
  mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
  mat.set_editor_property('is_sky',True)
  camera=node(mat,unreal.MaterialExpressionCameraPositionWS,-1100,500)
  panorama_uv=custom(mat,'float3 d=normalize(P-C);return float2(atan2(d.y,d.x)/6.2831853+.5+T*.00035,acos(clamp(d.z,-1.,1.))/3.14159265);',{'P':(world,''),'C':(camera,''),'T':(time,'')},output=unreal.CustomMaterialOutputType.CMOT_FLOAT2,y=100)
  panorama=node(mat,unreal.MaterialExpressionTextureSample,-250,100,texture=sky);link(panorama_uv,'',panorama,'UVs')
  code='''float3 d=normalize(P-C);float3 col=Tex*.85;
float haze=1-smoothstep(0,.2,abs(d.z));col=lerp(col,float3(.64,.65,.53),haze*.18);
float sun=pow(saturate(dot(d,normalize(float3(-.42,-.5,.73)))),640);
return col+float3(1,.69,.35)*sun*2.5;'''
  value=custom(mat,code,{'P':(world,''),'C':(camera,''),'Tex':(panorama,'RGB')})
  property_link(value,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 elif name=='M_RV_Waterfall':
  mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
  mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
  value=custom(mat,'float streak=pow(.5+.5*sin(P.x*.41+P.y*.32+sin(P.z*.03+T*8)),3);return float3(.14,.32,.27)+streak*float3(.27,.38,.31);',{'P':(world,''),'T':(time,'')})
  property_link(value,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
  opacity=custom(mat,'float s=.5+.5*sin(P.z*.12+T*12+sin(P.x*.15));return .22+s*.35;',{'P':(world,''),'T':(time,'')},output=unreal.CustomMaterialOutputType.CMOT_FLOAT1,y=450)
  property_link(opacity,'',unreal.MaterialProperty.MP_OPACITY)
 elif name=='M_RV_Water':
  colorcode='''float2 q=P.xy*.009; float t=T*.65;
float wave=sin(q.x*2.1+t+sin(q.y*1.1))*sin(q.y*2.7-t*.7);
float caustic=pow(saturate(sin(q.x*5+sin(q.y*4+t))+sin(q.y*4.2+sin(q.x*3-t))-.65),3)*.04;
return lerp(float3(.018,.083,.073),float3(.052,.23,.17),wave*.25+.45)+caustic;'''
  value=custom(mat,colorcode,{'P':(world,''),'T':(time,'')})
  property_link(value,'',unreal.MaterialProperty.MP_BASE_COLOR)
  norm=custom(mat,'float2 q=P.xy*.035;return normalize(float3(sin(q.x+T+sin(q.y))*.19,cos(q.y*1.24-T*.73+sin(q.x))*.17,1));',{'P':(world,''),'T':(time,'')},y=300)
  property_link(norm,'',unreal.MaterialProperty.MP_NORMAL)
  property_link(scalar(mat,.13,-200,500),'',unreal.MaterialProperty.MP_ROUGHNESS)
  property_link(scalar(mat,.36,-200,600),'',unreal.MaterialProperty.MP_METALLIC)
  property_link(scalar(mat,.68,-200,700),'',unreal.MaterialProperty.MP_SPECULAR)
 else:
  vertex=node(mat,unreal.MaterialExpressionVertexColor,-1100,-100)
  stone=any(s in name for s in ('Stone','Mortar','Moss','CrystalBase','Soil','Bark'))
  tex=node(mat,unreal.MaterialExpressionTextureSample,-850,150,texture=limestone)
  if name=='M_RV_Soil':
   soil_uv=custom(mat,'return P.xy*.004;',{'P':(world,'')},output=unreal.CustomMaterialOutputType.CMOT_FLOAT2,y=180)
   link(soil_uv,'',tex,'UVs')
  else:link(uv,'',tex,'UVs')
  if name=='M_RV_Soil':
   # A2.5m world tile preserves visible litter and damp/moss colour contrast.
   # Stone's uniform tint/wear lift erased that contrast in the first review.
   code='float damp=.88+.12*sin(P.x*.0011+sin(P.y*.0013));return Tex*float3(.90,1.04,.88)*damp;'
  elif stone:
   code='''float3 detail=lerp(float3(.7,.7,.7),Tex*1.75,.63);
float weather=saturate(V.b*.55+(sin(P.x*.0014+sin(P.y*.002))+sin(P.y*.0018+P.z*.002))*.065);
float3 base=lerp(C*detail,C*float3(.52,.69,.44),weather);
return max(float3(.008,.008,.008),base*(.72+V.r*.3)+V.g*.055);'''
  elif name in ('M_RV_Core','M_RV_Crystal'):
   code='float vein=pow(saturate(1-abs(sin((P.z+P.x*.7+sin(P.y*.14)*4)*.115))),22);return C*(.17+Tex*.48)*( .76+V.r*.24)+vein*float3(.045,.065,.048);'
  else:
   code='float value=.91+.06*sin(P.z*.075+sin(P.x*.11));return C*value*(.8+V.r*.2)+V.g*.035;'
  value=custom(mat,code,{'C':(tint,'RGB'),'Tex':(tex,'RGB'),'V':(vertex,''),'P':(world,'')})
  property_link(value,'',unreal.MaterialProperty.MP_BASE_COLOR)
  property_link(scalar(mat,rough,-200,420),'',unreal.MaterialProperty.MP_ROUGHNESS)
  property_link(scalar(mat,metal,-200,510),'',unreal.MaterialProperty.MP_METALLIC)
  # Sculpted geometry supplies relief. Analytic high-frequency UV normals
  # alias into a striped weave at playing distance, so never use them here.
  if 'Leaf' in name or 'Grass' in name or 'Flower' in name:
   wind=custom(mat,'float a=sin(P.x*.007+T*1.15)+sin(P.y*.009+T*.63);return float3(a*1.6,sin(P.x*.005+T)*.8,0);',{'P':(world,''),'T':(time,'')},y=700)
   property_link(wind,'',unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
  if name in ('M_RV_Core','M_RV_CombatGlow','M_RV_Frost','M_RV_Storm','M_RV_Ember','M_RV_Crystal'):
   strength=node(mat,unreal.MaterialExpressionScalarParameter,-600,-600,parameter_name='EmissiveStrength',default_value=1.4 if name=='M_RV_CombatGlow' else .035 if name in ('M_RV_Core','M_RV_Crystal') else .16)
   glow=node(mat,unreal.MaterialExpressionMultiply,-200,-450);link(tint,'RGB',glow,'A');link(strength,'',glow,'B')
   property_link(glow,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 compile_and_save_material(mat);REPORT['materials'].append(path);return mat

def sculpted_landmarks():
 config=json.loads((ROOT.parents[1]/'config.local.json').read_text(encoding='utf-8-sig'))
 private_root=Path(config['tools']['ai3d']['windowsOutputRoot'])/'reverie-v1'
 spec=importlib.util.spec_from_file_location('reverie_trellis',Path(__file__).with_name('inspect_ai3d_unreal.py'))
 helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
 REPORT['sculpted_landmarks']={}
 for name,subdir in (('FountainHero','fountain'),('SculptedBank','root-bank'),('CrownTree','crown-tree')):
  folder=private_root/subdir/'finished'
  opt={'asset_root':DEST,'mesh_name':'SM_RV_'+name,'material_name':'M_RV_'+name,
   'fbx':str(folder/('SM_RV_'+name+'.fbx')),'base_color':str(folder/'base_color.png'),'metallic_roughness':str(folder/'metallic_roughness.png')}
  for key in ('fbx','base_color','metallic_roughness'):
   if not Path(opt[key]).is_file():raise RuntimeError('Required new landmark source is missing: '+opt[key])
  base=helper.import_texture(opt,'base_color','T_RV_'+name+'_Base',True)
  packed=helper.import_texture(opt,'metallic_roughness','T_RV_'+name+'_MR',False)
  mat=helper.create_material(opt,base,packed);mesh=helper.import_mesh(opt,mat)
  hulls=helper.MESHES.get_convex_collision_count(mesh);uvs=helper.MESHES.get_num_uv_channels(mesh,0)
  triangles=mesh.get_num_nanite_triangles();nanite=bool(helper.MESHES.get_nanite_settings(mesh).get_editor_property('enabled'))
  if hulls!=3 or uvs!=2 or not nanite or triangles<=0:raise RuntimeError('New landmark import contract failed: '+name)
  REPORT['sculpted_landmarks'][name]={'mesh':mesh.get_path_name(),'material':mat.get_path_name(),'triangles':triangles,'collision_hulls':hulls,'uv_channels':uvs,'nanite_enabled':nanite,
   'dimensions_cm':[v*2 for v in helper.vec(mesh.get_bounds().box_extent)],'textures':[{'path':t.get_path_name(),'width':t.blueprint_get_size_x(),'height':t.blueprint_get_size_y(),'srgb':t.get_editor_property('srgb')} for t in (base,packed)]}
 REPORT['warnings'].extend(helper.REPORT.get('warnings',[]))

def main():
 for p in ('/Meshes','/Materials','/Textures'):LIB.make_directory(DEST+p)
 palette={}
 for family in ('environment','characters'):
  palette.update(json.loads((KIT/family/'materials.json').read_text(encoding='utf-8')))
 palette.update({'M_RV_CombatGlow':{'color':[.32,.9,.62],'roughness':.24,'metallic':.1},
 'M_RV_Frost':{'color':[.12,.62,.88],'roughness':.16,'metallic':.2},
 'M_RV_Storm':{'color':[.42,.16,.85],'roughness':.21,'metallic':.1},
 'M_RV_Ember':{'color':[.95,.22,.032],'roughness':.3,'metallic':.1},
 'M_RV_Waterfall':{'color':[.1,.3,.27]},
 'M_RV_Soil':{'color':[.36,.28,.17],'roughness':.94,'metallic':0}})
 limestone=texture(KIT/'textures/T_RV_Limestone.png','T_RV_Limestone')
 bark=texture(KIT/'textures/T_RV_Bark.png','T_RV_Bark')
 soil=texture(KIT/'textures/T_RV_Soil.png','T_RV_Soil')
 sky=texture(KIT/'textures/T_RV_Sky.png','T_RV_Sky')
 materials={name:make_material(name,data,soil if name=='M_RV_Soil' else bark if 'Bark' in name else limestone,sky) for name,data in palette.items()}
 if not SURFACES_ONLY:
  flag='Interchange.FeatureFlags.Import.FBX';prev=unreal.SystemLibrary.get_console_variable_int_value(flag)
  unreal.SystemLibrary.execute_console_command(None,flag+' 0')
  try:
   for family in ('environment','characters'):
    folder=KIT/family;metadata=json.loads((folder/'asset-metadata.json').read_text(encoding='utf-8'))
    for name,data in metadata.items():import_mesh(name,data,materials,folder)
  finally:unreal.SystemLibrary.execute_console_command(None,flag+' '+str(prev))
  sculpted_landmarks()
 audio_report=json.loads((ROOT/'assets/audio-reverie/audio-report.json').read_text(encoding='utf-8'))
 REPORT['audio_settings']={}
 attenuation_path='/Game/Audio/Reverie/A_RV_Threats'
 attenuation=LIB.load_asset(attenuation_path) if LIB.does_asset_exist(attenuation_path) else ASSET_TOOLS.create_asset('A_RV_Threats','/Game/Audio/Reverie',unreal.SoundAttenuation,unreal.SoundAttenuationFactory())
 spatial=attenuation.get_editor_property('attenuation')
 spatial.set_editor_property('spatialize',True);spatial.set_editor_property('attenuate',True)
 spatial.set_editor_property('attenuation_shape_extents',unreal.Vector(350,0,0))
 spatial.set_editor_property('falloff_distance',1850.0)
 attenuation.set_editor_property('attenuation',spatial);LIB.save_loaded_asset(attenuation)
 for source in sorted((ROOT/'assets/audio-reverie').glob('S_*.wav')):
  task=unreal.AssetImportTask();task.filename=str(source);task.destination_path='/Game/Audio/Reverie';task.destination_name=source.stem
  task.automated=True;task.replace_existing=True;task.save=True;ASSET_TOOLS.import_asset_tasks([task])
  sound=LIB.load_asset('/Game/Audio/Reverie/'+source.stem)
  if not isinstance(sound,unreal.SoundWave):raise RuntimeError('Sound import failed '+source.stem)
  if source.stem in ('S_ChargeLoop','S_WaterLoop','S_WindLoop'):sound.set_editor_property('looping',True)
  recipe=audio_report['cues'][source.stem]['recipe']
  settings=unreal.SoundConcurrencySettings()
  # Each cue's override is a shared per-SoundWave group, across all enemies.
  cap=3 if source.stem=='S_WaterLoop' else recipe['voice_cap']
  settings.set_editor_property('max_count',cap)
  settings.set_editor_property('limit_to_owner',False)
  settings.set_editor_property('resolution_rule',unreal.MaxConcurrentResolutionRule.STOP_FARTHEST_THEN_OLDEST)
  sound.set_editor_property('override_concurrency',True)
  sound.set_editor_property('concurrency_overrides',settings)
  sound.set_editor_property('priority',float(recipe['priority']))
  if source.stem.startswith('S_Enemy') or source.stem in ('S_BossTell','S_Impact','S_HeavyImpact','S_Guard'):
   sound.set_editor_property('attenuation_settings',attenuation)
  if source.stem=='S_WaterLoop':
   # Keep one loop for each distant fountain alive as the listener travels.
   # 400+900cm attenuation at 4200cm spacing leaves only one audible region.
   sound.set_editor_property('virtualization_mode',unreal.VirtualizationMode.PLAY_WHEN_SILENT)
  assigned=sound.get_editor_property('attenuation_settings')
  REPORT['audio_settings'][source.stem]={'voice_cap':cap,'limit_to_owner':False,'priority':recipe['priority'],'looping':bool(sound.get_editor_property('looping')),'attenuation':assigned.get_path_name() if assigned else None}
  LIB.save_loaded_asset(sound);REPORT['audio'].append(source.stem)
 LIB.save_directory(DEST,only_if_is_dirty=True,recursive=True)
 REPORT['complete']=True
 unreal.log('REVERIE_CONTENT_READY '+json.dumps({'scope':REPORT['scope'],'meshes':len(REPORT['imported'])+len(REPORT.get('sculpted_landmarks',{})),'sounds':len(REPORT['audio'])}))

if __name__=='__main__':
 output=Path(unreal.Paths.project_saved_dir()).resolve()/'Reverie';output.mkdir(parents=True,exist_ok=True)
 try:main()
 except Exception as error:REPORT['error']=str(error);REPORT['traceback']=traceback.format_exc();raise
 finally:(output/('surfaces-import.json' if SURFACES_ONLY else 'import.json')).write_text(json.dumps(REPORT,indent=2)+'\n',encoding='utf-8')

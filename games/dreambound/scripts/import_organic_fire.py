"""Import original fire volume materials and the separately authored organic audio."""
import hashlib
import json
from pathlib import Path
import traceback
import unreal

ROOT=Path(__file__).resolve().parents[1]
ART=ROOT/'art/organic-fire'
DEST='/Game/Art/OrganicFire/Materials'
AUDIO='/Game/Audio/OrganicFire'
LIB=unreal.EditorAssetLibrary
EDIT=unreal.MaterialEditingLibrary
TOOLS=unreal.AssetToolsHelpers.get_asset_tools()
REPORT={'complete':False,'materials':[],'audio':[]}

def node(mat,kind,x=0,y=0,**values):
    n=EDIT.create_material_expression(mat,kind,x,y)
    for k,v in values.items():n.set_editor_property(k,v)
    return n

def link(a,pin,b,target):
    if not EDIT.connect_material_expressions(a,pin,b,target):raise RuntimeError('Material connection failed: '+target)

def output(a,pin,prop):
    if not EDIT.connect_material_property(a,pin,prop):raise RuntimeError('Material output connection failed')

def custom(mat,code,inputs,kind=unreal.CustomMaterialOutputType.CMOT_FLOAT3):
    n=node(mat,unreal.MaterialExpressionCustom,400,0,code=code,output_type=kind)
    pins=[]
    for name in inputs:
        pin=unreal.CustomInput();pin.set_editor_property('input_name',name);pins.append(pin)
    n.set_editor_property('inputs',pins)
    for name,(source,pin) in inputs.items():link(source,pin,n,name)
    return n

def vector(mat,name,value,y):
    return node(mat,unreal.MaterialExpressionVectorParameter,-600,y,parameter_name=name,default_value=unreal.LinearColor(*value,0))

def scalar(mat,name,value,y):
    return node(mat,unreal.MaterialExpressionScalarParameter,-300,y,parameter_name=name,default_value=value)

def import_material():
    LIB.make_directory(DEST)
    name='M_OrganicFireVolume'; path=DEST+'/'+name
    mat=LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(name,DEST,unreal.Material,unreal.MaterialFactoryNew())
    EDIT.delete_all_material_expressions(mat)
    mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided',False)
    world=node(mat,unreal.MaterialExpressionWorldPosition,-900,-500)
    camera=node(mat,unreal.MaterialExpressionCameraPositionWS,-900,-350)
    inputs={'World':(world,''),'Camera':(camera,'')}
    for i,(key,value) in enumerate({'Center':(0,0,0),'Forward':(1,0,0),'Right':(0,1,0),'Up':(0,0,1),'HalfSize':(30,30,30)}.items()):
        inputs[key]=(vector(mat,key,value,i*150),'RGB')
    for i,(key,value) in enumerate({'Time':0,'Intensity':0,'Mode':0,'Seed':0,'Glow':1}.items()):
        inputs[key]=(scalar(mat,key,value,800+i*150),'')
    code=(ART/'noise.hlsl').read_text()+'\n'+(ART/'fire_volume.hlsl').read_text()
    ray=custom(mat,code,inputs,unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    rgb=node(mat,unreal.MaterialExpressionComponentMask,700,-100,r=True,g=True,b=True,a=False);link(ray,'',rgb,'')
    alpha=node(mat,unreal.MaterialExpressionComponentMask,700,100,r=False,g=False,b=False,a=True);link(ray,'',alpha,'')
    output(rgb,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR);output(alpha,'',unreal.MaterialProperty.MP_OPACITY)
    EDIT.recompile_material(mat);LIB.save_loaded_asset(mat,only_if_is_dirty=False)
    REPORT['materials'].append({'path':path,'shader_sha256':hashlib.sha256(code.encode()).hexdigest(),'ray_steps_max':34})

def add_furnace_to_skin(mat,existing_glow):
    """Called by the creature importer after building MireSeer's original skin."""
    world=node(mat,unreal.MaterialExpressionWorldPosition,-950,1100)
    inputs={'World':(world,'')}
    for i,(key,value) in enumerate({'HeatCenter':(0,0,0),'HeatForward':(1,0,0),'HeatRight':(0,1,0),'HeatUp':(0,0,1)}.items()):
        inputs[key]=(vector(mat,key,value,1250+i*150),'RGB')
    inputs['Heat']=(scalar(mat,'InternalHeat',0,1850),'')
    inputs['Time']=(scalar(mat,'FireTime',0,2000),'')
    code=(ART/'noise.hlsl').read_text()+'\n'+(ART/'furnace_surface.hlsl').read_text()
    heat=custom(mat,code,inputs)
    combined=node(mat,unreal.MaterialExpressionAdd,750,500)
    link(existing_glow,'',combined,'A');link(heat,'',combined,'B')
    output(combined,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)

def import_audio():
    folder=ROOT/'assets/audio-organic-fire'; metadata=folder/'audio-report.json'
    if not metadata.is_file():
        REPORT['audio_pending']='Audio author has not delivered audio-report.json yet.'
        return
    info=json.loads(metadata.read_text(encoding='utf-8-sig'))
    if 'rejected' in str(info.get('acceptance','')).lower():
        REPORT['audio_pending']='The authored draft was rejected by the owner; it is not imported.'
        return
    LIB.make_directory(AUDIO)
    attenuation_path=AUDIO+'/A_OrganicFire'
    attenuation=LIB.load_asset(attenuation_path) if LIB.does_asset_exist(attenuation_path) else TOOLS.create_asset('A_OrganicFire',AUDIO,unreal.SoundAttenuation,unreal.SoundAttenuationFactory())
    settings=attenuation.get_editor_property('attenuation')
    settings.set_editor_property('spatialize',True);settings.set_editor_property('attenuate',True)
    settings.set_editor_property('attenuation_shape_extents',unreal.Vector(260,0,0));settings.set_editor_property('falloff_distance',2100)
    attenuation.set_editor_property('attenuation',settings);LIB.save_loaded_asset(attenuation,only_if_is_dirty=False)
    for source in sorted(folder.glob('S_*.wav')):
        task=unreal.AssetImportTask();task.filename=str(source);task.destination_path=AUDIO;task.destination_name=source.stem
        task.automated=task.replace_existing=task.save=True;TOOLS.import_asset_tasks([task])
        sound=LIB.load_asset(AUDIO+'/'+source.stem)
        if not isinstance(sound,unreal.SoundWave):raise RuntimeError('Organic sound import failed: '+source.stem)
        looping=source.stem in ('S_CasterFurnaceLoop','S_CasterFlightLoop')
        sound.set_editor_property('looping',looping)
        sound.set_editor_property('attenuation_settings',attenuation)
        concurrency=unreal.SoundConcurrencySettings();concurrency.set_editor_property('max_count',4 if looping else 6)
        concurrency.set_editor_property('limit_to_owner',False)
        concurrency.set_editor_property('resolution_rule',unreal.MaxConcurrentResolutionRule.STOP_QUIETEST)
        sound.set_editor_property('override_concurrency',True);sound.set_editor_property('concurrency_overrides',concurrency)
        sound.set_editor_property('priority',72.0 if 'Release' in source.stem else 65.0)
        LIB.save_loaded_asset(sound,only_if_is_dirty=False)
        REPORT['audio'].append({'name':source.stem,'looping':looping,'sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'duration':float(sound.get_editor_property('duration'))})
    REPORT['audio_report_sha256']=hashlib.sha256(metadata.read_bytes()).hexdigest()

def main():
    import_material();import_audio();REPORT['complete']=True
    unreal.log('ORGANIC_FIRE_READY '+json.dumps(REPORT))

if __name__=='__main__':
    output_path=Path(unreal.Paths.project_saved_dir()).resolve()/'OrganicFire';output_path.mkdir(parents=True,exist_ok=True)
    try:main()
    except Exception:
        REPORT['error']=traceback.format_exc();raise
    finally:(output_path/'import.json').write_text(json.dumps(REPORT,indent=2)+'\n',encoding='utf-8')

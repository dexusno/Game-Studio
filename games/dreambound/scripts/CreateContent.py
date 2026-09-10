from pathlib import Path
import json
import runpy
import unreal
scripts=Path(__file__).resolve().parent
runpy.run_path(str(scripts/"import_reverie.py"),run_name="__main__")
runpy.run_path(str(scripts/"import_preferred_combat.py"),run_name="__main__")
runpy.run_path(str(scripts/"import_living_world.py"),run_name="__main__")
runpy.run_path(str(scripts/"import_organic_enemies.py"),run_name="__main__")
editor=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
asset="/Game/Maps/Reverie"
if unreal.EditorAssetLibrary.does_asset_exist(asset):
    editor.load_level(asset)
else:
    editor.new_level(asset)
editor.save_current_level()
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
placed=[]
for actor in actors:
    for component in actor.get_components_by_class(unreal.StaticMeshComponent):
        mesh=component.get_editor_property('static_mesh')
        if mesh:placed.append({'actor':actor.get_name(),'mesh':mesh.get_path_name()})
if placed:raise RuntimeError('The procedural Reverie entry map must not contain preplaced graphics: '+json.dumps(placed))
output=Path(unreal.Paths.project_saved_dir()).resolve()/'Reverie'
(output/'map.json').write_text(json.dumps({'map':asset,'preplaced_graphics':placed,'complete':True},indent=2)+'\n',encoding='utf-8')
unreal.log("DREAMBOUND_REVERIE_CONTENT_READY")

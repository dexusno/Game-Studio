from pathlib import Path
import runpy
import unreal
scripts=Path(__file__).resolve().parent
game=scripts.parent
runpy.run_path(str(scripts/"import_art.py"),run_name="__main__")
tasks=[]
for source in sorted((game/"assets"/"audio").glob("*.wav")):
    task=unreal.AssetImportTask()
    task.filename=str(source)
    task.destination_path="/Game/Audio"
    task.destination_name=source.stem
    task.automated=True
    task.replace_existing=True
    task.save=True
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
# The mechanical charge is the only continuous voice. Its component owns
# start/stop and pitch; importing it as a loop preserves that lifecycle.
charge=unreal.EditorAssetLibrary.load_asset('/Game/Audio/S_ChargeLoop')
if isinstance(charge,unreal.SoundWave):
    charge.set_editor_property('looping',True)
    unreal.EditorAssetLibrary.save_loaded_asset(charge)
editor=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
asset="/Game/Maps/Bellroot"
if unreal.EditorAssetLibrary.does_asset_exist(asset):
    editor.load_level(asset)
else:
    editor.new_level(asset)
editor.save_current_level()
unreal.EditorAssetLibrary.save_directory("/Game",only_if_is_dirty=False,recursive=True)
unreal.log("DREAMBOUND_CONTENT_READY")

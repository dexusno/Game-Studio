"""Import this game's original PCM into reproducible, ignored UE derivatives."""
import hashlib
import json
from pathlib import Path
import unreal as u

project = Path(u.Paths.project_dir()).resolve()
source = project.parent / "assets/audio/mechanical-v001"
audit = json.loads((source / "audio-audit.json").read_text(encoding="utf-8"))
destination = "/Game/FoundryAudio"
tools = u.AssetToolsHelpers.get_asset_tools()
editor = u.EditorAssetLibrary
editor.make_directory(destination)
tasks = []
for cue in audit["cues"]:
    path = source / cue["file"]
    if hashlib.sha256(path.read_bytes()).hexdigest() != cue["sha256"]:
        raise RuntimeError("PCM source changed after audio audit: " + cue["name"])
    task = u.AssetImportTask()
    task.filename = str(path)
    task.destination_path = destination
    task.destination_name = "S_" + cue["name"]
    task.automated = task.replace_existing = task.save = True
    tasks.append(task)
tools.import_asset_tasks(tasks)
report = {"version": audit["version"], "engine": u.SystemLibrary.get_engine_version(), "cues": []}
for cue, task in zip(audit["cues"], tasks):
    assets = task.get_objects()
    if len(assets) != 1 or not isinstance(assets[0], u.SoundWave):
        raise RuntimeError("Expected one imported SoundWave: " + cue["name"])
    sound = assets[0]
    sound.set_editor_property("looping", cue["loop"])
    sound.set_editor_property("volume", 1.0)
    sound.set_editor_property("compression_quality", 85)
    duration = sound.get_editor_property("duration")
    if abs(duration - cue["duration"]) > .002:
        raise RuntimeError("Duration changed during import: " + cue["name"])
    editor.save_loaded_asset(sound, only_if_is_dirty=False)
    report["cues"].append({"name": cue["name"], "asset": sound.get_path_name(), "duration": duration,
                           "looping": bool(sound.get_editor_property("looping")), "pcm_sha256": cue["sha256"]})
saved = project / "Saved/AudioImport"
saved.mkdir(parents=True, exist_ok=True)
(saved / "audio-import.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
u.log("FOUNDRY_AUDIO_IMPORTED " + str(len(report["cues"])))

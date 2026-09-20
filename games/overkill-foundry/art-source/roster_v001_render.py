"""Render saved original roster source without changing verified FBX exports."""
import argparse
import json
import sys
from pathlib import Path

import bpy

p = argparse.ArgumentParser()
p.add_argument("--output", required=True)
root = Path(p.parse_args(sys.argv[sys.argv.index("--") + 1:]).output).resolve()
report = json.loads((root / "asset-report.json").read_text(encoding="utf-8"))
bpy.ops.wm.open_mainfile(filepath=str(root / "Cinderwall_Roster_Source.blend"))
scene = bpy.context.scene
prefs = bpy.context.preferences.addons["cycles"].preferences
prefs.compute_device_type = "OPTIX"
prefs.get_devices()
for device in prefs.devices:
    device.use = device.type == "OPTIX"
scene.cycles.device = "GPU"
for name in report["robots"]:
    bpy.data.objects["SKM_" + name].hide_render = True
for name, info in report["robots"].items():
    mesh = bpy.data.objects["SKM_" + name]
    arm = bpy.data.objects["SK_" + name]
    arm.animation_data.action = bpy.data.actions[info["prefix"] + "idle"]
    mesh.hide_render = False
    scene.camera = bpy.data.objects[name + "_review"]
    scene.frame_set(1)
    scene.render.filepath = str(root / "renders" / (name + ".png"))
    bpy.ops.render.render(write_still=True)
    mesh.hide_render = True
print("ROSTER_SOURCE_RENDERS_READY " + str(root / "renders"))

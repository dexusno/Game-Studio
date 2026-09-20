"""CPU orthographic proof views of the frozen source; no asset changes."""
import argparse
from pathlib import Path
import sys
import bpy
from mathutils import Vector

p = argparse.ArgumentParser()
p.add_argument("--output", type=Path, required=True)
a = p.parse_args(sys.argv[sys.argv.index("--") + 1:])
out = a.output.resolve()
bpy.ops.wm.open_mainfile(filepath=str(out / "Mara_Surface.blend"))
scene = bpy.context.scene
scene.render.engine = "CYCLES"
scene.cycles.device = "CPU"
scene.cycles.samples = 16
scene.render.threads_mode = "FIXED"
scene.render.threads = 6
for name, location in (("surface_front", (4, 0, .93)), ("surface_side", (0, -4, .93)), ("surface_rear", (-4, 0, .93))):
    data = bpy.data.cameras.new(name + "_Proof")
    camera = bpy.data.objects.new(name + "_Proof", data)
    bpy.context.collection.objects.link(camera)
    camera.location = location
    camera.rotation_euler = (Vector((0, 0, .93)) - camera.location).to_track_quat("-Z", "Y").to_euler()
    data.type, data.ortho_scale = "ORTHO", 2.1
    scene.camera = camera
    scene.render.resolution_x, scene.render.resolution_y = 800, 1000
    scene.render.resolution_percentage = 100
    scene.render.filepath = str(out / "renders" / (name + ".png"))
    bpy.ops.render.render(write_still=True)

"""CPU source shading diagnostic; never a runtime quality approval."""
import argparse
from pathlib import Path
import sys
import bpy

p = argparse.ArgumentParser()
p.add_argument("--output", type=Path, required=True)
a = p.parse_args(sys.argv[sys.argv.index("--") + 1:])
a.output = a.output.resolve()
bpy.ops.wm.open_mainfile(filepath=str(a.output / "Mara_Surface.blend"))
s = bpy.context.scene
s.cycles.device = "CPU"
s.cycles.samples = 8
s.camera = bpy.data.objects["surface_front"]
low, high = bpy.data.objects["MaraOperator"], bpy.data.objects["Mara_SourceHigh"]
original = low.data.materials[0]
mat = bpy.data.materials.new("DiagnosticClay")
mat.use_nodes = True
mat.node_tree.nodes.get("Principled BSDF").inputs["Base Color"].default_value = (.5, .5, .5, 1)
mat.node_tree.nodes.get("Principled BSDF").inputs["Roughness"].default_value = .8
low.data.materials[0] = mat
s.render.filepath = str(a.output / "renders/diagnostic_clay_low.png")
bpy.ops.render.render(write_still=True)
low.hide_render = True
high.hide_render = False
high.data.materials[0] = mat
s.render.filepath = str(a.output / "renders/diagnostic_clay_high.png")
bpy.ops.render.render(write_still=True)
low.hide_render = False
high.hide_render = True
low.data.materials[0] = original
nodes, links = original.node_tree.nodes, original.node_tree.links
emission = nodes.new("ShaderNodeEmission")
links.new(nodes.get("Mara_BaseColor").outputs["Color"], emission.inputs["Color"])
links.new(emission.outputs[0], nodes.get("Material Output").inputs["Surface"])
s.render.filepath = str(a.output / "renders/diagnostic_base_only.png")
bpy.ops.render.render(write_still=True)
attr = nodes.new("ShaderNodeAttribute")
attr.attribute_name = "Mara_Base"
links.new(attr.outputs["Color"], emission.inputs["Color"])
s.render.filepath = str(a.output / "renders/diagnostic_attribute_only.png")
bpy.ops.render.render(write_still=True)

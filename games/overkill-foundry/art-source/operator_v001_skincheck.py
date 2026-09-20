"""Locate extreme deformation edges for weighting/source-topology diagnosis."""
import argparse
import json
from pathlib import Path
import sys
import bpy
import numpy as np

p = argparse.ArgumentParser()
p.add_argument("--output", type=Path, required=True)
a = p.parse_args(sys.argv[sys.argv.index("--") + 1:])
out = a.output.resolve()
bpy.ops.wm.open_mainfile(filepath=str(out / "Mara_Operator_Source.blend"))
mesh = bpy.data.objects["MaraOperator"]
arm = bpy.data.objects["SK_MaraOperator"]
arm.animation_data.action = bpy.data.actions["MO_idle"]
bpy.context.scene.frame_set(1)
evaluated = mesh.evaluated_get(bpy.context.evaluated_depsgraph_get()).to_mesh()
v = np.array([item.co[:] for item in mesh.data.vertices])
w = np.array([item.co[:] for item in evaluated.vertices])
edges = np.array([item.vertices[:] for item in mesh.data.edges])
length = np.linalg.norm(v[edges[:, 1]] - v[edges[:, 0]], axis=1)
posed = np.linalg.norm(w[edges[:, 1]] - w[edges[:, 0]], axis=1)
ratio = posed / np.maximum(length, .0001)
output = {"rest_edge_percentiles_m": np.quantile(length, [.5, .9, .99, 1]).tolist(), "worst_edges": []}
for index in np.argsort(posed - length)[-20:][::-1]:
    endpoints = []
    for vert in edges[index]:
        endpoints.append({"index": int(vert), "rest": v[vert].tolist(), "pose": w[vert].tolist(),
            "weights": {mesh.vertex_groups[g.group].name: g.weight for g in mesh.data.vertices[vert].groups}})
    output["worst_edges"].append({"rest_length": float(length[index]), "posed_length": float(posed[index]), "endpoints": endpoints})
(out / "skin-diagnostic.json").write_text(json.dumps(output, indent=2))
print(json.dumps(output, indent=2))

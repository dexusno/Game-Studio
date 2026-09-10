"""Prepare new original TRELLIS Reverie landforms without altering their atlas.

Blender background CPU: --asset CrownTree|SculptedBank --source textured.glb
--output <owned-private-finished-dir>. Raw source, original maps and latents
remain untouched. Uses proven coordinate/collision helpers read-only.
"""
import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import sys
import bpy
import numpy as np
from mathutils import Matrix, Vector


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--asset",choices=("CrownTree","SculptedBank"),required=True)
    parser.add_argument("--source",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args(sys.argv[sys.argv.index("--")+1:])
    source=args.source.resolve();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    if not source.is_file() or source.suffix.lower()!=".glb":raise RuntimeError("Completed source textured.glb required")
    helpers_path=Path(__file__).with_name("prepare_trellis_kit.py")
    spec=importlib.util.spec_from_file_location("trellis_prep_helpers",helpers_path)
    helpers=importlib.util.module_from_spec(spec);spec.loader.exec_module(helpers)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.preferences.filepaths.save_version=0
    bpy.ops.import_scene.gltf(filepath=str(source))
    meshes=[obj for obj in bpy.context.scene.objects if obj.type=="MESH"]
    if not meshes:raise RuntimeError("No generated mesh")
    bpy.ops.object.select_all(action="DESELECT")
    for obj in meshes:
        world=obj.matrix_world.copy();obj.parent=None;obj.matrix_world=world;obj.select_set(True)
    bpy.context.view_layer.objects.active=meshes[0]
    bpy.ops.object.convert(target="MESH");bpy.ops.object.join();obj=bpy.context.object
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    obj.name="SM_RV_"+args.asset;obj.data.name=obj.name+"_Mesh"
    if len(obj.data.materials)!=1 or not obj.data.uv_layers:raise RuntimeError("Expected one original atlas and UVs")
    coords=helpers.mesh_coordinates(obj.data);lo,hi=coords.min(axis=0),coords.max(axis=0)
    axis,target=(2,11.0) if args.asset=="CrownTree" else(0,5.4)
    scale=float(target/(hi[axis]-lo[axis]))
    matrix=Matrix.Diagonal(Vector((scale,scale,scale,1)))@Matrix.Translation(Vector((-float((lo[0]+hi[0])/2),-float((lo[1]+hi[1])/2),-float(lo[2]))))
    obj.data.transform(matrix);obj.data.update();obj.data.calc_loop_triangles()
    uv0=obj.data.uv_layers[0];uv0.name="UVMap"
    for layer in list(obj.data.uv_layers)[1:]:obj.data.uv_layers.remove(layer)
    uv1=obj.data.uv_layers.new(name="RepairDonorUV")
    values=np.empty(len(obj.data.loops)*2,dtype=np.float32);uv0.data.foreach_get("uv",values)
    if not np.isfinite(values).all():raise RuntimeError("Nonfinite source UV")
    uv1.data.foreach_set("uv",values);obj.data.uv_layers.active_index=0
    for attr in list(obj.data.color_attributes):obj.data.color_attributes.remove(attr)
    color=obj.data.color_attributes.new(name="RepairBlend",type="FLOAT_COLOR",domain="CORNER")
    masks=np.zeros((len(obj.data.loops),4),dtype=np.float32);masks[:,3]=1
    color.data.foreach_set("color",masks.reshape(-1));obj.data.color_attributes.active_color=color
    material=obj.data.materials[0];material.name="M_RV_"+args.asset
    images=list({node.image for node in material.node_tree.nodes if node.type=="TEX_IMAGE" and node.image})
    base=next((im for im in images if im.colorspace_settings.name=="sRGB"),None)
    packed=next((im for im in images if im!=base),None)
    if len(images)!=2 or base is None or packed is None:raise RuntimeError("Expected original base and packed metal/roughness maps")
    textures=[]
    for im,filename in((base,"base_color.png"),(packed,"metallic_roughness.png")):
        if not im.packed_file:im.pack()
        raw=bytes(im.packed_file.data);(out/filename).write_bytes(raw);im.filepath="//"+filename
        textures.append({"file":filename,"size":list(im.size),"sha256":hashlib.sha256(raw).hexdigest(),"source":"Unchanged embedded source GLB PNG; no re-encoding or texture bake"})
    coords=helpers.mesh_coordinates(obj.data);lo,hi=coords.min(axis=0),coords.max(axis=0)
    hulls=[];collision=[]
    h=float(hi[2])
    bands=[(0,.75),(.65,2.0),(1.9,3.2)] if args.asset=="CrownTree" else[(0,h*.42),(h*.36,h*.75),(h*.68,h)]
    for low,high in bands:
        points=coords[(coords[:,2]>=low)&(coords[:,2]<=high)]
        if not len(points):continue
        lower=points.min(axis=0);upper=points.max(axis=0)
        minimum=(float(lower[0]),float(lower[1]),low);maximum=(float(upper[0]),float(upper[1]),min(high,h))
        hull=helpers.collision_box(f"UCX_{obj.name}_{len(hulls):02d}",minimum,maximum)
        if hull:
            hulls.append(hull);collision.append({"lower_metres":minimum,"upper_metres":maximum})
    if not hulls:raise RuntimeError("No structural collision envelopes")
    footprints={}
    for cut in(1,2,3.2):
        points=coords[coords[:,2]<=cut]
        if len(points):
            low,high=points.min(axis=0),points.max(axis=0)
            footprints[f"below_{cut:g}m"]={"min_xy":[float(v) for v in low[:2]],"max_xy":[float(v) for v in high[:2]],"width_depth":[float(v) for v in(high-low)[:2]]}
    scene=bpy.context.scene;scene.unit_settings.system="METRIC";scene.unit_settings.scale_length=1
    scene.render.threads_mode="FIXED";scene.render.threads=4
    bpy.ops.object.select_all(action="DESELECT");obj.select_set(True)
    for hull in hulls:hull.select_set(True)
    bpy.context.view_layer.objects.active=obj
    fbx=out/(obj.name+".fbx")
    bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={"MESH"},axis_forward="-Y",axis_up="Z",
        apply_unit_scale=True,apply_scale_options="FBX_SCALE_UNITS",bake_anim=False,use_mesh_modifiers=True,mesh_smooth_type="FACE",add_leaf_bones=False,path_mode="STRIP",colors_type="LINEAR")
    for hull in hulls:hull.hide_render=True;hull.hide_set(True)
    report={"asset":args.asset,"source":str(source),"source_sha256":hashlib.sha256(source.read_bytes()).hexdigest(),"fbx":str(fbx),
        "dimensions_metres":[round(float(v),4) for v in hi-lo],"bounds_min_metres":[float(v) for v in lo],"bounds_max_metres":[float(v) for v in hi],
        "footprints":footprints,"pivot":"XY bounds center; ground minimumZ0; uniform metre normalization",
        "normalization_scale_xyz":[scale]*3,"front_axes":"Blender-Y to Unreal+Y; approved -Y/Z FBX conversion",
        "triangles":len(obj.data.loop_triangles),"collision_hulls":len(hulls),"collision_envelopes":collision,
        "collision":"Tree limited to lower3.2m base/trunk, canopy omitted; bank uses three stepped volume envelopes",
        "material":{"slots":1,"UV0":"Original atlas","UV1":"Exact UV0 duplicate","VertexColor.R":"RepairBlend linear zero everywhere",
                    "base_color":"sRGB RGB","packed_MR":"linear; roughnessG, metallicB","textures":textures},
        "cleanup":"Uniform scale, grounded pivot, original atlas preservation, base collision only; no geometry repair/remesh",
        "rights":"Original OpenAI image_gen reference; local TRELLIS.2 generation; retained model/runtime terms; local evaluation",
        "verification":"Actual Blender import/export, finite atlas UVs, uniform dimensions and preserved embedded PNG bytes; rendered source inspection follows"}
    (out/"prep-report.json").write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    # Honest source inspection. Renderer uses CPU while the next TRELLIS job
    # can use the GPU. This does not establish engine quality or collision play.
    floor_mat=bpy.data.materials.new("Source inspection floor");floor_mat.diffuse_color=(.16,.18,.16,1)
    bpy.ops.mesh.primitive_plane_add(size=max(h,float(hi[0]-lo[0]))*5,location=(0,0,-.015));floor=bpy.context.object;floor.data.materials.append(floor_mat)
    world=bpy.data.worlds.new("Source inspection sky");scene.world=world;world.use_nodes=True
    world.node_tree.nodes["Background"].inputs["Color"].default_value=(.23,.28,.33,1);world.node_tree.nodes["Background"].inputs["Strength"].default_value=.60
    data=bpy.data.lights.new("Gentle source key","SUN");data.energy=2.2;data.angle=.15;data.color=(1,.93,.82)
    light=bpy.data.objects.new("Gentle source key",data);scene.collection.objects.link(light);light.rotation_euler=(math.radians(25),math.radians(-25),math.radians(-35))
    camera_data=bpy.data.cameras.new("Generated source inspection");camera=bpy.data.objects.new("Generated source inspection",camera_data);scene.collection.objects.link(camera);scene.camera=camera
    width=float(hi[0]-lo[0]);camera.location=(width*.90,-max(width,h)*1.8,h*.52)
    camera.rotation_euler=(Vector((0,0,h*.48))-camera.location).to_track_quat("-Z","Y").to_euler();camera_data.type="ORTHO";camera_data.ortho_scale=max(h*1.15,width*1.4);camera_data.clip_end=1000
    scene.render.engine="CYCLES";scene.cycles.device="CPU";scene.cycles.samples=20;scene.cycles.use_denoising=True
    scene.render.resolution_x=1280 if args.asset=="CrownTree" else 1500
    scene.render.resolution_y=1400 if args.asset=="CrownTree" else 1100
    scene.render.resolution_percentage=100;scene.render.image_settings.file_format="PNG";scene.view_settings.view_transform="AgX";scene.view_settings.exposure=.25
    scene.render.filepath=str(out/"source-inspection.png");bpy.ops.render.render(write_still=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(out/(obj.name+".blend")))
    report["source_render"]="source-inspection.png"
    (out/"prep-report.json").write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    print("REVERIE_LANDFORM_READY "+json.dumps({k:report[k] for k in("asset","fbx","dimensions_metres","triangles","collision_hulls")}),flush=True)


if __name__=="__main__":main()

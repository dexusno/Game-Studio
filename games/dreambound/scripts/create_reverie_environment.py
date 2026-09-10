"""Author the original Reverie environment kit in background Blender.

Only art/reverie/environment is written. No prior model geometry or textures
are loaded. CraftUV: 1 UV unit per 100cm for stone/wood; leaves have blade UVs.
All structural collision is explicitly authored. CPU rendering, four threads.
"""
import bpy
import bmesh
import csv
import hashlib
import json
import math
import random
from pathlib import Path
from mathutils import Vector

GAME=Path(__file__).resolve().parents[1]
OUT=GAME/"art/reverie/environment"
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
scene=bpy.context.scene
scene.unit_settings.system="METRIC"
scene.unit_settings.scale_length=.01
scene.render.threads_mode="FIXED"
scene.render.threads=4
bpy.context.preferences.filepaths.save_version=0
uv_namespace={}
uv_source=GAME/"art/source/uv_tools.py"
exec(compile(uv_source.read_text(encoding="utf-8"),str(uv_source),"exec"),uv_namespace)
repair_uv=uv_namespace["repair_mesh_uv"]

# This is an original palette contract. Root supplies newly generated PBR
# maps to these slots; preview nodes are original and use no earlier textures.
PALETTE={
 "M_RV_Stone":((.34,.295,.22),.87,0),
 "M_RV_StoneLight":((.49,.423,.305),.83,0),
 "M_RV_StoneDark":((.205,.19,.155),.93,0),
 "M_RV_Mortar":((.105,.12,.085),.98,0),
 "M_RV_Moss":((.105,.17,.041),.94,0),
 "M_RV_Bark":((.19,.115,.060),.89,0),
 "M_RV_BarkLight":((.29,.195,.09),.87,0),
 "M_RV_Leaf":((.045,.145,.027),.81,0),
 "M_RV_LeafLight":((.155,.26,.050),.78,0),
 "M_RV_LeafGold":((.47,.36,.062),.76,0),
 "M_RV_Crystal":((.045,.36,.39),.24,.10),
 "M_RV_CrystalBase":((.08,.18,.16),.59,.07),
 "M_RV_Bronze":((.32,.17,.064),.43,.76),
 "M_RV_Patina":((.045,.19,.15),.70,.20),
}
MATS={}
for name,(color,rough,metal) in PALETTE.items():
    mat=bpy.data.materials.new(name)
    mat.diffuse_color=(*color,1)
    mat.use_nodes=True
    nodes,links=mat.node_tree.nodes,mat.node_tree.links
    bsdf=nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value=(*color,1)
    bsdf.inputs["Roughness"].default_value=rough
    bsdf.inputs["Metallic"].default_value=metal
    attr=nodes.new("ShaderNodeVertexColor");attr.layer_name="Color"
    sep=nodes.new("ShaderNodeSeparateColor");links.new(attr.outputs["Color"],sep.inputs["Color"])
    mix=nodes.new("ShaderNodeMixRGB");mix.blend_type="MULTIPLY";mix.inputs[0].default_value=1
    mix.inputs[1].default_value=(*color,1);links.new(sep.outputs["Red"],mix.inputs[2])
    worn=nodes.new("ShaderNodeMixRGB");links.new(sep.outputs["Green"],worn.inputs[0]);links.new(mix.outputs[0],worn.inputs[1])
    worn.inputs[2].default_value=(*(min(1,c*1.32) for c in color),1)
    links.new(worn.outputs[0],bsdf.inputs["Base Color"])
    if name=="M_RV_Crystal":
        bsdf.inputs["Emission Color"].default_value=(.03,.3,.32,1)
        bsdf.inputs["Emission Strength"].default_value=.22
    MATS[name]=mat

def active(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active=obj

def prism_data(outline,back,front,plane="XZ"):
    axes={"XZ":lambda a,b,d:(a,d,b),"XY":lambda a,b,d:(a,b,d),"YZ":lambda a,b,d:(d,a,b)}
    vertices=[axes[plane](a,b,d) for d in (back,front) for a,b in outline]
    n=len(outline)
    faces=[tuple(reversed(range(n))),tuple(n+i for i in range(n))]
    faces.extend((i,(i+1)%n,n+(i+1)%n,n+i) for i in range(n))
    return vertices,faces

def box_points(center,dims):
    x,y,z=center;w,d,h=dims
    return [(x+sx*w/2,y+sy*d/2,z+sz*h/2) for sx,sy,sz in
        [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]

BOX_FACES=[(3,2,1,0),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]
ASSETS={}

class Asset:
    def __init__(self,name,note):
        self.name="SM_RV_"+name
        self.note=note
        self.parts=[]
        self.hulls=[]

    def part(self,name,vertices,faces,mat="M_RV_Stone",bevel=0,smooth=False,seed=1,uv_override=None):
        mesh=bpy.data.meshes.new(name)
        mesh.from_pydata(vertices,[],faces);mesh.update()
        obj=bpy.data.objects.new(name,mesh);scene.collection.objects.link(obj)
        active(obj)
        bm=bmesh.new();bm.from_mesh(mesh)
        bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
        bm.to_mesh(mesh);bm.free()
        mesh.materials.append(MATS[mat])
        if bevel:
            mesh.materials.append(MATS[mat])
            mod=obj.modifiers.new("Chipped rounded stone arris","BEVEL")
            mod.width=bevel;mod.segments=2;mod.limit_method="ANGLE";mod.angle_limit=.48;mod.material=1
            bpy.ops.object.modifier_apply(modifier=mod.name)
            bm=bmesh.new();bm.from_mesh(mesh)
            bmesh.ops.dissolve_degenerate(bm,dist=.002,edges=list(bm.edges))
            bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(mesh);bm.free()
        uv=mesh.uv_layers.new(name="CraftUV")
        col=mesh.color_attributes.new(name="Color",type="FLOAT_COLOR",domain="CORNER")
        rng=random.Random(seed)
        value=rng.uniform(.83,.98)
        for poly in mesh.polygons:
            edge=poly.material_index==1
            poly.material_index=0;poly.use_smooth=smooth
            axis=max(range(3),key=lambda i:abs(poly.normal[i]))
            a,b=((1,2),(0,2),(0,1))[axis]
            for loop in poly.loop_indices:
                co=mesh.vertices[mesh.loops[loop].vertex_index].co
                uv.data[loop].uv=(co[a]/100,co[b]/100)
                col.data[loop].color=(min(1,value+(.025 if edge else 0)),.31 if edge else .008,.20 if not edge else .06,1)
        if bevel:
            mesh.materials.pop(index=1)
            mod=obj.modifiers.new("Broad sculpted face normals","WEIGHTED_NORMAL");mod.keep_sharp=True
            bpy.ops.object.modifier_apply(modifier=mod.name)
        self.parts.append(obj)
        return obj

    def hull(self,vertices,label="solid"):
        self.hulls.append((vertices,label))

    def box_hull(self,center,dims,label="solid"):
        self.hull(box_points(center,dims),label)

    def slab(self,name,outline,bottom,top,mat="M_RV_Stone",bevel=1.0,seed=1):
        return self.part(name,*prism_data(outline,bottom,top,"XY"),mat,bevel,seed=seed)

    def quarry(self,name,x1,x2,y1,y2,z,h,seed,mat=None,bevel=2.0):
        rng=random.Random(seed);clip=min((x2-x1)*.075,(y2-y1)*.10,7)
        outline=[(x1+clip,y1),(x2-clip*.8,y1),(x2,y1+clip),(x2,y2-clip),
            (x2-clip,y2),(x1+clip*.8,y2),(x1,y2-clip),(x1,y1+clip)]
        vertices=[(x,y,z) for x,y in outline]
        # Selected leading corners have an actual broken arris. The damage is
        # cut inward and down, so it does not expand the module contract.
        damaged=(4 if seed%3 else 5) if seed%4==0 else -1
        for i,(x,y) in enumerate(outline):
            chip=rng.uniform(2.0,5.2) if i==damaged and h>20 else 0
            xx=x+rng.uniform(-.55,.55)
            yy=y+rng.uniform(-.6,.6)
            if chip:
                xx+=(1 if i==5 else -1)*chip*.42
                yy-=chip*.6
            vertices.append((xx,yy,z+h+rng.uniform(-.7,.7)-chip))
        faces=[tuple(reversed(range(8))),tuple(8+i for i in range(8))]
        faces.extend((i,(i+1)%8,8+(i+1)%8,8+i) for i in range(8))
        return self.part(name,vertices,faces,mat or ("M_RV_StoneLight" if seed%7==0 else "M_RV_Stone"),
            min(bevel,(x2-x1)*.055,(y2-y1)*.055,h*.10),seed=seed)

    def course(self,name,x1,x2,y1,y2,z,h,seed,nominal=85,mat=None):
        rng=random.Random(seed);count=max(1,round((x2-x1)/nominal))
        weights=[rng.uniform(.75,1.2) for _ in range(count)]
        scale=(x2-x1)/sum(weights);x=x1
        for i,w in enumerate(weights):
            right=x+w*scale
            self.quarry(name,x+.5,right-.5,y1,y2,z,h,seed*13+i,mat)
            x=right

    def sweep(self,name,points,radii,mat="M_RV_Bark",sides=10,seed=1,flatten=1,fluting=0):
        points=[Vector(p) for p in points]
        if any(word in name.lower() for word in("trunk","canopy branch","branch fork","buttress root","bark ridge")):
            # Smooth authored branch arcs, preserving every original endpoint.
            # Catmull-Rom subdivision adds rounded elbows and continuous taper.
            dense=[];dense_r=[]
            for j in range(len(points)-1):
                p0=points[max(0,j-1)];p1=points[j];p2=points[j+1];p3=points[min(len(points)-1,j+2)]
                for step in range(4):
                    t=step/4
                    p=.5*((2*p1)+(-p0+p2)*t+(2*p0-5*p1+4*p2-p3)*t*t+(-p0+3*p1-3*p2+p3)*t*t*t)
                    dense.append(p);dense_r.append(radii[j]*(1-t)+radii[j+1]*t)
            dense.append(points[-1]);dense_r.append(radii[-1]);points=dense;radii=dense_r
            sides=max(sides,20 if "trunk" in name.lower() else 14)
        vertices=[]
        for j,p in enumerate(points):
            tangent=(points[min(j+1,len(points)-1)]-points[max(0,j-1)]).normalized()
            guide=Vector((0,0,1)) if abs(tangent.z)<.9 else Vector((0,1,0))
            u=tangent.cross(guide).normalized();v=tangent.cross(u).normalized()
            for i in range(sides):
                a=math.tau*i/sides
                radial=radii[j]*(1+.04*math.sin(i*2.6+seed)+fluting*math.cos(a*7+j*.12))
                vertices.append(tuple(p+radial*(u*math.cos(a)+v*math.sin(a)*flatten)))
        faces=[tuple(reversed(range(sides))),tuple((len(points)-1)*sides+i for i in range(sides))]
        for j in range(len(points)-1):
            for i in range(sides):
                faces.append((j*sides+i,j*sides+(i+1)%sides,(j+1)*sides+(i+1)%sides,(j+1)*sides+i))
        return self.part(name,vertices,faces,mat,0,True,seed)

    def finish(self,limits=None):
        active(self.parts[0])
        for obj in self.parts:obj.select_set(True)
        bpy.ops.object.join();obj=bpy.context.object
        obj.name=obj.data.name=self.name
        scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
        bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
        source_names=[mat.name for mat in obj.data.materials]
        face_material=[source_names[p.material_index] for p in obj.data.polygons]
        slots=[name for name in PALETTE if name in set(face_material)]
        obj.data.materials.clear()
        for name in slots:obj.data.materials.append(MATS[name])
        for p,name in zip(obj.data.polygons,face_material):p.material_index=slots.index(name)
        repair_count=repair_uv(obj.data)
        light=obj.data.uv_layers.new(name="LightmapUV");obj.data.uv_layers.active=light
        active(obj);bpy.ops.object.mode_set(mode="EDIT");bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.uv.smart_project(angle_limit=1.15,island_margin=.008)
        bpy.ops.object.mode_set(mode="OBJECT");obj.data.uv_layers.active_index=0;obj.data.uv_layers[0].active_render=True
        obj.data.calc_loop_triangles()
        bounds=[Vector(p) for p in obj.bound_box]
        lo=[min(p[i] for p in bounds) for i in range(3)];hi=[max(p[i] for p in bounds) for i in range(3)]
        if limits:
            for axis,minimum,maximum in limits:
                if lo[axis]<minimum-.05 or hi[axis]>maximum+.05:
                    raise RuntimeError(self.name+" authored bounds mismatch "+str((axis,lo[axis],hi[axis],minimum,maximum)))
        collision=[];collision_meta=[]
        for i,(points,label) in enumerate(self.hulls):
            bm=bmesh.new()
            for v in points:bm.verts.new(v)
            result=bmesh.ops.convex_hull(bm,input=list(bm.verts),use_existing_faces=False)
            unused=[v for v in set(result["geom_interior"]+result["geom_unused"]) if isinstance(v,bmesh.types.BMVert) and v.is_valid]
            if unused:bmesh.ops.delete(bm,geom=unused,context="VERTS")
            bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
            data=bpy.data.meshes.new("Collision");bm.to_mesh(data);bm.free()
            hull=bpy.data.objects.new(f"UCX_{self.name}_{i:02d}",data);scene.collection.objects.link(hull);collision.append(hull)
            collision_meta.append({"label":label,"bounds_min_cm":[min(v[a] for v in points) for a in range(3)],
                "bounds_max_cm":[max(v[a] for v in points) for a in range(3)]})
        active(obj)
        for hull in collision:hull.select_set(True)
        path=OUT/(self.name+".fbx")
        bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={"MESH"},
            apply_unit_scale=True,apply_scale_options="FBX_SCALE_UNITS",axis_forward="X",axis_up="Z",
            use_space_transform=True,bake_space_transform=True,use_mesh_modifiers=True,
            mesh_smooth_type="FACE",add_leaf_bones=False,bake_anim=False,use_custom_props=False,path_mode="STRIP")
        for hull in collision:bpy.data.objects.remove(hull,do_unlink=True)
        meta={"name":self.name,"dimensions_cm":[round(hi[i]-lo[i],3) for i in range(3)],
            "bounds_min_cm":lo,"bounds_max_cm":hi,"vertices":len(obj.data.vertices),"triangles":len(obj.data.loop_triangles),
            "materials":slots,"collision_hulls":len(self.hulls),"collision":collision_meta,
            "uv_channels":[layer.name for layer in obj.data.uv_layers],"uv0_scale_cm":100,
            "vertex_channels":{"R":"broad painted value","G":"actual exposed bevel wear","B":"recess/weathering","A":"opaque"},
            "uv_faces_repaired":repair_count,"pivot":"Authoring origin; front +Y, width X, centimeters. See placement note.",
            "placement":self.note,"sha256":hashlib.sha256(path.read_bytes()).hexdigest()}
        ASSETS[self.name]={"object":obj,"meta":meta}
        # Preserve usable partial output after each finished asset.
        write_records()
        print("RV_ASSET_READY "+self.name+" "+json.dumps({k:meta[k] for k in ("dimensions_cm","triangles","collision_hulls")}),flush=True)
        obj.hide_render=True
        return obj

def write_records():
    metadata={name:entry["meta"] for name,entry in ASSETS.items()}
    (OUT/"materials.json").write_text(json.dumps({name:{"color":list(spec[0]),"roughness":spec[1],"metallic":spec[2]}
        for name,spec in PALETTE.items()},indent=2)+"\n",encoding="utf-8")
    (OUT/"asset-metadata.json").write_text(json.dumps(metadata,indent=2)+"\n",encoding="utf-8")
    (OUT/"integration-manifest.json").write_text(json.dumps({"source":"scripts/create_reverie_environment.py",
        "destination_meshes":"/Game/Art/Reverie/Meshes","destination_materials":"/Game/Art/Reverie/Materials",
        "units":"centimeters","front":"+Y","wall_plane":"local X","traversal":"local Y",
        "materials":{name:{"color":list(spec[0]),"roughness":spec[1],"metallic":spec[2],"vertex_masks":True,
            "uv0_scale_cm":100,"textures":"Root creates a wholly new PBR family; none of the old textures are referenced"} for name,spec in PALETTE.items()},
        "assets":metadata},indent=2)+"\n",encoding="utf-8")

def angular_outline(cx,cy,w,d,seed,count=12):
    rng=random.Random(seed)
    return [(cx+math.cos(math.tau*i/count)*w*.5*rng.uniform(.90,1),
             cy+math.sin(math.tau*i/count)*d*.5*rng.uniform(.9,1)) for i in range(count)]

def moss(asset,center,width,depth,seed):
    x,y,z=center
    asset.slab("Attached moss shelf",angular_outline(x,y,width,depth,seed,14),z,z+1.8,"M_RV_Moss",.3,seed)

def relief_leaf(asset,root,direction,length,width,front,seed,mat="M_RV_StoneLight",sign=1):
    """Closed carved acanthus leaf with a raised central rib and cupped edge."""
    x,z=root;dx,dz=direction;n=math.sqrt(dx*dx+dz*dz);dx/=n;dz/=n
    outline=[(0,0),(.15,-.24),(.34,-.45),(.55,-.50),(.79,-.31),(1,0),
             (.79,.31),(.55,.50),(.34,.45),(.15,.24)]
    vertices=[]
    for depth in(-3.5,-1.5):
        for t,s in outline:
            vertices.append((x+dx*length*t-dz*width*s,sign*(front+depth),z+dz*length*t+dx*width*s))
    count=len(outline)
    vertices.append((x+dx*length*.52,sign*front,z+dz*length*.52))
    faces=[tuple(reversed(range(count)))]
    for i in range(count):
        faces.append((count*2,count+i,count+(i+1)%count))
        faces.append((i,(i+1)%count,count+(i+1)%count,count+i))
    asset.part("Sculpted botanical leaf relief",vertices,faces,mat,.26,False,seed)

def botanical_cartouche(asset,x,z,width,height,front,seed,sign=1,bronze=False):
    """A legible framed plant motif integrated into a recessed masonry face."""
    outline=[(x-width*.5,z+height*.13),(x-width*.40,z+6),(x,z),
             (x+width*.40,z+6),(x+width*.5,z+height*.13),
             (x+width*.5,z+height*.83),(x+width*.27,z+height*.96),
             (x,z+height),(x-width*.27,z+height*.96),(x-width*.5,z+height*.83)]
    back=sign*(front-7);edge=sign*(front-4)
    asset.part("Recessed carved botanical field",*prism_data(outline,min(back,edge),max(back,edge)),"M_RV_StoneDark",.7,seed=seed)
    # Thin modeled edge is inside the contractual foremost surface.
    border=[(xx,sign*(front-2.2),zz) for xx,zz in outline]+[(outline[0][0],sign*(front-2.2),outline[0][1])]
    asset.sweep("Chiselled cartouche border",border,[1.25]*len(border),"M_RV_StoneLight",8,seed+1)
    stem=[(x+math.sin(i*.77)*width*.035,sign*(front-1.9),z+height*(.10+i*.095)) for i in range(9)]
    asset.sweep("Carved curling botanical stem",stem,[1.55]*len(stem),"M_RV_Bronze" if bronze else"M_RV_StoneLight",8,seed+2)
    for i in range(6):
        centerz=z+height*(.19+i*.10)
        for side in(-1,1):
            mat="M_RV_Bronze" if bronze and i%3==0 else"M_RV_StoneLight"
            relief_leaf(asset,(x+side*1.4,centerz),(side*.72,.64),width*.43,width*.185,front-.25,
                        seed+10+i*2+side,mat,sign)
    # A small recessed verdigris bud gives the bronze an aged material edge.
    relief_leaf(asset,(x,z+height*.78),(0,1),height*.115,width*.26,front-.7,seed+39,"M_RV_Patina",sign)

def rosette(asset,x,z,radius,front,seed,sign=1):
    outline=[(x+math.cos(i*math.tau/12)*radius,z+math.sin(i*math.tau/12)*radius) for i in range(12)]
    asset.part("Inset aged bronze seal",*prism_data(outline,min(sign*(front-5),sign*(front-2)),max(sign*(front-5),sign*(front-2))),"M_RV_Patina",.7,seed=seed)
    for i in range(8):
        angle=i*math.tau/8
        relief_leaf(asset,(x,z),(math.cos(angle),math.sin(angle)),radius*.89,radius*.45,front,seed+10+i,"M_RV_Bronze",sign)

def rock(asset,center,dims,seed,collision=False,mat="M_RV_StoneDark"):
    x,y,z=center;w,d,h=dims;rng=random.Random(seed);sides=10
    outline=[(math.cos(math.tau*i/sides)*rng.uniform(.86,1),math.sin(math.tau*i/sides)*rng.uniform(.87,1)) for i in range(sides)]
    profile=[(0,.83,0,0),(.14,1,.025,-.015),(.38,.91,-.04,.025),(.62,.78,.05,.015),(.84,.64,.01,-.015),(1,.37,.05,.0)]
    vertices=[]
    for level,scale,sx,sy in profile:
        for i,(a,b) in enumerate(outline):
            vertices.append((x+w*(a*scale*.5+sx),y+d*(b*scale*.5+sy),z+h*level+(rng.uniform(-.013,.013)*h if level else 0)))
    faces=[tuple(reversed(range(sides))),tuple((len(profile)-1)*sides+i for i in range(sides))]
    for row in range(len(profile)-1):
        for i in range(sides):faces.append((row*sides+i,row*sides+(i+1)%sides,(row+1)*sides+(i+1)%sides,(row+1)*sides+i))
    obj=asset.part("Sculpted split strata",vertices,faces,mat,min(5,h*.02),False,seed)
    if collision:asset.hull(vertices,"tight rock mass")
    for j in (1,3):moss(asset,(x+w*(.04 if j==1 else-.08),y+d*.10,z+h*(.18 if j==1 else .64)),w*(.53 if j==1 else .29),d*.36,seed+j)
    return obj

class Leaves:
    def __init__(self):self.data={name:[[],[]] for name in ("M_RV_Leaf","M_RV_LeafLight","M_RV_LeafGold")}
    def add(self,root,direction,length,width,mat="M_RV_Leaf",curl=.18,twist=0):
        root=Vector(root);forward=Vector(direction).normalized()
        guide=Vector((0,0,1)) if abs(forward.z)<.95 else Vector((0,1,0))
        side=forward.cross(guide).normalized();normal=side.cross(forward).normalized()
        side2=side*math.cos(twist)+normal*math.sin(twist);normal2=side2.cross(forward).normalized()
        # Two ridged, closed skins; an authored gently curled pointed blade.
        outline=[(0,0),(.12,-.18),(.28,-.39),(.50,-.50),(.76,-.36),(1,0),
                 (.76,.36),(.50,.50),(.28,.39),(.12,.18)]
        count=len(outline)
        vertices=[]
        for layer in (-1,1):
            for t,cross in outline:
                vertices.append(tuple(root+forward*(t*length)+side2*(cross*width)+normal2*(math.sin(t*math.pi)*length*curl+layer*.23)))
            vertices.append(tuple(root+forward*(length*.52)+normal2*(length*(curl+.065)+layer*.23)))
        faces=[]
        for i in range(count):
            faces.append((count,i,(i+1)%count))
            faces.append((count*2+1,count+1+(i+1)%count,count+1+i))
            faces.append((i,count+1+i,count+1+(i+1)%count,(i+1)%count))
        verts,polys=self.data[mat];offset=len(verts);verts.extend(vertices);polys.extend(tuple(offset+i for i in f) for f in faces)
    def emit(self,asset):
        for i,(mat,(vertices,faces)) in enumerate(self.data.items()):
            if vertices:asset.part("Curled individually modeled foliage",vertices,faces,mat,0,False,6000+i)

def fern(asset,center,reach,seed):
    rng=random.Random(seed);c=Vector(center);leaves=Leaves()
    for frond in range(10):
        a=frond*math.tau/10+rng.uniform(-.10,.10);forward=Vector((math.cos(a),math.sin(a),0));side=Vector((-forward.y,forward.x,.06))
        extent=reach*rng.uniform(.75,1.06)
        points=[c+forward*(extent*t)+Vector((0,0,extent*(.04+.58*math.sin(t*math.pi*.86)))) for t in (0,.2,.4,.6,.8,1)]
        asset.sweep("Curved fern rachis",points,[1.1,1,.8,.6,.34,.10],"M_RV_BarkLight",6,seed+frond)
        for j in range(1,12):
            t=j/12;p=c+forward*(extent*t)+Vector((0,0,extent*(.04+.58*math.sin(t*math.pi*.86))))
            for sign in (-1,1):
                l=extent*.24*(1-t)+5
                leaves.add(p,(side*sign+forward*.3).normalized(),l,l*.30,"M_RV_LeafLight" if j%4==0 else"M_RV_Leaf",.12)
    leaves.emit(asset)


# 1-2. Human-scale worked flagstones; authored flat collision is independent
# of shallow chips, inset seams and tiny differences in the visible surface.
for variant in range(2):
    a=Asset("Tile" if not variant else"TileB","200x200 footprint. BaseZ0/topZ20. Place atZ-20 for walk surfaceZ0. Single flat UCX.")
    a.slab("Recessed continuous bedding",[(-100,-100),(100,-100),(100,100),(-100,100)],0,15,"M_RV_Mortar",.3)
    rng=random.Random(101+variant)
    rows=[-100,-51,-1,48,100] if not variant else[-100,-59,-15,42,100]
    for row in range(4):
        y1,y2=rows[row],rows[row+1]
        splits=[-100,-45,14,66,100] if (row+variant)%2 else[-100,-66,-12,48,100]
        for col in range(4):
            x1,x2=splits[col],splits[col+1];gap=.9;clip=rng.uniform(2.2,6.0)
            outline=[(x1+gap+clip,y1+gap),(x2-gap-2,y1+gap),(x2-gap,y1+gap+clip),
                (x2-gap,y2-gap-clip),(x2-gap-clip,y2-gap),(x1+gap+2,y2-gap),
                (x1+gap,y2-gap-clip),(x1+gap,y1+gap+clip)]
            a.slab("Individually chipped flagstone",outline,10.5,20-rng.uniform(0,.9),
                "M_RV_StoneLight" if (row+col+variant)%6==0 else"M_RV_Stone",1.1,111+row*7+col+variant*30)
            if (row*4+col+variant)%9==0:
                moss(a,((x1+x2)*.5,y1+1.6,19),min(20,x2-x1-6),2.3,201+row*3+col)
    a.box_hull((0,0,10),(200,200,20),"continuous flat paving surface")
    a.finish([(0,-100,100),(1,-100,100),(2,0,22)])

# 3. Substantial bonded abbey wall, recessed construction and carved courses.
a=Asset("Wall","400W x100D x600H, front+Y. FootZ0. Solid UCX excludes decorative bevel protrusions.")
a.part("Inset structural mortar",box_points((0,0,300),(398,87,600)),BOX_FACES,"M_RV_Mortar",.5)
for row in range(12):
    a.course("Deeply coursed limestone",-200,200,-46,42 if 3<=row<=6 else 48,row*50+1,47,300+row,80 if row%2 else 108)
for z in (98,347,548):
    a.course("Worn horizontal stringcourse",-200,200,-47.5,50,z,14,400+z,101,"M_RV_StoneLight")
for x in (-154,83):
    a.part("Small hand-cut mason inset",*prism_data([(x-6,245),(x+6,245),(x+6,259),(x-6,259)],48.0,49.2),"M_RV_StoneDark",.4,seed=430+int(x))
for x in(-102,102):botanical_cartouche(a,x,157,94,182,50.1,455+x,bronze=True)
# Three inset leaves form a deliberate repeating frieze below the coping.
for x in range(-153,154,51):
    relief_leaf(a,(x-14,535),(1,.2),29,12,49.6,488+x,"M_RV_Bronze")
a.box_hull((0,0,300),(400,94,600),"solid wall bay")
a.finish([(0,-201,201),(1,-51,51),(2,0,601)])

# 4. Layered cornice/coping with curved chamfer profile and separated stones.
a=Asset("Coping","420W x132D x42H. BaseZ0, front+Y. Overhead trim; no collision.")
profile=[(-55,0),(45,0),(47,8),(55,14),(66,19),(66,22),(61,23),(61,30),
         (66,31),(66,34),(61,42),(-62,42),(-66,34),(-66,20),(-58,14)]
for i in range(4):
    a.part("Profiled drip and coping",*prism_data(profile,-210+i*105+.4,-210+(i+1)*105-.4,"YZ"),"M_RV_StoneLight",1.25,seed=501+i)
for index,x in enumerate(range(-189,190,27)):
    relief_leaf(a,(x-9,24),(1,.12),19,6.5,65.8,551+index,"M_RV_Bronze" if index%4 else"M_RV_Patina")
moss(a,(-124,9,42),102,49,509)
a.finish()

# 5. Tapered structural buttress with true stacked offsets and carved front.
a=Asset("Buttress","180W x180D,~720H. BaseZ0, front+Y. Three compact structural hulls; no broad canopy box.")
for row in range(10):
    width=180-row*7.5;depth=180-row*6.2;z=row*65
    a.quarry("Setback buttress load course",-width/2,width/2,-depth/2,depth/2,z,63,600+row,
        "M_RV_StoneLight" if row in(0,5,9) else"M_RV_Stone",3)
for z,w,d in ((194,181,175),(454,153,146),(647,142,136)):
    a.quarry("Projecting buttress belt",-w/2,w/2,-d/2,d/2,z,25,700+z,"M_RV_StoneLight",2.5)
rock(a,(0,-8,674),(125,119,49),720,False,"M_RV_StoneLight")
a.part("Recessed buttress face channel",box_points((0,65,387),(22,5,239)),BOX_FACES,"M_RV_StoneDark",.6)
for x in(-21,21):a.quarry("Fine channel cheek",x-5,x+5,61,78,266,244,751+x,"M_RV_StoneLight",.5)
botanical_cartouche(a,0,275,76,233,82.0,781,bronze=True)
rosette(a,0,558,22,69,789)
for x,y,z,w,d,h in((0,0,109,172,170,218),(0,-2,335,149,149,232),(0,-6,569,119,120,245)):
    a.box_hull((x,y,z),(w,d,h),"buttress load mass")
a.finish()

# 6. Genuine passage. The full rectangular corridor |X|<=220,Z<=500 stays
# empty. Pointed voussoirs close only above that clearance, not across it.
a=Asset("Arch","740W x160D x~800H. Wall planeX, traversalY. Guaranteed clear rectangle440W x500H from ground0. Real connected gateway only.")
for side in(-1,1):
    x1,x2=(220,368) if side==1 else(-368,-220)
    for row in range(10):
        a.quarry("Gate pier dressed course",x1,x2,-70,70,row*50+1,48,800+row+side*25,"M_RV_StoneLight" if row in(0,9) else"M_RV_Stone",2.5)
    a.box_hull(((x1+x2)/2,0,250),(x2-x1,140,500),"gateway side pier; outside440cm clearance")
    a.quarry("Gate pier foot",x1,x2,-80,80,0,28,901+side,"M_RV_StoneLight",2)
    a.quarry("Gate spring capital",x1,x2,-80,80,478,31,911+side,"M_RV_StoneLight",2)
for side in(-1,1):
    for index in range(12):
        t0=index/12+.0015;t1=(index+1)/12-.0015
        def curve(t,w,h):return(side*w*(1-t*t),500+h*t)
        outline=[curve(t0,220,220),curve(t1,220,220),curve(t1,315,300),curve(t0,315,300)]
        vertices,faces=prism_data(outline,-69,69)
        a.part("Massive pointed arch voussoir",vertices,faces,"M_RV_StoneLight" if index%3==0 else"M_RV_Stone",1.15,seed=1001+index+side*16)
        a.hull(vertices,"upper arch stone; minimumZ>=500")
        # Separate raised bead on both faces gives the gate visible depth.
        for front in(-1,1):
            thin=[curve(t0,224,226),curve(t1,224,226),curve(t1,239,243),curve(t0,239,243)]
            a.part("Carved gate archivolt bead",*prism_data(thin,front*70-4,front*70+4),"M_RV_StoneLight",.6,seed=1091+index)
rock(a,(-287,6,508),(122,136,169),1131,False,"M_RV_Stone")
rock(a,(289,-4,510),(120,130,153),1132,False,"M_RV_Stone")
for sign in(-1,1):
    for x in(-294,294):botanical_cartouche(a,x,88,94,320,76,1171+x+sign*12,sign,True)
    rosette(a,0,753,27,76,1191+sign,sign)
    # Carved leaves follow the outer archivolt, well above the walk opening.
    for side in(-1,1):
        for index,t in enumerate((.17,.34,.51,.68,.84)):
            x=side*273*(1-t*t);z=500+270*t
            relief_leaf(a,(x,z),(-side*.68,.74),32,15,75,1200+index+side*7,"M_RV_StoneLight",sign)
moss(a,(-297,40,507),102,31,1151);moss(a,(281,-33,505),87,28,1152)
a.finish()

# 7-8. Reusable original rock banks and distant broken limestone silhouettes.
a=Asset("RockBank","Approx650W x420D x400H. Ground0. Three tight convex stone hulls. Plant outside encounter lanes.")
rock(a,(-175,-15,0),(325,340,285),1201,True)
rock(a,(65,-34,0),(373,386,401),1202,True,"M_RV_Stone")
rock(a,(232,69,0),(212,253,221),1203,True)
a.sweep("Root joining bank strata",[(-192,118,229),(-153,151,171),(-88,163,99),(40,158,33),(182,143,6)],[11,12,10,7,2],"M_RV_Bark",9,1210)
a.finish()
a=Asset("Cliff","Distant layered cliff/~900W x550D x1250H. BaseZ0. Visual only, behind playable banks.")
rock(a,(-241,40,0),(489,495,1060),1301,False)
rock(a,(58,-53,0),(535,541,1243),1302,False,"M_RV_Stone")
rock(a,(330,80,0),(337,382,824),1303,False)
rock(a,(-55,141,0),(542,318,372),1304,False,"M_RV_StoneLight")
a.finish()

# 9-10. Mature branching tree and a hanging bough: genuinely new branch
# skeleton, flared buttressed roots, visible bark ridges and lush leaf groups.
def author_tree(asset,seed,bough_only=False):
    rng=random.Random(seed)
    leaves=Leaves()
    if not bough_only:
        trunk=[(0,0,0),(-21,2,132),(14,-13,299),(-32,-4,465),(5,24,643),(-28,13,815),(25,18,1002)]
        asset.sweep("Twisted mature trunk",trunk,[93,71,64,57,43,30,6],"M_RV_Bark",28,seed,fluting=.105)
        for i in range(8):
            angle=i*math.tau/8+.17;direction=Vector((math.cos(angle),math.sin(angle),0));reach=rng.uniform(263,400)
            points=[direction*38+Vector((0,0,87)),direction*95+Vector((0,0,45)),direction*(reach*.55)+Vector((0,0,15)),direction*reach+Vector((0,0,3))]
            asset.sweep("Flared buttress root",points,[32,33,18,2.8],"M_RV_BarkLight" if i%3==0 else"M_RV_Bark",18,seed+i,fluting=.11)
            root_ridge=[v+Vector((0,0,r)) for v,r in zip(points,[27,27,13,0])]
            asset.sweep("Raised root ridge",root_ridge,[3.8,3.2,2.1,.35],"M_RV_BarkLight",8,seed+70+i)
            if i%2==0:asset.hull([tuple(v+Vector((sx*12,sy*12,sz*7))) for v in points[:3] for sx,sy,sz in((-1,-1,-1),(1,1,1),(1,-1,-1),(-1,1,1))],"low root close to trunk")
        asset.hull([(math.cos(i*math.tau/10)*75,math.sin(i*math.tau/10)*75,z) for z in(0,575) for i in range(10)],"tree trunk only")
        # Raised longitudinal ridges follow the twist rather than random noise.
        for i in range(11):
            angle=i*math.tau/11;ridge=[]
            for j,(x,y,z) in enumerate(trunk[:6]):
                r=(91,70,63,55,41,28)[j];a=angle+j*.16
                ridge.append((x+math.cos(a)*r,y+math.sin(a)*r,z+5))
            asset.sweep("Carved twisting bark ridge",ridge,[3.9,3.5,3,2.6,2.1,.6],"M_RV_BarkLight",6,seed+90+i)
    branch_count=7 if bough_only else 15
    for index in range(branch_count):
        angle=index*2.399+seed*.01
        start=Vector((0,0,0 if bough_only else 420+(index%5)*67))
        extent=rng.uniform(280,470) if not bough_only else rng.uniform(240,370)
        direction=Vector((math.cos(angle),math.sin(angle),0))
        crown=start+direction*extent+Vector((0,0,rng.uniform(180,325)))
        middle=start+direction*(extent*.46)+Vector((0,0,145))
        asset.sweep("Ascending canopy branch",[start,middle,crown],[31 if not bough_only else 18,18,3.2],"M_RV_Bark",10,seed+200+index)
        for fork in(-1,1):
            lateral=Vector((-direction.y,direction.x,0))*fork
            tip=crown+direction*rng.uniform(65,125)+lateral*rng.uniform(65,120)+Vector((0,0,rng.uniform(14,75)))
            asset.sweep("Fine branch fork",[middle,crown,tip],[11,6,.65],"M_RV_BarkLight",7,seed+260+index)
            center=tip+Vector((0,0,20))
            for leaf in range(49 if not bough_only else 30):
                theta=rng.uniform(0,math.tau);rr=math.sqrt(rng.random())*130
                p=center+Vector((math.cos(theta)*rr,math.sin(theta)*rr*.82,rng.uniform(-45,55)))
                aim=Vector((math.cos(theta),math.sin(theta),rng.uniform(-.25,.6)))
                length=rng.uniform(47,78);width=length*rng.uniform(.52,.70)
                mat="M_RV_LeafLight" if leaf%5==0 else"M_RV_LeafGold" if leaf%29==0 else"M_RV_Leaf"
                leaves.add(p,aim,length,width,mat,.15,rng.uniform(-.7,.7))
    leaves.emit(asset)

a=Asset("Tree","Mature tree footZ0, broad rooted base~800diam. Trunk/root hulls only; canopy noncolliding. Exact foliage extents in metadata.")
author_tree(a,1401);a.finish()
a=Asset("Bough","Overhead leafy bough attachment at origin; no collision. For framing above accessible routes, never ground-level obstruction.")
author_tree(a,1408,True);a.finish()

# 11-14. Foliage with meaningful silhouettes at game scale and compact batches.
a=Asset("Fern","Grounded fern group; ~240cm spread,~95H. No collision.")
fern(a,(0,0,0),108,1501);fern(a,(-41,25,0),57,1502);a.finish()
a=Asset("Grass","Low arched sedge drift; groundZ0; no collision.")
leaves=Leaves();rng=random.Random(1601)
for i in range(78):
    x=rng.uniform(-99,99);y=rng.uniform(-60,60);angle=rng.uniform(0,math.tau)
    length=rng.uniform(34,81)
    leaves.add((x,y,1),(math.cos(angle)*.52,math.sin(angle)*.52,1),length,rng.uniform(2.4,5.5),
        "M_RV_LeafGold" if i%17==0 else"M_RV_LeafLight" if i%4==0 else"M_RV_Leaf",-.21,rng.uniform(-.3,.3))
leaves.emit(a);a.finish()
a=Asset("Ivy","Wall vine/front+Y. Origin at upper attachment; trails down~330cm, width~220. No collision.")
leaves=Leaves();rng=random.Random(1701)
for vine in range(6):
    x=-88+vine*32;extent=rng.uniform(214,340)
    points=[(x+math.sin(j*.9+vine)*15,0,-extent*j/7) for j in range(8)]
    a.sweep("Hanging vine stem",points,[2.5,2.3,2,1.7,1.4,1.1,.65,.2],"M_RV_Bark",6,1701+vine)
    for j in range(14):
        t=j/14;p=Vector((x+math.sin(t*7*.9+vine)*15,1,-extent*t))
        sign=-1 if j%2 else 1
        leaves.add(p,(sign*.8,.35,-.3),rng.uniform(22,35),rng.uniform(20,30),"M_RV_LeafLight" if j%5==0 else"M_RV_Leaf",.12,.2)
leaves.emit(a);a.finish()
a=Asset("Flowers","A grounded flowering understory drift with small restrained gold blooms. No collision.")
leaves=Leaves();rng=random.Random(1801)
for stem in range(20):
    start=Vector((rng.uniform(-82,82),rng.uniform(-59,59),0));tip=start+Vector((rng.uniform(-9,9),rng.uniform(-7,7),rng.uniform(36,86)))
    a.sweep("Fine flowering stem",[start,(start+tip)*.5+Vector((5,0,0)),tip],[.9,.65,.22],"M_RV_BarkLight",5,1801+stem)
    for j in range(5):
        angle=math.tau*j/5
        leaves.add(tip,(math.cos(angle),math.sin(angle),.3),10,8,"M_RV_LeafGold",.13)
    for sign in(-1,1):leaves.add(start+(tip-start)*.45,(sign,.4,.1),23,9,"M_RV_Leaf",.12)
leaves.emit(a);a.finish()

# 15. Complementary mineral floor clusters. No replacement for root's TRELLIS shrine.
a=Asset("CrystalCluster","Complementary ground mineral cluster,~210cm wide/~180high. Three mineral collision hulls; teal emission belongs to new material family.")
rock(a,(0,0,0),(191,161,40),1901,False,"M_RV_CrystalBase")
for index,(center,radius,height,tilt) in enumerate([((-35,0,17),34,160,(-18,4)),((43,17,14),29,120,(27,10)),((3,-42,10),23,87,(2,-25)),((-73,28,5),17,57,(-14,9))]):
    x,y,z=center;sides=6;vertices=[]
    for zz,rr,shift in ((0,radius,0),(height*.72,radius*.91,.65),(height,radius*.15,1)):
        for i in range(sides):
            angle=math.tau*i/sides+.19*index
            vertices.append((x+math.cos(angle)*rr+tilt[0]*shift,y+math.sin(angle)*rr+tilt[1]*shift,z+zz))
    faces=[tuple(reversed(range(sides))),tuple(12+i for i in range(sides))]
    for row in range(2):
        for i in range(sides):faces.append((row*6+i,row*6+(i+1)%6,(row+1)*6+(i+1)%6,(row+1)*6+i))
    a.part("Elongated six-sided mineral",vertices,faces,"M_RV_Crystal",.4,False,1950+index)
    if index<3:a.hull(vertices,"mineral cluster shaft")
a.finish()

# 16. Water feature stonework; ring hulls retain the hollow interior.
a=Asset("Basin","600cm diameter. FootZ0. Interior water planeZ35/radius~230. Ring collision only; actual empty water volume, no opaque top cap.")
for i in range(20):
    a0=math.tau*i/20+.003;a1=math.tau*(i+1)/20-.003
    outline=[(math.cos(a0)*230,math.sin(a0)*230),(math.cos(a1)*230,math.sin(a1)*230),
        (math.cos(a1)*300,math.sin(a1)*300),(math.cos(a0)*300,math.sin(a0)*300)]
    vertices,faces=prism_data(outline,0,76,"XY")
    a.part("Basin shaped radial masonry",vertices,faces,"M_RV_StoneLight" if i%4==0 else"M_RV_Stone",2.1,seed=2001+i)
    a.hull(vertices,"basin rim ring segment")
    cap=[(math.cos(t)*r,math.sin(t)*r) for t,r in ((a0,226),(a1,226),(a1,300),(a0,300))]
    a.slab("Basin rounded coping",cap,72,82,"M_RV_StoneLight",2,2040+i)
    if i in(1,5,12):moss(a,(math.cos(a0)*270,math.sin(a0)*270,82),69,32,2060+i)
a.slab("Basin inset wet floor",[(math.cos(i*math.tau/32)*234,math.sin(i*math.tau/32)*234) for i in range(32)],-15,-8,"M_RV_StoneDark",.8)
a.finish()

# 17. Straight rill along X; water is supplied by Unreal at localZ-15.
a=Asset("Rill","600longX x180wideY. BedtopZ-30, banktopZ12. WaterplaneZ-15, width~130. Three simple hulls, open channel.")
for side in(-1,1):
    y1,y2=(66,90) if side==1 else(-90,-66)
    a.course("Rill hand-cut bank stones",-300,300,y1,y2,-30,42,2101+side,99,"M_RV_StoneLight")
    a.box_hull((0,(y1+y2)/2,-9),(600,24,42),"rill side bank")
for x in range(-300,300,100):a.quarry("Rill submerged bed slab",x+.6,x+99.4,-66,66,-40,10,2150+x,"M_RV_StoneDark",.8)
a.box_hull((0,0,-35),(600,132,10),"rill recessed bed")
a.finish()

# 18. Functional wide stone bridge, flat top for unambiguous traversal.
a=Asset("Bridge","700longX x700wideY. DeckTOP Z0. Solid flat UCX. Decorative curb atY edges rises32; span supports below deck, open central water span.")
for row in range(10):
    a.course("Bridge interlocked deck pavers",-350,350,-350+row*70+.7,-350+(row+1)*70-.7,-30,29.3,2201+row,103,"M_RV_Stone")
# The foundation follows a shallower vaulted underside across the span.
outline=[(-350,-26),(350,-26),(350,-140),(292,-140),(204,-99),(112,-66),(0,-58),(-112,-66),(-204,-99),(-292,-140),(-350,-140)]
a.part("Sculpted bridge vaulted substrate",*prism_data(outline,-338,338),"M_RV_StoneDark",2.0,seed=2251)
for side in(-1,1):
    y1,y2=(316,350) if side==1 else(-350,-316)
    a.course("Low beveled bridge curb",-350,350,y1,y2,-12,44,2260+side,116,"M_RV_StoneLight")
    a.box_hull((0,(y1+y2)/2,10),(700,34,44),"bridge edge curb")
a.box_hull((0,0,-29),(700,632,58),"continuous flat bridge decktop0")
for x in(-304,304):a.box_hull((x,0,-96),(92,630,80),"bridge end foundation")
a.finish()

# 19. Carved arrival/reward plinth with readable inset center and radial rim.
a=Asset("ArrivalPlinth","Diameter220, topZ70. Ground0. Low octagonal UCX. Footing below ward crystal interaction atZ76.")
for layer,(radius,bottom,top,mat) in enumerate([(110,0,23,"M_RV_StoneDark"),(105,22,48,"M_RV_Stone"),(99,47,70,"M_RV_StoneLight")]):
    outline=[(math.cos(i*math.tau/12)*radius,math.sin(i*math.tau/12)*radius) for i in range(12)]
    a.slab("Stepped radial shrine footing",outline,bottom,top,mat,2,2301+layer)
for i in range(12):
    ang=math.tau*i/12
    points=[(math.cos(ang)*r,math.sin(ang)*r,70.5) for r in(64,75,86)]
    a.sweep("Inlaid bronze radial index",points,[1.8,1.8,1.8],"M_RV_Bronze",6,2340+i)
a.hull([(math.cos(i*math.tau/12)*109,math.sin(i*math.tau/12)*109,z) for z in(0,70) for i in range(12)],"low plinth footprint")
a.finish()

# 20. Eight shallow physical steps ascending +Y.
a=Asset("Steps","440wideX x600runY; eight20cm rises, total160. Ascends+Y fromY-300 toY+300. BaseZ0. One box UCX per step.")
for step in range(8):
    y1=-300+step*75;y2=y1+75;top=(step+1)*20
    a.course("Stair riser masonry",-220,220,y1+.5,y2-.5,0,top-4,2401+step,91,"M_RV_Stone")
    for col in range(5):
        a.quarry("Rounded tread over riser",-220+col*88+.45,-220+(col+1)*88-.45,y1,y2,top-5,4.5,2450+step*5+col,"M_RV_StoneLight",.6)
    a.box_hull((0,(y1+y2)/2,top*.5),(440,75,top),"independent20cm stair rise")
a.finish()

# 21. Small ruin scatter with identifiable cut-stone fragments.
a=Asset("Rubble","Grounded low architectural rubble,~300x220x100. No collision; use outside active path.")
for i,(x,y,w,d,h) in enumerate([(-86,-41,123,99,61),(47,13,118,105,96),(115,-49,68,86,43),(-39,68,117,65,52)]):
    rock(a,(x,y,0),(w,d,h),2501+i,False,"M_RV_Stone")
a.finish()

# 22. Continuous planted landform beneath designed paths. Real layered banks,
# no visible flat rectangular arena foundation. Central walk datum staysZ0.
a=Asset("TerrainPatch","~4400x4400 island footing. Central~3200x3200 nearZ0; irregular perimeter rolls to-80, skirt to-260. Visual only. BedZ-22 under paving/paths.")
rng=random.Random(2601);count=48;verts=[]
for ring,(extent,z) in enumerate([(1490,0),(1890,-21),(2200,-80),(2160,-260)]):
    for i in range(count):
        ang=math.tau*i/count
        # Squircle provides generous cardinal width without square corners.
        xx=math.copysign(abs(math.cos(ang))**.68,math.cos(ang));yy=math.copysign(abs(math.sin(ang))**.68,math.sin(ang))
        wobble=1 if ring==0 else 1+.018*math.sin(i*1.74)+.012*math.cos(i*.91)
        zz=z+(0 if ring==0 else rng.uniform(-9,9))
        verts.append((xx*extent*wobble,yy*extent*wobble,zz))
verts.append((0,0,0));faces=[(len(verts)-1,i,(i+1)%count) for i in range(count)]
for row in range(3):
    for i in range(count):faces.append((row*count+i,row*count+(i+1)%count,(row+1)*count+(i+1)%count,(row+1)*count+i))
faces.append(tuple(reversed([3*count+i for i in range(count)])))
a.part("Continuous weathered island shoulder",verts,faces,"M_RV_Moss",0,True,2601)
for index in range(10):
    ang=index*math.tau/10+.17
    rock(a,(math.cos(ang)*1900,math.sin(ang)*1900,-177),(394,300,144),2620+index,False,"M_RV_StoneDark")
a.finish()

# 23. Broken upper architecture: clearly a ruined upper wall, not false doors.
a=Asset("RuinCrown","500W,~145D,~480H. BaseZ0, front+Y. Stepped broken upper masonry, no collision. Ground on existing upper walls only.")
sections=[(-250,-128,476),(-128,0,382),(0,128,282),(128,250,184)]
for section,(x1,x2,height) in enumerate(sections):
    for row,z in enumerate(range(0,height-18,39)):
        a.course("Broken upper bonded courses",x1,x2,-60,61,z+.5,min(37,height-18-z),2701+section*21+row,81,"M_RV_Stone")
    outline=[(x1+.5,height-21),(x2-.5,height-21),(x2-.5,height-3),(x2-17,height+2),
        ((x1+x2)*.5+9,height-2),((x1+x2)*.5-4,height+7),(x1+16,height+2),(x1+.5,height-7)]
    a.part("Broken chiseled upper coping",*prism_data(outline,-68,76),"M_RV_StoneLight",1.4,seed=2801+section)
    moss(a,((x1+x2)*.5,25,height+5),84,28,2811+section)
a.finish()

# 24. Narrow dressed end pier to close real wall terminals consistently.
a=Asset("WallEnd","200W x150D x650H. Front+Y, baseZ0. Capped wall terminal with one solid hull.")
for row in range(12):
    width=196 if row%3 else 200
    a.quarry("Terminal pier dressed course",-width/2,width/2,-71,71,row*50,48.5,2901+row,"M_RV_StoneLight" if row%4==0 else"M_RV_Stone",2)
for z in(0,589,622):
    a.quarry("Terminal pier modeled cap",-100,100,-75,75,z,26,2940+z,"M_RV_StoneLight",2)
a.box_hull((0,0,324),(194,141,648),"wall end load core")
a.finish()

# Compact source inspection with actual final geometry, not an invented
# concept or gameplay view. Metadata uses export-space bounds before posing.
for index,(name,entry) in enumerate(ASSETS.items()):
    obj=entry["object"];obj.hide_render=False
    dims=entry["meta"]["dimensions_cm"]
    if name in("SM_RV_Tree","SM_RV_Cliff","SM_RV_TerrainPatch"):
        obj.hide_render=True
        continue
    col=index%6;row=index//6
    obj.location=(col*800-2000,row*930,0)
    if name=="SM_RV_Ivy":obj.location.z=345
    if name=="SM_RV_Bough":obj.location.z=190
    if name=="SM_RV_Bridge":obj.location.z=160
    if name=="SM_RV_Rill":obj.location.z=45
tree=ASSETS["SM_RV_Tree"]["object"];tree.hide_render=False;tree.location=(2570,1450,0)
cliff=ASSETS["SM_RV_Cliff"]["object"];cliff.hide_render=False;cliff.location=(-2540,3450,0)
floor=bpy.data.materials.new("PREVIEW neutral ground");floor.use_nodes=True
floor.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value=(.087,.098,.085,1)
floor.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value=.94
bpy.ops.mesh.primitive_plane_add(size=22000,location=(0,0,-30));bpy.context.object.name="PREVIEW inspection ground";bpy.context.object.data.materials.append(floor)
world=bpy.data.worlds.new("Reverie inspection sky");scene.world=world;world.use_nodes=True
world.node_tree.nodes["Background"].inputs["Color"].default_value=(.23,.29,.33,1)
world.node_tree.nodes["Background"].inputs["Strength"].default_value=.48
sun_data=bpy.data.lights.new("Warm inspection sun","SUN");sun_data.energy=3.0;sun_data.angle=.12;sun_data.color=(1,.86,.65)
sun=bpy.data.objects.new("Warm inspection sun",sun_data);scene.collection.objects.link(sun);sun.rotation_euler=(math.radians(33),math.radians(-20),math.radians(130))
cam_data=bpy.data.cameras.new("Reverie kit inspection");camera=bpy.data.objects.new("Reverie kit inspection",cam_data);scene.collection.objects.link(camera);scene.camera=camera
camera.location=(5000,7500,6150);camera.rotation_euler=(Vector((0,1370,180))-camera.location).to_track_quat("-Z","Y").to_euler();cam_data.type="ORTHO";cam_data.ortho_scale=7400;cam_data.clip_end=50000
scene.render.engine="CYCLES";scene.cycles.device="CPU";scene.cycles.samples=24;scene.cycles.use_denoising=True
scene.render.resolution_x=1800;scene.render.resolution_y=1300;scene.render.resolution_percentage=100;scene.render.image_settings.file_format="PNG"
scene.view_settings.view_transform="AgX";scene.view_settings.exposure=.75
scene.render.filepath=str(OUT/"reverie-environment-kit.png");bpy.ops.render.render(write_still=True)
# Save a close architectural/detail view in addition to the complete overview.
for entry in ASSETS.values():entry["object"].hide_render=True
for name,location in[("SM_RV_Arch",(0,0,0)),("SM_RV_Wall",(-570,0,0)),("SM_RV_Buttress",(-788,0,0)),
    ("SM_RV_Tree",(735,-420,0)),("SM_RV_RockBank",(480,-270,0)),("SM_RV_Fern",(-470,170,0)),("SM_RV_Ivy",(-546,65,590)),
    ("SM_RV_CrystalCluster",(490,161,0)),("SM_RV_Flowers",(358,284,0)),("SM_RV_Basin",(-510,790,0))]:
    obj=ASSETS[name]["object"];obj.hide_render=False;obj.location=location
for x in range(-4,5):
    for y in range(-4,6):
        original=ASSETS["SM_RV_Tile" if (x+y)%2 else"SM_RV_TileB"]["object"]
        obj=original.copy();obj.data=original.data;scene.collection.objects.link(obj);obj.name="PREVIEW paved approach";obj.hide_render=False;obj.location=(x*200,y*200,-20)
camera.location=(1370,2370,840);camera.rotation_euler=(Vector((10,-80,390))-camera.location).to_track_quat("-Z","Y").to_euler();camera.data.type="PERSP";camera.data.lens=43
scene.render.resolution_x=1600;scene.render.resolution_y=1100;scene.render.filepath=str(OUT/"reverie-environment-detail.png");bpy.ops.render.render(write_still=True)
camera.location=(680,1480,655);camera.rotation_euler=(Vector((-25,0,404))-camera.location).to_track_quat("-Z","Y").to_euler();camera.data.lens=53
scene.render.filepath=str(OUT/"reverie-craft-closeup.png");bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/"Reverie_Environment.blend"),compress=True)
rows=[]
for name,entry in ASSETS.items():
    meta=entry["meta"]
    rows.append([name.lower(),"art/reverie/environment/"+name+".fbx","","Game Studio with OpenAI Codex",
        "Original project-authored asset; no third-party inputs","scripts/create_reverie_environment.py",
        f"New original Blender {bpy.app.version_string} sculpted/procedural geometry; {meta['triangles']}triangles; {meta['collision_hulls']} authored UCX; CraftUV1unit/m plus LightmapUV; original M_RV_ palette and vertex masks; no previous game geometry reused","original"])
for asset_id,path,note in[("reverie_environment_source","Reverie_Environment.blend","Editable original Blender source; no external texture dependency"),
    ("reverie_environment_kit_preview","reverie-environment-kit.png","Actual Blender Cycles asset kit inspection; not gameplay footage"),
    ("reverie_environment_detail_preview","reverie-environment-detail.png","Actual Blender Cycles architectural detail inspection; not gameplay footage"),
    ("reverie_environment_craft_preview","reverie-craft-closeup.png","Actual Blender Cycles close inspection of carved stone relief, damaged arrises and bronze/patina inlays; not gameplay footage")]:
    rows.append([asset_id,"art/reverie/environment/"+path,"","Game Studio with OpenAI Codex","Original project-authored asset; no third-party inputs","scripts/create_reverie_environment.py",note,"original"])
with(OUT/"manifest-rows.csv").open("w",newline="",encoding="utf-8") as handle:
    writer=csv.writer(handle);writer.writerow(["asset_id","path","source_url","creator","license","proof_path","modifications","approval_status"]);writer.writerows(rows)
(OUT/"README.md").write_text("""# Reverie environment kit

Twenty-four wholly new original Blender meshes for the sculpted painterly
abbey/garden direction. No old models or textures were loaded. Root supplies
new generated PBR textures, sky and water shaders; the source material nodes
are honest untextured vertex-colored previews of the new geometry.

Use integration-manifest.json and asset-metadata.json for exact bounds,
material slots, collision hulls, placement datums and exported file hashes.
All fronts are +Y, architectural width is X and traversal through Arch is Y.
CraftUV uses 1unit/100cm projected along each actual face; LightmapUV is a
separate smart-packed unwrap. Vertex Color: R=painted value, G=bevel wear,
B=recess/weathering, A=opaque. No new UV-dependent old image textures exist.

Tile/TileB: base0/top20; place-20 for a groundZ0 route. Bridge decktop0.
Arch: keep the entire440wide x500high rectangular ground passage clear.
Rill: longitudinalX; bed-30, water-15. Basin: water35, inner radius230.
Tree has trunk and low-root hulls; foliage has no collision. TerrainPatch and
Cliff are visual landforms and require scene-owned playable floor boundaries.
No decorative gateways should be placed against solid wall faces.

The two PNGs are Blender source inspections of actual meshes, not Unreal
gameplay captures. Material import, actual game integration, performance and
play validation belong to the root integration owner.
""",encoding="utf-8")
print("REVERIE_ENVIRONMENT_COMPLETE "+json.dumps({"meshes":len(ASSETS),"triangles":sum(e["meta"]["triangles"] for e in ASSETS.values())}),flush=True)

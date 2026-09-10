"""Original detailed habitat dressing for Dreambound; Blender background entry.

All dimensions are centimeters. This builds original geometry and three small
analytic surface texture pairs. No downloaded models, photographs, or textures
are consumed. Outputs only art/living-world. Geometry is decorative, no collision.
"""
import bpy
import csv
import hashlib
import json
import math
import random
from pathlib import Path
import numpy as np
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'art/living-world'
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = .01
scene.render.threads_mode = 'FIXED'
scene.render.threads = 4
bpy.context.preferences.filepaths.save_version = 0
TAU = math.tau

PALETTE = {
 'M_LW_Leaf': dict(color=[.068,.19,.063],roughness=.73,metallic=0,foliage=True,two_sided=True,texture='T_LW_Leaf',motion='breeze'),
 'M_LW_LeafLight': dict(color=[.19,.31,.087],roughness=.72,metallic=0,foliage=True,two_sided=True,texture='T_LW_Leaf',motion='breeze'),
 'M_LW_Reed': dict(color=[.31,.34,.12],roughness=.80,metallic=0,foliage=True,two_sided=True,texture='T_LW_Leaf',motion='breeze'),
 'M_LW_Bark': dict(color=[.17,.105,.066],roughness=.91,metallic=0,texture='T_LW_Bark'),
 'M_LW_BarkLight': dict(color=[.26,.165,.084],roughness=.91,metallic=0,texture='T_LW_Bark'),
 'M_LW_Wood': dict(color=[.43,.28,.13],roughness=.89,metallic=0,texture='T_LW_Bark'),
 'M_LW_EndGrain': dict(color=[.39,.245,.11],roughness=.91,metallic=0,texture='T_LW_EndGrain'),
 'M_LW_Fungus': dict(color=[.38,.18,.089],roughness=.77,metallic=0,texture='T_LW_Leaf'),
 'M_LW_Cream': dict(color=[.66,.55,.32],roughness=.78,metallic=0,two_sided=True,texture='T_LW_Leaf'),
 'M_LW_Petal': dict(color=[.65,.25,.14],roughness=.62,metallic=0,foliage=True,two_sided=True,texture='T_LW_Leaf',motion='breeze'),
 'M_LW_Ceramic': dict(color=[.29,.41,.34],roughness=.53,metallic=0,texture='T_LW_Ceramic'),
 'M_LW_Stone': dict(color=[.43,.37,.26],roughness=.87,metallic=0,texture='T_LW_Stone'),
 'M_LW_Moss': dict(color=[.16,.22,.071],roughness=.95,metallic=0,texture='T_LW_Leaf'),
 'M_LW_Insect': dict(color=[.063,.26,.27],roughness=.38,metallic=.15,texture='T_LW_Leaf',motion='insect_body'),
 'M_LW_Wing': dict(color=[.52,.62,.48],roughness=.40,metallic=.08,two_sided=True,texture='T_LW_Leaf',motion='insect_wing'),
 'M_LW_WingMark': dict(color=[.063,.26,.27],roughness=.38,metallic=.15,two_sided=True,texture='T_LW_Leaf',motion='insect_wing'),
}
for name,spec in PALETTE.items():
    spec.update(normal_texture=spec['texture']+'Normal',normal_strength=.24,
                vertex_masks=True,wear_color=[min(1,c*1.3) for c in spec['color']],
                weather_color=[c*.62 for c in spec['color']],weather_amount=.42,
                tone_low=[.78,.82,.76],tone_high=[1.13,1.10,1.03])
    if name in ('M_LW_Bark','M_LW_BarkLight','M_LW_Wood','M_LW_EndGrain'):
        spec.update(normal_strength=.67,tone_low=[.43,.40,.34],tone_high=[1.40,1.25,1.12])
    if name in ('M_LW_Stone','M_LW_Ceramic'):
        spec.update(normal_strength=.47,weather_color=[.10,.13,.060],weather_amount=.7,
                    tone_low=[.61,.64,.53],tone_high=[1.28,1.24,1.15])


def textures():
    n=2048
    yy,xx=(np.mgrid[0:n,0:n]/n).astype(np.float32)
    rng=np.random.default_rng(90513)
    def smooth_noise(cells):
        grid=rng.uniform(-1,1,(cells+1,cells+1)).astype(np.float32)
        grid[-1,:]=grid[0,:];grid[:,-1]=grid[:,0]
        sx=xx*cells;sy=yy*cells;ix=sx.astype(np.int32);iy=sy.astype(np.int32)
        tx=sx-ix;ty=sy-iy;tx=tx*tx*(3-2*tx);ty=ty*ty*(3-2*ty)
        return ((1-tx)*(1-ty)*grid[iy,ix]+tx*(1-ty)*grid[iy,ix+1]+(1-tx)*ty*grid[iy+1,ix]+tx*ty*grid[iy+1,ix+1]).astype(np.float32)
    broad=smooth_noise(5)*.65+smooth_noise(11)*.35
    grain=smooth_noise(27)*.5+smooth_noise(73)*.3+smooth_noise(151)*.2
    grit=smooth_noise(239)
    warp=.19*np.sin(TAU*yy*2)+.095*np.sin(TAU*yy*9)
    fissure=np.exp(-130*np.sin(math.pi*(xx*17+warp))**2)
    crosscrack=np.exp(-95*np.sin(math.pi*(yy*8+.24*np.sin(xx*TAU*13)))**2)
    radial=np.sqrt(((xx-.5)*1.08)**2+((yy-.5)*.94)**2)
    fields={
      'Leaf': .22*np.cos(TAU*xx)+.10*np.cos(TAU*(xx*3+yy*2))+.095*np.cos(TAU*(yy*11+abs(xx-.5)*13))+.07*grain,
      'Bark': .25*broad+.04*np.cos(TAU*(xx*51+.07*np.sin(yy*TAU*2)))+.12*grain-.23*fissure-.11*crosscrack,
      'EndGrain': .12*np.sin(TAU*(radial*31+.025*np.sin(xx*TAU*7)))+.035*np.sin(TAU*radial*83)+grain*.07,
      'Stone': .33*broad+.18*grain+.075*grit,
      'Ceramic': .33*broad+.105*grain+.025*grit,
    }
    for family,height in fields.items():
        pigment=np.ones((n,n,4),dtype=np.float32)
        pigment[:,:,0]=np.clip(.50+height*.85,0,1)
        pigment[:,:,1]=np.clip(.55+grain*.35,0,1)
        pigment[:,:,2]=np.clip(.30+height*.8,0,1)
        dy,dx=np.gradient(height)
        normal=np.stack((-dx*28,-dy*28,np.ones_like(dx)),axis=-1)
        normal/=np.linalg.norm(normal,axis=-1,keepdims=True)
        norm=np.ones_like(pigment);norm[:,:,:3]=normal*.5+.5
        for suffix,array in (('',pigment),('Normal',norm)):
            name='T_LW_'+family+suffix
            image=bpy.data.images.new(name,width=n,height=n,alpha=True)
            image.colorspace_settings.name='Non-Color'
            image.pixels.foreach_set(array.ravel())
            image.filepath_raw=str(OUT/(name+'.png'));image.file_format='PNG';image.save()


MATERIALS={}
def make_materials():
    for name,spec in PALETTE.items():
        mat=bpy.data.materials.new(name);mat.diffuse_color=(*spec['color'],1);mat.use_nodes=True
        nodes,links=mat.node_tree.nodes,mat.node_tree.links
        p=nodes.get('Principled BSDF');p.inputs['Roughness'].default_value=spec['roughness'];p.inputs['Metallic'].default_value=spec['metallic']
        col=nodes.new('ShaderNodeVertexColor');col.layer_name='Color'
        separate=nodes.new('ShaderNodeSeparateColor');links.new(col.outputs['Color'],separate.inputs['Color'])
        tex=nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.get(spec['texture'])
        grey=nodes.new('ShaderNodeMath');grey.operation='MULTIPLY_ADD';grey.inputs[1].default_value=.88;grey.inputs[2].default_value=.48
        links.new(tex.outputs['Color'],grey.inputs[0])
        mult=nodes.new('ShaderNodeMixRGB');mult.blend_type='MULTIPLY';mult.inputs[0].default_value=1
        mult.inputs[1].default_value=(*spec['color'],1);links.new(grey.outputs[0],mult.inputs[2])
        val=nodes.new('ShaderNodeMixRGB');val.blend_type='MULTIPLY';val.inputs[0].default_value=1
        weather=nodes.new('ShaderNodeMixRGB');weather.inputs[2].default_value=(*spec['weather_color'],1)
        amount=nodes.new('ShaderNodeMath');amount.operation='MULTIPLY';amount.inputs[1].default_value=spec['weather_amount']
        links.new(separate.outputs['Blue'],amount.inputs[0]);links.new(amount.outputs[0],weather.inputs[0]);links.new(mult.outputs[0],weather.inputs[1])
        links.new(weather.outputs[0],val.inputs[1]);links.new(separate.outputs['Red'],val.inputs[2]);links.new(val.outputs[0],p.inputs['Base Color'])
        nt=nodes.new('ShaderNodeTexImage');nt.image=bpy.data.images.get(spec['normal_texture'])
        nm=nodes.new('ShaderNodeNormalMap');nm.inputs['Strength'].default_value=spec['normal_strength']
        links.new(nt.outputs['Color'],nm.inputs['Color']);links.new(nm.outputs['Normal'],p.inputs['Normal'])
        MATERIALS[name]=mat


def catmull(points,steps=5):
    points=[Vector(p) for p in points];out=[]
    for j in range(len(points)-1):
        a,b,c,d=points[max(0,j-1)],points[j],points[j+1],points[min(len(points)-1,j+2)]
        for i in range(steps):
            t=i/steps
            out.append(.5*((2*b)+(-a+c)*t+(2*a-5*b+4*c-d)*t*t+(-a+3*b-3*c+d)*t*t*t))
    return out+[points[-1]]


class Mesh:
    def __init__(self,name,note):
        self.name=name;self.note=note;self.v=[];self.f=[];self.fm=[];self.fuv=[];self.fc=[];self.sm=[]
    def part(self,v,f,mat,smooth=True,uv=None,alpha=None,seed=1,wear=.015):
        # Compact shared parameter grids to only vertices used by this material
        # band. This keeps hollow wood and fan-fungus exports economical.
        used=sorted({i for face in f for i in face});mapping={old:new for new,old in enumerate(used)}
        offset=len(self.v);self.v.extend(tuple(v[i]) for i in used)
        value=.87+.11*random.Random(seed).random()
        for face in f:
            self.f.append(tuple(offset+mapping[i] for i in face));self.fm.append(mat);self.sm.append(smooth)
            self.fuv.append([uv[i] if uv else (v[i][0]/60,v[i][2]/60+v[i][1]/97) for i in face])
            colors=[]
            for i in face:
                x,y,z=v[i]
                local=(math.sin(x*.107+z*.047)+math.sin(y*.17-z*.071))*.035
                weather=.10
                if mat in ('M_LW_Bark','M_LW_BarkLight','M_LW_Stone','M_LW_Ceramic'):
                    weather=.08+max(0,1-z/34)*.31+max(0,math.sin(x*.14+y*.1+z*.09))*.12
                colors.append((min(1,value+local),wear,weather,alpha[i] if alpha else 0))
            self.fc.append(colors)
    def tube(self,points,radii,mat='M_LW_Bark',sides=10,steps=3,flute=.05,seed=1):
        path=catmull(points,steps);rr=[]
        for j in range(len(radii)-1):
            rr.extend(radii[j]+(radii[j+1]-radii[j])*t/steps for t in range(steps))
        rr.append(radii[-1]);v=[];uv=[]
        for j,p in enumerate(path):
            tangent=(path[min(len(path)-1,j+1)]-path[max(0,j-1)]).normalized()
            guide=Vector((0,0,1)) if abs(tangent.z)<.9 else Vector((0,1,0))
            u=tangent.cross(guide).normalized();w=tangent.cross(u).normalized()
            for k in range(sides):
                a=TAU*k/sides
                r=rr[j]*(1+flute*math.sin(7*a+j*.21+seed))
                v.append(p+(u*math.cos(a)+w*math.sin(a))*r);uv.append((k/sides,j/len(path)*2))
        f=[tuple(reversed(range(sides))),tuple((len(path)-1)*sides+k for k in range(sides))]
        for j in range(len(path)-1):
            for k in range(sides):f.append((j*sides+k,j*sides+(k+1)%sides,(j+1)*sides+(k+1)%sides,(j+1)*sides+k))
        self.part(v,f,mat,True,uv,seed=seed)
    def leaf(self,start,end,width,mat='M_LW_Leaf',curl=4,roll=0,seed=1,vein=False,wing=False):
        start,end=Vector(start),Vector(end);delta=end-start
        side=Vector((-delta.y,delta.x,0)).normalized()
        if side.length<.1:side=Vector((1,0,0))
        v=[];uv=[];alpha=[];rows=8 if width<3.2 and not wing else 12;columns=3 if width<3.2 and not wing else 5
        for j in range(rows+1):
            t=j/rows;cen=start+delta*t+Vector((0,0,math.sin(math.pi*t)*curl))
            w=width*(math.sin(math.pi*t)**.8)*(.94+.06*math.sin(j*3.1+seed))+.012
            for k in range(columns):
                across=(k/(columns-1)*2-1)
                zz=(1-abs(across))*.14*w-abs(across)*.09*w+roll*across*t
                v.append(cen+side*w*across+Vector((0,0,zz)))
                uv.append((k/(columns-1),t));alpha.append(min(1,t)*abs(across) if wing else t*.65)
        f=[]
        for j in range(rows):
            for k in range(columns-1):f.append((j*columns+k,j*columns+k+1,(j+1)*columns+k+1,(j+1)*columns+k))
        self.part(v,f,mat,True,uv,alpha,seed)
        if vein:
            path=[start+delta*t+Vector((0,0,math.sin(math.pi*t)*curl+width*.11*math.sin(math.pi*t)+.13)) for t in (0,.2,.4,.6,.8,1)]
            self.tube(path,[.24,.23,.22,.18,.13,.02],'M_LW_Reed',5,1,0,seed)
    def stone(self,center,size,seed=1,mat='M_LW_Stone'):
        rng=random.Random(seed);x,y,z=center;w,d,h=size;outline=[]
        for a,b in [(-.40,-.50),(.36,-.50),(.50,-.34),(.50,.33),(.37,.5),(-.33,.5),(-.5,.35),(-.5,-.35)]:
            outline.append((x+a*w+rng.uniform(-.015,.015)*w,y+b*d+rng.uniform(-.025,.025)*d))
        v=[]
        for ring,zz,scale in ((0,z,.93),(1,z+2,1),(2,z+h-3,1),(3,z+h,.92)):
            for i,(px,py) in enumerate(outline):
                v.append((x+(px-x)*scale,y+(py-y)*scale,zz+(rng.uniform(-1,1) if ring>1 else 0)))
        f=[tuple(reversed(range(8))),tuple(24+i for i in range(8))]
        for j in range(3):
            for i in range(8):f.append((j*8+i,j*8+(i+1)%8,(j+1)*8+(i+1)%8,(j+1)*8+i))
        self.part(v,f,mat,False,seed=seed,wear=.13)
    def finish(self):
        if self.name=='SM_LW_TwistedScrub':self.fuse_wood()
        used=sorted({i for face in self.f for i in face});mapping={old:new for new,old in enumerate(used)}
        self.v=[self.v[i] for i in used];self.f=[tuple(mapping[i] for i in face) for face in self.f]
        mesh=bpy.data.meshes.new(self.name);mesh.from_pydata(self.v,[],self.f);mesh.update()
        obj=bpy.data.objects.new(self.name,mesh);scene.collection.objects.link(obj)
        slots=[k for k in PALETTE if k in self.fm]
        for k in slots:mesh.materials.append(MATERIALS[k])
        uv=mesh.uv_layers.new(name='CraftUV');col=mesh.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
        for j,p in enumerate(mesh.polygons):
            p.material_index=slots.index(self.fm[j]);p.use_smooth=self.sm[j]
            for k,index in enumerate(p.loop_indices):uv.data[index].uv=self.fuv[j][k];col.data[index].color=self.fc[j][k]
        # Tiny curved leaves are explicit geometry with blade-space UVs. Repair
        # only collapsed side/cap islands while retaining the authored channels.
        ns={};uvpath=ROOT/'art/source/uv_tools.py';exec(compile(uvpath.read_text(),str(uvpath),'exec'),ns)
        repaired=ns['repair_mesh_uv'](mesh)
        bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
        light=mesh.uv_layers.new(name='LightmapUV');mesh.uv_layers.active=light
        bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(angle_limit=1.15,island_margin=.008);bpy.ops.object.mode_set(mode='OBJECT')
        mesh.uv_layers.active_index=0;mesh.uv_layers[0].active_render=True;mesh.calc_loop_triangles()
        low=[min(v[i] for v in self.v) for i in range(3)];high=[max(v[i] for v in self.v) for i in range(3)]
        path=OUT/(self.name+'.fbx')
        bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},apply_unit_scale=True,
          apply_scale_options='FBX_SCALE_UNITS',axis_forward='X',axis_up='Z',use_space_transform=True,bake_space_transform=True,
          use_mesh_modifiers=True,mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False,use_custom_props=False,path_mode='STRIP')
        META[self.name]=dict(name=self.name,vertices=len(mesh.vertices),triangles=len(mesh.loop_triangles),
          dimensions_cm=[round(high[i]-low[i],3) for i in range(3)],bounds_min_cm=low,bounds_max_cm=high,
          materials=slots,collision_hulls=0,uv_channels=['CraftUV','LightmapUV'],uv_faces_repaired=repaired,
          vertex_channels={'R':'authored value','G':'wear','B':'recess','A':'wind/wing tip weight'},
          placement=self.note,sha256=hashlib.sha256(path.read_bytes()).hexdigest())
        OBJECTS[self.name]=obj;obj.hide_render=True
        (OUT/'asset-metadata.json').write_text(json.dumps(META,indent=2)+'\n')
        print('LIVING_WORLD_READY '+self.name+' '+str(META[self.name]['triangles']),flush=True)
        return obj

    def fuse_wood(self):
        """Round biological forks using a small voxel union, then keep details.

        Leaf blades and seeds are untouched. The woody structure becomes one
        continuous surface instead of visibly intersecting tapered primitives.
        """
        wood=[j for j,name in enumerate(self.fm) if name=='M_LW_Bark']
        used=sorted({i for j in wood for i in self.f[j]});lookup={old:new for new,old in enumerate(used)}
        data=bpy.data.meshes.new('Woody union source');data.from_pydata([self.v[i] for i in used],[],[tuple(lookup[i] for i in self.f[j]) for j in wood]);data.update()
        obj=bpy.data.objects.new('Rounded woody structure',data);scene.collection.objects.link(obj)
        bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
        remesh=obj.modifiers.new('Continuous organic forks','REMESH');remesh.mode='VOXEL';remesh.voxel_size=.55;remesh.use_smooth_shade=True
        bpy.ops.object.modifier_apply(modifier=remesh.name)
        smooth=obj.modifiers.new('Rounded growth tissue','SMOOTH');smooth.factor=1.3;smooth.iterations=4;bpy.ops.object.modifier_apply(modifier=smooth.name)
        decimate=obj.modifiers.new('Reusable prop budget','DECIMATE');decimate.ratio=.33;bpy.ops.object.modifier_apply(modifier=decimate.name)
        vv=[tuple(v.co) for v in obj.data.vertices];ff=[tuple(p.vertices) for p in obj.data.polygons]
        keep=[j for j in range(len(self.f)) if j not in set(wood)]
        self.f=[self.f[j] for j in keep];self.fm=[self.fm[j] for j in keep];self.sm=[self.sm[j] for j in keep];self.fuv=[self.fuv[j] for j in keep];self.fc=[self.fc[j] for j in keep]
        self.part(vv,ff,'M_LW_Bark',True,seed=939)
        bpy.data.objects.remove(obj,do_unlink=True)


def mushroom(m,center,radius,seed=1):
    rng=random.Random(seed);c=Vector(center);segments=28;rings=8;v=[];uv=[]
    # Fan grows from its stem at the narrow rear. Alternating pale rings form
    # actual rounded lips, not flat disks or painted stripes on a cylinder.
    for j in range(rings):
        r=(j+.15)/(rings-1+.15)
        for i in range(segments+1):
            a=-math.pi*.83+i/segments*math.pi*1.66
            edge=1+.04*math.sin(a*11+seed)+.025*math.sin(a*19-seed)
            v.append(c+Vector((math.sin(a)*radius*r*edge,math.cos(a)*radius*r*.74,math.sin(r*math.pi)*radius*.14-r*r*radius*.08)))
            uv.append((i/segments,r))
    for j in range(rings-1):
        f=[]
        for i in range(segments):f.append((j*(segments+1)+i,j*(segments+1)+i+1,(j+1)*(segments+1)+i+1,(j+1)*(segments+1)+i))
        m.part(v,f,'M_LW_Cream' if j in (2,6) else 'M_LW_Fungus',True,uv,seed=seed+j)
    lower=[p-Vector((0,0,radius*.04)) for p in v[-segments-1:]]
    m.part([c-Vector((0,0,2))]+lower,[(0,i+1,i+2) for i in range(segments)],'M_LW_Cream',True,seed=seed)
    for i in range(0,segments,2):
        a=-math.pi*.83+i/segments*math.pi*1.66
        m.tube([c+Vector((0,0,-2)),c+Vector((math.sin(a)*radius*.5,math.cos(a)*radius*.36,-1)),c+Vector((math.sin(a)*radius*.94,math.cos(a)*radius*.69,-radius*.11))],[.20,.22,.09],'M_LW_Cream',4,1,0,seed+i)


def reed():
    m=Mesh('SM_LW_Reeds','Damp soil margin; bury base 2 cm. 1.4 m irregular curved reed and seedhead clump.')
    rng=random.Random(18)
    for i in range(15):
        a=rng.random()*TAU;r=rng.uniform(0,30);x,y=math.cos(a)*r,math.sin(a)*r;h=rng.uniform(61,142)
        end=Vector((x+math.cos(a)*16,y+math.sin(a)*16,h))
        m.tube([(x,y,-3),(x+3,y,h*.4),(x+8*math.cos(a),y+8*math.sin(a),h*.8),end],[.7,.58,.42,.22],'M_LW_Reed',7,3,.02,i)
        for k in range(3):
            t=.13+k*.19;s=Vector((x,y,h*t));theta=a+k*2.6
            e=s+Vector((math.cos(theta)*rng.uniform(27,45),math.sin(theta)*rng.uniform(27,45),rng.uniform(20,42)))
            m.leaf(s,e,rng.uniform(2.6,4.4),'M_LW_LeafLight' if i%4 else 'M_LW_Reed',9,-4,i+k,True)
        if i%3==0:
            m.tube([end-Vector((0,0,17)),end-Vector((0,0,15)),end-Vector((0,0,3)),end],[1.5,2.6,2.1,.6],'M_LW_Fungus',14,2,.1,i)
    m.finish()


def lily():
    m=Mesh('SM_LW_WaterLily','Water surface pivot; place at water Z + 0.7 cm. Five notched pads and a many-petal flower; no collision.')
    for k,(x,y,r) in enumerate([(-21,-9,24),(21,16,19),(3,-31,18),(-37,29,17),(33,-29,12)]):
        v=[(x,y,.4)];uv=[(.5,.5)];segments=48
        for i in range(segments+1):
            a=.22+(TAU-.44)*i/segments;r1=r*(1+.025*math.sin(i*2.5+k))
            v.append((x+math.cos(a)*r1,y+math.sin(a)*r1,1.5*math.sin(a*3+k)+.15));uv.append((.5+.48*math.cos(a),.5+.48*math.sin(a)))
        m.part(v,[(0,i+1,i+2) for i in range(segments)],'M_LW_Leaf' if k%2 else 'M_LW_LeafLight',True,uv,seed=k)
        for a in [i*TAU/11 for i in range(1,11)]:
            m.tube([(x,y,.7),(x+math.cos(a)*r*.5,y+math.sin(a)*r*.5,1.0),(x+math.cos(a)*r*.9,y+math.sin(a)*r*.9,1.5*math.sin(a*3+k)+.4)],[.27,.20,.03],'M_LW_Reed',4,2,0,k)
    for layer,count in enumerate((12,10,8)):
        for i in range(count):
            a=TAU*(i/count+layer*.045);r=15-layer*4;z=4+layer*2
            m.leaf((0,0,z),(math.cos(a)*r,math.sin(a)*r,z+3+layer*3),3.6-layer*.6,'M_LW_Cream' if layer!=1 else 'M_LW_Petal',4+layer,seed=i+layer)
    for i in range(17):
        a=i*2.4;r=3.7*math.sqrt((i+1)/17)
        m.tube([(math.cos(a)*r,math.sin(a)*r,7),(math.cos(a)*r,math.sin(a)*r,13)],[.55,.28],'M_LW_Reed',6,1,0,i)
    m.finish()


def deadwood():
    m=Mesh('SM_LW_FungusLog','Forest/spring soil; pivot base, bury 3 cm. Hollow splintered log with bark ridges, branch stubs and shelf fungi.')
    sides=48;rows=15;v=[];uv=[]
    for inner in (False,True):
        for j in range(rows):
            x=-84+j*168/(rows-1)
            for i in range(sides):
                a=TAU*i/sides;r=(21 if inner else 28)*(1+.055*math.sin(a*9+j*.11)+.025*math.sin(a*17))
                splinter=(4*math.sin(a*7)+2*math.sin(a*13))*(1 if j in (0,rows-1) else 0)
                v.append((x+splinter,math.sin(a)*r+math.sin(j/(rows-1)*math.pi)*7,28+math.cos(a)*r));uv.append((i/sides,j/(rows-1)*2.6))
    offset=sides*rows
    for inner in (0,1):
        f=[]
        for j in range(rows-1):
            for i in range(sides):
                f.append(tuple(inner*offset+k for k in (j*sides+i,j*sides+(i+1)%sides,(j+1)*sides+(i+1)%sides,(j+1)*sides+i)))
        m.part(v,f,'M_LW_Wood' if inner else 'M_LW_Bark',True,uv,seed=39)
    for j in (0,rows-1):
        grain_uv=[(.5+p[1]/61,.5+(p[2]-28)/61) for p in v]
        m.part(v,[(j*sides+i,j*sides+(i+1)%sides,offset+j*sides+(i+1)%sides,offset+j*sides+i) for i in range(sides)],'M_LW_EndGrain',True,grain_uv,seed=9)
        for ring in (22,24,26):
            pts=[]
            for step in range(49):
                a=TAU*step/48;r=ring*(1+.025*math.sin(a*7))
                x=(-84 if j==0 else 84)+4*math.sin(a*7)+2*math.sin(a*13)+(-.12 if j==0 else .12)
                pts.append((x,math.sin(a)*r,28+math.cos(a)*r))
            m.tube(pts,[.13]*len(pts),'M_LW_Wood',4,1,0,ring+j)
    # Short offset plates make actual overlapping bark shadows. Their ends are
    # lifted irregularly while the beveled sides bury into the core surface.
    rng=random.Random(624)
    for row in range(5):
        for band in range(9):
            center=-64+row*31+rng.uniform(-5,5);angle=TAU*(band+.15*(row%2))/9
            length=rng.uniform(29,45);width=rng.uniform(.24,.43);vv=[];uvv=[]
            for j in range(9):
                t=j/8;x=center+(t-.5)*length
                for k in range(5):
                    q=k/4;a=angle+(q-.5)*width*(.82+.18*math.sin(t*math.pi))
                    r=28*(1+.04*math.sin(a*9+x*.01))+.2+math.sin(q*math.pi)*(1.3+1.9*math.sin(t*math.pi))
                    vv.append((x,math.sin(a)*r+math.sin((x+84)/168*math.pi)*7,28+math.cos(a)*r));uvv.append((q,t*1.7))
            ff=[(j*5+k,j*5+k+1,(j+1)*5+k+1,(j+1)*5+k) for j in range(8) for k in range(4)]
            m.part(vv,ff,'M_LW_BarkLight' if (row+band)%4==0 else 'M_LW_Bark',True,uvv,seed=row*9+band)
    m.tube([(-28,3,41),(-38,10,61),(-35,18,72)],[11,7,4],'M_LW_Bark',28,7,.035,55)
    m.tube([(45,8,43),(56,10,57),(70,17,61)],[8,5,2],'M_LW_Bark',24,7,.035,56)
    for i in range(22):
        x=rng.uniform(-65,60);a=rng.uniform(-.65,.65);r=29.5
        p=Vector((x,math.sin(a)*r+5,28+math.cos(a)*r))
        m.leaf(p,p+Vector((rng.uniform(6,13),rng.uniform(-3,4),.1)),rng.uniform(2.0,4.5),'M_LW_Moss',.6,seed=i)
    for i,(x,y,z,r) in enumerate([(-47,17,39,20),(-19,23,25,25),(14,24,31,23),(35,18,47,17),(60,13,35,15)]):mushroom(m,(x,y,z),r,i+3)
    m.finish()
    m=Mesh('SM_LW_ShelfFungi','Damp soil or root-bed anchor, bury 2 cm. Stump and tiered fan fungi with rounded rims and modeled gills.')
    m.tube([(0,0,-3),(-2,2,20),(2,3,36)],[14,11,8],'M_LW_Bark',20,4,.14,72)
    for i,(x,y,z,r) in enumerate([(0,4,8,26),(-8,3,21,23),(5,2,36,19),(-4,0,47,13)]):mushroom(m,(x,y,z),r,i+21)
    m.finish()


def fern():
    m=Mesh('SM_LW_FernRosette','Moist understory ground; root base -2 cm; eleven divided fronds and coiled new growth.')
    rng=random.Random(77)
    for k in range(11):
        angle=k*TAU/11+rng.uniform(-.15,.15);length=rng.uniform(47,77);tipheight=rng.uniform(35,62)
        direction=Vector((math.cos(angle),math.sin(angle),0));side=Vector((-direction.y,direction.x,0))
        pts=[direction*(length*t)+Vector((0,0,tipheight*math.sin(t*math.pi*.63))) for t in (0,.2,.4,.6,.8,1)]
        m.tube(pts,[.72,.64,.48,.38,.25,.04],'M_LW_Reed',6,2,0,k)
        for j in range(1,12):
            t=j/13;at=direction*(length*t)+Vector((0,0,tipheight*math.sin(t*math.pi*.63)));length2=18*(math.sin(t*math.pi)**.8)*(1-.22*t)
            for sign in (-1,1):
                end=at+side*length2*sign+direction*length2*.45+Vector((0,0,3-t*5))
                m.leaf(at,end,2.2*(1-.45*t),'M_LW_LeafLight' if k%3==0 else 'M_LW_Leaf',2,seed=j+k)
    for i in range(3):
        theta=i*2.2;center=Vector((math.cos(theta)*8,math.sin(theta)*8,35+i*5));pts=[Vector((0,0,0)),center-Vector((3,0,13))]
        for j in range(30):
            a=-math.pi*.5+j/29*TAU*1.3;r=6*(1-j/34)
            pts.append(center+Vector((math.cos(a)*r,0,math.sin(a)*r)))
        m.tube(pts,[.85,.8]+[.75*(1-j/40) for j in range(30)],'M_LW_LeafLight',7,1,0,i)
    m.finish()


def flowers():
    m=Mesh('SM_LW_MeadowFlowers','Dry ruin garden / light understory. Root pivot -2 cm, seven drooping bell clusters and broad veined leaves.')
    rng=random.Random(500)
    for k in range(7):
        a=k*2.4;r=rng.uniform(1,24);h=rng.uniform(30,62);base=Vector((math.cos(a)*r,math.sin(a)*r,-2));top=base+Vector((math.cos(a)*7,math.sin(a)*7,h))
        m.tube([base,base+Vector((2,1,h*.5)),top],[.65,.45,.22],'M_LW_Leaf',6,3,0,k)
        for j in range(3):
            start=base+Vector((0,0,h*j*.18+3));aa=a+j*2.2
            m.leaf(start,start+Vector((math.cos(aa)*23,math.sin(aa)*23,7)),5.2,'M_LW_Leaf',5,seed=k+j,vein=True)
        for j in range(3):
            a2=a+j*TAU/3;flower=top+Vector((math.cos(a2)*7,math.sin(a2)*7,-j*8))
            m.tube([top-Vector((0,0,j*8)),flower+Vector((0,0,3)),flower],[.26,.20,.10],'M_LW_Leaf',6,2,0,j)
            for petal in range(5):
                aa=petal*TAU/5;start=flower+Vector((math.cos(aa),math.sin(aa),1))
                end=flower+Vector((math.cos(aa)*5.5,math.sin(aa)*5.5,-7))
                m.leaf(start,end,2.8,'M_LW_Cream' if k%3 else 'M_LW_Petal',-1,seed=k+petal)
            m.tube([flower-Vector((0,0,2)),flower-Vector((0,0,9))],[.5,.2],'M_LW_Reed',6,1,0,j)
    m.finish()


def scrub():
    m=Mesh('SM_LW_TwistedScrub','Dry garden rooted base -3 cm, 1.1 m high; five sinuous stems with forks, small leaves and seedpods.')
    rng=random.Random(91)
    for k in range(5):
        a=k*TAU/5;d=Vector((math.cos(a),math.sin(a),0));side=Vector((-d.y,d.x,0));h=rng.uniform(73,112)
        base=d*4-Vector((0,0,3));p1=d*13+side*5+Vector((0,0,h*.3));p2=d*27-side*8+Vector((0,0,h*.65));tip=d*38+side*7+Vector((0,0,h))
        m.tube([base,p1,p2,tip],[7.5,4.5,2.1,.15],'M_LW_Bark',14,7,.13,k)
        for j,t in enumerate((.3,.5,.7,.88)):
            at=Vector((1-t)**3*base+3*(1-t)**2*t*p1+3*(1-t)*t*t*p2+t**3*tip)
            sign=1 if j%2 else -1;end=at+d*(12+j*2)+side*sign*(21-j*2)+Vector((0,0,9))
            m.tube([at,(at+end)*.5+Vector((0,0,5)),end],[1.7,.7,.03],'M_LW_Bark',8,4,.07,j)
            for q in range(4):
                s=at.lerp(end,.25+q*.2)
                leafend=s+side*(1 if q%2 else -1)*12+d*4+Vector((0,0,7))
                m.leaf(s,leafend,3.0,'M_LW_LeafLight' if q%2 else 'M_LW_Reed',3,seed=q+j)
            if j%2==0:m.tube([end,end+Vector((0,0,3)),end+Vector((0,0,7))],[.5,1.6,.03],'M_LW_Fungus',9,2,.09,j)
    for k in range(6):
        a=k*TAU/6;d=Vector((math.cos(a),math.sin(a),0))
        m.tube([Vector((0,0,7)),d*20+Vector((0,0,4)),d*37-Vector((0,0,2))],[4,2,.05],'M_LW_Bark',10,4,.08,k)
    m.finish()


def urn():
    m=Mesh('SM_LW_CrackedUrn','Dry ruin edge; base at -2 cm. Hollow 80 cm ceramic urn, asymmetric broken rim, carved ribs, handles and three fallen sherds.')
    profile=[(0,15),(4,20),(11,20),(18,28),(34,33),(50,30),(63,23),(71,21),(76,25),(80,25)]
    sides=64;rows=len(profile);v=[];uv=[]
    for inside in (0,1):
        for j,(z,r) in enumerate(profile):
            for k in range(sides):
                a=k*TAU/sides
                missing=(min(1,max(0,(j-6)/3))*(11+8*math.sin(a*13))) if 11<=k<=22 else 0
                radius=r-inside*3+math.cos(a*16)*(.8 if j not in (0,1,8,9) else .25)
                v.append((math.cos(a)*radius,math.sin(a)*radius,z-2-missing));uv.append((k/sides,z/80))
    off=rows*sides
    for inside in (0,1):
        f=[]
        for j in range(rows-1):
            for k in range(sides):
                ids=(inside*off+j*sides+k,inside*off+j*sides+(k+1)%sides,inside*off+(j+1)*sides+(k+1)%sides,inside*off+(j+1)*sides+k)
                f.append(tuple(reversed(ids)) if inside else ids)
        m.part(v,f,'M_LW_Stone' if inside else 'M_LW_Ceramic',True,uv,seed=63)
    m.part(v,[((rows-1)*sides+k,(rows-1)*sides+(k+1)%sides,off+(rows-1)*sides+(k+1)%sides,off+(rows-1)*sides+k) for k in range(sides)],'M_LW_Stone',True,uv,seed=63)
    for sign in (-1,1):
        m.tube([(sign*27,0,55),(sign*42,0,61),(sign*47,0,46),(sign*39,0,32),(sign*30,0,33)],[2.7,3,3.4,3,2.4],'M_LW_Ceramic',12,6,.03,sign+20)
    for i in range(16):
        a=i*TAU/16
        m.tube([(math.cos(a)*r,math.sin(a)*r,z-2) for z,r in [(18,29),(27,32.3),(36,34),(46,32)]],[.6,.85,.8,.2],'M_LW_Cream',6,3,0,i)
    for band,z,r in ((0,12,22.7),(1,64,22.8)):
        pts=[(math.cos(TAU*i/64)*r,math.sin(TAU*i/64)*r,z+.3*math.sin(i*.73)) for i in range(65)]
        m.tube(pts,[.55]*65,'M_LW_Cream',6,1,0,band)
    # Mineral staining and tiny lichen lobes gather at the damp lower body.
    rng=random.Random(222)
    for patch in range(13):
        a=rng.random()*TAU;z=rng.uniform(7,23);r=22 if z<13 else 27+(z-13)*.39
        center=Vector((math.cos(a)*r,math.sin(a)*r,z));vv=[center];uvv=[(.5,.5)]
        tangent=Vector((-math.sin(a),math.cos(a),0));radial=Vector((math.cos(a),math.sin(a),0))
        for j in range(13):
            t=TAU*j/12;size=2.5+.7*math.sin(t*5+patch)
            vv.append(center+tangent*math.cos(t)*size+Vector((0,0,math.sin(t)*size*.65))+radial*.18);uvv.append((.5+.4*math.cos(t),.5+.4*math.sin(t)))
        m.part(vv,[(0,j+1,j+2) for j in range(12)],'M_LW_Moss',True,uvv,seed=patch)
    # Irregular fracture runs down from the broken rim, modeled as a shallow
    # dark ceramic separation. It never reads as a clean mechanical panel seam.
    a=17*TAU/sides
    m.tube([(math.cos(a+.02)*25,math.sin(a+.02)*25,67),(math.cos(a-.05)*25,math.sin(a-.05)*25,59),(math.cos(a+.04)*30.6,math.sin(a+.04)*30.6,49),(math.cos(a-.02)*33.5,math.sin(a-.02)*33.5,35)],[.5,.43,.37,.05],'M_LW_Bark',5,2,0,88)
    for i in range(3):m.stone((37+i*10,-27+i*9,-1),(13+i*3,12,3.3),i+24,'M_LW_Ceramic')
    m.finish()


def lintel():
    m=Mesh('SM_LW_FallenLintel','Dry ruin ground; bury 3 cm. Chipped carved limestone, scalloped relief border and layered rubble; decorative only.')
    m.stone((0,0,-3),(140,45,30),901)
    m.stone((-26,30,-2),(47,32,20),902)
    m.stone((61,-24,-2),(42,29,12),903)
    for x in range(-53,54,13):
        pts=[(x-6,8,27),(x-5,0,29),(x,-5,30),(x+5,0,29),(x+6,8,27)]
        m.tube(pts,[.8,1.0,1.1,1,.8],'M_LW_Cream',7,3,0,x)
        m.leaf((x,8,28),(x,17,29),3.3,'M_LW_Stone',1,seed=x)
    m.tube([(-61,-16,24),(-35,-16,25),(0,-16,25),(35,-16,25),(62,-16,23)],[1.1,1.1,1.1,1.1,.5],'M_LW_Cream',8,2,0,5)
    for i in range(7):
        x=-57+i*17
        m.leaf((x,5,28),(x+7,15,29),6,'M_LW_Moss',1.2,seed=i)
    for i in range(18):
        x=-62+i*7.0
        m.stone((x,-20.0,8+4*math.sin(i*1.9)),(6.5,3.5,2.4),i+610,'M_LW_Stone' if i%4 else 'M_LW_Cream')
    m.finish()


def roots():
    m=Mesh('SM_LW_RootFan','Tree/ruin dry soil; all endpoints dip under ground. Sinuous tapering roots with forks, moss and curled leaf litter.')
    rng=random.Random(512)
    for i in range(8):
        a=i*TAU/8+rng.uniform(-.1,.1);d=Vector((math.cos(a),math.sin(a),0));s=Vector((-d.y,d.x,0));length=rng.uniform(39,72)
        pts=[Vector((0,0,8)),d*length*.3+s*5+Vector((0,0,7)),d*length*.7-s*4+Vector((0,0,4)),d*length-Vector((0,0,3))]
        m.tube(pts,[6,4.3,2.0,.05],'M_LW_Bark',11,6,.12,i)
        branch=pts[2]+s*(15 if i%2 else -17)
        m.tube([pts[1],pts[2],branch-Vector((0,0,5))],[2.4,1.6,.04],'M_LW_Wood',8,4,.03,i)
    for i in range(18):
        a=rng.random()*TAU;r=rng.uniform(12,68);p=Vector((math.cos(a)*r,math.sin(a)*r,.4));end=p+Vector((math.cos(a+1)*12,math.sin(a+1)*12,2))
        m.leaf(p,end,4,'M_LW_Reed' if i%3 else 'M_LW_Moss',rng.uniform(1,4),2,seed=i)
    m.finish()


def insects():
    m=Mesh('SM_LW_Dragonfly','Airborne at damp margins, 20 cm wing span; wing-tip material flutter plus common body bob, no collision or AI.')
    m.tube([(0,-7,0),(0,-3,.6),(0,2,1.1),(0,5,1),(0,9,.3)],[.38,.62,1.1,1.1,.30],'M_LW_Insect',12,4,.08,3)
    for y in range(-6,0):m.tube([(-.4,y,0),(.0,y,.58),(.4,y,0)],[.1,.12,.1],'M_LW_Insect',5,2,0,y)
    for sign in (-1,1):
        for y in (1,3):m.leaf((sign*.7,y,.9),(sign*11,y-4,1.8),2.1,'M_LW_Wing',.3,seed=y,wing=True)
        m.tube([(sign*.7,3,.7),(sign*1.5,3,1.5),(sign*.7,4,1.8)],[.5,.7,.3],'M_LW_Insect',9,3,0,sign)
        for j in range(3):m.tube([(sign*.5,2-j,0),(sign*2.1,1-j,-1),(sign*2.9,-j,-1.7)],[.16,.13,.04],'M_LW_Insect',5,2,0,j)
    m.finish()
    m=Mesh('SM_LW_GardenMoth','Airborne above dry flower clumps; pale broad scalloped wings with inset teal eyespots; material wing flutter and body bob.')
    m.tube([(0,-4,0),(0,0,0),(0,3,.4)],[.3,1.0,.4],'M_LW_Insect',10,4,.1,6)
    for sign in (-1,1):
        m.leaf((0,1,.1),(sign*7,2,3.0),3.5,'M_LW_Wing',.7,seed=6,wing=True)
        m.leaf((0,-1,.1),(sign*5.3,-5,1.5),2.7,'M_LW_Wing',.8,seed=7,wing=True)
        m.leaf((sign*3,1,2.1),(sign*5,1.4,2.5),1.0,'M_LW_WingMark',.05,seed=7,wing=True)
        m.tube([(sign*.3,2,.4),(sign*1,4,.8),(sign*2,4.4,.8)],[.13,.11,.03],'M_LW_Insect',5,3,0,sign)
    m.finish()


def render_previews():
    # These source-rendered vignettes are labelled as source art in provenance;
    # the integration owner performs the actual packaged gameplay review.
    scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=24
    scene.cycles.use_denoising=True;scene.render.resolution_x=1500;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
    scene.world.color=(.065,.085,.11)
    ground=bpy.data.materials.new('Preview earth');ground.diffuse_color=(.095,.09,.07,1)
    bpy.ops.mesh.primitive_plane_add(size=2000,location=(0,0,-4));plane=bpy.context.object;plane.name='SOURCE_PREVIEW_ONLY_ground';plane.data.materials.append(ground)
    bpy.ops.object.light_add(type='AREA',location=(-150,-220,400));key=bpy.context.object;key.data.energy=2300000;key.data.shape='DISK';key.data.size=270
    key.rotation_euler=(Vector((0,0,35))-key.location).to_track_quat('-Z','Y').to_euler()
    bpy.ops.object.light_add(type='AREA',location=(250,100,300));fill=bpy.context.object;fill.data.energy=660000;fill.data.size=300
    fill.rotation_euler=(Vector((0,0,40))-fill.location).to_track_quat('-Z','Y').to_euler()
    bpy.ops.object.camera_add(location=(260,-330,255));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,43))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=360;scene.camera=cam
    groups={
      'living-world-spring-source.png': [('SM_LW_FungusLog',(-25,25,0),.9),('SM_LW_Reeds',(43,65,0),1),('SM_LW_ShelfFungi',(-84,7,0),1),('SM_LW_FernRosette',(-73,-45,0),.8),('SM_LW_WaterLily',(35,-56,0),1.0),('SM_LW_Dragonfly',(21,-13,84),2.3)],
      'living-world-ruin-source.png': [('SM_LW_CrackedUrn',(0,20,0),1),('SM_LW_TwistedScrub',(74,55,0),1),('SM_LW_FallenLintel',(-60,-26,0),.9),('SM_LW_MeadowFlowers',(59,-41,0),1),('SM_LW_RootFan',(-37,27,0),1),('SM_LW_GardenMoth',(48,-2,81),2.5)]
    }
    for filename,placements in groups.items():
        for obj in OBJECTS.values():obj.hide_render=True
        for name,position,size in placements:
            obj=OBJECTS[name];obj.hide_render=False;obj.location=position;obj.scale=(size,)*3
        scene.render.filepath=str(OUT/filename);bpy.ops.render.render(write_still=True)
    for obj in OBJECTS.values():obj.hide_render=False;obj.location=(0,0,0);obj.scale=(1,1,1)
    for obj in list(scene.objects):
        if obj.name not in OBJECTS:bpy.data.objects.remove(obj,do_unlink=True)
    # Source scene stores assets in a tidy gallery; FBXs retain ground pivots.
    for i,obj in enumerate(OBJECTS.values()):obj.location=((i%4)*230,(i//4)*230,0)


META={};OBJECTS={}
textures();make_materials()
for build in (reed,lily,deadwood,fern,flowers,scrub,urn,lintel,roots,insects):build()
(OUT/'materials.json').write_text(json.dumps(PALETTE,indent=2)+'\n')
render_previews()
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Living_World.blend'))
rows=[]
for p in sorted(OUT.glob('*')):
    if p.suffix not in ('.fbx','.png','.blend'):continue
    rows.append(dict(asset_id=p.stem.lower(),path='art/living-world/'+p.name,source_url='',creator='Game Studio with OpenAI Codex',
      license='Original project-authored asset; no third-party inputs',proof_path='scripts/create_living_world.py',
      modifications='Original parametric Blender modeling; sculpted curved mesh detail; original analytic textures; vertex masks; UVs; FBX export' if p.suffix=='.fbx' else 'Original procedural source/texture or labelled source-art preview',approval_status='original'))
with (OUT/'manifest-rows.csv').open('w',newline='',encoding='utf-8') as f:
    writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
print('LIVING_WORLD_COMPLETE '+json.dumps(dict(meshes=len(META),triangles=sum(m['triangles'] for m in META.values()))),flush=True)

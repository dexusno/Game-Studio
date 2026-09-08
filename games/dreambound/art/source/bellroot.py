"""Original Bellroot correction, executed by create_art.py with its FBX helpers.

Authored profiles and compositions; no external geometry, textures or libraries.
Vertex Color stores R=painted value, G=edge wear, B=recess/weathering, A=1.
"""


def decorate(obj, mat, bevel=0, smooth=False):
    import bmesh
    bm=bmesh.new();bm.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(obj.data);bm.free()
    obj.data.materials.append(MATS[mat])
    active(obj); bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        # A temporary second slot marks generated chamfer faces for local wear.
        obj.data.materials.append(MATS[mat])
        mod=obj.modifiers.new('Controlled rounded edges','BEVEL')
        mod.width=bevel; mod.segments=3 if mat.startswith('M_Shield') else 2
        mod.material=1; mod.affect='EDGES'
        bpy.ops.object.modifier_apply(modifier=mod.name)
    col=obj.data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
    uv=single_craft_uv(obj.data)
    stone=mat.startswith('M_Stone') or mat=='M_Mortar'
    organic=mat.startswith('M_Root') or mat.startswith('M_Leaf') or mat=='M_Moss'
    shade=random.uniform(.82,.99) if stone else random.uniform(.91,1.0)
    for face in obj.data.polygons:
        edge=face.material_index==1
        face.material_index=0; face.use_smooth=smooth
        scale=95 if stone else 45 if organic else 35
        # Projection uses the same world-space axes as the sampled points.
        # Cylinder primitives can retain object rotation until finish().
        project_face_uv(obj.data,face,uv,scale,obj.matrix_world)
        for index in face.loop_indices:
            p=obj.matrix_world @ obj.data.vertices[obj.data.loops[index].vertex_index].co
            field=noise_vector(p*.048+Vector((5.2,9.7,2.3))).x
            wear=(.28+.32*max(0,field+.3)) if edge else .012
            damp=max(0,.35+noise_vector(p*.016+Vector((9,2,4))).y)*(.4 if stone else .28)
            if organic: wear=0; damp*=.45
            col.data[index].color=(max(.68,min(1,shade+field*.085)),wear,damp*(1-wear),1)
    if bevel:
        obj.data.materials.pop(index=1)
        if not smooth:
            mod=obj.modifiers.new('Sculpted broad face normals','WEIGHTED_NORMAL');mod.keep_sharp=True;mod.weight=45
            bpy.ops.object.modifier_apply(modifier=mod.name)
    parts.append(obj)
    return obj


def path_tube(points,radii,mat,sides=12,substeps=4,bark=False):
    points=[Vector(p) for p in points]; centers=[]; widths=[]
    for j in range(len(points)-1):
        p0=points[max(j-1,0)];p1=points[j];p2=points[j+1];p3=points[min(j+2,len(points)-1)]
        for i in range(substeps):
            t=i/substeps
            centers.append(.5*((2*p1)+(-p0+p2)*t+(2*p0-5*p1+4*p2-p3)*t*t+(-p0+3*p1-3*p2+p3)*t*t*t))
            widths.append(radii[j]*(1-t)+radii[j+1]*t)
    centers.append(points[-1]);widths.append(radii[-1]);verts=[];faces=[];length=0;lengths=[]
    previous_axis=None
    for j,p in enumerate(centers):
        if j:length+=(p-centers[j-1]).length
        lengths.append(length)
        tangent=(centers[min(j+1,len(centers)-1)]-centers[max(j-1,0)]).normalized()
        if previous_axis is None:
            guide=Vector((0,0,1)) if abs(tangent.z)<.88 else Vector((0,1,0))
            a=tangent.cross(guide).normalized()
        else:
            a=(previous_axis-tangent*previous_axis.dot(tangent)).normalized()
        b=tangent.cross(a).normalized();previous_axis=a
        for i in range(sides):
            angle=math.tau*i/sides
            flute=1+(.09*math.cos(angle*5+j*.095)+.035*math.sin(angle*9-j*.13) if bark else 0)
            verts.append(tuple(p+(a*math.cos(angle)+b*math.sin(angle))*widths[j]*flute))
    faces=[tuple(reversed(range(sides))),tuple((len(centers)-1)*sides+i for i in range(sides))]
    for j in range(len(centers)-1):
        for i in range(sides):faces.append((j*sides+i,j*sides+(i+1)%sides,(j+1)*sides+(i+1)%sides,(j+1)*sides+i))
    obj=mesh('Authored flowing profile',verts,faces,mat,0,True)
    # Bark UV follows the branch instead of stretching packed islands.
    for face in obj.data.polygons:
        if face.index < 2:
            project_face_uv(obj.data,face,obj.data.uv_layers.active,45,centered=True)
            continue
        columns=[obj.data.loops[i].vertex_index % sides for i in face.loop_indices]
        seam=0 in columns and sides-1 in columns
        for index in face.loop_indices:
            row,column=divmod(obj.data.loops[index].vertex_index,sides)
            u=1 if seam and column==0 else column/sides
            obj.data.uv_layers.active.data[index].uv=(u,lengths[row]/110)
    return obj


def plate_outline(outline,x,depth,mat,bevel=.4,bulge=0):
    verts=[(x+offset+bulge*(1-min(1,abs(y)/45)),y,z) for offset in [-depth/2,depth/2] for y,z in outline]
    n=len(outline);faces=[tuple(reversed(range(n))),tuple(n+i for i in range(n))]
    faces.extend((i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n))
    return mesh('Designed armor outline',verts,faces,mat,bevel)


def body_sections(sections,mat,bevel=.7):
    verts=[];sides=12
    for z,front,back,width in sections:
        for i in range(sides):
            a=math.tau*i/sides;c=math.cos(a)
            verts.append((c*(front if c>0 else back),math.sin(a)*width,z))
    faces=[tuple(reversed(range(sides))),tuple((len(sections)-1)*sides+i for i in range(sides))]
    for j in range(len(sections)-1):
        for i in range(sides):faces.append((j*sides+i,j*sides+(i+1)%sides,(j+1)*sides+(i+1)%sides,(j+1)*sides+i))
    return mesh('Purposeful tapered armor',verts,faces,mat,bevel)


def stroke(points,radius=.25,mat='M_Patina'):
    return path_tube(points,[radius]*len(points),mat,6,2)


def bolt(p,radius=1.0,axis='X',mat='M_DarkMetal'):
    cylinder(p,radius,.65,mat,axis,10,.12)


# A true shield: ceramic panels protect the full disc, around a central mechanism.
ring((0,0,0),42.5,35.3,7,'M_ShieldBronze','X',.55,72)
ring((1.5,0,0),40.5,12.0,4.8,'M_DarkMetal','X',.5,72)
ring((3.5,0,0),14.7,9.0,8.2,'M_ShieldBronze','X',.35,56)
ring((7.0,0,0),12.8,9.4,1.6,'M_Patina','X',.16,48)
cylinder((1.9,0,0),8.9,4.7,'M_Core','X',40,.25)
for i in range(8):
    a=i*math.tau/8
    stroke([(4.5,math.cos(a)*4,math.sin(a)*4),(4.5,math.cos(a+.17)*8.7,math.sin(a+.17)*8.7)],.35,'M_ShieldBronze')
for sector in range(6):
    start=sector*60+3;end=(sector+1)*60-3
    arc(15.6,40.35,start,end,4.7,'M_ShieldCeramic','X',(4.1,0,0),.65,18)
    # Inset ceramic overlap and tapered bronze seams retain a circular silhouette.
    arc(37.6,38.35,start+3,end-3,.45,'M_ShieldBronze','X',(6.65,0,0),.10,18)
    a=math.radians(start+27)
    y,z=math.cos(a),math.sin(a)
    for r in [19,34.2]:bolt((6.9,y*r,z*r),.83)
    # A cast, curved spear-leaf motif points toward the protected core.
    stroke([(7,y*23,z*23),(7.2,math.cos(a+.09)*27,math.sin(a+.09)*27),(7.0,y*31,z*31)],.21,'M_ShieldBronze')
    stroke([(7,y*23,z*23),(7.2,math.cos(a-.09)*27,math.sin(a-.09)*27),(7.0,y*31,z*31)],.21,'M_ShieldBronze')
for angle in range(0,360,30):
    a=math.radians(angle)
    bolt((3.8,math.cos(a)*41,math.sin(a)*41),.92,'X','M_ShieldBronze')
    # Rear structural spokes and bearing caps are visible while held.
    path_tube([(-3,math.cos(a)*15,math.sin(a)*15),(-4.5,math.cos(a+.045)*26,math.sin(a+.045)*26),(-2.5,math.cos(a)*38.7,math.sin(a)*38.7)],[1.6,1.3,1.0],'M_ShieldBronze',8,3)
for sector in range(6):
    arc(20.5,36.7,sector*60+8,sector*60+52,1.35,'M_ShieldCeramic','X',(-3.7,0,0),.32,12)
ring((-4.2,0,0),17.2,13.9,2,'M_ShieldBronze','X',.2,48)
for side in [-1,1]:
    path_tube([(-4,side*11,-10),(-8,side*10,-12),(-12,side*8,-12)],[1.6,1.5,1.4],'M_ShieldBronze',10,3)
    bolt((-6,side*12,-10),1.3)
path_tube([(-12,-8,-12),(-12,0,-12),(-12,8,-12)],[1.4,1.5,1.4],'M_Cloth',12,2)
finish('SM_ShieldPlate','Complete physical shield; face +X; centered disc; rear grip(-12,0,-12)',[0,0,0])

# Separate core can remain animated and element-swapped on the rear face.
ring((0,0,0),9.2,6.5,3.8,'M_ShieldBronze','X',.22,48)
cylinder((0,0,0),6.45,3.1,'M_Core','X',40,.24)
for side in [-1,1]:
    ring((side*1.65,0,0),5.95,5.35,.4,'M_DarkMetal','X',.08,40)
    for j in range(8):
        a=j*math.tau/8
        stroke([(side*1.85,math.cos(a)*2.8,math.sin(a)*2.8),(side*1.85,math.cos(a+.2)*5.5,math.sin(a+.2)*5.5)],.22,'M_ShieldBronze')
    orb((side*1.8,0,0),(.65,2.4,2.4),'M_Core')
finish('SM_Core','Core center; shield attachment(-6,0,0)',[-6,0,0])

# Articulated human-scale cyborg hand: grasp center is the origin, elbow behind.
path_tube([(-54,0,-5),(-40,0,-3.8),(-26,0,-2.5),(-17,0,-1.0)],[8.7,8.3,6.4,5.3],'M_Cloth',12,3)
for a,b,w in [(-53,-40,8.9),(-38.5,-27,7.8),(-25.5,-17,6.3)]:
    loft([(a,w,4.8,1),(a+3,w+.4,5.6,1),(b,w-.8,4.5,1)],'M_Ceramic','X',.65)
    ring((a+1.1,0,-2),w+.4,w-1.2,1.7,'M_Bronze','X',.2,28)
for side in [-1,1]:
    path_tube([(-51,side*7,-7),(-35,side*6.4,-6),(-17,side*4.4,-5)],[1,1,.7],'M_Bronze',8,3)
    for x in [-48,-34,-23]:bolt((x,side*7,1),.8,'Y')
cylinder((-14,0,-.8),5.7,11,'M_Bronze','Y',24,.4)
body_sections([(-4,5,6,5),(2,5.5,6.5,5.5)],'M_DarkMetal',.5).location=(-8,0,0)
plate_outline([(-5,-2),(-5,3),(0,4.3),(5,3),(5,-2),(0,-3.2)],-10,2.2,'M_Ceramic',.45)
for y,length in [(-4.8,.94),(-1.6,1.08),(1.6,1.05),(4.8,.88)]:
    joints=[(-8,y,2),(-2.5,y,4.0),(3.2*length,y,2.0),(3.0*length,y,-2.3),(-.6,y,-3.2)]
    for a,b in zip(joints,joints[1:]):
        path_tube([a,tuple((Vector(a)+Vector(b))*.5),b],[1.15,1.2,.95],'M_Bronze',8,2)
    for p in joints[1:-1]:orb(p,(1.25,1.3,1.2),'M_DarkMetal',1)
    box((-6,y,3.6),(3.8,2.45,1.2),'M_Ceramic',.32)
path_tube([(-10,6,-1),(-6,8,-3),(0,7,-4),(2.3,4.5,-2.8)],[1.6,1.5,1.25,1.0],'M_Bronze',10,3)
for p in [(-6,8,-3),(0,7,-4)]:orb(p,(1.6,1.5,1.35),'M_DarkMetal',1)
finish('SM_Forearm','Grip origin; curled hand around Y bar; forearm extends -X',[-12,0,-12])


# Sentinel cuirass, flexible understructure, layered hip armor and a weighted tabard.
body_sections([(-11,12,11,16),(6,17,13,18),(30,24,16,25),(49,25,17,29),(62,15,13,22)],'M_DarkMetal',1.0)
plate_outline([(-24,51),(-20,61),(-8,62),(0,57),(8,62),(20,61),(24,51),(20,24),(8,10),(0,6),(-8,10),(-20,24)],23,4.2,'M_Bronze',1.2,2)
for side in [-1,1]:
    plate_outline([(side*4,54),(side*9,59),(side*21,55),(side*19,42),(side*11,30),(side*6,32)],26,1.5,'M_Ceramic',.55)
    for z in [17,10,3]:
        plate_outline([(side*8,z+9),(side*24,z+6),(side*28,z-6),(side*13,z-10)],15,3.8,'M_Bronze',.7)
    stroke([(27,side*7,48),(27,side*12,40),(25,side*16,26)],.42,'M_Patina')
ring((27,0,35),10.5,7.8,4,'M_Bronze','X',.35,40)
cylinder((28,0,35),7.7,2.8,'M_Core','X',32,.15)
for j in range(8):
    a=j*math.tau/8
    stroke([(29.8,math.cos(a)*7.5,35+math.sin(a)*7.5),(29.8,math.cos(a+.1)*4,35+math.sin(a+.1)*4)],.36,'M_Bronze')
for z in [9,3,-3]:ring((0,0,z),18.4,15.2,2.1,'M_Bronze','Z',.3,24)
cylinder((0,0,65),11.6,10,'M_DarkMetal','Z',24,.4)
ring((0,0,62),15.8,11.9,4.2,'M_Bronze','Z',.4,32)
# Cloth is a folded, thick silhouette with a ragged hem, not flat rectangles.
verts=[];faces=[];rows=[(-8,14),(-22,15),(-40,13),(-59,11)]
for row,(z,width) in enumerate(rows):
    for j in range(9):
        u=j/8*2-1
        verts.append((20+math.cos(u*math.pi*2)*2.3+row*.8,u*width,z+(2.8*math.sin(j*2.1) if row==3 else 0)))
for row in range(3):
    for j in range(8):faces.append((row*9+j,row*9+j+1,(row+1)*9+j+1,(row+1)*9+j))
tabard=mesh('Folded heavy tabard',verts,faces,'M_Cloth',0,True)
solid=tabard.modifiers.new('Woven cloth thickness','SOLIDIFY');solid.thickness=.7;active(tabard);bpy.ops.object.modifier_apply(modifier=solid.name)
for side in [-1,1]:stroke([(22,side*13,-9),(22,side*14,-24),(23,side*12,-43),(25,side*10,-59)],.55,'M_Bronze')
stroke([(25,0,-21),(26,-5,-29),(26,0,-41),(26,5,-29),(25,0,-21)],.5,'M_Bronze')
finish('SM_GuardianBody','Hip center; +X front; source feet+94',[0,0,94])

# Closed helm with a narrow visor and a cast central crest.
body_sections([(0,10,9,10),(8,15,11,15),(24,17,12,15),(34,13,11,10),(39,6,7,5)],'M_Bronze',.65)
plate_outline([(-12,25),(-8,31),(0,34),(8,31),(12,25),(10,17),(0,12),(-10,17)],17,2,'M_DarkMetal',.4)
plate_outline([(-10,24),(-8,26),(8,26),(10,24),(8,22),(-8,22)],18.2,1,'M_Core',.18)
plate_outline([(-2.2,34),(0,38),(2.2,34),(2.8,18),(0,12),(-2.8,18)],20,1.4,'M_Bronze',.3)
for side in [-1,1]:
    plate_outline([(side*3,14),(side*10,17),(side*14,11),(side*11,2),(side*4,4)],15.8,2.1,'M_Bronze',.5)
    cylinder((0,side*14.5,16),5.5,3,'M_DarkMetal','Y',24,.25)
    cylinder((0,side*16,16),3.8,.9,'M_Bronze','Y',16,.22)
    for z in [6,9,12]:stroke([(17,side*5,z),(16,side*10,z+1.5)],.32,'M_Patina')
body_sections([(31,13,10,1.1),(41,9,10,1.0),(49,1,4,.5)],'M_Bronze',.3)
finish('SM_GuardianHead','Neck base; +X front; source feet+156',[0,0,156])

# Same shoulder-pivot arm serves both sides; broad pauldron, long striking gauntlet.
orb((0,0,0),(13.5,13.5,13.5),'M_DarkMetal',2)
body_sections([(-17,12,10,12),(-7,19,14,19),(7,20,15,19),(17,12,11,12)],'M_Bronze',1)
for z,width in [(6,17),(-3,19),(-12,16)]:
    plate_outline([(-width,z+7),(-width*.6,z+10),(width*.6,z+10),(width,z+7),(width*.85,z-3),(0,z-9),(-width*.85,z-3)],19.3,2.5,'M_Bronze',.6)
plate_outline([(-9,10),(0,13),(9,10),(7,1),(0,-4),(-7,1)],22,1.4,'M_Ceramic',.4)
for y in [-11,11]:bolt((22,y,-5),1.3)
path_tube([(0,0,-16),(0,0,-25),(0,0,-35)],[9.8,8.4,7.4],'M_DarkMetal',12,3)
for side in [-1,1]:path_tube([(5,side*7,-16),(6,side*6,-26),(4,side*5,-35)],[1.8,1.5,1.4],'M_Bronze',10,3)
cylinder((0,0,-36),8.7,19,'M_Bronze','Y',24,.45)
body_sections([(-65,8.3,7.2,8),(-59,13,9,11),(-45,14,10,12),(-40,10,8,9)],'M_Bronze',.8)
plate_outline([(-7,-41),(-10,-49),(-8,-62),(0,-68),(8,-62),(10,-49),(7,-41)],14,2,'M_Ceramic',.55)
stroke([(15,-3,-44),(16,0,-59),(15,3,-44)],.45,'M_Bronze')
orb((0,0,-68),(7.2,7.4,7),'M_DarkMetal',2)
for y in [-6,-2,2,6]:
    path_tube([(2,y,-69),(10,y,-73),(9,y,-80),(3,y,-83)],[1.8,2,1.9,1.3],'M_Bronze',8,3)
    bolt((11.6,y,-75),1,'X')
path_tube([(-3,8,-70),(4,11,-74),(7,6,-79)],[2.3,2.2,1.6],'M_Bronze',10,3)
finish('SM_GuardianArm','Shoulder center; arm hangs -Z; symmetric reuse',[0,36,146])

orb((0,0,-3),(10,10.8,11),'M_DarkMetal',2)
body_sections([(-8,11,8,11),(-20,13,9,13),(-36,9,7,9)],'M_Bronze',.85)
plate_outline([(-7,-10),(0,-6),(7,-10),(8,-21),(5,-33),(0,-37),(-5,-33),(-8,-21)],13,2,'M_Bronze',.5)
orb((0,0,-43),(9.2,9,9),'M_DarkMetal',2)
cylinder((9,0,-43),8.5,7,'M_Bronze','X',20,.5)
plate_outline([(-6,-43),(0,-37),(6,-43),(3,-49),(-3,-49)],13.4,1.3,'M_Ceramic',.45)
body_sections([(-52,11,7,10),(-64,13,8,11),(-82,9,6,8)],'M_Bronze',.75)
plate_outline([(-7,-52),(0,-49),(7,-52),(6,-72),(0,-86),(-6,-72)],13.8,2,'M_Bronze',.55)
for side in [-1,1]:stroke([(15,side*3,-55),(16,side*3,-72),(13,0,-82)],.45,'M_Patina')
verts=[(-13,-11,-94),(24,-11,-94),(27,-6,-94),(27,6,-94),(24,11,-94),(-13,11,-94),
       (-11,-10,-78),(12,-10,-79),(23,-6,-85),(23,6,-85),(12,10,-79),(-11,10,-78)]
faces=[tuple(reversed(range(6))),tuple(range(6,12))]+[(j,(j+1)%6,(j+1)%6+6,j+6) for j in range(6)]
mesh('Sloped articulated boot',verts,faces,'M_DarkMetal',1.2)
for y in [-5.5,5.5]:box((15,y,-85),(17,9.7,8),'M_Bronze',1.1,(0,.13,0))
ring((0,0,-80),9.8,7.8,3,'M_Bronze','Z',.32,24)
finish('SM_GuardianLeg','Hip joint; sole -94cm; common guardian scale .90',[0,18,94])


def clipped_cell(seed,seeds):
    polygon=[Vector((-100,-100)),Vector((100,-100)),Vector((100,100)),Vector((-100,100))]
    for other in seeds:
        if other==seed:continue
        normal=Vector(other)-Vector(seed);threshold=(Vector(other).length_squared-Vector(seed).length_squared)/2
        output=[]
        for j,p in enumerate(polygon):
            q=polygon[(j+1)%len(polygon)];dp=p.dot(normal)-threshold;dq=q.dot(normal)-threshold
            if dp<=0:output.append(p)
            if (dp<0)!=(dq<0):output.append(p+(q-p)*(-dp/(dq-dp)))
        polygon=output
        if not polygon:break
    return polygon


def flagstone(poly,top,mat):
    center=sum(poly,Vector((0,0)))/len(poly)
    poly=[p+(center-p).normalized()*1.5 for p in poly]
    verts=[(p.x,p.y,z+(random.uniform(-.6,.6) if z==top else 0)) for z in [8,top] for p in poly]
    n=len(poly);faces=[tuple(reversed(range(n))),tuple(range(n,2*n))]+[(j,(j+1)%n,(j+1)%n+n,j+n) for j in range(n)]
    return mesh('Irregular hand-cut flagstone',verts,faces,mat,.85)


for tile_name,seeds in [
    ('SM_StoneTile',[(-72,-75),(-13,-78),(63,-76),(-78,-14),(-20,-11),(52,-17),(91,26),(-71,57),(-11,58),(46,68)]),
    ('SM_StoneTile_B',[(-78,-83),(-29,-66),(49,-79),(85,-30),(-72,-16),(0,-6),(56,23),(-87,61),(-36,55),(24,66),(81,84)]),
]:
    box((0,0,5),(200,200,10),'M_Mortar',.4)
    for i,seed in enumerate(seeds):flagstone(clipped_cell(seed,seeds),random.uniform(23.1,24),'M_StoneLight' if i in [1,5,8] else 'M_Stone')
    collision_box((0,0,12),(200,200,24))
    finish(tile_name,'Ground center; upper walking surface near +24cm')


def stone_block(loc,dims,mat='M_Stone',bevel=1.1,damage=1):
    # Cut corner lengths differ by face; the large planes remain controlled.
    x,y,z=dims;cuts=[random.uniform(.04,.13)*min(x,y) for unused in range(4)]
    polygon=[(-x/2+cuts[0],-y/2),(x/2-cuts[1],-y/2),(x/2,-y/2+cuts[1]),(x/2,y/2-cuts[2]),(x/2-cuts[2],y/2),(-x/2+cuts[3],y/2),(-x/2,y/2-cuts[3]),(-x/2,-y/2+cuts[0])]
    verts=[]
    for height in [-z/2,z/2]:
        for px,py in polygon:verts.append((loc[0]+px+random.uniform(-damage,damage),loc[1]+py+random.uniform(-damage,damage),loc[2]+height+random.uniform(-damage*.6,damage*.6)))
    faces=[tuple(reversed(range(8))),tuple(range(8,16))]+[(i,(i+1)%8,(i+1)%8+8,i+8) for i in range(8)]
    return mesh('Chisel-cut masonry',verts,faces,mat,bevel)


stone_block((0,0,10),(400,68,20),'M_StoneLight',1.5)
for row,cuts in enumerate([[-200,-118,-29,63,200],[-200,-156,-64,39,130,200],[-200,-111,-13,103,200],[-200,-153,-44,69,152,200],[-200,-100,17,125,200]]):
    for j,(a,b) in enumerate(zip(cuts,cuts[1:])):
        height=54 if row<4 else [54,46,62,39][j%4]
        stone_block(((a+b)/2,random.uniform(-.8,.8),47+row*55),(b-a-2.7,59,height),'M_StoneLight' if (row+j)%7==0 else 'M_Stone',1.3,1.4)
for x,width,height in [(-145,108,17),(-37,99,20),(68,103,13),(160,75,9)]:
    stone_block((x,0,305),(width,68,height),'M_StoneLight',1.3,1.2)
collision_box((0,0,154),(397,56,308))
finish('SM_Wall','Ground center; X width; broken crown and irregular ashlar')


def pillar(height=350,broken=False):
    stone_block((0,0,11),(103,103,22),'M_Stone',2,1.4)
    stone_block((0,0,30),(86,86,17),'M_StoneLight',1.4,1)
    verts=[];faces=[];sides=32
    for j,z in enumerate([39,49,79,height-50,height-28]):
        for i in range(sides):
            a=math.tau*i/sides;radius=(33-j*.85)*(1-.075*(.5+.5*math.cos(a*8)))
            top_offset=(math.sin(a*3)*10+math.cos(a*7)*3 if broken and j==4 else 0)
            verts.append((math.cos(a)*radius,math.sin(a)*radius,z+top_offset))
    faces=[tuple(reversed(range(sides))),tuple(4*sides+i for i in range(sides))]
    for j in range(4):
        for i in range(sides):faces.append((j*sides+i,j*sides+(i+1)%sides,(j+1)*sides+(i+1)%sides,(j+1)*sides+i))
    mesh('Single carved fluted shaft',verts,faces,'M_Stone',.45)
    for z,r in [(48,36),(70,34)]:ring((0,0,z),r,r-3.5,5,'M_StoneLight','Z',.6,32)
    if not broken:
        lathe([(-12,27),(-8,35),(0,39),(8,43),(12,43),(12,27)],(0,0,height-20),'M_StoneLight','Z',12,.8)
        stone_block((0,0,height-6),(95,95,12),'M_StoneLight',1.4,1.8)
    collision_box((0,0,height/2),(69,69,height))
pillar();finish('SM_Pillar','Ground center; single shaft, height350cm')
pillar(173,True);finish('SM_PillarBroken','Ground center; broken shaft; decorative cover')

for step in range(6):
    height=(step+1)*16
    stone_block((0,16+step*32,height-7.5),(200,33,15),'M_StoneLight' if step%3==0 else 'M_Stone',.85,1)
    for side in [-1,1]:stone_block((side*91,16+step*32,(height-15)/2),(18,33,max(1,height-15)),'M_Stone',.8,.6)
    collision_box((0,16+step*32,height/2),(200,32,height))
finish('SM_Stair','Lower-front origin; ascend +Y; six16cm rises; 200cm width')

for side in [-1,1]:
    stone_block((side*171,0,12),(104,92,24),'M_StoneLight',1.5)
    for row in range(5):stone_block((side*171,0,47+row*39),(73,74,37.6),'M_Stone',1.25,1.3)
    stone_block((side*171,0,229),(91,85,18),'M_StoneLight',1.2)
    collision_box((side*171,0,114),(72,72,228))
for i in range(15):
    a=i*12+.55;b=(i+1)*12-.55
    obj=arc(132,198+(4 if i in [6,7,8] else 0),a,b,76,'M_StoneLight' if i==7 else 'M_Stone','Y',(0,0,236),1.0,3)
    for vertex in obj.data.vertices:
        vertex.co+=noise_vector(vertex.co*.17)*.65
    collider=obj.copy();collider.data=obj.data.copy();scene.collection.objects.link(collider);collisions.append(collider)
for a,b in [(3,56),(59,114),(117,177)]:arc(199,208,a,b,81,'M_StoneLight','Y',(0,0,236),.8,12)
finish('SM_Arch','Ground center; open passage along Y; worn voussoirs; crown~444cm')

# Cast bronze bell, stepped lip and raised crest; no detached decorative bands.
lathe([(0,63),(4,71),(12,70),(19,58),(44,49),(78,34),(111,25),(127,20),(135,13),(135,9),(124,15),(109,20),(76,29),(42,43),(18,51),(8,62)],(0,0,0),'M_Bronze','Z',64,.6)
for z,r in [(16,60),(25,56),(104,27),(120,22)]:ring((0,0,z),r+1.3,r-1.0,2.7,'M_Bronze','Z',.22,64)
ring((0,0,148),14,8.5,7,'M_Bronze','Y',.5,32)
for z,axis in [(164,'Y'),(174,'X'),(184,'Y')]:ring((0,0,z),6.2,3.8,2,'M_DarkMetal',axis,.3,24)
path_tube([(0,0,126),(0,0,75),(0,0,18)],[2.7,2.5,2.2],'M_DarkMetal',12,3)
orb((0,0,15),(10,10,14),'M_Bronze',2)
for side in [-1,1]:
    y=side
    stroke([(-13,45*y,47),(-18,39*y,65),(-9,33*y,83),(0,29*y,93),(9,33*y,83),(18,39*y,65),(13,45*y,47),(0,48*y,40),(-13,45*y,47)],1.1,'M_Bronze')
    stroke([(0,46*y,46),(0,40*y,65),(0,32*y,88)],.95,'M_Patina')
    for sign in [-1,1]:stroke([(0,40*y,65),(sign*10,42*y,59),(sign*13,37*y,75),(0,35*y,79)],.7,'M_Patina')
finish('SM_Bell','Lower lip origin; suspension eye Z148; tree attach(-8,-112,286)')


def root_system():
    paths=[([(0,-5,96),(-35,-45,55),(-105,-90,24),(-220,-170,8),(-330,-190,3)],[39,32,23,10,1.2]),
           ([(24,-5,81),(66,-46,47),(158,-80,16),(285,-131,5),(354,-185,2)],[37,31,18,8,1]),
           ([(-12,20,105),(-81,56,42),(-169,105,16),(-283,172,3)],[34,29,18,1]),
           ([(22,22,92),(67,77,51),(158,178,20),(223,264,5)],[37,30,18,1.2]),
           ([(-13,-13,53),(23,-104,32),(13,-216,8),(85,-310,3)],[29,25,12,1]),
           ([(0,33,57),(-31,134,30),(-100,229,4)],[31,22,1])]
    for points,radii in paths:path_tube(points,radii,'M_Root',16,6,True)
    for points,radii in [([(-105,-90,24),(-147,-147,12),(-188,-257,2)],[12,7,1]), ([(158,-80,16),(211,-43,7),(291,-29,2)],[10,6,1]),([(-169,105,16),(-258,65,3),(-341,82,1)],[10,5,.8])]:
        path_tube(points,radii,'M_Root',12,5,True)


root_system()
trunk=[(0,0,0),(5,6,113),(-13,16,238),(13,25,351),(34,47,459),(9,77,581),(46,130,660)]
path_tube(trunk,[85,67,56,49,37,21,3],'M_Root',24,7,True)
for points,radii in [
    ([(0,21,294),(-67,49,399),(-175,71,501),(-294,115,568),(-381,63,627)],[43,35,24,13,2.3]),
    ([(22,31,381),(131,66,449),(208,29,535),(304,-31,589),(378,-91,650)],[35,29,22,11,2]),
    ([(8,10,333),(-21,-66,400),(63,-121,467),(125,-112,551),(98,-131,624)],[31,27,20,12,2]),
    ([(0,55,428),(-60,140,507),(-116,251,600),(-37,335,677)],[33,25,15,2]),
    ([(-175,71,501),(-216,-30,540),(-291,-100,610)],[18,12,2]),
    ([(208,29,535),(295,132,598),(366,183,661)],[19,12,2]),
]:path_tube(points,radii,'M_Root',18,6,True)
path_tube([(52,-110,459),(24,-117,469),(-8,-112,478)],[14,11,7],'M_Root',14,5,True)
# Unequal bark ribs follow the trunk and join its buttress roots.
for i,angle in enumerate([-.2,.5,1.1,1.8,2.45,3.2,3.85,4.5,5.1,5.65]):
    points=[]
    for j,p in enumerate(trunk):
        radius=[78,62,52,46,33,18,2][j]
        a=angle+j*.06
        points.append((p[0]+math.cos(a)*radius,p[1]+math.sin(a)*radius,p[2]))
    path_tube(points,[7.2,5.8,4.4,3.9,2.8,1.3,.3],'M_RootDark' if i%3 else 'M_RootLight',7,4)
# Hollow scar and twisted rim create a local focal surface on the front trunk.
orb((-15,-56,252),(18,3.8,39),'M_RootDark',2)
path_tube([(-31,-57,220),(-35,-59,254),(-23,-58,288),(-8,-59,296),(4,-59,277),(3,-58,237),(-10,-57,215),(-31,-57,220)],[4,4,3,2.4,3,4,3.7,4],'M_Root',9,4,True)
bpy.ops.mesh.primitive_cylinder_add(vertices=12,radius=65,depth=265,location=(0,0,132.5));collisions.append(bpy.context.object)
finish('SM_BellTree','Grounded trunk origin; roots and controlled branches; one trunk-base hull')


def add_leaf(verts,faces,center,direction,width,length,roll=0,heart=False):
    center=Vector(center);direction=Vector(direction).normalized()
    guide=Vector((0,0,1)) if abs(direction.z)<.85 else Vector((0,1,0))
    side=direction.cross(guide).normalized();up=side.cross(direction).normalized()
    side2=side*math.cos(roll)+up*math.sin(roll);up2=up*math.cos(roll)-side*math.sin(roll)
    base=len(verts)
    # Central ridge, folded side planes and pointed, asymmetrical contour.
    for t,w in [(0,.08),(.27,.82),(.55,1.0),(.81,.58),(1,0)]:
        ridge=center+direction*(length*t)+up2*(math.sin(math.pi*t)*width*.18)
        verts.extend([tuple(ridge-side2*w*width),tuple(ridge+up2*width*.10),tuple(ridge+side2*w*width*.93)])
    for row in range(4):
        i=base+row*3;faces.extend([(i,i+3,i+4,i+1),(i+1,i+4,i+5,i+2)])


# The canopy is a branched crown of individual folded leaves, with open sky gaps.
clusters=[(-320,75,-24,150,105,85),(-242,-80,-5,135,120,100),(-130,26,21,155,145,105),
          (15,-80,13,151,104,91),(120,-129,11,116,110,100),(274,-26,19,161,137,105),
          (382,-95,31,115,112,92),(329,150,60,140,110,95),(-91,246,68,139,132,102),
          (-8,336,76,135,118,93),(100,137,89,140,145,90)]
for mat,count in [('M_Leaf',90),('M_LeafLight',60),('M_LeafGold',18)]:
    verts=[];faces=[]
    for cx,cy,cz,rx,ry,rz in clusters:
        for unused in range(count):
            a=random.uniform(0,math.tau);u=random.uniform(-1,1);radius=random.uniform(.34,1)**.5
            center=(cx+rx*math.cos(a)*math.sqrt(1-u*u)*radius,cy+ry*math.sin(a)*math.sqrt(1-u*u)*radius,cz+rz*u*.72)
            direction=(math.cos(a+.3),math.sin(a+.3),random.uniform(-.2,.35))
            add_leaf(verts,faces,center,direction,random.uniform(8,15),random.uniform(28,43),random.uniform(-.55,.55))
    mesh('Layered leaf crown '+mat,verts,faces,mat,0,False)
for p in [[(-210,55,-101),(-277,83,-48),(-390,109,-12)],[(210,10,-88),(321,-4,-34),(398,-96,0)],[(-79,191,-65),(-46,280,-25),(20,357,33)]]:
    path_tube(p,[6,3.2,.6],'M_Root',10,4,True)
for cx,cy,cz,rx,ry,rz in clusters:
    for angle in [.3,2.4,4.5]:
        end=(cx+math.cos(angle)*rx*.65,cy+math.sin(angle)*ry*.65,cz+rz*.22)
        path_tube([(cx,cy,cz-35),(cx+math.cos(angle)*rx*.3,cy+math.sin(angle)*ry*.3,cz-12),end],[2.8,1.3,.22],'M_Root',7,3)
finish('SM_Canopy','Leaf crown center; attach to BellTree at local Z620',[0,0,620])

for i in range(7):
    a=i*math.tau/7+.25;length=[87,108,76,114,95,78,103][i]
    points=[Vector((math.cos(a)*t*t*length*.7,math.sin(a)*t*t*length*.7,length*(1.3*t-.56*t*t))) for t in [j/10 for j in range(11)]]
    path_tube(points,[.65*(1-j/12) for j in range(11)],'M_Moss',6,1)
    verts=[];faces=[]
    for j in range(1,10):
        t=j/10;size=18*math.sin(math.pi*t)**.6
        for side in [-1,1]:add_leaf(verts,faces,points[j],(math.cos(a+side*1.15),math.sin(a+side*1.15),.10),size*.24,size,side*.23)
    mesh('Fern paired pinnae',verts,faces,'M_LeafLight' if i%3 else 'M_Leaf',0)
finish('SM_Fern','Ground center; seven curved fronds with paired leaflets')

for j in range(4):
    points=[(j*14-21,0,0),(j*10-15,-4,-42),(j*19-30,-8,-101),(j*14-22,-5,-174+j*13)]
    path_tube(points,[1.0,.8,.65,.2],'M_RootDark',6,4)
    verts=[];faces=[]
    for i in range(10):
        t=i/10;x=j*14-21+math.sin(i*1.7)*8;z=-t*(174-j*13)
        add_leaf(verts,faces,(x,-7,z),((-1 if i%2 else 1)*.6,-.1,-.8),random.uniform(6,10),random.uniform(15,23),.65)
    mesh('Hanging ivy leaves',verts,faces,'M_Leaf' if j%2 else 'M_LeafLight',0)
finish('SM_Ivy','Top anchor; hanging foliage descends -Z, front -Y')

for i in range(9):
    a=i*2.39996;loc=(math.cos(a)*random.uniform(10,58),math.sin(a)*random.uniform(12,45),random.uniform(9,22))
    obj=stone_block(loc,(random.uniform(23,58),random.uniform(20,42),random.uniform(17,41)),'M_StoneLight' if i%3==0 else 'M_Stone',1.4,2.4)
    obj.rotation_euler=(random.uniform(-.15,.15),random.uniform(-.2,.2),random.uniform(-1,1))
finish('SM_Rubble','Ground-centered loose cut masonry; decorative, no blocking collision')

# A short standalone buttress root is useful only at wall/tree contact points.
path_tube([(0,0,45),(47,12,32),(104,5,17),(202,42,5),(270,18,2)],[32,25,18,8,1],'M_Root',16,6,True)
path_tube([(93,6,22),(135,-25,12),(208,-59,2)],[15,10,1],'M_Root',12,5,True)
for y in [-10,4,13]:path_tube([(3,y,70),(47,y+12,51),(104,y+5,29),(191,y+32,10)],[2.1,2,1.3,.3],'M_RootDark',7,5)
finish('SM_Root','Crown contact at origin; a rooted buttress, never repeat as a horizontal strip')

# Supporting crate, crystal and small grass preserve stable integration names.
box((0,0,39),(81,68,74),'M_DarkMetal',1.3)
for y in [-36,36]:
    for j in range(5):stone=box(((j-2)*16.5,y,41),(15.4,4,76),'M_Wood',.5)
for x in [-42,42]:
    for j in range(4):box((x,(j-1.5)*17,41),(4,15.8,76),'M_Wood',.45)
for j in range(5):box(((j-2)*16.5,0,79),(15.4,73,5),'M_Wood',.45)
for x in [-32,32]:
    box((x,0,82),(5,78,2.5),'M_Bronze',.25)
    for y in [-38.5,38.5]:
        box((x,y,41),(5,2.5,79),'M_Bronze',.25)
        for z in [8,72]:bolt((x,y,z),.9,'Y')
collision_box((0,0,42),(86,79,84));finish('SM_Crate','Ground center; supporting timber prop')

for cx,cy,r,height,lean in [(0,0,9,45,5),(10,3,5,29,6),(-7,5,5,24,-5)]:
    verts=[]
    for z,rr,offset in [(0,r*.7,0),(height*.72,r,lean*.7),(height,0,lean)]:
        for j in range(6):a=j*math.tau/6;verts.append((cx+math.cos(a)*rr+offset,cy+math.sin(a)*rr,z))
    faces=[tuple(reversed(range(6)))]+[(row*6+j,row*6+(j+1)%6,(row+1)*6+(j+1)%6,(row+1)*6+j) for row in range(2) for j in range(6)]
    mesh('Faceted mineral growth',verts,faces,'M_Crystal',.1)
ring((0,0,3),15.2,11.4,6,'M_Bronze','Z',.3,32);finish('SM_Crystal','Base origin; attachment or pickup')

verts=[];faces=[]
for j in range(12):
    a=j*2.39996
    add_leaf(verts,faces,(math.cos(a)*8,math.sin(a)*8,0),(math.cos(a)*.65,math.sin(a)*.65,1),1.2,random.uniform(18,36),.2)
mesh('Small irregular verge grass',verts,faces,'M_Moss',0);finish('SM_Grass','Ground center; small supporting grass tuft')

box((0,0,157),(212,38,312),'M_DarkMetal',5)
for x in [-105,105]:body=box((x,-2,157),(17,49,313),'M_Bronze',2)
ring((0,-24,181),70,55,11,'M_Bronze','Y',.8,56)
cylinder((0,-26,181),54,4,'M_DarkMetal','Y',48,.4)
ring((0,-30,181),48,45,2,'M_Core','Y',.15,48)
for x in [-83,83]:
    box((x,-24,160),(15,8,164),'M_Ceramic',1.6)
    for z in [112,137,162,187,212]:box((x,-29,z),(8,3,4),'M_Bronze',.2)
box((0,-31,181),(9,4,57),'M_Core',1.0)
collision_box((0,0,157),(226,50,314));finish('SM_TechPanel','Ground center; contrasting arrival, front -Y')

ring((0,0,.5),100,94,1,'M_CombatGlow','Z',0,64)
finish('SM_TelegraphRing','Ground center; outer radius100cm; no collision')
cylinder((0,0,.25),100,.5,'M_CombatGlow','Z',64,0)
finish('SM_TelegraphDisk','Ground center; radius100cm; no collision')

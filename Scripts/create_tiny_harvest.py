import unreal, json, math
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
L=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
E=unreal.EditorAssetLibrary
T=unreal.AssetToolsHelpers.get_asset_tools()
M=unreal.MaterialEditingLibrary
DST='/Game/Maps/TinyHarvest'
assert not E.does_asset_exist(DST), 'Destination already exists; do not overwrite authored work'
assert E.duplicate_asset('/Game/Maps/PortalPuzzle',DST)
assert L.load_level(DST)
def mat(name,col,metal=0,rough=.6):
    m=unreal.load_asset('/Game/TinyHarvest/Materials/'+name)
    if m: return m
    m=T.create_asset(name,'/Game/TinyHarvest/Materials',unreal.Material,unreal.MaterialFactoryNew())
    c=M.create_material_expression(m,unreal.MaterialExpressionConstant3Vector); c.set_editor_property('constant',unreal.LinearColor(*col,1)); M.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
    for value,prop in [(rough,unreal.MaterialProperty.MP_ROUGHNESS),(metal,unreal.MaterialProperty.MP_METALLIC)]:
        n=M.create_material_expression(m,unreal.MaterialExpressionConstant); n.set_editor_property('r',value); M.connect_material_property(n,'',prop)
    M.recompile_material(m); E.save_loaded_asset(m); return m
wood=mat('M_HoneyWood',(.37,.17,.055))
wood2=mat('M_LightWood',(.51,.29,.115))
teal=mat('M_DeepTeal',(.022,.13,.115),.3,.35)
brass=mat('M_Brass',(.65,.35,.085),.7,.28)
cream=mat('M_Ceramic',(.85,.78,.57),.05,.3)
dark=mat('M_Seam',(.075,.034,.017))
cube=unreal.load_asset('/Engine/BasicShapes/Cube')
def mesh(name,path,pos,size=None,rot=(0,0,0),material=None,collision=False):
    m=unreal.load_asset(path) if isinstance(path,str) else path
    assert m, path
    a=A.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*pos),unreal.Rotator(pitch=rot[0],yaw=rot[1],roll=rot[2])); a.set_actor_label(name); a.set_folder_path('Tiny Harvest/Set dressing')
    c=a.static_mesh_component; c.set_static_mesh(m)
    if size: a.set_actor_scale3d(unreal.Vector(*(v/100 for v in size)))
    if material: c.set_material(0,material)
    if not collision:
        c.set_collision_profile_name('NoCollision')
        c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return a

def block(n,p,s,m,collision=False): return mesh(n,cube,p,s,material=m,collision=collision)
def prop(name,path,x,y,bottom,span,yaw=0):
    a=mesh(name,path,(x,y,bottom),rot=(0,yaw,0)); b=a.static_mesh_component.get_editor_property('static_mesh').get_bounds(); scale=span/max(b.box_extent.x*2,b.box_extent.y*2,b.box_extent.z*2); a.set_actor_scale3d(unreal.Vector(scale,scale,scale)); a.set_actor_location(unreal.Vector(x,y,bottom-(b.origin.z-b.box_extent.z)*scale),False,False); return a
# Preserve gameplay actors, links and all playable floor positions.
for a in list(A.get_all_level_actors()):
    name=a.get_actor_label()
    if isinstance(a,unreal.TextRenderActor) or isinstance(a,unreal.PointLight): A.destroy_actor(a); continue
    if name in ['Ceiling','Vault ceiling']: A.destroy_actor(a); continue
    if isinstance(a,unreal.StaticMeshActor):
        if name.startswith('Observation bars'): a.static_mesh_component.set_material(0,brass)
        elif 'floor' in name.lower(): a.static_mesh_component.set_material(0,wood)
        elif 'indicator' in name.lower(): pass
        elif name != 'Vault observation barrier': a.static_mesh_component.set_material(0,teal)
        if name in ['South wall','West wall','Vault west','Vault east','Vault back','East wall left','East wall right']:
            # Keep physical enclosure, but open the upper sight line to the giant world.
            a.static_mesh_component.set_visibility(False)
            p=a.get_actor_location(); s=a.get_actor_scale3d()
            block('Planter rim / '+name,(p.x,p.y,95),(s.x*100,s.y*100,190),teal)
            block('Brass edge / '+name,(p.x,p.y,193),(s.x*100+5,s.y*100+5,6),brass)
    if a.get_class().get_name()=='PortalLevelExit':
        a.set_editor_property('objective_text','Retrieve the power cube across the gap. F: carry / drop. Power the harvest gate.')
        a.set_editor_property('complete_text','TINY HARVEST | First delivery complete!')
# Fine board joints sit below feet and do not change collision.
for y in range(-700,801,150): block('Board joint',(0,y,.3),(1995,3,.5),dark)
for y in [1390,1540,1690,1840,1990]: block('Vault board joint',(0,y,.3),(1095,3,.5),dark)
for x in [-950,950]:
    for y in range(-680,720,180): mesh('Brass pin','/Engine/BasicShapes/Cylinder',(x,y,1),(10,10,2),material=brass)
soil=mat('M_ForestSoil',(.095,.13,.036))
leaf=mat('M_LeafGreen',(.11,.28,.065))
roof=mat('M_Terracotta',(.36,.07,.025))
block('Forest floor',(0,800,-460),(22000,22000,120),soil)
F='/Game/3Dfruit_Particle/Meshes/'
prop('Giant strawberry / left landmark',F+'SM_strawberry__3d_asse',-1650,1050,-365,1650,25)
prop('Giant lemon / right landmark',F+'SM_lemon__3d_asset_0_g',1800,1950,-365,2050,-20)
prop('Watermelon horizon',F+'SM_watermelon_half__3d',-850,3650,-365,3400,135)
prop('Banana crescent',F+'SM_banana__3d_asset_0_',2700,-1350,-365,2800,45)
prop('Strawberry behind entry',F+'SM_strawberry__3d_asse',-2450,-1250,-365,2100,-35)
prop('Kiwi on table',F+'SM_kiwi__3d_asset_0_gl',1800,3400,-365,1300,15)
prop('Lemon far horizon',F+'SM_lemon__3d_asset_0_g',3300,4800,-365,1900,60)

# A wild clearing and a settlement at the scale of its inhabitants.
import random
rng=random.Random(41)
V='/Game/PLATFORMER_StylizedCubeWorld_Vol1/Models/Environment/Vegetations/'
P='/Game/PLATFORMER_StylizedCubeWorld_Vol1/Models/Props/'
I='/Game/Isometric_World/Sky_Temple/Meshes/'
for i in range(55):
    x,y=rng.uniform(-5200,5200),rng.uniform(-3600,6300)
    if -1250<x<1950 and -1050<y<2380: continue
    prop('Wild grass %02d'%i,V+'SM_Grass_01_A',x,y,-395,rng.uniform(500,1200),rng.uniform(0,360))
for i,(x,y) in enumerate([(-2100,2200),(2350,2800),(-1800,-2100),(2800,-2400),(-3400,1000),(3900,2500),(650,3800)]):
    prop('Giant clover %02d'%i,V+'SM_Plant_01_A',x,y,-390,1800+i*90,i*51)
for i,(x,y) in enumerate([(-5000,500),(-4100,4300),(-1000,6500),(3300,6400),(5400,2500),(-4200,-3200),(4500,-3100)]):
    prop('Forest canopy %02d'%i,I+'Foliage/SM_Foliage_Tree_01',x,y,-390,5200,i*37)
for i,(x,y) in enumerate([(-1450,2700),(1550,3000),(2500,300),(-1500,-800)]):
    prop('Mushroom grove %02d'%i,V+'SM_Mushroom_01_A',x,y,-390,650,i*67)
# Small timber homes on raised mossy islands; doors approx. character height.
for h,(x,y,z) in enumerate([(-1280,2380,10),(1070,2630,-20),(2250,1180,-40)]):
    prop('Village moss island %d'%h,I+'Grounds/SM_Ground_Grass_01',x,y,z-220,1050,h*65)
    block('Cottage plaster %d'%h,(x,y,z+155),(370,330,310),cream)
    for dx in [-175,175]: block('Cottage corner timber',(x+dx,y-173,z+155),(22,20,320),wood)
    block('Cottage timber sill',(x,y-173,z+18),(375,20,25),wood)
    block('Cottage door',(x,y-169,z+98),(95,12,196),teal)
    for dx in [-125,125]:
        block('Cottage window trim',(x+dx,y-171,z+195),(72,16,84),wood)
        block('Warm cottage window',(x+dx,y-181,z+195),(56,4,66),brass)
        block('Window mullion',(x+dx,y-185,z+195),(5,4,66),cream)
    for dx,angle in [(-110,-35),(110,35)]: mesh('Cottage pitched roof',cube,(x+dx,y,z+366),(270,410,24),rot=(angle,0,0),material=roof)
    block('Cottage chimney',(x+105,y+60,z+405),(45,48,150),wood)
    block('Cottage doorstep',(x,y-215,z+5),(150,80,20),wood)
    for j in range(6): block('Footpath stepping plank',(x+j*20,y-300-j*85,z-15),(110,48,12),wood2)
    prop('Residents supply crate',P+'SM_Crate_01',x-260,y-175,z,80,15)
    prop('Residents earthenware','/Game/Fantasy_Props/Small_Vase/Meshes/SM_Small_Vase',x+245,y-180,z,70)
# Nearby signs of everyday life, clear of all tested paths and portal shot lines.
for x,y in [(-710,-610),(-770,-610),(-730,-530),(730,530)]: prop('Harvest supply crate',P+'SM_Crate_01',x,y,1,65,12)
block('Work bench top',(-560,-660,100),(200,65,15),wood2)
for x in [-635,-485]: block('Work bench leg',(x,-660,48),(18,45,96),teal)
prop('Small residents jug','/Game/Fantasy_Props/Jug/Meshes/SM_Jug',-620,-655,108,48)
prop('Small residents cup','/Game/Fantasy_Props/Boar_Cup/Meshes/SM_Boar_Cup_1',-510,-655,108,24)
for x,y in [(-850,690),(850,690),(-450,1380),(450,1940),(-850,-680)]:
    block('Lantern post',(x,y,125),(12,12,250),wood)
    block('Lantern base',(x,y,253),(45,45,12),brass)
    block('Lantern glass',(x,y,280),(30,30,48),cream)
    block('Lantern cap',(x,y,309),(48,48,12),teal)

# Human-scale crafted portal stations. Geometry stays clear of the aperture.
for x,y,axis,w in [(-1008,0,'x',480),(0,2028,'y',1040)]:
    if axis=='x':
        block('Entry station crown',(x,y,515),(55,w+70,30),brass)
        for off in [-w/2-22,w/2+22]: block('Entry station upright',(x,y+off,250),(55,24,500),brass)
    else:
        block('Vault station crown',(x,y,515),(w+60,55,30),brass)
        for off in [-w/2-18,w/2+18]: block('Vault station upright',(x+off,y,250),(22,55,500),brass)
# Gate has a distinct cream and brass frame.
for y in [-145,145]: block('Harvest gate jamb',(986,y,170),(52,20,340),brass)
block('Harvest gate crown',(986,0,352),(52,310,22),brass)
def text(n,t,p,yaw,size=28,color=(235,220,160,255)):
    a=A.spawn_actor_from_class(unreal.TextRenderActor,unreal.Vector(*p),unreal.Rotator(pitch=0,yaw=yaw,roll=0)); a.set_actor_label(n); a.set_folder_path('Tiny Harvest/Wayfinding'); a.text_render.set_text(t); a.text_render.set_world_size(size); a.text_render.set_text_render_color(unreal.Color(*color)); return a
text('Title','TINY HARVEST',(-520,780,455),-90,64)
text('Subtitle','01  /  THE FIRST DELIVERY',(-370,778,392),-90,24)
text('Entry station label','01 / TRANSFER',(-974,-200,445),0,27)
text('Vault station label','02 / POWER SEED',(-410,1988,448),-90,32)
text('Gate label','HARVEST GATE',(954,125,315),180,22)
text('Button instruction','POWER DOCK',(620,-775,230),90,26)
# Sky dome and warm directional light, with broad soft fill for readable portal views.
sky=mesh('Sky','/Engine/BasicShapes/Sphere',(0,0,0),(80000,80000,80000))
sky.static_mesh_component.set_material(0,unreal.load_asset('/Engine/EngineSky/M_Sky_Panning_Clouds2'))
sky.static_mesh_component.set_cast_shadow(False)
sun=A.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,1500),unreal.Rotator(pitch=-48,yaw=-32,roll=0)); sun.light_component.set_editor_property('intensity',3.2); sun.light_component.set_editor_property('light_color',unreal.Color(255,226,183,255))
for p,intensity,col in [((0,300,1400),220,(210,230,255,255)),((0,1700,1000),150,(255,235,200,255)),((-500,-400,850),100,(255,224,178,255)),((1400,0,260),65,(160,240,220,255))]:
    a=A.spawn_actor_from_class(unreal.PointLight,unreal.Vector(*p)); c=a.point_light_component; c.set_editor_property('intensity',intensity); c.set_editor_property('attenuation_radius',6000); c.set_editor_property('cast_shadows',False); c.set_editor_property('light_color',unreal.Color(*col))
# Presentation camera is saved in the map for composing future investor shots.
a=A.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(700,-650,340),unreal.Rotator(pitch=5,yaw=117,roll=0)); a.set_actor_label('Presentation / Tiny Harvest'); a.camera_component.set_editor_property('field_of_view',85)
assert L.save_current_level()
E.save_directory('/Game/TinyHarvest')
unreal.log('TINY_HARVEST_CREATED actors='+str(len(A.get_all_level_actors())))

import math
import unreal

level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
path='/Game/Maps/PortalFoundationLab'
if unreal.EditorAssetLibrary.does_asset_exist(path):
    if '-RebuildFoundation' not in unreal.SystemLibrary.get_command_line():
        raise RuntimeError('Foundation lab exists; pass -RebuildFoundation only to replace this generated fixture')
    assert level.load_level(path)
    for actor in actors.get_all_level_actors():
        if not isinstance(actor,(unreal.WorldSettings,unreal.Brush)):
            actors.destroy_actor(actor)
else:
    assert level.new_level(path)
angle=37.0
def location(p):
    c,s=math.cos(math.radians(angle)),math.sin(math.radians(angle))
    return unreal.Vector(3100+p[0]*c-p[1]*s,-2400+p[0]*s+p[1]*c,400+p[2])
def spawn(cls,p,tag,yaw=0):
    a=actors.spawn_actor_from_class(unreal.load_class(None,'/Script/PortalPrototype.'+cls),location(p),unreal.Rotator(pitch=0,yaw=angle+yaw,roll=0))
    a.set_actor_label(tag)
    a.set_editor_property('tags',[tag])
    return a
def block(p,size,label):
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,location(p),unreal.Rotator(pitch=0,yaw=angle,roll=0))
    a.set_actor_label(label)
    a.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    a.static_mesh_component.set_material(0,unreal.load_asset('/Game/Materials/M_Floor'))
    a.set_actor_scale3d(unreal.Vector(*(x/100 for x in size)))
    return a
block((0,0,-25),(2400,2000,50),'Foundation floor')
block((1000,-550,250),(40,650,500),'Exit partition south')
block((1000,550,250),(40,650,500),'Exit partition north')
block((1000,0,420),(40,450,160),'Exit lintel')
block((1350,0,-25),(700,450,50),'Exit corridor floor')
block((1350,-240,180),(700,30,360),'Corridor side A')
block((1350,240,180),(700,30,360),'Corridor side B')
block((1700,0,180),(30,510,360),'Corridor end')
a=spawn('PortalPressureButton',(-300,-250,12),'InputA')
b=spawn('PortalPressureButton',(300,-250,12),'InputB')
b.set_editor_property('radius',105)
b.set_editor_property('minimum_mass',10)
b.set_actor_location(location((300,-250,12)),False,False)
load_a=spawn('PortalCarryable',(-450,100,55),'LoadA')
load_a.set_editor_property('mass_kg',6)
load_a.get_editor_property('mesh').set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cylinder'))
load_a.set_actor_scale3d(unreal.Vector(.5,.5,.8))
load_b=spawn('PortalCube',(350,150,60),'LoadB')
load_b.set_editor_property('mass_kg',18)
for tag,pos,inputs in [('OnlyA',(-350,550,100),[a]),('OnlyB',(350,550,100),[b]),('Both',(1000,0,150),[a,b])]:
    door=spawn('PortalDoor',pos,tag)
    door.input.set_editor_property('sources',inputs)
    if tag!='Both':
        door.set_editor_property('panel_size',unreal.Vector(30,150,200))
        door.set_editor_property('open_offset',unreal.Vector(0,180,0))
        door.set_editor_property('travel_seconds',.6)
        door.set_actor_location(location(pos),False,False)
exit=spawn('PortalLevelExit',(1480,0,100),'Exit')
exit.input.set_editor_property('sources',[a,b])
exit.set_editor_property('objective_text','Put both weights on their buttons. The larger button requires at least 10 kg. F: carry/drop.')
for tag,p,yaw,width in [('SurfaceA',(-1150,0,260),0,500),('SurfaceB',(0,950,260),-90,800)]:
    wall=spawn('PortalWall',p,tag,yaw)
    wall.set_editor_property('half_width',width)
    wall.set_editor_property('half_height',260)
    wall.set_actor_location(location(p),False,False)
blocked=spawn('PortalWall',(-900,-950,260),'NonPortalSurface',90)
blocked.set_editor_property('half_width',250)
blocked.set_editor_property('portalable',False)
blocked.set_actor_location(location((-900,-950,260)),False,False)
for p in [(0,0,550),(-700,-200,400),(650,-150,400),(1400,0,300)]:
    light=actors.spawn_actor_from_class(unreal.PointLight,location(p))
    light.point_light_component.set_editor_property('intensity',80)
    light.point_light_component.set_editor_property('attenuation_radius',2000)
    light.point_light_component.set_editor_property('cast_shadows',False)
actors.spawn_actor_from_class(unreal.PlayerStart,location((0,-650,100)),unreal.Rotator(pitch=0,yaw=angle+90,roll=0))
assert level.save_current_level()
unreal.log('FOUNDATION_LAB_CREATED')

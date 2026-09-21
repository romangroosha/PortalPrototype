import unreal

level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not level.new_level('/Game/Maps/PortalGunLab'):
    raise RuntimeError('PortalGunLab already exists; refusing to overwrite.')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
cube=unreal.load_asset('/Engine/BasicShapes/Cube')
def block(label,pos,size,material):
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*pos))
    a.set_actor_label(label)
    a.static_mesh_component.set_static_mesh(cube)
    a.set_actor_scale3d(unreal.Vector(*(v/100 for v in size)))
    a.static_mesh_component.set_material(0,unreal.load_asset('/Game/Materials/'+material))
    return a
block('Floor',(0,0,-20),(2300,2100,40),'M_Floor')
wall_class=unreal.load_class(None,'/Script/PortalPrototype.PortalWall')
for name,pos,yaw in [('East',(1000,0,240),180),('West',(-1000,0,240),0),('North',(0,900,240),-90),('South',(0,-900,240),90)]:
    a=actors.spawn_actor_from_class(wall_class,unreal.Vector(*pos),unreal.Rotator(pitch=0,yaw=yaw,roll=0))
    a.set_actor_label('Portal Panel '+name)
for p in [(-970,-870,240),(-970,870,240),(970,-870,240),(970,870,240)]:
    block('Corner',p,(160,180,480),'M_Floor')
block('Blue landmark',(350,400,60),(120,120,120),'M_Blue')
block('Orange landmark',(-350,-400,60),(120,120,120),'M_Orange')
for p in [(0,0,400),(-650,0,350),(650,0,350),(0,-600,350),(0,600,350)]:
    light=actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(*p))
    light.point_light_component.set_editor_property('intensity',50)
    light.point_light_component.set_editor_property('attenuation_radius',1700)
    light.point_light_component.set_editor_property('cast_shadows',False)
actors.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(0,0,100))
assert level.save_current_level()
unreal.log('PORTAL_GUN_LAB_CREATED')

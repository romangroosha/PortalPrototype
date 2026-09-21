import unreal

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
path = '/Game/Maps/PortalPuzzle'
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(path):
    assert level.load_level(path)
    for actor in actors.get_all_level_actors():
        if not isinstance(actor,(unreal.WorldSettings,unreal.Brush)):
            actors.destroy_actor(actor)
else:
    assert level.new_level(path)
shape = unreal.load_asset('/Engine/BasicShapes/Cube')

def block(name, pos, size, mat='M_Floor'):
    a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*pos))
    a.set_actor_label(name)
    a.static_mesh_component.set_static_mesh(shape)
    a.static_mesh_component.set_material(0, unreal.load_asset('/Game/Materials/'+mat))
    a.set_actor_scale3d(unreal.Vector(*(v/100 for v in size)))
    return a

def panel(name,pos,yaw,width=240,height=250):
    a=actors.spawn_actor_from_class(unreal.load_class(None,'/Script/PortalPrototype.PortalWall'),unreal.Vector(*pos),unreal.Rotator(pitch=0,yaw=yaw,roll=0))
    a.set_actor_label(name)
    a.set_editor_property('half_width',width)
    a.set_editor_property('half_height',height)
    # Re-run construction after changing the authored panel dimensions.
    a.set_actor_location(unreal.Vector(*pos),False,False)
    return a

def label(text,pos,yaw=180,size=32):
    a=actors.spawn_actor_from_class(unreal.TextRenderActor,unreal.Vector(*pos),unreal.Rotator(pitch=0,yaw=yaw,roll=0))
    a.text_render.set_text(text)
    a.text_render.set_world_size(size)
    a.text_render.set_text_render_color(unreal.Color(160,220,255,255))
    return a

# Main chamber. Exit on the east; a cube vault is visible across a deep gap north.
block('Main floor',(0,0,-25),(2000,1600,50))
block('West wall',(-1100,0,250),(30,1600,500))
block('South wall',(0,-815,250),(2000,30,500))
block('Ceiling',(0,0,515),(2040,1640,30))
panel('01 Entry portal panel',(-990,0,250),0)
block('East wall left',(1015,-470,250),(30,660,500))
block('East wall right',(1015,470,250),(30,660,500))
block('Exit lintel',(1015,0,405),(30,280,190))
# View opening has bars: the cube and its portal panel are visible, but jumping is blocked.
block('Observation sill',(0,800,35),(2000,35,70))
block('Observation header',(0,800,410),(2000,35,180))
for x in range(-920,1000,160):
    block('Observation bars',(x,800,190),(14,35,260))
block('Vault floor',(0,1670,-25),(1100,700,50))
block('Vault ceiling',(0,1670,515),(1100,700,30))
block('Vault west',(-565,1670,250),(30,730,500))
block('Vault east',(565,1670,250),(30,730,500))
block('Vault back',(0,2130,250),(1100,30,500))
panel('02 Cube vault portal panel',(0,2010,250),-90,520)
# Safety glass across vault front (collision only); gun rays and light can pass.
barrier=block('Vault observation barrier',(0,1320,250),(1100,20,500))
barrier.static_mesh_component.set_visibility(False)
barrier.static_mesh_component.set_collision_profile_name('Custom')
barrier.static_mesh_component.set_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY,unreal.CollisionResponseType.ECR_IGNORE)
block('Button pedestal',(400,-360,4),(200,200,8),'M_Wall')
for x in range(440,930,80):
    block('Button to door indicator',(x,-360,1),(45,8,2),'M_Orange')
block('Exit floor',(1375,0,-25),(750,280,50))
block('Exit north',(1375,155,180),(750,30,360))
block('Exit south',(1375,-155,180),(750,30,360))
block('Exit end',(1760,0,180),(30,340,360),'M_Blue')
block('Exit ceiling',(1375,0,375),(750,340,30))
cube=actors.spawn_actor_from_class(unreal.load_class(None,'/Script/PortalPrototype.PortalCube'),unreal.Vector(0,1650,50))
cube.set_actor_label('Weighted cube - retrieve through portals')
button=actors.spawn_actor_from_class(unreal.load_class(None,'/Script/PortalPrototype.PortalPressureButton'),unreal.Vector(400,-360,12))
door=actors.spawn_actor_from_class(unreal.load_class(None,'/Script/PortalPrototype.PortalDoor'),unreal.Vector(1000,0,150))
exit=actors.spawn_actor_from_class(unreal.load_class(None,'/Script/PortalPrototype.PortalLevelExit'),unreal.Vector(1470,0,90))
door.get_editor_property('input').set_editor_property('sources',[button])
exit.get_editor_property('input').set_editor_property('sources',[button])
exit.set_editor_property('objective_text','Reach the cube using portals. F: pick up / drop. Put the cube on the orange button.')
label('01  /  PORTAL TRANSFER',(-975,-210,375),0,32)
label('02  /  CUBE VAULT',(-440,1990,370),-90,35)
label('EXIT',(990,100,330),180,38)
label('CUBE ON BUTTON',(600,-790,210),90,28)
for pos in [(0,0,410),(-650,0,370),(650,-350,370),(0,1650,380),(0,1390,180),(1430,0,270)]:
    a=actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(*pos))
    a.point_light_component.set_editor_property('intensity',65)
    a.point_light_component.set_editor_property('attenuation_radius',1900)
    a.point_light_component.set_editor_property('cast_shadows',False)
actors.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(0,0,100),unreal.Rotator(pitch=0,yaw=90,roll=0))
assert level.save_current_level()
unreal.log('PORTAL_PUZZLE_CREATED')

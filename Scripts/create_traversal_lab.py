import unreal

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
path = '/Game/Maps/PortalTraversalLab'
if unreal.EditorAssetLibrary.does_asset_exist(path):
    raise RuntimeError('Traversal lab already exists; preserve authored edits')
assert level.new_level(path)

def spawn(name, pos, label, rotation=(0,0,0)):
    actor = actors.spawn_actor_from_class(unreal.load_class(None, '/Script/PortalPrototype.'+name), unreal.Vector(*pos), unreal.Rotator(pitch=rotation[0],yaw=rotation[1],roll=rotation[2]))
    actor.set_actor_label(label)
    actor.set_editor_property('tags', [label])
    return actor

def panel(pos, rotation, width, height, label):
    actor = spawn('PortalWall',pos,label,rotation)
    actor.set_editor_property('half_width',width)
    actor.set_editor_property('half_height',height)
    actor.set_actor_location(unreal.Vector(*pos),False,False)
    return actor

# Each complete surface owns its physical opening. No hidden solid floor behind it.
panel((0,0,0),(90,0,0),1000,1600,'TraversalFloor')
panel((0,0,1000),(-90,0,0),1000,1600,'TraversalCeiling')
panel((1600,0,500),(0,180,0),1000,500,'TraversalEast')
panel((-1600,0,500),(0,0,0),1000,500,'TraversalWest')
panel((0,1000,500),(0,-90,0),1600,500,'TraversalNorth')
panel((0,-1000,500),(0,90,0),1600,500,'TraversalSouth')
panel((100,-550,230),(45,0,0),280,300,'TraversalIncline')
platform=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(1050,0,350))
platform.set_actor_label('Landing platform')
platform.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
platform.static_mesh_component.set_material(0,unreal.load_asset('/Game/Materials/M_Floor'))
platform.set_actor_scale3d(unreal.Vector(8,6,.5))
button=spawn('PortalPressureButton',(1000,0,383),'LandingButton')
spawn('PortalCube',(-700,400,45),'TraversalCube')
exit=spawn('PortalLevelExit',(1250,0,480),'TraversalGoal')
exit.input.set_editor_property('sources',[button])
exit.set_editor_property('objective_text','Traversal lab: portals work on floor, ceiling and sloped panels. Reach the raised platform and bring its button a cube. R: reset.')
for x in (-1000,0,1000):
    for y in (-500,500):
        light=actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(x,y,700))
        light.point_light_component.set_editor_property('intensity',110)
        light.point_light_component.set_editor_property('attenuation_radius',2200)
        light.point_light_component.set_editor_property('cast_shadows',False)
actors.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(-1050,-400,100),unreal.Rotator(pitch=0,yaw=15,roll=0))
assert level.save_current_level()
unreal.log('TRAVERSAL_LAB_CREATED')

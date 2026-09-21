import unreal
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert level.load_level('/Game/Maps/PortalLab')
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor, unreal.PointLight):
        actor.point_light_component.set_editor_property('intensity', 50)
    if actor.get_class().get_name() == 'PortalGate':
        x = actor.get_actor_location().x
        actor.set_actor_rotation(unreal.Rotator(pitch=0, yaw=180 if x > 0 else 0, roll=0), False)
        unreal.log('GATE ' + str(actor.get_actor_transform()) + ' LINK ' + str(actor.get_editor_property('linked')))
assert level.save_current_level()

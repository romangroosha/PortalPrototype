import unreal

level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert level.load_level('/Game/Maps/PortalPuzzle')
legacy=[a for a in actors.get_all_level_actors() if a.get_class().get_name()=='PortalPuzzle']
for old in legacy:
    def spawn(cls,location,label):
        a=actors.spawn_actor_from_class(unreal.load_class(None,'/Script/PortalPrototype.'+cls),location)
        a.set_actor_label(label)
        return a
    button=spawn('PortalPressureButton',old.get_editor_property('button_position'),'Cube pressure button')
    door=spawn('PortalDoor',old.get_editor_property('door_position'),'Exit door - button controlled')
    exit=spawn('PortalLevelExit',old.get_editor_property('finish_position'),'Chamber completion')
    door.get_editor_property('input').set_editor_property('sources',[button])
    exit.get_editor_property('input').set_editor_property('sources',[button])
    exit.set_editor_property('objective_text','Reach the cube using portals. F: pick up / drop. Put the cube on the orange button.')
    actors.destroy_actor(old)
assert level.save_current_level()
unreal.log('MIGRATED_PUZZLE_MECHANISMS count='+str(len(legacy)))

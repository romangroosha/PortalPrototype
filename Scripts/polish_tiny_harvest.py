import unreal
L=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert L.load_level('/Game/Maps/TinyHarvest')
for a in A.get_all_level_actors():
    if isinstance(a,unreal.StaticMeshActor) and str(a.get_folder_path())=='Tiny Harvest/Set dressing':
        a.static_mesh_component.set_collision_profile_name('NoCollision')
        a.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    if a.get_actor_label().startswith('Cottage pitched roof'):
        r=a.get_actor_rotation(); a.set_actor_rotation(unreal.Rotator(pitch=r.roll,yaw=0,roll=0),False)
assert L.save_current_level()
unreal.log('TINY_HARVEST_COLLISION_FIXED')

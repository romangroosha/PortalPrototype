import unreal
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert level.load_level('/Game/Maps/PortalLab')
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    p = actor.get_actor_location()
    if isinstance(actor, unreal.StaticMeshActor) and abs(abs(p.x)-600)<.1:
        if abs(abs(p.y)-390)<.1 and abs(p.z-200)<.1:
            actor.set_actor_location(unreal.Vector(p.x,396 if p.y>0 else -396,p.z),False,False)
            actor.set_actor_scale3d(unreal.Vector(.3,5.88,4))
        if abs(p.y)<.1 and (abs(p.z-340)<.1 or abs(p.z-346)<.1):
            actor.set_actor_location(unreal.Vector(p.x,0,346),False,False)
            actor.set_actor_scale3d(unreal.Vector(.3,2.04,1.08))
assert level.save_current_level()

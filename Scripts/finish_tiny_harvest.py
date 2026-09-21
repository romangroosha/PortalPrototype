import unreal,random
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem); L=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert L.load_level('/Game/Maps/TinyHarvest')
wood=unreal.load_asset('/Game/TinyHarvest/Materials/M_HoneyWood'); brass=unreal.load_asset('/Game/TinyHarvest/Materials/M_Brass'); soil=unreal.load_asset('/Game/TinyHarvest/Materials/M_ForestSoil')
def block(n,p,s,m):
    a=A.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*p)); a.set_actor_label(n); a.set_folder_path('Tiny Harvest/Set dressing'); c=a.static_mesh_component; c.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube')); c.set_material(0,m); c.set_collision_profile_name('NoCollision'); a.set_actor_scale3d(unreal.Vector(*(v/100 for v in s))); return a
for a in list(A.get_all_level_actors()):
    n=a.get_actor_label()
    if a.get_class().get_name()=='PortalWall':
        p=a.get_actor_location(); a.set_editor_property('half_width',170); a.set_editor_property('half_height',170); a.set_actor_location(unreal.Vector(p.x,p.y,170),False,False)
    if n.startswith(('Entry station crown','Entry station upright','Vault station crown','Vault station upright')): A.destroy_actor(a)
    if n.startswith('Observation bars'):
        p=a.get_actor_location(); a.set_actor_location(unreal.Vector(p.x,p.y,95),False,False); a.set_actor_scale3d(unreal.Vector(.1,.22,1.2))
    if n=='Observation header': a.set_actor_location(unreal.Vector(0,800,154),False,False); a.set_actor_scale3d(unreal.Vector(20,.27,.1))
    if n in ['Title','Subtitle']:
        a.set_actor_location(unreal.Vector(-610,775,300 if n=='Title' else 264),False,False); a.text_render.set_world_size(27 if n=='Title' else 13)
    if n=='Vault station label': a.set_actor_location(unreal.Vector(0,1988,310),False,False); a.text_render.set_world_size(20)
    if n=='Entry station label': a.set_actor_location(unreal.Vector(-974,0,310),False,False); a.text_render.set_world_size(20)
    if isinstance(a,unreal.StaticMeshActor):
        c=a.static_mesh_component
        if n.startswith(('Giant','Watermelon','Banana','Kiwi','Lemon')): c.set_editor_property('forced_lod_model',1)
        if n.startswith('Forest canopy'): c.set_cast_shadow(False)
for p,s in [((-1008,0,350),(55,400,22)),((0,2028,350),(400,55,22))]: block('Compact station crown',p,s,brass)
for off in [-186,186]:
    block('Compact entry upright',(-1008,off,170),(55,22,340),brass)
    block('Compact vault upright',(off,2028,170),(22,55,340),brass)
block('Settlement sign board',(-610,797,283),(340,20,104),wood)
for x in [-755,-465]: block('Settlement sign post',(x,807,165),(15,15,330),wood)
# Keep rich ambient fill on foliage and cottage fronts.
a=A.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,1400),unreal.Rotator(pitch=-55,yaw=145,roll=0)); a.set_actor_label('Soft cool forest fill'); a.light_component.set_editor_property('intensity',1.5); a.light_component.set_editor_property('cast_shadows',False); a.light_component.set_editor_property('light_color',unreal.Color(204,229,255,255))
# Organic banks break up the flat ground beneath the surrounding settlement.
rng=random.Random(7)
for i,(x,y,sx,sy) in enumerate([(-1400,1500,900,1700),(1250,2100,1000,1400),(-1100,-1300,1900,900),(2450,800,1300,2200),(0,3100,1700,1000)]):
    a=A.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(x,y,-390)); a.set_actor_label('Mossy earth bank'); a.set_folder_path('Tiny Harvest/Set dressing'); c=a.static_mesh_component; c.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Sphere')); c.set_material(0,soil); c.set_collision_profile_name('NoCollision'); a.set_actor_scale3d(unreal.Vector(sx/100,sy/100,3.8))
# Move the camera to the playable clearing when the map is opened.
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(unreal.Vector(700,-650,340),unreal.Rotator(pitch=5,yaw=117,roll=0))
assert L.save_current_level(); unreal.log('TINY_HARVEST_COMPOSITION_FINAL')

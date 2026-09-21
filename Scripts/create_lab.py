import unreal

assets = unreal.AssetToolsHelpers.get_asset_tools()
edit = unreal.MaterialEditingLibrary
def material(name, color):
    path = '/Game/Materials/' + name
    m = unreal.load_asset(path)
    if m:
        return m
    m = assets.create_asset(name, '/Game/Materials', unreal.Material, unreal.MaterialFactoryNew())
    c = edit.create_material_expression(m, unreal.MaterialExpressionConstant3Vector)
    c.set_editor_property('constant', unreal.LinearColor(*color))
    edit.connect_material_property(c, '', unreal.MaterialProperty.MP_BASE_COLOR)
    edit.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m

portal = unreal.load_asset('/Game/Materials/M_Portal')
if not portal:
    portal = assets.create_asset('M_Portal', '/Game/Materials', unreal.Material, unreal.MaterialFactoryNew())
    portal.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    portal.set_editor_property('two_sided', True)
    tex = edit.create_material_expression(portal, unreal.MaterialExpressionTextureSampleParameter2D)
    tex.set_editor_property('parameter_name', 'PortalTexture')
    tex.set_editor_property('texture', unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture'))
    screen = edit.create_material_expression(portal, unreal.MaterialExpressionScreenPosition)
    edit.connect_material_expressions(screen, 'ViewportUV', tex, 'UVs')
    edit.connect_material_property(tex, 'RGB', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    edit.recompile_material(portal)
    unreal.EditorAssetLibrary.save_loaded_asset(portal)

wall = material('M_Wall', (.55, .6, .65, 1))
floor = material('M_Floor', (.14, .18, .22, 1))
blue = material('M_Blue', (.01, .18, 1, 1))
orange = material('M_Orange', (1, .2, .01, 1))
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not level.new_level('/Game/Maps/PortalLab'):
    raise RuntimeError('PortalLab already exists; refusing to overwrite an authored level.')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
cube = unreal.load_asset('/Engine/BasicShapes/Cube')
def block(name, pos, size, mat):
    a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*pos))
    a.set_actor_label(name)
    a.static_mesh_component.set_static_mesh(cube)
    a.set_actor_scale3d(unreal.Vector(*(s/100 for s in size)))
    a.static_mesh_component.set_material(0, mat)
    return a

block('Floor', (0, 0, -20), (2600, 1800, 40), floor)
# Two wall openings with 180 x 280 clear aperture. No collision disabling is needed.
gate_class = unreal.load_class(None, '/Script/PortalPrototype.PortalGate')
gates = []
for x, yaw, mat in [(600, 180, blue), (-600, 0, orange)]:
    for y in [-396, 396]:
        block('Wall side', (x, y, 200), (30, 588, 400), wall)
    block('Wall header', (x, 0, 346), (30, 204, 108), wall)
    for y in [-96, 96]:
        block('Portal frame', (x, y, 140), (34, 12, 280), mat)
    block('Portal top frame', (x, 0, 286), (34, 204, 12), mat)
    a = actors.spawn_actor_from_class(gate_class, unreal.Vector(x, 0, 140), unreal.Rotator(pitch=0, yaw=yaw, roll=0))
    a.set_actor_label('Blue Portal' if x > 0 else 'Orange Portal')
    gates.append(a)
gates[0].set_editor_property('linked', gates[1])
gates[1].set_editor_property('linked', gates[0])
for pos, size, mat in [((250, 340, 60), (120,120,120), blue), ((-300,-350,90),(160,160,180),orange), ((850,300,50),(100,100,100),orange)]:
    block('Perspective landmark', pos, size, mat)
for x in [-900, 0, 900]:
    light = actors.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x, 0, 350))
    light.point_light_component.set_editor_property('intensity', 50)
    light.point_light_component.set_editor_property('attenuation_radius', 1600)
    light.point_light_component.set_editor_property('cast_shadows', False)
actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 100))
level.save_current_level()
unreal.log('PORTAL_LAB_CREATED')

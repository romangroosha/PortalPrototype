import unreal
from pathlib import Path
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem); L=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem); E=unreal.EditorAssetLibrary; M=unreal.MaterialEditingLibrary; T=unreal.AssetToolsHelpers.get_asset_tools()
assert L.load_level('/Game/Maps/TinyHarvest')
root=Path(r'C:\Unreal Projects\PortalPrototype\Content\3Dfruit_Particle\Texture')
mats={}
for folder in ['strawberry','lemon','watermelon_half','banana','kiwi']:
    m=unreal.load_asset('/Game/TinyHarvest/Materials/M_Fruit_'+folder)
    M.delete_all_material_expressions(m)
    for suffix,prop in [('rgb_',unreal.MaterialProperty.MP_BASE_COLOR),('normals_',unreal.MaterialProperty.MP_NORMAL),('roughness_',unreal.MaterialProperty.MP_ROUGHNESS)]:
        f=next((root/folder).glob('*'+suffix+'.uasset'))
        tex=unreal.load_asset('/Game/3Dfruit_Particle/Texture/'+folder+'/'+f.stem)
        n=M.create_material_expression(m,unreal.MaterialExpressionTextureSample); n.set_editor_property('texture',tex)
        if suffix=='normals_': n.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        elif suffix=='roughness_': n.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
        M.connect_material_property(n,'RGB' if suffix!='roughness_' else 'R',prop)
    M.recompile_material(m); E.save_loaded_asset(m); mats[folder]=m
sky=unreal.load_asset('/Game/TinyHarvest/Materials/M_ClearSky'); M.delete_all_material_expressions(sky); sky.set_editor_property('is_sky',True); sky.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT); sky.set_editor_property('two_sided',True)
c=M.create_material_expression(sky,unreal.MaterialExpressionConstant3Vector); c.set_editor_property('constant',unreal.LinearColor(.24,.48,.72,1)); M.connect_material_property(c,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR); M.recompile_material(sky); E.save_loaded_asset(sky)
for a in A.get_all_level_actors():
    name=a.get_actor_label()
    if isinstance(a,unreal.StaticMeshActor):
        c=a.static_mesh_component; mesh=c.get_editor_property('static_mesh'); mn=mesh.get_name() if mesh else ''
        for folder,m in mats.items():
            if folder in mn: c.set_material(0,m); break
        if name=='Sky': c.set_material(0,sky)
        if name=='Observation header':
            a.set_actor_location(unreal.Vector(0,800,350),False,False); a.set_actor_scale3d(unreal.Vector(20,.35,.18)); c.set_material(0,unreal.load_asset('/Game/TinyHarvest/Materials/M_HoneyWood'))
        if name.startswith('Observation bars'):
            s=a.get_actor_scale3d(); a.set_actor_scale3d(unreal.Vector(.07,s.y,s.z)); c.set_material(0,unreal.load_asset('/Game/TinyHarvest/Materials/M_HoneyWood'))
    if isinstance(a,unreal.TextRenderActor):
        a.text_render.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
        if name=='Title': a.set_actor_location(unreal.Vector(0,778,410),False,False); a.text_render.set_world_size(40)
        if name=='Subtitle': a.set_actor_location(unreal.Vector(0,778,373),False,False); a.text_render.set_world_size(18)
        if name=='Vault station label': a.set_actor_location(unreal.Vector(0,1988,448),False,False)
        if name=='Entry station label': a.set_actor_location(unreal.Vector(-974,0,445),False,False)
# Ambient illumination comes from the actual sky instead of black unlit shadows.
a=next(a for a in A.get_all_level_actors() if isinstance(a,unreal.SkyLight)); a.set_actor_label('Soft forest skylight'); a.light_component.set_editor_property('intensity',1.1); a.light_component.set_editor_property('mobility',unreal.ComponentMobility.MOVABLE); a.light_component.set_editor_property('real_time_capture',True)
assert L.save_current_level(); unreal.log('TINY_HARVEST_LOOKDEV_SAVED')

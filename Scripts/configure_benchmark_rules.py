import unreal

path='/Game/Blueprints/BP_BenchmarkRules'
bp=unreal.load_asset(path)
if not bp:
    factory=unreal.BlueprintFactory()
    factory.set_editor_property('parent_class',unreal.load_class(None,'/Script/PortalPrototype.PortalLabGameMode'))
    bp=unreal.AssetToolsHelpers.get_asset_tools().create_asset('BP_BenchmarkRules','/Game/Blueprints',unreal.Blueprint,factory)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.get_default_object(bp.generated_class()).set_editor_property('enable_portal_gun',False)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp)
level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert level.load_level('/Game/Maps/PortalLab')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',bp.generated_class())
assert level.save_current_level()
unreal.log('BENCHMARK_RULES_CONFIGURED')

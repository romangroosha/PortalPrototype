import unreal

path = '/Game/Maps/LevelMenu'
editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(path):
    editor.load_level(path)
else:
    if not editor.new_level(path):
        raise RuntimeError('Could not create level menu')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property(
    'default_game_mode', unreal.load_class(None, '/Script/PortalPrototype.PortalLevelMenuGameMode'))
if not editor.save_current_level():
    raise RuntimeError('Could not save level menu')
unreal.log('LEVEL_MENU_MAP_CREATED')

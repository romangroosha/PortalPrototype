using UnrealBuildTool;
public class PortalPrototypeEditorTarget : TargetRules {
 public PortalPrototypeEditorTarget(TargetInfo Target) : base(Target) {
  Type = TargetType.Editor; DefaultBuildSettings = BuildSettingsVersion.V6;
  IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
  ExtraModuleNames.Add("PortalPrototype");
 }
}

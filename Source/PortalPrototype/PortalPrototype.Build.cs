using UnrealBuildTool;
public class PortalPrototype : ModuleRules {
 public PortalPrototype(ReadOnlyTargetRules Target) : base(Target) {
  PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
  PublicDependencyModuleNames.AddRange(new string[] {"Core", "CoreUObject", "Engine", "InputCore", "RHI", "PhysicsCore"});
  PrivateDependencyModuleNames.AddRange(new string[] {"Slate", "SlateCore", "AssetRegistry"});
 }
}

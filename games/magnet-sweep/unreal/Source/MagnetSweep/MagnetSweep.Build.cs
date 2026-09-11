using UnrealBuildTool;
public class MagnetSweep : ModuleRules {
 public MagnetSweep(ReadOnlyTargetRules Target) : base(Target) {
  PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
  PrivateIncludePaths.Add(ModuleDirectory);
  PublicDependencyModuleNames.AddRange(new string[]{"Core","CoreUObject","Engine","InputCore","Json","JsonUtilities"});
  PrivateDependencyModuleNames.AddRange(new string[]{"Slate","SlateCore","RenderCore","RHI","ApplicationCore"});
 }
}

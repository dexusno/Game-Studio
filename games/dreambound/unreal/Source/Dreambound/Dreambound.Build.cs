using UnrealBuildTool;
public class Dreambound : ModuleRules {
 public Dreambound(ReadOnlyTargetRules Target) : base(Target) {
 PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
 PublicDependencyModuleNames.AddRange(new string[]{"Core","CoreUObject","Engine","InputCore","Json","JsonUtilities","AudioMixer"});
 PrivateDependencyModuleNames.AddRange(new string[]{"Slate","SlateCore","RenderCore","RHI"});
 }
}

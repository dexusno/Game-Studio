using UnrealBuildTool;
public class DreamboundTarget : TargetRules {
 public DreamboundTarget(TargetInfo Target) : base(Target) {
 Type=TargetType.Game; DefaultBuildSettings=BuildSettingsVersion.Latest;
 IncludeOrderVersion=EngineIncludeOrderVersion.Latest; ExtraModuleNames.Add("Dreambound");
 }
}

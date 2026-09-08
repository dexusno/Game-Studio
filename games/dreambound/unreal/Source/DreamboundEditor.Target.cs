using UnrealBuildTool;
public class DreamboundEditorTarget : TargetRules {
 public DreamboundEditorTarget(TargetInfo Target) : base(Target) {
 Type=TargetType.Editor; DefaultBuildSettings=BuildSettingsVersion.Latest;
 IncludeOrderVersion=EngineIncludeOrderVersion.Latest; ExtraModuleNames.Add("Dreambound");
 }
}

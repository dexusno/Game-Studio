using UnrealBuildTool;
public class MagnetSweepEditorTarget : TargetRules {
 public MagnetSweepEditorTarget(TargetInfo Target) : base(Target) {
  Type=TargetType.Editor; DefaultBuildSettings=BuildSettingsVersion.Latest;
  IncludeOrderVersion=EngineIncludeOrderVersion.Latest; ExtraModuleNames.Add("MagnetSweep");
 }
}

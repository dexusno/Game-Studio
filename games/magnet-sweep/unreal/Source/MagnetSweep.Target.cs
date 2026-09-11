using UnrealBuildTool;
public class MagnetSweepTarget : TargetRules {
 public MagnetSweepTarget(TargetInfo Target) : base(Target) {
  Type=TargetType.Game; DefaultBuildSettings=BuildSettingsVersion.Latest;
  IncludeOrderVersion=EngineIncludeOrderVersion.Latest; ExtraModuleNames.Add("MagnetSweep");
 }
}

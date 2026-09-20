using UnrealBuildTool;

public class OverkillFoundryEditorTarget : TargetRules
{
    public OverkillFoundryEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("OverkillFoundry");
    }
}

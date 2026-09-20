using UnrealBuildTool;
using System.IO;

public class OverkillFoundry : ModuleRules
{
    public OverkillFoundry(ReadOnlyTargetRules Target) : base(Target)
    {
        // The two shared-core translation units must compile without Unreal's
        // PCH/macros and without unity-combining their anonymous namespaces.
        PCHUsage = PCHUsageMode.NoPCHs;
        bUseUnity = false;
        bEnableExceptions = true;
        PublicIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../core/include")));
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore" });
    }
}

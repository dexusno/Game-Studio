using UnrealBuildTool;
using System.IO;
using System;
using System.Linq;
using System.Security.Cryptography;
using System.Text;

public class OverkillFoundry : ModuleRules
{
    public OverkillFoundry(ReadOnlyTargetRules Target) : base(Target)
    {
        // The shared-core translation units must compile without Unreal's
        // PCH/macros and without unity-combining their anonymous namespaces.
        PCHUsage = PCHUsageMode.NoPCHs;
        bUseUnity = false;
        bEnableExceptions = true;
        // An explicit local review snapshot permits visual work while another
        // worker edits the live core. Normal builds always use the shared core.
        string ReviewRoot = Environment.GetEnvironmentVariable("FOUNDRY_CORE_ROOT");
        string CoreRoot = Path.GetFullPath(string.IsNullOrWhiteSpace(ReviewRoot)
            ? Path.Combine(ModuleDirectory, "../../../core") : ReviewRoot);
        ExternalDependencies.Add(Path.GetFullPath(Path.Combine(ModuleDirectory, "../../Saved/BuildCoreRoot.txt")));
        PublicIncludePaths.Add(Path.Combine(CoreRoot, "include"));
        bool WithCampaign = File.Exists(Path.Combine(CoreRoot, "src/upgrades.cpp"));
        PublicDefinitions.Add("FOUNDRY_WITH_CAMPAIGN=" + (WithCampaign ? "1" : "0"));
        var Units = new[] { "core", "serialization", "catalogue", "robots" }.AsEnumerable();
        if (WithCampaign) Units = Units.Concat(new[] { "upgrades", "route", "city_content", "campaign", "campaign_upgrades", "campaign_serialization" });
        foreach (string Unit in Units)
        {
            string FileName = Path.Combine(CoreRoot, "src", Unit + ".cpp");
            if (!File.Exists(FileName)) throw new BuildException("Missing shared core unit: " + FileName);
            PrivateDefinitions.Add("FOUNDRY_CORE_" + Unit.ToUpperInvariant() + "=\"" + FileName.Replace('\\', '/') + "\"");
        }
        if (WithCampaign)
        {
            string PlatformRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../platform"));
            PublicIncludePaths.Add(Path.Combine(PlatformRoot, "include"));
            foreach (string Unit in new[] { "windows_save_store", "campaign_session" })
                PrivateDefinitions.Add("FOUNDRY_PLATFORM_" + Unit.ToUpperInvariant() + "=\"" + Path.Combine(PlatformRoot, "src", Unit + ".cpp").Replace('\\', '/') + "\"");
            foreach (string FileName in Directory.GetFiles(PlatformRoot, "*", SearchOption.AllDirectories).Where(p => p.EndsWith(".cpp") || p.EndsWith(".hpp"))) ExternalDependencies.Add(FileName);
            PublicSystemLibraries.Add("bcrypt.lib");
        }
        var SourceFiles = Directory.GetFiles(Path.Combine(CoreRoot, "src"), "*", SearchOption.AllDirectories)
            .Concat(Directory.GetFiles(Path.Combine(CoreRoot, "include"), "*", SearchOption.AllDirectories))
            .OrderBy(FileName => Path.GetRelativePath(CoreRoot, FileName), StringComparer.Ordinal);
        using (var DigestInput = new MemoryStream())
        {
            foreach (string FileName in SourceFiles)
            {
                byte[] Name = Encoding.UTF8.GetBytes(Path.GetRelativePath(CoreRoot, FileName).Replace('\\', '/') + "\n");
                DigestInput.Write(Name, 0, Name.Length);
                byte[] Bytes = File.ReadAllBytes(FileName);
                DigestInput.Write(Bytes, 0, Bytes.Length);
                ExternalDependencies.Add(FileName);
            }
            string Digest = Convert.ToHexString(SHA256.HashData(DigestInput.ToArray())).ToLowerInvariant();
            PublicDefinitions.Add("FOUNDRY_CORE_SOURCE_DIGEST=\"" + Digest + "\"");
            PublicDefinitions.Add("FOUNDRY_CORE_REVIEW_SNAPSHOT=" + (string.IsNullOrWhiteSpace(ReviewRoot) ? "0" : "1"));
            Console.WriteLine("Foundry core source: " + CoreRoot + " digest=" + Digest + " review_snapshot=" + !string.IsNullOrWhiteSpace(ReviewRoot));
        }
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "Slate", "SlateCore", "AudioMixer" });
    }
}

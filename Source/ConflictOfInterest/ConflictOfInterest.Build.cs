using UnrealBuildTool;
public class ConflictOfInterest : ModuleRules {
 public ConflictOfInterest(ReadOnlyTargetRules Target) : base(Target) {
 PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
 PrivateIncludePaths.Add(ModuleDirectory);
 PublicDependencyModuleNames.AddRange(new[]{"Core","CoreUObject","Engine","InputCore"});
 }
}

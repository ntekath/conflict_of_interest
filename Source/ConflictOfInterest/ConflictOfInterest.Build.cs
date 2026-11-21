using UnrealBuildTool;
public class ConflictOfInterest : ModuleRules {
 public ConflictOfInterest(ReadOnlyTargetRules Target) : base(Target) {
 PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
 PrivateIncludePaths.Add(ModuleDirectory);
 PublicDependencyModuleNames.AddRange(new[]{"Core","CoreUObject","Engine","InputCore","Json","UMG","Slate","SlateCore","RenderCore","RHI"});
 }
}
// AP04: pure-data simulation and world-owned runtime bridge.

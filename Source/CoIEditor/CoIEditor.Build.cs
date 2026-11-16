using UnrealBuildTool;
public class CoIEditor : ModuleRules {
 public CoIEditor(ReadOnlyTargetRules Target) : base(Target) {
 PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
 PrivateIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory,"../ConflictOfInterest"));
 PrivateDependencyModuleNames.AddRange(new[]{"Core","CoreUObject","Engine","UnrealEd","EngineSettings","ConflictOfInterest","AssetRegistry","MeshDescription","StaticMeshDescription","Json"});
 }
}

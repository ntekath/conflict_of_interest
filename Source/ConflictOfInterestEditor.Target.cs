using UnrealBuildTool;
public class ConflictOfInterestEditorTarget : TargetRules {
 public ConflictOfInterestEditorTarget(TargetInfo Target) : base(Target) {
 Type=TargetType.Editor;DefaultBuildSettings=BuildSettingsVersion.V7;
 IncludeOrderVersion=EngineIncludeOrderVersion.Unreal5_8;
 ExtraModuleNames.Add("ConflictOfInterest");
 }
}

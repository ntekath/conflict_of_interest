using UnrealBuildTool;
public class ConflictOfInterestTarget : TargetRules {
 public ConflictOfInterestTarget(TargetInfo Target) : base(Target) {
 Type=TargetType.Game;DefaultBuildSettings=BuildSettingsVersion.V7;
 IncludeOrderVersion=EngineIncludeOrderVersion.Unreal5_8;
 ExtraModuleNames.Add("ConflictOfInterest");
 }
}

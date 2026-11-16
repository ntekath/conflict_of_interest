#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "CameraMath.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoICameraTest,"CoI.Camera.BoundsAndAnchor",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCoICameraTest::RunTest(const FString& Parameters)
{
 TestEqual(TEXT("Minimum zoom"),CoICamera::Zoom(17000,1000,600,22000),600.);
 TestEqual(TEXT("Maximum zoom"),CoICamera::Zoom(17000,-1000,600,22000),22000.);
 auto P=CoICamera::ClampFocus({-10000,10000,80},{-6500,-6500},{6500,6500});
 TestTrue(TEXT("All focus boundaries and plane"),P.Equals(FVector(-6500,6500,0)));
 FVector A(1000,2000,0),F(0,0,0);auto New=CoICamera::AnchorFocus(F,A,17000,8500);
 TestTrue(TEXT("Cursor remains at same normalized camera position"),((A-F)/17000).Equals((A-New)/8500));
 TestTrue(TEXT("Diagonal movement normalized"),FMath::IsNearlyEqual(FVector2D(1,1).GetClampedToMaxSize(1).Length(),1.));
 return true;
}
#endif

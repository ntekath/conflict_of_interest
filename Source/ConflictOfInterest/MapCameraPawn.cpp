#include "MapCameraPawn.h"
#include "CameraMath.h"
#include "CoIGameMode.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
AMapCameraPawn::AMapCameraPawn()
{
 PrimaryActorTick.bCanEverTick=true;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
 Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));Camera->SetupAttachment(RootComponent);
 Camera->SetRelativeRotation(FRotator(-90,0,0));Camera->SetRelativeLocation(FVector(0,0,Height));Camera->SetFieldOfView(55);
 Camera->PostProcessSettings.bOverride_AutoExposureMethod=true;Camera->PostProcessSettings.AutoExposureMethod=AEM_Manual;
 Camera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;
 Camera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
}
void AMapCameraPawn::BeginPlay()
{Super::BeginPlay();SetActorLocationAndRotation(InitialFocus,FRotator::ZeroRotator);Height=InitialZoom;Camera->SetRelativeLocation(FVector(0,0,Height));}
void AMapCameraPawn::SetupPlayerInputComponent(UInputComponent* Input)
{
 Super::SetupPlayerInputComponent(Input);Input->BindAxis(TEXT("MapNorth"),this,&AMapCameraPawn::North);
 Input->BindAxis(TEXT("MapEast"),this,&AMapCameraPawn::East);Input->BindAxis(TEXT("MapZoom"),this,&AMapCameraPawn::Zoom);
}
void AMapCameraPawn::North(float Value){Movement.X=Value;}
void AMapCameraPawn::East(float Value){Movement.Y=Value;}
void AMapCameraPawn::Tick(float Dt)
{Super::Tick(Dt);if(auto* PC=Cast<ACoIPlayerController>(GetController());PC&&PC->IsMapKeyboardBlocked())return;auto M=Movement.GetClampedToMaxSize(1);SetActorLocation(CoICamera::ClampFocus(GetActorLocation()+FVector(M.X,M.Y,0)*Height*.65*Dt,Minimum,Maximum));}
bool AMapCameraPawn::CursorPlane(FVector& Point) const
{
 auto* PC=Cast<APlayerController>(GetController());FVector O,D;
 if(!PC||!PC->DeprojectMousePositionToWorld(O,D)||D.Z>=-1.e-6)return false;
 Point=O+D*(-O.Z/D.Z);return true;
}
void AMapCameraPawn::Zoom(float Value)
{
 if(FMath::IsNearlyZero(Value))return;
 if(auto* PC=Cast<ACoIPlayerController>(GetController());PC&&PC->IsMapPointerBlocked())return;FVector Anchor;bool Anchored=CursorPlane(Anchor);double Old=Height;
 Height=CoICamera::Zoom(Height,Value,MinimumZoom,MaximumZoom);
 if(Anchored)SetActorLocation(CoICamera::AnchorFocus(GetActorLocation(),Anchor,Old,Height));
 SetActorLocation(CoICamera::ClampFocus(GetActorLocation(),Minimum,Maximum));Camera->SetRelativeLocation(FVector(0,0,Height));
}
void AMapCameraPawn::SetMapBounds(FVector2D Min,FVector2D Max,FVector Center,double OverviewHeight)
{Minimum=Min;Maximum=Max;InitialFocus=Center;InitialZoom=FMath::Clamp(OverviewHeight,MinimumZoom,MaximumZoom);Height=InitialZoom;SetActorLocation(CoICamera::ClampFocus(Center,Min,Max));Camera->SetRelativeLocation(FVector(0,0,Height));}

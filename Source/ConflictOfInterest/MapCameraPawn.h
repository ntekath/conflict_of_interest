#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MapCameraPawn.generated.h"
class UCameraComponent;
UCLASS()
class CONFLICTOFINTEREST_API AMapCameraPawn : public APawn
{
 GENERATED_BODY()
public:
 AMapCameraPawn();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
 UPROPERTY(EditAnywhere,Category="Map Camera") FVector2D Minimum=FVector2D(-6500,-6500);
 UPROPERTY(EditAnywhere,Category="Map Camera") FVector2D Maximum=FVector2D(6500,6500);
 UPROPERTY(EditAnywhere,Category="Map Camera") double MinimumZoom=600;
 UPROPERTY(EditAnywhere,Category="Map Camera") double MaximumZoom=22000;
 UPROPERTY(EditAnywhere,Category="Map Camera") double InitialZoom=17000;
 double GetZoomHeight() const{return Height;}
 void SetMapBounds(FVector2D Min,FVector2D Max,FVector Center,double OverviewHeight);
private:
 UPROPERTY() TObjectPtr<UCameraComponent> Camera;
 FVector2D Movement=FVector2D::ZeroVector;
 double Height=17000;
 void North(float Value);void East(float Value);void Zoom(float Value);
 bool CursorPlane(FVector& Point) const;
};

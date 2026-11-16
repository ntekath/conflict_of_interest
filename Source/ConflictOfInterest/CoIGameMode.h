#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "CoIGameMode.generated.h"
UCLASS()
class CONFLICTOFINTEREST_API ACoIPlayerController : public APlayerController
{
 GENERATED_BODY()
public: ACoIPlayerController();
 bool IsMapPointerBlocked() const;
 UFUNCTION(Exec) void MapStatus();
private: UPROPERTY() TObjectPtr<class UCoIMapInterface> MapInterface;
protected: virtual void BeginPlay() override;
};
UCLASS()
class CONFLICTOFINTEREST_API ACoIGameMode : public AGameModeBase
{
 GENERATED_BODY()
public: ACoIGameMode();
 virtual void StartPlay() override;
 virtual void HandleStartingNewPlayer_Implementation(APlayerController* Player) override;
private: void ApplyMapCamera();
};

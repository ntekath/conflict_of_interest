#pragma once
#include "CoreMinimal.h"
#include "Simulation/DeveloperBuild.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "CoIGameMode.generated.h"
UCLASS()
class CONFLICTOFINTEREST_API ACoIPlayerController : public APlayerController
{
 GENERATED_BODY()
public: ACoIPlayerController();
 bool IsMapPointerBlocked() const;bool IsMapKeyboardBlocked() const;void ClosePlayerView();
 UFUNCTION(Exec) void MapStatus();
 UFUNCTION(Exec) void SimStatus();
#if COI_WITH_DEV_CONSOLE
 void ToggleDeveloperConsole();
 TSharedPtr<class SCoIDeveloperConsole> DeveloperWidget;FKey DeveloperKey;bool DeveloperOpen=false;
#endif
private: UPROPERTY() TObjectPtr<class UCoIMapInterface> MapInterface;
protected: virtual void EndPlay(const EEndPlayReason::Type Reason) override;virtual void BeginPlay() override;virtual void SetupInputComponent() override;
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

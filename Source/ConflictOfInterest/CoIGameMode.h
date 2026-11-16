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
protected: virtual void BeginPlay() override;
};
UCLASS()
class CONFLICTOFINTEREST_API ACoIGameMode : public AGameModeBase
{
 GENERATED_BODY()
public: ACoIGameMode();
};

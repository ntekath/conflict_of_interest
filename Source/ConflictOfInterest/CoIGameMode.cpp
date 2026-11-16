#include "CoIGameMode.h"
#include "MapCameraPawn.h"
ACoIPlayerController::ACoIPlayerController(){bShowMouseCursor=true;}
void ACoIPlayerController::BeginPlay()
{Super::BeginPlay();FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);}
ACoIGameMode::ACoIGameMode(){DefaultPawnClass=AMapCameraPawn::StaticClass();PlayerControllerClass=ACoIPlayerController::StaticClass();}

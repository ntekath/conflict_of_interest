#include "CoIGameMode.h"
#include "MapCameraPawn.h"
ACoIPlayerController::ACoIPlayerController(){bShowMouseCursor=true;}
void ACoIPlayerController::BeginPlay()
{Super::BeginPlay();FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);}
ACoIGameMode::ACoIGameMode(){DefaultPawnClass=AMapCameraPawn::StaticClass();PlayerControllerClass=ACoIPlayerController::StaticClass();}

#include "GeoCatalog.h"
#include "GeoMapData.h"
#include "EngineUtils.h"
void ACoIGameMode::StartPlay(){Super::StartPlay();ApplyMapCamera();}
void ACoIGameMode::HandleStartingNewPlayer_Implementation(APlayerController* Player)
{Super::HandleStartingNewPlayer_Implementation(Player);ApplyMapCamera();}
void ACoIGameMode::ApplyMapCamera()
{
 for(TActorIterator<ACoIGeoReference> It(GetWorld());It;++It)
 {
  if(!It->Catalog)return;FCoIGeoMapData Data;FString Error;
  if(!Data.Load(It->Catalog->DefinitionJson,Error)){UE_LOG(LogTemp,Error,TEXT("COI_GEO_ERROR %s"),*Error);return;}
  for(TActorIterator<AMapCameraPawn> Pawn(GetWorld());Pawn;++Pawn)
   Pawn->SetMapBounds(Data.Minimum,Data.Maximum,FVector(Data.Center.X,Data.Center.Y,0),Data.OverviewHeight);
  UE_LOG(LogTemp,Display,TEXT("COI_SCENARIO %s start=%s cities=%d"),*Data.ScenarioId,*Data.StartDate,Data.Cities.Num());return;
 }
}

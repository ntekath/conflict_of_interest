#include "CoIGameMode.h"
#include "MapCameraPawn.h"
#include "Components/InputComponent.h"
#include "MapInterface.h"
#include "Simulation/SimulationSubsystem.h"
#if COI_WITH_DEV_CONSOLE
#include "UI/DeveloperConsoleWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/GameViewportClient.h"
#include "Misc/ConfigCacheIni.h"
#endif
ACoIPlayerController::ACoIPlayerController(){bShowMouseCursor=true;}
void ACoIPlayerController::BeginPlay()
{Super::BeginPlay();FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);
 MapInterface=CreateWidget<UCoIMapInterface>(this,UCoIMapInterface::StaticClass());MapInterface->AddToViewport();}
bool ACoIPlayerController::IsMapPointerBlocked() const{
#if COI_WITH_DEV_CONSOLE
 if(DeveloperOpen)return true;
#endif
 return MapInterface&&MapInterface->IsPointerOverPanel();}
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

void ACoIPlayerController::MapStatus()
{
 auto* CameraPawn=Cast<AMapCameraPawn>(GetPawn());
 UE_LOG(LogTemp,Display,TEXT("COI_MAP_STATUS selection=%s zoom=%.3f blocked=%d"),MapInterface?*MapInterface->GetSelectedId():TEXT("none"),CameraPawn?CameraPawn->GetZoomHeight():0.,IsMapPointerBlocked());
}

void ACoIPlayerController::SimStatus()
{
 if(auto* Sim=GetWorld()->GetSubsystem<UCoISimulationSubsystem>())UE_LOG(LogTemp,Display,TEXT("COI_SIM_STATUS step=%lld paused=%d speed=%d hash=%s"),Sim->GetSnapshot().Step,Sim->IsPaused(),Sim->GetSpeed(),*Sim->Fingerprint());
}


void ACoIPlayerController::SetupInputComponent(){Super::SetupInputComponent();InputComponent->BindKey(EKeys::Escape,IE_Pressed,this,&ACoIPlayerController::ClosePlayerView);
#if COI_WITH_DEV_CONSOLE
 FString Key=TEXT("F10");GConfig->GetString(TEXT("CoIDeveloperConsole"),TEXT("ToggleKey"),Key,GGameIni);DeveloperKey=FKey(*Key);if(!DeveloperKey.IsValid())DeveloperKey=EKeys::F10;InputComponent->BindKey(DeveloperKey,IE_Pressed,this,&ACoIPlayerController::ToggleDeveloperConsole);
#endif
}
void ACoIPlayerController::ClosePlayerView(){if(MapInterface)MapInterface->HandleEscape();}
bool ACoIPlayerController::IsMapKeyboardBlocked() const{
#if COI_WITH_DEV_CONSOLE
 if(DeveloperOpen)return true;
#endif
 return MapInterface&&MapInterface->IsKeyboardBlocked();}

#if COI_WITH_DEV_CONSOLE
void ACoIPlayerController::ToggleDeveloperConsole(){
 if(!GetWorld()||!GetWorld()->GetGameViewport())return;
 if(DeveloperOpen){GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(DeveloperWidget.ToSharedRef());DeveloperOpen=false;DeveloperWidget.Reset();FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);SetInputMode(Mode);FSlateApplication::Get().SetAllUserFocusToGameViewport();return;}
 auto* Sim=GetWorld()->GetSubsystem<UCoISimulationSubsystem>();if(!Sim)return;
 SAssignNew(DeveloperWidget,SCoIDeveloperConsole).Sim(Sim).ToggleKey(DeveloperKey).OnClose(FSimpleDelegate::CreateUObject(this,&ACoIPlayerController::ToggleDeveloperConsole));DeveloperOpen=true;GetWorld()->GetGameViewport()->AddViewportWidgetContent(DeveloperWidget.ToSharedRef(),1000);FInputModeUIOnly Mode;Mode.SetWidgetToFocus(DeveloperWidget->FocusTarget());Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);DeveloperWidget->FocusInput();
}
#endif

void ACoIPlayerController::EndPlay(const EEndPlayReason::Type Reason){
#if COI_WITH_DEV_CONSOLE
 if(DeveloperWidget.IsValid()&&GetWorld()&&GetWorld()->GetGameViewport())GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(DeveloperWidget.ToSharedRef());DeveloperWidget.Reset();DeveloperOpen=false;
#endif
 Super::EndPlay(Reason);
}

#include "SimulationSubsystem.h"
#include "GeoCatalog.h"
#include "GeoMapData.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
void UCoISimulationSubsystem::OnWorldBeginPlay(UWorld& World)
 {Super::OnWorldBeginPlay(World);Restart();}
bool UCoISimulationSubsystem::Restart()
{
 check(IsInGameThread());Paused=true;FCoIGeoMapData Geo;bool Found=false;
 for(TActorIterator<ACoIGeoReference> It(GetWorld());It;++It)if(It->Catalog){Found=Geo.Load(It->Catalog->DefinitionJson,LoadError);break;}
 if(!Found){Stop();LoadError=TEXT("Kein gültiger Geokatalog für die Simulation.");OnChanged.Broadcast();return false;}
 FString Json;
 if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("Simulation/scenario.coisim"))))
 {LoadError=TEXT("Content/Simulation/scenario.coisim fehlt.");OnChanged.Broadcast();return false;}
 if(!Simulation.Load(Json,Geo,LoadError)){UE_LOG(LogTemp,Error,TEXT("COI_SIM_LOAD %s"),*LoadError);OnChanged.Broadcast();return false;}
 Paused=true;Speed=1;FractionalMicros=0;NotifySeconds=0;GameThreadMicros=0;
 UE_LOG(LogTemp,Display,TEXT("COI_SIM_READY generation=%llu seed=%lld population=%lld player_cents=%lld"),Simulation.GetGeneration(),Simulation.GetScenario().Settings.Seed,GetSnapshot().Population,GetSnapshot().PlayerMoneyCents);
 OnChanged.Broadcast();return true;
}
void UCoISimulationSubsystem::Stop()
 {check(IsInGameThread());Simulation.Shutdown();Paused=true;FractionalMicros=0;NotifySeconds=0;}
void UCoISimulationSubsystem::OnWorldEndPlay(UWorld& World)
 {Stop();OnChanged.Clear();Super::OnWorldEndPlay(World);}
void UCoISimulationSubsystem::Deinitialize()
 {Stop();OnChanged.Clear();Super::Deinitialize();}
void UCoISimulationSubsystem::Tick(float Dt)
{
 check(IsInGameThread());TRACE_CPUPROFILER_EVENT_SCOPE(CoI_GameThreadBridge);const double Begin=FPlatformTime::Seconds();
 if(FMath::IsFinite(Dt)&&Dt>0)
 {
  // Convert engine float delta exactly once at the boundary; retain fractional microseconds.
  double Micros=(double)Dt*1000000.+FractionalMicros;int64 Whole=(int64)FMath::FloorToDouble(Micros);FractionalMicros=Micros-Whole;
  Simulation.Advance(Whole,Speed,Paused);NotifySeconds+=Dt;
  if(NotifySeconds>=.25){NotifySeconds=0;OnChanged.Broadcast();}
 }
 GameThreadMicros=(FPlatformTime::Seconds()-Begin)*1000000.;
}
void UCoISimulationSubsystem::TogglePause()
 {check(IsInGameThread());if(!IsReady())return;Paused=!Paused;UE_LOG(LogTemp,Display,TEXT("COI_SIM_CONTROL paused=%d speed=%d step=%lld"),Paused,Speed,GetSnapshot().Step);OnChanged.Broadcast();}
void UCoISimulationSubsystem::SetSpeed(int32 Value)
 {check(IsInGameThread());if(Value!=1&&Value!=4&&Value!=16)return;Speed=Value;UE_LOG(LogTemp,Display,TEXT("COI_SIM_CONTROL paused=%d speed=%d step=%lld"),Paused,Speed,GetSnapshot().Step);OnChanged.Broadcast();}
bool UCoISimulationSubsystem::Submit(CoISim::FCommand Command,FString& Error)
 {check(IsInGameThread());return Simulation.Enqueue(MoveTemp(Command),Error);}

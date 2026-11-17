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
 if(!Found){Stop();LoadError=TEXT("Kein gÃ¼ltiger Geokatalog fÃ¼r die Simulation.");OnChanged.Broadcast();return false;}
 FString Json;
 if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("Simulation/scenario.coisim"))))
 {LoadError=TEXT("Content/Simulation/scenario.coisim fehlt.");OnChanged.Broadcast();return false;}
 if(!Simulation.Load(Json,Geo,LoadError)){UE_LOG(LogTemp,Error,TEXT("COI_SIM_LOAD %s"),*LoadError);OnChanged.Broadcast();return false;}
 LastPlayerToken=0;Paused=true;Speed=1;FractionalMicros=0;NotifySeconds=0;GameThreadMicros=0;
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

bool UCoISimulationSubsystem::SubmitPlayer(CoISim::FCommand C,int64 Token,int64& Ticket,FString& Error) {
 check(IsInGameThread());if(!IsReady()){Error=TEXT("Simulation ist nicht bereit.");return false;}
 if(Token<=LastPlayerToken){Error=TEXT("Diese Bestätigung wurde bereits gesendet.");return false;}
 if(C.Generation!=Simulation.GetGeneration()){Error=TEXT("Szenario gewechselt. Neue Vorschau öffnen.");return false;}
 C.Sequence=Simulation.NextSequence();C.TargetStep=GetSnapshot().Step+1;
 const FString Actor=GetSnapshot().Player.ActorId;
 if(C.Kind==CoISim::ECommandKind::PlaceOrder){C.Order.ActorId=Actor;C.Order.Id=FString::Printf(TEXT("player.order.%lld"),C.Sequence);}
 else if(C.Kind==CoISim::ECommandKind::CancelOrder)C.Order.ActorId=Actor;
 else if(C.Kind==CoISim::ECommandKind::BookTransport){C.Transport.OwnerId=Actor;C.Transport.Id=FString::Printf(TEXT("player.haul.%lld"),C.Sequence);}
 else if(C.Kind==CoISim::ECommandKind::CancelTransport)C.Transport.OwnerId=Actor;
 else {Error=TEXT("Dieser Befehl ist keine Spieleraktion.");return false;}
 Ticket=C.Sequence;if(!Simulation.Enqueue(MoveTemp(C),Error)){Error=CoISim::PlayerReason(Error);return false;}
 LastPlayerToken=Token;OnChanged.Broadcast();return true;
}

#include "SimulationSubsystem.h"
#include "GeoCatalog.h"
#include "GeoMapData.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#if COI_WITH_DEV_CONSOLE
#include "Serialization/JsonSerializer.h"
#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"
#endif
void UCoISimulationSubsystem::OnWorldBeginPlay(UWorld& World)
 {Super::OnWorldBeginPlay(World);Restart();}
bool UCoISimulationSubsystem::Restart()
{
 check(IsInGameThread());
#if COI_WITH_DEV_CONSOLE
 Dev.Reset(Simulation);
#endif
 Paused=true;FCoIGeoMapData Geo;bool Found=false;
 for(TActorIterator<ACoIGeoReference> It(GetWorld());It;++It)if(It->Catalog){Found=Geo.Load(It->Catalog->DefinitionJson,LoadError);break;}
 if(!Found){Stop();LoadError=TEXT("Kein gÃ¼ltiger Geokatalog fÃ¼r die Simulation.");PublishPresentation();return false;}
 FString Json;
 if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("Simulation/scenario.coisim"))))
 {LoadError=TEXT("Content/Simulation/scenario.coisim fehlt.");PublishPresentation();return false;}
 if(!Simulation.Load(Json,Geo,LoadError)){UE_LOG(LogTemp,Error,TEXT("COI_SIM_LOAD %s"),*LoadError);PublishPresentation();return false;}
 LastPlayerToken=0;Paused=true;Speed=1;FractionalMicros=0;NotifySeconds=0;GameThreadMicros=0;
 UE_LOG(LogTemp,Display,TEXT("COI_SIM_READY generation=%llu seed=%lld population=%lld player_cents=%lld"),Simulation.GetGeneration(),Simulation.GetScenario().Settings.Seed,Simulation.GetSnapshot().Population,Simulation.GetSnapshot().PlayerMoneyCents);
#if COI_WITH_DEV_CONSOLE
 InitializeDeveloper();
#endif
 PublishPresentation();return true;
}
void UCoISimulationSubsystem::Stop()
 {check(IsInGameThread());
#if COI_WITH_DEV_CONSOLE
 Dev.Reset(Simulation);
#endif
 Simulation.Shutdown();Paused=true;FractionalMicros=0;NotifySeconds=0;}
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
#if COI_WITH_DEV_CONSOLE
  if(Dev.IsAdvancing()){Paused=true;Dev.Tick(Simulation);if(!Dev.IsAdvancing()){NotifySeconds=0;PublishPresentation();}}else Simulation.Advance(Whole,Speed,Paused);
#else
  Simulation.Advance(Whole,Speed,Paused);
#endif
  NotifySeconds+=Dt;
  if(NotifySeconds>=.25){NotifySeconds=0;PublishPresentation();}
 }
 GameThreadMicros=(FPlatformTime::Seconds()-Begin)*1000000.;
}
void UCoISimulationSubsystem::TogglePause()
 {check(IsInGameThread());if(!IsReady())return;
#if COI_WITH_DEV_CONSOLE
 if(Dev.IsAdvancing())return;
#endif
 Paused=!Paused;UE_LOG(LogTemp,Display,TEXT("COI_SIM_CONTROL paused=%d speed=%d step=%lld"),Paused,Speed,GetSnapshot().Step);PublishPresentation();}
void UCoISimulationSubsystem::SetSpeed(int32 Value)
 {check(IsInGameThread());if(Value!=1&&Value!=4&&Value!=16)return;Speed=Value;UE_LOG(LogTemp,Display,TEXT("COI_SIM_CONTROL paused=%d speed=%d step=%lld"),Paused,Speed,GetSnapshot().Step);PublishPresentation();}
bool UCoISimulationSubsystem::Submit(CoISim::FCommand Command,FString& Error)
 {check(IsInGameThread());
#if COI_WITH_DEV_CONSOLE
 if(Dev.IsAdvancing()){Error=TEXT("Entwicklervorschub aktiv.");return false;}
#endif
 return Simulation.Enqueue(MoveTemp(Command),Error);}

bool UCoISimulationSubsystem::SubmitPlayer(CoISim::FCommand C,int64 Token,int64& Ticket,FString& Error) {
 check(IsInGameThread());
#if COI_WITH_DEV_CONSOLE
 if(Dev.IsAdvancing()){Error=TEXT("Entwicklervorschub aktiv. Erst cancel oder Abschluss abwarten.");return false;}
#endif
 if(!IsReady()){Error=TEXT("Simulation ist nicht bereit.");return false;}
 if(Token<=LastPlayerToken){Error=TEXT("Diese Bestätigung wurde bereits gesendet.");return false;}
 if(C.Generation!=Simulation.GetGeneration()){Error=TEXT("Szenario gewechselt. Neue Vorschau öffnen.");return false;}
 C.Sequence=Simulation.NextSequence();C.TargetStep=Simulation.GetState().Step+1;
 const FString Actor=Simulation.GetSession().PlayerCompanyId;C.Origin=CoISim::ECommandOrigin::Player;C.CompanyId=Actor;
 if(C.Kind==CoISim::ECommandKind::PlaceOrder){C.Order.ActorId=Actor;C.Order.Id=FString::Printf(TEXT("player.order.%lld"),C.Sequence);}
 else if(C.Kind==CoISim::ECommandKind::CancelOrder)C.Order.ActorId=Actor;
 else if(C.Kind==CoISim::ECommandKind::BookTransport){C.Transport.OwnerId=Actor;C.Transport.Id=FString::Printf(TEXT("player.haul.%lld"),C.Sequence);}
 else if(C.Kind==CoISim::ECommandKind::CancelTransport)C.Transport.OwnerId=Actor;
 else {Error=TEXT("Dieser Befehl ist keine Spieleraktion.");return false;}
 Ticket=C.Sequence;if(!Simulation.Enqueue(MoveTemp(C),Error)){Error=CoISim::PlayerReason(Error);return false;}
 LastPlayerToken=Token;PublishPresentation();return true;
}

void UCoISimulationSubsystem::PublishPresentation(){check(IsInGameThread());Presentation.Refresh(Simulation.GetSnapshot());OnChanged.Broadcast();}

#if COI_WITH_DEV_CONSOLE
void UCoISimulationSubsystem::InitializeDeveloper(){
 JournalFailed=false;
 DeveloperDirectory=FPaths::ProjectSavedDir()/TEXT("Developer")/FGuid::NewGuid().ToString(EGuidFormats::Digits);IFileManager::Get().MakeDirectory(*DeveloperDirectory,true);
 auto Read=[&](const TCHAR* Key,int32 Default,int32 Min,int32 Max){int32 N=Default;GConfig->GetInt(TEXT("CoIDeveloperConsole"),Key,N,GGameIni);return FMath::Clamp(N,Min,Max);};
 Dev.Limits.OutputLines=Read(TEXT("OutputLines"),160,40,1000);Dev.Limits.HistoryLines=Read(TEXT("HistoryLines"),64,1,256);Dev.Limits.AuditRecords=Read(TEXT("AuditRecords"),512,64,4096);Dev.Limits.MaxStepsPerFrame=Read(TEXT("MaxStepsPerFrame"),4,1,64);Dev.Limits.BudgetMicros=Read(TEXT("BudgetMicros"),2000,100,10000);Dev.Limits.MaxAdvanceHours=Read(TEXT("MaxAdvanceHours"),8760,1,87600);
 Dev.Journal=[this](const CoISim::FDevRecord& R){if(!FFileHelper::SaveStringToFile(R.Json()+TEXT("\n"),*(DeveloperDirectory/TEXT("ConsoleJournal.jsonl")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,&IFileManager::Get(),FILEWRITE_Append)){JournalFailed=true;UE_LOG(LogTemp,Error,TEXT("COI_DEV_JOURNAL_WRITE_FAILED"));}};
 Dev.RestartScenario=[this](const FString& Id,int64 Seed,FString& E){return RestartDeveloper(Id,Seed,E);};Dev.ExportLog=[this](FString& E){return ExportDeveloper(E);};
 const FString ConfigPath=DeveloperDirectory/FString::Printf(TEXT("Configuration_%llu.json"),Simulation.GetGeneration());if(!FFileHelper::SaveStringToFile(CoISim::FDeveloperConsole::Configuration(Simulation),*ConfigPath,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))JournalFailed=true;
 Dev.Execute(Simulation,TEXT("repro"));
}
bool UCoISimulationSubsystem::ExecuteDeveloper(const FString& Text){check(IsInGameThread());CoISim::FDevCommand C;FString Error;if(CoISim::FDeveloperConsole::Parse(Text,C,Error)&&(C.Mutating||C.Kind==CoISim::EDevKind::Pause))Paused=true;const bool OK=Dev.Execute(Simulation,Text);PublishPresentation();return OK;}
bool UCoISimulationSubsystem::RestartDeveloper(const FString& Id,int64 Seed,FString& Error){
 FString Json;FCoIGeoMapData Geo;bool Valid=false;for(TActorIterator<ACoIGeoReference> It(GetWorld());It;++It)if(It->Catalog){Valid=Geo.Load(It->Catalog->DefinitionJson,Error);break;}
 TSharedPtr<FJsonObject> Root;if(!Valid||!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("Simulation/scenario.coisim")))||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root)||!Root.IsValid()){Error=TEXT("Konfiguriertes Szenario nicht lesbar.");return false;}
 if(Root->GetStringField(TEXT("Id"))!=Id){Error=TEXT("SCENARIO muss dem vorhandenen konfigurierten Szenario entsprechen; keine Dateipfade.");return false;}
 Root->GetObjectField(TEXT("Settings"))->SetNumberField(TEXT("Seed"),Seed);FString Modified;FJsonSerializer::Serialize(Root.ToSharedRef(),TJsonWriterFactory<>::Create(&Modified));if(!Simulation.Load(Modified,Geo,Error))return false;
 Paused=true;Speed=1;LastPlayerToken=0;NotifySeconds=0;FractionalMicros=0;Dev.Reset(Simulation);
 const FString ConfigPath=DeveloperDirectory/FString::Printf(TEXT("Configuration_%llu.json"),Simulation.GetGeneration());if(!FFileHelper::SaveStringToFile(CoISim::FDeveloperConsole::Configuration(Simulation),*ConfigPath,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))JournalFailed=true;
 Dev.Execute(Simulation,TEXT("repro"));return true;
}
bool UCoISimulationSubsystem::ExportDeveloper(FString& Result){if(JournalFailed){Result=TEXT("Journal-/Konfigurationsschreibfehler; siehe Entwicklungslog.");return false;}const FString Target=DeveloperDirectory/TEXT("ConsoleExport.jsonl");if(IFileManager::Get().Copy(*Target,*(DeveloperDirectory/TEXT("ConsoleJournal.jsonl")),true,true)!=COPY_OK){Result=TEXT("Journalexport nicht möglich.");return false;}Result=Target+TEXT("; vollständige Konfiguration je Generation: Configuration_<Generation>.json");return true;}
#endif

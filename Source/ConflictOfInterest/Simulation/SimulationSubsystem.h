#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SimCore.h"
#include "DeveloperConsole.h"
#include "SaveGame.h"
#include "SimulationSubsystem.generated.h"
DECLARE_MULTICAST_DELEGATE(FCoISimulationChanged);
// Sole authoritative UE owner. All entry points run synchronously on the Game Thread.
UCLASS()
class CONFLICTOFINTEREST_API UCoISimulationSubsystem : public UTickableWorldSubsystem
{
 GENERATED_BODY()
public:
 virtual void OnWorldBeginPlay(UWorld& InWorld) override;
 virtual void OnWorldEndPlay(UWorld& InWorld) override;
 virtual void Deinitialize() override;
 virtual void Tick(float DeltaTime) override;
 virtual TStatId GetStatId() const override {RETURN_QUICK_DECLARE_CYCLE_STAT(UCoISimulationSubsystem,STATGROUP_Tickables);}
 virtual bool IsTickable() const override {return Super::IsTickable()&&Simulation.IsLoaded();}
 virtual bool DoesSupportWorldType(EWorldType::Type Type) const override {return Type==EWorldType::Game||Type==EWorldType::PIE;}
 void TogglePause();void SetSpeed(int32 Value);bool Restart();
 bool SaveSlot(int32 Slot,FString& Error);bool LoadSlot(int32 Slot,FString& Error);
 bool Submit(CoISim::FCommand Command,FString& Error);
 bool SubmitPlayer(CoISim::FCommand Command,int64 Token,int64& Ticket,FString& Error);
 bool PreviewLoan(CoISim::FLoanRequest,CoISim::FLoanQuote&,FString&) const;
 bool PreviewDelivery(CoISim::FDeliveryRequest Request,FString& Error) const;
 bool PreviewOperatingRule(CoISim::FOperatingRule Rule,FString& Error) const;
 bool PreviewInvestment(CoISim::FInvestmentRequest Request,CoISim::FInvestmentQuote& Quote,FString& Error) const;
 const TArray<CoISim::FCommandResult>& GetCommandResults() const {return Simulation.GetResults();}
 const CoISim::FSnapshot& GetSnapshot() const {return Presentation.Get();}
 const CoISim::FDiagnostics& GetDiagnostics() const {return Simulation.GetDiagnostics();}
 bool IsPaused() const {return Paused;}int32 GetSpeed() const {return Speed;}
 bool IsReady() const {return Simulation.IsLoaded();}
 double GetGameThreadMicros() const {return GameThreadMicros;}
 const FString& GetError() const {return LoadError;}
 FString Fingerprint() const {return Simulation.Fingerprint();}
#if COI_WITH_DEV_CONSOLE
 bool ExecuteDeveloper(const FString& Text);
 FString DeveloperStatus() const{return Dev.Status(Simulation);}
 const CoISim::FDeveloperConsole& Developer() const{return Dev;}
#endif
 FCoISimulationChanged OnChanged;
private:
#if COI_WITH_DEV_CONSOLE
 CoISim::FDeveloperConsole Dev;FString DeveloperDirectory;bool JournalFailed=false;
 void InitializeDeveloper();bool RestartDeveloper(const FString&,int64,FString&);bool ExportDeveloper(FString&);
#endif
 CoISim::FPresentationSnapshot Presentation;void PublishPresentation();
 CoISim::FSimulation Simulation;
 bool Paused=true;int32 Speed=1;double FractionalMicros=0,NotifySeconds=0,GameThreadMicros=0;
 int64 LastPlayerToken=0;FString LoadError;
 void Stop();
};

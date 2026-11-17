#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SimCore.h"
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
 bool Submit(CoISim::FCommand Command,FString& Error);
 bool SubmitPlayer(CoISim::FCommand Command,int64 Token,int64& Ticket,FString& Error);
 const TArray<CoISim::FCommandResult>& GetCommandResults() const {return Simulation.GetResults();}
 const CoISim::FSnapshot& GetSnapshot() const {return Presentation.Get();}
 const CoISim::FDiagnostics& GetDiagnostics() const {return Simulation.GetDiagnostics();}
 bool IsPaused() const {return Paused;}int32 GetSpeed() const {return Speed;}
 bool IsReady() const {return Simulation.IsLoaded();}
 double GetGameThreadMicros() const {return GameThreadMicros;}
 const FString& GetError() const {return LoadError;}
 FString Fingerprint() const {return Simulation.Fingerprint();}
 FCoISimulationChanged OnChanged;
private:
 CoISim::FPresentationSnapshot Presentation;void PublishPresentation();
 CoISim::FSimulation Simulation;
 bool Paused=true;int32 Speed=1;double FractionalMicros=0,NotifySeconds=0,GameThreadMicros=0;
 int64 LastPlayerToken=0;FString LoadError;
 void Stop();
};

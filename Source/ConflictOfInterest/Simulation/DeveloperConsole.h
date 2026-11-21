#pragma once
#include "DeveloperBuild.h"
#if COI_WITH_DEV_CONSOLE
#include "SimCore.h"
namespace CoISim {
enum class EDevKind:uint8 {Help,List,Inspect,Pause,Step,Advance,Status,Cancel,MoneySet,MoneyAdd,GoodsAdd,Relocate,Validate,Repro,Config,Restart,Export,EventList,EventStart,EventStop,NetworkStatus,NetworkRoute};
struct FDevCommand {EDevKind Kind=EDevKind::Help;TArray<FString> Args;FString Raw;bool Mutating=false;};
struct FDevSpec {FString Syntax,Description;};
struct FDevRecord {int64 Sequence=0,Step=0;uint64 Generation=0;bool Accepted=false;FString Command,Phase,Before,After,Message;FString Json() const;};
struct FDevLimits {int32 OutputLines=160,HistoryLines=64,AuditRecords=512,MaxStepsPerFrame=4;int64 MaxAdvanceHours=8760,BudgetMicros=2000;};
class CONFLICTOFINTEREST_API FDeveloperConsole {
public:
 FDevLimits Limits;TFunction<void(const FDevRecord&)> Journal;
 TFunction<bool(const FString&,int64,FString&)> RestartScenario;TFunction<bool(FString&)> ExportLog;
 static const TArray<FDevSpec>& Registry();static bool Parse(const FString&,FDevCommand&,FString&);
 bool Execute(FSimulation&,const FString&);void Tick(FSimulation&);void Reset(FSimulation&);
 bool IsAdvancing() const{return Remaining>0;}FString Status(const FSimulation&) const;
 const TArray<FString>& Output() const{return Lines;}const TArray<FString>& History() const{return Commands;}const TArray<FDevRecord>& Records() const{return Audit;}
 static FString Configuration(const FSimulation&);
private:
 TArray<FString> Lines,Commands;TArray<FDevRecord> Audit;int64 Sequence=0,Remaining=0,Total=0,Completed=0,AdvanceSequence=0;uint64 AdvanceGeneration=0;
 void Record(FSimulation&,const FString&,bool,const FString&,const FString& Phase=TEXT("result"),const FString& Before=TEXT(""),const FString& After=TEXT(""),int64 Correlation=0);
 bool Mutate(FSimulation&,const FDevCommand&,FString&,FString&,FString&);
 bool Validate(const FSimulation&,FString&) const;
 FString Read(const FSimulation&,const FDevCommand&,bool&) const;
};
}
#endif

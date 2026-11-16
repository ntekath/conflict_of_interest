#pragma once
#include "SimModel.h"
#include "GeoMapData.h"
namespace CoISim
{
enum class ECommandKind : uint8 { Reserve,Release,PlaceOrder,CancelOrder,BookTransport,CancelTransport };
struct FCommand
{
 int64 TargetStep=0,Sequence=0; uint64 Generation=0;
 ECommandKind Kind=ECommandKind::Reserve; FReservation Resources; FMarketOrder Order; FTransport Transport;
};
struct FCommandResult { int64 Sequence=0,Step=0; bool Accepted=false; FString Reason; };
struct FRegionProposal { int32 Index=0; int64 Step=0; uint32 Random=0; };
struct FDiagnostics
{
 double CommandsMicros=0,CalculateMicros=0,CommitMicros=0,AggregateMicros=0,StepMicros=0,FrameMicros=0,SnapshotMicros=0;
 int64 BacklogSteps=0,PendingCommands=0,StepsThisFrame=0,ThrottledFrames=0;
 double ActualHoursPerSecond=0; bool BudgetLimited=false;
};
struct FSnapshot
{
 uint64 Generation=0,Version=0; int64 Step=0,Population=0,Employed=0,PlayerMoneyCents=0,PlayerReservedCents=0,PlayerStorageCapacity=0,PlayerStorageUsed=0,PlayerTrucks=0;
 FString Date,PlayerName,EconomySummary;
 TMap<FString,FString> EconomyDetails; TArray<FRegion> Regions; TArray<FCountry> Countries;
};
bool LoadScenario(const FString& Json,const FCoIGeoMapData& Geo,FScenario& Out,FString& Error);
bool ValidateScenario(const FScenario& Scenario,const FCoIGeoMapData& Geo,FString& Error);
uint32 EntityRandom(int64 Seed,int64 Step,const FString& Id);
class CONFLICTOFINTEREST_API FSimulation
{
public:
 bool Load(const FString& Json,const FCoIGeoMapData& Geo,FString& Error);
 bool Enqueue(FCommand Command,FString& Error);
 void Advance(int64 RealMicros,int32 Speed,bool Paused);
 bool StepOnce();
 void Shutdown();
 const FScenario& GetScenario() const { return Scenario; }
 const FState& GetState() const { return State; }
 const FSnapshot& GetSnapshot() const { return Snapshot; }
 const FDiagnostics& GetDiagnostics() const { return Diagnostics; }
 const TArray<FCommandResult>& GetResults() const { return Results; }
 FString Fingerprint() const;
 uint64 GetGeneration() const { return Generation; }
 int64 NextSequence() const { return LastIssuedSequence+1; }
 bool AuditRuntime(FString& Error) const;
 bool IsLoaded() const { return Loaded; }
private:
 FScenario Scenario; FState State; FSnapshot Snapshot; FDiagnostics Diagnostics;
 TArray<FCommand> Pending; TArray<FCommandResult> Results;
 uint64 Generation=0,Version=0; int64 LastIssuedSequence=0,CreditMicros=0;
 double RateSeconds=0; int64 RateSteps=0; bool Loaded=false;
 bool Apply(const FCommand&,FString& Reason);
 bool RunStep();
 void Aggregate(); void Publish();
 void EconomyHour(int64 Hour);
 void Payroll(int64 Hour);
 void Produce(int64 Hour);
 bool Pay(const FString& From,const FString& To,int64 Cents,const FString& Reason);
 FString ActorAccount(const FString& ActorId) const;
 int32 StockIndex(const FString& Actor,const FString& Store,const FString& Good) const;
 int32 EnsureStock(const FString& Actor,const FString& Store,const FString& Good);
 int64 TakeStock(int32 Index,int64 Kg);int64 TakeSpoilCredit(int32 Index,int64 Kg);
 void InitializeAudit();void PublishEconomy(FSnapshot& Out) const;
 bool PlaceOrder(FMarketOrder Order,FString& Error);
 bool CancelOrder(const FString& Id,const FString& Actor,const FString& Status);
 void MatchOrders(); void ExpireOrders();
 void Demand(int64 Hour);void Consume();
 bool BookTransport(FTransport Job,FString& Error);bool CancelTransport(const FString& Id,const FString& Actor);
 int64 RouteKm(const FString& From,const FString& To) const;
 void TransportHour(int64 Hour);void Costs(int64 Hour);void Spoil();void Companies(int64 Hour);void Trader(int64 Hour);
 void Offer(const FString& Actor,const FString& Store,const FString& Good,const FString& Side,int64 Kg,int64 Price);
 int64 UsedStorage(const FString& Id) const;
};
}

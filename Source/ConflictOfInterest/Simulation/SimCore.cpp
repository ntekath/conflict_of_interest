#include "SimCore.h"
#include <atomic>
#include "SimArchive.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
namespace CoISim
{
static double Micros(){return FPlatformTime::Seconds()*1000000.;}
uint32 EntityRandom(int64 Seed,int64 Step,const FString& Id)
{
 uint64 Hash=14695981039346656037ULL;
 auto Byte=[&](uint8 B){Hash=(Hash^B)*1099511628211ULL;};
 for(int I=0;I<8;++I)Byte(((uint64)Seed>>(8*I))&255);
 for(int I=0;I<8;++I)Byte(((uint64)Step>>(8*I))&255);
 FTCHARToUTF8 Text(*Id);for(int I=0;I<Text.Length();++I)Byte((uint8)Text.Get()[I]);
 // Pure stream key: no shared/global random generator, task order cannot consume another entity's draws.
 Hash^=Hash>>30;Hash*=0xbf58476d1ce4e5b9ULL;Hash^=Hash>>27;Hash*=0x94d049bb133111ebULL;Hash^=Hash>>31;return (uint32)Hash;
}
bool FSimulation::Load(const FString& Json,const FCoIGeoMapData& Geo,FString& Error)
{
 FScenario Candidate;if(!LoadScenario(Json,Geo,Candidate,Error))return false;
 Shutdown();Scenario=MoveTemp(Candidate);State=Scenario.InitialState;Session={Scenario.Id,Scenario.PlayerCompanyId};Loaded=true;
 for(const auto& R:Scenario.Definitions.Regions){FRegion Region;Region.Id=R.Id;Region.RemainingForestKg=R.ForestKg;Region.RemainingOreKg=R.OreKg;State.Regions.Add(Region);}
 for(const auto& X:Scenario.Definitions.ResourceSites)State.ResourceSites.Add({X.Id,X.InitialKg,0});
 InitializeOperatingRules();InitializeAudit();BeginDayMetrics(0);Aggregate();Publish();return true;
}
void FSimulation::Shutdown()
{
 // A local counter would reuse generation 1 in the next PIE world. Epochs are process-unique.
 static std::atomic<uint64> NextEpoch{0};
 Loaded=false;Generation=NextEpoch.fetch_add(1,std::memory_order_relaxed)+1;Version=0;LastIssuedSequence=0;CreditMicros=0;RateSeconds=0;RateSteps=0;
 Pending.Reset();Results.Reset();Session=FSession();State=FState();Snapshot=FSnapshot();Scenario=FScenario();Diagnostics=FDiagnostics();
}
bool FSimulation::Enqueue(FCommand Command,FString& Error)
{
 if(!Loaded||Command.Generation!=Generation){Error=TEXT("Stale scenario generation");return false;}
 if(Command.Sequence<=LastIssuedSequence||Command.Sequence>1000000000000LL||Command.TargetStep<=State.Step||Command.TargetStep-State.Step>Scenario.Settings.CommandHorizonSteps)
 {Error=TEXT("Duplicate/nonmonotonic sequence, past target or excessive horizon");return false;}
 if(Pending.Num()>=Scenario.Settings.MaxPendingCommands){Error=TEXT("Command queue full; caller must retry explicitly");return false;}
 if(Command.Kind>ECommandKind::Delivery){Error=TEXT("Unsupported command kind");return false;}
 if(!Authorized(Command,Error))return false;
 LastIssuedSequence=Command.Sequence;Pending.Add(MoveTemp(Command));
 Pending.Sort([](const FCommand& A,const FCommand& B){return A.TargetStep==B.TargetStep?A.Sequence<B.Sequence:A.TargetStep<B.TargetStep;});
 Error.Empty();Diagnostics.PendingCommands=Pending.Num();return true;
}
int64 FSimulation::UsedStorage(const FString& Id) const
{
 int64 Used=0;for(const auto& I:State.Inventories)if(I.StorageId==Id)Used+=I.QuantityKg*Find(Scenario.Definitions.Goods,I.GoodId)->StoragePointsPerKg;return Used;
}
bool FSimulation::Apply(const FCommand& Command,FString& Reason)
{
 if(!Authorized(Command,Reason))return false;
 if(Command.Kind==ECommandKind::Delivery)return ChangeDelivery(Command.Delivery,Reason);
 if(Command.Kind==ECommandKind::OperatingRule)return ChangeOperatingRule(Command.Rule,Reason);
 if(Command.Kind==ECommandKind::Invest)return Invest(Command.Investment,Reason);
 if(Command.Kind==ECommandKind::BookTransport)return BookTransport(Command.Transport,Reason);
 if(Command.Kind==ECommandKind::CancelTransport){bool OK=CancelTransport(Command.Transport.Id,Command.Transport.OwnerId);Reason=OK?TEXT("Cancelled"):TEXT("Not cancellable");return OK;}
 if(Command.Kind==ECommandKind::PlaceOrder)return PlaceOrder(Command.Order,Reason);
 if(Command.Kind==ECommandKind::CancelOrder){bool OK=CancelOrder(Command.Order.Id,Command.Order.ActorId,TEXT("cancelled"));Reason=OK?TEXT("Cancelled"):TEXT("Missing live owned order");return OK;}
 const auto& R=Command.Resources;
 auto Reject=[&](const TCHAR* Message){Reason=Message;return false;};
 if(R.Id.IsEmpty()||R.Id.Len()>128)return Reject(TEXT("Invalid reservation ID"));
 if(Command.Kind==ECommandKind::Release)
 {
  auto* Existing=Find(State.Reservations,R.Id);
  if(!Existing||!Existing->Active||Existing->ActorId!=R.ActorId)return Reject(TEXT("Missing/inactive reservation or wrong owner"));
  if(Existing->MoneyCents)Find(State.Accounts,Existing->AccountId)->ReservedCents-=Existing->MoneyCents;
  if(Existing->Kg)Find(State.Inventories,Existing->InventoryId)->ReservedKg-=Existing->Kg;
  if(Existing->IncomingPoints)Find(State.Warehouses,Existing->WarehouseId)->ReservedIncomingPoints-=Existing->IncomingPoints;
  if(!Existing->VehicleId.IsEmpty()){auto* V=Find(State.Vehicles,Existing->VehicleId);V->Bound=false;V->TransportId.Empty();}
  Existing->Active=false;Reason=TEXT("Released");return true;
 }
 if(Find(State.Reservations,R.Id)||State.Reservations.Num()>=4096)return Reject(TEXT("Reservation ID already used or session reservation limit reached"));
 if(!Find(State.Companies,R.ActorId)&&!Find(State.Groups,R.ActorId))return Reject(TEXT("Unknown actor"));
 if(R.MoneyCents<0||R.Kg<0||R.IncomingPoints<0||R.MoneyCents>1000000000000LL||R.Kg>1000000000||R.IncomingPoints>1000000000000LL)return Reject(TEXT("Invalid reservation quantity"));
 if(!R.MoneyCents&&!R.Kg&&!R.IncomingPoints&&R.VehicleId.IsEmpty())return Reject(TEXT("Empty reservation"));
 auto* Account=Find(State.Accounts,R.AccountId);auto* Stock=Find(State.Inventories,R.InventoryId);auto* Store=Find(State.Warehouses,R.WarehouseId);auto* Vehicle=Find(State.Vehicles,R.VehicleId);
 if(R.MoneyCents&&(!Account||Account->OwnerId!=R.ActorId||R.MoneyCents>Account->BalanceCents-Account->ReservedCents))return Reject(TEXT("Insufficient owned funds"));
 if(R.Kg&&(!Stock||Stock->OwnerId!=R.ActorId||R.Kg>Stock->QuantityKg-Stock->ReservedKg))return Reject(TEXT("Insufficient owned local stock"));
 if(R.IncomingPoints&&(!Store||Store->OwnerId!=R.ActorId||R.IncomingPoints>Find(Scenario.Definitions.WarehouseTypes,Store->TypeId)->CapacityPoints-UsedStorage(Store->Id)-Store->ReservedIncomingPoints))return Reject(TEXT("Insufficient owned destination capacity"));
 if(!R.VehicleId.IsEmpty()&&(!Vehicle||Vehicle->OwnerId!=R.ActorId||Vehicle->Bound))return Reject(TEXT("Vehicle unavailable or not owned"));
 // Central commit: every predicate was checked before mutating any resource.
 if(R.MoneyCents)Account->ReservedCents+=R.MoneyCents;
 if(R.Kg)Stock->ReservedKg+=R.Kg;
 if(R.IncomingPoints)Store->ReservedIncomingPoints+=R.IncomingPoints;
 if(Vehicle){Vehicle->Bound=true;Vehicle->TransportId=R.Id;}
 auto Copy=R;Copy.Active=true;State.Reservations.Add(MoveTemp(Copy));Reason=TEXT("Reserved atomically");return true;
}
bool FSimulation::RunStep()
{
 if(!Loaded||InStep)return false;
 TGuardValue<bool> StepBoundary(InStep,true);
 FDateTime Start;FDateTime::ParseIso8601(*(Scenario.StartDate+TEXT("T00:00:00Z")),Start);
 if(State.Step>=(FDateTime::MaxValue().GetTicks()-Start.GetTicks())/(ETimespan::TicksPerHour*Scenario.Settings.HoursPerStep))return false;
 TRACE_CPUPROFILER_EVENT_SCOPE(CoI_SimulationStep);
 const double Begin=Micros();const int64 Next=State.Step+1;
 {
  TRACE_CPUPROFILER_EVENT_SCOPE(CoI_Commands);double T=Micros();int32 Done=0;
  for(const auto& C:Pending)
  {
   if(C.TargetStep>Next)break;
   FCommandResult Result;Result.Sequence=C.Sequence;Result.Step=Next;
   Result.ActorId=C.Kind==ECommandKind::Delivery?C.Delivery.OwnerId:C.Kind==ECommandKind::OperatingRule?C.Rule.OwnerId:C.Kind==ECommandKind::Invest?C.Investment.OwnerId:(C.Kind==ECommandKind::PlaceOrder||C.Kind==ECommandKind::CancelOrder)?C.Order.ActorId:C.Transport.OwnerId;Result.ObjectId=C.Kind==ECommandKind::Delivery?C.Delivery.Id:C.Kind==ECommandKind::OperatingRule?C.Rule.Id:C.Kind==ECommandKind::Invest?C.Investment.Id:(C.Kind==ECommandKind::PlaceOrder||C.Kind==ECommandKind::CancelOrder)?C.Order.Id:C.Transport.Id;
   Result.Accepted=C.Generation==Generation&&Apply(C,Result.Reason);if(C.Kind==ECommandKind::Invest&&C.Investment.Action==TEXT("buy"))if(const auto* Sale=Find(State.AssetSales,C.Investment.TargetId))Result.ObjectId=Sale->AssetId;if(Result.ActorId==Session.PlayerCompanyId)PlayerEvent(C.Kind==ECommandKind::Delivery?TEXT("delivery"):(C.Kind==ECommandKind::Invest||C.Kind==ECommandKind::OperatingRule)?TEXT("investment"):C.Kind==ECommandKind::PlaceOrder||C.Kind==ECommandKind::CancelOrder?TEXT("order"):TEXT("transport"),Result.ObjectId,(C.Kind==ECommandKind::Invest||C.Kind==ECommandKind::OperatingRule||C.Kind==ECommandKind::Delivery)?Result.Reason:Result.Accepted?TEXT("Auftrag bestätigt."):TEXT("Auftrag abgelehnt: ")+PlayerReason(Result.Reason));Results.Add(MoveTemp(Result));++Done;
  }
  if(Done)Pending.RemoveAt(0,Done,EAllowShrinking::No);
  if(Results.Num()>128)Results.RemoveAt(0,Results.Num()-128,EAllowShrinking::No);
  Diagnostics.CommandsMicros=Micros()-T;
 }
 // Match all queued market commands against the committed state before the next hour ages stock.
 if(Scenario.Economy.Stage>=2){ExpireOrders();MatchOrders();}
 for(int64 Hour=State.Step*Scenario.Settings.HoursPerStep;Hour<Next*Scenario.Settings.HoursPerStep;++Hour)
 {State.EconomyHour=Hour;PlayerDay();EconomyHour(Hour);State.EconomyHour=Hour+1;ConstructionHour(Hour+1);if(State.EconomyHour%24==0)FinishDayMetrics(Next);PlayerWarnings();}
 // Snapshot inputs are const and independent of rendering. Results have disjoint per-region slots.
 TArray<FRegionProposal> Proposals;Proposals.SetNum(State.Regions.Num());
 {
  TRACE_CPUPROFILER_EVENT_SCOPE(CoI_Calculate);double T=Micros();
  for(int32 I=0;I<State.Regions.Num();++I)Proposals[I]={I,Next,EntityRandom(Scenario.Settings.Seed,Next,State.Regions[I].Id)};
  Diagnostics.CalculateMicros=Micros()-T;
 }
 {
  TRACE_CPUPROFILER_EVENT_SCOPE(CoI_Commit);double T=Micros();
  for(const auto& P:Proposals){auto& Region=State.Regions[P.Index];Region.LastProcessedStep=P.Step;Region.DiagnosticRandom=P.Random;}
  State.Step=Next;++Version;Diagnostics.CommitMicros=Micros()-T;
 }
 Aggregate();Diagnostics.StepMicros=Micros()-Begin;Diagnostics.PendingCommands=Pending.Num();return true;
}
bool FSimulation::StepOnce(){if(!RunStep())return false;Publish();return true;}
void FSimulation::Advance(int64 RealMicros,int32 Speed,bool Paused)
{
 TRACE_CPUPROFILER_EVENT_SCOPE(CoI_SimulationFrame);const double Begin=Micros();Diagnostics.StepsThisFrame=0;Diagnostics.BudgetLimited=false;
 if(!Loaded||Paused||RealMicros<=0||(Speed!=1&&Speed!=4&&Speed!=16)){Diagnostics.FrameMicros=Micros()-Begin;return;}
 const auto& C=Scenario.Settings;const int64 Capacity=C.RealMicrosPerStep*C.MaxBacklogSteps;
 // Admit bounded wall-time demand. Saturation slows GAME TIME, it never advances/skips a numbered step.
 int64 Requested=FMath::Min(RealMicros,Capacity/Speed+1)*Speed,Room=Capacity-CreditMicros;
 bool Throttled=Requested>Room;if(Throttled)++Diagnostics.ThrottledFrames;
 CreditMicros+=FMath::Min(Requested,Room);
 while(CreditMicros>=C.RealMicrosPerStep&&Diagnostics.StepsThisFrame<C.MaxStepsPerFrame)
 {
  if(Diagnostics.StepsThisFrame&&Micros()-Begin>=C.StepBudgetMicros){Diagnostics.BudgetLimited=true;break;}
  if(!RunStep()){CreditMicros=0;break;}
  CreditMicros-=C.RealMicrosPerStep;++Diagnostics.StepsThisFrame;++RateSteps;
 }
 Diagnostics.BacklogSteps=CreditMicros/C.RealMicrosPerStep;Diagnostics.BudgetLimited|=Throttled||Diagnostics.BacklogSteps>0;
 RateSeconds+=(double)RealMicros/1000000.;if(RateSeconds>=1.0){Diagnostics.ActualHoursPerSecond=RateSteps*Scenario.Settings.HoursPerStep/RateSeconds;RateSeconds=0;RateSteps=0;}
 if(Diagnostics.StepsThisFrame)Publish();Diagnostics.FrameMicros=Micros()-Begin;
}
void FSimulation::Aggregate()
{
 TRACE_CPUPROFILER_EVENT_SCOPE(CoI_Aggregate);double T=Micros();State.Countries.Reset();
 for(auto& R:State.Regions)
 {
  R.Population=0;R.Employed=0;R.Consumed.Reset();for(const auto& G:Scenario.Definitions.Goods)R.Consumed.Add({G.Id,0});
  for(const auto& G:State.Groups)if(Find(Scenario.Definitions.Groups,G.Id)->RegionId==R.Id)
  {R.Population+=G.Population;R.Employed+=G.Employed;for(const auto& Amount:G.Consumed)for(auto& Total:R.Consumed)if(Total.GoodId==Amount.GoodId)Total.Kg+=Amount.Kg;}
  const FString CountryId=Find(Scenario.Definitions.Regions,R.Id)->CountryId;auto* Country=Find(State.Countries,CountryId);
  if(!Country){FCountry New;New.Id=CountryId;for(const auto& G:Scenario.Definitions.Goods)New.Consumed.Add({G.Id,0});State.Countries.Add(New);Country=&State.Countries.Last();}
  Country->Population+=R.Population;Country->Employed+=R.Employed;
  for(const auto& Amount:R.Consumed)for(auto& Total:Country->Consumed)if(Total.GoodId==Amount.GoodId)Total.Kg+=Amount.Kg;
 }
 Diagnostics.AggregateMicros=Micros()-T;
}
void FSimulation::Publish()
{
 TRACE_CPUPROFILER_EVENT_SCOPE(CoI_PublishSnapshot);double T=Micros();FSnapshot New;
 New.Generation=Generation;New.Version=Version;New.Step=State.Step;New.Regions=State.Regions;New.Countries=State.Countries;
 FDateTime Start;FDateTime::ParseIso8601(*(Scenario.StartDate+TEXT("T00:00:00Z")),Start);New.Date=(Start+FTimespan::FromHours((double)State.Step*Scenario.Settings.HoursPerStep)).ToString(TEXT("%d.%m.%Y %H:%M"));
 for(const auto& C:State.Countries){New.Population+=C.Population;New.Employed+=C.Employed;}
 const auto* Player=Find(State.Companies,Session.PlayerCompanyId);const auto* A=Find(State.Accounts,Player->AccountId);
 New.PlayerName=Find(Scenario.Definitions.Companies,Player->Id)->Name;New.PlayerMoneyCents=A->BalanceCents;New.PlayerReservedCents=A->ReservedCents;
 for(const auto& Store:State.Warehouses)if(Store.OwnerId==Player->Id){New.PlayerStorageCapacity+=Find(Scenario.Definitions.WarehouseTypes,Store.TypeId)->CapacityPoints;New.PlayerStorageUsed+=UsedStorage(Store.Id);}
 for(const auto& Vehicle:State.Vehicles)if(Vehicle.OwnerId==Player->Id)++New.PlayerTrucks;
 New.MapDay=State.LastDayMetrics;PublishPlayer(New);PublishEconomy(New);Snapshot=MoveTemp(New);Diagnostics.SnapshotMicros=Micros()-T;
}
FString FSimulation::Fingerprint() const
{
 FFingerprint F;F.Write(Scenario);F.Write(State);F.Write(LastIssuedSequence);
 for(const auto& C:Pending){F.Write(C.TargetStep);F.Write(C.Sequence);F.Write((int64)C.Kind);F.Write((int64)C.Origin);F.Write(C.CompanyId);F.Write(C.Resources);F.Write(C.Order);F.Write(C.Transport);F.Write(C.Investment);F.Write(C.Rule);F.Write(C.Delivery);}return F.Finish();
}
}

namespace CoISim {
bool FSimulation::IsAutonomous(const FString& Id) const {return Id!=Session.PlayerCompanyId&&Find(State.Companies,Id)!=nullptr;}
bool FSimulation::Authorized(const FCommand& C,FString& Error) const {
 if(C.Origin==ECommandOrigin::System)return true; // trusted scenario/test harness, never exposed by the player bridge
 FString Actor=C.Kind==ECommandKind::Delivery?C.Delivery.OwnerId:C.Kind==ECommandKind::OperatingRule?C.Rule.OwnerId:C.Kind==ECommandKind::Invest?C.Investment.OwnerId:(C.Kind==ECommandKind::PlaceOrder||C.Kind==ECommandKind::CancelOrder)?C.Order.ActorId:(C.Kind==ECommandKind::BookTransport||C.Kind==ECommandKind::CancelTransport)?C.Transport.OwnerId:C.Resources.ActorId;
 if(C.CompanyId!=Actor||!Find(State.Companies,Actor)||(C.Origin==ECommandOrigin::Player?Actor!=Session.PlayerCompanyId:!IsAutonomous(Actor))){Error=TEXT("Company control authorization failed");return false;}return true;
}
FString FSimulation::EconomicFingerprint() const {auto Economic=State;Economic.PlayerDays.Reset();Economic.PlayerEvents.Reset();FFingerprint F;F.Write(Scenario.Definitions);F.Write(Scenario.Settings);F.Write(Economic);return F.Finish();}
}

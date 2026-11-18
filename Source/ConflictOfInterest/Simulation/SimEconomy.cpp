#include "SimCore.h"
#include "CityHubs.h"
#include "Investments.h"
#include "SimArchive.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
namespace CoISim
{
FString FSimulation::ActorAccount(const FString& Id) const
{
 if(const auto* C=Find(State.Companies,Id))return C->AccountId;
 if(const auto* G=Find(State.Groups,Id))return G->AccountId;
 return {};
}
bool FSimulation::Pay(const FString& From,const FString& To,int64 Cents,const FString& Reason)
{
 auto* A=Find(State.Accounts,From);auto* B=Find(State.Accounts,To);
 if(!A||!B||Cents<0||Cents>A->BalanceCents-A->ReservedCents||B->BalanceCents>MAX_int64-Cents)return false;
 if(!Cents||From==To)return true;
 A->BalanceCents-=Cents;B->BalanceCents+=Cents;
 if(A->OwnerId==Session.PlayerCompanyId)PlayerDay().ExpenseCents+=Cents;
 if(B->OwnerId==Session.PlayerCompanyId)PlayerDay().IncomeCents+=Cents;
 State.CashFlows.Add({From,To,Reason,State.EconomyHour,Cents});
 if(State.CashFlows.Num()>Scenario.Economy.HistoryLimit)State.CashFlows.RemoveAt(0,State.CashFlows.Num()-Scenario.Economy.HistoryLimit,EAllowShrinking::No);
 auto* Total=Find(State.CashTotals,Reason);if(!Total){State.CashTotals.Add({Reason,0});Total=&State.CashTotals.Last();}Total->Cents+=Cents;
 return true;
}
int32 FSimulation::StockIndex(const FString& Actor,const FString& Store,const FString& Good) const
 {return State.Inventories.IndexOfByPredicate([&](const FInventory& I){return I.OwnerId==Actor&&I.StorageId==Store&&I.GoodId==Good;});}
int32 FSimulation::EnsureStock(const FString& Actor,const FString& Store,const FString& Good)
{
 int32 Index=StockIndex(Actor,Store,Good);if(Index!=INDEX_NONE)return Index;
 FInventory I;I.Id=TEXT("inventory.")+Actor+TEXT("|")+Store+TEXT("|")+Good;I.OwnerId=Actor;I.StorageId=Store;I.GoodId=Good;
 return State.Inventories.Add(MoveTemp(I));
}
int64 FSimulation::TakeSpoilCredit(int32 Index,int64 Kg)
{auto& I=State.Inventories[Index];int64 Credit=I.QuantityKg?I.SpoilRemainder/I.QuantityKg*Kg+(I.SpoilRemainder%I.QuantityKg)*Kg/I.QuantityKg:0;I.SpoilRemainder-=Credit;return Credit;}
int64 FSimulation::TakeStock(int32 Index,int64 Kg)
{
 auto& I=State.Inventories[Index];check(Kg>=0&&Kg<=I.QuantityKg-I.ReservedKg);
 int64 Value=I.QuantityKg?I.BookValueCents/I.QuantityKg*Kg+(I.BookValueCents%I.QuantityKg)*Kg/I.QuantityKg:0;
 I.BookValueCents-=Value;I.QuantityKg-=Kg;return Value;
}
void FSimulation::InitializeAudit()
{
 for(auto& I:State.Inventories)I.BookValueCents=I.QuantityKg*Find(Scenario.Definitions.Goods,I.GoodId)->InitialUnitCostCents;
 State.InitialMoneyCents=0;for(const auto& A:State.Accounts)State.InitialMoneyCents+=A.BalanceCents;
 State.GoodsAudit.Reset();for(const auto& G:Scenario.Definitions.Goods){FGoodsAudit Row;Row.Id=G.Id;for(const auto& I:State.Inventories)if(I.GoodId==G.Id)Row.InitialKg+=I.QuantityKg;State.GoodsAudit.Add(Row);}
}
bool FSimulation::AuditRuntime(FString& Error) const
{
 if(!ValidateInvestments(Scenario,State,false,Error)||!ValidateCityHubs(Scenario,State,false,Error))return false;
 auto Fail=[&](const FString& Message){Error=Message;return false;};
 int64 Money=0;for(const auto& A:State.Accounts){if(A.BalanceCents<0||A.ReservedCents<0||A.ReservedCents>A.BalanceCents)return Fail(TEXT("Account reservation/balance: ")+A.Id);Money+=A.BalanceCents;}
 if(Money!=State.InitialMoneyCents+State.DeveloperMoneySources-State.DeveloperMoneySinks)return Fail(TEXT("Global money conservation failed"));
 for(const auto& I:State.Inventories)if(I.QuantityKg<0||I.QuantityKg>1000000000||I.ReservedKg<0||I.ReservedKg>I.QuantityKg||I.BookValueCents<0)return Fail(TEXT("Physical stock: ")+I.Id);
 for(const auto& S:State.Warehouses)if(S.ReservedIncomingPoints<0||UsedStorage(S.Id)+S.ReservedIncomingPoints>Find(Scenario.Definitions.WarehouseTypes,S.TypeId)->CapacityPoints)return Fail(TEXT("Storage capacity: ")+S.Id);
 for(const auto& G:State.GoodsAudit){int64 Actual=0;for(const auto& I:State.Inventories)if(I.GoodId==G.Id)Actual+=I.QuantityKg;if(Actual!=G.InitialKg+G.DeveloperAddedKg+G.ProducedKg-G.InputKg-G.ConsumedKg-G.SpoiledKg)return Fail(TEXT("Goods source/sink equation: ")+G.Id);}
 for(const auto& G:State.Groups)
 {int64 Workers=0;for(const auto& F:State.Facilities)for(const auto& J:F.Employment)if(J.GroupId==G.Id)Workers+=J.Workers;if(Workers!=G.Employed||Workers>G.Workforce)return Fail(TEXT("Employment: ")+G.Id);}

 for(const auto& A:State.Accounts){int64 Backed=0;for(const auto& P:State.Projects)if(P.Status==TEXT("building")&&ActorAccount(P.OwnerId)==A.Id)Backed+=P.CostCents;for(const auto& O:State.Orders)if(ActorAccount(O.ActorId)==A.Id)Backed+=O.ReservedCents;for(const auto& J:State.Transports)if(J.Status==TEXT("planned")){if(ActorAccount(J.OwnerId)==A.Id)Backed+=J.CostCents;if(J.OwnerId!=J.ProviderId&&ActorAccount(J.ProviderId)==A.Id)Backed+=J.OperatingCents;}for(const auto& R:State.Reservations)if(R.Active&&R.AccountId==A.Id)Backed+=R.MoneyCents;if(Backed!=A.ReservedCents)return Fail(TEXT("Unbacked money hold: ")+A.Id);}
 for(const auto& I:State.Inventories){int64 Backed=0;for(const auto& O:State.Orders)if(O.Side==TEXT("sell")&&O.ActorId==I.OwnerId&&O.StorageId==I.StorageId&&O.GoodId==I.GoodId)Backed+=O.ReservedKg;for(const auto& J:State.Transports)if(J.Status==TEXT("planned")&&J.SourceInventoryId==I.Id)Backed+=J.Kg;for(const auto& R:State.Reservations)if(R.Active&&R.InventoryId==I.Id)Backed+=R.Kg;if(Backed!=I.ReservedKg)return Fail(TEXT("Unbacked stock hold: ")+I.Id);
  if(!Find(State.Warehouses,I.StorageId)){bool Cargo=false;for(const auto& J:State.Transports)if(J.Status==TEXT("moving")&&J.CargoInventoryId==I.Id)Cargo=true;if(!Cargo)return Fail(TEXT("Stock without physical location: ")+I.Id);}
 }
 for(const auto& S:State.Warehouses){int64 Backed=0;for(const auto& J:State.Transports)if((J.Status==TEXT("planned")||J.Status==TEXT("moving"))&&J.DestinationStorageId==S.Id)Backed+=J.ReservedIncomingPoints;for(const auto& R:State.Reservations)if(R.Active&&R.WarehouseId==S.Id)Backed+=R.IncomingPoints;if(Backed!=S.ReservedIncomingPoints)return Fail(TEXT("Unbacked destination hold: ")+S.Id);}
 for(const auto& V:State.Vehicles){int Count=0;for(const auto& J:State.Transports)if((J.Status==TEXT("planned")||J.Status==TEXT("moving"))&&J.VehicleId==V.Id){++Count;if(V.TransportId!=J.Id)return Fail(TEXT("Wrong vehicle job"));}for(const auto& R:State.Reservations)if(R.Active&&R.VehicleId==V.Id)++Count;if(Count>1||V.Bound!=(Count==1))return Fail(TEXT("Vehicle double binding: ")+V.Id);}
 Error.Empty();return true;
}
void FSimulation::Payroll(int64 Hour)
{
 TRACE_CPUPROFILER_EVENT_SCOPE(CoI_Payroll);
 if(Hour%Scenario.Economy.PayrollHours)return;
 for(auto& G:State.Groups){G.Employed=0;G.DailyIncomeCents=0;}
 TArray<int32> Order;for(int32 I=0;I<State.Facilities.Num();++I)Order.Add(I);
 Order.Sort([&](int32 A,int32 B){auto* X=Find(Scenario.Definitions.FacilityTypes,State.Facilities[A].TypeId);auto* Y=Find(Scenario.Definitions.FacilityTypes,State.Facilities[B].TypeId);return X->DailyWageCents==Y->DailyWageCents?State.Facilities[A].Id<State.Facilities[B].Id:X->DailyWageCents>Y->DailyWageCents;});
 for(int32 Index:Order)
 {
  auto& F=State.Facilities[Index];const auto* Type=Find(Scenario.Definitions.FacilityTypes,F.TypeId);const auto* Place=Find(Scenario.Definitions.Places,F.CityId);
  F.Employment.Reset();F.PaidUntilHour=Hour;int64 Needed=Type->Jobs;
  const int64 Wage=Type->DailyWageCents*(Scenario.Economy.PayrollHours/24);auto* Account=Find(State.Accounts,ActorAccount(F.OwnerId));
  int64 Affordable=FMath::Min(Needed,(Account->BalanceCents-Account->ReservedCents)/Wage);F.UnfundedJobs=Needed-Affordable;
  if(Scenario.Economy.Stage>=4&&(F.OperatingPaidUntilHour<=Hour||Find(State.Warehouses,F.StorageId)->ArrearsCents)){F.UnfundedJobs=Needed;continue;}
  for(auto& G:State.Groups)
  {
   if(Find(Scenario.Definitions.Groups,G.Id)->RegionId!=Place->RegionId)continue;
   int64 Workers=FMath::Min(Affordable,G.Workforce-G.Employed);if(!Workers)continue;
   if(!Pay(Account->Id,G.AccountId,Workers*Wage,TEXT("wages")))continue;
   F.Employment.Add({G.Id,Workers});G.Employed+=Workers;G.DailyIncomeCents+=Workers*Type->DailyWageCents;Affordable-=Workers;
  }
  if(!F.Employment.IsEmpty())F.PaidUntilHour=Hour+Scenario.Economy.PayrollHours;
 }
}
void FSimulation::Produce(int64 Hour)
{
 TRACE_CPUPROFILER_EVENT_SCOPE(CoI_ProductionCalculate);
 struct FPlan {int32 Index;int64 Batches;};TArray<FPlan> Plans;
 // Every proposal reads the same pre-production state. No output of an earlier loop entry is an input here.
 for(int32 Index=0;Index<State.Facilities.Num();++Index)
 {
  auto& F=State.Facilities[Index];F.LastBatches=0;F.UtilizationPermille=0;
  const auto* T=Find(Scenario.Definitions.FacilityTypes,F.TypeId);const auto* P=Find(Scenario.Definitions.Places,F.CityId);const auto* Site=Find(Scenario.Definitions.ResourceSites,F.ResourceSiteId);const auto* ResourceState=Find(State.ResourceSites,F.ResourceSiteId);
  int64 Workers=0;for(const auto& J:F.Employment)Workers+=J.Workers;
  int64 Suitability=Site?Site->SuitabilityPermille:1000;
  if(Site&&Site->Kind!=TEXT("farmland")&&(!ResourceState||!ResourceState->RemainingKg))continue;
  int64 Denominator=T->Jobs*1000;
  // Counterfactual capacity before liquidity: current workers plus unfunded jobs
  // that could be filled by the region's still available workforce. No jobs or wages are created.
  int64 Available=0;for(const auto& G:State.Groups)if(Find(Scenario.Definitions.Groups,G.Id)->RegionId==P->RegionId)Available+=G.Workforce-G.Employed;
  int64 PlannedWorkers=Workers+FMath::Min(F.UnfundedJobs,Available);
  int64 Planned=(F.ProductionRemainder+T->CapacityBatchesPerHour*PlannedWorkers*Suitability)/Denominator;
  for(const auto& A:T->Inputs)DayMetric(P->RegionId,A.GoodId).InputWantedKg+=A.Kg*Planned;
  if((Scenario.Economy.Stage>=4&&Find(State.Warehouses,F.StorageId)->ArrearsCents)||!Workers||F.PaidUntilHour<=Hour||(Scenario.Economy.Stage>=4&&F.OperatingPaidUntilHour<=Hour))continue;
  F.ProductionRemainder+=T->CapacityBatchesPerHour*Workers*Suitability;
  int64 Batches=F.ProductionRemainder/Denominator;F.ProductionRemainder%=Denominator;
  int64 Net=0;for(const auto& A:T->Inputs){int32 I=StockIndex(F.OwnerId,F.StorageId,A.GoodId);Batches=FMath::Min(Batches,I==INDEX_NONE?0:(State.Inventories[I].QuantityKg-State.Inventories[I].ReservedKg)/FMath::Max(1LL,A.Kg));Net-=A.Kg*Find(Scenario.Definitions.Goods,A.GoodId)->StoragePointsPerKg;}
  int64 ResourcePerBatch=0;for(const auto& A:T->Outputs){Net+=A.Kg*Find(Scenario.Definitions.Goods,A.GoodId)->StoragePointsPerKg;int32 I=StockIndex(F.OwnerId,F.StorageId,A.GoodId);Batches=FMath::Min(Batches,(1000000000-(I==INDEX_NONE?0:State.Inventories[I].QuantityKg))/FMath::Max(1LL,A.Kg));if((T->ResourceRequirement==TEXT("forest")&&A.GoodId==TEXT("good.wood"))||(T->ResourceRequirement==TEXT("ore")&&A.GoodId==TEXT("good.ore")))ResourcePerBatch+=A.Kg;}
  if(Site)Batches=FMath::Min(Batches,F.ResourceBatchesPerHour);if(ResourcePerBatch)Batches=FMath::Min(Batches,ResourceState->RemainingKg/ResourcePerBatch);
  const auto* Store=Find(State.Warehouses,F.StorageId);if(Net>0)Batches=FMath::Min(Batches,(Find(Scenario.Definitions.WarehouseTypes,Store->TypeId)->CapacityPoints-UsedStorage(Store->Id)-Store->ReservedIncomingPoints)/Net);
  if(Batches>0)Plans.Add({Index,Batches});
 }
 for(const auto& Plan:Plans)
 {
  TRACE_CPUPROFILER_EVENT_SCOPE(CoI_ProductionCommit);
  auto& F=State.Facilities[Plan.Index];const auto* T=Find(Scenario.Definitions.FacilityTypes,F.TypeId);int64 B=Plan.Batches;
  // Revalidate shared resources at the deterministic central commit, even for shared company stores.
  for(const auto& A:T->Inputs){int32 I=StockIndex(F.OwnerId,F.StorageId,A.GoodId);B=FMath::Min(B,I==INDEX_NONE?0:(State.Inventories[I].QuantityKg-State.Inventories[I].ReservedKg)/FMath::Max(1LL,A.Kg));}
  auto* Region=Find(State.Regions,Find(Scenario.Definitions.Places,F.CityId)->RegionId);
  int64 Resource=0,Net=0;for(const auto& A:T->Inputs)Net-=A.Kg*Find(Scenario.Definitions.Goods,A.GoodId)->StoragePointsPerKg;
  for(const auto& A:T->Outputs){int32 Existing=StockIndex(F.OwnerId,F.StorageId,A.GoodId);B=FMath::Min(B,(1000000000-(Existing==INDEX_NONE?0:State.Inventories[Existing].QuantityKg))/A.Kg);Net+=A.Kg*Find(Scenario.Definitions.Goods,A.GoodId)->StoragePointsPerKg;if((T->ResourceRequirement==TEXT("forest")&&A.GoodId==TEXT("good.wood"))||(T->ResourceRequirement==TEXT("ore")&&A.GoodId==TEXT("good.ore")))Resource+=A.Kg;}
  auto* ResourceState=Find(State.ResourceSites,F.ResourceSiteId);if(ResourceState){B=FMath::Min(B,F.ResourceBatchesPerHour);if(Resource)B=FMath::Min(B,ResourceState->RemainingKg/Resource);}
  const auto* Store=Find(State.Warehouses,F.StorageId);if(Net>0)B=FMath::Min(B,(Find(Scenario.Definitions.WarehouseTypes,Store->TypeId)->CapacityPoints-UsedStorage(Store->Id)-Store->ReservedIncomingPoints)/Net);if(B<=0)continue;
  int64 InputValue=0;for(const auto& A:T->Inputs){TakeSpoilCredit(StockIndex(F.OwnerId,F.StorageId,A.GoodId),A.Kg*B);InputValue+=TakeStock(StockIndex(F.OwnerId,F.StorageId,A.GoodId),A.Kg*B);Find(State.GoodsAudit,A.GoodId)->InputKg+=A.Kg*B;DayMetric(Region->Id,A.GoodId).InputUsedKg+=A.Kg*B;}
  int64 OutputKg=0;for(const auto& A:T->Outputs)OutputKg+=A.Kg*B;
  int64 Workers=0;for(const auto& J:F.Employment)Workers+=J.Workers;
  int64 WageCost=(Workers*T->DailyWageCents+23)/24;
  for(const auto& A:T->Outputs){int32 I=EnsureStock(F.OwnerId,F.StorageId,A.GoodId);State.Inventories[I].QuantityKg+=A.Kg*B;State.Inventories[I].BookValueCents+=(InputValue+WageCost)*(A.Kg*B)/FMath::Max(1LL,OutputKg);Find(State.GoodsAudit,A.GoodId)->ProducedKg+=A.Kg*B;}
  if(ResourceState&&Resource){ResourceState->RemainingKg-=Resource*B;ResourceState->ExtractedKg+=Resource*B;}
  if(T->ResourceRequirement==TEXT("forest"))Region->RemainingForestKg-=Resource*B;
  if(T->ResourceRequirement==TEXT("ore"))Region->RemainingOreKg-=Resource*B;
  F.LastBatches=B;F.UtilizationPermille=FMath::Min(1000LL,B*1000/T->CapacityBatchesPerHour);
 }
}
void FSimulation::EconomyHour(int64 Hour)
{
 if(Scenario.Economy.Stage<1)return;
 if(Scenario.Economy.Stage>=4){TransportHour(Hour);Costs(Hour);Spoil();}
 Payroll(Hour);Produce(Hour);
 if(Scenario.Economy.Stage>=2)ExpireOrders();
 if(Scenario.Economy.Stage>=4)Companies(Hour);
 if(Scenario.Economy.Stage>=3)Demand(Hour);
 if(Scenario.Economy.Stage>=2)MatchOrders();
 if(Scenario.Economy.Stage>=3)Consume();
 if(Scenario.Economy.Stage>=4){Trader(Hour);MatchOrders();}
}
}

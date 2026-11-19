#include "SimCore.h"
#include "SimArchive.h"
namespace CoISim {
static FAmount& Amount(TArray<FAmount>& Rows,const FString& Good){for(auto& R:Rows)if(R.GoodId==Good)return R;Rows.Add({Good,0});return Rows.Last();}
void FSimulation::Demand(int64 Hour) {
 for(auto& G:State.Groups){const auto* D=Find(Scenario.Definitions.Groups,G.Id);const auto* N=Find(Scenario.Definitions.Needs,D->NeedProfileId);auto* A=Find(State.Accounts,G.AccountId);
  if(Hour%Scenario.Economy.DemandHours==0){
   if(Scenario.Economy.HouseholdModel){G.LastSpentCents=G.DailySpentCents;G.LastIncomeCents=G.DailyIncomeCents;G.NextBudgetHour=Hour+Scenario.Economy.DemandHours;int64 Reference=FMath::Max(1LL,G.Population*N->BasicKgPerThousandPerDay/1000*N->BasicMaxPriceCents);int64 CashDays=FMath::Clamp((A->BalanceCents-A->ReservedCents)/Reference,0LL,30LL);int64 Target=CashDays*N->MaxExtraPermille/30;for(const auto& U:G.UnmetNeed)if(U.GoodId==N->BasicGoodId&&U.Kg>0)Target/=2;G.WealthPermille+=(Target-G.WealthPermille)/N->WealthDays;}for(auto& O:State.Orders)if(O.ActorId==G.Id&&O.Status==TEXT("open"))CancelOrder(O.Id,G.Id,TEXT("day_end"));
   for(const auto& U:G.UnmetNeed)Amount(G.MissedNeed,U.GoodId).Kg+=U.Kg;
   G.BasicRemainder+=G.Population*N->BasicKgPerThousandPerDay;G.LuxuryRemainder+=G.Population*N->LuxuryKgPerThousandPerDay*(1000+(Scenario.Economy.HouseholdModel?G.WealthPermille:0))/1000;
   Amount(G.UnmetNeed,N->BasicGoodId).Kg=G.BasicRemainder/1000;G.BasicRemainder%=1000;Amount(G.UnmetNeed,N->LuxuryGoodId).Kg=G.LuxuryRemainder/1000;G.LuxuryRemainder%=1000;
   DayMetric(D->RegionId,N->BasicGoodId).HouseholdWantedKg+=Amount(G.UnmetNeed,N->BasicGoodId).Kg;DayMetric(D->RegionId,N->LuxuryGoodId).HouseholdWantedKg+=Amount(G.UnmetNeed,N->LuxuryGoodId).Kg;
   G.BudgetCents=(A->BalanceCents-A->ReservedCents)*Scenario.Economy.DailyBudgetPermille/1000;if(Scenario.Economy.HouseholdModel)G.BudgetCents=FMath::Min(G.BudgetCents,(A->BalanceCents-A->ReservedCents)*(1000-N->SavingPermille)/1000);G.DailySpentCents=G.LuxurySpentCents=0;
  }
  if(!Scenario.Economy.LegacyHouseholdPurchases)continue;
  // Basic need is a prerequisite, including when no affordable basic offer exists.
  bool Basic=Amount(G.UnmetNeed,N->BasicGoodId).Kg>0;FString Good=Basic?N->BasicGoodId:N->LuxuryGoodId;int64 Due=Amount(G.UnmetNeed,Good).Kg;
  if(!Due)continue;bool HasOrder=false;for(const auto& O:State.Orders)if(O.ActorId==G.Id&&O.GoodId==Good&&O.Status==TEXT("open"))HasOrder=true;if(HasOrder)continue;
  int64 Limit=Basic?N->BasicMaxPriceCents:N->LuxuryMaxPriceCents;int64 Budget=FMath::Min(G.BudgetCents-G.DailySpentCents,A->BalanceCents-A->ReservedCents);if(!Basic)Budget=FMath::Min(Budget,G.BudgetCents*N->LuxuryBudgetPermille/1000-G.LuxurySpentCents);
  const FMarketOrder* Best=nullptr;for(const auto& O:State.Orders)if(O.Status==TEXT("open")&&O.Side==TEXT("sell")&&O.CityId==D->ConsumptionCityId&&O.GoodId==Good&&O.LimitCentsPerKg<=Limit&&(!Best||O.LimitCentsPerKg<Best->LimitCentsPerKg||(O.LimitCentsPerKg==Best->LimitCentsPerKg&&O.Sequence<Best->Sequence)))Best=&O;
  FString Store=Best?Best->StorageId:TEXT("storage.service.")+D->ConsumptionCityId.RightChop(8);int64 Price=Best?Best->LimitCentsPerKg:Limit;
  int64 Kg=FMath::Min3(Due,Budget/Price,Scenario.Economy.MaxOrderKg);if(Kg<=0)continue;
  FMarketOrder O;O.Id=FString::Printf(TEXT("household.%s.%lld"),*G.Id,State.OrderSequence+1);O.ActorId=G.Id;O.CityId=D->ConsumptionCityId;O.MarketId=Find(Scenario.Definitions.Regions,D->RegionId)->MarketId;O.StorageId=Store;O.GoodId=Good;O.Side=TEXT("buy");O.OriginalKg=Kg;O.LimitCentsPerKg=Price;FString Error;PlaceOrder(O,Error);
 }
}
void FSimulation::Consume() {
 for(auto& G:State.Groups){const auto* D=Find(Scenario.Definitions.Groups,G.Id);const auto* N=Find(Scenario.Definitions.Needs,D->NeedProfileId);
  for(const FString& Good:{N->BasicGoodId,N->LuxuryGoodId}){if(Good==N->LuxuryGoodId&&Amount(G.UnmetNeed,N->BasicGoodId).Kg>0)break;auto& Due=Amount(G.UnmetNeed,Good);
   for(int32 I=0;I<State.Inventories.Num()&&Due.Kg>0;++I){const auto& Stock=State.Inventories[I];const auto* Store=Find(State.Warehouses,Stock.StorageId);if(Stock.OwnerId!=G.Id||Stock.GoodId!=Good||!Store||Store->CityId!=D->ConsumptionCityId)continue;
    int64 Kg=FMath::Min(Due.Kg,Stock.QuantityKg-Stock.ReservedKg);TakeSpoilCredit(I,Kg);TakeStock(I,Kg);Due.Kg-=Kg;Amount(G.Consumed,Good).Kg+=Kg;Find(State.GoodsAudit,Good)->ConsumedKg+=Kg;DayMetric(D->RegionId,Good).HouseholdUsedKg+=Kg;
   }
  }
 }
}
}

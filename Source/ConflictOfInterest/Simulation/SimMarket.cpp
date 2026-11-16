#include "SimCore.h"
#include "SimArchive.h"
namespace CoISim {
bool FSimulation::PlaceOrder(FMarketOrder O,FString& Error) {
 auto Reject=[&](const TCHAR* E){Error=E;return false;};const auto& E=Scenario.Economy;
 auto* A=Find(State.Accounts,ActorAccount(O.ActorId));auto* S=Find(State.Warehouses,O.StorageId);auto* P=Find(Scenario.Definitions.Places,O.CityId);
 int64 Open=0;for(const auto& R:State.Orders)if(R.Status==TEXT("open"))++Open;
 if(!A||!S||!P||S->CityId!=P->Id||Find(Scenario.Definitions.Regions,P->RegionId)->MarketId!=O.MarketId||!Find(Scenario.Definitions.Goods,O.GoodId)||O.Id.IsEmpty()||O.Id.Len()>128||Find(State.Orders,O.Id)||Open>=E.MaxOpenOrders||O.OriginalKg<=0||O.OriginalKg>E.MaxOrderKg||O.LimitCentsPerKg<=0||O.LimitCentsPerKg>E.MaxPriceCents||(O.Side!=TEXT("buy")&&O.Side!=TEXT("sell")))return Reject(TEXT("Invalid order, physical pickup or book limit"));
 if(auto* G=Find(State.Groups,O.ActorId)){if(O.Side==TEXT("sell"))return Reject(TEXT("Households do not resell"));int64 Holds=0;for(const auto& R:State.Orders)if(R.ActorId==G->Id)Holds+=R.ReservedCents;if(O.OriginalKg*O.LimitCentsPerKg>G->BudgetCents-G->DailySpentCents-Holds)return Reject(TEXT("Household budget exhausted"));}
 if(O.Side==TEXT("buy")&&S->ArrearsCents)return Reject(TEXT("Storage arrears"));
 O.ReservedCents=O.ReservedKg=0;O.RemainingKg=O.OriginalKg;
 if(O.Side==TEXT("buy")){int64 Cost=O.OriginalKg*O.LimitCentsPerKg;if(Cost>A->BalanceCents-A->ReservedCents)return Reject(TEXT("Insufficient available money"));O.ReservedCents=Cost;}
 else {int32 I=StockIndex(O.ActorId,O.StorageId,O.GoodId);if(I==INDEX_NONE||O.OriginalKg>State.Inventories[I].QuantityKg-State.Inventories[I].ReservedKg)return Reject(TEXT("Insufficient owned pickup stock"));O.ReservedKg=O.OriginalKg;}
 // Ownership changes at the same warehouse. No incoming capacity or geographic relocation.
 O.Sequence=++State.OrderSequence;O.CreatedStep=State.EconomyHour;O.ExpiresStep=State.EconomyHour+E.OrderLifetimeHours;O.Status=TEXT("open");
 A->ReservedCents+=O.ReservedCents;if(O.ReservedKg)State.Inventories[StockIndex(O.ActorId,O.StorageId,O.GoodId)].ReservedKg+=O.ReservedKg;
 State.Orders.Add(MoveTemp(O));Error.Empty();return true;
}
bool FSimulation::CancelOrder(const FString& Id,const FString& Actor,const FString& Status) {
 auto* O=Find(State.Orders,Id);if(!O||O->ActorId!=Actor||O->Status!=TEXT("open"))return false;
 Find(State.Accounts,ActorAccount(Actor))->ReservedCents-=O->ReservedCents;
 if(O->ReservedKg)State.Inventories[StockIndex(Actor,O->StorageId,O->GoodId)].ReservedKg-=O->ReservedKg;
 O->ReservedCents=O->ReservedKg=0;O->Status=Status;return true;
}
void FSimulation::ExpireOrders() {
 for(auto& O:State.Orders)if(O.Status==TEXT("open")&&O.ExpiresStep<=State.EconomyHour)CancelOrder(O.Id,O.ActorId,TEXT("expired"));
 int32 Closed=0;for(const auto& O:State.Orders)if(O.Status!=TEXT("open"))++Closed;
 for(int32 I=0;Closed>Scenario.Economy.HistoryLimit&&I<State.Orders.Num();)if(State.Orders[I].Status!=TEXT("open")){State.Orders.RemoveAt(I,1,EAllowShrinking::No);--Closed;}else ++I;
}
void FSimulation::MatchOrders() {
 TArray<int32> Buys,Sells;for(int32 I=0;I<State.Orders.Num();++I)if(State.Orders[I].Status==TEXT("open"))(State.Orders[I].Side==TEXT("buy")?Buys:Sells).Add(I);
 Buys.Sort([&](int32 A,int32 B){const auto& X=State.Orders[A];const auto& Y=State.Orders[B];return X.LimitCentsPerKg==Y.LimitCentsPerKg?X.Sequence<Y.Sequence:X.LimitCentsPerKg>Y.LimitCentsPerKg;});
 Sells.Sort([&](int32 A,int32 B){const auto& X=State.Orders[A];const auto& Y=State.Orders[B];return X.LimitCentsPerKg==Y.LimitCentsPerKg?X.Sequence<Y.Sequence:X.LimitCentsPerKg<Y.LimitCentsPerKg;});
 for(int32 BI:Buys)for(int32 SI:Sells){auto& B=State.Orders[BI];auto& S=State.Orders[SI];if(!B.RemainingKg||!S.RemainingKg||B.ActorId==S.ActorId||B.MarketId!=S.MarketId||B.GoodId!=S.GoodId||B.StorageId!=S.StorageId||B.LimitCentsPerKg<S.LimitCentsPerKg)continue;
  int64 Kg=FMath::Min(B.RemainingKg,S.RemainingKg),Price=B.Sequence<S.Sequence?B.LimitCentsPerKg:S.LimitCentsPerKg,Cost=Kg*Price;
  auto* BA=Find(State.Accounts,ActorAccount(B.ActorId));BA->ReservedCents-=Kg*B.LimitCentsPerKg;B.ReservedCents-=Kg*B.LimitCentsPerKg;
  const bool Paid=Pay(BA->Id,ActorAccount(S.ActorId),Cost,TEXT("trade"));check(Paid);
  if(auto* G=Find(State.Groups,B.ActorId)){G->DailySpentCents+=Cost;if(Find(Scenario.Definitions.Goods,B.GoodId)->NeedCategory==TEXT("luxury"))G->LuxurySpentCents+=Cost;}
  int32 Source=StockIndex(S.ActorId,S.StorageId,S.GoodId);State.Inventories[Source].ReservedKg-=Kg;S.ReservedKg-=Kg;
  int64 Value=TakeStock(Source,Kg);int32 Dest=EnsureStock(B.ActorId,S.StorageId,S.GoodId);State.Inventories[Dest].QuantityKg+=Kg;State.Inventories[Dest].BookValueCents+=Cost;State.Inventories[Dest].AcquiredHour=State.EconomyHour;
  (void)Value;B.RemainingKg-=Kg;S.RemainingKg-=Kg;if(!B.RemainingKg)B.Status=TEXT("filled");if(!S.RemainingKg)S.Status=TEXT("filled");
  FTrade T;T.Id=FString::Printf(TEXT("trade.%lld"),++State.TradeSequence);T.BuyOrderId=B.Id;T.SellOrderId=S.Id;T.BuyerId=B.ActorId;T.SellerId=S.ActorId;T.MarketId=B.MarketId;T.CityId=S.CityId;T.GoodId=B.GoodId;T.Step=State.EconomyHour;T.QuantityKg=Kg;T.PriceCentsPerKg=Price;T.TotalCents=Cost;State.Trades.Add(MoveTemp(T));
  if(State.Trades.Num()>Scenario.Economy.HistoryLimit)State.Trades.RemoveAt(0,1,EAllowShrinking::No);
 }
}
}

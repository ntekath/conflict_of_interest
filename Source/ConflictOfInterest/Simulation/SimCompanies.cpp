#include "SimCore.h"
#include "SimArchive.h"
namespace CoISim {
void FSimulation::Offer(const FString& Actor,const FString& Store,const FString& Good,const FString& Side,int64 Kg,int64 Price) {
 if(!IsAutonomous(Actor)||Kg<=0||Price<=0)return;for(const auto& O:State.Orders)if(O.Status==TEXT("open")&&O.ActorId==Actor&&O.StorageId==Store&&O.GoodId==Good&&O.Side==Side)return;auto* A=Find(State.Accounts,ActorAccount(Actor));if(Side==TEXT("buy"))Kg=FMath::Min(Kg,(A->BalanceCents-A->ReservedCents)/Price);FString E;OperatingOrder(Actor,Store,Good,Side,Kg,Price,TEXT(""),E);
}
void FSimulation::Companies(int64 H) {
 if(H%Scenario.Economy.DecisionHours)return;
 for(auto& O:State.Orders){const auto* C=Find(Scenario.Definitions.Companies,O.ActorId);if(IsAutonomous(O.ActorId)&&C&&C->DecisionProfile==TEXT("trader")&&O.RuleId.IsEmpty()&&O.Status==TEXT("open"))CancelOrder(O.Id,O.ActorId,TEXT("repriced"));}
 for(const auto& Stock:State.Inventories){const auto* C=Find(Scenario.Definitions.Companies,Stock.OwnerId);if(!IsAutonomous(Stock.OwnerId)||!C||C->DecisionProfile!=TEXT("trader")||Stock.QuantityKg<=Stock.ReservedKg||!Find(State.Warehouses,Stock.StorageId)||Stock.RentArrearsCents)continue;
  int64 Unit=(Stock.BookValueCents+Stock.QuantityKg-1)/Stock.QuantityKg;int64 Price=FMath::Max(1LL,(Unit*(1000+Scenario.Economy.TraderMarkupPermille)+999)/1000);Offer(Stock.OwnerId,Stock.StorageId,Stock.GoodId,TEXT("sell"),Stock.QuantityKg-Stock.ReservedKg,Price);
 }
}
void FSimulation::Trader(int64 H) {
 if(H%Scenario.Economy.DecisionHours)return;
 for(const auto& Company:Scenario.Definitions.Companies){if(!IsAutonomous(Company.Id)||Company.DecisionProfile!=TEXT("trader"))continue;const FString Actor=Company.Id;auto* Cash=Find(State.Accounts,ActorAccount(Actor));
  // Pick an observed destination bid for already owned stock. Prefer a paid carrier over own vehicle.
  for(int32 I=0;I<State.Inventories.Num();++I){const FInventory Stock=State.Inventories[I];const auto* From=Find(State.Warehouses,Stock.StorageId);if(Stock.OwnerId!=Actor||!From||Stock.QuantityKg<=0||Stock.RentArrearsCents)continue;
   // Own asks must release cargo before evaluating physical availability.
   for(auto& O:State.Orders)if(O.ActorId==Actor&&O.Side==TEXT("sell")&&O.StorageId==Stock.StorageId&&O.GoodId==Stock.GoodId&&O.Status==TEXT("open"))CancelOrder(O.Id,Actor,TEXT("route_evaluation"));
   const auto Current=State.Inventories[I];int64 BestProfit=Scenario.Economy.TraderMinProfitCents-1;FTransport Best;
   for(const auto& Bid:State.Orders){if(Bid.Status!=TEXT("open")||Bid.Side!=TEXT("buy")||Bid.ActorId==Actor||Bid.GoodId!=Stock.GoodId||Bid.StorageId==Stock.StorageId)continue;
    for(int Pass=0;Pass<2;++Pass){bool Found=false;for(const auto& V:State.Vehicles){const auto* Owner=Find(Scenario.Definitions.Companies,V.OwnerId);if(V.OwnerId==Session.PlayerCompanyId||V.Bound||V.ArrearsCents||V.CityId!=From->CityId||(Pass==0?Owner->DecisionProfile!=TEXT("carrier"):V.OwnerId!=Actor))continue;const auto* VT=Find(Scenario.Definitions.VehicleTypes,V.TypeId);
     int64 Kg=FMath::Min3(Current.QuantityKg-Current.ReservedKg,Bid.RemainingKg,FMath::Min(VT->CapacityKg,Scenario.Economy.TraderMaxLoadKg));const auto* Dest=Find(State.Warehouses,Bid.StorageId);Kg=FMath::Min(Kg,(Find(Scenario.Definitions.WarehouseTypes,Dest->TypeId)->CapacityPoints-UsedStorage(Dest->Id)-Dest->ReservedIncomingPoints)/Find(Scenario.Definitions.Goods,Stock.GoodId)->StoragePointsPerKg);if(Kg<=0)continue;
     int64 Op=RouteKm(From->CityId,Bid.CityId)*VT->CostCentsPerKm;int64 Fee=V.OwnerId==Actor?Op:(Op*(1000+Scenario.Economy.FreightMarkupPermille)+999)/1000;int64 Value=Current.BookValueCents/Current.QuantityKg*Kg+(Current.BookValueCents%Current.QuantityKg)*Kg/Current.QuantityKg;int64 Profit=Kg*Bid.LimitCentsPerKg-Value-Fee;
     if(Profit>BestProfit&&Fee<=Cash->BalanceCents-Cash->ReservedCents){BestProfit=Profit;Best.Id=FString::Printf(TEXT("haul.%s.%lld.%s"),*Actor,H,*V.Id);Best.OwnerId=Actor;Best.VehicleId=V.Id;Best.FromCityId=From->CityId;Best.ToCityId=Bid.CityId;Best.SourceInventoryId=Stock.Id;Best.DestinationStorageId=Bid.StorageId;Best.Kg=Kg;Found=true;}
    }if(Found)break;}
   }
   if(!Best.Id.IsEmpty()){FString Error;BookTransport(Best,Error);}
  }
  // Purchase only from visible asks with a visible profitable bid elsewhere and a finite local vehicle.
  FMarketOrder Buy;int64 BestProfit=Scenario.Economy.TraderMinProfitCents-1;
  for(const auto& Ask:State.Orders){if(Ask.Status!=TEXT("open")||Ask.Side!=TEXT("sell")||Ask.ActorId==Actor)continue;
   for(const auto& Bid:State.Orders){if(Bid.Status!=TEXT("open")||Bid.Side!=TEXT("buy")||Bid.ActorId==Actor||Bid.GoodId!=Ask.GoodId||Bid.StorageId==Ask.StorageId)continue;
    for(const auto& V:State.Vehicles){const auto* Owner=Find(Scenario.Definitions.Companies,V.OwnerId);if(V.OwnerId==Session.PlayerCompanyId||V.Bound||V.ArrearsCents||V.CityId!=Ask.CityId||(Owner->DecisionProfile!=TEXT("carrier")&&V.OwnerId!=Actor))continue;const auto* VT=Find(Scenario.Definitions.VehicleTypes,V.TypeId);int64 Kg=FMath::Min3(Ask.RemainingKg,Bid.RemainingKg,FMath::Min(Scenario.Economy.TraderMaxLoadKg,VT->CapacityKg));int64 Op=RouteKm(Ask.CityId,Bid.CityId)*VT->CostCentsPerKm;int64 Fee=V.OwnerId==Actor?Op:(Op*(1000+Scenario.Economy.FreightMarkupPermille)+999)/1000;
     int64 Budget=Cash->BalanceCents-Cash->ReservedCents-Scenario.Economy.TraderCashReserveCents-Fee;Kg=FMath::Min(Kg,FMath::Max(0LL,Budget)/Ask.LimitCentsPerKg);int64 Profit=Kg*(Bid.LimitCentsPerKg-Ask.LimitCentsPerKg)-Fee;if(Kg>0&&Profit>BestProfit){BestProfit=Profit;Buy=Ask;Buy.RuleId.Empty();Buy.ActorId=Actor;Buy.Side=TEXT("buy");Buy.OriginalKg=Kg;Buy.Id=FString::Printf(TEXT("merchant.buy.%lld"),State.OrderSequence+1);}
    }
   }
  }
  if(!Buy.Id.IsEmpty()){FString Error;PlaceOrder(Buy,Error);}
 }
 // Idle fleet repositions explicitly, at its owner's expense, towards observed supply/demand spreads.
 for(const auto& V:State.Vehicles){const auto* Owner=Find(Scenario.Definitions.Companies,V.OwnerId);if(V.OwnerId==Session.PlayerCompanyId||V.Bound||V.ArrearsCents||(Owner->DecisionProfile!=TEXT("carrier")&&Owner->DecisionProfile!=TEXT("trader")))continue;const auto* VT=Find(Scenario.Definitions.VehicleTypes,V.TypeId);FString City;int64 Best=Scenario.Economy.TraderMinProfitCents-1;
  for(const auto& Ask:State.Orders)if(Ask.Status==TEXT("open")&&Ask.Side==TEXT("sell"))for(const auto& Bid:State.Orders)if(Bid.Status==TEXT("open")&&Bid.Side==TEXT("buy")&&Bid.GoodId==Ask.GoodId&&Bid.StorageId!=Ask.StorageId){int64 Kg=FMath::Min3(Ask.RemainingKg,Bid.RemainingKg,VT->CapacityKg);int64 Score=Kg*(Bid.LimitCentsPerKg-Ask.LimitCentsPerKg)-(RouteKm(V.CityId,Ask.CityId)+RouteKm(Ask.CityId,Bid.CityId))*VT->CostCentsPerKm;if(Score>Best){Best=Score;City=Ask.CityId;}}
  if(!City.IsEmpty()&&City!=V.CityId){FTransport J;J.Id=FString::Printf(TEXT("empty.%s.%lld"),*V.Id,H);J.OwnerId=V.OwnerId;J.VehicleId=V.Id;J.FromCityId=V.CityId;J.ToCityId=City;J.Empty=true;FString Error;BookTransport(J,Error);}
 }
}
}

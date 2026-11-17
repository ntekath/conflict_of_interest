#include "SimCore.h"
#include "SimArchive.h"
namespace CoISim {
void FSimulation::Costs(int64 H) {
 if(H%Scenario.Economy.CostsHours)return;
 for(auto& S:State.Warehouses){int64 Due=Find(Scenario.Definitions.WarehouseTypes,S.TypeId)->DailyCostCents+S.ArrearsCents;S.ArrearsCents=Pay(ActorAccount(S.OwnerId),S.CostRecipientAccountId,Due,TEXT("storage_fixed"))?0:Due;}
 for(auto& V:State.Vehicles){int64 Due=Find(Scenario.Definitions.VehicleTypes,V.TypeId)->DailyCostCents+V.ArrearsCents;V.ArrearsCents=Pay(ActorAccount(V.OwnerId),V.CostRecipientAccountId,Due,TEXT("vehicle_fixed"))?0:Due;}
 for(auto& F:State.Facilities){int64 Due=Find(Scenario.Definitions.FacilityTypes,F.TypeId)->DailyOperatingCostCents+F.OperatingArrearsCents;bool Paid=Pay(ActorAccount(F.OwnerId),F.CostRecipientAccountId,Due,TEXT("production_operation"));F.OperatingArrearsCents=Paid?0:Due;F.OperatingPaidUntilHour=Paid?H+Scenario.Economy.CostsHours:H;}
 for(auto& I:State.Inventories){auto* S=Find(State.Warehouses,I.StorageId);if(!S||S->OwnerId==I.OwnerId||H-I.AcquiredHour<Scenario.Economy.PickupGraceHours)continue;
  int64 Due=I.QuantityKg*Find(Scenario.Definitions.Goods,I.GoodId)->StoragePointsPerKg*Scenario.Economy.RentalCentsPerPointDay+I.RentArrearsCents;I.RentArrearsCents=Pay(ActorAccount(I.OwnerId),ActorAccount(S->OwnerId),Due,TEXT("foreign_storage"))?0:Due;
 }
}
void FSimulation::Spoil() {
 for(int32 Index=0;Index<State.Inventories.Num();++Index){auto& I=State.Inventories[Index];int64 Rate=Find(Scenario.Definitions.Goods,I.GoodId)->SpoilPartsPerMillionPerDay;if(!Rate||!I.QuantityKg)continue;
  I.SpoilRemainder+=I.QuantityKg*Rate;int64 Lost=I.SpoilRemainder/24000000;I.SpoilRemainder%=24000000;if(!Lost)continue;
  // Cancel reservations that would otherwise refer to lost physical stock, before applying spoilage.
  if(Lost>I.QuantityKg-I.ReservedKg){for(auto& O:State.Orders)if(O.Status==TEXT("open")&&O.Side==TEXT("sell")&&O.ActorId==I.OwnerId&&O.StorageId==I.StorageId&&O.GoodId==I.GoodId)CancelOrder(O.Id,O.ActorId,TEXT("spoilage"));
   for(auto& J:State.Transports)if(J.Status==TEXT("planned")&&J.SourceInventoryId==I.Id){CancelTransport(J.Id,J.OwnerId);if(J.OwnerId==Session.PlayerCompanyId)PlayerEvent(TEXT("transport"),J.Id,TEXT("Geplanter Transport wegen Verderb der Quellware aufgehoben. Reservierungen wurden freigegeben; verbliebene Ware bleibt im Quelllager."));}
   for(auto& R:State.Reservations)if(R.Active&&R.InventoryId==I.Id){FCommand C;C.Kind=ECommandKind::Release;C.Resources=R;FString Error;Apply(C,Error);}
  }
  TakeStock(Index,Lost);Find(State.GoodsAudit,I.GoodId)->SpoiledKg+=Lost;
  if(I.OwnerId==Session.PlayerCompanyId){auto& Rows=PlayerDay().Spoiled;auto* Row=Rows.FindByPredicate([&](const FAmount& A){return A.GoodId==I.GoodId;});if(!Row){Rows.Add({I.GoodId,0});Row=&Rows.Last();}Row->Kg+=Lost;}
 }
}
}

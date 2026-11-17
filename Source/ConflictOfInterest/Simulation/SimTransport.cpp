#include "SimCore.h"
#include "SimArchive.h"
#include "GeoTransform.h"
#include "PlayerView.h"
namespace CoISim {
int64 FSimulation::RouteKm(const FString& From,const FString& To) const {
 const auto* A=Find(Scenario.Definitions.Places,From);const auto* B=Find(Scenario.Definitions.Places,To);if(!A||!B)return 0;
 return From==To?Scenario.Economy.LocalHaulKm:FMath::Max(1LL,(int64)FMath::CeilToDouble(CoIGeo::DistanceKm(A->Latitude,A->Longitude,B->Latitude,B->Longitude)*Scenario.Economy.RouteDetourPermille/1000.));
}
bool FSimulation::BookTransport(FTransport J,FString& Error) {
 auto Fail=[&](const TCHAR* M){Error=M;return false;};auto* V=Find(State.Vehicles,J.VehicleId);auto* A=Find(State.Accounts,ActorAccount(J.OwnerId));auto* Source=Find(State.Inventories,J.SourceInventoryId);auto* Dest=Find(State.Warehouses,J.DestinationStorageId);
 if(!V||!A||J.Id.IsEmpty()||J.Id.Len()>128||Find(State.Transports,J.Id)||V->Bound||V->ArrearsCents||V->CityId!=J.FromCityId||!Find(Scenario.Definitions.Places,J.ToCityId))return Fail(TEXT("Unavailable vehicle or invalid route"));
 const auto* VT=Find(Scenario.Definitions.VehicleTypes,V->TypeId);J.ProviderId=V->OwnerId;
 if(J.OwnerId!=J.ProviderId&&Find(Scenario.Definitions.Companies,J.ProviderId)->DecisionProfile!=TEXT("carrier"))return Fail(TEXT("Vehicle is not offered by a carrier"));
 if(J.Empty){if(J.OwnerId!=V->OwnerId||J.FromCityId==J.ToCityId)return Fail(TEXT("Empty reposition must be paid by vehicle owner"));J.Kg=0;J.ReservedIncomingPoints=0;}
 else {const auto* Store=Source?Find(State.Warehouses,Source->StorageId):nullptr;const auto* Good=Source?Find(Scenario.Definitions.Goods,Source->GoodId):nullptr;
  if(!Source||!Store||!Good||Source->OwnerId!=J.OwnerId||Store->CityId!=J.FromCityId||Source->RentArrearsCents||!Dest||Dest->CityId!=J.ToCityId||Dest->ArrearsCents||Source->StorageId==Dest->Id||J.Kg<=0||J.Kg>VT->CapacityKg||J.Kg>Source->QuantityKg-Source->ReservedKg)return Fail(TEXT("Invalid cargo, arrears, capacity or source ownership"));
  // Private receiving sites accept incoming stock only when their owner has a public bid for this good.
  bool Access=Dest->OwnerId==J.OwnerId||Dest->OwnerId==TEXT("company.services");for(const auto& O:State.Orders)if(O.Status==TEXT("open")&&O.Side==TEXT("buy")&&O.ActorId==Dest->OwnerId&&O.StorageId==Dest->Id&&O.GoodId==Source->GoodId)Access=true;
  if(!Access)return Fail(TEXT("No destination access"));
  int32 Existing=StockIndex(J.OwnerId,Dest->Id,Source->GoodId);int64 Incoming=0;for(const auto& Other:State.Transports)if((Other.Status==TEXT("planned")||Other.Status==TEXT("moving"))&&Other.OwnerId==J.OwnerId&&Other.DestinationStorageId==Dest->Id&&Other.GoodId==Source->GoodId)Incoming+=Other.Kg;
  if(J.Kg>1000000000-(Existing==INDEX_NONE?0:State.Inventories[Existing].QuantityKg)-Incoming)return Fail(TEXT("Inventory quantity bound"));J.GoodId=Source->GoodId;J.ReservedIncomingPoints=J.Kg*Good->StoragePointsPerKg;
  if(J.ReservedIncomingPoints>Find(Scenario.Definitions.WarehouseTypes,Dest->TypeId)->CapacityPoints-UsedStorage(Dest->Id)-Dest->ReservedIncomingPoints)return Fail(TEXT("Destination full"));
 }
 const auto Terms=TransportTerms(Scenario.Definitions,Scenario.Economy,J.FromCityId,J.ToCityId,*VT,J.OwnerId==J.ProviderId,State.EconomyHour,J.RequestedDepartureHour);
 if(!Terms.Km)return Fail(TEXT("No route"));
 if(Terms.Departure<State.EconomyHour+Scenario.Economy.BookingLeadHours||Terms.Departure>State.EconomyHour+Scenario.Economy.MaxBookingDelayHours)return Fail(TEXT("Departure outside booking window"));
 J.RouteKm=Terms.Km;J.OperatingCents=Terms.Operation;J.FreightCents=Terms.Freight;J.CostCents=Terms.Cost;
 auto* P=Find(State.Accounts,ActorAccount(J.ProviderId));int64 Need=J.OwnerId==J.ProviderId?J.OperatingCents:J.FreightCents;
 if(!P||Need>A->BalanceCents-A->ReservedCents||(J.OwnerId!=J.ProviderId&&J.OperatingCents>P->BalanceCents-P->ReservedCents))return Fail(TEXT("Unfunded shipper or provider"));
 int64 Active=0;for(const auto& T:State.Transports)if(T.Status==TEXT("planned")||T.Status==TEXT("moving"))++Active;if(Active>=State.Vehicles.Num())return Fail(TEXT("Finite fleet full"));
 // Commit only after ALL predicates; source, destination, provider and payer become bound together.
 A->ReservedCents+=Need;if(J.OwnerId!=J.ProviderId)P->ReservedCents+=J.OperatingCents;
 if(!J.Empty){Source->ReservedKg+=J.Kg;Dest->ReservedIncomingPoints+=J.ReservedIncomingPoints;}
 V->Bound=true;V->TransportId=J.Id;J.Status=TEXT("planned");J.Service=J.OwnerId==J.ProviderId?TEXT("own_truck"):TEXT("rented_truck");J.DepartureStep=Terms.Departure;J.ArrivalStep=Terms.Arrival;
 State.Transports.Add(MoveTemp(J));Error.Empty();return true;
}
bool FSimulation::CancelTransport(const FString& Id,const FString& Actor) {
 auto* J=Find(State.Transports,Id);if(!J||J->OwnerId!=Actor||J->Status!=TEXT("planned"))return false;
 Find(State.Accounts,ActorAccount(Actor))->ReservedCents-=J->CostCents;if(Actor!=J->ProviderId)Find(State.Accounts,ActorAccount(J->ProviderId))->ReservedCents-=J->OperatingCents;
 if(!J->Empty){Find(State.Inventories,J->SourceInventoryId)->ReservedKg-=J->Kg;Find(State.Warehouses,J->DestinationStorageId)->ReservedIncomingPoints-=J->ReservedIncomingPoints;}
 auto* V=Find(State.Vehicles,J->VehicleId);V->Bound=false;V->TransportId.Empty();J->Status=TEXT("cancelled");return true;
}
void FSimulation::TransportHour(int64 Hour) {
 for(auto& J:State.Transports){auto* V=Find(State.Vehicles,J.VehicleId);
  if(J.Status==TEXT("planned")&&J.DepartureStep<=Hour){auto* A=Find(State.Accounts,ActorAccount(J.OwnerId));auto* P=Find(State.Accounts,ActorAccount(J.ProviderId));A->ReservedCents-=J.CostCents;if(J.OwnerId!=J.ProviderId){P->ReservedCents-=J.OperatingCents;const bool FreightPaid=Pay(A->Id,P->Id,J.FreightCents,TEXT("freight"));check(FreightPaid);}const bool OperationPaid=Pay(P->Id,V->CostRecipientAccountId,J.OperatingCents,TEXT("haul_operation"));check(OperationPaid);
   if(!J.Empty){int32 I=State.Inventories.IndexOfByPredicate([&](const FInventory& X){return X.Id==J.SourceInventoryId;});State.Inventories[I].ReservedKg-=J.Kg;int64 SpoilCredit=TakeSpoilCredit(I,J.Kg);int64 Value=TakeStock(I,J.Kg);FInventory C;C.Id=TEXT("cargo.")+J.Id;C.OwnerId=J.OwnerId;C.GoodId=J.GoodId;C.StorageId=C.Id;C.SpoilRemainder=SpoilCredit;C.QuantityKg=J.Kg;C.BookValueCents=Value+J.CostCents;J.CargoInventoryId=C.Id;State.Inventories.Add(MoveTemp(C));}
   J.Status=TEXT("moving");V->CityId.Empty();
  }
  if(J.Status==TEXT("moving")&&J.ArrivalStep<=Hour){if(!J.Empty){int32 I=State.Inventories.IndexOfByPredicate([&](const FInventory& X){return X.Id==J.CargoInventoryId;});FInventory Cargo=State.Inventories[I];State.Inventories.RemoveAt(I);int32 D=EnsureStock(J.OwnerId,J.DestinationStorageId,J.GoodId);if(!State.Inventories[D].QuantityKg)State.Inventories[D].AcquiredHour=Hour;State.Inventories[D].SpoilRemainder+=Cargo.SpoilRemainder;State.Inventories[D].QuantityKg+=Cargo.QuantityKg;State.Inventories[D].BookValueCents+=Cargo.BookValueCents;Find(State.Warehouses,J.DestinationStorageId)->ReservedIncomingPoints-=J.ReservedIncomingPoints;}
   J.Status=TEXT("arrived");if(J.OwnerId==Scenario.PlayerCompanyId)PlayerEvent(TEXT("transport"),J.Id,TEXT("Lieferung angekommen. Fahrzeug und verbleibende Ware stehen am Ziel bereit."));V->CityId=J.ToCityId;V->Bound=false;V->TransportId.Empty();
  }
 }
 int32 Closed=0;for(const auto& J:State.Transports)if(J.Status!=TEXT("planned")&&J.Status!=TEXT("moving"))++Closed;
 for(int32 I=0;Closed>Scenario.Economy.HistoryLimit&&I<State.Transports.Num();)if(State.Transports[I].Status!=TEXT("planned")&&State.Transports[I].Status!=TEXT("moving")){State.Transports.RemoveAt(I);--Closed;}else ++I;
}
}

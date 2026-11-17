#include "PlayerView.h"
#include "SimCore.h"
#include "SimArchive.h"
#include "GeoTransform.h"
namespace CoISim {
FTransportTerms TransportTerms(const FDefinitions& D,const FEconomyRules& E,const FString& From,const FString& To,const FVehicleDef& V,bool Own,int64 Hour,int64 Requested) {
 FTransportTerms T;const auto* A=Find(D.Places,From);const auto* B=Find(D.Places,To);if(!A||!B||V.SpeedKmPerHour<=0)return T;
 T.Km=From==To?E.LocalHaulKm:FMath::Max(1LL,(int64)FMath::CeilToDouble(CoIGeo::DistanceKm(A->Latitude,A->Longitude,B->Latitude,B->Longitude)*E.RouteDetourPermille/1000.));
 T.Duration=(T.Km+V.SpeedKmPerHour-1)/V.SpeedKmPerHour;T.Departure=Requested?Requested:Hour+E.BookingLeadHours;T.Arrival=T.Departure+T.Duration;T.Operation=T.Km*V.CostCentsPerKm;T.Freight=Own?0:(T.Operation*(1000+E.FreightMarkupPermille)+999)/1000;T.Cost=Own?T.Operation:T.Freight;return T;
}
FString PlayerReason(const FString& R) {
 if(R.Contains(TEXT("Destination full"))||R.Contains(TEXT("destination capacity")))return TEXT("Im Ziellager ist nicht genügend freie Kapazität vorhanden. Es wurde nichts reserviert.");
 if(R.Contains(TEXT("Invalid cargo")))return TEXT("Ware ist nicht verfügbar, bereits reserviert oder nicht an diesem Abholort. Prüfen Sie auch Lagerzugang und Lkw-Kapazität.");
 if(R.Contains(TEXT("Unavailable vehicle")))return TEXT("Der Lkw ist inzwischen belegt, gesperrt oder nicht am Startort.");
 if(R.Contains(TEXT("Unfunded"))||R.Contains(TEXT("money"))||R.Contains(TEXT("funds")))return TEXT("Verfügbares Guthaben des Auftraggebers oder Anbieters reicht nicht aus. Reserviertes Geld kann nicht erneut verwendet werden.");
 if(R.Contains(TEXT("pickup stock"))||R.Contains(TEXT("local stock")))return TEXT("Nicht genügend eigene, unreservierte Ware am ausgewählten Quelllager.");
 if(R.Contains(TEXT("Departure")))return TEXT("Die geplante Abfahrt liegt inzwischen in der Vergangenheit oder außerhalb des Buchungsfensters. Vorschau aktualisieren.");
 if(R.Contains(TEXT("cancellable")))return TEXT("Dieser Transport ist bereits abgefahren, abgeschlossen oder gehört nicht zur Firma. Nur geplante Transporte können storniert werden.");
 if(R.Contains(TEXT("owned order")))return TEXT("Dieser Marktauftrag ist bereits beendet oder gehört nicht zur Firma.");
 if(R.Contains(TEXT("arrears")))return TEXT("Offene Lagergebühren sperren den Auftrag. In der Lageransicht stehen die Rückstände.");
 if(R.Contains(TEXT("destination access")))return TEXT("Kein Zugang zum Ziellager. Nutzen Sie ein eigenes Lager, ein öffentliches Mietlager oder einen Standort mit passendem Kaufgebot.");
 if(R.Contains(TEXT("generation")))return TEXT("Das Szenario wurde neu geladen. Bitte öffnen Sie eine neue Vorschau.");
 if(R.Contains(TEXT("queue")))return TEXT("Die Auftragswarteschlange ist belegt. Bitte später erneut bestätigen.");
 if(R.Contains(TEXT("duplicate"),ESearchCase::IgnoreCase))return TEXT("Diese Bestätigung wurde bereits gesendet.");
 if(R.Contains(TEXT("route")))return TEXT("Keine gültige Verbindung zwischen den ausgewählten Orten.");
 return TEXT("Auftrag nicht möglich. Auswahl, positive ganze kg, Preislimit, Eigentum, freie Mittel und verfügbare Kapazität prüfen.");
}
FPlayerDay& FSimulation::PlayerDay() {
 const int64 Day=State.EconomyHour/24;for(auto& D:State.PlayerDays)if(D.Day==Day)return D;
 FPlayerDay D;D.Day=Day;D.Id=FString::Printf(TEXT("day.%lld"),Day);State.PlayerDays.Add(D);
 while(State.PlayerDays.Num()>Scenario.Economy.UiHistoryDays)State.PlayerDays.RemoveAt(0);return State.PlayerDays.Last();
}
void FSimulation::PlayerEvent(const FString& Kind,const FString& Object,const FString& Text) {
 FString Key=Kind+TEXT("|")+Object;int32 Index=State.PlayerEvents.IndexOfByPredicate([&](const FPlayerEvent& E){return E.Id==Key;});FPlayerEvent E;
 if(Index!=INDEX_NONE){E=State.PlayerEvents[Index];if(E.Text==Text)return;State.PlayerEvents.RemoveAt(Index);++E.Count;}
 E.Id=Key;E.Kind=Kind;E.ObjectId=Object;E.Text=Text;E.Hour=State.EconomyHour;State.PlayerEvents.Add(MoveTemp(E));
 while(State.PlayerEvents.Num()>Scenario.Economy.UiEventLimit)State.PlayerEvents.RemoveAt(0);
}
void FSimulation::PlayerWarnings() {
 const auto* A=Find(State.Accounts,ActorAccount(Session.PlayerCompanyId));if(A->BalanceCents-A->ReservedCents<=Scenario.Economy.UiLowCashCents)PlayerEvent(TEXT("company"),Session.PlayerCompanyId,TEXT("Liquidität knapp: wenig frei verfügbares Geld. Reservierungen und laufende Kosten prüfen."));
 for(const auto& S:State.Warehouses)if(S.OwnerId==Session.PlayerCompanyId){int64 Cap=Find(Scenario.Definitions.WarehouseTypes,S.TypeId)->CapacityPoints;int64 Free=Cap-UsedStorage(S.Id)-S.ReservedIncomingPoints;if(Free*1000<=Cap*Scenario.Economy.UiCapacityWarningPermille)PlayerEvent(TEXT("storage"),S.Id,TEXT("Lagerkapazität knapp oder erschöpft. Freie und reservierte Zielkapazität prüfen."));}
}
void FSimulation::PublishPlayer(FSnapshot& Out) const {
 auto& P=Out.Player;P.ActorId=Session.PlayerCompanyId;P.StartDate=Scenario.StartDate;P.Hour=State.EconomyHour;P.Catalog=Scenario.Definitions;P.Rules=Scenario.Economy;P.Days=State.PlayerDays;P.Events=State.PlayerEvents;
 // No competitor account balances, private stocks or individual freight jobs cross the HUD boundary.
 for(const auto& S:State.Warehouses){FStoreView V;V.Id=S.Id;V.CityId=S.CityId;V.Own=S.OwnerId==P.ActorId;V.Public=S.OwnerId==TEXT("company.services");V.OwnerName=Find(Scenario.Definitions.Companies,S.OwnerId)->Name;V.Name=(V.Own?TEXT("Eigenes Lager"):V.Public?TEXT("Mietlager"):TEXT("Betriebslager"))+FString(TEXT(" · "))+Find(Scenario.Definitions.Places,S.CityId)->Name+(V.Own||V.Public?TEXT(""):TEXT(" · ")+V.OwnerName);V.Capacity=Find(Scenario.Definitions.WarehouseTypes,S.TypeId)->CapacityPoints;V.DailyCost=Find(Scenario.Definitions.WarehouseTypes,S.TypeId)->DailyCostCents;V.Used=UsedStorage(S.Id);V.Incoming=S.ReservedIncomingPoints;V.Blocked=S.ArrearsCents>0;V.Arrears=V.Own?S.ArrearsCents:0;
  for(const auto& O:State.Orders)if(O.Status==TEXT("open")&&O.Side==TEXT("buy")&&O.ActorId==S.OwnerId&&O.StorageId==S.Id)V.AcceptedGoods.AddUnique(O.GoodId);P.Stores.Add(MoveTemp(V));}
 for(const auto& V:State.Vehicles){bool Own=V.OwnerId==P.ActorId;const auto* C=Find(Scenario.Definitions.Companies,V.OwnerId);if(!Own&&C->DecisionProfile!=TEXT("carrier"))continue;const auto* T=Find(Scenario.Definitions.VehicleTypes,V.TypeId);FVehicleView X;X.Id=V.Id;X.Own=Own;X.ProviderId=V.OwnerId;X.ProviderName=C->Name;X.Name=Own?TEXT("Eigener Lkw"):C->Name+TEXT(" · Lkw ")+FString::FromInt(P.Vehicles.Num()+1);X.CityId=V.CityId;X.Bound=V.Bound;X.JobId=Own?V.TransportId:TEXT("");X.Blocked=V.ArrearsCents>0;X.Capacity=T->CapacityKg;X.Speed=T->SpeedKmPerHour;X.CostPerKm=T->CostCentsPerKm;X.DailyCost=T->DailyCostCents;P.Vehicles.Add(MoveTemp(X));}
 for(const auto& I:State.Inventories)if(I.OwnerId==P.ActorId&&(I.QuantityKg>0||I.RentArrearsCents))P.Stocks.Add(I);
 for(const auto& O:State.Orders)if(O.Status==TEXT("open")||O.ActorId==P.ActorId)P.Orders.Add(O);
 P.Trades=State.Trades;P.Markets=State.MarketTotals;
 for(const auto& J:State.Transports)if(J.OwnerId==P.ActorId)P.Shipments.Add(J);
 for(const auto& F:State.Facilities)if(F.OwnerId==P.ActorId)P.Facilities.Add(F);
 for(const auto& J:State.Transports)if(J.Status==TEXT("planned")||J.Status==TEXT("moving")){
  const bool Own=J.OwnerId==P.ActorId;if(!Own&&!P.Rules.MapPublicRouteActivity)continue;
  const FString Key=(Own?TEXT("route.own|"):TEXT("route.public|"))+J.FromCityId+TEXT("|")+J.ToCityId;
  auto* R=P.Routes.FindByPredicate([&](const FRouteActivity& X){return X.Id==Key;});if(!R){FRouteActivity X;X.Id=Key;X.FromCityId=J.FromCityId;X.ToCityId=J.ToCityId;X.Own=Own;P.Routes.Add(X);R=&P.Routes.Last();}
  ++R->Active;if(J.Status==TEXT("moving"))++R->Moving;
  if(Own){const auto* Cargo=Find(State.Inventories,J.CargoInventoryId);R->OwnKg+=J.Empty?0:J.Status==TEXT("moving")?(Cargo?Cargo->QuantityKg:0):J.Kg;R->OwnJobs.Add(J.Id);if(!R->NextArrival||J.ArrivalStep<R->NextArrival)R->NextArrival=J.ArrivalStep;}
 }
 P.Routes.Sort([](const FRouteActivity& A,const FRouteActivity& B){return A.Id<B.Id;});
}
}

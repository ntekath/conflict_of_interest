#pragma once
#include "SimModel.h"
namespace CoISim {
struct FStoreView {
 FString Id,Name,CityId,OwnerName;bool Own=false,Public=false,Blocked=false;TArray<FString> AcceptedGoods;
 int64 Capacity=0,Used=0,Incoming=0,DailyCost=0,Arrears=0;
};
struct FVehicleView {
 FString Id,Name,ProviderId,ProviderName,CityId,JobId;bool Own=false,Bound=false,Blocked=false;
 int64 Capacity=0,Speed=0,CostPerKm=0,DailyCost=0;
};
struct FTransportTerms {int64 Km=0,Duration=0,Departure=0,Arrival=0,Operation=0,Freight=0,Cost=0;};
// Shared pure pricing function: preview and authoritative booking use identical terms.
FTransportTerms TransportTerms(const FDefinitions& D,const FEconomyRules& E,const FString& From,const FString& To,const FVehicleDef& V,bool Own,int64 Hour,int64 Requested=0);
struct FPlayerView {
 FString ActorId,StartDate;int64 Hour=0;FDefinitions Catalog;FEconomyRules Rules;
 TArray<FStoreView> Stores;TArray<FVehicleView> Vehicles;TArray<FInventory> Stocks;
 TArray<FMarketOrder> Orders;TArray<FTrade> Trades;TArray<FMarketTotal> Markets;TArray<FTransport> Shipments;TArray<FFacility> Facilities;
 TArray<FPlayerDay> Days;TArray<FPlayerEvent> Events;
};
FString PlayerReason(const FString& Reason);
}

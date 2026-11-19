#pragma once
#include "SimModel.h"
#include "Presentation.h"
namespace CoISim {
struct FStoreView {
 FString TypeId;FString Id,Name,CityId,OwnerId,OwnerName;bool Own=false,Public=false,Blocked=false;TArray<FString> AcceptedGoods;
 int64 RetailWorkers=0,RetailPaidUntil=0;int64 RetailSoldKg=0,RetailRevenueCents=0,RetailCostCents=0;FString RetailCause;int64 Capacity=0,Used=0,Incoming=0,DailyCost=0,Arrears=0;
};
struct FVehicleView {
 FString HomeCityId;FString Id,Name,ProviderId,ProviderName,CityId,JobId;bool Own=false,Bound=false,Blocked=false;
 int64 Capacity=0,Speed=0,CostPerKm=0,DailyCost=0;
};
struct FTransportTerms {int64 Km=0,Duration=0,Departure=0,Arrival=0,Operation=0,Freight=0,Cost=0;};
// Shared pure pricing function: preview and authoritative booking use identical terms.
FTransportTerms TransportTerms(const FDefinitions& D,const FEconomyRules& E,const FString& From,const FString& To,const FVehicleDef& V,bool Own,int64 Hour,int64 Requested=0);
// Deliberately contains no foreign actor, job, cargo, account or schedule fields.
struct FRouteActivity {FString Id,FromCityId,ToCityId;bool Own=false;int64 Active=0,Moving=0,OwnKg=0,NextArrival=0;TArray<FString> OwnJobs;};
// Explicit public presence; no foreign cash, inventory, employment or plans.
struct FCityPresence {FString CompanyId,Name;bool Headquarters=false,Own=false;int64 Warehouses=0,Facilities=0,PublicVehicles=0;};
struct FCityView {FString Id,Name,RegionId;int64 OwnEmployees=0;int64 OwnStockKg=0;int64 Residents=0,Workforce=0,RegionalEmployed=0,RegionalPopulation=0;TArray<FString> Groups;TArray<FCityPresence> Presence;};
struct FPlayerView {
 TArray<FAccountingPrice> AccountingPrices;TArray<FAccountingPeriod> AccountingPeriods;FCompanyPresentation Company;FString ActorId,StartDate;int64 Hour=0;FDefinitions Catalog;FEconomyRules Rules;
 TArray<FDeliveryContract> Deliveries;TArray<FDeliverySlot> DeliverySlots;TArray<FSiteLedger> SiteLedgers;TArray<FInternalTransfer> InternalTransfers;
 TArray<FOperatingRule> OperatingRules;TArray<FOperatingStatus> OperatingStatus;
 TArray<FConstructionProject> Projects;TArray<FAssetSale> AssetSales;TArray<FString> OccupiedPlots;TArray<FCityView> Cities;TArray<FStoreView> Stores;TArray<FVehicleView> Vehicles;TArray<FInventory> Stocks;
 TArray<FMarketOrder> Orders;TArray<FTrade> Trades;TArray<FMarketTotal> Markets;TArray<FTransport> Shipments;TArray<FFacility> Facilities;
 TArray<FRouteActivity> Routes;TArray<FPlayerDay> Days;TArray<FPlayerEvent> Events;
};
bool CityMarketLocation(const FPlayerView&,const FString& StoreId,const FString& CityId,bool EntireRegion);
FString PlayerReason(const FString& Reason);
}

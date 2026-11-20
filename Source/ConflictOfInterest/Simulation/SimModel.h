#pragma once
#include "CoreMinimal.h"
// Plain data: no Actor, UObject, widget, world or streaming dependency.
// All money is USD cents, goods kg, time integral simulation hours.
namespace CoISim
{
#define COI_F(X) R(TEXT(#X),X)
#define COI_O(X) R.Optional(TEXT(#X),X)
#define COI_RECORD(...) template<class Reader> void Fields(Reader& R){__VA_ARGS__}
struct FAmount { FString GoodId; int64 Kg=0; COI_RECORD(COI_F(GoodId);COI_F(Kg);) };
struct FGoodDef
{
 FString Id,Name,Unit,NeedCategory; int64 MapPriceLowCents=0,MapPriceHighCents=0;int64 InitialUnitCostCents=0,InputLimitCents=0; int64 StoragePointsPerKg=0,SpoilPartsPerMillionPerDay=0;
 COI_RECORD(COI_F(Id);COI_F(Name);COI_F(Unit);COI_F(NeedCategory);COI_F(MapPriceLowCents);COI_F(MapPriceHighCents);COI_F(InitialUnitCostCents);COI_F(InputLimitCents);COI_F(StoragePointsPerKg);COI_F(SpoilPartsPerMillionPerDay);)
};
struct FCompanyDef { FString Id,Name,DecisionProfile,Description,StartCityId,HeadquartersCityId;bool Playable=false,AccountingOnly=false; COI_RECORD(COI_F(Id);COI_F(Name);COI_F(DecisionProfile);COI_F(Playable);COI_F(AccountingOnly);COI_F(Description);COI_F(StartCityId);COI_O(HeadquartersCityId);) };
struct FNeedDef
{
 int64 SavingPermille=200,WealthDays=30,MaxExtraPermille=250;
 FString Id,BasicGoodId,LuxuryGoodId; int64 BasicKgPerThousandPerDay=0,LuxuryKgPerThousandPerDay=0,BasicMaxPriceCents=0,LuxuryMaxPriceCents=0,LuxuryBudgetPermille=0;
 COI_RECORD(COI_O(SavingPermille);COI_O(WealthDays);COI_O(MaxExtraPermille);COI_F(Id);COI_F(BasicGoodId);COI_F(LuxuryGoodId);COI_F(BasicKgPerThousandPerDay);COI_F(LuxuryKgPerThousandPerDay);COI_F(BasicMaxPriceCents);COI_F(LuxuryMaxPriceCents);COI_F(LuxuryBudgetPermille);)
};
struct FGroupDef { FString Id,RegionId,NeedProfileId,IncomeBand,ConsumptionCityId; COI_RECORD(COI_F(Id);COI_F(RegionId);COI_F(NeedProfileId);COI_F(IncomeBand);COI_F(ConsumptionCityId);) };
struct FRegionDef
{
 FString Id,CountryId,MarketId,Name,CountryName; int64 FarmlandPermille=0,ForestKg=0,OreKg=0;
 COI_RECORD(COI_O(Name);COI_O(CountryName);COI_F(Id);COI_F(CountryId);COI_F(MarketId);COI_F(FarmlandPermille);COI_F(ForestKg);COI_F(OreKg);)
};
struct FPlaceDef { FString Id,Name,RegionId; double Latitude=0,Longitude=0; COI_RECORD(COI_F(Id);COI_F(Name);COI_F(RegionId);COI_F(Latitude);COI_F(Longitude);) };
struct FFacilityDef
{
 FString Id,Name,ResourceRequirement; TArray<FAmount> Inputs,Outputs;
 int64 CapacityBatchesPerHour=0,Jobs=0,DailyWageCents=0,DailyOperatingCostCents=0,MarginPermille=0;
 COI_RECORD(COI_F(Id);COI_F(Name);COI_F(ResourceRequirement);COI_F(Inputs);COI_F(Outputs);COI_F(CapacityBatchesPerHour);COI_F(Jobs);COI_F(DailyWageCents);COI_F(DailyOperatingCostCents);COI_F(MarginPermille);)
};
struct FRetailOffer {FString GoodId;int64 BuyLimitCents=0;int64 PriceCents=1,QualityPermille=1000,TargetKg=500;COI_RECORD(COI_F(GoodId);COI_F(BuyLimitCents);COI_F(PriceCents);COI_F(QualityPermille);COI_F(TargetKg);)};
struct FWarehouseDef {FString RetailName;TArray<FRetailOffer> RetailOffers;int64 RetailJobs=2,RetailWageCents=150,RetailDailyKg=1000; FString Id; int64 CapacityPoints=0,DailyCostCents=0; COI_RECORD(COI_O(RetailName);COI_O(RetailOffers);COI_O(RetailJobs);COI_O(RetailWageCents);COI_O(RetailDailyKg);COI_F(Id);COI_F(CapacityPoints);COI_F(DailyCostCents);) };
struct FVehicleDef
{
 FString Id,Mode; int64 CapacityKg=0,SpeedKmPerHour=0,CostCentsPerKm=0,DailyCostCents=0;
 COI_RECORD(COI_F(Id);COI_F(Mode);COI_F(CapacityKg);COI_F(SpeedKmPerHour);COI_F(CostCentsPerKm);COI_F(DailyCostCents);)
};
// A physical hinterland site belongs to exactly one hub; regional amounts are aggregates.
struct FResourceSiteDef {FString Id,CityId,Kind;int64 InitialKg=0,SuitabilityPermille=1000,MaxBatchesPerHour=1;COI_RECORD(COI_F(Id);COI_F(CityId);COI_F(Kind);COI_F(InitialKg);COI_F(SuitabilityPermille);COI_F(MaxBatchesPerHour);)};
struct FResourceSite {FString Id;int64 RemainingKg=0,ExtractedKg=0;COI_RECORD(COI_F(Id);COI_F(RemainingKg);COI_F(ExtractedKg);)};
struct FConstructionDef {FString Id,Name,Kind,TypeId,StorageTypeId,FromTypeId,ProviderAccountId;int64 CostCents=0,DurationHours=0;COI_RECORD(COI_F(Id);COI_F(Name);COI_F(Kind);COI_F(TypeId);COI_F(StorageTypeId);COI_F(FromTypeId);COI_F(ProviderAccountId);COI_F(CostCents);COI_F(DurationHours);)};
struct FBuildPlotDef {FString Id,CityId,Kind,ResourceSiteId;COI_RECORD(COI_F(Id);COI_F(CityId);COI_F(Kind);COI_F(ResourceSiteId);)};
struct FInvestmentRequest {FString Id,OwnerId,CityId,Action,DefinitionId,PlotId,TargetId;COI_RECORD(COI_F(Id);COI_F(OwnerId);COI_F(CityId);COI_F(Action);COI_F(DefinitionId);COI_F(PlotId);COI_F(TargetId);)};
struct FConstructionProject {FString Id,OwnerId,CityId,DefinitionId,PlotId,TargetId,AssetId,StorageId,Status;int64 StartHour=0,FinishHour=0,CostCents=0;COI_RECORD(COI_F(Id);COI_F(OwnerId);COI_F(CityId);COI_F(DefinitionId);COI_F(PlotId);COI_F(TargetId);COI_F(AssetId);COI_F(StorageId);COI_F(Status);COI_F(StartHour);COI_F(FinishHour);COI_F(CostCents);)};
struct FAssetSale {FString Id,SellerId,AssetId,CityId,Kind;int64 PriceCents=0,ExpiresHour=0;bool Active=true;COI_RECORD(COI_F(Id);COI_F(SellerId);COI_F(AssetId);COI_F(CityId);COI_F(Kind);COI_F(PriceCents);COI_F(ExpiresHour);COI_F(Active);)};
struct FDeliveryTerms {FString SourceAssetId,DestinationAssetId,FromCityId,ToCityId,GoodId,VehicleId;int64 Kg=0,IntervalHours=24,StartHour=0,EndHour=0,PriceCentsPerKg=0,MaxFreightCents=10000;bool MarketFallback=false;COI_RECORD(COI_F(SourceAssetId);COI_F(DestinationAssetId);COI_F(FromCityId);COI_F(ToCityId);COI_F(GoodId);COI_F(VehicleId);COI_F(Kg);COI_F(IntervalHours);COI_F(StartHour);COI_F(EndHour);COI_F(PriceCentsPerKg);COI_F(MaxFreightCents);COI_F(MarketFallback);)};
struct FDeliveryRequest {FString Id,OwnerId,Action;FDeliveryTerms Terms;COI_RECORD(COI_F(Id);COI_F(OwnerId);COI_F(Action);COI_F(Terms);)};
struct FDeliveryContract {FString Id,OwnerId,SupplierId,BuyerId,SourceStoreId,DestinationStoreId,Status,Cause;FDeliveryTerms Terms;int64 AcceptedHour=0,AcceptedUnitCents=0,NextDueHour=0,LastMissedKg=0,RequestedKg=0,BookedKg=0,DeliveredKg=0,MissedKg=0;COI_RECORD(COI_F(Id);COI_F(OwnerId);COI_F(SupplierId);COI_F(BuyerId);COI_F(SourceStoreId);COI_F(DestinationStoreId);COI_F(Status);COI_F(Cause);COI_F(Terms);COI_F(AcceptedHour);COI_F(AcceptedUnitCents);COI_F(NextDueHour);COI_F(LastMissedKg);COI_F(RequestedKg);COI_F(BookedKg);COI_F(DeliveredKg);COI_F(MissedKg);)};
struct FDeliverySlot {FString Id,ContractId,OutboundId,ReturnId,Status,Cause;int64 DueHour=0,RequestedKg=0,BookedKg=0,DeliveredKg=0,MissedKg=0,PurchaseCents=0,InternalValueCents=0,ReturnHoldCents=0,ReturnCostCents=0,ReturnDeadlineHour=0;bool Accounted=false;COI_RECORD(COI_F(Id);COI_F(ContractId);COI_F(OutboundId);COI_F(ReturnId);COI_F(Status);COI_F(Cause);COI_F(DueHour);COI_F(RequestedKg);COI_F(BookedKg);COI_F(DeliveredKg);COI_F(MissedKg);COI_F(PurchaseCents);COI_F(InternalValueCents);COI_F(ReturnHoldCents);COI_F(ReturnCostCents);COI_F(ReturnDeadlineHour);COI_F(Accounted);)};
struct FSiteLedger {FString Id,ActorId,SiteId;int64 ExternalRevenueCents=0,ExternalCostCents=0,InternalCreditCents=0,InternalDebitCents=0;COI_RECORD(COI_F(Id);COI_F(ActorId);COI_F(SiteId);COI_F(ExternalRevenueCents);COI_F(ExternalCostCents);COI_F(InternalCreditCents);COI_F(InternalDebitCents);)};
struct FInternalTransfer {FString Id,ActorId,SourceId,DestinationId,GoodId;int64 Kg=0,ValueCents=0;COI_RECORD(COI_F(Id);COI_F(ActorId);COI_F(SourceId);COI_F(DestinationId);COI_F(GoodId);COI_F(Kg);COI_F(ValueCents);)};
struct FDefinitions
{
 TArray<FConstructionDef> Construction;TArray<FBuildPlotDef> BuildPlots;TArray<FResourceSiteDef> ResourceSites;TArray<FGoodDef> Goods; TArray<FCompanyDef> Companies; TArray<FNeedDef> Needs; TArray<FGroupDef> Groups;
 TArray<FRegionDef> Regions; TArray<FPlaceDef> Places; TArray<FFacilityDef> FacilityTypes;
 TArray<FWarehouseDef> WarehouseTypes; TArray<FVehicleDef> VehicleTypes;
 COI_RECORD(COI_O(Construction);COI_O(BuildPlots);COI_O(ResourceSites);COI_F(Goods);COI_F(Companies);COI_F(Needs);COI_F(Groups);COI_F(Regions);COI_F(Places);COI_F(FacilityTypes);COI_F(WarehouseTypes);COI_F(VehicleTypes);)
};
struct FAccount { FString Id,OwnerId; int64 BalanceCents=0,ReservedCents=0; COI_RECORD(COI_F(Id);COI_F(OwnerId);COI_F(BalanceCents);COI_F(ReservedCents);) };
struct FCompany { FString Id,AccountId; COI_RECORD(COI_F(Id);COI_F(AccountId);) };
struct FGroup
{
 int64 WealthPermille=0,LastIncomeCents=0,LastSpentCents=0,NextBudgetHour=0;
 FString Id,AccountId; int64 Population=0,Workforce=0,Employed=0,DailyIncomeCents=0,BudgetCents=0;
 int64 DailySpentCents=0,LuxurySpentCents=0,BasicRemainder=0,LuxuryRemainder=0;
 TArray<FAmount> MissedNeed; TArray<FAmount> Consumed,UnmetNeed;
 COI_RECORD(COI_O(WealthPermille);COI_O(LastIncomeCents);COI_O(LastSpentCents);COI_O(NextBudgetHour);COI_F(Id);COI_F(AccountId);COI_F(Population);COI_F(Workforce);COI_F(Employed);COI_F(DailyIncomeCents);COI_F(BudgetCents);COI_O(DailySpentCents);COI_O(LuxurySpentCents);COI_O(BasicRemainder);COI_O(LuxuryRemainder);COI_O(MissedNeed);COI_F(Consumed);COI_F(UnmetNeed);)
};
struct FEmployment { FString GroupId; int64 Workers=0; COI_RECORD(COI_F(GroupId);COI_F(Workers);) };
struct FFacility
{
 int64 PaidDailyWageCents=0,UtilizationRemainder=0;FString ResourceSiteId;int64 ResourceBatchesPerHour=0;FString Id,TypeId,OwnerId,CityId,StorageId,CostRecipientAccountId; int64 OperatingPaidUntilHour=0,OperatingArrearsCents=0; int64 PaidUntilHour=0,ProductionRemainder=0,LastBatches=0,UnfundedJobs=0,UtilizationPermille=0,PayrollArrearsCents=0;
 TArray<FEmployment> Employment;
 COI_RECORD(COI_O(PaidDailyWageCents);COI_O(UtilizationRemainder);COI_F(Id);COI_F(TypeId);COI_F(OwnerId);COI_F(CityId);COI_F(StorageId);COI_O(ResourceSiteId);COI_O(ResourceBatchesPerHour);COI_F(CostRecipientAccountId);COI_O(OperatingPaidUntilHour);COI_O(OperatingArrearsCents);COI_O(PaidUntilHour);COI_O(ProductionRemainder);COI_O(LastBatches);COI_O(UnfundedJobs);COI_F(UtilizationPermille);COI_F(PayrollArrearsCents);COI_F(Employment);)
};
struct FWarehouse
{
 int64 RetailDay=-1,RetailSoldKg=0,RetailRevenueCents=0,RetailCostCents=0;FString RetailCause;
 FString DisplayName;FString Id,TypeId,OwnerId,CityId,CostRecipientAccountId; int64 ReservedIncomingPoints=0,ArrearsCents=0;
 COI_RECORD(COI_O(RetailDay);COI_O(RetailSoldKg);COI_O(RetailRevenueCents);COI_O(RetailCostCents);COI_O(RetailCause);COI_O(DisplayName);COI_F(Id);COI_F(TypeId);COI_F(OwnerId);COI_F(CityId);COI_F(CostRecipientAccountId);COI_F(ReservedIncomingPoints);COI_F(ArrearsCents);)
};
struct FVehicle
{
 FString HomeCityId;FString Id,TypeId,OwnerId,CityId,TransportId,CostRecipientAccountId; int64 ArrearsCents=0; bool Bound=false;
 COI_RECORD(COI_F(Id);COI_F(TypeId);COI_F(OwnerId);COI_F(CityId);COI_F(TransportId);COI_O(HomeCityId);COI_F(CostRecipientAccountId);COI_O(ArrearsCents);COI_F(Bound);)
};
// AP09.4: carrying costs and monthly recognition; no additional cash or physical stock.
struct FAccountingCost {
 int64 Material=0,Labor=0,Operating=0,Freight=0;
 int64 Total() const {return Material+Labor+Operating+Freight;}
 COI_RECORD(COI_F(Material);COI_F(Labor);COI_F(Operating);COI_F(Freight);)
};
struct FAccountingCharge {FString Id,OwnerId;int64 Hour=0,Amount=0;COI_RECORD(COI_F(Id);COI_F(OwnerId);COI_F(Hour);COI_F(Amount);)};
struct FAccountingAsset {FString Id,OwnerId;int64 ValueCents=0;bool Known=false;COI_RECORD(COI_F(Id);COI_F(OwnerId);COI_F(ValueCents);COI_F(Known);)};
struct FAccountingPeriod {
 FString Id,ActorId;int64 BeginHour=0,EndHour=0;bool Complete=false,Partial=false,Known=true;
 int64 ProductionIdle=0;int64 GoodsRevenue=0,ServiceRevenue=0,CostOfGoods=0,Wages=0,Operating=0,CapitalizedWages=0,CapitalizedOperating=0,FreightExpense=0,Spoilage=0,DisposalGain=0;
 int64 IdleCosts() const {return Wages+Operating-CapitalizedWages-CapitalizedOperating;}
 int64 Profit() const {return GoodsRevenue+ServiceRevenue-CostOfGoods-IdleCosts()-FreightExpense-Spoilage+DisposalGain;}
 COI_RECORD(COI_F(Id);COI_F(ActorId);COI_F(BeginHour);COI_F(EndHour);COI_F(Complete);COI_F(Partial);COI_F(Known);COI_F(ProductionIdle);COI_F(GoodsRevenue);COI_F(ServiceRevenue);COI_F(CostOfGoods);COI_F(Wages);COI_F(Operating);COI_F(CapitalizedWages);COI_F(CapitalizedOperating);COI_F(FreightExpense);COI_F(Spoilage);COI_F(DisposalGain);)
};
struct FAccountingPrice {
 FString Id,ActorId,StorageId,GoodId;int64 Version=0,Hour=0;bool Startup=true;
 FAccountingCost UnitCost;int64 Overhead=0;
 int64 Base() const {return UnitCost.Total()+Overhead;}
 COI_RECORD(COI_F(Id);COI_F(ActorId);COI_F(StorageId);COI_F(GoodId);COI_F(Version);COI_F(Hour);COI_F(Startup);COI_F(UnitCost);COI_F(Overhead);)
};

struct FInventory
{
 FString Id,GoodId,OwnerId,StorageId; int64 AcquiredHour=0,RentArrearsCents=0; int64 BookValueCents=0,SpoilRemainder=0; int64 QuantityKg=0,ReservedKg=0;FAccountingCost AccountingCost;
 COI_RECORD(COI_O(AccountingCost);COI_F(Id);COI_F(GoodId);COI_F(OwnerId);COI_F(StorageId);COI_O(AcquiredHour);COI_O(RentArrearsCents);COI_O(BookValueCents);COI_O(SpoilRemainder);COI_F(QuantityKg);COI_F(ReservedKg);)
};
// Settings are commands; counters and causes are authoritative runtime data.
struct FStockRule {FString GoodId;int64 MinimumKg=0,TargetKg=0,BuyLimitCents=0,SellLimitCents=0;int64 AccountingMarkupPermille=200;COI_RECORD(COI_O(AccountingMarkupPermille);COI_F(GoodId);COI_F(MinimumKg);COI_F(TargetKg);COI_F(BuyLimitCents);COI_F(SellLimitCents);)};
struct FOperatingRule {FString Id,OwnerId,RecipeId;bool Explicit=false,Enabled=false;int64 UtilizationPermille=1000,DailyWageCents=1,BuyBudgetCents=0;TArray<FStockRule> Goods;COI_RECORD(COI_F(Id);COI_F(OwnerId);COI_F(RecipeId);COI_F(Explicit);COI_F(Enabled);COI_F(UtilizationPermille);COI_F(DailyWageCents);COI_F(BuyBudgetCents);COI_F(Goods);)};
struct FOperatingStatus {FString Id,Cause;int64 NextDecisionHour=0,BudgetWindow=0,SpentCents=0;COI_RECORD(COI_F(Id);COI_F(Cause);COI_F(NextDecisionHour);COI_F(BudgetWindow);COI_F(SpentCents);)};
struct FMarketOrder
{
 int64 AccountingMarkupPermille=200,AccountingPriceVersion=0;
 FString RuleId;FString Id,ActorId,MarketId,CityId,StorageId,GoodId,Side,Status;
 int64 OriginalKg=0,RemainingKg=0,LimitCentsPerKg=0,Sequence=0,CreatedStep=0,ExpiresStep=0,ReservedCents=0,ReservedKg=0;
 COI_RECORD(COI_O(AccountingMarkupPermille);COI_O(AccountingPriceVersion);COI_O(RuleId);COI_F(Id);COI_F(ActorId);COI_F(MarketId);COI_F(CityId);COI_F(StorageId);COI_F(GoodId);COI_F(Side);COI_F(Status);COI_F(OriginalKg);COI_F(RemainingKg);COI_F(LimitCentsPerKg);COI_F(Sequence);COI_F(CreatedStep);COI_F(ExpiresStep);COI_F(ReservedCents);COI_F(ReservedKg);)
};
struct FTrade
{
 FString Id,BuyOrderId,SellOrderId,BuyerId,SellerId,MarketId,CityId,GoodId;
 int64 Step=0,QuantityKg=0,PriceCentsPerKg=0,TotalCents=0;
 COI_RECORD(COI_F(Id);COI_F(BuyOrderId);COI_F(SellOrderId);COI_F(BuyerId);COI_F(SellerId);COI_F(MarketId);COI_F(CityId);COI_F(GoodId);COI_F(Step);COI_F(QuantityKg);COI_F(PriceCentsPerKg);COI_F(TotalCents);)
};
struct FTransportOffer
{
 FString Id,ProviderId,VehicleId,FromCityId,ToCityId; int64 AvailableKg=0,FixedCents=0,CentsPerKg=0,ExpiresStep=0;
 COI_RECORD(COI_F(Id);COI_F(ProviderId);COI_F(VehicleId);COI_F(FromCityId);COI_F(ToCityId);COI_F(AvailableKg);COI_F(FixedCents);COI_F(CentsPerKg);COI_F(ExpiresStep);)
};
struct FTransport
{
 FString AccountingServicePayerId;int64 AccountingServiceCents=0;
 FString DeliveryId,ReportSiteId;int64 DeliveredKg=0,DeliveredBookCents=0,ActualArrivalHour=0;
 FString Id,OwnerId,ProviderId,VehicleId,FromCityId,ToCityId,GoodId,CargoInventoryId,DestinationStorageId,Service,Status;
 int64 RequestedDepartureHour=0;FString SourceInventoryId;bool Empty=false;int64 OperatingCents=0,FreightCents=0,RouteKm=0; int64 Kg=0,DepartureStep=0,ArrivalStep=0,CostCents=0,ReservedIncomingPoints=0;
 COI_RECORD(COI_O(AccountingServicePayerId);COI_O(AccountingServiceCents);COI_O(DeliveryId);COI_O(ReportSiteId);COI_O(DeliveredKg);COI_O(DeliveredBookCents);COI_O(ActualArrivalHour);COI_F(Id);COI_F(OwnerId);COI_F(ProviderId);COI_F(VehicleId);COI_F(FromCityId);COI_F(ToCityId);COI_F(GoodId);COI_F(CargoInventoryId);COI_F(DestinationStorageId);COI_F(Service);COI_F(Status);COI_O(RequestedDepartureHour);COI_O(SourceInventoryId);COI_O(Empty);COI_O(OperatingCents);COI_O(FreightCents);COI_O(RouteKm);COI_F(Kg);COI_F(DepartureStep);COI_F(ArrivalStep);COI_F(CostCents);COI_F(ReservedIncomingPoints);)
};
struct FReservation
{
 FString Id,ActorId,AccountId,InventoryId,WarehouseId,VehicleId;
 int64 MoneyCents=0,Kg=0,IncomingPoints=0; bool Active=false;
 COI_RECORD(COI_F(Id);COI_F(ActorId);COI_F(AccountId);COI_F(InventoryId);COI_F(WarehouseId);COI_F(VehicleId);COI_F(MoneyCents);COI_F(Kg);COI_F(IncomingPoints);COI_F(Active);)
};
struct FRegion
{
 int64 FinalConsumptionCents=0,NetInventoryTransferCents=0,ModelGdpCents=0;
 FString Id; int64 RemainingForestKg=0,RemainingOreKg=0,Population=0,Employed=0,LastProcessedStep=0; uint32 DiagnosticRandom=0; TArray<FAmount> Consumed;
 COI_RECORD(COI_F(FinalConsumptionCents);COI_F(NetInventoryTransferCents);COI_F(ModelGdpCents);COI_F(Id);COI_F(Population);COI_F(Employed);COI_O(RemainingForestKg);COI_O(RemainingOreKg);COI_F(LastProcessedStep);COI_F(DiagnosticRandom);COI_F(Consumed);)
};
struct FCountry { FString Id; int64 Population=0,Employed=0,ModelGdpCents=0; TArray<FAmount> Consumed; COI_RECORD(COI_F(ModelGdpCents);COI_F(Id);COI_F(Population);COI_F(Employed);COI_F(Consumed);) };
struct FCashFlow
{
 FString FromAccountId,ToAccountId,Reason; int64 Hour=0,Cents=0;
 COI_RECORD(COI_F(FromAccountId);COI_F(ToAccountId);COI_F(Reason);COI_F(Hour);COI_F(Cents);)
};
struct FCashTotal { FString Id;int64 Cents=0;COI_RECORD(COI_F(Id);COI_F(Cents);) };
struct FGoodsAudit
{
 FString Id;int64 DeveloperAddedKg=0;int64 InitialKg=0,ProducedKg=0,InputKg=0,ConsumedKg=0,SpoiledKg=0;
 COI_RECORD(COI_O(DeveloperAddedKg);COI_F(Id);COI_F(InitialKg);COI_F(ProducedKg);COI_F(InputKg);COI_F(ConsumedKg);COI_F(SpoiledKg);)
};
struct FEconomyRules
{
 bool AccountingEnabled=false;
 int64 AccountingPricingHours=24,AccountingNormalUtilizationPermille=800,AccountingNormalTradeKgDay=1000,AccountingDefaultMarkupPermille=200,AccountingMaxMarkupPermille=2000,AccountingHistoryMonths=24;
 TArray<FAccountingAsset> AccountingOpeningAssets;
 bool RetailEnabled=false;
 bool HouseholdModel=false,LegacyHouseholdPurchases=true;int64 WarehouseJobs=2,TraderJobs=3,VehicleJobs=1,ServiceWageCents=150;
 int64 DeliveryMaxRules=64,DeliveryMinIntervalHours=6,DeliveryMaxIntervalHours=168,DeliveryMaxDurationHours=8760,DeliveryMaxKg=2000,DeliveryReturnGraceHours=24,DeliveryPromiseHours=24,DeliveryDefaultFreightCapCents=10000;
 int64 RuleBudgetHours=24,RuleDefaultBuyBudgetCents=50000,RuleMaxBuyBudgetCents=1000000000,RuleMaxWageCents=100000,RuleMaxStockKg=1000000;
 bool MapPublicRouteActivity=true;
 int64 MapTrucksPerRoute=3,MapTrucksTotal=100,MapTruckZoom=16000,MapRouteDetailZoom=12000;

 int64 UiLowCashCents=10000,UiCapacityWarningPermille=100,UiEventLimit=64,UiVisibleEvents=4,UiHistoryDays=7,MaxBookingDelayHours=168;
 int64 Stage=0,PayrollHours=24,DecisionHours=6,DemandHours=24,CostsHours=24,OrderLifetimeHours=12,PickupGraceHours=24;
 int64 RentalCentsPerPointDay=1,MaxOrderKg=100000,MaxPriceCents=1000000,MaxOpenOrders=512,HistoryLimit=256;
 int64 DailyBudgetPermille=500,LocalHaulKm=1,RouteDetourPermille=1400,FreightMarkupPermille=200,BookingLeadHours=1;
 int64 InputTargetHours=48,ProducerReserveKg=100,TraderMaxLoadKg=2000,TraderCashReserveCents=10000,TraderMinProfitCents=100,TraderMarkupPermille=200;
 COI_RECORD(COI_O(AccountingEnabled);COI_O(AccountingPricingHours);COI_O(AccountingNormalUtilizationPermille);COI_O(AccountingNormalTradeKgDay);COI_O(AccountingDefaultMarkupPermille);COI_O(AccountingMaxMarkupPermille);COI_O(AccountingHistoryMonths);COI_O(AccountingOpeningAssets);COI_O(RetailEnabled);COI_O(HouseholdModel);COI_O(LegacyHouseholdPurchases);COI_O(WarehouseJobs);COI_O(TraderJobs);COI_O(VehicleJobs);COI_O(ServiceWageCents);COI_O(DeliveryMaxRules);COI_O(DeliveryMinIntervalHours);COI_O(DeliveryMaxIntervalHours);COI_O(DeliveryMaxDurationHours);COI_O(DeliveryMaxKg);COI_O(DeliveryReturnGraceHours);COI_O(DeliveryPromiseHours);COI_O(DeliveryDefaultFreightCapCents);COI_O(RuleBudgetHours);COI_O(RuleDefaultBuyBudgetCents);COI_O(RuleMaxBuyBudgetCents);COI_O(RuleMaxWageCents);COI_O(RuleMaxStockKg);COI_F(MapPublicRouteActivity);COI_F(MapTrucksPerRoute);COI_F(MapTrucksTotal);COI_F(MapTruckZoom);COI_F(MapRouteDetailZoom);COI_F(UiLowCashCents);COI_F(UiCapacityWarningPermille);COI_F(UiEventLimit);COI_F(UiVisibleEvents);COI_F(UiHistoryDays);COI_F(MaxBookingDelayHours);COI_F(Stage);COI_F(PayrollHours);COI_F(DecisionHours);COI_F(DemandHours);COI_F(CostsHours);COI_F(OrderLifetimeHours);COI_F(PickupGraceHours);COI_F(RentalCentsPerPointDay);COI_F(MaxOrderKg);COI_F(MaxPriceCents);COI_F(MaxOpenOrders);COI_F(HistoryLimit);COI_F(DailyBudgetPermille);COI_F(LocalHaulKm);COI_F(RouteDetourPermille);COI_F(FreightMarkupPermille);COI_F(BookingLeadHours);COI_F(InputTargetHours);COI_F(ProducerReserveKg);COI_F(TraderMaxLoadKg);COI_F(TraderCashReserveCents);COI_F(TraderMinProfitCents);COI_F(TraderMarkupPermille);)
};
struct FMarketTotal
{
 FString Id,MarketId,GoodId;int64 LastPrice=0,LastHour=0,VolumeKg=0,Day=0,DayVolumeKg=0;
 COI_RECORD(COI_F(Id);COI_F(MarketId);COI_F(GoodId);COI_F(LastPrice);COI_F(LastHour);COI_F(VolumeKg);COI_F(Day);COI_F(DayVolumeKg);)
};
struct FPlayerDay
{
 FString Id;int64 Day=0,IncomeCents=0,ExpenseCents=0;TArray<FAmount> Spoiled;
 COI_RECORD(COI_F(Id);COI_F(Day);COI_F(IncomeCents);COI_F(ExpenseCents);COI_F(Spoiled);)
};
struct FPlayerEvent
{
 FString Id,Kind,ObjectId,Text;int64 Hour=0,Count=1;
 COI_RECORD(COI_F(Id);COI_F(Kind);COI_F(ObjectId);COI_F(Text);COI_F(Hour);COI_F(Count);)
};
struct FDayMetric
{
 FString Id,RegionId,GoodId;int64 HouseholdWantedKg=0,HouseholdUsedKg=0,InputWantedKg=0,InputUsedKg=0,TradeKg=0,TradeCents=0;
 COI_RECORD(COI_F(Id);COI_F(RegionId);COI_F(GoodId);COI_F(HouseholdWantedKg);COI_F(HouseholdUsedKg);COI_F(InputWantedKg);COI_F(InputUsedKg);COI_F(TradeKg);COI_F(TradeCents);)
};
struct FDayMetrics
{
 int64 Day=0,CompletedStep=0;bool Complete=false;TArray<FDayMetric> Rows;
 COI_RECORD(COI_F(Day);COI_F(CompletedStep);COI_F(Complete);COI_F(Rows);)
};
struct FServiceEmployment {FString Id;int64 PaidUntilHour=0;TArray<FEmployment> Employment;int64 AccountingPaidDailyWageCents=0;COI_RECORD(COI_F(Id);COI_F(PaidUntilHour);COI_F(Employment);COI_O(AccountingPaidDailyWageCents);)};
struct FState
{
 TArray<FAccountingCharge> AccountingCharges;int64 AccountingPriceSequence=0;int64 AccountingStartHour=-1,AccountingReliableFromHour=-1;TArray<FAccountingPeriod> AccountingPeriods;TArray<FAccountingPrice> AccountingPrices;TArray<FAccountingAsset> AccountingAssets;
 TArray<FServiceEmployment> ServiceEmployment;
 TArray<FDeliveryContract> Deliveries;TArray<FDeliverySlot> DeliverySlots;TArray<FSiteLedger> SiteLedgers;TArray<FInternalTransfer> InternalTransfers;
 TArray<FOperatingRule> OperatingRules;TArray<FOperatingStatus> OperatingStatus;
 TArray<FConstructionProject> Projects;TArray<FAssetSale> AssetSales;TArray<FResourceSite> ResourceSites;bool DeveloperModified=false;int64 DeveloperMoneySources=0,DeveloperMoneySinks=0;

 FDayMetrics CurrentDayMetrics,LastDayMetrics;TArray<FPlayerDay> PlayerDays;TArray<FPlayerEvent> PlayerEvents;int64 OrderSequence=0,TradeSequence=0; int64 EconomyHour=0,InitialMoneyCents=0;TArray<FMarketTotal> MarketTotals;TArray<FCashFlow> CashFlows;TArray<FCashTotal> CashTotals;TArray<FGoodsAudit> GoodsAudit;
 int64 Step=0; TArray<FAccount> Accounts; TArray<FCompany> Companies; TArray<FGroup> Groups;
 TArray<FFacility> Facilities; TArray<FWarehouse> Warehouses; TArray<FVehicle> Vehicles;
 TArray<FInventory> Inventories; TArray<FMarketOrder> Orders; TArray<FTrade> Trades;
 TArray<FTransport> Transports; TArray<FTransportOffer> Offers; TArray<FReservation> Reservations;
 TArray<FRegion> Regions; TArray<FCountry> Countries;
 COI_RECORD(COI_O(AccountingCharges);COI_O(AccountingPriceSequence);COI_O(AccountingStartHour);COI_O(AccountingReliableFromHour);COI_O(AccountingPeriods);COI_O(AccountingPrices);COI_O(AccountingAssets);COI_O(ServiceEmployment);COI_O(Deliveries);COI_O(DeliverySlots);COI_O(SiteLedgers);COI_O(InternalTransfers);COI_O(OperatingRules);COI_O(OperatingStatus);COI_O(Projects);COI_O(AssetSales);COI_O(ResourceSites);COI_O(DeveloperModified);COI_O(DeveloperMoneySources);COI_O(DeveloperMoneySinks);COI_O(CurrentDayMetrics);COI_O(LastDayMetrics);COI_O(PlayerDays);COI_O(PlayerEvents);COI_O(OrderSequence);COI_O(TradeSequence);COI_O(EconomyHour);COI_O(InitialMoneyCents);COI_O(MarketTotals);COI_O(CashFlows);COI_O(CashTotals);COI_O(GoodsAudit);COI_F(Step);COI_F(Accounts);COI_F(Companies);COI_F(Groups);COI_F(Facilities);COI_F(Warehouses);COI_F(Vehicles);COI_F(Inventories);COI_F(Orders);COI_F(Trades);COI_F(Transports);COI_F(Offers);COI_F(Reservations);COI_F(Regions);COI_F(Countries);)
};
struct FSettings
{
 int64 Seed=1913,HoursPerStep=1,RealMicrosPerStep=1000000,MaxStepsPerFrame=4,MaxBacklogSteps=8,MaxPendingCommands=256,CommandHorizonSteps=720,StepBudgetMicros=2000;
 COI_RECORD(COI_F(Seed);COI_F(HoursPerStep);COI_F(RealMicrosPerStep);COI_F(MaxStepsPerFrame);COI_F(MaxBacklogSteps);COI_F(MaxPendingCommands);COI_F(CommandHorizonSteps);COI_F(StepBudgetMicros);)
};
// Examples are schema fixtures, NEVER applied to live accounts or orders.
struct FExamples { TArray<FMarketOrder> Orders; TArray<FTrade> Trades; TArray<FTransport> Transports; COI_RECORD(COI_F(Orders);COI_F(Trades);COI_F(Transports);) };
// Session control is separate from economic ownership; persisted since AP08.4.
struct FSession {FString ScenarioId,PlayerCompanyId;COI_RECORD(COI_F(ScenarioId);COI_F(PlayerCompanyId);)};
struct FScenario
{
 int64 HubSchema=0;int64 Schema=2; FString Id,StartDate,PlayerCompanyId,BalancingNotice,Currency;
 FEconomyRules Economy; FSettings Settings; FDefinitions Definitions; FState InitialState; FExamples ContractExamples;
 COI_RECORD(COI_O(HubSchema);COI_F(Schema);COI_F(Id);COI_F(StartDate);COI_F(PlayerCompanyId);COI_F(BalancingNotice);COI_F(Currency);COI_F(Economy);COI_F(Settings);COI_F(Definitions);COI_F(InitialState);COI_F(ContractExamples);)
};
#undef COI_RECORD
#undef COI_O
#undef COI_F
}

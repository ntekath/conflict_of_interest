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
struct FCompanyDef { FString Id,Name,DecisionProfile,Description,StartCityId;bool Playable=false,AccountingOnly=false; COI_RECORD(COI_F(Id);COI_F(Name);COI_F(DecisionProfile);COI_F(Playable);COI_F(AccountingOnly);COI_F(Description);COI_F(StartCityId);) };
struct FNeedDef
{
 FString Id,BasicGoodId,LuxuryGoodId; int64 BasicKgPerThousandPerDay=0,LuxuryKgPerThousandPerDay=0,BasicMaxPriceCents=0,LuxuryMaxPriceCents=0,LuxuryBudgetPermille=0;
 COI_RECORD(COI_F(Id);COI_F(BasicGoodId);COI_F(LuxuryGoodId);COI_F(BasicKgPerThousandPerDay);COI_F(LuxuryKgPerThousandPerDay);COI_F(BasicMaxPriceCents);COI_F(LuxuryMaxPriceCents);COI_F(LuxuryBudgetPermille);)
};
struct FGroupDef { FString Id,RegionId,NeedProfileId,IncomeBand,ConsumptionCityId; COI_RECORD(COI_F(Id);COI_F(RegionId);COI_F(NeedProfileId);COI_F(IncomeBand);COI_F(ConsumptionCityId);) };
struct FRegionDef
{
 FString Id,CountryId,MarketId; int64 FarmlandPermille=0,ForestKg=0,OreKg=0;
 COI_RECORD(COI_F(Id);COI_F(CountryId);COI_F(MarketId);COI_F(FarmlandPermille);COI_F(ForestKg);COI_F(OreKg);)
};
struct FPlaceDef { FString Id,Name,RegionId; double Latitude=0,Longitude=0; COI_RECORD(COI_F(Id);COI_F(Name);COI_F(RegionId);COI_F(Latitude);COI_F(Longitude);) };
struct FFacilityDef
{
 FString Id,Name,ResourceRequirement; TArray<FAmount> Inputs,Outputs;
 int64 CapacityBatchesPerHour=0,Jobs=0,DailyWageCents=0,DailyOperatingCostCents=0,MarginPermille=0;
 COI_RECORD(COI_F(Id);COI_F(Name);COI_F(ResourceRequirement);COI_F(Inputs);COI_F(Outputs);COI_F(CapacityBatchesPerHour);COI_F(Jobs);COI_F(DailyWageCents);COI_F(DailyOperatingCostCents);COI_F(MarginPermille);)
};
struct FWarehouseDef { FString Id; int64 CapacityPoints=0,DailyCostCents=0; COI_RECORD(COI_F(Id);COI_F(CapacityPoints);COI_F(DailyCostCents);) };
struct FVehicleDef
{
 FString Id,Mode; int64 CapacityKg=0,SpeedKmPerHour=0,CostCentsPerKm=0,DailyCostCents=0;
 COI_RECORD(COI_F(Id);COI_F(Mode);COI_F(CapacityKg);COI_F(SpeedKmPerHour);COI_F(CostCentsPerKm);COI_F(DailyCostCents);)
};
struct FDefinitions
{
 TArray<FGoodDef> Goods; TArray<FCompanyDef> Companies; TArray<FNeedDef> Needs; TArray<FGroupDef> Groups;
 TArray<FRegionDef> Regions; TArray<FPlaceDef> Places; TArray<FFacilityDef> FacilityTypes;
 TArray<FWarehouseDef> WarehouseTypes; TArray<FVehicleDef> VehicleTypes;
 COI_RECORD(COI_F(Goods);COI_F(Companies);COI_F(Needs);COI_F(Groups);COI_F(Regions);COI_F(Places);COI_F(FacilityTypes);COI_F(WarehouseTypes);COI_F(VehicleTypes);)
};
struct FAccount { FString Id,OwnerId; int64 BalanceCents=0,ReservedCents=0; COI_RECORD(COI_F(Id);COI_F(OwnerId);COI_F(BalanceCents);COI_F(ReservedCents);) };
struct FCompany { FString Id,AccountId; COI_RECORD(COI_F(Id);COI_F(AccountId);) };
struct FGroup
{
 FString Id,AccountId; int64 Population=0,Workforce=0,Employed=0,DailyIncomeCents=0,BudgetCents=0;
 int64 DailySpentCents=0,LuxurySpentCents=0,BasicRemainder=0,LuxuryRemainder=0;
 TArray<FAmount> MissedNeed; TArray<FAmount> Consumed,UnmetNeed;
 COI_RECORD(COI_F(Id);COI_F(AccountId);COI_F(Population);COI_F(Workforce);COI_F(Employed);COI_F(DailyIncomeCents);COI_F(BudgetCents);COI_O(DailySpentCents);COI_O(LuxurySpentCents);COI_O(BasicRemainder);COI_O(LuxuryRemainder);COI_O(MissedNeed);COI_F(Consumed);COI_F(UnmetNeed);)
};
struct FEmployment { FString GroupId; int64 Workers=0; COI_RECORD(COI_F(GroupId);COI_F(Workers);) };
struct FFacility
{
 FString Id,TypeId,OwnerId,CityId,StorageId,CostRecipientAccountId; int64 OperatingPaidUntilHour=0,OperatingArrearsCents=0; int64 PaidUntilHour=0,ProductionRemainder=0,LastBatches=0,UnfundedJobs=0,UtilizationPermille=0,PayrollArrearsCents=0;
 TArray<FEmployment> Employment;
 COI_RECORD(COI_F(Id);COI_F(TypeId);COI_F(OwnerId);COI_F(CityId);COI_F(StorageId);COI_F(CostRecipientAccountId);COI_O(OperatingPaidUntilHour);COI_O(OperatingArrearsCents);COI_O(PaidUntilHour);COI_O(ProductionRemainder);COI_O(LastBatches);COI_O(UnfundedJobs);COI_F(UtilizationPermille);COI_F(PayrollArrearsCents);COI_F(Employment);)
};
struct FWarehouse
{
 FString Id,TypeId,OwnerId,CityId,CostRecipientAccountId; int64 ReservedIncomingPoints=0,ArrearsCents=0;
 COI_RECORD(COI_F(Id);COI_F(TypeId);COI_F(OwnerId);COI_F(CityId);COI_F(CostRecipientAccountId);COI_F(ReservedIncomingPoints);COI_F(ArrearsCents);)
};
struct FVehicle
{
 FString Id,TypeId,OwnerId,CityId,TransportId,CostRecipientAccountId; int64 ArrearsCents=0; bool Bound=false;
 COI_RECORD(COI_F(Id);COI_F(TypeId);COI_F(OwnerId);COI_F(CityId);COI_F(TransportId);COI_F(CostRecipientAccountId);COI_O(ArrearsCents);COI_F(Bound);)
};
struct FInventory
{
 FString Id,GoodId,OwnerId,StorageId; int64 AcquiredHour=0,RentArrearsCents=0; int64 BookValueCents=0,SpoilRemainder=0; int64 QuantityKg=0,ReservedKg=0;
 COI_RECORD(COI_F(Id);COI_F(GoodId);COI_F(OwnerId);COI_F(StorageId);COI_O(AcquiredHour);COI_O(RentArrearsCents);COI_O(BookValueCents);COI_O(SpoilRemainder);COI_F(QuantityKg);COI_F(ReservedKg);)
};
struct FMarketOrder
{
 FString Id,ActorId,MarketId,CityId,StorageId,GoodId,Side,Status;
 int64 OriginalKg=0,RemainingKg=0,LimitCentsPerKg=0,Sequence=0,CreatedStep=0,ExpiresStep=0,ReservedCents=0,ReservedKg=0;
 COI_RECORD(COI_F(Id);COI_F(ActorId);COI_F(MarketId);COI_F(CityId);COI_F(StorageId);COI_F(GoodId);COI_F(Side);COI_F(Status);COI_F(OriginalKg);COI_F(RemainingKg);COI_F(LimitCentsPerKg);COI_F(Sequence);COI_F(CreatedStep);COI_F(ExpiresStep);COI_F(ReservedCents);COI_F(ReservedKg);)
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
 FString Id,OwnerId,ProviderId,VehicleId,FromCityId,ToCityId,GoodId,CargoInventoryId,DestinationStorageId,Service,Status;
 int64 RequestedDepartureHour=0;FString SourceInventoryId;bool Empty=false;int64 OperatingCents=0,FreightCents=0,RouteKm=0; int64 Kg=0,DepartureStep=0,ArrivalStep=0,CostCents=0,ReservedIncomingPoints=0;
 COI_RECORD(COI_F(Id);COI_F(OwnerId);COI_F(ProviderId);COI_F(VehicleId);COI_F(FromCityId);COI_F(ToCityId);COI_F(GoodId);COI_F(CargoInventoryId);COI_F(DestinationStorageId);COI_F(Service);COI_F(Status);COI_O(RequestedDepartureHour);COI_O(SourceInventoryId);COI_O(Empty);COI_O(OperatingCents);COI_O(FreightCents);COI_O(RouteKm);COI_F(Kg);COI_F(DepartureStep);COI_F(ArrivalStep);COI_F(CostCents);COI_F(ReservedIncomingPoints);)
};
struct FReservation
{
 FString Id,ActorId,AccountId,InventoryId,WarehouseId,VehicleId;
 int64 MoneyCents=0,Kg=0,IncomingPoints=0; bool Active=false;
 COI_RECORD(COI_F(Id);COI_F(ActorId);COI_F(AccountId);COI_F(InventoryId);COI_F(WarehouseId);COI_F(VehicleId);COI_F(MoneyCents);COI_F(Kg);COI_F(IncomingPoints);COI_F(Active);)
};
struct FRegion
{
 FString Id; int64 RemainingForestKg=0,RemainingOreKg=0,Population=0,Employed=0,LastProcessedStep=0; uint32 DiagnosticRandom=0; TArray<FAmount> Consumed;
 COI_RECORD(COI_F(Id);COI_F(Population);COI_F(Employed);COI_O(RemainingForestKg);COI_O(RemainingOreKg);COI_F(LastProcessedStep);COI_F(DiagnosticRandom);COI_F(Consumed);)
};
struct FCountry { FString Id; int64 Population=0,Employed=0; TArray<FAmount> Consumed; COI_RECORD(COI_F(Id);COI_F(Population);COI_F(Employed);COI_F(Consumed);) };
struct FCashFlow
{
 FString FromAccountId,ToAccountId,Reason; int64 Hour=0,Cents=0;
 COI_RECORD(COI_F(FromAccountId);COI_F(ToAccountId);COI_F(Reason);COI_F(Hour);COI_F(Cents);)
};
struct FCashTotal { FString Id;int64 Cents=0;COI_RECORD(COI_F(Id);COI_F(Cents);) };
struct FGoodsAudit
{
 FString Id;int64 InitialKg=0,ProducedKg=0,InputKg=0,ConsumedKg=0,SpoiledKg=0;
 COI_RECORD(COI_F(Id);COI_F(InitialKg);COI_F(ProducedKg);COI_F(InputKg);COI_F(ConsumedKg);COI_F(SpoiledKg);)
};
struct FEconomyRules
{
 int64 UiLowCashCents=10000,UiCapacityWarningPermille=100,UiEventLimit=64,UiVisibleEvents=4,UiHistoryDays=7,MaxBookingDelayHours=168;
 int64 Stage=0,PayrollHours=24,DecisionHours=6,DemandHours=24,CostsHours=24,OrderLifetimeHours=12,PickupGraceHours=24;
 int64 RentalCentsPerPointDay=1,MaxOrderKg=100000,MaxPriceCents=1000000,MaxOpenOrders=512,HistoryLimit=256;
 int64 DailyBudgetPermille=500,LocalHaulKm=1,RouteDetourPermille=1400,FreightMarkupPermille=200,BookingLeadHours=1;
 int64 InputTargetHours=48,ProducerReserveKg=100,TraderMaxLoadKg=2000,TraderCashReserveCents=10000,TraderMinProfitCents=100,TraderMarkupPermille=200;
 COI_RECORD(COI_F(UiLowCashCents);COI_F(UiCapacityWarningPermille);COI_F(UiEventLimit);COI_F(UiVisibleEvents);COI_F(UiHistoryDays);COI_F(MaxBookingDelayHours);COI_F(Stage);COI_F(PayrollHours);COI_F(DecisionHours);COI_F(DemandHours);COI_F(CostsHours);COI_F(OrderLifetimeHours);COI_F(PickupGraceHours);COI_F(RentalCentsPerPointDay);COI_F(MaxOrderKg);COI_F(MaxPriceCents);COI_F(MaxOpenOrders);COI_F(HistoryLimit);COI_F(DailyBudgetPermille);COI_F(LocalHaulKm);COI_F(RouteDetourPermille);COI_F(FreightMarkupPermille);COI_F(BookingLeadHours);COI_F(InputTargetHours);COI_F(ProducerReserveKg);COI_F(TraderMaxLoadKg);COI_F(TraderCashReserveCents);COI_F(TraderMinProfitCents);COI_F(TraderMarkupPermille);)
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
struct FState
{
 FDayMetrics CurrentDayMetrics,LastDayMetrics;TArray<FPlayerDay> PlayerDays;TArray<FPlayerEvent> PlayerEvents;int64 OrderSequence=0,TradeSequence=0; int64 EconomyHour=0,InitialMoneyCents=0;TArray<FMarketTotal> MarketTotals;TArray<FCashFlow> CashFlows;TArray<FCashTotal> CashTotals;TArray<FGoodsAudit> GoodsAudit;
 int64 Step=0; TArray<FAccount> Accounts; TArray<FCompany> Companies; TArray<FGroup> Groups;
 TArray<FFacility> Facilities; TArray<FWarehouse> Warehouses; TArray<FVehicle> Vehicles;
 TArray<FInventory> Inventories; TArray<FMarketOrder> Orders; TArray<FTrade> Trades;
 TArray<FTransport> Transports; TArray<FTransportOffer> Offers; TArray<FReservation> Reservations;
 TArray<FRegion> Regions; TArray<FCountry> Countries;
 COI_RECORD(COI_O(CurrentDayMetrics);COI_O(LastDayMetrics);COI_O(PlayerDays);COI_O(PlayerEvents);COI_O(OrderSequence);COI_O(TradeSequence);COI_O(EconomyHour);COI_O(InitialMoneyCents);COI_O(MarketTotals);COI_O(CashFlows);COI_O(CashTotals);COI_O(GoodsAudit);COI_F(Step);COI_F(Accounts);COI_F(Companies);COI_F(Groups);COI_F(Facilities);COI_F(Warehouses);COI_F(Vehicles);COI_F(Inventories);COI_F(Orders);COI_F(Trades);COI_F(Transports);COI_F(Offers);COI_F(Reservations);COI_F(Regions);COI_F(Countries);)
};
struct FSettings
{
 int64 Seed=1913,HoursPerStep=1,RealMicrosPerStep=1000000,MaxStepsPerFrame=4,MaxBacklogSteps=8,MaxPendingCommands=256,CommandHorizonSteps=720,StepBudgetMicros=2000;
 COI_RECORD(COI_F(Seed);COI_F(HoursPerStep);COI_F(RealMicrosPerStep);COI_F(MaxStepsPerFrame);COI_F(MaxBacklogSteps);COI_F(MaxPendingCommands);COI_F(CommandHorizonSteps);COI_F(StepBudgetMicros);)
};
// Examples are schema fixtures, NEVER applied to live accounts or orders.
struct FExamples { TArray<FMarketOrder> Orders; TArray<FTrade> Trades; TArray<FTransport> Transports; COI_RECORD(COI_F(Orders);COI_F(Trades);COI_F(Transports);) };
// Session control is separate from economic ownership; serializable for AP09.
struct FSession {FString ScenarioId,PlayerCompanyId;COI_RECORD(COI_F(ScenarioId);COI_F(PlayerCompanyId);)};
struct FScenario
{
 int64 Schema=2; FString Id,StartDate,PlayerCompanyId,BalancingNotice,Currency;
 FEconomyRules Economy; FSettings Settings; FDefinitions Definitions; FState InitialState; FExamples ContractExamples;
 COI_RECORD(COI_F(Schema);COI_F(Id);COI_F(StartDate);COI_F(PlayerCompanyId);COI_F(BalancingNotice);COI_F(Currency);COI_F(Economy);COI_F(Settings);COI_F(Definitions);COI_F(InitialState);COI_F(ContractExamples);)
};
#undef COI_RECORD
#undef COI_O
#undef COI_F
}

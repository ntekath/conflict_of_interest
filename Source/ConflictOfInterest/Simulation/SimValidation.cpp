#include "SimCore.h"
#include "CityHubs.h"
#include "Investments.h"
#include "SimArchive.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
namespace CoISim
{
#define REQUIRE(C,M) if(!(C)){Error=TEXT(M);return false;}
template<class T> bool Unique(const TArray<T>& Rows)
{
 FString Last;for(const auto& R:Rows){if(R.Id.IsEmpty()||(!Last.IsEmpty()&&R.Id<=Last))return false;Last=R.Id;}return true;
}
bool LoadScenario(const FString& Json,const FCoIGeoMapData& Geo,FScenario& Out,FString& Error)
{
 Error.Empty();REQUIRE(Json.Len()<4000000,"Scenario exceeds size limit");
 TSharedPtr<FJsonObject> Root;REQUIRE(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root)&&Root.IsValid(),"Invalid scenario JSON");
 FScenario Candidate;FRead Reader{Root,Error};Candidate.Fields(Reader);if(!Error.IsEmpty())return false;
 if(!MigrateCityHubs(Candidate,Error)||!ValidateScenario(Candidate,Geo,Error))return false;Out=MoveTemp(Candidate);return true;
}
bool ValidateScenario(const FScenario& S,const FCoIGeoMapData& Geo,FString& Error)
{
 const auto& Op=S.Economy;if(Op.RuleBudgetHours<1||Op.RuleBudgetHours>168||Op.RuleDefaultBuyBudgetCents<0||Op.RuleDefaultBuyBudgetCents>Op.RuleMaxBuyBudgetCents||Op.RuleMaxBuyBudgetCents<1||Op.RuleMaxBuyBudgetCents>1000000000000LL||Op.RuleMaxWageCents<1||Op.RuleMaxWageCents>1000000||Op.RuleMaxStockKg<1||Op.RuleMaxStockKg>1000000000||!S.InitialState.OperatingRules.IsEmpty()||!S.InitialState.OperatingStatus.IsEmpty()){Error=TEXT("Invalid operating rules configuration or nonempty initial runtime rules");return false;}

 const auto& D=S.Definitions;const auto& W=S.InitialState;const auto& C=S.Settings;
 FDateTime Start;REQUIRE(S.Schema==2&&S.Id==Geo.ScenarioId&&S.Currency==TEXT("USD")&&!S.BalancingNotice.IsEmpty(),"Scenario schema, ID, currency or balancing notice invalid");
 REQUIRE(FDateTime::ParseIso8601(*(S.StartDate+TEXT("T00:00:00Z")),Start)&&S.StartDate.Len()==10,"Invalid start date");
 REQUIRE(C.HoursPerStep>=1&&C.HoursPerStep<=24&&C.Seed>=0&&C.Seed<=MAX_int32&&C.RealMicrosPerStep>=1000&&C.RealMicrosPerStep<=60000000&&C.MaxStepsPerFrame>=1&&C.MaxStepsPerFrame<=64&&C.MaxBacklogSteps>=C.MaxStepsPerFrame&&C.MaxBacklogSteps<=128&&C.MaxPendingCommands>=1&&C.MaxPendingCommands<=4096&&C.CommandHorizonSteps>=1&&C.CommandHorizonSteps<=100000&&C.StepBudgetMicros>=1&&C.StepBudgetMicros<=100000,"Invalid simulation limits");
 const auto& E=S.Economy;
 REQUIRE(E.MapTrucksPerRoute>=0&&E.MapTrucksPerRoute<=20&&E.MapTrucksTotal>=0&&E.MapTrucksTotal<=1000&&E.MapTruckZoom>0&&E.MapRouteDetailZoom>0,"Invalid map route display limits");
 REQUIRE(!W.DeveloperModified&&W.DeveloperMoneySources==0&&W.DeveloperMoneySinks==0,"Initial developer interventions forbidden; use explicit developer interface");
 REQUIRE(W.CurrentDayMetrics.Rows.IsEmpty()&&W.LastDayMetrics.Rows.IsEmpty()&&!W.LastDayMetrics.Complete,"Initial map metrics must be empty");
 REQUIRE(W.PlayerDays.IsEmpty()&&W.PlayerEvents.IsEmpty(),"Initial player history must be empty");
 REQUIRE(W.EconomyHour==0&&W.InitialMoneyCents==0&&W.OrderSequence==0&&W.TradeSequence==0&&W.CashFlows.IsEmpty()&&W.CashTotals.IsEmpty()&&W.GoodsAudit.IsEmpty()&&W.MarketTotals.IsEmpty(),"Initial derived economic counters must be empty");
 for(const auto& G:W.Groups)REQUIRE(G.DailySpentCents==0&&G.LuxurySpentCents==0&&G.BasicRemainder==0&&G.LuxuryRemainder==0&&G.MissedNeed.IsEmpty(),"Initial consumer counters must be empty");
 for(const auto& F:W.Facilities)REQUIRE(F.OperatingPaidUntilHour==0&&F.OperatingArrearsCents==0&&F.PaidUntilHour==0&&F.ProductionRemainder==0&&F.LastBatches==0&&F.UnfundedJobs==0,"Initial production counters must be empty");
 for(const auto& I:W.Inventories)REQUIRE(I.AcquiredHour==0&&I.RentArrearsCents==0&&I.BookValueCents==0&&I.SpoilRemainder==0,"Initial book values are derived from declared good costs");

 REQUIRE(E.UiLowCashCents>=0&&E.UiLowCashCents<=1000000000&&E.UiCapacityWarningPermille>=0&&E.UiCapacityWarningPermille<=1000&&E.UiEventLimit>=8&&E.UiEventLimit<=256&&E.UiVisibleEvents>=1&&E.UiVisibleEvents<=8&&E.UiHistoryDays>=2&&E.UiHistoryDays<=31&&E.MaxBookingDelayHours>=24&&E.MaxBookingDelayHours<=720,"Invalid HUD limits");
 REQUIRE(E.Stage>=0&&E.Stage<=5&&E.PayrollHours>=24&&E.PayrollHours<=168&&E.PayrollHours%24==0&&E.DecisionHours>=1&&E.DecisionHours<=24&&E.DemandHours==24&&E.CostsHours==24&&E.OrderLifetimeHours>=1&&E.OrderLifetimeHours<=168&&E.PickupGraceHours>=1&&E.PickupGraceHours<=168&&E.RentalCentsPerPointDay>=0&&E.RentalCentsPerPointDay<=100&&E.MaxOrderKg>0&&E.MaxOrderKg<=1000000&&E.MaxPriceCents>0&&E.MaxPriceCents<=1000000&&E.MaxOpenOrders>=16&&E.MaxOpenOrders<=4096&&E.HistoryLimit>=16&&E.HistoryLimit<=4096&&E.DailyBudgetPermille>=0&&E.DailyBudgetPermille<=1000&&E.LocalHaulKm>=1&&E.LocalHaulKm<=100&&E.RouteDetourPermille>=1000&&E.RouteDetourPermille<=4000&&E.FreightMarkupPermille>=0&&E.FreightMarkupPermille<=2000&&E.BookingLeadHours>=1&&E.BookingLeadHours<=24&&E.InputTargetHours>=1&&E.InputTargetHours<=168&&E.ProducerReserveKg>=0&&E.TraderMaxLoadKg>0&&E.TraderMaxLoadKg<=1000000&&E.TraderCashReserveCents>=0&&E.TraderMinProfitCents>=0&&E.TraderMarkupPermille>=0&&E.TraderMarkupPermille<=2000,"Invalid economic intervals or limits");
 REQUIRE(Unique(D.Goods)&&Unique(D.Companies)&&Unique(D.Needs)&&Unique(D.Groups)&&Unique(D.Regions)&&Unique(D.Places)&&Unique(D.FacilityTypes)&&Unique(D.WarehouseTypes)&&Unique(D.VehicleTypes),"Definition IDs must be unique, nonempty, sorted");
 REQUIRE(Unique(W.Accounts)&&Unique(W.Companies)&&Unique(W.Groups)&&Unique(W.Facilities)&&Unique(W.Warehouses)&&Unique(W.Vehicles)&&Unique(W.Inventories)&&Unique(W.Offers),"Runtime IDs must be unique, nonempty, sorted");
 REQUIRE(W.Step==0&&W.Orders.IsEmpty()&&W.Trades.IsEmpty()&&W.Transports.IsEmpty()&&W.Reservations.IsEmpty()&&W.Regions.IsEmpty()&&W.Countries.IsEmpty(),"Initial live bookings and derived aggregates must be empty");
 REQUIRE(D.Goods.Num()==5&&D.Places.Num()==Geo.Cities.Num()&&D.Regions.Num()==Geo.Regions.Num()&&D.Groups.Num()==D.Regions.Num()*3&&W.Groups.Num()==D.Groups.Num()&&W.Companies.Num()==D.Companies.Num(),"Incomplete scenario definition/runtime coverage");
 auto Actor=[&](const FString& Id){return Find(D.Companies,Id)!=nullptr||Find(D.Groups,Id)!=nullptr;};
 auto Amounts=[&](const TArray<FAmount>& A){TSet<FString> Seen;for(const auto& V:A){if(!Find(D.Goods,V.GoodId)||V.Kg<0||V.Kg>1000000000||Seen.Contains(V.GoodId))return false;Seen.Add(V.GoodId);}return true;};
 for(const TCHAR* Id:{TEXT("good.food"),TEXT("good.wood"),TEXT("good.ore"),TEXT("good.tools"),TEXT("good.luxury")})REQUIRE(Find(D.Goods,FString(Id)),"Missing required prototype good");
 for(const auto& Company:D.Companies)REQUIRE(!Company.Name.IsEmpty()&&(Company.DecisionProfile==TEXT("producer")||Company.DecisionProfile==TEXT("trader")||Company.DecisionProfile==TEXT("carrier")||Company.DecisionProfile==TEXT("service")),"Unknown company decision profile");
 for(const auto& G:D.Goods)REQUIRE(G.MapPriceLowCents>=0&&G.MapPriceHighCents>G.MapPriceLowCents&&G.MapPriceHighCents<=E.MaxPriceCents&&G.InitialUnitCostCents>=0&&G.InitialUnitCostCents<=E.MaxPriceCents&&G.InputLimitCents>0&&G.InputLimitCents<=E.MaxPriceCents&&G.Unit==TEXT("kg")&&G.StoragePointsPerKg>0&&G.StoragePointsPerKg<=10000&&G.SpoilPartsPerMillionPerDay>=0&&G.SpoilPartsPerMillionPerDay<=1000000&&(G.NeedCategory==TEXT("basic")||G.NeedCategory==TEXT("luxury")||G.NeedCategory==TEXT("none")),"Invalid goods units, storage or spoilage definition");
 for(const auto& P:D.Places){const auto* G=Geo.Cities.FindByPredicate([&](const FCoIGeoCity& X){return X.Id==P.Id;});REQUIRE(G&&P.RegionId==G->RegionId&&FMath::Abs(P.Latitude-G->Latitude)<1e-7&&FMath::Abs(P.Longitude-G->Longitude)<1e-7,"Place does not agree with geographic catalog");}
 TSet<FString> Markets;
 for(const auto& R:D.Regions)
 {
  const auto* G=Geo.Regions.FindByPredicate([&](const FCoIGeoSurface& X){return X.Id==R.Id;});
  REQUIRE(G&&R.CountryId==G->CountryId&&!R.MarketId.IsEmpty()&&!Markets.Contains(R.MarketId)&&R.FarmlandPermille>=0&&R.FarmlandPermille<=1000&&R.ForestKg>=0&&R.ForestKg<=1000000000&&R.OreKg>=0&&R.OreKg<=1000000000,"Invalid region, unique market or resources");Markets.Add(R.MarketId);
  TSet<FString> Bands;for(const auto& Group:D.Groups)if(Group.RegionId==R.Id)Bands.Add(Group.IncomeBand);
  REQUIRE(Bands.Num()==3&&Bands.Contains(TEXT("low"))&&Bands.Contains(TEXT("middle"))&&Bands.Contains(TEXT("high")),"Exactly three income groups per region required");
 }
 for(const auto& N:D.Needs)
 {
  const auto* Basic=Find(D.Goods,N.BasicGoodId);const auto* Luxury=Find(D.Goods,N.LuxuryGoodId);
  REQUIRE(Basic&&Luxury&&Basic->NeedCategory==TEXT("basic")&&Luxury->NeedCategory==TEXT("luxury")&&N.BasicKgPerThousandPerDay>=0&&N.BasicKgPerThousandPerDay<=100000&&N.LuxuryKgPerThousandPerDay>=0&&N.LuxuryKgPerThousandPerDay<=100000&&N.BasicMaxPriceCents>0&&N.BasicMaxPriceCents<=1000000000&&N.LuxuryMaxPriceCents>0&&N.LuxuryMaxPriceCents<=1000000000&&N.LuxuryBudgetPermille>=0&&N.LuxuryBudgetPermille<=1000,"Invalid basic/luxury need profile");
 }
 for(const auto& A:W.Accounts)REQUIRE(Actor(A.OwnerId)&&A.BalanceCents>=0&&A.BalanceCents<=1000000000000LL&&A.ReservedCents==0,"Invalid account owner, balance or unbacked initial reservation");
 for(const auto& Company:W.Companies){auto* A=Find(W.Accounts,Company.AccountId);REQUIRE(Find(D.Companies,Company.Id)&&A&&A->OwnerId==Company.Id,"Invalid company account");}
 for(const auto& G:D.Groups)
 {
  const auto* P=Find(D.Places,G.ConsumptionCityId);REQUIRE(Find(D.Regions,G.RegionId)&&Find(D.Needs,G.NeedProfileId)&&P&&P->RegionId==G.RegionId,"Invalid consumer location or needs");
  auto* State=Find(W.Groups,G.Id);REQUIRE(State,"Missing consumer state");auto* A=Find(W.Accounts,State->AccountId);
  REQUIRE(A&&A->OwnerId==G.Id&&State->Population>=0&&State->Population<=1000000000&&State->Workforce>=0&&State->Workforce<=State->Population&&State->Employed>=0&&State->Employed<=State->Workforce&&State->DailyIncomeCents>=0&&State->BudgetCents>=0&&State->BudgetCents<=A->BalanceCents&&Amounts(State->Consumed)&&Amounts(State->UnmetNeed),"Invalid group population, employment, budget or consumption");
 }
 for(const auto& T:D.WarehouseTypes)REQUIRE(T.CapacityPoints>0&&T.CapacityPoints<=1000000000000LL&&T.DailyCostCents>0,"Warehouses require finite capacity and positive daily costs");
 for(const auto& T:D.VehicleTypes)REQUIRE(T.Mode==TEXT("truck")&&T.CapacityKg>0&&T.CapacityKg<=1000000000&&T.SpeedKmPerHour>0&&T.SpeedKmPerHour<=200&&T.CostCentsPerKm>0&&T.DailyCostCents>0,"Invalid truck type");
 for(const auto& T:D.FacilityTypes){for(const auto& A:T.Inputs)REQUIRE(A.Kg>0,"Recipe input must be positive");for(const auto& A:T.Outputs)REQUIRE(A.Kg>0,"Recipe output must be positive");if(T.ResourceRequirement==TEXT("forest")||T.ResourceRequirement==TEXT("ore")){bool Resource=false;for(const auto& A:T.Outputs)Resource|=A.GoodId==(T.ResourceRequirement==TEXT("forest")?TEXT("good.wood"):TEXT("good.ore"));REQUIRE(Resource,"Extraction requires matching resource output");}}
 for(const auto& T:D.FacilityTypes)REQUIRE(Amounts(T.Inputs)&&Amounts(T.Outputs)&&!T.Outputs.IsEmpty()&&T.CapacityBatchesPerHour>0&&T.CapacityBatchesPerHour<=100000&&T.Jobs>0&&T.Jobs<=1000000&&T.DailyWageCents>0&&T.DailyWageCents<=S.Economy.RuleMaxWageCents&&T.DailyOperatingCostCents>=0&&T.MarginPermille>=0&&T.MarginPermille<=10000&&(T.ResourceRequirement==TEXT("none")||T.ResourceRequirement==TEXT("farmland")||T.ResourceRequirement==TEXT("forest")||T.ResourceRequirement==TEXT("ore")),"Invalid facility recipe, workforce or resource requirement");
 for(const auto& Store:W.Warehouses)REQUIRE(Find(D.WarehouseTypes,Store.TypeId)&&Find(D.Companies,Store.OwnerId)&&Find(D.Places,Store.CityId)&&Find(W.Accounts,Store.CostRecipientAccountId)&&Store.ReservedIncomingPoints==0&&Store.ArrearsCents>=0,"Invalid warehouse references or incoming reservation");
 TSet<FString> StockKeys;
 for(const auto& I:W.Inventories)
 {
  FString Key=I.OwnerId+TEXT("|")+I.StorageId+TEXT("|")+I.GoodId;
  REQUIRE(Actor(I.OwnerId)&&Find(D.Goods,I.GoodId)&&Find(W.Warehouses,I.StorageId)&&I.QuantityKg>=0&&I.QuantityKg<=1000000000&&I.ReservedKg==0&&!StockKeys.Contains(Key),"Invalid/duplicate physical inventory or reservation");StockKeys.Add(Key);
 }
 for(const auto& Store:W.Warehouses){int64 Used=0;for(const auto& I:W.Inventories)if(I.StorageId==Store.Id)Used+=I.QuantityKg*Find(D.Goods,I.GoodId)->StoragePointsPerKg;REQUIRE(Used<=Find(D.WarehouseTypes,Store.TypeId)->CapacityPoints,"Warehouse exceeds finite capacity");}
 for(const auto& V:W.Vehicles)REQUIRE(Find(D.VehicleTypes,V.TypeId)&&Find(D.Companies,V.OwnerId)&&Find(D.Places,V.CityId)&&Find(W.Accounts,V.CostRecipientAccountId)&&!V.Bound&&V.TransportId.IsEmpty(),"Invalid initial vehicle or double binding");
 for(const auto& F:W.Facilities)
 {
  auto* T=Find(D.FacilityTypes,F.TypeId);auto* P=Find(D.Places,F.CityId);auto* Store=Find(W.Warehouses,F.StorageId);
  REQUIRE(T&&P&&Store&&Find(D.Companies,F.OwnerId)&&Store->CityId==F.CityId&&Store->OwnerId==F.OwnerId&&Find(W.Accounts,F.CostRecipientAccountId)&&F.UtilizationPermille>=0&&F.UtilizationPermille<=1000&&F.PayrollArrearsCents>=0,"Invalid facility ownership/location");
  auto* R=Find(D.Regions,P->RegionId);REQUIRE(R,"Missing facility region");
  REQUIRE(T->ResourceRequirement==TEXT("none")||(T->ResourceRequirement==TEXT("farmland")&&R->FarmlandPermille>0)||(T->ResourceRequirement==TEXT("forest")&&R->ForestKg>0)||(T->ResourceRequirement==TEXT("ore")&&R->OreKg>0),"Facility site lacks its configured resource");
  int64 Count=0;TSet<FString> Seen;for(const auto& Job:F.Employment){auto* G=Find(D.Groups,Job.GroupId);REQUIRE(G&&G->RegionId==P->RegionId&&Job.Workers>=0&&!Seen.Contains(Job.GroupId),"Invalid employment or cross-region commuting");Count+=Job.Workers;Seen.Add(Job.GroupId);}REQUIRE(Count<=T->Jobs,"Facility exceeds job slots");
 }
 for(const auto& G:W.Groups){int64 Workers=0;for(const auto& F:W.Facilities)for(const auto& Job:F.Employment)if(Job.GroupId==G.Id)Workers+=Job.Workers;REQUIRE(Workers==G.Employed&&Workers<=G.Workforce,"Employment double counted or exceeds workforce");}
 for(const auto& O:W.Offers)
 {
  auto* V=Find(W.Vehicles,O.VehicleId);REQUIRE(V&&O.ProviderId==V->OwnerId&&Find(D.Places,O.FromCityId)&&Find(D.Places,O.ToCityId)&&O.FromCityId!=O.ToCityId&&V->CityId==O.FromCityId&&O.AvailableKg>0&&O.AvailableKg<=Find(D.VehicleTypes,V->TypeId)->CapacityKg&&O.FixedCents>=0&&O.CentsPerKg>=0&&O.ExpiresStep>0,"Invalid finite transport offer");
 }
 const auto* Selected=Find(D.Companies,S.PlayerCompanyId);
 REQUIRE(Selected&&Selected->Playable,"Selected company is missing or not playable; session cannot start");
 int Playable=0;for(const auto& Company:D.Companies){if(Company.Playable){++Playable;REQUIRE(!Company.AccountingOnly&&!Company.Description.IsEmpty()&&Find(D.Places,Company.StartCityId),"Invalid playable company metadata");}}
 REQUIRE(Playable>=2,"At least two existing companies must be playable");
 for(const auto& O:S.ContractExamples.Orders)
 {auto* P=Find(D.Places,O.CityId);auto* Store=Find(W.Warehouses,O.StorageId);auto* R=P?Find(D.Regions,P->RegionId):nullptr;REQUIRE(Actor(O.ActorId)&&P&&Store&&Store->CityId==O.CityId&&R&&R->MarketId==O.MarketId&&Find(D.Goods,O.GoodId)&&(O.Side==TEXT("buy")||O.Side==TEXT("sell"))&&O.Status==TEXT("draft")&&O.OriginalKg>0&&O.RemainingKg<=O.OriginalKg&&O.RemainingKg>=0&&O.LimitCentsPerKg>0&&O.ExpiresStep>O.CreatedStep&&O.ReservedKg==0&&O.ReservedCents==0,"Invalid market-order contract example");}
 for(const auto& T:S.ContractExamples.Trades)REQUIRE(Find(S.ContractExamples.Orders,T.BuyOrderId)&&Find(S.ContractExamples.Orders,T.SellOrderId)&&Actor(T.BuyerId)&&Actor(T.SellerId)&&T.BuyerId!=T.SellerId&&Markets.Contains(T.MarketId)&&Find(D.Places,T.CityId)&&Find(D.Goods,T.GoodId)&&T.QuantityKg>0&&T.QuantityKg<=1000000000&&T.PriceCentsPerKg>0&&T.PriceCentsPerKg<=1000000000&&T.TotalCents==T.QuantityKg*T.PriceCentsPerKg,"Invalid trade contract example");
 for(const auto& T:S.ContractExamples.Transports)
 {auto* V=Find(W.Vehicles,T.VehicleId);auto* Store=Find(W.Warehouses,T.DestinationStorageId);REQUIRE(V&&Actor(T.OwnerId)&&Find(D.Companies,T.ProviderId)&&Find(D.Places,T.FromCityId)&&Find(D.Places,T.ToCityId)&&Store&&Store->CityId==T.ToCityId&&Find(D.Goods,T.GoodId)&&Find(W.Inventories,T.CargoInventoryId)&&T.Status==TEXT("draft")&&(T.Service==TEXT("own_truck")||T.Service==TEXT("rented_truck"))&&T.Kg>0&&T.Kg<=Find(D.VehicleTypes,V->TypeId)->CapacityKg&&T.ArrivalStep>T.DepartureStep&&T.CostCents>=0&&T.ReservedIncomingPoints==0,"Invalid transport contract example");}
 if(!ValidateInvestments(S,W,true,Error)||!ValidateCityHubs(S,W,true,Error))return false;Error.Empty();return true;
}
#undef REQUIRE
}

#if WITH_DEV_AUTOMATION_TESTS
#include "GeoCatalog.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Simulation/SaveGame.h"
#include "Simulation/SimArchive.h"
using namespace CoISim;
namespace
{
bool OperatingFixture(FString &J, FCoIGeoMapData &G, TFunction<void(TSharedPtr<FJsonObject>)> Edit = {})
{
    FString E;
    auto *C = LoadObject<UCoIGeoCatalog>(nullptr, TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));
    if (!C || !G.Load(C->DefinitionJson, E) ||
        !FFileHelper::LoadFileToString(J, *(FPaths::ProjectContentDir() / TEXT("Simulation/scenario.coisim"))))
        return false;
    TSharedPtr<FJsonObject> R;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(J), R);
    if (Edit)
        Edit(R);
    J.Empty();
    FJsonSerializer::Serialize(R.ToSharedRef(), TJsonWriterFactory<>::Create(&J));
    return true;
}
FCommand OpCommand(FSimulation &S, ECommandKind Kind)
{
    FCommand C;
    C.Kind = Kind;
    C.Generation = S.GetGeneration();
    C.Sequence = S.NextSequence();
    C.TargetStep = S.GetState().Step + 1;
    return C;
}
bool QueueOp(FSimulation &S, FOperatingRule R, FString &E)
{
    auto C = OpCommand(S, ECommandKind::OperatingRule);
    R.Explicit = true;
    C.Rule = R;
    C.Origin = ECommandOrigin::Player;
    C.CompanyId = R.OwnerId;
    return S.Enqueue(C, E);
}
FStockRule &OpGood(FOperatingRule &R, const FString &Id)
{
    return *R.Goods.FindByPredicate([&](const FStockRule &X) { return X.GoodId == Id; });
}
int64 OpHeld(const FSimulation &S, const FString &Rule)
{
    int64 C = 0;
    for (const auto &O : S.GetState().Orders)
        if (O.Status == TEXT("open") && O.RuleId == Rule)
            C += O.ReservedCents;
    return C;
}
FMarketOrder OpMarket(const FString &Id, const FString &Actor, const FString &Store, const FString &City,
                      const FString &Good, const FString &Side, int64 Kg, int64 Price)
{
    FMarketOrder O;
    O.Id = Id;
    O.ActorId = Actor;
    O.StorageId = Store;
    O.CityId = City;
    O.MarketId = TEXT("market.") + City.RightChop(8);
    O.GoodId = Good;
    O.Side = Side;
    O.OriginalKg = Kg;
    O.LimitCentsPerKg = Price;
    return O;
}
} // namespace
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOperatingFlow, "CoI.Operating.ExplicitProcurementProductionSaleAndSave",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOperatingFlow::RunTest(const FString &)
{
    FString J, E, Disk;
    FCoIGeoMapData G;
    OperatingFixture(J, G, [](auto Root) {
        Root->SetStringField(TEXT("PlayerCompanyId"), TEXT("company.tools"));
        for (auto V : Root->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("Companies")))
            if (V->AsObject()->GetStringField(TEXT("Id")) == TEXT("company.tools"))
            {
                V->AsObject()->SetBoolField(TEXT("Playable"), true);
                V->AsObject()->SetStringField(TEXT("Description"), TEXT("Test-only selected input producer"));
                V->AsObject()->SetStringField(TEXT("StartCityId"), TEXT("city.us.pittsburgh"));
            }
        for (auto X : Root->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Inventories")))
            if (X->AsObject()->GetStringField(TEXT("Id")) == TEXT("stock.tools.ore"))
                X->AsObject()->SetStringField(TEXT("OwnerId"), TEXT("company.ore"));
    });
    FSimulation A, B;
    if (!A.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    const FString Facility = TEXT("facility.tools");
    const auto *Existing = Find(A.GetState().OperatingRules, Facility);
    if (!TestNotNull(TEXT("Existing plant settings"), Existing))
        return false;
    auto Rule = *Existing;
    Rule.Enabled = true;
    Rule.BuyBudgetCents = 10000;
    for (auto &Good : Rule.Goods)
    {
        Good.MinimumKg = 0;
        Good.TargetKg = Good.GoodId == TEXT("good.ore") ? 500 : 0;
        Good.BuyLimitCents = Good.GoodId == TEXT("good.ore") ? 10 : 0;
        Good.SellLimitCents = Good.GoodId == TEXT("good.tools") ? 1 : 0;
    }
    TestTrue(TEXT("Explicit rule command"), QueueOp(A, Rule, E));
    auto Supply = OpCommand(A, ECommandKind::PlaceOrder);
    Supply.Order = OpMarket(TEXT("op.supply"), TEXT("company.ore"), TEXT("storage.tools"), TEXT("city.us.pittsburgh"),
                            TEXT("good.ore"), TEXT("sell"), 800, 2);
    TestTrue(TEXT("Real third-party supply"), A.Enqueue(Supply, E));
    auto Demand = OpCommand(A, ECommandKind::PlaceOrder);
    Demand.Order = OpMarket(TEXT("op.demand"), TEXT("company.food"), TEXT("storage.tools"), TEXT("city.us.pittsburgh"),
                            TEXT("good.tools"), TEXT("buy"), 2000, 10);
    A.Enqueue(Demand, E);
    A.StepOnce();
    TestTrue(TEXT("Procurement paid from real account"), Find(A.GetState().OperatingStatus, Facility)->SpentCents > 0);
    TestTrue(TEXT("No manual input purchase"), A.GetState().Trades.ContainsByPredicate([](const FTrade &T) {
        return T.BuyerId == TEXT("company.tools") && T.GoodId == TEXT("good.ore");
    }));
    for (int H = 0; H < 8; ++H)
        A.StepOnce();
    TestTrue(TEXT("Normal production uses purchased inputs"),
             Find(A.GetState().GoodsAudit, FString(TEXT("good.tools")))->ProducedKg > 0);
    TestTrue(TEXT("Automatic output sale is real money transfer"),
             A.GetState().Trades.ContainsByPredicate([](const FTrade &T) {
                 return T.SellerId == TEXT("company.tools") && T.GoodId == TEXT("good.tools");
             }));
    // Pending rule changes and existing rule counter/queues must survive together.
    Rule.BuyBudgetCents = 100;
    TestTrue(TEXT("Budget reduction queued"), QueueOp(A, Rule, E));
    if (!TestTrue(TEXT("Save active service and pending edit: ") + E, A.SaveGameJson(Disk, E)))
    {
        AddError(E);
        return false;
    }
    int32 Speed = 1;
    if (!B.LoadGameJson(Disk, G, A.GetScenario(), E, Speed))
    {
        AddError(E);
        return false;
    }
    for (int H = 0; H < 96; ++H)
    {
        A.StepOnce();
        B.StepOnce();
        if (H == 0)
        {
            TestEqual(TEXT("Spent budget is not reset by reducing the limit"),
                      Find(A.GetState().OperatingStatus, Facility)->SpentCents, 1600LL);
            TestEqual(TEXT("No purchase after spending exceeds reduced cap"), OpHeld(A, Facility), 0LL);
        }
        if (A.Fingerprint() != B.Fingerprint())
        {
            AddError(TEXT("Operating save continuation differs"));
            return false;
        }
        if (!A.ValidateSaveState(E))
        {
            AddError(E);
            return false;
        }
    }
    TestEqual(TEXT("Rule budget retained"), Find(B.GetState().OperatingRules, Facility)->BuyBudgetCents, 100LL);
    TestTrue(TEXT("Conservation"), B.AuditRuntime(E));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOperatingBudget, "CoI.Operating.SharedLiquidityBudgetsDisableAndManualOrders",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOperatingBudget::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    OperatingFixture(J, G, [](auto Root) {
        for (auto X : Root->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Warehouses")))
            if (X->AsObject()->GetStringField(TEXT("Id")) == TEXT("storage.food"))
                X->AsObject()->SetStringField(TEXT("OwnerId"), TEXT("company.player"));
        for (auto X : Root->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Facilities")))
            if (X->AsObject()->GetStringField(TEXT("Id")) == TEXT("facility.food"))
                X->AsObject()->SetStringField(TEXT("OwnerId"), TEXT("company.player"));
        for (auto X : Root->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Accounts")))
            if (X->AsObject()->GetStringField(TEXT("OwnerId")) == TEXT("company.player"))
                X->AsObject()->SetNumberField(TEXT("BalanceCents"), 20000);
    });
    FSimulation S;
    if (!S.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    auto First = *Find(S.GetState().OperatingRules, FString(TEXT("storage.player"))),
         Second = *Find(S.GetState().OperatingRules, FString(TEXT("storage.food")));
    for (auto *R : {&First, &Second})
    {
        R->Enabled = true;
        R->BuyBudgetCents = 20000;
        auto &Good = OpGood(*R, TEXT("good.ore"));
        Good.TargetKg = 10000;
        Good.BuyLimitCents = 10;
        TestTrue(TEXT("Rule queued"), QueueOp(S, *R, E));
    }
    S.StepOnce();
    int64 Holds = OpHeld(S, First.Id) + OpHeld(S, Second.Id);
    TestTrue(TEXT("At least one real reservation"), Holds > 0);
    const auto *Cash = Find(S.GetState().Accounts, FString(TEXT("account.company.player")));
    TestTrue(TEXT("Two assets cannot reserve the same cash"),
             Holds <= Cash->BalanceCents && Cash->ReservedCents <= Cash->BalanceCents);
    TestTrue(TEXT("Shortage visible"),
             S.GetSnapshot().Player.OperatingStatus.ContainsByPredicate([](const FOperatingStatus &X) {
                 return X.Cause.Contains(TEXT("Kapital")) || X.Cause.Contains(TEXT("budget"));
             }));
    auto Manual = OpCommand(S, ECommandKind::PlaceOrder);
    Manual.Order = OpMarket(TEXT("op.manual"), TEXT("company.player"), TEXT("storage.player"), TEXT("city.us.detroit"),
                            TEXT("good.food"), TEXT("sell"), 10, 100);
    TestTrue(TEXT("Manual order queued"), S.Enqueue(Manual, E));
    First.Enabled = false;
    Second.BuyBudgetCents = 0;
    TestTrue(TEXT("Disable one rule"), QueueOp(S, First, E));
    TestTrue(TEXT("Zero budget other rule"), QueueOp(S, Second, E));
    S.StepOnce();
    TestEqual(TEXT("Disabled rule frees all automatic money holds"), OpHeld(S, First.Id), 0LL);
    TestEqual(TEXT("Lowered zero budget cannot retain/recreate holds"), OpHeld(S, Second.Id), 0LL);
    TestEqual(TEXT("Manual order preserved on rule edits"),
              Find(S.GetState().Orders, FString(TEXT("op.manual")))->Status, FString(TEXT("open")));
    for (int H = 0; H < 36; ++H)
        S.StepOnce();
    TestEqual(TEXT("Disabled rule stays inactive next period"), OpHeld(S, First.Id), 0LL);
    TestEqual(TEXT("Zero budget still zero next period"), OpHeld(S, Second.Id), 0LL);
    TestTrue(TEXT("Ledger invariant"), S.AuditRuntime(E));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOperatingWage, "CoI.Operating.WageUtilizationAuthorizationAndRecipeLimits",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOperatingWage::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    OperatingFixture(J, G, [](auto R) { R->SetStringField(TEXT("PlayerCompanyId"), TEXT("company.food")); });
    FSimulation S;
    if (!S.Load(J, G, E))
        return false;
    auto Rule = *Find(S.GetState().OperatingRules, FString(TEXT("facility.food")));
    Rule.Explicit = true;
    Rule.UtilizationPermille = 500;
    Rule.DailyWageCents = 200;
    Rule.Enabled = false;
    auto Hash = S.Fingerprint();
    TestTrue(TEXT("Read-only preview"), S.PreviewOperatingRule(Rule, E));
    TestEqual(TEXT("Preview cannot alter state"), S.Fingerprint(), Hash);
    TestTrue(TEXT("Set half utilization and offered wage"), QueueOp(S, Rule, E));
    S.StepOnce();
    const auto *F = Find(S.GetState().Facilities, Rule.Id);
    int64 Workers = 0;
    for (const auto &W : F->Employment)
        Workers += W.Workers;
    TestEqual(TEXT("Half utilization requests half of twenty jobs"), Workers, 10LL);
    TestEqual(TEXT("Actual prepaid wage recorded"), F->PaidDailyWageCents, 200LL);
    int64 Wages = Find(S.GetState().CashTotals, FString(TEXT("wages")))->Cents;
    Rule.DailyWageCents = 300;
    QueueOp(S, Rule, E);
    S.StepOnce();
    TestEqual(TEXT("No mid-period double payroll"), Find(S.GetState().CashTotals, FString(TEXT("wages")))->Cents,
              Wages);
    TestEqual(TEXT("Paid wage unaffected until next payroll"),
              Find(S.GetState().Facilities, Rule.Id)->PaidDailyWageCents, 200LL);
    for (int H = 2; H < 24; ++H)
        S.StepOnce();
    int64 Produced = Find(S.GetState().GoodsAudit, FString(TEXT("good.food")))->ProducedKg;
    TestEqual(TEXT("50 percent with 800 permille site: nine full batches in 24h"), Produced, 900LL);
    S.StepOnce();
    TestEqual(TEXT("New wage at next regular payroll"), Find(S.GetState().Facilities, Rule.Id)->PaidDailyWageCents,
              300LL);
    auto Bad = Rule;
    Bad.RecipeId = TEXT("facility_type.tools");
    TestFalse(TEXT("Cannot switch to unrelated recipe"), S.PreviewOperatingRule(Bad, E));
    Bad = Rule;
    Bad.Goods[0].TargetKg = -1;
    TestFalse(TEXT("Invalid stock target"), S.PreviewOperatingRule(Bad, E));
    Bad = Rule;
    Bad.DailyWageCents = 0;
    TestFalse(TEXT("No unpaid free work"), S.PreviewOperatingRule(Bad, E));
    Bad = Rule;
    Bad.UtilizationPermille = 1001;
    TestFalse(TEXT("Utilization bound"), S.PreviewOperatingRule(Bad, E));
    auto Foreign = OpCommand(S, ECommandKind::OperatingRule);
    Foreign.Origin = ECommandOrigin::Player;
    Foreign.CompanyId = TEXT("company.tools");
    Foreign.Rule = *Find(S.GetState().OperatingRules, FString(TEXT("facility.tools")));
    Foreign.Rule.Explicit = true;
    TestFalse(TEXT("Foreign automation command denied"), S.Enqueue(Foreign, E));
    Rule.UtilizationPermille = 0;
    QueueOp(S, Rule, E);
    S.StepOnce();
    Produced = Find(S.GetState().GoodsAudit, FString(TEXT("good.food")))->ProducedKg;
    for (int H = 0; H < 26; ++H)
        S.StepOnce();
    TestEqual(TEXT("Zero utilization stops production"),
              Find(S.GetState().GoodsAudit, FString(TEXT("good.food")))->ProducedKg, Produced);
    TestTrue(TEXT("Visible stop cause"),
             Find(S.GetSnapshot().Player.OperatingStatus, Rule.Id)->Cause.Contains(TEXT("0 %")));
    TestTrue(TEXT("Payroll and stock accounting"), S.AuditRuntime(E));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOperatingTargets, "CoI.Operating.SharedTargetsCapacityAndIncomingCargo",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOperatingTargets::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    OperatingFixture(J, G, [](auto Root) {
        auto W = Root->GetObjectField(TEXT("InitialState"));
        for (auto X : W->GetArrayField(TEXT("Warehouses")))
            if (X->AsObject()->GetStringField(TEXT("Id")) == TEXT("storage.tools"))
                X->AsObject()->SetStringField(TEXT("OwnerId"), TEXT("company.player"));
        for (auto X : W->GetArrayField(TEXT("Facilities")))
            if (X->AsObject()->GetStringField(TEXT("Id")) == TEXT("facility.tools"))
                X->AsObject()->SetStringField(TEXT("OwnerId"), TEXT("company.player"));
        for (auto X : W->GetArrayField(TEXT("Inventories")))
        {
            auto I = X->AsObject();
            if (I->GetStringField(TEXT("OwnerId")) == TEXT("company.tools"))
                I->SetStringField(TEXT("OwnerId"), TEXT("company.player"));
            if (I->GetStringField(TEXT("Id")) == TEXT("stock.tools.ore"))
            {
                I->SetNumberField(TEXT("QuantityKg"), 0);
                I->SetNumberField(TEXT("BookValueCents"), 0);
            }
        }
    });
    FSimulation S;
    if (!S.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    for (const FString Asset : {FString(TEXT("facility.tools")), FString(TEXT("storage.tools"))})
    {
        auto R = *Find(S.GetState().OperatingRules, Asset);
        R.Enabled = true;
        R.UtilizationPermille = 0;
        R.BuyBudgetCents = 10000;
        for (auto &Good : R.Goods)
        {
            Good.MinimumKg = 0;
            Good.TargetKg = Good.GoodId == TEXT("good.ore") ? 500 : 0;
            Good.BuyLimitCents = Good.GoodId == TEXT("good.ore") ? 10 : 0;
            Good.SellLimitCents = 0;
        }
        TestTrue(TEXT("Shared-store asset rule"), QueueOp(S, R, E));
    }
    S.StepOnce();
    int64 Ordered = 0;
    for (const auto &O : S.GetState().Orders)
        if (O.ActorId == TEXT("company.player") && O.Status == TEXT("open") && O.Side == TEXT("buy") &&
            O.StorageId == TEXT("storage.tools") && O.GoodId == TEXT("good.ore"))
            Ordered += O.RemainingKg;
    TestEqual(TEXT("Plant and warehouse do not each buy the same target"), Ordered, 500LL);
    FString CapacityJson;
    OperatingFixture(CapacityJson, G, [](auto Root) {
        for (auto X : Root->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("WarehouseTypes")))
            if (X->AsObject()->GetStringField(TEXT("Id")) == TEXT("warehouse.player"))
                X->AsObject()->SetNumberField(TEXT("CapacityPoints"), 100);
    });
    FSimulation Full;
    if (!Full.Load(CapacityJson, G, E))
    {
        AddError(E);
        return false;
    }
    auto R = *Find(Full.GetState().OperatingRules, FString(TEXT("storage.player")));
    R.Enabled = true;
    auto &Ore = OpGood(R, TEXT("good.ore"));
    Ore.TargetKg = 500;
    Ore.BuyLimitCents = 10;
    QueueOp(Full, R, E);
    Full.StepOnce();
    TestEqual(TEXT("Full warehouse cannot order more physical stock"), OpHeld(Full, R.Id), 0LL);
    TestTrue(TEXT("Capacity cause published"),
             Find(Full.GetSnapshot().Player.OperatingStatus, R.Id)->Cause.Contains(TEXT("Lagerkapazität")));
    FString IncomingJson;
    OperatingFixture(IncomingJson, G, [](auto Root) {
        auto W = Root->GetObjectField(TEXT("InitialState"));
        for (auto X : W->GetArrayField(TEXT("Inventories")))
            if (X->AsObject()->GetStringField(TEXT("Id")) == TEXT("stock.player.food"))
                X->AsObject()->SetStringField(TEXT("StorageId"), TEXT("storage.service.cleveland"));
        for (auto X : W->GetArrayField(TEXT("Vehicles")))
            if (X->AsObject()->GetStringField(TEXT("Id")) == TEXT("vehicle.player"))
                X->AsObject()->SetStringField(TEXT("CityId"), TEXT("city.us.cleveland"));
    });
    FSimulation Incoming;
    if (!Incoming.Load(IncomingJson, G, E))
    {
        AddError(E);
        return false;
    }
    auto Trip = OpCommand(Incoming, ECommandKind::BookTransport);
    Trip.Transport.Id = TEXT("op.incoming");
    Trip.Transport.OwnerId = TEXT("company.player");
    Trip.Transport.VehicleId = TEXT("vehicle.player");
    Trip.Transport.SourceInventoryId = TEXT("stock.player.food");
    Trip.Transport.DestinationStorageId = TEXT("storage.player");
    Trip.Transport.FromCityId = TEXT("city.us.cleveland");
    Trip.Transport.ToCityId = TEXT("city.us.detroit");
    Trip.Transport.Kg = 50;
    Trip.Transport.RequestedDepartureHour = 3;
    TestTrue(TEXT("Real planned cargo"), Incoming.Enqueue(Trip, E));
    R = *Find(Incoming.GetState().OperatingRules, FString(TEXT("storage.player")));
    R.Enabled = true;
    auto &Food = OpGood(R, TEXT("good.food"));
    Food.TargetKg = 50;
    Food.BuyLimitCents = 10;
    QueueOp(Incoming, R, E);
    Incoming.StepOnce();
    TestTrue(TEXT("Transport booking accepted"),
             Find(Incoming.GetState().Transports, FString(TEXT("op.incoming"))) != nullptr);
    TestEqual(TEXT("No duplicate buy for planned incoming stock"), OpHeld(Incoming, R.Id), 0LL);
    for (int H = 0; H < 5; ++H)
        Incoming.StepOnce();
    TestEqual(TEXT("No duplicate buy for moving stock"), OpHeld(Incoming, R.Id), 0LL);
    TestTrue(TEXT("All incoming holds consistent"), Incoming.AuditRuntime(E));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOperatingSharedReserve, "CoI.Operating.RuleChangesProtectSharedPhysicalReserves",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOperatingSharedReserve::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    OperatingFixture(J, G, [](auto Root) {
        auto W = Root->GetObjectField(TEXT("InitialState"));
        for (auto X : W->GetArrayField(TEXT("Facilities")))
            if (X->AsObject()->GetStringField(TEXT("Id")) == TEXT("facility.food"))
                X->AsObject()->SetStringField(TEXT("OwnerId"), TEXT("company.player"));
        for (auto X : W->GetArrayField(TEXT("Warehouses")))
            if (X->AsObject()->GetStringField(TEXT("Id")) == TEXT("storage.food"))
                X->AsObject()->SetStringField(TEXT("OwnerId"), TEXT("company.player"));
        for (auto X : W->GetArrayField(TEXT("Inventories")))
            if (X->AsObject()->GetStringField(TEXT("Id")) == TEXT("stock.food.food"))
                X->AsObject()->SetStringField(TEXT("OwnerId"), TEXT("company.player"));
    });
    FSimulation S;
    if (!S.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    auto Warehouse = *Find(S.GetState().OperatingRules, FString(TEXT("storage.food")));
    Warehouse.Enabled = true;
    auto &GRule = OpGood(Warehouse, TEXT("good.food"));
    GRule.TargetKg = 100;
    GRule.SellLimitCents = 1000000;
    TestTrue(TEXT("Warehouse sale rule"), QueueOp(S, Warehouse, E));
    S.StepOnce();
    FString OldOrder;
    int64 Reserved = 0;
    for (const auto &O : S.GetState().Orders)
        if (O.RuleId == Warehouse.Id && O.Status == TEXT("open") && O.Side == TEXT("sell"))
        {
            OldOrder = O.Id;
            Reserved += O.ReservedKg;
        }
    if (!TestTrue(TEXT("An unfilled automatic surplus order exists"), Reserved > 0))
        return false;
    auto Plant = *Find(S.GetState().OperatingRules, FString(TEXT("facility.food")));
    Plant.Enabled = true;
    for (auto &X : Plant.Goods)
    {
        X.MinimumKg = 0;
        X.TargetKg = 900;
        X.SellLimitCents = 1000000;
    }
    TestTrue(TEXT("Raise shared plant reserve"), QueueOp(S, Plant, E));
    S.StepOnce();
    TestEqual(TEXT("Old warehouse offer cancelled at command boundary"), Find(S.GetState().Orders, OldOrder)->Status,
              FString(TEXT("rule_changed")));
    const auto *Stock = Find(S.GetState().Inventories, FString(TEXT("stock.food.food")));
    if (!TestNotNull(TEXT("Real owned stock"), Stock))
        return false;
    TestTrue(TEXT("All automatic sales together protect newly raised shared target"),
             Stock->ReservedKg <= FMath::Max(0LL, Stock->QuantityKg - 900));
    TestTrue(TEXT("Shared order cancellation conserves assets and payments"), S.AuditRuntime(E));
    return true;
}
#endif

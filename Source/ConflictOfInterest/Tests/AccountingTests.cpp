#if WITH_DEV_AUTOMATION_TESTS
#include "GeoCatalog.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Simulation/Accounting.h"
#include "Simulation/SaveGame.h"
#include "Simulation/SimArchive.h"
using namespace CoISim;
namespace
{
bool CostFixture(FString &J, FCoIGeoMapData &G, bool Legacy = false, TFunction<void(TSharedPtr<FJsonObject>)> Edit = {})
{
    FString E;
    auto *C = LoadObject<UCoIGeoCatalog>(nullptr, TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));
    if (!C || !G.Load(C->DefinitionJson, E) ||
        !FFileHelper::LoadFileToString(J, *(FPaths::ProjectContentDir() / TEXT("Simulation/") +
                                            (Legacy ? TEXT("ap093_legacy_fixture.coisim") : TEXT("scenario.coisim")))))
        return false;
    if (Edit)
    {
        TSharedPtr<FJsonObject> Root;
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(J), Root);
        Edit(Root);
        J.Empty();
        FJsonSerializer::Serialize(Root.ToSharedRef(), TJsonWriterFactory<>::Create(&J));
    }
    return true;
}
bool QueueCost(FSimulation &S, FCommand C, FString &E)
{
    C.Generation = S.GetGeneration();
    C.Sequence = S.NextSequence();
    C.TargetStep = S.GetState().Step + 1;
    return S.Enqueue(C, E);
}
FCommand CostTrip(bool Internal = false)
{
    FCommand C;
    C.Kind = ECommandKind::BookTransport;
    auto &T = C.Transport;
    T.Id = TEXT("cost.trip");
    T.OwnerId = TEXT("company.player");
    T.VehicleId = TEXT("vehicle.player");
    T.SourceInventoryId = TEXT("stock.player.food");
    T.DestinationStorageId = TEXT("retail.store.chicago.player");
    T.FromCityId = TEXT("city.us.detroit");
    T.ToCityId = TEXT("city.us.chicago");
    T.Kg = 10;
    return C;
}
FString CostEncode(const FSaveData &D)
{
    FWrite W;
    auto Copy = D;
    Copy.Fields(W);
    FFingerprint H;
    H.Write(D);
    auto Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("Format"), TEXT("CoI.Save"));
    Root->SetStringField(TEXT("Checksum"), H.Finish());
    Root->SetObjectField(TEXT("Payload"), W.Object);
    FString J;
    FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&J));
    return J;
}
bool CostDecode(const FString &J, FSaveData &D, FString &E)
{
    TSharedPtr<FJsonObject> Root;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(J), Root);
    FRead R{Root->GetObjectField(TEXT("Payload")), E, true};
    D.Fields(R);
    return E.IsEmpty();
}
const FAccountingPeriod *CostPeriod(const FSimulation &S, const FString &A)
{
    return S.GetState().AccountingPeriods.FindByPredicate([&](const FAccountingPeriod &P) { return P.ActorId == A; });
}
} // namespace
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCostMath, "CoI.Accounting.CostFlowRoundingAndNoInternalMarkup",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCostMath::RunTest(const FString &)
{
    TestEqual(TEXT("10 USD plus 20 percent"), AccountingPrice(1000, 200, 1000000), 1200LL);
    TestEqual(TEXT("Higher transport cost increases cost plus price"), AccountingPrice(1500, 200, 1000000), 1800LL);
    int64 Markup = 0;
    TestTrue(TEXT("Fractional percent input"), Display::ParseMarkupInput(TEXT("20,5"), Markup));
    TestEqual(TEXT("Percent translated to permille"), Markup, 205LL);
    TestEqual(TEXT("Markup round trip"), Display::MarkupInput(205), FString(TEXT("20,5")));
    TestFalse(TEXT("No silent fractional truncation"), Display::ParseMarkupInput(TEXT("20,51"), Markup));
    TestEqual(TEXT("Money rounds upward exactly once"), AccountingPrice(101, 200, 1000000), 122LL);
    TestEqual(TEXT("Do not silently cap an unrepresentable cost plus price"), AccountingPrice(1000, 200, 100), 0LL);
    FAccountingCost Stock{1001, 107, 53, 29};
    auto First = AccountingPortion(Stock, 3, 10);
    FAccountingCost Rest{Stock.Material - First.Material, Stock.Labor - First.Labor, Stock.Operating - First.Operating,
                         Stock.Freight - First.Freight};
    auto Last = AccountingPortion(Rest, 7, 7);
    TestEqual(TEXT("Moving weighted costs retain all rounding residue"), First.Total() + Last.Total(), Stock.Total());
    TestTrue(TEXT("No negative cost components"),
             Rest.Material >= 0 && Rest.Labor >= 0 && Rest.Operating >= 0 && Rest.Freight >= 0);
    TestEqual(TEXT("Unit cost components sum to ceiling average"), AccountingUnit(Stock, 10).Total(), 119LL);
    TestEqual(TEXT("No division by zero"), AccountingUnit(Stock, 0).Total(), 0LL);
    FAccountingPeriod P;
    P.GoodsRevenue = 1200;
    P.CostOfGoods = 1000;
    P.Wages = 100;
    P.Operating = 50;
    P.CapitalizedWages = 40;
    P.CapitalizedOperating = 10;
    TestEqual(TEXT("Real realized result includes actual unallocated costs"), P.Profit(), 100LL);
    TestEqual(TEXT("Neither internal sale nor acquisition cash is revenue"), P.ServiceRevenue, 0LL);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCostOrders, "CoI.Accounting.AuthoritativeMarkupAndRealizedSale",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCostOrders::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    CostFixture(J, G, false, [](auto R) {
        R->SetStringField(TEXT("PlayerCompanyId"), TEXT("company.food"));
        for (auto X : R->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("Goods")))
            X->AsObject()->SetNumberField(TEXT("InitialUnitCostCents"), 1000);
    });
    FSimulation S;
    if (!S.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    auto Order = [&](const TCHAR *Id, const TCHAR *Actor, const TCHAR *Side, int64 Markup) {
        FCommand C;
        C.Kind = ECommandKind::PlaceOrder;
        auto &O = C.Order;
        O.Id = Id;
        O.ActorId = Actor;
        O.Side = Side;
        O.StorageId = TEXT("storage.food");
        O.CityId = TEXT("city.us.chicago");
        O.MarketId = TEXT("market.chicago");
        O.GoodId = TEXT("good.food");
        O.OriginalKg = 10;
        O.LimitCentsPerKg = Side == FString(TEXT("buy")) ? 1400 : 1;
        O.AccountingMarkupPermille = Markup;
        return C;
    };
    auto Sell = Order(TEXT("cost.sell"), TEXT("company.food"), TEXT("sell"), 200);
    TestTrue(TEXT("Queue sell"), QueueCost(S, Sell, E));
    TestTrue(TEXT("Queue funded buyer"),
             QueueCost(S, Order(TEXT("cost.buy"), TEXT("company.player"), TEXT("buy"), 0), E));
    S.StepOnce();
    const auto *O = Find(S.GetState().Orders, FString(TEXT("cost.sell")));
    if (!TestNotNull(TEXT("Accepted market sale"), O))
        return false;
    TestEqual(TEXT("Forged arbitrary price replaced centrally"), O->LimitCentsPerKg, 1200LL);
    TestEqual(TEXT("Actual quantity sold"), O->RemainingKg, 0LL);
    const auto *P = CostPeriod(S, TEXT("company.food"));
    if (!P)
        return false;
    TestEqual(TEXT("True external receipts"), P->GoodsRevenue, 12000LL);
    TestEqual(TEXT("Actual removed carrying cost"), P->CostOfGoods, 10000LL);
    const auto *Stock = S.GetState().Inventories.FindByPredicate(
        [](const FInventory &I) { return I.OwnerId == TEXT("company.player") && I.StorageId == TEXT("storage.food"); });
    if (!Stock)
        return false;
    TestEqual(TEXT("Buyer uses actual acquisition price"), Stock->BookValueCents, 12000LL);
    TestTrue(TEXT("New margin queued"),
             QueueCost(S, Order(TEXT("cost.next"), TEXT("company.food"), TEXT("sell"), 400), E));
    S.StepOnce();
    TestEqual(TEXT("Old order never retrospectively repriced"),
              Find(S.GetState().Orders, FString(TEXT("cost.sell")))->LimitCentsPerKg, 1200LL);
    TestEqual(TEXT("New sale uses new markup"), Find(S.GetState().Orders, FString(TEXT("cost.next")))->LimitCentsPerKg,
              1400LL);
    auto Stale = Order(TEXT("cost.stale"), TEXT("company.food"), TEXT("sell"), 200);
    Stale.Order.AccountingPriceVersion = 1;
    QueueCost(S, Stale, E);
    S.StepOnce();
    TestFalse(TEXT("Outdated price-basis preview rejected"), S.GetResults().Last().Accepted);
    TestTrue(TEXT("Stale order left no partial reservation"),
             Find(S.GetState().Orders, FString(TEXT("cost.stale"))) == nullptr);
    auto Invalid = Order(TEXT("cost.bad"), TEXT("company.food"), TEXT("sell"), 10001);
    QueueCost(S, Invalid, E);
    S.StepOnce();
    TestFalse(TEXT("Out of range markup rejected centrally"), S.GetResults().Last().Accepted);
    TestTrue(TEXT("Conservation"), S.AuditRuntime(E));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCostTransport, "CoI.Accounting.RealFreightAndSaveInTransit",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCostTransport::RunTest(const FString &)
{
    FString J, E, Disk;
    FCoIGeoMapData G;
    CostFixture(J, G);
    FSimulation S, R;
    if (!S.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    QueueCost(S, CostTrip(), E);
    S.StepOnce();
    S.StepOnce();
    const auto *T = Find(S.GetState().Transports, FString(TEXT("cost.trip")));
    if (!TestNotNull(TEXT("Real transport booked"), T))
        return false;
    TestEqual(TEXT("In transit"), T->Status, FString(TEXT("moving")));
    const auto *Cargo = Find(S.GetState().Inventories, T->CargoInventoryId);
    if (!Cargo)
        return false;
    TestEqual(TEXT("Freight in actual carrying cost"), Cargo->AccountingCost.Freight, T->CostCents);
    TestEqual(TEXT("Breakdown exact"), Cargo->BookValueCents, Cargo->AccountingCost.Total());
    TestEqual(TEXT("Loaded freight not expensed twice"), CostPeriod(S, TEXT("company.player"))->FreightExpense, 0LL);
    TestTrue(TEXT("Save while cargo moving"), S.SaveGameJson(Disk, E));
    int32 Speed = 1;
    if (!R.LoadGameJson(Disk, G, S.GetScenario(), E, Speed))
    {
        AddError(E);
        return false;
    }
    for (int H = 0; H < 50; ++H)
    {
        S.StepOnce();
        R.StepOnce();
        if (!S.AuditRuntime(E))
        {
            AddError(E);
            return false;
        }
        TestEqual(TEXT("Exact continuation, no duplicate freight or sale"), S.Fingerprint(), R.Fingerprint());
    }
    TestEqual(TEXT("Delivered once"), Find(S.GetState().Transports, FString(TEXT("cost.trip")))->Status,
              FString(TEXT("arrived")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCostInternal, "CoI.Accounting.InternalValuationEliminatedAndIdleCosts",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCostInternal::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    CostFixture(J, G);
    FSimulation S;
    if (!S.Load(J, G, E))
        return false;
    FCommand C;
    C.Kind = ECommandKind::Delivery;
    C.Delivery.Id = TEXT("cost.internal");
    C.Delivery.OwnerId = TEXT("company.player");
    C.Delivery.Action = TEXT("create");
    auto &T = C.Delivery.Terms;
    T.SourceAssetId = TEXT("storage.player");
    T.DestinationAssetId = TEXT("retail.store.chicago.player");
    T.FromCityId = TEXT("city.us.detroit");
    T.ToCityId = TEXT("city.us.chicago");
    T.GoodId = TEXT("good.food");
    T.VehicleId = TEXT("vehicle.player");
    T.Kg = 10;
    T.IntervalHours = 24;
    T.StartHour = 1;
    T.EndHour = 2;
    T.PriceCentsPerKg = 999;
    T.MaxFreightCents = 100000;
    QueueCost(S, C, E);
    bool Internal = false;
    for (int H = 0; H < 48; ++H)
    {
        S.StepOnce();
        if (!S.AuditRuntime(E))
        {
            AddError(E);
            return false;
        }
        for (const auto &X : S.GetState().InternalTransfers)
            if (X.ActorId == TEXT("company.player"))
                Internal |= X.ValueCents > 0;
    }
    TestTrue(TEXT("Real internal delivery with separate site valuation completed"), Internal);
    int64 Retail = 0;
    for (const auto &F : S.GetState().CashFlows)
        if (F.Reason == TEXT("retail_sale") &&
            Find(S.GetState().Accounts, F.ToAccountId)->OwnerId == TEXT("company.player"))
            Retail += F.Cents;
    const auto *P = CostPeriod(S, TEXT("company.player"));
    TestEqual(TEXT("Only real external retail sales in company revenue"), P->GoodsRevenue, Retail);
    TestTrue(TEXT("Internal markup not capitalized into own inventory"), P->GoodsRevenue < 9990);
    TestTrue(TEXT("Unallocated costs charged with zero/low sales"), P->IdleCosts() > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCostMonth, "CoI.Accounting.FortyDaysMonthCloseAndSaveReplay",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCostMonth::RunTest(const FString &)
{
    FString J, E, Disk;
    FCoIGeoMapData G;
    CostFixture(J, G);
    FSimulation S, R;
    if (!S.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    auto NetPosition = [](const FState &W, const FString &Actor) {
        int64 N = 0;
        for (const auto &A : W.Accounts)
            if (A.OwnerId == Actor)
                N += A.BalanceCents;
        for (const auto &I : W.Inventories)
            if (I.OwnerId == Actor)
                N += I.BookValueCents - I.RentArrearsCents;
        for (const auto &A : W.AccountingAssets)
            if (A.OwnerId == Actor && A.Known)
                N += A.ValueCents;
        for (const auto &A : W.Warehouses)
            if (A.OwnerId == Actor)
                N -= A.ArrearsCents;
        for (const auto &A : W.Vehicles)
            if (A.OwnerId == Actor)
                N -= A.ArrearsCents;
        for (const auto &A : W.Facilities)
            if (A.OwnerId == Actor)
                N -= A.OperatingArrearsCents;
        for (const auto &I : W.Inventories)
            if (const auto *Store = Find(W.Warehouses, I.StorageId);
                Store && Store->OwnerId == Actor && I.OwnerId != Actor)
                N += I.RentArrearsCents;
        for (const auto &T : W.Transports)
            if (T.Status == TEXT("planned") && T.AccountingServiceCents)
            {
                if (T.AccountingServicePayerId == Actor)
                    N += T.AccountingServiceCents;
                if (T.OwnerId == Actor)
                    N -= T.AccountingServiceCents;
            }
        return N;
    };
    const int64 Opening = NetPosition(S.GetState(), TEXT("company.player"));
    TMap<FString, int64> OpeningCompanies;
    for (const auto &A : S.GetState().Companies)
        OpeningCompanies.Add(A.Id, NetPosition(S.GetState(), A.Id));
    int64 Initial = 0;
    for (const auto &A : S.GetState().Accounts)
        Initial += A.BalanceCents;
    FString Csv = TEXT("hour,total_money_cents,player_external_revenue_cents,player_cogs_cents,player_period_cost_"
                       "cents,player_profit_cents,price_version,period_begin_hour,period_end_hour\n");
    for (int H = 0; H < 40 * 24; ++H)
    {
        S.StepOnce();
        if (R.IsLoaded())
        {
            R.StepOnce();
            if (S.Fingerprint() != R.Fingerprint())
            {
                AddError(TEXT("Monthly save continuation diverged"));
                return false;
            }
        }
        if (!S.ValidateSaveState(E))
        {
            AddError(FString::Printf(TEXT("h%d: %s"), H + 1, *E));
            return false;
        }
        if (H == 742)
        {
            if (!S.SaveGameJson(Disk, E))
            {
                AddError(E);
                return false;
            }
            int32 Speed = 1;
            if (!R.LoadGameJson(Disk, G, S.GetScenario(), E, Speed))
            {
                AddError(E);
                return false;
            }
        }
        if (H + 1 == 31 * 24)
        {
            const auto *Month = CostPeriod(S, TEXT("company.player"));
            TestEqual(TEXT("Independent cash, carrying assets and liabilities reconcile to realized monthly profit"),
                      NetPosition(S.GetState(), TEXT("company.player")) - Opening, Month->Profit());
            for (const auto &A : S.GetState().Companies)
            {
                const auto *P = CostPeriod(S, A.Id);
                TestEqual(*FString::Printf(TEXT("Independent monthly balance bridge %s"), *A.Id),
                          NetPosition(S.GetState(), A.Id) - OpeningCompanies[A.Id], P->Profit());
            }
        }
        if ((H + 1) % 24 == 0)
        {
            int64 Cash = 0;
            for (const auto &A : S.GetState().Accounts)
                Cash += A.BalanceCents;
            TestEqual(TEXT("No invisible money source"), Cash, Initial);
            const FAccountingPeriod *P = nullptr;
            for (const auto &Row : S.GetState().AccountingPeriods)
                if (Row.ActorId == TEXT("company.player") && (!P || Row.BeginHour > P->BeginHour))
                    P = &Row;
            Csv += FString::Printf(TEXT("%d,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld\n"), H + 1, Cash,
                                   P->GoodsRevenue + P->ServiceRevenue, P->CostOfGoods,
                                   P->IdleCosts() + P->FreightExpense + P->Spoilage, P->Profit(),
                                   S.GetState().AccountingPriceSequence, P->BeginHour, P->EndHour);
        }
    }
    const auto *Jan = CostPeriod(S, TEXT("company.player"));
    if (!Jan)
        return false;
    TestTrue(TEXT("January closed once"), Jan->Complete && !Jan->Partial && Jan->EndHour == 31 * 24);
    TestTrue(TEXT("Real month available in UI01 contract"),
             S.GetSnapshot().Player.Company.MonthlyResult.State == EDisplayState::Value);
    TestEqual(TEXT("Shared monthly result"), S.GetSnapshot().Player.Company.MonthlyResult.Value, Jan->Profit());
    TestTrue(TEXT("Actual idle costs with no guaranteed profit"), Jan->IdleCosts() > 0);
    FFileHelper::SaveStringToFile(Csv, *(FPaths::ProjectSavedDir() / TEXT("Diagnostics/AP09.4_40days.csv")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCostCorruption, "CoI.Accounting.CurrentSaveCorruptionRejected", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCostCorruption::RunTest(const FString&){FString J,E,Disk;FCoIGeoMapData G;CostFixture(J,G);FSimulation S;if(!S.Load(J,G,E))return false;S.StepOnce();if(!S.SaveGameJson(Disk,E))return false;FSaveData D;if(!CostDecode(Disk,D,E))return false;D.State.Inventories[0].AccountingCost.Material+=1;const auto Before=S.Fingerprint();int32 Speed=1;TestFalse(TEXT("Invalid current-format costs rejected despite valid checksum"),S.LoadGameJson(CostEncode(D),G,S.GetScenario(),E,Speed));TestEqual(TEXT("Failed load preserves live world"),S.Fingerprint(),Before);return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCostConfig, "CoI.Accounting.InvalidConfigurationAndStableZeroSalesBasis",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCostConfig::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    CostFixture(J, G, false, [](auto R) {
        R->GetObjectField(TEXT("Economy"))->SetNumberField(TEXT("AccountingNormalUtilizationPermille"), 0);
    });
    FSimulation Bad;
    TestFalse(TEXT("Reject normal capacity zero before division"), Bad.Load(J, G, E));
    CostFixture(J, G);
    FSimulation S;
    if (!S.Load(J, G, E))
        return false;
    const auto Initial = *AccountingFindPrice(S.GetState().AccountingPrices, TEXT("company.player"),
                                              TEXT("retail.store.chicago.player"), TEXT("good.food"));
    TestTrue(TEXT("Empty shop explicitly uses startup basis"), Initial.Startup);
    for (int H = 0; H < 24; ++H)
        S.StepOnce();
    const auto *Later = AccountingFindPrice(S.GetState().AccountingPrices, TEXT("company.player"),
                                            TEXT("retail.store.chicago.player"), TEXT("good.food"));
    TestEqual(TEXT("No sales does not recursively inflate costs"), Later->Base(), Initial.Base());
    TestEqual(TEXT("Absent stock remains absent"),
              S.GetState().Inventories.ContainsByPredicate([](const FInventory &I) {
                  return I.StorageId == TEXT("retail.store.chicago.player") && I.QuantityKg > 0;
              }),
              false);
    TestTrue(TEXT("Cost source version changes only on scheduled publication"), Later->Version > Initial.Version);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCostConstruction, "CoI.Accounting.ConstructionIsCapitalAndFeesFollowRealAssessments",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCostConstruction::RunTest(const FString &)
{
    FString J, E, Disk;
    FCoIGeoMapData G;
    CostFixture(J, G, false, [](auto R) {
        for (auto X : R->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("Construction")))
            if (X->AsObject()->GetStringField(TEXT("Id")) == TEXT("build.warehouse"))
                X->AsObject()->SetNumberField(TEXT("DurationHours"), 3);
    });
    FSimulation S, R;
    if (!S.Load(J, G, E))
        return false;
    FCommand C;
    C.Kind = ECommandKind::Invest;
    C.Investment.Id = TEXT("cost.build");
    C.Investment.OwnerId = TEXT("company.player");
    C.Investment.CityId = TEXT("city.us.detroit");
    C.Investment.Action = TEXT("build");
    C.Investment.DefinitionId = TEXT("build.warehouse");
    C.Investment.PlotId = TEXT("plot.city.us.detroit.warehouse.1");
    QueueCost(S, C, E);
    S.StepOnce();
    S.StepOnce();
    if (!S.SaveGameJson(Disk, E))
        return false;
    int32 Speed = 1;
    if (!R.LoadGameJson(Disk, G, S.GetScenario(), E, Speed))
        return false;
    S.StepOnce();
    R.StepOnce();
    TestEqual(TEXT("Build completion replay"), S.Fingerprint(), R.Fingerprint());
    const auto *Asset = Find(S.GetState().AccountingAssets, FString(TEXT("cost.build.asset")));
    if (!TestNotNull(TEXT("Real completed construction capitalized"), Asset))
        return false;
    TestEqual(TEXT("Actual construction bill as carrying amount"), Asset->ValueCents, 200000LL);
    const auto *M = CostPeriod(S, TEXT("company.player"));
    TestEqual(TEXT("Construction payment is not an operating expense"), M->Operating, 300LL);
    TestTrue(TEXT("New warehouse not charged a hypothetical early fee"),
             Find(S.GetState().AccountingCharges, FString(TEXT("cost.build.asset"))) == nullptr);
    for (int H = 3; H < 25; ++H)
        S.StepOnce();
    const auto *Charge = Find(S.GetState().AccountingCharges, FString(TEXT("cost.build.asset")));
    if (!TestNotNull(TEXT("Real next daily fee exists"), Charge))
        return false;
    TestEqual(TEXT("Assessed at real 24h boundary"), Charge->Hour, 24LL);
    TestTrue(TEXT("New build and actual charges remain valid"), S.AuditRuntime(E));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCostAssetSale, "CoI.Accounting.AssetDisposalGainAndBuyerCarryingAmount",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCostAssetSale::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    CostFixture(J, G);
    FSimulation S;
    if (!S.Load(J, G, E))
        return false;
    FCommand C;
    C.Kind = ECommandKind::Invest;
    auto &B = C.Investment;
    B.Id = TEXT("cost.asset.buy");
    B.OwnerId = TEXT("company.player");
    B.CityId = TEXT("city.us.pittsburgh");
    B.Action = TEXT("buy");
    B.TargetId = TEXT("sale.pittsburgh.warehouse");
    QueueCost(S, C, E);
    S.StepOnce();
    if (!TestTrue(TEXT("Real explicit asset sale accepted"), S.GetResults().Last().Accepted))
        return false;
    const auto *A = Find(S.GetState().AccountingAssets, FString(TEXT("storage.service.pittsburgh")));
    if (!A)
        return false;
    TestEqual(TEXT("Asset book follows buyer exactly once"), A->OwnerId, FString(TEXT("company.player")));
    TestEqual(TEXT("Buyer uses actual acquisition bill, not seller's old basis"), A->ValueCents, 300000LL);
    TestEqual(TEXT("Seller realizes price less explicit opening carrying amount"),
              CostPeriod(S, TEXT("company.services"))->DisposalGain, 250000LL);
    TestEqual(TEXT("No buyer profit from an investment"), CostPeriod(S, TEXT("company.player"))->DisposalGain, 0LL);
    return TestTrue(TEXT("Capital transaction invariants"), S.AuditRuntime(E));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCostReturn, "CoI.Accounting.PrepaidReturnEarnedAtDepartureAndSaved",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCostReturn::RunTest(const FString &)
{
    FString J, E, Disk;
    FCoIGeoMapData G;
    CostFixture(J, G);
    FSimulation S, R;
    if (!S.Load(J, G, E))
        return false;
    FCommand C;
    C.Kind = ECommandKind::Delivery;
    C.Delivery.Id = TEXT("cost.hired.internal");
    C.Delivery.OwnerId = TEXT("company.player");
    C.Delivery.Action = TEXT("create");
    auto &T = C.Delivery.Terms;
    T.SourceAssetId = TEXT("storage.player");
    T.DestinationAssetId = TEXT("retail.store.chicago.player");
    T.FromCityId = TEXT("city.us.detroit");
    T.ToCityId = TEXT("city.us.chicago");
    T.GoodId = TEXT("good.food");
    T.Kg = 10;
    T.IntervalHours = 24;
    T.StartHour = 0;
    T.EndHour = 1;
    T.PriceCentsPerKg = 999;
    T.MaxFreightCents = 100000;
    QueueCost(S, C, E);
    bool Checked = false;
    for (int H = 0; H < 60 && !Checked; ++H)
    {
        S.StepOnce();
        if (!S.AuditRuntime(E))
        {
            AddError(E);
            return false;
        }
        for (const auto &Back : S.GetState().Transports)
            if (Back.AccountingServiceCents)
            {
                const auto BeforeProvider = CostPeriod(S, Back.OwnerId)->ServiceRevenue;
                const auto BeforeBuyer = CostPeriod(S, Back.AccountingServicePayerId)->FreightExpense;
                const auto Amount = Back.AccountingServiceCents;
                const auto JobId = Back.Id;
                const auto Provider = Back.OwnerId;
                const auto Buyer = Back.AccountingServicePayerId;
                TestEqual(TEXT("Prepaid return still planned"), Back.Status, FString(TEXT("planned")));
                if (!S.SaveGameJson(Disk, E))
                    return false;
                int32 Speed = 1;
                if (!R.LoadGameJson(Disk, G, S.GetScenario(), E, Speed))
                {
                    AddError(E);
                    return false;
                }
                S.StepOnce();
                R.StepOnce();
                TestEqual(TEXT("Prepayment survives save exactly"), S.Fingerprint(), R.Fingerprint());
                TestEqual(TEXT("Provider earns service only on departure"),
                          CostPeriod(S, Provider)->ServiceRevenue - BeforeProvider, Amount);
                TestEqual(TEXT("Buyer realizes return expense exactly once"),
                          CostPeriod(S, Buyer)->FreightExpense - BeforeBuyer, Amount);
                TestEqual(TEXT("Deferral cleared after recognition"),
                          Find(S.GetState().Transports, JobId)->AccountingServiceCents, 0LL);
                Checked = true;
                break;
            }
    }
    if (!Checked)
    {
        for (const auto &InfoRow : S.GetResults())
            AddInfo(InfoRow.Reason);
        for (const auto &InfoRow : S.GetState().DeliverySlots)
            AddInfo(InfoRow.Status + TEXT(": ") + InfoRow.Cause);
        for (const auto &JobRow : S.GetState().Transports)
            AddInfo(JobRow.Id + TEXT(": ") + JobRow.Status + TEXT(" provider ") + JobRow.ProviderId);
    }
    TestTrue(TEXT("Real hired return path exercised"), Checked);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCostPaidWage, "CoI.Accounting.RetailUpgradeDoesNotRepricePaidWages",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCostPaidWage::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    CostFixture(J, G, false, [](auto R) {
        for (auto X : R->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("WarehouseTypes")))
            if (X->AsObject()->GetStringField(TEXT("Id")) == TEXT("retail.type.food.large"))
                X->AsObject()->SetNumberField(TEXT("RetailWageCents"), 350);
        for (auto X : R->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("Construction")))
            if (X->AsObject()->GetStringField(TEXT("Id")) == TEXT("retail.build.food.large"))
                X->AsObject()->SetNumberField(TEXT("DurationHours"), 3);
    });
    FSimulation S;
    if (!S.Load(J, G, E))
        return false;
    FCommand C;
    C.Kind = ECommandKind::Invest;
    C.Investment.Id = TEXT("cost.retail.upgrade");
    C.Investment.OwnerId = TEXT("company.player");
    C.Investment.CityId = TEXT("city.us.chicago");
    C.Investment.Action = TEXT("upgrade");
    C.Investment.DefinitionId = TEXT("retail.build.food.large");
    C.Investment.TargetId = TEXT("retail.store.chicago.player");
    QueueCost(S, C, E);
    for (int H = 0; H < 24; ++H)
        S.StepOnce();
    TestEqual(TEXT("Upgrade completed mid-payroll period"),
              Find(S.GetState().Warehouses, FString(TEXT("retail.store.chicago.player")))->TypeId,
              FString(TEXT("retail.type.food.large")));
    TestEqual(TEXT("Actual old wage retained until next payroll"),
              Find(S.GetState().ServiceEmployment, FString(TEXT("retail.store.chicago.player")))
                  ->AccountingPaidDailyWageCents,
              150LL);
    TestEqual(TEXT("No retrospective wage expense from changed type"), CostPeriod(S, TEXT("company.player"))->Wages,
              900LL);
    S.StepOnce();
    TestEqual(TEXT("New wage applies only after real funded payroll"),
              Find(S.GetState().ServiceEmployment, FString(TEXT("retail.store.chicago.player")))
                  ->AccountingPaidDailyWageCents,
              350LL);
    return TestTrue(TEXT("Upgrade cost invariants"), S.AuditRuntime(E));
}

#endif

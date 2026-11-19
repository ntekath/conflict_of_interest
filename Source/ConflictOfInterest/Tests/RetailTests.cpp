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
bool RetailFixture(FString &J, FCoIGeoMapData &G, TFunction<void(TSharedPtr<FJsonObject>)> Edit = {},
                   bool Legacy = false)
{
    FString E;
    auto *C = LoadObject<UCoIGeoCatalog>(nullptr, TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));
    if (!C || !G.Load(C->DefinitionJson, E) ||
        !FFileHelper::LoadFileToString(J, *(FPaths::ProjectContentDir() / TEXT("Simulation/") +
                                            (Legacy ? TEXT("ap092_legacy_fixture.coisim") : TEXT("scenario.coisim")))))
        return false;
    TSharedPtr<FJsonObject> R;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(J), R);
    if (Edit)
        Edit(R);
    for (const auto &Name : {TEXT("Definitions"), TEXT("InitialState")})
        for (auto &Pair : R->GetObjectField(Name)->Values)
        {
            const TArray<TSharedPtr<FJsonValue>> *Rows = nullptr;
            if (!Pair.Value->TryGetArray(Rows) || Rows->IsEmpty() || !(*Rows)[0]->AsObject()->HasField(TEXT("Id")))
                continue;
            auto Sorted = *Rows;
            Sorted.Sort([](const auto &A, const auto &B) {
                return A->AsObject()->GetStringField(TEXT("Id")) < B->AsObject()->GetStringField(TEXT("Id"));
            });
            Pair.Value = MakeShared<FJsonValueArray>(Sorted);
        }
    J.Empty();
    FJsonSerializer::Serialize(R.ToSharedRef(), TJsonWriterFactory<>::Create(&J));
    return true;
}
template <class T> FString RetailHash(const T &X, bool Legacy = false)
{
    FFingerprint H;
    H.LegacyRetail = Legacy;
    H.Write(X);
    return H.Finish();
}
TSharedPtr<FJsonObject> RetailStock(const FString &Id, const FString &Good, const FString &Owner, const FString &Store,
                                    int64 Kg)
{
    auto X = MakeShared<FJsonObject>();
    X->SetStringField(TEXT("Id"), Id);
    X->SetStringField(TEXT("GoodId"), Good);
    X->SetStringField(TEXT("OwnerId"), Owner);
    X->SetStringField(TEXT("StorageId"), Store);
    X->SetNumberField(TEXT("QuantityKg"), Kg);
    X->SetNumberField(TEXT("ReservedKg"), 0);
    return X;
}
void TwoStockedShops(TSharedPtr<FJsonObject> R)
{
    auto D = R->GetObjectField(TEXT("Definitions"));
    for (auto T : D->GetArrayField(TEXT("WarehouseTypes")))
    {
        const TArray<TSharedPtr<FJsonValue>> *Offers = nullptr;
        if (T->AsObject()->TryGetArrayField(TEXT("RetailOffers"), Offers))
            for (auto O : *Offers)
                O->AsObject()->SetNumberField(TEXT("TargetKg"), 0);
    }
    for (auto N : D->GetArrayField(TEXT("Needs")))
    {
        N->AsObject()->SetNumberField(TEXT("BasicKgPerThousandPerDay"), 10);
        N->AsObject()->SetNumberField(TEXT("LuxuryKgPerThousandPerDay"), 0);
    }
    auto W = R->GetObjectField(TEXT("InitialState"));
    auto Stocks = W->GetArrayField(TEXT("Inventories"));
    for (auto X : Stocks)
        if (X->AsObject()->GetStringField(TEXT("Id")) == TEXT("stock.food.food"))
            X->AsObject()->SetNumberField(TEXT("QuantityKg"), 0);
    Stocks.Add(MakeShared<FJsonValueObject>(RetailStock(
        TEXT("test.retail.a"), TEXT("good.food"), TEXT("company.player"), TEXT("retail.store.chicago.player"), 500)));
    Stocks.Add(
        MakeShared<FJsonValueObject>(RetailStock(TEXT("test.retail.b"), TEXT("good.food"), TEXT("company.merchant"),
                                                 TEXT("retail.store.chicago.merchant"), 500)));
    W->SetArrayField(TEXT("Inventories"), Stocks);
}
FCommand RetailCommand(FSimulation &S, ECommandKind K)
{
    FCommand C;
    C.Kind = K;
    C.Generation = S.GetGeneration();
    C.Sequence = S.NextSequence();
    C.TargetStep = S.GetState().Step + 1;
    C.Origin = ECommandOrigin::Player;
    C.CompanyId = S.GetSession().PlayerCompanyId;
    return C;
}
FString RetailV4(FSimulation &S, FString &E)
{
    FString Disk;
    if (!S.SaveGameJson(Disk, E))
        return {};
    TSharedPtr<FJsonObject> Root;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Disk), Root);
    FSaveData D;
    FRead Read{Root->GetObjectField(TEXT("Payload")), E, true};
    D.Fields(Read);
    D.FormatVersion = D.SimulationVersion = D.DefinitionsVersion = 4;
    auto Scenario = D.Scenario;
    Scenario.Settings.Seed = 0;
    Scenario.PlayerCompanyId.Empty();
    D.CompatibilityHash = RetailHash(Scenario, true);
    FWrite W;
    W.LegacyRetail = true;
    D.Fields(W);
    auto Out = MakeShared<FJsonObject>();
    Out->SetStringField(TEXT("Format"), TEXT("CoI.Save"));
    Out->SetStringField(TEXT("Checksum"), RetailHash(D, true));
    Out->SetObjectField(TEXT("Payload"), W.Object);
    Disk.Empty();
    FJsonSerializer::Serialize(Out, TJsonWriterFactory<>::Create(&Disk));
    return Disk;
}
} // namespace
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRetailWeights, "CoI.Retail.WeightedPriceQualityAndRealPayments",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRetailWeights::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    for (int Variant = 0; Variant < 3; ++Variant)
    {
        RetailFixture(J, G, [&](auto R) {
            TwoStockedShops(R);
            if (Variant)
                for (auto T : R->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("WarehouseTypes")))
                    if (T->AsObject()->GetStringField(TEXT("Id")) == TEXT("retail.type.food"))
                    { /* Both shops share a type. A distinct second type makes the price/quality comparison independent.
                       */
                        auto Copy = MakeShared<FJsonObject>(*T->AsObject());
                        auto Offers = Copy->GetArrayField(TEXT("RetailOffers"));
                        auto O = MakeShared<FJsonObject>(*Offers[0]->AsObject());
                        O->SetNumberField(Variant == 1 ? TEXT("PriceCents") : TEXT("QualityPermille"),
                                          Variant == 1 ? 6 : 250);
                        Offers[0] = MakeShared<FJsonValueObject>(O);
                        Copy->SetArrayField(TEXT("RetailOffers"), Offers);
                        Copy->SetStringField(TEXT("Id"), TEXT("retail.type.test"));
                        auto Types = R->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("WarehouseTypes"));
                        Types.Add(MakeShared<FJsonValueObject>(Copy));
                        R->GetObjectField(TEXT("Definitions"))->SetArrayField(TEXT("WarehouseTypes"), Types);
                        for (auto W : R->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Warehouses")))
                            if (W->AsObject()->GetStringField(TEXT("Id")) == TEXT("retail.store.chicago.player"))
                                W->AsObject()->SetStringField(TEXT("TypeId"), TEXT("retail.type.test"));
                        break;
                    }
        });
        FSimulation S;
        if (!S.Load(J, G, E))
        {
            AddError(E);
            return false;
        }
        S.StepOnce();
        const auto *A = Find(S.GetState().Warehouses, FString(TEXT("retail.store.chicago.player")));
        const auto *B = Find(S.GetState().Warehouses, FString(TEXT("retail.store.chicago.merchant")));
        TestEqual(TEXT("Demand consumed once"), A->RetailSoldKg + B->RetailSoldKg, 100LL);
        if (!Variant)
            TestEqual(TEXT("Equal offers share demand"), A->RetailSoldKg, B->RetailSoldKg);
        if (Variant == 1)
            TestTrue(TEXT("Lower price sells more"), A->RetailSoldKg > B->RetailSoldKg);
        if (Variant == 2)
            TestTrue(TEXT("Lower quality sells less"), A->RetailSoldKg < B->RetailSoldKg);
        const auto *Paid = Find(S.GetState().CashTotals, FString(TEXT("retail_sale")));
        TestEqual(TEXT("Households really pay owners"), Paid ? Paid->Cents : 0,
                  A->RetailRevenueCents + B->RetailRevenueCents);
        for (const auto &O : S.GetState().Orders)
            TestNull(TEXT("No household in wholesale book"), Find(S.GetState().Groups, O.ActorId));
        if (!S.ValidateSaveState(E))
        {
            AddError(E);
            return false;
        }
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRetailEmpty, "CoI.Retail.ProducerStockIsNotRetailAndBudgetLimits",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRetailEmpty::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    RetailFixture(J, G, [](auto R) {
        for (auto T : R->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("WarehouseTypes")))
        {
            const TArray<TSharedPtr<FJsonValue>> *A = nullptr;
            if (T->AsObject()->TryGetArrayField(TEXT("RetailOffers"), A))
                for (auto O : *A)
                    O->AsObject()->SetNumberField(TEXT("TargetKg"), 0);
        }
    });
    FSimulation S;
    if (!S.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    S.StepOnce();
    TestEqual(TEXT("Producer goods alone satisfy no household"),
              Find(S.GetState().GoodsAudit, FString(TEXT("good.food")))->ConsumedKg, 0LL);
    auto C = RetailCommand(S, ECommandKind::PlaceOrder);
    C.Origin = ECommandOrigin::System;
    C.Order.Id = TEXT("test.illegal.household");
    C.Order.ActorId = TEXT("group.chicago.high");
    C.Order.GoodId = TEXT("good.food");
    C.Order.StorageId = TEXT("storage.food");
    C.Order.CityId = TEXT("city.us.chicago");
    C.Order.MarketId = TEXT("market.chicago");
    C.Order.Side = TEXT("buy");
    C.Order.OriginalKg = 1;
    C.Order.LimitCentsPerKg = 12;
    S.Enqueue(C, E);
    S.StepOnce();
    TestFalse(TEXT("Producer direct sale path centrally refused"), S.GetResults().Last().Accepted);
    RetailFixture(J, G, [](auto R) {
        TwoStockedShops(R);
        R->GetObjectField(TEXT("Economy"))->SetNumberField(TEXT("DailyBudgetPermille"), 0);
    });
    FSimulation Poor;
    if (!Poor.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    Poor.StepOnce();
    TestEqual(TEXT("No budget, no free purchase"),
              Find(Poor.GetState().GoodsAudit, FString(TEXT("good.food")))->ConsumedKg, 0LL);
    int64 Need = 0;
    for (const auto &X : Poor.GetState().Groups)
        for (const auto &U : X.UnmetNeed)
            Need += U.Kg;
    TestTrue(TEXT("Unmet need survives without credit"), Need > 0);
    return TestTrue(TEXT("Money and goods remain balanced"), Poor.AuditRuntime(E));
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRetailPriority, "CoI.Retail.BasicPriorityAndFiniteInventory",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRetailPriority::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    RetailFixture(J, G, [](auto R) {
        TwoStockedShops(R);
        for (auto N : R->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("Needs")))
            N->AsObject()->SetNumberField(TEXT("LuxuryKgPerThousandPerDay"), 10);
        for (auto W : R->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Warehouses")))
            if (W->AsObject()->GetStringField(TEXT("Id")) == TEXT("retail.store.chicago.player"))
                W->AsObject()->SetStringField(TEXT("TypeId"), TEXT("retail.type.mixed"));
        auto State = R->GetObjectField(TEXT("InitialState"));
        auto Stocks = State->GetArrayField(TEXT("Inventories"));
        for (auto X : Stocks)
        {
            auto O = X->AsObject();
            auto Id = O->GetStringField(TEXT("Id"));
            if (Id == TEXT("test.retail.a"))
                O->SetNumberField(TEXT("QuantityKg"), 1);
            if (Id == TEXT("test.retail.b"))
                O->SetNumberField(TEXT("QuantityKg"), 0);
            if (Id == TEXT("stock.luxury.luxury"))
                O->SetNumberField(TEXT("QuantityKg"), 950);
        }
        Stocks.Add(MakeShared<FJsonValueObject>(RetailStock(
            TEXT("test.lux"), TEXT("good.luxury"), TEXT("company.player"), TEXT("retail.store.chicago.player"), 50)));
        State->SetArrayField(TEXT("Inventories"), Stocks);
        for (auto T : R->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("WarehouseTypes")))
        {
            const TArray<TSharedPtr<FJsonValue>> *A = nullptr;
            if (T->AsObject()->TryGetArrayField(TEXT("RetailOffers"), A))
                for (auto O : *A)
                    O->AsObject()->SetNumberField(TEXT("TargetKg"), 0);
        }
    });
    FSimulation S;
    if (!S.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    S.StepOnce();
    TestEqual(TEXT("Only real 1 kg food is sold"),
              Find(S.GetState().GoodsAudit, FString(TEXT("good.food")))->ConsumedKg, 1LL);
    TestEqual(TEXT("No luxury before basic fulfillment"),
              Find(S.GetState().GoodsAudit, FString(TEXT("good.luxury")))->ConsumedKg, 0LL);
    return TestTrue(TEXT("No inventory duplication"), S.AuditRuntime(E));
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRetailSupply, "CoI.Retail.RealSupplyAndThirtyDaySaveReplay",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRetailSupply::RunTest(const FString &)
{
    FString J, E, Disk;
    FCoIGeoMapData G;
    RetailFixture(J, G);
    FSimulation S, Replay;
    if (!S.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    FString Csv = TEXT(
        "hour,money_cents,household_cash_cents,retail_receipts_cents,food_consumed_kg,luxury_consumed_kg,step_us\n");
    double MaxUs = 0;
    bool Saved = false, Arrived = false;
    for (int H = 0; H < 24 * 30; ++H)
    {
        S.StepOnce();
        if (Replay.IsLoaded())
        {
            Replay.StepOnce();
            if (S.Fingerprint() != Replay.Fingerprint())
            {
                AddError(TEXT("Retail continuation differs after load"));
                return false;
            }
        }
        if (!S.ValidateSaveState(E))
        {
            AddError(FString::Printf(TEXT("h%d: %s"), H, *E));
            return false;
        }
        MaxUs = FMath::Max(MaxUs, S.GetDiagnostics().StepMicros);
        if ((H + 1) % 24 == 0)
        {
            int64 Money = 0, Cash = 0;
            for (const auto &A : S.GetState().Accounts)
            {
                Money += A.BalanceCents;
                if (Find(S.GetState().Groups, A.OwnerId))
                    Cash += A.BalanceCents;
            }
            const auto *Receipt = Find(S.GetState().CashTotals, FString(TEXT("retail_sale")));
            Csv += FString::Printf(
                TEXT("%d,%lld,%lld,%lld,%lld,%lld,%.1f\n"), H + 1, Money, Cash, Receipt ? Receipt->Cents : 0,
                Find(S.GetState().GoodsAudit, FString(TEXT("good.food")))->ConsumedKg,
                Find(S.GetState().GoodsAudit, FString(TEXT("good.luxury")))->ConsumedKg, S.GetDiagnostics().StepMicros);
        }
        if (!H)
        {
            int64 Sold = 0;
            for (const auto &W : S.GetState().Warehouses)
                Sold += W.RetailSoldKg;
            TestEqual(TEXT("No instant first-hour stock transfer"), Sold, 0LL);
        }
        for (const auto &X : S.GetState().DeliverySlots)
            if (X.ContractId.StartsWith(TEXT("retail.supply.")))
            {
                const auto *Out = Find(S.GetState().Transports, X.OutboundId);
                Arrived |= X.DeliveredKg > 0;
                if (!Saved && Out && Out->Status == TEXT("moving"))
                {
                    if (!S.SaveGameJson(Disk, E))
                    {
                        AddError(E);
                        return false;
                    }
                    int32 Speed = 1;
                    if (!Replay.LoadGameJson(Disk, G, S.GetScenario(), E, Speed))
                    {
                        AddError(E);
                        return false;
                    }
                    Saved = true;
                }
            }
    }
    IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir() / TEXT("Diagnostics")), true);
    FFileHelper::SaveStringToFile(Csv, *(FPaths::ProjectSavedDir() / TEXT("Diagnostics/AP09.3_30days.csv")),
                                  FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    AddInfo(FString::Printf(TEXT("30-day retail scenario maximum measured step %.1f us"), MaxUs));
    TestTrue(TEXT("Save during real last-mile movement"), Saved);
    TestTrue(TEXT("Actual replenishment arrives"), Arrived);
    TestTrue(TEXT("Own producer supplies separate shop through real internal transport"),S.GetState().InternalTransfers.ContainsByPredicate([](const FInternalTransfer& X){return X.ActorId==TEXT("company.luxury")&&X.DestinationId==TEXT("retail.store.detroit.luxury")&&X.Kg>0;}));
    TestTrue(TEXT("Real retail payments over time"),
             Find(S.GetState().CashTotals, FString(TEXT("retail_sale"))) != nullptr);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRetailMigration, "CoI.Retail.V4MigrationPreservesObligationsAndReplay",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRetailMigration::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    RetailFixture(J, G, {}, true);
    FSimulation Old;
    if (!Old.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    Old.StepOnce();
    auto C=RetailCommand(Old,ECommandKind::Delivery);auto& R=C.Delivery;R.Id=TEXT("migration.delivery");R.OwnerId=TEXT("company.player");R.Action=TEXT("create");R.Terms.SourceAssetId=TEXT("storage.luxury");R.Terms.DestinationAssetId=TEXT("storage.player");R.Terms.FromCityId=R.Terms.ToCityId=TEXT("city.us.detroit");R.Terms.GoodId=TEXT("good.luxury");R.Terms.VehicleId=TEXT("vehicle.player");R.Terms.Kg=25;R.Terms.StartHour=1;R.Terms.EndHour=73;R.Terms.IntervalHours=24;R.Terms.PriceCentsPerKg=1000;R.Terms.MaxFreightCents=100000;
    if(!Old.PreviewDelivery(R,E)||!Old.Enqueue(C,E)){AddError(E);return false;}Old.StepOnce();Old.StepOnce();
    const auto* Moving=Find(Old.GetState().Transports,FString(TEXT("migration.delivery.at.1.out")));if(!TestNotNull(TEXT("Real contracted shipment exists before migration"),Moving))return false;TestEqual(TEXT("Save while contracted cargo is moving"),Moving->Status,FString(TEXT("moving")));
    FString Disk = RetailV4(Old, E);
    const auto Before = Old.GetState();
    RetailFixture(J, G);
    FSimulation Target;
    if (!Target.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    int32 Speed = 1;
    if (!Target.LoadGameJson(Disk, G, Target.GetScenario(), E, Speed))
    {
        AddError(E);
        return false;
    }
    for (const auto &A : Before.Accounts)
        TestEqual(TEXT("Migration preserves actual money"), Find(Target.GetState().Accounts, A.Id)->BalanceCents,
                  A.BalanceCents);
    TestEqual(TEXT("Standing delivery contract retained"),RetailHash(Target.GetState().Deliveries),RetailHash(Before.Deliveries));TestEqual(TEXT("Delivery slot and accounting flags retained"),RetailHash(Target.GetState().DeliverySlots),RetailHash(Before.DeliverySlots));
    TestEqual(TEXT("Physical stock preserved"), RetailHash(Target.GetState().Inventories),
              RetailHash(Before.Inventories));
    TestEqual(TEXT("Moving transport and reservations retained"), RetailHash(Target.GetState().Transports),
              RetailHash(Before.Transports));
    for (const auto &O : Before.Orders)
        if (!Find(Before.Groups, O.ActorId))
            TestEqual(TEXT("B2B order retained exactly"), RetailHash(*Find(Target.GetState().Orders, O.Id)),
                      RetailHash(O));
    for (const auto &O : Target.GetState().Orders)
        if (Find(Target.GetState().Groups, O.ActorId))
            TestTrue(TEXT("Old household holds released"), O.Status != TEXT("open") && !O.ReservedCents);
    TestTrue(TEXT("Retail active after migration"), Target.GetScenario().Economy.RetailEnabled);
    if (!Target.SaveGameJson(Disk, E))
    {
        AddError(E);
        return false;
    }
    FSimulation Replay;
    if (!Replay.LoadGameJson(Disk, G, Target.GetScenario(), E, Speed))
    {
        AddError(E);
        return false;
    }
    for (int H = 0; H < 72; ++H)
    {
        Target.StepOnce();
        Replay.StepOnce();
        if (Target.Fingerprint() != Replay.Fingerprint())
        {
            AddError(TEXT("Migrated replay differs"));
            return false;
        }
        if (!Target.AuditRuntime(E))
        {
            AddError(E);
            return false;
        }
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRetailBuild, "CoI.Retail.BuildUpgradeAndExplicitPurchase",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRetailBuild::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    RetailFixture(J, G);
    FSimulation S;
    if (!S.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    auto C = RetailCommand(S, ECommandKind::Invest);
    C.Investment.Id = TEXT("test.shop.build");
    C.Investment.OwnerId = TEXT("company.player");
    C.Investment.CityId = TEXT("city.us.detroit");
    C.Investment.Action = TEXT("build");
    C.Investment.DefinitionId = TEXT("retail.build.food");
    C.Investment.PlotId = TEXT("retail.plot.detroit.0");
    S.Enqueue(C, E);
    for (int H = 0; H < 24; ++H)
        S.StepOnce();
    const auto *Shop = Find(S.GetState().Warehouses, FString(TEXT("test.shop.build.asset")));
    if (!TestNotNull(TEXT("Paid shop is constructed through AP08.3"), Shop))
        return false;
    TestEqual(TEXT("Starts as food shop"), Shop->TypeId, FString(TEXT("retail.type.food")));
    for (const auto &I : S.GetState().Inventories)
        if (I.StorageId == Shop->Id)
            TestEqual(TEXT("No free opening stock"), I.QuantityKg, 0LL);
    C = RetailCommand(S, ECommandKind::Invest);
    C.Investment.Id = TEXT("test.shop.upgrade");
    C.Investment.OwnerId = TEXT("company.player");
    C.Investment.CityId = TEXT("city.us.detroit");
    C.Investment.Action = TEXT("upgrade");
    C.Investment.DefinitionId = TEXT("retail.build.food.large");
    C.Investment.TargetId = Shop->Id;
    S.Enqueue(C, E);
    for (int H = 0; H < 24; ++H)
        S.StepOnce();
    TestEqual(TEXT("Paid delayed retail upgrade"),
              Find(S.GetState().Warehouses, FString(TEXT("test.shop.build.asset")))->TypeId,
              FString(TEXT("retail.type.food.large")));
    if (!S.ValidateSaveState(E))
    {
        AddError(E);
        return false;
    }
    FSimulation Buy;
    if (!Buy.Load(J, G, E))
        return false;
    C = RetailCommand(Buy, ECommandKind::Invest);
    C.Investment.Id = TEXT("test.shop.purchase");
    C.Investment.OwnerId = TEXT("company.player");
    C.Investment.CityId = TEXT("city.us.chicago");
    C.Investment.Action = TEXT("buy");
    C.Investment.TargetId = TEXT("retail.sale.chicago");
    Buy.Enqueue(C, E);
    Buy.StepOnce();
    TestEqual(TEXT("Explicit empty shop sale transfers owner"),
              Find(Buy.GetState().Warehouses, FString(TEXT("retail.store.chicago.merchant")))->OwnerId,
              FString(TEXT("company.player")));
    return TestTrue(TEXT("No extra company cash"), Buy.AuditRuntime(E));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRetailLuxury,"CoI.Retail.LuxuryAfterBasicsAndDailyCapacity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRetailLuxury::RunTest(const FString&){FString J,E;FCoIGeoMapData G;for(int Variant=0;Variant<2;++Variant){RetailFixture(J,G,[&](auto R){TwoStockedShops(R);if(Variant){for(auto T:R->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("WarehouseTypes")))T->AsObject()->SetNumberField(TEXT("RetailDailyKg"),20);return;}for(auto N:R->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("Needs")))N->AsObject()->SetNumberField(TEXT("LuxuryKgPerThousandPerDay"),10);for(auto W:R->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Warehouses")))if(W->AsObject()->GetStringField(TEXT("Id"))==TEXT("retail.store.chicago.player"))W->AsObject()->SetStringField(TEXT("TypeId"),TEXT("retail.type.mixed"));auto State=R->GetObjectField(TEXT("InitialState"));auto Stocks=State->GetArrayField(TEXT("Inventories"));for(auto X:Stocks)if(X->AsObject()->GetStringField(TEXT("Id"))==TEXT("stock.luxury.luxury"))X->AsObject()->SetNumberField(TEXT("QuantityKg"),950);Stocks.Add(MakeShared<FJsonValueObject>(RetailStock(TEXT("test.available.luxury"),TEXT("good.luxury"),TEXT("company.player"),TEXT("retail.store.chicago.player"),50)));State->SetArrayField(TEXT("Inventories"),Stocks);});FSimulation S;if(!S.Load(J,G,E)){AddError(E);return false;}S.StepOnce();TestEqual(TEXT("Physical daily throughput bounds sales"),Find(S.GetState().GoodsAudit,FString(TEXT("good.food")))->ConsumedKg,Variant?40LL:100LL);if(!Variant)TestEqual(TEXT("Real luxury bought only after group's basic need"),Find(S.GetState().GoodsAudit,FString(TEXT("good.luxury")))->ConsumedKg,50LL);if(!S.ValidateSaveState(E)){AddError(E);return false;}}return true;}
#endif

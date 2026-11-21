#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GeoCatalog.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Simulation/CityDetails.h"
#include "Simulation/SimulationSubsystem.h"
#include "Simulation/SimArchive.h"
#include "UI/PlayerHUD.h"
using namespace CoISim;
namespace
{
int64 Kg(const TArray<FAmount> &A, const FString &Id)
{
    const auto *X = A.FindByPredicate([&](const FAmount &V) { return V.GoodId == Id; });
    return X ? X->Kg : 0;
}
} // namespace
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCityNeedCarry, "CoI.UI05.PeriodFractionNoNeedAndMissing",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCityNeedCarry::RunTest(const FString &)
{
    FGroup G;
    FGroupDef D;
    D.IncomeBand = TEXT("low");
    FNeedDef N;
    N.BasicGoodId = TEXT("food");
    N.LuxuryGoodId = TEXT("luxury");
    N.BasicKgPerThousandPerDay = 137;
    N.LuxuryKgPerThousandPerDay = 31;
    N.MaxExtraPermille = 250;
    FAccount A;
    A.BalanceCents = 123456789012LL;
    A.ReservedCents = 12;
    FEconomyRules Q;
    Q.HouseholdModel = true;
    G.Id = TEXT("group.test");
    G.Population = 1;
    G.BudgetCents = 100;
    G.DailySpentCents = 75;
    TestFalse(TEXT("No demand is not zero supply"), HouseholdDisplay(G, D, N, A, Q).Basic.Known);
    G.NextBudgetHour = 24;
    G.UnmetNeed = {{TEXT("food"), 0}, {TEXT("luxury"), 0}};
    for (int64 Population : {0LL, 1LL, 7LL, 333LL, 1001LL})
        for (int64 Prior : {0LL, 137LL, 999LL})
        {
            G.Population = Population;
            G.WealthPermille = 250;
            const int64 Increment = Population * N.BasicKgPerThousandPerDay, Want = (Increment + Prior) / 1000;
            G.BasicRemainder = (Increment + Prior) % 1000;
            G.UnmetNeed[0].Kg = Want / 2;
            const int64 Lux = Population * N.LuxuryKgPerThousandPerDay * 1250 / 1000, LuxWant = (Lux + Prior) / 1000;
            G.LuxuryRemainder = (Lux + Prior) % 1000;
            G.UnmetNeed[1].Kg = LuxWant;
            auto V = HouseholdDisplay(G, D, N, A, Q);
            TestEqual(TEXT("Exact inverse of existing fractional kg carry"), V.Basic.WantedKg, Want);
            TestEqual(TEXT("Actual consumed amount"), V.Basic.UsedKg, Want - Want / 2);
            TestEqual(TEXT("Luxury includes real wealth adjustment and carry"), V.Luxury.WantedKg, LuxWant);
            TestEqual(TEXT("Exact large savings preserved"), V.SavingsCents, A.BalanceCents);
            TestEqual(TEXT("Remaining budget is not total savings"), V.RemainingBudgetCents, 25LL);
            TestEqual(TEXT("Period end retained at day boundary"), V.EndHour, 24LL);
        }
    G.Population = 0;
    G.BasicRemainder = 0;
    G.UnmetNeed[0].Kg = 0;
    TestEqual(TEXT("No need explicitly labelled"), HouseholdSupply(HouseholdDisplay(G, D, N, A, Q).Basic),
              FString(TEXT("Kein Bedarf")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCityActualData, "CoI.UI05.ActualConsumptionProjectionAndPrivatePresence",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCityActualData::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData Geo;
    auto *Catalog = LoadObject<UCoIGeoCatalog>(nullptr, TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));
    if (!Catalog || !Geo.Load(Catalog->DefinitionJson, E) ||
        !FFileHelper::LoadFileToString(J, *(FPaths::ProjectContentDir() / TEXT("Simulation/scenario.coisim"))))
        return false;
    FScenario Scenario;
    if (!LoadScenario(J, Geo, Scenario, E))
        return false;
    FInventory Shelf;
    Shelf.Id = TEXT("stock.ui05.shelf");
    Shelf.GoodId = TEXT("good.food");
    Shelf.OwnerId = TEXT("company.merchant");
    Shelf.StorageId = TEXT("retail.store.chicago.merchant");
    Shelf.QuantityKg = 100;
    Scenario.InitialState.Inventories.Add(Shelf);
    Scenario.InitialState.Inventories.Sort([](const FInventory &A, const FInventory &B) { return A.Id < B.Id; });
    for (auto &N : Scenario.Definitions.Needs)
        N.BasicMaxPriceCents = 100;
    FWrite W;
    Scenario.Fields(W);
    J.Empty();
    FJsonSerializer::Serialize(W.Object, TJsonWriterFactory<>::Create(&J));
    FSimulation S;
    if (!S.Load(J, Geo, E))
        return false;
    TMap<FString, int64> BaseBasic, BaseLuxury, ExpectedBasic, ExpectedLuxury;
    bool Consumed = false;
    for (int32 H = 0; H < 49; ++H)
    {
        const auto Before = S.GetState().Groups;
        S.StepOnce();
        for (const auto &City : S.GetSnapshot().Player.Cities)
            for (const auto &V : City.Households)
            {
                const auto *Live = Find(S.GetState().Groups, V.Id);
                const auto *Old = Find(Before, V.Id);
                const auto *Def = Find(Scenario.Definitions.Groups, V.Id);
                const auto *N = Find(Scenario.Definitions.Needs, Def->NeedProfileId);
                if (Live->NextBudgetHour != Old->NextBudgetHour)
                {
                    BaseBasic.Add(V.Id, Kg(Old->Consumed, N->BasicGoodId));
                    BaseLuxury.Add(V.Id, Kg(Old->Consumed, N->LuxuryGoodId));
                    ExpectedBasic.Add(V.Id,
                                      (Live->Population * N->BasicKgPerThousandPerDay + Old->BasicRemainder) / 1000);
                    ExpectedLuxury.Add(
                        V.Id, (Live->Population * N->LuxuryKgPerThousandPerDay * (1000 + Live->WealthPermille) / 1000 +
                               Old->LuxuryRemainder) /
                                  1000);
                }
                TestEqual(TEXT("Projected current demand matches independent pre-demand carry"), V.Basic.WantedKg,
                          ExpectedBasic[V.Id]);
                TestEqual(TEXT("Current consumption matches difference of real cumulative consumption"), V.Basic.UsedKg,
                          Kg(Live->Consumed, N->BasicGoodId) - BaseBasic[V.Id]);
                TestEqual(TEXT("Luxury demand period exact"), V.Luxury.WantedKg, ExpectedLuxury[V.Id]);
                TestEqual(TEXT("Luxury actual consumption exact"), V.Luxury.UsedKg,
                          Kg(Live->Consumed, N->LuxuryGoodId) - BaseLuxury[V.Id]);
                TestEqual(TEXT("Savings from existing group account"), V.SavingsCents,
                          Find(S.GetState().Accounts, Live->AccountId)->BalanceCents);
                Consumed |= V.Basic.UsedKg > 0;
            }
    }
    TestTrue(TEXT("Control uses actual retail consumption, not just zero values"), Consumed);
    TestTrue(*E, S.AuditRuntime(E));
    const auto Fingerprint = S.Fingerprint();
    for (const auto &City : S.GetSnapshot().Player.Cities)
        for (const auto &X : City.Presence)
        {
            if (!X.Own)
            {
                int64 Public = 0;
                for (const auto &V : S.GetSnapshot().Player.Vehicles)
                    if (V.ProviderId == X.CompanyId && V.CityId == City.Id)
                        ++Public;
                TestEqual(TEXT("Foreign private trucks not exposed by presence"), X.PublicVehicles, Public);
            }
            TestTrue(TEXT("Public shop count bounded by real storage sites"), X.Shops <= X.Warehouses);
        }
    TestEqual(TEXT("Reading projections changes no money, goods or state"), S.Fingerprint(), Fingerprint);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCityDetailWidgets, "CoI.UI05.LiveFoldSortSelectionAndSnapshotUpdates",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCityDetailWidgets::RunTest(const FString &)
{
    auto *Catalog =
        LoadObject<UCoIGeoCatalog>(nullptr, TEXT("/Game/Geography/Data/GeoCatalog_DE_NL_1913.GeoCatalog_DE_NL_1913"));
    if (!Catalog)
        return false;
    const auto IV = UWorld::InitializationValues()
                        .AllowAudioPlayback(false)
                        .CreatePhysicsScene(false)
                        .CreateNavigation(false)
                        .CreateAISystem(false)
                        .ShouldSimulatePhysics(false);
    auto *World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &IV);
    if (!World)
        return false;
    ON_SCOPE_EXIT
    {
        World->DestroyWorld(false);
    };
    World->SetGameInstance(NewObject<UGameInstance>(World));
    World->SpawnActor<ACoIGeoReference>()->Catalog = Catalog;
    auto *Sim = World->GetSubsystem<UCoISimulationSubsystem>();
    if (!Sim->Restart())
        return false;
    auto *HUD = NewObject<UCoIPlayerHUD>(World);
    HUD->Initialize();
    auto Slate = HUD->TakeWidget();
    ON_SCOPE_EXIT
    {
        HUD->ReleaseSlateResources(true);
    };
    auto HasText = [&](const FString &Value) {
        TArray<UWidget *> All;
        HUD->WidgetTree->GetAllWidgets(All);
        for (const auto *W : All)
            if (const auto *T = Cast<UTextBlock>(W); T && T->GetText().ToString() == Value)
                return true;
        return false;
    };
    const auto Hash = Sim->Fingerprint();
    HUD->Action(TEXT("city"));
    const auto City = HUD->CityContext;
    const auto *C = Find(Sim->GetSnapshot().Player.Cities, City);
    if (!C || C->Households.IsEmpty() || C->Presence.IsEmpty())
        return false;
    const FString GroupId = C->Households[0].Id, CompanyId = C->Presence[0].CompanyId;
    TestFalse(TEXT("Household details initially closed"), HUD->CityDetails[City].Population);
    TestFalse(TEXT("Company table initially closed"), HUD->CityDetails[City].Companies);
    HUD->Action(TEXT("cityFold|companies"));
    TestTrue(TEXT("Public company table has activities"), HasText(TEXT("Firma · Standortrolle · Tätigkeiten · Präsenz")));
    TestFalse(TEXT("Cryptic combined column removed"), HasText(TEXT("Betriebe / öffentliche Lkw")));
    HUD->Action(TEXT("cityCompany|") + CompanyId);
    HUD->WorkScroll->SetScrollOffset(48);
    HUD->Action(TEXT("citySort|name"));
    TestEqual(TEXT("Sorting retains company selection"), HUD->CityDetails[City].CompanyId, CompanyId);
    TestEqual(TEXT("Sorting preserves scroll request"), HUD->WorkScroll->GetScrollOffset(), 48.f);
    HUD->Action(TEXT("cityFold|population"));
    HUD->Action(TEXT("cityGroup|") + GroupId);
    TestTrue(TEXT("Group details expose savings without broad table"),
             HasText(TEXT("Ersparnisse / davon unreserviert")));
    TestTrue(TEXT("Unmodelled leisure never presented as zero"), HasText(TEXT("Nicht separat modelliert")));
    TestFalse(TEXT("No company account IDs in public details"), HasText(TEXT("account.company.food")));
    HUD->Action(TEXT("cityGroup|group.not.present"));
    TestEqual(TEXT("Unlisted group cannot replace selection"), HUD->CityDetails[City].GroupId, GroupId);
    for (FVector2D Size : {FVector2D(1280, 720), FVector2D(1920, 1080)})
    {
        HUD->NativeTick(FGeometry::MakeRoot(Size, FSlateLayoutTransform()), 0);
        TestEqual(TEXT("Responsive detail state follows viewport"), HUD->CompactCityDetails, true);
        TestEqual(TEXT("Responsive rebuilding retains group"), HUD->CityDetails[City].GroupId, GroupId);
    }
    // Presentation-only stress cases: never alter simulation companies or accounts.
    const auto OriginalCity = *C;
    auto &VisibleCity = const_cast<FCityView &>(*C);
    FCityPresence LongCompany;
    LongCompany.CompanyId = TEXT("ui05.public.long");
    LongCompany.Name = TEXT("Internationales regionales Handelshaus mit sehr langem öffentlichen Firmennamen und "
                            "mehreren Tätigkeitsbereichen für die Tabellenprüfung");
    LongCompany.Warehouses = 3;
    LongCompany.Activities = {TEXT("Einzelhandel"), TEXT("Lagerhaltung")};
    VisibleCity.Presence.Add(LongCompany);
    for (int32 I = 0; I < 50; ++I)
    {
        auto Extra = LongCompany;
        Extra.CompanyId += LexToString(I);
        Extra.Name = TEXT("Öffentliche Testpräsenz ") + LexToString(I);
        VisibleCity.Presence.Add(Extra);
    }
    HUD->RefreshTable();
    TestTrue(TEXT("Long public names remain available"), HasText(LongCompany.Name));
    VisibleCity.Presence.Reset();
    VisibleCity.Households.Reset();
    HUD->RefreshTable();
    TestTrue(TEXT("Empty company list explains state"),
             HasText(TEXT("Keine öffentlich sichtbare Firmenpräsenz an diesem Hub.")));
    TestTrue(TEXT("Empty household list explains state"),
             HasText(TEXT("Diesem Hub sind keine Haushaltsgruppen zugeordnet.")));
    TestTrue(TEXT("Missing group drops stale detail selection"), HUD->CityDetails[City].GroupId.IsEmpty());
    VisibleCity = OriginalCity;
    HUD->RefreshTable();
    HUD->Action(TEXT("cityCompany|") + CompanyId);
    HUD->Action(TEXT("cityGroup|") + GroupId);
    TestEqual(TEXT("UI actions are read only"), Sim->Fingerprint(), Hash);
    Sim->TogglePause();
    Sim->Tick(1.1f);
    Sim->TogglePause();
    TestEqual(TEXT("Snapshot refresh retains company"), HUD->CityDetails[City].CompanyId, CompanyId);
    TestEqual(TEXT("Snapshot refresh retains group"), HUD->CityDetails[City].GroupId, GroupId);
    HUD->Action(TEXT("cityTab|Markt"));
    TestEqual(TEXT("Existing market action remains linked"), HUD->CurrentView(), FString(TEXT("Märkte")));
    HUD->Action(TEXT("cityTab|Übersicht"));
    TestTrue(TEXT("Returning to overview keeps expanded sections"), HUD->CityDetails[City].Population);
    Sim->Restart();
    HUD->Action(TEXT("city"));
    TestTrue(TEXT("New party clears old group selection"), HUD->CityDetails[HUD->CityContext].GroupId.IsEmpty());
    return true;
}
#endif

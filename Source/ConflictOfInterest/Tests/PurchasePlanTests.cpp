#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Simulation/SimulationSubsystem.h"
#include "UI/PlayerHUD.h"
#include "GeoCatalog.h"
#include "Simulation/SimArchive.h"
#include "Simulation/SaveGame.h"
#include "Simulation/PurchasePlan.h"
#include "Simulation/TradeBusiness.h"
#include "Serialization/JsonSerializer.h"
using namespace CoISim;
namespace
{
struct FPurchaseFixture
{
    FCoIGeoMapData Geo;
    FScenario Scenario;
    FString Source, Target, Good, Seller, Vehicle;
    bool Load(FSimulation &S, FString &E, bool Water = false, bool Rented = false)
    {
        FString Raw;
        auto *Catalog = LoadObject<UCoIGeoCatalog>(
            nullptr, Water ? TEXT("/Game/Geography/Data/GeoCatalog_DE_NL_1913.GeoCatalog_DE_NL_1913")
                           : TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));
        if (!Catalog || !Geo.Load(Catalog->DefinitionJson, E) ||
            !FFileHelper::LoadFileToString(
                Raw, *(FPaths::ProjectContentDir() /
                       (Water ? TEXT("Simulation/scenario.de_nl1913.coisim") : TEXT("Simulation/scenario.coisim")))) ||
            !LoadScenario(Raw, Geo, Scenario, E))
            return false;
        if (Water)
        {
            Source = TEXT("storage.offer.rotterdam");
            Target = TEXT("storage.service.city.de.1913.rheinprovinz");
            Seller = TEXT("company.merchant");
            Good = TEXT("good.food");
            Vehicle = TEXT("vehicle.carrier.rhine");
            for (auto &Company : Scenario.Definitions.Companies)
                if (!Company.AccountingOnly && Company.Id != Scenario.PlayerCompanyId)
                    Company.DecisionProfile = TEXT("carrier");
            FInventory I;
            I.Id = TEXT("fixture.water.sale");
            I.OwnerId = Seller;
            I.StorageId = Source;
            I.GoodId = Good;
            I.QuantityKg = 50;
            Scenario.InitialState.Inventories.Add(I);
            Scenario.InitialState.Inventories.Sort([](const auto &A, const auto &B) { return A.Id < B.Id; });
        }
        else
        {
            Scenario.Economy.Stage = 4;
            Scenario.Economy.Public = {};
            if (Rented)
                Scenario.InitialState.Offers.Reset();
            Scenario.Economy.AccountingEnabled = Scenario.Economy.Bank.Enabled = Scenario.Economy.RetailEnabled =
                Scenario.Economy.HouseholdModel = Scenario.Economy.LegacyHouseholdPurchases = false;
            for (auto &G : Scenario.Definitions.Goods)
                G.SpoilPartsPerMillionPerDay = 0;

            for (const auto &I : Scenario.InitialState.Inventories)
                if (I.OwnerId != Scenario.PlayerCompanyId && Find(Scenario.Definitions.Companies, I.OwnerId) &&
                    I.QuantityKg >= 50)
                {
                    Source = I.StorageId;
                    Good = I.GoodId;
                    Seller = I.OwnerId;
                    break;
                }
            for (const auto &W : Scenario.InitialState.Warehouses)
                if (W.OwnerId == Scenario.PlayerCompanyId && W.Id != Source)
                {
                    Target = W.Id;
                    break;
                }
            for (auto &V : Scenario.InitialState.Vehicles)
                if (V.OwnerId == (Rented ? TEXT("company.carrier") : Scenario.PlayerCompanyId))
                {
                    V.CityId = Find(Scenario.InitialState.Warehouses, Source)->CityId;
                    Vehicle = V.Id;
                    break;
                }
            for (auto &C : Scenario.Definitions.Companies)
                if (!C.AccountingOnly && C.Id != Scenario.PlayerCompanyId)
                    C.DecisionProfile = TEXT("carrier");
        }
        FWrite W;
        Scenario.Fields(W);
        FJsonSerializer::Serialize(W.Object, TJsonWriterFactory<>::Create(&Raw));
        return S.Load(Raw, Geo, E);
    }
};
FCommand PurchaseCmd(FSimulation &S, ECommandKind K)
{
    FCommand C;
    C.Kind = K;
    C.Sequence = S.NextSequence();
    C.TargetStep = S.GetState().Step + 1;
    C.Generation = S.GetGeneration();
    return C;
}
FCommand PurchaseOrder(FSimulation &S, const FPurchaseFixture &F, const FString &Id, const FString &Actor, int64 Kg,
                       bool Buy)
{
    auto C = PurchaseCmd(S, ECommandKind::PlaceOrder);
    auto &O = C.Order;
    O.Id = Id;
    O.ActorId = Actor;
    O.Side = Buy ? TEXT("buy") : TEXT("sell");
    O.StorageId = F.Source;
    O.GoodId = F.Good;
    O.OriginalKg = Kg;
    O.LimitCentsPerKg = 1;
    O.CityId = Find(S.GetState().Warehouses, F.Source)->CityId;
    O.MarketId = Find(S.GetScenario().Definitions.Regions, Find(S.GetScenario().Definitions.Places, O.CityId)->RegionId)
                     ->MarketId;
    return C;
}
bool PurchaseSell(FSimulation &S, const FPurchaseFixture &F, FString &E, int64 Kg = 40)
{
    auto C = PurchaseOrder(S, F, TEXT("purchase.ask"), F.Seller, Kg, false);
    if (!S.Enqueue(C, E) || !S.StepOnce())
        return false;
    return Find(S.GetState().Orders, C.Order.Id) != nullptr;
}
bool PurchaseDraft(FSimulation &S, const FPurchaseFixture &F, FCommand &C, FPurchaseQuote &Q, FString &E, int64 Kg = 20)
{
    C = PurchaseOrder(S, F, TEXT("purchase.buy"), S.GetSession().PlayerCompanyId, Kg, true);
    C.Origin = ECommandOrigin::Player;
    C.CompanyId = C.Order.ActorId;
    C.TrackTrade = true;
    C.Order.LimitCentsPerKg = Find(S.GetState().Orders, FString(TEXT("purchase.ask")))->LimitCentsPerKg;
    auto &I = C.Purchase;
    I.Enabled = true;
    I.OfferId = TEXT("purchase.ask");
    I.VehicleId = F.Vehicle;
    I.DestinationStorageId = F.Target;
    I.ApprovedHour = S.GetState().EconomyHour;
    I.DepartureHour = I.ApprovedHour + S.GetScenario().Settings.HoursPerStep + S.GetScenario().Economy.BookingLeadHours;
    if (!S.PreviewPurchase(C.Order, I, Q, E))
        return false;
    I.MaxFreightCents = Q.Transport.Cost;
    I.LatestArrivalHour = Q.Transport.Arrival;
    return true;
}
} // namespace
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPurchaseFlow, "CoI.UI09.PurchaseThenDeliverySaveAndNoDuplicates",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPurchaseFlow::RunTest(const FString &)
{
    FSimulation S;
    FPurchaseFixture F;
    FString E;
    if (!F.Load(S, E) || !PurchaseSell(S, F, E))
    {
        AddError(E);
        return false;
    }
    FCommand C;
    FPurchaseQuote Q;
    const auto Before = S.EconomicFingerprint();
    if (!TestTrue(*E, PurchaseDraft(S, F, C, Q, E)))
        return false;
    TestEqual(TEXT("Preview reserves nothing"), S.EconomicFingerprint(), Before);
    TestTrue(TEXT("Buy queued"), S.Enqueue(C, E));
    TestFalse(TEXT("Identical command ID/sequence not sent twice"), S.Enqueue(C, E));
    FString PendingSave;
    if (!TestTrue(TEXT("Save queued combined command"), S.SaveGameJson(PendingSave, E)))
        return false;
    FSimulation PendingLoad;
    int32 PendingSpeed = 1;
    if (!TestTrue(TEXT("Load queued combined command"),
                  PendingLoad.LoadGameJson(PendingSave, F.Geo, S.GetScenario(), E, PendingSpeed)))
        return false;
    S.StepOnce();
    PendingLoad.StepOnce();
    TestEqual(TEXT("Pending purchase survives save without changed payments"), S.EconomicFingerprint(),
              PendingLoad.EconomicFingerprint());
    const FString BId = TEXT("business.purchase.buy");
    auto *B = Find(S.GetState().Businesses, BId);
    if (!TestNotNull(TEXT("One combined business"), B))
        return false;
    TestEqual(TEXT("Actual purchase first"), B->FilledKg, 20LL);
    TestEqual(TEXT("Distinct next-step phase"), B->Purchase.Phase, FString(TEXT("ready")));
    TestEqual(TEXT("No transport before acquisition commit"), B->Legs.Num(), 0);
    FString Save;
    if (!TestTrue(*E, S.SaveGameJson(Save, E)))
        return false;
    FSimulation R;
    int32 Speed = 1;
    if (!TestTrue(*E, R.LoadGameJson(Save, F.Geo, S.GetScenario(), E, Speed)))
        return false;
    auto Duplicate = C;
    Duplicate.Sequence = S.NextSequence();
    Duplicate.TargetStep = S.GetState().Step + 1;
    TestTrue(TEXT("Repeat command with same order ID reaches domain validation"), S.Enqueue(Duplicate, E));
    S.StepOnce();
    R.StepOnce();
    B = Find(S.GetState().Businesses, BId);
    TestEqual(TEXT("Exactly one delivery link"), B->Legs.Num(), 1);
    if (!B->Legs.Num())
    {
        AddError(B->LastReason);
        return false;
    }
    const auto J = *Find(S.GetState().Transports, B->Legs[0].Id);
    TestEqual(TEXT("Only acquired quantity transported"), J.Kg, 20LL);
    TestTrue(TEXT("Authorized price ceiling"), J.CostCents <= C.Purchase.MaxFreightCents);
    while (S.GetState().EconomyHour < J.ArrivalStep + 2)
    {
        S.StepOnce();
        R.StepOnce();
    }
    TestEqual(TEXT("Deterministic economy through load/arrival"), S.EconomicFingerprint(), R.EconomicFingerprint());
    B = Find(S.GetState().Businesses, BId);
    TestEqual(TEXT("No second purchase payment"), B->SettledCents, 20LL);
    TestEqual(TEXT("One arrival"), B->Legs[0].Status, FString(TEXT("arrived")));
    TestEqual(TEXT("Saved phase retained"), B->Purchase.Phase, Find(R.GetState().Businesses, BId)->Purchase.Phase);
    TestTrue(TEXT("All money/goods/reservations balanced"), S.AuditRuntime(E));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPurchaseRetry, "CoI.UI09.TransportFailureKeepsGoodsAndRetryDoesNotBuy",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPurchaseRetry::RunTest(const FString &)
{
    FSimulation S;
    FPurchaseFixture F;
    FString E;
    if (!F.Load(S, E) || !PurchaseSell(S, F, E))
        return false;
    FCommand C;
    FPurchaseQuote Q;
    if (!PurchaseDraft(S, F, C, Q, E) || !S.Enqueue(C, E))
        return false;
    S.StepOnce();
    auto Block = PurchaseCmd(S, ECommandKind::Reserve);
    Block.Resources.Id = TEXT("test.busy");
    Block.Resources.ActorId = S.GetSession().PlayerCompanyId;
    Block.Resources.VehicleId = F.Vehicle;
    TestTrue(TEXT("Bind vehicle after purchase"), S.Enqueue(Block, E));
    S.StepOnce();
    const FString Id = TEXT("business.purchase.buy");
    auto *B = Find(S.GetState().Businesses, Id);
    TestEqual(TEXT("Pickup required, not rolled back purchase"), B->Purchase.Phase, FString(TEXT("pickup_required")));
    TestEqual(TEXT("No half-reserved freight"), B->Legs.Num(), 0);
    TestTrue(TEXT("Useful failure cause"), !B->LastReason.IsEmpty());
    const auto *I = S.GetState().Inventories.FindByPredicate(
        [&](const FInventory &X) { return X.OwnerId == B->OwnerId && X.StorageId == F.Source && X.GoodId == F.Good; });
    if (!TestNotNull(TEXT("Goods remain owned at source"), I))
        return false;
    TestEqual(TEXT("No partial goods hold"), I->ReservedKg, 0LL);
    const FString StockId = I->Id;
    FString Save;
    TestTrue(TEXT("Save pickup state"), S.SaveGameJson(Save, E));
    FSimulation R;
    int32 Speed = 1;
    TestTrue(TEXT("Load pickup state"), R.LoadGameJson(Save, F.Geo, S.GetScenario(), E, Speed));
    TestEqual(TEXT("Pickup survives load"), Find(R.GetState().Businesses, Id)->Purchase.Phase, B->Purchase.Phase);
    auto Release = PurchaseCmd(S, ECommandKind::Release);
    Release.Resources = Block.Resources;
    TestTrue(TEXT("Explicitly release busy vehicle"), S.Enqueue(Release, E));
    S.StepOnce();
    auto Haul = PurchaseCmd(S, ECommandKind::BookTransport);
    Haul.Origin = ECommandOrigin::Player;
    Haul.CompanyId = S.GetSession().PlayerCompanyId;
    Haul.TradeCaseId = Id;
    auto &J = Haul.Transport;
    J.Id = TEXT("purchase.retry");
    J.OwnerId = Haul.CompanyId;
    J.VehicleId = F.Vehicle;
    J.SourceInventoryId = StockId;
    J.DestinationStorageId = F.Target;
    J.FromCityId = Find(S.GetState().Warehouses, F.Source)->CityId;
    J.ToCityId = Find(S.GetState().Warehouses, F.Target)->CityId;
    J.Kg = 20;
    TestTrue(TEXT("Transport-only retry queued"), S.Enqueue(Haul, E));
    S.StepOnce();
    B = Find(S.GetState().Businesses, Id);
    if (!TestEqual(TEXT("Retry linked once"), B->Legs.Num(), 1))
    {
        AddError(B->LastReason);
        return false;
    }
    TestEqual(TEXT("Retry never buys again"), B->FilledKg, 20LL);
    TestEqual(TEXT("One payment for goods"), B->SettledCents, 20LL);
    TestTrue(TEXT("Retry balanced"), S.AuditRuntime(E));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPurchasePartial, "CoI.UI09.PartialFillAndChangedQuoteControls",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPurchasePartial::RunTest(const FString &)
{
    FSimulation S;
    FPurchaseFixture F;
    FString E;
    if (!F.Load(S, E) || !PurchaseSell(S, F, E, 20))
        return false;
    FCommand C;
    FPurchaseQuote Q;
    if (!PurchaseDraft(S, F, C, Q, E))
        return false;
    const auto P = S.GetSnapshot().Player;
    auto Changed = P;
    Find(Changed.Vehicles, F.Vehicle)->Bound = true;
    TestFalse(TEXT("Busy own/provider vehicle rejects preview"),
              QuotePurchase(Changed, MAX_int64, C.Order, C.Purchase, Q, E));
    Changed = P;
    Find(Changed.Stores, F.Target)->Capacity =
        Find(Changed.Stores, F.Target)->Used + Find(Changed.Stores, F.Target)->Incoming;
    TestFalse(TEXT("Full destination rejects"), QuotePurchase(Changed, MAX_int64, C.Order, C.Purchase, Q, E));
    Changed = P;
    Find(Changed.Orders, C.Purchase.OfferId)->RemainingKg = 0;
    TestFalse(TEXT("No goods means no plan"), QuotePurchase(Changed, MAX_int64, C.Order, C.Purchase, Q, E));
    Changed = P;
    Find(Changed.Orders, C.Purchase.OfferId)->LimitCentsPerKg++;
    TestFalse(TEXT("Price above authorization needs new quote"),
              QuotePurchase(Changed, MAX_int64, C.Order, C.Purchase, Q, E));
    TestFalse(TEXT("Insufficient combined funds"), QuotePurchase(P, 0, C.Order, C.Purchase, Q, E));
    auto Rival = PurchaseOrder(S, F, TEXT("purchase.rival"), TEXT("company.merchant"), 5, true);
    if (Rival.Order.ActorId == F.Seller)
        Rival.Order.ActorId = TEXT("company.carrier");
    TestTrue(TEXT("Earlier same-book buyer queued"), S.Enqueue(Rival, E));
    C.Sequence = S.NextSequence();
    TestTrue(TEXT("Combined buyer queued"), S.Enqueue(C, E));
    S.StepOnce();
    auto *B = Find(S.GetState().Businesses, FString(TEXT("business.purchase.buy")));
    TestEqual(TEXT("Actual partial fill"), B->FilledKg, 15LL);
    S.StepOnce();
    B = Find(S.GetState().Businesses, FString(TEXT("business.purchase.buy")));
    TestEqual(TEXT("One partial delivery"), B->Legs.Num(), 1);
    if (B->Legs.Num())
        TestEqual(TEXT("Transport never includes unfunded remainder"), B->Legs[0].Kg, 15LL);
    TestTrue(TEXT("Partial flow balanced"), S.AuditRuntime(E));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPurchaseWater, "CoI.UI09.CombinedWaterOfferUsesFiniteFleet",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPurchaseWater::RunTest(const FString &)
{
    FSimulation S;
    FPurchaseFixture F;
    FString E;
    if (!F.Load(S, E, true) || !PurchaseSell(S, F, E))
    {
        AddError(E);
        return false;
    }
    FCommand C;
    FPurchaseQuote Q;
    if (!TestTrue(*E, PurchaseDraft(S, F, C, Q, E)))
        return false;
    TestEqual(TEXT("Water handling included"), Q.HandlingMinutes, 240LL);
    TestTrue(TEXT("Submit water purchase"), S.Enqueue(C, E));
    S.StepOnce();
    S.StepOnce();
    const auto *B = Find(S.GetState().Businesses, FString(TEXT("business.purchase.buy")));
    if (!TestEqual(TEXT("Water transport booked"), B->Legs.Num(), 1))
    {
        AddError(B->LastReason);
        return false;
    }
    const auto *J = Find(S.GetState().Transports, B->Legs[0].Id);
    TestEqual(TEXT("Water service reused"), J->Service, FString(TEXT("rented_ship")));
    TestEqual(TEXT("Finite harbor feeders"), J->FeederVehicleIds.Num(), 2);
    TestEqual(TEXT("Correct actual purchase kg"), J->Kg, 20LL);
    TestTrue(TEXT("Water plan balanced"), S.AuditRuntime(E));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPurchaseRejections, "CoI.UI09.LiveStockTargetAndCarrierRejections",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPurchaseRejections::RunTest(const FString &)
{
    for (int32 Case = 0; Case < 3; ++Case)
    {
        FSimulation S;
        FPurchaseFixture F;
        FString E;
        if (!F.Load(S, E, false, Case == 1) || !PurchaseSell(S, F, E))
        {
            AddError(E);
            return false;
        }
        FCommand C;
        FPurchaseQuote Q;
        if (!PurchaseDraft(S, F, C, Q, E))
        {
            AddError(E);
            return false;
        }
        if (Case == 2)
        {
            auto Cancel = PurchaseCmd(S, ECommandKind::CancelOrder);
            Cancel.Order.Id = TEXT("purchase.ask");
            Cancel.Order.ActorId = F.Seller;
            TestTrue(TEXT("Seller withdraws before buy execution"), S.Enqueue(Cancel, E));
            C.Sequence = S.NextSequence();
            TestTrue(TEXT("Queued old draft"), S.Enqueue(C, E));
            S.StepOnce();
            S.StepOnce();
            TestNull(TEXT("Rejected purchase creates no business"),
                     Find(S.GetState().Businesses, FString(TEXT("business.purchase.buy"))));
            TestTrue(TEXT("Rejected purchase creates no freight"), S.GetState().Transports.IsEmpty());
        }
        else
        {
            TestTrue(TEXT("Buy accepted"), S.Enqueue(C, E));
            S.StepOnce();
            auto Block = PurchaseCmd(S, ECommandKind::Reserve);
            Block.Resources.Id = TEXT("test.changed");
            if (Case == 0)
            {
                const auto *W = Find(S.GetSnapshot().Player.Stores, F.Target);
                Block.Resources.ActorId = S.GetSession().PlayerCompanyId;
                Block.Resources.WarehouseId = F.Target;
                Block.Resources.IncomingPoints = W->Capacity - W->Used - W->Incoming;
            }
            else
            {
                Block.Resources.ActorId = TEXT("company.carrier");
                Block.Resources.VehicleId = F.Vehicle;
            }
            TestTrue(TEXT("Actual competing reservation queued"), S.Enqueue(Block, E));
            S.StepOnce();
            const auto *B = Find(S.GetState().Businesses, FString(TEXT("business.purchase.buy")));
            TestEqual(TEXT("Purchased goods survive changed availability"), B->FilledKg, 20LL);
            TestEqual(TEXT("Pickup required"), B->Purchase.Phase, FString(TEXT("pickup_required")));
            TestTrue(TEXT("No partial freight booking"), B->Legs.IsEmpty() && S.GetState().Transports.IsEmpty());
            TestTrue(TEXT("Failure explains cause"), !B->LastReason.IsEmpty());
        }
        TestTrue(TEXT("No money or goods imbalance"), S.AuditRuntime(E));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPurchaseWidgets, "CoI.UI09.GuidedFormNoFakeProfitStaleAndDuplicateConfirm",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPurchaseWidgets::RunTest(const FString &)
{
    auto *Catalog =
        LoadObject<UCoIGeoCatalog>(nullptr, TEXT("/Game/Geography/Data/GeoCatalog_DE_NL_1913.GeoCatalog_DE_NL_1913"));
    const auto IV = UWorld::InitializationValues()
                        .AllowAudioPlayback(false)
                        .CreatePhysicsScene(false)
                        .CreateNavigation(false)
                        .CreateAISystem(false)
                        .ShouldSimulatePhysics(false);
    auto *World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &IV);
    if (!World || !Catalog)
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
    Sim->TogglePause();
    Sim->Tick(1.1f);
    Sim->TogglePause();
    HUD->Action(TEXT("view|Märkte"));
    FString OfferId;
    for (int32 Attempt = 0; Attempt < 25 && OfferId.IsEmpty(); ++Attempt)
    {
        for (const auto &O : Sim->GetSnapshot().Player.Orders)
            if (O.Side == TEXT("sell") && O.Status == TEXT("open") && O.ActorId != Sim->GetSnapshot().Player.ActorId &&
                Sim->GetSnapshot().Player.Vehicles.ContainsByPredicate([&](const auto &V) {
                    return V.CityId == O.CityId && !V.Bound && !V.Blocked && V.Mode == TEXT("truck") &&
                           !Find(Sim->GetSnapshot().Player.Catalog.VehicleTypes, V.TypeId)->TerminalOnly;
                }))
            {
                OfferId = O.Id;
                break;
            }
        if (OfferId.IsEmpty())
        {
            Sim->TogglePause();
            Sim->Tick(1.1f);
            Sim->TogglePause();
        }
    }
    if (!TestFalse(TEXT("Physical offer with available provider"), OfferId.IsEmpty()))
        return false;
    auto Hash = Sim->Fingerprint();
    HUD->Action(TEXT("purchasePlan|") + OfferId);
    TestEqual(TEXT("Guided dialog"), HUD->DialogKind, FString(TEXT("purchase")));
    HUD->Fields[TEXT("quantity")]->SetText(FText::FromString(TEXT("1")));
    HUD->Preview();
    AddInfo(FString::Printf(TEXT("UI draft target=%s vehicle=%s price=%lld kg=%lld"),
                            *HUD->Draft.Purchase.DestinationStorageId, *HUD->Draft.Purchase.VehicleId,
                            HUD->Draft.Order.LimitCentsPerKg, HUD->Draft.Order.OriginalKg));
    if (!TestTrue(*HUD->FormPreview->GetText().ToString(), HUD->ValidDraft))
        return false;
    TestTrue(TEXT("All relevant costs visible"),
             HUD->FormPreview->GetText().ToString().Contains(TEXT("Gesamtausgabe")));
    TestFalse(TEXT("No imagined profit without resale plan"),
              HUD->FormPreview->GetText().ToString().Contains(TEXT("geschätztes Ergebnis")));
    TestEqual(TEXT("Opening/editing draft is read only"), Sim->Fingerprint(), Hash);
    HUD->Escape();
    TestEqual(TEXT("Closing unsubmitted plan reserves nothing"), Sim->Fingerprint(), Hash);
    HUD->Action(TEXT("purchasePlan|") + OfferId);
    HUD->Fields[TEXT("quantity")]->SetText(FText::FromString(TEXT("1")));
    HUD->Preview();
    HUD->Selected.Add(TEXT("form.purchase.resale"), TEXT("yes"));
    HUD->Fields[TEXT("purchase.resale")]->SetText(FText::FromString(TEXT("1,00")));
    HUD->Preview();
    TestTrue(TEXT("Optional result explicitly uncertain"),
             HUD->FormPreview->GetText().ToString().Contains(TEXT("Unverbindliche Weiterverkaufsschätzung")));
    TestEqual(TEXT("Only a buy command, never implicit resale"), HUD->Draft.Order.Side, FString(TEXT("buy")));
    Sim->TogglePause();
    Sim->Tick(1.1f);
    Sim->TogglePause();
    HUD->Refresh();
    TestFalse(TEXT("State change invalidates old confirmation"), HUD->ConfirmButton->GetIsEnabled());
    HUD->Preview();
    if (!TestTrue(TEXT("Refreshed preview valid"), HUD->ValidDraft))
        return false;
    HUD->Confirm();
    auto Ticket = HUD->PendingTicket;
    HUD->Confirm();
    TestTrue(TEXT("Single command ticket"), Ticket > 0 && HUD->PendingTicket == Ticket);
    HUD->Escape();
    Sim->TogglePause();
    Sim->Tick(1.1f);
    Sim->TogglePause();
    TestEqual(TEXT("Exactly one committed business after closing submitted dialog"),
              Sim->GetSnapshot().Player.Businesses.Num(), 1);
    for (auto Size : {FVector2D(1280, 720), FVector2D(1920, 1080)})
        HUD->NativeTick(FGeometry::MakeRoot(Size, FSlateLayoutTransform()), 0);
    return true;
}
#endif

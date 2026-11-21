#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "GeoCatalog.h"
#include "Simulation/SimulationSubsystem.h"
#include "Simulation/SimArchive.h"
#include "Simulation/TradeBusiness.h"
#include "Simulation/SaveGame.h"
#include "Serialization/JsonSerializer.h"
#include "UI/PlayerHUD.h"
using namespace CoISim;
namespace
{
bool BusinessFixture(FString &Json, FCoIGeoMapData &Geo, FScenario &Scenario, FString &Source, FString &Target,
                     FString &Good, FString &Seller, FString &Truck)
{
    auto *Catalog = LoadObject<UCoIGeoCatalog>(nullptr, TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));
    FString E, Raw;
    if (!Catalog || !Geo.Load(Catalog->DefinitionJson, E) ||
        !FFileHelper::LoadFileToString(Raw, *(FPaths::ProjectContentDir() / TEXT("Simulation/scenario.coisim"))) ||
        !LoadScenario(Raw, Geo, Scenario, E))
        return false;
    Scenario.Economy.Stage = 4;
    Scenario.Economy.AccountingEnabled = false;
    Scenario.Economy.Bank.Enabled = false;
    Scenario.Economy.RetailEnabled = false;
    Scenario.Economy.HouseholdModel = false;
    Scenario.Economy.LegacyHouseholdPurchases = false;
    for (auto &G : Scenario.Definitions.Goods)
        G.SpoilPartsPerMillionPerDay = 0;
    for (const auto &I : Scenario.InitialState.Inventories)
        if (I.OwnerId != Scenario.PlayerCompanyId && Find(Scenario.Definitions.Companies, I.OwnerId) &&
            I.QuantityKg >= 10)
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
    auto *W = Find(Scenario.InitialState.Warehouses, Source);
    if (!W || Target.IsEmpty())
        return false;
    for (auto &V : Scenario.InitialState.Vehicles)
        if (V.OwnerId == Scenario.PlayerCompanyId)
        {
            V.CityId = W->CityId;
            Truck = V.Id;
            break;
        }
    FWrite Writer;
    Scenario.Fields(Writer);
    FJsonSerializer::Serialize(Writer.Object, TJsonWriterFactory<>::Create(&Json));
    return !Truck.IsEmpty();
}
FCommand Order(FSimulation &S, const FString &Id, const FString &Actor, const FString &Store, const FString &Good,
               const FString &Side, int64 Kg, bool Track = false)
{
    FCommand C;
    C.Kind = ECommandKind::PlaceOrder;
    C.Sequence = S.NextSequence();
    C.TargetStep = S.GetState().Step + 1;
    C.Generation = S.GetGeneration();
    C.CompanyId = Actor;
    C.Origin = Track ? ECommandOrigin::Player : ECommandOrigin::System;
    C.TrackTrade = Track;
    auto &O = C.Order;
    O.Id = Id;
    O.ActorId = Actor;
    O.StorageId = Store;
    O.GoodId = Good;
    O.Side = Side;
    O.OriginalKg = Kg;
    O.LimitCentsPerKg = 1;
    O.CityId = Find(S.GetState().Warehouses, Store)->CityId;
    O.MarketId = Find(S.GetScenario().Definitions.Regions, Find(S.GetScenario().Definitions.Places, O.CityId)->RegionId)
                     ->MarketId;
    return C;
}
} // namespace
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTradeBusinessPersistence, "CoI.UI08.PartialPurchaseTransportPersistenceAndRights",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTradeBusinessPersistence::RunTest(const FString &)
{
    FString Json, E, Source, Target, Good, Seller, Truck;
    FCoIGeoMapData Geo;
    FScenario Scenario;
    if (!BusinessFixture(Json, Geo, Scenario, Source, Target, Good, Seller, Truck))
        return false;
    FSimulation S;
    if (!TestTrue(*E, S.Load(Json, Geo, E)))
        return false;
    auto Sell = Order(S, TEXT("ui08.sell"), Seller, Source, Good, TEXT("sell"), 10);
    TestTrue(*E, S.Enqueue(Sell, E));
    auto Buy = Order(S, TEXT("ui08.buy"), Scenario.PlayerCompanyId, Source, Good, TEXT("buy"), 20, true);
    TestTrue(*E, S.Enqueue(Buy, E));
    FString Save;
    TestTrue(TEXT("Pending tracked order saved"), S.SaveGameJson(Save, E));
    FSimulation Loaded;
    int32 Speed = 0;
    if (!TestTrue(*E, Loaded.LoadGameJson(Save, Geo, S.GetScenario(), E, Speed)))
        return false;
    S.StepOnce();
    Loaded.StepOnce();
    TestEqual(TEXT("Pending metadata resumes identically"), S.Fingerprint(), Loaded.Fingerprint());
    if (!TestEqual(TEXT("Exactly one business per order"), S.GetState().Businesses.Num(), 1))
        return false;
    const auto B = S.GetState().Businesses[0];
    TestEqual(TEXT("Partial fill from actual settlement"), B.FilledKg, 10LL);
    TestEqual(TEXT("Actual paid amount"), B.SettledCents, 10LL);
    TestEqual(TEXT("Partially bought state"), BusinessPhase(B, false), FString(TEXT("Teilweise gekauft")));
    TestTrue(*E, S.ValidateSaveState(E));
    FCommand Cancel;
    Cancel.Kind = ECommandKind::CancelOrder;
    Cancel.Generation = S.GetGeneration();
    Cancel.TargetStep = S.GetState().Step + 1;
    Cancel.Sequence = S.NextSequence();
    Cancel.Order.Id = Buy.Order.Id;
    Cancel.Order.ActorId = Scenario.PlayerCompanyId;
    TestTrue(*E, S.Enqueue(Cancel, E));
    S.StepOnce();
    TestEqual(TEXT("Partial purchase survives cancel; pickup required"),
              BusinessPhase(S.GetState().Businesses[0], false), FString(TEXT("Gekauft – Abholung erforderlich")));
    auto *Stock = S.GetState().Inventories.FindByPredicate([&](const FInventory &I) {
        return I.OwnerId == Scenario.PlayerCompanyId && I.StorageId == Source && I.GoodId == Good;
    });
    if (!Stock)
        return false;
    FCommand Haul;
    Haul.Kind = ECommandKind::BookTransport;
    Haul.CompanyId = Scenario.PlayerCompanyId;
    Haul.Origin = ECommandOrigin::Player;
    Haul.TradeCaseId = B.Id;
    Haul.Generation = S.GetGeneration();
    Haul.TargetStep = S.GetState().Step + 1;
    Haul.Sequence = S.NextSequence();
    auto &J = Haul.Transport;
    J.Id = TEXT("ui08.haul");
    J.OwnerId = Scenario.PlayerCompanyId;
    J.VehicleId = Truck;
    J.SourceInventoryId = Stock->Id;
    J.GoodId = Good;
    J.FromCityId = Find(S.GetState().Warehouses, Source)->CityId;
    J.ToCityId = Find(S.GetState().Warehouses, Target)->CityId;
    J.DestinationStorageId = Target;
    J.Kg = 11;
    const FString SourceStockId = Stock->Id;
    const auto Pre = S.GetState();
    TestTrue(*E, S.Enqueue(Haul, E));
    S.StepOnce();
    TestEqual(TEXT("Too large link creates no transport"), S.GetState().Transports.Num(), Pre.Transports.Num());
    TestTrue(TEXT("Failed linkage has visible reason"), !S.GetState().Businesses[0].LastReason.IsEmpty());
    TestEqual(TEXT("Purchased stock not lost after failure"), Find(S.GetState().Inventories, SourceStockId)->QuantityKg,
              10LL);
    Haul.Sequence = S.NextSequence();
    Haul.TargetStep = S.GetState().Step + 1;
    Haul.Transport.Kg = 10;
    Haul.Transport.RequestedDepartureHour = S.GetState().EconomyHour + 2;
    TestTrue(*E, S.Enqueue(Haul, E));
    S.StepOnce();
    if (!TestEqual(TEXT("Only valid transport linked"), S.GetState().Businesses[0].Legs.Num(), 1))
        return false;
    TestTrue(*E, S.ValidateSaveState(E));
    TestTrue(*E, S.SaveGameJson(Save, E));
    if (!TestTrue(*E, Loaded.LoadGameJson(Save, Geo, S.GetScenario(), E, Speed)))
        return false;
    int64 Arrival = Find(S.GetState().Transports, J.Id)->ArrivalStep;
    for (int32 I = 0; S.GetState().EconomyHour <= Arrival && I < 400; ++I)
    {
        S.StepOnce();
        Loaded.StepOnce();
    }
    TestEqual(TEXT("Arrival and organization resume exactly once"), S.Fingerprint(), Loaded.Fingerprint());
    TestEqual(TEXT("Delivered from actual job"), S.GetState().Businesses[0].Legs[0].DeliveredKg, 10LL);
    TestEqual(TEXT("No second purchase"), S.GetState().Businesses[0].SettledCents, 10LL);
    TestTrue(*E, S.ValidateSaveState(E));
    TestTrue(*E, S.SaveGameJson(Save, E));
    TSharedPtr<FJsonObject> Root;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Save), Root);
    FSaveData Saved;
    FRead Read{Root->GetObjectField(TEXT("Payload")), E, true};
    Saved.Fields(Read);
    Read.Complete();
    auto RejectDamaged = [&](FSaveData Bad, const FString &Label) {
        FFingerprint Hash;
        Hash.Write(Bad);
        FWrite Writer;
        Bad.Fields(Writer);
        auto Envelope = MakeShared<FJsonObject>();
        Envelope->SetStringField(TEXT("Format"), TEXT("CoI.Save"));
        Envelope->SetStringField(TEXT("Checksum"), Hash.Finish());
        Envelope->SetObjectField(TEXT("Payload"), Writer.Object);
        FString Broken;
        FJsonSerializer::Serialize(Envelope, TJsonWriterFactory<>::Create(&Broken));
        const auto Before = S.Fingerprint();
        TestFalse(*Label, S.LoadGameJson(Broken, Geo, S.GetScenario(), E, Speed));
        TestEqual(TEXT("Rejected save changes no live state"), S.Fingerprint(), Before);
    };
    auto Bad = Saved;
    Bad.State.Businesses[0].FilledKg += 1;
    RejectDamaged(Bad, TEXT("Fabricated bought quantity rejected"));
    Bad = Saved;
    const auto DuplicateLeg = Bad.State.Businesses[0].Legs[0];
    Bad.State.Businesses[0].Legs.Add(DuplicateLeg);
    RejectDamaged(Bad, TEXT("Duplicate transport attribution rejected"));
    Bad = Saved;
    Bad.State.Businesses[0].OwnerId = Seller;
    RejectDamaged(Bad, TEXT("Foreign business ownership rejected"));
    for (const auto &Trade : S.GetSnapshot().Player.Trades)
        TestTrue(TEXT("No foreign private trade references in player projection"),
                 Trade.BuyerId == Scenario.PlayerCompanyId || Trade.SellerId == Scenario.PlayerCompanyId);
    TestTrue(TEXT("Foreign closed order absent from public view"),
             Find(S.GetSnapshot().Player.Orders, Sell.Order.Id) == nullptr);
    auto Foreign = Haul;
    Foreign.Sequence = S.NextSequence();
    Foreign.TargetStep = S.GetState().Step + 1;
    Foreign.CompanyId = Seller;
    Foreign.Transport.OwnerId = Seller;
    TestFalse(TEXT("Foreign actor cannot link private business"), S.Enqueue(Foreign, E));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTradeMenuWidgets, "CoI.UI08.MenuFiltersPlanningLiveOffersAndNoDuplicateReads",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTradeMenuWidgets::RunTest(const FString &)
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
    Sim->TogglePause();
    Sim->Tick(1.1f);
    Sim->TogglePause();
    auto HasText = [&](const FString &Value) {
        TArray<UWidget *> All;
        HUD->WidgetTree->GetAllWidgets(All);
        for (const auto *W : All)
            if (const auto *T = Cast<UTextBlock>(W); T && T->GetText().ToString() == Value)
                return true;
        return false;
    };
    const auto Hash = Sim->Fingerprint();
    HUD->Action(TEXT("view|Märkte"));
    TestTrue(TEXT("Navigation shows Handel"), HasText(TEXT("Handel")));
    TestTrue(TEXT("Three sections"),
             HasText(TEXT("Angebote")) && HasText(TEXT("Handelsplanung")) && HasText(TEXT("Laufende Geschäfte")));
    const auto PreviousGood = HUD->Id(TEXT("good"));
    HUD->Action(TEXT("tradeFilters"));
    TestEqual(TEXT("Opening filters never changes selected good"), HUD->Id(TEXT("good")), PreviousGood);
    HUD->TradingMinimum->SetText(FText::FromString(TEXT("-1")));
    HUD->TradingFilterChanged();
    TestTrue(TEXT("Bad filter explains valid kg"),
             HasText(TEXT("Mengenfilter: ganze kg zwischen 0 und ") +
                     Display::Number(Sim->GetSnapshot().Player.Rules.MaxOrderKg) + TEXT(" eingeben.")));
    HUD->TradingMinimum->SetText(FText::FromString(TEXT("0")));
    HUD->TradingFilterChanged();
    const auto *Offer = Sim->GetSnapshot().Player.Orders.FindByPredicate([&](const FMarketOrder &O) {
        return O.ActorId != Sim->GetSnapshot().Player.ActorId && O.Side == TEXT("sell") && O.Status == TEXT("open");
    });
    if (!TestNotNull(TEXT("Real public offer"), Offer))
        return false;
    auto PlanButtons = [&]() {
        TArray<UWidget *> All;
        HUD->WidgetTree->GetAllWidgets(All);
        int32 Count = 0;
        for (const auto *W : All)
            if (const auto *B = Cast<UCoIActionButton>(W); B && B->Action.StartsWith(TEXT("tradePlan|")))
                ++Count;
        return Count;
    };
    HUD->Selected.Add(TEXT("good"), Offer->GoodId);
    HUD->Selected.Add(TEXT("trade.provider"), Offer->ActorId);
    HUD->Selected.Add(TEXT("trade.minimum"), LexToString(Offer->RemainingKg));
    HUD->RefreshTable();
    TestTrue(TEXT("Filter caption immediately follows selected good"),
             CastChecked<UTextBlock>(HUD->TradingFilterToggle->GetContent())
                 ->GetText()
                 .ToString()
                 .Contains(Display::Name(HUD->Good(Offer->GoodId))));
    int32 Expected = 0;
    for (const auto &O : Sim->GetSnapshot().Player.Orders)
        if (O.Status == TEXT("open") && O.Side == TEXT("sell") && O.GoodId == Offer->GoodId &&
            O.ActorId == Offer->ActorId && O.RemainingKg >= Offer->RemainingKg)
            ++Expected;
    TestEqual(TEXT("Provider, good and quantity filters match current book"), PlanButtons(), Expected);
    HUD->Selected.Add(TEXT("trade.minimum"), TEXT("0"));
    HUD->RefreshTable();
    const FString OfferId = Offer->Id;
    HUD->Action(TEXT("tradePlan|") + OfferId);
    TestTrue(TEXT("Physical source visible in plan"),
             HasText(Display::Name(TEXT("Tatsächlicher Quell-/Handelsort: ") + HUD->Store(Offer->StorageId))));
    TestTrue(TEXT("No resale required"), HUD->Choices.Contains(TEXT("trade.purpose")));
    HUD->Action(TEXT("tradeQuote"));
    TestEqual(TEXT("Existing buy dialog used"), HUD->DialogKind, FString(TEXT("buy")));
    HUD->Escape();
    auto &P = const_cast<FPlayerView &>(Sim->GetSnapshot().Player);
    auto *Live = Find(P.Orders, OfferId);
    const auto Old = *Live;
    Live->Status = TEXT("cancelled");
    HUD->RefreshTable();
    HUD->Action(TEXT("tradeQuote"));
    TestTrue(TEXT("Stale offer cannot create quote"), HUD->DialogKind.IsEmpty());
    *Live = Old;
    for (int32 I = 0; I < 8; ++I)
    {
        HUD->Action(TEXT("tradeTab|offers"));
        HUD->Action(TEXT("tradeTab|running"));
    }
    TestEqual(TEXT("Opening sections never creates organization or orders"), Sim->Fingerprint(), Hash);
    HUD->Action(TEXT("city"));
    const auto Home = HUD->CityContext;
    const auto Good = HUD->Id(TEXT("good"));
    HUD->Action(TEXT("cityTab|Markt"));
    TestEqual(TEXT("City context carried"), HUD->CityContext, Home);
    TestEqual(TEXT("Good carried"), HUD->Id(TEXT("good")), Good);
    for (const FVector2D Size : {FVector2D(1280, 720), FVector2D(1920, 1080)})
        HUD->NativeTick(FGeometry::MakeRoot(Size, FSlateLayoutTransform()), 0);
    return true;
}
#endif

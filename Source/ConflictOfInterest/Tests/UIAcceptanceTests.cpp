#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "HAL/FileManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Components/Border.h"
#include "GeoCatalog.h"
#include "Simulation/SimulationSubsystem.h"
#include "Simulation/SimArchive.h"
#include "Simulation/CityDetails.h"
#include "UI/PlayerHUD.h"
#include "Slate/WidgetRenderer.h"
#include "ImageUtils.h"
#include "RenderingThread.h"
using namespace CoISim;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIEndToEnd, "CoI.UI06.EndToEndSnapshotFocusAndPerformance",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUIEndToEnd::RunTest(const FString &)
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
    auto Choose = [&](const FString &Key, const FString &Value) {
        for (const auto &Pair : HUD->ChoiceValues.FindRef(Key))
            if (Pair.Value == Value)
            {
                HUD->Choices[Key]->SetSelectedOption(Pair.Key);
                return true;
            }
        return false;
    };
    auto Advance = [&]() {
        if (Sim->IsPaused())
            Sim->TogglePause();
        Sim->Tick(1.1f);
        if (!Sim->IsPaused())
            Sim->TogglePause();
    };
    HUD->Action(TEXT("header|overview"));
    TestTrue(TEXT("Company KPIs use same published snapshot"),
             HasText(Sim->GetSnapshot().Player.Company.AvailableMoney.Display()));
    HUD->Action(TEXT("city"));
    const FString Home = HUD->CityContext;
    HUD->Action(TEXT("view|Märkte"));HUD->Action(TEXT("buy"));
    const auto* InitialStore=Find(Sim->GetSnapshot().Player.Stores,HUD->Id(TEXT("form.store")));
    TestTrue(TEXT("Global market without offer defaults to selected region"),InitialStore&&InitialStore->CityId==Home);
    HUD->Escape();HUD->Action(TEXT("city|")+Home);
    HUD->Action(TEXT("cityTab|Markt"));
    HUD->Action(TEXT("buy"));
    HUD->Fields[TEXT("quantity")]->SetText(FText::FromString(TEXT("1")));
    HUD->Fields[TEXT("price")]->SetText(FText::FromString(TEXT("0,01")));
    HUD->Action(TEXT("preview"));
    TestTrue(TEXT("Existing market command preview valid"), HUD->ValidDraft);
    HUD->Action(TEXT("confirm"));
    const int64 TradeTicket = HUD->PendingTicket;
    HUD->Action(TEXT("confirm"));
    TestEqual(TEXT("Second confirm retains same ticket"), HUD->PendingTicket, TradeTicket);
    Advance();
    int32 Results = 0;
    for (const auto &R : Sim->GetCommandResults())
        if (R.Sequence == TradeTicket)
        {
            ++Results;
            TestTrue(TEXT("Trade accepted by existing authoritative service"), R.Accepted);
        }
    TestEqual(TEXT("Double confirm executes one command"), Results, 1);
    FString StockId, TargetId;
    for (const auto &I : Sim->GetSnapshot().Player.Stocks)
        if (I.QuantityKg > I.ReservedKg)
        {
            const auto *W = Find(Sim->GetSnapshot().Player.Stores, I.StorageId);
            if (W && W->Own && W->CityId == Home)
            {
                StockId = I.Id;
                break;
            }
        }
    for (const auto &W : Sim->GetSnapshot().Player.Stores)
        if (W.Public && W.CityId != Home)
        {
            TargetId = W.Id;
            break;
        }
    if (!TestFalse(TEXT("Existing stock available"), StockId.IsEmpty()) ||
        !TestFalse(TEXT("Existing public target"), TargetId.IsEmpty()))
        return false;
    HUD->Action(TEXT("cityTab|Transporte"));
    HUD->Action(TEXT("haul|") + StockId);
    TestTrue(TEXT("Existing target selectable"), Choose(TEXT("form.target"), TargetId));
    HUD->Fields[TEXT("quantity")]->SetText(FText::FromString(TEXT("1")));
    HUD->Action(TEXT("preview"));
    TestTrue(TEXT("Transport quote valid"), HUD->ValidDraft);
    TestTrue(TEXT("Typing context blocks camera keys"), HUD->KeyboardBlocked());
    HUD->Action(TEXT("confirm"));
    Advance();
    TestEqual(TEXT("One authoritative transport"), Sim->GetSnapshot().Player.Shipments.Num(), 1);
    if (Sim->GetSnapshot().Player.Shipments.IsEmpty())
        return false;
    const FString Job = Sim->GetSnapshot().Player.Shipments[0].Id;
    HUD->Action(TEXT("view|Meldungen"));
    TestTrue(
        TEXT("Transport confirmation visible in real event stream"),
        Sim->GetSnapshot().Player.Events.ContainsByPredicate([&](const FPlayerEvent &E) { return E.ObjectId == Job; }));
    const auto *Event =
        Sim->GetSnapshot().Player.Events.FindByPredicate([&](const FPlayerEvent &E) { return E.ObjectId == Job; });
    if (Event)
        HUD->Action(TEXT("event|") + Event->Id);
    TestEqual(TEXT("Event opens associated transport"), HUD->View, FString(TEXT("Transporte")));
    TestEqual(TEXT("Event keeps object identity"), HUD->FocusedObject, Job);
    HUD->Action(TEXT("city|") + Home);
    HUD->Action(TEXT("cityTab|Eigene Assets"));
    TestEqual(TEXT("Own asset actions still reachable"), HUD->CityTab, FString(TEXT("Eigene Assets")));
    HUD->Action(TEXT("cityTab|Investitionen"));
    TestTrue(TEXT("Investment catalog still reachable"), HasText(TEXT("NEUBAU")));
    HUD->Action(TEXT("deliveryList"));
    TestEqual(TEXT("Existing delivery-rule screen reachable"), HUD->View, FString(TEXT("Lieferregeln")));
    HUD->Action(TEXT("city|") + Home);
    HUD->NativeTick(FGeometry::MakeRoot(FVector2D(1280, 720), FSlateLayoutTransform()), 0);
    const auto Count = HUD->TableBuildCount;
    const auto Hash = Sim->Fingerprint();
    const double Started = FPlatformTime::Seconds();
    for (int32 I = 0; I < 240; ++I)
    {
        HUD->NativeTick(FGeometry::MakeRoot(FVector2D(1280, 720), FSlateLayoutTransform()), 1.f / 60);
        HUD->Refresh();
    }
    const double FrameMicros = (FPlatformTime::Seconds() - Started) * 1000000. / 240.;
    TestEqual(TEXT("240 idle frames never rebuild tables"), HUD->TableBuildCount, Count);
    TestEqual(TEXT("UI performance reads do not book transactions"), Sim->Fingerprint(), Hash);
    if (Sim->IsPaused())
        Sim->TogglePause();
    HUD->Action(TEXT("cityTab|Markt"));
    TestFalse(TEXT("Opening ordinary panel never pauses time"), Sim->IsPaused());
    Sim->Tick(1.1f);
    Sim->TogglePause();
    TestTrue(TEXT("Snapshot version reaches HUD"), HUD->SeenVersion == Sim->GetSnapshot().Version);
    // Render-only stress modifies a detached public presentation copy, not the simulation.
    HUD->Action(TEXT("city|") + Home);
    auto *C = const_cast<FCityView *>(Find(Sim->GetSnapshot().Player.Cities, Home));
    if (!C)
        return false;
    const auto Original = *C;
    FCityPresence Long;
    Long.CompanyId = TEXT("ui06.stress");
    Long.Name = TEXT(
        "Internationales Handelshaus für Rohstoffe, Lebensmittel und regionale Logistik mit sehr langem Firmennamen");
    Long.Activities = {TEXT("Einzelhandel"), TEXT("Lagerhaltung")};
    Long.Warehouses = 3;
    for (int32 I = 0; I < 50; ++I)
    {
        auto Row = Long;
        Row.CompanyId += LexToString(I);
        C->Presence.Add(Row);
    }
    HUD->CityDetails[Home].Companies = true;
    HUD->RefreshTable();
    TestTrue(TEXT("Full long name retained"), HasText(Long.Name));
    const bool Render = FParse::Param(FCommandLine::Get(), TEXT("CoIUIRender"));
    if (Render)
    {
        auto &Published = const_cast<FSnapshot &>(Sim->GetSnapshot());
        const auto OldName = Published.PlayerName;
        const auto OldResult = Published.Player.Company.MonthlyResult;
        Published.PlayerName = Long.Name;
        Published.Player.Company.MonthlyResult.State = EDisplayState::Value;
        Published.Player.Company.MonthlyResult.Value = -12345678;
        HUD->RefreshCompanyHeader();
        FWidgetRenderer Renderer(false);
        const FString Directory = FPaths::ProjectSavedDir() / TEXT("UI06_Render");
        IFileManager::Get().MakeDirectory(*Directory, true);
        auto Shot = [&](const FString &Name, FVector2D Size, float Scale) {
            HUD->NativeTick(FGeometry::MakeRoot(Size / Scale, FSlateLayoutTransform(Scale)), 0);
            Slate->SlatePrepass(Scale);
            auto *RT = FWidgetRenderer::CreateTargetFor(Size, TF_Bilinear, false);
            if (!RT)
                return false;
            // Two passes resolve desired sizes, clipping and scroll offsets like the live viewport.
            Renderer.DrawWidget(RT, Slate, Scale, Size, 0);
            Renderer.DrawWidget(RT, Slate, Scale, Size, 0);
            FlushRenderingCommands();
            TArray<FColor> Pixels;
            FReadSurfaceDataFlags Flags(RCM_UNorm);
            Flags.SetLinearToGamma(false);
            if (!RT->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags))
                return false;
            TArray64<uint8> PNG;
            FImageUtils::PNGCompressImageArray(int32(Size.X), int32(Size.Y), Pixels, PNG);
            return FFileHelper::SaveArrayToFile(PNG, *(Directory / Name));
        };
        for (const auto Size : {FVector2D(1280, 720), FVector2D(1920, 1080)})
        {
            const FString Prefix = LexToString(int32(Size.X));
            HUD->WorkScroll->SetScrollOffset(570);
            TestTrue(TEXT("Stress render saved"), Shot(Prefix + TEXT("_firms.png"), Size, 1.f));
            HUD->WorkScroll->SetScrollOffset(0);
            TestTrue(TEXT("DPI stress render saved"), Shot(Prefix + TEXT("_dpi125.png"), Size, 1.25f));
        }
        HUD->Action(TEXT("view|Lager"));
        for (auto &W : Published.Player.Stores)
            if (W.Own && W.CityId == Home)
            {
                const auto Old = W;
                W.Used = W.Capacity;
                W.Incoming = 0;
                HUD->FocusedObject = W.Id;
                HUD->RefreshTable();
                HUD->WorkScroll->SetScrollOffset(180);
                TestTrue(TEXT("Full store visible"), HasText(TEXT("Frei: 0")));
                TestTrue(TEXT("Full warehouse render saved"),
                         Shot(TEXT("1280_full_warehouse.png"), FVector2D(1280, 720), 1.f));
                W = Old;
                break;
            }
        Published.PlayerName = OldName;
        Published.Player.Company.MonthlyResult = OldResult;
        HUD->RefreshCompanyHeader();
    }
    *C = Original;
    HUD->RefreshTable();
    const FString Report = FString::Printf(
        TEXT("240 paused refresh/tick frames: mean %.2f us; table builds before/after %llu/%llu\nLatest table build "
             "%.2f us; peak %.2f us. Single local measurement, not a portable benchmark.\n"),
        FrameMicros, Count, Count, HUD->TableBuildMicros, HUD->PeakTableBuildMicros);
    FFileHelper::SaveStringToFile(Report, *(FPaths::ProjectSavedDir() / TEXT("UI06_Performance.txt")));
    AddInfo(Report);
    HUD->Action(TEXT("buy"));
    Sim->Restart();
    TestTrue(TEXT("Restart closes stale draft"), HUD->DialogKind.IsEmpty());
    TestTrue(TEXT("Restart clears previous city window"), HUD->View.IsEmpty());
    TestTrue(TEXT("Restart clears previous navigation"), HUD->PanelHistory.IsEmpty());
    return true;
}
#endif

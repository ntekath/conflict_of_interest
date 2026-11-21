#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "GeoCatalog.h"
#include "Simulation/SimulationSubsystem.h"
#include "Simulation/SimArchive.h"
#include "UI/PlayerHUD.h"
#include "UI/PanelLayout.h"
using namespace CoISim;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSidePanelFlow, "CoI.UI07.DockNavigationDraftAndLiveUpdates",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSidePanelFlow::RunTest(const FString &)
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
    const auto Before = Sim->Fingerprint();
    HUD->Action(TEXT("city"));
    const auto First = HUD->CityContext;
    for (const FVector2D Size : {FVector2D(1280, 720), FVector2D(1920, 1080)})
    {
        HUD->NativeTick(FGeometry::MakeRoot(Size, FSlateLayoutTransform()), 0);
        auto *P = CastChecked<UCanvasPanelSlot>(HUD->Work->Slot);
        TestEqual(TEXT("Left dock"), P->GetPosition().X, 16.);
        TestEqual(TEXT("Configured width"), P->GetSize().X, CoIPanels::Width(Size.X));
        TestTrue(TEXT("Map stays free beside panel"), !HUD->OverlayCovered({P->GetSize().X + 70, 400}, {60, 60}));
        TestTrue(TEXT("City intercept rectangle"), HUD->OverlayCovered({20, P->GetPosition().Y + 10}));
        TestEqual(TEXT("Duplicated right panel hidden"), HUD->SelectionPanel->GetVisibility(),
                  ESlateVisibility::Collapsed);
        TestTrue(TEXT("Map keyboard available without text focus"), !HUD->KeyboardBlocked());
        TestTrue(TEXT("Compact details also at 1080"), HUD->CompactCityDetails);
    }
    HUD->WorkScroll->SetScrollOffset(73);
    HUD->Action(TEXT("cityTab|Markt"));
    HUD->Selected.Add(TEXT("good"), TEXT("good.food"));
    HUD->WorkScroll->SetScrollOffset(109);
    HUD->Action(TEXT("cityTab|Transporte"));
    HUD->Action(TEXT("panelBack"));
    TestEqual(TEXT("Back restores market"), HUD->View, FString(TEXT("Märkte")));
    TestEqual(TEXT("Back restores filter"), HUD->Id(TEXT("good")), FString(TEXT("good.food")));
    TestEqual(TEXT("Back restores scroll"), HUD->WorkScroll->GetScrollOffset(), 109.f);
    HUD->Action(TEXT("panelBack"));
    TestEqual(TEXT("Back restores overview scroll"), HUD->WorkScroll->GetScrollOffset(), 73.f);
    HUD->Action(TEXT("buy"));
    auto *Field = HUD->Fields.FindRef(TEXT("quantity")).Get();
    if (!TestNotNull(TEXT("Existing trade input"), Field))
        return false;
    Field->SetText(FText::FromString(TEXT("17")));
    const auto Token = HUD->Token;
    HUD->Action(TEXT("city|") + Sim->GetSnapshot().Player.Cities[0].Id);
    HUD->Action(TEXT("view|Transporte"));
    TestEqual(TEXT("Draft city not silently discarded"), HUD->CityContext, First);
    TestEqual(TEXT("Draft token preserved"), HUD->Token, Token);
    TestEqual(TEXT("Typed quantity preserved"), Field->GetText().ToString(), FString(TEXT("17")));
    TestTrue(TEXT("Dialog blocks camera"), HUD->KeyboardBlocked());
    HUD->Escape();
    TestTrue(TEXT("First Escape closes detail"), HUD->DialogKind.IsEmpty() && !HUD->View.IsEmpty());
    auto Other = Sim->GetSnapshot().Player.Cities.FindByPredicate([&](const FCityView &X) { return X.Id != First; });
    if (!Other)
        return false;
    const FString OtherId = Other->Id;
    auto *Surface = HUD->Work.Get();
    HUD->MapSelection(OtherId);
    TestEqual(TEXT("Map click changes existing city panel"), HUD->CityContext, OtherId);
    TestTrue(TEXT("Same main surface reused"), HUD->Work.Get() == Surface);
    HUD->Action(TEXT("cityTab|Investitionen"));
    HUD->Action(TEXT("cityTab|Eigene Assets"));
    HUD->Action(TEXT("cityTab|Übersicht"));
    TestEqual(TEXT("Navigation never books economic changes"), Sim->Fingerprint(), Before);
    Sim->TogglePause();
    Sim->Tick(1.1f);
    Sim->TogglePause();
    TestEqual(TEXT("Live snapshot retains city"), HUD->CityContext, OtherId);
    HUD->Escape();
    TestTrue(TEXT("Second Escape closes main"), HUD->View.IsEmpty());
    TestTrue(TEXT("Ordinary windows never auto pause"), Sim->IsPaused());
    Sim->Restart();
    TestTrue(TEXT("New party clears back history"), HUD->PanelHistory.IsEmpty());
    return true;
}
#endif

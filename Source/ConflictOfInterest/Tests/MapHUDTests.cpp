#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/CanvasPanelSlot.h"
#include "GeoCatalog.h"
#include "Simulation/SimulationSubsystem.h"
#include "Simulation/SimArchive.h"
#include "UI/PlayerHUD.h"
using namespace CoISim;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapHUDNavigation,"CoI.UI03.ContextFiltersNavigationAndMapSpace",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMapHUDNavigation::RunTest(const FString&)
{
 auto* Catalog=LoadObject<UCoIGeoCatalog>(nullptr,TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));if(!TestNotNull(TEXT("Existing US regression catalog"),Catalog))return false;
 const auto IV=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&IV);if(!World)return false;ON_SCOPE_EXIT{World->DestroyWorld(false);};World->SetGameInstance(NewObject<UGameInstance>(World));World->SpawnActor<ACoIGeoReference>()->Catalog=Catalog;
 auto* Sim=World->GetSubsystem<UCoISimulationSubsystem>();if(!Sim->Restart())return false;auto* HUD=NewObject<UCoIPlayerHUD>(World);HUD->Initialize();auto Slate=HUD->TakeWidget();ON_SCOPE_EXIT{HUD->ReleaseSlateResources(true);};
 auto Shown=[](const UWidget* Widget){for(auto* W=Widget;W;W=W->GetParent())if(W->GetVisibility()==ESlateVisibility::Collapsed||W->GetVisibility()==ESlateVisibility::Hidden)return false;return true;};
 const auto Before=Sim->Fingerprint();
 TestFalse(TEXT("Analysis starts closed"),Shown(HUD->AnalysisBody));TestFalse(TEXT("Display options start closed"),Shown(HUD->DisplayBody));TestFalse(TEXT("No empty selection box"),Shown(HUD->SelectionPanel));TestFalse(TEXT("No permanent explanation legend"),Shown(HUD->MapLegend));TestFalse(TEXT("No passive notice footer"),Shown(HUD->Notices));TestTrue(TEXT("One discreet scenario hint"),Shown(HUD->ScenarioHint));
 HUD->Action(TEXT("analysisToggle"));TestTrue(TEXT("Analysis expands"),Shown(HUD->AnalysisBody));TestFalse(TEXT("Regions need no goods"),Shown(HUD->Choices[TEXT("map.good")]));
 HUD->Action(TEXT("mode|supply"));TestTrue(TEXT("Supply shows goods and need kind"),Shown(HUD->Choices[TEXT("map.good")])&&Shown(HUD->Choices[TEXT("map.kind")]));TestTrue(TEXT("Supply opens compact legend"),Shown(HUD->MapLegend));
 HUD->Choices[TEXT("map.good")]->SetSelectedIndex(1);HUD->Choices[TEXT("map.kind")]->SetSelectedIndex(1);const auto Good=HUD->Id(TEXT("map.good"));
 HUD->Action(TEXT("displayToggle"));HUD->Choices[TEXT("map.routes")]->SetSelectedIndex(2);HUD->Choices[TEXT("map.animate")]->SetSelectedIndex(1);HUD->Action(TEXT("displayToggle"));HUD->Action(TEXT("mode|price"));TestTrue(TEXT("Price has goods"),Shown(HUD->Choices[TEXT("map.good")]));TestFalse(TEXT("Price has no need kind"),Shown(HUD->Choices[TEXT("map.kind")]));HUD->Action(TEXT("mode|regions"));HUD->Action(TEXT("analysisToggle"));HUD->Action(TEXT("view|Unternehmen"));HUD->Escape();HUD->Action(TEXT("mode|supply"));HUD->Action(TEXT("analysisToggle"));
 TestEqual(TEXT("Goods survive layer, collapse and work view"),HUD->Id(TEXT("map.good")),Good);TestEqual(TEXT("Need kind preserved"),HUD->Id(TEXT("map.kind")),FString(TEXT("inputs")));TestEqual(TEXT("Route filter preserved"),HUD->Id(TEXT("map.routes")),FString(TEXT("public")));TestEqual(TEXT("Animation setting preserved"),HUD->Id(TEXT("map.animate")),FString(TEXT("no")));
 const auto City=Sim->GetSnapshot().Player.Cities[0];HUD->MapSelection(City.RegionId);TestTrue(TEXT("Region offers primary city action"),Shown(HUD->CityOpenButton));TestTrue(TEXT("Region offers its market"),Shown(HUD->MarketOpenButton));TestFalse(TEXT("Region has no arbitrary cargo action"),Shown(HUD->PlanButton));HUD->Action(TEXT("openSelectedCity"));TestEqual(TEXT("Region opens actual hub"),HUD->CityContext,City.Id);TestEqual(TEXT("City and selected hub synchronized"),HUD->SelectionId,City.Id);HUD->Escape();TestTrue(TEXT("Selection restored after city closes"),Shown(HUD->SelectionPanel));
 HUD->Action(TEXT("selectedMarket"));TestEqual(TEXT("Context market is same city"),HUD->CityContext,City.Id);HUD->Escape();HUD->MapSelection(TEXT("country.usa"),TEXT("USA · Land"));TestFalse(TEXT("Country has no city action"),Shown(HUD->CityOpenButton));TestFalse(TEXT("Country has no economic region detail"),Shown(HUD->MetricDetails));
 // Read only visibility contract: insert public presentation rows, never simulation entities.
 auto& P=const_cast<FPlayerView&>(Sim->GetSnapshot().Player);FRouteActivity Public;Public.Id=TEXT("route.ui03.public");Public.FromCityId=P.Cities[0].Id;Public.ToCityId=P.Cities[1].Id;Public.Active=2;P.Routes.Add(Public);HUD->MapSelection(Public.Id);TestFalse(TEXT("Public route cannot open private jobs"),Shown(HUD->RouteOpenButton));TestFalse(TEXT("Public route cannot offer cargo action"),Shown(HUD->PlanButton));TestFalse(TEXT("Public route has no own cargo label"),HUD->SelectionText->GetText().ToString().Contains(TEXT("Eigene Fracht")));P.Routes.Last().Own=true;HUD->MapSelection(Public.Id);TestTrue(TEXT("Own route exposes own jobs action"),Shown(HUD->RouteOpenButton));P.Routes.Pop();
 HUD->MapSelection(TEXT(""));HUD->Action(TEXT("mode|regions"));HUD->Action(TEXT("analysisToggle"));
 for(FVector2D Size:{FVector2D(1280,720),FVector2D(1920,1080)}){const auto Geometry=FGeometry::MakeRoot(Size,FSlateLayoutTransform());HUD->NativeTick(Geometry,0);TestFalse(TEXT("Central map remains clear"),HUD->OverlayCovered({500,400},{100,100}));TestFalse(TEXT("Hidden legend does not block labels"),HUD->OverlayCovered({20,Size.Y-150},{10,10}));TestTrue(TEXT("Closed analysis bar blocks map"),HUD->OverlayCovered({20,CastChecked<UCanvasPanelSlot>(HUD->MapControls->Slot)->GetPosition().Y+8},{10,10}));TestTrue(TEXT("Closed controls compact"),CastChecked<UCanvasPanelSlot>(HUD->MapControls->Slot)->GetSize().Y==48);}
 TestEqual(TEXT("All map reads and actions leave economy unchanged"),Sim->Fingerprint(),Before);
 HUD->LastStatus=TEXT("Konkreter Aktionsstatus");HUD->Refresh();TestTrue(TEXT("Action status appears"),Shown(HUD->Notices));HUD->Action(TEXT("dismissNotice"));TestFalse(TEXT("Status dismisses"),Shown(HUD->Notices));HUD->Action(TEXT("menu|open"));TestTrue(TEXT("Modal intercepts wheel everywhere"),HUD->NativeOnMouseWheel(FGeometry(),FPointerEvent()).IsEventHandled());HUD->Escape();
 HUD->AnalysisExpanded=HUD->DisplayExpanded=true;HUD->LastStatus=TEXT("Auftrag wird geprüft");HUD->Refresh();HUD->Action(TEXT("mode|supply"));
 for(FVector2D Size:{FVector2D(1280,720),FVector2D(1920,1080)}){HUD->NativeTick(FGeometry::MakeRoot(Size,FSlateLayoutTransform()),0);auto* L=CastChecked<UCanvasPanelSlot>(HUD->MapLegend->Slot);auto* D=CastChecked<UCanvasPanelSlot>(HUD->DisplayOptions->Slot);const auto A=L->GetPosition(),AS=L->GetSize(),B=D->GetPosition(),BS=D->GetSize();TestFalse(TEXT("Expanded options and legend never overlap with action footer"),A.X<B.X+BS.X&&B.X<A.X+AS.X&&A.Y<B.Y+BS.Y&&B.Y<A.Y+AS.Y);TestTrue(TEXT("Legend stays inside viewport"),A.X>=0&&A.Y>=0&&A.X+AS.X<=Size.X&&A.Y+AS.Y<=Size.Y);}
 HUD->Action(TEXT("dismissNotice"));
 TestEqual(TEXT("Name tag removed only for presentation"),Display::Name(TEXT("Firma (TEST)")),FString(TEXT("Firma")));TestEqual(TEXT("Ordinary name unchanged"),Display::Name(TEXT("Testfirma Nord")),FString(TEXT("Testfirma Nord")));
 Sim->SetSpeed(16);Sim->TogglePause();for(int32 Frame=0;Sim->GetSnapshot().Step<25&&Frame<100;++Frame)Sim->Tick(.5f);Sim->TogglePause();HUD->Action(TEXT("mode|supply"));
 TestTrue(TEXT("Real completed day available"),Sim->GetSnapshot().MapDay.Complete);TestTrue(TEXT("Legend identifies completed data step"),HUD->LegendText->GetText().ToString().Contains(TEXT("Datenstand: Schritt ")+Display::Number(Sim->GetSnapshot().MapDay.CompletedStep)));TestTrue(TEXT("Legend retains percent unit"),HUD->LegendText->GetText().ToString().Contains(TEXT("0 – 100 %")));HUD->Action(TEXT("mode|price"));TestTrue(TEXT("Price legend retains USD/kg unit"),HUD->LegendText->GetText().ToString().Contains(TEXT("USD"))&&HUD->LegendText->GetText().ToString().Contains(TEXT("/ kg")));
 return true;
}
#endif

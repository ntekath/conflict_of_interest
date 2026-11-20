#include "PlayerHUD.h"
#include "Simulation/SimulationSubsystem.h"
#include "Simulation/SimArchive.h"
#include "Simulation/MapNavigation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
using namespace CoISim;
namespace {
void Show(UWidget* W,bool Visible){if(W)W->SetVisibility(Visible?ESlateVisibility::Visible:ESlateVisibility::Collapsed);}
void Label(UCoIActionButton* B,const FString& Value){CastChecked<UTextBlock>(B->GetContent())->SetText(FText::FromString(Value));}
}
void UCoIPlayerHUD::BuildMapPanels()
{
 MapControls=Surface({16,214},{360,48});auto* Analysis=WidgetTree->ConstructWidget<UVerticalBox>();MapControls->SetContent(Analysis);
 AnalysisToggle=Button(Analysis,TEXT("Kartenanalyse +"),TEXT("analysisToggle"));
 AnalysisBody=WidgetTree->ConstructWidget<UVerticalBox>();Analysis->AddChild(AnalysisBody);
 Choice(AnalysisBody,TEXT("map.layer"),TEXT("Layer"),{{TEXT("regions"),TEXT("Regionen")},{TEXT("countries"),TEXT("Länder")},{TEXT("ownership"),TEXT("Eigene Assets")},{TEXT("supply"),TEXT("Versorgung")},{TEXT("price"),TEXT("Tagespreis")}},TEXT("regions"))->SetToolTipText(FText::FromString(TEXT("Eigene Assets markiert Unternehmensstandorte, kein territoriales Eigentum. Versorgung und Preise stammen vom letzten abgeschlossenen Tag.")));
 auto* FieldsRow=Row(AnalysisBody);Choice(FieldsRow,TEXT("map.good"),TEXT("Ware"),{{TEXT("good.food"),TEXT("Nahrung")}},TEXT("good.food"));Choice(FieldsRow,TEXT("map.kind"),TEXT("Versorgungsart"),{{TEXT("house"),TEXT("Haushalte")},{TEXT("inputs"),TEXT("Produktionsinputs")}},TEXT("house"));
 DisplayOptions=Surface({16,270},{360,48});auto* Options=WidgetTree->ConstructWidget<UVerticalBox>();DisplayOptions->SetContent(Options);DisplayToggle=Button(Options,TEXT("Anzeigeoptionen +"),TEXT("displayToggle"));
 DisplayBody=WidgetTree->ConstructWidget<UVerticalBox>();Options->AddChild(DisplayBody);auto* OptionsRow=Row(DisplayBody);
 Choice(OptionsRow,TEXT("map.routes"),TEXT("Routen"),{{TEXT("off"),TEXT("Aus")},{TEXT("own"),TEXT("Eigene")},{TEXT("public"),TEXT("Eigene + öffentliche")}},TEXT("own"))->SetToolTipText(FText::FromString(TEXT("Öffentliche Aktivität ist aggregiertes Szenariowissen. Fremde Einzelaufträge, Frachtmengen, Konten und Zeitpläne bleiben verborgen.")));
 Choice(OptionsRow,TEXT("map.animate"),TEXT("Fahrzeuge"),{{TEXT("yes"),TEXT("An")},{TEXT("no"),TEXT("Aus")}},TEXT("yes"))->SetToolTipText(FText::FromString(TEXT("Lkw-Symbole zeigen exemplarische Bewegungen; sie sind keine vollständige Fahrzeugzählung.")));
 ScenarioHint=Surface({16,680},{260,30});ScenarioHint->SetPadding(FMargin(8,4));auto* Hint=Text(nullptr,TEXT("Testszenario · fiktive Wirtschaft"),10);ScenarioHint->SetContent(Hint);ScenarioHint->SetToolTipText(FText::FromString(TEXT("Wirtschaft, Bevölkerung und Firmen sind fiktive Testwerte. Öffentliche Routen sind aggregiert, bewegte Lkw exemplarisch. Historische Kartengrundlage und Wirtschaftssimulation sind getrennt.")));
 MapLegend=Surface({16,490},{400,142});auto* Legend=WidgetTree->ConstructWidget<UVerticalBox>();MapLegend->SetContent(Legend);LegendText=Text(Legend,TEXT(""),10);auto* Scale=Row(Legend);
 for(int32 I=0;I<3;++I){auto* Color=WidgetTree->ConstructWidget<UBorder>();Color->SetPadding(FMargin(8,3));Scale->AddChildToHorizontalBox(Color)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Color->SetContent(Text(nullptr,I==0?TEXT("niedrig"):I==1?TEXT("Mitte"):TEXT("hoch"),10));LegendColors.Add(Color);}
 SelectionPanel=Surface({958,214},{306,326});auto* Selection=WidgetTree->ConstructWidget<UVerticalBox>();SelectionPanel->SetContent(Selection);auto* Heading=Row(Selection);Text(Heading,TEXT("AUSWAHL"),12,true);Button(Heading,TEXT("– / +"),TEXT("collapse"),false,TEXT("Auswahl ein-/ausklappen"));
 auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();Selection->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));auto* Content=WidgetTree->ConstructWidget<UVerticalBox>();Scroll->AddChild(Content);
 SelectionText=Text(Content,TEXT(""),13);MetricDetails=Text(Content,TEXT(""),11);
 CityOpenButton=Button(Content,TEXT("Stadt öffnen"),TEXT("openSelectedCity"),true);
 MarketOpenButton=Button(Content,TEXT("Markt öffnen"),TEXT("selectedMarket"));StoreOpenButton=Button(Content,TEXT("Eigene Lager"),TEXT("selectedStore"));PlanButton=Button(Content,TEXT("Transport planen"),TEXT("mapPlan"));RouteOpenButton=Button(Content,TEXT("Routenaufträge öffnen"),TEXT("routeJobs"));
 RefreshMapPanels();SelectionPanel->SetVisibility(ESlateVisibility::Collapsed);
}
void UCoIPlayerHUD::RefreshMapPanels()
{
 const bool Map=View.IsEmpty(),Economic=Id(TEXT("map.layer"))==TEXT("supply")||Id(TEXT("map.layer"))==TEXT("price");
 Show(MapControls,Map);Show(DisplayOptions,Map);Show(ScenarioHint,Map);Show(MapLegend,Map&&Economic);
 Show(AnalysisBody,AnalysisExpanded);Show(DisplayBody,DisplayExpanded);Label(AnalysisToggle,FString(TEXT("Kartenanalyse · "))+Choices[TEXT("map.layer")]->GetSelectedOption()+(AnalysisExpanded?TEXT(" -"):TEXT(" +")));Label(DisplayToggle,DisplayExpanded?TEXT("Anzeigeoptionen -"):TEXT("Anzeigeoptionen +"));
 if(auto* GoodChoice=Choices.FindRef(TEXT("map.good")).Get())Show(GoodChoice->GetParent(),Economic);
 if(auto* KindChoice=Choices.FindRef(TEXT("map.kind")).Get())Show(KindChoice->GetParent(),Id(TEXT("map.layer"))==TEXT("supply"));
}
void UCoIPlayerHUD::RefreshSelection()
{
 if(!Sim||!Sim->IsReady()||!SelectionText)return;
 const auto& S=Sim->GetSnapshot();const auto& P=S.Player;
 const auto* Route=Find(P.Routes,SelectionId);const bool IsRoute=SelectionId.StartsWith(TEXT("route."));
 const auto* Place=Find(P.Catalog.Places,SelectionId);const auto* Region=Find(P.Catalog.Regions,SelectionId);const auto* Storage=Find(P.Stores,SelectionId);const auto* Job=Find(P.Shipments,SelectionId);
 const FString Hub=IsRoute?TEXT(""):MapObjectCity(P,SelectionId);const auto* HubPlace=Find(P.Catalog.Places,Hub);const auto* HubRegion=HubPlace?Find(P.Catalog.Regions,HubPlace->RegionId):nullptr;
 const FString Location=HubRegion?HubRegion->Name+TEXT(" · ")+HubRegion->CountryName:FString();
 FString Info=SelectionSummary;
 if(Place){const auto* CityView=Find(P.Cities,Place->Id);Info=Place->Name+TEXT("\nStadthub · ")+Location;if(CityView)Info+=TEXT("\nEigene Beschäftigte: ")+Display::Number(CityView->OwnEmployees)+TEXT("\nEigene Ware: ")+Display::Quantity(CityView->OwnStockKg);}
 else if(Region){Info=(Region->Name.IsEmpty()?City(Hub):Region->Name)+TEXT("\nWirtschaftsregion · ")+Region->CountryName+TEXT("\nStadthub: ")+City(Hub);for(const auto& Row:S.Regions)if(Row.Id==Region->Id)Info+=TEXT("\nBevölkerung: ")+Display::Number(Row.Population)+TEXT(" · beschäftigt: ")+Display::Number(Row.Employed);}
 else if(Storage){Info=Storage->Name+TEXT("\n")+(Storage->Own?TEXT("Eigenes Lager"):Storage->Public?TEXT("Öffentliches Lager"):TEXT("Lagerstandort"))+TEXT(" · ")+City(Hub)+TEXT("\n")+Location;if(Storage->Own)Info+=TEXT("\nBelegt: ")+Display::Number(Storage->Used)+TEXT(" / ")+Display::Number(Storage->Capacity)+TEXT(" Lagerpunkte");}
 else if(Job)Info=City(Job->FromCityId)+TEXT(" → ")+City(Job->ToCityId)+TEXT("\nEigener Transport · ")+Status(Job->Status)+TEXT("\n")+Location+TEXT("\nAnkunft: ")+Time(Job->ArrivalStep);
 else if(Route){Info=City(Route->FromCityId)+TEXT(" → ")+City(Route->ToCityId)+TEXT("\n")+(Route->Own?TEXT("Eigene Route"):TEXT("Öffentliche Route"))+TEXT("\nAktiv: ")+Display::Number(Route->Active)+TEXT(" · unterwegs: ")+Display::Number(Route->Moving);if(const auto* Destination=Find(P.Catalog.Places,Route->ToCityId))if(const auto* DestinationRegion=Find(P.Catalog.Regions,Destination->RegionId))Info+=TEXT("\nZiel: ")+DestinationRegion->Name+TEXT(" · ")+DestinationRegion->CountryName;if(Route->Own)Info+=TEXT("\nEigene Fracht: ")+Display::Quantity(Route->OwnKg)+TEXT("\nNächste Ankunft: ")+Time(Route->NextArrival);SelectionPanel->SetToolTipText(FText::FromString(TEXT("Erneuter Klick wechselt nahe Routen. Öffentliche Aktivität ist aggregiert; keine fremden Einzelaufträge oder Frachtmengen. Lkw-Symbole sind exemplarisch.")));}
 else if(IsRoute)Info=TEXT("Route nicht mehr aktiv oder nicht freigegeben.");
 else for(const auto& Country:S.Countries)if(Country.Id==SelectionId){Info+=TEXT("\nBevölkerung: ")+Display::Number(Country.Population)+TEXT(" · beschäftigt: ")+Display::Number(Country.Employed);}
 if(!IsRoute)SelectionPanel->SetToolTipText(FText::FromString(TEXT("Auswahl und Kennzahlen beschreiben gemeinsame Orte. Unternehmensstandorte bedeuten kein territoriales Eigentum.")));
 SelectionText->SetText(FText::FromString(Display::Name(Info)));
 const bool Expanded=!SelectionCollapsed;Show(SelectionPanel,View.IsEmpty()&&!Info.IsEmpty());
 auto* Selection=CastChecked<UVerticalBox>(SelectionPanel->GetContent());Show(Selection->GetChildAt(1),Expanded);
 const bool HasHub=HubPlace!=nullptr;Show(CityOpenButton,HasHub);Show(MarketOpenButton,HasHub);bool HasOwnStore=false;for(const auto& W:P.Stores)if(W.CityId==Hub&&W.Own)HasOwnStore=true;Show(StoreOpenButton,HasHub&&HasOwnStore);
 const bool HasAccess=Storage&&(Storage->Own||Storage->Public||Storage->AcceptedGoods.Num()>0);
 Show(PlanButton,HasHub&&HasAccess);Show(RouteOpenButton,Route&&Route->Own);RouteOpenButton->SetIsEnabled(Route&&Route->Own);
 Show(MetricDetails,Expanded&&HasHub&&(Id(TEXT("map.layer"))==TEXT("supply")||Id(TEXT("map.layer"))==TEXT("price")));
}

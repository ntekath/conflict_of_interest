#include "PlayerHUD.h"
#include "PanelLayout.h"
#include "Simulation/SimulationSubsystem.h"
#include "Simulation/SimArchive.h"
#include "MapInterface.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
using namespace CoISim;
double UCoIPlayerHUD::PanelWidth(double Width) const
{
    return View.IsEmpty() ? 0. : CoIPanels::Width(Width);
}
UCoIPlayerHUD::FPanelVisit UCoIPlayerHUD::CapturePanel() const
{
    FPanelVisit V;
    V.View = View;
    V.City = CityContext;
    V.Tab = CityTab;
    V.Focus = FocusedObject;
    V.Header = HeaderDetail;
    V.Region = EntireRegion;
    for (const auto &Pair : Selected)
        if (!Pair.Key.StartsWith(TEXT("map.")) && !Pair.Key.StartsWith(TEXT("form.")))
            V.Filters.Add(Pair);
    V.Scroll = WorkScroll && !View.IsEmpty() ? WorkScroll->GetScrollOffset() : 0;
    return V;
}
void UCoIPlayerHUD::RestorePanel(const FPanelVisit &V)
{
    CityContext = V.City;
    CityTab = V.Tab;
    FocusedObject = V.Focus;
    HeaderDetail = V.Header;
    EntireRegion = V.Region;
    TArray<FString> Keys;
    Selected.GetKeys(Keys);
    for (const auto &K : Keys)
        if (!K.StartsWith(TEXT("map.")))
            Selected.Remove(K);
    for (const auto &Pair : V.Filters)
        Selected.Add(Pair);
    BuildView(V.View);
    if (WorkScroll)
        WorkScroll->SetScrollOffset(V.Scroll);
    if (!CityContext.IsEmpty())
    {
        SwitchingCity = true;
        if (MapOwner)
            MapOwner->FocusObject(CityContext, CityContext);
        else
            MapSelection(CityContext);
        SwitchingCity = false;
    }
}
bool UCoIPlayerHUD::PanelAction(const FString &Verb, const FString &Arg)
{
    const bool Navigate = Verb == TEXT("view") || Verb == TEXT("header") || Verb == TEXT("city") ||
                          Verb == TEXT("cityTab") || Verb == TEXT("globalView") || Verb == TEXT("selectedMarket") ||
                          Verb == TEXT("selectedStore") || Verb == TEXT("deliveryList") ||
                          Verb == TEXT("siteReports") || Verb == TEXT("event") || Verb == TEXT("notice") ||
                          Verb == TEXT("mapPlan") || Verb == TEXT("routeJobs") || Verb == TEXT("openSelectedCity") ||
                          Verb == TEXT("centerStore") || Verb == TEXT("centerJob") || Verb == TEXT("closeWork") ||
                          Verb == TEXT("panelBack") || Verb == TEXT("tradeTab") || Verb == TEXT("tradePlan");
    // Never silently destroy a quote, typed values, token or pending confirmation.
    if (Navigate && !DialogKind.IsEmpty())
    {
        if (FormState)
            FormState->SetText(FText::FromString(TEXT(
                "Offenen Entwurf zuerst ausdrücklich abbrechen (Esc) oder bestätigen. Danach die Ansicht wechseln.")));
        return true;
    }
    if (Verb == TEXT("panelBack"))
    {
        if (!PanelHistory.IsEmpty())
        {
            auto Previous = PanelHistory.Pop();
            RestorePanel(Previous);
        }
        return true;
    }
    if (Verb == TEXT("centerCity"))
    {
        if (MapOwner && !CityContext.IsEmpty())
            MapOwner->FocusObject(CityContext, CityContext);
        return true;
    }
    if (Navigate && !View.IsEmpty() && Verb != TEXT("closeWork"))
    {
        PanelHistory.Add(CapturePanel());
        if (PanelHistory.Num() > CoIPanels::MaxHistory)
            PanelHistory.RemoveAt(0);
    }
    return false;
}
void UCoIPlayerHUD::BuildCityHeading()
{
    const auto &P = Sim->GetSnapshot().Player;
    const auto *C = Find(P.Cities, CityContext);
    if (!C)
        return;
    WorkTitle->SetText(FText::FromString(Display::Name(C->Name)));
    WorkTitle->SetToolTipText(FText::FromString(Display::Name(C->Name)));
    if (const auto *R = Find(P.Catalog.Regions, C->RegionId))
        Text(WorkBox, TEXT("Wirtschaftsregion ") + R->Name + TEXT(" · ") + R->CountryName, 12);
    int32 Stores = 0, Facilities = 0;
    for (const auto &W : P.Stores)
        if (W.Own && W.CityId == C->Id)
            ++Stores;
    for (const auto &F : P.Facilities)
        if (F.CityId == C->Id)
            ++Facilities;
    const auto *Company = Find(P.Catalog.Companies, P.ActorId);
    Text(WorkBox,
         (Company && Company->HeadquartersCityId == C->Id ? FString(TEXT("Hauptsitz"))
          : Stores || Facilities                          ? FString(TEXT("Niederlassung"))
                                                          : FString(TEXT("Keine eigene Niederlassung"))) +
             TEXT(" · ") + Display::Number(Stores) + TEXT(" Lager · ") + Display::Number(Facilities) +
             TEXT(" Betriebe"),
         12, true);
    auto *Switch = Row(WorkBox);
    TArray<TPair<FString, FString>> Hubs;
    for (const auto &X : P.Cities)
        Hubs.Add({X.Id, X.Name});
    auto *HubChoice = Choice(Switch, TEXT("hub.city"), TEXT("Stadt wechseln"), Hubs, CityContext);
    HubChoice->SetToolTipText(FText::FromString(TEXT("Anderen Stadthub im selben Panel öffnen")));
    Button(Switch, TEXT("Karte"), TEXT("centerCity"), false, TEXT("Hub in freier Kartenfläche zentrieren"));
    auto *Tabs = Row(WorkBox);
    int32 TabIndex=0;
    for (const auto &Pair : TArray<TPair<FString, FString>>{{TEXT("Übersicht"), TEXT("Übersicht")},
                                                            {TEXT("Markt"), TEXT("Markt")},
                                                            {TEXT("Assets"), TEXT("Eigene Assets")},
                                                            {TEXT("Investitionen"), TEXT("Investitionen")}})
    {
        if(TabIndex++==2)Tabs=Row(WorkBox);
        auto *B = Button(Tabs, Pair.Key, TEXT("cityTab|") + Pair.Value, CityTab == Pair.Value);
        auto Style = B->GetStyle();
        Style.NormalPadding = FMargin(6, 6);
        B->SetStyle(Style);
        CastChecked<UHorizontalBoxSlot>(B->Slot)->SetPadding(FMargin(0, 0, 4, 0));
        CastChecked<UHorizontalBoxSlot>(B->Slot)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }
    if (View == TEXT("Märkte"))
        Button(WorkBox, EntireRegion ? TEXT("Ortsfilter: gesamte Region") : TEXT("Ortsfilter: diese Stadt"),
               TEXT("regionScope"));
}

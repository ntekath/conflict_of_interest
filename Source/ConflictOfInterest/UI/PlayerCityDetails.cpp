#include "PlayerHUD.h"
#include "Simulation/SimulationSubsystem.h"
#include "Simulation/CityDetails.h"
#include "Simulation/SimArchive.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
using namespace CoISim;
namespace
{
FString Band(const FString &B)
{
    return B == TEXT("low")      ? TEXT("Niedriges Einkommen")
           : B == TEXT("middle") ? TEXT("Mittleres Einkommen")
           : B == TEXT("high")   ? TEXT("Hohes Einkommen")
                                 : TEXT("Weitere Einkommensgruppe");
}
int32 Rank(const FString &B)
{
    return B == TEXT("low") ? 0 : B == TEXT("middle") ? 1 : B == TEXT("high") ? 2 : 3;
}
} // namespace
void UCoIPlayerHUD::CityDataRow(const TArray<FString> &Values, const TArray<float> &Weights, const FString &Key,
                                bool Header, bool Own, bool SelectedRow)
{
    if(Header&&!View.IsEmpty()){Text(Body,FString::Join(Values,TEXT(" · ")),11,true);return;}
    auto *Background = WidgetTree->ConstructWidget<UBorder>();
    Background->SetPadding(FMargin(6, 4));
    Background->SetBrushColor(SelectedRow ? FLinearColor(.22f, .19f, .11f)
                              : Own       ? FLinearColor(.13f, .14f, .09f)
                                          : FLinearColor(.045f, .06f, .065f));
    Body->AddChild(Background);
    if(!View.IsEmpty()&&Values.Num()>2){
        auto* Card=WidgetTree->ConstructWidget<UVerticalBox>();Background->SetContent(Card);
        for(int32 I=0;I<Values.Num();++I){if(I==Values.Num()-1&&!Key.IsEmpty())Button(Card,Values[I],Key,SelectedRow);else Text(Card,Values[I],Header?11:12,Header||Own);}
        return;
    }
    auto *R = Row(nullptr);
    Background->SetContent(R);
    for (int32 I = 0; I < Values.Num(); ++I)
    {
        UWidget *Cell = nullptr;
        if (I == Values.Num() - 1 && !Key.IsEmpty())
            Cell = Button(R, Values[I], Key, SelectedRow);
        else
        {
            auto *T = Text(R, Values[I], Header ? 11 : 12, Header || Own);
            T->SetToolTipText(FText::FromString(Values[I]));
            Cell = T;
        }
        auto *CellSlot = CastChecked<UHorizontalBoxSlot>(Cell->Slot);
        FSlateChildSize Fill(ESlateSizeRule::Fill);
        Fill.Value = Weights.IsValidIndex(I) ? Weights[I] : 1.;
        CellSlot->SetSize(Fill);
        CellSlot->SetPadding(FMargin(5, 4));
    }
}
bool UCoIPlayerHUD::CityDetailAction(const FString &Verb, const FString &Arg)
{
    if (Verb != TEXT("cityFold") && Verb != TEXT("cityCompany") && Verb != TEXT("cityGroup") &&
        Verb != TEXT("citySort"))
        return false;
    if (!Sim || !Sim->IsReady() || View != TEXT("Stadt") || CityTab != TEXT("Übersicht"))
        return true;
    const auto *C = Find(Sim->GetSnapshot().Player.Cities, CityContext);
    if (!C)
        return true;
    auto &State = CityDetails.FindOrAdd(CityContext);
    if (Verb == TEXT("cityFold"))
    {
        if (Arg == TEXT("companies"))
            State.Companies = !State.Companies;
        if (Arg == TEXT("population"))
            State.Population = !State.Population;
    }
    if (Verb == TEXT("cityCompany") &&
        C->Presence.ContainsByPredicate([&](const FCityPresence &X) { return X.CompanyId == Arg; }))
        State.CompanyId = State.CompanyId == Arg ? TEXT("") : Arg;
    if (Verb == TEXT("cityGroup") && Find(C->Households, Arg))
        State.GroupId = State.GroupId == Arg ? TEXT("") : Arg;
    if (Verb == TEXT("citySort"))
    {
        if (Arg == TEXT("own"))
            State.OwnFirst = !State.OwnFirst;
        else if (Arg == TEXT("name"))
            State.NameDescending = !State.NameDescending;
    }
    RefreshTable();
    return true;
}
void UCoIPlayerHUD::CityCompanies(const FCityView &C)
{
    auto &State = CityDetails.FindOrAdd(C.Id);
    if (!C.Presence.ContainsByPredicate([&](const FCityPresence &X) { return X.CompanyId == State.CompanyId; }))
        State.CompanyId.Empty();
    Button(Body,
           TEXT("Unternehmen vor Ort · ") + Display::Number(C.Presence.Num()) +
               (State.Companies ? TEXT(" −") : TEXT(" +")),
           TEXT("cityFold|companies"));
    if (!State.Companies)
        return;
    if (C.Presence.IsEmpty())
    {
        Empty(Body, TEXT("Keine öffentlich sichtbare Firmenpräsenz an diesem Hub."));
        return;
    }
    auto *Sort = Body.Get();
    Button(Sort, State.NameDescending ? TEXT("Firma Z–A") : TEXT("Firma A–Z"), TEXT("citySort|name"));
    Button(Sort, State.OwnFirst ? TEXT("Eigene Firma zuerst · an") : TEXT("Eigene Firma zuerst"), TEXT("citySort|own"));
    auto Rows = C.Presence;
    Rows.Sort([&](const FCityPresence &A, const FCityPresence &B) {
        if (State.OwnFirst && A.Own != B.Own)
            return A.Own;
        const int32 Order = Display::Name(A.Name).Compare(Display::Name(B.Name), ESearchCase::IgnoreCase);
        if (Order)
            return State.NameDescending ? Order > 0 : Order < 0;
        return A.CompanyId < B.CompanyId;
    });
    const TArray<float> Widths = {2.5f, 1.3f, 2.2f, 1.3f, 1};
    CityDataRow({TEXT("Firma"), TEXT("Standortrolle"), TEXT("Tätigkeiten"), TEXT("Präsenz")}, Widths,
                TEXT(""), true);
    for (const auto &X : Rows)
    {
        const bool Active = State.CompanyId == X.CompanyId;
        CityDataRow({X.Name + (X.Own ? TEXT(" · eigene Firma") : TEXT("")),
                     X.Headquarters ? TEXT("Hauptsitz") : TEXT("Niederlassung"),
                     X.Activities.IsEmpty() ? TEXT("Keine weiteren Tätigkeiten sichtbar")
                                            : FString::Join(X.Activities, TEXT(", ")),
                     Display::Number(X.Warehouses) + TEXT(" Lagerstandorte"),
                     Active ? TEXT("Schließen") : TEXT("Details")},
                    Widths, TEXT("cityCompany|") + X.CompanyId, false, X.Own, Active);
        if (!Active)
            continue;
        Text(Body, Display::Name(X.Name) + TEXT(" · öffentliche Standortdetails"), 13, true);
        CityDataRow({TEXT("Lagerstandorte"), Display::Number(X.Warehouses)}, {1, 2});
        CityDataRow({TEXT("Produktionsbetriebe"), Display::Number(X.Facilities)}, {1, 2});
        CityDataRow({TEXT("Geschäfte"), Display::Number(X.Shops)}, {1, 2});
        CityDataRow({X.Own ? TEXT("Eigene Lkw am Hub") : TEXT("Öffentlich angebotene Lkw am Hub"),
                     Display::Number(X.PublicVehicles)},
                    {1, 2});
        auto *Explain = Text(Body, TEXT("Öffentlich sichtbare Präsenz am ausgewählten Hub"), 11);
        Explain->SetToolTipText(FText::FromString(TEXT(
            "Lagerstandorte schließen Geschäfts- und Betriebslager ein. Fahrzeuge unterwegs werden keinem Hub "
            "zugerechnet. Private Fremdfahrzeuge, Konten, Warenmengen und Einzelaufträge werden nicht offengelegt.")));
        auto *Actions = Body.Get();
        Button(Actions, TEXT("Markt öffnen"), TEXT("cityTab|Markt"));
        if (X.Own)
        {
            Button(Actions, TEXT("Eigene Assets"), TEXT("cityTab|Eigene Assets"));
            Button(Actions, TEXT("Unternehmensübersicht"), TEXT("view|Unternehmen"));
        }
    }
}
void UCoIPlayerHUD::CityHouseholds(const FCityView &C)
{
    const auto &P = Sim->GetSnapshot().Player;
    auto &State = CityDetails.FindOrAdd(C.Id);
    if (!Find(C.Households, State.GroupId))
        State.GroupId.Empty();
    Button(Body,
           TEXT("Bevölkerung und Konsum · ") + Display::Number(C.Households.Num()) + TEXT(" Gruppen") +
               (State.Population ? TEXT(" −") : TEXT(" +")),
           TEXT("cityFold|population"));
    if (!State.Population)
        return;
    if (C.Households.IsEmpty())
    {
        Empty(Body, TEXT("Diesem Hub sind keine Haushaltsgruppen zugeordnet."));
        return;
    }
    Text(Body,
         TEXT("Geldwerte je gesamter Gruppe · Einkommen als bezahlte Tagesrate · Konsum je ausgewiesener Bedarfsperiode "
              "· Ersparnisse als aktuelles Guthaben"),
         11)
        ->SetToolTipText(
            FText::FromString(TEXT("Alle Werte stammen aus dem veröffentlichten Zustand. Der Konsum umfasst "
                                   "tatsächliche Käufe; Guthaben ist weder garantiertes Einkommen noch frei "
                                   "ausgebbares Tagesbudget. Gruppendetails trennen Grund- und Luxusbedarf.")));
    if(P.Rules.Public.Enabled){const auto* Office=P.Rules.Public.Offices.FindByPredicate([&](const FPublicOffice& O){return O.CityId==C.Id;});if(Office)Text(Body,TEXT("Öffentliche Dienste: ")+Display::Number(Office->Jobs)+TEXT(" Stellen angeboten · ")+Display::Money(Office->DailyWageCents)+TEXT(" je Stelle/Tag. National gedeckt, bei Mittelmangel gekürzt. Unterstützung ist eine befristete Testszenarioregel."),11);}
    const auto &Period = C.Households[0];
    bool SamePeriod = true;
    for (const auto &Group : C.Households)
        SamePeriod &= Group.BeginHour == Period.BeginHour && Group.EndHour == Period.EndHour;
    Text(Body,
         (SamePeriod && Period.EndHour
              ? TEXT("Bedarfsperiode: ") + Time(Period.BeginHour) + TEXT(" bis ") + Time(Period.EndHour)
          : Period.EndHour ? FString(TEXT("Bedarfszeiträume je Gruppe in den Details"))
                           : FString(TEXT("Noch keine Bedarfsperiode begonnen"))) +
             TEXT(" · Datenstand ") + Time(P.Hour),
         11);
    auto Rows = C.Households;
    Rows.Sort([](const FHouseholdView &A, const FHouseholdView &B) {
        return Rank(A.Band) == Rank(B.Band) ? A.Id < B.Id : Rank(A.Band) < Rank(B.Band);
    });
    const TArray<float> Widths =
        CompactCityDetails ? TArray<float>{2, 1, 1.4f, 1} : TArray<float>{1.5f, 1.2f, 1.2f, 1.2f, 1.3f, 1.1f, 1};
    CityDataRow(
        CompactCityDetails
            ? TArray<FString>{TEXT("Einkommensgruppe"), TEXT("Personen"), TEXT("Grundversorgung"), TEXT("Auswahl")}
            : TArray<FString>{TEXT("Einkommensgruppe"), TEXT("Einkommen / Tag"), TEXT("Konsum / Periode"),
                              TEXT("Ersparnisse"), TEXT("Arbeitslosigkeit"), TEXT("Grundversorgung"), TEXT("Auswahl")},
        Widths, TEXT(""), true);
    for (const auto &G : Rows)
    {
        const bool Active = State.GroupId == G.Id;
        const FString Unemployed =
            G.Workforce ? Display::Number(G.Workforce - G.Employed) + TEXT(" / ") + Display::Number(G.Workforce)
                        : TEXT("Keine Erwerbspersonen");
        const TArray<FString> Values =
            CompactCityDetails ? TArray<FString>{Band(G.Band), Display::Number(G.Population), HouseholdSupply(G.Basic),
                                                 Active ? TEXT("Schließen") : TEXT("Details")}
                               : TArray<FString>{Band(G.Band),
                                                 Display::Money(G.IncomeCentsDay),
                                                 Display::Money(G.SpentCents),
                                                 Display::Money(G.SavingsCents),
                                                 Unemployed,
                                                 HouseholdSupply(G.Basic),
                                                 Active ? TEXT("Schließen") : TEXT("Details")};
        CityDataRow(Values, Widths, TEXT("cityGroup|") + G.Id, false, false, Active);
        if (!Active)
            continue;
        Text(Body, Band(G.Band) + TEXT(" · ") + Display::Number(G.Population) + TEXT(" Personen"), 14, true);
        Text(Body,
             G.EndHour ? TEXT("Bedarfsperiode: ") + Time(G.BeginHour) + TEXT(" bis ") + Time(G.EndHour) +
                             (P.Hour >= G.EndHour ? TEXT(" · abgeschlossen") : TEXT(" · laufend"))
                       : TEXT("Bedarfsperiode: noch nicht begonnen"),
             11)
            ->SetToolTipText(FText::FromString(
                TEXT("Periodenbeginn eingeschlossen, Ende ausgeschlossen. Die Anzeige folgt der tatsächlich zuletzt "
                     "berechneten Bedarfsperiode, auch genau an einem Tageswechsel.")));
        const FString Stamp = TEXT("Stand: ") + Time(P.Hour);
        Text(Body, Stamp, 11);
        CityDataRow({TEXT("Einkommen gesamt"), Display::Money(G.IncomeCentsDay) + TEXT(" / Tag")}, {1, 2});
        CityDataRow({TEXT("Private und Banklöhne"),Display::Money(G.PrivateIncomeCentsDay)+TEXT(" / Tag")},{1,2});
        CityDataRow({TEXT("Öffentliche Löhne"),Display::Money(G.PublicIncomeCentsDay)+TEXT(" / Tag · ")+Display::Number(G.PublicEmployed)+TEXT(" Stellen")},{1,2});
        CityDataRow({TEXT("Tatsächlich gezahlte Unterstützung"),Display::Money(G.TransferIncomeCentsDay)+TEXT(" / Tag")},{1,2});
        CityDataRow({TEXT("Tatsächlicher Konsum"), Display::Money(G.SpentCents) + TEXT(" / Bedarfsperiode")}, {1, 2});
        CityDataRow({TEXT("Konsumbudget / ungenutzter Rest"),
                     G.EndHour ? Display::Money(G.BudgetCents) + TEXT(" / ") + Display::Money(G.RemainingBudgetCents)
                               : FString(TEXT("Noch kein Periodenbudget"))},
                    {1, 2});
        CityDataRow({TEXT("Ersparnisse / davon unreserviert"),
                     Display::Money(G.SavingsCents) + TEXT(" / ") + Display::Money(G.FreeSavingsCents)},
                    {1, 2});
        CityDataRow(
            {TEXT("Arbeitslos / Erwerbspersonen"),
             Unemployed + (G.Workforce ? TEXT(" · ") + Display::Percent(double(G.Workforce - G.Employed) / G.Workforce)
                                       : TEXT(""))},
            {1, 2});
        auto Need = [&](const FString &Label, const FHouseholdNeedView &V) {
            const auto *Good = Find(P.Catalog.Goods, V.GoodId);
            CityDataRow({Label + (Good ? TEXT(" · ") + Good->Name : TEXT("")),
                         HouseholdSupply(V) + (V.Known ? TEXT(" · ") + Display::Quantity(V.UsedKg) + TEXT(" von ") +
                                                             Display::Quantity(V.WantedKg)
                                                       : TEXT(""))},
                        {1, 2});
        };
        Need(TEXT("Grundversorgung"), G.Basic);
        CityDataRow({TEXT("Freizeitversorgung"), TEXT("Nicht separat modelliert")}, {1, 2});
        Need(TEXT("Luxusversorgung"), G.Luxury);
        Text(Body,
             TEXT("Versorgung = tatsächlich konsumierte Menge / gewünschter Bedarf der Periode, vor Budgetbegrenzung. "
                  "Kein Bedarf und fehlende Daten sind getrennte Zustände."),
             11)
            ->SetToolTipText(FText::FromString(TEXT(
                "Zahlungsfähigkeit, Lieferbarkeit und das Preislimit können Käufe begrenzen. Luxus wird erst nach "
                "erfülltem Grundbedarf gekauft. Prozentwerte sind keine garantierte Versorgung des nächsten Tages.")));
        CityDataRow({TEXT("Wohlstandszuwachs gegenüber Grundniveau"),
                     Display::Percent(G.WealthPermille / 1000.) + TEXT(" · Skala 0 bis ") +
                         Display::Percent(G.MaxWealthPermille / 1000.)},
                    {1, 2});
        Button(Body, TEXT("Versorgung im Markt prüfen"), TEXT("cityTab|Markt"));
    }
}

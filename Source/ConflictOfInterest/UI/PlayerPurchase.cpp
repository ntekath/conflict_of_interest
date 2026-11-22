#include "PlayerHUD.h"
#include "Simulation/SimulationSubsystem.h"
#include "Simulation/PurchasePlan.h"
#include "Simulation/SimArchive.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
using namespace CoISim;
void UCoIPlayerHUD::OpenPurchase(const FString &OfferId)
{
    if (!Sim || !Sim->IsReady())
        return;
    const auto &P = Sim->GetSnapshot().Player;
    const auto *O = Find(P.Orders, OfferId);
    if (!O || O->Side != TEXT("sell") || O->Status != TEXT("open") || O->ActorId == P.ActorId)
    {
        LastStatus = TEXT("Verkaufsangebot nicht mehr verfügbar.");
        Refresh();
        return;
    }
    DialogKind = TEXT("purchase");
    ContextId = OfferId;
    OpenDialog(TEXT("KAUF MIT LIEFERUNG PLANEN"));
    Text(Form,
         Good(O->GoodId) + TEXT(" · ") + Store(O->StorageId) + TEXT("\nAngebot: ") + Display::Quantity(O->RemainingKg) +
             TEXT(" · ") + Display::Money(O->LimitCentsPerKg) + TEXT(" / kg"),
         13, true);
    Text(Form, TEXT("1 · Menge und Kaufbedingungen"), 14, true);
    auto *Inputs = Row(Form);
    Field(Inputs, TEXT("quantity"), TEXT("Menge bis zu (ganze kg)"), LexToString(FMath::Min(100LL, O->RemainingKg)));
    Field(Inputs, TEXT("price"), TEXT("Kaufpreislimit (USD/kg)"), LexToString(O->LimitCentsPerKg));
    TArray<TPair<FString, FString>> Targets = {{TEXT(""), TEXT("Ziellager / eigene Anlage wählen")}},
                                    Vehicles = {{TEXT(""), TEXT("Transport wählen")}};
    FString DefaultTarget, DefaultVehicle;
    for (const auto &W : P.Stores)
        if (W.Id != O->StorageId && (W.Own || W.Public || W.AcceptedGoods.Contains(O->GoodId)))
        {
            Targets.Add({W.Id, W.Name});
            if (DefaultTarget.IsEmpty() && W.Own)
                DefaultTarget = W.Id;
        }
    for (const auto &V : P.Vehicles)
    {
        const auto *T = Find(P.Catalog.VehicleTypes, V.TypeId);
        if (!T || T->TerminalOnly || V.CityId != O->CityId)
            continue;
        Vehicles.Add({V.Id, V.Name + TEXT(" · ") + Display::Quantity(V.Capacity) +
                                (V.Bound ? TEXT(" · belegt") : TEXT(" · frei"))});
        if (DefaultVehicle.IsEmpty() && !V.Bound && !V.Blocked)
            DefaultVehicle = V.Id;
        if (V.Own && !V.Bound && !V.Blocked)
            DefaultVehicle = V.Id;
    }
    Text(Form, TEXT("2 · Ziel und vorhandener Transport"), 14, true);
    Choice(Form, TEXT("form.purchase.target"), TEXT("Ziellager / Anlagenlager"), Targets, DefaultTarget);
    Choice(Form, TEXT("form.purchase.vehicle"), TEXT("Eigener oder gemieteter Transport am Quellort"), Vehicles,
           DefaultVehicle);
    Field(Form, TEXT("delay"), TEXT("Abfahrt nach dem Kaufschritt in Stunden"), LexToString(P.Rules.BookingLeadHours));
    Text(Form, TEXT("3 · Verwendung und optionale Annahmen"), 14, true);
    Choice(Form, TEXT("form.purchase.purpose"), TEXT("Verwendung"),
           {{TEXT("restock"), TEXT("Bestände auffüllen")},
            {TEXT("production"), TEXT("Produktion versorgen")},
            {TEXT("resale"), TEXT("Weiterverkauf vorbereiten")}},
           Id(TEXT("trade.purpose")).IsEmpty() ? TEXT("restock") : Id(TEXT("trade.purpose")));
    Choice(Form, TEXT("form.purchase.resale"), TEXT("Unverbindliche Weiterverkaufsschätzung"),
           {{TEXT("no"), TEXT("Keine Erlösschätzung")},
            {TEXT("yes"), TEXT("Mit eigenem Schätzpreis, ohne Verkaufsauftrag")}},
           TEXT("no"));
    auto *Optional = Row(Form);
    Field(Optional, TEXT("purchase.resale"), TEXT("Geschätzter Verkaufspreis USD/kg"), TEXT("0,00"));
    Field(Optional, TEXT("purchase.days"), TEXT("Angenommene spätere Miettage"), TEXT("1"));
    Text(Form, TEXT("4 · Kosten prüfen und ausdrücklich bestätigen"), 14, true);
    Button(Form, TEXT("Für Prüfung pausieren und aktualisieren"), TEXT("purchasePause"));
    FormPreview = Text(Form, TEXT(""), 13);
    Updating = false;
    Preview();
}
void UCoIPlayerHUD::PurchasePreview()
{
    const auto &S = Sim->GetSnapshot();
    const auto &P = S.Player;
    const auto *Offer = Find(P.Orders, ContextId);
    FString Error, Summary;
    int64 Kg = 0, Price = 0, Delay = 0, Days = 0, Resale = 0;
    FPurchaseQuote Q;
    if (!Offer || Offer->Status != TEXT("open") || Offer->Side != TEXT("sell") || Offer->ActorId == P.ActorId)
        Error = TEXT("Angebot inzwischen nicht mehr verfügbar. Aktuelle Angebote öffnen.");
    else if (!ReadPositive(TEXT("quantity"), Kg) || !ReadPositive(TEXT("price"), Price) ||
             !ReadPositive(TEXT("delay"), Delay) || !ReadPositive(TEXT("purchase.days"), Days, true) || Days > 365)
        Error = TEXT("Positive ganze kg, Abfahrt in Stunden und USD/kg mit maximal zwei Nachkommastellen eingeben. "
                     "Mietannahme 0–365 Tage.");
    else if (Id(TEXT("form.purchase.resale")) == TEXT("yes") &&
             (!ReadPositive(TEXT("purchase.resale"), Resale) || Resale > P.Rules.MaxPriceCents))
        Error = TEXT("Für die optionale Erlösschätzung einen positiven Schätzpreis eingeben.");
    else
    {
        auto &O = Draft.Order;
        O.ActorId = P.ActorId;
        O.Side = TEXT("buy");
        O.StorageId = Offer->StorageId;
        O.CityId = Offer->CityId;
        O.MarketId = Offer->MarketId;
        O.GoodId = Offer->GoodId;
        O.OriginalKg = Kg;
        O.LimitCentsPerKg = Price;
        auto &I = Draft.Purchase;
        I.Enabled = true;
        I.OfferId = Offer->Id;
        I.VehicleId = Id(TEXT("form.purchase.vehicle"));
        I.DestinationStorageId = Id(TEXT("form.purchase.target"));
        I.ApprovedHour = P.Hour;
        I.DepartureHour = P.Hour + P.StepHours + Delay;
        Draft.Kind = ECommandKind::PlaceOrder;
        Draft.TrackTrade = true;
        Draft.TradePurpose = Id(TEXT("form.purchase.purpose"));
        if (Sim->PreviewPurchase(O, I, Q, Error))
        {
            I.MaxFreightCents = Q.Transport.Cost;
            I.LatestArrivalHour = Q.Transport.Arrival;
            Summary = TEXT("Warenpreis bis ") + Display::Money(Q.GoodsCents) + TEXT(" · ") + Display::Quantity(Kg) +
                      TEXT(" zum Limit ") + Display::Money(Price) + TEXT(" / kg\nTransport einschließlich Umschlag: ") +
                      Display::Money(Q.Transport.Cost) + TEXT(" · Umschlag ") + Display::Number(Q.HandlingMinutes) +
                      TEXT(" min\nBekannte zusätzliche Quellmiete bis Abfahrt: ") + Display::Money(Q.SourceRentCents) +
                      TEXT("\nGesamtausgabe dieses Plans bis ") + Display::Money(Q.TotalCents) +
                      TEXT(" (ohne bestehende Standort-/Lohnkosten).\nAbfahrt ") + Time(Q.Transport.Departure) +
                      TEXT(" · Ankunft ") + Time(Q.Transport.Arrival) + TEXT("\nZiel: ") +
                      Store(I.DestinationStorageId) + TEXT(" · benötigt ") + Display::Number(Q.Points) +
                      TEXT(" freie Lagerpunkte.\nSpätere Miete: Annahme ") + Display::Number(Days) +
                      TEXT(" berechnete Tage nach einer etwaigen Freifrist: ") +
                      Display::Money(Q.DestinationRentDayCents * Days) +
                      TEXT(". Laufende Kosten sind zeit-/bestandsabhängig und kein garantierter Fixbetrag.");
            if (const auto *W = Find(P.Stores, I.DestinationStorageId))
                Summary += TEXT("\nBestehende Ziellager-Fixkosten: ") + Display::Money(W->DailyCost) +
                           TEXT(" / Tag für den ganzen Standort; nicht nochmals als Frachtpreis addiert.");
            if (Id(TEXT("form.purchase.resale")) == TEXT("yes"))
                Summary += TEXT("\nUnverbindliche Weiterverkaufsschätzung: Erlös ") + Display::Money(Kg * Resale) +
                           TEXT(" · geschätztes Ergebnis vor weiteren Kosten ") +
                           Display::Money(Kg * Resale - Q.TotalCents - Q.DestinationRentDayCents * Days) +
                           TEXT(". Preis, Absatz und Marge sind nicht zugesichert; es entsteht kein Verkaufsauftrag.");
            Summary +=
                TEXT("\n\nKaufauftrag zuerst, Transportprüfung im folgenden Simulationsschritt nur für tatsächlich "
                     "erworbene kg. Teilfüllungen bleiben möglich. Das Orderbuch nutzt Preis-/Zeitpriorität; kein "
                     "exklusiver Verkäufer zugesichert.\nDiese Vorschau reserviert nichts. Es gibt keine globale "
                     "atomare Kauf-/Lieferzusage. Scheitert der Transport, bleibt die gekaufte Ware am Quellort. "
                     "Weitere Teilkäufe brauchen gegebenenfalls einen neuen Transport. Ein offener Kaufrest gilt "
                     "weiter bis Storno/Ablauf. Schließen dieses Dialogs storniert keinen gesendeten Auftrag.");
        }
    }
    ValidDraft = Error.IsEmpty() && !PendingTicket;
    FormPreview->SetText(FText::FromString(Display::Name(Error.IsEmpty() ? Summary : Error)));
    FormState->SetText(FText::FromString(PendingTicket ? TEXT("Auftrag bereits gesendet; bei Pause Start drücken.")
                                                       : TEXT("Vorschau von ") + Time(P.Hour) +
                                                             TEXT(". Kauf und anschließenden Transport bestätigen.")));
    ConfirmButton->SetIsEnabled(ValidDraft);
}

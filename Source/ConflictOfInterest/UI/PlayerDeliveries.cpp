#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "PlayerHUD.h"
#include "Simulation/SimArchive.h"
#include "Simulation/SimulationSubsystem.h"
using namespace CoISim;
namespace
{
FString DeliveryNum(int64 V)
{
    return FString::Printf(TEXT("%lld"), V);
}
FString DeliveryMoney(int64 V)
{
    return Display::Money(V);
}
} // namespace
void UCoIPlayerHUD::OpenDelivery(const FString &Source)
{
    if (!Sim || !Sim->IsReady())
        return;
    const auto &P = Sim->GetSnapshot().Player;
    FString From = Source;
    if (From.IsEmpty())
        for (const auto &W : P.Stores)
            if (W.Own)
            {
                From = W.Id;
                break;
            }
    const auto *F = Find(P.Facilities, From);
    const auto *W = Find(P.Stores, F ? F->StorageId : From);
    if (W && W->Own)
    {
        CityContext = LastCityContext = W->CityId;
        CityTab = TEXT("Eigene Assets");
    }
    BuildView(TEXT("Lieferregeln"));
    DeliveryDraft = {};
    DeliveryDraft.Action = TEXT("create");
    DeliveryDraft.OwnerId = P.ActorId;
    DialogKind = TEXT("delivery");
    OpenDialog(TEXT("LIEFERBEZIEHUNG ANLEGEN"));
    TArray<TPair<FString, FString>> Assets, Goods,
        Vehicles = {{TEXT("hire"), TEXT("Bezahlter Fremdtransport · verfügbare Lkw")}};
    for (const auto &S : P.Stores)
    {
        const auto *C = Find(P.Catalog.Companies, S.OwnerId);
        if (C && !C->AccountingOnly)
            Assets.Add({S.Id, S.Name + (S.Own ? TEXT(" · eigene Firma") : TEXT(" · ") + S.OwnerName)});
    }
    for (const auto &Plant : P.Facilities)
    {
        const auto *T = Find(P.Catalog.FacilityTypes, Plant.TypeId);
        Assets.Add({Plant.Id, (T ? T->Name : TEXT("Betrieb")) + TEXT(" · ") + City(Plant.CityId) +
                                  TEXT(" · eigenes Betriebslager")});
    }
    for (const auto &G : P.Catalog.Goods)
        Goods.Add({G.Id, G.Name});
    for (const auto &V : P.Vehicles)
        if (V.Own)
            Vehicles.Add({V.Id, V.Name + TEXT(" · ") + City(V.CityId)});
    Choice(Form, TEXT("form.delivery.from"), TEXT("Quell-Asset / Stadthub"), Assets, From);
    FString To;
    for (const auto &S : P.Stores)
        if (S.Own && S.Id != (F ? F->StorageId : From))
        {
            To = S.Id;
            break;
        }
    Choice(Form, TEXT("form.delivery.to"), TEXT("Ziel-Asset / Stadthub"), Assets, To);
    Choice(Form, TEXT("form.delivery.good"), TEXT("Ware"), Goods, TEXT("good.food"));
    Choice(Form, TEXT("form.delivery.vehicle"), TEXT("Transportzuordnung · Käufer/Zielinhaber bezahlt"), Vehicles,
           TEXT("hire"));
    auto *Amounts = Row(Form);
    Field(Amounts, TEXT("delivery.kg"), TEXT("Menge je Termin kg (Teillieferung möglich)"), TEXT("50"));
    Field(Amounts, TEXT("delivery.interval"), TEXT("Intervall in Stunden"), TEXT("24"));
    auto *Dates = Row(Form);
    Field(Dates, TEXT("delivery.start"), TEXT("Beginn in Spielstunden ab Vorschau"), TEXT("1"));
    Field(Dates, TEXT("delivery.duration"), TEXT("Laufzeit ab Beginn in Stunden"), TEXT("168"));
    auto *Prices = Row(Form);
    Field(Prices, TEXT("delivery.price"), TEXT("Fester Waren-/Verrechnungspreis USD/kg"), TEXT("1"));
    Field(Prices, TEXT("delivery.freight"), TEXT("Max. Hin-/Rückfracht USD je Termin"),
          DeliveryNum(P.Rules.DeliveryDefaultFreightCapCents));
    Choice(Form, TEXT("form.delivery.fallback"),
           TEXT("Bei Ausfall: bestehende Zielbestandsregel am Markt ergänzen lassen"),
           {{TEXT("wait"), TEXT("Nächste zugesagte Menge berücksichtigen")},
            {TEXT("market"), TEXT("Marktregel darf die Bestandslücke ergänzen")}},
           TEXT("wait"));
    Text(Form,
         TEXT("Interne Lieferung: beide Assets dieselbe Firma; Verrechnung ohne Warenzahlung. Externer Partner: "
              "Zustimmung nur bei passendem sichtbarem Angebot/Gebot und tragfähigem Preis. Warenzahlung und "
              "Eigentumswechsel erfolgen bei gedeckter Buchung am Quelllager. Hin- und Rückfahrt nutzen reale "
              "Fahrzeuge; Kosten werden vorab gedeckt. Kein Strafgeld und kein späteres Aufstauen ausgefallener "
              "Mengen. Aktive Zielbestände, offene Käufe und ankommende Lieferungen begrenzen Zusatzbeschaffung. Mit "
              "eigenem Lkw zuerst regulär zum Quellhub fahren."),
         12);
    FormPreview = Text(Form, TEXT(""), 12);
    Updating = false;
    Preview();
}
bool UCoIPlayerHUD::ReadDeliveryForm(FString &Error)
{
    const auto &P = Sim->GetSnapshot().Player;
    auto &R = DeliveryDraft.Terms;
    R.SourceAssetId = Id(TEXT("form.delivery.from"));
    R.DestinationAssetId = Id(TEXT("form.delivery.to"));
    R.GoodId = Id(TEXT("form.delivery.good"));
    R.VehicleId = Id(TEXT("form.delivery.vehicle"));
    if (R.VehicleId == TEXT("hire"))
        R.VehicleId.Empty();
    auto StoreId = [&](const FString &A) {
        const auto *F = Find(P.Facilities, A);
        return F ? F->StorageId : A;
    };
    const auto *From = Find(P.Stores, StoreId(R.SourceAssetId));
    const auto *To = Find(P.Stores, StoreId(R.DestinationAssetId));
    if (!From || !To)
    {
        Error = TEXT("Quell- und Ziel-Asset wählen.");
        return false;
    }
    R.FromCityId = From->CityId;
    R.ToCityId = To->CityId;
    int64 Delay = 0, Duration = 0;
    if (!ReadPositive(TEXT("delivery.kg"), R.Kg) || !ReadPositive(TEXT("delivery.interval"), R.IntervalHours) ||
        !ReadPositive(TEXT("delivery.start"), Delay, true) ||
        !ReadPositive(TEXT("delivery.duration"), Duration, true) ||
        !ReadPositive(TEXT("delivery.price"), R.PriceCentsPerKg, true) ||
        !ReadPositive(TEXT("delivery.freight"), R.MaxFreightCents, true) || Delay > P.Rules.DeliveryMaxDurationHours ||
        Duration > P.Rules.DeliveryMaxDurationHours)
    {
        Error = TEXT("Gültige ganze kg/Stunden und USD mit maximal zwei Nachkommastellen innerhalb der Szenariogrenzen eingeben.");
        return false;
    }
    R.StartHour = P.Hour + Delay;
    R.EndHour = R.StartHour + Duration;
    R.MarketFallback = Id(TEXT("form.delivery.fallback")) == TEXT("market");
    return true;
}
void UCoIPlayerHUD::OpenDeliveryStop(const FString &IdValue)
{
    if (!Sim)
        return;
    DeliveryDraft = {};
    DeliveryDraft.OwnerId = Sim->GetSnapshot().Player.ActorId;
    DeliveryDraft.Action = TEXT("stop");
    DeliveryDraft.Id = IdValue;
    DialogKind = TEXT("deliveryStop");
    OpenDialog(TEXT("LIEFERREGEL STOPPEN"));
    Text(Form,
         TEXT("Künftige Fälligkeiten stoppen. Bereits bezahlte Käufe und gebuchte Hin-/Rückfahrten bleiben verbindlich "
              "und werden regulär abgeschlossen."),
         14);
    FormPreview = Text(Form, TEXT(""), 12);
    Updating = false;
    Preview();
}
void UCoIPlayerHUD::RefreshDeliveries()
{
    const auto &P = Sim->GetSnapshot().Player;
    auto *A = Row(Body);
    Button(A, TEXT("Neue Lieferbeziehung …"), TEXT("deliveryNew"), true);
    Button(A, TEXT("Standortberichte"), TEXT("siteReports"));
    Text(Body,
         TEXT("Ein Versuch je Termin; keine Nachholschlange. Bereits gebuchte Transporte und Rückfahrten laufen nach "
              "Stopp/Ende weiter. Fehlmengen und Ursachen bleiben sichtbar."),
         12);
    int32 Count = 0;
    for (const auto &C : P.Deliveries)
    {
        if (!CityContext.IsEmpty() && CityContext != C.Terms.FromCityId && CityContext != C.Terms.ToCityId)
            continue;
        ++Count;
        Text(Body,
             City(C.Terms.FromCityId) + TEXT(" → ") + City(C.Terms.ToCityId) + TEXT(" · ") + Good(C.Terms.GoodId) +
                 (C.SupplierId == C.BuyerId ? TEXT(" · intern") : TEXT(" · externer Vertrag")),
             14, true);
        Cells(Body,
              {C.Status == TEXT("active")    ? TEXT("Aktiv")
               : C.Status == TEXT("stopped") ? TEXT("Gestoppt")
                                             : TEXT("Ausgelaufen"),
               DeliveryNum(C.Terms.Kg) + TEXT(" kg / ") + DeliveryNum(C.Terms.IntervalHours) + TEXT(" h"),
               C.Status == TEXT("active") ? Time(C.NextDueHour) : TEXT("Keine neue Fälligkeit"),
               DeliveryMoney(C.Terms.PriceCentsPerKg) + TEXT(" / kg")},
              {1, 1, 1.6f, 1}, false, C.Status == TEXT("active") ? TEXT("Stoppen …") : TEXT(""),
              TEXT("deliveryStop|") + C.Id);
        Text(Body,
             TEXT("Quelle ") + Store(C.SourceStoreId) + TEXT(" · Ziel ") + Store(C.DestinationStoreId) +
                 TEXT("\nZugesagt ") + DeliveryNum(C.RequestedKg) + TEXT(" kg · gebucht ") + DeliveryNum(C.BookedKg) +
                 TEXT(" kg · zugestellt ") + DeliveryNum(C.DeliveredKg) + TEXT(" kg · ausgefallen/verdorben ") +
                 DeliveryNum(C.MissedKg) + TEXT(" kg\n") + C.Cause,
             12);
        for (int32 I = P.DeliverySlots.Num() - 1, Shown = 0; I >= 0 && Shown < 6; --I)
        {
            const auto &X = P.DeliverySlots[I];
            if (X.ContractId != C.Id)
                continue;
            ++Shown;
            Text(Body,
                 Time(X.DueHour) + TEXT(" · ") + DeliveryNum(X.BookedKg) + TEXT("/") + DeliveryNum(X.RequestedKg) +
                     TEXT(" kg gebucht, ") + DeliveryNum(X.DeliveredKg) + TEXT(" kg angekommen · ") + X.Cause,
                 11);
        }
    }
    if (!Count)
        Text(Body,
             TEXT("Keine eigenen Lieferbeziehungen in dieser Auswahl. Mindestens ein beteiligtes Asset muss Ihnen "
                  "gehören; für interne Versorgung werden zwei eigene physische Lager benötigt."),
             12);
}
void UCoIPlayerHUD::RefreshSiteReports()
{
    const auto &P = Sim->GetSnapshot().Player;
    Text(Body, TEXT("STANDORTE · echte Zahlungen und getrennte interne Bewertung"), 15, true);
    Text(Body,
         TEXT("Externe Erlöse/Kosten sind gebuchte Geldflüsse seit Partiebeginn. Interne Gut-/Lastschriften sind "
              "paarige Bewertungsbuchungen und verändern kein Firmenbargeld. Lagerbuchwert ist unrealisiert auf "
              "Kostenbasis, kein Gewinn und kein interner Aufschlag."),
         12);
    int64 Revenue = 0, Cost = 0, Credit = 0, Debit = 0, Book = 0;
    for (const auto &I : P.Stocks)
        Book += I.BookValueCents;
    Cells(Body,
          {TEXT("Standort"), TEXT("Externe Erlöse"), TEXT("Externe Kosten"), TEXT("Intern gut / Last"),
           TEXT("Lagerbuchwert")},
          {1.6f, 1, 1, 1.3f, 1}, true);
    for (const auto &L : P.SiteLedgers)
    {
        Revenue += L.ExternalRevenueCents;
        Cost += L.ExternalCostCents;
        Credit += L.InternalCreditCents;
        Debit += L.InternalDebitCents;
        int64 SiteBook = 0;
        for (const auto &I : P.Stocks)
            if (I.StorageId == L.SiteId)
                SiteBook += I.BookValueCents;
        const auto *V = Find(P.Vehicles, L.SiteId);
        FString Name = L.SiteId.IsEmpty() ? TEXT("Unternehmensgemeinkosten")
                       : V                ? V->Name + TEXT(" · Flotte")
                                          : Store(L.SiteId);
        Cells(Body,
              {Name, DeliveryMoney(L.ExternalRevenueCents), DeliveryMoney(L.ExternalCostCents),
               DeliveryMoney(L.InternalCreditCents) + TEXT(" / ") + DeliveryMoney(L.InternalDebitCents),
               DeliveryMoney(SiteBook)},
              {1.6f, 1, 1, 1.3f, 1});
        if (!L.SiteId.IsEmpty())
            Text(Body,
                 TEXT("Standort-Verrechnungssaldo (kein Konzerngewinn): ") +
                     DeliveryMoney(L.ExternalRevenueCents - L.ExternalCostCents + L.InternalCreditCents -
                                   L.InternalDebitCents),
                 11);
    }
    Text(Body,
         TEXT("Unternehmen: externe Erlöse ") + DeliveryMoney(Revenue) + TEXT(" · externe Kosten ") +
             DeliveryMoney(Cost) + TEXT(" · reiner Zahlungssaldo ") + DeliveryMoney(Revenue - Cost),
         13, true);
    Text(Body,
         TEXT("Interne Gutschriften ") + DeliveryMoney(Credit) + TEXT(" = interne Lastschriften ") +
             DeliveryMoney(Debit) + TEXT(" · gesamte Lager-/Frachtbuchwerte ") + DeliveryMoney(Book),
         12);
    if (P.SiteLedgers.IsEmpty())
        Text(Body,
             TEXT("Noch keine gebuchten Zahlungen oder internen Lieferbewertungen. Anfangskapital ist kein Umsatz."),
             12);
    Button(Body, TEXT("Zu Lieferregeln"), TEXT("deliveryList"));
}

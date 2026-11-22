#include "PlayerHUD.h"
#include "Simulation/SimulationSubsystem.h"
#include "Simulation/SimArchive.h"
#include "Simulation/TradeBusiness.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
using namespace CoISim;
namespace
{
FString Purpose(const FString &Id)
{
    return Id == TEXT("production") ? TEXT("Produktion versorgen")
           : Id == TEXT("resale")   ? TEXT("Weiterverkauf vorbereiten")
                                    : TEXT("Bestände auffüllen");
}
bool PublicOrder(const FPlayerView &P, const FMarketOrder *O)
{
    return O && O->Status == TEXT("open") && O->ActorId != P.ActorId && O->RemainingKg > 0;
}
} // namespace
void UCoIPlayerHUD::BuildTrading()
{
    const auto &P = Sim->GetSnapshot().Player;
    Selected.Add(TEXT("place"),
                 TEXT("")); // physical source is chosen explicitly in the order, not a hidden inherited storage filter.
    const FString Tab = Id(TEXT("trade.tab")).IsEmpty() ? TEXT("offers") : Id(TEXT("trade.tab"));
    Selected.Add(TEXT("trade.tab"), Tab);
    Button(WorkBox, TEXT("Angebote"), TEXT("tradeTab|offers"), Tab == TEXT("offers"));
    Button(WorkBox, TEXT("Handelsplanung"), TEXT("tradeTab|plan"), Tab == TEXT("plan"));
    Button(WorkBox, TEXT("Laufende Geschäfte"), TEXT("tradeTab|running"), Tab == TEXT("running"));
    TradingMinimum = nullptr;
    TradingFilterToggle = nullptr;
    TradingFilterSummary = nullptr;
    if (Tab == TEXT("plan"))
        return;
    const auto *Region = Find(P.Catalog.Regions, Id(TEXT("region")));
    TradingFilterToggle =
        Button(WorkBox,
               TEXT("Filter · ") + (Id(TEXT("good")).IsEmpty() ? TEXT("alle Waren") : Good(Id(TEXT("good")))) +
                   TEXT(" · ") + (Region ? Region->Name : TEXT("alle Regionen")),
               TEXT("tradeFilters"), Id(TEXT("trade.filters")) == TEXT("open"),
               TEXT("Region/Hub, Ware, Anbieter und Mindestmenge ändern"));
    const auto *Provider = Find(P.Catalog.Companies, Id(TEXT("trade.provider")));
    if (Tab == TEXT("offers"))
        Text(WorkBox,
             (Provider ? Provider->Name : TEXT("Alle Anbieter")) + FString(TEXT(" · ab ")) +
                 (Id(TEXT("trade.minimum")).IsEmpty() ? TEXT("0") : Id(TEXT("trade.minimum"))) + TEXT(" kg · ") +
                 (Id(TEXT("trade.side")) == TEXT("buy") ? TEXT("Kaufgebote") : TEXT("Verkaufsangebote")),
             11);
    if (Id(TEXT("trade.filters")) != TEXT("open"))
        return;
    TArray<TPair<FString, FString>> Regions = {{TEXT(""), TEXT("Alle Regionen / Hubs")}},
                                    Goods = {{TEXT(""), TEXT("Alle Waren")}},
                                    Providers = {{TEXT(""), TEXT("Alle Anbieter / Käufer")}};
    for (const auto &R : P.Catalog.Regions)
    {
        if (!CityContext.IsEmpty() && R.Id != Find(P.Cities, CityContext)->RegionId)
            continue;
        const auto *Hub = P.Catalog.Places.FindByPredicate([&](const FPlaceDef &X) { return X.RegionId == R.Id; });
        Regions.Add({R.Id, R.Name + (Hub ? TEXT(" · ") + Hub->Name : TEXT(""))});
    }
    for (const auto &G : P.Catalog.Goods)
        Goods.Add({G.Id, G.Name});
    TSet<FString> Seen;
    for (const auto &O : P.Orders)
        if (PublicOrder(P, &O) && !Seen.Contains(O.ActorId))
        {
            if (const auto *C = Find(P.Catalog.Companies, O.ActorId))
            {
                Providers.Add({C->Id, C->Name});
                Seen.Add(C->Id);
            }
        }
    if (!Id(TEXT("trade.provider")).IsEmpty() && !Seen.Contains(Id(TEXT("trade.provider"))))
        if (const auto *Company = Find(P.Catalog.Companies, Id(TEXT("trade.provider"))))
            Providers.Add({Company->Id, Company->Name});
    Choice(WorkBox, TEXT("region"), TEXT("Region / Hub"), Regions, Id(TEXT("region")));
    Choice(WorkBox, TEXT("good"), TEXT("Ware"), Goods, Id(TEXT("good")));
    if (Tab == TEXT("offers"))
    {
        Choice(WorkBox, TEXT("trade.provider"), TEXT("Anbieter / Käufer"), Providers, Id(TEXT("trade.provider")));
        Choice(WorkBox, TEXT("trade.side"), TEXT("Orderbuchseite"),
               {{TEXT("sell"), TEXT("Verkaufsangebote")}, {TEXT("buy"), TEXT("Kaufgebote")}},
               Id(TEXT("trade.side")).IsEmpty() ? TEXT("sell") : Id(TEXT("trade.side")));
        Field(WorkBox, TEXT("trade.minimum"), TEXT("Mindestens verfügbare kg · 0 = alle"),
              Id(TEXT("trade.minimum")).IsEmpty() ? TEXT("0") : Id(TEXT("trade.minimum")));
        TradingMinimum = Fields[TEXT("trade.minimum")];
    }
}
void UCoIPlayerHUD::TradingFilterChanged()
{
    if (TradingMinimum)
        Selected.Add(TEXT("trade.minimum"), TradingMinimum->GetText().ToString());
    RefreshTable();
}
bool UCoIPlayerHUD::TradingAction(const FString &Verb, const FString &Arg)
{
    if (Verb == TEXT("tradeFilters"))
    {
        if (!DialogKind.IsEmpty())
            return true;
        Selected.Add(TEXT("trade.filters"), Id(TEXT("trade.filters")) == TEXT("open") ? TEXT("closed") : TEXT("open"));
        BuildView(TEXT("Märkte"));
        return true;
    }
    if (Verb == TEXT("tradeTab"))
    {
        if (!DialogKind.IsEmpty())
            return true;
        if (Arg == TEXT("offers") || Arg == TEXT("plan") || Arg == TEXT("running"))
        {
            Selected.Add(TEXT("trade.tab"), Arg);
            BuildView(TEXT("Märkte"));
        }
        return true;
    }
    if (Verb == TEXT("tradePlan"))
    {
        if (!Sim || !DialogKind.IsEmpty())
            return true;
        const auto &P = Sim->GetSnapshot().Player;
        Selected.Add(TEXT("trade.offer"), Arg);
        Selected.Add(TEXT("trade.tab"), TEXT("plan"));
        BuildView(TEXT("Märkte"));
        return true;
    }
    if(Verb==TEXT("purchasePlan")){if(Sim&&DialogKind.IsEmpty())OpenPurchase(Arg);return true;}
    if(Verb==TEXT("purchasePause")){if(Sim&&DialogKind==TEXT("purchase")){if(!Sim->IsPaused())Sim->TogglePause();Preview();}return true;}
    if (Verb == TEXT("tradeQuote"))
    {
        if (!Sim || !DialogKind.IsEmpty())
            return true;
        const auto &P = Sim->GetSnapshot().Player;
        const auto *O = Find(P.Orders, Id(TEXT("trade.offer")));
        if (!PublicOrder(P, O))
        {
            LastStatus = TEXT("Angebot nicht mehr verfügbar. Aktuelles Orderbuch erneut prüfen.");
            Refresh();
            RefreshTable();
            return true;
        }
        OpenTrade(O->Side == TEXT("sell"), O->Id);
        int64 Wanted = 100;
        const auto Min = Id(TEXT("trade.minimum"));
        LexTryParseString(Wanted, *Min);
        if (Wanted == 0)
            Wanted = 100;
        Wanted = FMath::Clamp(Wanted, 1LL, O->RemainingKg);
        if (auto *Quantity = Fields.FindRef(TEXT("quantity")).Get())
            Quantity->SetText(FText::FromString(LexToString(Wanted)));
        Preview();
        return true;
    }
    if (Verb == TEXT("businessHaul"))
    {
        if (!Sim || !DialogKind.IsEmpty())
            return true;
        const auto &P = Sim->GetSnapshot().Player;
        const auto *B = Find(P.Businesses, Arg);
        if (!B || B->OwnerId != P.ActorId || B->Side != TEXT("buy"))
            return true;
        const auto *Stock = P.Stocks.FindByPredicate([&](const FInventory &I) {
            return I.StorageId == B->StorageId && I.GoodId == B->GoodId && I.QuantityKg > I.ReservedKg;
        });
        const int64 Remaining = B->FilledKg - BusinessAssignedKg(*B);
        if (!Stock || Remaining <= 0)
        {
            LastStatus = TEXT("Keine passende freie Ware am Quelllager. Bestände können inzwischen verbraucht, "
                              "verkauft, reserviert oder verdorben sein.");
            Refresh();
            return true;
        }
        OpenTransport(Stock->Id,false,B->Purchase.Enabled?B->Purchase.DestinationStorageId:FString());
        ActiveTradeCase = B->Id;
        Fields[TEXT("quantity")]->SetText(
            FText::FromString(LexToString(FMath::Min(Remaining, Stock->QuantityKg - Stock->ReservedKg))));
        Preview();
        return true;
    }
    return false;
}
void UCoIPlayerHUD::RefreshTrading()
{
    const auto &S = Sim->GetSnapshot();
    const auto &P = S.Player;
    const FString Tab = Id(TEXT("trade.tab"));
    if (TradingFilterToggle)
        if (auto *Label = Cast<UTextBlock>(TradingFilterToggle->GetContent()))
        {
            const auto *R = Find(P.Catalog.Regions, Id(TEXT("region")));
            Label->SetText(FText::FromString(Display::Name(
                TEXT("Filter · ") + (Id(TEXT("good")).IsEmpty() ? TEXT("alle Waren") : Good(Id(TEXT("good")))) +
                TEXT(" · ") + (R ? R->Name : TEXT("alle Regionen")))));
        }
    if (TradingFilterSummary)
    {
        const auto *Provider = Find(P.Catalog.Companies, Id(TEXT("trade.provider")));
        TradingFilterSummary->SetText(FText::FromString(Display::Name(
            (Provider ? Provider->Name : TEXT("Alle Anbieter")) + FString(TEXT(" · ab ")) +
            (Id(TEXT("trade.minimum")).IsEmpty() ? TEXT("0") : Id(TEXT("trade.minimum"))) + TEXT(" kg · ") +
            (Id(TEXT("trade.side")) == TEXT("buy") ? TEXT("Kaufgebote") : TEXT("Verkaufsangebote")))));
    }

    Text(Body, TEXT("Datenstand: ") + Time(P.Hour) + TEXT(" · veröffentlichter Schritt ") + Display::Number(S.Step),
         11);
    if (Tab == TEXT("plan"))
    {
        Text(Body, TEXT("HANDELSPLANUNG"), 14, true);
        const auto *O = Find(P.Orders, Id(TEXT("trade.offer")));
        if (!PublicOrder(P, O))
        {
            Empty(Body, TEXT("Kein aktuelles Angebot ausgewählt oder Angebot nicht mehr verfügbar."));
            Button(Body, TEXT("Aktuelle Angebote öffnen"), TEXT("tradeTab|offers"));
            return;
        }
        const auto *C = Find(P.Catalog.Companies, O->ActorId);
        Text(Body,
             (O->Side == TEXT("sell") ? TEXT("Kauf vorbereiten · ") : TEXT("Verkauf vorbereiten · ")) + Good(O->GoodId),
             14, true);
        Text(Body, TEXT("Anbieter / Käufer: ") + (C ? C->Name : TEXT("Nicht verfügbar")));
        Text(Body, TEXT("Tatsächlicher Quell-/Handelsort: ") + Store(O->StorageId));
        Text(Body, TEXT("Preislimit: ") + Display::Money(O->LimitCentsPerKg) + TEXT(" / kg · offen: ") +
                       Display::Quantity(O->RemainingKg));
        Choice(Body, TEXT("trade.purpose"), TEXT("Verwendung · kein Weiterverkauf erforderlich"),
               {{TEXT("production"), TEXT("Eigene Produktion versorgen")},
                {TEXT("restock"), TEXT("Bestände auffüllen")},
                {TEXT("resale"), TEXT("Weiterverkauf vorbereiten")}},
               Id(TEXT("trade.purpose")).IsEmpty() ? TEXT("restock") : Id(TEXT("trade.purpose")));
        Text(Body,
             TEXT("Eigentumswechsel findet am angegebenen Lager statt. Kauf mit Lieferung plant danach ausschließlich "
                  "tatsächlich erworbene Ware ein. Ein reiner Limitauftrag bleibt ebenfalls möglich."),
             12);
        Text(Body,
             TEXT("Offenes Orderbuch: Preis-/Zeitpriorität. Der angezeigte Anbieter ist kein exklusiv zugesicherter "
                  "Vertragspartner. Teilfüllungen sind möglich."),
             11);
        if(O->Side==TEXT("sell"))Button(Body,TEXT("Kauf mit Lieferung planen …"),TEXT("purchasePlan|")+O->Id,true);
        Button(Body, TEXT("Limitauftrag prüfen …"), TEXT("tradeQuote"), true);
        return;
    }
    auto Location = [&](const FString &StoreId) {
        const auto *W = Find(P.Stores, StoreId);
        const auto *C = W ? Find(P.Catalog.Places, W->CityId) : nullptr;
        return W && CityMarketLocation(P, StoreId, CityContext, EntireRegion) &&
               (Id(TEXT("region")).IsEmpty() || (C && C->RegionId == Id(TEXT("region")))) &&
               (Id(TEXT("place")).IsEmpty() || Id(TEXT("place")) == StoreId);
    };
    auto GoodMatches = [&](const FString &G) { return Id(TEXT("good")).IsEmpty() || Id(TEXT("good")) == G; };
    if (Tab == TEXT("running"))
    {
        Text(Body, TEXT("LAUFENDE GESCHÄFTE UND VERLAUF"), 14, true);
        int32 Count = 0;
        for (int32 Index = P.Businesses.Num() - 1; Index >= 0; --Index)
        {
            const auto &B = P.Businesses[Index];
            if (!GoodMatches(B.GoodId) || !Location(B.StorageId))
                continue;
            ++Count;
            const auto *W = Find(P.Stores, B.StorageId);
            Text(Body,
                 TEXT("Vorgang ") + B.OrderId.RightChop(FString(TEXT("player.order.")).Len()) + TEXT(" · ") +
                     Good(B.GoodId),
                 14, true);
            Text(Body, BusinessPhase(B, W && W->Own), 13, true);
            Text(Body, Purpose(B.Purpose) + TEXT(" · ") + Store(B.StorageId), 12);
            Text(Body, (B.Side == TEXT("buy") ? TEXT("Gekauft: ") : TEXT("Verkauft: ")) +
                           Display::Quantity(B.FilledKg) + TEXT(" / beauftragt ") + Display::Quantity(B.RequestedKg) +
                           TEXT(" · gebucht ") + Display::Money(B.SettledCents));
            Text(Body,
                 TEXT("Marktauftrag: ") + Status(B.OrderStatus) + TEXT(" · Restmenge ") +
                     Display::Quantity(B.RequestedKg - B.FilledKg),
                 12);
            if (!B.LastReason.IsEmpty())
                Text(Body, TEXT("Letzter Fehler: ") + B.LastReason, 12);
            const auto *Order = Find(P.Orders, B.OrderId);
            if (Order && Order->Status == TEXT("open"))
                Button(Body, TEXT("Restauftrag stornieren …"), TEXT("cancelOrder|") + B.OrderId);
            if (B.Side == TEXT("buy") && B.FilledKg)
            {
                const auto *Stock = P.Stocks.FindByPredicate(
                    [&](const FInventory &I) { return I.StorageId == B.StorageId && I.GoodId == B.GoodId; });
                const int64 Free = Stock ? Stock->QuantityKg - Stock->ReservedKg : 0;
                Text(Body,
                     TEXT("Noch keinem Transport zugeordnet: ") +
                         Display::Quantity(B.FilledKg - BusinessAssignedKg(B)) +
                         TEXT(" · tatsächlich frei am Quellort: ") + Display::Quantity(Free),
                     12);
                if (Free > 0 && B.FilledKg > BusinessAssignedKg(B))
                    Button(Body, B.Purchase.Enabled?TEXT("Transport erneut planen …"):TEXT("Transport dieser Kaufmenge planen …"), TEXT("businessHaul|") + B.Id);
                else if (B.FilledKg > BusinessAssignedKg(B))
                    Empty(Body, TEXT("Kein freier passender Abholbestand. Verbrauch, Verkauf, Verderb und "
                                     "Reservierungen prüfen."));
            }
            for (const auto &L : B.Legs)
            {
                Text(Body,
                     TEXT("Transport · ") + Status(L.Status) + TEXT(" · gebucht ") + Display::Quantity(L.Kg) +
                         TEXT(" · angekommen ") + Display::Quantity(L.DeliveredKg) + TEXT(" · Ziel ") +
                         Store(L.DestinationStorageId),
                     12);
                if (L.Status == TEXT("arrived") && L.DeliveredKg < L.Kg)
                    Text(Body,
                         TEXT("Teilmenge angekommen: Verluste während der Fahrt sind in der tatsächlichen "
                              "Ankunftsmenge enthalten."),
                         11);
                Button(Body, TEXT("Transport auf Karte zeigen"), TEXT("centerJob|") + L.Id);
                if (L.Status == TEXT("planned"))
                    Button(Body, TEXT("Transport stornieren …"), TEXT("cancelHaul|") + L.Id);
            }
        }
        if (!Count)
            Empty(Body, TEXT("Noch keine Handelsvorgänge für diese Auswahl. Ein bestätigter manueller Marktauftrag "
                             "wird genau einmal erfasst."));
        Text(Body, TEXT("WEITERE EIGENE MARKTAUFTRÄGE"), 14, true);
        int32 Other = 0;
        for (const auto &O : P.Orders)
            if (O.ActorId == P.ActorId && GoodMatches(O.GoodId) && Location(O.StorageId) &&
                !P.Businesses.ContainsByPredicate([&](const FTradeBusiness &B) { return B.OrderId == O.Id; }))
            {
                ++Other;
                Text(Body, Good(O.GoodId) + TEXT(" · ") + Store(O.StorageId) + TEXT(" · ") + Status(O.Status) +
                               TEXT(" · Rest ") + Display::Quantity(O.RemainingKg));
                if (O.Status == TEXT("open"))
                    Button(Body, TEXT("Restauftrag stornieren …"), TEXT("cancelOrder|") + O.Id);
            }
        if (!Other)
            Empty(Body, TEXT("Keine weiteren eigenen Aufträge. Betriebs- und Lieferregeln bleiben eigenständige "
                             "Automatisierungen."));
        return;
    }
    Text(Body, TEXT("ÖFFENTLICHE B2B-ANGEBOTE"), 14, true);
    int64 Minimum = 0;
    const FString Raw = Id(TEXT("trade.minimum"));
    if (!Raw.IsEmpty())
    {
        bool Digits = true;
        for (TCHAR C : Raw)
            Digits &= C >= TEXT('0') && C <= TEXT('9');
        if (!Digits || Raw.Len() > 9 || !LexTryParseString(Minimum, *Raw) || Minimum < 0 ||
            Minimum > P.Rules.MaxOrderKg)
        {
            Empty(Body, TEXT("Mengenfilter: ganze kg zwischen 0 und ") + Display::Number(P.Rules.MaxOrderKg) +
                            TEXT(" eingeben."));
            return;
        }
    }
    int32 Count = 0;
    for (const auto &O : P.Orders)
        if (PublicOrder(P, &O) &&
            O.Side == (Id(TEXT("trade.side")).IsEmpty() ? TEXT("sell") : Id(TEXT("trade.side"))) &&
            GoodMatches(O.GoodId) && Location(O.StorageId) &&
            (Id(TEXT("trade.provider")).IsEmpty() || O.ActorId == Id(TEXT("trade.provider"))) &&
            O.RemainingKg >= Minimum)
        {
            ++Count;
            const auto *C = Find(P.Catalog.Companies, O.ActorId);
            Text(Body,
                 Good(O.GoodId) + TEXT(" · ") + (O.Side == TEXT("sell") ? TEXT("Verkaufsangebot") : TEXT("Kaufgebot")),
                 14, true);
            Text(Body, TEXT("Limit ") + Display::Money(O.LimitCentsPerKg) + TEXT(" / kg · verfügbar ") +
                           Display::Quantity(O.RemainingKg));
            Text(Body, TEXT("Physischer Ort: ") + Store(O.StorageId));
            Text(Body, TEXT("Anbieter / Käufer: ") + (C ? C->Name : TEXT("Nicht verfügbar")), 12);
            Text(Body, TEXT("Offen bis ") + Time(O.ExpiresStep) + TEXT(" · Ausführung prüft aktuellen Zustand"), 11);
            Button(Body, TEXT("Handel planen"), TEXT("tradePlan|") + O.Id, true);
        }
    if (!Count)
        Empty(Body,
              TEXT("Keine aktuellen Angebote für diese Filter. Region, Ware, Anbieter oder Mindestmenge ändern."));
    Button(Body, TEXT("Eigenen Kaufauftrag prüfen …"), TEXT("buy"));
    Button(Body, TEXT("Eigenen Verkaufsauftrag prüfen …"), TEXT("sell"));
    Text(Body,
         TEXT("Haushaltskäufe im Laden bleiben Einzelhandel. Dieses Menü bedient B2B-Regionsmärkte; Preis, Lagerort "
              "und Ware werden nicht global zusammengelegt."),
         11);
}

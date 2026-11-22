#include "PurchasePlan.h"
#include "SimCore.h"
#include "SimArchive.h"
#include "TradeBusiness.h"
#include "WaterTransport.h"
namespace CoISim
{
bool QuotePurchase(const FPlayerView &P, int64 Cash, const FMarketOrder &O, const FPurchaseIntent &I, FPurchaseQuote &Q,
                   FString &E)
{
    Q = {};
    auto Fail = [&](const FString &M) {
        E = M;
        return false;
    };
    const auto *Offer = Find(P.Orders, I.OfferId);
    const auto *From = Find(P.Stores, O.StorageId);
    const auto *To = Find(P.Stores, I.DestinationStorageId);
    const auto *V = Find(P.Vehicles, I.VehicleId);
    const auto *G = Find(P.Catalog.Goods, O.GoodId);
    if (!I.Enabled || !Offer || Offer->Status != TEXT("open") || Offer->Side != TEXT("sell") ||
        Offer->ActorId == P.ActorId || Offer->RemainingKg < 1 || Offer->StorageId != O.StorageId ||
        Offer->GoodId != O.GoodId || Offer->CityId != O.CityId || Offer->MarketId != O.MarketId ||
        O.ActorId != P.ActorId || O.Side != TEXT("buy"))
        return Fail(TEXT("Kein aktuelles Verkaufsangebot am bestätigten Quellort. Keine Fracht beauftragt."));
    if (!From || !To || !G || !V || O.OriginalKg < 1 || O.OriginalKg > P.Rules.MaxOrderKg ||
        O.LimitCentsPerKg < Offer->LimitCentsPerKg || O.LimitCentsPerKg > P.Rules.MaxPriceCents)
        return Fail(TEXT("Menge, Preislimit, Ziellager oder Transportangebot ungültig. Vorschau aktualisieren."));
    if (From->Id == To->Id)
        return Fail(TEXT("Quelle und Ziel sind identisch. Dafür genügt ein gewöhnlicher Kauf ohne Lieferung."));
    if (From->Blocked || To->Blocked)
        return Fail(TEXT("Offene Lagergebühren sperren den Plan."));
    if (!To->Own && !To->Public && !To->AcceptedGoods.Contains(O.GoodId))
        return Fail(TEXT("Kein Ziellagerzugang für diese Ware."));
    const auto *Type = Find(P.Catalog.VehicleTypes, V->TypeId);
    if (!Type || Type->TerminalOnly || V->Bound || V->Blocked || V->CityId != From->CityId ||
        O.OriginalKg > V->Capacity)
        return Fail(
            TEXT("Fahrzeug belegt, gesperrt, zu klein oder nicht am Quellort. Kein Kauf durch diese Vorschau."));
    const auto *Old = P.Stocks.FindByPredicate(
        [&](const FInventory &X) { return X.StorageId == O.StorageId && X.GoodId == O.GoodId; });
    if (Old && Old->RentArrearsCents)
        return Fail(TEXT("Offene Miete am Quellbestand; erst begleichen."));
    if (G->StoragePointsPerKg < 1 || O.OriginalKg > 1000000000000LL / G->StoragePointsPerKg)
        return Fail(TEXT("Benötigte Zielkapazität über dem Planlimit."));
    Q.Points = O.OriginalKg * G->StoragePointsPerKg;
    if (Q.Points > To->Capacity - To->Used - To->Incoming)
        return Fail(TEXT("Ziellager voll oder Zielkapazität inzwischen gebunden."));
    const int64 TransportHour =
        P.Hour + P.StepHours; // purchase commits in one step; delivery is attempted in the following step.
    if (I.DepartureHour < TransportHour + P.Rules.BookingLeadHours ||
        I.DepartureHour > TransportHour + P.Rules.MaxBookingDelayHours)
        return Fail(TEXT("Abfahrt außerhalb des Buchungsfensters nach dem Kaufschritt."));
    if (V->Mode == TEXT("ship"))
    {
        const auto W = WaterQuote(P.Catalog, P.Rules, From->CityId, To->CityId, V->Own, TransportHour, I.DepartureHour,
                                  O.OriginalKg, CarrierCountry(P.Catalog, V->ProviderId), P.StartDate);
        if (!W.Terms.Km)
            return Fail(W.Reason);
        Q.Transport = W.Terms;
        for (const auto &Id : W.Plan.EdgeIds)
            if (const auto *Edge = Find(P.Catalog.Network.Edges, Id); Edge && Edge->Mode == TEXT("transfer"))
                Q.HandlingMinutes += Edge->FixedMinutes;
        TArray<FVehicle> Fleet;
        for (const auto &X : P.Vehicles)
        {
            FVehicle F;
            F.Id = X.Id;
            F.TypeId = X.TypeId;
            F.OwnerId = X.ProviderId;
            F.CityId = X.CityId;
            F.Bound = X.Bound;
            F.ArrearsCents = X.Blocked ? 1 : 0;
            Fleet.Add(F);
        }
        TArray<FString> Feeders;
        if (!WaterFeeders(P.Catalog, Fleet, V->ProviderId, From->CityId, To->CityId, O.OriginalKg, Feeders, E))
            return false;
    }
    else
        Q.Transport =
            TransportTerms(P.Catalog, P.Rules, From->CityId, To->CityId, *Type, V->Own, TransportHour, I.DepartureHour);
    if (!Q.Transport.Km)
        return Fail(TEXT("Keine gültige Verbindung."));
    if (O.OriginalKg > 1000000000000000LL / O.LimitCentsPerKg || Q.Transport.Cost < 0 ||
        Q.Transport.Cost > 1000000000000LL)
        return Fail(TEXT("Kosten überschreiten das Planlimit."));
    Q.GoodsCents = O.OriginalKg * O.LimitCentsPerKg;
    // Only the new purchase quantity is estimated. Existing inventory/site costs remain separate.
    if (P.Rules.RentalCentsPerPointDay && Q.Points > 1000000000000000LL / P.Rules.RentalCentsPerPointDay)
        return Fail(TEXT("Mietkosten über dem Planlimit."));
    const int64 Acquired = Old && Old->QuantityKg ? Old->AcquiredHour : P.Hour;
    if (!From->Own)
    {
        const int64 PerBill = Q.Points * P.Rules.RentalCentsPerPointDay;
        if (PerBill < 0 || PerBill > 1000000000000000LL)
            return Fail(TEXT("Mietkosten über dem Planlimit."));
        for (int64 H = (P.Hour + P.Rules.CostsHours - 1) / P.Rules.CostsHours * P.Rules.CostsHours; H < I.DepartureHour;
             H += P.Rules.CostsHours)
            if (H - Acquired >= P.Rules.PickupGraceHours)
            {
                if (Q.SourceRentCents > 1000000000000000LL - PerBill)
                    return Fail(TEXT("Quellmiete über dem Planlimit."));
                Q.SourceRentCents += PerBill;
            }
    }
    Q.DestinationRentDayCents = To->Own ? 0 : Q.Points * P.Rules.RentalCentsPerPointDay;
    Q.TotalCents = Q.GoodsCents + Q.Transport.Cost + Q.SourceRentCents;
    if (Q.TotalCents < 0 || Q.TotalCents > Cash)
        return Fail(TEXT("Freie Mittel reichen nicht für Warenpreis, sicheren Transport und bekannte Quellmiete. Noch "
                         "nichts reserviert."));
    E.Empty();
    return true;
}
bool FSimulation::PreviewPurchase(const FMarketOrder &O, const FPurchaseIntent &I, FPurchaseQuote &Q, FString &E) const
{
    FSnapshot Current;
    PublishPlayer(Current);
    const auto *A = Find(State.Accounts, ActorAccount(Session.PlayerCompanyId));
    return A && QuotePurchase(Current.Player, A->BalanceCents - A->ReservedCents, O, I, Q, E);
}
void FSimulation::ProgressPurchases()
{
    for (auto &B : State.Businesses)
    {
        auto &I = B.Purchase;
        if (!I.Enabled || (I.Phase != TEXT("awaiting_purchase") && I.Phase != TEXT("ready")))
            continue;
        const auto *O = Find(State.Orders, B.OrderId);
        const int64 Bought = O ? O->OriginalKg - O->RemainingKg : B.FilledKg;
        if (I.Phase == TEXT("awaiting_purchase"))
        {
            if (Bought > 0)
            {
                I.Phase = TEXT("ready");
                I.ReadyStep = State.Step + 2;
            }
            else if (!O || O->Status != TEXT("open"))
                I.Phase = TEXT("no_purchase");
            continue;
        }
        if (State.Step + 1 < I.ReadyStep)
            continue;
        I.Phase = TEXT(
            "pickup_required"); // One attempt, persisted BEFORE calling the existing service; never repeat the buy.
        const int64 Remaining = Bought - BusinessAssignedKg(B);
        if (Remaining <= 0)
        {
            I.Phase = B.Legs.IsEmpty() ? TEXT("no_purchase") : TEXT("booked");
            continue;
        }
        const auto *Stock = State.Inventories.FindByPredicate([&](const FInventory &X) {
            return X.OwnerId == B.OwnerId && X.StorageId == B.StorageId && X.GoodId == B.GoodId;
        });
        const auto *Source = Find(State.Warehouses, B.StorageId);
        const auto *Target = Find(State.Warehouses, I.DestinationStorageId);
        const auto *V = Find(State.Vehicles, I.VehicleId);
        FString Error;
        FTransportTerms Terms;
        if (!Stock || !Source || !Target || !V || Stock->QuantityKg - Stock->ReservedKg < Remaining)
        {
            B.LastReason = TEXT("Gekauft – Abholung erforderlich: erworbene Menge nicht mehr vollständig frei; "
                                "Transport erneut planen.");
            continue;
        }
        const auto *Type = Find(Scenario.Definitions.VehicleTypes, V->TypeId);
        if (Type->Mode == TEXT("ship"))
        {
            auto Q = WaterQuote(Scenario.Definitions, Scenario.Economy, Source->CityId, Target->CityId,
                                V->OwnerId == B.OwnerId, State.EconomyHour, I.DepartureHour, Remaining,
                                CarrierCountry(Scenario.Definitions, V->OwnerId), Scenario.StartDate);
            Terms = Q.Terms;
            Error = Q.Reason;
        }
        else
            Terms = TransportTerms(Scenario.Definitions, Scenario.Economy, Source->CityId, Target->CityId, *Type,
                                   V->OwnerId == B.OwnerId, State.EconomyHour, I.DepartureHour);
        if (!Terms.Km || Terms.Cost > I.MaxFreightCents || Terms.Arrival > I.LatestArrivalHour ||
            I.DepartureHour < State.EconomyHour + Scenario.Economy.BookingLeadHours)
        {
            B.LastReason = TEXT("Gekauft – Abholung erforderlich: autorisierte Transportkosten/-zeiten überschritten. "
                                "Neue Vorschau bestätigen. ") +
                           Error;
            continue;
        }
        FCommand C;
        C.Kind = ECommandKind::BookTransport;
        C.Origin = ECommandOrigin::Player;
        C.CompanyId = B.OwnerId;
        C.TradeCaseId = B.Id;
        auto &J = C.Transport;
        J.Id = TEXT("purchase.haul.") + B.OrderId;
        J.OwnerId = B.OwnerId;
        J.VehicleId = I.VehicleId;
        J.FromCityId = Source->CityId;
        J.ToCityId = Target->CityId;
        J.SourceInventoryId = Stock->Id;
        J.DestinationStorageId = I.DestinationStorageId;
        J.Kg = Remaining;
        J.RequestedDepartureHour = I.DepartureHour;
        if (BusinessTransport(C, Error))
            I.Phase = TEXT("booked");
        else
            B.LastReason = TEXT("Gekauft – Abholung erforderlich: ") + PlayerReason(Error);
    }
}
bool ValidPurchaseIntent(const FDefinitions &D, const FState &W, const FTradeBusiness &B, FString &E)
{
    const auto &I = B.Purchase;
    if (!I.Enabled)
        return true;
    const auto *V = Find(W.Vehicles, I.VehicleId);
    const auto *T = Find(W.Warehouses, I.DestinationStorageId);
    if (B.Side != TEXT("buy") || !V || !T || T->Id == B.StorageId || I.OfferId.IsEmpty() || I.MaxFreightCents < 0 ||
        I.MaxFreightCents > 1000000000000LL || I.ApprovedHour < 0 || I.ApprovedHour > B.CreatedHour ||
        I.DepartureHour <= I.ApprovedHour || I.LatestArrivalHour <= I.DepartureHour || I.ReadyStep < 0 ||
        I.ReadyStep > W.Step + 1 ||
        (I.Phase != TEXT("awaiting_purchase") && I.Phase != TEXT("ready") && I.Phase != TEXT("booked") &&
         I.Phase != TEXT("pickup_required") && I.Phase != TEXT("no_purchase")) ||
        (I.Phase == TEXT("ready") && !I.ReadyStep))
    {
        E = TEXT("Ungültiger gespeicherter Kauf-/Abholplan.");
        return false;
    }
    return true;
}
} // namespace CoISim

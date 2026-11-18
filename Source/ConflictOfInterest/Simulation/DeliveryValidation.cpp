#include "SimArchive.h"
#include "SimCore.h"
namespace CoISim
{
namespace
{
template <class T> bool DeliveryUnique(const TArray<T> &Rows)
{
    TSet<FString> Ids;
    for (const auto &X : Rows)
    {
        if (X.Id.IsEmpty() || Ids.Contains(X.Id))
            return false;
        Ids.Add(X.Id);
    }
    return true;
}
bool ClosedDeliverySlot(const FString &S)
{
    return S == TEXT("missed") || S == TEXT("complete") || S == TEXT("cancelled") || S == TEXT("return_failed");
}
} // namespace
bool FSimulation::ValidateDeliveries(FString &E) const
{
    const auto &W = State;
    const auto &D = Scenario.Definitions;
    const auto &Q = Scenario.Economy;
    int64 H = W.EconomyHour;
    auto Fail = [&](const FString &S) {
        E = TEXT("Liefer-/Standortbilanz ungültig: ") + S;
        return false;
    };
    if (!DeliveryUnique(W.Deliveries) || !DeliveryUnique(W.DeliverySlots) || !DeliveryUnique(W.SiteLedgers) ||
        !DeliveryUnique(W.InternalTransfers) || W.Deliveries.Num() > Q.DeliveryMaxRules)
        return Fail(TEXT("IDs oder Regelgrenze"));
    for (const auto &C : W.Deliveries)
    {
        const auto &R = C.Terms;
        const auto *From = Find(W.Warehouses, C.SourceStoreId);
        const auto *To = Find(W.Warehouses, C.DestinationStoreId);
        if (!From || !To || C.SourceStoreId == C.DestinationStoreId ||
            DeliveryStore(R.SourceAssetId) != C.SourceStoreId ||
            DeliveryStore(R.DestinationAssetId) != C.DestinationStoreId || From->CityId != R.FromCityId ||
            To->CityId != R.ToCityId || !Find(D.Goods, R.GoodId) || !Find(W.Companies, C.OwnerId) ||
            !Find(W.Companies, C.SupplierId) || !Find(W.Companies, C.BuyerId) ||
            (C.OwnerId != C.SupplierId && C.OwnerId != C.BuyerId) || Find(D.Companies, C.SupplierId)->AccountingOnly ||
            Find(D.Companies, C.BuyerId)->AccountingOnly ||
            (C.Status != TEXT("active") && C.Status != TEXT("stopped") && C.Status != TEXT("expired")))
            return Fail(C.Id);
        if (C.Status == TEXT("active") && (From->OwnerId != C.SupplierId || To->OwnerId != C.BuyerId))
            return Fail(C.Id + TEXT(" Eigentum"));
        if (R.Kg < 1 || R.Kg > Q.DeliveryMaxKg || R.Kg > Q.MaxOrderKg || R.IntervalHours < Q.DeliveryMinIntervalHours ||
            R.IntervalHours > Q.DeliveryMaxIntervalHours || R.StartHour < C.AcceptedHour ||
            R.StartHour > C.AcceptedHour + Q.DeliveryMaxDurationHours || R.EndHour < R.StartHour ||
            R.EndHour - R.StartHour > Q.DeliveryMaxDurationHours || R.EndHour > 50000000 || R.PriceCentsPerKg < 0 ||
            R.PriceCentsPerKg > Q.MaxPriceCents || R.MaxFreightCents < 0 || R.MaxFreightCents > 1000000000 ||
            C.AcceptedHour < 0 || C.AcceptedHour > H || C.NextDueHour < R.StartHour ||
            (C.NextDueHour - R.StartHour) % R.IntervalHours || C.NextDueHour > R.EndHour + R.IntervalHours ||
            C.RequestedKg != (C.NextDueHour - R.StartHour) / R.IntervalHours * R.Kg || C.BookedKg < 0 ||
            C.BookedKg > C.RequestedKg || C.DeliveredKg < 0 || C.DeliveredKg > C.BookedKg || C.MissedKg < 0 ||
            C.MissedKg > C.RequestedKg || C.LastMissedKg < 0 || C.LastMissedKg > R.Kg)
            return Fail(C.Id + TEXT(" Zeit/Mengen"));
        if (C.Status == TEXT("active") && (C.NextDueHour < H || C.NextDueHour > R.EndHour))
            return Fail(C.Id + TEXT(" Fälligkeit"));
        if (C.Status == TEXT("expired") && C.NextDueHour <= R.EndHour)
            return Fail(C.Id);
        if (!R.VehicleId.IsEmpty())
        {
            const auto *V = Find(W.Vehicles, R.VehicleId);
            if (!V || V->OwnerId != C.BuyerId)
                return Fail(C.Id + TEXT(" Fahrzeug"));
        }
        if (C.SupplierId != C.BuyerId && (R.PriceCentsPerKg < 1 || C.AcceptedUnitCents < 1 ||
                                          (C.BuyerId == C.OwnerId ? R.PriceCentsPerKg < C.AcceptedUnitCents
                                                                  : R.PriceCentsPerKg > C.AcceptedUnitCents)))
            return Fail(C.Id + TEXT(" Zustimmung"));
        int64 UnarrivedKg = 0;
        for (const auto &X : W.DeliverySlots)
            if (X.ContractId == C.Id && !X.Accounted && X.Status == TEXT("outbound"))
                UnarrivedKg += X.BookedKg;
        if (C.DeliveredKg + C.MissedKg + UnarrivedKg != C.RequestedKg)
            return Fail(C.Id + TEXT(" offene Liefermenge"));
    }
    TSet<FString> DueKeys;
    for (const auto &X : W.DeliverySlots)
    {
        const auto *C = Find(W.Deliveries, X.ContractId);
        if (!C)
            return Fail(X.Id);
        const auto &R = C->Terms;
        const FString Key = X.ContractId + TEXT("|") + LexToString(X.DueHour);
        if (DueKeys.Contains(Key) || X.Id != C->Id + TEXT(".at.") + LexToString(X.DueHour) || X.DueHour < R.StartHour ||
            X.DueHour > R.EndHour || X.DueHour >= C->NextDueHour || X.DueHour > H ||
            (X.DueHour - R.StartHour) % R.IntervalHours || X.RequestedKg != R.Kg || X.BookedKg < 0 ||
            X.BookedKg > X.RequestedKg || X.DeliveredKg < 0 || X.DeliveredKg > X.BookedKg || X.MissedKg < 0 ||
            X.MissedKg > X.RequestedKg ||
            X.PurchaseCents != (C->SupplierId == C->BuyerId ? 0 : X.BookedKg * R.PriceCentsPerKg) ||
            X.InternalValueCents !=
                (X.Accounted && C->SupplierId == C->BuyerId ? X.DeliveredKg * R.PriceCentsPerKg : 0) ||
            X.ReturnHoldCents < 0 || X.ReturnHoldCents > X.ReturnCostCents || X.ReturnCostCents > R.MaxFreightCents ||
            X.ReturnCostCents < 0)
            return Fail(X.Id);
        DueKeys.Add(Key);
        bool Closed = ClosedDeliverySlot(X.Status);
        if (!Closed && X.Status != TEXT("outbound") && X.Status != TEXT("return_wait") && X.Status != TEXT("returning"))
            return Fail(X.Id + TEXT(" Status"));
        if ((X.Accounted || X.Status == TEXT("missed") || X.Status == TEXT("cancelled"))
                ? X.MissedKg != X.RequestedKg - X.DeliveredKg
                : X.MissedKg != X.RequestedKg - X.BookedKg)
            return Fail(X.Id + TEXT(" Restmenge"));
        if ((Closed || X.Status == TEXT("returning")) && X.ReturnHoldCents)
            return Fail(X.Id + TEXT(" verwaister Kostenhold"));
        if ((X.Status == TEXT("outbound") || X.Status == TEXT("return_wait")) && X.ReturnHoldCents != X.ReturnCostCents)
            return Fail(X.Id + TEXT(" Rückfahrthold"));
        if (X.Status == TEXT("outbound") && X.Accounted)
            return Fail(X.Id);
        if ((X.Status == TEXT("return_wait") || X.Status == TEXT("returning") || X.Status == TEXT("complete")) &&
            !X.Accounted)
            return Fail(X.Id);
        if (X.BookedKg)
        {
            if (X.OutboundId != X.Id + TEXT(".out"))
                return Fail(X.Id);
            const auto *J = Find(W.Transports, X.OutboundId);
            if (!Closed && !J)
                return Fail(X.Id + TEXT(" Hinfahrt fehlt"));
            if (J && (J->DeliveryId != X.Id || J->OwnerId != C->BuyerId || J->Empty || J->FromCityId != R.FromCityId ||
                      J->ToCityId != R.ToCityId || J->GoodId != R.GoodId || J->Kg != X.BookedKg ||
                      J->DestinationStorageId != C->DestinationStoreId || J->ReportSiteId != C->DestinationStoreId))
                return Fail(X.Id + TEXT(" Hinfahrtzuordnung"));
        }
        else if (X.Status != TEXT("missed") || !X.OutboundId.IsEmpty())
            return Fail(X.Id);
        if (!X.ReturnId.IsEmpty())
        {
            const auto *J = Find(W.Transports, X.ReturnId);
            if (X.ReturnId != X.Id + TEXT(".back") || (!Closed && !J) ||
                (J &&
                 (!J->Empty || J->DeliveryId != X.Id || J->FromCityId != R.ToCityId || J->ToCityId != R.FromCityId)))
                return Fail(X.Id + TEXT(" Rückfahrtzuordnung"));
        }
        else if (X.Status == TEXT("returning"))
            return Fail(X.Id);
        if (X.Accounted && (X.ReturnDeadlineHour < X.DueHour || X.ReturnDeadlineHour > H + Q.DeliveryReturnGraceHours))
            return Fail(X.Id + TEXT(" Rückfahrtfrist"));
    }
    for (const auto &J : W.Transports)
        if (!J.DeliveryId.IsEmpty() && (J.Status == TEXT("planned") || J.Status == TEXT("moving")))
        {
            const auto *X = Find(W.DeliverySlots, J.DeliveryId);
            if (!X || (X->OutboundId != J.Id && X->ReturnId != J.Id) || ClosedDeliverySlot(X->Status))
                return Fail(J.Id + TEXT(" aktive Lieferverknüpfung"));
        }
    for (const auto &T : W.InternalTransfers)
    {
        if (!Find(W.Companies, T.ActorId) || !Find(W.Warehouses, T.SourceId) || !Find(W.Warehouses, T.DestinationId) ||
            T.SourceId == T.DestinationId || !Find(D.Goods, T.GoodId) || T.Kg <= 0 || T.ValueCents < 0 ||
            T.Id != T.ActorId + TEXT("|") + T.SourceId + TEXT("|") + T.DestinationId + TEXT("|") + T.GoodId ||
            !Find(W.SiteLedgers, T.ActorId + TEXT("|") + T.SourceId) ||
            !Find(W.SiteLedgers, T.ActorId + TEXT("|") + T.DestinationId))
            return Fail(T.Id);
    }
    for (const auto &L : W.SiteLedgers)
    {
        if (!Find(W.Companies, L.ActorId) ||
            (!L.SiteId.IsEmpty() && !Find(W.Warehouses, L.SiteId) && !Find(W.Vehicles, L.SiteId)) ||
            L.Id != L.ActorId + TEXT("|") + L.SiteId || L.ExternalRevenueCents < 0 || L.ExternalCostCents < 0 ||
            L.InternalCreditCents < 0 || L.InternalDebitCents < 0)
            return Fail(L.Id);
        int64 Credit = 0, Debit = 0;
        for (const auto &T : W.InternalTransfers)
            if (T.ActorId == L.ActorId)
            {
                if (T.SourceId == L.SiteId)
                    Credit += T.ValueCents;
                if (T.DestinationId == L.SiteId)
                    Debit += T.ValueCents;
            }
        if (Credit != L.InternalCreditCents || Debit != L.InternalDebitCents)
            return Fail(L.Id + TEXT(" unpaarige interne Bewertung"));
    }
    if (!W.DeveloperModified)
        for (const auto &C : W.Companies)
        {
            const auto *A = Find(W.Accounts, C.AccountId);
            const auto *Initial = Find(Scenario.InitialState.Accounts, C.AccountId);
            if (!A || !Initial)
                return Fail(C.Id);
            int64 Net = 0;
            for (const auto &L : W.SiteLedgers)
                if (L.ActorId == C.Id)
                    Net += L.ExternalRevenueCents - L.ExternalCostCents;
            if (Net != A->BalanceCents - Initial->BalanceCents)
                return Fail(C.Id + TEXT(" externe Zahlungen stimmen nicht mit Firmenbargeld überein"));
        }
    E.Empty();
    return true;
}
} // namespace CoISim

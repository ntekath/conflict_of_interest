#include "TradeBusiness.h"
#include "SimCore.h"
#include "SimArchive.h"
namespace CoISim
{
int64 BusinessAssignedKg(const FTradeBusiness &B)
{
    int64 Kg = 0;
    for (const auto &L : B.Legs)
        if (L.Status != TEXT("cancelled"))
            Kg += L.Kg;
    return Kg;
}
FString BusinessPhase(const FTradeBusiness &B, bool OwnSource)
{
    if (B.OrderStatus == TEXT("rejected"))
        return TEXT("Fehler · Auftrag abgelehnt");
    if (B.Side == TEXT("sell"))
    {
        if (B.OrderStatus == TEXT("open"))
            return B.FilledKg ? TEXT("Verkaufsauftrag offen · teilweise verkauft") : TEXT("Verkaufsauftrag offen");
        return TEXT("Abgeschlossen");
    }
    for (const auto &L : B.Legs)
        if (L.Status == TEXT("moving"))
            return TEXT("Unterwegs");
    for (const auto &L : B.Legs)
        if (L.Status == TEXT("planned"))
            return TEXT("Transport geplant");
    if (B.OrderStatus == TEXT("open"))
        return B.FilledKg ? TEXT("Teilweise gekauft") : TEXT("Auftrag offen");
    if (!B.FilledKg)
        return TEXT("Abgeschlossen · ohne Kauf");
    if (BusinessAssignedKg(B) < B.FilledKg)
        return OwnSource ? TEXT("Abgeschlossen · am eigenen Standort") : TEXT("Gekauft – Abholung erforderlich");
    return TEXT("Angekommen · abgeschlossen");
}
bool BusinessFinished(const FTradeBusiness &B, bool OwnSource)
{
    const auto P = BusinessPhase(B, OwnSource);
    return P.StartsWith(TEXT("Abgeschlossen")) || P.StartsWith(TEXT("Angekommen")) || B.OrderStatus == TEXT("rejected");
}
bool FSimulation::BusinessOrder(const FCommand &C, FString &E)
{
    const auto &O = C.Order;
    if (C.Origin != ECommandOrigin::Player || C.CompanyId != Session.PlayerCompanyId || O.ActorId != C.CompanyId ||
        !C.TradeCaseId.IsEmpty() ||
        (C.TradePurpose != TEXT("restock") && C.TradePurpose != TEXT("production") &&
         C.TradePurpose != TEXT("resale")) ||
        !Find(Scenario.Definitions.Goods, O.GoodId) || !Find(State.Warehouses, O.StorageId) || O.Id.IsEmpty() ||
        O.Id.Len() > 128 || O.OriginalKg <= 0 || O.OriginalKg > Scenario.Economy.MaxOrderKg ||
        (O.Side != TEXT("buy") && O.Side != TEXT("sell")))
    {
        E = TEXT("Ungültiger Handelsvorgang.");
        return false;
    }
    if (State.Businesses.ContainsByPredicate([&](const FTradeBusiness &B) { return B.OrderId == O.Id; }) ||
        Find(State.Orders, O.Id))
    {
        E = TEXT("Marktauftrag bereits erfasst.");
        return false;
    }
    if (State.Businesses.Num() >= BusinessLimit)
    {
        E = TEXT("Zu viele offene Handelsvorgänge. Vorhandene Aufträge zuerst abschließen.");
        return false;
    }
    const bool Accepted = PlaceOrder(O, E);
    FTradeBusiness B;
    B.Id = TEXT("business.") + O.Id;
    B.OwnerId = O.ActorId;
    B.OrderId = O.Id;
    B.StorageId = O.StorageId;
    B.GoodId = O.GoodId;
    B.Side = O.Side;
    B.Purpose = C.TradePurpose;
    B.RequestedKg = O.OriginalKg;
    B.CreatedHour = State.EconomyHour;
    B.OrderStatus = Accepted ? TEXT("open") : TEXT("rejected");
    if (!Accepted)
        B.LastReason = PlayerReason(E);
    State.Businesses.Add(MoveTemp(B));
    return Accepted;
}
bool FSimulation::BusinessTransport(const FCommand &C, FString &E)
{
    auto *B = Find(State.Businesses, C.TradeCaseId);
    const auto *I = Find(State.Inventories, C.Transport.SourceInventoryId);
    if (C.Origin != ECommandOrigin::Player || !B || B->OwnerId != C.CompanyId || B->OwnerId != C.Transport.OwnerId ||
        B->Side != TEXT("buy"))
    {
        E = TEXT("Kein eigener Kaufvorgang für diese Transportzuordnung.");
        return false;
    }
    const auto *Order = Find(State.Orders, B->OrderId);
    const int64 Bought = Order ? Order->OriginalKg - Order->RemainingKg : B->FilledKg;
    int32 TotalLegs = 0;
    for (const auto &Row : State.Businesses)
        TotalLegs += Row.Legs.Num();
    if (TotalLegs >= BusinessTotalLegLimit || B->Legs.Num() >= BusinessLegLimit || C.Transport.Empty || !I ||
        I->OwnerId != B->OwnerId || I->GoodId != B->GoodId || I->StorageId != B->StorageId || C.Transport.Kg <= 0 ||
        C.Transport.Kg > Bought - BusinessAssignedKg(*B))
    {
        E = TEXT("Transport passt nicht zu erworbener Ware, Quelllager oder noch nicht zugeordneter Kaufmenge.");
        B->LastReason = E;
        return false;
    }
    const bool OK = BookTransport(C.Transport, E);
    if (!OK)
    {
        B->LastReason = PlayerReason(E);
        return false;
    }
    FBusinessLeg L;
    L.Id = C.Transport.Id;
    L.DestinationStorageId = C.Transport.DestinationStorageId;
    L.Kg = C.Transport.Kg;
    L.Status = TEXT("planned");
    B->Legs.Add(MoveTemp(L));
    B->LastReason.Empty();
    return true;
}
bool FSimulation::BusinessOrderReferenced(const FString &Id) const
{
    return State.Businesses.ContainsByPredicate([&](const FTradeBusiness &B) { return B.OrderId == Id; });
}
bool FSimulation::BusinessTransportReferenced(const FString &Id) const
{
    for (const auto &B : State.Businesses)
        for (const auto &L : B.Legs)
            if (L.Id == Id)
                return true;
    return false;
}
void FSimulation::ReconcileBusinesses()
{
    for (auto &B : State.Businesses)
    {
        if (const auto *O = Find(State.Orders, B.OrderId))
        {
            B.FilledKg = O->OriginalKg - O->RemainingKg;
            B.OrderStatus = O->Status;
        }
        for (auto &L : B.Legs)
            if (const auto *T = Find(State.Transports, L.Id))
            {
                L.Status = T->Status;
                L.DeliveredKg = T->DeliveredKg;
            }
    }
    int32 Closed = 0;
    for (const auto &B : State.Businesses)
    {
        const auto *W = Find(State.Warehouses, B.StorageId);
        Closed += BusinessFinished(B, W && W->OwnerId == B.OwnerId);
    }
    for (int32 I = 0; Closed > Scenario.Economy.HistoryLimit && I < State.Businesses.Num();)
    {
        const auto &B = State.Businesses[I];
        const auto *W = Find(State.Warehouses, B.StorageId);
        if (BusinessFinished(B, W && W->OwnerId == B.OwnerId))
        {
            State.Businesses.RemoveAt(I);
            --Closed;
        }
        else
            ++I;
    }
}
bool FSimulation::ValidateBusinesses(FString &E) const
{
    auto Fail = [&]() {
        E = TEXT("Ungültige Handelsvorgänge oder Mengen-/Eigentumszuordnung.");
        return false;
    };
    if (State.Businesses.Num() > BusinessLimit)
        return Fail();
    TSet<FString> Cases, Orders, Jobs;
    for (const auto &B : State.Businesses)
    {
        const auto *O = Find(State.Orders, B.OrderId);
        if (B.Id != TEXT("business.") + B.OrderId || B.OrderId.IsEmpty() || Cases.Contains(B.Id) ||
            Orders.Contains(B.OrderId) || !Find(State.Companies, B.OwnerId) || !Find(State.Warehouses, B.StorageId) ||
            !Find(Scenario.Definitions.Goods, B.GoodId) || (B.Side != TEXT("buy") && B.Side != TEXT("sell")) ||
            (B.Purpose != TEXT("production") && B.Purpose != TEXT("restock") && B.Purpose != TEXT("resale")) ||
            B.CreatedHour < 0 || B.CreatedHour > State.EconomyHour || B.RequestedKg <= 0 ||
            B.RequestedKg > Scenario.Economy.MaxOrderKg || B.FilledKg < 0 || B.FilledKg > B.RequestedKg ||
            B.SettledCents < 0 || B.SettledCents > B.FilledKg * Scenario.Economy.MaxPriceCents ||
            B.Legs.Num() > BusinessLegLimit)
            return Fail();
        Cases.Add(B.Id);
        Orders.Add(B.OrderId);
        if (B.OrderStatus == TEXT("rejected"))
        {
            if (O || B.FilledKg || B.SettledCents || !B.Legs.IsEmpty())
                return Fail();
        }
        else if (!O || O->ActorId != B.OwnerId || O->StorageId != B.StorageId || O->GoodId != B.GoodId ||
                 O->Side != B.Side || O->Status != B.OrderStatus || O->OriginalKg != B.RequestedKg ||
                 O->OriginalKg - O->RemainingKg != B.FilledKg)
            return Fail();
        int64 Linked = 0;
        for (const auto &L : B.Legs)
        {
            const auto *T = Find(State.Transports, L.Id);
            const auto *I = T ? Find(State.Inventories, T->SourceInventoryId) : nullptr;
            if (B.Side != TEXT("buy") || Jobs.Contains(L.Id) || !T || T->Empty || T->OwnerId != B.OwnerId ||
                T->GoodId != B.GoodId || !I || I->StorageId != B.StorageId || I->OwnerId != B.OwnerId ||
                T->DestinationStorageId != L.DestinationStorageId || T->Status != L.Status || T->Kg != L.Kg ||
                T->DeliveredKg != L.DeliveredKg || L.Kg <= 0 || L.DeliveredKg < 0 || L.DeliveredKg > L.Kg)
                return Fail();
            Jobs.Add(L.Id);
            if (L.Status != TEXT("cancelled"))
                Linked += L.Kg;
        }
        if (Linked > B.FilledKg)
            return Fail();
    }
    if (Jobs.Num() > BusinessTotalLegLimit)
        return Fail();
    return true;
}
} // namespace CoISim

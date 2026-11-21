#include "WaterTransport.h"
#include "SimArchive.h"
namespace CoISim
{
FString CarrierCountry(const FDefinitions &D, const FString &Provider)
{
    const auto *C = Find(D.Companies, Provider);
    const auto *P = C ? Find(D.Places, C->HeadquartersCityId) : nullptr;
    const auto *R = P ? Find(D.Regions, P->RegionId) : nullptr;
    return R ? R->CountryId : FString();
}
int64 TransportYear(const FString &Date, int64 Hour)
{
    FDateTime Start;
    if (!FDateTime::ParseIso8601(*(Date + TEXT("T00:00:00Z")), Start))
        return 0;
    return (Start + FTimespan::FromHours(Hour)).GetYear();
}
FWaterQuote WaterQuote(const FDefinitions &D, const FEconomyRules &E, const FString &From, const FString &To, bool Own,
                       int64 Hour, int64 Requested, int64 Kg, const FString &Country, const FString &Date)
{
    FWaterQuote Q;
    auto &T = Q.Terms;
    T.Departure = Requested ? Requested : Hour + E.BookingLeadHours;
    if (T.Departure != Hour + E.BookingLeadHours)
    {
        Q.Reason = TEXT("Wasserkette nur zur nächsten buchbaren Abfahrt; keine langfristige Flottenbindung.");
        return Q;
    }
    Q.Plan = PlanWaterRoute(D, From, To, TransportYear(Date, T.Departure), FMath::Max(1LL, Kg), Country);
    if (!Q.Plan.Valid)
    {
        Q.Reason = Q.Plan.Reason;
        return Q;
    }
    T.Duration = (Q.Plan.Minutes + 59) / 60;
    T.Arrival = T.Departure + T.Duration;
    // At most a three-day currently securable pilot chain, never a months-long reservation.
    if (T.Duration > E.WaterMaxChainHours || TransportYear(Date, T.Arrival) != TransportYear(Date, T.Departure))
    {
        Q.Reason = TEXT("Wasserkette außerhalb des freigegebenen Zeitfensters.");
        T = FTransportTerms();
        return Q;
    }
    T.Km = (Q.Plan.Meters + 999) / 1000;
    T.Operation = Q.Plan.CostCents;
    T.Freight = Own ? 0 : (T.Operation * (1000 + E.FreightMarkupPermille) + 999) / 1000;
    T.Cost = Own ? T.Operation : T.Freight;
    return Q;
}
bool WaterFeeders(const FDefinitions &D, const TArray<FVehicle> &Fleet, const FString &Provider, const FString &From,
                  const FString &To, int64 Kg, TArray<FString> &Ids, FString &Reason)
{
    Ids.Reset();
    for (const auto &City : {From, To})
    {
        const FVehicle *Best = nullptr;
        for (const auto &V : Fleet)
        {
            const auto *T = Find(D.VehicleTypes, V.TypeId);
            if (T && T->Mode == TEXT("truck") && T->CapacityKg >= Kg && V.OwnerId == Provider && V.CityId == City &&
                !V.Bound && !V.ArrearsCents && (!Best || V.Id < Best->Id))
                Best = &V;
        }
        if (!Best)
        {
            Reason = TEXT("Kein freier, ausreichend großer Zubringer-Lkw des Anbieters an beiden Häfen.");
            return false;
        }
        Ids.Add(Best->Id);
    }
    return true;
}
FString WaterStage(const FDefinitions &D, const FTransport &J, int64 Hour)
{
    if (J.NetworkId.IsEmpty())
        return FString();
    if (J.Status != TEXT("moving"))
        return J.Status == TEXT("planned")   ? TEXT("Warten auf Abfahrt")
               : J.Status == TEXT("arrived") ? TEXT("Zugestellt")
                                             : TEXT("Storniert");
    int64 Minute = FMath::Max(0LL, Hour - J.DepartureStep) * 60;
    for (const auto &Id : J.ChainEdgeIds)
    {
        const auto *E = Find(D.Network.Edges, Id);
        if (!E)
            return TEXT("Unbekannter Abschnitt");
        int64 Time = E->FixedMinutes +
                     (E->LengthMeters ? (E->LengthMeters * 60 + E->SpeedMetersHour - 1) / E->SpeedMetersHour : 0);
        if (Minute < Time) {
            const auto* From=Find(D.Network.Nodes,E->FromId);const auto* To=Find(D.Network.Nodes,E->ToId);
            if(E->Mode==TEXT("transfer"))return From&&From->Kind==TEXT("terminal")?TEXT("Beladung im Hafen"):TEXT("Entladung im Hafen");
            if(E->Mode==TEXT("road"))return From&&From->Kind==TEXT("hub")?TEXT("Zubringer zum Hafen"):TEXT("Zubringer zum Ziellager");
            FString A=From?From->Name:TEXT("Hafen"),B=To?To->Name:TEXT("Hafen");if(!A.IsEmpty())A[0]=FChar::ToUpper(A[0]);if(!B.IsEmpty())B[0]=FChar::ToUpper(B[0]);return TEXT("Schiff · ")+A+TEXT(" → ")+B;
        }
        Minute -= Time;
    }
    return TEXT("Zielübergabe");
}
bool ValidateWaterBookings(const FScenario &S, const FState &W, FString &Error)
{
    auto Fail = [&](const FString &M) {
        Error = TEXT("Wasserkette: ") + M;
        return false;
    };
    for (const auto &J : W.Transports)
    {
        const auto *V = Find(W.Vehicles, J.VehicleId);
        const auto *T = V ? Find(S.Definitions.VehicleTypes, V->TypeId) : nullptr;
        if (!T)
            continue;
        const bool Ship = T->Mode == TEXT("ship"), Active = J.Status == TEXT("planned") || J.Status == TEXT("moving");
        if (!Ship)
        {
            if (T->TerminalOnly || !J.NetworkId.IsEmpty() || !J.ChainEdgeIds.IsEmpty() || !J.FeederVehicleIds.IsEmpty())
                return Fail(J.Id);
            continue;
        }
        const auto Q = WaterQuote(S.Definitions, S.Economy, J.FromCityId, J.ToCityId, J.OwnerId == J.ProviderId,
                                  J.DepartureStep - S.Economy.BookingLeadHours, J.DepartureStep, J.Kg,
                                  CarrierCountry(S.Definitions, J.ProviderId), S.StartDate);
        if (!Q.Terms.Km || J.NetworkId != Q.Plan.NetworkId || J.ChainEdgeIds != Q.Plan.EdgeIds ||
            J.RouteKm != Q.Terms.Km || J.OperatingCents != Q.Terms.Operation || J.FreightCents != Q.Terms.Freight ||
            J.CostCents != Q.Terms.Cost || J.ArrivalStep != Q.Terms.Arrival ||
            J.Service != (J.OwnerId == J.ProviderId ? TEXT("own_ship") : TEXT("rented_ship")) ||
            J.FeederVehicleIds.Num() != 2 || J.FeederVehicleIds[0] == J.FeederVehicleIds[1])
            return Fail(J.Id);
        for (int32 I = 0; I < 2; ++I)
        {
            const auto *F = Find(W.Vehicles, J.FeederVehicleIds[I]);
            const auto *FT = F ? Find(S.Definitions.VehicleTypes, F->TypeId) : nullptr;
            if (!F || !FT || FT->Mode != TEXT("truck") || FT->CapacityKg < J.Kg ||
                (Active && (F->OwnerId != J.ProviderId || !F->Bound || F->TransportId != J.Id ||
                            F->CityId != (I ? J.ToCityId : J.FromCityId))))
                return Fail(J.Id + TEXT(" feeder"));
        }
        if (Active)
            for (const auto &Other : W.Transports)
                if (Other.Id != J.Id && !Other.NetworkId.IsEmpty() &&
                    (Other.Status == TEXT("planned") || Other.Status == TEXT("moving")) &&
                    (Other.FromCityId == J.FromCityId || Other.ToCityId == J.FromCityId ||
                     Other.FromCityId == J.ToCityId || Other.ToCityId == J.ToCityId))
                    return Fail(TEXT("Terminal doppelt gebunden"));
    }
    return true;
}
} // namespace CoISim

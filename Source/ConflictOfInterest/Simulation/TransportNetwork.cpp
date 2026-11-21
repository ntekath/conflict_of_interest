#include "TransportNetwork.h"
#include "SimModel.h"
#include "SimArchive.h"
#include "GeoTransform.h"
namespace CoISim
{
namespace
{
double Distance(const FNetworkPoint &A, const FNetworkPoint &B)
{
    return CoIGeo::DistanceKm(A.Latitude, A.Longitude, B.Latitude, B.Longitude) * 1000.;
}
bool Position(const FNetworkPoint &P)
{
    return FMath::IsFinite(P.Latitude) && FMath::IsFinite(P.Longitude) && FMath::Abs(P.Latitude) <= 90 &&
           FMath::Abs(P.Longitude) <= 180;
}
bool Water(const FString &M)
{
    return M == TEXT("river") || M == TEXT("canal") || M == TEXT("coast");
}
bool Years(int64 A, int64 B)
{
    return A >= 1800 && B <= 2200 && A <= B;
}
template <class T> bool Unique(const TArray<T> &Rows)
{
    TSet<FString> Ids;
    for (const auto &R : Rows)
    {
        if (R.Id.IsEmpty() || R.Id.Len() > 128 || Ids.Contains(R.Id))
            return false;
        Ids.Add(R.Id);
    }
    return true;
}
const FNetworkNode *HubNode(const FTransportNetwork &N, const FString &Hub)
{
    return N.Nodes.FindByPredicate([&](const FNetworkNode &X) { return X.Kind == TEXT("hub") && X.HubId == Hub; });
}
bool Allowed(const FTransportNetwork &N, const FNetworkEdge &E, int64 Year, int64 Kg, const FString &Country)
{
    const auto *A = Find(N.Access, E.AccessId);
    return E.Enabled && Year >= E.FirstYear && Year <= E.LastYear && Kg <= E.CapacityKg && A && A->Enabled &&
           Year >= A->FirstYear && Year <= A->LastYear && A->Carriers.Contains(Country);
}
int64 Minutes(const FNetworkEdge &E)
{
    return E.FixedMinutes + (E.LengthMeters ? (E.LengthMeters * 60 + E.SpeedMetersHour - 1) / E.SpeedMetersHour : 0);
}
int64 Cost(const FNetworkEdge &E, int64 Kg)
{
    return E.FixedCents + (E.LengthMeters * E.CentsPerKm + 999) / 1000 + Kg * E.CentsPerKg;
}
} // namespace
bool ValidateTransportNetwork(const FDefinitions &D, FString &Error)
{
    const auto &N = D.Network;
    auto Fail = [&](const FString &S) {
        Error = TEXT("Transportnetz: ") + S;
        return false;
    };
    if (N.Id.IsEmpty())
        return N.Nodes.IsEmpty() && N.Edges.IsEmpty() && N.Ports.IsEmpty() && N.Access.IsEmpty() && N.Evidence.IsEmpty()
                   ? true
                   : Fail(TEXT("Netz-ID fehlt"));
    FDateTime Date;
    if (!FDateTime::ParseIso8601(*(N.Date + TEXT("T00:00:00Z")), Date) || N.Date.Len() != 10 || N.Nodes.Num() > 512 ||
        N.Edges.Num() > 2048 || N.Ports.Num() > 64 || N.Access.Num() > 64 || N.Evidence.Num() > 128 ||
        !Unique(N.Nodes) || !Unique(N.Edges) || !Unique(N.Ports) || !Unique(N.Access) || !Unique(N.Evidence))
        return Fail(TEXT("Datum, Umfang oder IDs ungültig"));
    TSet<FString> Countries, Hubs;
    for (const auto &R : D.Regions)
        Countries.Add(R.CountryId);
    for (const auto &E : N.Evidence)
        if (!E.Url.StartsWith(TEXT("https://")) || E.License.IsEmpty() || E.Note.IsEmpty())
            return Fail(TEXT("Quellennachweis unvollständig: ") + E.Id);
    for (const auto &A : N.Access)
    {
        if (!Years(A.FirstYear, A.LastYear) || A.Countries.IsEmpty() || A.Carriers.IsEmpty() ||
            (A.Enabled && !Find(N.Evidence, A.EvidenceId)))
            return Fail(TEXT("Zugangsregel ungültig: ") + A.Id);
        for (const auto &C : A.Countries)
            if (!Countries.Contains(C))
                return Fail(TEXT("Nicht simuliertes Transitland: ") + C);
        for (const auto &C : A.Carriers)
            if (!Countries.Contains(C))
                return Fail(TEXT("Nicht freigegebene Flagge: ") + C);
    }
    for (const auto &V : N.Nodes)
    {
        if (!Position(V.Position) || !Countries.Contains(V.CountryId) ||
            (V.Kind != TEXT("hub") && V.Kind != TEXT("terminal") && V.Kind != TEXT("river") &&
             V.Kind != TEXT("canal") && V.Kind != TEXT("coast")))
            return Fail(TEXT("Knoten/Land ungültig: ") + V.Id);
        if (V.Kind == TEXT("hub"))
        {
            const auto *P = Find(D.Places, V.HubId);
            const auto *R = P ? Find(D.Regions, P->RegionId) : nullptr;
            if (!P || !R || R->CountryId != V.CountryId || Hubs.Contains(V.HubId) ||
                Distance(V.Position, {P->Latitude, P->Longitude}) > 2.)
                return Fail(TEXT("Hubzuordnung ungültig: ") + V.Id);
            Hubs.Add(V.HubId);
        }
        else if (!V.HubId.IsEmpty())
            return Fail(TEXT("Zusätzlicher Wirtschaftshub am Wasser: ") + V.Id);
    }
    if (Hubs.Num() != D.Places.Num())
        return Fail(TEXT("Hubabdeckung unvollständig"));
    for (const auto &E : N.Edges)
    {
        const auto *A = Find(N.Nodes, E.FromId);
        const auto *B = Find(N.Nodes, E.ToId);
        const auto *Access = Find(N.Access, E.AccessId);
        if (!A || !B || A == B || !Access || !Access->Countries.Contains(A->CountryId) ||
            !Access->Countries.Contains(B->CountryId) || !Years(E.FirstYear, E.LastYear) ||
            (!Water(E.Mode) && E.Mode != TEXT("road") && E.Mode != TEXT("transfer")))
            return Fail(TEXT("Kantenreferenz, Verkehrsart oder Zugang ungültig: ") + E.Id);
        if (E.Enabled && (!Find(N.Evidence, E.EvidenceId) || !Access->Enabled))
            return Fail(TEXT("Unbelegter/freigegebener Abschnitt: ") + E.Id);
        if (E.CapacityKg < 1 || E.CapacityKg > 1000000 || E.LengthMeters < 0 || E.LengthMeters > 5000000 ||
            E.FixedMinutes < 0 || E.FixedMinutes > 10080 || E.FixedCents < 0 || E.FixedCents > 1000000 ||
            E.CentsPerKm < 0 || E.CentsPerKm > 10000 || E.CentsPerKg < 0 || E.CentsPerKg > 100 ||
            E.Geometry.Num() < 2 || E.Geometry.Num() > 512)
            return Fail(TEXT("Längen-/Kapazitäts-/Kostenparameter ungültig: ") + E.Id);
        double M = 0;
        for (int32 I = 0; I < E.Geometry.Num(); ++I)
        {
            if (!Position(E.Geometry[I]))
                return Fail(TEXT("Geometrie ungültig: ") + E.Id);
            if (I)
                M += Distance(E.Geometry[I - 1], E.Geometry[I]);
        }
        if (Distance(A->Position, E.Geometry[0]) > 2. || Distance(B->Position, E.Geometry.Last()) > 2.)
            return Fail(TEXT("Geometrie schließt nicht am Knoten an: ") + E.Id);
        if (E.Mode == TEXT("transfer"))
        {
            if (E.LengthMeters || E.SpeedMetersHour || E.FixedMinutes < 1 || M > 2000. ||
                !((A->Kind == TEXT("terminal") && Water(B->Kind)) || (Water(A->Kind) && B->Kind == TEXT("terminal"))))
                return Fail(TEXT("Umschlag ungültig: ") + E.Id);
        }
        else
        {
            if (E.LengthMeters < 1 || E.SpeedMetersHour < 1 || E.SpeedMetersHour > 1000000 ||
                FMath::Abs(double(E.LengthMeters) - FMath::CeilToDouble(M)) > 1.)
                return Fail(TEXT("Fahrmeter nicht aus WGS84-Linie: ") + E.Id);
            if (Water(E.Mode) && (!Water(A->Kind) || !Water(B->Kind)))
                return Fail(TEXT("Wasserverkehr an Landknoten: ") + E.Id);
            if (E.Mode == TEXT("road") && ((A->Kind != TEXT("hub") && A->Kind != TEXT("terminal")) ||
                                           (B->Kind != TEXT("hub") && B->Kind != TEXT("terminal"))))
                return Fail(TEXT("Landzubringer an Wasserknoten: ") + E.Id);
        }
        if (E.Enabled && E.FixedCents == 0 && E.CentsPerKm == 0 && E.CentsPerKg == 0)
            return Fail(TEXT("Kostenlose Kante nicht freigegeben: ") + E.Id);
    }
    TSet<FString> PortHubs;
    for (const auto &P : N.Ports)
    {
        const auto *H = HubNode(N, P.HubId);
        const auto *L = Find(N.Nodes, P.LandNodeId);
        const auto *W = Find(N.Nodes, P.WaterNodeId);
        if (!H || !L || !W || L->Kind != TEXT("terminal") || !Water(W->Kind) || H->CountryId != L->CountryId ||
            H->CountryId != W->CountryId || PortHubs.Contains(P.HubId))
            return Fail(TEXT("Hafen-Hubbindung ungültig: ") + P.Id);
        PortHubs.Add(P.HubId);
        auto Edge = [&](const FString &Id, const FString &From, const FString &To, const FString &Mode) {
            const auto *E = Find(N.Edges, Id);
            return E && E->FromId == From && E->ToId == To && E->Mode == Mode;
        };
        if (!Edge(P.FeederOutId, H->Id, L->Id, TEXT("road")) || !Edge(P.FeederBackId, L->Id, H->Id, TEXT("road")) ||
            !Edge(P.TransferOutId, L->Id, W->Id, TEXT("transfer")) ||
            !Edge(P.TransferBackId, W->Id, L->Id, TEXT("transfer")))
            return Fail(TEXT("Hafen-Zubringer/Umschlag fehlt: ") + P.Id);
    }
    return true;
}
FNetworkPlan PlanWaterRoute(const FDefinitions &D, const FString &From, const FString &To, int64 Year, int64 Kg,
                            const FString &Country)
{
    FNetworkPlan P;
    const auto &N = D.Network;
    P.NetworkId = N.Id;
    if (!ValidateTransportNetwork(D, P.Reason))
        return P;
    if (N.Id.IsEmpty())
    {
        P.Reason = TEXT("Kein konfiguriertes Wassertransportnetz.");
        return P;
    }
    if (Kg < 1 || Kg > 1000000 || Year < 1800 || Year > 2200 || From == To || !HubNode(N, From) || !HubNode(N, To))
    {
        P.Reason = TEXT("Ungültige Hubs, Menge oder historisches Jahr.");
        return P;
    }
    const auto *A = N.Ports.FindByPredicate([&](const FNetworkPort &X) { return X.Enabled && X.HubId == From; });
    const auto *B = N.Ports.FindByPredicate([&](const FNetworkPort &X) { return X.Enabled && X.HubId == To; });
    if (!A || !B)
    {
        P.Reason = TEXT("Kein freigegebener Hafen-/Umschlaganschluss an Quelle oder Ziel.");
        return P;
    }
    TArray<FString> Required = {A->FeederOutId, A->TransferOutId, B->TransferBackId, B->FeederBackId};
    for (const auto &Id : Required)
        if (!Allowed(N, *Find(N.Edges, Id), Year, Kg, Country))
        {
            P.Reason = TEXT("Landzubringer/Umschlag gesperrt: Jahr, Flagge oder Kapazität nicht freigegeben.");
            return P;
        }
    const int32 Count = N.Nodes.Num();
    auto Index = [&](const FString &Id) {
        return N.Nodes.IndexOfByPredicate([&](const FNetworkNode &X) { return X.Id == Id; });
    };
    const int32 Start = Index(A->WaterNodeId), End = Index(B->WaterNodeId);
    TArray<int64> Times, Costs;
    Times.Init(MAX_int64 / 4, Count);
    Costs.Init(MAX_int64 / 4, Count);
    TArray<int32> Previous;
    Previous.Init(INDEX_NONE, Count);
    TArray<FString> Keys;
    Keys.SetNum(Count);
    TArray<bool> Done;
    Done.Init(false, Count);
    Times[Start] = Costs[Start] = 0;
    for (int32 Loop = 0; Loop < Count; ++Loop)
    {
        int32 V = INDEX_NONE;
        for (int32 I = 0; I < Count; ++I)
            if (!Done[I] && Times[I] < MAX_int64 / 4 &&
                (V == INDEX_NONE || Times[I] < Times[V] ||
                 (Times[I] == Times[V] && (Costs[I] < Costs[V] || (Costs[I] == Costs[V] && Keys[I] < Keys[V])))))
                V = I;
        if (V == INDEX_NONE || V == End)
            break;
        Done[V] = true;
        for (int32 I = 0; I < N.Edges.Num(); ++I)
        {
            const auto &E = N.Edges[I];
            if (E.FromId != N.Nodes[V].Id || !Water(E.Mode) || !Allowed(N, E, Year, Kg, Country))
                continue;
            const int32 U = Index(E.ToId);
            const int64 T = Times[V] + Minutes(E), C = Costs[V] + Cost(E, Kg);
            const FString Key = Keys[V] + TEXT("|") + E.Id;
            if (T < Times[U] || (T == Times[U] && (C < Costs[U] || (C == Costs[U] && Key < Keys[U]))))
            {
                Times[U] = T;
                Costs[U] = C;
                Keys[U] = Key;
                Previous[U] = I;
            }
        }
    }
    if (Previous[End] == INDEX_NONE)
    {
        P.Reason = TEXT(
            "Kein freigegebenes Fahrwasser: Netzunterbrechung, Verkehrsart, Zeitraum, Flagge oder Kapazität prüfen.");
        return P;
    }
    TArray<FString> Middle;
    for (int32 V = End; V != Start;)
    {
        const int32 I = Previous[V];
        if (I == INDEX_NONE || Middle.Num() > Count)
        {
            P.Reason = TEXT("Ungültige Pfadrekonstruktion.");
            return P;
        }
        Middle.Insert(N.Edges[I].Id, 0);
        V = Index(N.Edges[I].FromId);
    }
    P.EdgeIds = {Required[0], Required[1]};
    P.EdgeIds.Append(Middle);
    P.EdgeIds.Add(Required[2]);
    P.EdgeIds.Add(Required[3]);
    P.CapacityKg = MAX_int64;
    for (const auto &Id : P.EdgeIds)
    {
        const auto &E = *Find(N.Edges, Id);
        P.Meters += E.LengthMeters;
        if (Water(E.Mode))
            P.WaterMeters += E.LengthMeters;
        P.Minutes += Minutes(E);
        P.CostCents += Cost(E, Kg);
        P.CapacityKg = FMath::Min(P.CapacityKg, E.CapacityKg);
    }
    P.Valid = true;
    P.Reason = TEXT("Netzvorschau; keine Buchung, kein Fahrzeug und keine Reservierung.");
    return P;
}
FString TransportNetworkDiagnostics(const FDefinitions &D, int64 Year, const FString &Country)
{
    const auto &N = D.Network;
    FString Error;
    if (!ValidateTransportNetwork(D, Error))
        return Error;
    if (N.Id.IsEmpty())
        return TEXT("MAP04: kein Wassertransportnetz für dieses Testszenario.");
    FString T = FString::Printf(TEXT("MAP04 %s · Datenstand %s · %d Knoten / %d Kanten / %d Hafenanschlüsse\nParameter "
                                     "sind Spielwerte; keine Kapazitätsreservierung.\n"),
                                *N.Id, *N.Date, N.Nodes.Num(), N.Edges.Num(), N.Ports.Num());
    int32 Missing = 0;
    for (const auto &V : N.Nodes)
        if (V.Kind == TEXT("hub") &&
            !N.Ports.ContainsByPredicate([&](const FNetworkPort &P) { return P.Enabled && P.HubId == V.HubId; }))
            ++Missing;
    T += FString::Printf(TEXT("%d Hubs ohne freigegebenen Wasseranschluss. Kreuzende Linien sind keine Netzknoten.\n"),
                         Missing);
    for (const auto &E : N.Edges)
        if (!E.Enabled || !Find(N.Evidence, E.EvidenceId))
            T += E.Id + TEXT(" · ") + E.Mode + TEXT(" · gesperrt") +
                 (Find(N.Evidence, E.EvidenceId) ? TEXT(" · Abschnittsfreigabe fehlt\n")
                                                 : TEXT(" · unbelegter Abschnitt\n"));
    if (N.Ports.Num() >= 2)
    {
        auto P = PlanWaterRoute(D, N.Ports[0].HubId, N.Ports[1].HubId, Year, 1000, Country);
        T += P.Valid
                 ? FString::Printf(TEXT("Pilotroute: %lld m (%lld m Wasser), %lld min, %lld Cent, Engpass %lld kg.\n"),
                                   P.Meters, P.WaterMeters, P.Minutes, P.CostCents, P.CapacityKg)
                 : P.Reason;
    }
    return T;
}
} // namespace CoISim

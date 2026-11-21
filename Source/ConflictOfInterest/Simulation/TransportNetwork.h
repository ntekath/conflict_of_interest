#pragma once
#include "CoreMinimal.h"
namespace CoISim
{
struct FNetworkPoint
{
    double Latitude = 0, Longitude = 0;
    template <class R> void Fields(R &W)
    {
        W(TEXT("Latitude"), Latitude);
        W(TEXT("Longitude"), Longitude);
    }
};
struct FNetworkNode
{
    FString Id, Name, Kind, CountryId, HubId;
    FNetworkPoint Position;
    template <class R> void Fields(R &W)
    {
        W(TEXT("Id"), Id);
        W(TEXT("Name"), Name);
        W(TEXT("Kind"), Kind);
        W(TEXT("CountryId"), CountryId);
        W(TEXT("HubId"), HubId);
        W(TEXT("Position"), Position);
    }
};
struct FNetworkAccess
{
    FString Id, EvidenceId;
    bool Enabled = false;
    int64 FirstYear = 1913, LastYear = 1913;
    TArray<FString> Countries, Carriers;
    template <class R> void Fields(R &W)
    {
        W(TEXT("Id"), Id);
        W(TEXT("EvidenceId"), EvidenceId);
        W(TEXT("Enabled"), Enabled);
        W(TEXT("FirstYear"), FirstYear);
        W(TEXT("LastYear"), LastYear);
        W(TEXT("Countries"), Countries);
        W(TEXT("Carriers"), Carriers);
    }
};
struct FNetworkEvidence
{
    FString Id, Url, License, Note;
    template <class R> void Fields(R &W)
    {
        W(TEXT("Id"), Id);
        W(TEXT("Url"), Url);
        W(TEXT("License"), License);
        W(TEXT("Note"), Note);
    }
};
struct FNetworkEdge
{
    FString Id, FromId, ToId, Mode, AccessId, EvidenceId;
    bool Enabled = false;
    int64 FirstYear = 1913, LastYear = 1913, LengthMeters = 0, CapacityKg = 0, SpeedMetersHour = 0, FixedMinutes = 0,
          FixedCents = 0, CentsPerKm = 0, CentsPerKg = 0;
    TArray<FNetworkPoint> Geometry;
    template <class R> void Fields(R &W)
    {
        W(TEXT("Id"), Id);
        W(TEXT("FromId"), FromId);
        W(TEXT("ToId"), ToId);
        W(TEXT("Mode"), Mode);
        W(TEXT("AccessId"), AccessId);
        W(TEXT("EvidenceId"), EvidenceId);
        W(TEXT("Enabled"), Enabled);
        W(TEXT("FirstYear"), FirstYear);
        W(TEXT("LastYear"), LastYear);
        W(TEXT("LengthMeters"), LengthMeters);
        W(TEXT("CapacityKg"), CapacityKg);
        W(TEXT("SpeedMetersHour"), SpeedMetersHour);
        W(TEXT("FixedMinutes"), FixedMinutes);
        W(TEXT("FixedCents"), FixedCents);
        W(TEXT("CentsPerKm"), CentsPerKm);
        W(TEXT("CentsPerKg"), CentsPerKg);
        W(TEXT("Geometry"), Geometry);
    }
};
struct FNetworkPort
{
    FString Id, Name, HubId, LandNodeId, WaterNodeId, FeederOutId, FeederBackId, TransferOutId, TransferBackId;
    bool Enabled = false;
    template <class R> void Fields(R &W)
    {
        W(TEXT("Id"), Id);
        W(TEXT("Name"), Name);
        W(TEXT("HubId"), HubId);
        W(TEXT("LandNodeId"), LandNodeId);
        W(TEXT("WaterNodeId"), WaterNodeId);
        W(TEXT("FeederOutId"), FeederOutId);
        W(TEXT("FeederBackId"), FeederBackId);
        W(TEXT("TransferOutId"), TransferOutId);
        W(TEXT("TransferBackId"), TransferBackId);
        W(TEXT("Enabled"), Enabled);
    }
};
struct FTransportNetwork
{
    FString Id, Date;
    TArray<FNetworkNode> Nodes;
    TArray<FNetworkEdge> Edges;
    TArray<FNetworkPort> Ports;
    TArray<FNetworkAccess> Access;
    TArray<FNetworkEvidence> Evidence;
    template <class R> void Fields(R &W)
    {
        W(TEXT("Id"), Id);
        W(TEXT("Date"), Date);
        W(TEXT("Nodes"), Nodes);
        W(TEXT("Edges"), Edges);
        W(TEXT("Ports"), Ports);
        W(TEXT("Access"), Access);
        W(TEXT("Evidence"), Evidence);
    }
};
struct FNetworkPlan
{
    bool Valid = false;
    FString NetworkId, Reason;
    TArray<FString> EdgeIds;
    int64 Meters = 0, WaterMeters = 0, Minutes = 0, CostCents = 0, CapacityKg = 0;
};
struct FDefinitions;
bool ValidateTransportNetwork(const FDefinitions &, FString &);
FNetworkPlan PlanWaterRoute(const FDefinitions &, const FString &FromHub, const FString &ToHub, int64 Year, int64 Kg,
                            const FString &CarrierCountry);
FString TransportNetworkDiagnostics(const FDefinitions &, int64 Year, const FString &CarrierCountry);
} // namespace CoISim

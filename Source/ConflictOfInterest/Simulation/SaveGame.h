#pragma once
#include "SimCore.h"
namespace CoISim
{
// Generation is a process-local lifetime token, NEVER persisted or trusted from disk.
struct FStoredCommand
{
    int64 Kind = 0, Origin = 0, TargetStep = 0, Sequence = 0;
    FString CompanyId;
    FReservation Resources;
    FMarketOrder Order;
    FTransport Transport;
    FInvestmentRequest Investment;
    FOperatingRule Rule;FDeliveryRequest Delivery;
    template <class R> void Fields(R &W)
    {
        W(TEXT("Kind"), Kind);
        W(TEXT("Origin"), Origin);
        W(TEXT("TargetStep"), TargetStep);
        W(TEXT("Sequence"), Sequence);
        W(TEXT("CompanyId"), CompanyId);
        W(TEXT("Resources"), Resources);
        W(TEXT("Order"), Order);
        W(TEXT("Transport"), Transport);
        W(TEXT("Investment"), Investment);
        W(TEXT("Rule"), Rule);W(TEXT("Delivery"),Delivery);
    }
};
struct FStoredResult
{
    int64 Sequence = 0, Step = 0;
    bool Accepted = false;
    FString Reason, ActorId, ObjectId;
    template <class R> void Fields(R &W)
    {
        W(TEXT("Sequence"), Sequence);
        W(TEXT("Step"), Step);
        W(TEXT("Accepted"), Accepted);
        W(TEXT("Reason"), Reason);
        W(TEXT("ActorId"), ActorId);
        W(TEXT("ObjectId"), ObjectId);
    }
};
struct FSaveData
{
    int64 FormatVersion = 6, SimulationVersion = 6, DefinitionsVersion = 6, SnapshotVersion = 0, LastSequence = 0,
          CreditMicros = 0, Speed = 1;
    FString Platform = TEXT("Win64"), RandomAlgorithm = TEXT("entity-fnv-splitmix-v1"), SavedUtc, CompatibilityHash;
    FScenario Scenario;
    FSession Session;
    FState State;
    TArray<FStoredCommand> Pending;
    TArray<FStoredResult> Results;
    template <class R> void Fields(R &W)
    {
        W(TEXT("FormatVersion"), FormatVersion);
        W(TEXT("SimulationVersion"), SimulationVersion);
        W(TEXT("DefinitionsVersion"), DefinitionsVersion);
        W(TEXT("Platform"), Platform);
        W(TEXT("RandomAlgorithm"), RandomAlgorithm);
        W(TEXT("SavedUtc"), SavedUtc);
        W(TEXT("CompatibilityHash"), CompatibilityHash);
        W(TEXT("SnapshotVersion"), SnapshotVersion);
        W(TEXT("LastSequence"), LastSequence);
        W(TEXT("CreditMicros"), CreditMicros);
        W(TEXT("Speed"), Speed);
        W(TEXT("Scenario"), Scenario);
        W(TEXT("Session"), Session);
        W(TEXT("State"), State);
        W(TEXT("Pending"), Pending);
        W(TEXT("Results"), Results);
    }
};
struct FSaveSummary
{
    FString Company, Date, SavedUtc;
    int64 Step = 0;
    bool DeveloperModified = false;
};
struct FSaveSlotInfo
{
    int32 Slot = 0;
    bool Exists = false, Readable = false;
    FString Description;
};
FString SaveCompatibility(const FScenario &);
bool InspectSaveGame(const FString &, FSaveSummary &, FString &);
// Fixed numbered slots only. No arbitrary UI paths. File code is separated from the simulation.
FString SaveSlotPath(int32 Slot);
bool WriteSaveSlot(int32 Slot, const FString &Json, FString &Error);
bool ReadSaveSlot(int32 Slot, FString &Json, FString &Error);
TArray<FSaveSlotInfo> ListSaveSlots();
} // namespace CoISim

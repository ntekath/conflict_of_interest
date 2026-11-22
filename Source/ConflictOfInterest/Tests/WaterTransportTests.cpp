#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GeoCatalog.h"
#include "Simulation/SimArchive.h"
#include "Simulation/SaveGame.h"
#include "Simulation/WaterTransport.h"
#include "Serialization/JsonSerializer.h"
using namespace CoISim;
namespace
{
bool WaterFixture(FSimulation &S, FCoIGeoMapData &G, FString &E, bool Carrier = false)
{
    FString J;
    auto *C =
        LoadObject<UCoIGeoCatalog>(nullptr, TEXT("/Game/Geography/Data/GeoCatalog_DE_NL_1913.GeoCatalog_DE_NL_1913"));
    if (!C || !G.Load(C->DefinitionJson, E) ||
        !FFileHelper::LoadFileToString(J,
                                       *(FPaths::ProjectContentDir() / TEXT("Simulation/scenario.de_nl1913.coisim"))))
        return false;
    if (Carrier)
    {
        TSharedPtr<FJsonObject> O;
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(J), O);
        O->SetStringField(TEXT("PlayerCompanyId"), TEXT("company.carrier"));
        FJsonSerializer::Serialize(O.ToSharedRef(), TJsonWriterFactory<>::Create(&J));
    }
    return S.Load(J, G, E);
}
FCommand Command(FSimulation &S, ECommandKind Kind)
{
    FCommand C;
    C.Kind = Kind;
    C.Generation = S.GetGeneration();
    C.Sequence = S.NextSequence();
    C.TargetStep = S.GetState().Step + 1;
    return C;
}
FCommand Haul(FSimulation &S, const FString &Id = TEXT("water.test"))
{
    auto C = Command(S, ECommandKind::BookTransport);
    auto &J = C.Transport;
    J.Id = Id;
    J.OwnerId = TEXT("company.player");
    J.VehicleId = TEXT("vehicle.carrier.rhine");
    J.FromCityId = TEXT("city.nl.1913.zuid_holland");
    J.ToCityId = TEXT("city.de.1913.rheinprovinz");
    J.SourceInventoryId = TEXT("stock.storage.player.food");
    J.DestinationStorageId = TEXT("storage.service.city.de.1913.rheinprovinz");
    J.Kg = 100;
    return C;
}
bool Send(FSimulation &S, const FCommand &C, FString &E)
{
    if (!S.Enqueue(C, E) || !S.StepOnce())
        return false;
    const auto *R = S.GetResults().FindByPredicate([&](const FCommandResult &X) { return X.Sequence == C.Sequence; });
    if (!R)
    {
        E = TEXT("No command result");
        return false;
    }
    E = R->Reason;
    return R->Accepted;
}
} // namespace
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaterArrival, "CoI.MAP05.PaidChainAndSaveDuringWaterLeg",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaterArrival::RunTest(const FString &)
{
    FSimulation S;
    FCoIGeoMapData G;
    FString E;
    if (!WaterFixture(S, G, E))
    {
        AddError(E);
        return false;
    }
    auto C = Haul(S);
    if (!TestTrue(TEXT("Water booking"), Send(S, C, E)))
    {
        AddError(E);
        return false;
    }
    auto J = *Find(S.GetState().Transports, C.Transport.Id);
    TestEqual(TEXT("Water service"), J.Service, FString(TEXT("rented_ship")));
    TestEqual(TEXT("Network legs"), J.ChainEdgeIds.Num(), 16);
    TestEqual(TEXT("Two real feeder assets"), J.FeederVehicleIds.Num(), 2);
    TestEqual(TEXT("Goods held once before departure"),
              Find(S.GetState().Inventories, C.Transport.SourceInventoryId)->ReservedKg, 100LL);
    TestEqual(TEXT("Entire freight reserved"), S.GetSnapshot().PlayerReservedCents, J.CostCents);
    TestFalse(TEXT("No freight payment before departure"),S.GetState().CashFlows.ContainsByPredicate([](const FCashFlow& F){return F.Reason==TEXT("freight")&&F.FromAccountId==TEXT("account.company.player");}));
    const auto Cash=S.GetSnapshot().PlayerMoneyCents;
    TestTrue(TEXT("Booking invariants"), S.AuditRuntime(E));
    for (int H = 0; H < 8; ++H)
        S.StepOnce();
    J = *Find(S.GetState().Transports, C.Transport.Id);
    TestEqual(TEXT("Water cargo physically in flight"), J.Status, FString(TEXT("moving")));
    TestTrue(TEXT("Current water leg available"),
             WaterStage(S.GetScenario().Definitions, J, S.GetState().EconomyHour).StartsWith(TEXT("Schiff")));
    TestNotNull(TEXT("Exactly one real cargo inventory"), Find(S.GetState().Inventories, J.CargoInventoryId));
    TestEqual(TEXT("Freight was debited"), S.GetSnapshot().PlayerMoneyCents, Cash - J.CostCents);
    TestTrue(TEXT("Holds now back only actual remaining obligations"),S.AuditRuntime(E));
    FString Save;
    if (!TestTrue(*E, S.SaveGameJson(Save, E)))
        return false;
    FSimulation Restored;
    int32 Speed = 1;
    if (!TestTrue(*E, Restored.LoadGameJson(Save, G, S.GetScenario(), E, Speed)))
        return false;
    const int64 End = J.ArrivalStep + 3;
    while (S.GetState().EconomyHour < End)
    {
        S.StepOnce();
        Restored.StepOnce();
        if (!TestTrue(*E, S.AuditRuntime(E)))
            return false;
    }
    TestEqual(TEXT("Identical save continuation"), S.EconomicFingerprint(), Restored.EconomicFingerprint());
    J = *Find(S.GetState().Transports, C.Transport.Id);
    TestEqual(TEXT("Arrived once"), J.Status, FString(TEXT("arrived")));
    TestTrue(TEXT("Actual goods delivered, spoilage retained"), J.DeliveredKg > 0 && J.DeliveredKg <= 100);
    TestTrue(TEXT("Freight inventory removed once"), !Find(S.GetState().Inventories, J.CargoInventoryId));
    const auto *V = Find(S.GetState().Vehicles, J.VehicleId);
    TestEqual(TEXT("Ship ends at destination"), V->CityId, J.ToCityId);
    TestFalse(TEXT("Ship released"), V->Bound);
    for (int I = 0; I < 2; ++I)
    {
        const auto *F = Find(S.GetState().Vehicles, J.FeederVehicleIds[I]);
        TestFalse(TEXT("Feeder released"), F->Bound);
        TestEqual(TEXT("Feeder remains local"), F->CityId, I ? J.ToCityId : J.FromCityId);
    }
    TestTrue(TEXT("No interventions"), !S.GetState().DeveloperModified);
    FFileHelper::SaveStringToFile(
        FString::Printf(TEXT("network=%s\nkg_booked=%lld\nkg_delivered=%lld\nfreight_cents=%lld\noperation_cents=%"
                             "lld\ndeparture=%lld\narrival=%lld\nfeeders=%s\nstate=%s\n"),
                        *J.NetworkId, J.Kg, J.DeliveredKg, J.CostCents, J.OperatingCents, J.DepartureStep,
                        J.ArrivalStep, *FString::Join(J.FeederVehicleIds, TEXT(",")), *J.Status),
        *(FPaths::ProjectSavedDir() / TEXT("MAP05_delivery.txt")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaterCancel, "CoI.MAP05.AtomicCancelAndDoubleBooking",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaterCancel::RunTest(const FString &)
{
    FSimulation S;
    FCoIGeoMapData G;
    FString E;
    if (!WaterFixture(S, G, E))
        return false;
    auto C = Haul(S);
    if (!Send(S, C, E))
    {
        AddError(E);
        return false;
    }
    const auto J = *Find(S.GetState().Transports, C.Transport.Id);
    const int64 Cash = S.GetSnapshot().PlayerMoneyCents;
    // Both commands execute before the first departure: rejection must not leak holds.
    auto Duplicate = Haul(S, TEXT("water.double"));
    TestTrue(TEXT("Queue duplicate"), S.Enqueue(Duplicate, E));
    auto Cancel = Command(S, ECommandKind::CancelTransport);
    Cancel.Transport.Id = J.Id;
    Cancel.Transport.OwnerId = J.OwnerId;
    TestTrue(TEXT("Queue cancel"), S.Enqueue(Cancel, E));
    S.StepOnce();
    const auto *R =
        S.GetResults().FindByPredicate([&](const FCommandResult &X) { return X.Sequence == Duplicate.Sequence; });
    TestTrue(TEXT("Same ship cannot double book"), R && !R->Accepted);
    TestEqual(TEXT("Cancelled before departure"), Find(S.GetState().Transports, J.Id)->Status,
              FString(TEXT("cancelled")));
    TestEqual(TEXT("No freight charge"), S.GetSnapshot().PlayerMoneyCents, Cash);
    TestTrue(TEXT("Funds back only remaining obligations"),S.AuditRuntime(E));
    TestEqual(TEXT("Goods released"), Find(S.GetState().Inventories, C.Transport.SourceInventoryId)->ReservedKg, 0LL);
    TestEqual(TEXT("Destination released"),
              Find(S.GetState().Warehouses, J.DestinationStorageId)->ReservedIncomingPoints, 0LL);
    for (const auto &Id : J.FeederVehicleIds)
        TestFalse(TEXT("Feeder unbound"), Find(S.GetState().Vehicles, Id)->TransportId == J.Id);
    TestTrue(TEXT("Atomic release audit"), S.AuditRuntime(E));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaterInvalid, "CoI.MAP05.UnavailableChainAndNoTeleportCancellation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaterInvalid::RunTest(const FString &)
{
    FSimulation S;
    FCoIGeoMapData G;
    FString E;
    if (!WaterFixture(S, G, E))
        return false;
    auto C = Haul(S);
    C.Transport.RequestedDepartureHour = 168;
    TestFalse(TEXT("Months/week advance ship lock disallowed"), Send(S, C, E));
    TestFalse(TEXT("No failed ship binding"), Find(S.GetState().Vehicles, C.Transport.VehicleId)->Bound);
    TestEqual(TEXT("No failed goods hold"), Find(S.GetState().Inventories, C.Transport.SourceInventoryId)->ReservedKg,
              0LL);
    C = Haul(S, TEXT("water.inland"));
    C.Transport.ToCityId = TEXT("city.de.1913.hannover");
    C.Transport.DestinationStorageId = TEXT("storage.service.city.de.1913.hannover");
    TestFalse(TEXT("Inland destination rejected"), Send(S, C, E));
    TArray<FString> Ids;
    TestFalse(TEXT("Real feeder capacity enforced"),
              WaterFeeders(S.GetScenario().Definitions, S.GetState().Vehicles, TEXT("company.carrier"),
                           TEXT("city.nl.1913.zuid_holland"), TEXT("city.de.1913.rheinprovinz"), 2001, Ids, E));
    // A fresh scenario isolates the actual mid-voyage cancellation from autonomous repositioning.
    FSimulation Running;
    if (!WaterFixture(Running, G, E))
        return false;
    C = Haul(Running);
    if (!Send(Running, C, E))
    {
        AddError(E);
        return false;
    }
    Running.StepOnce();
    auto Cancel = Command(Running, ECommandKind::CancelTransport);
    Cancel.Transport.Id = C.Transport.Id;
    Cancel.Transport.OwnerId = C.Transport.OwnerId;
    TestFalse(TEXT("No cancellation during sailing"), Send(Running, Cancel, E));
    const auto *J = Find(Running.GetState().Transports, C.Transport.Id);
    TestEqual(TEXT("Cargo remains in transit"), J->Status, FString(TEXT("moving")));
    TestNotNull(TEXT("Cargo not teleported"), Find(Running.GetState().Inventories, J->CargoInventoryId));
    TestTrue(TEXT("Rejected cancel preserves invariants"), Running.AuditRuntime(E));
    auto Damaged=Running.GetState();Find(Damaged.Transports,C.Transport.Id)->ChainEdgeIds[3]=TEXT("missing");TestFalse(TEXT("Damaged chain cannot load/audit"),ValidateWaterBookings(Running.GetScenario(),Damaged,E));Damaged=Running.GetState();auto* Bad=Find(Damaged.Transports,C.Transport.Id);Bad->FeederVehicleIds[1]=Bad->FeederVehicleIds[0];TestFalse(TEXT("Same feeder cannot cover both ports"),ValidateWaterBookings(Running.GetScenario(),Damaged,E));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaterEmpty, "CoI.MAP05.OwnShipPaidRepositionAndQuote",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaterEmpty::RunTest(const FString &)
{
    FSimulation S;
    FCoIGeoMapData G;
    FString E;
    if (!WaterFixture(S, G, E, true))
        return false;
    auto C = Haul(S);
    C.Transport.OwnerId = TEXT("company.carrier");
    C.Transport.Empty = true;
    C.Transport.Kg = 0;
    C.Transport.SourceInventoryId.Empty();
    C.Transport.DestinationStorageId.Empty();
    const auto &Sc = S.GetScenario();
    const auto Q = WaterQuote(Sc.Definitions, Sc.Economy, C.Transport.FromCityId, C.Transport.ToCityId, true, 0, 1, 0,
                              TEXT("country.de.1913"), Sc.StartDate);
    TestTrue(TEXT("Paid quote includes handling even when empty"), Q.Terms.Cost > 0);
    if (!TestTrue(TEXT("Paid owner reposition"), Send(S, C, E)))
    {
        AddError(E);
        return false;
    }
    const auto *J = Find(S.GetState().Transports, C.Transport.Id);
    TestEqual(TEXT("Authoritative cost equals preview"), J->CostCents, Q.Terms.Cost);
    const int64 End = J->ArrivalStep + 1;
    while (S.GetState().EconomyHour < End)
        S.StepOnce();
    J = Find(S.GetState().Transports, C.Transport.Id);
    TestEqual(TEXT("Empty arrived"), J->Status, FString(TEXT("arrived")));
    TestTrue(TEXT("No synthetic empty cargo"), J->CargoInventoryId.IsEmpty());
    TestTrue(TEXT("Own-ship audit"), S.AuditRuntime(E));
    return true;
}
#endif

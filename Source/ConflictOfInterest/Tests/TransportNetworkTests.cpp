#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Algo/Reverse.h"
#include "GeoCatalog.h"
#include "Simulation/SimArchive.h"
#include "Simulation/SaveGame.h"
#include "Simulation/DeveloperConsole.h"
using namespace CoISim;
namespace
{
const FString Cologne = TEXT("city.de.1913.rheinprovinz"), Rotterdam = TEXT("city.nl.1913.zuid_holland"),
              German = TEXT("country.de.1913");
bool Fixture(FSimulation &S, FCoIGeoMapData &G, FString &E)
{
    FString J;
    auto *C =
        LoadObject<UCoIGeoCatalog>(nullptr, TEXT("/Game/Geography/Data/GeoCatalog_DE_NL_1913.GeoCatalog_DE_NL_1913"));
    return C && G.Load(C->DefinitionJson, E) &&
           FFileHelper::LoadFileToString(
               J, *(FPaths::ProjectContentDir() / TEXT("Simulation/scenario.de_nl1913.coisim"))) &&
           S.Load(J, G, E);
}
FNetworkPlan Route(const FDefinitions &D, int64 Year = 1913, int64 Kg = 1000, const FString &Country = German)
{
    return PlanWaterRoute(D, Cologne, Rotterdam, Year, Kg, Country);
}
} // namespace
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNetworkRouteTest, "CoI.MAP04.RouteAndDeterminism",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNetworkRouteTest::RunTest(const FString &)
{
    FSimulation S;
    FCoIGeoMapData G;
    FString E;
    if (!TestTrue(*E, Fixture(S, G, E)))
        return false;
    auto D = S.GetScenario().Definitions;
    const auto P = Route(D);
    if (!TestTrue(*P.Reason, P.Valid))
        return false;
    TestEqual(TEXT("Stable network ID"), P.NetworkId, FString(TEXT("network.de_nl.rhine.1913.v1")));
    TestEqual(TEXT("12 water and 4 feeder/handling edges"), P.EdgeIds.Num(), 16);
    TestEqual(TEXT("Geographic configured water meters"), P.WaterMeters, 270917LL);
    TestEqual(TEXT("Terminal bottleneck"), P.CapacityKg, 10000LL);
    TestTrue(TEXT("Paid road distance and handling included"),
             P.Meters > P.WaterMeters && P.Minutes > 240 && P.CostCents > 2100);
    int32 Roads = 0, Transfers = 0;
    for (const auto &Id : P.EdgeIds)
    {
        const auto *Edge = Find(D.Network.Edges, Id);
        Roads += Edge->Mode == TEXT("road");
        Transfers += Edge->Mode == TEXT("transfer");
    }
    TestEqual(TEXT("Two explicit road feeders"), Roads, 2);
    TestEqual(TEXT("Two handling operations"), Transfers, 2);
    const auto Up = PlanWaterRoute(D, Rotterdam, Cologne, 1913, 1000, TEXT("country.nl.1913"));
    TestTrue(TEXT("Explicit Dutch carrier access and upstream route"), Up.Valid);
    TestTrue(TEXT("Upstream slower"), Up.Minutes > P.Minutes);
    TestEqual(TEXT("Same physical distance"), Up.Meters, P.Meters);
    Algo::Reverse(D.Network.Nodes);
    Algo::Reverse(D.Network.Edges);
    const auto Q = Route(D);
    TestTrue(TEXT("Input order cannot change route IDs"), Q.EdgeIds == P.EdgeIds);
    TestEqual(TEXT("Stable time"), Q.Minutes, P.Minutes);
    TestEqual(TEXT("Stable cost"), Q.CostCents, P.CostCents);
    FFileHelper::SaveStringToFile(
        FString::Printf(TEXT("network=%s\nmeters=%lld\nwater_meters=%lld\nminutes_down=%lld\nminutes_up=%lld\ncost_"
                             "1000kg_cents=%lld\ncapacity_kg=%lld\nedges=%s\n"),
                        *P.NetworkId, P.Meters, P.WaterMeters, P.Minutes, Up.Minutes, P.CostCents, P.CapacityKg,
                        *FString::Join(P.EdgeIds, TEXT(" -> "))),
        *(FPaths::ProjectSavedDir() / TEXT("MAP04_route.txt")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNetworkRejectTest, "CoI.MAP04.AccessCapacityDatesAndClosures",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNetworkRejectTest::RunTest(const FString &)
{
    FSimulation S;
    FCoIGeoMapData G;
    FString E;
    if (!Fixture(S, G, E))
    {
        AddError(E);
        return false;
    }
    const auto D = S.GetScenario().Definitions;
    TestFalse(TEXT("Inland hub has no direct water access"),
              PlanWaterRoute(D, TEXT("city.de.1913.hannover"), Rotterdam, 1913, 1000, German).Valid);
    TestFalse(TEXT("No earlier historical clearance"), Route(D, 1912).Valid);
    TestFalse(TEXT("No unresearched later clearance"), Route(D, 1914).Valid);
    TestTrue(TEXT("Exact bottleneck fits"), Route(D, 1913, 10000).Valid);
    TestFalse(TEXT("Bottleneck overflow"), Route(D, 1913, 10001).Valid);
    TestFalse(TEXT("Zero freight"), Route(D, 1913, 0).Valid);
    TestFalse(TEXT("Unknown foreign carrier"), Route(D, 1913, 1000, TEXT("country.be.1913")).Valid);
    auto X = D;
    Find(X.Network.Edges, FString(TEXT("port.koeln.load")))->Enabled = false;
    TestTrue(TEXT("Closed handling is valid config"), ValidateTransportNetwork(X, E));
    TestFalse(TEXT("Closed handling prevents route"), Route(X).Valid);
    X = D;
    Find(X.Network.Edges, FString(TEXT("reach.duisburg.wesel.down")))->Enabled = false;
    TestTrue(TEXT("Closed reach is valid"), ValidateTransportNetwork(X, E));
    TestFalse(TEXT("Closed reach prevents route, candidates no shortcut"), Route(X).Valid);
    X = D;
    X.Network.Ports[0].TransferOutId = TEXT("missing");
    TestFalse(TEXT("Missing handling reference rejected"), ValidateTransportNetwork(X, E));
    TestFalse(TEXT("Missing handling never dereferenced"), Route(X).Valid);
    X = D;
    X.Network.Access[0].Carriers.Remove(German);
    TestTrue(TEXT("Restricted access valid"), ValidateTransportNetwork(X, E));
    TestFalse(TEXT("Explicit restriction enforced"), Route(X).Valid);
    TestTrue(TEXT("Diagnosis explains absent hubs and unsupported modes"),
             TransportNetworkDiagnostics(D, 1913, German).Contains(TEXT("48 Hubs")) &&
                 TransportNetworkDiagnostics(D, 1913, German).Contains(TEXT("unbelegter Abschnitt")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNetworkInvalidTest, "CoI.MAP04.InvalidGeographyModesAndEvidence",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNetworkInvalidTest::RunTest(const FString &)
{
    FSimulation S;
    FCoIGeoMapData G;
    FString E;
    if (!Fixture(S, G, E))
        return false;
    const auto D = S.GetScenario().Definitions;
    auto X = D;
    X.Network.Edges[0].LengthMeters += 100;
    TestFalse(TEXT("Screen/arbitrary distance rejected"), ValidateTransportNetwork(X, E));
    X = D;
    X.Network.Edges[0].Geometry[0].Latitude += 1;
    TestFalse(TEXT("Endpoint mismatch rejected"), ValidateTransportNetwork(X, E));
    X = D;
    X.Network.Edges[0].Mode = TEXT("air");
    TestFalse(TEXT("Unallowed mode diagnosed"), ValidateTransportNetwork(X, E));
    X = D;
    Find(X.Network.Edges, FString(TEXT("candidate.north_sea")))->Enabled = true;
    TestFalse(TEXT("Unproven sea reach cannot activate"), ValidateTransportNetwork(X, E));
    X = D;
    X.Network.Access[0].Countries.Add(TEXT("country.be.1913"));
    TestFalse(TEXT("Unsimulated country is no free transit"), ValidateTransportNetwork(X, E));
    X = D;
    X.Network.Nodes[0].Id = X.Network.Nodes[1].Id;
    TestFalse(TEXT("Duplicate node IDs"), ValidateTransportNetwork(X, E));
    X = D;
    auto Copy = *Find(X.Network.Nodes, FString(TEXT("water.duisburg")));
    Copy.Id = TEXT("water.coincident.unconnected");
    X.Network.Nodes.Add(Copy);
    Find(X.Network.Edges, FString(TEXT("reach.duisburg.wesel.down")))->FromId = Copy.Id;
    TestTrue(TEXT("Coincident but distinct junction geometry valid"), ValidateTransportNetwork(X, E));
    TestFalse(TEXT("Coincident geometry creates no implicit connection"), Route(X).Valid);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNetworkPureTest, "CoI.MAP04.ReadOnlyPreviewAndSameVersionSave",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNetworkPureTest::RunTest(const FString &)
{
    FSimulation S;
    FCoIGeoMapData G;
    FString E;
    if (!Fixture(S, G, E))
        return false;
    const auto Before = S.EconomicFingerprint();
    const auto Step = S.GetState().Step;
    for (int I = 0; I < 30; ++I)
        TestTrue(TEXT("Repeated preview"), Route(S.GetScenario().Definitions).Valid);
#if COI_WITH_DEV_CONSOLE
    FDeveloperConsole C;
    TestTrue(TEXT("Read-only diagnostic command"), C.Execute(S, TEXT("network status")));
    TestTrue(TEXT("Read-only route command"),
             C.Execute(S, TEXT("network route city.de.1913.rheinprovinz city.nl.1913.zuid_holland 1000 1913")));
    TestFalse(TEXT("Invalid quantity command rejected"),
              C.Execute(S, TEXT("network route city.de.1913.rheinprovinz city.nl.1913.zuid_holland nope")));
#endif
    TestEqual(TEXT("Preview gives no money/goods or reservations"), S.EconomicFingerprint(), Before);
    TestEqual(TEXT("Preview advances no time"), S.GetState().Step, Step);
    TestFalse(TEXT("No developer intervention"), S.GetState().DeveloperModified);
    FString Save;
    if (!TestTrue(TEXT("Save current version"), S.SaveGameJson(Save, E)))
        return false;
    FSimulation Restored;
    int32 Speed = 1;
    if (!TestTrue(*E, Restored.LoadGameJson(Save, G, S.GetScenario(), E, Speed)))
        return false;
    TestEqual(TEXT("Same version retains economy"), Restored.EconomicFingerprint(), Before);
    TestTrue(TEXT("Same route IDs after load"),
             Route(Restored.GetScenario().Definitions).EdgeIds == Route(S.GetScenario().Definitions).EdgeIds);
    for (int H = 0; H < 24; ++H)
    {
        S.StepOnce();
        Restored.StepOnce();
    }
    TestEqual(TEXT("Identical next-day continuation"), S.EconomicFingerprint(), Restored.EconomicFingerprint());
    TestTrue(TEXT("Runtime invariants"), S.AuditRuntime(E));
    return true;
}
#endif

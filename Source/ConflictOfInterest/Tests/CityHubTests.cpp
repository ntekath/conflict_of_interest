#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GeoCatalog.h"
#include "Simulation/CityHubs.h"
#include "Simulation/SimCore.h"
#include "Simulation/SimArchive.h"
#include "Serialization/JsonSerializer.h"
using namespace CoISim;
namespace {
bool HubFixture(FString& J,FCoIGeoMapData& G){FString E;auto* C=LoadObject<UCoIGeoCatalog>(nullptr,TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));return C&&G.Load(C->DefinitionJson,E)&&FFileHelper::LoadFileToString(J,*(FPaths::ProjectContentDir()/TEXT("Simulation/ap092_legacy_fixture.coisim")));}
FString HubJson(TSharedPtr<FJsonObject> R){FString J;FJsonSerializer::Serialize(R.ToSharedRef(),TJsonWriterFactory<>::Create(&J));return J;}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCityHubMigration,"CoI.Hubs.MigrationAndTransit",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCityHubMigration::RunTest(const FString&){FString J,E;FCoIGeoMapData G;if(!HubFixture(J,G))return false;TSharedPtr<FJsonObject> Root;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(J),Root);
 Root->RemoveField(TEXT("HubSchema"));auto D=Root->GetObjectField(TEXT("Definitions"));D->RemoveField(TEXT("ResourceSites"));for(auto X:D->GetArrayField(TEXT("Companies")))X->AsObject()->RemoveField(TEXT("HeadquartersCityId"));auto W=Root->GetObjectField(TEXT("InitialState"));for(auto X:W->GetArrayField(TEXT("Vehicles")))X->AsObject()->RemoveField(TEXT("HomeCityId"));for(auto X:W->GetArrayField(TEXT("Facilities"))){X->AsObject()->RemoveField(TEXT("ResourceSiteId"));X->AsObject()->RemoveField(TEXT("ResourceBatchesPerHour"));}
 FSimulation A,B;if(!TestTrue(TEXT("Versioned hubs load"),A.Load(J,G,E))||!TestTrue(TEXT("Legacy migration load"),B.Load(HubJson(Root),G,E)))return false;
 TestEqual(TEXT("Migration preserves entire economic state"),A.EconomicFingerprint(),B.EconomicFingerprint());for(int H=0;H<168;++H){A.StepOnce();B.StepOnce();}TestEqual(TEXT("168-hour continuation identical"),A.EconomicFingerprint(),B.EconomicFingerprint());TestTrue(TEXT("Resource conservation"),A.AuditRuntime(E));
 FCommand Trip;Trip.Kind=ECommandKind::BookTransport;Trip.Generation=A.GetGeneration();Trip.Sequence=A.NextSequence();Trip.TargetStep=A.GetState().Step+1;Trip.Transport.Id=TEXT("hub.transit");Trip.Transport.OwnerId=TEXT("company.player");Trip.Transport.VehicleId=TEXT("vehicle.player");Trip.Transport.FromCityId=TEXT("city.us.detroit");Trip.Transport.ToCityId=TEXT("city.us.chicago");Trip.Transport.Empty=true;
 TestTrue(TEXT("Queue existing transport"),A.Enqueue(Trip,E));A.StepOnce();TestTrue(TEXT("Transport accepted"),A.GetResults().Last().Accepted);A.StepOnce();const auto* V=Find(A.GetState().Vehicles,FString(TEXT("vehicle.player")));TestEqual(TEXT("Home unchanged during transit"),V->HomeCityId,FString(TEXT("city.us.detroit")));TestTrue(TEXT("Moving vehicle is in no city"),V->CityId.IsEmpty());TestTrue(TEXT("Transit hub validation"),A.AuditRuntime(E));for(int H=0;H<100&&V->Bound;++H){A.StepOnce();V=Find(A.GetState().Vehicles,FString(TEXT("vehicle.player")));}TestEqual(TEXT("Arrival at exactly one destination"),V->CityId,FString(TEXT("city.us.chicago")));TestEqual(TEXT("Home still original hub"),V->HomeCityId,FString(TEXT("city.us.detroit")));return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCityHubAssignments,"CoI.Hubs.AssignmentsAndResourceLimits",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCityHubAssignments::RunTest(const FString&){FString J,E;FCoIGeoMapData G;if(!HubFixture(J,G))return false;FScenario S;if(!LoadScenario(J,G,S,E))return false;
 TSet<FString> Firms,Cities;for(const auto& W:S.InitialState.Warehouses){if(W.CityId==TEXT("city.us.detroit"))Firms.Add(W.OwnerId);}TestTrue(TEXT("Shared city, multiple firms"),Firms.Num()>1);for(const auto& V:S.InitialState.Vehicles)if(V.OwnerId==TEXT("company.carrier"))Cities.Add(V.HomeCityId);TestTrue(TEXT("One company in multiple cities"),Cities.Num()>1);
 auto Reject=[&](FScenario Bad,const FString& Why){FString Error;TestFalse(*Why,ValidateScenario(Bad,G,Error));TestFalse(TEXT("Clear diagnostic"),Error.IsEmpty());};
 auto Bad=S;Bad.InitialState.Vehicles[0].HomeCityId=TEXT("missing");Reject(Bad,TEXT("Invalid home"));Bad=S;Bad.Definitions.Companies[0].HeadquartersCityId=TEXT("missing");Reject(Bad,TEXT("Invalid headquarters"));Bad=S;Bad.InitialState.Facilities[0].ResourceBatchesPerHour=2;Reject(Bad,TEXT("Overclaimed site"));Bad=S;Bad.InitialState.Facilities[0].ResourceSiteId=TEXT("site|city.us.pittsburgh|ore");Reject(Bad,TEXT("Wrong resource and hub"));Bad=S;for(auto& X:Bad.Definitions.ResourceSites)if(X.Kind==TEXT("forest")){++X.InitialKg;break;}Reject(Bad,TEXT("Duplicated regional source"));
 return true;
}
#endif

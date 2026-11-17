#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GeoCatalog.h"
#include "Simulation/SimCore.h"
#include "Simulation/SimArchive.h"
#include "Simulation/RouteDisplay.h"
#include "Serialization/JsonSerializer.h"
using namespace CoISim;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRouteDisplayTest,"CoI.Routes.PrivacyLimitsAndTime",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRouteDisplayTest::RunTest(const FString&){FString Json,E;FCoIGeoMapData Geo;auto* Catalog=LoadObject<UCoIGeoCatalog>(nullptr,TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));if(!Catalog||!Geo.Load(Catalog->DefinitionJson,E)||!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("Simulation/scenario.coisim"))))return false;FSimulation S;if(!S.Load(Json,Geo,E)){AddError(E);return false;}
 FCommand C;C.Kind=ECommandKind::BookTransport;C.Generation=S.GetGeneration();C.TargetStep=1;C.Sequence=S.NextSequence();C.Transport.Id=TEXT("route.test.empty");C.Transport.OwnerId=S.GetSession().PlayerCompanyId;C.Transport.VehicleId=TEXT("vehicle.player");C.Transport.FromCityId=TEXT("city.us.detroit");C.Transport.ToCityId=TEXT("city.us.chicago");C.Transport.Empty=true;C.Transport.RequestedDepartureHour=1;TestTrue(TEXT("Real own empty trip booked, no fake assets"),S.Enqueue(C,E));for(int H=0;H<4;++H)S.StepOnce();TestTrue(TEXT("Booking accepted"),S.GetResults().Last().Accepted);
 bool Own=false,Public=false;for(const auto& R:S.GetSnapshot().Player.Routes){if(R.Own){Own=true;TestEqual(TEXT("Own route links real own job"),R.OwnJobs[0],C.Transport.Id);TestEqual(TEXT("Empty own route has zero cargo"),R.OwnKg,0LL);}else {Public=true;TestTrue(TEXT("Public activity has no foreign job IDs"),R.OwnJobs.IsEmpty());TestEqual(TEXT("No foreign cargo"),R.OwnKg,0LL);TestEqual(TEXT("No foreign arrival schedule"),R.NextArrival,0LL);}}
 TestTrue(TEXT("Own route published"),Own);for(int H=4;H<72;++H)S.StepOnce();for(const auto& R:S.GetSnapshot().Player.Routes)if(!R.Own)Public=true;TestTrue(TEXT("Autonomous public activity exists"),Public);
 FRouteActivity R;R.Id=TEXT("public.test");R.Moving=500;FEconomyRules Rules;TestEqual(TEXT("Three examples per route"),CoIRoutes::TruckCount(R,Rules,10000,100,true),3);TestEqual(TEXT("Global remaining budget respected"),CoIRoutes::TruckCount(R,Rules,10000,1,true),1);TestEqual(TEXT("Overview hides examples"),CoIRoutes::TruckCount(R,Rules,20000,100,true),0);TestEqual(TEXT("Disabled animation has no examples"),CoIRoutes::TruckCount(R,Rules,10000,100,false),0);
 TestEqual(TEXT("Own phase is simulation-time fraction"),CoIRoutes::Progress(5,1,9),.5);TestEqual(TEXT("Before departure"),CoIRoutes::Progress(0,1,9),0.);TestEqual(TEXT("After arrival"),CoIRoutes::Progress(20,1,9),1.);
 const auto Before=S.Fingerprint();double Phase=CoIRoutes::ExamplePhase(R,0,S.GetSnapshot().Player);S.Advance(1000000,16,true);TestEqual(TEXT("Pause freezes public representative phase"),CoIRoutes::ExamplePhase(R,0,S.GetSnapshot().Player),Phase);
 for(int I=0;I<1000;++I){CoIRoutes::TruckCount(R,Rules,I%2?1000:22000,100,I%3!=0);CoIRoutes::Point({0,0},{100,100},.5,true,I%2!=0);}TestEqual(TEXT("Camera/animation/display never change economic state"),S.Fingerprint(),Before);TestTrue(TEXT("No duplicate delivery or unbacked money"),S.AuditRuntime(E));
 TestEqual(TEXT("Near crossing geometry distance"),CoIRoutes::Distance({50,51},{0,0},{100,100}),FMath::Sqrt(.5));
 TSharedPtr<FJsonObject> Root;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root);Root->GetObjectField(TEXT("Economy"))->SetBoolField(TEXT("MapPublicRouteActivity"),false);FString PrivateJson;FJsonSerializer::Serialize(Root.ToSharedRef(),TJsonWriterFactory<>::Create(&PrivateJson));FSimulation Private;TestTrue(TEXT("Public policy is scenario controlled"),Private.Load(PrivateJson,Geo,E));for(int H=0;H<72;++H)Private.StepOnce();for(const auto& X:Private.GetSnapshot().Player.Routes)TestTrue(TEXT("Disabled public policy publishes no foreign routes"),X.Own);return true;}
#endif

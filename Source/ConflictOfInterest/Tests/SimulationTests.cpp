#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GeoCatalog.h"
#include "Simulation/SimCore.h"
#include "Simulation/SimArchive.h"
#include "Serialization/JsonSerializer.h"
using namespace CoISim;
namespace
{
bool Fixture(FString& Json,FCoIGeoMapData& Geo)
{
 auto* C=LoadObject<UCoIGeoCatalog>(nullptr,TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));FString Error;
 if(!C||!Geo.Load(C->DefinitionJson,Error)||!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("Simulation/scenario.coisim"))))return false;
 TSharedPtr<FJsonObject> Root;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root);Root->GetObjectField(TEXT("Economy"))->SetNumberField(TEXT("Stage"),0);
 Json.Empty();FJsonSerializer::Serialize(Root.ToSharedRef(),TJsonWriterFactory<>::Create(&Json));return true;
}
FString Edited(const FString& Json,TFunction<void(TSharedPtr<FJsonObject>)> Edit)
 {TSharedPtr<FJsonObject> Root;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root);Edit(Root);FString Out;FJsonSerializer::Serialize(Root.ToSharedRef(),TJsonWriterFactory<>::Create(&Out));return Out;}
FCommand Reservation(FSimulation& Sim,int64 Seq,int64 Target,const FString& Id)
{
 FCommand C;C.Generation=Sim.GetGeneration();C.Sequence=Seq;C.TargetStep=Target;C.Resources.Id=Id;C.Resources.ActorId=TEXT("company.player");
 C.Resources.AccountId=TEXT("account.company.player");C.Resources.InventoryId=TEXT("stock.player.food");C.Resources.WarehouseId=TEXT("storage.player");C.Resources.VehicleId=TEXT("vehicle.player");
 C.Resources.MoneyCents=100;C.Resources.Kg=70;C.Resources.IncomingPoints=100;return C;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoISimContract,"CoI.Simulation.ContractAndValidation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCoISimContract::RunTest(const FString& Parameters)
{
 FString Json,Error;FCoIGeoMapData Geo;if(!TestTrue(TEXT("Fixture available"),Fixture(Json,Geo)))return false;
 FSimulation Sim;if(!TestTrue(TEXT("Strict scenario validation"),Sim.Load(Json,Geo,Error))){AddError(Error);return false;}
 TestEqual(TEXT("Five configured goods"),Sim.GetScenario().Definitions.Goods.Num(),5);
 TestEqual(TEXT("Three groups in each of four regions"),Sim.GetState().Groups.Num(),12);
 TestEqual(TEXT("Population aggregates only regional groups"),Sim.GetSnapshot().Population,40000LL);
 TestEqual(TEXT("Declared initial employment aggregates exactly once"),Sim.GetSnapshot().Employed,20LL);
 TestEqual(TEXT("Exactly one player truck"),Sim.GetSnapshot().PlayerTrucks,1LL);
 TestEqual(TEXT("Configured starting capital"),Sim.GetSnapshot().PlayerMoneyCents,2500000LL);
 TestEqual(TEXT("Bounded player storage"),Sim.GetSnapshot().PlayerStorageCapacity,10000LL);
 TestTrue(TEXT("Start date is configurable 1913"),Sim.GetSnapshot().Date.StartsWith(TEXT("01.01.1913 00:00")));
 TestTrue(TEXT("Typed schema fixtures, no live trades"),!Sim.GetScenario().ContractExamples.Trades.IsEmpty()&&Sim.GetState().Trades.IsEmpty()&&Sim.GetState().Orders.IsEmpty()&&Sim.GetState().Transports.IsEmpty());
 FString Original=Sim.Fingerprint();
 auto Reject=[&](const TCHAR* What,TFunction<void(TSharedPtr<FJsonObject>)> Edit)
 {TestFalse(What,Sim.Load(Edited(Json,Edit),Geo,Error));TestEqual(TEXT("Failed load preserves authoritative state"),Sim.Fingerprint(),Original);};
 Reject(TEXT("Missing reference"),[](auto R){R->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Vehicles"))[0]->AsObject()->SetStringField(TEXT("TypeId"),TEXT("missing"));});
 Reject(TEXT("Unbacked funds reservation"),[](auto R){R->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Accounts"))[0]->AsObject()->SetNumberField(TEXT("ReservedCents"),1);});
 Reject(TEXT("Negative stock"),[](auto R){R->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Inventories"))[0]->AsObject()->SetNumberField(TEXT("QuantityKg"),-1);});
 Reject(TEXT("Fractional kg"),[](auto R){R->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Inventories"))[0]->AsObject()->SetNumberField(TEXT("QuantityKg"),.5);});
 Reject(TEXT("No free infinite warehouse"),[](auto R){R->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("WarehouseTypes"))[0]->AsObject()->SetNumberField(TEXT("DailyCostCents"),0);});
 Reject(TEXT("Warehouse overflow"),[](auto R){R->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("WarehouseTypes"))[0]->AsObject()->SetNumberField(TEXT("CapacityPoints"),1);});
 Reject(TEXT("Missing geographic resource"),[](auto R){for(auto& V:R->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("Regions")))V->AsObject()->SetNumberField(TEXT("OreKg"),0);});
 Reject(TEXT("No duplicate employment"),[](auto R){R->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Groups"))[0]->AsObject()->SetNumberField(TEXT("Employed"),1);});
 Reject(TEXT("Invalid step duration"),[](auto R){R->GetObjectField(TEXT("Settings"))->SetNumberField(TEXT("HoursPerStep"),0);});
 Reject(TEXT("Invalid date"),[](auto R){R->SetStringField(TEXT("StartDate"),TEXT("1913-02-30"));});
 Reject(TEXT("Missing required field"),[](auto R){R->RemoveField(TEXT("Settings"));});
 FString Consumed=Edited(Json,[](auto R){R->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Groups"))[0]->AsObject()->GetArrayField(TEXT("Consumed"))[0]->AsObject()->SetNumberField(TEXT("Kg"),17);});
 TestTrue(TEXT("Configured group consumption accepted"),Sim.Load(Consumed,Geo,Error));
 int64 CountryConsumption=0;for(const auto& C:Sim.GetSnapshot().Countries)for(const auto& A:C.Consumed)CountryConsumption+=A.Kg;
 TestEqual(TEXT("Country consumption is not counted twice"),CountryConsumption,17LL);
 FString Calendar=Edited(Json,[](auto R){R->SetStringField(TEXT("StartDate"),TEXT("1916-02-28"));R->GetObjectField(TEXT("Settings"))->SetNumberField(TEXT("HoursPerStep"),2);});
 TestTrue(TEXT("Configurable date and fixed-step duration"),Sim.Load(Calendar,Geo,Error));for(int I=0;I<12;++I)Sim.StepOnce();
 TestTrue(TEXT("Calendar advances across leap day"),Sim.GetSnapshot().Date.StartsWith(TEXT("29.02.1916 00:00")));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoISimDeterminism,"CoI.Simulation.FixedStepsAndPause",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCoISimDeterminism::RunTest(const FString& Parameters)
{
 FString Json,Error;FCoIGeoMapData Geo;if(!TestTrue(TEXT("Fixture"),Fixture(Json,Geo)))return false;
 FSimulation A,B,C;for(auto* Sim:{&A,&B,&C})
 {
  if(!TestTrue(TEXT("Scenario"),Sim->Load(Json,Geo,Error)))return false;
  auto First=Reservation(*Sim,1,2,TEXT("hold.first"));TestTrue(TEXT("First command queued"),Sim->Enqueue(First,Error));
  auto Rival=Reservation(*Sim,2,2,TEXT("hold.rival"));TestTrue(TEXT("Conflicting command queued"),Sim->Enqueue(Rival,Error));
  auto Release=Reservation(*Sim,3,5,TEXT("hold.first"));Release.Kind=ECommandKind::Release;TestTrue(TEXT("Release queued"),Sim->Enqueue(Release,Error));
 }
 FString PausedHash=A.Fingerprint();auto Version=A.GetSnapshot().Version;
 for(int I=0;I<100;++I)A.Advance(1000000,16,true);
 TestEqual(TEXT("Paused state, pending commands and funds unchanged"),A.Fingerprint(),PausedHash);TestEqual(TEXT("Pause publishes no new state"),A.GetSnapshot().Version,Version);
 // Equal simulated hours at different frame rates and speed multipliers.
 for(int I=0;I<24000;++I)A.Advance(10000,1,false);
 for(int I=0;I<2400;++I)B.Advance(25000,4,false);
 for(int I=0;I<150;++I)C.Advance(100000,16,false);
 TestEqual(TEXT("1x hours"),A.GetState().Step,240LL);TestEqual(TEXT("4x hours"),B.GetState().Step,240LL);TestEqual(TEXT("16x hours"),C.GetState().Step,240LL);
 TestEqual(TEXT("Identical rules across FPS and 4x"),A.Fingerprint(),B.Fingerprint());TestEqual(TEXT("Identical rules across FPS and 16x"),A.Fingerprint(),C.Fingerprint());
 for(const auto& R:A.GetState().Regions)TestEqual(TEXT("Every region advances without any visible actor"),R.LastProcessedStep,240LL);
 TestEqual(TEXT("Exactly three commands executed"),A.GetResults().Num(),3);TestTrue(TEXT("First wins then second rejects then release succeeds"),A.GetResults()[0].Accepted&&!A.GetResults()[1].Accepted&&A.GetResults()[2].Accepted);
 TestEqual(TEXT("No economic rules create hidden money"),A.GetSnapshot().PlayerMoneyCents,2500000LL);
 TestEqual(TEXT("No production/consumption before AP05"),Find(A.GetState().Inventories,TEXT("stock.player.food"))->QuantityKg,100LL);
 TestTrue(TEXT("Seed and ID key independent streams"),EntityRandom(1913,7,TEXT("a"))!=EntityRandom(1913,7,TEXT("b"))&&EntityRandom(1913,7,TEXT("a"))!=EntityRandom(1914,7,TEXT("a")));
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoISimCommands,"CoI.Simulation.AtomicCommandsAndLifetime",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCoISimCommands::RunTest(const FString& Parameters)
{
 FString Json,Error;FCoIGeoMapData Geo;if(!TestTrue(TEXT("Fixture"),Fixture(Json,Geo)))return false;FSimulation Sim;
 if(!TestTrue(TEXT("Scenario"),Sim.Load(Json,Geo,Error)))return false;
 FSimulation Other;if(!TestTrue(TEXT("Independent scenario instance"),Other.Load(Json,Geo,Error)))return false;
 TestFalse(TEXT("Command from another newly-created world is stale"),Other.Enqueue(Reservation(Sim,1,1,TEXT("hold.cross_world")),Error));
 auto One=Reservation(Sim,1,1,TEXT("hold.one"));auto Two=Reservation(Sim,2,1,TEXT("hold.two"));
 TestTrue(TEXT("Enqueue one"),Sim.Enqueue(One,Error));TestFalse(TEXT("Duplicate sequence cannot be queued"),Sim.Enqueue(One,Error));TestTrue(TEXT("Enqueue two"),Sim.Enqueue(Two,Error));
 Sim.StepOnce();
 TestEqual(TEXT("Only first debits reservation counters"),Sim.GetSnapshot().PlayerReservedCents,100LL);
 TestEqual(TEXT("Only 70kg reserved"),Find(Sim.GetState().Inventories,TEXT("stock.player.food"))->ReservedKg,70LL);
 TestEqual(TEXT("Destination capacity reserved atomically"),Find(Sim.GetState().Warehouses,TEXT("storage.player"))->ReservedIncomingPoints,100LL);
 TestTrue(TEXT("Single vehicle binding"),Find(Sim.GetState().Vehicles,TEXT("vehicle.player"))->Bound);
 auto Wrong=Reservation(Sim,3,2,TEXT("hold.one"));Wrong.Kind=ECommandKind::Release;Wrong.Resources.ActorId=TEXT("company.merchant");Sim.Enqueue(Wrong,Error);Sim.StepOnce();
 TestFalse(TEXT("Other actor cannot release ownership reservation"),Sim.GetResults().Last().Accepted);
 auto Release=Reservation(Sim,4,3,TEXT("hold.one"));Release.Kind=ECommandKind::Release;Sim.Enqueue(Release,Error);Sim.StepOnce();
 TestEqual(TEXT("Funds released"),Sim.GetSnapshot().PlayerReservedCents,0LL);TestEqual(TEXT("Stock released"),Find(Sim.GetState().Inventories,TEXT("stock.player.food"))->ReservedKg,0LL);
 TestFalse(TEXT("Vehicle available after release"),Find(Sim.GetState().Vehicles,TEXT("vehicle.player"))->Bound);
 auto Insufficient=Reservation(Sim,5,4,TEXT("hold.too_much"));Insufficient.Resources.MoneyCents=2500001;Sim.Enqueue(Insufficient,Error);Sim.StepOnce();
 TestFalse(TEXT("Insufficient money rejects whole bundle"),Sim.GetResults().Last().Accepted);TestEqual(TEXT("Rejected bundle leaves stock unchanged"),Find(Sim.GetState().Inventories,TEXT("stock.player.food"))->ReservedKg,0LL);
 auto OldGeneration=Reservation(Sim,6,5,TEXT("hold.old"));auto PreviousGeneration=Sim.GetGeneration();Sim.Shutdown();
 TestFalse(TEXT("Stopped instance cannot step"),Sim.StepOnce());TestTrue(TEXT("Generation advances"),Sim.GetGeneration()>PreviousGeneration);
 Sim.Load(Json,Geo,Error);TestFalse(TEXT("Previous scenario command rejected after reload"),Sim.Enqueue(OldGeneration,Error));TestEqual(TEXT("New scenario is clean"),Sim.GetState().Step,0LL);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoISimBudget,"CoI.Simulation.BoundedBacklogAndOrdering",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCoISimBudget::RunTest(const FString& Parameters)
{
 FString Json,Error;FCoIGeoMapData Geo;if(!TestTrue(TEXT("Fixture"),Fixture(Json,Geo)))return false;
 Json=Edited(Json,[](auto R){R->GetObjectField(TEXT("Settings"))->SetNumberField(TEXT("MaxPendingCommands"),2);R->GetObjectField(TEXT("Settings"))->SetNumberField(TEXT("StepBudgetMicros"),1);});
 FSimulation Sim;if(!TestTrue(TEXT("Budget fixture validates"),Sim.Load(Json,Geo,Error)))return false;
 auto Future=Reservation(Sim,1,3,TEXT("hold.future"));auto Near=Reservation(Sim,2,1,TEXT("hold.near"));Near.Resources.Kg=10;
 TestTrue(TEXT("Future accepted"),Sim.Enqueue(Future,Error));TestTrue(TEXT("Earlier target accepted after future sequence"),Sim.Enqueue(Near,Error));
 TestFalse(TEXT("Queue limit enforced"),Sim.Enqueue(Reservation(Sim,3,1,TEXT("hold.full")),Error));
 Sim.Advance(MAX_int64,16,false);TestTrue(TEXT("Per-frame work bounded"),Sim.GetDiagnostics().StepsThisFrame>=1&&Sim.GetDiagnostics().StepsThisFrame<=4);
 TestTrue(TEXT("Bounded accumulated work"),Sim.GetDiagnostics().BacklogSteps<=8&&Sim.GetDiagnostics().ThrottledFrames==1);
 TestEqual(TEXT("Target step takes priority over arrival sequence"),Sim.GetResults()[0].Sequence,2LL);
 const auto Before=Sim.GetState().Step;Sim.Advance(MAX_int64,16,false);TestTrue(TEXT("No step-number jump during overload"),Sim.GetState().Step-Before<=4);
 while(Sim.GetState().Step<3)Sim.StepOnce();
 TestEqual(TEXT("Commands execute once in target order"),Sim.GetResults().Num(),2);TestEqual(TEXT("Earlier-arriving future command stays after near command"),Sim.GetResults()[1].Sequence,1LL);
 FString Hash=Sim.Fingerprint();for(int I=0;I<5;++I)Sim.Advance(MAX_int64,16,true);TestEqual(TEXT("Pause freezes even with backlog"),Sim.Fingerprint(),Hash);
 return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GeoCatalog.h"
#include "Simulation/SimCore.h"
#include "Simulation/SimArchive.h"
#include "Serialization/JsonSerializer.h"
using namespace CoISim;
namespace {
bool Fixture(FString& J,FCoIGeoMapData& G,int Stage,TFunction<void(TSharedPtr<FJsonObject>)> Edit={}) {
 auto* C=LoadObject<UCoIGeoCatalog>(nullptr,TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));FString E;
 if(!C||!G.Load(C->DefinitionJson,E)||!FFileHelper::LoadFileToString(J,*(FPaths::ProjectContentDir()/TEXT("Simulation/scenario.coisim"))))return false;
 TSharedPtr<FJsonObject> R;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(J),R);R->GetObjectField(TEXT("Economy"))->SetNumberField(TEXT("Stage"),Stage);if(Edit)Edit(R);
 J.Empty();FJsonSerializer::Serialize(R.ToSharedRef(),TJsonWriterFactory<>::Create(&J));return true;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProduction,"CoI.Economy.ProductionPayroll",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FProduction::RunTest(const FString&) {
 FString J,E;FCoIGeoMapData G;TestTrue(TEXT("Fixture"),Fixture(J,G,1));FSimulation S; if(!TestTrue(TEXT("Load"),S.Load(J,G,E))){AddError(E);return false;}
 const auto Before=S.GetState();S.StepOnce();
 TestEqual(TEXT("100 jobs funded by employers"),S.GetSnapshot().Employed,100LL);
 TestEqual(TEXT("Food employer prepays 20 x 150 cents"),Find(S.GetState().Accounts,FString(TEXT("account.company.food")))->BalanceCents,Find(Before.Accounts,FString(TEXT("account.company.food")))->BalanceCents-3000);
 TestEqual(TEXT("Tools recipe consumes 100 ore"),Find(S.GetState().Inventories,FString(TEXT("stock.tools.ore")))->QuantityKg,900LL);
 for(int H=1;H<24;++H)S.StepOnce();
 TestEqual(TEXT("80 percent farmland: 19 batches in 24 hours"),Find(S.GetState().GoodsAudit,FString(TEXT("good.food")))->ProducedKg,1900LL);
 TestEqual(TEXT("Inputs cap tools output at ten batches"),Find(S.GetState().GoodsAudit,FString(TEXT("good.tools")))->ProducedKg,400LL);
 TestTrue(TEXT("All ledger invariants"),S.AuditRuntime(E));
 TestTrue(TEXT("Unfunded fixture"),Fixture(J,G,1,[](auto R){for(auto V:R->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Accounts")))if(V->AsObject()->GetStringField(TEXT("OwnerId"))==TEXT("company.food"))V->AsObject()->SetNumberField(TEXT("BalanceCents"),0);for(auto V:R->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("Regions")))if(V->AsObject()->GetNumberField(TEXT("OreKg"))>0)V->AsObject()->SetNumberField(TEXT("OreKg"),50);}));
 TestTrue(TEXT("Load poor employer"),S.Load(J,G,E));for(int H=0;H<48;++H)S.StepOnce();
 TestEqual(TEXT("No paid workers no food"),Find(S.GetState().GoodsAudit,FString(TEXT("good.food")))->ProducedKg,0LL);
 TestEqual(TEXT("Insufficient finite ore no extraction batch"),Find(S.GetState().GoodsAudit,FString(TEXT("good.ore")))->ProducedKg,0LL);
 TestTrue(TEXT("No hidden funding or goods"),S.AuditRuntime(E));return true;
}
#endif

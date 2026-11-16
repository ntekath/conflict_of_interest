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

namespace {
FCommand Order(FSimulation& S,const TCHAR* Id,const TCHAR* Actor,const TCHAR* Side,int64 Kg,int64 Price,const TCHAR* Store=TEXT("storage.player"),const TCHAR* City=TEXT("city.us.detroit")) {
 FCommand C;C.Generation=S.GetGeneration();C.Sequence=S.NextSequence();C.TargetStep=S.GetState().Step+1;C.Kind=ECommandKind::PlaceOrder;
 auto& O=C.Order;O.Id=Id;O.ActorId=Actor;O.Side=Side;O.OriginalKg=Kg;O.LimitCentsPerKg=Price;O.StorageId=Store;O.CityId=City;O.GoodId=TEXT("good.food");O.MarketId=TEXT("market.")+FString(City).RightChop(8);return C;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarket,"CoI.Economy.OrderSettlement",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarket::RunTest(const FString&) {
 FString J,E;FCoIGeoMapData G;Fixture(J,G,2);FSimulation S;if(!TestTrue(TEXT("Load"),S.Load(J,G,E)))return false;
 auto Submit=[&](FCommand C){TestTrue(TEXT("Enqueue"),S.Enqueue(C,E));};
 Submit(Order(S,TEXT("sell"),TEXT("company.player"),TEXT("sell"),100,5));
 Submit(Order(S,TEXT("buy.a"),TEXT("company.merchant"),TEXT("buy"),60,10));
 Submit(Order(S,TEXT("buy.b"),TEXT("company.food"),TEXT("buy"),60,8));S.StepOnce();
 TestEqual(TEXT("Two buyers share only 100 kg"),S.GetState().Trades.Num(),2);
 TestEqual(TEXT("Older sell sets execution price"),S.GetState().Trades[0].PriceCentsPerKg,5LL);
 TestEqual(TEXT("Higher bid first"),S.GetState().Trades[0].BuyerId,FString(TEXT("company.merchant")));
 TestEqual(TEXT("Partial buyer remainder"),Find(S.GetState().Orders,FString(TEXT("buy.b")))->RemainingKg,20LL);
 TestEqual(TEXT("Partial reservation"),Find(S.GetState().Orders,FString(TEXT("buy.b")))->ReservedCents,160LL);
 TestEqual(TEXT("Money goes to real seller"),Find(S.GetState().Accounts,FString(TEXT("account.company.player")))->BalanceCents,2500500LL);
 FCommand C;C.Generation=S.GetGeneration();C.Sequence=S.NextSequence();C.TargetStep=2;C.Kind=ECommandKind::CancelOrder;C.Order.Id=TEXT("buy.b");C.Order.ActorId=TEXT("company.food");Submit(C);S.StepOnce();TestEqual(TEXT("Cancellation releases cash"),Find(S.GetState().Orders,FString(TEXT("buy.b")))->ReservedCents,0LL);
 Submit(Order(S,TEXT("unfunded"),TEXT("company.external"),TEXT("buy"),1,5));
 Submit(Order(S,TEXT("same"),TEXT("company.merchant"),TEXT("sell"),10,5));
 Submit(Order(S,TEXT("self"),TEXT("company.merchant"),TEXT("buy"),10,5));S.StepOnce();TestFalse(TEXT("No uncovered buy"),S.GetResults()[S.GetResults().Num()-3].Accepted);
 TestEqual(TEXT("No self-trade"),S.GetState().Trades.Num(),2);
 for(int H=0;H<14;++H)S.StepOnce();TestEqual(TEXT("Expiry releases stock"),Find(S.GetState().Orders,FString(TEXT("same")))->ReservedKg,0LL);
 TestEqual(TEXT("Expiry releases money"),Find(S.GetState().Orders,FString(TEXT("self")))->ReservedCents,0LL);TestTrue(TEXT("Trade conserves all funds and goods"),S.AuditRuntime(E));return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConsumer,"CoI.Economy.ActualConsumption",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FConsumer::RunTest(const FString&) {
 FString J,E;FCoIGeoMapData G;Fixture(J,G,3,[](auto R){for(auto V:R->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("Needs"))){V->AsObject()->SetNumberField(TEXT("BasicKgPerThousandPerDay"),1);V->AsObject()->SetNumberField(TEXT("LuxuryKgPerThousandPerDay"),1);}});FSimulation S;if(!TestTrue(TEXT("Load"),S.Load(J,G,E)))return false;
 auto A=Order(S,TEXT("food.supply"),TEXT("company.player"),TEXT("sell"),20,1);TestTrue(TEXT("Offer food"),S.Enqueue(A,E));
 auto B=Order(S,TEXT("lux.supply"),TEXT("company.luxury"),TEXT("sell"),20,10,TEXT("storage.luxury"));B.Order.GoodId=TEXT("good.luxury");TestTrue(TEXT("Offer luxury"),S.Enqueue(B,E));
 S.StepOnce();TestEqual(TEXT("Food really removed"),Find(S.GetState().GoodsAudit,FString(TEXT("good.food")))->ConsumedKg,10LL);TestEqual(TEXT("No luxury until basics consumed"),Find(S.GetState().GoodsAudit,FString(TEXT("good.luxury")))->ConsumedKg,0LL);
 S.StepOnce();TestEqual(TEXT("Luxury after basics"),Find(S.GetState().GoodsAudit,FString(TEXT("good.luxury")))->ConsumedKg,10LL);
 int64 C=0;for(const auto& R:S.GetSnapshot().Countries)for(const auto& V:R.Consumed)C+=V.Kg;TestEqual(TEXT("Country consumption once"),C,20LL);
 TestEqual(TEXT("Food seller paid, no free goods"),Find(S.GetState().Accounts,FString(TEXT("account.company.player")))->BalanceCents,2500010LL);TestTrue(TEXT("Consumption equation"),S.AuditRuntime(E));
 Fixture(J,G,3);TestTrue(TEXT("Load shortage"),S.Load(J,G,E));for(int H=0;H<25;++H)S.StepOnce();
 TestEqual(TEXT("No food supplied: no invented consumption"),Find(S.GetState().GoodsAudit,FString(TEXT("good.food")))->ConsumedKg,0LL);
 int64 Missed=0;for(const auto& R:S.GetState().Groups)for(const auto& U:R.MissedNeed)if(U.GoodId==TEXT("good.food"))Missed+=U.Kg;TestEqual(TEXT("Shortage visible across four regions"),Missed,4400LL);
 for(const auto& O:S.GetState().Orders)TestTrue(TEXT("No luxury buys while basic missing"),O.GoodId!=TEXT("good.luxury"));return TestTrue(TEXT("No hidden funding"),S.AuditRuntime(E));
}
#endif

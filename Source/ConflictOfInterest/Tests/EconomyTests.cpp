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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTransportTest,"CoI.Economy.TransportAndCosts",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTransportTest::RunTest(const FString&) {
 FString J,E;FCoIGeoMapData G;Fixture(J,G,4,[](auto R){for(auto V:R->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("Companies")))if(V->AsObject()->GetStringField(TEXT("DecisionProfile"))!=TEXT("player"))V->AsObject()->SetStringField(TEXT("DecisionProfile"),TEXT("service"));});FSimulation S;if(!TestTrue(TEXT("Load"),S.Load(J,G,E)))return false;
 auto Book=[&](const TCHAR* Id,int64 Kg){FCommand C;C.Generation=S.GetGeneration();C.Sequence=S.NextSequence();C.TargetStep=S.GetState().Step+1;C.Kind=ECommandKind::BookTransport;auto& T=C.Transport;T.Id=Id;T.OwnerId=TEXT("company.player");T.VehicleId=TEXT("vehicle.carrier.1");T.FromCityId=TEXT("city.us.detroit");T.ToCityId=TEXT("city.us.chicago");T.SourceInventoryId=TEXT("stock.player.food");T.DestinationStorageId=TEXT("storage.service.chicago");T.Kg=Kg;TestTrue(TEXT("Queue transport"),S.Enqueue(C,E));};
 Book(TEXT("over"),2001);S.StepOnce();TestFalse(TEXT("Oversize atomic rejection"),S.GetResults().Last().Accepted);TestFalse(TEXT("Rejected vehicle stays free"),Find(S.GetState().Vehicles,FString(TEXT("vehicle.carrier.1")))->Bound);
 Book(TEXT("cancel"),10);S.StepOnce();TestTrue(TEXT("Book finite freight"),S.GetResults().Last().Accepted);
 FCommand C;C.Generation=S.GetGeneration();C.Sequence=S.NextSequence();C.TargetStep=S.GetState().Step+1;C.Kind=ECommandKind::CancelTransport;C.Transport.Id=TEXT("cancel");C.Transport.OwnerId=TEXT("company.player");TestTrue(TEXT("Queue cancel"),S.Enqueue(C,E));S.StepOnce();TestTrue(TEXT("Cancel before departure"),S.GetResults().Last().Accepted);TestEqual(TEXT("Cancel releases incoming capacity"),Find(S.GetState().Warehouses,FString(TEXT("storage.service.chicago")))->ReservedIncomingPoints,0LL);
 Book(TEXT("ship"),50);Book(TEXT("double"),10);S.StepOnce();TestFalse(TEXT("No double vehicle binding"),S.GetResults().Last().Accepted);S.StepOnce();TestEqual(TEXT("Source moved exactly once"),Find(S.GetState().Inventories,FString(TEXT("stock.player.food")))->QuantityKg,50LL);
 C.Generation=S.GetGeneration();C.Sequence=S.NextSequence();C.TargetStep=S.GetState().Step+1;C.Transport.Id=TEXT("ship");TestTrue(TEXT("Queue late cancel"),S.Enqueue(C,E));S.StepOnce();TestFalse(TEXT("No cancel after departure"),S.GetResults().Last().Accepted);
 for(int H=0;H<100;++H){S.StepOnce();if(!S.AuditRuntime(E)){AddError(E);return false;}}
 TestEqual(TEXT("Cargo arrived"),Find(S.GetState().Transports,FString(TEXT("ship")))->Status,FString(TEXT("arrived")));TestEqual(TEXT("Truck remains at destination"),Find(S.GetState().Vehicles,FString(TEXT("vehicle.carrier.1")))->CityId,FString(TEXT("city.us.chicago")));
 TestTrue(TEXT("Carrier receives real fee"),Find(S.GetState().CashTotals,FString(TEXT("freight")))->Cents>0);TestTrue(TEXT("Empty warehouses incur fixed costs"),Find(S.GetState().CashTotals,FString(TEXT("storage_fixed")))->Cents>0);TestTrue(TEXT("Foreign stock pays rent after grace"),Find(S.GetState().CashTotals,FString(TEXT("foreign_storage")))->Cents>0);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAutonomous,"CoI.Economy.Autonomous72Hours",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAutonomous::RunTest(const FString&) {
 FString J,E;FCoIGeoMapData G;Fixture(J,G,4);FSimulation S;if(!TestTrue(TEXT("Load"),S.Load(J,G,E)))return false;bool MerchantBought=false,MerchantSold=false,Shipped=false;
 for(int H=0;H<72;++H){S.StepOnce();if(!S.AuditRuntime(E)){AddError(FString::Printf(TEXT("Hour %d: %s"),H,*E));return false;}for(const auto& T:S.GetState().Trades){MerchantBought|=T.BuyerId==TEXT("company.merchant");MerchantSold|=T.SellerId==TEXT("company.merchant");}for(const auto& T:S.GetState().Transports)Shipped|=!T.Empty&&T.OwnerId==TEXT("company.merchant")&&T.FromCityId!=T.ToCityId&&T.Service==TEXT("rented_truck")&&T.Status==TEXT("arrived");}
 TestTrue(TEXT("Merchant purchases actual offers"),MerchantBought);TestTrue(TEXT("Merchant sells actual inventory"),MerchantSold);TestTrue(TEXT("Merchant pays carrier for intercity delivery"),Shipped);TestTrue(TEXT("Consumer actual food"),Find(S.GetState().GoodsAudit,FString(TEXT("good.food")))->ConsumedKg>0);TestTrue(TEXT("Real freight fee booked"),Find(S.GetState().CashTotals,FString(TEXT("freight")))!=nullptr);return true;
}
#endif

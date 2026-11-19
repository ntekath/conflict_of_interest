#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GeoCatalog.h"
#include "Simulation/SimCore.h"
#include "Simulation/SimArchive.h"
#include "Serialization/JsonSerializer.h"
using namespace CoISim;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanyControl,"CoI.Control.ExistingCompanySelection",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCompanyControl::RunTest(const FString&){FString Json,E;FCoIGeoMapData Geo;auto* Catalog=LoadObject<UCoIGeoCatalog>(nullptr,TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));if(!Catalog||!Geo.Load(Catalog->DefinitionJson,E)||!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("Simulation/ap092_legacy_fixture.coisim"))))return false;
 auto Select=[&](const FString& Id){TSharedPtr<FJsonObject> R;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),R);R->SetStringField(TEXT("PlayerCompanyId"),Id);FString Out;FJsonSerializer::Serialize(R.ToSharedRef(),TJsonWriterFactory<>::Create(&Out));return Out;};
 FSimulation A,B,C;TestTrue(TEXT("Existing trading house"),A.Load(Select(TEXT("company.player")),Geo,E));TestTrue(TEXT("Existing producer"),B.Load(Select(TEXT("company.food")),Geo,E));TestTrue(TEXT("Existing carrier"),C.Load(Select(TEXT("company.carrier")),Geo,E));
 TestEqual(TEXT("Choice does not alter economic world"),A.EconomicFingerprint(),B.EconomicFingerprint());TestEqual(TEXT("Carrier choice same assets and money"),A.EconomicFingerprint(),C.EconomicFingerprint());TestEqual(TEXT("Producer account shown"),B.GetSnapshot().PlayerMoneyCents,5000000LL);TestEqual(TEXT("Producer receives no free truck"),B.GetSnapshot().PlayerTrucks,0LL);TestEqual(TEXT("Carrier owns existing two trucks"),C.GetSnapshot().PlayerTrucks,2LL);TestEqual(TEXT("Producer own plant shown"),B.GetSnapshot().Player.Facilities.Num(),1);
 for(const TCHAR* Bad:{TEXT("missing"),TEXT("company.external"),TEXT("group.chicago.low")}){FSimulation Invalid;TestFalse(TEXT("Invalid/nonplayable choice rejected"),Invalid.Load(Select(Bad),Geo,E));TestFalse(TEXT("No partial session"),Invalid.IsLoaded());}
 auto Make=[&](ECommandOrigin Origin,const FString& Actor){FCommand Q;Q.Generation=B.GetGeneration();Q.Sequence=B.NextSequence();Q.TargetStep=B.GetState().Step+1;Q.Origin=Origin;Q.CompanyId=Actor;Q.Kind=ECommandKind::PlaceOrder;auto& O=Q.Order;O.Id=TEXT("controlled.order");O.ActorId=Actor;O.GoodId=TEXT("good.food");O.Side=TEXT("sell");O.CityId=TEXT("city.us.chicago");O.StorageId=TEXT("storage.food");O.MarketId=TEXT("market.chicago");O.OriginalKg=50;O.LimitCentsPerKg=4;return Q;};
 TestFalse(TEXT("Forged foreign player company rejected centrally"),B.Enqueue(Make(ECommandOrigin::Player,TEXT("company.wood")),E));auto Forged=Make(ECommandOrigin::Player,TEXT("company.food"));Forged.Order.ActorId=TEXT("company.wood");TestFalse(TEXT("Forged payload rejected centrally"),B.Enqueue(Forged,E));TestFalse(TEXT("AI cannot command controlled producer"),B.Enqueue(Make(ECommandOrigin::Autonomous,TEXT("company.food")),E));
 for(int H=0;H<24;++H){B.StepOnce();if(!B.AuditRuntime(E)){AddError(E);return false;}}
 for(const auto& O:B.GetState().Orders)TestTrue(TEXT("Selected company has no autonomous orders"),O.ActorId!=TEXT("company.food"));TestEqual(TEXT("Selected producer still produces"),Find(B.GetState().GoodsAudit,FString(TEXT("good.food")))->ProducedKg,1900LL);TestTrue(TEXT("Selected employer still pays wages"),Find(B.GetState().CashTotals,FString(TEXT("wages")))->Cents>0);TestTrue(TEXT("Other firms remain active"),B.GetState().Orders.ContainsByPredicate([](const FMarketOrder& O){return O.ActorId==TEXT("company.wood");}));
 auto Own=Make(ECommandOrigin::Player,TEXT("company.food"));TestTrue(TEXT("Explicit authorized order accepted"),B.Enqueue(Own,E));B.StepOnce();TestTrue(TEXT("Authorized command executes"),B.GetResults().Last().Accepted);return true;}
#endif

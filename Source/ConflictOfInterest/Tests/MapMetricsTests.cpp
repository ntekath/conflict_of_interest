#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GeoCatalog.h"
#include "Simulation/SimCore.h"
#include "Simulation/SimArchive.h"
#include "Serialization/JsonSerializer.h"
using namespace CoISim;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapMetricRules,"CoI.MapMetrics.CalculationAndAvailability",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMapMetricRules::RunTest(const FString&){FDayMetrics D;FDayMetric R;R.Id=TEXT("region|good");R.RegionId=TEXT("region");R.GoodId=TEXT("good");R.HouseholdWantedKg=100;R.HouseholdUsedKg=80;R.TradeKg=10+30;R.TradeCents=10*200+30*400;D.Rows.Add(R);
 TestTrue(TEXT("Incomplete day is missing, not zero"),MetricValue(D,R.RegionId,R.GoodId,EMapMetric::Households).Availability==EMetricAvailability::Missing);D.Complete=true;
 TestEqual(TEXT("80 consumed / 100 desired = 80 percent"),MetricValue(D,R.RegionId,R.GoodId,EMapMetric::Households).Value,80.);
 TestTrue(TEXT("Zero recipe need is NoNeed"),MetricValue(D,R.RegionId,R.GoodId,EMapMetric::Inputs).Availability==EMetricAvailability::NoNeed);
 TestEqual(TEXT("10 kg at 2 USD plus 30 kg at 4 USD = 3.50 USD/kg"),MetricValue(D,R.RegionId,R.GoodId,EMapMetric::Price).Value,350.);
 D.Rows[0].TradeKg=0;D.Rows[0].TradeCents=0;TestTrue(TEXT("No trades has no fabricated daily price"),MetricValue(D,R.RegionId,R.GoodId,EMapMetric::Price).Availability==EMetricAvailability::Missing);
 FGoodDef G;G.MapPriceLowCents=200;G.MapPriceHighCents=400;FMetricValue V;V.Availability=EMetricAvailability::Value;V.Value=800;auto High=MetricColor(V,EMapMetric::Price,G);V.Value=400;TestTrue(TEXT("Out-of-range price uses end color"),High.Equals(MetricColor(V,EMapMetric::Price,G)));V.Value=800;MetricColor(V,EMapMetric::Price,G);TestEqual(TEXT("Color clamp never changes actual value"),V.Value,800.);V.Availability=EMetricAvailability::NoNeed;auto NoNeed=MetricColor(V,EMapMetric::Households,G);V.Availability=EMetricAvailability::Missing;TestFalse(TEXT("NoNeed has a distinct neutral color, with N in UI"),NoNeed.Equals(MetricColor(V,EMapMetric::Households,G)));return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapMetricDays,"CoI.MapMetrics.CompletedDaysAndConservation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMapMetricDays::RunTest(const FString&){FString Json,E;FCoIGeoMapData Geo;auto* Catalog=LoadObject<UCoIGeoCatalog>(nullptr,TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));if(!Catalog||!Geo.Load(Catalog->DefinitionJson,E)||!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("Simulation/ap092_legacy_fixture.coisim"))))return false;FSimulation S,Fast;if(!S.Load(Json,Geo,E)||!Fast.Load(Json,Geo,E)){AddError(E);return false;}
 TestFalse(TEXT("No invented day at game start"),S.GetSnapshot().MapDay.Complete);for(int H=0;H<23;++H)S.StepOnce();TestFalse(TEXT("Partial first day never published as complete"),S.GetSnapshot().MapDay.Complete);S.StepOnce();TestEqual(TEXT("First day source step"),S.GetSnapshot().MapDay.CompletedStep,24LL);TestEqual(TEXT("First day index"),S.GetSnapshot().MapDay.Day,0LL);
 TMap<FString,int64> PreviousConsumed,PreviousInput;for(int Day=0;Day<4;++Day){if(Day)for(int H=0;H<24;++H)S.StepOnce();const auto& D=S.GetSnapshot().MapDay;TestTrue(TEXT("Day complete"),D.Complete);TestEqual(TEXT("Snapshot refers to last completed day"),D.Day,(int64)Day);TestEqual(TEXT("Metrics bounded by region x goods"),D.Rows.Num(),S.GetScenario().Definitions.Regions.Num()*S.GetScenario().Definitions.Goods.Num());
  for(const auto& G:S.GetState().GoodsAudit){int64 Consumed=0,Input=0;for(const auto& M:D.Rows)if(M.GoodId==G.Id){Consumed+=M.HouseholdUsedKg;Input+=M.InputUsedKg;TestTrue(TEXT("Actual household use within desired need"),M.HouseholdUsedKg<=M.HouseholdWantedKg);TestTrue(TEXT("Actual recipe input within planned need"),M.InputUsedKg<=M.InputWantedKg);}TestEqual(TEXT("Household day metrics equal actual goods sinks"),Consumed,G.ConsumedKg-PreviousConsumed.FindRef(G.Id));TestEqual(TEXT("Input day metrics equal actual goods sinks"),Input,G.InputKg-PreviousInput.FindRef(G.Id));PreviousConsumed.Add(G.Id,G.ConsumedKg);PreviousInput.Add(G.Id,G.InputKg);}
  if(!S.AuditRuntime(E)){AddError(E);return false;}
 }
 // Liquidity must not erase desired recipe inputs when local workforce is available.
 TSharedPtr<FJsonObject> Root;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root);
 for(auto V:Root->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Accounts")))if(V->AsObject()->GetStringField(TEXT("OwnerId"))==TEXT("company.tools"))V->AsObject()->SetNumberField(TEXT("BalanceCents"),0);
 FString PoorJson;FJsonSerializer::Serialize(Root.ToSharedRef(),TJsonWriterFactory<>::Create(&PoorJson));FSimulation Poor;if(!Poor.Load(PoorJson,Geo,E)){AddError(E);return false;}for(int H=0;H<24;++H)Poor.StepOnce();int64 PlannedOre=0,UsedOre=0;for(const auto& M:Poor.GetSnapshot().MapDay.Rows)if(M.GoodId==TEXT("good.ore")){PlannedOre+=M.InputWantedKg;UsedOre+=M.InputUsedKg;}TestEqual(TEXT("Unfunded but locally staffable tools plant still plans 2400 kg ore"),PlannedOre,2400LL);TestEqual(TEXT("No money means no actual input use"),UsedOre,0LL);TestTrue(TEXT("No money or goods injected by metrics"),Poor.AuditRuntime(E));
 const FString Before=S.Fingerprint();for(const auto& R:S.GetSnapshot().MapDay.Rows)for(EMapMetric Kind:{EMapMetric::Households,EMapMetric::Inputs,EMapMetric::Price})MetricColor(MetricValue(S.GetSnapshot().MapDay,R.RegionId,R.GoodId,Kind),Kind,*Find(S.GetScenario().Definitions.Goods,R.GoodId));TestEqual(TEXT("All layer calculations leave simulation unchanged"),S.Fingerprint(),Before);S.Advance(1000000,16,true);TestEqual(TEXT("Pause leaves economics and completed day unchanged"),S.Fingerprint(),Before);
 for(int I=0;Fast.GetState().Step<96&&I<10000;++I)Fast.Advance(Fast.GetScenario().Settings.RealMicrosPerStep,16,false);while(S.GetState().Step<Fast.GetState().Step)S.StepOnce();TestEqual(TEXT("Speed changes scheduling only, including metrics"),S.Fingerprint(),Fast.Fingerprint());return true;}
#endif

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformTime.h"
#include "GeoCatalog.h"
#include "Simulation/SimArchive.h"
#include "Simulation/SaveGame.h"
#include "Serialization/JsonSerializer.h"
using namespace CoISim;
namespace {
bool EuropeFixture(FString& J,FCoIGeoMapData& G,FString& E){auto* C=LoadObject<UCoIGeoCatalog>(nullptr,TEXT("/Game/Geography/Data/GeoCatalog_DE_NL_1913.GeoCatalog_DE_NL_1913"));return C&&G.Load(C->DefinitionJson,E)&&FFileHelper::LoadFileToString(J,*(FPaths::ProjectContentDir()/TEXT("Simulation/scenario.de_nl1913.coisim")));}
FCommand EUCommand(FSimulation& S,ECommandKind Kind){FCommand C;C.Kind=Kind;C.Generation=S.GetGeneration();C.Sequence=S.NextSequence();C.TargetStep=S.GetState().Step+1;return C;}
FString SelectCompany(const FString& J,const FString& Id){TSharedPtr<FJsonObject> R;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(J),R);R->SetStringField(TEXT("PlayerCompanyId"),Id);FString Out;FJsonSerializer::Serialize(R.ToSharedRef(),TJsonWriterFactory<>::Create(&Out));return Out;}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEuropeStart,"CoI.EuropeScenario.FreshWorldAndCompanySelection",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEuropeStart::RunTest(const FString&){FString J,E;FCoIGeoMapData G;if(!TestTrue(TEXT("Installed Europe scenario"),EuropeFixture(J,G,E)))return false;FSimulation S;if(!TestTrue(*E,S.Load(J,G,E))){AddError(E);return false;}
 TestEqual(TEXT("Catalog selects correct economic file"),G.SimulationFile,FString(TEXT("scenario.de_nl1913.coisim")));
 TestEqual(TEXT("50 regions"),S.GetState().Regions.Num(),50);TestEqual(TEXT("150 household groups"),S.GetState().Groups.Num(),150);TestEqual(TEXT("50,000 fictional residents, once"),S.GetSnapshot().Population,50000LL);
 TSet<FString> Markets;int64 CountryPopulation=0;for(const auto& R:S.GetScenario().Definitions.Regions)Markets.Add(R.MarketId);for(const auto& C:S.GetState().Countries)CountryPopulation+=C.Population;
 TestEqual(TEXT("50 unique B2B markets"),Markets.Num(),50);TestEqual(TEXT("Country aggregation equals regional population"),CountryPopulation,50000LL);TestEqual(TEXT("Two countries only"),S.GetState().Countries.Num(),2);
 const auto State=S.EconomicFingerprint();for(const auto& C:S.GetScenario().Definitions.Companies)if(C.Playable){FSimulation Other;if(!TestTrue(TEXT("Every existing playable company starts"),Other.Load(SelectCompany(J,C.Id),G,E)))return false;TestEqual(TEXT("Selection changes no economic assets or opening cash"),Other.EconomicFingerprint(),State);TestEqual(TEXT("Selected control identity"),Other.GetSession().PlayerCompanyId,C.Id);}
 TestFalse(TEXT("Unknown choice rejected"),S.Load(SelectCompany(J,TEXT("company.missing")),G,E));TestEqual(TEXT("Failed new world is atomic"),S.EconomicFingerprint(),State);
 for(const auto& P:S.GetScenario().Definitions.Places)TestFalse(TEXT("No active US city"),P.Id.StartsWith(TEXT("city.us.")));
 for(const auto& R:S.GetState().Regions)TestEqual(TEXT("GDP proxy starts at zero"),R.ModelGdpCents,0LL);
 TestTrue(TEXT("Initial world invariants"),S.AuditRuntime(E));return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEuropeTrade,"CoI.EuropeScenario.BuyTransportSellSaveContinuation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEuropeTrade::RunTest(const FString&){FString J,E;FCoIGeoMapData G;if(!EuropeFixture(J,G,E))return false;FSimulation S;if(!S.Load(J,G,E)){AddError(E);return false;}
 const FString From=TEXT("city.nl.1913.friesland"),To=TEXT("city.nl.1913.zuid_holland"),Store=TEXT("storage.food.friesland");
 auto Empty=EUCommand(S,ECommandKind::BookTransport);Empty.Transport.Id=TEXT("eu.reposition");Empty.Transport.OwnerId=TEXT("company.player");Empty.Transport.VehicleId=TEXT("vehicle.player");Empty.Transport.FromCityId=To;Empty.Transport.ToCityId=From;Empty.Transport.Empty=true;
 if(!TestTrue(TEXT("Own paid reposition queued"),S.Enqueue(Empty,E)))return false;S.StepOnce();if(!TestTrue(TEXT("Reposition accepted"),S.GetResults().Last().Accepted)){AddError(S.GetResults().Last().Reason);return false;}
 auto Buy=EUCommand(S,ECommandKind::PlaceOrder);Buy.Order.Id=TEXT("eu.buy");Buy.Order.ActorId=TEXT("company.player");Buy.Order.Side=TEXT("buy");Buy.Order.CityId=From;Buy.Order.StorageId=Store;Buy.Order.MarketId=TEXT("market.nl.1913.friesland");Buy.Order.GoodId=TEXT("good.food");Buy.Order.OriginalKg=50;Buy.Order.LimitCentsPerKg=1000;
 TestTrue(TEXT("Buy queued in regional book"),S.Enqueue(Buy,E));S.StepOnce();
 for(int H=0;H<72&&Find(S.GetState().Vehicles,FString(TEXT("vehicle.player")))->CityId!=From;++H)S.StepOnce();
 const auto* Stock=S.GetState().Inventories.FindByPredicate([&](const FInventory& I){return I.OwnerId==TEXT("company.player")&&I.StorageId==Store&&I.GoodId==TEXT("good.food");});
 if(!TestNotNull(TEXT("Actual purchase remains at producer pickup warehouse"),Stock))return false;
 auto Haul=EUCommand(S,ECommandKind::BookTransport);auto& T=Haul.Transport;T.Id=TEXT("eu.delivery");T.OwnerId=TEXT("company.player");T.VehicleId=TEXT("vehicle.player");T.FromCityId=From;T.ToCityId=To;T.SourceInventoryId=Stock->Id;T.DestinationStorageId=TEXT("storage.player");T.Kg=FMath::Min(20LL,Stock->QuantityKg-Stock->ReservedKg);
 TestTrue(TEXT("Purchased goods transport queued"),S.Enqueue(Haul,E));S.StepOnce();if(!TestTrue(TEXT("Physical haul accepted"),S.GetResults().Last().Accepted)){AddError(S.GetResults().Last().Reason);return false;}
 FString Save;FScenario Expected=S.GetScenario();if(!TestTrue(TEXT("Save while transport booked"),S.SaveGameJson(Save,E))){AddError(E);return false;}FSimulation Restored;int32 Speed=1;if(!TestTrue(TEXT("Same-version load"),Restored.LoadGameJson(Save,G,Expected,E,Speed))){AddError(E);return false;}
 for(int H=0;H<48;++H){S.StepOnce();Restored.StepOnce();}TestEqual(TEXT("Identical economy after payroll, retail and arrival"),S.EconomicFingerprint(),Restored.EconomicFingerprint());
 const auto* Job=Find(S.GetState().Transports,FString(TEXT("eu.delivery")));if(!TestNotNull(TEXT("Paid delivery retained"),Job))return false;TestEqual(TEXT("Delivered once"),Job->Status,FString(TEXT("arrived")));TestTrue(TEXT("Freight was paid"),Job->CostCents>0);
 auto Sell=EUCommand(S,ECommandKind::PlaceOrder);auto& O=Sell.Order;O.Id=TEXT("eu.sell");O.ActorId=TEXT("company.player");O.Side=TEXT("sell");O.CityId=To;O.StorageId=TEXT("storage.player");O.MarketId=TEXT("market.nl.1913.zuid_holland");O.GoodId=TEXT("good.food");O.OriginalKg=1;O.LimitCentsPerKg=1;TestTrue(TEXT("Sell queued"),S.Enqueue(Sell,E));
 auto Demand=EUCommand(S,ECommandKind::PlaceOrder);Demand.Order=O;Demand.Order.Id=TEXT("eu.demand");Demand.Order.ActorId=TEXT("company.merchant");Demand.Order.Side=TEXT("buy");Demand.Order.LimitCentsPerKg=10000;TestTrue(TEXT("Real funded counterparty queued"),S.Enqueue(Demand,E));S.StepOnce();
 TestTrue(TEXT("Final sale settled using common orderbook"),S.GetState().Trades.ContainsByPredicate([](const FTrade& X){return X.SellOrderId==TEXT("eu.sell");}));TestTrue(TEXT("Whole flow financial and physical invariants"),S.AuditRuntime(E));
 auto Wrong=Expected;Wrong.Id=TEXT("scenario.us.great_lakes.1913");const auto Before=S.EconomicFingerprint();TestFalse(TEXT("Other scenario save context refused"),S.LoadGameJson(Save,G,Wrong,E,Speed));TestEqual(TEXT("No mixed-world mutation"),S.EconomicFingerprint(),Before);return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEuropeAutonomy,"CoI.EuropeScenario.FourteenDaysAndCountryStatistics",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEuropeAutonomy::RunTest(const FString&){FString J,E;FCoIGeoMapData G;if(!EuropeFixture(J,G,E))return false;FSimulation S;if(!S.Load(J,G,E)){AddError(E);return false;}FString Csv=TEXT("day,population,employed,money_cents,food_produced_kg,food_consumed_kg,model_gdp_cents,step_micros\n");
 const double Begin=FPlatformTime::Seconds();for(int Day=1;Day<=14;++Day){for(int H=0;H<24;++H)if(!S.StepOnce()){AddError(TEXT("Simulation step failed"));return false;}if(!TestTrue(*E,S.AuditRuntime(E))){AddError(E);return false;}
 int64 Money=0,RegionalGdp=0,CountryGdp=0,People=0,Workers=0;for(const auto& A:S.GetState().Accounts)Money+=A.BalanceCents;for(const auto& R:S.GetState().Regions)RegionalGdp+=R.ModelGdpCents;for(const auto& C:S.GetState().Countries){CountryGdp+=C.ModelGdpCents;People+=C.Population;Workers+=C.Employed;}
 TestEqual(TEXT("GDP proxy counted once per country"),CountryGdp,RegionalGdp);TestEqual(TEXT("Population aggregation"),People,S.GetSnapshot().Population);TestEqual(TEXT("Employment aggregation"),Workers,S.GetSnapshot().Employed);TestEqual(TEXT("No invisible money"),Money,S.GetState().InitialMoneyCents);
 const auto* Food=Find(S.GetState().GoodsAudit,FString(TEXT("good.food")));Csv+=FString::Printf(TEXT("%d,%lld,%lld,%lld,%lld,%lld,%lld,%.0f\n"),Day,People,Workers,Money,Food->ProducedKg,Food->ConsumedKg,CountryGdp,S.GetDiagnostics().StepMicros);}
 TestTrue(TEXT("NPC producers operate"),Find(S.GetState().GoodsAudit,FString(TEXT("good.food")))->ProducedKg>0);TestTrue(TEXT("Real households consume shop stocks"),Find(S.GetState().GoodsAudit,FString(TEXT("good.food")))->ConsumedKg>0);TestTrue(TEXT("NPC employment funded"),S.GetSnapshot().Employed>0);
 TestTrue(TEXT("City views cover new IDs"),S.GetSnapshot().Player.Cities.Num()==50);TestTrue(TEXT("No developer interventions"),!S.GetState().DeveloperModified);FFileHelper::SaveStringToFile(Csv,*(FPaths::ProjectSavedDir()/TEXT("MAP03_14days.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);AddInfo(FString::Printf(TEXT("14-day wall time: %.2f seconds"),FPlatformTime::Seconds()-Begin));return true;}
#endif

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GeoTransform.h"
#include "GeoMapData.h"
#include "GeoCatalog.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoIGeoTest,"CoI.Geo.CatalogProjectionAndDistances",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCoIGeoTest::RunTest(const FString& Parameters)
{
 auto* Catalog=LoadObject<UCoIGeoCatalog>(nullptr,TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));if(!TestNotNull(TEXT("Cook-referenced catalog"),Catalog))return false;
 FCoIGeoMapData Data;FString Error;if(!TestTrue(TEXT("Validate imported US data"),Data.Load(Catalog->DefinitionJson,Error))){AddError(Error);return false;}
 TestEqual(TEXT("Four test cities"),Data.Cities.Num(),4);TestEqual(TEXT("Five Great Lakes"),Data.Lakes.Num(),5);TestEqual(TEXT("US regions"),Data.Regions.Num(),4);TestEqual(TEXT("Start date metadata"),Data.StartDate,FString(TEXT("1913-01-01")));
 TSharedPtr<FJsonObject> Root;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Catalog->DefinitionJson),Root);
 for(auto& Value:Root->GetArrayField(TEXT("projection_references")))
 {auto O=Value->AsObject();auto P=CoIGeo::Project(O->GetNumberField(TEXT("latitude")),O->GetNumberField(TEXT("longitude")));
  TestTrue(TEXT("C++ LCC agrees with independent PROJ to 0.001 UE unit"),FMath::Abs(P.North-O->GetNumberField(TEXT("north")))<.001&&FMath::Abs(P.East-O->GetNumberField(TEXT("east")))<.001);}
 TSharedPtr<FJsonObject> Report;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Catalog->ProvenanceJson),Report);
 for(auto& Value:Report->GetArrayField(TEXT("distances")))
 {auto O=Value->AsObject();auto* A=Data.Cities.FindByPredicate([&](const FCoIGeoCity& C){return C.Id==O->GetStringField(TEXT("from"));});auto* B=Data.Cities.FindByPredicate([&](const FCoIGeoCity& C){return C.Id==O->GetStringField(TEXT("to"));});
  if(!A||!B){AddError(TEXT("Missing distance reference city"));return false;}double Km=CoIGeo::DistanceKm(A->Latitude,A->Longitude,B->Latitude,B->Longitude),Ref=O->GetNumberField(TEXT("independent_wgs84_km"));
  TestTrue(TEXT("Haversine agrees with independent WGS84 reference within 0.5 percent"),FMath::Abs(Km-Ref)/Ref<.005);}
 auto P=CoIGeo::Project(44,-86.5);auto* USA=Data.Countries.FindByPredicate([](const FCoIGeoSurface& S){return S.Id==TEXT("country.usa");});
 TestTrue(TEXT("Lake Michigan not filled with US land"),USA&&!USA->Contains({P.North,P.East}));
 TestFalse(TEXT("Corrupt schema rejected"),Data.Load(TEXT("{\"schema\":99}"),Error));TestEqual(TEXT("No partial state on failed import"),Data.Cities.Num(),4);
 return true;
}
#endif

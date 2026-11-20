#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GeoCatalog.h"
#include "GeoMapData.h"
#include "MapSelection.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
UCoIGeoCatalog* EuropeCatalog()
{return LoadObject<UCoIGeoCatalog>(nullptr,TEXT("/Game/Geography/Data/GeoCatalog_DE_NL_1913.GeoCatalog_DE_NL_1913"));}
FString JsonText(const TSharedPtr<FJsonObject>& Root)
{FString Text;FJsonSerializer::Serialize(Root.ToSharedRef(),TJsonWriterFactory<>::Create(&Text));return Text;}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEuropeGeography,"CoI.Geo.Europe1913HierarchyPhysicalAndPolitical",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEuropeGeography::RunTest(const FString&)
{
 auto* Catalog=EuropeCatalog();if(!TestNotNull(TEXT("Offline cooked Europe catalog"),Catalog))return false;
 FCoIGeoMapData Data;FString Error;
 if(!TestTrue(TEXT("Load new map"),Data.Load(Catalog->DefinitionJson,Error))){AddError(Error);return false;}
 TestTrue(TEXT("MAP02 is geography only; no wrong US economics"),Data.bGeographyOnly);
 TestEqual(TEXT("Countries"),Data.Countries.Num(),2);TestEqual(TEXT("Regions"),Data.Regions.Num(),50);TestEqual(TEXT("Hubs"),Data.Cities.Num(),50);
 TSet<FString> Regions;for(const auto& Hub:Data.Cities){Regions.Add(Hub.RegionId);const auto Hit=Data.Query(Hub.Position);TestEqual(TEXT("Correct geographic hub region"),Hit.RegionId,Hub.RegionId);TestEqual(TEXT("Correct hub country"),Hit.CountryId,Hub.CountryId);}
 TestEqual(TEXT("Exactly one hub per region"),Regions.Num(),50);
 TSharedPtr<FJsonObject> Root;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Catalog->DefinitionJson),Root);
 for(const auto& Value:Root->GetArrayField(TEXT("projection_references")))
 {
  const auto O=Value->AsObject();const auto P=Data.Project(O->GetNumberField(TEXT("latitude")),O->GetNumberField(TEXT("longitude")));
  TestTrue(TEXT("Independent PROJ vectors agree"),P.Equals({O->GetNumberField(TEXT("north")),O->GetNumberField(TEXT("east"))},.001));
 }
 TSharedPtr<FJsonObject> Report;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Catalog->ProvenanceJson),Report);
 for(const auto& Value:Report->GetArrayField(TEXT("probes")))
 {
  const auto O=Value->AsObject();const auto Point=Data.Project(O->GetNumberField(TEXT("latitude")),O->GetNumberField(TEXT("longitude")));const auto Hit=Data.Query(Point);
  TestEqual(*O->GetStringField(TEXT("name")),Hit.RegionId,O->GetStringField(TEXT("region")));
  TestEqual(TEXT("Independent physical land/water probe"),Hit.bLand,O->GetBoolField(TEXT("land")));
 }
 const auto LakePoint=Data.Project(53.69,11.48);const auto Lake=Data.Query(LakePoint);
 TestFalse(TEXT("Lake is water"),Lake.bLand);TestFalse(TEXT("Water geometry is identified"),Lake.WaterId.IsEmpty());
 TestEqual(TEXT("Water does not remove political region"),Lake.RegionId,FString(TEXT("region.de.1913.mecklenburg_schwerin")));
 TestEqual(TEXT("Political selection works on lake"),CoISelection::PickSurface(Data,LakePoint,ECoIMapView::Regions),Lake.RegionId);
 TestTrue(TEXT("Watercourse visualization available"),Data.Rivers.Num()>0);
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEuropeGeographyValidation,"CoI.Geo.Europe1913RejectsCorruptionAtomically",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEuropeGeographyValidation::RunTest(const FString&)
{
 auto* Catalog=EuropeCatalog();if(!Catalog)return false;FCoIGeoMapData Data;FString Error;
 if(!Data.Load(Catalog->DefinitionJson,Error))return false;
 auto Reject=[&](TFunction<void(TSharedPtr<FJsonObject>)> Corrupt)
 {
  TSharedPtr<FJsonObject> R;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Catalog->DefinitionJson),R);Corrupt(R);
  TestFalse(TEXT("Invalid candidate rejected"),Data.Load(JsonText(R),Error));TestFalse(TEXT("Clear import diagnostic"),Error.IsEmpty());
  TestEqual(TEXT("Original catalog remains intact"),Data.Cities.Num(),50);
 };
 Reject([](auto R){auto Cities=R->GetArrayField(TEXT("cities"));Cities[1]->AsObject()->SetStringField(TEXT("region_id"),Cities[0]->AsObject()->GetStringField(TEXT("region_id")));});
 Reject([](auto R){R->GetArrayField(TEXT("regions"))[0]->AsObject()->SetStringField(TEXT("country_id"),TEXT("country.missing"));});
 Reject([](auto R){R->GetObjectField(TEXT("projection"))->GetObjectField(TEXT("parameters"))->SetNumberField(TEXT("UnitsPerKm"),0);});
 Reject([](auto R){R->GetArrayField(TEXT("rivers"))[0]->AsObject()->SetBoolField(TEXT("navigable"),true);});
 Reject([](auto R){R->GetArrayField(TEXT("cities"))[0]->AsObject()->SetNumberField(TEXT("latitude"),10);});
 return true;
}
#endif

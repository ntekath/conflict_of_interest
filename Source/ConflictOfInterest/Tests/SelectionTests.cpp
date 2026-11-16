#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GeoCatalog.h"
#include "MapSelection.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoISelectionTest,"CoI.Map.SelectionPriorityAndViews",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCoISelectionTest::RunTest(const FString& Parameters)
{
 auto* Catalog=LoadObject<UCoIGeoCatalog>(nullptr,TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));if(!TestNotNull(TEXT("Catalog"),Catalog))return false;
 FCoIGeoMapData Data;FString Error;if(!TestTrue(TEXT("Valid geography"),Data.Load(Catalog->DefinitionJson,Error)))return false;
 TArray<FVector2D> Centers;TArray<bool> Visible;
 for(int I=0;I<Data.Cities.Num();++I){Centers.Add({100.+I*100,100});Visible.Add(true);}
 for(int I=0;I<Data.Cities.Num();++I)
 {
  const auto& C=Data.Cities[I];
  TestEqual(TEXT("Visible city has priority over country"),CoISelection::Pick(Data,C.Position,ECoIMapView::Countries,Centers,Visible,Centers[I]),C.Id);
  TestEqual(TEXT("Region surface lookup"),CoISelection::PickSurface(Data,C.Position,ECoIMapView::Regions),C.RegionId);
  TestEqual(TEXT("Ownership retains region granularity"),CoISelection::PickSurface(Data,C.Position,ECoIMapView::Ownership),C.RegionId);
  TestEqual(TEXT("Country surface lookup"),CoISelection::PickSurface(Data,C.Position,ECoIMapView::Countries),C.CountryId);
 }
 Visible[0]=false;TestEqual(TEXT("Hidden city cannot intercept click"),CoISelection::PickCity(Centers,Visible,Centers[0]),INDEX_NONE);
 Visible[0]=true;TestEqual(TEXT("Screen hit radius boundary"),CoISelection::PickCity(Centers,Visible,Centers[0]+FVector2D(8,0)),0);
 TestEqual(TEXT("Outside visible symbol misses"),CoISelection::PickCity(Centers,Visible,Centers[0]+FVector2D(9,0)),INDEX_NONE);
 TestTrue(TEXT("Background clears selection"),CoISelection::PickSurface(Data,{-99999,-99999},ECoIMapView::Regions).IsEmpty());
 auto Lake=Data.Lakes.FindByPredicate([](const FCoIGeoSurface& S){return S.Id==TEXT("lake.michigan");});
 if(Lake&&Lake->Triangles.Num()>=3){auto P=(Lake->Vertices[Lake->Triangles[0]]+Lake->Vertices[Lake->Triangles[1]]+Lake->Vertices[Lake->Triangles[2]])/3;TestTrue(TEXT("Water cannot select underlying land"),CoISelection::PickSurface(Data,P,ECoIMapView::Countries).IsEmpty());}
 TestTrue(TEXT("Label collision rejects overlapping rectangles"),CoISelection::Overlaps({10,10},{100,20},{90,15},{50,20}));
 TestFalse(TEXT("Separated labels allowed"),CoISelection::Overlaps({10,10},{100,20},{120,15},{50,20}));
 return true;
}
#endif

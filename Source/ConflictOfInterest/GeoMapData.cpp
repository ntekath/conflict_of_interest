#include "GeoMapData.h"
#include "GeoTransform.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
static double Cross(FVector2D A,FVector2D B){return A.X*B.Y-A.Y*B.X;}
bool FCoIGeoSurface::Contains(FVector2D P) const
{
 for(int32 I=0;I+2<Triangles.Num();I+=3)
 {
  auto A=Vertices[Triangles[I]],B=Vertices[Triangles[I+1]],C=Vertices[Triangles[I+2]];
  double X=Cross(B-A,P-A),Y=Cross(C-B,P-B),Z=Cross(A-C,P-C);
  if((X>=-1e-5&&Y>=-1e-5&&Z>=-1e-5)||(X<=1e-5&&Y<=1e-5&&Z<=1e-5))return true;
 }
 return false;
}
static bool Pair(const TSharedPtr<FJsonValue>& V,FVector2D& P)
{
 const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(!V||!V->TryGetArray(A)||A->Num()!=2)return false;
 double X,Y;if(!(*A)[0]->TryGetNumber(X)||!(*A)[1]->TryGetNumber(Y)||!FMath::IsFinite(X)||!FMath::IsFinite(Y)||FMath::Abs(X)>1.e6||FMath::Abs(Y)>1.e6)return false;
 P={X,Y};return true;
}
bool FCoIGeoMapData::Load(const FString& Text,FString& Error)
{
 FCoIGeoMapData Result;TSharedPtr<FJsonObject> Root;TSet<FString> IDs;
 auto Fail=[&](const TCHAR* Reason){Error=Reason;return false;};
 if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root)||!Root)return Fail(TEXT("Invalid geographic JSON."));
 double Schema;if(!Root->TryGetNumberField(TEXT("schema"),Schema)||Schema!=1)return Fail(TEXT("Unsupported geographic schema."));
 const TSharedPtr<FJsonObject> *Scenario=nullptr,*Camera=nullptr,*Projection=nullptr,*Parameters=nullptr;
 if(!Root->TryGetObjectField(TEXT("scenario"),Scenario)||!(*Scenario)->TryGetStringField(TEXT("id"),Result.ScenarioId)||!Result.ScenarioId.StartsWith(TEXT("scenario.us."))||!(*Scenario)->TryGetStringField(TEXT("start_date"),Result.StartDate))return Fail(TEXT("Expected a US scenario definition."));
 FDateTime Date;if(!FDateTime::ParseIso8601(*(Result.StartDate+TEXT("T00:00:00Z")),Date))return Fail(TEXT("Invalid scenario start date."));
 if(!Root->TryGetObjectField(TEXT("projection"),Projection)||!(*Projection)->TryGetObjectField(TEXT("parameters"),Parameters))return Fail(TEXT("Missing projection parameters."));
 auto Match=[&](const TCHAR* Key,double Expected){double N;return (*Parameters)->TryGetNumberField(Key,N)&&FMath::IsFinite(N)&&FMath::Abs(N-Expected)<1e-8;};
 if(!Match(TEXT("RadiusKm"),CoIGeo::RadiusKm)||!Match(TEXT("OriginLatitude"),CoIGeo::OriginLatitude)||!Match(TEXT("OriginLongitude"),CoIGeo::OriginLongitude)||!Match(TEXT("Parallel1"),CoIGeo::Parallel1)||!Match(TEXT("Parallel2"),CoIGeo::Parallel2)||!Match(TEXT("UnitsPerKm"),CoIGeo::UnitsPerKm))return Fail(TEXT("Stale projection: run offline import again."));
 if(!Root->TryGetObjectField(TEXT("camera"),Camera))return Fail(TEXT("Camera definition missing."));
 auto ReadPair=[&](const TCHAR* Key,FVector2D& Out){const auto* V=(*Camera)->Values.Find(Key);return V&&Pair(*V,Out);};
 if(!ReadPair(TEXT("minimum"),Result.Minimum)||!ReadPair(TEXT("maximum"),Result.Maximum)||!ReadPair(TEXT("center"),Result.Center)||!(*Camera)->TryGetNumberField(TEXT("height"),Result.OverviewHeight)||!FMath::IsFinite(Result.OverviewHeight)||Result.OverviewHeight<600||Result.OverviewHeight>22000||Result.Minimum.X>=Result.Maximum.X||Result.Minimum.Y>=Result.Maximum.Y||Result.Center.X<Result.Minimum.X||Result.Center.X>Result.Maximum.X||Result.Center.Y<Result.Minimum.Y||Result.Center.Y>Result.Maximum.Y)return Fail(TEXT("Invalid camera bounds."));
 auto Surfaces=[&](const TCHAR* Key,const TCHAR* Prefix,TArray<FCoIGeoSurface>& Out,bool Test)->bool
 {
  const TArray<TSharedPtr<FJsonValue>>* List=nullptr;if(!Root->TryGetArrayField(Key,List)||List->IsEmpty())return false;
  for(const auto& Value:*List)
  {
   const TSharedPtr<FJsonObject>* O=nullptr;if(!Value->TryGetObject(O)||!O->IsValid())return false;FCoIGeoSurface S;
   if(!(*O)->TryGetStringField(TEXT("id"),S.Id)||!S.Id.StartsWith(Prefix)||IDs.Contains(S.Id)||!(*O)->TryGetStringField(TEXT("name"),S.Name)||S.Name.IsEmpty())return false;
   IDs.Add(S.Id);S.bTest=Test;
   if(Test&&(!(*O)->TryGetStringField(TEXT("country_id"),S.CountryId)||S.CountryId!=TEXT("country.usa")||!(*O)->TryGetBoolField(TEXT("is_test"),S.bTest)||!S.bTest))return false;
   const TArray<TSharedPtr<FJsonValue>> *V=nullptr,*T=nullptr,*R=nullptr;
   if(!(*O)->TryGetArrayField(TEXT("vertices"),V)||V->Num()<3||!(*O)->TryGetArrayField(TEXT("triangles"),T)||T->IsEmpty()||T->Num()%3||!(*O)->TryGetArrayField(TEXT("rings"),R))return false;
   for(const auto& P:*V){FVector2D XY;if(!Pair(P,XY))return false;S.Vertices.Add(XY);}
   for(const auto& Index:*T){double N;if(!Index->TryGetNumber(N)||!FMath::IsFinite(N)||N<0||N>=V->Num()||N!=FMath::FloorToDouble(N))return false;S.Triangles.Add((int32)N);}
   for(int32 I=0;I<S.Triangles.Num();I+=3)
    if(FMath::Abs(Cross(S.Vertices[S.Triangles[I+1]]-S.Vertices[S.Triangles[I]],S.Vertices[S.Triangles[I+2]]-S.Vertices[S.Triangles[I]]))<1e-10)return false;
   for(const auto& Ring:*R){const TArray<TSharedPtr<FJsonValue>>* Points=nullptr;if(!Ring->TryGetArray(Points)||Points->Num()<4)return false;TArray<FVector2D> RR;
    for(const auto& P:*Points){FVector2D XY;if(!Pair(P,XY))return false;RR.Add(XY);}if(!RR[0].Equals(RR.Last(),1e-6))return false;S.Rings.Add(MoveTemp(RR));}
   Out.Add(MoveTemp(S));
  }
  return true;
 };
 if(!Surfaces(TEXT("countries"),TEXT("country."),Result.Countries,false)||!Surfaces(TEXT("lakes"),TEXT("lake."),Result.Lakes,false)||!Surfaces(TEXT("regions"),TEXT("region.test."),Result.Regions,true)||Result.Countries.Num()!=2||!IDs.Contains(TEXT("country.usa"))||!IDs.Contains(TEXT("country.can")))return Fail(TEXT("Invalid surface geometry, IDs or US country references."));
 const TArray<TSharedPtr<FJsonValue>>* CitiesJson=nullptr;if(!Root->TryGetArrayField(TEXT("cities"),CitiesJson)||CitiesJson->Num()!=4)return Fail(TEXT("Expected four US test cities."));
 for(const auto& Value:*CitiesJson)
 {
  const TSharedPtr<FJsonObject>* O=nullptr;if(!Value->TryGetObject(O)||!O->IsValid())return Fail(TEXT("Invalid city object."));FCoIGeoCity C;
  if(!(*O)->TryGetStringField(TEXT("id"),C.Id)||!C.Id.StartsWith(TEXT("city.us."))||IDs.Contains(C.Id)||!(*O)->TryGetStringField(TEXT("name"),C.Name)||!(*O)->TryGetStringField(TEXT("country_id"),C.CountryId)||C.CountryId!=TEXT("country.usa")||!(*O)->TryGetStringField(TEXT("region_id"),C.RegionId)||!(*O)->TryGetNumberField(TEXT("latitude"),C.Latitude)||!(*O)->TryGetNumberField(TEXT("longitude"),C.Longitude)||!FMath::IsFinite(C.Latitude)||!FMath::IsFinite(C.Longitude)||C.Latitude<39||C.Latitude>50||C.Longitude<-94||C.Longitude>-75)return Fail(TEXT("Invalid US city definition."));
  const auto* P=(*O)->Values.Find(TEXT("position"));if(!P||!Pair(*P,C.Position))return Fail(TEXT("Invalid city position."));
  auto Projected=CoIGeo::Project(C.Latitude,C.Longitude);if(FMath::Abs(Projected.North-C.Position.X)>.001||FMath::Abs(Projected.East-C.Position.Y)>.001)return Fail(TEXT("C++/offline projection mismatch."));
  const auto* Region=Result.Regions.FindByPredicate([&](const FCoIGeoSurface& S){return S.Id==C.RegionId;});
  if(!Region||!Region->Contains(C.Position))return Fail(TEXT("City outside assigned test region."));
  IDs.Add(C.Id);Result.Cities.Add(MoveTemp(C));
 }
 *this=MoveTemp(Result);Error.Reset();return true;
}

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
 const TSharedPtr<FJsonObject> *Scenario=nullptr,*Camera=nullptr,*ProjectionJson=nullptr,*Parameters=nullptr;
 if(!Root->TryGetObjectField(TEXT("scenario"),Scenario)||!(*Scenario)->TryGetStringField(TEXT("id"),Result.ScenarioId)||!Result.ScenarioId.StartsWith(TEXT("scenario."))||!(*Scenario)->TryGetStringField(TEXT("start_date"),Result.StartDate))return Fail(TEXT("Expected a geographic scenario definition."));
 (*Scenario)->TryGetBoolField(TEXT("geography_only"),Result.bGeographyOnly);
 (*Scenario)->TryGetStringField(TEXT("simulation_file"),Result.SimulationFile);
 if(!Result.SimulationFile.IsEmpty()&&(!Result.SimulationFile.EndsWith(TEXT(".coisim"))||Result.SimulationFile.Contains(TEXT("/"))||Result.SimulationFile.Contains(TEXT("\\"))||Result.SimulationFile.Contains(TEXT(".."))||Result.SimulationFile.Contains(TEXT(":"))))return Fail(TEXT("Invalid scenario filename."));
 FDateTime Date;if(!FDateTime::ParseIso8601(*(Result.StartDate+TEXT("T00:00:00Z")),Date))return Fail(TEXT("Invalid scenario start date."));
 if(!Root->TryGetObjectField(TEXT("projection"),ProjectionJson)||!(*ProjectionJson)->TryGetObjectField(TEXT("parameters"),Parameters))return Fail(TEXT("Missing projection parameters."));
 auto Read=[&](const TCHAR* Key,double& Out){return (*Parameters)->TryGetNumberField(Key,Out)&&FMath::IsFinite(Out);};
 auto& P=Result.Projection;
 if(!Read(TEXT("RadiusKm"),P.RadiusKm)||!Read(TEXT("OriginLatitude"),P.OriginLatitude)||!Read(TEXT("OriginLongitude"),P.OriginLongitude)||!Read(TEXT("Parallel1"),P.Parallel1)||!Read(TEXT("Parallel2"),P.Parallel2)||!Read(TEXT("UnitsPerKm"),P.UnitsPerKm)||!P.IsValid())return Fail(TEXT("Invalid geographic projection profile."));
 if(!Root->TryGetObjectField(TEXT("camera"),Camera))return Fail(TEXT("Camera definition missing."));
 auto ReadPair=[&](const TCHAR* Key,FVector2D& Out){const auto* V=(*Camera)->Values.Find(Key);return V&&Pair(*V,Out);};
 if(!ReadPair(TEXT("minimum"),Result.Minimum)||!ReadPair(TEXT("maximum"),Result.Maximum)||!ReadPair(TEXT("center"),Result.Center)||!(*Camera)->TryGetNumberField(TEXT("height"),Result.OverviewHeight)||!FMath::IsFinite(Result.OverviewHeight)||Result.OverviewHeight<600||Result.OverviewHeight>22000||Result.Minimum.X>=Result.Maximum.X||Result.Minimum.Y>=Result.Maximum.Y||Result.Center.X<Result.Minimum.X||Result.Center.X>Result.Maximum.X||Result.Center.Y<Result.Minimum.Y||Result.Center.Y>Result.Maximum.Y)return Fail(TEXT("Invalid camera bounds."));
 auto Surfaces=[&](const TCHAR* Key,const TCHAR* Prefix,TArray<FCoIGeoSurface>& Out,bool Region,bool ReuseIDs=false)->bool
 {
  const TArray<TSharedPtr<FJsonValue>>* List=nullptr;if(!Root->TryGetArrayField(Key,List)||List->IsEmpty())return false;
  TSet<FString> LocalIDs;
  for(const auto& Value:*List)
  {
   const TSharedPtr<FJsonObject>* O=nullptr;if(!Value->TryGetObject(O)||!O->IsValid())return false;FCoIGeoSurface S;
   if(!(*O)->TryGetStringField(TEXT("id"),S.Id)||!S.Id.StartsWith(Prefix)||LocalIDs.Contains(S.Id)||(!ReuseIDs&&IDs.Contains(S.Id))||!(*O)->TryGetStringField(TEXT("name"),S.Name)||S.Name.IsEmpty())return false;
   LocalIDs.Add(S.Id);if(!ReuseIDs)IDs.Add(S.Id);
   if(Region&&(!(*O)->TryGetStringField(TEXT("country_id"),S.CountryId)||!(*O)->TryGetBoolField(TEXT("is_test"),S.bTest)))return false;
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
 if(!Surfaces(TEXT("countries"),TEXT("country."),Result.Countries,false)||!Surfaces(TEXT("lakes"),Result.bGeographyOnly?TEXT("water."):TEXT("lake."),Result.Lakes,false)||!Surfaces(TEXT("regions"),TEXT("region."),Result.Regions,true))return Fail(TEXT("Invalid surface geometry or IDs."));
 for(const auto& R:Result.Regions)if(!Result.Countries.ContainsByPredicate([&](const FCoIGeoSurface& C){return C.Id==R.CountryId;}))return Fail(TEXT("Region references unknown country."));
 if(Result.bGeographyOnly)
 {
  if(!Surfaces(TEXT("land"),TEXT("land."),Result.Land,false)||!Surfaces(TEXT("region_land"),TEXT("region."),Result.RegionLand,false,true)||Result.RegionLand.Num()!=Result.Regions.Num())return Fail(TEXT("Missing separate physical land surfaces."));
  for(const auto& R:Result.RegionLand)if(!Result.Regions.ContainsByPredicate([&](const FCoIGeoSurface& S){return S.Id==R.Id;}))return Fail(TEXT("Unknown region land ID."));
  const TArray<TSharedPtr<FJsonValue>>* RiverValues=nullptr;
  if(!Root->TryGetArrayField(TEXT("rivers"),RiverValues))return Fail(TEXT("Missing watercourse geometry."));
  for(const auto& V:*RiverValues)
  {
   const auto O=V->AsObject();FCoIGeoRiver R;bool Navigable=true;const TArray<TSharedPtr<FJsonValue>>* Points=nullptr;
   if(!O||!O->TryGetStringField(TEXT("id"),R.Id)||!R.Id.StartsWith(TEXT("river."))||IDs.Contains(R.Id)||!O->TryGetStringField(TEXT("name"),R.Name)||!O->TryGetNumberField(TEXT("half_width"),R.HalfWidth)||!FMath::IsFinite(R.HalfWidth)||R.HalfWidth<=0||R.HalfWidth>10||!O->TryGetBoolField(TEXT("navigable"),Navigable)||Navigable||!O->TryGetArrayField(TEXT("points"),Points)||Points->Num()<2)return Fail(TEXT("Invalid watercourse; geometry does not grant navigation."));
   for(const auto& Point:*Points){FVector2D XY;if(!Pair(Point,XY))return Fail(TEXT("Invalid watercourse point."));R.Points.Add(XY);}
   IDs.Add(R.Id);Result.Rivers.Add(MoveTemp(R));
  }
 }
 const TArray<TSharedPtr<FJsonValue>>* CitiesJson=nullptr;if(!Root->TryGetArrayField(TEXT("cities"),CitiesJson)||CitiesJson->IsEmpty())return Fail(TEXT("Expected geographic hubs."));
 TSet<FString> HubRegions;
 for(const auto& Value:*CitiesJson)
 {
  const TSharedPtr<FJsonObject>* O=nullptr;if(!Value->TryGetObject(O)||!O->IsValid())return Fail(TEXT("Invalid city object."));FCoIGeoCity C;
  if(!(*O)->TryGetStringField(TEXT("id"),C.Id)||!C.Id.StartsWith(TEXT("city."))||IDs.Contains(C.Id)||!(*O)->TryGetStringField(TEXT("name"),C.Name)||C.Name.IsEmpty()||!(*O)->TryGetStringField(TEXT("country_id"),C.CountryId)||!(*O)->TryGetStringField(TEXT("region_id"),C.RegionId)||HubRegions.Contains(C.RegionId)||!(*O)->TryGetNumberField(TEXT("latitude"),C.Latitude)||!(*O)->TryGetNumberField(TEXT("longitude"),C.Longitude)||!FMath::IsFinite(C.Latitude)||!FMath::IsFinite(C.Longitude)||FMath::Abs(C.Latitude)>=89||FMath::Abs(C.Longitude)>180)return Fail(TEXT("Invalid or duplicate geographic hub."));
  const auto* PositionJson=(*O)->Values.Find(TEXT("position"));if(!PositionJson||!Pair(*PositionJson,C.Position))return Fail(TEXT("Invalid city position."));
  if(!Result.Project(C.Latitude,C.Longitude).Equals(C.Position,.001))return Fail(TEXT("C++/offline projection mismatch."));
  const auto* Region=Result.Regions.FindByPredicate([&](const FCoIGeoSurface& S){return S.Id==C.RegionId;});
  if(!Region||Region->CountryId!=C.CountryId||!Region->Contains(C.Position))return Fail(TEXT("Hub outside assigned region/country."));
  (*O)->TryGetNumberField(TEXT("shore_tolerance_units"),C.ShoreTolerance);
  if(!FMath::IsFinite(C.ShoreTolerance)||C.ShoreTolerance<0||C.ShoreTolerance>7)return Fail(TEXT("Invalid documented shoreline tolerance."));
  if(Result.bGeographyOnly&&!Result.Land.ContainsByPredicate([&](const FCoIGeoSurface& S){return S.Contains(C.Position);}))
  {
   double Distance=MAX_dbl;
   for(const auto& S:Result.Land)for(const auto& Ring:S.Rings)for(int32 I=1;I<Ring.Num();++I)
   {const auto A=Ring[I-1],D=Ring[I]-A;const double L=D.SizeSquared(),T=L>0?FMath::Clamp(FVector2D::DotProduct(C.Position-A,D)/L,0.,1.):0.;Distance=FMath::Min(Distance,(C.Position-A-D*T).Size());}
   if(Distance>C.ShoreTolerance||C.ShoreTolerance<=0)return Fail(TEXT("Hub outside physical land/declared shoreline uncertainty."));
  }
  HubRegions.Add(C.RegionId);IDs.Add(C.Id);Result.Cities.Add(MoveTemp(C));
 }
 if(HubRegions.Num()!=Result.Regions.Num())return Fail(TEXT("Each region requires exactly one hub."));
 *this=MoveTemp(Result);Error.Reset();return true;
}

FCoIGeoHit FCoIGeoMapData::Query(FVector2D Point) const
{
 FCoIGeoHit Hit;
 for(const auto& R:Regions)if((Hit.RegionId.IsEmpty()||R.Id<Hit.RegionId)&&R.Contains(Point)){Hit.RegionId=R.Id;Hit.CountryId=R.CountryId;}
 if(Hit.CountryId.IsEmpty())for(const auto& C:Countries)if(C.Contains(Point)){Hit.CountryId=C.Id;break;}
 for(const auto& W:Lakes)if(W.Contains(Point)){Hit.WaterId=W.Id;break;}
 if(Hit.WaterId.IsEmpty())for(const auto& R:Rivers)
 {
  for(int32 I=1;I<R.Points.Num();++I)
  {
   const auto A=R.Points[I-1],D=R.Points[I]-A;const double Length=D.SizeSquared();
   const double T=Length>0?FMath::Clamp(FVector2D::DotProduct(Point-A,D)/Length,0.,1.):0.;
   if((Point-(A+D*T)).SizeSquared()<=R.HalfWidth*R.HalfWidth){Hit.WaterId=R.Id;break;}
  }
  if(!Hit.WaterId.IsEmpty())break;
 }
 const auto& Physical=Land.IsEmpty()?Countries:Land;
 Hit.bLand=Hit.WaterId.IsEmpty()&&Physical.ContainsByPredicate([&](const FCoIGeoSurface& S){return S.Contains(Point);});
 return Hit;
}

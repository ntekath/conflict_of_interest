#pragma once
#include "CoreMinimal.h"
#include "GeoMapData.h"
// Stable IDs survive camera/view changes. Screen-space city picking matches drawn symbols.
enum class ECoIMapView : uint8 { Regions, Countries, Ownership };
struct FCoISelection
{
 FString Id;
 bool IsEmpty() const { return Id.IsEmpty(); }
};
namespace CoISelection
{
 inline int32 PickCity(const TArray<FVector2D>& Centers,const TArray<bool>& Visible,FVector2D Cursor,double Radius=8)
 {
  int32 Best=INDEX_NONE;double Distance=Radius*Radius;
  for(int32 I=0;I<Centers.Num();++I)if(Visible.IsValidIndex(I)&&Visible[I])
  {double D=(Centers[I]-Cursor).SizeSquared();if(D<=Distance){Distance=D;Best=I;}}
  return Best;
 }
 inline FString PickSurface(const FCoIGeoMapData& Data,FVector2D Point,ECoIMapView View)
 {
  for(const auto& Lake:Data.Lakes)if(Lake.Contains(Point))return {};
  if(View!=ECoIMapView::Countries)for(const auto& Region:Data.Regions)if(Region.Contains(Point))return Region.Id;
  for(const auto& Country:Data.Countries)if(Country.Contains(Point))return Country.Id;
  return {};
 }
 inline FString Pick(const FCoIGeoMapData& Data,FVector2D Point,ECoIMapView View,const TArray<FVector2D>& Centers,const TArray<bool>& Visible,FVector2D Pointer)
 {
  int32 City=PickCity(Centers,Visible,Pointer);
  if(Data.Cities.IsValidIndex(City))return Data.Cities[City].Id;
  return PickSurface(Data,Point,View);
 }
 inline bool Overlaps(FVector2D A,FVector2D AS,FVector2D B,FVector2D BS)
 {return A.X<B.X+BS.X&&A.X+AS.X>B.X&&A.Y<B.Y+BS.Y&&A.Y+AS.Y>B.Y;}
}

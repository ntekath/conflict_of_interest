#pragma once
#include "CoreMinimal.h"
namespace CoIHeader
{
struct FLayout
{
 double Height,NavY,ContentY;
 FVector2D IdentityPosition,IdentitySize,MetricsPosition,MetricsSize,TimePosition,TimeSize;
};
inline FLayout Layout(double ViewportWidth)
{
 const double Inner=FMath::Max(600.,ViewportWidth-56.);
 const bool Compact=ViewportWidth<1700.;
 FLayout L;L.Height=Compact?140.:82.;L.NavY=12.+L.Height+8.;L.ContentY=L.NavY+54.;
 L.IdentityPosition={0,0};L.IdentitySize={Compact?Inner-374.:270.,58};
 L.TimePosition={Inner-358.,0};L.TimeSize={358,62};
 L.MetricsPosition=Compact?FVector2D(0,64):FVector2D(286,0);
 L.MetricsSize={Compact?Inner:Inner-660.,58};return L;
}
}

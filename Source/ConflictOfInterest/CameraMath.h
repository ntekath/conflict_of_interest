#pragma once
#include "CoreMinimal.h"
namespace CoICamera
{
 inline double Zoom(double Height,double Wheel,double Minimum,double Maximum)
 {return FMath::Clamp(Height*FMath::Exp(FMath::Clamp(-Wheel*.16,-40.,40.)),Minimum,Maximum);}
 inline FVector AnchorFocus(FVector Focus,FVector Anchor,double OldHeight,double NewHeight)
 {FVector Result=Anchor+(Focus-Anchor)*(NewHeight/OldHeight);Result.Z=0;return Result;}
 inline FVector ClampFocus(FVector P,FVector2D Minimum,FVector2D Maximum)
 {return FVector(FMath::Clamp(P.X,Minimum.X,Maximum.X),FMath::Clamp(P.Y,Minimum.Y,Maximum.Y),0);}
}

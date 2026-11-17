#pragma once
#include "PlayerView.h"
namespace CoIRoutes {
FVector2D Point(FVector2D A,FVector2D B,double T,bool Own,bool Detail);
double Distance(FVector2D P,FVector2D A,FVector2D B);
double Progress(int64 Hour,int64 Departure,int64 Arrival);
int32 TruckCount(const CoISim::FRouteActivity&,const CoISim::FEconomyRules&,double Zoom,int32 Remaining,bool Animate);
double ExamplePhase(const CoISim::FRouteActivity&,int32 Index,const CoISim::FPlayerView&);
}

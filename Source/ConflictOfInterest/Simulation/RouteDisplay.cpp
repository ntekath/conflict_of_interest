#include "RouteDisplay.h"
#include "SimArchive.h"
namespace CoIRoutes {
FVector2D Point(FVector2D A,FVector2D B,double T,bool Own,bool Detail){auto D=B-A;auto N=FVector2D(-D.Y,D.X).GetSafeNormal();double Bend=Detail?FMath::Min(80.,D.Size()*.16)*(Own?1.:-1.):0.;return FMath::Lerp(A,B,T)+N*(4*T*(1-T)*Bend);}
double Distance(FVector2D P,FVector2D A,FVector2D B){auto D=B-A;double T=D.SizeSquared()?FMath::Clamp(FVector2D::DotProduct(P-A,D)/D.SizeSquared(),0.,1.):0.;return (P-(A+D*T)).Size();}
double Progress(int64 Hour,int64 Departure,int64 Arrival){return Arrival>Departure?FMath::Clamp((double)(Hour-Departure)/(Arrival-Departure),0.,1.):0.;}
int32 TruckCount(const CoISim::FRouteActivity& R,const CoISim::FEconomyRules& E,double Zoom,int32 Remaining,bool Animate){return Animate&&Zoom<E.MapTruckZoom?FMath::Max(0,FMath::Min3((int32)R.Moving,(int32)E.MapTrucksPerRoute,Remaining)):0;}
double ExamplePhase(const CoISim::FRouteActivity& R,int32 Index,const CoISim::FPlayerView& P){if(R.Own){int32 N=0;for(const auto& Id:R.OwnJobs)if(const auto* J=CoISim::Find(P.Shipments,Id);J&&J->Status==TEXT("moving")){if(N++==Index)return Progress(P.Hour,J->DepartureStep,J->ArrivalStep);}}return FMath::Fmod(P.Hour/24.+Index*.31+(GetTypeHash(R.Id)%997)/997.,1.);}
}

#pragma once
#include <cmath>
// Canonical projection parameters. The offline importer reads these same constants.
namespace CoIGeo
{
 constexpr double Pi=3.14159265358979323846, RadiusKm=6371.0088;
 constexpr double OriginLatitude=44.5, OriginLongitude=-84.5;
 constexpr double Parallel1=40.0, Parallel2=48.0, UnitsPerKm=7.0;
 struct Point{double North,East;};
 struct Projection
 {
  double RadiusKm=6371.0088,OriginLatitude=44.5,OriginLongitude=-84.5;
  double Parallel1=40.,Parallel2=48.,UnitsPerKm=7.;
  bool IsValid() const
  {
   return std::isfinite(RadiusKm)&&RadiusKm>0&&std::isfinite(OriginLatitude)&&std::abs(OriginLatitude)<89&&std::isfinite(OriginLongitude)&&std::abs(OriginLongitude)<=180&&std::isfinite(Parallel1)&&std::isfinite(Parallel2)&&std::abs(Parallel1)<89&&std::abs(Parallel2)<89&&std::abs(Parallel1-Parallel2)>1e-6&&Parallel1*Parallel2>0&&std::isfinite(UnitsPerKm)&&UnitsPerKm>0&&UnitsPerKm<10000;
  }
 };
 inline double Radians(double D){return D*Pi/180.;}
 inline double T(double Phi){return std::tan(Pi/4.+Phi/2.);}
 inline Point Project(double Latitude,double Longitude,const Projection& P)
 {
  const double A=Radians(P.Parallel1),B=Radians(P.Parallel2);
  const double N=std::log(std::cos(A)/std::cos(B))/std::log(T(B)/T(A));
  const double F=std::cos(A)*std::pow(T(A),N)/N;
  const double Rho=P.RadiusKm*F/std::pow(T(Radians(Latitude)),N);
  const double Rho0=P.RadiusKm*F/std::pow(T(Radians(P.OriginLatitude)),N);
  const double Theta=N*Radians(Longitude-P.OriginLongitude);
  return {(Rho0-Rho*std::cos(Theta))*P.UnitsPerKm,Rho*std::sin(Theta)*P.UnitsPerKm};
 }
 // Existing US regression fixtures retain their own projection profile.
 inline Point Project(double Latitude,double Longitude){return Project(Latitude,Longitude,Projection{});}
 inline double DistanceKm(double Lat1,double Lon1,double Lat2,double Lon2)
 {
  const double A=std::sin(Radians(Lat2-Lat1)/2),B=std::sin(Radians(Lon2-Lon1)/2);
  double H=A*A+std::cos(Radians(Lat1))*std::cos(Radians(Lat2))*B*B;H=H<0?0:(H>1?1:H);
  return 2*RadiusKm*std::atan2(std::sqrt(H),std::sqrt(1-H));
 }
}

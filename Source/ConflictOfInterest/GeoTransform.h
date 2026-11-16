#pragma once
#include <cmath>
// Canonical projection parameters. The offline importer reads these same constants.
namespace CoIGeo
{
 constexpr double Pi=3.14159265358979323846, RadiusKm=6371.0088;
 constexpr double OriginLatitude=44.5, OriginLongitude=-84.5;
 constexpr double Parallel1=40.0, Parallel2=48.0, UnitsPerKm=7.0;
 struct Point{double North,East;};
 inline double Radians(double D){return D*Pi/180.;}
 inline double T(double Phi){return std::tan(Pi/4.+Phi/2.);}
 inline Point Project(double Latitude,double Longitude)
 {
  const double A=Radians(Parallel1),B=Radians(Parallel2);
  const double N=std::log(std::cos(A)/std::cos(B))/std::log(T(B)/T(A));
  const double F=std::cos(A)*std::pow(T(A),N)/N;
  const double Rho=RadiusKm*F/std::pow(T(Radians(Latitude)),N);
  const double Rho0=RadiusKm*F/std::pow(T(Radians(OriginLatitude)),N);
  const double Theta=N*Radians(Longitude-OriginLongitude);
  return {(Rho0-Rho*std::cos(Theta))*UnitsPerKm,Rho*std::sin(Theta)*UnitsPerKm};
 }
 inline double DistanceKm(double Lat1,double Lon1,double Lat2,double Lon2)
 {
  const double A=std::sin(Radians(Lat2-Lat1)/2),B=std::sin(Radians(Lon2-Lon1)/2);
  double H=A*A+std::cos(Radians(Lat1))*std::cos(Radians(Lat2))*B*B;H=H<0?0:(H>1?1:H);
  return 2*RadiusKm*std::atan2(std::sqrt(H),std::sqrt(1-H));
 }
}

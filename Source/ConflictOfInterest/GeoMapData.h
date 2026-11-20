#pragma once
#include "CoreMinimal.h"
#include "GeoTransform.h"
struct FCoIGeoSurface
{
 FString Id,Name,CountryId;
 bool bTest=false;
 TArray<FVector2D> Vertices;
 TArray<int32> Triangles;
 TArray<TArray<FVector2D>> Rings;
 bool Contains(FVector2D Point) const;
};
struct FCoIGeoCity
{
 FString Id,Name,CountryId,RegionId;
 double Latitude=0,Longitude=0,ShoreTolerance=0;
 FVector2D Position;
};
struct FCoIGeoRiver
{
 FString Id,Name;
 TArray<FVector2D> Points;
 double HalfWidth=0;
};
struct FCoIGeoHit
{
 FString CountryId,RegionId,WaterId;
 bool bLand=false;
};
struct CONFLICTOFINTEREST_API FCoIGeoMapData
{
 FString ScenarioId,StartDate,SimulationFile;
 bool bGeographyOnly=false;
 CoIGeo::Projection Projection;
 TArray<FCoIGeoSurface> Countries,Lakes,Regions;
 TArray<FCoIGeoSurface> Land,RegionLand;
 TArray<FCoIGeoRiver> Rivers;
 TArray<FCoIGeoCity> Cities;
 FVector2D Minimum,Maximum,Center;
 double OverviewHeight=21000;
 bool Load(const FString& Text,FString& Error);
 FVector2D Project(double Latitude,double Longitude) const
 {const auto P=CoIGeo::Project(Latitude,Longitude,Projection);return {P.North,P.East};}
 FCoIGeoHit Query(FVector2D Point) const;
};

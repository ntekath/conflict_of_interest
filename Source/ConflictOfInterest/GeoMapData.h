#pragma once
#include "CoreMinimal.h"
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
 double Latitude=0,Longitude=0;
 FVector2D Position;
};
struct CONFLICTOFINTEREST_API FCoIGeoMapData
{
 FString ScenarioId,StartDate;
 TArray<FCoIGeoSurface> Countries,Lakes,Regions;
 TArray<FCoIGeoCity> Cities;
 FVector2D Minimum,Maximum,Center;
 double OverviewHeight=21000;
 bool Load(const FString& Text,FString& Error);
};

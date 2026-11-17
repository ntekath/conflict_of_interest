#pragma once
#include "PlayerView.h"
namespace CoISim {
struct FMapNavigation {FString CityId,RegionId,SourceInventoryId,TargetStorageId;};
FString MapObjectCity(const FPlayerView&,const FString& Object);
FMapNavigation MapNavigation(const FPlayerView&,const FString& Object,const FString& Good);
}

#pragma once
#include "SimModel.h"
namespace CoISim {
bool MigrateCityHubs(FScenario&,FString&);
bool ValidateCityHubs(const FScenario&,const FState&,bool Initial,FString&);
FString AssetCity(const FState&,const FString&);
}

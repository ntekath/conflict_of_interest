#pragma once
#include "SimModel.h"
namespace CoISim
{
// Bounded organizational history. Neither limit grants economic resources.
constexpr int32 BusinessLimit = 4096, BusinessLegLimit = 256, BusinessTotalLegLimit = 4096;
int64 BusinessAssignedKg(const FTradeBusiness &);
FString BusinessPhase(const FTradeBusiness &, bool OwnSource);
bool BusinessFinished(const FTradeBusiness &, bool OwnSource);
} // namespace CoISim

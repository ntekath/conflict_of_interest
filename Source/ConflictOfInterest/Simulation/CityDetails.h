#pragma once
#include "PlayerView.h"
namespace CoISim
{
FHouseholdView HouseholdDisplay(const FGroup &, const FGroupDef &, const FNeedDef &, const FAccount &,
                                const FEconomyRules &);
FString HouseholdSupply(const FHouseholdNeedView &);
} // namespace CoISim

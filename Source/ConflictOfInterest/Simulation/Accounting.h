#pragma once
#include "SimModel.h"
namespace CoISim
{
int64 AccountingPrice(int64 BaseCents, int64 MarkupPermille, int64 MaxPriceCents);
FAccountingCost AccountingPortion(const FAccountingCost &, int64 Kg, int64 TotalKg);
FAccountingCost AccountingUnit(const FAccountingCost &, int64 Kg);
void AccountingAdd(FAccountingCost &, const FAccountingCost &);
const FAccountingPrice *AccountingFindPrice(const TArray<FAccountingPrice> &, const FString &Actor,
                                            const FString &Storage, const FString &Good);
} // namespace CoISim

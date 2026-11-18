#pragma once
#include "SimModel.h"
namespace CoISim {
struct FInvestmentQuote {int64 CostCents=0,FinishHour=0,Hour=0;uint64 Generation=0,Version=0;FString Summary;};
bool ValidateInvestments(const FScenario&,const FState&,bool Initial,FString&);
}

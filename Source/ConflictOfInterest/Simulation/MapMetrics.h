#pragma once
#include "SimModel.h"
namespace CoISim {
enum class EMapMetric:uint8 { Households,Inputs,Price };
enum class EMetricAvailability:uint8 { Missing,NoNeed,Value };
struct FMetricValue {EMetricAvailability Availability=EMetricAvailability::Missing;double Value=0;int64 Wanted=0,Used=0,TradeKg=0,TradeCents=0;};
FMetricValue MetricValue(const FDayMetrics&,const FString& Region,const FString& Good,EMapMetric Kind);
FLinearColor MetricColor(const FMetricValue&,EMapMetric,const FGoodDef&);
}

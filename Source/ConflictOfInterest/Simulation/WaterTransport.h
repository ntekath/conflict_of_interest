#pragma once
#include "PlayerView.h"
namespace CoISim
{
struct FWaterQuote
{
    FTransportTerms Terms;
    FNetworkPlan Plan;
    FString Reason;
};
FString CarrierCountry(const FDefinitions &, const FString &Provider);
int64 TransportYear(const FString &StartDate, int64 Hour);
FWaterQuote WaterQuote(const FDefinitions &, const FEconomyRules &, const FString &From, const FString &To, bool Own,
                       int64 Hour, int64 Requested, int64 Kg, const FString &Country, const FString &StartDate);
bool WaterFeeders(const FDefinitions &, const TArray<FVehicle> &, const FString &Provider, const FString &From,
                  const FString &To, int64 Kg, TArray<FString> &Ids, FString &Reason);
FString WaterStage(const FDefinitions &, const FTransport &, int64 Hour);
bool ValidateWaterBookings(const FScenario &, const FState &, FString &);
} // namespace CoISim

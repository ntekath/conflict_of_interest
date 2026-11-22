#pragma once
#include "PlayerView.h"
namespace CoISim
{
struct FPurchaseQuote
{
    FTransportTerms Transport;
    int64 GoodsCents = 0, SourceRentCents = 0, DestinationRentDayCents = 0, TotalCents = 0, Points = 0,
          HandlingMinutes = 0;
};
bool QuotePurchase(const FPlayerView &, int64 FreeCents, const FMarketOrder &, const FPurchaseIntent &,
                   FPurchaseQuote &, FString &);
bool ValidPurchaseIntent(const FDefinitions &, const FState &, const FTradeBusiness &, FString &);
} // namespace CoISim

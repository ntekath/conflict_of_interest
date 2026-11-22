#include "CityDetails.h"
namespace CoISim
{
FHouseholdView HouseholdDisplay(const FGroup &G, const FGroupDef &D, const FNeedDef &N, const FAccount &A,
                                const FEconomyRules &Q)
{
    FHouseholdView V;
    V.Id = G.Id;
    V.Band = D.IncomeBand;
    V.Population = G.Population;
    V.Workforce = G.Workforce;
    V.Employed = G.Employed;
    V.IncomeCentsDay = G.DailyIncomeCents;V.PrivateIncomeCentsDay=G.DailyIncomeCents-G.PublicIncomeCentsDay-G.TransferIncomeCentsDay;V.PublicIncomeCentsDay=G.PublicIncomeCentsDay;V.TransferIncomeCentsDay=G.TransferIncomeCentsDay;V.PublicEmployed=G.PublicEmployed;
    V.SpentCents = G.DailySpentCents;
    V.SavingsCents = A.BalanceCents;
    V.FreeSavingsCents = A.BalanceCents - A.ReservedCents;
    V.BudgetCents = G.BudgetCents;
    V.RemainingBudgetCents = FMath::Max(0LL, FMath::Min(G.BudgetCents - G.DailySpentCents, V.FreeSavingsCents));
    V.WealthPermille = G.WealthPermille;
    V.MaxWealthPermille = N.MaxExtraPermille;
    V.EndHour = G.NextBudgetHour;
    V.BeginHour = FMath::Max(0LL, V.EndHour - Q.DemandHours);
    auto Need = [&](const FString &Id, int64 Increment, int64 Remainder) {
        FHouseholdNeedView View;
        View.GoodId = Id;
        const auto *Missing = G.UnmetNeed.FindByPredicate([&](const FAmount &X) { return X.GoodId == Id; });
        View.Known = G.NextBudgetHour > 0 && Missing;
        if (View.Known)
        {
            // Demand already carried the fractional kg forward. Invert that carry exactly,
            // so the UI reads the current period without adding state or running Demand again.
            View.WantedKg = FMath::Max(0LL, (Increment + 999 - Remainder) / 1000);
            View.UsedKg = FMath::Max(0LL, View.WantedKg - Missing->Kg);
            if (Missing->Kg > View.WantedKg)
                View.Known = false;
        }
        return View;
    };
    V.Basic = Need(N.BasicGoodId, G.Population * N.BasicKgPerThousandPerDay, G.BasicRemainder);
    V.Luxury =
        Need(N.LuxuryGoodId,
             G.Population * N.LuxuryKgPerThousandPerDay * (1000 + (Q.HouseholdModel ? G.WealthPermille : 0)) / 1000,
             G.LuxuryRemainder);
    return V;
}
FString HouseholdSupply(const FHouseholdNeedView &V)
{
    if (!V.Known)
        return TEXT("Noch keine Bedarfsdaten");
    if (!V.WantedKg)
        return TEXT("Kein Bedarf");
    return Display::Percent(double(V.UsedKg) / V.WantedKg);
}
} // namespace CoISim

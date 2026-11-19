#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "SimArchive.h"
#include "SimCore.h"
#include "Accounting.h"
namespace CoISim
{
namespace
{
FAmount &RetailAmount(TArray<FAmount> &Rows, const FString &Good)
{
    for (auto &X : Rows)
        if (X.GoodId == Good)
            return X;
    Rows.Add({Good, 0});
    return Rows.Last();
}
int64 RetailWorkers(const FState &W, const FString &Id, int64 H)
{
    const auto *S = Find(W.ServiceEmployment, Id);
    int64 N = 0;
    if (S && S->PaidUntilHour > H)
        for (const auto &E : S->Employment)
            N += E.Workers;
    return N;
}
} // namespace
void FSimulation::RetailPlan(int64 H)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(CoI_RetailPlan);
    TArray<FString> Stores;
    for (const auto &W : State.Warehouses)
        Stores.Add(W.Id);
    for (const auto &StoreId : Stores)
    {
        auto *Live = Find(State.Warehouses, StoreId);
        if (Live->RetailDay != H / 24)
        {
            Live->RetailDay = H / 24;
            Live->RetailSoldKg = Live->RetailRevenueCents = Live->RetailCostCents = 0;
        }
        const auto W = *Live;
        const auto *T = Find(Scenario.Definitions.WarehouseTypes, W.TypeId);
        if (T->RetailOffers.IsEmpty())
            continue;
        Find(State.Warehouses, StoreId)->RetailCause =
            TEXT("Geöffnet; Verkauf nur aus eigenem verfügbarem Bestand, mit bezahltem Personal.");
        if (W.ArrearsCents)
        {
            Find(State.Warehouses, StoreId)->RetailCause = TEXT("Geschlossen: Lagerkosten rückständig.");
            continue;
        }
        if (!RetailWorkers(State, W.Id, H))
        {
            Find(State.Warehouses, StoreId)->RetailCause =
                TEXT("Geschlossen: keine gedeckt bezahlten Ladenstellen bis zum nächsten Lohnlauf.");
            continue;
        }
        const auto *CurrentRule = Find(State.OperatingRules, W.Id);
        const FOperatingRule RuleCopy = CurrentRule ? *CurrentRule : FOperatingRule();
        const auto *Rule = &RuleCopy;
        if (!CurrentRule || !Rule->Enabled)
        {
            Find(State.Warehouses, StoreId)->RetailCause = TEXT("Geschäft durch Betriebsregel deaktiviert.");
            continue;
        }
        if (H % Scenario.Economy.DecisionHours)
            continue;
        for (const auto &Offer : T->RetailOffers)
        {
            const auto *Goal =
                Rule->Goods.FindByPredicate([&](const FStockRule &X) { return X.GoodId == Offer.GoodId; });
            if (!Goal || !Goal->BuyLimitCents || !Goal->TargetKg || !Rule->BuyBudgetCents)
                continue;
            bool Has = false;
            for (const auto &C : State.Deliveries)
                if (C.BuyerId == W.OwnerId && C.DestinationStoreId == W.Id && C.Terms.GoodId == Offer.GoodId &&
                    (C.Status == TEXT("active") ||
                     (C.Status == TEXT("stopped") && C.Id.StartsWith(TEXT("retail.supply.")))))
                {
                    Has = true;
                    Find(State.Warehouses, StoreId)->RetailCause = TEXT("Nachschub: ") + C.Cause;
                }
            if (Has)
                continue;
            int32 I = StockIndex(W.OwnerId, W.Id, Offer.GoodId);
            int64 Have = I == INDEX_NONE ? 0 : State.Inventories[I].QuantityKg - State.Inventories[I].ReservedKg;
            for (const auto &O : State.Orders)
                if (O.Status == TEXT("open") && O.RuleId.IsEmpty() && O.ActorId == W.OwnerId && O.StorageId == W.Id &&
                    O.GoodId == Offer.GoodId && O.Side == TEXT("buy"))
                    Have += O.RemainingKg;
            for (const auto &J : State.Transports)
                if ((J.Status == TEXT("planned") || J.Status == TEXT("moving")) && J.OwnerId == W.OwnerId &&
                    J.DestinationStorageId == W.Id && J.GoodId == Offer.GoodId)
                {
                    const auto *Cargo = Find(State.Inventories, J.CargoInventoryId);
                    Have += Cargo ? Cargo->QuantityKg : J.Kg;
                }
            if (Have >= Goal->TargetKg)
                continue;
            TArray<FMarketOrder> Offers;
            for (const auto &O : State.Orders)
                if (O.Status == TEXT("open") && O.Side == TEXT("sell") && O.GoodId == Offer.GoodId &&
                     O.StorageId != W.Id && (O.ActorId==W.OwnerId||O.LimitCentsPerKg <= Goal->BuyLimitCents))
                    Offers.Add(O);
            Offers.Sort([&](const FMarketOrder &A, const FMarketOrder &B) {
                if((A.ActorId==W.OwnerId)!=(B.ActorId==W.OwnerId))return A.ActorId==W.OwnerId;
                return A.LimitCentsPerKg == B.LimitCentsPerKg ? A.Id < B.Id : A.LimitCentsPerKg < B.LimitCentsPerKg;
            });
            bool Created = false;
            for (const auto &O : Offers)
            {
                FDeliveryRequest R;
                R.Id = TEXT("retail.supply.") + LexToString(State.Deliveries.Num() + 1) + TEXT(".") + W.Id;
                R.OwnerId = W.OwnerId;
                R.Action = TEXT("create");
                auto &X = R.Terms;
                X.SourceAssetId = O.StorageId;
                X.DestinationAssetId = W.Id;
                X.FromCityId = O.CityId;
                X.ToCityId = W.CityId;
                X.GoodId = Offer.GoodId;
                X.Kg = FMath::Min3(Goal->TargetKg, Scenario.Economy.DeliveryMaxKg, O.RemainingKg);
                X.IntervalHours = 24;
                X.StartHour = H;
                X.EndHour = H + Scenario.Economy.DeliveryMaxDurationHours;
                X.PriceCentsPerKg = O.LimitCentsPerKg;
                X.MaxFreightCents = Scenario.Economy.DeliveryDefaultFreightCapCents;
                X.MarketFallback = true;
                FString E;
                if (X.Kg > 0 && ChangeDelivery(R, E))
                {
                    RunDeliveryDue(R.Id, H);
                    Created = true;
                    break;
                }
                Find(State.Warehouses, StoreId)->RetailCause = TEXT("Nachschub wartet: ") + E.Left(300);
            }
            if (!Created && Offers.IsEmpty())
                Find(State.Warehouses, StoreId)->RetailCause =
                    TEXT("Nachschub fehlt: kein bezahlbares öffentliches Quellangebot. Keine Gratisware.");
        }
    }
}
void FSimulation::RetailConsume()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(CoI_RetailConsume);
    // Group order is stable; within each group reweight eligible shops after each allocation pass.
    for (auto &G : State.Groups)
    {
        const auto *GD = Find(Scenario.Definitions.Groups, G.Id);
        const auto *N = Find(Scenario.Definitions.Needs, GD->NeedProfileId);
        for (const auto &Good : {N->BasicGoodId, N->LuxuryGoodId})
        {
            const bool Basic = Good == N->BasicGoodId;
            if (!Basic && RetailAmount(G.UnmetNeed, N->BasicGoodId).Kg > 0)
                break;
            auto &Due = RetailAmount(G.UnmetNeed, Good);
            // Finish legitimate pre-migration household purchases first; they are never purchased twice.
            for (int32 I = 0; I < State.Inventories.Num() && Due.Kg > 0; ++I)
            {
                const auto &Stock = State.Inventories[I];
                const auto *W = Find(State.Warehouses, Stock.StorageId);
                if (Stock.OwnerId != G.Id || Stock.GoodId != Good || !W || W->CityId != GD->ConsumptionCityId)
                    continue;
                int64 Kg = FMath::Min(Due.Kg, Stock.QuantityKg - Stock.ReservedKg);
                TakeSpoilCredit(I, Kg);
                TakeStock(I, Kg);
                Due.Kg -= Kg;
                RetailAmount(G.Consumed, Good).Kg += Kg;
                Find(State.GoodsAudit, Good)->ConsumedKg += Kg;
                DayMetric(GD->RegionId, Good).HouseholdUsedKg += Kg;
            }
            struct FCandidate
            {
                FString Store;
                int64 Weight, Price, Capacity;
            };
            while (Due.Kg > 0)
            {
                auto *A = Find(State.Accounts, G.AccountId);
                int64 Budget = FMath::Min(G.BudgetCents - G.DailySpentCents, A->BalanceCents - A->ReservedCents);
                if (!Basic)
                    Budget = FMath::Min(Budget, G.BudgetCents * N->LuxuryBudgetPermille / 1000 - G.LuxurySpentCents);
                if (Budget <= 0)
                    break;
                TArray<FCandidate> Choices;
                int64 Sum = 0;
                for (const auto &W : State.Warehouses)
                {
                    if (W.CityId != GD->ConsumptionCityId || W.ArrearsCents)
                        continue;
                    const auto *T = Find(Scenario.Definitions.WarehouseTypes, W.TypeId);
                    const auto *O =
                        T->RetailOffers.FindByPredicate([&](const FRetailOffer &X) { return X.GoodId == Good; });
                    const auto *R = Find(State.OperatingRules, W.Id);
                    if (!O || !R || !R->Enabled)
                        continue;
                    const auto *PriceRule =
                        R->Goods.FindByPredicate([&](const FStockRule &X) { return X.GoodId == Good; });
                    int64 Price = PriceRule ? PriceRule->SellLimitCents : 0;if(Scenario.Economy.AccountingEnabled&&PriceRule){const auto* Q=AccountingFindPrice(State.AccountingPrices,W.OwnerId,W.Id,Good);Price=Q&&PriceRule->SellLimitCents?AccountingPrice(Q->Base(),PriceRule->AccountingMarkupPermille,Scenario.Economy.MaxPriceCents):0;}
                    if (Price <= 0 || Price > (Basic ? N->BasicMaxPriceCents : N->LuxuryMaxPriceCents) ||
                        Price > Budget)
                        continue;
                    int64 Workers = RetailWorkers(State, W.Id, State.EconomyHour);
                    int64 Capacity =
                        T->RetailDailyKg * FMath::Min(Workers, T->RetailJobs) / T->RetailJobs - W.RetailSoldKg;
                    int32 I = StockIndex(W.OwnerId, W.Id, Good);
                    if (I == INDEX_NONE || Capacity <= 0)
                        continue;
                    int64 Available = FMath::Min(Capacity, State.Inventories[I].QuantityKg -
                                                               State.Inventories[I].ReservedKg - PriceRule->MinimumKg);
                    if (Available <= 0)
                        continue;
                    int64 Weight = FMath::Max(1LL, O->QualityPermille * 1000 / Price);
                    Choices.Add({W.Id, Weight, Price, Available});
                    Sum += Weight;
                }
                if (Choices.IsEmpty())
                    break;
                Choices.Sort([](const FCandidate &A, const FCandidate &B) { return A.Store < B.Store; });
                const int64 Wanted = Due.Kg;
                int64 Bought = 0;
                for (const auto &Choice : Choices)
                {
                    auto *W = Find(State.Warehouses, Choice.Store);
                    A = Find(State.Accounts, G.AccountId);
                    Budget = FMath::Min(G.BudgetCents - G.DailySpentCents, A->BalanceCents - A->ReservedCents);
                    if (!Basic)
                        Budget =
                            FMath::Min(Budget, G.BudgetCents * N->LuxuryBudgetPermille / 1000 - G.LuxurySpentCents);
                    int64 Share = FMath::Max(1LL, Wanted * Choice.Weight / Sum);
                    int64 Kg = FMath::Min(FMath::Min(Share, Due.Kg),
                                          FMath::Min(Choice.Capacity, FMath::Max(0LL, Budget) / Choice.Price));
                    if (Kg <= 0)
                        continue;
                    const int32 I = StockIndex(W->OwnerId, W->Id, Good);
                    const int64 Cost = Kg * Choice.Price;
                    // All predicates refer to the same committed serial state. No inventory/cash mutation before funded
                    // Pay.
                    if (!Pay(G.AccountId, ActorAccount(W->OwnerId), Cost, TEXT("retail_sale"), W->Id, W->Id))
                        continue;
                    TakeSpoilCredit(I, Kg);
                    int64 Book = TakeStock(I, Kg);AccountingSale(W->OwnerId,Cost,Book);
                    G.DailySpentCents += Cost;
                    if (!Basic)
                        G.LuxurySpentCents += Cost;
                    Due.Kg -= Kg;
                    RetailAmount(G.Consumed, Good).Kg += Kg;
                    Find(State.GoodsAudit, Good)->ConsumedKg += Kg;
                    DayMetric(GD->RegionId, Good).HouseholdUsedKg += Kg;
                    W->RetailSoldKg += Kg;
                    W->RetailRevenueCents += Cost;
                    W->RetailCostCents += Book;
                    Bought += Kg;
                }
                if (!Bought)
                    break;
            }
        }
    }
}
bool FSimulation::ValidateRetail(FString &E) const
{
    auto Fail = [&](const FString &Id) {
        E = TEXT("Ungültiger Einzelhandel: ") + Id;
        return false;
    };
    const auto &D = Scenario.Definitions;
    if (Scenario.Economy.RetailEnabled &&
        (!Scenario.Economy.HouseholdModel || Scenario.Economy.LegacyHouseholdPurchases))
        return Fail(TEXT("Kaufmodi dürfen nicht gleichzeitig aktiv sein"));
    for (const auto &T : D.WarehouseTypes)
    {
        if (T.RetailJobs < 1 || T.RetailJobs > 10000 || T.RetailWageCents < 1 || T.RetailWageCents > 100000 ||
            T.RetailDailyKg < 1 || T.RetailDailyKg > 1000000)
            return Fail(T.Id);
        TSet<FString> Goods;
        for (const auto &X : T.RetailOffers)
        {
            if (Goods.Contains(X.GoodId) || !Find(D.Goods, X.GoodId) || X.BuyLimitCents<0||X.BuyLimitCents>Scenario.Economy.MaxPriceCents||X.PriceCents < 1 ||
                X.PriceCents > Scenario.Economy.MaxPriceCents || X.QualityPermille < 1 || X.QualityPermille > 1000 ||
                X.TargetKg < 0 || X.TargetKg > Scenario.Economy.RuleMaxStockKg)
                return Fail(T.Id);
            Goods.Add(X.GoodId);
        }
    }
    for (const auto &W : State.Warehouses)
    {
        const auto *T = Find(D.WarehouseTypes, W.TypeId);
        if (!T)
            return Fail(W.Id);
        if (W.RetailDay < -1 || W.RetailDay > State.EconomyHour / 24 || W.RetailSoldKg < 0 ||
            W.RetailSoldKg > T->RetailDailyKg || W.RetailRevenueCents < 0 || W.RetailCostCents < 0)
            return Fail(W.Id);
        if (!T->RetailOffers.IsEmpty())
        {
            if (Find(D.Companies, W.OwnerId)->AccountingOnly)
                return Fail(W.Id);
            for (const auto &F : State.Facilities)
                if (F.StorageId == W.Id)
                    return Fail(TEXT("Produzent ist kein separater Laden: ") + W.Id);
        }
    }
    if (Scenario.Economy.RetailEnabled)
        for (const auto &O : State.Orders)
            if (O.Status == TEXT("open") && Find(State.Groups, O.ActorId))
                return Fail(O.Id + TEXT(" Haushalt im B2B-Buch"));
    return true;
}
} // namespace CoISim

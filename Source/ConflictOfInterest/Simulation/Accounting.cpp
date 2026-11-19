#include "Accounting.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "SimArchive.h"
#include "SimCore.h"
namespace CoISim
{
namespace
{
int64 Portion(int64 V, int64 Kg, int64 Total)
{
    return Total ? V / Total * Kg + (V % Total) * Kg / Total : 0;
}
int64 HourShare(int64 Daily, int64 H)
{
    return Daily / 24 + ((Daily % 24) * (H % 24 + 1)) / 24 - ((Daily % 24) * (H % 24)) / 24;
}
int64 CeilUnit(int64 Value, int64 Kg)
{
    return Kg ? (Value + Kg - 1) / Kg : 0;
}
} // namespace
int64 AccountingPrice(int64 Base, int64 Markup, int64 Max)
{
    if (Base < 0 || Markup < 0 || Base > 1000000000000LL || Markup > 100000)
        return 0;
    int64 Price = FMath::Max(1LL, (Base * (1000 + Markup) + 999) / 1000);
    return Price <= Max ? Price : 0;
}
FAccountingCost AccountingPortion(const FAccountingCost &C, int64 Kg, int64 Total)
{
    return {Portion(C.Material, Kg, Total), Portion(C.Labor, Kg, Total), Portion(C.Operating, Kg, Total),
            Portion(C.Freight, Kg, Total)};
}
FAccountingCost AccountingUnit(const FAccountingCost &C, int64 Kg)
{
    if (!Kg)
        return {};
    auto R = AccountingPortion(C, 1, Kg);
    int64 Left = CeilUnit(C.Total(), Kg) - R.Total();
    int64 *Values[] = {&R.Material, &R.Labor, &R.Operating, &R.Freight};
    int64 Parts[] = {C.Material, C.Labor, C.Operating, C.Freight};
    for (int I = 0; I < 4 && Left; ++I)
        if (Parts[I] % Kg)
        {
            ++*Values[I];
            --Left;
        }
    return R;
}
void AccountingAdd(FAccountingCost &A, const FAccountingCost &B)
{
    A.Material += B.Material;
    A.Labor += B.Labor;
    A.Operating += B.Operating;
    A.Freight += B.Freight;
}
const FAccountingPrice *AccountingFindPrice(const TArray<FAccountingPrice> &Rows, const FString &A, const FString &W,
                                            const FString &G)
{
    return Rows.FindByPredicate(
        [&](const FAccountingPrice &X) { return X.ActorId == A && X.StorageId == W && X.GoodId == G; });
}
FAccountingPeriod &FSimulation::AccountingPeriod(const FString &Actor)
{
    FDateTime Start;
    FDateTime::ParseIso8601(*(Scenario.StartDate + TEXT("T00:00:00Z")), Start);
    const auto Now = Start + FTimespan::FromHours(State.EconomyHour);
    FDateTime Begin(Now.GetYear(), Now.GetMonth(), 1);
    FDateTime End =
        Now.GetMonth() == 12 ? FDateTime(Now.GetYear() + 1, 1, 1) : FDateTime(Now.GetYear(), Now.GetMonth() + 1, 1);
    const FString Id = Actor + TEXT("|") + Begin.ToString(TEXT("%Y-%m"));
    if (auto *P = Find(State.AccountingPeriods, Id))
        return *P;
    FAccountingPeriod P;
    P.Id = Id;
    P.ActorId = Actor;
    P.BeginHour = (Begin - Start).GetTicks() / ETimespan::TicksPerHour;
    P.EndHour = (End - Start).GetTicks() / ETimespan::TicksPerHour;
    P.Partial = P.BeginHour < State.AccountingReliableFromHour;
    P.Known = !P.Partial;
    State.AccountingPeriods.Add(P);
    return State.AccountingPeriods.Last();
}
void FSimulation::AccountingPrepaidService(FTransport &J)
{
    if (!Scenario.Economy.AccountingEnabled || !J.AccountingServiceCents)
        return;
    AccountingPeriod(J.OwnerId).ServiceRevenue += J.AccountingServiceCents;
    AccountingPeriod(J.AccountingServicePayerId).FreightExpense += J.AccountingServiceCents;
    J.AccountingServiceCents = 0;
    J.AccountingServicePayerId.Empty();
}
void FSimulation::InitializeAccounting()
{
    if (!Scenario.Economy.AccountingEnabled || State.AccountingStartHour >= 0)
        return;
    State.AccountingStartHour = State.EconomyHour;
    State.AccountingReliableFromHour = State.EconomyHour;
    for (auto &I : State.Inventories)
    {
        I.AccountingCost = {};
        I.AccountingCost.Material = I.BookValueCents;
    }
    for (auto &E : State.ServiceEmployment)
    {
        if (!E.Employment.IsEmpty() && E.PaidUntilHour > State.EconomyHour)
            State.AccountingReliableFromHour = FMath::Max(State.AccountingReliableFromHour, E.PaidUntilHour);
        const auto *W = Find(State.Warehouses, E.Id);
        const auto *T = W ? Find(Scenario.Definitions.WarehouseTypes, W->TypeId) : nullptr;
        E.AccountingPaidDailyWageCents =
            T && !T->RetailOffers.IsEmpty() ? T->RetailWageCents : Scenario.Economy.ServiceWageCents;
    }
    State.AccountingAssets = Scenario.Economy.AccountingOpeningAssets;
    // Reconstruction uses real completed construction costs; unknown acquired legacy assets stay explicitly unknown.
    for (const auto &P : State.Projects)
        if (P.Status == TEXT("complete") && P.TargetId.IsEmpty() && !Find(State.AccountingAssets, P.AssetId))
            State.AccountingAssets.Add({P.AssetId, P.OwnerId, P.CostCents, true});
    for (auto &A : State.AccountingAssets)
    {
        const auto *W = Find(State.Warehouses, A.Id);
        const auto *F = Find(State.Facilities, A.Id);
        const auto *V = Find(State.Vehicles, A.Id);
        FString Owner = W ? W->OwnerId : F ? F->OwnerId : V ? V->OwnerId : TEXT("");
        if (Owner != A.OwnerId)
        {
            A.OwnerId = Owner;
            A.Known = false;
        }
    }
    for (const auto &X : State.DeliverySlots)
        if (auto *J = Find(State.Transports, X.ReturnId))
            if (J->Status == TEXT("planned"))
            {
                const auto *C = Find(State.Deliveries, X.ContractId);
                if (C && C->BuyerId != J->OwnerId)
                {
                    J->AccountingServicePayerId = C->BuyerId;
                    J->AccountingServiceCents = X.ReturnCostCents;
                }
            }
    RefreshAccountingPrices(State.EconomyHour, true);
}
void FSimulation::AccountingPayment(const FString &From, const FString &To, int64 Cents, const FString &Reason)
{
    if (!Scenario.Economy.AccountingEnabled)
        return;
    const bool Service = Reason == TEXT("freight") || Reason == TEXT("haul_operation") ||
                         Reason == TEXT("foreign_storage") || Reason == TEXT("delivery_return_freight") ||
                         Reason == TEXT("storage_fixed") || Reason == TEXT("vehicle_fixed") ||
                         Reason == TEXT("production_operation") || Reason == TEXT("construction") ||
                         Reason == TEXT("asset_upgrade");
    if (Service && Reason != TEXT("delivery_return_freight") && Reason != TEXT("foreign_storage") && From != To &&
        Find(State.Companies, To))
        AccountingPeriod(To).ServiceRevenue += Cents;
    // Return freight is a prepaid service until its actual departure (or non-refundable cancellation).
}
void FSimulation::AccountingSale(const FString &Seller, int64 Revenue, int64 Book)
{
    if (!Scenario.Economy.AccountingEnabled || !Find(State.Companies, Seller))
        return;
    auto &P = AccountingPeriod(Seller);
    P.GoodsRevenue += Revenue;
    P.CostOfGoods += Book;
}
void FSimulation::AccountingCharge(const FString &Asset, const FString &Owner, int64 Amount, int64 H)
{
    if (!Scenario.Economy.AccountingEnabled)
        return;
    auto *C = Find(State.AccountingCharges, Asset);
    if (!C)
    {
        State.AccountingCharges.Add({Asset, Owner, H, Amount});
    }
    else
        *C = {Asset, Owner, H, Amount};
    AccountingPeriod(Owner).Operating += Amount;
}
int64 FSimulation::AccountingChargeHour(const FString &Asset, const FString &Owner, int64 H) const
{
    const auto *C = Find(State.AccountingCharges, Asset);
    return C && C->OwnerId == Owner && C->Hour <= H && H < C->Hour + 24 ? HourShare(C->Amount, H) : 0;
}
void FSimulation::AccountingHour(int64 H)
{
    if (!Scenario.Economy.AccountingEnabled)
        return;
    TRACE_CPUPROFILER_EVENT_SCOPE(CoI_CostRecognition);
    for (const auto &C : State.Companies)
        AccountingPeriod(C.Id);
    // Fixed costs follow actual new daily assessments in Costs(), not current ownership or hypothetical invoices.

    for (const auto &F : State.Facilities)
    {
        auto &P = AccountingPeriod(F.OwnerId);
        if (F.PaidUntilHour > H)
            for (const auto &J : F.Employment)
                P.Wages += HourShare(J.Workers * F.PaidDailyWageCents, H);
    }
    for (const auto &F : State.Facilities)
    {
        int64 Wage = 0;
        if (F.PaidUntilHour > H)
            for (const auto &J : F.Employment)
                Wage += HourShare(J.Workers * F.PaidDailyWageCents, H);
        const auto *W = Find(State.Warehouses, F.StorageId);
        AccountingPeriod(F.OwnerId).ProductionIdle +=
            Wage + AccountingChargeHour(F.Id, F.OwnerId, H) +
            (State.Facilities.FindByPredicate([&](const FFacility &X) { return X.StorageId == W->Id; }) == &F
                 ? AccountingChargeHour(W->Id, F.OwnerId, H)
                 : 0);
    }
    for (const auto &E : State.ServiceEmployment)
        if (E.PaidUntilHour > H)
        {
            const auto *W = Find(State.Warehouses, E.Id);
            const auto *V = Find(State.Vehicles, E.Id);
            if (!W && !V)
                continue;
            int64 Wage = E.AccountingPaidDailyWageCents;
            for (const auto &J : E.Employment)
                AccountingPeriod(W ? W->OwnerId : V->OwnerId).Wages += HourShare(J.Workers * Wage, H);
        }
}
FAccountingCost FSimulation::AccountingProduction(const FFacility &F, int64 B, int64 H)
{
    FAccountingCost C;
    if (!Scenario.Economy.AccountingEnabled)
        return C;
    const auto *T = Find(Scenario.Definitions.FacilityTypes, F.TypeId);
    const auto *W = Find(State.Warehouses, F.StorageId);
    int64 Wage = 0;
    if (F.PaidUntilHour > H)
        for (const auto &E : F.Employment)
            Wage += HourShare(E.Workers * F.PaidDailyWageCents, H);
    int64 Op = AccountingChargeHour(F.Id, F.OwnerId, H) +
               (State.Facilities.FindByPredicate([&](const FFacility &X) { return X.StorageId == W->Id; }) == &F
                    ? AccountingChargeHour(W->Id, F.OwnerId, H)
                    : 0);
    const int64 Normal = FMath::Max(1LL, T->CapacityBatchesPerHour *
                                             Scenario.Economy.AccountingNormalUtilizationPermille / 1000),
                Used = FMath::Min(B, Normal);
    C.Labor = Portion(Wage, Used, Normal);
    C.Operating = Portion(Op, Used, Normal);
    auto &P = AccountingPeriod(F.OwnerId);
    P.CapitalizedWages += C.Labor;
    P.CapitalizedOperating += C.Operating;
    P.ProductionIdle -= C.Labor + C.Operating;
    return C;
}
void FSimulation::RefreshAccountingPrices(int64 H, bool Force)
{
    if (!Scenario.Economy.AccountingEnabled || (!Force && H % Scenario.Economy.AccountingPricingHours))
        return;
    TRACE_CPUPROFILER_EVENT_SCOPE(CoI_CostPriceBasis);
    const int64 BasisVersion = ++State.AccountingPriceSequence;
    TArray<FAccountingPrice> New;
    for (const auto &A : State.Companies)
        for (const auto &W : State.Warehouses)
            for (const auto &G : Scenario.Definitions.Goods)
            {
                FAccountingPrice P;
                P.ActorId = A.Id;
                P.StorageId = W.Id;
                P.GoodId = G.Id;
                P.Id = A.Id + TEXT("|") + W.Id + TEXT("|") + G.Id;
                P.Hour = H;
                P.Version = BasisVersion;
                const int32 Index = StockIndex(A.Id, W.Id, G.Id);
                const auto *Stock = Index == INDEX_NONE ? nullptr : &State.Inventories[Index];
                const auto *Plant = State.Facilities.FindByPredicate(
                    [&](const FFacility &F) { return F.OwnerId == A.Id && F.StorageId == W.Id; });
                P.Startup = !Stock || !Stock->QuantityKg;
                if (!P.Startup)
                    P.UnitCost = AccountingUnit(Stock->AccountingCost, Stock->QuantityKg);
                else
                {
                    P.UnitCost.Material = G.InitialUnitCostCents;
                    if (Plant)
                    {
                        const auto *T = Find(Scenario.Definitions.FacilityTypes, Plant->TypeId);
                        int64 Out = 0, Input = 0;
                        for (const auto &X : T->Outputs)
                            Out += X.Kg;
                        for (const auto &X : T->Inputs)
                        {
                            int32 I = StockIndex(A.Id, W.Id, X.GoodId);
                            Input += X.Kg * (I != INDEX_NONE && State.Inventories[I].QuantityKg
                                                 ? CeilUnit(State.Inventories[I].BookValueCents,
                                                            State.Inventories[I].QuantityKg)
                                                 : Find(Scenario.Definitions.Goods, X.GoodId)->InitialUnitCostCents);
                        }
                        if (!T->Inputs.IsEmpty())
                            P.UnitCost.Material = CeilUnit(Input, FMath::Max(1LL, Out));
                        const int64 Normal =
                            FMath::Max(1LL, 24 * T->CapacityBatchesPerHour * FMath::Max(1LL, Out) *
                                                Scenario.Economy.AccountingNormalUtilizationPermille / 1000);
                        P.UnitCost.Labor = CeilUnit(T->Jobs * OfferedWage(*Plant), Normal);
                        P.UnitCost.Operating =
                            CeilUnit(T->DailyOperatingCostCents +
                                         Find(Scenario.Definitions.WarehouseTypes, W.TypeId)->DailyCostCents,
                                     Normal);
                    }
                }
                if (W.OwnerId == A.Id && !Plant)
                {
                    const auto *T = Find(Scenario.Definitions.WarehouseTypes, W.TypeId);
                    int64 Jobs = !T->RetailOffers.IsEmpty() ? T->RetailJobs
                                 : Find(Scenario.Definitions.Companies, A.Id)->DecisionProfile == TEXT("trader")
                                     ? Scenario.Economy.TraderJobs
                                     : Scenario.Economy.WarehouseJobs;
                    int64 Wage = !T->RetailOffers.IsEmpty() ? T->RetailWageCents : Scenario.Economy.ServiceWageCents;
                    int64 Normal =
                        !T->RetailOffers.IsEmpty()
                            ? FMath::Max(1LL,
                                         T->RetailDailyKg * Scenario.Economy.AccountingNormalUtilizationPermille / 1000)
                            : Scenario.Economy.AccountingNormalTradeKgDay;
                    P.Overhead = CeilUnit(T->DailyCostCents + Jobs * Wage, Normal);
                }
                New.Add(P);
            }
    State.AccountingPrices = MoveTemp(New);
}
void FSimulation::CloseAccounting(int64 Hour)
{
    if (!Scenario.Economy.AccountingEnabled)
        return;
    for (auto &P : State.AccountingPeriods)
        if (P.EndHour <= Hour)
            P.Complete = true;
    const int64 Keep = Scenario.Economy.AccountingHistoryMonths * 31 * 24;
    State.AccountingPeriods.RemoveAll(
        [&](const FAccountingPeriod &P) { return P.Complete && P.EndHour < Hour - Keep; });
}
void FSimulation::AccountingAssetBuy(const FString &Asset, const FString &Buyer, const FString &Seller, int64 Cost)
{
    if (!Scenario.Economy.AccountingEnabled)
        return;
    auto *A = Find(State.AccountingAssets, Asset);
    int64 Value = A ? A->ValueCents : 0;
    bool Known = A && A->Known;
    if (const auto *F = Find(State.Facilities, Asset))
    {
        if (auto *W = Find(State.AccountingAssets, F->StorageId))
        {
            Value += W->ValueCents;
            Known &= W->Known;
            W->OwnerId = Buyer;
            W->ValueCents = 0;
            W->Known = true;
        }
    }
    if (Known)
        AccountingPeriod(Seller).DisposalGain += Cost - Value;
    else
        AccountingPeriod(Seller).Known = false;
    if (!A)
    {
        State.AccountingAssets.Add({Asset, Buyer, Cost, true});
    }
    else
    {
        A->OwnerId = Buyer;
        A->ValueCents = Cost;
        A->Known = true;
    }
}
void FSimulation::AccountingConstruction(const FConstructionProject &P)
{
    if (!Scenario.Economy.AccountingEnabled)
        return;
    auto *A = Find(State.AccountingAssets, P.AssetId);
    if (A)
        A->ValueCents += P.CostCents;
    else
        State.AccountingAssets.Add({P.AssetId, P.OwnerId, P.CostCents, true});
}
bool FSimulation::ValidateAccounting(FString &E) const
{
    if (!Scenario.Economy.AccountingEnabled)
        return true;
    auto Fail = [&](const FString &Why) {
        E = TEXT("Kostenrechnung: ") + Why;
        return false;
    };
    const auto &Q = Scenario.Economy;
    if (Q.AccountingPricingHours < 1 || Q.AccountingPricingHours > 168 || Q.AccountingNormalUtilizationPermille < 1 ||
        Q.AccountingNormalUtilizationPermille > 1000 || Q.AccountingNormalTradeKgDay < 1 ||
        Q.AccountingNormalTradeKgDay > 1000000 || Q.AccountingDefaultMarkupPermille < 0 ||
        Q.AccountingDefaultMarkupPermille > Q.AccountingMaxMarkupPermille || Q.AccountingMaxMarkupPermille > 10000 ||
        Q.AccountingHistoryMonths < 1 || Q.AccountingHistoryMonths > 120)
        return Fail(TEXT("Ungültige Kalkulationsparameter"));
    if (State.AccountingStartHour < 0 || State.AccountingStartHour > State.EconomyHour ||
        State.AccountingReliableFromHour < State.AccountingStartHour ||
        State.AccountingReliableFromHour > State.AccountingStartHour + Q.PayrollHours ||
        State.AccountingPrices.Num() > 10000 || State.AccountingPeriods.Num() > 10000)
        return Fail(TEXT("Ungültiger Start oder Datenumfang"));
    for (const auto &I : State.Inventories)
        if (I.AccountingCost.Material < 0 || I.AccountingCost.Labor < 0 || I.AccountingCost.Operating < 0 ||
            I.AccountingCost.Freight < 0 || I.AccountingCost.Total() != I.BookValueCents ||
            (!I.QuantityKg && I.BookValueCents))
            return Fail(TEXT("Warenkosten: ") + I.Id);
    TSet<FString> Seen;
    for (const auto &P : State.AccountingPeriods)
    {
        if (Seen.Contains(P.Id) || !Find(State.Companies, P.ActorId) || P.EndHour <= P.BeginHour ||
            P.Complete != (P.EndHour <= State.EconomyHour) || P.ProductionIdle < 0 || P.GoodsRevenue < 0 ||
            P.ServiceRevenue < 0 || P.CostOfGoods < 0 || P.Wages < 0 || P.Operating < 0 || P.CapitalizedWages < 0 ||
            P.CapitalizedWages > P.Wages || P.CapitalizedOperating < 0 || P.CapitalizedOperating > P.Operating ||
            P.FreightExpense < 0 || P.Spoilage < 0)
            return Fail(TEXT("Monatsbeleg: ") + P.Id);
        Seen.Add(P.Id);
    }
    Seen.Reset();
    for (const auto &P : State.AccountingPrices)
    {
        if (Seen.Contains(P.Id) || !Find(State.Companies, P.ActorId) || !Find(State.Warehouses, P.StorageId) ||
            !Find(Scenario.Definitions.Goods, P.GoodId) || P.Version < 1 || P.Version > State.AccountingPriceSequence ||
            P.Hour > State.EconomyHour || P.Base() < 0 || P.Base() > 1000000000000LL || P.UnitCost.Material < 0 ||
            P.UnitCost.Labor < 0 || P.UnitCost.Operating < 0 || P.UnitCost.Freight < 0 || P.Overhead < 0)
            return Fail(TEXT("Preisbasis: ") + P.Id);
        Seen.Add(P.Id);
    }
    for (const auto &J : State.Transports)
        if (J.AccountingServiceCents < 0 || J.AccountingServiceCents > 1000000000LL ||
            (J.AccountingServiceCents &&
             (!J.Empty || J.DeliveryId.IsEmpty() || !Find(State.Companies, J.AccountingServicePayerId) ||
              J.Status != TEXT("planned"))))
            return Fail(TEXT("Vorausbezahlte Rückfracht: ") + J.Id);
    Seen.Reset();
    for (const auto &C : State.AccountingCharges)
    {
        if (Seen.Contains(C.Id) || !Find(State.Companies, C.OwnerId) || C.Hour < 0 || C.Hour > State.EconomyHour ||
            C.Hour % 24 || C.Amount < 0 || C.Amount > 1000000000000LL ||
            (!Find(State.Warehouses, C.Id) && !Find(State.Facilities, C.Id) && !Find(State.Vehicles, C.Id)))
            return Fail(TEXT("Fixkostenbeleg: ") + C.Id);
        Seen.Add(C.Id);
    }
    Seen.Reset();
    for (const auto &A : State.AccountingAssets)
    {
        if (Seen.Contains(A.Id) || A.ValueCents < 0 || (!A.OwnerId.IsEmpty() && !Find(State.Companies, A.OwnerId)))
            return Fail(TEXT("Assetbuchwert: ") + A.Id);
        Seen.Add(A.Id);
    }
    return true;
}
} // namespace CoISim

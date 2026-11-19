#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "SimArchive.h"
#include "SimCore.h"
namespace CoISim
{
void FSimulation::HouseholdPayroll(int64 H)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(CoI_HouseholdPayroll);
    struct FJob
    {
        FString Id, Owner, City, Site;
        int64 Needed, Wage;
        FFacility *Facility;
        FServiceEmployment *Service;
    };
    for (auto &G : State.Groups)
    {
        G.LastIncomeCents = G.DailyIncomeCents;
        G.DailyIncomeCents = 0;
        G.Employed = 0;
    }
    State.ServiceEmployment.Reset();
    // Allocate array slots first: pointers below remain stable throughout settlement.
    for (const auto &W : State.Warehouses)
    {
        bool Plant = false;
        for (const auto &F : State.Facilities)
            Plant |= F.StorageId == W.Id;
        if (!Plant)
        {
            FServiceEmployment X;
            X.Id = W.Id;
            State.ServiceEmployment.Add(X);
        }
    }
    for (const auto &V : State.Vehicles)
    {
        FServiceEmployment X;
        X.Id = V.Id;
        State.ServiceEmployment.Add(X);
    }
    TArray<FJob> Jobs;
    for (auto &F : State.Facilities)
    {
        F.Employment.Reset();
        F.PaidUntilHour = H;
        F.PaidDailyWageCents = OfferedWage(F);
        F.UnfundedJobs = PlannedJobs(F);
        if (Scenario.Economy.Stage < 4 ||
            (F.OperatingPaidUntilHour > H && !Find(State.Warehouses, F.StorageId)->ArrearsCents))
            Jobs.Add({F.Id, F.OwnerId, F.CityId, F.StorageId, PlannedJobs(F), OfferedWage(F), &F, nullptr});
    }
    const auto &Q = Scenario.Economy;
    for (auto &S : State.ServiceEmployment)
    {
        if (const auto *W = Find(State.Warehouses, S.Id))
            Jobs.Add({W->Id, W->OwnerId, W->CityId, W->Id,
                      Scenario.Economy.RetailEnabled&&!Find(Scenario.Definitions.WarehouseTypes,W->TypeId)->RetailOffers.IsEmpty()?Find(Scenario.Definitions.WarehouseTypes,W->TypeId)->RetailJobs:Find(Scenario.Definitions.Companies, W->OwnerId)->DecisionProfile == TEXT("trader")
                          ? Q.TraderJobs
                          : Q.WarehouseJobs,
                      Scenario.Economy.RetailEnabled&&!Find(Scenario.Definitions.WarehouseTypes,W->TypeId)->RetailOffers.IsEmpty()?Find(Scenario.Definitions.WarehouseTypes,W->TypeId)->RetailWageCents:Q.ServiceWageCents, nullptr, &S});
        else if (const auto *V = Find(State.Vehicles, S.Id))
            Jobs.Add({V->Id, V->OwnerId, V->HomeCityId, V->Id, Q.VehicleJobs, Q.ServiceWageCents, nullptr, &S});
    }
    Jobs.Sort([](const FJob &A, const FJob &B) { return A.Wage == B.Wage ? A.Id < B.Id : A.Wage > B.Wage; });
    for (auto &J : Jobs)
    {
        auto *A = Find(State.Accounts, ActorAccount(J.Owner));
        int64 Wage = J.Wage * (Q.PayrollHours / 24);
        int64 Left = FMath::Min(J.Needed, (A->BalanceCents - A->ReservedCents) / FMath::Max(1LL, Wage));
        auto &Employment = J.Facility ? J.Facility->Employment : J.Service->Employment;
        for (auto &G : State.Groups)
        {
            if (Find(Scenario.Definitions.Groups, G.Id)->RegionId !=
                Find(Scenario.Definitions.Places, J.City)->RegionId)
                continue;
            int64 N = FMath::Min(Left, G.Workforce - G.Employed);
            if (N <= 0)
                continue;
            if (!Pay(A->Id, G.AccountId, N * Wage, TEXT("wages"), J.Site))
                continue;
            Employment.Add({G.Id, N});
            G.Employed += N;
            G.DailyIncomeCents += N * J.Wage;
            Left -= N;
        }
        if (J.Facility)
        {
            int64 Count = 0;
            for (const auto &E : Employment)
                Count += E.Workers;
            J.Facility->UnfundedJobs = J.Needed - Count;
            if (Count)
                J.Facility->PaidUntilHour = H + Q.PayrollHours;
        }
        else{
            J.Service->PaidUntilHour = H + Q.PayrollHours;
            if(Scenario.Economy.AccountingEnabled)J.Service->AccountingPaidDailyWageCents=J.Wage;
        }
    }
}
bool FSimulation::ValidateHouseholds(FString &E) const
{
    const auto &Q = Scenario.Economy;
    if (Q.WarehouseJobs < 0 || Q.WarehouseJobs > 10000 || Q.TraderJobs < 0 || Q.TraderJobs > 10000 ||
        Q.VehicleJobs < 0 || Q.VehicleJobs > 100 || Q.ServiceWageCents < 1 || Q.ServiceWageCents > 100000)
    {
        E = TEXT("Invalid service employment parameters");
        return false;
    }
    for (const auto &N : Scenario.Definitions.Needs)
        if (N.SavingPermille < 0 || N.SavingPermille > 1000 || N.WealthDays < 2 || N.WealthDays > 365 ||
            N.MaxExtraPermille < 0 || N.MaxExtraPermille > 1000)
        {
            E = TEXT("Invalid household profile");
            return false;
        }
    for (const auto &G : State.Groups)
        if (G.WealthPermille < 0 || G.WealthPermille > 1000 || G.LastIncomeCents < 0 || G.LastSpentCents < 0 ||
            G.NextBudgetHour < 0 || G.NextBudgetHour > State.EconomyHour + Q.DemandHours)
        {
            E = TEXT("Invalid household state: ") + G.Id;
            return false;
        }
    TSet<FString> Seen;
    for (const auto &S : State.ServiceEmployment)
    {
        const auto *W = Find(State.Warehouses, S.Id);
        const auto *V = Find(State.Vehicles, S.Id);
        if (Seen.Contains(S.Id) || (!W && !V) || S.PaidUntilHour < 0 ||
            S.PaidUntilHour > State.EconomyHour + Q.PayrollHours || (Q.AccountingEnabled && (S.AccountingPaidDailyWageCents < 0 || S.AccountingPaidDailyWageCents > 1000000 || (!S.Employment.IsEmpty() && !S.AccountingPaidDailyWageCents))))
        {
            E = TEXT("Invalid employment asset: ") + S.Id;
            return false;
        }
        Seen.Add(S.Id);
        TSet<FString> Groups;
        for (const auto &J : S.Employment)
        {
            const auto *G = Find(State.Groups, J.GroupId);
            const auto *D = Find(Scenario.Definitions.Groups, J.GroupId);
            if (!G || !D || Groups.Contains(J.GroupId) || J.Workers <= 0 || J.Workers > G->Workforce ||
                D->RegionId != Find(Scenario.Definitions.Places, W ? W->CityId : V->HomeCityId)->RegionId)
            {
                E = TEXT("Invalid service workers: ") + S.Id;
                return false;
            }
            Groups.Add(J.GroupId);
        }
    }
    return true;
}
} // namespace CoISim

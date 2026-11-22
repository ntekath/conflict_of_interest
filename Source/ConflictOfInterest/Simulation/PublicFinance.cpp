#include "PublicFinance.h"
#include "SimCore.h"
#include "SimArchive.h"
namespace CoISim
{
FString PublicCountry(const FDefinitions &D, const FString &Actor)
{
    const auto *C = Find(D.Companies, Actor);
    const auto *P = C ? Find(D.Places, C->HeadquartersCityId) : nullptr;
    const auto *R = P ? Find(D.Regions, P->RegionId) : nullptr;
    return R ? R->CountryId : FString();
}
int64 PublicTaxAmount(int64 Profit, int64 Rate)
{
    const int64 B = FMath::Max(0LL, Profit);
    return B / 1000 * Rate + (B % 1000) * Rate / 1000;
}
bool ValidatePublicScenario(const FScenario &S, FString &E)
{
    const auto &Q = S.Economy.Public;
    auto Fail = [&](const FString &M) {
        E = TEXT("Staatshaushalt: ") + M;
        return false;
    };
    if (!S.InitialState.PublicTreasuries.IsEmpty() || !S.InitialState.PublicClaims.IsEmpty() ||
        !S.InitialState.PublicTaxes.IsEmpty())
        return Fail(TEXT("Initiale Laufzeitdaten müssen leer sein."));
    if (Q.Treasuries.IsEmpty())
        return !Q.Enabled && Q.Offices.IsEmpty() ? true : Fail(TEXT("Nationale Haushalte fehlen."));
    if (!S.Economy.HouseholdModel || !S.Economy.AccountingEnabled || S.Economy.PayrollHours != 24 ||
        Q.TransferHours != 24 || Q.TransferCentsPersonDay < 0 || Q.TransferCentsPersonDay > 100000 ||
        Q.TransferMaxPeopleGroup < 0 || Q.TransferMaxPeopleGroup > 1000000 || Q.TransferDurationDays < 1 ||
        Q.TransferDurationDays > 365 || Q.MaxSavingsCentsPerson < 0 || Q.MaxSavingsCentsPerson > 100000000 ||
        Q.EligibleBand.IsEmpty())
        return Fail(TEXT("Ungültige Regeln/Perioden; öffentliches Minimalmodell benötigt tägliche Löhne."));
    TSet<FString> Countries, Ids, Accounts, Regions;
    for (const auto &R : S.Definitions.Regions)
        Countries.Add(R.CountryId);
    for (const auto &T : Q.Treasuries)
    {
        const auto *A = Find(S.InitialState.Accounts, T.AccountId);
        if (!Countries.Contains(T.Id) || Ids.Contains(T.Id) || Accounts.Contains(T.AccountId) || T.Name.IsEmpty() ||
            T.OpeningReserveCents < 0 || T.OpeningReserveCents > 1000000000000LL || T.TaxPermille < 0 ||
            T.TaxPermille > 1000 || T.TransferCapCentsDay < 0 || T.TransferCapCentsDay > 100000000 || !A ||
            A->OwnerId != T.Id || A->BalanceCents != T.OpeningReserveCents || A->ReservedCents ||
            Find(S.Definitions.Companies, T.Id) || Find(S.Definitions.Groups, T.Id))
            return Fail(T.Id);
        Ids.Add(T.Id);
        Accounts.Add(T.AccountId);
    }
    if (Ids.Num() != Countries.Num())
        return Fail(TEXT("Ein Haushalt je simuliertem Land erforderlich."));
    Ids.Reset();
    for (const auto &O : Q.Offices)
    {
        const auto *R = Find(S.Definitions.Regions, O.RegionId);
        const auto *P = Find(S.Definitions.Places, O.CityId);
        if (O.Id.IsEmpty() || Ids.Contains(O.Id) || Regions.Contains(O.RegionId) || !R || !P || P->RegionId != R->Id ||
            R->CountryId != O.CountryId || !Find(Q.Treasuries, O.CountryId) || O.Jobs < 0 || O.Jobs > 10000 ||
            O.DailyWageCents < 1 || O.DailyWageCents > 100000 || Find(S.InitialState.Warehouses, O.Id) ||
            Find(S.InitialState.Vehicles, O.Id) || Find(S.InitialState.Facilities, O.Id))
            return Fail(O.Id);
        Ids.Add(O.Id);
        Regions.Add(O.RegionId);
    }
    if (Regions.Num() != S.Definitions.Regions.Num())
        return Fail(TEXT("Öffentliche Ausgabenstelle je Region fehlt."));
    return true;
}
void FSimulation::InitializePublic()
{
    State.PublicTreasuries.Reset();
    State.PublicClaims.Reset();
    State.PublicTaxes.Reset();
    if (Scenario.Economy.Public.Treasuries.IsEmpty())
        return;
    for (const auto &T : Scenario.Economy.Public.Treasuries)
    {
        FPublicTreasury X;
        X.Id = T.Id;
        State.PublicTreasuries.Add(X);
    }
    for (const auto &G : Scenario.Definitions.Groups)
    {
        FPublicClaim X;
        X.Id = G.Id;
        X.CountryId = Find(Scenario.Definitions.Regions, G.RegionId)->CountryId;
        State.PublicClaims.Add(X);
    }
    for (const auto &C : Scenario.Definitions.Companies)
        if (!C.AccountingOnly)
        {
            FPublicTax X;
            X.Id = C.Id;
            X.CountryId = PublicCountry(Scenario.Definitions, C.Id);
            State.PublicTaxes.Add(X);
        }
}
void FSimulation::PublicPayment(const FString &From, const FString &To, int64 Cents, const FString &Reason)
{
    if (Reason == TEXT("public_wages"))
    {
        if (auto *T = Find(State.PublicTreasuries, From))
            T->WagesPaid += Cents;
    }
    else if (Reason == TEXT("public_transfer"))
    {
        if (auto *T = Find(State.PublicTreasuries, From))
            T->TransfersPaid += Cents;
    }
    else if (Reason == TEXT("public_tax"))
    {
        if (auto *T = Find(State.PublicTreasuries, To))
            T->TaxPaid += Cents;
    }
}
void FSimulation::PublicRevenue(int64 H)
{
    const auto &Q = Scenario.Economy.Public;
    if (!Q.Enabled)
        return;
    for (auto &X : State.PublicTaxes)
    {
        const auto *Rule = Find(Q.Treasuries, X.CountryId);
        auto *Budget = Find(State.PublicTreasuries, X.CountryId);
        // Copy before creating the current-period ledger, which may grow/reallocate AccountingPeriods.
        TArray<FAccountingPeriod> Closed;
        for (const auto &P : State.AccountingPeriods)
            if (P.ActorId == X.Id && P.Complete && P.Known && !P.Partial && P.EndHour > X.LastPeriodEnd &&
                P.EndHour <= H)
                Closed.Add(P);
        Closed.Sort([](const auto &A, const auto &B) { return A.EndHour < B.EndHour; });
        for (const auto &P : Closed)
        {
            X.LastPeriodEnd = P.EndHour;
            X.LastBase = FMath::Max(0LL, P.Profit() + P.TaxExpense);
            X.LastAssessment = PublicTaxAmount(X.LastBase, Rule->TaxPermille);
            X.Assessed += X.LastAssessment;
            Budget->TaxAssessed += X.LastAssessment;
            AccountingPeriod(X.Id).TaxExpense += X.LastAssessment;
        }
        auto *A = Find(State.Accounts, ActorAccount(X.Id));
        const int64 Payable = FMath::Min(X.Assessed - X.Paid, A->BalanceCents - A->ReservedCents);
        if (Payable && Pay(A->Id, Rule->AccountId, Payable, TEXT("public_tax")))
            X.Paid += Payable;
    }
}
void FSimulation::PublicTransfers(int64 H)
{
    const auto &Q = Scenario.Economy.Public;
    if (!Q.Enabled)
        return;
    for (auto &Budget : State.PublicTreasuries)
    {
        if (H < Budget.NextTransferHour)
            continue;
        Budget.NextTransferHour = H + Q.TransferHours;
        const auto *Rule = Find(Q.Treasuries, Budget.Id);
        auto *Treasury = Find(State.Accounts, Rule->AccountId);
        int64 Cap = Rule->TransferCapCentsDay;
        TArray<FString> Order;
        for (auto &X : State.PublicClaims)
            if (X.CountryId == Budget.Id)
            {
                auto *G = Find(State.Groups, X.Id);
                G->DailyIncomeCents -= G->TransferIncomeCentsDay;
                G->TransferIncomeCentsDay = 0;
                X.LastDay = H / 24;
                X.LastRecipients = X.LastDue = X.LastPaid = 0;
                Order.Add(X.Id);
            }
        Order.Sort([&](const FString &A, const FString &B) {
            const auto *GA = Find(State.Groups, A);
            const auto *GB = Find(State.Groups, B);
            const auto *AA = Find(State.Accounts, GA->AccountId);
            const auto *AB = Find(State.Accounts, GB->AccountId);
            const int64 SA = (AA->BalanceCents - AA->ReservedCents) / FMath::Max(1LL, GA->Population),
                        SB = (AB->BalanceCents - AB->ReservedCents) / FMath::Max(1LL, GB->Population);
            return SA == SB ? A < B : SA < SB;
        });
        for (const auto &Id : Order)
        {
            auto *X = Find(State.PublicClaims, Id);
            auto *G = Find(State.Groups, Id);
            const auto *D = Find(Scenario.Definitions.Groups, Id);
            const auto *A = Find(State.Accounts, G->AccountId);
            const int64 Eligible = FMath::Min(Q.TransferMaxPeopleGroup, G->Workforce - G->Employed);
            if (D->IncomeBand != Q.EligibleBand || G->Population <= 0 || Eligible <= 0 ||
                (A->BalanceCents - A->ReservedCents) > G->Population * Q.MaxSavingsCentsPerson)
                continue;
            if (X->FirstDay < 0)
                X->FirstDay = H / 24;
            if (H / 24 >= X->FirstDay + Q.TransferDurationDays)
                continue;
            X->LastRecipients = Eligible;
            X->LastDue = Eligible * Q.TransferCentsPersonDay;
            X->Assessed += X->LastDue;
            const int64 Amount = FMath::Min3(X->LastDue, Cap, Treasury->BalanceCents - Treasury->ReservedCents);
            if (Amount && Pay(Treasury->Id, G->AccountId, Amount, TEXT("public_transfer")))
            {
                X->LastPaid = Amount;
                X->Paid += Amount;
                Cap -= Amount;
                G->TransferIncomeCentsDay = Amount;
                G->DailyIncomeCents += Amount;
            }
            Budget.TransferCuts += X->LastDue - X->LastPaid;
        }
    }
}
bool ValidatePublicState(const FScenario &S, const FState &W, FString &E)
{
    const auto &Q = S.Economy.Public;
    auto Fail = [&](const FString &M) {
        E = TEXT("Öffentliche Bilanz: ") + M;
        return false;
    };
    if (Q.Treasuries.IsEmpty())
        return W.PublicTreasuries.IsEmpty() && W.PublicClaims.IsEmpty() && W.PublicTaxes.IsEmpty()
                   ? true
                   : Fail(TEXT("unerwartete Haushalte"));
    if (W.PublicTreasuries.Num() != Q.Treasuries.Num() || W.PublicClaims.Num() != W.Groups.Num())
        return Fail(TEXT("Abdeckung"));
    TSet<FString> Seen;
    for (const auto &B : W.PublicTreasuries)
    {
        const auto *R = Find(Q.Treasuries, B.Id);
        const auto *A = R ? Find(W.Accounts, R->AccountId) : nullptr;
        if (!R || !A || Seen.Contains(B.Id) || A->ReservedCents || B.TaxAssessed < 0 || B.TaxPaid < 0 ||
            B.TaxPaid > B.TaxAssessed || B.WagesPaid < 0 || B.TransfersPaid < 0 || B.TransferCuts < 0 ||
            B.LastUnfundedJobs < 0 || B.LastVacantJobs < 0 || B.NextTransferHour < 0 ||
            B.NextTransferHour > W.EconomyHour + 24 ||
            A->BalanceCents != R->OpeningReserveCents + B.TaxPaid - B.WagesPaid - B.TransfersPaid)
            return Fail(B.Id);
        Seen.Add(B.Id);
        int64 Tax = 0, PaidTax = 0, Transfers = 0, Cuts = 0;
        for (const auto &X : W.PublicTaxes)
            if (X.CountryId == B.Id)
            {
                Tax += X.Assessed;
                PaidTax += X.Paid;
            }
        for (const auto &C : W.PublicClaims)
            if (C.CountryId == B.Id)
            {
                Transfers += C.Paid;
                Cuts += C.Assessed - C.Paid;
            }
        if (Tax != B.TaxAssessed || PaidTax != B.TaxPaid || Transfers != B.TransfersPaid || Cuts != B.TransferCuts)
            return Fail(B.Id + TEXT(" Summen"));
    }
    Seen.Reset();
    int32 Payers = 0;
    for (const auto &C : S.Definitions.Companies)
        Payers += !C.AccountingOnly;
    if (W.PublicTaxes.Num() != Payers)
        return Fail(TEXT("Steuerpflichtige"));
    for (const auto &X : W.PublicTaxes)
    {
        const auto *C = Find(S.Definitions.Companies, X.Id);
        if (!C || C->AccountingOnly || Seen.Contains(X.Id) || X.CountryId != PublicCountry(S.Definitions, X.Id) ||
            X.Assessed < 0 || X.Paid < 0 || X.Paid > X.Assessed || X.LastBase < 0 || X.LastAssessment < 0 ||
            X.LastAssessment != PublicTaxAmount(X.LastBase, Find(Q.Treasuries, X.CountryId)->TaxPermille) ||
            X.LastPeriodEnd < 0 || X.LastPeriodEnd > W.EconomyHour)
            return Fail(X.Id);
        Seen.Add(X.Id);
    }
    Seen.Reset();
    TMap<FString, int64> PublicWorkers, PublicIncome;
    for (const auto &Staff : W.ServiceEmployment)
        if (const auto *Office = Find(Q.Offices, Staff.Id))
        {
            if (!Q.Enabled)
                return Fail(TEXT("Deaktivierter öffentlicher Arbeitgeber"));
            int64 Workers = 0;
            for (const auto &J : Staff.Employment)
            {
                Workers += J.Workers;
                PublicWorkers.FindOrAdd(J.GroupId) += J.Workers;
                PublicIncome.FindOrAdd(J.GroupId) += J.Workers * Office->DailyWageCents;
            }
            if (Workers > Office->Jobs)
                return Fail(Office->Id + TEXT(" Stellenlimit"));
        }
    for (const auto &X : W.PublicClaims)
    {
        const auto *G = Find(W.Groups, X.Id);
        const auto *D = Find(S.Definitions.Groups, X.Id);
        if (!G || !D || Seen.Contains(X.Id) || X.CountryId != Find(S.Definitions.Regions, D->RegionId)->CountryId ||
            X.FirstDay < -1 || X.FirstDay > W.EconomyHour / 24 || X.LastDay < -1 || X.LastDay > W.EconomyHour / 24 ||
            X.LastRecipients < 0 || X.LastRecipients > Q.TransferMaxPeopleGroup || X.LastDue < 0 ||
            X.LastDue != X.LastRecipients * Q.TransferCentsPersonDay || X.LastPaid < 0 || X.LastPaid > X.LastDue ||
            X.Assessed < 0 || X.Paid < 0 || X.Paid > X.Assessed ||
            (X.LastDue &&
             (D->IncomeBand != Q.EligibleBand || X.FirstDay < 0 || X.LastDay >= X.FirstDay + Q.TransferDurationDays)))
            return Fail(X.Id);
        Seen.Add(X.Id);
        if (Q.Enabled &&
            (G->PublicEmployed < 0 || G->PublicEmployed > G->Employed || G->PrivateIncomeCentsDay < 0 ||
             G->PublicIncomeCentsDay < 0 || G->TransferIncomeCentsDay < 0 ||
             G->PublicEmployed != PublicWorkers.FindRef(G->Id) ||
             G->PublicIncomeCentsDay != PublicIncome.FindRef(G->Id) || G->TransferIncomeCentsDay != X.LastPaid ||
             G->DailyIncomeCents != G->PrivateIncomeCentsDay + G->PublicIncomeCentsDay + G->TransferIncomeCentsDay))
            return Fail(G->Id + TEXT(" Einkommen"));
    }
    if (!Q.Enabled)
        for (const auto &B : W.PublicTreasuries)
            if (B.TaxAssessed || B.TaxPaid || B.WagesPaid || B.TransfersPaid || B.TransferCuts || B.NextTransferHour)
                return Fail(TEXT("deaktivierter Haushalt mit Zahlungen"));
    return true;
}
FString PublicDiagnostics(const FScenario &S, const FState &W)
{
    if (S.Economy.Public.Treasuries.IsEmpty())
        return TEXT("");
    FString T = TEXT("\nAP09.5a · Öffentliche Dienste sind abstrakt; keine Warenproduktion. Staatskredit aus; "
                     "Schuldendienst 0 Cent.\n");
    for (const auto &B : W.PublicTreasuries)
    {
        const auto *R = Find(S.Economy.Public.Treasuries, B.Id);
        const auto *A = Find(W.Accounts, R->AccountId);
        T +=
            FString::Printf(TEXT("%s · Konto %s · Start %lld / aktuell %lld Cent\nAbgabe: fällig %lld / bezahlt %lld / "
                                 "offen %lld · Löhne %lld · Transfers %lld · gekürzt %lld Cent\nLetzter Lohnlauf: %lld "
                                 "Stellen ungedeckt / %lld finanzierbar, aber unbesetzt · nächster Transfer h%lld\n"),
                            *R->Name, *R->AccountId, R->OpeningReserveCents, A->BalanceCents, B.TaxAssessed, B.TaxPaid,
                            B.TaxAssessed - B.TaxPaid, B.WagesPaid, B.TransfersPaid, B.TransferCuts, B.LastUnfundedJobs,
                            B.LastVacantJobs, B.NextTransferHour);
    }
    return T;
}
} // namespace CoISim

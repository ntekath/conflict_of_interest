#include "Banking.h"
#include "SimCore.h"
#include "SimArchive.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
namespace CoISim
{
int64 BankMoneyChange(const FBankState &B)
{
    return B.CreatedCents - B.RepaidCents - B.InterestCents + B.WagesCents + B.OperatingCents;
}
int64 BankEquity(const FBankRules &Q, const FBankState &B)
{
    return Q.OpeningCapitalCents + B.InterestCents - B.WagesCents - B.OperatingCents - B.OperatingArrearsCents -
           B.WriteOffCents;
}
int64 BankSpendable(const FBankRules &Q, const FBankState &B)
{
    return FMath::Max(0LL, FMath::Min(B.InterestCents * Q.SpendPermille / 1000 - B.WagesCents - B.OperatingCents,
                                      B.InterestCents - B.WagesCents - B.OperatingCents - B.WriteOffCents));
}
bool Pledged(const FState &S, const FString &Asset)
{
    for (const auto &L : S.Loans)
        if (L.Status != TEXT("closed") && L.CollateralIds.Contains(Asset))
            return true;
    return false;
}
bool ValidateBankScenario(const FScenario &S, FString &E)
{
    const auto &Q = S.Economy.Bank;
    auto Fail = [&](const TCHAR *T) {
        E = T;
        return false;
    };
    FFingerprint Finger;
    Finger.Write(S.InitialState.Bank);
    FFingerprint Empty;
    Empty.Write(FBankState());
    if (!S.InitialState.Loans.IsEmpty() || !S.InitialState.DebtDues.IsEmpty() ||
        !S.InitialState.BankHistory.IsEmpty() || Finger.Finish() != Empty.Finish())
        return Fail(TEXT("Initial bank runtime state must be empty"));
    if (!Q.Enabled)
        return true;
    int64 Deposits = 0;
    for (const auto &A : S.InitialState.Accounts)
    {
        if (A.BalanceCents < 0 || A.BalanceCents > 1000000000000LL || A.Id == BankLedgerId || A.OwnerId == BankLedgerId)
            return Fail(TEXT("Bank ledger is not a customer account"));
        Deposits += A.BalanceCents;
    }
    if (!S.Economy.AccountingEnabled || !S.Economy.HouseholdModel || Q.OpeningCapitalCents < 1 ||
        Q.OpeningCapitalCents > 1000000000000LL || Q.OpeningBackingCents != Deposits + Q.OpeningCapitalCents ||
        !Find(S.Definitions.Places, Q.CityId) || !Find(S.InitialState.Accounts, Q.OperatingRecipientAccountId))
        return Fail(TEXT("Invalid bank opening balance, staffing location or expense recipient"));
    if (Q.BaseRateBps < 0 || Q.BaseRateBps > 3000 || Q.StartupSpreadBps < 0 || Q.StartupSpreadBps > 2000 ||
        Q.RiskSpreadBps < 0 || Q.RiskSpreadBps > 2000 || Q.YearHours != 8760 || Q.PaymentHours < 24 ||
        Q.PaymentHours > 720 || Q.PaymentHours % 24 || Q.MinTermHours < Q.PaymentHours ||
        Q.MinTermHours % Q.PaymentHours || Q.MaxTermHours < Q.MinTermHours || Q.MaxTermHours > 8760 ||
        Q.MaxTermHours % Q.PaymentHours || Q.MaxLoanCents < 1 || Q.MaxLoanCents > 1000000000 ||
        Q.StartupLimitCents < 1 || Q.StartupLimitCents > Q.MaxLoanCents || Q.CollateralPermille < 1 ||
        Q.CollateralPermille > 1000 || Q.CapitalLeveragePermille < 1 || Q.CapitalLeveragePermille > 10000 ||
        Q.HistoryDays < 1 || Q.HistoryDays > 90 || Q.CoveragePermille < 1000 || Q.CoveragePermille > 5000 ||
        Q.DefaultHours < Q.PaymentHours || Q.DefaultHours > 8760 || Q.MaxLoans < 1 || Q.MaxLoans > 512 ||
        Q.MaxCompanyLoans < 1 || Q.MaxCompanyLoans > 8 || Q.SpendPermille < 0 || Q.SpendPermille > 1000 || Q.Jobs < 0 ||
        Q.Jobs > 100 || Q.DailyWageCents < 1 || Q.DailyWageCents > 100000 || Q.OperatingCentsPerDay < 0 ||
        Q.OperatingCentsPerDay > 1000000 || Q.NpcDecisionHours < 24 || Q.NpcDecisionHours > 8760 ||
        Q.NpcReserveCents < 0 || Q.NpcReserveCents > Q.MaxLoanCents)
        return Fail(TEXT("Invalid bank rates, maturities, risk or expense limits"));
    return true;
}
bool AssessLoan(const FScenario &S, const FState &W, const FLoanRequest &R, FLoanQuote &Out, FString &E)
{
    Out = {};
    const auto &Q = S.Economy.Bank;
    auto Fail = [&](const TCHAR *Why) {
        E = Why;
        return false;
    };
    const auto *C = Find(W.Companies, R.OwnerId);
    const auto *Definition = Find(S.Definitions.Companies, R.OwnerId);
    if (!Q.Enabled || !C || !Definition || Definition->AccountingOnly)
        return Fail(TEXT("Kredit: Bank nicht verfügbar oder keine finanzierbare Firma."));
    if (R.Action != TEXT("apply") || R.AmountCents < 1 || R.AmountCents > Q.MaxLoanCents ||
        R.TermHours < Q.MinTermHours || R.TermHours > Q.MaxTermHours || R.TermHours % Q.PaymentHours)
        return Fail(TEXT("Kredit: Betrag oder Laufzeit außerhalb der Szenariogrenzen."));
    int64 NetClaims = 0, Debt = 0, ExistingService = 0;
    int32 Active = 0;
    for (const auto &L : W.Loans)
    {
        NetClaims += L.OutstandingCents - L.WrittenOffCents;
        if (L.OwnerId != R.OwnerId || L.Status == TEXT("closed"))
            continue;
        ++Active;
        Debt += L.OutstandingCents;
        ExistingService += (L.InitialCents / (L.TermHours / Q.PaymentHours) +
                            L.OutstandingCents * L.RateBps * Q.PaymentHours / (10000 * Q.YearHours));
        if (L.Status == TEXT("defaulted"))
            return Fail(TEXT("Kredit: Ausgefallene Altverpflichtung sperrt neue Finanzierung."));
        for (const auto &D : W.DebtDues)
            if (D.LoanId == L.Id && D.Status != TEXT("paid"))
                return Fail(TEXT("Kredit: Offener Schuldendienst sperrt neue Finanzierung."));
    }
    if (Active >= Q.MaxCompanyLoans || W.Loans.Num() >= Q.MaxLoans)
        return Fail(TEXT("Kredit: Vertragslimit erreicht."));
    const int64 Equity = BankEquity(Q, W.Bank), Capacity = FMath::Max(0LL, Equity) * Q.CapitalLeveragePermille / 1000;
    if (Equity <= 0 || R.AmountCents > Capacity - NetClaims)
        return Fail(TEXT("Kredit: Risikobudget der Bank reicht nicht aus."));
    for (const auto &A : W.AccountingAssets)
        if (A.OwnerId == R.OwnerId && A.Known && !Pledged(W, A.Id))
        {
            bool Offered = false;
            for (const auto &Sale : W.AssetSales)
                Offered |= Sale.AssetId == A.Id && Sale.Active;
            if (!Offered)
            {
                Out.CollateralCents += A.ValueCents;
                Out.CollateralIds.Add(A.Id);
            }
        }
    const int64 CollateralSupport = Out.CollateralCents * Q.CollateralPermille / 1000;
    Out.Startup = W.EconomyHour < Q.HistoryDays * 24;
    int64 Cash = 0;
    const int64 Day = W.EconomyHour / 24;
    for (const auto &H : W.BankHistory)
        if (H.OwnerId == R.OwnerId && H.Day < Day && H.Day >= Day - Q.HistoryDays)
            Cash += H.IncomeCents - H.ExpenseCents;
    Out.DailyCashCents = Cash / Q.HistoryDays;
    if (Out.Startup)
        Out.SupportedCents = FMath::Min(Q.StartupLimitCents, FMath::Max(0LL, CollateralSupport - Debt));
    else
    {
        int64 Profit = 0;
        bool Known = true, HasResult = false;
        for (const auto &P : W.AccountingPeriods)
            if (P.ActorId == R.OwnerId && P.EndHour > W.EconomyHour - Q.HistoryDays * 24)
            {
                HasResult = true;
                Profit += P.Profit();
                Known &= P.Known;
            }
        if (!HasResult || !Known || Profit < 0 || Out.DailyCashCents <= 0)
            return Fail(TEXT("Kredit: Ergebnis-/Cashflowhistorie trägt keinen zusätzlichen Schuldendienst."));
        Out.SupportedCents = FMath::Max(
            0LL,
            FMath::Max(CollateralSupport, Out.DailyCashCents * (R.TermHours / 24) * 1000 / Q.CoveragePermille) - Debt);
    }
    if (R.AmountCents > Out.SupportedCents)
        return Fail(TEXT("Kredit: Sicherheiten oder tragfähige Historie reichen für den Betrag nicht aus."));
    Out.SpreadBps =
        (Out.Startup ? Q.StartupSpreadBps : 0) + (Debt + R.AmountCents > CollateralSupport / 2 ? Q.RiskSpreadBps : 0);
    Out.RateBps = Q.BaseRateBps + Out.SpreadBps;
    Out.PeriodPrincipalCents = (R.AmountCents + R.TermHours / Q.PaymentHours - 1) / (R.TermHours / Q.PaymentHours);
    Out.FirstInterestCents = R.AmountCents * Out.RateBps * Q.PaymentHours / (10000 * Q.YearHours);
    if (!Out.Startup && (Out.PeriodPrincipalCents + Out.FirstInterestCents + ExistingService) * Q.CoveragePermille >
                            Out.DailyCashCents * (Q.PaymentHours / 24) * 1000)
        return Fail(TEXT("Kredit: Laufender Cashflow deckt die periodische Belastung nicht."));
    Out.Summary = FString::Printf(
        TEXT("%s · %.2f %% p.a. (Basis %.2f %% + Risiko %.2f %%)\nErste maximale Rate: %s Tilgung + %s Zins je %lld "
             "h.\nSicherheiten (Buchwert): %s · Finanzierbarer Betrag: %s.\n%s Keine Preis- oder Gewinngarantie."),
        *Display::Money(R.AmountCents), Out.RateBps / 100., Q.BaseRateBps / 100., Out.SpreadBps / 100.,
        *Display::Money(Out.PeriodPrincipalCents), *Display::Money(Out.FirstInterestCents), Q.PaymentHours,
        *Display::Money(Out.CollateralCents), *Display::Money(Out.SupportedCents),
        Out.Startup ? TEXT("Begrenzte besicherte Startregel ohne Historie.")
                    : TEXT("Ergebnis und bezahlter Cashflow geprüft."));
    E.Empty();
    return true;
}
bool FSimulation::PreviewLoan(const FLoanRequest &R, FLoanQuote &Q, FString &E) const
{
    if (R.Action == TEXT("apply"))
        return AssessLoan(Scenario, State, R, Q, E);
    const auto *L = Find(State.Loans, R.LoanId);
    if (!Scenario.Economy.Bank.Enabled || R.Action != TEXT("repay") || !L || L->OwnerId != R.OwnerId ||
        L->Status != TEXT("active") || R.AmountCents < 1 || R.AmountCents > L->OutstandingCents)
    {
        E = TEXT("Tilgung: gültigen eigenen aktiven Kredit und Betrag bis zur Restschuld wählen.");
        return false;
    }
    int64 Interest = L->AccruedInterestCents;
    for (const auto &D : State.DebtDues)
        if (D.LoanId == L->Id)
            Interest += D.InterestCents - D.InterestPaidCents;
    const auto *A = Find(State.Accounts, ActorAccount(R.OwnerId));
    if (!A || A->BalanceCents - A->ReservedCents < R.AmountCents + Interest)
    {
        E = TEXT("Tilgung: freie Mittel reichen für Tilgung plus offene Zinsen nicht aus.");
        return false;
    }
    Q.Summary = TEXT("Vorzeitige Tilgung ") + Display::Money(R.AmountCents) + TEXT(" + offene/aufgelaufene Zinsen ") +
                Display::Money(Interest) + TEXT(". Keine zusätzliche Gebühr.");
    E.Empty();
    return true;
}
void FSimulation::InitializeBank()
{
    State.Bank = {};
    State.Loans.Reset();
    State.DebtDues.Reset();
    State.BankHistory.Reset();
}
void FSimulation::BankHistoryPayment(const FString &From, const FString &To, int64 Amount, const FString &Reason)
{
    if (!Scenario.Economy.Bank.Enabled || Reason == TEXT("loan_interest"))
        return;
    for (int32 Side = 0; Side < 2; ++Side)
    {
        const FString &Actor = Side ? To : From;
        if (!Find(State.Companies, Actor))
            continue;
        const int64 Day = State.EconomyHour / 24;
        const FString Id = Actor + TEXT("|") + LexToString(Day);
        auto *Row = Find(State.BankHistory, Id);
        if (!Row)
        {
            State.BankHistory.Add({Id, Actor, Day, 0, 0});
            Row = &State.BankHistory.Last();
        }
        if (Side)
            Row->IncomeCents += Amount;
        else
            Row->ExpenseCents += Amount;
    }
    const int64 Earliest = State.EconomyHour / 24 - Scenario.Economy.Bank.HistoryDays;
    State.BankHistory.RemoveAll([&](const FBankHistory &H) { return H.Day < Earliest; });
}
bool FSimulation::BankPayment(const FString &From, const FString &To, int64 Amount, const FString &Reason)
{
    const auto &Q = Scenario.Economy.Bank;
    if (!Q.Enabled || Amount < 0 || Amount > 1000000000000LL || (From == BankLedgerId) == (To == BankLedgerId))
        return false;
    const bool Out = From == BankLedgerId;
    auto *A = Find(State.Accounts, Out ? To : From);
    if (!A || (!Out && Amount > A->BalanceCents - A->ReservedCents) || (Out && A->BalanceCents > MAX_int64 - Amount))
        return false;
    const bool Loan = Reason == TEXT("loan_disbursement"), Principal = Reason == TEXT("loan_principal"),
               Interest = Reason == TEXT("loan_interest"), Wage = Reason == TEXT("bank_wages"),
               Operating = Reason == TEXT("bank_operating");
    if ((Out && !(Loan || Wage || Operating)) || (!Out && !(Principal || Interest)) ||
        (Loan && !Find(State.Companies, A->OwnerId)) || (Wage && !Find(State.Groups, A->OwnerId)) ||
        (Operating && A->Id != Q.OperatingRecipientAccountId) ||
        ((Wage || Operating) && Amount > BankSpendable(Q, State.Bank)))
        return false;
    if (Out)
        A->BalanceCents += Amount;
    else
        A->BalanceCents -= Amount;
    if (Loan)
    {
        State.Bank.CreatedCents += Amount;
        SiteLedger(A->OwnerId, TEXT("")).CreditCents += Amount;
    }
    if (Principal)
    {
        State.Bank.RepaidCents += Amount;
        SiteLedger(A->OwnerId, TEXT("")).PrincipalCents += Amount;
    }
    if (Interest)
    {
        State.Bank.InterestCents += Amount;
        SiteLedger(A->OwnerId, TEXT("")).InterestCents += Amount;
        BankHistoryPayment(A->OwnerId, TEXT(""), Amount, Reason);
    }
    if (Wage)
        State.Bank.WagesCents += Amount;
    if (Operating)
    {
        State.Bank.OperatingCents += Amount;
        SitePayment(A->OwnerId, TEXT(""), Amount, true);
        AccountingPayment(BankLedgerId, A->OwnerId, Amount, Reason);
        BankHistoryPayment(TEXT(""), A->OwnerId, Amount, Reason);
    }
    State.CashFlows.Add({From, To, Reason, State.EconomyHour, Amount});
    while (State.CashFlows.Num() > Scenario.Economy.HistoryLimit)
        State.CashFlows.RemoveAt(0);
    auto *Total = Find(State.CashTotals, Reason);
    if (!Total)
    {
        State.CashTotals.Add({Reason, 0});
        Total = &State.CashTotals.Last();
    }
    Total->Cents += Amount;
    if (A->OwnerId == Session.PlayerCompanyId)
    {
        if (Loan)
            PlayerDay().CreditInCents += Amount;
        if (Principal)
            PlayerDay().PrincipalOutCents += Amount;
        if (Interest)
            PlayerDay().InterestOutCents += Amount;
    }
    // Finance flows are separate from operating PlayerDay income/expense.
    return true;
}
bool FSimulation::ChangeLoan(const FLoanRequest &R, FString &E)
{
    FLoanQuote Quote;
    if (!PreviewLoan(R, Quote, E))
        return false;
    if (R.Action == TEXT("apply"))
    {
        if (R.Id.IsEmpty() || R.Id.Len() > 96 || Find(State.Loans, R.Id))
        {
            E = TEXT("Kredit: ungültige oder doppelte Vertrags-ID.");
            return false;
        }
        if (!Pay(BankLedgerId, ActorAccount(R.OwnerId), R.AmountCents, TEXT("loan_disbursement")))
        {
            E = TEXT("Kredit: Guthaben konnte nicht sicher gebucht werden.");
            return false;
        }
        FLoan L;
        L.Id = R.Id;
        L.OwnerId = R.OwnerId;
        L.Status = TEXT("active");
        L.InitialCents = L.OutstandingCents = L.RiskExposureCents = R.AmountCents;
        L.RateBps = Quote.RateBps;
        L.SpreadBps = Quote.SpreadBps;
        L.CollateralCents = Quote.CollateralCents;
        L.CollateralIds = Quote.CollateralIds;
        L.CreatedHour = State.EconomyHour;
        L.TermHours = R.TermHours;
        L.NextDueHour = L.CreatedHour + Scenario.Economy.Bank.PaymentHours;
        State.Loans.Add(L);
        E = TEXT("Kredit gebucht: ") + Quote.Summary;
        return true;
    }
    auto *L = Find(State.Loans, R.LoanId);
    int64 Interest = L->AccruedInterestCents;
    for (const auto &D : State.DebtDues)
        if (D.LoanId == L->Id)
            Interest += D.InterestCents - D.InterestPaidCents;
    // Preview checked the complete atomic outlay, including already accrued interest.
    const FState Backup = State;
    if ((Interest && !Pay(ActorAccount(R.OwnerId), BankLedgerId, Interest, TEXT("loan_interest"))) ||
        !Pay(ActorAccount(R.OwnerId), BankLedgerId, R.AmountCents, TEXT("loan_principal")))
    {
        State = Backup;
        E = TEXT("Tilgung vollständig zurückgerollt: Zahlung nicht gedeckt.");
        return false;
    }
    L->InterestPaidCents += Interest;
    L->AccruedInterestCents = 0;
    L->OutstandingCents -= R.AmountCents;
    L->RepaidCents += R.AmountCents;
    int64 Left = R.AmountCents;
    for (auto &D : State.DebtDues)
        if (D.LoanId == L->Id)
        {
            D.InterestPaidCents = D.InterestCents;
            const int64 P = FMath::Min(Left, D.PrincipalCents - D.PrincipalPaidCents);
            D.PrincipalPaidCents += P;
            Left -= P;
            D.Status = D.PrincipalPaidCents == D.PrincipalCents ? TEXT("paid") : TEXT("partial");
        }
    if (!L->OutstandingCents)
    {
        L->Status = TEXT("closed");
        L->NextDueHour = 0;
        L->InterestRemainder = 0;
    }
    E = TEXT("Tilgung gebucht: ") + Display::Money(R.AmountCents) + TEXT(" · Zinsen separat ") +
        Display::Money(Interest);
    return true;
}
void FSimulation::SettleLoan(FLoan &L)
{
    for (auto &D : State.DebtDues)
        if (D.LoanId == L.Id && D.Status != TEXT("paid"))
        {
            auto *A = Find(State.Accounts, D.FromAccountId);
            int64 Cash = A->BalanceCents - A->ReservedCents;
            const int64 Interest = FMath::Min(Cash, D.InterestCents - D.InterestPaidCents);
            if (Interest && Pay(A->Id, BankLedgerId, Interest, TEXT("loan_interest")))
            {
                D.InterestPaidCents += Interest;
                L.InterestPaidCents += Interest;
                Cash -= Interest;
            }
            if (D.InterestPaidCents == D.InterestCents)
            {
                const int64 Principal = FMath::Min(Cash, D.PrincipalCents - D.PrincipalPaidCents);
                if (Principal && Pay(A->Id, BankLedgerId, Principal, TEXT("loan_principal")))
                {
                    D.PrincipalPaidCents += Principal;
                    L.OutstandingCents -= Principal;
                    L.RepaidCents += Principal;
                }
            }
            D.Status = D.InterestPaidCents == D.InterestCents && D.PrincipalPaidCents == D.PrincipalCents ? TEXT("paid")
                       : (D.InterestPaidCents || D.PrincipalPaidCents) ? TEXT("partial")
                                                                       : TEXT("overdue");
            if (D.Status != TEXT("paid"))
                break; // oldest due first, never spend held money
        }
    bool Open = false;
    for (const auto &D : State.DebtDues)
        if (D.LoanId == L.Id && D.Status != TEXT("paid"))
            Open = true;
    if (!L.OutstandingCents && !L.AccruedInterestCents && !Open)
    {
        L.Status = TEXT("closed");
        L.NextDueHour = 0;
        L.InterestRemainder = 0;
    }
}
void FSimulation::BankHour(int64 H)
{
    const auto &Q = Scenario.Economy.Bank;
    if (!Q.Enabled)
        return;
    TRACE_CPUPROFILER_EVENT_SCOPE(CoI_Banking);
    TGuardValue<int64> Time(State.EconomyHour, H);
    for (auto &L : State.Loans)
        if (L.Status == TEXT("active"))
        {
            const int64 Numerator = L.OutstandingCents * L.RateBps + L.InterestRemainder,
                        Denominator = 10000 * Q.YearHours, Accrued = Numerator / Denominator;
            L.InterestRemainder = Numerator % Denominator;
            L.AccruedInterestCents += Accrued;
            {
                TGuardValue<int64> RecognitionHour(State.EconomyHour, H - 1);
                AccountingPeriod(L.OwnerId).FinancingExpense += Accrued;
            }
            if (L.NextDueHour && H >= L.NextDueHour)
            {
                int64 AlreadyDue = 0;
                for (const auto &D : State.DebtDues)
                    if (D.LoanId == L.Id)
                        AlreadyDue += D.PrincipalCents - D.PrincipalPaidCents;
                const int64 Periods = L.TermHours / Q.PaymentHours;
                ++L.PeriodsBilled;
                FDebtDue D;
                D.Id = L.Id + TEXT("|") + LexToString(L.PeriodsBilled);
                D.LoanId = L.Id;
                D.FromAccountId = ActorAccount(L.OwnerId);
                D.ToAccountId = BankLedgerId;
                D.Purpose = TEXT("debt_service");
                D.DueHour = L.NextDueHour;
                D.PrincipalCents = FMath::Min(FMath::Max(0LL, L.OutstandingCents - AlreadyDue),
                                              L.PeriodsBilled == Periods ? L.OutstandingCents
                                                                         : (L.InitialCents + Periods - 1) / Periods);
                D.InterestCents = L.AccruedInterestCents;
                L.AccruedInterestCents = 0;
                D.Status = TEXT("overdue");
                State.DebtDues.Add(D);
                L.NextDueHour = L.PeriodsBilled < Periods ? L.CreatedHour + (L.PeriodsBilled + 1) * Q.PaymentHours : 0;
            }
            else if (!L.NextDueHour && L.AccruedInterestCents)
            {
                for (int32 I = State.DebtDues.Num() - 1; I >= 0; --I)
                    if (State.DebtDues[I].LoanId == L.Id)
                    {
                        State.DebtDues[I].InterestCents += L.AccruedInterestCents;
                        L.AccruedInterestCents = 0;
                        break;
                    }
            }
            SettleLoan(L);
            for (auto &D : State.DebtDues)
                if (D.LoanId == L.Id && D.Status != TEXT("paid") && H - D.DueHour >= Q.DefaultHours)
                {
                    L.Status = TEXT("defaulted");
                    L.NextDueHour = 0;
                    L.WrittenOffCents = L.OutstandingCents;
                    State.Bank.WriteOffCents += L.WrittenOffCents;
                    for (auto &Other : State.DebtDues)
                        if (Other.LoanId == L.Id && Other.Status != TEXT("paid"))
                            Other.Status = TEXT("defaulted");
                    if (L.OwnerId == Session.PlayerCompanyId)
                        PlayerEvent(TEXT("company"), L.Id,
                                    TEXT("Kredit ausgefallen: Bankforderung abgeschrieben; Schulden bleiben offen. "
                                         "Keine Löschung anderer Kundeneinlagen."));
                    break;
                }
        }
    if (H >= State.Bank.NextOperatingHour)
    {
        const int64 Assessments = (H - State.Bank.NextOperatingHour) / Scenario.Economy.PayrollHours + 1;
        State.Bank.OperatingArrearsCents += Assessments * Q.OperatingCentsPerDay * (Scenario.Economy.PayrollHours / 24);
        State.Bank.NextOperatingHour += Assessments * Scenario.Economy.PayrollHours;
        const int64 Reserve = Q.Jobs * Q.DailyWageCents * (Scenario.Economy.PayrollHours / 24);
        const int64 Amount =
            FMath::Min(State.Bank.OperatingArrearsCents, FMath::Max(0LL, BankSpendable(Q, State.Bank) - Reserve));
        if (Amount && Pay(BankLedgerId, Q.OperatingRecipientAccountId, Amount, TEXT("bank_operating")))
            State.Bank.OperatingArrearsCents -= Amount;
    }
    if (Q.NpcEnabled && H % Q.NpcDecisionHours == 0)
        BankNpc(H);
}
void FSimulation::BankNpc(int64 H)
{
    const auto &Q = Scenario.Economy.Bank;
    if (H < Q.HistoryDays * 24)
        return;
    for (const auto &C : State.Companies)
    {
        const auto *Definition = Find(Scenario.Definitions.Companies, C.Id);
        if (!IsAutonomous(C.Id) || Definition->AccountingOnly)
            continue;
        bool HasLoan = false;
        for (const auto &L : State.Loans)
            if (L.OwnerId == C.Id && L.Status != TEXT("closed"))
                HasLoan = true;
        if (HasLoan)
            continue;
        const auto *A = Find(State.Accounts, C.AccountId);
        const int64 Gap = Q.NpcReserveCents - (A->BalanceCents - A->ReservedCents);
        if (Gap <= 0)
            continue;
        FLoanRequest R;
        R.Id = TEXT("npc.loan.") + C.Id + TEXT(".") + LexToString(H);
        R.OwnerId = C.Id;
        R.Action = TEXT("apply");
        R.AmountCents = Gap;
        R.TermHours = FMath::Min(Q.MaxTermHours, FMath::Max(Q.MinTermHours, 4 * Q.PaymentHours));
        FString E;
        ChangeLoan(R, E);
    }
}
bool FSimulation::ValidateBank(FString &E) const
{
    const auto &Q = Scenario.Economy.Bank;
    auto Fail = [&](const TCHAR *Why) {
        E = FString(TEXT("Bankbilanz: ")) + Why;
        return false;
    };
    if (!ValidateBankScenario(Scenario, E))
        return false;
    if (!Q.Enabled)
    {
        FFingerprint A, B;
        A.Write(State.Bank);
        B.Write(FBankState());
        if (A.Finish() != B.Finish() || !State.Loans.IsEmpty() || !State.DebtDues.IsEmpty() ||
            !State.BankHistory.IsEmpty())
            return Fail(TEXT("Bankzustand ohne aktivierte Bank"));
        return true;
    }
    const auto &B = State.Bank;
    for (int64 X : {B.CreatedCents, B.RepaidCents, B.InterestCents, B.WagesCents, B.OperatingCents, B.WriteOffCents,
                    B.OperatingArrearsCents, B.NextOperatingHour})
        if (X < 0 || X > 1000000000000000LL)
            return Fail(TEXT("Ungültiger Buchungszähler"));
    const int64 ExpectedOperating =
        State.EconomyHour ? ((State.EconomyHour / Scenario.Economy.PayrollHours) + 1) * Scenario.Economy.PayrollHours
                          : 0;
    if (B.NextOperatingHour != ExpectedOperating ||
        B.OperatingArrearsCents != (ExpectedOperating / Scenario.Economy.PayrollHours) * Q.OperatingCentsPerDay *
                                           (Scenario.Economy.PayrollHours / 24) -
                                       B.OperatingCents)
        return Fail(TEXT("Bankbetrieb doppelt oder falsch fällig"));
    if (State.Loans.Num() > Q.MaxLoans || State.DebtDues.Num() > Q.MaxLoans * (Q.MaxTermHours / Q.PaymentHours))
        return Fail(TEXT("Vertrags-/Fälligkeitsgrenze"));
    TSet<FString> Ids;
    int64 Created = 0, Repaid = 0, Interest = 0, WriteOff = 0, Claims = 0;
    for (const auto &L : State.Loans)
    {
        const auto *Company = Find(Scenario.Definitions.Companies, L.OwnerId);
        if (L.Id.IsEmpty() || L.Id.Len() > 96 || Ids.Contains(L.Id) || !Company || Company->AccountingOnly ||
            (L.Status != TEXT("active") && L.Status != TEXT("closed") && L.Status != TEXT("defaulted")) ||
            L.InitialCents < 1 || L.InitialCents > Q.MaxLoanCents || L.RepaidCents < 0 || L.OutstandingCents < 0 ||
            L.InitialCents != L.RepaidCents + L.OutstandingCents || L.WrittenOffCents < 0 ||
            L.WrittenOffCents > L.OutstandingCents || L.RateBps != Q.BaseRateBps + L.SpreadBps || L.SpreadBps < 0 ||
            L.SpreadBps > Q.StartupSpreadBps + Q.RiskSpreadBps || L.CollateralCents < 0 ||
            L.CollateralCents > 1000000000000000LL || L.RiskExposureCents != L.InitialCents || L.CreatedHour < 0 ||
            L.CreatedHour > State.EconomyHour || L.TermHours < Q.MinTermHours || L.TermHours > Q.MaxTermHours ||
            L.TermHours % Q.PaymentHours || L.PeriodsBilled < 0 || L.PeriodsBilled > L.TermHours / Q.PaymentHours ||
            L.AccruedInterestCents < 0 || L.AccruedInterestCents > 1000000000000LL || L.InterestRemainder < 0 ||
            L.InterestRemainder >= 10000 * Q.YearHours || L.InterestPaidCents < 0 ||
            L.InterestPaidCents > 1000000000000LL)
            return Fail(TEXT("Ungültiger Kreditvertrag"));
        Ids.Add(L.Id);
        if (L.Status == TEXT("closed") && (L.OutstandingCents || L.AccruedInterestCents || L.NextDueHour))
            return Fail(TEXT("Geschlossener Kredit mit Restpflicht"));
        if (L.Status == TEXT("defaulted") && (L.WrittenOffCents != L.OutstandingCents || L.NextDueHour))
            return Fail(TEXT("Ausfall ohne getrennte Abschreibung"));
        if (L.Status != TEXT("defaulted") && L.WrittenOffCents)
            return Fail(TEXT("Abschreibung bei aktivem Kredit"));
        const int64 Expected = L.Status == TEXT("active") && L.PeriodsBilled < L.TermHours / Q.PaymentHours
                                   ? L.CreatedHour + (L.PeriodsBilled + 1) * Q.PaymentHours
                                   : 0;
        if (L.NextDueHour != Expected)
            return Fail(TEXT("Doppelte/verschobene Fälligkeit"));
        TSet<FString> Security;
        for (const auto &Asset : L.CollateralIds)
        {
            const auto *A = Find(State.AccountingAssets, Asset);
            if (Security.Contains(Asset) || !A || (L.Status != TEXT("closed") && A->OwnerId != L.OwnerId))
                return Fail(TEXT("Ungültige Sicherheit"));
            Security.Add(Asset);
            if (L.Status != TEXT("closed"))
                for (const auto &Other : State.Loans)
                    if (Other.Id != L.Id && Other.Status != TEXT("closed") && Other.CollateralIds.Contains(Asset))
                        return Fail(TEXT("Doppelt verpfändetes Asset"));
        }
        int64 InterestPaid = 0, PrincipalDue = 0;
        int32 Count = 0;
        for (const auto &D : State.DebtDues)
            if (D.LoanId == L.Id)
            {
                ++Count;
                if (D.Id != L.Id + TEXT("|") + LexToString(Count) ||
                    D.DueHour != L.CreatedHour + Count * Q.PaymentHours)
                    return Fail(TEXT("Doppelte oder fehlende Vertragsrate"));
                InterestPaid += D.InterestPaidCents;
                PrincipalDue += D.PrincipalCents - D.PrincipalPaidCents;
            }
        if (Count != L.PeriodsBilled || PrincipalDue > L.OutstandingCents || InterestPaid > L.InterestPaidCents)
            return Fail(TEXT("Fälligkeit/Restschuld"));
        Created += L.InitialCents;
        Repaid += L.RepaidCents;
        Interest += L.InterestPaidCents;
        WriteOff += L.WrittenOffCents;
        Claims += L.OutstandingCents - L.WrittenOffCents;
    }
    TSet<FString> DueIds;
    for (const auto &D : State.DebtDues)
    {
        const auto *L = Find(State.Loans, D.LoanId);
        if (!L || D.Id.IsEmpty() || DueIds.Contains(D.Id) || D.FromAccountId != ActorAccount(L->OwnerId) ||
            D.ToAccountId != BankLedgerId || D.Purpose != TEXT("debt_service") || D.DueHour <= L->CreatedHour ||
            D.DueHour > State.EconomyHour || (D.DueHour - L->CreatedHour) % Q.PaymentHours ||
            D.DueHour > L->CreatedHour + L->TermHours || D.PrincipalCents < 0 || D.InterestCents < 0 ||
            D.PrincipalPaidCents < 0 || D.InterestPaidCents < 0 || D.PrincipalPaidCents > D.PrincipalCents ||
            D.InterestPaidCents > D.InterestCents)
            return Fail(TEXT("Ungültige periodische Verpflichtung"));
        const bool Paid = D.PrincipalPaidCents == D.PrincipalCents && D.InterestPaidCents == D.InterestCents;
        if ((D.Status == TEXT("paid")) != Paid ||
            (!Paid && D.Status != TEXT("partial") && D.Status != TEXT("overdue") && D.Status != TEXT("defaulted")) ||
            (D.Status == TEXT("defaulted") && L->Status != TEXT("defaulted")))
            return Fail(TEXT("Zahlungsstatus widerspricht Buchung"));
        DueIds.Add(D.Id);
    }
    if (Created != B.CreatedCents || Repaid != B.RepaidCents || Interest != B.InterestCents ||
        WriteOff != B.WriteOffCents)
        return Fail(TEXT("Kredite und Bankhauptbuch widersprechen sich"));
    const TCHAR *Reasons[] = {TEXT("loan_disbursement"), TEXT("loan_principal"), TEXT("loan_interest"),
                              TEXT("bank_wages"), TEXT("bank_operating")};
    const int64 Amounts[] = {B.CreatedCents, B.RepaidCents, B.InterestCents, B.WagesCents, B.OperatingCents};
    for (int32 I = 0; I < 5; ++I)
    {
        const auto *Total = Find(State.CashTotals, FString(Reasons[I]));
        if ((Total ? Total->Cents : 0) != Amounts[I])
            return Fail(TEXT("Zahlungsjournal und Bankhauptbuch"));
    }
    if (B.WagesCents + B.OperatingCents > B.InterestCents * Q.SpendPermille / 1000)
        return Fail(TEXT("Unfinanzierte Bankausgabe"));
    int64 Deposits = 0;
    for (const auto &A : State.Accounts)
        Deposits += A.BalanceCents;
    if (Q.OpeningBackingCents + State.DeveloperMoneySources - State.DeveloperMoneySinks + Claims !=
        Deposits + BankEquity(Q, B) + B.OperatingArrearsCents)
        return Fail(TEXT("Aktiva ungleich Einlagen plus Eigenkapital"));
    TSet<FString> HistoryIds;
    for (const auto &H : State.BankHistory)
    {
        if (!Find(State.Companies, H.OwnerId) || H.Id != H.OwnerId + TEXT("|") + LexToString(H.Day) ||
            HistoryIds.Contains(H.Id) || H.Day < 0 || H.Day > State.EconomyHour / 24 || H.IncomeCents < 0 ||
            H.ExpenseCents < 0 || H.IncomeCents > 1000000000000000LL || H.ExpenseCents > 1000000000000000LL)
            return Fail(TEXT("Ungültige Cashflowhistorie"));
        HistoryIds.Add(H.Id);
    }
    return true;
}
} // namespace CoISim

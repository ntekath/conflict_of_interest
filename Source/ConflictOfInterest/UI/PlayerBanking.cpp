#include "PlayerHUD.h"
#include "Simulation/Banking.h"
#include "Simulation/SimulationSubsystem.h"
#include "Simulation/SimArchive.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
using namespace CoISim;
void UCoIPlayerHUD::BankInfo()
{
    const auto &P = Sim->GetSnapshot().Player;
    const auto &Q = P.Rules.Bank;
    if (!Q.Enabled)
        return;
    Text(Body, TEXT("FINANZIERUNG"), 15, true);
    Button(Body, TEXT("Kredit beantragen …"), TEXT("loanApply"), true);
    Text(Body,
         TEXT("Bestehendes Firmenkonto · automatische Raten alle ") + Display::Number(Q.PaymentHours) +
             TEXT(" h · Basiszins ") + Display::Percent(Q.BaseRateBps / 10000.) +
             TEXT(" p.a. plus begründeter Risikoaufschlag."),
         11);
    int64 OwnDebt = 0, OwnInterest = 0, OwnPrincipal = 0;
    for (const auto &L : P.Loans)
    {
        OwnDebt += L.OutstandingCents;
        OwnInterest += L.InterestPaidCents;
        OwnPrincipal += L.RepaidCents;
    }
    Text(Body,
         TEXT("Restschuld ") + Display::Money(OwnDebt) + TEXT(" · bereits getilgt ") + Display::Money(OwnPrincipal) +
             TEXT(" · Zinsen bezahlt ") + Display::Money(OwnInterest),
         13);
    if (P.Loans.IsEmpty())
        Empty(Body, TEXT("Keine Kreditverträge. Antrag mit Betrag und Laufzeit wird vor Bestätigung geprüft."));
    for (const auto &L : P.Loans)
    {
        int64 Arrears = 0, InterestDue = L.AccruedInterestCents;
        for (const auto &D : P.DebtDues)
            if (D.LoanId == L.Id)
            {
                Arrears += D.PrincipalCents - D.PrincipalPaidCents + D.InterestCents - D.InterestPaidCents;
                InterestDue += D.InterestCents - D.InterestPaidCents;
            }
        const FString State = L.Status == TEXT("active")   ? TEXT("Aktiv")
                              : L.Status == TEXT("closed") ? TEXT("Abbezahlt")
                                                           : TEXT("Ausgefallen · Schuld bleibt offen");
        Cells(Body,
              {State, Display::Money(L.OutstandingCents), Display::Percent(L.RateBps / 10000.) + TEXT(" p.a."),
               L.NextDueHour ? Time(L.NextDueHour)
               : Arrears     ? TEXT("Überfällig")
                             : TEXT("Keine weitere Rate")},
              {2, 1, 1, 1.6f}, false, L.Status == TEXT("active") && L.OutstandingCents ? TEXT("Tilgen …") : TEXT(""),
              TEXT("loanRepay|") + L.Id);
        auto *Detail = Text(
            Body,
            TEXT("Offener Schuldendienst ") + Display::Money(Arrears) + TEXT(" · offene/aufgelaufene Zinsen ") +
                Display::Money(InterestDue) + TEXT(" · Sicherheiten bei Abschluss ") +
                Display::Money(L.CollateralCents) + TEXT(" · reguläre Tilgung bis ") +
                Display::Money((L.InitialCents + L.TermHours / Q.PaymentHours - 1) / (L.TermHours / Q.PaymentHours)) +
                TEXT(" je Rate plus Zins auf Restschuld. Vertragsende ") + Time(L.CreatedHour + L.TermHours),
            11);
        TArray<FString> Security;
        for (const auto &Id : L.CollateralIds)
        {
            if (const auto *StoreView = Find(P.Stores, Id))
                Security.Add(StoreView->Name);
            else if (const auto *VehicleView = Find(P.Vehicles, Id))
                Security.Add(VehicleView->Name);
            else if (const auto *Plant = Find(P.Facilities, Id))
            {
                const auto *Type = Find(P.Catalog.FacilityTypes, Plant->TypeId);
                Security.Add(Type ? Type->Name : TEXT("Betrieb"));
            }
        }
        int64 BilledPrincipal = 0;
        for (const auto &Due : P.DebtDues)
            if (Due.LoanId == L.Id)
                BilledPrincipal += Due.PrincipalCents - Due.PrincipalPaidCents;
        if (L.NextDueHour)
        {
            const int64 Hours = FMath::Max(0LL, L.NextDueHour - P.Hour),
                        Interest =
                            L.AccruedInterestCents +
                            (L.OutstandingCents * L.RateBps * Hours + L.InterestRemainder) / (10000 * Q.YearHours),
                        Principal = FMath::Min(FMath::Max(0LL, L.OutstandingCents - BilledPrincipal),
                                               (L.InitialCents + L.TermHours / Q.PaymentHours - 1) /
                                                   (L.TermHours / Q.PaymentHours));
            Text(Body,
                 TEXT("Nächste Planrate: ") + Display::Money(Principal) + TEXT(" Tilgung + ") +
                     Display::Money(Interest) + TEXT(" Zins = ") + Display::Money(Principal + Interest) +
                     TEXT(" (bei unveränderter Restschuld)."),
                 11);
        }
        Detail->SetToolTipText(FText::FromString(
            TEXT("Sicherheiten: ") + FString::Join(Security, TEXT(", ")) +
            TEXT(
                "\nKredit-/Zinsbuchungen sind von operativem Gewinn und Warenbestand getrennt. Offene Pflichten werden "
                "erneut versucht, ohne Konten zu überziehen. Abschreibung der Bank hebt die Schuld nicht auf.")));
    }
    auto *Aggregate =
        Text(Body,
             TEXT("Bank gesamt: Einlagen ") + Display::Money(P.BankDeposits) + TEXT(" · Forderungen netto ") +
                 Display::Money(P.BankClaims) + TEXT(" · Eigenkapital ") + Display::Money(BankEquity(Q, P.Bank)),
             11);
    Aggregate->SetToolTipText(FText::FromString(
        TEXT("Eröffnungsgegenposten ") + Display::Money(Q.OpeningBackingCents) +
        TEXT(" (nicht ausgebbar; Einlagen einschließlich Reservierungen, kein Preisindex). Kreditvergabe +") +
        Display::Money(P.Bank.CreatedCents) + TEXT(" · Tilgung −") + Display::Money(P.Bank.RepaidCents) +
        TEXT(" · vereinnahmte Zinsen ") + Display::Money(P.Bank.InterestCents) + TEXT(" · Banklöhne ") +
        Display::Money(P.Bank.WagesCents) + TEXT(" · Bankbetrieb ") + Display::Money(P.Bank.OperatingCents) +
        TEXT(" · einbehaltene Erträge ") +
        Display::Money(P.Bank.InterestCents - P.Bank.WagesCents - P.Bank.OperatingCents) + TEXT(" · Abschreibungen ") +
        Display::Money(P.Bank.WriteOffCents) + TEXT(". Bekannte reale Asset-/Warenbuchwerte separat: ") +
        Display::Money(P.RealAssetBookCents)));
    Text(Body,
         TEXT("Bankergebnis inkl. offener Betriebspflichten ") +
             Display::Money(BankEquity(Q, P.Bank) - Q.OpeningCapitalCents),
         11);
    Text(Body,
         TEXT("Bankertrag einbehalten ") +
             Display::Money(P.Bank.InterestCents - P.Bank.WagesCents - P.Bank.OperatingCents) +
             TEXT(" · Bankausgaben ") + Display::Money(P.Bank.WagesCents + P.Bank.OperatingCents) +
             TEXT(" · Bankbetrieb offen ") + Display::Money(P.Bank.OperatingArrearsCents),
         11);
}
void UCoIPlayerHUD::OpenLoan(const FString &LoanId)
{
    if (!Sim || !Sim->IsReady() || !Sim->GetSnapshot().Player.Rules.Bank.Enabled)
        return;
    const auto &P = Sim->GetSnapshot().Player;
    LoanDraft = {};
    LoanDraft.OwnerId = P.ActorId;
    LoanDraft.Action = LoanId.IsEmpty() ? TEXT("apply") : TEXT("repay");
    LoanDraft.LoanId = LoanId;
    DialogKind = TEXT("loan");
    OpenDialog(LoanId.IsEmpty() ? TEXT("KREDITANTRAG") : TEXT("VORZEITIGE TILGUNG"));
    int64 Default = 100000;
    if (const auto *L = Find(P.Loans, LoanId))
        Default = FMath::Min(Default, L->OutstandingCents);
    Field(Form, TEXT("loan.amount"), LoanId.IsEmpty() ? TEXT("Kreditbetrag (USD)") : TEXT("Tilgungsbetrag (USD)"),
          LexToString(Default));
    if (LoanId.IsEmpty())
    {
        Field(Form, TEXT("loan.periods"),
              TEXT("Laufzeit in Ratenperioden (je ") + Display::Number(P.Rules.Bank.PaymentHours) + TEXT(" h)"),
              LexToString(FMath::Clamp(10LL, P.Rules.Bank.MinTermHours / P.Rules.Bank.PaymentHours,
                                       P.Rules.Bank.MaxTermHours / P.Rules.Bank.PaymentHours)));
        Text(Form,
             TEXT("Prüfung: Ergebnis und bezahlter Cashflow, bestehende Schulden, verfügbare Sicherheiten und "
                  "Bankrisiko. Ohne Historie gilt eine begrenzte besicherte Startregel. Keine Rettung verlustfähiger "
                  "Vorhaben und keine Gewinngarantie."),
             12);
    }
    else
        Text(Form,
             TEXT("Offene und aufgelaufene Zinsen werden zusätzlich bezahlt und gesondert ausgewiesen. Vorzeitige "
                  "Tilgung kostet keine zusätzliche Gebühr."),
             12);
    FormPreview = Text(Form, TEXT(""), 13);
    Updating = false;
    Preview();
}
bool UCoIPlayerHUD::ReadLoanForm(FString &E)
{
    int64 Amount = 0, Periods = 0;
    if (!ReadPositive(TEXT("loan.amount"), Amount))
    {
        E = TEXT("Positiven USD-Betrag mit höchstens zwei Nachkommastellen eingeben.");
        return false;
    }
    LoanDraft.AmountCents = Amount;
    if (LoanDraft.Action == TEXT("apply"))
    {
        const auto &Q = Sim->GetSnapshot().Player.Rules.Bank;
        if (!ReadPositive(TEXT("loan.periods"), Periods) || Periods > Q.MaxTermHours / Q.PaymentHours ||
            Periods < Q.MinTermHours / Q.PaymentHours)
        {
            E = TEXT("Laufzeit innerhalb der angezeigten Szenariogrenzen wählen: ") +
                Display::Number(Q.MinTermHours / Q.PaymentHours) + TEXT(" bis ") +
                Display::Number(Q.MaxTermHours / Q.PaymentHours) + TEXT(" Ratenperioden.");
            return false;
        }
        LoanDraft.TermHours = Periods * Q.PaymentHours;
    }
    return true;
}

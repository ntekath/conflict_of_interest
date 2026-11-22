#include "Presentation.h"
#include "Internationalization/Internationalization.h"
#include "SimArchive.h"

namespace CoISim
{
namespace Display
{
FString Name(const FString& Value){FString Out=Value;Out.ReplaceInline(TEXT(" (TEST)"),TEXT(""),ESearchCase::CaseSensitive);return Out;}
FString Number(int64 V)
{
    return FText::AsNumber(V, nullptr, FInternationalization::Get().GetCulture(TEXT("de-DE"))).ToString();
}
FString MoneyInput(int64 V)
{
    return (V < 0 && V > -100 ? FString(TEXT("-")) : FString()) + LexToString(V / 100) +
           FString::Printf(TEXT(",%02lld"), FMath::Abs(V % 100));
}
FString Money(int64 V)
{
    // Keep all cents exact, including large balances beyond double's integer precision.
    return (V < 0 && V > -100 ? FString(TEXT("-")) : FString()) + Number(V / 100) +
           FString::Printf(TEXT(",%02lld USD"), FMath::Abs(V % 100));
}
FString Percent(double Ratio)
{
    FNumberFormattingOptions F;
    F.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(1);
    return FText::AsNumber(Ratio * 100., &F, FInternationalization::Get().GetCulture(TEXT("de-DE"))).ToString() +
           TEXT(" %");
}
FString Quantity(int64 Kg)
{
    FString R = Number(Kg) + TEXT(" kg");
    if (Kg >= 1000 || Kg <= -1000)
    {
        FNumberFormattingOptions F;
        F.SetMaximumFractionalDigits(3);
        R +=
            TEXT(" (") +
            FText::AsNumber(double(Kg) / 1000., &F, FInternationalization::Get().GetCulture(TEXT("de-DE"))).ToString() +
            TEXT(" t)");
    }
    return R;
}
FString Duration(int64 H)
{
    return Number(H) + TEXT(" h");
}
FString Time(const FString &StartDate, int64 Hour)
{
    FDateTime Start;
    if (!FDateTime::ParseIso8601(*(StartDate + TEXT("T00:00:00Z")), Start))
        return TEXT("Zeit nicht verfügbar");
    return (Start + FTimespan::FromHours(Hour)).ToString(TEXT("%d.%m.%Y %H:%M"));
}
FString Period(const FString &StartDate, int64 BeginHour, int64 EndHour)
{
    return Time(StartDate, BeginHour) + TEXT(" bis ") + Time(StartDate, EndHour) + TEXT(" (Ende exklusiv)");
}
bool ParseMoneyInput(const FString &Input, int64 &Cents, bool AllowZero)
{
    FString S = Input.TrimStartAndEnd();
    if (S.IsEmpty() || S.Len() > 21)
        return false;
    int64 Whole = 0, Fraction = 0;
    int32 Decimals = -1, Digits = 0;
    for (TCHAR C : S)
    {
        if (C == TEXT(',') || C == TEXT('.'))
        {
            if (Decimals >= 0 || !Digits)
                return false;
            Decimals = 0;
        }
        else if (C >= TEXT('0') && C <= TEXT('9'))
        {
            int64 D = C - TEXT('0');
            if (Decimals >= 0)
            {
                if (++Decimals > 2)
                    return false;
                Fraction = Fraction * 10 + D;
            }
            else
            {
                if (Whole > (MAX_int64 / 100 - D) / 10)
                    return false;
                Whole = Whole * 10 + D;
                ++Digits;
            }
        }
        else
            return false;
    }
    if (Decimals == 0)
        return false;
    if (Decimals == 1)
        Fraction *= 10;
    if (Whole > (MAX_int64 - Fraction) / 100)
        return false;
    const int64 Result = Whole * 100 + Fraction;
    if (!AllowZero && !Result)
        return false;
    Cents = Result;
    return true;
}
FString MarkupInput(int64 Permille){return LexToString(Permille/10)+(Permille%10?FString::Printf(TEXT(",%lld"),Permille%10):FString());}
bool ParseMarkupInput(const FString& S,int64& Permille){int64 N=0;if(!ParseMoneyInput(S,N,true)||N%10)return false;Permille=N/10;return true;}
bool IsMarkupField(const FString& Key){return Key==TEXT("markup")||(Key.StartsWith(TEXT("op."))&&Key.EndsWith(TEXT(".markup")));}
bool IsMoneyField(const FString &Key)
{
    return Key == TEXT("loan.amount") || Key == TEXT("price") || Key == TEXT("op.wage") || Key == TEXT("op.budget") ||
           Key == TEXT("delivery.price") || Key == TEXT("delivery.freight") ||
           (Key.StartsWith(TEXT("op.")) && (Key.EndsWith(TEXT(".buy")) || Key.EndsWith(TEXT(".sell"))));
}
FString Empty(EDisplayState State)
{
    return State == EDisplayState::NoMonthlyHistory ? TEXT("Noch keine Monatsdaten") : TEXT("Nicht verfügbar");
}
} // namespace Display
FString FDisplayMetric::Display() const
{
    if (State != EDisplayState::Value)
        return Display::Empty(State);
    return Currency ? Display::Money(Value) : Display::Number(Value);
}
FString FDisplayMetric::Tooltip() const
{
    return Label + TEXT("\nQuelle: ") + Source + TEXT("\nZeitraum: ") + Period + TEXT("\nBerechnung: ") + Calculation;
}
FCompanyPresentation CompanyPresentation(const FState &S, const FDefinitions &D, const FString &Actor,
                                         const FString &StartDate)
{
    FCompanyPresentation P;
    const FString Now = Display::Time(StartDate, S.EconomyHour);
    auto Metric = [&](const TCHAR *Label, const TCHAR *Source, const TCHAR *Formula, bool Currency = false) {
        FDisplayMetric M;
        M.Label = Label;
        M.Source = Source;
        M.Calculation = Formula;
        M.Period = Now;
        M.Currency = Currency;
        return M;
    };
    P.AvailableMoney =
        Metric(TEXT("Verfügbares Geld"), TEXT("Firmenkonto und Geldreservierungen im veröffentlichten Zustand"),
               TEXT("Kontostand minus Reservierungen. Reserviertes Geld ist bereits im Kontostand enthalten."), true);
    if (const auto *C = Find(S.Companies, Actor))
        if (const auto *A = Find(S.Accounts, C->AccountId))
        {
            P.AvailableMoney.State = EDisplayState::Value;
            P.AvailableMoney.Value = A->BalanceCents - A->ReservedCents;
            P.AvailableMoney.Calculation += TEXT(" Kontostand ") + Display::Money(A->BalanceCents) +
                                            TEXT("; reserviert ") + Display::Money(A->ReservedCents) + TEXT(".");
        }
    P.Employees = Metric(
        TEXT("Mitarbeiter"),
        TEXT("Bezahlte Beschäftigungszuordnungen eigener Betriebe, Lager, Geschäfte und Fahrzeuge"),
        TEXT(
            "Summe aktiver besetzter Stellen mit Lohnende nach dem angezeigten Zeitpunkt. Keine offenen Stellen, keine "
            "regionalen Fremdbeschäftigten. Betriebslager zählen nicht nochmals. Fahrzeugpersonal am Heimatort."));
    P.Sites =
        Metric(TEXT("Standorte"), TEXT("Eigene fertiggestellte Betriebs-, Lager- und Fahrzeugassets"),
               TEXT("Unterschiedliche Stadthubs mit mindestens einem vorhandenen eigenen Asset. Hauptsitz ohne Asset "
                    "und Bauprojekte zählen nicht. Fahrzeug-Heimatort statt Fahrtziel; Lager und Betrieb am selben Hub "
                    "zählen zusammen einmal. Vorübergehender Stillstand entfernt keinen bestehenden Standort."));
    P.Employees.State = P.Sites.State = EDisplayState::Value;
    auto Site = [&](const FString &City) {
        if (Find(D.Places, City))
        {
            P.SiteCityIds.AddUnique(City);
            P.EmployeesByCity.FindOrAdd(City);
        }
    };
    auto Staff = [&](const FString &City, int64 PaidUntil, const TArray<FEmployment> &Jobs) {
        if (PaidUntil <= S.EconomyHour)
            return;
        for (const auto &J : Jobs)
        {
            P.Employees.Value += J.Workers;
            P.EmployeesByCity.FindOrAdd(City) += J.Workers;
        }
    };
    for (const auto &F : S.Facilities)
        if (F.OwnerId == Actor)
        {
            Site(F.CityId);
            Staff(F.CityId, F.PaidUntilHour, F.Employment);
        }
    for (const auto &W : S.Warehouses)
        if (W.OwnerId == Actor)
        {
            Site(W.CityId);
            if (!S.Facilities.ContainsByPredicate([&](const FFacility &F) { return F.StorageId == W.Id; }))
                if (const auto *E = Find(S.ServiceEmployment, W.Id))
                    Staff(W.CityId, E->PaidUntilHour, E->Employment);
        }
    for (const auto &V : S.Vehicles)
        if (V.OwnerId == Actor)
        {
            Site(V.HomeCityId);
            if (const auto *E = Find(S.ServiceEmployment, V.Id))
                Staff(V.HomeCityId, E->PaidUntilHour, E->Employment);
        }
    P.SiteCityIds.Sort();
    P.Sites.Value = P.SiteCityIds.Num();
    P.MonthlyResult = Metric(
        TEXT("Ergebnis · letzter abgeschlossener Monat"),
        TEXT("Monatliche realisierte Kostenrechnung ist noch nicht vorhanden"),
        TEXT("Externe realisierte Erlöse minus Wareneinsatz und Periodenkosten; interne Umsätze und unrealisierte "
             "Aufschläge eliminieren. Festgesetzte Ergebnisabgaben sind Periodenaufwand. Tages- und Standort-Zahlungssalden ersetzen keinen Monatsgewinn."),
        true);
    FDateTime Start;
    if (FDateTime::ParseIso8601(*(StartDate + TEXT("T00:00:00Z")), Start))
    {
        FDateTime NowDate = Start + FTimespan::FromHours(S.EconomyHour);
        FDateTime End(NowDate.GetYear(), NowDate.GetMonth(), 1);
        FDateTime Previous = End - FTimespan::FromDays(1);
        FDateTime Begin(Previous.GetYear(), Previous.GetMonth(), 1);
        P.MonthlyResult.State = Begin >= Start ? EDisplayState::Unavailable : EDisplayState::NoMonthlyHistory;
        P.MonthlyResult.Period = Begin >= Start
                                     ? Begin.ToString(TEXT("%d.%m.%Y")) + TEXT(" bis ") +
                                           End.ToString(TEXT("%d.%m.%Y")) + TEXT(" (Ende exklusiv)")
                                     : TEXT("Seit Partiebeginn noch kein vollständig abgeschlossener Kalendermonat");
    }
    if(S.AccountingStartHour>=0){const FAccountingPeriod* Last=nullptr;for(const auto& Month:S.AccountingPeriods)if(Month.ActorId==Actor&&Month.Complete&&(!Last||Month.EndHour>Last->EndHour))Last=&Month;
        P.MonthlyResult.State=EDisplayState::NoMonthlyHistory;P.MonthlyResult.Period=TEXT("Seit Beginn der Kostenrechnung noch kein vollständiger Monatsabschluss");
        P.MonthlyResult.Source=TEXT("Persistierter monatlicher Kostenabschluss der eigenen Firma");
        if(Last){P.MonthlyResult.Period=Display::Period(StartDate,Last->BeginHour,Last->EndHour);P.MonthlyResult.State=Last->Partial?EDisplayState::NoMonthlyHistory:Last->Known?EDisplayState::Value:EDisplayState::Unavailable;P.MonthlyResult.Value=Last->Profit();}
        P.MonthlyResult.Calculation=TEXT("Externe Waren-/Dienstleistungserlöse minus realisierter Wareneinsatz, Löhne/Betrieb abzüglich aktivierter Herstellungskosten, Leer-/Rückfracht und Verderb, zuzüglich realisierter Assetabgänge. Keine internen Umsätze oder internen Aufschläge. Teilmonate nach Migration sind keine vollständige Monatshistorie.");
    }
    P.NetWorth =
        Metric(TEXT("Nettovermögen"), TEXT("Vollständige Assetbewertung und konsolidierte Verbindlichkeiten fehlen"),
               TEXT("Bewertete Vermögenswerte minus Verbindlichkeiten nach offenzulegender Bewertungsmethode. "
                    "Warenbuchwerte allein reichen nicht. Geldreservierungen sind keine zusätzlichen Schulden. Kein "
                    "Übernahmewert und keine Gleichsetzung mit Bargeld."),
               true);
    return P;
}
} // namespace CoISim

#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "PlayerHUD.h"
#include "Simulation/Accounting.h"
#include "Simulation/SimArchive.h"
#include "Simulation/SimulationSubsystem.h"
using namespace CoISim;
void UCoIPlayerHUD::CostInfo(UPanelWidget *Parent, const FString &StoreId, const FString &GoodId, int64 Markup)
{
    const auto &P = Sim->GetSnapshot().Player;
    if (!P.Rules.AccountingEnabled)
        return;
    const auto *Q = AccountingFindPrice(P.AccountingPrices, P.ActorId, StoreId, GoodId);
    if (!Q)
    {
        Empty(Parent, TEXT("Noch keine Kostenbasis für diesen Ort."));
        return;
    }
    Text(Parent,
         Good(GoodId) + TEXT(" · gespeicherte Vorgabe · Kostenbasis ") + Display::Money(Q->Base()) +
             TEXT(" / kg · Aufschlag ") + Display::Percent(double(Markup) / 1000.) + TEXT(" → ") +
             Display::Money(AccountingPrice(Q->Base(), Markup, P.Rules.MaxPriceCents)) + TEXT(" / kg"),
         12, true);
    Text(Parent,
         TEXT("Je kg: Einkauf/Inputs/Eröffnung ") + Display::Money(Q->UnitCost.Material) + TEXT(" · Fertigungslohn ") +
             Display::Money(Q->UnitCost.Labor) + TEXT(" · Herstellbetrieb ") + Display::Money(Q->UnitCost.Operating) +
             TEXT(" · enthaltene Fracht ") + Display::Money(Q->UnitCost.Freight) +
             TEXT(" · normale Handels-/Ladenkosten ") + Display::Money(Q->Overhead),
         11);
    Text(Parent,
         (Q->Startup
              ? FString(TEXT("Anlaufbasis: deklarierte Start-/Normalkosten; keine Einkaufshistorie."))
              : FString(TEXT(
                    "Basis aus gleitenden Buchkosten des vorhandenen Bestands einschließlich Eröffnungswerten."))) +
             TEXT(" Kalkulationsstand ") + Time(Q->Hour) + TEXT(" · Version ") + Display::Number(Q->Version) +
             TEXT(". Absatz null erhöht die Basis nicht. Kein Gewinnversprechen."),
         11);
}
void UCoIPlayerHUD::AccountingReport()
{
    const auto &P = Sim->GetSnapshot().Player;
    if (!P.Rules.AccountingEnabled)
        return;
    Text(Body, TEXT("KOSTENRECHNUNG · KALENDERMONATE"), 14, true);
    Text(Body,
         TEXT("Realisierte externe Erlöse minus abgegangene Warenkosten und Periodenkosten. Unverkaufte Herstellung "
              "liegt im Warenbuchwert; interne Verrechnung erzeugt keinen Konzerngewinn."),
         11);
    if (P.AccountingPeriods.IsEmpty())
    {
        Empty(Body,
              TEXT("Noch keine Kostenperiode. Der erste reguläre Simulationsschritt beginnt die laufende Erfassung."));
        return;
    }
    for (int32 I = P.AccountingPeriods.Num() - 1; I >= 0 && I >= P.AccountingPeriods.Num() - 3; --I)
    {
        const auto &M = P.AccountingPeriods[I];
        Text(Body,
             Display::Period(P.StartDate, M.BeginHour, M.EndHour) +
                 (M.Complete ? TEXT(" · abgeschlossen") : TEXT(" · laufend, kein Monatsabschluss")) +
                 (M.Partial ? TEXT(" · unvollständige Eröffnungsperiode") : TEXT("")),
             12, true);
        if(M.Partial){Empty(Body,TEXT("Eröffnungs- oder Lohnhistorie unvollständig. Noch kein belastbarer Monatsabschluss; tatsächliche Zahlungen und Warenbuchwerte bleiben separat sichtbar."));continue;}
        Cells(Body,
              {TEXT("Externe Erlöse"), TEXT("Wareneinsatz"), TEXT("Periodenkosten"), TEXT("Fracht / Verderb"),
               TEXT("Ergebnis")},
              {1, 1, 1.2f, 1.3f, 1}, true);
        Cells(Body,
              {Display::Money(M.GoodsRevenue + M.ServiceRevenue), Display::Money(M.CostOfGoods),
               Display::Money(M.IdleCosts()),
               Display::Money(M.FreightExpense) + TEXT(" / ") + Display::Money(M.Spoilage),
               M.Known ? Display::Money(M.Profit()) : TEXT("Nicht verfügbar: Assetbasis fehlt")},
              {1, 1, 1.2f, 1.3f, 1});
        Text(Body,
             TEXT("Löhne ") + Display::Money(M.Wages) + TEXT(" + Betrieb/Lager ") + Display::Money(M.Operating) +
                 TEXT(" − in Herstellung aktiviert ") + Display::Money(M.CapitalizedWages + M.CapitalizedOperating) +
                 TEXT(" = nicht aktivierte Periodenkosten, davon Produktionsleerkosten ") +
                 Display::Money(M.ProductionIdle) + TEXT(". Realisierter Assetabgangssaldo ") +
                 Display::Money(M.DisposalGain) + TEXT(". Finanzierungskosten separat ")+Display::Money(M.FinancingExpense)+ TEXT(". Nationale Ergebnisabgabe ")+Display::Money(M.TaxExpense)+TEXT(". Investitionszahlungen sind kein unmittelbarer Aufwand."),
             11);
    }
}

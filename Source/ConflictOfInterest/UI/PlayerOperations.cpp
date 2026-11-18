#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "PlayerHUD.h"
#include "Simulation/SimArchive.h"
#include "Simulation/SimulationSubsystem.h"
using namespace CoISim;
namespace
{
FString OpNum(int64 N)
{
    return FString::Printf(TEXT("%lld"), N);
}
FString OpMoney(int64 N)
{
    return FText::AsNumber((double)N / 100.).ToString() + TEXT(" USD");
}
} // namespace
void UCoIPlayerHUD::OperatingInfo(const FString &Asset)
{
    const auto &P = Sim->GetSnapshot().Player;
    const auto *R = Find(P.OperatingRules, Asset);
    const auto *X = Find(P.OperatingStatus, Asset);
    if (!R || !X)
        return;
    Text(Body,
         FString(R->Enabled ? TEXT("Regelbetrieb aktiv") : TEXT("Automatischer Einkauf/Verkauf inaktiv")) +
             TEXT(" · Einkauf gebucht ") + OpMoney(X->SpentCents) + TEXT(" / Budget ") + OpMoney(R->BuyBudgetCents) +
             TEXT(" je ") + OpNum(P.Rules.RuleBudgetHours) + TEXT(" h"),
         12, true);
    if (const auto *Plant = Find(P.Facilities, Asset))
    {
        int64 Workers = 0;
        for (const auto &Job : Plant->Employment)
            Workers += Job.Workers;
        Text(Body,
             TEXT("Plan ") + OpNum(R->UtilizationPermille / 10) + TEXT(" % · zugeordnete Stellen ") + OpNum(Workers) +
                 TEXT(" · Lohnangebot ") + OpMoney(R->DailyWageCents) + TEXT(" je Stelle/Tag"),
             12);
    }
    Text(Body, X->Cause, 12);
    Button(Body, R->RecipeId.IsEmpty() ? TEXT("Lagerregeln …") : TEXT("Betriebsregeln …"), TEXT("operate|") + Asset);
}
void UCoIPlayerHUD::OpenOperating(const FString &Asset)
{
    if (!Sim || !Sim->IsReady())
        return;
    const auto &P = Sim->GetSnapshot().Player;
    const auto *R = Find(P.OperatingRules, Asset);
    if (!R)
    {
        LastStatus = TEXT("Nur eigene Assets besitzen bearbeitbare Betriebsregeln.");
        Refresh();
        return;
    }
    OperatingDraft = *R;
    OperatingDraft.Explicit = true;
    const auto *F = Find(P.Facilities, Asset);
    const auto *W = Find(P.Stores, F ? F->StorageId : Asset);
    if (!W || !W->Own)
        return;
    CityContext = LastCityContext = W->CityId;
    CityTab = TEXT("Eigene Assets");
    FocusedObject = Asset;
    BuildView(TEXT("Stadt"));
    DialogKind = TEXT("operate");
    ContextId = Asset;
    OpenDialog(F ? TEXT("BETRIEBSVORGABEN") : TEXT("LAGERREGELN"));
    Text(Form, W->Name + TEXT(" · ") + City(W->CityId), 14, true);
    if (const auto *X = Find(P.OperatingStatus, Asset))
    {
        Text(Form, X->Cause, 12);
        Text(Form,
             TEXT("Budgetperiode ab ") + Time(X->BudgetWindow * P.Rules.RuleBudgetHours) +
                 TEXT(" · tatsächlich ausgegeben ") + OpMoney(X->SpentCents),
             12);
    }
    Choice(Form, TEXT("form.op.enabled"), TEXT("Automatischer Einkauf / Verkauf"),
           {{TEXT("no"), TEXT("Inaktiv")}, {TEXT("yes"), TEXT("Aktiv")}},
           OperatingDraft.Enabled ? TEXT("yes") : TEXT("no"));
    if (F)
    {
        const auto *T = Find(P.Catalog.FacilityTypes, F->TypeId);
        int64 Workers = 0;
        for (const auto &Job : F->Employment)
            Workers += Job.Workers;
        Text(Form,
             TEXT("Zugeordnete Stellen: ") + OpNum(Workers) + TEXT(" / ") + OpNum(T->Jobs) +
                 TEXT(" · zuletzt bezahlter Tageslohn ") + OpMoney(F->PaidDailyWageCents) +
                 TEXT(" je Stelle · gedeckt bis ") + Time(F->PaidUntilHour),
             12);
        if (F->PaidUntilHour <= P.Hour)
            Text(Form,
                 TEXT("Aktuelle Lohndeckung ausstehend/abgelaufen. Produktion benötigt einen regulär gedeckten "
                      "Lohnlauf."),
                 12);
        Choice(Form, TEXT("form.op.recipe"), TEXT("Zulässiges Rezept dieses Betriebstyps"), {{T->Id, T->Name}}, T->Id);
        FString Recipe = TEXT("Je Charge: ");
        for (const auto &A : T->Inputs)
            Recipe += OpNum(A.Kg) + TEXT(" kg ") + Good(A.GoodId) + TEXT(" + ");
        Recipe += TEXT("→ ");
        for (const auto &A : T->Outputs)
            Recipe += OpNum(A.Kg) + TEXT(" kg ") + Good(A.GoodId) + TEXT("; ");
        Text(Form, Recipe, 12);
        auto *Production = Row(Form);
        Field(Production, TEXT("op.util"), TEXT("Geplante Auslastung % (0–100)"),
              OpNum(OperatingDraft.UtilizationPermille / 10));
        Field(Production, TEXT("op.wage"), TEXT("Lohnangebot Cent / Stelle / Tag"),
              OpNum(OperatingDraft.DailyWageCents));
        Text(Form,
             TEXT("Auslastung 0 hält Produktion an. Löhne ändern sich beim nächsten regulären Lohnlauf; bereits "
                  "bezahlte Zeit wird nicht erstattet. Feste Betriebskosten laufen weiter."),
             11);
    }
    Field(Form, TEXT("op.budget"),
          TEXT("Kaufbudget in Cent je ") + OpNum(P.Rules.RuleBudgetHours) + TEXT(" h (0 = keine neuen Käufe)"),
          OpNum(OperatingDraft.BuyBudgetCents));
    Text(Form,
         TEXT("Das Kaufbudget gilt je fester Simulationsperiode. Löhne, Betrieb, Lager und Transport sind "
              "separate Verpflichtungen. Gemeinsame Firmenliquidität gilt für alle Assets."),
         11);
    for (const auto &G : OperatingDraft.Goods)
    {
        Text(Form, Good(G.GoodId).ToUpper(), 13, true);
        auto *Values = Row(Form);
        Field(Values, TEXT("op.") + G.GoodId + TEXT(".min"), TEXT("Mindestreserve kg"), OpNum(G.MinimumKg));
        Field(Values, TEXT("op.") + G.GoodId + TEXT(".target"), TEXT("Zielbestand kg"), OpNum(G.TargetKg));
        bool Input = !F, Output = !F;
        if (F)
        {
            const auto *T = Find(P.Catalog.FacilityTypes, F->TypeId);
            Input = T->Inputs.ContainsByPredicate([&](const FAmount &A) { return A.GoodId == G.GoodId; });
            Output = T->Outputs.ContainsByPredicate([&](const FAmount &A) { return A.GoodId == G.GoodId; });
        }
        if (Input)
            Field(Values, TEXT("op.") + G.GoodId + TEXT(".buy"), TEXT("Einkauf max. Cent/kg; 0 aus"),
                  OpNum(G.BuyLimitCents));
        if (Output)
            Field(Values, TEXT("op.") + G.GoodId + TEXT(".sell"), TEXT("Überschuss min. Cent/kg; 0 aus"),
                  OpNum(G.SellLimitCents));
    }
    Text(Form,
         TEXT("Verkauft wird nur verfügbare Ware über Reserve UND Zielbestand. Einkauf berücksichtigt vorhandene "
              "Mengen, offene eigene Kaufaufträge und bereits unterwegs befindliche Ware. Keine automatische "
              "kostenlose Ortsverlagerung: Angebote an fremden Lagern brauchen weiterhin reale Transporte. Änderungen "
              "stornieren nur automatische Restaufträge dieses Assets."),
         12);
    FormPreview = Text(Form, TEXT(""), 12);
    Updating = false;
    Preview();
}
bool UCoIPlayerHUD::ReadOperatingForm(FString &Error)
{
    const auto &P = Sim->GetSnapshot().Player;
    OperatingDraft.Enabled = Id(TEXT("form.op.enabled")) == TEXT("yes");
    OperatingDraft.Explicit = true;
    if (!OperatingDraft.RecipeId.IsEmpty())
    {
        OperatingDraft.RecipeId = Id(TEXT("form.op.recipe"));
        int64 Percent = 0;
        if (!ReadPositive(TEXT("op.util"), Percent, true) || Percent > 100 ||
            !ReadPositive(TEXT("op.wage"), OperatingDraft.DailyWageCents))
        {
            Error = TEXT("Auslastung als ganze 0–100 % und positiven Tageslohn in Cent eingeben.");
            return false;
        }
        OperatingDraft.UtilizationPermille = Percent * 10;
    }
    if (!ReadPositive(TEXT("op.budget"), OperatingDraft.BuyBudgetCents, true))
    {
        Error = TEXT("Kaufbudget als ganze Centzahl ab 0 eingeben.");
        return false;
    }
    for (auto &G : OperatingDraft.Goods)
    {
        if (!ReadPositive(TEXT("op.") + G.GoodId + TEXT(".min"), G.MinimumKg, true) ||
            !ReadPositive(TEXT("op.") + G.GoodId + TEXT(".target"), G.TargetKg, true))
        {
            Error = TEXT("Reserve und Zielbestand als ganze kg ab 0 eingeben.");
            return false;
        }
        for (int Side = 0; Side < 2; ++Side)
        {
            const FString Key = TEXT("op.") + G.GoodId + (Side ? TEXT(".sell") : TEXT(".buy"));
            if (Fields.Contains(Key) && !ReadPositive(Key, Side ? G.SellLimitCents : G.BuyLimitCents, true))
            {
                Error = TEXT("Preislimit als ganze Cent/kg ab 0 eingeben.");
                return false;
            }
        }
    }
    if (OperatingDraft.BuyBudgetCents > P.Rules.RuleMaxBuyBudgetCents)
    {
        Error = TEXT("Kaufbudget überschreitet die Szenariogrenze.");
        return false;
    }
    return true;
}

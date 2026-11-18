#include "SimArchive.h"
#include "SimCore.h"
namespace CoISim
{
namespace
{
FOperatingRule DefaultOperatingRule(const FScenario &S, const FString &Id, const FString &Owner, const FFacility *F)
{
    FOperatingRule R;
    R.Id = Id;
    R.OwnerId = Owner;
    R.BuyBudgetCents = S.Economy.RuleDefaultBuyBudgetCents;
    if (F)
    {
        const auto *T = Find(S.Definitions.FacilityTypes, F->TypeId);
        R.RecipeId = T->Id;
        R.DailyWageCents = T->DailyWageCents;
        for (const auto &A : T->Inputs)
        {
            FStockRule G;
            G.GoodId = A.GoodId;
            G.TargetKg =
                FMath::Min(S.Economy.RuleMaxStockKg, A.Kg * T->CapacityBatchesPerHour * S.Economy.InputTargetHours);
            G.BuyLimitCents = FMath::Min(S.Economy.MaxPriceCents, Find(S.Definitions.Goods, A.GoodId)->InputLimitCents);
            R.Goods.Add(G);
        }
        for (const auto &A : T->Outputs)
        {
            auto *G = R.Goods.FindByPredicate([&](const FStockRule &X) { return X.GoodId == A.GoodId; });
            if (!G)
            {
                FStockRule X;
                X.GoodId = A.GoodId;
                R.Goods.Add(X);
                G = &R.Goods.Last();
            }
            G->MinimumKg = FMath::Min(S.Economy.RuleMaxStockKg, S.Economy.ProducerReserveKg);
            G->TargetKg = FMath::Max(G->TargetKg, G->MinimumKg);
            G->SellLimitCents = FMath::Clamp(
                (Find(S.Definitions.Goods, A.GoodId)->InitialUnitCostCents * (1000 + T->MarginPermille) + 999) / 1000,
                1LL, S.Economy.MaxPriceCents);
        }
    }
    else
        for (const auto &G : S.Definitions.Goods)
        {
            FStockRule X;
            X.GoodId = G.Id;
            R.Goods.Add(X);
        }
    return R;
}
} // namespace
void FSimulation::InitializeOperatingRules()
{
    auto Add = [&](const FString &Id, const FString &Owner, const FFacility *F) {
        auto *R = Find(State.OperatingRules, Id);
        if (!R)
        {
            State.OperatingRules.Add(DefaultOperatingRule(Scenario, Id, Owner, F));
            FOperatingStatus X;
            X.Id = Id;
            X.BudgetWindow = State.EconomyHour / Scenario.Economy.RuleBudgetHours;
            State.OperatingStatus.Add(X);
        }
        else if (R->OwnerId != Owner)
        {
            *R = DefaultOperatingRule(Scenario, Id, Owner, F);
            auto *X = Find(State.OperatingStatus, Id);
            *X = FOperatingStatus();
            X->Id = Id;
            X->BudgetWindow = State.EconomyHour / Scenario.Economy.RuleBudgetHours;
        }
    };
    for (const auto &F : State.Facilities)
        Add(F.Id, F.OwnerId, &F);
    for (const auto &W : State.Warehouses)
        Add(W.Id, W.OwnerId, nullptr);
}
bool FSimulation::OperatingEnabled(const FOperatingRule &R) const
{
    if (R.Explicit)
        return R.Enabled;
    return Find(State.Facilities, R.Id) && IsAutonomous(R.OwnerId) &&
           Find(Scenario.Definitions.Companies, R.OwnerId)->DecisionProfile == TEXT("producer");
}
int64 FSimulation::OfferedWage(const FFacility &F) const
{
    const auto *R = Find(State.OperatingRules, F.Id);
    return R ? R->DailyWageCents : Find(Scenario.Definitions.FacilityTypes, F.TypeId)->DailyWageCents;
}
int64 FSimulation::PlannedJobs(const FFacility &F) const
{
    const auto *R = Find(State.OperatingRules, F.Id);
    const auto *T = Find(Scenario.Definitions.FacilityTypes, F.TypeId);
    return (T->Jobs * (R ? R->UtilizationPermille : 1000) + 999) / 1000;
}
bool FSimulation::PreviewOperatingRule(const FOperatingRule &R, FString &E) const
{
    auto Fail = [&](const TCHAR *Why) {
        E = Why;
        return false;
    };
    const auto &Q = Scenario.Economy;
    const auto *F = Find(State.Facilities, R.Id);
    const auto *W = Find(State.Warehouses, F ? F->StorageId : R.Id);
    const auto *T = F ? Find(Scenario.Definitions.FacilityTypes, F->TypeId) : nullptr;
    if (!W || W->OwnerId != R.OwnerId || (F && F->OwnerId != R.OwnerId) || !Find(State.Companies, R.OwnerId))
        return Fail(TEXT("Asset fehlt oder gehört nicht zu diesem Unternehmen."));
    if ((F ? R.RecipeId != F->TypeId : !R.RecipeId.IsEmpty()) || R.UtilizationPermille < 0 ||
        R.UtilizationPermille > 1000 || R.DailyWageCents < 1 || R.DailyWageCents > Q.RuleMaxWageCents ||
        R.BuyBudgetCents < 0 || R.BuyBudgetCents > Q.RuleMaxBuyBudgetCents)
        return Fail(TEXT("Unzulässiges Rezept, Auslastung, Lohn oder Einkaufsbudget."));
    TSet<FString> Goods;
    for (const auto &G : R.Goods)
    {
        if (!Find(Scenario.Definitions.Goods, G.GoodId) || Goods.Contains(G.GoodId) || G.MinimumKg < 0 ||
            G.TargetKg < G.MinimumKg || G.TargetKg > Q.RuleMaxStockKg || G.BuyLimitCents < 0 ||
            G.BuyLimitCents > Q.MaxPriceCents || G.SellLimitCents < 0 || G.SellLimitCents > Q.MaxPriceCents)
            return Fail(TEXT("Warenregel ungültig: Reserve ≤ Zielbestand; Mengen und Cent/kg innerhalb der Grenzen."));
        Goods.Add(G.GoodId);
        if (T)
        {
            bool Input = T->Inputs.ContainsByPredicate([&](const FAmount &A) { return A.GoodId == G.GoodId; }),
                 Output = T->Outputs.ContainsByPredicate([&](const FAmount &A) { return A.GoodId == G.GoodId; });
            if ((!Input && !Output) || (G.BuyLimitCents && !Input) || (G.SellLimitCents && !Output))
                return Fail(TEXT("Einkauf nur für Rezeptinputs; Verkauf nur für zulässige Outputs."));
        }
    }
    int64 Required = 0;
    for (const auto &G : Scenario.Definitions.Goods)
    {
        bool Uses = !T || T->Inputs.ContainsByPredicate([&](const FAmount &A) { return A.GoodId == G.Id; }) ||
                    T->Outputs.ContainsByPredicate([&](const FAmount &A) { return A.GoodId == G.Id; });
        if (Uses)
        {
            ++Required;
            if (!Goods.Contains(G.Id))
                return Fail(TEXT("Warenregeln müssen alle zulässigen Rezept-/Lagerwaren enthalten."));
        }
    }
    if (R.Goods.Num() != Required)
        return Fail(TEXT("Warenregelabdeckung unvollständig."));
    E.Empty();
    return true;
}
bool FSimulation::ChangeOperatingRule(const FOperatingRule &R, FString &E)
{
    if (!R.Explicit)
    {
        E = TEXT("Spielervorgaben müssen ausdrücklich bestätigt werden.");
        return false;
    }
    if (!PreviewOperatingRule(R, E))
        return false;
    auto *Existing = Find(State.OperatingRules, R.Id);
    auto *Status = Find(State.OperatingStatus, R.Id);
    if (!Existing || !Status)
    {
        E = TEXT("Betriebszustand fehlt.");
        return false;
    }
    const auto *Plant = Find(State.Facilities, R.Id);
    const FString StoreId = Plant ? Plant->StorageId : R.Id;
    // A plant and its warehouse can protect the same physical stock. Reconcile their automatic orders together
    // before the pre-hour matcher can fill an order based on an obsolete shared reserve.
    for (const auto &Related : State.OperatingRules)
    {
        const auto *F = Find(State.Facilities, Related.Id);
        if (Related.OwnerId != R.OwnerId || (F ? F->StorageId : Related.Id) != StoreId)
            continue;
        for (auto &O : State.Orders)
            if (O.Status == TEXT("open") && O.RuleId == Related.Id)
                CancelOrder(O.Id, R.OwnerId, TEXT("rule_changed"));
        Find(State.OperatingStatus, Related.Id)->NextDecisionHour = State.EconomyHour;
    }
    *Existing = R;
    Status->NextDecisionHour = State.EconomyHour;
    Status->Cause = TEXT("Vorgabe übernommen; Prüfung im regulären Betriebsdienst.");
    E = TEXT("Betriebsvorgaben übernommen. Automatische Restaufträge am betroffenen Lagerort storniert; manuelle "
             "Aufträge bleiben. "
             "Lohnänderung gilt beim nächsten Lohnlauf; bereits bezahlte Arbeit wird nicht erstattet.");
    return true;
}
bool FSimulation::OperatingOrder(const FString &Actor, const FString &Store, const FString &Good, const FString &Side,
                                 int64 Kg, int64 Price, const FString &RuleId, FString &Error)
{
    if (Kg <= 0 || Price <= 0)
    {
        Error = TEXT("Keine verfügbare Menge oder gültiges Preislimit.");
        return false;
    }
    const auto *W = Find(State.Warehouses, Store);
    if (!W)
    {
        Error = TEXT("Physisches Lager fehlt.");
        return false;
    }
    FMarketOrder O;
    O.Id = FString::Printf(TEXT("auto.%lld"), State.OrderSequence + 1);
    O.ActorId = Actor;
    O.StorageId = Store;
    O.CityId = W->CityId;
    O.MarketId = Find(Scenario.Definitions.Regions, Find(Scenario.Definitions.Places, W->CityId)->RegionId)->MarketId;
    O.GoodId = Good;
    O.Side = Side;
    O.OriginalKg = FMath::Min(Kg, Scenario.Economy.MaxOrderKg);
    O.LimitCentsPerKg = FMath::Min(Price, Scenario.Economy.MaxPriceCents);
    if (!PlaceOrder(O, Error))
        return false;
    State.Orders.Last().RuleId = RuleId;
    if (!RuleId.IsEmpty())
        State.Orders.Last().ExpiresStep =
            FMath::Min(State.Orders.Last().ExpiresStep,
                       (State.EconomyHour / Scenario.Economy.RuleBudgetHours + 1) * Scenario.Economy.RuleBudgetHours);
    return true;
}
void FSimulation::OperatingService(int64 H)
{
    for (const auto &R : State.OperatingRules)
    {
        auto *Status = Find(State.OperatingStatus, R.Id);
        if (H < Status->NextDecisionHour)
            continue;
        Status->NextDecisionHour = H + Scenario.Economy.DecisionHours;
        for (auto &O : State.Orders)
            if (O.Status == TEXT("open") && O.RuleId == R.Id)
                CancelOrder(O.Id, R.OwnerId, TEXT("repriced"));
        if (Status->BudgetWindow != H / Scenario.Economy.RuleBudgetHours)
        {
            Status->BudgetWindow = H / Scenario.Economy.RuleBudgetHours;
            Status->SpentCents = 0;
        }
        if (!OperatingEnabled(R))
        {
            Status->Cause = TEXT("Automatischer Einkauf und Verkauf inaktiv.");
            continue;
        }
        const auto *F = Find(State.Facilities, R.Id);
        const auto *W = Find(State.Warehouses, F ? F->StorageId : R.Id);
        const auto *T = F ? Find(Scenario.Definitions.FacilityTypes, F->TypeId) : nullptr;
        auto *Account = Find(State.Accounts, ActorAccount(R.OwnerId));
        FString Causes;
        auto Cause = [&](const FString &Text) {
            if (!Causes.Contains(Text))
            {
                if (!Causes.IsEmpty())
                    Causes += TEXT(" · ");
                Causes += Text;
            }
        };
        // Keep next regular wages and fixed costs visible/available; every asset shares the real account holds.
        int64 WorkingCapital = 0;
        for (const auto &Plant : State.Facilities)
            if (Plant.OwnerId == R.OwnerId)
                WorkingCapital += PlannedJobs(Plant) * OfferedWage(Plant) * (Scenario.Economy.PayrollHours / 24) +
                                  Find(Scenario.Definitions.FacilityTypes, Plant.TypeId)->DailyOperatingCostCents;
        for (const auto &S : State.Warehouses)
            if (S.OwnerId == R.OwnerId)
                WorkingCapital += Find(Scenario.Definitions.WarehouseTypes, S.TypeId)->DailyCostCents;
        for (const auto &V : State.Vehicles)
            if (V.OwnerId == R.OwnerId)
                WorkingCapital += Find(Scenario.Definitions.VehicleTypes, V.TypeId)->DailyCostCents;
        for (const auto &G : R.Goods)
        {
            int32 Index = StockIndex(R.OwnerId, W->Id, G.GoodId);
            const auto *Stock = Index == INDEX_NONE ? nullptr : &State.Inventories[Index];
            int64 Quantity = Stock ? Stock->QuantityKg : 0, Reserved = Stock ? Stock->ReservedKg : 0, Incoming = 0,
                  Ordered = 0;
            for (const auto &O : State.Orders)
                if (O.Status == TEXT("open") && O.Side == TEXT("buy") && O.ActorId == R.OwnerId &&
                    O.StorageId == W->Id && O.GoodId == G.GoodId)
                    Ordered += O.RemainingKg;
            for (const auto &J : State.Transports)
                if ((J.Status == TEXT("planned") || J.Status == TEXT("moving")) && J.OwnerId == R.OwnerId &&
                    J.DestinationStorageId == W->Id && J.GoodId == G.GoodId)
                {
                    const auto *Cargo = Find(State.Inventories, J.CargoInventoryId);
                    Incoming += J.Status == TEXT("moving") && Cargo ? Cargo->QuantityKg : J.Kg;
                }
            int64 Keep = FMath::Max(G.MinimumKg, G.TargetKg);
            for (const auto &Other : State.OperatingRules)
                if (OperatingEnabled(Other) && Other.OwnerId == R.OwnerId)
                {
                    const auto *Plant = Find(State.Facilities, Other.Id);
                    if ((Plant ? Plant->StorageId : Other.Id) == W->Id)
                        for (const auto &X : Other.Goods)
                            if (X.GoodId == G.GoodId)
                                Keep = FMath::Max(Keep, FMath::Max(X.MinimumKg, X.TargetKg));
                }
            int64 SellPrice = G.SellLimitCents;
            if (!R.Explicit && T && SellPrice)
            {
                const auto *Output = T->Outputs.FindByPredicate([&](const FAmount &A) { return A.GoodId == G.GoodId; });
                int64 Unit = Quantity ? (Stock->BookValueCents + Quantity - 1) / Quantity
                                      : Find(Scenario.Definitions.Goods, G.GoodId)->InitialUnitCostCents;
                int64 Overhead =
                    (T->DailyOperatingCostCents + Find(Scenario.Definitions.WarehouseTypes, W->TypeId)->DailyCostCents +
                     24 * Output->Kg - 1) /
                    (24 * Output->Kg);
                SellPrice = FMath::Max(1LL, ((Unit + Overhead) * (1000 + T->MarginPermille) + 999) / 1000);
            }
            if (SellPrice && Quantity - Reserved > Keep)
            {
                FString Error;
                if (!OperatingOrder(R.OwnerId, W->Id, G.GoodId, TEXT("sell"), Quantity - Reserved - Keep, SellPrice,
                                    R.Id, Error))
                    Cause(Error);
            }
            int64 Need = FMath::Max(0LL, G.TargetKg - (Quantity - Reserved) - Incoming - Ordered - DeliveryPromises(R.OwnerId,W->Id,G.GoodId));
            if (!G.BuyLimitCents || !Need)
                continue;
            const auto *Good = Find(Scenario.Definitions.Goods, G.GoodId);
            int64 OfferedHere = 0;
            for (const auto &O : State.Orders)
                if (O.Status == TEXT("open") && O.Side == TEXT("sell") && O.ActorId != R.OwnerId &&
                    O.StorageId == W->Id && O.GoodId == G.GoodId)
                    OfferedHere += O.RemainingKg;
            int64 Space = Find(Scenario.Definitions.WarehouseTypes, W->TypeId)->CapacityPoints - UsedStorage(W->Id) -
                          W->ReservedIncomingPoints + OfferedHere * Good->StoragePointsPerKg;
            int64 Held = 0;
            for (const auto &O : State.Orders)
                if (O.Status == TEXT("open") && O.Side == TEXT("buy") && O.RuleId == R.Id)
                    Held += O.ReservedCents;
            int64 Budget = FMath::Max(0LL, R.BuyBudgetCents - Status->SpentCents - Held),
                  Cash = FMath::Max(0LL, Account->BalanceCents - Account->ReservedCents - WorkingCapital);
            int64 Kg = FMath::Min(Need, FMath::Min(Budget, Cash) / G.BuyLimitCents);
            Kg = FMath::Min(Kg, FMath::Max(0LL, Space) / Good->StoragePointsPerKg);
            if (Kg < Need)
            {
                if (Space < Need * Good->StoragePointsPerKg)
                    Cause(TEXT("Lagerkapazität begrenzt"));
                if (Budget < Need * G.BuyLimitCents)
                    Cause(TEXT("Einkaufsbudget begrenzt"));
                if (Cash < Need * G.BuyLimitCents)
                    Cause(TEXT("Freies Kapital nach Verpflichtungen begrenzt"));
            }
            if (Kg > 0)
            {
                FString Error;
                if (!OperatingOrder(R.OwnerId, W->Id, G.GoodId, TEXT("buy"), Kg, G.BuyLimitCents, R.Id, Error))
                    Cause(Error);
                else
                    Cause(TEXT(
                        "Beschaffung offen: eigenes Lager als Lieferort; fremde Orte benötigen bezahlten Transport"));
            }
        }
        Status->Cause = Causes.IsEmpty() ? TEXT("Zielbestände/Reserven gedeckt oder keine freigegebenen Überschüsse.")
                                         : Causes.Left(500);
    }
}
FString FSimulation::OperatingCause(const FOperatingRule &R) const
{
    const auto *Status = Find(State.OperatingStatus, R.Id);
    FString Result = OperatingEnabled(R) ? (Status ? Status->Cause : TEXT("Prüfung ausstehend"))
                                         : TEXT("Automatischer Einkauf/Verkauf inaktiv.");
    const auto *F = Find(State.Facilities, R.Id);
    if (!F)
        return Result;
    auto Add = [&](const TCHAR *T) {
        if (!Result.IsEmpty())
            Result += TEXT(" · ");
        Result += T;
    };
    if (!R.UtilizationPermille)
    {
        Add(TEXT("Produktion angehalten: geplante Auslastung 0 %."));
        return Result;
    }
    int64 Workers = 0;
    for (const auto &J : F->Employment)
        Workers += J.Workers;
    if (Workers < PlannedJobs(*F))
        Add(F->UnfundedJobs ? TEXT("Personal: Lohnzahlung nicht gedeckt oder Betriebskosten rückständig.")
                            : TEXT("Personal: regionale Arbeitskräfte fehlen oder nächster Lohnlauf steht aus."));
    const auto *T = Find(Scenario.Definitions.FacilityTypes, F->TypeId);
    for (const auto &G : T->Inputs)
    {
        int32 I = StockIndex(F->OwnerId, F->StorageId, G.GoodId);
        if (I == INDEX_NONE || State.Inventories[I].QuantityKg - State.Inventories[I].ReservedKg < G.Kg)
        {
            Add(TEXT("Produktion: Rezeptinputs fehlen am Betriebslager."));
            break;
        }
    }
    const auto *W = Find(State.Warehouses, F->StorageId);
    int64 Net = 0;
    for (const auto &A : T->Outputs)
        Net += A.Kg * Find(Scenario.Definitions.Goods, A.GoodId)->StoragePointsPerKg;
    for (const auto &A : T->Inputs)
        Net -= A.Kg * Find(Scenario.Definitions.Goods, A.GoodId)->StoragePointsPerKg;
    if (Net > 0 && UsedStorage(W->Id) + W->ReservedIncomingPoints + Net >
                       Find(Scenario.Definitions.WarehouseTypes, W->TypeId)->CapacityPoints)
        Add(TEXT("Produktion: Lagerkapazität für eine Charge fehlt."));
    if (F->OperatingArrearsCents || W->ArrearsCents)
        Add(TEXT("Betriebs-/Lagerkosten rückständig."));
    if (const auto *Site = Find(State.ResourceSites, F->ResourceSiteId))
        if (!Site->RemainingKg && Find(Scenario.Definitions.ResourceSites, F->ResourceSiteId)->Kind != TEXT("farmland"))
            Add(TEXT("Endliche Rohstoffquelle erschöpft."));
    return Result;
}
bool FSimulation::ValidateOperatingRules(FString &E) const
{
    auto Fail = [&](const TCHAR *Why) {
        E = Why;
        return false;
    };
    if (State.OperatingRules.Num() != State.Facilities.Num() + State.Warehouses.Num() ||
        State.OperatingStatus.Num() != State.OperatingRules.Num())
        return Fail(TEXT("Betriebsregelabdeckung ungültig."));
    TSet<FString> Seen, Statuses;
    for (const auto &R : State.OperatingRules)
    {
        if (Seen.Contains(R.Id))
            return Fail(TEXT("Doppelte Betriebsregel-ID."));
        if (!PreviewOperatingRule(R, E))
            return false;
        Seen.Add(R.Id);
    }
    for (const auto &X : State.OperatingStatus)
    {
        if (Statuses.Contains(X.Id) || !Seen.Contains(X.Id) || X.BudgetWindow < 0 ||
            X.BudgetWindow > State.EconomyHour / Scenario.Economy.RuleBudgetHours || X.NextDecisionHour < 0 ||
            X.NextDecisionHour > State.EconomyHour + Scenario.Economy.DecisionHours || X.SpentCents < 0 ||
            X.SpentCents > Scenario.Economy.RuleMaxBuyBudgetCents)
            return Fail(TEXT("Betriebsbudget/-zeitpunkt ungültig."));
        Statuses.Add(X.Id);
    }
    for (const auto &O : State.Orders)
        if (!O.RuleId.IsEmpty())
        {
            const auto *R = Find(State.OperatingRules, O.RuleId);
            const auto *F = R ? Find(State.Facilities, R->Id) : nullptr;
            if (!R || (O.Status == TEXT("open") &&
                       (O.ActorId != R->OwnerId || O.StorageId != (F ? F->StorageId : R->Id) || !OperatingEnabled(*R))))
                return Fail(TEXT("Automatischer Auftrag ohne gültige Betriebszuordnung."));
        }
    for (const auto &R : State.OperatingRules)
    {
        const auto *X = Find(State.OperatingStatus, R.Id);
        int64 Held = 0;
        for (const auto &O : State.Orders)
            if (O.Status == TEXT("open") && O.RuleId == R.Id)
            {
                if (O.ExpiresStep > (X->BudgetWindow + 1) * Scenario.Economy.RuleBudgetHours)
                    return Fail(TEXT("Betriebsauftrag überschreitet Budgetperiode."));
                if (O.Side == TEXT("buy"))
                    Held += O.ReservedCents;
            }
        if (Held > FMath::Max(0LL, R.BuyBudgetCents - X->SpentCents))
            return Fail(TEXT("Ungedeckte Betriebsbudgetreservierung."));
    }
    return true;
}
} // namespace CoISim

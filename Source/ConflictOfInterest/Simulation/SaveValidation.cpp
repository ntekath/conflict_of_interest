#include "SaveGame.h"
#include "SimArchive.h"
#include "Banking.h"
#include "WaterTransport.h"
namespace CoISim
{
namespace
{
template <class T> bool SaveUnique(const TArray<T> &Rows)
{
    TSet<FString> Seen;
    for (const auto &R : Rows)
    {
        if (R.Id.IsEmpty() || Seen.Contains(R.Id))
            return false;
        Seen.Add(R.Id);
    }
    return true;
}
} // namespace
bool FSimulation::ValidateSaveState(FString &E) const
{
    const auto &W = State;
    const auto &D = Scenario.Definitions;
    const int64 H = W.EconomyHour;
    auto Fail = [&](const FString &Why) {
        E = TEXT("Ungültiger Spielzustand: ") + Why;
        return false;
    };
    auto Actor = [&](const FString &Id) { return Find(W.Companies, Id) || Find(W.Groups, Id); };
    auto Amounts = [&](const TArray<FAmount> &A) {
        TSet<FString> Seen;
        for (const auto &X : A)
        {
            if (!Find(D.Goods, X.GoodId) || X.Kg < 0 || Seen.Contains(X.GoodId))
                return false;
            Seen.Add(X.GoodId);
        }
        return true;
    };
    if (Session.ScenarioId != Scenario.Id || Session.PlayerCompanyId != Scenario.PlayerCompanyId)
        return Fail(TEXT("Steuerungszuordnung"));
    const auto *Player = Find(D.Companies, Session.PlayerCompanyId);
    if (!Player || !Player->Playable || Player->AccountingOnly)
        return Fail(TEXT("Spielerfirma"));
    if (W.Step < 0 || W.Step > 1000000000LL || H != W.Step * Scenario.Settings.HoursPerStep || H > 50000000 ||
        W.OrderSequence < 0 || W.TradeSequence < 0 || W.DeveloperMoneySources < 0 || W.DeveloperMoneySinks < 0)
        return Fail(TEXT("Zeit/Zähler"));
#define CHECK_IDS(Name)                                                                                                \
    if (!SaveUnique(W.Name))                                                                                           \
        return Fail(TEXT(#Name " IDs"));
    CHECK_IDS(Accounts)
    CHECK_IDS(Companies)
    CHECK_IDS(Groups)
    CHECK_IDS(Facilities)
    CHECK_IDS(Warehouses) CHECK_IDS(Vehicles) CHECK_IDS(Inventories) CHECK_IDS(Orders) CHECK_IDS(Trades)
        CHECK_IDS(Transports) CHECK_IDS(Offers) CHECK_IDS(Reservations) CHECK_IDS(Regions) CHECK_IDS(Countries)
            CHECK_IDS(GoodsAudit) CHECK_IDS(MarketTotals) CHECK_IDS(CashTotals) CHECK_IDS(Projects)
                CHECK_IDS(AssetSales) CHECK_IDS(ResourceSites) CHECK_IDS(PlayerDays) CHECK_IDS(PlayerEvents)
#undef CHECK_IDS
                    if (W.Accounts.Num() != Scenario.InitialState.Accounts.Num() ||
                        W.Companies.Num() != D.Companies.Num() || W.Groups.Num() != D.Groups.Num() ||
                        W.Regions.Num() != D.Regions.Num() ||
                        W.GoodsAudit.Num() != D.Goods.Num()) return Fail(TEXT("Entitätenabdeckung"));
    int64 InitialMoney = 0;
    for (const auto &A : Scenario.InitialState.Accounts)
        InitialMoney += A.BalanceCents;
    if (InitialMoney != W.InitialMoneyCents)
        return Fail(TEXT("Startgeld"));
    for (const auto &A : W.Accounts)
    {
        const auto *I = Find(Scenario.InitialState.Accounts, A.Id);
        if (!I || A.OwnerId != I->OwnerId || !Actor(A.OwnerId))
            return Fail(A.Id);
    }
    for (const auto &C : W.Companies)
    {
        const auto *I = Find(Scenario.InitialState.Companies, C.Id);
        if (!I || C.AccountId != I->AccountId || !Find(W.Accounts, C.AccountId))
            return Fail(C.Id);
    }
    for (const auto &G : W.Groups)
    {
        const auto *I = Find(Scenario.InitialState.Groups, G.Id);
        if (!I || G.AccountId != I->AccountId || !Find(W.Accounts, G.AccountId) || G.Population != I->Population ||
            G.Workforce != I->Workforce || G.Employed < 0 || G.Employed > G.Workforce || G.DailyIncomeCents < 0 ||
            G.BudgetCents < 0 || G.DailySpentCents < 0 || G.LuxurySpentCents < 0 ||
            G.LuxurySpentCents > G.DailySpentCents || G.BasicRemainder < 0 || G.BasicRemainder >= 1000 ||
            G.LuxuryRemainder < 0 || G.LuxuryRemainder >= 1000 || !Amounts(G.Consumed) || !Amounts(G.UnmetNeed) ||
            !Amounts(G.MissedNeed))
            return Fail(G.Id);
    }
    for (const auto &S : W.Warehouses)
        if (!Find(D.WarehouseTypes, S.TypeId) || !Find(W.Companies, S.OwnerId) || !Find(D.Places, S.CityId) ||
            !Find(W.Accounts, S.CostRecipientAccountId) || S.ArrearsCents < 0)
            return Fail(S.Id);
    for (const auto &V : W.Vehicles)
        if (!Find(D.VehicleTypes, V.TypeId) || !Find(W.Companies, V.OwnerId) ||
            !Find(W.Accounts, V.CostRecipientAccountId) || V.ArrearsCents < 0 || V.Bound != !V.TransportId.IsEmpty())
            return Fail(V.Id);
    for (const auto &F : W.Facilities)
    {
        const auto *T = Find(D.FacilityTypes, F.TypeId);
        const auto *P = Find(D.Places, F.CityId);
        if (!T || !P || !Find(W.Companies, F.OwnerId) || !Find(W.Warehouses, F.StorageId) ||
            !Find(W.Accounts, F.CostRecipientAccountId) ||
            (!F.Employment.IsEmpty() && F.PaidUntilHour > H && F.PaidDailyWageCents == 0) || F.PaidDailyWageCents < 0 ||
            F.PaidDailyWageCents > Scenario.Economy.RuleMaxWageCents || F.UtilizationRemainder < 0 ||
            F.UtilizationRemainder >= 1000 || F.PaidUntilHour < 0 ||
            F.PaidUntilHour > H + Scenario.Economy.PayrollHours || F.OperatingPaidUntilHour < 0 ||
            F.OperatingPaidUntilHour > H + Scenario.Economy.CostsHours || F.ProductionRemainder < 0 ||
            F.ProductionRemainder >= T->Jobs * 1000 || F.LastBatches < 0 || F.LastBatches > T->CapacityBatchesPerHour ||
            F.UnfundedJobs < 0 || F.UnfundedJobs > T->Jobs || F.UtilizationPermille < 0 ||
            F.UtilizationPermille > 1000 || F.PayrollArrearsCents < 0 || F.OperatingArrearsCents < 0)
            return Fail(F.Id);
        TSet<FString> Seen;
        int64 Workers = 0;
        for (const auto &J : F.Employment)
        {
            const auto *G = Find(D.Groups, J.GroupId);
            if (!G || G->RegionId != P->RegionId || J.Workers <= 0 || Seen.Contains(J.GroupId))
                return Fail(F.Id);
            Seen.Add(J.GroupId);
            Workers += J.Workers;
        }
        if (Workers > T->Jobs)
            return Fail(F.Id);
    }
    TSet<FString> PhysicalStocks;
    for (const auto &I : W.Inventories)
    {
        if (!Find(D.Goods, I.GoodId) || !Actor(I.OwnerId) || I.AcquiredHour < 0 || I.AcquiredHour > H ||
            I.RentArrearsCents < 0 || I.SpoilRemainder < 0)
            return Fail(I.Id);
        if (Find(W.Warehouses, I.StorageId))
        {
            FString Key = I.OwnerId + TEXT("|") + I.StorageId + TEXT("|") + I.GoodId;
            if (PhysicalStocks.Contains(Key))
                return Fail(I.Id);
            PhysicalStocks.Add(Key);
        }
        else
        {
            const auto *J = W.Transports.FindByPredicate(
                [&](const FTransport &X) { return X.Status == TEXT("moving") && X.CargoInventoryId == I.Id; });
            if (!J || I.StorageId != I.Id || J->OwnerId != I.OwnerId || J->GoodId != I.GoodId || I.QuantityKg > J->Kg ||
                I.ReservedKg)
                return Fail(I.Id);
        }
    }
    TSet<int64> OrderSequences;
    for (const auto &O : W.Orders)
    {
        const auto *P = Find(D.Places, O.CityId);
        const auto *S = Find(W.Warehouses, O.StorageId);
        if (!P || !S || S->CityId != O.CityId || !Actor(O.ActorId) || !Find(D.Goods, O.GoodId) ||
            Find(D.Regions, P->RegionId)->MarketId != O.MarketId || (O.Side != TEXT("buy") && O.Side != TEXT("sell")) ||
            O.OriginalKg <= 0 || O.OriginalKg > Scenario.Economy.MaxOrderKg || O.RemainingKg < 0 ||
            O.RemainingKg > O.OriginalKg || O.LimitCentsPerKg <= 0 ||
            O.LimitCentsPerKg > Scenario.Economy.MaxPriceCents || O.Sequence <= 0 || O.Sequence > W.OrderSequence ||
            OrderSequences.Contains(O.Sequence) || O.CreatedStep < 0 || O.CreatedStep > H ||
            O.ExpiresStep <= O.CreatedStep)
            return Fail(O.Id);
        OrderSequences.Add(O.Sequence);
        if (O.Status == TEXT("open"))
        {
            if (O.RemainingKg <= 0 ||
                O.ReservedCents != (O.Side == TEXT("buy") ? O.RemainingKg * O.LimitCentsPerKg : 0) ||
                O.ReservedKg != (O.Side == TEXT("sell") ? O.RemainingKg : 0))
                return Fail(O.Id);
            if (O.Side == TEXT("sell") && StockIndex(O.ActorId, O.StorageId, O.GoodId) == INDEX_NONE)
                return Fail(O.Id);
        }
        else if (O.ReservedCents || O.ReservedKg ||
                 (O.Status != TEXT("filled") && O.Status != TEXT("cancelled") && O.Status != TEXT("expired") &&
                  O.Status != TEXT("spoilage") && O.Status != TEXT("day_end") && O.Status != TEXT("repriced") &&
                  O.Status != TEXT("route_evaluation") && O.Status != TEXT("rule_changed")))
            return Fail(O.Id);
    }
    for (const auto &T : W.Trades)
        if (!Actor(T.BuyerId) || !Actor(T.SellerId) || T.BuyerId == T.SellerId || !Find(D.Places, T.CityId) ||
            !Find(D.Goods, T.GoodId) || T.MarketId != Find(D.Regions, Find(D.Places, T.CityId)->RegionId)->MarketId ||
            T.QuantityKg <= 0 || T.QuantityKg > Scenario.Economy.MaxOrderKg || T.PriceCentsPerKg <= 0 ||
            T.PriceCentsPerKg > Scenario.Economy.MaxPriceCents || T.TotalCents != T.QuantityKg * T.PriceCentsPerKg ||
            T.Step < 0 || T.Step > H)
            return Fail(T.Id);
    for (const auto &J : W.Transports)
    {
        const auto *V = Find(W.Vehicles, J.VehicleId);
        const auto *T = V ? Find(D.VehicleTypes, V->TypeId) : nullptr;
        bool Active = J.Status == TEXT("planned") || J.Status == TEXT("moving");
        if (J.DeliveredKg<0||J.DeliveredKg>J.Kg||J.DeliveredBookCents<0||J.ActualArrivalHour<0||J.ActualArrivalHour>H||(J.Status==TEXT("arrived")?J.ActualArrivalHour!=J.ArrivalStep:J.ActualArrivalHour!=0)||(!J.ReportSiteId.IsEmpty()&&!Find(W.Warehouses,J.ReportSiteId)&&!Find(W.Vehicles,J.ReportSiteId)))return Fail(J.Id);
        if (!V || !T || !Find(W.Companies, J.OwnerId) || !Find(W.Companies, J.ProviderId) ||
            !Find(D.Places, J.FromCityId) || !Find(D.Places, J.ToCityId) || J.DepartureStep < 0 ||
            J.ArrivalStep <= J.DepartureStep || J.CostCents < 0 || J.OperatingCents < 0 || J.FreightCents < 0 ||
            J.Kg < 0 || J.Kg > T->CapacityKg ||
            (!Active && J.Status != TEXT("arrived") && J.Status != TEXT("cancelled")))
            return Fail(J.Id);
        if (!J.Empty)
        {
            const auto *Good = Find(D.Goods, J.GoodId);
            const auto *Dest = Find(W.Warehouses, J.DestinationStorageId);
            if (!Good || !Dest || Dest->CityId != J.ToCityId || J.Kg <= 0 ||
                J.ReservedIncomingPoints != J.Kg * Good->StoragePointsPerKg)
                return Fail(J.Id);
        }
        else if (J.Kg || J.ReservedIncomingPoints)
            return Fail(J.Id);
        const auto Terms = TransportTerms(D, Scenario.Economy, J.FromCityId, J.ToCityId, *T, J.OwnerId == J.ProviderId,
                                          0, J.DepartureStep);
        if (T->Mode!=TEXT("ship")&&(J.RouteKm != Terms.Km || J.OperatingCents != Terms.Operation || J.FreightCents != Terms.Freight ||
            J.CostCents != Terms.Cost || J.ArrivalStep != Terms.Arrival ||
            J.Service != (J.OwnerId == J.ProviderId ? TEXT("own_truck") : TEXT("rented_truck"))))
            return Fail(J.Id);
        if (Active && V->OwnerId != J.ProviderId)
            return Fail(J.Id);
        if (J.Status == TEXT("planned") && (J.DepartureStep < H || V->CityId != J.FromCityId))
            return Fail(J.Id);
        if (J.Status == TEXT("moving") && (J.DepartureStep >= H || J.ArrivalStep < H))
            return Fail(J.Id);
        if (J.Status == TEXT("arrived") && J.ArrivalStep >= H)
            return Fail(J.Id);
        if (J.Status == TEXT("planned") && !J.Empty)
        {
            const auto *I = Find(W.Inventories, J.SourceInventoryId);
            const auto *S = I ? Find(W.Warehouses, I->StorageId) : nullptr;
            if (!I || !S || S->CityId != J.FromCityId || I->OwnerId != J.OwnerId || I->GoodId != J.GoodId)
                return Fail(J.Id);
        }
        if (J.Status == TEXT("moving") && !J.Empty && !Find(W.Inventories, J.CargoInventoryId))
            return Fail(J.Id);
    }
    for (const auto &R : W.Reservations)
    {
        if (R.MoneyCents < 0 || R.Kg < 0 || R.IncomingPoints < 0)
            return Fail(R.Id);
        if (!Actor(R.ActorId))
            return Fail(R.Id);
        if (!R.Active)
            continue;
        if (R.MoneyCents)
        {
            const auto *A = Find(W.Accounts, R.AccountId);
            if (!A || A->OwnerId != R.ActorId)
                return Fail(R.Id);
        }
        if (R.Kg)
        {
            const auto *I = Find(W.Inventories, R.InventoryId);
            if (!I || I->OwnerId != R.ActorId)
                return Fail(R.Id);
        }
        if (R.IncomingPoints)
        {
            const auto *S = Find(W.Warehouses, R.WarehouseId);
            if (!S || S->OwnerId != R.ActorId)
                return Fail(R.Id);
        }
        if (!R.VehicleId.IsEmpty())
        {
            const auto *V = Find(W.Vehicles, R.VehicleId);
            if (!V || V->OwnerId != R.ActorId || V->TransportId != R.Id)
                return Fail(R.Id);
        }
    }
    for (const auto &O : W.Offers)
        if (!Find(W.Companies, O.ProviderId) || !Find(W.Vehicles, O.VehicleId) || !Find(D.Places, O.FromCityId) ||
            !Find(D.Places, O.ToCityId) || O.AvailableKg < 0 || O.FixedCents < 0 || O.CentsPerKg < 0)
            return Fail(O.Id);
    for (const auto &G : W.GoodsAudit)
    {
        int64 Initial = 0;
        for (const auto &I : Scenario.InitialState.Inventories)
            if (I.GoodId == G.Id)
                Initial += I.QuantityKg;
        if (!Find(D.Goods, G.Id) || G.InitialKg != Initial || G.DeveloperAddedKg < 0 || G.ProducedKg < 0 ||
            G.InputKg < 0 || G.ConsumedKg < 0 || G.SpoiledKg < 0 || (!W.DeveloperModified && G.DeveloperAddedKg))
            return Fail(G.Id);
    }
    if (!W.DeveloperModified && (W.DeveloperMoneySources || W.DeveloperMoneySinks))
        return Fail(TEXT("Entwicklerkennzeichnung"));
    for (const auto &R : W.Regions)
        if (!Find(D.Regions, R.Id) || R.LastProcessedStep < 0 || R.LastProcessedStep > W.Step || !Amounts(R.Consumed))
            return Fail(R.Id);
    for (const auto &C : W.Countries)
        if (C.Population < 0 || C.Employed < 0 || !Amounts(C.Consumed))
            return Fail(C.Id);
    for (const auto &C : W.CashFlows)
        if ((!Find(W.Accounts, C.FromAccountId) && !(Scenario.Economy.Bank.Enabled&&C.FromAccountId==BankLedgerId&&(C.Reason==TEXT("loan_disbursement")||C.Reason==TEXT("bank_wages")||C.Reason==TEXT("bank_operating")))) || (!Find(W.Accounts, C.ToAccountId) && !(Scenario.Economy.Bank.Enabled&&C.ToAccountId==BankLedgerId&&(C.Reason==TEXT("loan_principal")||C.Reason==TEXT("loan_interest")))) || C.Cents < 0 || C.Hour < 0 ||
            C.Hour > H)
            return Fail(TEXT("Zahlungsjournal"));
    for (const auto &C : W.CashTotals)
        if (C.Cents < 0)
            return Fail(C.Id);
    for (const auto &M : W.MarketTotals)
    {
        bool Market = false;
        for (const auto &R : D.Regions)
            if (R.MarketId == M.MarketId)
                Market = true;
        if (!Market || !Find(D.Goods, M.GoodId) || M.Id != M.MarketId + TEXT("|") + M.GoodId || M.LastPrice <= 0 ||
            M.LastHour < 0 || M.LastHour > H || M.VolumeKg < 0 || M.DayVolumeKg < 0 || M.Day < 0 || M.Day > H / 24)
            return Fail(M.Id);
    }
    for (const auto &P : W.Projects)
    {
        if (P.StartHour > H || (P.Status == TEXT("building") ? P.FinishHour <= H : P.FinishHour > H) ||
            P.AssetId != (P.TargetId.IsEmpty() ? P.Id + TEXT(".asset") : P.TargetId) ||
            P.StorageId != P.Id + TEXT(".store"))
            return Fail(P.Id);
        if (P.Status == TEXT("building") && P.TargetId.IsEmpty() &&
            (Find(W.Warehouses, P.AssetId) || Find(W.Facilities, P.AssetId) || Find(W.Warehouses, P.StorageId)))
            return Fail(P.Id);
    }
    for (const auto &Sale : W.AssetSales)
        if (Sale.Kind == TEXT("warehouse") ? !Find(W.Warehouses, Sale.AssetId) : !Find(W.Facilities, Sale.AssetId))
            return Fail(Sale.Id);
    auto Metrics = [&](const FDayMetrics &Day) {
        if (!SaveUnique(Day.Rows) || Day.Day < 0 || Day.Day > H / 24 || Day.CompletedStep < 0 || Day.CompletedStep > H)
            return false;
        for (const auto &M : Day.Rows)
            if (!Find(D.Regions, M.RegionId) || !Find(D.Goods, M.GoodId) || M.Id != M.RegionId + TEXT("|") + M.GoodId ||
                M.HouseholdWantedKg < 0 || M.HouseholdUsedKg < 0 || M.InputWantedKg < 0 || M.InputUsedKg < 0 ||
                M.TradeKg < 0 || M.TradeCents < 0)
                return false;
        return true;
    };
    if (!Metrics(W.CurrentDayMetrics) || !Metrics(W.LastDayMetrics) || W.CurrentDayMetrics.Complete ||
        W.CurrentDayMetrics.Day != H / 24 ||
        (W.LastDayMetrics.Complete && (W.LastDayMetrics.Rows.Num() != D.Regions.Num() * D.Goods.Num() ||
                                       W.LastDayMetrics.CompletedStep != (W.LastDayMetrics.Day + 1) * 24)) ||
        W.CurrentDayMetrics.Rows.Num() != D.Regions.Num() * D.Goods.Num() ||
        (H >= 24 && (!W.LastDayMetrics.Complete || W.LastDayMetrics.Day != H / 24 - 1)))
        return Fail(TEXT("Tagesmetriken"));
    for (const auto &P : W.PlayerDays)
        if (P.Day < 0 || P.Day > H / 24 || P.IncomeCents < 0 || P.ExpenseCents < 0 || P.CreditInCents < 0 || P.PrincipalOutCents < 0 || P.InterestOutCents < 0 || !Amounts(P.Spoiled))
            return Fail(P.Id);
    for (const auto &P : W.PlayerEvents)
        if (P.Hour < 0 || P.Hour > H || P.Count <= 0)
            return Fail(P.Id);
    return AuditRuntime(E);
}
} // namespace CoISim

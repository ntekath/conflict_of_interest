#include "SimArchive.h"
#include "SimCore.h"
namespace CoISim
{
namespace
{
bool DeliveryTerminal(const FString &S)
{
    return S == TEXT("missed") || S == TEXT("complete") || S == TEXT("cancelled") || S == TEXT("return_failed");
}
const FVehicle *DeliveryVehicle(const FScenario &S, const FState &W, const FDeliveryTerms &R, const FString &Buyer,
                                bool Available)
{
    const FVehicle *Best = nullptr;
    for (const auto &V : W.Vehicles)
    {
        const auto *C = Find(S.Definitions.Companies, V.OwnerId);
        const auto *T = Find(S.Definitions.VehicleTypes, V.TypeId);
        if (!T ||
            (!R.VehicleId.IsEmpty()
                 ? V.Id != R.VehicleId || V.OwnerId != Buyer
                 : !C || C->AccountingOnly || C->DecisionProfile != TEXT("carrier") || V.OwnerId == Buyer) ||
            T->CapacityKg < R.Kg)
            continue;
        if (Available && (V.Bound || V.ArrearsCents || V.CityId != R.FromCityId))
            continue;
        if (!Best || V.Id < Best->Id)
            Best = &V;
    }
    return Best;
}
} // namespace
FString FSimulation::DeliveryStore(const FString &Asset) const
{
    if (const auto *F = Find(State.Facilities, Asset))
        return F->StorageId;
    return Find(State.Warehouses, Asset) ? Asset : FString();
}
FSiteLedger &FSimulation::SiteLedger(const FString &Actor, const FString &Site)
{
    FString Id = Actor + TEXT("|") + Site;
    auto *L = Find(State.SiteLedgers, Id);
    if (!L)
    {
        FSiteLedger X;
        X.Id = Id;
        X.ActorId = Actor;
        X.SiteId = Site;
        State.SiteLedgers.Add(X);
        L = &State.SiteLedgers.Last();
    }
    return *L;
}
void FSimulation::SitePayment(const FString &Actor, const FString &Site, int64 Cents, bool Revenue)
{
    if (!Find(State.Companies, Actor) || !Cents)
        return;
    auto &L = SiteLedger(Actor, Site);
    if (Revenue)
        L.ExternalRevenueCents += Cents;
    else
        L.ExternalCostCents += Cents;
}
void FSimulation::InternalDelivery(const FDeliveryContract &C, int64 Kg)
{
    if (!Kg)
        return;
    int64 Value = Kg * C.Terms.PriceCentsPerKg;
    SiteLedger(C.BuyerId, C.SourceStoreId).InternalCreditCents += Value;
    SiteLedger(C.BuyerId, C.DestinationStoreId).InternalDebitCents += Value;
    FString Id =
        C.BuyerId + TEXT("|") + C.SourceStoreId + TEXT("|") + C.DestinationStoreId + TEXT("|") + C.Terms.GoodId;
    auto *T = Find(State.InternalTransfers, Id);
    if (!T)
    {
        FInternalTransfer X;
        X.Id = Id;
        X.ActorId = C.BuyerId;
        X.SourceId = C.SourceStoreId;
        X.DestinationId = C.DestinationStoreId;
        X.GoodId = C.Terms.GoodId;
        State.InternalTransfers.Add(X);
        T = &State.InternalTransfers.Last();
    }
    T->Kg += Kg;
    T->ValueCents += Value;
}
bool FSimulation::PreviewDelivery(const FDeliveryRequest &Request, FString &E) const
{
    auto Fail = [&](const FString &S) {
        E = S;
        return false;
    };
    if (Request.Action == TEXT("stop"))
    {
        const auto *C = Find(State.Deliveries, Request.Id);
        if (!C || C->OwnerId != Request.OwnerId || C->Status != TEXT("active"))
            return Fail(TEXT("Nur eine eigene aktive Lieferregel kann gestoppt werden."));
        E = TEXT("Künftige Fälligkeiten stoppen. Bereits gekaufte Ware und gebuchte Hin-/Rückfahrten werden regulär "
                 "abgewickelt; keine Rückabwicklung.");
        return true;
    }
    if (Scenario.Economy.Stage < 4 || Request.Action != TEXT("create") ||
        State.Deliveries.Num() >= Scenario.Economy.DeliveryMaxRules)
        return Fail(TEXT("Lieferaktion ungültig oder Regellimit erreicht."));
    const auto &R = Request.Terms;
    const auto &D = Scenario.Definitions;
    const auto &Q = Scenario.Economy;
    const auto *From = Find(State.Warehouses, DeliveryStore(R.SourceAssetId));
    const auto *To = Find(State.Warehouses, DeliveryStore(R.DestinationAssetId));
    if (!From || !To || From->Id == To->Id || From->CityId != R.FromCityId || To->CityId != R.ToCityId ||
        !Find(D.Goods, R.GoodId) || !Find(State.Companies, Request.OwnerId) ||
        (From->OwnerId != Request.OwnerId && To->OwnerId != Request.OwnerId))
        return Fail(TEXT("Quelle/Ziel müssen reale getrennte Assets mit passenden Stadthubs sein; mindestens eine "
                         "Seite gehört Ihnen."));
    const auto *Supplier = Find(D.Companies, From->OwnerId);
    const auto *Buyer = Find(D.Companies, To->OwnerId);
    if (!Supplier || !Buyer || Supplier->AccountingOnly || Buyer->AccountingOnly)
        return Fail(TEXT("Abrechnungskonten sind keine Liefervertragspartner."));
    if (R.Kg < 1 || R.Kg > Q.DeliveryMaxKg || R.Kg > Q.MaxOrderKg || R.IntervalHours < Q.DeliveryMinIntervalHours ||
        R.IntervalHours > Q.DeliveryMaxIntervalHours || R.StartHour < State.EconomyHour ||
        R.StartHour > State.EconomyHour + Q.DeliveryMaxDurationHours || R.EndHour < R.StartHour ||
        R.EndHour - R.StartHour > Q.DeliveryMaxDurationHours || R.EndHour > 50000000 || R.PriceCentsPerKg < 0 ||
        R.PriceCentsPerKg > Q.MaxPriceCents || R.MaxFreightCents < 0 || R.MaxFreightCents > 1000000000)
        return Fail(TEXT("Menge, Intervall, Beginn/Ende oder Preise außerhalb der Szenariogrenzen."));
    const auto *Vehicle = DeliveryVehicle(Scenario, State, R, Buyer->Id, false);
    if (!Vehicle)
        return Fail(TEXT("Keine passende eigene Fahrzeugzuordnung oder reale mietbare Fahrzeugkapazität."));
    if (!R.VehicleId.IsEmpty() && (Buyer->Id != Request.OwnerId || Vehicle->CityId != From->CityId || Vehicle->Bound))
        return Fail(TEXT(
            "Eigenen Lkw zuerst frei am Quellhub bereitstellen; erforderliche einmalige Leerfahrt regulär planen."));
    const auto *Type = Find(D.VehicleTypes, Vehicle->TypeId);
    auto Out = TransportTerms(D, Q, R.FromCityId, R.ToCityId, *Type, Vehicle->OwnerId == Buyer->Id, State.EconomyHour);
    auto Back = TransportTerms(D, Q, R.ToCityId, R.FromCityId, *Type, Vehicle->OwnerId == Buyer->Id, State.EconomyHour);
    if (R.FromCityId == R.ToCityId)
        Back.Cost = 0;
    if (Out.Cost + Back.Cost > R.MaxFreightCents)
        return Fail(FString::Printf(
            TEXT(
                "Kostenrahmen zu klein: Hin- und Rückfahrt zusammen mindestens %lld Cent für diese Fahrzeugzuordnung."),
            Out.Cost + Back.Cost));
    if (Supplier->Id != Buyer->Id)
    {
        const FString Partner = Supplier->Id == Request.OwnerId ? Buyer->Id : Supplier->Id;
        if (!IsAutonomous(Partner) || R.PriceCentsPerKg < 1)
            return Fail(TEXT("Externer Partner muss autonom und der Festpreis positiv sein."));
        const bool Selling = Buyer->Id == Request.OwnerId;
        int64 Quote = 0, Kg = 0;
        for (const auto &O : State.Orders)
            if (O.Status == TEXT("open") && O.ActorId == Partner && O.GoodId == R.GoodId &&
                O.StorageId == (Selling ? From->Id : To->Id) && O.Side == (Selling ? TEXT("sell") : TEXT("buy")))
            {
                if (!Quote || (Selling ? O.LimitCentsPerKg < Quote : O.LimitCentsPerKg > Quote))
                    Quote = O.LimitCentsPerKg;
            }
        for (const auto &O : State.Orders)
            if (O.Status == TEXT("open") && O.ActorId == Partner && O.GoodId == R.GoodId &&
                O.StorageId == (Selling ? From->Id : To->Id) && O.Side == (Selling ? TEXT("sell") : TEXT("buy")) &&
                O.LimitCentsPerKg == Quote)
                Kg += O.RemainingKg;
        if (!Quote || Kg < R.Kg)
            return Fail(TEXT("Partnerzustimmung fehlt: aktuell muss ein sichtbares passendes Verkaufsangebot/Kaufgebot "
                             "mindestens die Intervallmenge decken."));
        if (Selling ? R.PriceCentsPerKg < Quote : R.PriceCentsPerKg * R.Kg + Out.Cost + Back.Cost > Quote * R.Kg)
            return Fail(FString::Printf(TEXT("Partner lehnt ab: öffentliches Limit %lld Cent/kg; bei fremdem Käufer "
                                             "zählt Festpreis plus gedeckte Hin-/Rückfracht."),
                                        Quote));
    }
    E = FString::Printf(TEXT("%s: %lld kg alle %lld h von %s nach %s. Fester Waren-/Verrechnungspreis %lld Cent/kg; "
                             "Transportkäufer ist der Zielinhaber. Hin-/Rückfahrt voraussichtlich %lld Cent, "
                             "Kostenrahmen %lld Cent je Fälligkeit. Fehlmengen verfallen je Termin; keine "
                             "Strafzahlung. Bereits gebuchte Fahrten laufen auch nach Stopp/Ende weiter."),
                        Supplier->Id == Buyer->Id
                            ? TEXT("Interne Versorgung ohne Warenzahlung")
                            : TEXT("Externer Vertrag mit Partnerzustimmung nach öffentlichem Angebot"),
                        R.Kg, R.IntervalHours, *Find(D.Places, R.FromCityId)->Name, *Find(D.Places, R.ToCityId)->Name,
                        R.PriceCentsPerKg, Out.Cost + Back.Cost, R.MaxFreightCents);
    return true;
}
bool FSimulation::ChangeDelivery(const FDeliveryRequest &R, FString &E)
{
    if (!PreviewDelivery(R, E))
        return false;
    if (R.Action == TEXT("stop"))
    {
        auto *C = Find(State.Deliveries, R.Id);
        C->Status = TEXT("stopped");
        C->Cause = TEXT("Vom Auftraggeber gestoppt; gebuchte Fahrten bleiben verbindlich.");
        return true;
    }
    if (R.Id.IsEmpty() || R.Id.Len() > 80 || Find(State.Deliveries, R.Id))
    {
        E = TEXT("Ungültige oder doppelte Lieferregel-ID.");
        return false;
    }
    FDeliveryContract C;
    C.Id = R.Id;
    C.OwnerId = R.OwnerId;
    C.Terms = R.Terms;
    C.SourceStoreId = DeliveryStore(R.Terms.SourceAssetId);
    C.DestinationStoreId = DeliveryStore(R.Terms.DestinationAssetId);
    C.SupplierId = Find(State.Warehouses, C.SourceStoreId)->OwnerId;
    C.BuyerId = Find(State.Warehouses, C.DestinationStoreId)->OwnerId;
    C.Status = TEXT("active");
    C.AcceptedHour = State.EconomyHour;
    C.NextDueHour = R.Terms.StartHour;
    C.Cause = TEXT("Angenommen; nächste Fälligkeit vorgemerkt.");
    if (C.SupplierId != C.BuyerId)
    {
        bool Selling = C.BuyerId == C.OwnerId;
        for (const auto &O : State.Orders)
            if (O.Status == TEXT("open") && O.ActorId == (Selling ? C.SupplierId : C.BuyerId) &&
                O.GoodId == C.Terms.GoodId && O.StorageId == (Selling ? C.SourceStoreId : C.DestinationStoreId) &&
                O.Side == (Selling ? TEXT("sell") : TEXT("buy")) &&
                (!C.AcceptedUnitCents ||
                 (Selling ? O.LimitCentsPerKg < C.AcceptedUnitCents : O.LimitCentsPerKg > C.AcceptedUnitCents)))
                C.AcceptedUnitCents = O.LimitCentsPerKg;
    }
    State.Deliveries.Add(C);
    for (auto &O : State.Orders)
        if (O.Status == TEXT("open") && !O.RuleId.IsEmpty() && O.ActorId == C.BuyerId &&
            O.StorageId == C.DestinationStoreId && O.GoodId == C.Terms.GoodId && O.Side == TEXT("buy"))
            CancelOrder(O.Id, C.BuyerId, TEXT("rule_changed"));
    for (auto &X : State.OperatingStatus)
        if (const auto *Rule = Find(State.OperatingRules, X.Id))
        {
            const auto *F = Find(State.Facilities, Rule->Id);
            if (Rule->OwnerId == C.BuyerId && (F ? F->StorageId : Rule->Id) == C.DestinationStoreId)
                X.NextDecisionHour = State.EconomyHour;
        }
    return true;
}
int64 FSimulation::DeliveryPromises(const FString &Buyer, const FString &Store, const FString &Good) const
{
    int64 Kg = 0;
    for (const auto &C : State.Deliveries)
        if (C.Status == TEXT("active") && C.BuyerId == Buyer && C.DestinationStoreId == Store &&
            C.Terms.GoodId == Good && C.NextDueHour <= State.EconomyHour + Scenario.Economy.DeliveryPromiseHours)
        {
            if (!C.Terms.MarketFallback || !C.LastMissedKg)
                Kg += C.Terms.Kg;
        }
    return Kg;
}
bool FSimulation::DeliveryTransportReferenced(const FString &Id) const
{
    for (const auto &X : State.DeliverySlots)
        if (!DeliveryTerminal(X.Status) && (X.OutboundId == Id || X.ReturnId == Id))
            return true;
    return false;
}
void FSimulation::RunDeliveryDue(const FString &ContractId, int64 H)
{
    auto *Initial = Find(State.Deliveries, ContractId);
    const FDeliveryContract Contract = *Initial;
    const auto R = Contract.Terms;
    FDeliverySlot X;
    X.Id = Contract.Id + TEXT(".at.") + LexToString(H);
    X.ContractId = Contract.Id;
    X.DueHour = H;
    X.RequestedKg = R.Kg;
    X.Status = TEXT("missed");
    State.DeliverySlots.Add(X);
    Initial->RequestedKg += R.Kg;
    Initial->NextDueHour += R.IntervalHours;
    if (Initial->NextDueHour > R.EndHour)
        Initial->Status = TEXT("expired");
    const FState Backup = State;
    FString Error;
    int64 Booked = 0;
    auto Attempt = [&]() {
        const auto *V = DeliveryVehicle(Scenario, State, R, Contract.BuyerId, true);
        if (!V)
        {
            Error = TEXT("Fälligkeit ausgefallen: passendes Fahrzeug am Quellhub fehlt/ist gebunden; kein kostenloser "
                         "Ortswechsel.");
            return false;
        }
        const auto *Type = Find(Scenario.Definitions.VehicleTypes, V->TypeId);
        const auto Out = TransportTerms(Scenario.Definitions, Scenario.Economy, R.FromCityId, R.ToCityId, *Type,
                                        V->OwnerId == Contract.BuyerId, H);
        auto Back = TransportTerms(Scenario.Definitions, Scenario.Economy, R.ToCityId, R.FromCityId, *Type,
                                   V->OwnerId == Contract.BuyerId, H);
        if (R.FromCityId == R.ToCityId)
            Back.Cost = 0;
        if (Out.Cost + Back.Cost > R.MaxFreightCents)
        {
            Error = TEXT("Fälligkeit ausgefallen: Kostenrahmen für Hin-/Rückfahrt überschritten.");
            return false;
        }
        for (auto &O : State.Orders)
            if (O.Status == TEXT("open") && !O.RuleId.IsEmpty() && O.Side == TEXT("sell") &&
                O.ActorId == Contract.SupplierId && O.StorageId == Contract.SourceStoreId && O.GoodId == R.GoodId)
                CancelOrder(O.Id, Contract.SupplierId, TEXT("rule_changed"));
        int32 Source = StockIndex(Contract.SupplierId, Contract.SourceStoreId, R.GoodId);
        if (Source == INDEX_NONE)
        {
            Error = TEXT("Fälligkeit ausgefallen: eigene Quellware fehlt.");
            return false;
        }
        int64 Minimum = 0;
        for (const auto &Rule : State.OperatingRules)
            if (OperatingEnabled(Rule) && Rule.OwnerId == Contract.SupplierId)
            {
                const auto *F = Find(State.Facilities, Rule.Id);
                if ((F ? F->StorageId : Rule.Id) == Contract.SourceStoreId)
                    for (const auto &G : Rule.Goods)
                        if (G.GoodId == R.GoodId)
                            Minimum = FMath::Max(Minimum, G.MinimumKg);
            }
        const auto *Dest = Find(State.Warehouses, Contract.DestinationStoreId);
        const auto *Good = Find(Scenario.Definitions.Goods, R.GoodId);
        int64 Space = Find(Scenario.Definitions.WarehouseTypes, Dest->TypeId)->CapacityPoints - UsedStorage(Dest->Id) -
                      Dest->ReservedIncomingPoints;
        auto *Account = Find(State.Accounts, ActorAccount(Contract.BuyerId));
        int64 Free = Account->BalanceCents - Account->ReservedCents - Out.Cost - Back.Cost;
        int64 Goal = -1;
        for (const auto &Rule : State.OperatingRules)
            if (OperatingEnabled(Rule) && Rule.OwnerId == Contract.BuyerId)
            {
                const auto *F = Find(State.Facilities, Rule.Id);
                if ((F ? F->StorageId : Rule.Id) == Contract.DestinationStoreId)
                    for (const auto &G : Rule.Goods)
                        if (G.GoodId == R.GoodId && G.BuyLimitCents > 0)
                            Goal = FMath::Max(Goal, G.TargetKg);
            }
        int64 GoalNeed = R.Kg;
        if (Goal >= 0)
        {
            int32 Existing = StockIndex(Contract.BuyerId, Contract.DestinationStoreId, R.GoodId);
            int64 Have = Existing == INDEX_NONE
                             ? 0
                             : State.Inventories[Existing].QuantityKg - State.Inventories[Existing].ReservedKg;
            for (const auto &O : State.Orders)
                if (O.Status == TEXT("open") && O.ActorId == Contract.BuyerId &&
                    O.StorageId == Contract.DestinationStoreId && O.GoodId == R.GoodId && O.Side == TEXT("buy"))
                    Have += O.RemainingKg;
            for (const auto &J : State.Transports)
                if ((J.Status == TEXT("planned") || J.Status == TEXT("moving")) && J.OwnerId == Contract.BuyerId &&
                    J.DestinationStorageId == Contract.DestinationStoreId && J.GoodId == R.GoodId)
                {
                    const auto *Cargo = Find(State.Inventories, J.CargoInventoryId);
                    Have += J.Status == TEXT("moving") && Cargo ? Cargo->QuantityKg : J.Kg;
                }
            GoalNeed = FMath::Max(0LL, Goal - Have);
        }
        if (!GoalNeed)
        {
            Error = TEXT(
                "Termin ohne Zusatzbeschaffung: Ziel bereits durch Bestand, Kaufaufträge oder Lieferungen gedeckt.");
            return false;
        }
        int64 Qty =
            FMath::Min(FMath::Min(R.Kg, GoalNeed), FMath::Max(0LL, State.Inventories[Source].QuantityKg -
                                                                       State.Inventories[Source].ReservedKg - Minimum));
        Qty = FMath::Min(Qty, FMath::Max(0LL, Space) / Good->StoragePointsPerKg);
        if (Free < 0)
            Qty = 0;
        else if (Contract.SupplierId != Contract.BuyerId)
            Qty = FMath::Min(Qty, Free / FMath::Max(1LL, R.PriceCentsPerKg));
        if(Contract.Id.StartsWith(TEXT("retail.supply."))){const auto* Rule=Find(State.OperatingRules,Contract.DestinationStoreId);const auto* Status=Find(State.OperatingStatus,Contract.DestinationStoreId);int64 Spent=Status&&Status->BudgetWindow==H/Scenario.Economy.RuleBudgetHours?Status->SpentCents:0,Held=0;for(const auto& O:State.Orders)if(O.Status==TEXT("open")&&O.Side==TEXT("buy")&&O.RuleId==Contract.DestinationStoreId)Held+=O.ReservedCents;Qty=Rule&&Rule->Enabled?(Contract.SupplierId==Contract.BuyerId?Qty:FMath::Min(Qty,FMath::Max(0LL,Rule->BuyBudgetCents-Spent-Held)/FMath::Max(1LL,R.PriceCentsPerKg))):0;}
        if (Qty <= 0)
        {
            Error = TEXT("Fälligkeit ausgefallen: verfügbare Quellware, freies Zielvolumen oder gedecktes "
                         "Kauf-/Frachtkapital fehlen.");
            return false;
        }
        if (Contract.SupplierId != Contract.BuyerId && Contract.BuyerId != Contract.OwnerId &&
            Qty * R.PriceCentsPerKg + Out.Cost + Back.Cost > Qty * Contract.AcceptedUnitCents)
        {
            Error =
                TEXT("Fälligkeit ausgefallen: Teilladung überschreitet das zugesagte Gesamtpreislimit des Käufers.");
            return false;
        }
        auto *Slot = Find(State.DeliverySlots, X.Id);
        Slot->ReturnHoldCents = Back.Cost;
        Slot->ReturnCostCents = Back.Cost;
        Account->ReservedCents += Back.Cost;
        int32 CargoSource = Source;
        if (Contract.SupplierId != Contract.BuyerId)
        {
            int64 Price = Qty * R.PriceCentsPerKg;
            if (!Pay(Account->Id, ActorAccount(Contract.SupplierId), Price, TEXT("delivery_purchase"),
                     Contract.DestinationStoreId, Contract.SourceStoreId))
            {
                Error = TEXT("Warenzahlung nicht gedeckt.");
                return false;
            }
            int64 Credit = TakeSpoilCredit(Source, Qty);
            TakeStock(Source, Qty);
            CargoSource = EnsureStock(Contract.BuyerId, Contract.SourceStoreId, R.GoodId);
            if (State.Inventories[CargoSource].QuantityKg > 1000000000 - Qty)
            {
                Error = TEXT("Bestandsgrenze überschritten.");
                return false;
            }
            auto &Stock = State.Inventories[CargoSource];
            if (!Stock.QuantityKg)
                Stock.AcquiredHour = H;
            Stock.QuantityKg += Qty;
            Stock.BookValueCents += Price;
            Stock.SpoilRemainder += Credit;
            Slot->PurchaseCents = Price;
            if(Contract.Id.StartsWith(TEXT("retail.supply."))){auto* Status=Find(State.OperatingStatus,Contract.DestinationStoreId);if(Status->BudgetWindow!=H/Scenario.Economy.RuleBudgetHours){Status->BudgetWindow=H/Scenario.Economy.RuleBudgetHours;Status->SpentCents=0;}Status->SpentCents+=Price;}

        }
        FTransport Job;
        Job.Id = X.Id + TEXT(".out");
        Job.DeliveryId = X.Id;
        Job.ReportSiteId = Contract.DestinationStoreId;
        Job.OwnerId = Contract.BuyerId;
        Job.VehicleId = V->Id;
        Job.FromCityId = R.FromCityId;
        Job.ToCityId = R.ToCityId;
        Job.SourceInventoryId = State.Inventories[CargoSource].Id;
        Job.DestinationStorageId = Contract.DestinationStoreId;
        Job.Kg = Qty;
        if (!BookTransport(Job, Error, true))
            return false;
        Slot = Find(State.DeliverySlots, X.Id);
        Slot->OutboundId = Job.Id;
        Slot->BookedKg = Qty;
        Slot->MissedKg = R.Kg - Qty;
        Slot->Status = TEXT("outbound");
        Slot->Cause = Qty < R.Kg ? TEXT("Teillieferung gebucht; fehlende Menge verfällt für diesen Termin.")
                                 : TEXT("Lieferung gebucht; Waren- und Kostenreservierungen gedeckt.");
        Booked = Qty;
        return true;
    };
    bool OK = Attempt();
    if (!OK)
    {
        State = Backup;
        auto *Slot = Find(State.DeliverySlots, X.Id);
        Slot->MissedKg = R.Kg;
        Slot->Cause = Error.Left(500);
    }
    auto *C = Find(State.Deliveries, ContractId);
    C->BookedKg += Booked;
    C->MissedKg += R.Kg - Booked;
    C->LastMissedKg = R.Kg - Booked;
    C->Cause = Find(State.DeliverySlots, X.Id)->Cause;
    if (C->OwnerId == Session.PlayerCompanyId)
        PlayerEvent(TEXT("delivery"), C->Id, C->Cause);
}
void FSimulation::ProgressDelivery(const FString &SlotId, int64 H)
{
    auto *X = Find(State.DeliverySlots, SlotId);
    if (!X || DeliveryTerminal(X->Status))
        return;
    const auto C = *Find(State.Deliveries, X->ContractId);
    const auto *Out = Find(State.Transports, X->OutboundId);
    if (!Out)
        return;
    if (!X->Accounted)
    {
        if (Out->Status == TEXT("cancelled"))
        {
            Find(State.Accounts, ActorAccount(C.BuyerId))->ReservedCents -= X->ReturnHoldCents;
            X->ReturnHoldCents = 0;
            X->MissedKg = X->RequestedKg;
            auto *Actual = Find(State.Deliveries, C.Id);
            Actual->MissedKg += X->BookedKg;
            if (X->DueHour == Actual->NextDueHour - Actual->Terms.IntervalHours)
                Actual->LastMissedKg = X->RequestedKg;
            X->Status = TEXT("cancelled");
            X->Cause = TEXT("Hinfahrt storniert; bereits gekaufte Ware bleibt beim Käufer am Quelllager. Keine erneute "
                            "Fälligkeit.");
            return;
        }
        if (Out->Status != TEXT("arrived"))
            return;
        X->DeliveredKg = Out->DeliveredKg;
        X->MissedKg = X->RequestedKg - X->DeliveredKg;
        X->ReturnDeadlineHour = Out->ActualArrivalHour + Scenario.Economy.DeliveryReturnGraceHours;
        X->Accounted = true;
        if (C.SupplierId == C.BuyerId)
        {
            X->InternalValueCents = X->DeliveredKg * C.Terms.PriceCentsPerKg;
            InternalDelivery(C, X->DeliveredKg);
        }
        auto *Actual = Find(State.Deliveries, C.Id);
        Actual->DeliveredKg += X->DeliveredKg;
        Actual->MissedKg += X->BookedKg - X->DeliveredKg;
        if (X->DueHour == Actual->NextDueHour - Actual->Terms.IntervalHours)
            Actual->LastMissedKg = X->MissedKg;
        X->Status = TEXT("return_wait");
        X->Cause = TEXT("Tatsächliche Menge genau einmal zugestellt; Rückfahrt wird gedeckt gebucht.");
        if (C.OwnerId == Session.PlayerCompanyId)
            PlayerEvent(TEXT("delivery"), C.Id, X->Cause);
    }
    X = Find(State.DeliverySlots, SlotId);
    if (X->Status == TEXT("returning"))
    {
        const auto *Back = Find(State.Transports, X->ReturnId);
        if (Back && (Back->Status == TEXT("arrived") || Back->Status == TEXT("cancelled")))
        {
            X->Status = Back->Status == TEXT("arrived") ? TEXT("complete") : TEXT("return_failed");
            X->Cause = Back->Status == TEXT("arrived")
                           ? TEXT("Hin- und Rückfahrt abgeschlossen. Fehlmengen werden nicht nachgeholt.")
                           : TEXT("Rückfahrt storniert; Fahrzeug bleibt am realen Standort.");
        }
        return;
    }
    if (X->Status != TEXT("return_wait"))
        return;
    if (H > X->ReturnDeadlineHour)
    {
        Find(State.Accounts, ActorAccount(C.BuyerId))->ReservedCents -= X->ReturnHoldCents;
        X->ReturnHoldCents = 0;
        X->Status = TEXT("return_failed");
        X->Cause = TEXT("Rückfahrtfrist abgelaufen; Kostenhold freigegeben. Fahrzeug bleibt am realen Standort.");
        if (C.OwnerId == Session.PlayerCompanyId)
            PlayerEvent(TEXT("delivery"), C.Id, X->Cause);
        return;
    }
    const auto VehicleId = Out->VehicleId;
    const auto Provider = Out->ProviderId;
    const FState Backup = State;
    const int64 Cost = X->ReturnHoldCents;
    Find(State.Accounts, ActorAccount(C.BuyerId))->ReservedCents -= Cost;
    X->ReturnHoldCents = 0;
    bool OK = true;
    if (Provider != C.BuyerId)
        OK = Pay(ActorAccount(C.BuyerId), ActorAccount(Provider), Cost, TEXT("delivery_return_freight"),
                 C.DestinationStoreId, VehicleId);
    FString Error;
    FTransport Back;
    Back.Id = SlotId + TEXT(".back");
    Back.DeliveryId = SlotId;
    Back.ReportSiteId = Provider == C.BuyerId ? C.DestinationStoreId : VehicleId;
    Back.OwnerId = Provider;
    Back.VehicleId = VehicleId;
    Back.FromCityId = C.Terms.ToCityId;
    Back.ToCityId = C.Terms.FromCityId;
    Back.Empty = true;
    // A same-hub delivery still consumes a real local route, but cannot use the ordinary cross-hub empty reposition.
    if (C.Terms.FromCityId == C.Terms.ToCityId)
    {
        State = Backup;
        X = Find(State.DeliverySlots, SlotId);
        Find(State.Accounts, ActorAccount(C.BuyerId))->ReservedCents -= X->ReturnHoldCents;
        X->ReturnHoldCents = 0;
        X->Status = TEXT("complete");
        X->Cause = TEXT("Lokale Lieferung abgeschlossen; Fahrzeug steht bereits am Quellhub, keine Leerfahrt nötig.");
        return;
    }
    if (OK)
        OK = BookTransport(Back, Error, true);
    if (!OK)
    {
        State = Backup;
        Find(State.DeliverySlots, SlotId)->Cause = TEXT("Rückfahrt wartet: ") + Error.Left(450);
        return;
    }
    X = Find(State.DeliverySlots, SlotId);
    X->ReturnId = Back.Id;
    X->Status = TEXT("returning");
    X->Cause = TEXT("Bezahlte Rückfahrt gebucht; Fahrzeug wird nicht teleportiert.");
}
void FSimulation::DeliveryService(int64 H)
{
    TArray<FString> Slots;
    for (const auto &X : State.DeliverySlots)
        if (!DeliveryTerminal(X.Status))
            Slots.Add(X.Id);
    for (const auto &Id : Slots)
        ProgressDelivery(Id, H);
    TArray<FString> Due;
    for (const auto &C : State.Deliveries)
        if (C.Status == TEXT("active") && C.NextDueHour <= H && C.NextDueHour <= C.Terms.EndHour)
            Due.Add(C.Id);
    for (const auto &Id : Due)
        RunDeliveryDue(Id, H);
    int32 Closed = 0;
    for (const auto &X : State.DeliverySlots)
        if (DeliveryTerminal(X.Status))
            ++Closed;
    for (int32 I = 0; Closed > Scenario.Economy.HistoryLimit && I < State.DeliverySlots.Num();)
        if (DeliveryTerminal(State.DeliverySlots[I].Status))
        {
            State.DeliverySlots.RemoveAt(I);
            --Closed;
        }
        else
            ++I;
}
} // namespace CoISim

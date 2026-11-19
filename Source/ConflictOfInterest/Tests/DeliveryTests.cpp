#if WITH_DEV_AUTOMATION_TESTS
#include "GeoCatalog.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Simulation/SaveGame.h"
#include "Simulation/SimArchive.h"
using namespace CoISim;
namespace
{
bool DlvFixture(FString &J, FCoIGeoMapData &G, TFunction<void(TSharedPtr<FJsonObject>)> Edit = {})
{
    FString E;
    auto *C = LoadObject<UCoIGeoCatalog>(nullptr, TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));
    if (!C || !G.Load(C->DefinitionJson, E) ||
        !FFileHelper::LoadFileToString(J, *(FPaths::ProjectContentDir() / TEXT("Simulation/scenario.coisim"))))
        return false;
    TSharedPtr<FJsonObject> R;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(J), R);if(R->HasField(TEXT("Economy")))R->GetObjectField(TEXT("Economy"))->SetBoolField(TEXT("HouseholdModel"),false);
    if (Edit)
        Edit(R);
    J.Empty();
    FJsonSerializer::Serialize(R.ToSharedRef(), TJsonWriterFactory<>::Create(&J));
    return true;
}
FCommand DlvCommand(FSimulation &S, ECommandKind K)
{
    FCommand C;
    C.Kind = K;
    C.Generation = S.GetGeneration();
    C.Sequence = S.NextSequence();
    C.TargetStep = S.GetState().Step + 1;
    return C;
}
FDeliveryRequest DlvRule(const TCHAR *Id, const TCHAR *From, const TCHAR *To, const TCHAR *CityFrom,
                         const TCHAR *CityTo)
{
    FDeliveryRequest R;
    R.Id = Id;
    R.OwnerId = TEXT("company.player");
    R.Action = TEXT("create");
    R.Terms.SourceAssetId = From;
    R.Terms.DestinationAssetId = To;
    R.Terms.FromCityId = CityFrom;
    R.Terms.ToCityId = CityTo;
    R.Terms.GoodId = TEXT("good.food");
    R.Terms.Kg = 25;
    R.Terms.IntervalHours = 24;
    R.Terms.StartHour = 1;
    R.Terms.EndHour = 73;
    R.Terms.PriceCentsPerKg = 1;
    R.Terms.MaxFreightCents = 100000;
    return R;
}
bool DlvQueue(FSimulation &S, const FDeliveryRequest &R, FString &E)
{
    auto C = DlvCommand(S, ECommandKind::Delivery);
    C.Delivery = R;
    C.CompanyId = R.OwnerId;
    C.Origin = ECommandOrigin::Player;
    return S.Enqueue(C, E);
}
void OwnCleveland(TSharedPtr<FJsonObject> Root)
{
    for (auto X : Root->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Warehouses")))
        if (X->AsObject()->GetStringField(TEXT("Id")) == TEXT("storage.service.cleveland"))
            X->AsObject()->SetStringField(TEXT("OwnerId"), TEXT("company.player"));
}
template <class T> FString DlvHash(const T &X)
{
    FFingerprint H;
    H.Write(X);
    return H.Finish();
}
FString DlvRepack(FSaveData &D)
{
    FWrite W;
    D.Fields(W);
    auto R = MakeShared<FJsonObject>();
    R->SetStringField(TEXT("Format"), TEXT("CoI.Save"));
    R->SetStringField(TEXT("Checksum"), DlvHash(D));
    R->SetObjectField(TEXT("Payload"), W.Object);
    FString J;
    FJsonSerializer::Serialize(R, TJsonWriterFactory<>::Create(&J));
    return J;
}
FSaveData DlvUnpack(const FString &J)
{
    TSharedPtr<FJsonObject> R;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(J), R);if(R->HasField(TEXT("Economy")))R->GetObjectField(TEXT("Economy"))->SetBoolField(TEXT("HouseholdModel"),false);
    FSaveData D;
    FString E;
    FRead Reader{R->GetObjectField(TEXT("Payload")), E, true};
    D.Fields(Reader);
    return D;
}
} // namespace
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeliveryInternal, "CoI.Delivery.InternalRecurrenceReturnsValuationAndReplay",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDeliveryInternal::RunTest(const FString &)
{
    FString J, E, Save;
    FCoIGeoMapData G;
    if (!DlvFixture(J, G, OwnCleveland))
        return false;
    FSimulation A, B, Replay;
    if (!A.Load(J, G, E) || !B.Load(J, G, E))
        return false;
    auto R = DlvRule(TEXT("dlv.internal"), TEXT("storage.player"), TEXT("storage.service.cleveland"),
                     TEXT("city.us.detroit"), TEXT("city.us.cleveland"));
    R.Terms.VehicleId = TEXT("vehicle.player");
    auto Before = A.Fingerprint();
    TestTrue(TEXT("Read-only preview"), A.PreviewDelivery(R, E));
    TestEqual(TEXT("Preview creates no obligations"), A.Fingerprint(), Before);
    DlvQueue(A, R, E);
    R.Terms.PriceCentsPerKg = 1000;
    DlvQueue(B, R, E);
    bool SavedOutbound = false, SavedReturn = false;
    for (int H = 0; H < 120; ++H)
    {
        A.StepOnce();
        B.StepOnce();
        if (Replay.IsLoaded())
        {
            Replay.StepOnce();
            if (A.Fingerprint() != Replay.Fingerprint())
            {
                AddError(TEXT("Delivery replay diverged"));
                return false;
            }
        }
        if (!A.ValidateSaveState(E))
        {
            AddError(E);
            return false;
        }
        TestEqual(TEXT("Internal price cannot create company cash"), DlvHash(A.GetState().Accounts),
                  DlvHash(B.GetState().Accounts));
        TestEqual(TEXT("Internal markup cannot inflate actual inventory cost basis"), DlvHash(A.GetState().Inventories),
                  DlvHash(B.GetState().Inventories));
        for (const auto &X : A.GetState().DeliverySlots)
        {
            const auto *Out = Find(A.GetState().Transports, X.OutboundId);
            bool Moving = Out && Out->Status == TEXT("moving");
            if ((!SavedOutbound && Moving) || (!SavedReturn && X.Status == TEXT("returning")))
            {
                if (!A.SaveGameJson(Save, E))
                {
                    AddError(E);
                    return false;
                }
                int32 Speed = 1;
                if (!Replay.LoadGameJson(Save, G, A.GetScenario(), E, Speed))
                {
                    AddError(E);
                    return false;
                }
                if (Moving)
                    SavedOutbound = true;
                if (X.Status == TEXT("returning"))
                    SavedReturn = true;
            }
        }
    }
    TestTrue(TEXT("Save during actual cargo movement"), SavedOutbound);
    TestTrue(TEXT("Save after delivery accounting and during return"), SavedReturn);
    const auto *C = Find(A.GetState().Deliveries, R.Id);
    if (!TestNotNull(TEXT("Standing contract retained"), C))
        return false;
    TestEqual(TEXT("Exactly four due slots"), C->RequestedKg, 100LL);
    TestTrue(TEXT("Multiple real deliveries"), C->DeliveredKg > 50);
    TestTrue(TEXT("Truck completes a paid return to source"),
             Find(A.GetState().Vehicles, FString(TEXT("vehicle.player")))->CityId == TEXT("city.us.detroit"));
    int64 CreditsA = 0, DebitsA = 0, CreditsB = 0, Revenue = 0;
    for (const auto &L : A.GetState().SiteLedgers)
        if (L.ActorId == TEXT("company.player"))
        {
            CreditsA += L.InternalCreditCents;
            DebitsA += L.InternalDebitCents;
            Revenue += L.ExternalRevenueCents;
        }
    for (const auto &L : B.GetState().SiteLedgers)
        if (L.ActorId == TEXT("company.player"))
            CreditsB += L.InternalCreditCents;
    TestTrue(TEXT("Nonzero site valuation"), CreditsA > 0);
    TestEqual(TEXT("Paired site credit and debit"), CreditsA, DebitsA);
    TestEqual(TEXT("Internal price affects site valuation only"), CreditsB, CreditsA * 1000);
    TestEqual(TEXT("Internal delivery is not external revenue"), Revenue, 0LL);
    TestTrue(TEXT("Conservation and exactly-once accounting"), A.AuditRuntime(E));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeliveryExternal, "CoI.Delivery.ExternalConsentPaymentAndFundedRentalReturn",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDeliveryExternal::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    auto Fixture = [](auto Root) {
        Root->GetObjectField(TEXT("Economy"))->SetNumberField(TEXT("DailyBudgetPermille"), 0);
        for (auto C : Root->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("Companies")))
            if (C->AsObject()->GetStringField(TEXT("Id")) == TEXT("company.merchant"))
                C->AsObject()->SetStringField(TEXT("DecisionProfile"), TEXT("service"));
        for (auto V : Root->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Vehicles")))
            if (V->AsObject()->GetStringField(TEXT("OwnerId")) == TEXT("company.carrier"))
                V->AsObject()->SetStringField(TEXT("CityId"), TEXT("city.us.chicago"));
        for (auto O : Root->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Offers")))
        {
            O->AsObject()->SetStringField(TEXT("FromCityId"), TEXT("city.us.chicago"));
            O->AsObject()->SetStringField(TEXT("ToCityId"), TEXT("city.us.detroit"));
        }
    };
    DlvFixture(J, G, Fixture);
    FSimulation S;
    if (!S.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    auto R = DlvRule(TEXT("dlv.external"), TEXT("storage.food"), TEXT("storage.player"), TEXT("city.us.chicago"),
                     TEXT("city.us.detroit"));
    R.Terms.Kg = 50;
    R.Terms.PriceCentsPerKg = 100;
    R.Terms.EndHour = 1;
    TestFalse(TEXT("No fabricated partner assent before public offers"), S.PreviewDelivery(R, E));
    S.StepOnce();
    auto Hash = S.Fingerprint();
    if (!S.PreviewDelivery(R, E))
    {
        AddError(E);
        return false;
    }
    TestTrue(TEXT("Visible supplier offer grants bounded assent"), true);
    TestEqual(TEXT("Assent preview is read-only"), S.Fingerprint(), Hash);
    DlvQueue(S, R, E);
    S.StepOnce();
    const auto *X = Find(S.GetState().DeliverySlots, FString(TEXT("dlv.external.at.1")));
    if (!TestNotNull(TEXT("One interval recorded"), X))
        return false;
    TestEqual(TEXT("Real fixed-price purchase"), X->PurchaseCents, 5000LL);
    TestEqual(TEXT("Seller receives exactly booked goods payment"),
              Find(S.GetState().CashTotals, FString(TEXT("delivery_purchase")))->Cents, 5000LL);
    TestTrue(TEXT("Return fee covered before cargo departure"), X->ReturnHoldCents > 0);
    TestEqual(TEXT("Goods are not instantly delivered"), X->DeliveredKg, 0LL);
    for (int H = 0; H < 72; ++H)
    {
        S.StepOnce();
        if (!S.ValidateSaveState(E))
        {
            AddError(E);
            return false;
        }
    }
    X = Find(S.GetState().DeliverySlots, FString(TEXT("dlv.external.at.1")));
    TestEqual(TEXT("Finite rented vehicle returns physically"), X->Status, FString(TEXT("complete")));
    TestTrue(TEXT("Real return freight booked"),
             Find(S.GetState().CashTotals, FString(TEXT("delivery_return_freight")))->Cents > 0);
    TestEqual(TEXT("No double invoice after arrival/return"),
              Find(S.GetState().CashTotals, FString(TEXT("delivery_purchase")))->Cents, 5000LL);
    TestTrue(TEXT("No unbacked account"), S.AuditRuntime(E));
    FString Poor;
    DlvFixture(Poor, G, [&](auto Root) {
        Fixture(Root);
        for (auto A : Root->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Accounts")))
            if (A->AsObject()->GetStringField(TEXT("OwnerId")) == TEXT("company.player"))
                A->AsObject()->SetNumberField(TEXT("BalanceCents"), 0);
    });
    FSimulation P;
    if (!P.Load(Poor, G, E))
    {
        AddError(E);
        return false;
    }
    P.StepOnce();
    R.Id = TEXT("dlv.poor");
    DlvQueue(P, R, E);
    const auto Seller = Find(P.GetState().Accounts, FString(TEXT("account.company.food")))->BalanceCents;
    P.StepOnce();
    TestNull(TEXT("Unfunded buyer produces no invoice"),
             Find(P.GetState().CashTotals, FString(TEXT("delivery_purchase"))));
    TestEqual(TEXT("No phantom seller receipt"),
              Find(P.GetState().Accounts, FString(TEXT("account.company.food")))->BalanceCents, Seller);
    TestEqual(TEXT("One failed interval, no hidden retry"), Find(P.GetState().Deliveries, R.Id)->MissedKg, 50LL);
    TestTrue(TEXT("Failure conserves all money/goods"), P.AuditRuntime(E));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeliveryFallback, "CoI.Delivery.PromiseMarketCoordinationAndMissedSlotPolicy",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDeliveryFallback::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    DlvFixture(J, G, [](auto Root) {
        OwnCleveland(Root);
        for (auto I : Root->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Inventories")))
            if (I->AsObject()->GetStringField(TEXT("Id")) == TEXT("stock.player.food"))
            {
                I->AsObject()->SetStringField(TEXT("GoodId"), TEXT("good.wood"));
                I->AsObject()->SetNumberField(TEXT("QuantityKg"), 0);
            }
    });
    FSimulation A, B;
    for (auto *S : {&A, &B})
    {
        if (!S->Load(J, G, E))
            return false;
        auto Op = *Find(S->GetState().OperatingRules, FString(TEXT("storage.service.cleveland")));
        Op.Explicit = Op.Enabled = true;
        for (auto &X : Op.Goods)
            if (X.GoodId == TEXT("good.wood"))
            {
                X.TargetKg = 50;
                X.BuyLimitCents = 10;
            }
        auto C = DlvCommand(*S, ECommandKind::OperatingRule);
        C.Rule = Op;
        C.Origin = ECommandOrigin::Player;
        C.CompanyId = Op.OwnerId;
        S->Enqueue(C, E);
        auto R = DlvRule(TEXT("dlv.fallback"), TEXT("storage.player"), TEXT("storage.service.cleveland"),
                         TEXT("city.us.detroit"), TEXT("city.us.cleveland"));
        R.Terms.GoodId = TEXT("good.wood");
        R.Terms.VehicleId = TEXT("vehicle.player");
        R.Terms.Kg = 50;
        R.Terms.EndHour = 49;
        R.Terms.MarketFallback = S == &A;
        DlvQueue(*S, R, E);
        S->StepOnce();
        int64 BeforeDue = 0;
        for (const auto &O : S->GetState().Orders)
            if (O.ActorId == TEXT("company.player") && O.Status == TEXT("open") && O.Side == TEXT("buy"))
                BeforeDue += O.RemainingKg;
        TestEqual(TEXT("Promised same-target quantity suppresses duplicate procurement"), BeforeDue, 0LL);
    }
    for (int H = 0; H < 6; ++H)
    {
        A.StepOnce();
        B.StepOnce();
    }
    auto Quantity = [](const FSimulation &S) {
        int64 Kg = 0;
        for (const auto &O : S.GetState().Orders)
            if (O.ActorId == TEXT("company.player") && O.Status == TEXT("open") && O.Side == TEXT("buy"))
                Kg += O.RemainingKg;
        return Kg;
    };
    TestEqual(TEXT("Fallback covers actual target gap"), Quantity(A), 50LL);
    TestEqual(TEXT("Wait policy counts next promised slot"), Quantity(B), 0LL);
    for (auto *S : {&A, &B})
    {
        TestEqual(TEXT("One failed due, not hourly repeated attempts"), S->GetState().DeliverySlots.Num(), 1);
        TestEqual(TEXT("Next due advanced by interval"), S->GetState().Deliveries[0].NextDueHour, 25LL);
        TestTrue(TEXT("No hidden stock replenishment"), S->AuditRuntime(E));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeliveryValidation, "CoI.Delivery.StopCorruptionAndAtomicLoadRollback",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDeliveryValidation::RunTest(const FString &)
{
    FString J, E, Disk;
    FCoIGeoMapData G;
    DlvFixture(J, G, OwnCleveland);
    FSimulation S;
    if (!S.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    auto R = DlvRule(TEXT("dlv.validate"), TEXT("storage.player"), TEXT("storage.service.cleveland"),
                     TEXT("city.us.detroit"), TEXT("city.us.cleveland"));
    R.Terms.VehicleId = TEXT("vehicle.player");
    auto Bad = R;
    Bad.Terms.ToCityId = TEXT("city.us.chicago");
    TestFalse(TEXT("Stable hub/asset references checked"), S.PreviewDelivery(Bad, E));
    Bad = R;
    Bad.Terms.IntervalHours = 0;
    TestFalse(TEXT("Invalid interval rejected"), S.PreviewDelivery(Bad, E));
    Bad = R;
    Bad.Terms.MaxFreightCents = 0;
    TestFalse(TEXT("No free unfunded return"), S.PreviewDelivery(Bad, E));
    DlvQueue(S, R, E);
    for (int H = 0; H < 4; ++H)
        S.StepOnce();
    if (!S.SaveGameJson(Disk, E))
    {
        AddError(E);
        return false;
    }
    auto Base = DlvUnpack(Disk);
    auto Snapshot = S.Fingerprint();
    const uint64 Generation = S.GetGeneration();
    int32 Speed = 1;
    auto Reject = [&](FSaveData &D) {
        TestFalse(TEXT("Corrupt delivery save refused"), S.LoadGameJson(DlvRepack(D), G, S.GetScenario(), E, Speed));
        TestEqual(TEXT("Failed load leaves live state intact"), S.Fingerprint(), Snapshot);
        TestTrue(TEXT("Failed load leaves generation intact"), S.GetGeneration() == Generation);
    };
    auto D = Base;
    D.State.Deliveries[0].NextDueHour--;
    Reject(D);
    D = Base;
    D.State.DeliverySlots[0].ReturnHoldCents++;
    Reject(D);
    D = Base;
    D.State.DeliverySlots[0].OutboundId = TEXT("missing");
    Reject(D);
    D = Base;
    D.State.SiteLedgers[0].ExternalCostCents++;
    Reject(D);
    D = Base;
    D.State.Deliveries[0].BuyerId = TEXT("missing");
    Reject(D);
    D = Base;
    {
        auto Copy = D.State.DeliverySlots[0];
        Copy.Id += TEXT(".duplicate");
        D.State.DeliverySlots.Add(Copy);
    }
    Reject(D);
    R.Action = TEXT("stop");
    TestTrue(TEXT("Stop command queued"), DlvQueue(S, R, E));
    S.StepOnce();
    const auto Requests = S.GetState().Deliveries[0].RequestedKg;
    for (int H = 0; H < 72; ++H)
        S.StepOnce();
    TestEqual(TEXT("Stopping cannot create future goods attempts"), S.GetState().Deliveries[0].RequestedKg, Requests);
    TestEqual(TEXT("Already booked cargo and return still complete"), S.GetState().DeliverySlots[0].Status,
              FString(TEXT("complete")));
    TestTrue(TEXT("No double arrival or orphaned holds"), S.ValidateSaveState(E));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeliverySeller, "CoI.Delivery.ExternalBuyerConsentAndLocalTransport",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDeliverySeller::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    DlvFixture(J, G, [](auto Root) {
        for(auto O:Root->GetObjectField(TEXT("ContractExamples"))->GetArrayField(TEXT("Orders")))if(O->AsObject()->GetStringField(TEXT("StorageId"))==TEXT("storage.player")){O->AsObject()->SetStringField(TEXT("CityId"),TEXT("city.us.pittsburgh"));O->AsObject()->SetStringField(TEXT("MarketId"),TEXT("market.pittsburgh"));}
        for (auto C : Root->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("Companies")))
            if (C->AsObject()->GetStringField(TEXT("Id")) == TEXT("company.merchant"))
                C->AsObject()->SetStringField(TEXT("DecisionProfile"), TEXT("service"));
        for (auto W : Root->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Warehouses")))
            if (W->AsObject()->GetStringField(TEXT("Id")) == TEXT("storage.player"))
                W->AsObject()->SetStringField(TEXT("CityId"), TEXT("city.us.pittsburgh"));
        for (auto I : Root->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Inventories")))
            if (I->AsObject()->GetStringField(TEXT("Id")) == TEXT("stock.player.food"))
                I->AsObject()->SetStringField(TEXT("GoodId"), TEXT("good.ore"));
        for (auto V : Root->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Vehicles")))
            if (V->AsObject()->GetStringField(TEXT("OwnerId")) == TEXT("company.carrier"))
                V->AsObject()->SetStringField(TEXT("CityId"), TEXT("city.us.pittsburgh"));
        for (auto O : Root->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Offers")))
        {
            O->AsObject()->SetStringField(TEXT("FromCityId"), TEXT("city.us.pittsburgh"));
            O->AsObject()->SetStringField(TEXT("ToCityId"), TEXT("city.us.detroit"));
        }
    });
    FSimulation S;
    if (!S.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    S.StepOnce();
    auto R = DlvRule(TEXT("dlv.seller"), TEXT("storage.player"), TEXT("storage.tools"), TEXT("city.us.pittsburgh"),
                     TEXT("city.us.pittsburgh"));
    R.Terms.GoodId = TEXT("good.ore");
    R.Terms.Kg = 50;
    R.Terms.EndHour = 1;
    if (!S.PreviewDelivery(R, E))
    {
        AddError(E);
        return false;
    }
    auto Bad = R;
    Bad.Terms.PriceCentsPerKg = 100;
    TestFalse(TEXT("NPC refuses excessive landed price"), S.PreviewDelivery(Bad, E));
    DlvQueue(S, R, E);
    for (int H = 0; H < 8; ++H)
    {
        S.StepOnce();
        if (!S.ValidateSaveState(E))
        {
            AddError(E);
            return false;
        }
    }
    const auto *X = Find(S.GetState().DeliverySlots, FString(TEXT("dlv.seller.at.1")));
    if (!TestNotNull(TEXT("Buyer accepted one real slot"), X))
        return false;
    TestEqual(TEXT("NPC pays seller"), X->PurchaseCents, 50LL);
    TestEqual(TEXT("Local real transport arrives"), X->DeliveredKg, 50LL);
    TestEqual(TEXT("Same hub needs no fake return"), X->Status, FString(TEXT("complete")));
    TestTrue(TEXT("No paid empty teleport"), X->ReturnId.IsEmpty() && X->ReturnCostCents == 0);
    int64 Revenue = 0;
    for (const auto &L : S.GetState().SiteLedgers)
        if (L.ActorId == TEXT("company.player"))
            Revenue += L.ExternalRevenueCents;
    TestEqual(TEXT("External seller revenue is real"), Revenue, 50LL);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeliveryTarget, "CoI.Delivery.TargetCapAndRecoveryWithoutBacklog",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDeliveryTarget::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    DlvFixture(J, G, [](auto Root) {
        OwnCleveland(Root);
        for (auto I : Root->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Inventories")))
            if (I->AsObject()->GetStringField(TEXT("Id")) == TEXT("stock.player.food"))
                I->AsObject()->SetStringField(TEXT("GoodId"), TEXT("good.wood"));
    });
    FSimulation S;
    if (!S.Load(J, G, E))
        return false;
    auto Op = *Find(S.GetState().OperatingRules, FString(TEXT("storage.service.cleveland")));
    Op.Explicit = Op.Enabled = true;
    for (auto &X : Op.Goods)
        if (X.GoodId == TEXT("good.wood"))
        {
            X.TargetKg = 50;
            X.BuyLimitCents = 10;
        }
    auto C = DlvCommand(S, ECommandKind::OperatingRule);
    C.Rule = Op;
    C.Origin = ECommandOrigin::Player;
    C.CompanyId = Op.OwnerId;
    S.Enqueue(C, E);
    auto R = DlvRule(TEXT("dlv.target"), TEXT("storage.player"), TEXT("storage.service.cleveland"),
                     TEXT("city.us.detroit"), TEXT("city.us.cleveland"));
    R.Terms.GoodId = TEXT("good.wood");
    R.Terms.VehicleId = TEXT("vehicle.player");
    R.Terms.Kg = 50;
    R.Terms.EndHour = 49;
    DlvQueue(S, R, E);
    for (int H = 0; H < 74; ++H)
    {
        S.StepOnce();
        if (!S.ValidateSaveState(E))
        {
            AddError(E);
            return false;
        }
    }
    const auto *Rule = Find(S.GetState().Deliveries, R.Id);
    TestEqual(TEXT("Three due attempts despite covered target"), Rule->RequestedKg, 150LL);
    TestEqual(TEXT("Already supplied target is not ordered twice"), Rule->BookedKg, 50LL);
    TestEqual(TEXT("Unused future quantities expire"), Rule->MissedKg, 100LL);
    int64 Dest = 0;
    for (const auto &I : S.GetState().Inventories)
        if (I.OwnerId == TEXT("company.player") && I.StorageId == TEXT("storage.service.cleveland") &&
            I.GoodId == TEXT("good.wood"))
            Dest += I.QuantityKg;
    TestEqual(TEXT("Physical target remains 50 kg"), Dest, 50LL);
    return true;
}

#endif

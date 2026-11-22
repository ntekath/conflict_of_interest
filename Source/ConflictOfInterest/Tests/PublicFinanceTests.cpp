#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GeoCatalog.h"
#include "Simulation/SimArchive.h"
#include "Simulation/SaveGame.h"
#include "Simulation/Banking.h"
#include "Serialization/JsonSerializer.h"
using namespace CoISim;
namespace
{
bool PublicFixture(FSimulation &S, FCoIGeoMapData &G, FString &E, bool Enabled = true, bool Empty = false,
                   bool NoConsumption = false, bool TinyPool = false)
{
    FString J;
    auto *C =
        LoadObject<UCoIGeoCatalog>(nullptr, TEXT("/Game/Geography/Data/GeoCatalog_DE_NL_1913.GeoCatalog_DE_NL_1913"));
    if (!C || !G.Load(C->DefinitionJson, E) ||
        !FFileHelper::LoadFileToString(J,
                                       *(FPaths::ProjectContentDir() / TEXT("Simulation/scenario.de_nl1913.coisim"))))
        return false;
    TSharedPtr<FJsonObject> O;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(J), O);
    auto Econ = O->GetObjectField(TEXT("Economy"));
    auto Q = Econ->GetObjectField(TEXT("Public"));
    Q->SetBoolField(TEXT("Enabled"), Enabled);
    auto W = O->GetObjectField(TEXT("InitialState"));
    if (Empty)
    {
        for (auto &V : Q->GetArrayField(TEXT("Treasuries")))
            V->AsObject()->SetNumberField(TEXT("OpeningReserveCents"), 0);
        for (auto &V : W->GetArrayField(TEXT("Accounts")))
            if (V->AsObject()->GetStringField(TEXT("Id")).StartsWith(TEXT("account.public.")))
                V->AsObject()->SetNumberField(TEXT("BalanceCents"), 0);
        int64 Money = 0;
        for (auto &V : W->GetArrayField(TEXT("Accounts")))
            Money += int64(V->AsObject()->GetNumberField(TEXT("BalanceCents")));
        auto Bank = Econ->GetObjectField(TEXT("Bank"));
        Bank->SetNumberField(TEXT("OpeningBackingCents"),
                             Money + int64(Bank->GetNumberField(TEXT("OpeningCapitalCents"))));
    }
    if (NoConsumption)
        for (auto &V : O->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("Needs")))
        {
            V->AsObject()->SetNumberField(TEXT("BasicKgPerThousandPerDay"), 0);
            V->AsObject()->SetNumberField(TEXT("LuxuryKgPerThousandPerDay"), 0);
        }
    if (TinyPool)
        for (auto &V : W->GetArrayField(TEXT("Groups")))
            V->AsObject()->SetNumberField(TEXT("Workforce"), 1);
    FJsonSerializer::Serialize(O.ToSharedRef(), TJsonWriterFactory<>::Create(&J));
    return S.Load(J, G, E);
}
int64 PublicTotal(const FState &W, int Kind)
{
    int64 N = 0;
    for (const auto &B : W.PublicTreasuries)
        N += Kind == 0   ? B.WagesPaid
             : Kind == 1 ? B.TransfersPaid
             : Kind == 2 ? B.TaxAssessed
             : Kind == 3 ? B.TaxPaid
                         : B.TransferCuts;
    return N;
}
} // namespace
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPublicPayrollTest, "CoI.AP095a.ExactWagesTransfersAndSharedWorkers",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPublicPayrollTest::RunTest(const FString &)
{
    FSimulation S;
    FCoIGeoMapData G;
    FString E;
    if (!TestTrue(*E, PublicFixture(S, G, E, true, false, true)))
        return false;
    const auto Before = S.GetState();
    S.StepOnce();
    if (!TestTrue(*E, S.AuditRuntime(E)))
    {
        AddError(E);
        return false;
    }
    TestEqual(TEXT("50 regional offices on two national accounts"), S.GetScenario().Economy.Public.Offices.Num(), 50);
    TestEqual(TEXT("Exactly 100 funded public positions"), PublicTotal(S.GetState(), 0), 18000LL);
    TestEqual(TEXT("Transfers capped nationally"), PublicTotal(S.GetState(), 1), 3000LL);
    for (const auto &B : S.GetState().PublicTreasuries)
    {
        const auto *R = Find(S.GetScenario().Economy.Public.Treasuries, B.Id);
        const auto *A = Find(S.GetState().Accounts, R->AccountId);
        TestEqual(TEXT("Treasury debited exactly, no replenishment"), A->BalanceCents,
                  R->OpeningReserveCents - B.WagesPaid - B.TransfersPaid);
    }
    for (const auto &X : S.GetState().Groups)
    {
        const auto *Old = Find(Before.Groups, X.Id);
        const auto *A = Find(S.GetState().Accounts, X.AccountId);
        const auto *A0 = Find(Before.Accounts, X.AccountId);
        TestEqual(TEXT("All wage/transfer sources arrive exactly at real group account"),
                  A->BalanceCents - A0->BalanceCents,
                  X.PrivateIncomeCentsDay + X.PublicIncomeCentsDay + X.TransferIncomeCentsDay);
        TestTrue(TEXT("No duplicate workforce"), X.Employed <= Old->Workforce && X.PublicEmployed <= X.Employed);
        const auto *D = Find(S.GetScenario().Definitions.Groups, X.Id);
        if (D->IncomeBand != TEXT("low"))
            TestEqual(TEXT("Only eligible band gets transfer"), X.TransferIncomeCentsDay, 0LL);
    }
    FSimulation Limited;
    if (!PublicFixture(Limited, G, E, true, false, true, true))
        return false;
    Limited.StepOnce();
    TestTrue(TEXT("Competing public/private jobs obey the same pool"), Limited.AuditRuntime(E));
    for (const auto &X : Limited.GetState().Groups)
        TestTrue(TEXT("One available worker never fills two jobs"), X.Employed <= 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPublicEmptyTest, "CoI.AP095a.EmptyTreasuryCutsAndDisabledComparison",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPublicEmptyTest::RunTest(const FString &)
{
    FSimulation S, Off;
    FCoIGeoMapData G;
    FString E;
    if (!PublicFixture(S, G, E, true, true) || !PublicFixture(Off, G, E, false, true))
        return false;
    for (int H = 0; H < 72; ++H)
    {
        S.StepOnce();
        Off.StepOnce();
    }
    TestEqual(TEXT("Empty reserve pays no unfunded wages"), PublicTotal(S.GetState(), 0), 0LL);
    TestEqual(TEXT("No unfunded transfers"), PublicTotal(S.GetState(), 1), 0LL);
    TestTrue(TEXT("Eligible but budget-cut benefits visible"), PublicTotal(S.GetState(), 4) > 0);
    for (const auto &B : S.GetState().PublicTreasuries)
        TestTrue(TEXT("Unfunded positions visible"), B.LastUnfundedJobs > 0);
    TestEqual(TEXT("Disabled state has no grants"), PublicTotal(Off.GetState(), 0) + PublicTotal(Off.GetState(), 1),
              0LL);
    for (const auto &G0 : Off.GetState().Groups)
        TestEqual(TEXT("Disabled public employer hires nobody"), G0.PublicEmployed, 0LL);
    TestTrue(TEXT("Enabled empty reserve invariants"), S.AuditRuntime(E));
    TestTrue(TEXT("Disabled comparison invariants"), Off.AuditRuntime(E));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPublicSaveTest, "CoI.AP095a.EligibilityExpiryAndSameVersionSave",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPublicSaveTest::RunTest(const FString &)
{
    FSimulation S;
    FCoIGeoMapData G;
    FString E;
    if (!PublicFixture(S, G, E))
        return false;
    for (int H = 0; H < 27; ++H)
        S.StepOnce();
    FString Save;
    if (!TestTrue(*E, S.SaveGameJson(Save, E)))
    {
        AddError(E);
        return false;
    }
    FSimulation R;
    int32 Speed = 1;
    if (!TestTrue(*E, R.LoadGameJson(Save, G, S.GetScenario(), E, Speed)))
    {
        AddError(E);
        return false;
    }
    while (S.GetState().EconomyHour < 16 * 24)
    {
        S.StepOnce();
        R.StepOnce();
    }
    TestEqual(TEXT("Identical claims, payroll and balances after load"), S.EconomicFingerprint(),
              R.EconomicFingerprint());
    TestTrue(TEXT("Finite transfer window expires"),
             S.GetState().PublicClaims.ContainsByPredicate([](const FPublicClaim &C) {
                 return C.FirstDay == 0 && C.LastDay >= 14 && C.Assessed > 0 && C.LastDue == 0;
             }));
    for (const auto &X : S.GetState().Groups)
        TestEqual(TEXT("No automatic income after benefit expires"), X.TransferIncomeCentsDay, 0LL);
    auto Bad = S.GetState();
    Bad.PublicClaims[0].Paid = Bad.PublicClaims[0].Assessed + 1;
    TestFalse(TEXT("Corrupt benefits rejected"), ValidatePublicState(S.GetScenario(), Bad, E));
    Bad = S.GetState();
    Bad.PublicTreasuries[0].TaxPaid++;
    TestFalse(TEXT("Corrupt treasury source rejected"), ValidatePublicState(S.GetScenario(), Bad, E));
    TestTrue(TEXT("Continuation invariants"), S.AuditRuntime(E));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPublicTaxTest, "CoI.AP095a.PositiveTaxBaseAndThirtyFiveDayComparison",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPublicTaxTest::RunTest(const FString &)
{
    TestEqual(TEXT("No tax on losses"), PublicTaxAmount(-50000, 100), 0LL);
    TestEqual(TEXT("Fixed positive result assessment"), PublicTaxAmount(12345, 100), 1234LL);
    TestEqual(TEXT("Cent floor"), PublicTaxAmount(9, 100), 0LL);
    FSimulation S, Off;
    FCoIGeoMapData G;
    FString E;
    if (!PublicFixture(S, G, E) || !PublicFixture(Off, G, E, false))
        return false;
    TestEqual(TEXT("Same initial world incl reserves"), S.EconomicFingerprint(), Off.EconomicFingerprint());
    FString Csv = TEXT("day,money_cents,public_workers,private_workers,public_wages_cents,transfers_cents,tax_assessed_"
                       "cents,tax_paid_cents,transfer_cuts_cents,treasury_cents\n");
    for (int Day = 1; Day <= 35; ++Day)
    {
        for (int H = 0; H < 24; ++H)
        {
            S.StepOnce();
            if (Day <= 3)
                Off.StepOnce();
        }
        if (!TestTrue(*E, S.AuditRuntime(E)))
        {
            AddError(E);
            return false;
        }
        int64 Money = 0, Pub = 0, Priv = 0, Cash = 0;
        for (const auto &A : S.GetState().Accounts)
            Money += A.BalanceCents;
        for (const auto &X : S.GetState().Groups)
        {
            Pub += X.PublicEmployed;
            Priv += X.Employed - X.PublicEmployed;
        }
        for (const auto &T : S.GetScenario().Economy.Public.Treasuries)
            Cash += Find(S.GetState().Accounts, T.AccountId)->BalanceCents;
        Csv += FString::Printf(TEXT("%d,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld\n"), Day, Money, Pub, Priv,
                               PublicTotal(S.GetState(), 0), PublicTotal(S.GetState(), 1), PublicTotal(S.GetState(), 2),
                               PublicTotal(S.GetState(), 3), PublicTotal(S.GetState(), 4), Cash);
    }
    TestTrue(TEXT("Real closed positive firm results assessed"), PublicTotal(S.GetState(), 2) > 0);
    TestTrue(TEXT("Actual funded tax transferred"), PublicTotal(S.GetState(), 3) > 0);
    TestTrue(TEXT("Finite budget, still no hidden grant"), S.AuditRuntime(E));
    TestTrue(TEXT("Disabled private rules comparison valid"), Off.AuditRuntime(E));
    TestEqual(TEXT("Disabled reserve stays untouched"),
              PublicTotal(Off.GetState(), 0) + PublicTotal(Off.GetState(), 1) + PublicTotal(Off.GetState(), 3), 0LL);
    FFileHelper::SaveStringToFile(Csv, *(FPaths::ProjectSavedDir() / TEXT("AP095a_35days.csv")),
                                  FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    FFileHelper::SaveStringToFile(PublicDiagnostics(S.GetScenario(), S.GetState()),
                                  *(FPaths::ProjectSavedDir() / TEXT("AP095a_budget.txt")),
                                  FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    FString Save;if(!TestTrue(TEXT("Save assessed and paid taxes"),S.SaveGameJson(Save,E))){AddError(E);return false;}
    FSimulation Reloaded;int32 Speed=1;if(!TestTrue(TEXT("Load tax cursors and treasury"),Reloaded.LoadGameJson(Save,G,S.GetScenario(),E,Speed))){AddError(E);return false;}
    const int64 Assessed=PublicTotal(S.GetState(),2);
    for(int H=0;H<48;++H){S.StepOnce();Reloaded.StepOnce();}
    TestEqual(TEXT("Monthly assessment is not billed twice after load"),PublicTotal(S.GetState(),2),Assessed);
    TestEqual(TEXT("Identical post-tax and exhausted-budget continuation"),S.EconomicFingerprint(),Reloaded.EconomicFingerprint());
    TestTrue(TEXT("Post-tax save invariants"),S.AuditRuntime(E));
    return true;
}
#endif

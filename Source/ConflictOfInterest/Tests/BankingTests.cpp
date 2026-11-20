#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GeoCatalog.h"
#include "Simulation/Banking.h"
#include "Simulation/Investments.h"
#include "Simulation/SaveGame.h"
#include "Simulation/SimArchive.h"
#include "Serialization/JsonSerializer.h"
using namespace CoISim;
namespace
{
int64 Deposits(const FSimulation &S)
{
    int64 X = 0;
    for (const auto &A : S.GetState().Accounts)
        X += A.BalanceCents;
    return X;
}
bool Fixture(FString &Json, FCoIGeoMapData &Geo, TFunction<void(FScenario &)> Edit = {})
{
    auto *C = LoadObject<UCoIGeoCatalog>(nullptr, TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));
    FString E, Raw;
    if (!C || !Geo.Load(C->DefinitionJson, E) ||
        !FFileHelper::LoadFileToString(Raw, *(FPaths::ProjectContentDir() / TEXT("Simulation/scenario.coisim"))))
        return false;
    FScenario Scenario;
    if (!LoadScenario(Raw, Geo, Scenario, E))
        return false;
    auto &Q = Scenario.Economy.Bank;
    Q.Enabled = true;
    Q.NpcEnabled = false;
    Q.CityId = TEXT("city.us.chicago");
    Q.OperatingRecipientAccountId = TEXT("account.company.services");
    Q.PaymentHours = Q.MinTermHours = 24;
    Q.MaxTermHours = 240;
    Q.DefaultHours = 48;
    Q.StartupLimitCents = 10000000;
    Q.CollateralPermille = 1000;
    Q.SpendPermille = 1000;
    // Explicit test-only opening valuation; no money or goods are credited.
    for (auto &A : Scenario.Economy.AccountingOpeningAssets)
        if (A.OwnerId == TEXT("company.player"))
        {
            A.ValueCents = 10000000;
            A.Known = true;
        }
    FInventory Shelf;
    Shelf.Id = TEXT("stock.bank.retail");
    Shelf.GoodId = TEXT("good.food");
    Shelf.OwnerId = TEXT("company.merchant");
    Shelf.StorageId = TEXT("retail.store.chicago.merchant");
    Shelf.QuantityKg = 50;
    Scenario.InitialState.Inventories.Add(Shelf);
    Scenario.InitialState.Inventories.Sort([](const FInventory &A, const FInventory &B) { return A.Id < B.Id; });
    for (auto &N : Scenario.Definitions.Needs)
        N.BasicMaxPriceCents = 100;
    if (Edit)
        Edit(Scenario);
    Q.OpeningBackingCents = Q.OpeningCapitalCents;
    for (const auto &A : Scenario.InitialState.Accounts)
        Q.OpeningBackingCents += A.BalanceCents;
    FWrite W;
    Scenario.Fields(W);
    return FJsonSerializer::Serialize(W.Object, TJsonWriterFactory<>::Create(&Json));
}
FCommand Loan(int64 Amount = 100000, int64 Term = 240, const FString &Owner = TEXT("company.player"),
              const FString &Id = TEXT("loan.test"))
{
    FCommand C;
    C.Kind = ECommandKind::Loan;
    C.Loan.Id = Id;
    C.Loan.OwnerId = Owner;
    C.Loan.Action = TEXT("apply");
    C.Loan.AmountCents = Amount;
    C.Loan.TermHours = Term;
    return C;
}
bool Queue(FSimulation &S, FCommand C, FString &E)
{
    C.Generation = S.GetGeneration();
    C.Sequence = S.NextSequence();
    C.TargetStep = S.GetState().Step + 1;
    return S.Enqueue(C, E);
}
bool Run(FSimulation &S, int64 Hour, FString &E)
{
    while (S.GetState().EconomyHour < Hour)
    {
        if (!S.StepOnce() || !S.AuditRuntime(E))
            return false;
    }
    return true;
}
} // namespace
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBankMoney, "CoI.Banking.OpeningCreationRepaymentAndNormalTransfers",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBankMoney::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    if (!Fixture(J, G))
        return false;
    FSimulation S;
    if (!TestTrue(TEXT("Fresh banking scenario"), S.Load(J, G, E)))
        return false;
    const int64 Initial = Deposits(S);
    TestTrue(TEXT("Opening identity, no second cash"), S.GetScenario().Economy.Bank.OpeningBackingCents ==
                                                           Initial + S.GetScenario().Economy.Bank.OpeningCapitalCents);
    const auto Core = S.EconomicFingerprint();
    FLoanQuote Q;
    TestTrue(TEXT("Preview approves explicit secured startup"), S.PreviewLoan(Loan().Loan, Q, E));
    TestEqual(TEXT("Preview is read only"), S.EconomicFingerprint(), Core);
    TestTrue(TEXT("Loan command queued"), Queue(S, Loan(), E));
    S.StepOnce();
    TestTrue(TEXT("Loan approved"), S.GetResults().Last().Accepted);
    TestEqual(TEXT("1000 USD creates exactly 1000 USD deposits"), Deposits(S) - Initial, 100000LL);
    TestEqual(TEXT("Same loan bank claim"), S.GetState().Loans[0].OutstandingCents, 100000LL);
    FCommand Repay = Loan(10000);
    Repay.Loan.Action = TEXT("repay");
    Repay.Loan.LoanId = TEXT("loan.test");
    TestTrue(TEXT("Repayment queued"), Queue(S, Repay, E));
    S.StepOnce();
    TestEqual(TEXT("100 USD destroys exactly 100 USD principal deposits"), S.GetState().Bank.RepaidCents, 10000LL);
    TestEqual(TEXT("Bank claim falls to 900 USD"), S.GetState().Loans[0].OutstandingCents, 90000LL);
    const int64 Created = S.GetState().Bank.CreatedCents;
    TestTrue(TEXT("Ordinary operations and household consumption retain balance equation"), Run(S, 80, E));
    TestEqual(TEXT("Normal payments do not originate loans"), S.GetState().Bank.CreatedCents, Created);
    TestTrue(TEXT("Retail consumption really paid"),
             Find(S.GetState().CashTotals, FString(TEXT("retail_sale"))) != nullptr);
    TestEqual(TEXT("Explicit money equation"), Deposits(S), Initial + BankMoneyChange(S.GetState().Bank));
    TestTrue(*E, S.ValidateSaveState(E));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBankSpending, "CoI.Banking.InterestFundedWagesExpensesAndSharedWorkers",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBankSpending::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    if (!Fixture(J, G))
        return false;
    FSimulation S;
    if (!S.Load(J, G, E))
        return false;
    TestTrue(TEXT("Large declared secured loan"), Queue(S, Loan(2000000), E));
    TestTrue(*E, Run(S, 97, E));
    const auto &B = S.GetState().Bank;
    TestTrue(TEXT("Interest actually collected"), B.InterestCents > 0);
    TestTrue(TEXT("Bank wages paid to existing household accounts"), B.WagesCents > 0);
    TestTrue(TEXT("Bank operating spending paid to real provider"), B.OperatingCents > 0);
    TestTrue(TEXT("No unfunded bank wages or expenses"), B.WagesCents + B.OperatingCents <= B.InterestCents);
    TestEqual(TEXT("Principal separate from earnings"),
              S.GetScenario().Economy.Bank.OpeningCapitalCents + B.InterestCents - B.WagesCents - B.OperatingCents -
                  B.OperatingArrearsCents - B.WriteOffCents,
              BankEquity(S.GetScenario().Economy.Bank, B));
    const auto *Staff = Find(S.GetState().ServiceEmployment, BankStaffId);
    TestNotNull(TEXT("Bank participates in same employment array"), Staff);
    if (Staff)
    {
        int64 Workers = 0;
        for (const auto &Job : Staff->Employment)
            Workers += Job.Workers;
        TestTrue(TEXT("Funded bank staff hired"), Workers > 0 && Workers <= S.GetScenario().Economy.Bank.Jobs);
    }
    TestTrue(*E, S.ValidateSaveState(E));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBankDefault, "CoI.Banking.ArrearsWriteoffDoesNotDeleteDeposits",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBankDefault::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    if (!Fixture(J, G, [](FScenario &C) {
            for (auto &I : C.InitialState.Inventories)
                if (I.OwnerId == TEXT("company.player"))
                    I.QuantityKg = 0;
        }))
        return false;
    FSimulation S;
    if (!S.Load(J, G, E))
        return false;
    const int64 Opening = Deposits(S);
    Queue(S, Loan(), E);
    S.StepOnce();
    const auto *A = Find(S.GetState().Accounts, FString(TEXT("account.company.player")));
    if (!A)
        return false;
    FCommand Hold;
    Hold.Kind = ECommandKind::Reserve;
    Hold.Resources.Id = TEXT("bank.test.freeze");
    Hold.Resources.ActorId = TEXT("company.player");
    Hold.Resources.AccountId = A->Id;
    Hold.Resources.MoneyCents = A->BalanceCents;
    TestTrue(TEXT("Explicit hold prevents spending reserved cash"), Queue(S, Hold, E));
    TestTrue(*E, Run(S, 73, E));
    const auto &L = S.GetState().Loans[0];
    TestEqual(TEXT("Default after configured overdue interval"), L.Status, FString(TEXT("defaulted")));
    TestEqual(TEXT("Borrower still owes principal"), L.OutstandingCents, 100000LL);
    TestEqual(TEXT("Separate bank impairment"), S.GetState().Bank.WriteOffCents, 100000LL);
    TestEqual(TEXT("Writeoff never deletes unrelated deposits"), Deposits(S), Opening + 100000);
    TestTrue(
        TEXT("Arrears remain in obligation ledger"),
        S.GetState().DebtDues.ContainsByPredicate([](const FDebtDue &D) { return D.Status == TEXT("defaulted"); }));
    TestTrue(*E, S.ValidateSaveState(E));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBankRisk, "CoI.Banking.RiskBoundsAuthorityAndNoAutomaticRescue",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBankRisk::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    if (!Fixture(J, G))
        return false;
    FSimulation S;
    if (!S.Load(J, G, E))
        return false;
    FLoanQuote Q;
    auto Request = Loan();
    Request.Loan.TermHours = 25;
    TestFalse(TEXT("No unconfigured term"), S.PreviewLoan(Request.Loan, Q, E));
    Request = Loan(10000001);
    TestFalse(TEXT("No unlimited credit"), S.PreviewLoan(Request.Loan, Q, E));
    Request = Loan();
    Request.Origin = ECommandOrigin::Player;
    Request.CompanyId = TEXT("company.food");
    Request.Loan.OwnerId = TEXT("company.food");
    TestFalse(TEXT("Player cannot borrow as NPC"), Queue(S, Request, E));
    auto Copy = S.GetState();
    auto Scenario = S.GetScenario();
    Scenario.Economy.Bank.OpeningCapitalCents = 1;
    TestFalse(TEXT("Bank risk budget applies"), AssessLoan(Scenario, Copy, Loan().Loan, Q, E));
    Copy = S.GetState();
    Copy.EconomyHour = 14 * 24;
    TestFalse(TEXT("Missing cash history never implies profits"), AssessLoan(S.GetScenario(), Copy, Loan().Loan, Q, E));
    TestTrue(TEXT("First loan eligible"), Queue(S, Loan(), E));
    S.StepOnce();
    Request = Loan(100000, 240, TEXT("company.player"), TEXT("loan.second"));
    TestFalse(TEXT("Already pledged assets cannot be reused"), S.PreviewLoan(Request.Loan, Q, E));
    TestTrue(*E, Run(S, 400, E));
    TestEqual(TEXT("No rescue money during loss run"), S.GetState().Bank.CreatedCents, 100000LL);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBankSave, "CoI.Banking.PendingLoanAndDueSaveLoadContinuation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBankSave::RunTest(const FString &)
{
    FString J, E, Save;
    FCoIGeoMapData G;
    if (!Fixture(J, G))
        return false;
    FSimulation A, B;
    if (!A.Load(J, G, E) || !B.Load(J, G, E))
        return false;
    Queue(A, Loan(), E);
    TestTrue(TEXT("Queued loan saved"), A.SaveGameJson(Save, E));
    int32 Speed = 1;
    TestTrue(*E, B.LoadGameJson(Save, G, A.GetScenario(), E, Speed));
    TestTrue(*E, Run(A, 23, E) && Run(B, 23, E));
    TestEqual(TEXT("Queued loan exactly once after restore"), A.EconomicFingerprint(), B.EconomicFingerprint());
    TestEqual(TEXT("One issued credit"), B.GetState().Loans.Num(), 1);
    FCommand P = Loan(10000);
    P.Loan.Action = TEXT("repay");
    P.Loan.LoanId = TEXT("loan.test");
    Queue(A, P, E);
    TestTrue(*E, A.SaveGameJson(Save, E));
    TestTrue(*E, B.LoadGameJson(Save, G, A.GetScenario(), E, Speed));
    TestTrue(*E, Run(A, 80, E) && Run(B, 80, E));
    TestEqual(TEXT("Interest, repayments, payroll and dues resume identically"), A.EconomicFingerprint(),
              B.EconomicFingerprint());
    auto Before = B.EconomicFingerprint();
    TSharedPtr<FJsonObject> Root;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Save), Root);
    Root->GetObjectField(TEXT("Payload"))->SetNumberField(TEXT("FormatVersion"), 7);
    Save.Empty();
    FJsonSerializer::Serialize(Root.ToSharedRef(), TJsonWriterFactory<>::Create(&Save));
    TestFalse(TEXT("Old save rejected without migration"), B.LoadGameJson(Save, G, A.GetScenario(), E, Speed));
    TestEqual(TEXT("Rejected load preserves live state"), B.EconomicFingerprint(), Before);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBankInvestment, "CoI.Banking.CreditFinancedCapacityAndLosingInvestment",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBankInvestment::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    if (!Fixture(J, G, [](FScenario &C) {
            for (auto &A : C.InitialState.Accounts)
                if (A.OwnerId == TEXT("company.player"))
                    A.BalanceCents = 0;
        }))
        return false;
    FSimulation S;
    if (!S.Load(J, G, E))
        return false;
    FCommand Build;
    Build.Kind = ECommandKind::Invest;
    auto &I = Build.Investment;
    I.Id = TEXT("loan.capacity");
    I.OwnerId = TEXT("company.player");
    I.CityId = TEXT("city.us.detroit");
    I.Action = TEXT("upgrade");
    I.DefinitionId = TEXT("upgrade.warehouse");
    I.TargetId = TEXT("storage.player");
    FInvestmentQuote Q;
    TestFalse(TEXT("No free unfunded capacity"), S.PreviewInvestment(I, Q, E));
    TestTrue(TEXT("Loan application"), Queue(S, Loan(200000), E));
    S.StepOnce();
    TestTrue(TEXT("Credit enables normal investment preview"), S.PreviewInvestment(I, Q, E));
    TestTrue(TEXT("Normal build command, no privileged capacity"), Queue(S, Build, E));
    TestTrue(*E, Run(S, 50, E));
    const auto *Project = Find(S.GetState().Projects, I.Id);
    TestTrue(TEXT("Financed upgrade completed"), Project && Project->Status == TEXT("complete"));
    const auto *Store = Find(S.GetState().Warehouses, I.TargetId);
    TestTrue(TEXT("Real warehouse expanded"), Store && Store->TypeId == TEXT("warehouse.player.large"));
    const auto *Period = S.GetState().AccountingPeriods.FindByPredicate(
        [](const FAccountingPeriod &P) { return P.ActorId == TEXT("company.player"); });
    TestTrue(TEXT("Borrowing did not turn a bad investment into booked profit"), Period && Period->Profit() < 0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBankNpcRules, "CoI.Banking.NpcUsesSameAuthorityAndHistoryRules",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBankNpcRules::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    if (!Fixture(J, G))
        return false;
    FSimulation S;
    if (!S.Load(J, G, E))
        return false;
    auto C = Loan(10000, 240, TEXT("company.food"), TEXT("npc.loan.test"));
    C.Origin = ECommandOrigin::Autonomous;
    C.CompanyId = C.Loan.OwnerId;
    TestTrue(TEXT("NPC submits normal validated command"), Queue(S, C, E));
    S.StepOnce();
    TestTrue(TEXT("NPC passes same bank approval"), S.GetResults().Last().Accepted);
    TestEqual(TEXT("NPC receives one matching debt"), S.GetState().Loans[0].OwnerId, FString(TEXT("company.food")));
    auto State = S.GetState();
    State.EconomyHour = 14 * 24;
    State.BankHistory.Add({TEXT("company.food|13"), TEXT("company.food"), 13, 100000, 0});
    for (auto &P : State.AccountingPeriods)
        if (P.ActorId == TEXT("company.food"))
        {
            P.GoodsRevenue = 10000;
            P.Known = true;
        }
    FLoanQuote Q;
    auto Request = C.Loan;
    Request.Id = TEXT("loan.history");
    Request.AmountCents = 1000;
    TestTrue(TEXT("Declared positive result/cash history supports limited finance"),
             AssessLoan(S.GetScenario(), State, Request, Q, E));
    TestFalse(TEXT("History branch no longer uses startup subsidy"), Q.Startup);
    if (!Fixture(J, G, [](FScenario &F) { F.PlayerCompanyId = TEXT("company.food"); }))
        return false;
    FSimulation Controlled;
    if (!Controlled.Load(J, G, E))
        return false;
    TestFalse(TEXT("AI cannot borrow for player-controlled company"), Queue(Controlled, C, E));
    C.Origin = ECommandOrigin::Player;
    TestTrue(TEXT("Same company can apply through its player controller"), Queue(Controlled, C, E));
    Controlled.StepOnce();
    TestTrue(TEXT("Same decision for player request"), Controlled.GetResults().Last().Accepted);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBankCorruption, "CoI.Banking.RejectsCorruptBalanceAndDuplicateDueWithoutMutation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBankCorruption::RunTest(const FString &)
{
    FString J, E, Save;
    FCoIGeoMapData G;
    if (!Fixture(J, G))
        return false;
    FSimulation S;
    if (!S.Load(J, G, E))
        return false;
    Queue(S, Loan(), E);
    if (!Run(S, 30, E) || !S.SaveGameJson(Save, E))
        return false;
    const auto Before = S.EconomicFingerprint();
    TSharedPtr<FJsonObject> Root;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Save), Root);
    FSaveData D;
    FRead Reader{Root->GetObjectField(TEXT("Payload")), E, true};
    D.Fields(Reader);
    auto Encode = [](FSaveData Data) {
        FWrite W;
        Data.Fields(W);
        FFingerprint H;
        H.Write(Data);
        auto Envelope = MakeShared<FJsonObject>();
        Envelope->SetStringField(TEXT("Format"), TEXT("CoI.Save"));
        Envelope->SetStringField(TEXT("Checksum"), H.Finish());
        Envelope->SetObjectField(TEXT("Payload"), W.Object);
        FString Out;
        FJsonSerializer::Serialize(Envelope, TJsonWriterFactory<>::Create(&Out));
        return Out;
    };
    int32 Speed = 1;
    auto Invalid = D;
    Invalid.State.Bank.NextOperatingHour = 0;
    TestFalse(TEXT("No duplicate bank operating assessment after load"),
              S.LoadGameJson(Encode(Invalid), G, S.GetScenario(), E, Speed));
    Invalid = D;
    Invalid.State.DebtDues[0].DueHour += 24;
    TestFalse(TEXT("No shifted or duplicated contract due"),
              S.LoadGameJson(Encode(Invalid), G, S.GetScenario(), E, Speed));
    Invalid = D;
    ++Invalid.State.Bank.CreatedCents;
    TestFalse(TEXT("Unbacked money creation rejected"), S.LoadGameJson(Encode(Invalid), G, S.GetScenario(), E, Speed));
    TestEqual(TEXT("All rejected saves preserve running state"), S.EconomicFingerprint(), Before);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBankEurope, "CoI.Banking.EuropeFortyDayLoanAndSaveLoad",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBankEurope::RunTest(const FString &)
{
    FString J, E, Save;
    FCoIGeoMapData G;
    auto *Catalog =
        LoadObject<UCoIGeoCatalog>(nullptr, TEXT("/Game/Geography/Data/GeoCatalog_DE_NL_1913.GeoCatalog_DE_NL_1913"));
    if (!Catalog || !G.Load(Catalog->DefinitionJson, E) ||
        !FFileHelper::LoadFileToString(J,
                                       *(FPaths::ProjectContentDir() / TEXT("Simulation/scenario.de_nl1913.coisim"))))
        return false;
    FSimulation S, T;
    if (!TestTrue(TEXT("Actual European banking scenario"), S.Load(J, G, E)))
        return false;
    const int64 Opening = Deposits(S);
    Queue(S, Loan(100000, 1680), E);
    S.StepOnce();
    TestTrue(TEXT("Actual opening assets secure 1000 USD"), S.GetResults().Last().Accepted);
    TestTrue(*E, Run(S, 40 * 24, E));
    TestTrue(*E, S.SaveGameJson(Save, E));
    int32 Speed = 1;
    TestTrue(*E, T.LoadGameJson(Save, G, S.GetScenario(), E, Speed));
    TestTrue(*E, Run(S, 41 * 24, E) && Run(T, 41 * 24, E));
    TestEqual(TEXT("European bank and economy resume exactly"), S.EconomicFingerprint(), T.EconomicFingerprint());
    TestEqual(TEXT("European money identity includes only declared bank flows"), Deposits(S),
              Opening + BankMoneyChange(S.GetState().Bank));
    AddInfo(FString::Printf(TEXT("41 Tage: Einlagen %lld, Kreditschöpfung %lld, Tilgung %lld, Zinsen %lld, Banklöhne "
                                 "%lld, Bankbetrieb %lld Cent"),
                            Deposits(S), S.GetState().Bank.CreatedCents, S.GetState().Bank.RepaidCents,
                            S.GetState().Bank.InterestCents, S.GetState().Bank.WagesCents,
                            S.GetState().Bank.OperatingCents));
    return true;
}
#endif

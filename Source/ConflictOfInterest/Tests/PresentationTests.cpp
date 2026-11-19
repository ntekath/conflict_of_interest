#if WITH_DEV_AUTOMATION_TESTS
#include "GeoCatalog.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Simulation/Presentation.h"
#include "Simulation/SimArchive.h"
#include "Simulation/SimCore.h"
using namespace CoISim;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDisplayFormats, "CoI.Presentation.ExactUnitsAndMoneyInput",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDisplayFormats::RunTest(const FString &)
{
    TestEqual(TEXT("USD, never raw cents"), Display::Money(350), FString(TEXT("3,50 USD")));
    TestEqual(TEXT("Negative sub-dollar"), Display::Money(-1), FString(TEXT("-0,01 USD")));
    TestEqual(TEXT("Large negative result"), Display::Money(-1234567890123456789LL),
              FString(TEXT("-12.345.678.901.234.567,89 USD")));
    TestEqual(TEXT("Maximum exact cents"), Display::Money(MAX_int64), FString(TEXT("92.233.720.368.547.758,07 USD")));
    TestEqual(TEXT("Minimum exact cents"), Display::Money(MIN_int64), FString(TEXT("-92.233.720.368.547.758,08 USD")));
    TestEqual(TEXT("Quantity kg with tonnes"), Display::Quantity(1250), FString(TEXT("1.250 kg (1,25 t)")));
    TestEqual(TEXT("Fractional percentages retained"), Display::Percent(.803), FString(TEXT("80,3 %")));
    TestEqual(TEXT("Hour interval"), Display::Duration(24), FString(TEXT("24 h")));
    TestTrue(TEXT("Period year and exclusive end"),
             Display::Period(TEXT("1913-01-01"), 0, 24).Contains(TEXT("02.01.1913 00:00 (Ende exklusiv)")));
    int64 Cents = 91;
    TestTrue(TEXT("German USD input"), Display::ParseMoneyInput(TEXT("12,34"), Cents));
    TestEqual(TEXT("No factor 100 error"), Cents, 1234LL);
    TestTrue(TEXT("Dot accepted without grouping"), Display::ParseMoneyInput(TEXT("12.3"), Cents));
    TestEqual(TEXT("Single decimal"), Cents, 1230LL);
    TestTrue(TEXT("Exact maximum input"), Display::ParseMoneyInput(TEXT("92233720368547758,07"), Cents));
    TestEqual(TEXT("Max preserved"), Cents, MAX_int64);
    for (const TCHAR *Bad : {TEXT("1,234"), TEXT("1.000,50"), TEXT("1e3"), TEXT("-1"), TEXT("+1"), TEXT("0"), TEXT(""),
                             TEXT("1,"), TEXT(",5"), TEXT("92233720368547758,08")})
    {
        Cents = 91;
        TestFalse(TEXT("Reject without rounding or overflow"), Display::ParseMoneyInput(Bad, Cents));
        TestEqual(TEXT("Failure leaves output unchanged"), Cents, 91LL);
    }
    TestTrue(TEXT("Explicit zero budget allowed"), Display::ParseMoneyInput(TEXT("0,00"), Cents, true));
    TestEqual(TEXT("Real zero"), Cents, 0LL);
    for (const TCHAR *Key : {TEXT("price"), TEXT("op.wage"), TEXT("op.budget"), TEXT("op.good.food.buy"),
                             TEXT("op.good.food.sell"), TEXT("delivery.price"), TEXT("delivery.freight")})
        TestTrue(TEXT("All existing financial forms use shared USD conversion"), Display::IsMoneyField(Key));
    TestFalse(TEXT("Kg are not money"), Display::IsMoneyField(TEXT("op.good.food.target")));
    TestFalse(TEXT("Hours are not money"), Display::IsMoneyField(TEXT("delivery.duration")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDisplayOwnership, "CoI.Presentation.OwnActiveEmployeesAndUniqueHubs",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDisplayOwnership::RunTest(const FString &)
{
    FState S;
    FDefinitions D;
    S.EconomyHour = 5;
    for (const TCHAR *Id : {TEXT("hub.a"), TEXT("hub.b"), TEXT("hub.c")})
    {
        FPlaceDef P;
        P.Id = Id;
        D.Places.Add(P);
    }
    FCompanyDef Co;
    Co.Id = TEXT("own");
    Co.HeadquartersCityId = TEXT("hub.c");
    D.Companies.Add(Co);
    S.Companies.Add({TEXT("own"), TEXT("account")});
    S.Accounts.Add({TEXT("account"), TEXT("own"), 10000, 2345});
    FFacility F;
    F.Id = TEXT("plant");
    F.OwnerId = TEXT("own");
    F.CityId = TEXT("hub.a");
    F.StorageId = TEXT("plant.store");
    F.PaidUntilHour = 24;
    F.UnfundedJobs = 100;
    F.Employment = {{TEXT("g1"), 3}, {TEXT("g2"), 2}};
    S.Facilities.Add(F);
    FWarehouse W;
    W.Id = TEXT("plant.store");
    W.OwnerId = TEXT("own");
    W.CityId = TEXT("hub.a");
    S.Warehouses.Add(W);
    W.Id = TEXT("warehouse");
    S.Warehouses.Add(W);
    S.ServiceEmployment.Add(
        {TEXT("plant.store"), 24, {{TEXT("g1"), 5}}}); // A production store must never duplicate plant staff.
    S.ServiceEmployment.Add({TEXT("warehouse"), 24, {{TEXT("g1"), 2}}});
    FVehicle V;
    V.Id = TEXT("truck");
    V.OwnerId = TEXT("own");
    V.HomeCityId = TEXT("hub.b");
    V.CityId = TEXT("");
    V.Bound = true;
    S.Vehicles.Add(V);
    S.ServiceEmployment.Add({TEXT("truck"), 24, {{TEXT("g2"), 1}}});
    F.Id = TEXT("expired");
    F.StorageId = TEXT("expired.store");
    F.PaidUntilHour = 5;
    F.Employment = {{TEXT("g1"), 40}};
    S.Facilities.Add(F);
    F.Id = TEXT("foreign");
    F.OwnerId = TEXT("foreign");
    F.CityId = TEXT("hub.c");
    F.PaidUntilHour = 24;
    F.Employment = {{TEXT("g1"), 500}};
    S.Facilities.Add(F);
    FConstructionProject Project;
    Project.OwnerId = TEXT("own");
    Project.CityId = TEXT("hub.c");
    Project.Status = TEXT("building");
    S.Projects.Add(Project);
    auto P = CompanyPresentation(S, D, TEXT("own"), TEXT("1913-01-01"));
    TestEqual(TEXT("Only active funded staff, no vacancies, no foreign staff, no plant-store duplication"),
              P.Employees.Value, 8LL);
    TestEqual(TEXT("Hub A staff"), P.EmployeesByCity.FindRef(TEXT("hub.a")), 7LL);
    TestEqual(TEXT("Moving driver remains at payroll home hub"), P.EmployeesByCity.FindRef(TEXT("hub.b")), 1LL);
    TestEqual(TEXT("Distinct existing hubs, no empty HQ or building site"), P.Sites.Value, 2LL);
    TestEqual(TEXT("Money reservations once"), P.AvailableMoney.Value, 7655LL);
    TestTrue(TEXT("Tooltip exposes gross and reservations"),
             P.AvailableMoney.Tooltip().Contains(TEXT("100,00 USD; reserviert 23,45 USD")));
    for (const auto *M : {&P.Employees, &P.Sites, &P.AvailableMoney, &P.MonthlyResult, &P.NetWorth})
        TestTrue(TEXT("Shared tooltip source, period and formula"), M->Tooltip().Contains(TEXT("Quelle:")) &&
                                                                        M->Tooltip().Contains(TEXT("Zeitraum:")) &&
                                                                        M->Tooltip().Contains(TEXT("Berechnung:")));
    S.EconomyHour = 24;
    P = CompanyPresentation(S, D, TEXT("own"), TEXT("1913-01-01"));
    TestEqual(TEXT("Expired payroll no longer active"), P.Employees.Value, 0LL);
    TestEqual(TEXT("Real zero distinct from unavailable"), P.Employees.Display(), FString(TEXT("0")));
    TestEqual(TEXT("Temporary lack of labor does not delete sites"), P.Sites.Value, 2LL);
    auto Foreign = CompanyPresentation(S, D, TEXT("foreign"), TEXT("1913-01-01"));
    TestEqual(TEXT("Controller changes ownership scope"), Foreign.Sites.Value, 1LL);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDisplayMissing, "CoI.Presentation.MonthBoundaryAndMissingValuation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDisplayMissing::RunTest(const FString &)
{
    FState S;
    FDefinitions D;
    auto P = CompanyPresentation(S, D, TEXT("own"), TEXT("1913-01-01"));
    TestEqual(TEXT("No fake monthly zero"), P.MonthlyResult.Display(), FString(TEXT("Noch keine Monatsdaten")));
    TestEqual(TEXT("No invented balance sheet"), P.NetWorth.Display(), FString(TEXT("Nicht verfügbar")));
    S.EconomyHour = 31 * 24 - 1;
    P = CompanyPresentation(S, D, TEXT("own"), TEXT("1913-01-01"));
    TestTrue(TEXT("Current month never completed early"), P.MonthlyResult.State == EDisplayState::NoMonthlyHistory);
    S.EconomyHour = 31 * 24;
    S.PlayerDays.Add({});
    S.PlayerDays.Last().IncomeCents = 1000000;
    S.PlayerDays.Last().ExpenseCents = 1;
    P = CompanyPresentation(S, D, TEXT("own"), TEXT("1913-01-01"));
    TestEqual(TEXT("Cashflow cannot masquerade as realized profit"), P.MonthlyResult.Display(),
              FString(TEXT("Nicht verfügbar")));
    TestEqual(TEXT("Last calendar month, never rolling 30 days"), P.MonthlyResult.Period,
              FString(TEXT("01.01.1913 bis 01.02.1913 (Ende exklusiv)")));
    TestTrue(TEXT("Internal revenue exclusion explicit"),
             P.MonthlyResult.Calculation.Contains(TEXT("interne Umsätze")));
    TestTrue(TEXT("Reservations not counted again as liabilities"),
             P.NetWorth.Calculation.Contains(TEXT("keine zusätzlichen Schulden")));
    S.EconomyHour = 17 * 24;
    P = CompanyPresentation(S, D, TEXT("own"), TEXT("1913-01-15"));
    TestTrue(TEXT("Partial first month not claimed complete"),
             P.MonthlyResult.State == EDisplayState::NoMonthlyHistory);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDisplaySnapshot, "CoI.Presentation.SnapshotConservationSaveAndCompanyScope",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDisplaySnapshot::RunTest(const FString &)
{
    FString J, E, Disk;
    FCoIGeoMapData G;
    auto *Catalog = LoadObject<UCoIGeoCatalog>(nullptr, TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));
    if (!Catalog || !G.Load(Catalog->DefinitionJson, E) ||
        !FFileHelper::LoadFileToString(J, *(FPaths::ProjectContentDir() / TEXT("Simulation/scenario.coisim"))))
        return false;
    FSimulation S, Restored;
    if (!S.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    S.StepOnce();
    const FString Hash = S.Fingerprint();
    const auto &V = S.GetSnapshot().Player;
    int64 People = 0, Staff = 0;
    for (const auto &C : V.Cities)
    {
        People += C.Residents;
        Staff += C.OwnEmployees;
        for (const auto &Group : C.Groups)
        {
            TestFalse(TEXT("No raw cents in population view"), Group.Contains(TEXT("Cent")));
            TestFalse(TEXT("Wealth scale named"), Group.Contains(TEXT("/1000")));
        }
    }
    TestEqual(TEXT("Residents assigned once across hubs"), People, S.GetSnapshot().Population);
    TestEqual(TEXT("Own staff sums once across hub views"), Staff, V.Company.Employees.Value);
    int64 Expected = 0;
    for (const auto &F : S.GetState().Facilities)
        if (F.OwnerId == V.ActorId && F.PaidUntilHour > V.Hour)
            for (const auto &Jb : F.Employment)
                Expected += Jb.Workers;
    for (const auto &W : S.GetState().ServiceEmployment)
        if (W.PaidUntilHour > V.Hour)
        {
            const auto *Store = Find(S.GetState().Warehouses, W.Id);
            const auto *Truck = Find(S.GetState().Vehicles, W.Id);
            if ((Store && Store->OwnerId == V.ActorId) || (Truck && Truck->OwnerId == V.ActorId))
                for (const auto &Jb : W.Employment)
                    Expected += Jb.Workers;
        }
    TestEqual(TEXT("KPI matches authoritative own employment"), V.Company.Employees.Value, Expected);
    for (int I = 0; I < 10; ++I)
        CompanyPresentation(S.GetState(), V.Catalog, V.ActorId, V.StartDate);
    TestEqual(TEXT("Viewing never books money or goods"), S.Fingerprint(), Hash);
    TestTrue(TEXT("Save existing schema"), S.SaveGameJson(Disk, E));
    int32 Speed = 1;
    if (!Restored.LoadGameJson(Disk, G, S.GetScenario(), E, Speed))
    {
        AddError(E);
        return false;
    }
    TestEqual(TEXT("Save/load economic identity"), Restored.Fingerprint(), Hash);
    TestEqual(TEXT("Derived employee snapshot reconstructed"), Restored.GetSnapshot().Player.Company.Employees.Value,
              V.Company.Employees.Value);
    TestEqual(TEXT("Derived site snapshot reconstructed"), Restored.GetSnapshot().Player.Company.Sites.Value,
              V.Company.Sites.Value);
    TestEqual(TEXT("Same money source as existing HUD"), V.Company.AvailableMoney.Value,
              S.GetSnapshot().PlayerMoneyCents - S.GetSnapshot().PlayerReservedCents);
    TestTrue(TEXT("Existing audit"), Restored.AuditRuntime(E));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDisplaySelection, "CoI.Presentation.DifferentPlayerCompanyUsesSameEconomicWorld",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDisplaySelection::RunTest(const FString &)
{
    FString J, E;
    FCoIGeoMapData G;
    auto *Catalog = LoadObject<UCoIGeoCatalog>(nullptr, TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));
    if (!Catalog || !G.Load(Catalog->DefinitionJson, E) ||
        !FFileHelper::LoadFileToString(J, *(FPaths::ProjectContentDir() / TEXT("Simulation/scenario.coisim"))))
        return false;
    FSimulation A, B;
    if (!A.Load(J, G, E))
        return false;
    TSharedPtr<FJsonObject> Root;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(J), Root);
    Root->SetStringField(TEXT("PlayerCompanyId"), TEXT("company.food"));
    J.Empty();
    FJsonSerializer::Serialize(Root.ToSharedRef(), TJsonWriterFactory<>::Create(&J));
    if (!B.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
    TestEqual(TEXT("No money or assets from new perspective"), A.EconomicFingerprint(), B.EconomicFingerprint());
    TestEqual(TEXT("Trading house has actual two asset hubs"), A.GetSnapshot().Player.Company.Sites.Value, 2LL);
    TestEqual(TEXT("Producer counts plant and store once"), B.GetSnapshot().Player.Company.Sites.Value, 1LL);
    TestEqual(TEXT("No employees invented before first payroll"), B.GetSnapshot().Player.Company.Employees.Value, 0LL);
    B.StepOnce();
    const auto &P = B.GetSnapshot().Player;
    int64 OwnPlant = 0;
    for (const auto &F : B.GetState().Facilities)
        if (F.OwnerId == P.ActorId && F.PaidUntilHour > P.Hour)
            for (const auto &Job : F.Employment)
                OwnPlant += Job.Workers;
    TestTrue(TEXT("Selected producer hires actual people"), OwnPlant > 0);
    TestEqual(TEXT("Company KPI excludes NPC service payroll"), P.Company.Employees.Value, OwnPlant);
    return true;
}
#endif

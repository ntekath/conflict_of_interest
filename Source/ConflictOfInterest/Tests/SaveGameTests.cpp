#if WITH_DEV_AUTOMATION_TESTS
#include "GenericPlatform/GenericPlatformFile.h"
#include "GeoCatalog.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Serialization/JsonSerializer.h"
#include "Simulation/DeveloperConsole.h"
#include "Simulation/SaveGame.h"
#include "Simulation/SimArchive.h"
using namespace CoISim;
namespace
{
bool SaveFixture(FString &J, FCoIGeoMapData &G)
{
    FString E;
    auto *C = LoadObject<UCoIGeoCatalog>(nullptr, TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));
    return C && G.Load(C->DefinitionJson, E) &&
           FFileHelper::LoadFileToString(J, *(FPaths::ProjectContentDir() / TEXT("Simulation/ap092_legacy_fixture.coisim")));
}
FCommand SaveCommand(FSimulation &S, ECommandKind K)
{
    FCommand C;
    C.Kind = K;
    C.Generation = S.GetGeneration();
    C.Sequence = S.NextSequence();
    C.TargetStep = S.GetState().Step + 1;
    return C;
}
FString Repack(FSaveData &D)
{
    FWrite W;
    D.Fields(W);
    FFingerprint H;
    H.Write(D);
    auto R = MakeShared<FJsonObject>();
    R->SetStringField(TEXT("Format"), TEXT("CoI.Save"));
    R->SetStringField(TEXT("Checksum"), H.Finish());
    R->SetObjectField(TEXT("Payload"), W.Object);
    FString J;
    FJsonSerializer::Serialize(R, TJsonWriterFactory<>::Create(&J));
    return J;
}
FSaveData Unpack(const FString &J)
{
    TSharedPtr<FJsonObject> R;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(J), R);if(R->HasField(TEXT("Economy")))R->GetObjectField(TEXT("Economy"))->SetBoolField(TEXT("HouseholdModel"),false);
    FString E;
    FRead Read{R->GetObjectField(TEXT("Payload")), E, true};
    FSaveData D;
    D.Fields(Read);
    return D;
}
} // namespace
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveContinuation, "CoI.Save.ConstructionPartialOrderTransportAndContinuation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveContinuation::RunTest(const FString &)
{
    FString J, E, Disk;
    FCoIGeoMapData G;
    if (!SaveFixture(J, G))
        return false;
    FSimulation A, B;
    if (!A.Load(J, G, E) || !B.Load(J, G, E))
        return false;
    const auto Scenario = A.GetScenario();
    auto Build = SaveCommand(A, ECommandKind::Invest);
    Build.Investment.Id = TEXT("save.build");
    Build.Investment.OwnerId = TEXT("company.player");
    Build.Investment.Action = TEXT("build");
    Build.Investment.DefinitionId = TEXT("build.warehouse");
    Build.Investment.CityId = TEXT("city.us.detroit");
    Build.Investment.PlotId = TEXT("plot.city.us.detroit.warehouse.1");
    TestTrue(TEXT("Construction queued"), A.Enqueue(Build, E));
    auto Buy = SaveCommand(A, ECommandKind::PlaceOrder);
    auto &O = Buy.Order;
    O.Id = TEXT("save.partial");
    O.ActorId = TEXT("company.player");
    O.StorageId = TEXT("storage.food");
    O.CityId = TEXT("city.us.chicago");
    O.MarketId = TEXT("market.chicago");
    O.GoodId = TEXT("good.food");
    O.Side = TEXT("buy");
    O.OriginalKg = 5000;
    O.LimitCentsPerKg = 10;
    A.Enqueue(Buy, E);
    auto Trip = SaveCommand(A, ECommandKind::BookTransport);
    auto &T = Trip.Transport;
    T.Id = TEXT("save.trip");
    T.OwnerId = TEXT("company.player");
    T.VehicleId = TEXT("vehicle.player");
    T.FromCityId = TEXT("city.us.detroit");
    T.ToCityId = TEXT("city.us.cleveland");
    T.SourceInventoryId = TEXT("stock.player.food");
    T.DestinationStorageId = TEXT("storage.service.cleveland");
    T.Kg = 50;
    T.RequestedDepartureHour = 3;
    TestTrue(TEXT("Transport queued"), A.Enqueue(Trip, E));
    A.StepOnce();
    if (!TestTrue(TEXT("Full boundary export: ") + E, A.SaveGameJson(Disk, E, 16)))
    {
        AddError(E);
        return false;
    }
    int32 Speed = 0;
    const uint64 Old = B.GetGeneration();
    if (!TestTrue(TEXT("Load isolated candidate: ") + E, B.LoadGameJson(Disk, G, Scenario, E, Speed)))
    {
        AddError(E);
        return false;
    }
    TestEqual(TEXT("Full state and holds"), A.Fingerprint(), B.Fingerprint());
    {
        auto InvalidProject = Unpack(Disk);
        InvalidProject.State.Projects[0].AssetId = TEXT("storage.player");
        const auto Before = B.Fingerprint();
        TestFalse(TEXT("Invalid deferred asset binding rejected"),
                  B.LoadGameJson(Repack(InvalidProject), G, Scenario, E, Speed));
        TestEqual(TEXT("Deferred binding error leaves current state intact"), B.Fingerprint(), Before);
    }

    TestEqual(TEXT("Saved speed"), Speed, 16);
    TestTrue(TEXT("Fresh generation"), B.GetGeneration() != Old);
    TestTrue(TEXT("Construction ongoing"),
             Find(A.GetState().Projects, FString(TEXT("save.build")))->Status == TEXT("building"));
    const auto *Partial = Find(A.GetState().Orders, FString(TEXT("save.partial")));
    if (!TestNotNull(TEXT("Partial order exists"), Partial))
        return false;
    TestTrue(TEXT("Order partly filled"), Partial->RemainingKg > 0 && Partial->RemainingKg < Partial->OriginalKg);
    auto Pending = SaveCommand(A, ECommandKind::CancelOrder);
    Pending.Order.Id = TEXT("save.partial");
    Pending.Order.ActorId = TEXT("company.player");
    Pending.Origin = ECommandOrigin::Player;
    Pending.CompanyId = TEXT("company.player");
    TestTrue(TEXT("Pending player command"), A.Enqueue(Pending, E));
    TestTrue(TEXT("Save pending"), A.SaveGameJson(Disk, E));
    TestTrue(TEXT("Restore pending"), B.LoadGameJson(Disk, G, Scenario, E, Speed));
    auto Stale = Pending;
    TestFalse(TEXT("Old generation cannot replay"), B.Enqueue(Stale, E));
    for (int I = 0; I < 672; ++I)
    {
        A.StepOnce();
        B.StepOnce();
        if (I == 25)
        {
            auto Upgrade = SaveCommand(A, ECommandKind::Invest);
            Upgrade.Investment.Id = TEXT("save.upgrade");
            Upgrade.Investment.OwnerId = TEXT("company.player");
            Upgrade.Investment.Action = TEXT("upgrade");
            Upgrade.Investment.DefinitionId = TEXT("upgrade.warehouse");
            Upgrade.Investment.CityId = TEXT("city.us.detroit");
            Upgrade.Investment.TargetId = TEXT("save.build.asset");
            TestTrue(TEXT("Upgrade queued"), A.Enqueue(Upgrade, E));
            if (!A.SaveGameJson(Disk, E) || !B.LoadGameJson(Disk, G, Scenario, E, Speed))
            {
                AddError(E);
                return false;
            }
        }
        if (I == 27)
        {
            TestTrue(TEXT("Upgrade remains in progress"),
                     Find(A.GetState().Projects, FString(TEXT("save.upgrade")))->Status == TEXT("building"));
            if (!A.SaveGameJson(Disk, E) || !B.LoadGameJson(Disk, G, Scenario, E, Speed))
            {
                AddError(E);
                return false;
            }
        }
        if (I == 4)
        {
            const auto *Moving = Find(A.GetState().Transports, FString(TEXT("save.trip")));
            TestTrue(TEXT("Transport actually in flight"), Moving && Moving->Status == TEXT("moving"));
            if (!A.SaveGameJson(Disk, E) || !B.LoadGameJson(Disk, G, Scenario, E, Speed))
            {
                AddError(E);
                return false;
            }
        }
        if (A.Fingerprint() != B.Fingerprint())
        {
            AddError(FString::Printf(TEXT("Divergence at hour %lld"), A.GetState().EconomyHour));
            return false;
        }
        if (I % 24 == 0 && !A.ValidateSaveState(E))
        {
            AddError(E);
            return false;
        }
    }
    TestEqual(TEXT("Exactly one completed warehouse"),
              A.GetState()
                  .Warehouses.FilterByPredicate([](const FWarehouse &W) { return W.Id == TEXT("save.build.asset"); })
                  .Num(),
              1);
    TestEqual(TEXT("One construction payment"), Find(A.GetState().CashTotals, FString(TEXT("construction")))->Cents,
              200000LL);
    TestEqual(TEXT("One upgrade payment"), Find(A.GetState().CashTotals, FString(TEXT("asset_upgrade")))->Cents,
              150000LL);
    TestEqual(TEXT("Same warehouse expanded once"),
              Find(A.GetState().Warehouses, FString(TEXT("save.build.asset")))->TypeId,
              FString(TEXT("warehouse.player.large")));
    TestEqual(TEXT("Delivery finished"), Find(A.GetState().Transports, FString(TEXT("save.trip")))->Status,
              FString(TEXT("arrived")));
    TestTrue(TEXT("Final accounting"), A.AuditRuntime(E));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveRejection, "CoI.Save.CorruptionReferencesVersionsAndAtomicRollback",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveRejection::RunTest(const FString &)
{
    FString J, E, Disk;
    FCoIGeoMapData G;
    if (!SaveFixture(J, G))
        return false;
    FSimulation S;
    if (!S.Load(J, G, E))
        return false;
    for (int I = 0; I < 49; ++I)
        S.StepOnce();
    if (!S.SaveGameJson(Disk, E))
    {
        AddError(E);
        return false;
    }
    const auto Scenario = S.GetScenario();
    const auto Hash = S.Fingerprint();
    const uint64 Generation = S.GetGeneration();
    int32 Speed = 4;
    auto Reject = [&](const FString &Bad, const TCHAR *Label) {
        TestFalse(Label, S.LoadGameJson(Bad, G, Scenario, E, Speed));
        TestEqual(TEXT("Failed load preserves current state"), S.Fingerprint(), Hash);
        TestTrue(TEXT("Failed load preserves generation"), S.GetGeneration() == Generation);
        TestEqual(TEXT("Failed load preserves selected speed"), Speed, 4);
        TestFalse(TEXT("Readable rejection"), E.IsEmpty());
    };
    Reject(Disk.Left(Disk.Len() / 2), TEXT("Truncated JSON"));
    Reject(TEXT("[]"), TEXT("Wrong root"));
    auto Base = Unpack(Disk);
    FString BrokenChecksum = Disk;
    BrokenChecksum.ReplaceInline(*Base.SavedUtc, TEXT("2001-01-01T00:00:00.000Z"));
    Reject(BrokenChecksum, TEXT("Checksum detects changed payload"));
    auto Bad = Base;
    Bad.State.Accounts[0].BalanceCents++;
    FString Damaged = Repack(Bad);
    Damaged.ReplaceInline(TEXT("CoI.Save"), TEXT("broken"));
    Reject(Damaged, TEXT("Wrong format"));
    Bad = Base;
    for(const int64 OldVersion:{4LL,5LL,99LL}){Bad=Base;Bad.FormatVersion=OldVersion;Reject(Repack(Bad),TEXT("Different save version rejected without migration"));TestTrue(TEXT("Clear new-game instruction"),E.Contains(TEXT("neue Partie")));}
    Bad=Base;Bad.SimulationVersion=CurrentSaveVersion-1;Reject(Repack(Bad),TEXT("Older simulation version rejected"));
    Bad=Base;Bad.DefinitionsVersion=CurrentSaveVersion-1;Reject(Repack(Bad),TEXT("Older definitions version rejected"));
    Bad = Base;
    Bad.RandomAlgorithm = TEXT("new-rng");
    Reject(Repack(Bad), TEXT("Unknown RNG"));
    Bad = Base;
    Bad.State.Inventories[0].GoodId = TEXT("missing");
    Reject(Repack(Bad), TEXT("Unknown stock good, valid checksum"));
    Bad = Base;
    Bad.State.Facilities[0].CostRecipientAccountId = TEXT("missing");
    Reject(Repack(Bad), TEXT("Unknown payment account"));
    Bad = Base;
    Bad.State.Companies[0].AccountId = TEXT("missing");
    Reject(Repack(Bad), TEXT("Unknown company account"));
    Bad = Base;
    {
        auto Duplicate = Bad.State.Accounts[0];
        Bad.State.Accounts.Add(Duplicate);
    }
    Reject(Repack(Bad), TEXT("Duplicate IDs"));
    Bad = Base;
    Bad.Session.PlayerCompanyId = TEXT("company.food");
    Reject(Repack(Bad), TEXT("No guessed ownership"));
    Bad = Base;
    Bad.State.Regions[0].Population++;
    Reject(Repack(Bad), TEXT("Incorrect aggregate"));
    Bad = Base;
    Bad.State.Groups[0].BasicRemainder = -1;
    Reject(Repack(Bad), TEXT("Invalid remainder"));
    Bad = Base;
    Bad.State.DeveloperMoneySources = 100;
    Reject(Repack(Bad), TEXT("Unmarked money source"));
    Bad = Base;
    Bad.Scenario.Economy.PayrollHours = 48;
    Reject(Repack(Bad), TEXT("Incompatible definitions"));
    TSharedPtr<FJsonObject> Root;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Disk), Root);
    Root->GetObjectField(TEXT("Payload"))->GetObjectField(TEXT("State"))->RemoveField(TEXT("Projects"));
    FString Missing;
    FJsonSerializer::Serialize(Root.ToSharedRef(), TJsonWriterFactory<>::Create(&Missing));
    Reject(Missing, TEXT("Missing optional scenario field is required in saves"));
    Root->GetObjectField(TEXT("Payload"))->SetNumberField(TEXT("Unknown"), 1);
    Missing.Empty();
    FJsonSerializer::Serialize(Root.ToSharedRef(), TJsonWriterFactory<>::Create(&Missing));
    Reject(Missing, TEXT("Unknown fields"));
    FSaveSummary Summary;
    TestTrue(TEXT("Metadata inspect"), InspectSaveGame(Disk, Summary, E));
    TestEqual(TEXT("Summary hour"), Summary.Step, 49LL);
    TestFalse(TEXT("Invalid slot cannot overwrite"), WriteSaveSlot(4, Disk, E));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveControl, "CoI.Save.SeedCompanyDeveloperAuditAndUpgrade",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveControl::RunTest(const FString &)
{
    FString J, E, Disk;
    FCoIGeoMapData G;
    if (!SaveFixture(J, G))
        return false;
    FSimulation Original;
    if (!Original.Load(J, G, E))
        return false;
    const auto Expected = Original.GetScenario();
    TSharedPtr<FJsonObject> Root;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(J), Root);
    Root->SetStringField(TEXT("PlayerCompanyId"), TEXT("company.food"));
    Root->GetObjectField(TEXT("Settings"))->SetNumberField(TEXT("Seed"), 42);
    J.Empty();
    FJsonSerializer::Serialize(Root.ToSharedRef(), TJsonWriterFactory<>::Create(&J));
    FSimulation Other;
    if (!Other.Load(J, G, E))
    {
        AddError(E);
        return false;
    }
#if COI_WITH_DEV_CONSOLE
    FDeveloperConsole Dev;
    TestTrue(TEXT("Explicit developer source"), Dev.Execute(Other, TEXT("money add company.food 1234")));
#endif
    for (int I = 0; I < 25; ++I)
        Other.StepOnce();
    if (!Other.SaveGameJson(Disk, E))
    {
        AddError(E);
        return false;
    }
    int32 Speed = 1;
    if (!Original.LoadGameJson(Disk, G, Expected, E, Speed))
    {
        AddError(E);
        return false;
    }
    TestEqual(TEXT("Saved company restored rather than default"), Original.GetSession().PlayerCompanyId,
              FString(TEXT("company.food")));
    TestEqual(TEXT("Saved seed restored"), Original.GetScenario().Settings.Seed, 42LL);
#if COI_WITH_DEV_CONSOLE
    TestTrue(TEXT("Developer marker survives"), Original.GetState().DeveloperModified);
    TestEqual(TEXT("Explicit source survives"), Original.GetState().DeveloperMoneySources, 1234LL);
    FDeveloperConsole Adv;
    TestTrue(TEXT("Start bounded developer job"), Adv.Execute(Original, TEXT("advance 100")));
    const auto H = Original.GetState().EconomyHour;
    TestTrue(TEXT("Fresh generation"), Original.LoadGameJson(Disk, G, Expected, E, Speed));
    Adv.Tick(Original);
    TestEqual(TEXT("Stale job cannot advance loaded world"), Original.GetState().EconomyHour, H);
#endif
    for (int I = 0; I < 48; ++I)
    {
        Original.StepOnce();
        Other.StepOnce();
    }
    TestEqual(TEXT("Same restored economy and random continuation"), Original.Fingerprint(), Other.Fingerprint());
    return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveDisk, "CoI.Save.DiskSlotReplacementAndReadback",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDisk::RunTest(const FString &)
{
    FString J, E, Disk, Read;
    FCoIGeoMapData G;
    if (!SaveFixture(J, G))
        return false;
    FSimulation A, B;
    if (!A.Load(J, G, E))
        return false;
    for (int I = 0; I < 5; ++I)
        A.StepOnce();
    if (!A.SaveGameJson(Disk, E))
    {
        AddError(E);
        return false;
    }
    const FString Path = SaveSlotPath(3);
    TArray<uint8> Backup;
    const bool Existed = IFileManager::Get().FileExists(*Path);
    if (Existed && (IFileManager::Get().IsReadOnly(*Path) || !FFileHelper::LoadFileToArray(Backup, *Path)))
    {
        AddError(TEXT("Cannot back up existing slot; no write attempted"));
        return false;
    }
    ON_SCOPE_EXIT
    {
        FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Path, false);
        if (Existed)
        {
            if (!FFileHelper::SaveArrayToFile(Backup, *Path))
                AddError(TEXT("Failed to restore original slot bytes"));
        }
        else
            IFileManager::Get().Delete(*Path, false, true);
    };
    TestTrue(TEXT("Write new/replace existing atomically: ") + E, WriteSaveSlot(3, Disk, E));
    TestTrue(TEXT("Read closed file"), ReadSaveSlot(3, Read, E));
    TestEqual(TEXT("UTF-8 complete byte-equivalent text"), Read, Disk);
    TestFalse(TEXT("Refuse corrupt replacement"), WriteSaveSlot(3, TEXT("{}"), E));
    ReadSaveSlot(3, Read, E);
    TestEqual(TEXT("Previous valid file preserved"), Read, Disk);
    A.StepOnce();
    FString Replacement;
    if (!A.SaveGameJson(Replacement, E))
        return false;
    TestTrue(TEXT("Set target read-only to simulate failed replacement"),
             FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Path, true));
    TestFalse(TEXT("OS replacement failure reported"), WriteSaveSlot(3, Replacement, E));
    ReadSaveSlot(3, Read, E);
    TestEqual(TEXT("Failed OS replacement leaves old file intact"), Read, Disk);
    FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Path, false);
    TestTrue(TEXT("Successful replacement"), WriteSaveSlot(3, Replacement, E));
    ReadSaveSlot(3, Read, E);
    TestEqual(TEXT("Complete replacement content"), Read, Replacement);
    int32 Speed = 1;
    TestTrue(TEXT("Independent simulation loads file"), B.LoadGameJson(Read, G, A.GetScenario(), E, Speed));
    TestEqual(TEXT("Disk roundtrip full fingerprint"), A.Fingerprint(), B.Fingerprint());
    auto Slots = ListSaveSlots();
    TestEqual(TEXT("Exactly three numbered slots"), Slots.Num(), 3);
    TestTrue(TEXT("Saved summary readable"), Slots[2].Readable);
    return true;
}
#endif

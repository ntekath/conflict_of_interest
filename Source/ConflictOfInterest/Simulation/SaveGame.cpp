#include "SaveGame.h"
#include "Serialization/JsonSerializer.h"
#include "SimArchive.h"
namespace CoISim
{
namespace
{
FString Digest(const FSaveData &D)
{
    FFingerprint H;
    H.LegacyRetail = D.FormatVersion == 4;H.LegacyAccounting=D.FormatVersion<6;
    H.Write(D);
    return H.Finish();
}
bool Decode(const FString &Json, FSaveData &D, FString &E)
{
    E.Empty();
    auto Fail = [&](const TCHAR *Why) {
        E = Why;
        return false;
    };
    if (Json.Len() > 32 * 1024 * 1024)
        return Fail(TEXT("Spielstand überschreitet 32 MiB."));
    int Depth = 0;
    bool String = false, Escape = false;
    for (TCHAR C : Json)
    {
        if (String)
        {
            if (Escape)
                Escape = false;
            else if (C == '\\')
                Escape = true;
            else if (C == '\"')
                String = false;
        }
        else if (C == '\"')
            String = true;
        else if (C == '{' || C == '[')
        {
            if (++Depth > 64)
                return Fail(TEXT("Spielstand zu tief verschachtelt."));
        }
        else if (C == '}' || C == ']')
        {
            if (--Depth < 0)
                return Fail(TEXT("Ungültiger Spielstand."));
        }
    }
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root)
        return Fail(TEXT("Spielstand ist kein gültiges JSON."));
    FString Format, Checksum;
    const TSharedPtr<FJsonObject> *Payload = nullptr;
    if (Root->Values.Num() != 3 || !Root->TryGetStringField(TEXT("Format"), Format) || Format != TEXT("CoI.Save") ||
        !Root->TryGetStringField(TEXT("Checksum"), Checksum) || !Root->TryGetObjectField(TEXT("Payload"), Payload))
        return Fail(TEXT("Ungültiger Spielstandkopf."));
    double VersionCode = 0;
    if (!(*Payload)->TryGetNumberField(TEXT("FormatVersion"), VersionCode) || (VersionCode != 4 && VersionCode != 5 && VersionCode != 6))
        return Fail(TEXT("Inkompatible Spielstandversion. AP09.4 benötigt Version 6 (Version 4/5 werden geprüft migriert); "
                         "ältere Dateien bleiben unverändert."));
    FRead R{*Payload, E, true};
    R.LegacyRetail = VersionCode == 4;R.LegacyAccounting=VersionCode<6;
    D.Fields(R);
    R.Complete();
    if (!E.IsEmpty())
        return false;
    if (((D.FormatVersion != 4 || D.SimulationVersion != 4 || D.DefinitionsVersion != 4) &&
         (D.FormatVersion != 5 || D.SimulationVersion != 5 || D.DefinitionsVersion != 5) && (D.FormatVersion != 6 || D.SimulationVersion != 6 || D.DefinitionsVersion != 6)) ||
        D.Platform != TEXT("Win64") || D.RandomAlgorithm != TEXT("entity-fnv-splitmix-v1"))
        return Fail(TEXT("Inkompatible Spielstand-, Simulations- oder Definitionsversion."));
    if (Checksum != Digest(D))
        return Fail(TEXT("Prüfsumme stimmt nicht: Spielstand beschädigt oder verändert."));
    if (D.Speed != 1 && D.Speed != 4 && D.Speed != 16)
        return Fail(TEXT("Ungültige Spielgeschwindigkeit."));
    FDateTime Utc;
    if (!FDateTime::ParseIso8601(*D.SavedUtc, Utc))
        return Fail(TEXT("Ungültiges Speicherdatum."));
    return true;
}
} // namespace
FString SaveCompatibility(const FScenario &S)
{
    auto Copy = S;
    Copy.Settings.Seed = 0;
    Copy.PlayerCompanyId.Empty();
    FFingerprint H;
    H.Write(Copy);
    return H.Finish();
}
bool InspectSaveGame(const FString &Json, FSaveSummary &Out, FString &E)
{
    FSaveData D;
    if (!Decode(Json, D, E))
        return false;
    Out.Company = D.Session.PlayerCompanyId;
    Out.Step = D.State.Step;
    Out.SavedUtc = D.SavedUtc;
    Out.DeveloperModified = D.State.DeveloperModified;
    Out.Date = FString::Printf(TEXT("Stunde %lld"), D.State.EconomyHour);
    if (const auto *C = Find(D.Scenario.Definitions.Companies, Out.Company))
        Out.Company = C->Name;
    return true;
}
bool FSimulation::SaveGameJson(FString &Json, FString &E, int32 Speed) const
{
    E.Empty();
    if (!Loaded || InStep)
    {
        E = TEXT("Speichern nur zwischen abgeschlossenen Simulationsschritten möglich.");
        return false;
    }
    if (!ValidateSaveState(E))
        return false;
    FSaveData D;
    D.Scenario = Scenario;
    D.State = State;
    D.Session = Session;
    D.SnapshotVersion = Version;
    D.LastSequence = LastIssuedSequence;
    D.CreditMicros = CreditMicros;
    D.Speed = Speed;
    D.SavedUtc = FDateTime::UtcNow().ToIso8601();
    D.CompatibilityHash = SaveCompatibility(Scenario);
    for (const auto &C : Pending)
    {
        FStoredCommand P;
        P.Kind = (int64)C.Kind;
        P.Origin = (int64)C.Origin;
        P.TargetStep = C.TargetStep;
        P.Sequence = C.Sequence;
        P.CompanyId = C.CompanyId;
        P.Resources = C.Resources;
        P.Order = C.Order;
        P.Transport = C.Transport;
        P.Investment = C.Investment;
        P.Rule = C.Rule;
        P.Delivery = C.Delivery;
        D.Pending.Add(P);
    }
    for (const auto &C : Results)
        D.Results.Add({C.Sequence, C.Step, C.Accepted, C.Reason, C.ActorId, C.ObjectId});
    FWrite W;
    D.Fields(W);
    auto Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("Format"), TEXT("CoI.Save"));
    Root->SetStringField(TEXT("Checksum"), Digest(D));
    Root->SetObjectField(TEXT("Payload"), W.Object);
    FString Candidate;
    if (!FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Candidate)))
    {
        E = TEXT("Spielstand konnte nicht serialisiert werden.");
        return false;
    }
    FSaveData Check;
    if (!Decode(Candidate, Check, E))
        return false;
    Json = MoveTemp(Candidate);
    return true;
}
bool FSimulation::LoadGameJson(const FString &Json, const FCoIGeoMapData &Geo, const FScenario &Expected, FString &E,
                               int32 &Speed)
{
    if (InStep)
    {
        E = TEXT("Laden nur zwischen abgeschlossenen Schritten möglich.");
        return false;
    }
    FSaveData D;
    if (!Decode(Json, D, E))
        return false;
    const bool MigrateRetail = D.FormatVersion == 4;
    if (MigrateRetail)
    {
        auto LegacyExpected = Expected;
        TSet<FString> RetailTypes;
        for (const auto &T : Expected.Definitions.WarehouseTypes)
            if (!T.RetailOffers.IsEmpty())
                RetailTypes.Add(T.Id);
        LegacyExpected.Definitions.WarehouseTypes.RemoveAll(
            [&](const FWarehouseDef &T) { return RetailTypes.Contains(T.Id); });
        LegacyExpected.Definitions.Construction.RemoveAll(
            [&](const FConstructionDef &X) { return RetailTypes.Contains(X.TypeId); });
        LegacyExpected.Definitions.BuildPlots.RemoveAll(
            [](const FBuildPlotDef &X) { return X.Id.StartsWith(TEXT("retail.plot.")); });
        LegacyExpected.InitialState.Warehouses.RemoveAll(
            [&](const FWarehouse &X) { return RetailTypes.Contains(X.TypeId); });
        LegacyExpected.InitialState.AssetSales.RemoveAll(
            [](const FAssetSale &X) { return X.Id.StartsWith(TEXT("retail.sale.")); });
        LegacyExpected.Economy.LegacyHouseholdPurchases = D.Scenario.Economy.LegacyHouseholdPurchases;
        auto LegacyHash = [](FScenario S) {
            S.Settings.Seed = 0;
            S.PlayerCompanyId.Empty();
            FFingerprint H;
            H.LegacyRetail = true;
            H.Write(S);
            return H.Finish();
        };
        if (D.CompatibilityHash != LegacyHash(D.Scenario) || D.CompatibilityHash != LegacyHash(LegacyExpected))
        {
            E = TEXT("Version-4-Migration: Basisszenario stimmt nicht überein.");
            return false;
        }
        const auto OldSeed = D.Scenario.Settings.Seed;
        D.Scenario = Expected;
        D.Scenario.Settings.Seed = OldSeed;
        D.Scenario.PlayerCompanyId = D.Session.PlayerCompanyId;
        for (const auto &W : D.Scenario.InitialState.Warehouses)
            if (!Find(D.State.Warehouses, W.Id))
            {
                if (!RetailTypes.Contains(W.TypeId))
                {
                    E = TEXT("Migration würde ein unbekanntes Asset ergänzen.");
                    return false;
                }
                D.State.Warehouses.Add(W);
            }
        for (const auto &Sale : D.Scenario.InitialState.AssetSales)
            if (!Find(D.State.AssetSales, Sale.Id) && Sale.Id.StartsWith(TEXT("retail.sale.")))
                D.State.AssetSales.Add(Sale);
    }
    else if(D.FormatVersion==5){auto Hash=[](FScenario S){S.Settings.Seed=0;S.PlayerCompanyId.Empty();FFingerprint H;H.LegacyAccounting=true;H.Write(S);return H.Finish();};if(D.CompatibilityHash!=Hash(D.Scenario)||D.CompatibilityHash!=Hash(Expected)){E=TEXT("Version-5-Migration: Basisszenario passt nicht.");return false;}const auto Seed=D.Scenario.Settings.Seed;D.Scenario=Expected;D.Scenario.Settings.Seed=Seed;D.Scenario.PlayerCompanyId=D.Session.PlayerCompanyId;}
    else if (D.CompatibilityHash != SaveCompatibility(D.Scenario) || D.CompatibilityHash != SaveCompatibility(Expected))
    {
        E = TEXT("Szenario-/Konfigurationsdaten passen nicht zu diesem Spielstand.");
        return false;
    }
    if (!ValidateScenario(D.Scenario, Geo, E))
        return false;
    FSimulation C;
    C.Scenario = MoveTemp(D.Scenario);
    C.State = MoveTemp(D.State);
    C.Session = MoveTemp(D.Session);
    C.Loaded = true;
    if (MigrateRetail)
    {
        TArray<FString> OldOrders;
        for (const auto &O : C.State.Orders)
            if (O.Status == TEXT("open") && Find(C.State.Groups, O.ActorId))
                OldOrders.Add(O.Id);
        for (const auto &Id : OldOrders)
        {
            const auto Copy = *Find(C.State.Orders, Id);
            C.CancelOrder(Id, Copy.ActorId, TEXT("cancelled"));
        }
        C.InitializeOperatingRules();
    }

    if(D.FormatVersion<6)C.InitializeAccounting();
    if (D.LastSequence < 0 || D.SnapshotVersion < 0 || D.CreditMicros < 0 ||
        D.CreditMicros > C.Scenario.Settings.RealMicrosPerStep * C.Scenario.Settings.MaxBacklogSteps ||
        D.Pending.Num() > C.Scenario.Settings.MaxPendingCommands || D.Results.Num() > 128)
    {
        E = TEXT("Ungültige Warteschlangen-/Zeitgrenzen.");
        return false;
    }
    C.LastIssuedSequence = D.LastSequence;
    C.Version = D.SnapshotVersion;
    C.CreditMicros = D.CreditMicros;
    TSet<int64> Sequences;
    int64 LastTarget = -1, LastSeq = -1;
    for (const auto &P : D.Pending)
    {
        if (P.Kind < 0 || P.Kind > (int64)ECommandKind::Delivery || P.Origin < 0 ||
            P.Origin > (int64)ECommandOrigin::Autonomous || P.Sequence <= 0 || P.Sequence > D.LastSequence ||
            Sequences.Contains(P.Sequence) || P.TargetStep <= C.State.Step ||
            P.TargetStep - C.State.Step > C.Scenario.Settings.CommandHorizonSteps || P.TargetStep < LastTarget ||
            (P.TargetStep == LastTarget && P.Sequence <= LastSeq))
        {
            E = TEXT("Ungültiger wartender Befehl.");
            return false;
        }
        Sequences.Add(P.Sequence);
        LastTarget = P.TargetStep;
        LastSeq = P.Sequence;
        FCommand Cmd;
        Cmd.Kind = (ECommandKind)P.Kind;
        Cmd.Origin = (ECommandOrigin)P.Origin;
        Cmd.TargetStep = P.TargetStep;
        Cmd.Sequence = P.Sequence;
        Cmd.CompanyId = P.CompanyId;
        Cmd.Resources = P.Resources;
        Cmd.Order = P.Order;
        Cmd.Transport = P.Transport;
        Cmd.Investment = P.Investment;
        Cmd.Rule = P.Rule;
        Cmd.Delivery = P.Delivery;
        if (!C.Authorized(Cmd, E))
            return false;
        C.Pending.Add(Cmd);
    }
    for (const auto &R : D.Results)
    {
        if (R.Sequence <= 0 || R.Sequence > D.LastSequence || Sequences.Contains(R.Sequence) || R.Step < 0 ||
            R.Step > C.State.Step)
        {
            E = TEXT("Ungültiger Befehlsbeleg.");
            return false;
        }
        Sequences.Add(R.Sequence);
        C.Results.Add({R.Sequence, R.Step, R.Accepted, R.Reason, R.ActorId, R.ObjectId});
    }
    if (!C.ValidateSaveState(E))
        return false;
    FFingerprint Before;
    Before.Write(C.State.Regions);
    Before.Write(C.State.Countries);
    FString AggregateHash = Before.Finish();
    C.Aggregate();
    FFingerprint After;
    After.Write(C.State.Regions);
    After.Write(C.State.Countries);
    if (AggregateHash != After.Finish())
    {
        E = TEXT("Gespeicherte Regional-/Landessummen sind inkonsistent.");
        return false;
    }
    // Adoption is the only mutation of the live world. No payroll, matching, delivery or completion here.
    Shutdown();
    Scenario = MoveTemp(C.Scenario);
    State = MoveTemp(C.State);
    Session = MoveTemp(C.Session);
    Pending = MoveTemp(C.Pending);
    Results = MoveTemp(C.Results);
    Version = C.Version;
    LastIssuedSequence = C.LastIssuedSequence;
    CreditMicros = C.CreditMicros;
    for (auto &Cmd : Pending)
        Cmd.Generation = Generation;
    Loaded = true;
    Diagnostics.PendingCommands = Pending.Num();
    Publish();
    Speed = (int32)D.Speed;
    E.Empty();
    return true;
}
} // namespace CoISim

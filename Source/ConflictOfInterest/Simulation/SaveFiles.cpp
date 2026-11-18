#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "SaveGame.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif
namespace CoISim
{
FString SaveSlotPath(int32 Slot)
{
    return Slot >= 1 && Slot <= 3 ? FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SaveGames") /
                                                                      FString::Printf(TEXT("slot_%d.coisave"), Slot))
                                  : FString();
}
bool ReadSaveSlot(int32 Slot, FString &Json, FString &E)
{
    E.Empty();
    FString P = SaveSlotPath(Slot);
    int64 Size = P.IsEmpty() ? -1 : IFileManager::Get().FileSize(*P);
    if (Size < 0 || Size > 32 * 1024 * 1024 || !FFileHelper::LoadFileToString(Json, *P))
    {
        E = TEXT("Speicherplatz fehlt, ist zu groß oder nicht lesbar.");
        return false;
    }
    return true;
}
bool WriteSaveSlot(int32 Slot, const FString &Json, FString &E)
{
    E.Empty();
    FSaveSummary Summary;
    if (!InspectSaveGame(Json, Summary, E))
        return false;
    FString P = SaveSlotPath(Slot);
    if (P.IsEmpty())
    {
        E = TEXT("Ungültiger Speicherplatz.");
        return false;
    }
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(P), true);
    FString Temp = P + TEXT(".") + FGuid::NewGuid().ToString() + TEXT(".tmp");
    FTCHARToUTF8 Utf8(*Json);
    if (Utf8.Length() > 32 * 1024 * 1024)
    {
        E = TEXT("Spielstand überschreitet 32 MiB UTF-8.");
        return false;
    }
    TUniquePtr<FArchive> File(IFileManager::Get().CreateFileWriter(*Temp));
    if (!File)
    {
        E = TEXT("Temporäre Speicherdatei konnte nicht geöffnet werden.");
        return false;
    }
    File->Serialize((void *)Utf8.Get(), Utf8.Length());
    File->Flush();
    bool OK = !File->IsError() && File->Close();
    File.Reset();
#if PLATFORM_WINDOWS
    if (OK)
        OK = !!MoveFileExW(*Temp, *P, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
#else
    OK = false;
#endif
    if (!OK)
    {
        IFileManager::Get().Delete(*Temp, false, true);
        E = TEXT("Speichern fehlgeschlagen. Vorheriger Spielstand bleibt erhalten.");
        return false;
    }
    return true;
}
TArray<FSaveSlotInfo> ListSaveSlots()
{
    TArray<FSaveSlotInfo> Out;
    for (int32 Slot = 1; Slot <= 3; ++Slot)
    {
        FSaveSlotInfo I;
        I.Slot = Slot;
        I.Exists = IFileManager::Get().FileExists(*SaveSlotPath(Slot));
        if (!I.Exists)
            I.Description = TEXT("Leer");
        else
        {
            FString J, E;
            FSaveSummary S;
            I.Readable = ReadSaveSlot(Slot, J, E) && InspectSaveGame(J, S, E);
            I.Description = I.Readable ? S.Company + TEXT(" · ") + S.Date + TEXT(" · ") + S.SavedUtc +
                                             (S.DeveloperModified ? TEXT(" · ENTWICKLER-MODIFIZIERT") : TEXT(""))
                                       : E;
        }
        Out.Add(I);
    }
    return Out;
}
} // namespace CoISim

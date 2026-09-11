#include "WorkbenchRuntime.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

using namespace MagnetSweep;
namespace
{
constexpr int32 MaxSaveCharacters = 2 * 1024 * 1024;

TSharedRef<FJsonObject> SnapshotJson(const FSalvageSnapshot& Snapshot)
{
    const auto Json = MakeShared<FJsonObject>();
    Json->SetNumberField(TEXT("version"), Snapshot.Version);
    Json->SetNumberField(TEXT("layout_index"), Snapshot.LayoutIndex);
    Json->SetNumberField(TEXT("epoch"), Snapshot.Epoch);
    Json->SetNumberField(TEXT("upgrade_level"), Snapshot.UpgradeLevel);
    Json->SetNumberField(TEXT("completed_delivery_count"), Snapshot.CompletedDeliveryCount);
    Json->SetNumberField(TEXT("cargo"), Snapshot.Cargo);
    Json->SetNumberField(TEXT("banked"), Snapshot.Banked);
    Json->SetNumberField(TEXT("goal"), Snapshot.Goal);
    Json->SetBoolField(TEXT("delivery_completed"), Snapshot.bDeliveryCompleted);
    TArray<TSharedPtr<FJsonValue>> Pieces;
    for (const FSalvagePiece& Piece : Snapshot.Pieces)
    {
        const auto Item = MakeShared<FJsonObject>();
        Item->SetNumberField(TEXT("id"), Piece.Id);
        Item->SetNumberField(TEXT("kind"), static_cast<uint8>(Piece.Kind));
        Item->SetNumberField(TEXT("state"), static_cast<uint8>(Piece.State));
        Item->SetNumberField(TEXT("x"), Piece.Position.X);
        Item->SetNumberField(TEXT("y"), Piece.Position.Y);
        Item->SetNumberField(TEXT("amount"), Piece.Amount);
        TArray<TSharedPtr<FJsonValue>> Links;
        for (int32 Id : Piece.DirectLinks) Links.Add(MakeShared<FJsonValueNumber>(Id));
        Item->SetArrayField(TEXT("links"), Links);
        Pieces.Add(MakeShared<FJsonValueObject>(Item));
    }
    Json->SetArrayField(TEXT("pieces"), Pieces);
    return Json;
}

bool Integral(const FJsonObject& Json, const TCHAR* Key, double Min, double Max, double& Result)
{
    return Json.TryGetNumberField(Key, Result) && FMath::IsFinite(Result)
        && Result >= Min && Result <= Max && FMath::FloorToDouble(Result) == Result;
}

bool IntField(const FJsonObject& Json, const TCHAR* Key, int32 Min, int32 Max, int32& Result)
{
    double Number = 0;
    if (!Integral(Json, Key, Min, Max, Number)) return false;
    Result = static_cast<int32>(Number);
    return true;
}

bool ParseSave(const FString& Text, FSalvageSnapshot& Snapshot, bool& bMuted)
{
    if (Text.Len() > MaxSaveCharacters) return false;
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid()) return false;
    int32 Version = 0;
    if (!IntField(*Root, TEXT("save_version"), 1, 1, Version)
        || !Root->TryGetBoolField(TEXT("muted"), bMuted)) return false;
    const TSharedPtr<FJsonObject>* Object = nullptr;
    if (!Root->TryGetObjectField(TEXT("snapshot"), Object) || !Object || !Object->IsValid()) return false;
    const FJsonObject& Json = **Object;
    double Epoch = 0;
    if (!IntField(Json, TEXT("version"), 1, 1, Snapshot.Version)
        || !IntField(Json, TEXT("layout_index"), 0, 1, Snapshot.LayoutIndex)
        || !Integral(Json, TEXT("epoch"), 1, MAX_uint32, Epoch)
        || !IntField(Json, TEXT("upgrade_level"), 0, 2, Snapshot.UpgradeLevel)
        || !IntField(Json, TEXT("completed_delivery_count"), 0, 1000000, Snapshot.CompletedDeliveryCount)
        || !IntField(Json, TEXT("cargo"), 0, 256000, Snapshot.Cargo)
        || !IntField(Json, TEXT("banked"), 0, 256000, Snapshot.Banked)
        || !IntField(Json, TEXT("goal"), 1, 256000, Snapshot.Goal)
        || !Json.TryGetBoolField(TEXT("delivery_completed"), Snapshot.bDeliveryCompleted)) return false;
    Snapshot.Epoch = static_cast<uint32>(Epoch);
    const TArray<TSharedPtr<FJsonValue>>* Pieces = nullptr;
    if (!Json.TryGetArrayField(TEXT("pieces"), Pieces) || !Pieces || Pieces->IsEmpty() || Pieces->Num() > 256)
        return false;
    Snapshot.Pieces.Reset();
    for (const TSharedPtr<FJsonValue>& Value : *Pieces)
    {
        if (!Value.IsValid() || Value->Type != EJson::Object) return false;
        const auto Item = Value->AsObject();
        if (!Item.IsValid()) return false;
        FSalvagePiece Piece;
        int32 Kind = 0, Ownership = 0;
        if (!IntField(*Item, TEXT("id"), 0, MAX_int32, Piece.Id)
            || !IntField(*Item, TEXT("kind"), 0, 1, Kind)
            || !IntField(*Item, TEXT("state"), 0, 2, Ownership)
            || !IntField(*Item, TEXT("amount"), 1, 1000, Piece.Amount)
            || !Item->TryGetNumberField(TEXT("x"), Piece.Position.X)
            || !Item->TryGetNumberField(TEXT("y"), Piece.Position.Y)) return false;
        Piece.Kind = static_cast<EPieceKind>(Kind);
        Piece.State = static_cast<EPieceState>(Ownership);
        const TArray<TSharedPtr<FJsonValue>>* Links = nullptr;
        if (!Item->TryGetArrayField(TEXT("links"), Links) || !Links || Links->Num() > 255) return false;
        for (const TSharedPtr<FJsonValue>& Link : *Links)
        {
            double Number = 0;
            if (!Link.IsValid() || !Link->TryGetNumber(Number) || !FMath::IsFinite(Number)
                || Number < 0 || Number > MAX_int32 || FMath::FloorToDouble(Number) != Number) return false;
            Piece.DirectLinks.Add(static_cast<int32>(Number));
        }
        Snapshot.Pieces.Add(MoveTemp(Piece));
    }
    FString Error;
    return FSalvageModel::ValidateSnapshot(Snapshot, Error);
}

bool ReadSaveFile(const FString& Path, FString& Text)
{
    const int64 Size = IFileManager::Get().FileSize(*Path);
    return Size >= 0 && Size <= MaxSaveCharacters && FFileHelper::LoadFileToString(Text, *Path);
}

bool AtomicReplace(const FString& Temporary, const FString& Destination)
{
    // Both paths are siblings on the same volume. Windows rename replaces the destination
    // without a delete-first window, while the previous validated save remains in .bak.
#if PLATFORM_WINDOWS
    const FString From = FPaths::ConvertRelativePathToFull(Temporary);
    const FString To = FPaths::ConvertRelativePathToFull(Destination);
    return ::MoveFileExW(*From, *To, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    // This demo targets Windows. Refuse to claim equivalent durability on an untested platform.
    return false;
#endif
}

bool StageAndReplace(const FString& Text, const FString& Temporary, const FString& Destination)
{
    return FFileHelper::SaveStringToFile(Text, *Temporary, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)
        && AtomicReplace(Temporary, Destination);
}

TSharedRef<FJsonObject> PointJson(const FVector2D& Point)
{
    const auto Json = MakeShared<FJsonObject>();
    Json->SetNumberField(TEXT("x"), Point.X);
    Json->SetNumberField(TEXT("y"), Point.Y);
    return Json;
}

const TCHAR* ActionName(EMagnetAction Action)
{
    switch (Action)
    {
    case EMagnetAction::Sweep: return TEXT("sweep");
    case EMagnetAction::Aim: return TEXT("aim");
    case EMagnetAction::Dump: return TEXT("dump");
    case EMagnetAction::UI: return TEXT("ui");
    default: return TEXT("none");
    }
}
}

FString FWorkbenchImpl::EncodeSave() const
{
    const auto Json = MakeShared<FJsonObject>();
    Json->SetNumberField(TEXT("save_version"), 1);
    Json->SetBoolField(TEXT("muted"), bMuted);
    Json->SetObjectField(TEXT("snapshot"), SnapshotJson(Model.GetSnapshot()));
    FString Result;
    FJsonSerializer::Serialize(Json, TJsonWriterFactory<>::Create(&Result));
    return Result;
}

bool FWorkbenchImpl::DecodeSave(const FString& Text)
{
    FSalvageSnapshot Snapshot;
    bool bSavedMuted = false;
    FString Error;
    if (!ParseSave(Text, Snapshot, bSavedMuted) || !Model.RestoreSnapshot(Snapshot, Error)) return false;
    bMuted = bSavedMuted;
    // Visual transfers are cosmetic; restored material ownership is already canonical.
    Preview = {};
    Action = EMagnetAction::None;
    AimedRing = INDEX_NONE;
    HoverRing = INDEX_NONE;
    bPendingNext = false;
    PourTimer = 0;
    ForgeTimer = 0;
    DisplayUpgrade = Model.GetUpgradeLevel();
    return true;
}

bool FWorkbenchImpl::Save()
{
    if (SavePath.IsEmpty()) return false;
    FString Error;
    if (!Model.CheckInvariants(Error))
    {
        UE_LOG(LogTemp, Error, TEXT("MAGNET_SAVE_REFUSED: %s"), *Error);
        return false;
    }
    IFileManager& Files = IFileManager::Get();
    if (!Files.MakeDirectory(*FPaths::GetPath(SavePath), true)) return false;
    const FString Text = EncodeSave();
    const FString Temporary = SavePath + TEXT(".tmp");
    const FString Backup = SavePath + TEXT(".bak");
    // Finish writing the candidate before changing either recoverable destination.
    if (!FFileHelper::SaveStringToFile(Text, *Temporary, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        return false;
    FString Written;
    FSalvageSnapshot Verified;
    bool bWrittenMuted = false;
    if (!ReadSaveFile(Temporary, Written) || !ParseSave(Written, Verified, bWrittenMuted)) return false;

    FString Previous;
    if (ReadSaveFile(SavePath, Previous))
    {
        FSalvageSnapshot PreviousSnapshot;
        bool bPreviousMuted = false;
        if (ParseSave(Previous, PreviousSnapshot, bPreviousMuted))
        {
            if (!StageAndReplace(Previous, Backup + TEXT(".tmp"), Backup)) return false;
        }
        else
        {
            // Do not overwrite the valid backup with corrupt/incompatible primary data.
            // Keep the rejected primary too, so a newer-format file is not destroyed.
            const FString Rejected = SavePath + TEXT(".rejected-")
                + FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S-%s"));
            if (!FFileHelper::SaveStringToFile(Previous, *Rejected,
                FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) return false;
        }
    }
    else if (Files.FileExists(*SavePath))
    {
        // Existing but unreadable/oversized data might contain recoverable progress.
        UE_LOG(LogTemp, Warning, TEXT("MAGNET_SAVE_REFUSED: existing primary cannot be read safely"));
        return false;
    }
    const bool bSaved = AtomicReplace(Temporary, SavePath);
    if (!bSaved) UE_LOG(LogTemp, Warning, TEXT("MAGNET_SAVE_FAILED: atomic replacement failed"));
    return bSaved;
}

bool FWorkbenchImpl::Load()
{
    if (SavePath.IsEmpty()) return false;
    FString Text;
    if (ReadSaveFile(SavePath, Text) && DecodeSave(Text))
    {
        UE_LOG(LogTemp, Display, TEXT("MAGNET_SAVE_LOADED profile=%s"), *Profile);
        return true;
    }
    if (ReadSaveFile(SavePath + TEXT(".bak"), Text) && DecodeSave(Text))
    {
        UE_LOG(LogTemp, Warning, TEXT("MAGNET_SAVE_RECOVERED_BACKUP profile=%s"), *Profile);
        return true;
    }
    if (IFileManager::Get().FileExists(*SavePath) || IFileManager::Get().FileExists(*(SavePath + TEXT(".bak"))))
        UE_LOG(LogTemp, Warning, TEXT("MAGNET_SAVE_INVALID: no valid primary or backup for profile=%s"), *Profile);
    return false;
}

void FWorkbenchImpl::WriteTelemetry()
{
    if (!bQA || SavePath.IsEmpty()) return;
    const auto Json = MakeShared<FJsonObject>();
    Json->SetNumberField(TEXT("telemetry_version"), 1);
    Json->SetStringField(TEXT("utc"), FDateTime::UtcNow().ToIso8601());
    Json->SetStringField(TEXT("profile"), Profile);
    Json->SetStringField(TEXT("event"), LastEvent);
    Json->SetObjectField(TEXT("model"), SnapshotJson(Model.GetSnapshot()));
    Json->SetStringField(TEXT("layout_name"), Model.GetLayoutName());
    Json->SetNumberField(TEXT("available"), Model.GetAvailableAmount());
    Json->SetNumberField(TEXT("total"), Model.GetTotalAmount());
    Json->SetNumberField(TEXT("reach"), Model.GetReach());
    Json->SetStringField(TEXT("action"), ActionName(Action));
    Json->SetNumberField(TEXT("hover_ring"), HoverRing);
    Json->SetNumberField(TEXT("aimed_ring"), AimedRing);
    const auto Pull = MakeShared<FJsonObject>();
    Pull->SetNumberField(TEXT("ring_id"), Preview.RingId);
    Pull->SetNumberField(TEXT("epoch"), Preview.Epoch);
    Pull->SetNumberField(TEXT("amount"), Preview.Amount);
    Pull->SetObjectField(TEXT("endpoint"), PointJson(Preview.Endpoint));
    TArray<TSharedPtr<FJsonValue>> PreviewIds;
    for (int32 Id : Preview.PieceIds) PreviewIds.Add(MakeShared<FJsonValueNumber>(Id));
    Pull->SetArrayField(TEXT("piece_ids"), PreviewIds);
    Json->SetObjectField(TEXT("preview"), Pull);
    Json->SetObjectField(TEXT("pointer"), PointJson(Pointer));
    Json->SetObjectField(TEXT("magnet"), PointJson(Magnet));
    Json->SetObjectField(TEXT("raw_world"), PointJson(RawWorld));
    Json->SetBoolField(TEXT("world_hit"), bWorldHit);
    Json->SetBoolField(TEXT("in_tray"), bInTray);
    Json->SetBoolField(TEXT("furnace_hover"), bFurnaceHover);
    Json->SetBoolField(TEXT("paused"), bPaused);
    Json->SetBoolField(TEXT("focused"), bWasFocused);
    Json->SetBoolField(TEXT("muted"), bMuted);
    Json->SetBoolField(TEXT("mouse_was_down"), bMouseWasDown);
    Json->SetBoolField(TEXT("pending_next"), bPendingNext);
    Json->SetBoolField(TEXT("confirm_retry"), bConfirmRetry);
    Json->SetBoolField(TEXT("confirm_new"), bConfirmNew);
    Json->SetNumberField(TEXT("upgrade"), Model.GetUpgradeLevel());
    Json->SetNumberField(TEXT("display_upgrade"), DisplayUpgrade);
    Json->SetNumberField(TEXT("deposits"), Deposits);
    Json->SetNumberField(TEXT("pulls"), Pulls);
    Json->SetNumberField(TEXT("sweeps"), Sweeps);
    Json->SetNumberField(TEXT("last_burst"), LastBurst);
    Json->SetNumberField(TEXT("time"), Time);
    Json->SetNumberField(TEXT("pour_timer"), PourTimer);
    Json->SetNumberField(TEXT("forge_timer"), ForgeTimer);
    Json->SetNumberField(TEXT("notice_timer"), NoticeTimer);
    Json->SetNumberField(TEXT("save_timer"), SaveTimer);
    Json->SetNumberField(TEXT("furnace_pulse"), FurnacePulse);
    Json->SetNumberField(TEXT("frame_average"), FrameAverage);
    Json->SetNumberField(TEXT("loaded_sound_count"), Sounds.Num());
    Json->SetNumberField(TEXT("viewport_width"), Width);
    Json->SetNumberField(TEXT("viewport_height"), Height);
    Json->SetNumberField(TEXT("ui_scale"), UIScale);
    Json->SetStringField(TEXT("notice_title"), NoticeTitle);
    Json->SetStringField(TEXT("notice_body"), NoticeBody);
    TArray<TSharedPtr<FJsonValue>> ScreenPieces;
    if (PC)
    {
        for (const FSalvagePiece& Piece : Model.GetPieces())
        {
            const auto Item = MakeShared<FJsonObject>();
            Item->SetNumberField(TEXT("id"), Piece.Id);
            Item->SetObjectField(TEXT("screen"), PointJson(Project(World(Piece.Position,
                Piece.Kind == EPieceKind::Tangle ? 58 : 13))));
            ScreenPieces.Add(MakeShared<FJsonValueObject>(Item));
        }
        Json->SetObjectField(TEXT("furnace_screen"), PointJson(Project(World({650,0},72))));
    }
    Json->SetArrayField(TEXT("piece_screens"), ScreenPieces);
    TArray<TSharedPtr<FJsonValue>> UI;
    for (const FDemoButton& Button : Buttons)
    {
        const auto Item = MakeShared<FJsonObject>();
        Item->SetNumberField(TEXT("id"), Button.Id);
        Item->SetStringField(TEXT("label"), Button.Label);
        Item->SetBoolField(TEXT("enabled"), Button.Enabled);
        Item->SetObjectField(TEXT("min"), PointJson(Button.Rect.Min));
        Item->SetObjectField(TEXT("max"), PointJson(Button.Rect.Max));
        UI.Add(MakeShared<FJsonValueObject>(Item));
    }
    Json->SetArrayField(TEXT("buttons"), UI);
    FString Text;
    if (!FJsonSerializer::Serialize(Json, TJsonWriterFactory<>::Create(&Text))) return;
    const FString Destination = FPaths::Combine(FPaths::GetPath(SavePath), Profile + TEXT("-telemetry.json"));
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Destination), true);
    StageAndReplace(Text, Destination + TEXT(".tmp"), Destination);
}

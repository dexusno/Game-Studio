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

struct FSaveSettings
{
    bool Muted = false;
    bool Workshop = false;
    bool Receipt = false;
    float Sfx = .8f;
    float Music = .3f;
    FTutorialProgress Tutorial;
};

TArray<TSharedPtr<FJsonValue>> IntArray(const TArray<int32>& Values)
{
    TArray<TSharedPtr<FJsonValue>> Result;
    for (int32 Value : Values) Result.Add(MakeShared<FJsonValueNumber>(Value));
    return Result;
}
TArray<TSharedPtr<FJsonValue>> BoolArray(const TArray<bool>& Values)
{
    TArray<TSharedPtr<FJsonValue>> Result;
    for (bool Value : Values) Result.Add(MakeShared<FJsonValueBoolean>(Value));
    return Result;
}
TSharedRef<FJsonObject> SnapshotJson(const FSalvageSnapshot& S)
{
    const auto Json = MakeShared<FJsonObject>();
    Json->SetNumberField(TEXT("version"), S.Version);
    Json->SetNumberField(TEXT("layout_index"), S.LayoutIndex);
    Json->SetNumberField(TEXT("seed"), S.Seed);
    Json->SetNumberField(TEXT("epoch"), S.Epoch);
    Json->SetNumberField(TEXT("upgrade_level"), S.UpgradeLevel);
    Json->SetNumberField(TEXT("completed_delivery_count"), S.CompletedDeliveryCount);
    Json->SetArrayField(TEXT("upgrade_tiers"), IntArray(S.UpgradeTiers));
    Json->SetNumberField(TEXT("wallet"), S.Wallet);
    Json->SetNumberField(TEXT("xp"), S.XP);
    Json->SetArrayField(TEXT("collected_cores"), IntArray(S.CollectedCores));
    Json->SetArrayField(TEXT("best_banked"), IntArray(S.BestBanked));
    Json->SetArrayField(TEXT("best_heats"), IntArray(S.BestHeats));
    Json->SetArrayField(TEXT("cleared_jobs"), BoolArray(S.ClearedJobs));
    Json->SetArrayField(TEXT("gold_jobs"), BoolArray(S.GoldJobs));
    Json->SetNumberField(TEXT("cargo"), S.Cargo);
    Json->SetNumberField(TEXT("cargo_mass"), S.CargoMass);
    Json->SetNumberField(TEXT("banked"), S.Banked);
    Json->SetNumberField(TEXT("goal"), S.Goal);
    Json->SetNumberField(TEXT("heats_used"), S.HeatsUsed);
    Json->SetNumberField(TEXT("capture_serial"), S.CaptureSerial);
    Json->SetNumberField(TEXT("fuse_elapsed"), S.FuseElapsed);
    Json->SetBoolField(TEXT("delivery_completed"), S.bDeliveryCompleted);
    Json->SetBoolField(TEXT("gold_awarded"), S.bGoldAwarded);
    Json->SetBoolField(TEXT("job_ended"), S.bJobEnded);
    TArray<TSharedPtr<FJsonValue>> Pieces;
    for (const FSalvagePiece& P : S.Pieces)
    {
        const auto Item = MakeShared<FJsonObject>();
        Item->SetNumberField(TEXT("id"), P.Id);
        Item->SetNumberField(TEXT("kind"), static_cast<uint8>(P.Kind));
        Item->SetNumberField(TEXT("material"), static_cast<uint8>(P.Material));
        Item->SetNumberField(TEXT("state"), static_cast<uint8>(P.State));
        Item->SetNumberField(TEXT("x"), P.Position.X);
        Item->SetNumberField(TEXT("y"), P.Position.Y);
        Item->SetNumberField(TEXT("mass"), P.Mass);
        Item->SetNumberField(TEXT("amount"), P.Amount);
        Item->SetNumberField(TEXT("core_id"), P.CoreId);
        Item->SetNumberField(TEXT("capture_order"), P.CaptureOrder);
        Item->SetArrayField(TEXT("links"), IntArray(P.DirectLinks));
        Pieces.Add(MakeShared<FJsonValueObject>(Item));
    }
    Json->SetArrayField(TEXT("pieces"), Pieces);
    return Json;
}
bool Integral(const FJsonObject& Json, const TCHAR* Key, double Min, double Max, double& Result)
{
    const auto Value=Json.TryGetField(Key);
    return Value.IsValid() && Value->Type==EJson::Number && Value->TryGetNumber(Result) && FMath::IsFinite(Result)
        && Result >= Min && Result <= Max && FMath::FloorToDouble(Result) == Result;
}
bool IntField(const FJsonObject& Json, const TCHAR* Key, int32 Min, int32 Max, int32& Result)
{
    double Number = 0;
    if (!Integral(Json, Key, Min, Max, Number)) return false;
    Result = static_cast<int32>(Number); return true;
}
bool BoolField(const FJsonObject& Json, const TCHAR* Key, bool& Result)
{
    const auto Value=Json.TryGetField(Key);
    return Value.IsValid() && Value->Type==EJson::Boolean && Value->TryGetBool(Result);
}
bool FloatField(const FJsonObject& Json, const TCHAR* Key, float Min, float Max, float& Result)
{
    double Number = 0;
    const auto Value=Json.TryGetField(Key);
    if (!Value.IsValid() || Value->Type!=EJson::Number || !Value->TryGetNumber(Number) || !FMath::IsFinite(Number) || Number < Min || Number > Max) return false;
    Result = static_cast<float>(Number); return true;
}
bool ReadInts(const FJsonObject& Json, const TCHAR* Key, int32 Min, int32 Max, int32 MaxCount, TArray<int32>& Out)
{
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Json.TryGetArrayField(Key, Values) || !Values || Values->Num() > MaxCount) return false;
    Out.Reset();
    for (const auto& Value : *Values)
    {
        double Number = 0;
        if (!Value.IsValid() || Value->Type!=EJson::Number || !Value->TryGetNumber(Number) || !FMath::IsFinite(Number)
            || Number < Min || Number > Max || FMath::FloorToDouble(Number) != Number) return false;
        Out.Add(static_cast<int32>(Number));
    }
    return true;
}
bool ReadBools(const FJsonObject& Json, const TCHAR* Key, int32 Count, TArray<bool>& Out)
{
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Json.TryGetArrayField(Key, Values) || !Values || Values->Num() != Count) return false;
    Out.Reset();
    for (const auto& Value : *Values)
    {
        bool Parsed = false; if (!Value.IsValid() || Value->Type!=EJson::Boolean || !Value->TryGetBool(Parsed)) return false; Out.Add(Parsed);
    }
    return true;
}
bool ParseSave(const FString& Text, FSalvageSnapshot& S, FSaveSettings& Settings)
{
    if (Text.Len() > MaxSaveCharacters) return false;
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid()) return false;
    int32 Version = 0;
    if (!IntField(*Root, TEXT("save_version"), 2, 2, Version)
        || !BoolField(*Root, TEXT("muted"), Settings.Muted)
        || !BoolField(*Root, TEXT("workshop"), Settings.Workshop)
        || !BoolField(*Root, TEXT("receipt"), Settings.Receipt)
        || !FloatField(*Root, TEXT("sfx_volume"), 0, 1, Settings.Sfx)
        || !FloatField(*Root, TEXT("music_volume"), 0, 1, Settings.Music)) return false;
    // Older careers have no tutorial object and remain playable without a forced restart.
    if (Root->HasField(TEXT("tutorial")))
    {
        const TSharedPtr<FJsonObject>* T = nullptr;
        int32 Step = 0, Mod = -1; FString TutorialError;
        if (!Root->TryGetObjectField(TEXT("tutorial"), T) || !T || !T->IsValid()
            || !IntField(**T, TEXT("step"), 0, static_cast<int32>(ETutorialStep::Skipped), Step)
            || !BoolField(**T, TEXT("enabled"), Settings.Tutorial.bEnabled)
            || !BoolField(**T, TEXT("risk_learned"), Settings.Tutorial.bRiskLearned)
            || !BoolField(**T, TEXT("risk_held"), Settings.Tutorial.bRiskHeld)
            || !BoolField(**T, TEXT("purchased_mod"), Settings.Tutorial.bPurchasedMod)
            || !IntField(**T, TEXT("mod"), -1, 2, Mod)
            || !FTutorialProgress::ValidateFields(Step, Settings.Tutorial.bEnabled, Settings.Tutorial.bRiskLearned,
                Settings.Tutorial.bRiskHeld, Settings.Tutorial.bPurchasedMod, Mod, TutorialError)) return false;
        Settings.Tutorial.Step=static_cast<ETutorialStep>(Step); Settings.Tutorial.PurchasedMod=Mod;
    }
    const TSharedPtr<FJsonObject>* Object = nullptr;
    if (!Root->TryGetObjectField(TEXT("snapshot"), Object) || !Object || !Object->IsValid()) return false;
    const FJsonObject& Json = **Object;
    double Epoch = 0;
    if (!IntField(Json, TEXT("version"), 2, 2, S.Version)
        || !IntField(Json, TEXT("layout_index"), 0, 5, S.LayoutIndex)
        || !IntField(Json, TEXT("seed"), 1, MAX_int32, S.Seed)
        || !Integral(Json, TEXT("epoch"), 1, MAX_uint32, Epoch)
        || !IntField(Json, TEXT("upgrade_level"), 0, 9, S.UpgradeLevel)
        || !IntField(Json, TEXT("completed_delivery_count"), 0, 1000000, S.CompletedDeliveryCount)
        || !ReadInts(Json, TEXT("upgrade_tiers"), 0, 3, 3, S.UpgradeTiers)
        || !IntField(Json, TEXT("wallet"), 0, 100000000, S.Wallet)
        || !IntField(Json, TEXT("xp"), 0, 100000000, S.XP)
        || !ReadInts(Json, TEXT("collected_cores"), 0, 5, 6, S.CollectedCores)
        || !ReadInts(Json, TEXT("best_banked"), 0, 100000, 6, S.BestBanked)
        || !ReadInts(Json, TEXT("best_heats"), 0, 4, 6, S.BestHeats)
        || !ReadBools(Json, TEXT("cleared_jobs"), 6, S.ClearedJobs)
        || !ReadBools(Json, TEXT("gold_jobs"), 6, S.GoldJobs)
        || !IntField(Json, TEXT("cargo"), 0, 256000, S.Cargo)
        || !IntField(Json, TEXT("cargo_mass"), 0, 1024, S.CargoMass)
        || !IntField(Json, TEXT("banked"), 0, 256000, S.Banked)
        || !IntField(Json, TEXT("goal"), 1, 256000, S.Goal)
        || !IntField(Json, TEXT("heats_used"), 0, 4, S.HeatsUsed)
        || !IntField(Json, TEXT("capture_serial"), 0, 100000000, S.CaptureSerial)
        || !FloatField(Json, TEXT("fuse_elapsed"), 0, 6, S.FuseElapsed)
        || !BoolField(Json, TEXT("delivery_completed"), S.bDeliveryCompleted)
        || !BoolField(Json, TEXT("gold_awarded"), S.bGoldAwarded)
        || !BoolField(Json, TEXT("job_ended"), S.bJobEnded)) return false;
    S.Epoch = static_cast<uint32>(Epoch);
    const TArray<TSharedPtr<FJsonValue>>* Pieces = nullptr;
    if (!Json.TryGetArrayField(TEXT("pieces"), Pieces) || !Pieces || Pieces->IsEmpty() || Pieces->Num() > 256) return false;
    S.Pieces.Reset();
    for (const auto& Value : *Pieces)
    {
        if (!Value.IsValid() || Value->Type != EJson::Object) return false;
        const auto Item = Value->AsObject(); if (!Item.IsValid()) return false;
        FSalvagePiece P; int32 Kind = 0, Material = 0, Ownership = 0;
        if (!IntField(*Item, TEXT("id"), 0, MAX_int32, P.Id)
            || !IntField(*Item, TEXT("kind"), 0, 1, Kind)
            || !IntField(*Item, TEXT("material"), 0, 4, Material)
            || !IntField(*Item, TEXT("state"), 0, 3, Ownership)
            || !IntField(*Item, TEXT("mass"), 1, 1000, P.Mass)
            || !IntField(*Item, TEXT("amount"), 0, 1000, P.Amount)
            || !IntField(*Item, TEXT("core_id"), -1, 5, P.CoreId)
            || !IntField(*Item, TEXT("capture_order"), 0, 100000000, P.CaptureOrder)
            || !Item->TryGetNumberField(TEXT("x"), P.Position.X)
            || !Item->TryGetNumberField(TEXT("y"), P.Position.Y)
            || !ReadInts(*Item, TEXT("links"), 0, MAX_int32, 255, P.DirectLinks)) return false;
        P.Kind = static_cast<EPieceKind>(Kind); P.Material = static_cast<EMaterial>(Material); P.State = static_cast<EPieceState>(Ownership);
        S.Pieces.Add(MoveTemp(P));
    }
    FString Error;
    return FSalvageModel::ValidateSnapshot(S, Error);
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
    Json->SetNumberField(TEXT("save_version"), 2);
    Json->SetBoolField(TEXT("workshop"), bWorkshop);
    Json->SetBoolField(TEXT("receipt"), bReceipt);
    Json->SetNumberField(TEXT("sfx_volume"), SfxVolume);
    Json->SetNumberField(TEXT("music_volume"), MusicVolume);
    Json->SetBoolField(TEXT("muted"), bMuted);
    const auto Teaching=MakeShared<FJsonObject>();
    Teaching->SetNumberField(TEXT("step"),static_cast<int32>(Tutorial.Step));
    Teaching->SetBoolField(TEXT("enabled"),Tutorial.bEnabled);
    Teaching->SetBoolField(TEXT("risk_learned"),Tutorial.bRiskLearned);
    Teaching->SetBoolField(TEXT("risk_held"),Tutorial.bRiskHeld);
    Teaching->SetBoolField(TEXT("purchased_mod"),Tutorial.bPurchasedMod);
    Teaching->SetNumberField(TEXT("mod"),Tutorial.PurchasedMod);
    Json->SetObjectField(TEXT("tutorial"),Teaching);
    Json->SetObjectField(TEXT("snapshot"), SnapshotJson(Model.GetSnapshot()));
    FString Result;
    FJsonSerializer::Serialize(Json, TJsonWriterFactory<>::Create(&Result));
    return Result;
}

bool FWorkbenchImpl::DecodeSave(const FString& Text)
{
    FSalvageSnapshot Snapshot;
    FSaveSettings Settings;
    FString Error;
    if (!ParseSave(Text, Snapshot, Settings) || !Model.RestoreSnapshot(Snapshot, Error)) return false;
    Tutorial = Settings.Tutorial;
    bMuted = Settings.Muted;
    bWorkshop = Settings.Workshop;
    bReceipt = Settings.Receipt;
    SfxVolume = Settings.Sfx;
    MusicVolume = Settings.Music;
    LastBank = {};
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
    if (!Model.CheckInvariants(Error) || !Tutorial.Validate(Error))
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
    FSaveSettings WrittenSettings;
    if (!ReadSaveFile(Temporary, Written) || !ParseSave(Written, Verified, WrittenSettings)) return false;

    FString Previous;
    if (ReadSaveFile(SavePath, Previous))
    {
        FSalvageSnapshot PreviousSnapshot;
        FSaveSettings PreviousSettings;
        if (ParseSave(Previous, PreviousSnapshot, PreviousSettings))
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
    Json->SetNumberField(TEXT("telemetry_version"), 2);
    Json->SetNumberField(TEXT("tutorial_step"),static_cast<int32>(Tutorial.Step));
    Json->SetBoolField(TEXT("tutorial_enabled"),Tutorial.bEnabled);
    Json->SetBoolField(TEXT("tutorial_risk_hold"),Tutorial.bRiskHeld);
    Json->SetBoolField(TEXT("tutorial_practice"),bTutorialPractice);
    Json->SetNumberField(TEXT("capacity"), Model.GetCapacity());
    Json->SetNumberField(TEXT("fuse_remaining"), Model.GetFuseRemaining());
    Json->SetBoolField(TEXT("unsafe"), Model.IsCargoUnsafe());
    Json->SetBoolField(TEXT("workshop"), bWorkshop);
    Json->SetBoolField(TEXT("receipt"), bReceipt);
    Json->SetBoolField(TEXT("precision"), bPrecision);
    Json->SetNumberField(TEXT("operator_level"), Model.GetPlayerLevel());
    Json->SetNumberField(TEXT("job_earnings"), Model.GetJobEarnings());
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

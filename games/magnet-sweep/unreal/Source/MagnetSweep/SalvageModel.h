#pragma once
#include "CoreMinimal.h"

namespace MagnetSweep
{
enum class EPieceKind : uint8 { Loose, Tangle };
enum class EPieceState : uint8 { Available, Cargo, Banked, Lost };
enum class EMaterial : uint8 { Iron, Copper, Alloy, Core, HotCell };
enum class EUpgrade : uint8 { Capacity, Coil, Stabilizer };

struct FSalvagePiece
{
    int32 Id = INDEX_NONE;
    EPieceKind Kind = EPieceKind::Loose;
    EMaterial Material = EMaterial::Iron;
    FVector2D Position = FVector2D::ZeroVector;
    int32 Mass = 2;
    int32 Amount = 4; // Smelt value; kept separate from mass.
    int32 CoreId = INDEX_NONE;
    int32 CaptureOrder = 0;
    EPieceState State = EPieceState::Available;
    TArray<int32> DirectLinks;
};

struct FRecoveryResult
{
    TArray<int32> PieceIds;
    int32 Amount = 0;
    int32 Mass = 0;
    bool bCapacityRefused = false;
    bool Succeeded() const { return !PieceIds.IsEmpty(); }
};

struct FSpillResult
{
    TArray<int32> ReleasedPieceIds;
    TArray<int32> DestroyedPieceIds;
    int32 LostValue = 0;
    bool bTripped = false;
    bool Changed() const { return !ReleasedPieceIds.IsEmpty() || !DestroyedPieceIds.IsEmpty(); }
};

struct FBankResult
{
    TArray<int32> PieceIds;
    TArray<int32> NewCoreIds;
    int32 Amount = 0;
    int32 Mass = 0;
    int32 CashAwarded = 0;
    int32 XPAwarded = 0;
    int32 PreviousPlayerLevel = 1;
    int32 NewPlayerLevel = 1;
    bool bCompletedNow = false;
    bool bGoldNow = false;
    int32 PreviousUpgradeLevel = 0;
    int32 NewUpgradeLevel = 0;
    bool ForgedUpgrade() const { return NewUpgradeLevel > PreviousUpgradeLevel; }
};

struct FJobDefinition
{
    FString Name;
    int32 Quota = 120;
    int32 GoldGoal = 180;
    int32 CompletionBonus = 80;
    int32 GoldBonus = 40;
    int32 RequiredLevel = 1;
    int32 IronCount = 24;
    int32 CopperCount = 8;
    int32 AlloyCount = 2;
    int32 CellCount = 2;
};

// Version 2 is an independent career. The rejected demo's v1 ledger is not migrated.
// Every field is persisted. Runtime owns presentation and motion; authoritative awards live here.
struct FSalvageSnapshot
{
    int32 Version = 2;
    int32 LayoutIndex = 0;
    int32 Seed = 12092026;
    uint32 Epoch = 1;
    int32 UpgradeLevel = 0; // Sum of purchased tiers, retained for display compatibility.
    int32 CompletedDeliveryCount = 0;
    TArray<int32> UpgradeTiers = {0, 0, 0};
    int32 Wallet = 0;
    int32 XP = 0;
    TArray<int32> CollectedCores;
    TArray<int32> BestBanked = {0, 0, 0, 0, 0, 0};
    TArray<int32> BestHeats = {0, 0, 0, 0, 0, 0};
    TArray<bool> ClearedJobs = {false, false, false, false, false, false};
    TArray<bool> GoldJobs = {false, false, false, false, false, false};
    TArray<FSalvagePiece> Pieces;
    int32 Cargo = 0;
    int32 CargoMass = 0;
    int32 Banked = 0;
    int32 Goal = 120;
    int32 HeatsUsed = 0;
    int32 CaptureSerial = 0;
    float FuseElapsed = 0;
    bool bDeliveryCompleted = false;
    bool bGoldAwarded = false;
    bool bJobEnded = false;
};

// Legacy preview type remains only to keep scene/persistence integration source-compatible.
// Ring-corridor capture is removed; new runtime uses CapturePieces and visible group membership.
struct FPullSelection
{
    int32 RingId = INDEX_NONE;
    FVector2D Endpoint = FVector2D::ZeroVector;
    TArray<int32> PieceIds;
    int32 Amount = 0;
    uint32 Epoch = 0;
    bool IsValid() const { return RingId != INDEX_NONE && !PieceIds.IsEmpty(); }
};

class FSalvageModel
{
public:
    static constexpr int32 SnapshotVersion = 2;
    static constexpr int32 LayoutCount = 6;
    static constexpr int32 HeatsPerJob = 4;
    static constexpr float TrayMinX = -500.0f, TrayMaxX = 500.0f;
    static constexpr float TrayMinY = -310.0f, TrayMaxY = 310.0f;
    static constexpr float PullRadius = 90.0f, SweepRadius = 85.0f, RingHitRadius = 43.0f;

    FSalvageModel();
    const TArray<FSalvagePiece>& GetPieces() const { return State.Pieces; }
    const FSalvagePiece* FindPiece(int32 Id) const;
    static const FJobDefinition& JobDefinition(int32 Index);
    const FJobDefinition& GetJob() const { return JobDefinition(State.LayoutIndex); }
    static FString MaterialName(EMaterial Material);
    static FString CoreName(int32 CoreId);
    FString PieceName(int32 Id) const;
    int32 GetCargo() const { return State.Cargo; }
    int32 GetCargoMass() const { return State.CargoMass; }
    int32 GetBanked() const { return State.Banked; }
    int32 GetGoal() const { return State.Goal; }
    int32 GetGoldGoal() const { return GetJob().GoldGoal; }
    int32 GetHeatsRemaining() const { return HeatsPerJob - State.HeatsUsed; }
    int32 GetHeatsUsed() const { return State.HeatsUsed; }
    int32 GetWallet() const { return State.Wallet; }
    int32 GetXP() const { return State.XP; }
    int32 GetJobEarnings() const { return State.Banked + (State.bDeliveryCompleted ? GetJob().CompletionBonus : 0) + (State.bGoldAwarded ? GetJob().GoldBonus : 0); }
    int32 GetPlayerLevel() const;
    static int32 LevelForXP(int32 XP);
    static int32 XPForLevel(int32 Level);
    int32 GetUpgradeTier(EUpgrade Upgrade) const;
    int32 GetUpgradePrice(EUpgrade Upgrade) const;
    bool CanPurchaseUpgrade(EUpgrade Upgrade, FString& Reason) const;
    bool PurchaseUpgrade(EUpgrade Upgrade);
    int32 GetCapacity() const { return 24 + 8 * GetUpgradeTier(EUpgrade::Capacity); }
    int32 GetMaxCaptureMass() const { return GetCapacity() * 3 / 2; }
    float GetFieldRadius() const { return 110.f + 35.f * GetUpgradeTier(EUpgrade::Coil); }
    float GetForceScale() const { return 1.f + .25f * GetUpgradeTier(EUpgrade::Coil); }
    float GetFuseDuration() const { return 3.f + GetUpgradeTier(EUpgrade::Stabilizer); }
    float GetFuseElapsed() const { return State.FuseElapsed; }
    float GetFuseRemaining() const { return FMath::Max(0.f, GetFuseDuration() - State.FuseElapsed); }
    bool HasHotCell() const;
    bool IsCargoUnsafe() const { return State.CargoMass > GetCapacity() || HasHotCell(); }
    const FSalvagePiece* GetAtRiskPiece() const;
    bool IsJobFailed() const;
    bool IsJobEnded() const { return State.bJobEnded; }
    bool IsDeliveryCompleted() const { return State.bDeliveryCompleted; }
    bool IsGoldAwarded() const { return State.bGoldAwarded; }
    bool IsJobUnlocked(int32 Index) const;
    bool IsJobCleared(int32 Index) const;
    bool IsJobGold(int32 Index) const;
    const TArray<int32>& GetCollectedCores() const { return State.CollectedCores; }
    int32 GetUpgradeLevel() const { return State.UpgradeLevel; }
    int32 GetLayoutIndex() const { return State.LayoutIndex; }
    int32 GetSeed() const { return State.Seed; }
    int32 GetCompletedDeliveryCount() const { return State.CompletedDeliveryCount; }
    uint32 GetEpoch() const { return State.Epoch; }
    FString GetLayoutName() const { return GetJob().Name; }
    int32 GetAvailableAmount() const;
    int32 GetTotalAmount() const;

    // Available connected components capture atomically; at most 150% capacity may be attached.
    TArray<int32> GetCaptureGroup(int32 Id) const;
    bool CanCaptureGroup(int32 Id) const;
    FRecoveryResult CapturePieces(const TArray<int32>& Ids);
    bool MoveAvailablePiece(int32 Id, const FVector2D& Position);
    // Drop the entire unbanked haul, stable or unsafe. All pieces remain recoverable;
    // connected components retain their shape and cells are separated for precision pickup.
    FSpillResult VentCargo(const FVector2D& Origin = FVector2D::ZeroVector);
    FSpillResult TriggerOverload(const FVector2D& Origin = FVector2D::ZeroVector);
    FSpillResult AdvanceRisk(float Delta, const FVector2D& Origin = FVector2D::ZeroVector);
    bool CanSmelt(FString& Reason) const;
    FBankResult BankCargo();
    // StartJob refuses to silently abandon an active attempt. Explicit Retry/Abandon are separate.
    bool StartJob(int32 Index, int32 Seed);
    void RetryJob(bool bNewSeed = false);
    bool FinishJob(); // requires quota and no cargo
    void AbandonJob(); // only unbanked cargo is forfeited; career ledger stays safe

    FSalvageSnapshot GetSnapshot() const { return State; }
    bool RestoreSnapshot(const FSalvageSnapshot& Snapshot, FString& OutError);
    bool CheckInvariants(FString& OutError) const;
    static bool ValidateSnapshot(const FSalvageSnapshot& Snapshot, FString& OutError);
    static bool IsInsideTray(const FVector2D& Position);
    static FVector2D ClampToTray(const FVector2D& Position);
    static bool IsInsideCapsule(const FVector2D& Point, const FVector2D& Start, const FVector2D& End, float Radius);

    // Old input entry points are inert/limited during integration, never unlimited captures.
    int32 FindAvailableRing(const FVector2D& Position) const;
    FRecoveryResult SweepAt(const FVector2D& Position);
    FPullSelection PreviewPull(int32 RingId, const FVector2D& DesiredEndpoint) const;
    FRecoveryResult CommitPull(const FPullSelection& Preview);
    void ResetLayout() { RetryJob(); }
    bool AdvanceLayout();
    bool HasBreakaway() const { return GetUpgradeTier(EUpgrade::Coil) > 0; }
    float GetReach() const { return GetFieldRadius(); }

private:
    FSalvageSnapshot State;
    void BuildLayout(int32 Index, int32 Seed);
    void IncrementEpoch();
    void RecountCargo();
    void PlaceDroppedCargo(const FVector2D& Origin, TArray<int32>& OutIds);
    void UpdateFailure();
};
}

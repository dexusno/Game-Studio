#pragma once

#include "CoreMinimal.h"

// Pure gameplay state. Rendering/animation never awards material and input modes live in the controller.
namespace MagnetSweep
{
enum class EPieceKind : uint8 { Loose, Tangle };
enum class EPieceState : uint8 { Available, Cargo, Banked };

struct FSalvagePiece
{
    int32 Id = INDEX_NONE;
    EPieceKind Kind = EPieceKind::Loose;
    FVector2D Position = FVector2D::ZeroVector;
    int32 Amount = 1;
    EPieceState State = EPieceState::Available;
    TArray<int32> DirectLinks;
};

struct FPullSelection
{
    int32 RingId = INDEX_NONE;
    FVector2D Endpoint = FVector2D::ZeroVector;
    TArray<int32> PieceIds;
    int32 Amount = 0;
    uint32 Epoch = 0;
    bool IsValid() const { return RingId != INDEX_NONE && !PieceIds.IsEmpty(); }
};

struct FRecoveryResult
{
    TArray<int32> PieceIds;
    int32 Amount = 0;
    bool Succeeded() const { return Amount > 0; }
};

struct FBankResult
{
    TArray<int32> PieceIds;
    int32 Amount = 0;
    bool bCompletedNow = false;
    int32 PreviousUpgradeLevel = 0;
    int32 NewUpgradeLevel = 0;
    bool ForgedUpgrade() const { return NewUpgradeLevel > PreviousUpgradeLevel; }
};

// Persist every member and every piece. No transient gesture or animation belongs in the save.
// RestoreSnapshot is transactional: malformed data leaves the current model untouched.
struct FSalvageSnapshot
{
    int32 Version = 1;
    int32 LayoutIndex = 0;
    uint32 Epoch = 1;
    int32 UpgradeLevel = 0;
    int32 CompletedDeliveryCount = 0;
    TArray<FSalvagePiece> Pieces;
    int32 Cargo = 0;
    int32 Banked = 0;
    int32 Goal = 100;
    bool bDeliveryCompleted = false;
};

class FSalvageModel
{
public:
    static constexpr int32 SnapshotVersion = 1;
    static constexpr int32 LayoutCount = 2;
    static constexpr float TrayMinX = -500.0f;
    static constexpr float TrayMaxX = 500.0f;
    static constexpr float TrayMinY = -310.0f;
    static constexpr float TrayMaxY = 310.0f;
    static constexpr float PullRadius = 90.0f;
    static constexpr float SweepRadius = 85.0f;
    static constexpr float RingHitRadius = 43.0f;

    FSalvageModel();

    const TArray<FSalvagePiece>& GetPieces() const { return State.Pieces; }
    const FSalvagePiece* FindPiece(int32 Id) const;
    int32 FindAvailableRing(const FVector2D& Position) const;
    int32 GetCargo() const { return State.Cargo; }
    int32 GetBanked() const { return State.Banked; }
    int32 GetGoal() const { return State.Goal; }
    int32 GetUpgradeLevel() const { return State.UpgradeLevel; }
    int32 GetLayoutIndex() const { return State.LayoutIndex; }
    int32 GetCompletedDeliveryCount() const { return State.CompletedDeliveryCount; }
    uint32 GetEpoch() const { return State.Epoch; }
    bool IsDeliveryCompleted() const { return State.bDeliveryCompleted; }
    bool HasBreakaway() const { return State.UpgradeLevel >= 1; }
    float GetReach() const { return State.UpgradeLevel >= 2 ? 720.0f : 440.0f; }
    int32 GetAvailableAmount() const;
    int32 GetTotalAmount() const;
    FString GetLayoutName() const;

    // Sweep collects loose material only. Caller resolves press context once, outside this model.
    FRecoveryResult SweepAt(const FVector2D& Position);
    FPullSelection PreviewPull(int32 RingId, const FVector2D& DesiredEndpoint) const;
    FRecoveryResult CommitPull(const FPullSelection& Preview);
    FBankResult BankCargo();
    // Explicit retry discards the current attempt only, retaining all earned rig improvements.
    void ResetLayout();
    // Must complete and deliberately bank surplus first; does not silently discard cargo.
    bool AdvanceLayout();

    FSalvageSnapshot GetSnapshot() const { return State; }
    bool RestoreSnapshot(const FSalvageSnapshot& Snapshot, FString& OutError);
    bool CheckInvariants(FString& OutError) const;
    static bool ValidateSnapshot(const FSalvageSnapshot& Snapshot, FString& OutError);
    static bool IsInsideTray(const FVector2D& Position);
    static FVector2D ClampToTray(const FVector2D& Position);
    static bool IsInsideCapsule(const FVector2D& Point, const FVector2D& Start,
        const FVector2D& End, float Radius);

private:
    FSalvageSnapshot State;
    void BuildLayout(int32 LayoutIndex);
    void IncrementEpoch();
    FRecoveryResult Recover(const TArray<int32>& PieceIds);
};
}

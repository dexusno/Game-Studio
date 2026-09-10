#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DBShieldChecks.generated.h"

class ADBGameMode;
class ADBCharacter;
class ADBEnemy;
class ADBThrownShield;
class ADBProjectile;
enum class EDBShieldPieceState : uint8;

/** Staged physical-shield checks, advanced by ordinary world ticks. Never ticks the world itself. */
UCLASS()
class DREAMBOUND_API ADBShieldCheckRunner : public AActor
{
    GENERATED_BODY()
public:
    ADBShieldCheckRunner();
    bool Initialize(ADBGameMode& InMode);
    virtual void Tick(float DeltaSeconds) override;

private:
    enum class EStep : uint8
    {
        Prepare, HalfCharge, LaunchRecovery, Paused, SecondBlock, FirstRebuild, LastRebuild,
        FirstPartial, SecondReady, CatchDuringCharge, RecallSurvivors,
        CoreStrike, EliteWait, EliteCharge, EliteContact, FrostCharge, FrostOutbound, FrostReturn,
        FoldIdle, FoldGuard, FoldReturn, FiveCharge, FiveContact, FullCharge, FullContact,
        MeleeFirst, MeleeSecond, MeleeThird, MeleeCover, Storm, Route, FirstEncounter, FirstPractice, SecondEncounter, SecondPractice,
        SaveCharge, Journal, FinalEncounter, Victory, Finished
    };
    enum class EMovementStep : uint8
    {
        Settle, Idle, Walk, Sprint, RecoverHeld, ResumeSprint, RejectLowSprint,
        WallSettle, WallApproach, WallHold, DashSettle, OpenDash, DashCooldown,
        WallDashSettle, WallDash, PauseSettle, PauseSprint, PauseDash, Paused,
        Resume, ResetDash, ResetWait
    };
    struct FMovementProbe
    {
        EMovementStep Step = EMovementStep::Settle;
        FVector Start = FVector::ZeroVector;
        FVector Last = FVector::ZeroVector;
        FVector Input = FVector::ZeroVector;
        FVector HeldPoint = FVector::ZeroVector;
        float Age = 0.f;
        float Elapsed = 0.f;
        float StartStamina = 0.f;
        float Distance = 0.f;
        float PeakSpeed = 0.f;
        float MaxZ = 0.f;
        float SampleDistance = 0.f;
        float SampleSeconds = 0.f;
        float FirstRecovery = -1.f;
        float SavedCooldown = 0.f;
        bool bIdleGood = false;
        bool bDelayGood = false;
        bool bFreshSprintGood = false;
        bool bCooldownBlocked = false;
        bool bHeldPointSet = false;
        bool bPauseGood = false;
        bool bResetStarted = false;
    };
    struct FCheck { FString Scenario; FString Id; bool bPassed = false; FString Detail; };
    struct FJournalCopy { FString Slot; bool bExisted = false; TArray<uint8> Bytes; };

    UPROPERTY() TObjectPtr<ADBGameMode> Mode;
    UPROPERTY() TObjectPtr<ADBCharacter> Player;
    UPROPERTY() TObjectPtr<ADBEnemy> Target;
    UPROPERTY() TObjectPtr<ADBEnemy> OtherTarget;
    UPROPERTY() TObjectPtr<ADBEnemy> BlockedBlastTarget;
    UPROPERTY() TObjectPtr<AActor> Cover;
    UPROPERTY() TArray<TObjectPtr<AActor>> FixtureActors;
    TWeakObjectPtr<ADBThrownShield> Flight;
    TWeakObjectPtr<ADBProjectile> Bolt;
    TArray<FCheck> Checks;
    TArray<FJournalCopy> JournalCopies;
    FString Scenario = TEXT("isolated_setup");
    EStep Step = EStep::Prepare;
    FVector Origin = FVector(0, 0, 10000);
    FVector LastFlightPoint = FVector::ZeroVector;
    FVector PausedPoint = FVector::ZeroVector;
    FVector MoveDestination = FVector::ZeroVector;
    float PhaseAge = 0.f;
    float TargetBefore = 0.f;
    float OutwardHealth = 0.f;
    float PartialVolleyDamage = 0.f;
    float FirstMeleeDamage = 0.f;
    float SecondMeleeDamage = 0.f;
    float FoldedSpan = 0.f;
    float HealthBefore = 0.f;
    float IntegrityBefore = 0.f;
    float MaxFlightStep = 0.f;
    float LastCatchDistance = 0.f;
    int32 BankBefore = 0;
    int32 OriginalSeed = 0;
    int32 MoveSamples = 0;
    int32 FirstLost = INDEX_NONE;
    int32 SecondLost = INDEX_NONE;
    int32 CaughtPiece = INDEX_NONE;
    float PausedProgress = 0.f;
    FString OriginalLayout;
    bool bSawEliteTell = false;
    double StartedAt = 0;
    double PhaseStartedAt = 0;
    bool bInitialized = false;
    bool bFinished = false;
    bool bHaveJournalCopies = false;
    bool bAborted = false;
    bool bSawReturnTravel = false;
    bool bMovementCheck = false;
    FMovementProbe MovementProbe;

    void Go(EStep Next);
    bool Check(bool bGood, const TCHAR* Id, const FString& Detail = FString());
    void Finish(bool bAbort = false);
    void WriteResult(bool bComplete) const;
    void ResetPlayer(FVector Location, bool bClearUpgrades = true);
    void Aim(FRotator Rotation);
    void PlacePlayer(FVector Location);
    AActor* MakeBox(FVector Center, FVector Extent);
    ADBEnemy* MakeTarget(FVector Location);
    ADBProjectile* FireBolt(FVector Start, FVector Direction);
    bool StartEncounter(int32 Index, int32 ExpectedCount);
    bool ClearEncounter(int32 Index);
    bool TakeReward(FName Id, int32 Room);
    bool CheckPieceOwnership(const TCHAR* Id);
    int32 FindPiece(EDBShieldPieceState State) const;
    bool CheckRouteGeometry();
    bool CheckPractice(FName Id, int32 Room);
    void DestroyFixtures();
    void GoMovement(EMovementStep Next);
    void TickMovement(float DeltaSeconds);
    void StageWalkingPlayer(FVector Location);
};

/** Call only from the recovery-slice -DBVerify branch, then return without running legacy checks/exiting. */
bool DBStartPhysicalShieldChecks(ADBGameMode& Mode);

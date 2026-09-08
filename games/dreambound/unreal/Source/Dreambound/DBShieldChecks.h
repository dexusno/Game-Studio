#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DBShieldChecks.generated.h"

class ADBGameMode;
class ADBCharacter;
class ADBEnemy;
class ADBThrownShield;
class ADBProjectile;

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
        Prepare, Strike, Charge, PausedFlight, Outward, Lodged, Return, CatchRecovery,
        CoverCharge, CoverOutward, CoverMove, CoverReturn,
        AnchorCharge, AnchorOutward, AnchorFront, AnchorFlank, AnchorRear,
        FirstEncounter, SecondEncounter, SaveCharge, Journal, FinalEncounter, Victory, Finished
    };
    struct FCheck { FString Scenario; FString Id; bool bPassed = false; FString Detail; };
    struct FJournalCopy { FString Slot; bool bExisted = false; TArray<uint8> Bytes; };

    UPROPERTY() TObjectPtr<ADBGameMode> Mode;
    UPROPERTY() TObjectPtr<ADBCharacter> Player;
    UPROPERTY() TObjectPtr<ADBEnemy> Target;
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
    float HealthBefore = 0.f;
    float IntegrityBefore = 0.f;
    float MaxFlightStep = 0.f;
    float LastCatchDistance = 0.f;
    int32 BankBefore = 0;
    int32 OriginalSeed = 0;
    int32 MoveSamples = 0;
    double StartedAt = 0;
    double PhaseStartedAt = 0;
    bool bInitialized = false;
    bool bFinished = false;
    bool bHaveJournalCopies = false;
    bool bAborted = false;
    bool bSawReturnTravel = false;

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
    void SampleReturn();
    void BeginReturn();
    bool CheckCatch(const TCHAR* Id);
    bool StartEncounter(int32 Index, int32 ExpectedCount);
    bool ClearEncounter(int32 Index);
    bool TakeReward(FName Id, int32 ExpectedNextPhase);
    void DestroyFixtures();
};

/** Call only from the recovery-slice -DBVerify branch, then return without running legacy checks/exiting. */
bool DBStartPhysicalShieldChecks(ADBGameMode& Mode);

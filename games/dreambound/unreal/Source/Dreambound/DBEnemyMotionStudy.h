#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DBEnemyMotionStudy.generated.h"

class ADBGameMode;
class ADBEnemy;
class ADBCharacter;
class ACameraActor;
class ADBProjectile;

/** Opt-in actual-AI visual study. Never spawned during ordinary player runs. */
UCLASS()
class DREAMBOUND_API ADBEnemyMotionStudy : public AActor
{
    GENERATED_BODY()
public:
    ADBEnemyMotionStudy();
    virtual void Tick(float DeltaSeconds) override;
protected:
    virtual void BeginPlay() override;
private:
    UPROPERTY() TObjectPtr<ADBEnemy> Creature;
    UPROPERTY() TObjectPtr<ADBCharacter> Player;
    UPROPERTY() TObjectPtr<ACameraActor> Camera;
    UPROPERTY() TObjectPtr<ADBGameMode> Mode;
    TWeakObjectPtr<ADBProjectile> ObservedProjectile;
    FVector LastProjectilePosition = FVector::ZeroVector;
    bool bProjectileObserved = false, bImpactObserved = false;
    FVector Center = FVector::ZeroVector;
    FString KindName, ViewName, Output, Samples, FailureReason;
    TMap<FString,int32> PhaseSamples;
    float Warmup = 0.f, Time = 0.f;
    float Duration = 32.f;
    float PlayerHealthBeforeRestore = 0.f;
    float CameraDistance = 0.f;
    double StartedAt = 0;
    double AudioStartedAt = 0, LastAudioFrameAt = -1, FinishedAt = 0;
    double PauseStartedAt = 0;
    FDelegateHandle ScreenshotHandle;
    FString PendingAudioFrame;
    int32 Frame = 0, MissedReadbacks = 0;
    int32 BossCloseTargetFrame = INDEX_NONE;
    bool bStarted = false, bFinished = false, bFrontHit = false, bSideHit = false;
    bool bDeathTriggered = false;
    bool bCameraObstructionAdjusted = false;
    bool bFootMarkers = false;
    bool bPassiveIdle = false;
    bool bAudioCapture = false, bPauseStudy = false, bPauseTriggered = false, bStudyPaused = false;
    void UpdateCamera(float DeltaSeconds);
    void RecordFrame(float DeltaSeconds);
    void SaveAudioFrame(int32 Width, int32 Height, const TArray<FColor>& Pixels);
    void Finish(bool bAborted);
};

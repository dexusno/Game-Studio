#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DBTypes.h"
#include "DBEnemy.generated.h"

class ADBCharacter;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPoseableMeshComponent;
class USceneComponent;
class UStaticMeshComponent;
class USoundBase;
class UAudioComponent;
class UPointLightComponent;

UENUM(BlueprintType)
enum class EDBEnemyPhase : uint8 { Dormant, Approach, Telegraph, Attack, Recovery, Staggered, Dead };

/** Read-only animation capture data. Targets/ankles are world-space centimetres. */
struct FDBEnemyAnimationDebug
{
    float speed_cm_s = 0.f;
    float visible_yaw = 0.f;
    float movement_blend = 0.f;
    FVector left_target_world = FVector::ZeroVector;
    FVector right_target_world = FVector::ZeroVector;
    FVector left_foot_world = FVector::ZeroVector;
    FVector right_foot_world = FVector::ZeroVector;
    FVector left_ground_normal = FVector::UpVector;
    FVector right_ground_normal = FVector::UpVector;
    FVector right_hand_world = FVector::ZeroVector;
    FVector left_hand_world = FVector::ZeroVector;
    FVector left_hand_target_world = FVector::ZeroVector;
    FVector right_hand_target_world = FVector::ZeroVector;
    FVector left_claw_contact_world = FVector::ZeroVector;
    FVector right_claw_contact_world = FVector::ZeroVector;
    FVector right_shoulder_world = FVector::ZeroVector;
    FVector facing_forward = FVector::ForwardVector;
    FVector pelvis_world = FVector::ZeroVector;
    FVector head_world = FVector::ZeroVector;
    bool left_planted = false;
    bool right_planted = false;
    float left_reach_error_cm = 0.f;
    float right_reach_error_cm = 0.f;
    float left_hand_reach_error_cm = 0.f;
    float right_hand_reach_error_cm = 0.f;
    float parent_unit_scale = 1.f;
    float attack_elapsed = 0.f;
    bool pounce_blocked = false;
    float blocked_pounce_elapsed = 0.f;
    FString attack_name;
};

/** A room-bound physical opponent. Movement never requires a baked navigation mesh. */
UCLASS()
class DREAMBOUND_API ADBEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    ADBEnemy();
    virtual void Tick(float DeltaSeconds) override;

    void Configure(EDBEnemyKind InKind, int32 InRoomId, float Difficulty);
    void SetArenaBounds(FVector Center, FVector2D HalfSize);
    void ApplyCombatHit(const FDBHit& Hit);
    /** True consumes this stance. The caller destroys only its own outbound piece. */
    bool TryInterceptShieldPiece(FVector IncomingDirection);
    FDBEnemyAnimationDebug GetAnimationDebugState() const;

    UPROPERTY(BlueprintReadOnly, Category="Combat") EDBEnemyKind Kind = EDBEnemyKind::Melee;
    UPROPERTY(BlueprintReadOnly, Category="Combat") int32 RoomId = 0;
    UPROPERTY(BlueprintReadOnly, Category="Combat") bool bDead = false;
    UPROPERTY(BlueprintReadOnly, Category="Combat") float Health = 220.f;
    UPROPERTY(BlueprintReadOnly, Category="Combat") float MaxHealth = 220.f;
    UPROPERTY(BlueprintReadOnly, Category="Combat") EDBEnemyPhase Phase = EDBEnemyPhase::Dormant;
    UPROPERTY(BlueprintReadOnly, Category="Combat") FString Telegraph;
    UPROPERTY(BlueprintReadOnly, Category="Combat") float TellTime = 0.f;
    UPROPERTY(BlueprintReadOnly, Category="Combat") float TellDuration = 0.f;
    UPROPERTY(BlueprintReadOnly, Category="Combat") bool bAimLocked = false;
    UPROPERTY(BlueprintReadOnly, Category="Combat") bool bVulnerable = false;
    UPROPERTY(BlueprintReadOnly, Category="Combat") FVector TellTarget = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category="Combat") float TellRadius = 0.f;
    UPROPERTY(BlueprintReadOnly, Category="Combat") int32 ChillStacks = 0;
    UPROPERTY(BlueprintReadOnly, Category="Combat") int32 StormMarks = 0;
    UPROPERTY(BlueprintReadOnly, Category="Combat") float BurnRemaining = 0.f;
    UPROPERTY(BlueprintReadOnly, Category="Combat") bool bRepositioning = false;
    UPROPERTY(BlueprintReadOnly, Category="Combat") bool bShieldInterceptionReady = false;
    UPROPERTY(BlueprintReadWrite, Category="Combat") bool bPracticeTarget = false;

protected:
    virtual void BeginPlay() override;

private:
    enum class EAttack : uint8 { None, Swing, Bolt, Lunge, Salvo, Slam, Ground, Intercept };

    UPROPERTY() TObjectPtr<USceneComponent> VisualRoot;
    UPROPERTY() TObjectPtr<UPoseableMeshComponent> OrganicMesh;
    UPROPERTY() TObjectPtr<USceneComponent> BodyPivot;
    UPROPERTY() TObjectPtr<USceneComponent> HeadPivot;
    UPROPERTY() TObjectPtr<USceneComponent> LeftArmPivot;
    UPROPERTY() TObjectPtr<USceneComponent> RightArmPivot;
    UPROPERTY() TObjectPtr<USceneComponent> LeftLegPivot;
    UPROPERTY() TObjectPtr<USceneComponent> RightLegPivot;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> BodyPart;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> HeadPart;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LeftArmPart;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> RightArmPart;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LeftLegPart;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> RightLegPart;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> CorePart;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ChargePart;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> OrganicFireCharge;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> OrganicFireMaterial;
    UPROPERTY() TObjectPtr<UPointLightComponent> OrganicFireLight;
    UPROPERTY() TObjectPtr<UAudioComponent> OrganicFurnaceAudio;
    UPROPERTY() TObjectPtr<UAudioComponent> OrganicIgnitionAudio;
    UPROPERTY() TArray<TObjectPtr<USoundBase>> OrganicFireReleases;
    UPROPERTY() TArray<TObjectPtr<UAudioComponent>> OrganicReleaseAudio;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> WarningMarks;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EffectMarks;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> FrostMarks;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EmberMarks;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> WarningMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CoreMaterial;
    UPROPERTY() TObjectPtr<USoundBase> EnemyHitSound;
    UPROPERTY() TObjectPtr<USoundBase> EnemyFireSound;
    UPROPERTY() TObjectPtr<USoundBase> BossTellSound;
    UPROPERTY() TObjectPtr<USoundBase> EnemyTellSound;
    UPROPERTY() TObjectPtr<USoundBase> EnemyDefeatSound;
    UPROPERTY() TObjectPtr<USoundBase> BodyImpactSound;

    TWeakObjectPtr<ADBCharacter> Target;
    TWeakObjectPtr<AActor> BurnInstigator;
    TArray<FTransform> DeathStartPose;
    TArray<FTransform> OrganicReferencePose;
    TArray<FTransform> OrganicReferenceComponentPose;
    TMap<FName, int32> OrganicBoneIndices;
    TArray<FTransform> OrganicDeathStartPose;
    TArray<FTransform> OrganicPhaseStartPose;
    TArray<FTransform> OrganicPhaseStartBonePose;
    struct FOrganicFoot
    {
        FVector Anchor = FVector::ZeroVector;
        FVector Position = FVector::ZeroVector;
        FVector SwingStart = FVector::ZeroVector;
        FVector SwingEnd = FVector::ZeroVector;
        FVector SwingStartVelocity = FVector::ZeroVector;
        FVector Velocity = FVector::ZeroVector;
        FVector Normal = FVector::UpVector;
        FVector LandingNormal = FVector::UpVector;
        FQuat Rotation = FQuat::Identity;
        FQuat StartRotation = FQuat::Identity;
        FQuat LandingRotation = FQuat::Identity;
        FQuat ToeRotation = FQuat::Identity;
        float Progress = 1.f;
        float Duration = 0.22f;
        float ExpectedTravel = 1.f;
        float Travel = 0.f;
        float AnkleHeight = 10.f;
        float FacingYaw = 0.f;
        float LandingLeadTime = 0.f;
        float LiftHeight = 12.f;
        float LiftPhaseStart = 0.f;
        float LiftVelocity = 0.f;
        bool bSwinging = false;
        bool bSettling = false;
    };
    FOrganicFoot OrganicFeet[2];
    FVector OrganicLastLocation = FVector::ZeroVector;
    FVector OrganicVelocity = FVector::ZeroVector;
    FVector OrganicMotionLean = FVector::ZeroVector;
    FVector OrganicMotionLeanVelocity = FVector::ZeroVector;
    FVector OrganicGaze = FVector::ZeroVector;
    FVector OrganicGazeVelocity = FVector::ZeroVector;
    float OrganicFacingVelocity = 0.f;
    bool bOrganicGazeInitialized = false;
    FVector OrganicAttentionWeights = FVector(1.f, 1.f, 0.f);
    FVector OrganicAttentionVelocity = FVector::ZeroVector;
    FVector OrganicCastOrigin = FVector::ZeroVector;
    bool bOrganicCastReleased = false;
    bool bOrganicFireActive = false;
    float OrganicFireHeat = 0.f;
    struct FPendingOrganicBolt
    {
        FVector Direction;
        FLinearColor Color;
        float Damage;
        int32 HandSide;
    };
    TArray<FPendingOrganicBolt> PendingOrganicBolts;
    void PrepareOrganicFire();
    void UpdateOrganicFire(float DeltaSeconds);
    void StopOrganicFire(bool bImmediate);
    FVector OrganicFireHand(int32 Side) const;
    FVector OrganicCrestMotion = FVector::ZeroVector;
    FVector OrganicCrestVelocity = FVector::ZeroVector;
    FVector OrganicPelvisOffset = FVector::ZeroVector;
    FVector OrganicSupportOffset = FVector::ZeroVector;
    FVector OrganicLandingPelvisStart = FVector::ZeroVector;
    float OrganicLandingTime = 0.f;
    bool bOrganicLungeNeedsLanding = false;
    bool bOrganicLanding = false;
    FVector DeathFootTargets[2];
    FQuat DeathFootRotations[2];
    FVector OrganicDeathPelvisStart = FVector::ZeroVector;
    FVector OrganicDeathPelvisContact = FVector::ZeroVector;
    FVector OrganicDeathHandContacts[2];
    bool bOrganicDeathHandContact[2] = { false, false };
    FVector OrganicSlamHandTargets[2];
    FVector OrganicSlamRequestedHands[2];
    FVector OrganicSlamStartPoles[2];
    FTransform OrganicSlamStartHands[2];
    FQuat OrganicSlamUpperRotations[2];
    FQuat OrganicSlamLowerRotations[2];
    FQuat OrganicSlamHandRotations[2];
    float OrganicFacingYaw = 0.f;
    float OrganicSpeed = 0.f;
    float OrganicArmDrive = 0.f;
    float OrganicHipYaw = 0.f;
    float OrganicHipRoll = 0.f;
    float OrganicSupportDrop = 0.f;
    float OrganicTurnRate = 0.f;
    float OrganicPhaseBlendTime = 0.f;
    float OrganicStepCooldown = 0.f;
    float OrganicPoseDelta = 0.f;
    float OrganicPerformanceCycle = 0.f;
    float OrganicIdlePhase = 0.f;
    bool bOrganicPounceBlocked = false;
    float OrganicBlockedPounceTime = 0.f;
    FVector OrganicPounceStart = FVector::ZeroVector;
    float OrganicPerformancePlantTime[2] = { 1.f, 1.f };
    bool bOrganicPerformanceFootSwinging[2] = { false, false };
    int32 OrganicPerformanceStepSide = INDEX_NONE;
    int32 OrganicStepsSinceStop = 0;
    float LeftElbowPitch = -20.f;
    float RightElbowPitch = -20.f;
    float LeftWristPitch = 0.f;
    float RightWristPitch = 0.f;
    EDBEnemyPhase OrganicPreviousPhase = EDBEnemyPhase::Dormant;
    bool bOrganicFeetInitialized = false;
    bool bOrganicGrounded = false;
    bool bOrganicSlamTargetsInitialized = false;
    EAttack Attack = EAttack::None;
    FVector ArenaCenter = FVector::ZeroVector;
    FVector2D ArenaHalfSize = FVector2D(1180.f, 1050.f);
    FVector HomePosition = FVector::ZeroVector;
    FVector LockedDirection = FVector::ForwardVector;
    FVector RepositionTarget = FVector::ZeroVector;
    FVector LastHitDirection = -FVector::ForwardVector;
    FVector ReactionLocalDirection = -FVector::ForwardVector;
    FVector DeathLocalDirection = -FVector::ForwardVector;
    FVector SteeringDirection = FVector::ZeroVector;
    float DifficultyScale = 1.f;
    float BaseSpeed = 245.f;
    float AttackDamage = 18.f;
    float Cooldown = 0.45f;
    float PhaseTime = 0.f;
    float AttackElapsed = 0.f;
    float NextShotTime = 0.f;
    float VisualTime = 0.f;
    float DeathTime = 0.f;
    float HitFlash = 0.f;
    float HitSoundCooldown = 0.f;
    float ChillRemaining = 0.f;
    float StormRemaining = 0.f;
    float BurnTickTime = 0.f;
    float BurnTickDamage = 4.f;
    float GroundPulseTime = 0.f;
    float StuckTime = 0.f;
    float AvoidanceSide = 1.f;
    float MeleeSetupTime = 0.f;
    float RepositionTime = 0.f;
    float GaitPhase = 0.f;
    float GaitBlend = 0.f;
    float ReactionTime = 0.f;
    float ReactionDuration = 0.4f;
    float ReactionStrength = 0.f;
    float KnockbackTime = 0.f;
    float RecoveryDuration = 1.f;
    float AttackKick = 0.f;
    float VisualScale = 0.9f;
    float SteeringTime = 0.f;
    float InterceptionCooldown = 3.f;
    float BurnPulse = 0.f;
    float ElementSoundCooldown = 0.f;
    uint32 ActiveAttackId = 0;
    int32 ShotsRemaining = 0;
    int32 BossAttackIndex = 0;
    bool bConfigured = false;
    bool bVisualsBuilt = false;
    bool bArenaBoundsSet = false;
    bool bHitAttempted = false;
    bool bChillStaggered = false;
    bool bDeathNotified = false;
    bool bNeedsReposition = false;
    bool bDeathLanded = false;

    bool IsRoomActive() const;
    bool HasSightTo(FVector Point, const AActor* AllowedActor = nullptr) const;
    FVector FeetLocation() const;
    FVector GroundBelow(FVector Point) const;
    FVector KeepInsideArena(FVector Point) const;
    FVector ShotOrigin() const;
    void BuildVisuals();
    void UpdateVisuals(float DeltaSeconds);
    void UpdateOrganicPose();
    void UpdateOrganicLocomotion(float DeltaSeconds);
    void ResetOrganicLocomotion();
    bool TraceOrganicFoot(FVector Candidate, float AnkleHeight, FVector& Position, FVector& Normal) const;
    void UpdateDeath(float DeltaSeconds);
    void ApplyHitReaction(const FDBHit& Hit);
    void ChooseRepositionTarget();
    void UpdateWarningGeometry();
    void UpdateStatusEffects(float DeltaSeconds);
    void UpdateElementVisuals(float DeltaSeconds);
    void ClearElementVisuals();
    void MoveToward(FVector Point, float DeltaSeconds, float SpeedMultiplier = 1.f);
    void MoveDirection(FVector Direction, float DeltaSeconds, float SpeedMultiplier = 1.f);
    void UpdateApproach(float DeltaSeconds);
    void BeginTell(EAttack InAttack, float Duration);
    void UpdateTell(float DeltaSeconds);
    void BeginAttack();
    void UpdateAttack(float DeltaSeconds);
    void BeginRecovery(float Duration);
    void FireBolt(FVector Direction, float Damage, FLinearColor Color);
    bool TryMeleeHit(float Range, float ConeCosine, float Damage, bool bUnblockable = false);
    void DetonateGround();
    void Stagger(float Duration);
    void DealHealthDamage(float Damage);
    void ChainToNearby(float Damage, int32 MaxTargets, float Radius, AActor* HitInstigator);
    void AddLine(UInstancedStaticMeshComponent* Component, FVector Start, FVector End, float Thickness);
    void AddRing(UInstancedStaticMeshComponent* Component, FVector Center, float Radius, float Thickness, int32 Segments = 28);
    void Die();
};

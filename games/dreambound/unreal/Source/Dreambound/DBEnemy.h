#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DBTypes.h"
#include "DBEnemy.generated.h"

class ADBCharacter;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMeshComponent;
class USoundBase;

UENUM(BlueprintType)
enum class EDBEnemyPhase : uint8 { Dormant, Approach, Telegraph, Attack, Recovery, Staggered, Dead };

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

protected:
    virtual void BeginPlay() override;

private:
    enum class EAttack : uint8 { None, Swing, Bolt, Lunge, Salvo, Slam, Ground };
    struct FArcVisual
    {
        FVector Start = FVector::ZeroVector;
        FVector End = FVector::ZeroVector;
        float Remaining = 0.f;
    };

    UPROPERTY() TObjectPtr<USceneComponent> VisualRoot;
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
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> WarningMarks;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EffectMarks;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> WarningMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> EffectMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CoreMaterial;
    UPROPERTY() TObjectPtr<USoundBase> EnemyHitSound;
    UPROPERTY() TObjectPtr<USoundBase> EnemyFireSound;
    UPROPERTY() TObjectPtr<USoundBase> BossTellSound;
    UPROPERTY() TObjectPtr<USoundBase> EnemyTellSound;
    UPROPERTY() TObjectPtr<USoundBase> EnemyDefeatSound;
    UPROPERTY() TObjectPtr<USoundBase> BodyImpactSound;

    TWeakObjectPtr<ADBCharacter> Target;
    TWeakObjectPtr<AActor> BurnInstigator;
    TArray<FArcVisual> ArcVisuals;
    TArray<FTransform> DeathStartPose;
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
    void UpdateDeath(float DeltaSeconds);
    void ApplyHitReaction(const FDBHit& Hit);
    void ChooseRepositionTarget();
    void UpdateWarningGeometry();
    void UpdateStatusEffects(float DeltaSeconds);
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

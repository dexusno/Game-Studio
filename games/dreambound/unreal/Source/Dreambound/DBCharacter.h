#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DBTypes.h"
#include "DBCharacter.generated.h"

class UCameraComponent;
class UAudioComponent;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class USoundBase;
class USoundConcurrency;
class ADBEnemy;
class ADBThrownShield;

UENUM(BlueprintType)
enum class EDBShieldState : uint8 { Held, Charging, Outbound, Lodged, Returning };

UENUM(BlueprintType)
enum class EDBShieldPieceState : uint8 { Attached, Selected, Outbound, Lodged, Returning, Regenerating };

/** First-person combined weapon/shield. Acquisition and run state belong to DBGameMode. */
UCLASS()
class DREAMBOUND_API ADBCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ADBCharacter();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void Landed(const FHitResult& Hit) override;

    void ApplyUpgrade(FName Id);
    bool HasUpgrade(FName Id) const;
    int32 GetUpgradeRank(FName Id) const;
    void ReceiveAttack(float Damage, FVector Source, bool bUnblockable = false, AActor* Attacker = nullptr, uint32 AttackId = 0);
    FVector GetAimDirection() const;
    FVector GetMuzzleLocation() const;
    void OnRunReset();
    void SuspendCombatInput();
    void RefreshEquipmentVisuals();
    bool CanAct() const;

    // The ordinary input entry points are public so an engine automation driver can exercise
    // the same rules without bypassing cooldowns, aim, resources, collision or menu state.
    void PressFire();
    void ReleaseFire();
    void PressGuard();
    void ReleaseGuard();
    void UseSpecial();
    void Dash();
    void ReleaseDash();
    void PressSprint();
    void ReleaseSprint();
    bool IsSprinting() const { return bSprinting; }
    bool IsDashing() const { return DashRootMotionId != 0; }
    void CycleElement();
    bool IsShieldAway() const;
    void RecallShield();
    FVector GetShieldCatchLocation() const;
    void OnShieldCaught(bool bEmergency = false);
    bool ApplyPhysicalShieldHit(ADBEnemy* Enemy, const FDBHit& Hit, bool bHeavyFeedback = false);
    void TriggerFullVolleyImpact(uint32 VolleyId, FVector Location, ADBEnemy* DirectTarget = nullptr);
    void PlayShieldImpactFeedback(FVector Location, bool bHeavy = false);
    void BankAnchoredForce(float Damage);
    static constexpr int32 ShieldPieceCount = 6;
    int32 GetShieldPieceCount() const { return ShieldPieceCount; }
    int32 GetAttachedPieceCount() const;
    int32 GetSelectedPieceCount() const;
    int32 GetDeployedPieceCount() const;
    int32 GetRegeneratingPieceCount() const;
    EDBShieldPieceState GetPieceState(int32 Index) const;
    float GetPieceRegenerationProgress(int32 Index) const;
    ADBThrownShield* GetPieceFlight(int32 Index) const;
    float GetTotalAnchorIntegrity() const;
    FVector GetPieceCatchLocation(int32 Index) const;
    FTransform GetPieceSocketTransform(int32 Index) const;
    static FVector GetPieceMeshCentre() { return FVector(-.794f, 0.f, 24.818f); }
    static float GetSelectionHoldTime(int32 Count);
    static float GetFullChargeHoldTime() { return GetSelectionHoldTime(ShieldPieceCount); }
    float GetChargeHoldTime() const { return ChargeHeld; }
    float GetChargeProgress() const;
    bool IsFullChargeReady() const;
    float GetShieldExpansion() const { return GuardBlend; }
    bool IsChargeLoopPlaying() const;
    void OnShieldPieceCaught(int32 Index, ADBThrownShield* Flight, bool bEmergency = false);
    void OnShieldPieceDestroyed(int32 Index, ADBThrownShield* Flight);
    void OnShieldPieceFlightState(int32 Index, ADBThrownShield* Flight, EDBShieldPieceState State);
    FString GetShieldStateLabel() const;
    FString GetSpecialLabel() const;
    FString GetElementLabel() const;
    static FString GetUpgradeDescription(FName Id, int32 Rank = 1);

    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> ViewCamera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> WeaponRoot;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> WeaponBody;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> WeaponCore;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Forearm;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> CoreLight;

    UPROPERTY() float Health = 100.f;
    UPROPERTY() float MaxHealth = 100.f;
    UPROPERTY() float GuardEnergy = 100.f;
    UPROPERTY() float MaxGuardEnergy = 100.f;
    UPROPERTY() float Heat = 0.f;
    UPROPERTY() float MaxHeat = 100.f;
    UPROPERTY() float DashCooldown = 0.f;
    UPROPERTY() float Stamina = 100.f;
    UPROPERTY() float MaxStamina = 100.f;
    UPROPERTY() float SpecialCooldown = 0.f;
    UPROPERTY() float GuardBreakTime = 0.f;
    UPROPERTY() float HitMarkerTime = 0.f;
    UPROPERTY() float HurtFlashTime = 0.f;
    UPROPERTY() float ParryFlashTime = 0.f;
    UPROPERTY() float MessageTime = 0.f;
    UPROPERTY() float MouseSensitivity = 1.f;
    UPROPERTY() int32 StoredShots = 0;
    UPROPERTY() int32 EmpoweredShots = 0;
    UPROPERTY() bool bGuarding = false;
    UPROPERTY() bool bDead = false;
    UPROPERTY() bool bOverheated = false;
    UPROPERTY() bool bLastHitKilled = false;
    UPROPERTY() EDBElement CurrentElement = EDBElement::Neutral;
    UPROPERTY() EDBShieldState ShieldState = EDBShieldState::Held;
    UPROPERTY() bool bShieldReady = true;
    UPROPERTY() float ThrowCharge = 0.f;
    UPROPERTY() float AttackRecovery = 0.f;
    UPROPERTY() TObjectPtr<ADBThrownShield> ThrownShield;
    UPROPERTY() TMap<FName, int32> Upgrades;
    UPROPERTY() FString LastCombatMessage;

private:
    struct FEchoShot
    {
        float Delay = 0.f;
        FVector AimPoint = FVector::ZeroVector;
        TWeakObjectPtr<ADBEnemy> Target;
        EDBElement Element = EDBElement::Neutral;
        float Damage = 0.f;
        bool bTrack = false;
        bool bStormfracture = false;
    };

    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ShieldPlates;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ShieldHub;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> PieceGlows;
    UPROPERTY() TArray<EDBShieldPieceState> PieceStates;
    UPROPERTY() TArray<TObjectPtr<ADBThrownShield>> PieceFlights;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> AttachmentParts;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> CombatEffects;
    UPROPERTY() TObjectPtr<UStaticMesh> BeamMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> SparkMesh;
    UPROPERTY() TObjectPtr<UMaterialInterface> CeramicMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> BronzeMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> DarkMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> CoreMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> SelectedPieceMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> FrostMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> StormMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> EmberMaterial;
    UPROPERTY() TObjectPtr<USoundBase> AttackSound;
    UPROPERTY() TObjectPtr<USoundBase> GuardSound;
    UPROPERTY() TObjectPtr<USoundBase> ParrySound;
    UPROPERTY() TObjectPtr<USoundBase> DashSound;
    UPROPERTY() TObjectPtr<USoundBase> FootstepASound;
    UPROPERTY() TObjectPtr<USoundBase> FootstepBSound;
    UPROPERTY() TObjectPtr<USoundBase> LandSound;
    UPROPERTY() TObjectPtr<USoundBase> ImpactSound;
    UPROPERTY() TObjectPtr<USoundBase> EquipSound;
    UPROPERTY() TObjectPtr<USoundBase> HurtSound;
    UPROPERTY() TObjectPtr<USoundBase> CatchSound;
    UPROPERTY() TObjectPtr<USoundBase> RecallSound;
    UPROPERTY() TObjectPtr<UAudioComponent> ChargeAudio;
    UPROPERTY() TObjectPtr<USoundBase> ChargeLoopSound;
    UPROPERTY() TObjectPtr<USoundBase> ChargeTickSound;
    UPROPERTY() TObjectPtr<USoundBase> ChargeReadySound;
    UPROPERTY() TObjectPtr<USoundBase> FullReleaseSound;
    UPROPERTY() TObjectPtr<USoundBase> MeleeSwingSound;
    UPROPERTY() TObjectPtr<USoundBase> HeavyImpactSound;
    UPROPERTY() TObjectPtr<USoundConcurrency> ImpactConcurrency;
    UPROPERTY() TObjectPtr<USoundConcurrency> HeavyImpactConcurrency;
    UPROPERTY() TObjectPtr<USoundConcurrency> CatchConcurrency;
    UPROPERTY() TObjectPtr<USoundConcurrency> FootstepConcurrency;
    UPROPERTY() TObjectPtr<USoundConcurrency> LandingConcurrency;

    TArray<float> EffectLife;
    TArray<FEchoShot> EchoShots;
    TArray<float> StoredDamage;
    TArray<float> PieceRegenRemaining;
    TArray<float> PieceRegenDuration;
    TArray<float> PieceDockPulse;
    TArray<int32> SelectionQueue;
    TArray<uint64> ReceivedAttackKeys;
    TArray<uint32> ResolvedFullVolleys;
    TSet<TWeakObjectPtr<ADBEnemy>> RushVictims;
    bool bWantsFire = false;
    bool bWantsGuard = false;
    bool bWantsSprint = false;
    bool bSprinting = false;
    bool bSprintExhausted = false;
    bool bDashHeld = false;
    bool bStrikeBuffered = false;
    bool bStrikePending = false;
    bool bHeavyStrike = false;
    bool bStrikePoseActive = false;
    bool bWasMenuBlocked = false;
    bool bRushActive = false;
    bool bReforgeUsed = false;
    bool bChargeReadyAnnounced = false;
    bool bChargeAudioStopping = false;
    bool bExpansionTarget = false;
    bool bMeleeFinisher = false;
    uint32 NextVolleyId = 0;
    int32 MeleeChainStep = 0;
    float MeleeChainWindow = 0.f;
    float MeleeSide = 1.f;
    float MeleeStepRemaining = 0.f;
    float ImpactSoundCooldown = 0.f;
    float HeavySoundCooldown = 0.f;
    float CameraKick = 0.f;
    float CameraSideKick = 0.f;
    float ReleasePose = 0.f;
    FVector MeleeStepDirection = FVector::ForwardVector;
    int32 SelectionCursor = 0;
    int32 NextBlockPiece = 0;
    float CatchSoundCooldown = 0.f;
    float FireCooldown = 0.f;
    float ChargeHeld = 0.f;
    float StrikeDelay = 0.f;
    float StrikeTotal = .5f;
    FVector StrikeStartLocation = FVector(120.f, -43.f, -42.f);
    FRotator StrikeStartRotation = FRotator(0.f, -8.f, -3.f);
    FVector WeaponBaseLocation = FVector(120.f, -43.f, -42.f);
    FRotator WeaponBaseRotation = FRotator(0.f, -8.f, -3.f);
    float CatchPose = 0.f;
    float GuardRaiseCooldown = 0.f;
    float SinceFired = 10.f;
    float SinceGuarded = 10.f;
    float SinceDamaged = 10.f;
    float DashTime = 0.f;
    uint16 DashRootMotionId = 0;
    float EvasionPoseAge = 1.f;
    FVector EvasionPoseDirection = FVector::ForwardVector;
    float AirbornePoseBlend = 0.f;
    float EvasionInvulnerabilityTime = 0.f;
    float StaminaRecoveryDelay = 0.f;
    float GroundedSpeed = 0.f;
    float GaitAmount = 0.f;
    float SprintBlend = 0.f;
    float LandingImpact = 0.f;
    float FootstepDistance = 0.f;
    float FootstepAudioCooldown = 0.f;
    float LocomotionAudioHold = .2f;
    int32 FootstepSequence = 0;
    FVector PreviousMotionLocation = FVector::ZeroVector;
    FVector PreviousMotionVelocity = FVector::ZeroVector;
    FVector MovementLean = FVector::ZeroVector;
    float RushTime = 0.f;
    float RushDamage = 60.f;
    float InvulnerabilityTime = 0.f;
    float GuardBlend = 0.f;
    float Recoil = 0.f;
    float ImpactPose = 0.f;
    float EquipPose = 0.f;
    float LookSwayX = 0.f;
    float LookSwayY = 0.f;
    float BobPhase = 0.f;
    int32 PulseSequence = 0;
    FVector RushDirection = FVector::ForwardVector;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void BeginJump();
    void EndJump();
    void Interact();
    void ChooseRewardOne();
    void ChooseRewardTwo();
    void ChooseRewardThree();
    void TogglePause();
    void ToggleBuild();
    void StartRimStrike(bool bHeavy);
    void ResolveRimStrike();
    void LaunchShield();
    void ClearPieceSelection();
    void UpdatePieces(float DeltaSeconds);
    void RefreshAggregateShieldState();
    void StartPieceRegeneration(int32 Index);
    bool SpendGuardPiece();
    bool AdvancePieceRegeneration(float Seconds);
    void ShieldImpact();
    void ResolveRush();
    void UpdateWeapon(float DeltaSeconds);
    void UpdateLocomotion(float DeltaSeconds);
    void EndEvasion(bool bStopImmediately = false);
    FTransform GetPieceLocalTransform(int32 Index) const;
    void UpdateChargeAudio();
    void StopChargeAudio(bool bImmediate = false);
    void UpdateMeleeStep(float DeltaSeconds);
    void UpdateEffects(float DeltaSeconds);
    void UpdateEchoes(float DeltaSeconds);
    void CaptureEnergy(float Damage);
    void SetCombatMessage(const FString& Text, float Duration = 2.f);
    void PlayCombatSound(USoundBase* Sound, float Volume = 1.f, float Pitch = 1.f);
    bool HitEnemy(ADBEnemy* Enemy, const FDBHit& Hit);
    void ApplyAreaHit(const FVector& Centre, float Radius, const FDBHit& Hit, ADBEnemy* Ignore = nullptr, int32 MaxTargets = 12);
    void DrawBeam(const FVector& Start, const FVector& End, EDBElement Element, float Width, float Duration = .085f);
    void DrawSpark(const FVector& Location, EDBElement Element, float Size = 9.f);
    UMaterialInterface* GetElementMaterial(EDBElement Element) const;
    FVector FindAimPoint() const;
    bool HasLineOfSight(const FVector& From, ADBEnemy* Enemy) const;
};

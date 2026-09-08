#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DBTypes.h"
#include "DBCharacter.generated.h"

class UCameraComponent;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class USoundBase;
class ADBEnemy;

/** First-person combined weapon/shield. Acquisition and run state belong to DBGameMode. */
UCLASS()
class DREAMBOUND_API ADBCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ADBCharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    void ApplyUpgrade(FName Id);
    bool HasUpgrade(FName Id) const;
    int32 GetUpgradeRank(FName Id) const;
    void ReceiveAttack(float Damage, FVector Source, bool bUnblockable = false, AActor* Attacker = nullptr);
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
    void CycleElement();
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
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> AttachmentParts;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> CombatEffects;
    UPROPERTY() TObjectPtr<UStaticMesh> BeamMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> SparkMesh;
    UPROPERTY() TObjectPtr<UMaterialInterface> CeramicMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> BronzeMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> DarkMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> CoreMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> FrostMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> StormMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> EmberMaterial;
    UPROPERTY() TObjectPtr<USoundBase> AttackSound;
    UPROPERTY() TObjectPtr<USoundBase> GuardSound;
    UPROPERTY() TObjectPtr<USoundBase> ParrySound;
    UPROPERTY() TObjectPtr<USoundBase> DashSound;
    UPROPERTY() TObjectPtr<USoundBase> ImpactSound;
    UPROPERTY() TObjectPtr<USoundBase> EquipSound;
    UPROPERTY() TObjectPtr<USoundBase> HurtSound;

    TArray<float> EffectLife;
    TArray<FEchoShot> EchoShots;
    TArray<float> StoredDamage;
    TSet<TWeakObjectPtr<ADBEnemy>> RushVictims;
    bool bWantsFire = false;
    bool bWasMenuBlocked = false;
    bool bRushActive = false;
    float FireCooldown = 0.f;
    float GuardRaiseCooldown = 0.f;
    float SinceFired = 10.f;
    float SinceGuarded = 10.f;
    float SinceDamaged = 10.f;
    float DashTime = 0.f;
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
    void FirePulse();
    void Counterfire();
    void ShieldImpact();
    void ResolveRush();
    void UpdateWeapon(float DeltaSeconds);
    void UpdateEffects(float DeltaSeconds);
    void UpdateEchoes(float DeltaSeconds);
    void CaptureEnergy(float Damage);
    void SetCombatMessage(const FString& Text, float Duration = 2.f);
    void PlayCombatSound(USoundBase* Sound, float Volume = 1.f, float Pitch = 1.f);
    ADBEnemy* TraceAttack(const FVector& AimPoint, const FDBHit& Hit, float BeamWidth = 2.f);
    bool HitEnemy(ADBEnemy* Enemy, const FDBHit& Hit);
    void ApplyAreaHit(const FVector& Centre, float Radius, const FDBHit& Hit, ADBEnemy* Ignore = nullptr, int32 MaxTargets = 12);
    void DrawBeam(const FVector& Start, const FVector& End, EDBElement Element, float Width, float Duration = .085f);
    void DrawSpark(const FVector& Location, EDBElement Element, float Size = 9.f);
    UMaterialInterface* GetElementMaterial(EDBElement Element) const;
    FVector FindAimPoint() const;
    bool HasLineOfSight(const FVector& From, ADBEnemy* Enemy) const;
};

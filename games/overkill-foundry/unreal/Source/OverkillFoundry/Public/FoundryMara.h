#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "FoundryMara.generated.h"

class UAnimSequence;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;

// Apply cosmetic cradle aiming to each freshly evaluated clip pose, before UE
// publishes its component-space bones. The sled stays planted; load/recoil
// animation remains in the same rig. No target or damage decisions live here.
UCLASS()
class OVERKILLFOUNDRY_API UFoundryAimedGun : public USkeletalMeshComponent
{
    GENERATED_BODY()
public:
    FVector TargetWorld = FVector::ZeroVector;
    bool bAim = false;
    FTransform GetCosmeticAimDelta() const { return CosmeticAimDelta; }
    virtual void FinalizeBoneTransform() override;
private:
    FTransform CosmeticAimDelta = FTransform::Identity;
};

// Match the authored arm gesture to the gun's cosmetic aim. Bone lengths and
// the planted lower body are preserved; no physics or gameplay state is read.
UCLASS()
class OVERKILLFOUNDRY_API UFoundryAimedOperator : public USkeletalMeshComponent
{
    GENERATED_BODY()
public:
    TWeakObjectPtr<UFoundryAimedGun> GunSource;
    float GetReachErrorCm() const { return ReachErrorCm; }
    float GetBodyLeanDegrees() const { return BodyLeanDegrees; }
    float GetLowerBodyShiftCm() const { return LowerBodyShiftCm; }
    virtual void FinalizeBoneTransform() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
private:
    float ReachErrorCm = 0;
    float BodyLeanDegrees = 0;
    float LowerBodyShiftCm = 0;
};

// Independent cosmetic rigs. Neither animation nor attachment grants resources,
// fires an action, changes a target, consumes a part, or delays authoritative state.
UCLASS()
class OVERKILLFOUNDRY_API AFoundryMara : public AActor
{
    GENERATED_BODY()
public:
    AFoundryMara();
    void Initialize();
    void ResetPresentation();
    void Collect(uint64 EventId, int32 ActualHaul);
    void Load(uint64 EventId, int32 ActualParts);
    void Unload(uint64 EventId);
    void Fire(uint64 EventId, int32 ActualShot, const FVector& Target);
    void AimAt(const FVector& Target);
    virtual void Tick(float DeltaSeconds) override;
    FString GetGunCue() const { return GunCue; }
    FString GetClawCue() const { return ClawCue; }
    FString GetOperatorCue() const { return OperatorCue; }
    float GetClawElapsed() const { return ClawElapsed; }
    bool HasPayload() const;
    bool IsReset() const;
private:
    void PlayGun(const FString& Name, uint64 EventId = 0);
    void PlayClaw(const FString& Name, uint64 EventId = 0);
    void PlayOperator(const FString& Name, uint64 EventId = 0);
    UPROPERTY() TObjectPtr<UFoundryAimedGun> Gun;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Claw;
    UPROPERTY() TObjectPtr<UFoundryAimedOperator> Operator;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Payload;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> MuzzleFlash;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ShotBeam;
    UPROPERTY() TObjectPtr<UPointLightComponent> ShotLight;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FlashMaterial;
    UPROPERTY() TMap<FString, TObjectPtr<UAnimSequence>> GunClips;
    UPROPERTY() TMap<FString, TObjectPtr<UAnimSequence>> ClawClips;
    UPROPERTY() TMap<FString, TObjectPtr<UAnimSequence>> OperatorClips;
    FString GunCue;
    FString ClawCue;
    FString OperatorCue;
    float GunElapsed = 0;
    float ClawElapsed = 0;
    float OperatorElapsed = 0;
    float FlashRemaining = 0;
    float ShotStrength = 1;
    bool bLoaded = false;
    bool bGrabbed = false;
    bool bDumped = false;
    FVector ShotTarget = FVector::ZeroVector;
    FVector FireMuzzleStart = FVector::ZeroVector;
    float MaxRecoilMotion = 0;
    float MinGrabHeight = 10000;
    FVector DropStart = FVector::ZeroVector;
};

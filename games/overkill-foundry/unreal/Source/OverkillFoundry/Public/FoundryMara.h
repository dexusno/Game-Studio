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
    virtual void FinalizeBoneTransform() override;
};

// Two independent cosmetic rigs. Neither animation nor attachment grants resources,
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
    float GetClawElapsed() const { return ClawElapsed; }
    bool HasPayload() const;
    bool IsReset() const;
private:
    void PlayGun(const FString& Name, uint64 EventId = 0);
    void PlayClaw(const FString& Name, uint64 EventId = 0);
    UPROPERTY() TObjectPtr<UFoundryAimedGun> Gun;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Claw;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Payload;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> MuzzleFlash;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ShotBeam;
    UPROPERTY() TObjectPtr<UPointLightComponent> ShotLight;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FlashMaterial;
    UPROPERTY() TMap<FString, TObjectPtr<UAnimSequence>> GunClips;
    UPROPERTY() TMap<FString, TObjectPtr<UAnimSequence>> ClawClips;
    FString GunCue;
    FString ClawCue;
    float GunElapsed = 0;
    float ClawElapsed = 0;
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

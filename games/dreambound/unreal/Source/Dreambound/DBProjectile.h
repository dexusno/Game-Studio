#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DBProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UAudioComponent;
class UPointLightComponent;
class USoundBase;

/** Hostile spell with swept world collision, visible travel and a bounded active lifetime. */
UCLASS()
class DREAMBOUND_API ADBProjectile : public AActor
{
    GENERATED_BODY()

public:
    ADBProjectile();
    virtual void Tick(float DeltaSeconds) override;
    void Initialize(FVector Direction, float Speed, float Damage, bool bUnblockable,
        AActor* OwnerEnemy, FLinearColor Color);

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY() TObjectPtr<USphereComponent> Collision;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Streak;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> GlowMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FireMaterial;
    UPROPERTY() TObjectPtr<UPointLightComponent> FireLight;
    UPROPERTY() TObjectPtr<UAudioComponent> FlightAudio;
    UPROPERTY() TObjectPtr<UAudioComponent> ImpactAudio;
    UPROPERTY() TArray<TObjectPtr<USoundBase>> FireImpacts;
    TWeakObjectPtr<AActor> SourceEnemy;
    FVector Velocity = FVector::ZeroVector;
    FVector VisualCenterOffset = FVector::ZeroVector;
    FVector BaseVisualScale = FVector(0.28f, 0.22f, 0.22f);
    FLinearColor BoltColor = FLinearColor(1.f, 0.35f, 0.12f);
    float HitDamage = 14.f;
    float LifeRemaining = 5.f;
    float VisualTime = 0.f;
    int32 SpawnRoomId = INDEX_NONE;
    bool bPiercesGuard = false;
    bool bInitialized = false;
    bool bImpacted = false;
    bool bOrganicFire = false;

    void PrepareVisuals();
    void UpdateFireVisuals();
    void Impact(FVector Location);
};

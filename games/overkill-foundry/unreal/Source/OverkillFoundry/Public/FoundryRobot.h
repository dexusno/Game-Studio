#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FoundryRobot.generated.h"

class UAnimSequence;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;

// Cosmetics only. The committed core state is already final when these cues run.
UCLASS()
class OVERKILLFOUNDRY_API AFoundryRobot : public AActor
{
    GENERATED_BODY()
public:
    AFoundryRobot();
    void Initialize(uint64 Id, bool bRam, int32 MaxHp);
    void EnemyAction(int32 Move, uint64 EventId);
    void NamedAction(const FString& ActionName, uint64 EventId);
    void Hit(int32 HpLoss, int32 ShieldLoss, uint64 EventId);
    void Die(uint64 EventId);
    void Escape(uint64 EventId);
    virtual void Tick(float DeltaSeconds) override;
    uint64 GetCoreId() const { return CoreId; }
    bool IsTerminal() const { return bTerminal; }
    bool IsCharged() const { return bCharged; }
    FString GetCue() const { return Cue; }
    float GetCueElapsed() const { return Elapsed; }
    int32 GetDissolveSlotCount() const { return Materials.Num(); }
    bool AreMaterialsSolid() const;
private:
    void Play(const FString& Clip, uint64 EventId = 0);
    void HoldCharge();
    void Flash(FName Socket, bool bBeam, float Strength = 1.0f);
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Body;
    UPROPERTY() TMap<FString, TObjectPtr<UAnimSequence>> Clips;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> FlashMesh;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FlashMaterial;
    UPROPERTY() TObjectPtr<UPointLightComponent> FlashLight;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> SteamMesh;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SteamMaterial;
    uint64 CoreId = 0;
    int32 MaxHp = 1;
    bool bRam = false;
    bool bCharged = false;
    bool bTerminal = false;
    bool bDisplayCueFired = false;
    bool bContactFired = false;
    bool bDissolveLogged = false;
    FString Cue;
    float Elapsed = 0;
    float Duration = 0;
    float FlashRemaining = 0;
    float SteamRemaining = 0;
    bool bSteamFired = false;
};

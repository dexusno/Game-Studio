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
    void Initialize(uint64 Id, const FString& Definition, int32 MaxHp, int32 Tiles = 0, bool bInitiallyCharged = false);
    void EnemyAction(int32 Move, uint64 EventId);
    void NamedAction(const FString& ActionName, uint64 EventId, int32 CommittedHits, bool bCommittedSummon);
    void Hit(int32 HpLoss, int32 ShieldLoss, uint64 EventId);
    void Die(uint64 EventId, bool bDeathRelease = false, bool bCommittedSummon = false);
    void Escape(uint64 EventId);
    void SetTiles(int32 Tiles);
    void AwaitReveal();
    void RevealAfter(float Seconds);
    void CompletePendingReveal();
    void SetSceneHidden(bool bInSceneHidden);
    virtual void Tick(float DeltaSeconds) override;
    uint64 GetCoreId() const { return CoreId; }
    bool IsTerminal() const { return bTerminal; }
    bool IsCharged() const { return bCharged; }
    FString GetCue() const { return Cue; }
    float GetCueElapsed() const { return Elapsed; }
    int32 GetDissolveSlotCount() const { return Materials.Num(); }
    bool AreMaterialsSolid() const;
    FVector GetImpactLocation() const;
    FBox GetBodyBounds() const;
    const FString& GetDefinition() const { return Definition; }
    int32 GetPresentedHitCount() const { return PresentedHits; }
    int32 GetPresentedSummonCount() const { return PresentedSummons; }
    int32 GetVisibleTileCount() const;
    FString GetVisualAssetName() const;
    bool HasAction(const FString& Name) const;
    float GetTerminalDelay() const { return PendingDeath.IsEmpty() ? 0.f : FMath::Max(0.f, Duration - Elapsed); }
private:
    void Play(const FString& Clip, uint64 EventId = 0);
    void HoldCharge();
    void Flash(FName Socket, bool bBeam, float Strength = 1.0f);
    void DisplayCue(const FString& Name, int32 HitIndex);
    void UpdateVisibility();
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
    int32 ExpectedMaterialSlots = 0;
    int32 VisibleTiles = -1;
    int32 CommittedHits = 0;
    int32 PresentedHits = 0;
    int32 PresentedSummons = 0;
    uint64 CueEventId = 0;
    uint64 DeathEventId = 0;
    FString Definition;
    bool bRam = false;
    bool bCharged = false;
    bool bTerminal = false;
    bool bDissolveLogged = false;
    bool bCommittedSummon = false;
    bool bAwaitingReveal = false;
    bool bSceneHidden = false;
    float RevealRemaining = 0;
    TSet<int32> DisplayedCues;
    FString ClipAsset;
    FString PendingReaction;
    FString PendingDeath;
    FString Cue;
    float Elapsed = 0;
    float Duration = 0;
    float FlashRemaining = 0;
    float SteamRemaining = 0;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DBTypes.h"
#include "DBCombatEffect.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class USceneComponent;

/** Short, non-damaging feedback owned by the world rather than a dying combatant. */
UCLASS(Transient, NotBlueprintable)
class DREAMBOUND_API ADBCombatEffect : public AActor
{
    GENERATED_BODY()

public:
    ADBCombatEffect();
    virtual void Tick(float DeltaSeconds) override;
    static void SpawnBurst(UWorld* World, FVector At, EDBElement Element, int32 RoomId, float Scale = 1.f);
    static void SpawnArc(UWorld* World, FVector From, FVector To, int32 RoomId);

private:
    UPROPERTY() TObjectPtr<USceneComponent> EffectRoot;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Shards;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Lines;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Glow;
    FVector ArcEnd = FVector::ZeroVector;
    EDBElement EffectElement = EDBElement::Neutral;
    int32 EffectRoomId = INDEX_NONE;
    int32 Expedition = INDEX_NONE;
    float Age = 0.f;
    float Duration = 0.6f;
    float EffectScale = 1.f;
    bool bArc = false;

    static ADBCombatEffect* Create(UWorld* World, FVector At);
    void Initialize(EDBElement Element, int32 RoomId, float Scale);
    void Rebuild();
    void AddLine(FVector From, FVector To, float Width);
    void AddShard(FVector At, FRotator Rotation, FVector Size);
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DBTypes.h"
#include "DBThrownShield.generated.h"

class ADBCharacter;
class ADBEnemy;
class UStaticMeshComponent;
class USceneComponent;
class USoundBase;

/** One identified shield segment. Its physical centre is the actor origin. */
UCLASS()
class DREAMBOUND_API ADBThrownShield : public AActor
{
    GENERATED_BODY()
public:
    ADBThrownShield();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void Initialize(ADBCharacter* InBearer, FVector Direction, float Charge, int32 InPieceId = 0);
    void Recall();
    void DestroyPiece();
    int32 GetPieceId() const { return PieceId; }
    bool InterceptProjectile(FVector Start, FVector End, float Damage, bool bUnblockable);
    bool IsReturning() const { return bReturning; }
    bool IsAnchored() const { return bLodged && bAnchor; }
    float GetAnchorIntegrity() const { return AnchorIntegrity; }

private:
    UPROPERTY() TObjectPtr<USceneComponent> FlightRoot;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Disc;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Core;
    UPROPERTY() TObjectPtr<ADBCharacter> Bearer;
    UPROPERTY() TObjectPtr<USoundBase> ImpactSound;
    UPROPERTY() TObjectPtr<USoundBase> BlockSound;
    TArray<FVector> OutwardPath;
    TArray<FVector> BearerPath;
    TSet<TWeakObjectPtr<ADBEnemy>> OutwardVictims;
    TSet<TWeakObjectPtr<ADBEnemy>> ReturnVictims;
    FVector FlightDirection = FVector::ForwardVector;
    FVector AnchorForward = FVector::ForwardVector;
    FVector LastBearerPoint = FVector::ZeroVector;
    float Speed = 1800.f;
    float Damage = 60.f;
    float Range = 1500.f;
    float Travelled = 0.f;
    float OutwardTime = 0.f;
    float ReturnTime = 0.f;
    float Spin = 0.f;
    float AnchorIntegrity = 110.f;
    float BlockFlash = 0.f;
    int32 RetraceIndex = -1;
    int32 BearerTrailIndex = 0;
    int32 Ricochets = 0;
    bool bInitialized = false;
    bool bReturning = false;
    bool bLodged = false;
    bool bAnchor = false;
    bool bFollowingBearerTrail = false;
    bool bResolved = false;
    bool bEmergencyReturn = false;
    int32 PieceId = INDEX_NONE;
    EDBElement LastElement = EDBElement::Neutral;

    void UpdateMaterial();
    void RecordBearerPath();
    void TickOutbound(float DeltaSeconds);
    void TickReturn(float DeltaSeconds);
    void Lodge(FVector Location, FVector SurfaceNormal);
    bool SweepTravel(FVector Destination, bool bOnReturn);
    bool ClearWorldPath(FVector From, FVector To, float Radius = 20.f) const;
    bool RedirectToEnemy(ADBEnemy* Previous);
    void Catch(bool bEmergency);
    void UpdatePiecePose(float DeltaSeconds);
};

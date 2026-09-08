#include "DBProjectile.h"

#include "DBCharacter.h"
#include "DBEnemy.h"
#include "DBGameMode.h"
#include "DBThrownShield.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ADBProjectile::ADBProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitSphereRadius(12.f);
    // The explicit sweep is authoritative and cannot tunnel between overlap callbacks.
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bolt"));
    Visual->SetupAttachment(Collision);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetCastShadow(false);
    Streak = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Trail"));
    Streak->SetupAttachment(Collision);
    Streak->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Streak->SetCastShadow(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded())
    {
        Visual->SetStaticMesh(Sphere.Object);
        Streak->SetStaticMesh(Sphere.Object);
    }
    Visual->SetRelativeScale3D(FVector(0.28f, 0.22f, 0.22f));
    Streak->SetRelativeLocation(FVector(-33.f, 0.f, 0.f));
    Streak->SetRelativeScale3D(FVector(0.72f, 0.055f, 0.055f));
}

void ADBProjectile::BeginPlay()
{
    Super::BeginPlay();
    PrepareVisuals();
}

void ADBProjectile::PrepareVisuals()
{
    if (UStaticMesh* Crystal = LoadObject<UStaticMesh>(nullptr,
        TEXT("/Game/Art/Meshes/SM_Crystal.SM_Crystal")))
    {
        Visual->SetStaticMesh(Crystal);
        const FVector Size = Crystal->GetBounds().BoxExtent * 2.f;
        BaseVisualScale = FVector(24.f / FMath::Max(1.f, Size.X),
            20.f / FMath::Max(1.f, Size.Y), 28.f / FMath::Max(1.f, Size.Z));
        Visual->SetRelativeScale3D(BaseVisualScale);
        VisualCenterOffset = -Crystal->GetBounds().Origin * BaseVisualScale;
        Visual->SetRelativeLocation(VisualCenterOffset);
    }
    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Art/Materials/M_CombatGlow.M_CombatGlow"));
    if (!Material) Material = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (Material)
    {
        GlowMaterial = UMaterialInstanceDynamic::Create(Material, this);
        GlowMaterial->SetVectorParameterValue(TEXT("Color"), BoltColor);
        GlowMaterial->SetVectorParameterValue(TEXT("Tint"), BoltColor);
        GlowMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 3.f);
        Visual->SetMaterial(0, GlowMaterial);
        Streak->SetMaterial(0, GlowMaterial);
    }
}

void ADBProjectile::Initialize(FVector Direction, float Speed, float Damage, bool bUnblockable,
    AActor* OwnerEnemy, FLinearColor Color)
{
    if (Direction.ContainsNaN() || Direction.IsNearlyZero()) Direction = FVector::ForwardVector;
    Velocity = Direction.GetSafeNormal() * FMath::Clamp(Speed, 100.f, 3000.f);
    HitDamage = FMath::Max(0.f, Damage);
    bPiercesGuard = bUnblockable;
    SourceEnemy = OwnerEnemy;
    SetOwner(OwnerEnemy);
    BoltColor = Color;
    LifeRemaining = 5.f;
    bImpacted = false;
    Streak->SetVisibility(true);
    Visual->SetRelativeScale3D(BaseVisualScale);
    const ADBEnemy* Enemy = Cast<ADBEnemy>(OwnerEnemy);
    if (Enemy && Enemy->RoomId >= 0) SpawnRoomId = Enemy->RoomId;
    else if (const ADBGameMode* Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode())) SpawnRoomId = Mode->CurrentRoomId;
    SetActorRotation(Velocity.Rotation());
    bInitialized = true;
    if (GlowMaterial)
    {
        GlowMaterial->SetVectorParameterValue(TEXT("Color"), Color);
        GlowMaterial->SetVectorParameterValue(TEXT("Tint"), Color);
    }
}

void ADBProjectile::Impact(FVector Location)
{
    if (bImpacted) return;
    bImpacted = true;
    Velocity = FVector::ZeroVector;
    SetActorLocation(Location, false);
    Streak->SetVisibility(false);
    LifeRemaining = 0.13f;
    // The stopped flash remains visible briefly; it cannot deal another contact.
    if (GlowMaterial) GlowMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.76f, 0.4f) * 2.f);
}

void ADBProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bInitialized) { Destroy(); return; }
    if (const ADBGameMode* Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode()))
    {
        // Clearing an encounter removes lingering danger before the reward choice.
        if (Mode->bChoosingReward || Mode->bTitle || Mode->bWon || Mode->bDefeated
            || (SpawnRoomId != INDEX_NONE && Mode->CurrentRoomId != SpawnRoomId))
        { Destroy(); return; }
        if (Mode->bPaused || Mode->bShowingBuild) return;
    }
    LifeRemaining -= DeltaSeconds;
    if (LifeRemaining <= 0.f) { Destroy(); return; }
    if (bImpacted)
    {
        const float Fade = FMath::Clamp(LifeRemaining / 0.13f, 0.f, 1.f);
        const float Expansion = 1.f + (1.f - Fade) * 2.f;
        Visual->SetRelativeScale3D(BaseVisualScale * Expansion);
        Visual->SetRelativeLocation(Visual->GetRelativeRotation().RotateVector(VisualCenterOffset * Expansion));
        if (GlowMaterial) GlowMaterial->SetVectorParameterValue(TEXT("Color"), BoltColor * (Fade * 2.5f));
        return;
    }
    VisualTime += DeltaSeconds;
    Visual->AddLocalRotation(FRotator(0.f, 0.f, DeltaSeconds * 210.f));
    Visual->SetRelativeLocation(Visual->GetRelativeRotation().RotateVector(VisualCenterOffset));
    Streak->SetRelativeScale3D(FVector(0.66f + FMath::Sin(VisualTime * 25.f) * 0.08f, 0.055f, 0.055f));

    const FVector Start = GetActorLocation();
    const FVector End = Start + Velocity * DeltaSeconds;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DBHostileBolt), false, this);
    Params.AddIgnoredActor(SourceEnemy.Get());
    FHitResult Hit;
    const bool bWorldContact = GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility,
        FCollisionShape::MakeSphere(Collision->GetScaledSphereRadius()), Params);
    const FVector UnobstructedEnd = bWorldContact ? Hit.Location : End;
    // Query every deployed identity, not the legacy first-flight pointer. Sort crossings so
    // the nearest valid anchor takes the bolt, once, before any farther overlapping piece.
    const FVector Segment = UnobstructedEnd - Start;
    TArray<TPair<float, ADBThrownShield*>> Crossings;
    TSet<ADBThrownShield*> SeenPieces;
    for (TActorIterator<ADBCharacter> It(GetWorld()); It; ++It)
    {
        for (int32 Index = 0; Index < It->GetShieldPieceCount(); ++Index)
        {
            ADBThrownShield* Piece = It->GetPieceFlight(Index);
            if (!IsValid(Piece) || !Piece->IsAnchored() || SeenPieces.Contains(Piece)) continue;
            SeenPieces.Add(Piece);
            const FVector Normal = Piece->GetActorForwardVector();
            const double Denominator = FVector::DotProduct(Segment, Normal);
            if (FMath::Abs(Denominator) < 0.001) continue;
            const double Fraction = FVector::DotProduct(Piece->GetActorLocation() - Start, Normal) / Denominator;
            if (Fraction >= 0.0 && Fraction <= 1.0) Crossings.Emplace(static_cast<float>(Fraction), Piece);
        }
    }
    Crossings.Sort([](const TPair<float, ADBThrownShield*>& A, const TPair<float, ADBThrownShield*>& B)
    { return A.Key == B.Key ? A.Value->GetUniqueID() < B.Value->GetUniqueID() : A.Key < B.Key; });
    for (const TPair<float, ADBThrownShield*>& Crossing : Crossings)
        if (IsValid(Crossing.Value) && Crossing.Value->InterceptProjectile(Start, UnobstructedEnd, HitDamage, bPiercesGuard))
        {
            Impact(Start + Segment * Crossing.Key);
            return;
        }
    if (bWorldContact)
    {
        if (ADBCharacter* Player = Cast<ADBCharacter>(Hit.GetActor()))
        {
            const FVector IncomingSource = Player->GetPawnViewLocation() - Velocity.GetSafeNormal() * 300.f;
            if (!Player->bDead) Player->ReceiveAttack(HitDamage, IncomingSource, bPiercesGuard, SourceEnemy.Get(), GetUniqueID());
        }
        Impact(Hit.Location);
        return;
    }
    SetActorLocation(End, false);
}

#include "DBProjectile.h"

#include "DBCharacter.h"
#include "DBEnemy.h"
#include "DBGameMode.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
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
        const FVector Scale(24.f / FMath::Max(1.f, Size.X),
            20.f / FMath::Max(1.f, Size.Y), 28.f / FMath::Max(1.f, Size.Z));
        Visual->SetRelativeScale3D(Scale);
        VisualCenterOffset = -Crystal->GetBounds().Origin * Scale;
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
    if (const ADBEnemy* Enemy = Cast<ADBEnemy>(OwnerEnemy)) SpawnRoomId = Enemy->RoomId;
    else if (const ADBGameMode* Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode())) SpawnRoomId = Mode->CurrentRoomId;
    SetActorRotation(Velocity.Rotation());
    bInitialized = true;
    if (GlowMaterial)
    {
        GlowMaterial->SetVectorParameterValue(TEXT("Color"), Color);
        GlowMaterial->SetVectorParameterValue(TEXT("Tint"), Color);
    }
}

void ADBProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bInitialized) { Destroy(); return; }
    if (const ADBGameMode* Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode()))
    {
        // Clearing an encounter removes lingering danger before the reward choice.
        if (Mode->bChoosingReward || (SpawnRoomId != INDEX_NONE && Mode->CurrentRoomId != SpawnRoomId))
        { Destroy(); return; }
        if (Mode->bPaused || Mode->bShowingBuild) return;
    }
    LifeRemaining -= DeltaSeconds;
    if (LifeRemaining <= 0.f) { Destroy(); return; }
    VisualTime += DeltaSeconds;
    Visual->AddLocalRotation(FRotator(0.f, 0.f, DeltaSeconds * 210.f));
    Visual->SetRelativeLocation(Visual->GetRelativeRotation().RotateVector(VisualCenterOffset));
    Streak->SetRelativeScale3D(FVector(0.66f + FMath::Sin(VisualTime * 25.f) * 0.08f, 0.055f, 0.055f));

    const FVector Start = GetActorLocation();
    const FVector End = Start + Velocity * DeltaSeconds;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DBHostileBolt), false, this);
    Params.AddIgnoredActor(SourceEnemy.Get());
    FHitResult Hit;
    if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility,
        FCollisionShape::MakeSphere(Collision->GetScaledSphereRadius()), Params))
    {
        SetActorLocation(Hit.Location);
        if (ADBCharacter* Player = Cast<ADBCharacter>(Hit.GetActor()))
        {
            const FVector IncomingSource = Player->GetPawnViewLocation() - Velocity.GetSafeNormal() * 300.f;
            if (!Player->bDead) Player->ReceiveAttack(HitDamage, IncomingSource, bPiercesGuard, SourceEnemy.Get());
        }
        Destroy();
        return;
    }
    SetActorLocation(End, false);
}

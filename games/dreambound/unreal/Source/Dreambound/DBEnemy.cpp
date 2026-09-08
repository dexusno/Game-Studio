#include "DBEnemy.h"

#include "DBCharacter.h"
#include "DBGameMode.h"
#include "DBProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    const FLinearColor DangerColor(1.f, 0.21f, 0.055f);
    const FLinearColor GroundColor(1.f, 0.04f, 0.16f);
    const FLinearColor OpenColor(0.12f, 1.f, 0.77f);
    const FLinearColor FrostColor(0.28f, 0.75f, 1.f);
    const FLinearColor StormColor(0.58f, 0.3f, 1.f);

    void FitPart(UStaticMeshComponent* Part, const TCHAR* Asset, UStaticMesh* Fallback,
        FVector DesiredSize, FVector DesiredCenter)
    {
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Asset);
        if (!Mesh) Mesh = Fallback;
        if (!Mesh) return;
        Part->SetStaticMesh(Mesh);
        const FBoxSphereBounds Bounds = Mesh->GetBounds();
        const FVector Size = Bounds.BoxExtent * 2.f;
        const FVector Scale(DesiredSize.X / FMath::Max(1.f, Size.X),
            DesiredSize.Y / FMath::Max(1.f, Size.Y), DesiredSize.Z / FMath::Max(1.f, Size.Z));
        Part->SetRelativeScale3D(Scale);
        Part->SetRelativeLocation(DesiredCenter - Bounds.Origin * Scale);
    }
}

ADBEnemy::ADBEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(38.f, 88.f);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    bUseControllerRotationYaw = false;
    AutoPossessAI = EAutoPossessAI::Disabled;
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->bRunPhysicsWithNoController = true;
    Movement->bOrientRotationToMovement = false;
    Movement->bEnablePhysicsInteraction = false;
    Movement->bCanWalkOffLedges = false;
    Movement->MaxWalkSpeed = 245.f;
    Movement->MaxAcceleration = 2200.f;
    Movement->BrakingDecelerationWalking = 2200.f;
    Movement->GroundFriction = 9.f;
    Movement->GravityScale = 1.35f;
    Movement->JumpZVelocity = 410.f;
    Movement->AirControl = 0.25f;
    Movement->bUseRVOAvoidance = true;
    Movement->AvoidanceConsiderationRadius = 180.f;
    Movement->AvoidanceWeight = 0.45f;
    VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SculptedParts"));
    VisualRoot->SetupAttachment(GetCapsuleComponent());
    auto Pivot = [this](const TCHAR* Name, FVector Location)
    {
        USceneComponent* Result = CreateDefaultSubobject<USceneComponent>(FName(Name));
        Result->SetupAttachment(VisualRoot);
        Result->SetRelativeLocation(Location);
        return Result;
    };
    BodyPivot = Pivot(TEXT("BodyPivot"), FVector(0.f, 0.f, 102.f));
    HeadPivot = Pivot(TEXT("HeadPivot"), FVector(5.f, 0.f, 151.f));
    LeftArmPivot = Pivot(TEXT("LeftShoulder"), FVector(0.f, -48.f, 131.f));
    RightArmPivot = Pivot(TEXT("RightShoulder"), FVector(0.f, 48.f, 131.f));
    LeftLegPivot = Pivot(TEXT("LeftHip"), FVector(0.f, -21.f, 65.f));
    RightLegPivot = Pivot(TEXT("RightHip"), FVector(0.f, 21.f, 65.f));
    auto Part = [this](const TCHAR* Name, USceneComponent* Parent)
    {
        UStaticMeshComponent* Result = CreateDefaultSubobject<UStaticMeshComponent>(FName(Name));
        Result->SetupAttachment(Parent);
        Result->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Result->SetGenerateOverlapEvents(false);
        return Result;
    };
    BodyPart = Part(TEXT("Body"), BodyPivot);
    HeadPart = Part(TEXT("Head"), HeadPivot);
    LeftArmPart = Part(TEXT("LeftArm"), LeftArmPivot);
    RightArmPart = Part(TEXT("RightArm"), RightArmPivot);
    LeftLegPart = Part(TEXT("LeftLeg"), LeftLegPivot);
    RightLegPart = Part(TEXT("RightLeg"), RightLegPivot);
    CorePart = Part(TEXT("ExposedCore"), VisualRoot);
    CorePart->SetCastShadow(false);
    WarningMarks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("AttackWarning"));
    WarningMarks->SetupAttachment(GetCapsuleComponent());
    WarningMarks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WarningMarks->SetCastShadow(false);
    EffectMarks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ElementArcs"));
    EffectMarks->SetupAttachment(GetCapsuleComponent());
    EffectMarks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    EffectMarks->SetCastShadow(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        WarningMarks->SetStaticMesh(Cube.Object);
        EffectMarks->SetStaticMesh(Cube.Object);
        for (UStaticMeshComponent* P : { BodyPart.Get(), HeadPart.Get(), LeftArmPart.Get(),
            RightArmPart.Get(), LeftLegPart.Get(), RightLegPart.Get(), CorePart.Get() }) P->SetStaticMesh(Cube.Object);
    }
    Tags.Add(TEXT("DBEnemy"));
}

void ADBEnemy::BeginPlay()
{
    Super::BeginPlay();
    if (!bConfigured) Configure(Kind, RoomId, 1.f);
    BuildVisuals();
}

void ADBEnemy::Configure(EDBEnemyKind InKind, int32 InRoomId, float Difficulty)
{
    const bool bKindChanged = Kind != InKind;
    Kind = InKind;
    RoomId = InRoomId;
    DifficultyScale = FMath::Clamp(Difficulty, 0.65f, 2.5f);
    switch (Kind)
    {
    case EDBEnemyKind::Caster: MaxHealth = 78.f; BaseSpeed = 205.f; AttackDamage = 14.f; break;
    case EDBEnemyKind::Hunter: MaxHealth = 110.f; BaseSpeed = 365.f; AttackDamage = 17.f; break;
    case EDBEnemyKind::Boss: MaxHealth = 820.f; BaseSpeed = 230.f; AttackDamage = 21.f; break;
    default: MaxHealth = 92.f; BaseSpeed = 245.f; AttackDamage = 18.f; break;
    }
    MaxHealth *= DifficultyScale;
    AttackDamage *= FMath::Sqrt(DifficultyScale);
    Health = MaxHealth;
    bDead = false;
    bDeathNotified = false;
    Phase = EDBEnemyPhase::Dormant;
    bVulnerable = false;
    HomePosition = GetActorLocation();
    if (!bArenaBoundsSet) ArenaCenter = HomePosition;
    const uint32 PlacementKey = static_cast<uint32>(FMath::RoundToInt(HomePosition.X)) * 73856093u
        ^ static_cast<uint32>(FMath::RoundToInt(HomePosition.Y)) * 19349663u
        ^ static_cast<uint32>(RoomId) * 83492791u;
    AvoidanceSide = PlacementKey & 1u ? 1.f : -1.f;
    Cooldown = 0.35f + ((PlacementKey >> 3u) & 3u) * 0.16f;
    const float Size = Kind == EDBEnemyKind::Boss ? 1.75f : Kind == EDBEnemyKind::Hunter ? 0.95f : 1.f;
    const float PreviousHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    GetCapsuleComponent()->SetCapsuleSize(38.f * Size, 88.f * Size);
    // Configure follows SpawnActor; growing the guardian must preserve the spawned foot height.
    SetActorLocation(GetActorLocation() + FVector(0.f, 0.f, 88.f * Size - PreviousHalfHeight), false);
    VisualRoot->SetRelativeLocation(FVector(0.f, 0.f, -88.f * Size));
    VisualRoot->SetRelativeScale3D(FVector(Size));
    GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
    GetCharacterMovement()->SetAvoidanceEnabled(true);
    bConfigured = true;
    if (bKindChanged) bVisualsBuilt = false;
    if (HasActorBegunPlay()) BuildVisuals();
}

void ADBEnemy::SetArenaBounds(FVector Center, FVector2D HalfSize)
{
    ArenaCenter = Center;
    ArenaHalfSize = FVector2D(FMath::Max(200.f, HalfSize.X), FMath::Max(200.f, HalfSize.Y));
    bArenaBoundsSet = true;
}

void ADBEnemy::BuildVisuals()
{
    if (bVisualsBuilt) return;
    bVisualsBuilt = true;
    EnemyHitSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/S_EnemyHit.S_EnemyHit"));
    EnemyFireSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/S_EnemyFire.S_EnemyFire"));
    BossTellSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/S_BossTell.S_BossTell"));
    UStaticMesh* Fallback = BodyPart->GetStaticMesh();
    const bool bHunter = Kind == EDBEnemyKind::Hunter;
    const bool bCaster = Kind == EDBEnemyKind::Caster;
    FitPart(BodyPart, TEXT("/Game/Art/Meshes/SM_GuardianBody.SM_GuardianBody"), Fallback,
        bHunter ? FVector(62.f, 49.f, 76.f) : bCaster ? FVector(65.f, 58.f, 84.f) : FVector(78.f, 61.f, 80.f), FVector::ZeroVector);
    FitPart(HeadPart, TEXT("/Game/Art/Meshes/SM_GuardianHead.SM_GuardianHead"), Fallback,
        bCaster ? FVector(48.f, 58.f, 57.f) : FVector(53.f, 54.f, 46.f), FVector::ZeroVector);
    FitPart(LeftArmPart, TEXT("/Game/Art/Meshes/SM_GuardianArm.SM_GuardianArm"), Fallback,
        FVector(29.f, 31.f, 72.f), FVector(0.f, 0.f, -33.f));
    FitPart(RightArmPart, TEXT("/Game/Art/Meshes/SM_GuardianArm.SM_GuardianArm"), Fallback,
        FVector(29.f, 31.f, 72.f), FVector(0.f, 0.f, -33.f));
    FitPart(LeftLegPart, TEXT("/Game/Art/Meshes/SM_GuardianLeg.SM_GuardianLeg"), Fallback,
        FVector(31.f, 33.f, 65.f), FVector(0.f, 0.f, -31.f));
    FitPart(RightLegPart, TEXT("/Game/Art/Meshes/SM_GuardianLeg.SM_GuardianLeg"), Fallback,
        FVector(31.f, 33.f, 65.f), FVector(0.f, 0.f, -31.f));
    FitPart(CorePart, TEXT("/Game/Art/Meshes/SM_Core.SM_Core"), Fallback,
        FVector(24.f, 24.f, 25.f), FVector(43.f, 0.f, 110.f));
    UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Art/Materials/M_CombatGlow.M_CombatGlow"));
    if (!Glow) Glow = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (Glow)
    {
        WarningMaterial = UMaterialInstanceDynamic::Create(Glow, this);
        EffectMaterial = UMaterialInstanceDynamic::Create(Glow, this);
        CoreMaterial = UMaterialInstanceDynamic::Create(Glow, this);
        WarningMarks->SetMaterial(0, WarningMaterial);
        EffectMarks->SetMaterial(0, EffectMaterial);
        CorePart->SetMaterial(0, CoreMaterial);
        WarningMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 2.f);
        EffectMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 3.f);
        CoreMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 1.7f);
    }
}

bool ADBEnemy::IsRoomActive() const
{
    if (!GetWorld()) return false;
    if (const ADBGameMode* Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode()))
        return !Mode->bPaused && !Mode->bChoosingReward && !Mode->bShowingBuild && Mode->CurrentRoomId == RoomId;
    return true;
}

FVector ADBEnemy::FeetLocation() const
{
    return GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
}

FVector ADBEnemy::GroundBelow(FVector Point) const
{
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DBGroundWarning), false, this);
    Params.AddIgnoredActor(Target.Get());
    if (GetWorld()->LineTraceSingleByChannel(Hit, Point + FVector(0.f, 0.f, 110.f),
        Point - FVector(0.f, 0.f, 600.f), ECC_WorldStatic, Params)) return Hit.ImpactPoint + FVector(0.f, 0.f, 4.f);
    return FVector(Point.X, Point.Y, FeetLocation().Z + 4.f);
}

FVector ADBEnemy::KeepInsideArena(FVector Point) const
{
    const float Inset = GetCapsuleComponent()->GetScaledCapsuleRadius() + 32.f;
    Point.X = FMath::Clamp(Point.X, ArenaCenter.X - ArenaHalfSize.X + Inset, ArenaCenter.X + ArenaHalfSize.X - Inset);
    Point.Y = FMath::Clamp(Point.Y, ArenaCenter.Y - ArenaHalfSize.Y + Inset, ArenaCenter.Y + ArenaHalfSize.Y - Inset);
    return Point;
}

FVector ADBEnemy::ShotOrigin() const
{
    return GetActorLocation() + FVector(0.f, 0.f, Kind == EDBEnemyKind::Boss ? 48.f : 26.f)
        + GetActorForwardVector() * (GetCapsuleComponent()->GetScaledCapsuleRadius() + 18.f);
}

bool ADBEnemy::HasSightTo(FVector Point, const AActor* AllowedActor) const
{
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DBEnemySight), false, this);
    FHitResult Hit;
    return !GetWorld()->LineTraceSingleByChannel(Hit, ShotOrigin(), Point, ECC_Visibility, Params)
        || Hit.GetActor() == AllowedActor;
}

void ADBEnemy::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Dt = FMath::Min(DeltaSeconds, 0.1f);
    if (bDead)
    {
        if (const ADBGameMode* Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode()))
            if (Mode->bPaused || Mode->bShowingBuild) return;
        DeathTime += Dt;
        UpdateVisuals(Dt);
        VisualRoot->SetRelativeRotation(FRotator(0.f, 0.f, FMath::Min(88.f, DeathTime * 150.f)));
        if (DeathTime > 2.5f) Destroy();
        return;
    }
    if (!IsRoomActive())
    {
        GetCharacterMovement()->StopMovementImmediately();
        ConsumeMovementInputVector();
        WarningMarks->ClearInstances();
        EffectMarks->ClearInstances();
        return;
    }
    if (!Target.IsValid()) Target = Cast<ADBCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!Target.IsValid() || Target->bDead || FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation()) > FMath::Square(3600.f))
    {
        GetCharacterMovement()->StopMovementImmediately();
        Phase = EDBEnemyPhase::Dormant;
        WarningMarks->ClearInstances();
        return;
    }
    if (Phase == EDBEnemyPhase::Dormant) Phase = EDBEnemyPhase::Approach;
    UpdateStatusEffects(Dt);
    if (bDead) return;
    Cooldown = FMath::Max(0.f, Cooldown - Dt);
    switch (Phase)
    {
    case EDBEnemyPhase::Approach: UpdateApproach(Dt); break;
    case EDBEnemyPhase::Telegraph: UpdateTell(Dt); break;
    case EDBEnemyPhase::Attack: UpdateAttack(Dt); break;
    case EDBEnemyPhase::Recovery:
    case EDBEnemyPhase::Staggered:
        PhaseTime -= Dt;
        GetCharacterMovement()->StopMovementImmediately();
        if (PhaseTime <= 0.f) { Phase = EDBEnemyPhase::Approach; bVulnerable = false; Telegraph.Empty(); }
        break;
    default: break;
    }
    UpdateVisuals(Dt);
}

void ADBEnemy::MoveToward(FVector Point, float DeltaSeconds, float SpeedMultiplier)
{
    MoveDirection(KeepInsideArena(Point) - GetActorLocation(), DeltaSeconds, SpeedMultiplier);
}

void ADBEnemy::MoveDirection(FVector Direction, float DeltaSeconds, float SpeedMultiplier)
{
    Direction.Z = 0.f;
    if (!Direction.Normalize()) return;
    const FVector Start = GetActorLocation();
    const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float Half = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const float LookAhead = FMath::Max(100.f, BaseSpeed * SpeedMultiplier * 0.42f);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DBLocalSteering), false, this);
    Params.AddIgnoredActor(Target.Get());
    auto RouteClear = [&](FVector Candidate)
    {
        const FVector End = Start + Candidate * LookAhead;
        if (FVector::DistSquared2D(End, KeepInsideArena(End)) > 1.f) return false;
        FHitResult Hit;
        return !GetWorld()->SweepSingleByChannel(Hit, Start + FVector(0.f, 0.f, 7.f),
            End + FVector(0.f, 0.f, 7.f), FQuat::Identity, ECC_Visibility,
            FCollisionShape::MakeCapsule(Radius * 0.88f, FMath::Max(Radius, Half - 12.f)), Params);
    };
    FVector Chosen = Direction;
    if (!RouteClear(Chosen))
    {
        bool bFound = false;
        for (float Angle : { 42.f, -42.f, 78.f, -78.f, 112.f, -112.f })
        {
            const FVector Candidate = Direction.RotateAngleAxis(Angle * AvoidanceSide, FVector::UpVector);
            if (RouteClear(Candidate)) { Chosen = Candidate; bFound = true; break; }
        }
        if (!bFound) { GetCharacterMovement()->StopMovementImmediately(); return; }
    }
    if (GetVelocity().SizeSquared2D() < FMath::Square(25.f)) StuckTime += DeltaSeconds;
    else StuckTime = 0.f;
    if (StuckTime > 0.8f) { AvoidanceSide *= -1.f; StuckTime = 0.f; }
    GetCharacterMovement()->MaxWalkSpeed = BaseSpeed * SpeedMultiplier * (1.f - ChillStacks * 0.16f);
    AddMovementInput(Chosen, 1.f, true);
    const FRotator Face(0.f, Direction.Rotation().Yaw, 0.f);
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), Face, DeltaSeconds, 7.f));
}

void ADBEnemy::UpdateApproach(float DeltaSeconds)
{
    if (!Target.IsValid()) return;
    const FVector ToPlayer = Target->GetActorLocation() - GetActorLocation();
    const float Distance = ToPlayer.Size2D();
    const bool bSight = HasSightTo(Target->GetActorLocation(), Target.Get());
    if (Kind == EDBEnemyKind::Caster)
    {
        if (Cooldown <= 0.f && Distance < 1650.f && bSight) { BeginTell(EAttack::Bolt, 1.02f); return; }
        if (Distance < 620.f) MoveDirection(-ToPlayer, DeltaSeconds);
        else if (Distance > 1100.f || !bSight) MoveToward(Target->GetActorLocation(), DeltaSeconds);
        else GetCharacterMovement()->StopMovementImmediately();
    }
    else if (Kind == EDBEnemyKind::Hunter)
    {
        if (Cooldown <= 0.f && Distance < 630.f && bSight) { BeginTell(EAttack::Lunge, 0.78f); return; }
        const FVector Side = FVector::CrossProduct(ToPlayer.GetSafeNormal2D(), FVector::UpVector) * AvoidanceSide;
        MoveToward(Target->GetActorLocation() + Side * (Distance < 950.f ? 330.f : 140.f), DeltaSeconds);
    }
    else if (Kind == EDBEnemyKind::Boss)
    {
        if (Cooldown <= 0.f && bSight && Distance < 2100.f)
        {
            if (Distance < 420.f) BeginTell(EAttack::Slam, 1.1f);
            else if (BossAttackIndex % 3 == 0) BeginTell(EAttack::Salvo, 1.35f);
            else BeginTell(EAttack::Ground, 1.5f);
            BossAttackIndex++;
            return;
        }
        MoveToward(Target->GetActorLocation(), DeltaSeconds, 0.85f);
    }
    else
    {
        if (Cooldown <= 0.f && Distance < 200.f && bSight) { BeginTell(EAttack::Swing, 0.73f); return; }
        MoveToward(Target->GetActorLocation(), DeltaSeconds);
    }
}

void ADBEnemy::BeginTell(EAttack InAttack, float Duration)
{
    Attack = InAttack;
    Phase = EDBEnemyPhase::Telegraph;
    TellTime = TellDuration = Duration;
    bVulnerable = false;
    GetCharacterMovement()->StopMovementImmediately();
    LockedDirection = Target.IsValid() ? (Target->GetActorLocation() - ShotOrigin()).GetSafeNormal() : GetActorForwardVector();
    bAimLocked = InAttack == EAttack::Ground || InAttack == EAttack::Slam;
    TellTarget = Target.IsValid() ? Target->GetActorLocation() : GetActorLocation() + GetActorForwardVector() * 300.f;
    TellRadius = 0.f;
    switch (Attack)
    {
    case EAttack::Swing: Telegraph = TEXT("SWING - DEFLECT / STEP BACK"); TellRadius = 185.f; break;
    case EAttack::Bolt: Telegraph = TEXT("AIMED BOLT - DEFLECT / MOVE"); break;
    case EAttack::Lunge: Telegraph = TEXT("LUNGE - SIDESTEP / DEFLECT"); break;
    case EAttack::Salvo: Telegraph = TEXT("AIMED SALVO - LEAVE THE LANES"); break;
    case EAttack::Slam:
        Telegraph = TEXT("SLAM - JUMP OR DASH OUT"); TellRadius = 380.f; TellTarget = GroundBelow(GetActorLocation()); break;
    case EAttack::Ground:
        Telegraph = TEXT("RUPTURE - LEAVE THE MARK"); TellRadius = 305.f; TellTarget = GroundBelow(TellTarget); break;
    default: break;
    }
    if (Kind == EDBEnemyKind::Boss)
    {
        if (BossTellSound) UGameplayStatics::PlaySoundAtLocation(this, BossTellSound, GetActorLocation(), 0.6f,
            Attack == EAttack::Salvo ? 1.f : Attack == EAttack::Ground ? 0.85f : 0.7f);
        if (ADBGameMode* Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode()))
            Mode->NotifyEvent(Telegraph, Attack == EAttack::Slam || Attack == EAttack::Ground ? GroundColor : DangerColor);
    }
}

void ADBEnemy::UpdateTell(float DeltaSeconds)
{
    if (!Target.IsValid()) { BeginRecovery(0.4f); return; }
    GetCharacterMovement()->StopMovementImmediately();
    if (!bAimLocked)
    {
        LockedDirection = (Target->GetActorLocation() - ShotOrigin()).GetSafeNormal();
        TellTarget = Target->GetActorLocation();
        if (TellTime <= TellDuration * 0.5f) bAimLocked = true;
    }
    if (Attack != EAttack::Ground && Attack != EAttack::Slam)
        SetActorRotation(FRotator(0.f, LockedDirection.Rotation().Yaw, 0.f));
    TellTime = FMath::Max(0.f, TellTime - DeltaSeconds);
    if (TellTime <= 0.f) BeginAttack();
}

void ADBEnemy::BeginAttack()
{
    Phase = EDBEnemyPhase::Attack;
    bAimLocked = true;
    AttackElapsed = 0.f;
    NextShotTime = 0.f;
    ShotsRemaining = Attack == EAttack::Salvo ? 3 : 1;
    bHitAttempted = false;
    if (Attack == EAttack::Lunge)
    {
        LockedDirection.Z = 0.f;
        LockedDirection.Normalize();
        GetCharacterMovement()->MaxWalkSpeed = 1050.f * (1.f - ChillStacks * 0.12f);
        GetCharacterMovement()->MaxAcceleration = 14000.f;
    }
}

void ADBEnemy::FireBolt(FVector Direction, float Damage, FLinearColor Color)
{
    if (!Target.IsValid() || Target->bDead) return;
    if (EnemyFireSound) UGameplayStatics::PlaySoundAtLocation(this, EnemyFireSound, ShotOrigin(), 0.42f,
        Kind == EDBEnemyKind::Boss ? 0.82f : 1.f);
    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (ADBProjectile* Bolt = GetWorld()->SpawnActor<ADBProjectile>(ShotOrigin(), Direction.Rotation(), Params))
        Bolt->Initialize(Direction, Kind == EDBEnemyKind::Boss ? 900.f : 790.f, Damage, false, this, Color);
}

bool ADBEnemy::TryMeleeHit(float Range, float ConeCosine, float Damage, bool bUnblockable)
{
    if (!Target.IsValid() || Target->bDead) return false;
    const FVector Difference = Target->GetActorLocation() - GetActorLocation();
    if (Difference.Size2D() > Range || FMath::Abs(Difference.Z) > 165.f) return false;
    if (FVector::DotProduct(Difference.GetSafeNormal2D(), LockedDirection.GetSafeNormal2D()) < ConeCosine) return false;
    if (!HasSightTo(Target->GetActorLocation(), Target.Get())) return false;
    Target->ReceiveAttack(Damage, GetActorLocation(), bUnblockable, this);
    return true;
}

void ADBEnemy::DetonateGround()
{
    GroundPulseTime = 0.38f;
    if (!Target.IsValid() || Target->bDead) return;
    const FVector PlayerFeet = Target->GetActorLocation()
        - FVector(0.f, 0.f, Target->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    const bool bNear = FVector::DistSquared2D(PlayerFeet, TellTarget) < FMath::Square(TellRadius + 16.f);
    const bool bLow = PlayerFeet.Z < TellTarget.Z + 98.f;
    if (bNear && bLow && HasSightTo(Target->GetActorLocation(), Target.Get()))
        Target->ReceiveAttack(AttackDamage * (Attack == EAttack::Slam ? 1.6f : 1.25f), TellTarget, true, this);
}

void ADBEnemy::UpdateAttack(float DeltaSeconds)
{
    AttackElapsed += DeltaSeconds;
    if (Attack == EAttack::Bolt || Attack == EAttack::Salvo)
    {
        while (ShotsRemaining > 0 && AttackElapsed >= NextShotTime)
        {
            const float Spread = Attack == EAttack::Salvo ? (2 - ShotsRemaining) * 7.f : 0.f;
            FireBolt(LockedDirection.RotateAngleAxis(Spread, FVector::UpVector), AttackDamage, DangerColor);
            ShotsRemaining--;
            NextShotTime += 0.2f;
        }
        if (AttackElapsed >= (Attack == EAttack::Salvo ? 0.65f : 0.18f))
            BeginRecovery(Kind == EDBEnemyKind::Boss ? 1.7f : 1.35f);
    }
    else if (Attack == EAttack::Lunge)
    {
        // Commitment follows the visible locked lane; CharacterMovement stops on solid cover.
        const FVector Next = GetActorLocation() + LockedDirection * 100.f;
        if (FVector::DistSquared2D(Next, KeepInsideArena(Next)) < 1.f)
            AddMovementInput(LockedDirection, 1.f, true);
        if (!bHitAttempted && TryMeleeHit(150.f, 0.15f, AttackDamage)) bHitAttempted = true;
        if (AttackElapsed >= 0.43f || (AttackElapsed > 0.15f && GetVelocity().Size2D() < 30.f)) BeginRecovery(1.05f);
    }
    else if (Attack == EAttack::Swing)
    {
        if (!bHitAttempted && AttackElapsed >= 0.065f)
        {
            TryMeleeHit(190.f, 0.25f, AttackDamage);
            bHitAttempted = true;
        }
        if (AttackElapsed > 0.28f) BeginRecovery(0.85f);
    }
    else
    {
        if (!bHitAttempted) { DetonateGround(); bHitAttempted = true; }
        if (AttackElapsed > 0.36f) BeginRecovery(1.85f);
    }
}

void ADBEnemy::BeginRecovery(float Duration)
{
    Phase = EDBEnemyPhase::Recovery;
    PhaseTime = Duration;
    bVulnerable = true;
    Telegraph = TEXT("EXPOSED - COUNTERATTACK");
    TellTime = 0.f;
    GetCharacterMovement()->MaxAcceleration = 2200.f;
    GetCharacterMovement()->StopMovementImmediately();
    Cooldown = 0.2f;
}

void ADBEnemy::Stagger(float Duration)
{
    if (bDead) return;
    // The guardian retains committed tells but still takes damage and every elemental status.
    if (Kind == EDBEnemyKind::Boss && (Phase == EDBEnemyPhase::Telegraph || Phase == EDBEnemyPhase::Attack)) return;
    Phase = EDBEnemyPhase::Staggered;
    PhaseTime = FMath::Min(1.2f, FMath::Max(PhaseTime, Duration));
    bVulnerable = true;
    Telegraph = TEXT("STAGGERED - COUNTERATTACK");
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->MaxAcceleration = 2200.f;
}

void ADBEnemy::UpdateStatusEffects(float DeltaSeconds)
{
    ChillRemaining = FMath::Max(0.f, ChillRemaining - DeltaSeconds);
    StormRemaining = FMath::Max(0.f, StormRemaining - DeltaSeconds);
    if (ChillRemaining <= 0.f) { ChillStacks = 0; bChillStaggered = false; }
    if (StormRemaining <= 0.f) StormMarks = 0;
    if (BurnRemaining > 0.f)
    {
        const float Active = FMath::Min(BurnRemaining, DeltaSeconds);
        BurnRemaining -= Active;
        BurnTickTime += Active;
        while (BurnTickTime >= 0.65f && !bDead)
        {
            BurnTickTime -= 0.65f;
            DealHealthDamage(BurnTickDamage);
        }
    }
}

void ADBEnemy::ApplyCombatHit(const FDBHit& Hit)
{
    if (bDead || !IsRoomActive() || !FMath::IsFinite(Hit.Damage) || Hit.Damage <= 0.f) return;
    const int32 PreviousChill = ChillStacks;
    const int32 PreviousStorm = StormMarks;
    int32 ConsumedChill = 0;
    int32 ConsumedStorm = 0;
    bool bFracture = false;
    float ShatterBonus = 0.f;
    if (!Hit.bSecondary)
    {
        if (Hit.bImpact && PreviousChill > 0)
        {
            ConsumedChill = PreviousChill;
            ShatterBonus = PreviousChill * 9.f;
        }
        bFracture = Hit.bStormfracture && (PreviousChill > 0 || PreviousStorm > 0);
        if (bFracture)
        {
            ConsumedChill = PreviousChill;
            ConsumedStorm = PreviousStorm;
        }
        else if (Hit.Element == EDBElement::Storm && PreviousStorm > 0) ConsumedStorm = PreviousStorm;
        // Consume the previous state atomically before applying damage or any nearby secondary hit.
        if (ConsumedChill > 0) { ChillStacks = 0; ChillRemaining = 0.f; bChillStaggered = false; }
        if (ConsumedStorm > 0) { StormMarks = 0; StormRemaining = 0.f; }
    }
    HitFlash = 0.16f;
    if (EnemyHitSound && HitSoundCooldown <= 0.f)
    {
        UGameplayStatics::PlaySoundAtLocation(this, EnemyHitSound, GetActorLocation(), 0.32f, Hit.bImpact ? 0.8f : 1.f);
        HitSoundCooldown = 0.085f;
    }
    const float Damage = Hit.Damage * (bVulnerable ? 1.35f : 1.f) + ShatterBonus;
    DealHealthDamage(Damage);
    if (!bDead)
    {
        if (Hit.Element == EDBElement::Frost)
        {
            ChillStacks = FMath::Min(3, ChillStacks + 1);
            ChillRemaining = 4.f;
            if (ChillStacks == 3 && !bChillStaggered)
            {
                bChillStaggered = true;
                Stagger(Kind == EDBEnemyKind::Boss ? 0.25f : 0.5f);
            }
        }
        else if (Hit.Element == EDBElement::Ember)
        {
            if (BurnRemaining <= 0.f) BurnTickTime = 0.f;
            BurnRemaining = 4.f;
            BurnTickDamage = FMath::Max(BurnTickDamage, FMath::Clamp(Hit.Damage * 0.22f, 3.f, 14.f));
            BurnInstigator = Hit.InstigatorActor;
        }
        else if (Hit.Element == EDBElement::Storm)
        {
            StormMarks = FMath::Min(3, StormMarks + 1);
            StormRemaining = 5.f;
        }
        if (Hit.bImpact && !Hit.bSecondary)
        {
            Stagger(Kind == EDBEnemyKind::Boss ? 0.25f : 0.45f);
            const FVector Away = (GetActorLocation() - Hit.Source).GetSafeNormal2D();
            LaunchCharacter(Away * (Kind == EDBEnemyKind::Boss ? 100.f : 290.f), true, false);
        }
    }
    if (bFracture)
        ChainToNearby(Hit.Damage * 0.55f + ConsumedChill * 6.f + ConsumedStorm * 8.f, 3, 720.f, Hit.InstigatorActor);
    else if (ConsumedStorm > 0)
        ChainToNearby(Hit.Damage * 0.42f + ConsumedStorm * 6.f, 2, 620.f, Hit.InstigatorActor);
    if (ShatterBonus > 0.f)
        if (ADBGameMode* Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode())) Mode->NotifyEvent(TEXT("SHATTER - CHILL CONSUMED"), FrostColor);
}

void ADBEnemy::ChainToNearby(float Damage, int32 MaxTargets, float Radius, AActor* HitInstigator)
{
    TArray<TPair<float, ADBEnemy*>> Candidates;
    const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 20.f);
    for (TActorIterator<ADBEnemy> It(GetWorld()); It; ++It)
    {
        ADBEnemy* Other = *It;
        if (Other == this || Other->bDead || Other->RoomId != RoomId) continue;
        const float DistanceSquared = FVector::DistSquared(Start, Other->GetActorLocation());
        if (DistanceSquared <= FMath::Square(Radius) && HasSightTo(Other->GetActorLocation(), Other))
            Candidates.Emplace(DistanceSquared, Other);
    }
    Candidates.Sort([](const TPair<float, ADBEnemy*>& A, const TPair<float, ADBEnemy*>& B)
    {
        return A.Key == B.Key ? A.Value->GetUniqueID() < B.Value->GetUniqueID() : A.Key < B.Key;
    });
    for (int32 Index = 0; Index < FMath::Min(MaxTargets, Candidates.Num()); ++Index)
    {
        ADBEnemy* Other = Candidates[Index].Value;
        const FVector End = Other->GetActorLocation();
        FDBHit Secondary;
        Secondary.Damage = Damage;
        Secondary.Element = EDBElement::Storm;
        Secondary.bSecondary = true;
        Secondary.bStormfracture = false;
        Secondary.Source = Start;
        Secondary.Direction = (End - Start).GetSafeNormal();
        Secondary.InstigatorActor = HitInstigator;
        ArcVisuals.Add({Start, End, 0.3f});
        Other->ApplyCombatHit(Secondary);
    }
}

void ADBEnemy::DealHealthDamage(float Damage)
{
    if (bDead || Damage <= 0.f) return;
    Health = FMath::Max(0.f, Health - Damage);
    if (Health <= 0.f) Die();
}

void ADBEnemy::Die()
{
    if (bDead) return;
    bDead = true;
    Phase = EDBEnemyPhase::Dead;
    bVulnerable = false;
    Telegraph.Empty();
    WarningMarks->ClearInstances();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetCharacterMovement()->DisableMovement();
    if (!bDeathNotified)
    {
        bDeathNotified = true;
        if (ADBGameMode* Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode())) Mode->NotifyEnemyKilled(this);
    }
}

void ADBEnemy::AddLine(UInstancedStaticMeshComponent* Component, FVector Start, FVector End, float Thickness)
{
    const FVector Delta = End - Start;
    const float Length = Delta.Size();
    if (Length < 0.1f) return;
    const FTransform World(Delta.Rotation(), (Start + End) * 0.5f, FVector(Length / 100.f, Thickness / 100.f, Thickness / 100.f));
    Component->AddInstance(World, true);
}

void ADBEnemy::AddRing(UInstancedStaticMeshComponent* Component, FVector Center, float Radius, float Thickness, int32 Segments)
{
    for (int32 Index = 0; Index < Segments; ++Index)
    {
        const float A = Index * 2.f * PI / Segments;
        const float B = (Index + 0.76f) * 2.f * PI / Segments;
        AddLine(Component, Center + FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * Radius,
            Center + FVector(FMath::Cos(B), FMath::Sin(B), 0.f) * Radius, Thickness);
    }
}

void ADBEnemy::UpdateWarningGeometry()
{
    WarningMarks->ClearInstances();
    if (Phase != EDBEnemyPhase::Telegraph && GroundPulseTime <= 0.f) return;
    const float Progress = TellDuration > 0.f ? 1.f - TellTime / TellDuration : 1.f;
    const bool bGround = Attack == EAttack::Ground || Attack == EAttack::Slam;
    if (WarningMaterial)
    {
        const FLinearColor Color = bGround ? GroundColor : DangerColor;
        WarningMaterial->SetVectorParameterValue(TEXT("Color"), Color * (bAimLocked ? 1.4f : 0.85f));
        WarningMaterial->SetVectorParameterValue(TEXT("Tint"), Color);
    }
    if (bGround)
    {
        AddRing(WarningMarks, TellTarget, TellRadius, 7.f);
        AddRing(WarningMarks, TellTarget + FVector(0.f, 0.f, 2.f), FMath::Max(18.f, TellRadius * Progress), 4.f, 20);
        const float Cross = GroundPulseTime > 0.f ? TellRadius * 0.8f : 42.f;
        AddLine(WarningMarks, TellTarget - FVector(Cross, 0.f, 0.f), TellTarget + FVector(Cross, 0.f, 0.f), 5.f);
        AddLine(WarningMarks, TellTarget - FVector(0.f, Cross, 0.f), TellTarget + FVector(0.f, Cross, 0.f), 5.f);
    }
    else if (Attack == EAttack::Swing)
    {
        const FVector Center = FeetLocation() + FVector(0.f, 0.f, 5.f);
        for (int32 Index = -4; Index <= 4; ++Index)
        {
            const FVector A = LockedDirection.GetSafeNormal2D().RotateAngleAxis(Index * 14.f, FVector::UpVector);
            const FVector B = LockedDirection.GetSafeNormal2D().RotateAngleAxis(Index * 14.f + 10.f, FVector::UpVector);
            AddLine(WarningMarks, Center + A * 185.f, Center + B * 185.f, 5.f);
        }
    }
    else
    {
        const FVector Start = ShotOrigin();
        const float Range = Attack == EAttack::Lunge ? 490.f : 1600.f;
        const int32 Lanes = Attack == EAttack::Salvo ? 3 : 1;
        for (int32 Lane = 0; Lane < Lanes; ++Lane)
        {
            FVector Direction = LockedDirection.RotateAngleAxis(Lanes == 3 ? (Lane - 1) * 7.f : 0.f, FVector::UpVector);
            FVector End = Start + Direction * Range;
            FHitResult Hit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(DBWarningLane), false, this);
            if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params)) End = Hit.ImpactPoint;
            AddLine(WarningMarks, Start, End, bAimLocked ? 3.8f : 2.f);
            if (Attack == EAttack::Lunge)
            {
                const FVector FloorStart = FeetLocation() + FVector(0.f, 0.f, 5.f);
                const FVector FloorEnd = FloorStart + Direction.GetSafeNormal2D() * Range;
                const FVector Side = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal() * 40.f;
                AddLine(WarningMarks, FloorEnd - Direction * 70.f + Side, FloorEnd, 7.f);
                AddLine(WarningMarks, FloorEnd - Direction * 70.f - Side, FloorEnd, 7.f);
            }
        }
    }
}

void ADBEnemy::UpdateVisuals(float DeltaSeconds)
{
    VisualTime += DeltaSeconds;
    HitFlash = FMath::Max(0.f, HitFlash - DeltaSeconds);
    HitSoundCooldown = FMath::Max(0.f, HitSoundCooldown - DeltaSeconds);
    GroundPulseTime = FMath::Max(0.f, GroundPulseTime - DeltaSeconds);
    const float Walking = FMath::Clamp(GetVelocity().Size2D() / FMath::Max(1.f, BaseSpeed), 0.f, 1.f);
    const float Stride = FMath::Sin(VisualTime * (Kind == EDBEnemyKind::Hunter ? 12.f : 8.f)) * Walking;
    const float TellProgress = Phase == EDBEnemyPhase::Telegraph && TellDuration > 0.f ? 1.f - TellTime / TellDuration : 0.f;
    float LeftArmPitch = Stride * 25.f;
    float RightArmPitch = -Stride * 25.f;
    if (Phase == EDBEnemyPhase::Telegraph)
    {
        if (Attack == EAttack::Swing) RightArmPitch = -70.f - TellProgress * 65.f;
        else if (Attack == EAttack::Ground || Attack == EAttack::Slam) LeftArmPitch = RightArmPitch = -105.f - TellProgress * 55.f;
        else LeftArmPitch = RightArmPitch = 85.f;
    }
    if (Phase == EDBEnemyPhase::Attack && (Attack == EAttack::Swing || Attack == EAttack::Slam))
        RightArmPitch = LeftArmPitch = FMath::Lerp(-150.f, 25.f, FMath::Clamp(AttackElapsed / 0.18f, 0.f, 1.f));
    if (bVulnerable) { LeftArmPitch = 18.f; RightArmPitch = 18.f; }
    LeftArmPivot->SetRelativeRotation(FRotator(LeftArmPitch, 0.f, bVulnerable ? -28.f : -8.f));
    RightArmPivot->SetRelativeRotation(FRotator(RightArmPitch, 0.f, bVulnerable ? 28.f : 8.f));
    LeftLegPivot->SetRelativeRotation(FRotator(-Stride * 29.f, 0.f, 0.f));
    RightLegPivot->SetRelativeRotation(FRotator(Stride * 29.f, 0.f, 0.f));
    BodyPivot->SetRelativeRotation(FRotator(bVulnerable ? 14.f : Kind == EDBEnemyKind::Hunter ? -15.f : Stride * 3.f,
        0.f, HitFlash > 0.f ? FMath::Sin(HitFlash * 100.f) * 4.f : Stride * 2.f));
    HeadPivot->SetRelativeRotation(FRotator(bVulnerable ? 10.f : -TellProgress * 8.f, FMath::Sin(VisualTime * 1.2f) * 3.f, 0.f));
    if (CoreMaterial)
    {
        FLinearColor Color = Kind == EDBEnemyKind::Hunter ? FLinearColor(1.f, 0.58f, 0.1f) : DangerColor;
        if (ChillStacks > 0) Color = FrostColor;
        if (StormMarks > 0) Color = StormColor;
        if (BurnRemaining > 0.f) Color = FLinearColor(1.f, 0.15f, 0.025f);
        if (bVulnerable) Color = OpenColor;
        if (HitFlash > 0.f) Color = FLinearColor::White;
        CoreMaterial->SetVectorParameterValue(TEXT("Color"), Color);
        CoreMaterial->SetVectorParameterValue(TEXT("Tint"), Color);
        CoreMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), bVulnerable ? 3.5f : 1.7f);
    }
    UpdateWarningGeometry();
    EffectMarks->ClearInstances();
    if (EffectMaterial)
    {
        EffectMaterial->SetVectorParameterValue(TEXT("Color"), StormColor);
        EffectMaterial->SetVectorParameterValue(TEXT("Tint"), StormColor);
    }
    for (int32 Index = ArcVisuals.Num() - 1; Index >= 0; --Index)
    {
        FArcVisual& Arc = ArcVisuals[Index];
        Arc.Remaining -= DeltaSeconds;
        if (Arc.Remaining <= 0.f) { ArcVisuals.RemoveAtSwap(Index); continue; }
        const FVector Middle = (Arc.Start + Arc.End) * 0.5f + FVector(0.f, 0.f, 35.f);
        AddLine(EffectMarks, Arc.Start, Middle, 6.f);
        AddLine(EffectMarks, Middle, Arc.End, 6.f);
    }
    const FVector Crown = GetActorLocation() + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 24.f);
    for (int32 Mark = 0; Mark < ChillStacks + StormMarks; ++Mark)
    {
        const float Angle = VisualTime * 1.5f + Mark * 2.f * PI / FMath::Max(1, ChillStacks + StormMarks);
        const FVector At = Crown + FVector(FMath::Cos(Angle) * 35.f, FMath::Sin(Angle) * 35.f, 0.f);
        AddLine(EffectMarks, At - FVector(0.f, 0.f, 8.f), At + FVector(0.f, 0.f, 8.f), Mark < ChillStacks ? 6.f : 10.f);
    }
}

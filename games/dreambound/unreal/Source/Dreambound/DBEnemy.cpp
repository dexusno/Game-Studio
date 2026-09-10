#include "DBEnemy.h"

#include "DBCharacter.h"
#include "DBCombatEffect.h"
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
    const FLinearColor InterceptColor(1.f, 0.66f, 0.12f);
    uint32 EnemyAttackSequence = 0;

    void AddElementShard(UInstancedStaticMeshComponent* Component, FVector At, FRotator Rotation, FVector Size)
    {
        if (!Component->GetStaticMesh()) return;
        const FBoxSphereBounds Bounds = Component->GetStaticMesh()->GetBounds();
        const FVector MeshSize = Bounds.BoxExtent * 2.f;
        const FVector Scale(Size.X / FMath::Max(1.0, MeshSize.X), Size.Y / FMath::Max(1.0, MeshSize.Y),
            Size.Z / FMath::Max(1.0, MeshSize.Z));
        Component->AddInstance(FTransform(Rotation, At - Rotation.RotateVector(Bounds.Origin * Scale), Scale), true);
    }

    void FitPart(UStaticMeshComponent* Part, const TCHAR* Asset, UStaticMesh* Fallback,
        FVector FallbackSize, FVector FallbackCenter, float AuthoredScale = 1.f)
    {
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Asset);
        if (Mesh)
        {
            // Preserve the authored shoulder/hip/neck pivot and proportions.
            Part->SetStaticMesh(Mesh);
            Part->SetRelativeScale3D(FVector(AuthoredScale));
            Part->SetRelativeLocation(FVector::ZeroVector);
            return;
        }
        if (!Mesh) Mesh = Fallback;
        if (!Mesh) return;
        Part->SetStaticMesh(Mesh);
        const FBoxSphereBounds Bounds = Mesh->GetBounds();
        const FVector Size = Bounds.BoxExtent * 2.f;
        const FVector Scale(FallbackSize.X / FMath::Max(1.f, Size.X),
            FallbackSize.Y / FMath::Max(1.f, Size.Y), FallbackSize.Z / FMath::Max(1.f, Size.Z));
        Part->SetRelativeScale3D(Scale);
        Part->SetRelativeLocation(FallbackCenter - Bounds.Origin * Scale);
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
    BodyPivot = Pivot(TEXT("BodyPivot"), FVector(0.f, 0.f, 94.f));
    HeadPivot = Pivot(TEXT("HeadPivot"), FVector(0.f, 0.f, 62.f));
    HeadPivot->SetupAttachment(BodyPivot);
    LeftArmPivot = Pivot(TEXT("LeftShoulder"), FVector(0.f, -36.f, 52.f));
    LeftArmPivot->SetupAttachment(BodyPivot);
    RightArmPivot = Pivot(TEXT("RightShoulder"), FVector(0.f, 36.f, 52.f));
    RightArmPivot->SetupAttachment(BodyPivot);
    LeftLegPivot = Pivot(TEXT("LeftHip"), FVector(0.f, -18.f, 94.f));
    RightLegPivot = Pivot(TEXT("RightHip"), FVector(0.f, 18.f, 94.f));
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
    CorePart = Part(TEXT("ExposedCore"), BodyPivot);
    CorePart->SetCastShadow(false);
    ChargePart = Part(TEXT("ChargedVolley"), BodyPivot);
    ChargePart->SetCastShadow(false);
    ChargePart->SetVisibility(false);
    WarningMarks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("AttackWarning"));
    WarningMarks->SetupAttachment(GetCapsuleComponent());
    WarningMarks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WarningMarks->SetCastShadow(false);
    EffectMarks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ElementArcs"));
    EffectMarks->SetupAttachment(GetCapsuleComponent());
    EffectMarks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    EffectMarks->SetCastShadow(false);
    FrostMarks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PersistentFrost"));
    EmberMarks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PersistentEmber"));
    for (UInstancedStaticMeshComponent* Component : { FrostMarks.Get(), EmberMarks.Get() })
    {
        Component->SetupAttachment(GetCapsuleComponent());
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->SetCastShadow(false);
        Component->SetCanEverAffectNavigation(false);
    }
    // Load mutable source assets when the enemy is configured, not in the editor CDO.

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
    case EDBEnemyKind::Caster: MaxHealth = 185.f; BaseSpeed = 285.f; AttackDamage = 15.f; break;
    case EDBEnemyKind::Hunter: MaxHealth = 240.f; BaseSpeed = 365.f; AttackDamage = 17.f; break;
    case EDBEnemyKind::Boss: MaxHealth = 650.f; BaseSpeed = 230.f; AttackDamage = 21.f; break;
    default: MaxHealth = 220.f; BaseSpeed = 280.f; AttackDamage = 23.f; break;
    }
    MaxHealth *= DifficultyScale;
    AttackDamage *= FMath::Sqrt(DifficultyScale);
    Health = MaxHealth;
    bDead = false;
    bDeathNotified = false;
    bDeathLanded = false;
    bNeedsReposition = bRepositioning = false;
    bHitAttempted = false;
    bChillStaggered = false;
    bAimLocked = false;
    bShieldInterceptionReady = false;
    Attack = EAttack::None;
    ChillStacks = StormMarks = ShotsRemaining = BossAttackIndex = 0;
    ChillRemaining = StormRemaining = BurnRemaining = BurnTickTime = DeathTime = 0.f;
    ReactionTime = ReactionStrength = KnockbackTime = HitFlash = HitSoundCooldown = 0.f;
    MeleeSetupTime = RepositionTime = GaitPhase = GaitBlend = AttackKick = 0.f;
    TellTime = TellDuration = PhaseTime = AttackElapsed = 0.f;
    SteeringTime = StuckTime = GroundPulseTime = NextShotTime = VisualTime = 0.f;
    BurnTickDamage = 4.f;
    BurnPulse = ElementSoundCooldown = 0.f;
    InterceptionCooldown = 3.f;
    ActiveAttackId = 0;
    BurnInstigator.Reset();
    SteeringDirection = FVector::ZeroVector;
    ChargePart->SetVisibility(false);
    ClearElementVisuals();
    DeathStartPose.Reset();
    Telegraph.Empty();
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
    VisualScale = 0.9f * Size;
    VisualRoot->SetRelativeScale3D(FVector(VisualScale));
    VisualRoot->SetRelativeRotation(FRotator::ZeroRotator);
    BodyPivot->SetRelativeLocation(FVector(0.f, 0.f, 94.f));
    HeadPivot->SetRelativeLocation(FVector(0.f, 0.f, 62.f));
    LeftLegPivot->SetRelativeLocation(FVector(0.f, -18.f, 94.f));
    RightLegPivot->SetRelativeLocation(FVector(0.f, 18.f, 94.f));
    for (USceneComponent* Joint : { BodyPivot.Get(), HeadPivot.Get(), LeftArmPivot.Get(),
        RightArmPivot.Get(), LeftLegPivot.Get(), RightLegPivot.Get() }) Joint->SetRelativeRotation(FRotator::ZeroRotator);
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->GroundFriction = 9.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2200.f;
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
    EnemyHitSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Reverie/S_EnemyHit.S_EnemyHit"));
    EnemyFireSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Reverie/S_EnemyFire.S_EnemyFire"));
    BossTellSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Reverie/S_BossTell.S_BossTell"));
    EnemyTellSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Reverie/S_EnemyTell.S_EnemyTell"));
    EnemyDefeatSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Reverie/S_EnemyDefeat.S_EnemyDefeat"));
    BodyImpactSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Reverie/S_Impact.S_Impact"));
    UStaticMesh* Fallback = LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    WarningMarks->SetStaticMesh(Fallback);
    EffectMarks->SetStaticMesh(Fallback);
    const bool bHunter = Kind == EDBEnemyKind::Hunter;
    const bool bCaster = Kind == EDBEnemyKind::Caster;
    FitPart(BodyPart, TEXT("/Game/Art/PreferredCombat/Meshes/SM_GuardianBody.SM_GuardianBody"), Fallback,
        FVector(48.f, 62.f, 76.f), FVector(0.f, 0.f, 30.f));
    FitPart(HeadPart, TEXT("/Game/Art/PreferredCombat/Meshes/SM_GuardianHead.SM_GuardianHead"), Fallback,
        FVector(28.f, 29.f, 39.f), FVector(0.f, 0.f, 19.5f), bCaster ? 1.07f : 1.f);
    FitPart(LeftArmPart, TEXT("/Game/Art/PreferredCombat/Meshes/SM_GuardianArm.SM_GuardianArm"), Fallback,
        FVector(35.f, 36.f, 97.f), FVector(0.f, 0.f, -30.f), bCaster ? 0.94f : 1.f);
    FitPart(RightArmPart, TEXT("/Game/Art/PreferredCombat/Meshes/SM_GuardianArm.SM_GuardianArm"), Fallback,
        FVector(35.f, 36.f, 97.f), FVector(0.f, 0.f, -30.f), bCaster ? 0.94f : bHunter ? 1.f : 1.06f);
    FitPart(LeftLegPart, TEXT("/Game/Art/PreferredCombat/Meshes/SM_GuardianLeg.SM_GuardianLeg"), Fallback,
        FVector(39.f, 28.f, 102.f), FVector(0.f, 0.f, -43.f));
    FitPart(RightLegPart, TEXT("/Game/Art/PreferredCombat/Meshes/SM_GuardianLeg.SM_GuardianLeg"), Fallback,
        FVector(39.f, 28.f, 102.f), FVector(0.f, 0.f, -43.f));
    FitPart(CorePart, TEXT("/Game/Art/PreferredCombat/Meshes/SM_Core.SM_Core"), Fallback,
        FVector(6.f, 13.f, 13.f), FVector::ZeroVector, 1.45f);
    CorePart->SetRelativeLocation(FVector(29.f, 0.f, 39.f));
    FitPart(ChargePart, TEXT("/Game/Art/PreferredCombat/Meshes/SM_Core.SM_Core"), Fallback,
        FVector(6.f, 13.f, 13.f), FVector::ZeroVector, 1.5f);
    ChargePart->SetRelativeLocation(FVector(78.f, 0.f, 37.f));
    LeftArmPivot->SetRelativeLocation(FVector(0.f, bCaster ? -31.f : -39.f, 52.f));
    RightArmPivot->SetRelativeLocation(FVector(0.f, bCaster ? 31.f : 39.f, 52.f));
    UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Art/PreferredCombat/Materials/M_CombatGlow.M_CombatGlow"));
    if (Glow)
    {
        WarningMaterial = UMaterialInstanceDynamic::Create(Glow, this);
        CoreMaterial = UMaterialInstanceDynamic::Create(Glow, this);
        WarningMarks->SetMaterial(0, WarningMaterial);
        for (int32 Index = 0; Index < CorePart->GetNumMaterials(); ++Index)
        {
            const TArray<FStaticMaterial>& Slots = CorePart->GetStaticMesh()->GetStaticMaterials();
            const FName Slot = Slots.IsValidIndex(Index) ? Slots[Index].MaterialSlotName : NAME_None;
            if (Slot.ToString().Contains(TEXT("Core")) || CorePart->GetNumMaterials() == 1) CorePart->SetMaterial(Index, CoreMaterial);
        }
        for (int32 Index = 0; Index < ChargePart->GetNumMaterials(); ++Index) ChargePart->SetMaterial(Index, WarningMaterial);
        WarningMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 2.f);
        CoreMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 1.7f);
    }
    if (UStaticMesh* Crystal = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/PreferredCombat/Meshes/SM_Crystal.SM_Crystal")))
    {
        FrostMarks->SetStaticMesh(Crystal);
        EmberMarks->SetStaticMesh(Crystal);
    }
    // Element silhouettes have their own materials; hit and recovery flashes cannot recolor them.
    if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/PreferredCombat/Materials/M_Frost.M_Frost")))
        for (int32 Index = 0; Index < FrostMarks->GetNumMaterials(); ++Index) FrostMarks->SetMaterial(Index, Material);
    if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/PreferredCombat/Materials/M_Ember.M_Ember")))
        for (int32 Index = 0; Index < EmberMarks->GetNumMaterials(); ++Index) EmberMarks->SetMaterial(Index, Material);
    if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/PreferredCombat/Materials/M_Storm.M_Storm")))
        EffectMarks->SetMaterial(0, Material);
}

bool ADBEnemy::IsRoomActive() const
{
    if (!GetWorld()) return false;
    if (const ADBGameMode* Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode()))
        return !Mode->bPaused && !Mode->bChoosingReward && !Mode->bShowingBuild
            && !Mode->bTitle && !Mode->bWon && !Mode->bDefeated
            && (RoomId == INDEX_NONE || Mode->CurrentRoomId == RoomId);
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
    if (bVisualsBuilt && Kind == EDBEnemyKind::Caster && ChargePart) return ChargePart->GetComponentLocation();
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
        UpdateDeath(Dt);
        return;
    }
    if (!IsRoomActive())
    {
        GetCharacterMovement()->StopMovementImmediately();
        ConsumeMovementInputVector();
        WarningMarks->ClearInstances();
        ClearElementVisuals();
        ChargePart->SetVisibility(false);
        return;
    }
    if (!Target.IsValid()) Target = Cast<ADBCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!Target.IsValid() || Target->bDead || FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation()) > FMath::Square(3600.f))
    {
        GetCharacterMovement()->StopMovementImmediately();
        Phase = EDBEnemyPhase::Dormant;
        WarningMarks->ClearInstances();
        ClearElementVisuals();
        ChargePart->SetVisibility(false);
        return;
    }
    if (Phase == EDBEnemyPhase::Dormant) Phase = EDBEnemyPhase::Approach;
    UpdateStatusEffects(Dt);
    if (bDead) return;
    Cooldown = FMath::Max(0.f, Cooldown - Dt);
    InterceptionCooldown = FMath::Max(0.f, InterceptionCooldown - Dt);
    ElementSoundCooldown = FMath::Max(0.f, ElementSoundCooldown - Dt);
    KnockbackTime = FMath::Max(0.f, KnockbackTime - Dt);
    GetCharacterMovement()->GroundFriction = KnockbackTime > 0.f ? 2.5f : 9.f;
    GetCharacterMovement()->BrakingDecelerationWalking = KnockbackTime > 0.f ? 450.f : 2200.f;
    if (bPracticeTarget)
    {
        GetCharacterMovement()->StopMovementImmediately();
        ConsumeMovementInputVector();
        PhaseTime = FMath::Max(0.f, PhaseTime - Dt);
        if (PhaseTime <= 0.f) { Phase = EDBEnemyPhase::Dormant; bVulnerable = false; Telegraph.Empty(); }
        UpdateVisuals(Dt);
        return;
    }
    switch (Phase)
    {
    case EDBEnemyPhase::Approach: UpdateApproach(Dt); break;
    case EDBEnemyPhase::Telegraph: UpdateTell(Dt); break;
    case EDBEnemyPhase::Attack: UpdateAttack(Dt); break;
    case EDBEnemyPhase::Recovery:
    case EDBEnemyPhase::Staggered:
        PhaseTime -= Dt;
        // A struck body keeps its collision-resolved impulse during the first recovery beat.
        ConsumeMovementInputVector();
        if (KnockbackTime <= 0.f) GetCharacterMovement()->StopMovementImmediately();
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
    SteeringTime = FMath::Max(0.f, SteeringTime - DeltaSeconds);
    FVector Chosen = SteeringTime > 0.f && RouteClear(SteeringDirection) ? SteeringDirection : Direction;
    if (!RouteClear(Chosen))
    {
        bool bFound = false;
        for (float Angle : { 38.f, -38.f, 72.f, -72.f, 108.f, -108.f, 145.f, -145.f, 180.f })
        {
            const FVector Candidate = Direction.RotateAngleAxis(Angle * AvoidanceSide, FVector::UpVector);
            if (RouteClear(Candidate))
            {
                Chosen = Candidate; SteeringDirection = Chosen; SteeringTime = 0.32f;
                bFound = true; break;
            }
        }
        if (!bFound)
        {
            StuckTime += DeltaSeconds;
            if (StuckTime > 0.45f) { AvoidanceSide *= -1.f; StuckTime = 0.f; }
            // Let CharacterMovement depenetrate a close contact instead of disabling motion forever.
            const FVector Escape = (KeepInsideArena(Start - Direction * 100.f) - Start).GetSafeNormal2D();
            AddMovementInput(Escape, 0.45f, true);
            return;
        }
    }
    if (GetVelocity().SizeSquared2D() < FMath::Square(25.f)) StuckTime += DeltaSeconds;
    else StuckTime = 0.f;
    if (StuckTime > 0.8f) { AvoidanceSide *= -1.f; StuckTime = 0.f; }
    GetCharacterMovement()->MaxWalkSpeed = BaseSpeed * SpeedMultiplier * (1.f - ChillStacks * 0.16f);
    AddMovementInput(Chosen, 1.f, true);
    const FVector Facing = Target.IsValid() && (Kind == EDBEnemyKind::Melee || bRepositioning)
        && FVector::DistSquared2D(Start, Target->GetActorLocation()) < FMath::Square(1200.f)
        ? Target->GetActorLocation() - Start : Chosen;
    const FRotator Face(0.f, Facing.Rotation().Yaw, 0.f);
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), Face, DeltaSeconds, 7.f));
}

void ADBEnemy::ChooseRepositionTarget()
{
    if (!Target.IsValid()) return;
    const FVector PlayerPosition = Target->GetActorLocation();
    const FVector Radial = (GetActorLocation() - PlayerPosition).GetSafeNormal2D();
    float BestScore = TNumericLimits<float>::Max();
    FVector Best = KeepInsideArena(GetActorLocation() + FVector::CrossProduct(Radial, FVector::UpVector) * 380.f * AvoidanceSide);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DBCasterPosition), false, this);
    Params.AddIgnoredActor(Target.Get());
    for (float Angle : { 38.f, -38.f, 66.f, -66.f, 96.f, -96.f })
    {
        FVector Candidate = KeepInsideArena(PlayerPosition + Radial.RotateAngleAxis(Angle * AvoidanceSide, FVector::UpVector) * 850.f);
        Candidate.Z = GetActorLocation().Z;
        const float Travel = FVector::Dist2D(Candidate, GetActorLocation());
        if (Travel < 180.f) continue;
        if (GetWorld()->OverlapBlockingTestByChannel(Candidate, FQuat::Identity, ECC_Visibility,
            FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius(),
                GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 12.f), Params)) continue;
        FHitResult CoverHit, RouteHit;
        const bool bCovered = GetWorld()->LineTraceSingleByChannel(CoverHit, Candidate + FVector(0.f,0.f,28.f),
            PlayerPosition, ECC_Visibility, Params);
        const bool bBlockedRoute = GetWorld()->SweepSingleByChannel(RouteHit, GetActorLocation() + FVector(0.f,0.f,8.f),
            Candidate + FVector(0.f,0.f,8.f), FQuat::Identity, ECC_Visibility,
            FCollisionShape::MakeSphere(GetCapsuleComponent()->GetScaledCapsuleRadius()), Params);
        const float Score = FMath::Abs(Travel - 420.f) + (bCovered ? 550.f : 0.f) + (bBlockedRoute ? 300.f : 0.f);
        if (Score < BestScore) { BestScore = Score; Best = Candidate; }
    }
    RepositionTarget = Best;
    RepositionTime = 0.f;
    bRepositioning = true;
}

void ADBEnemy::UpdateApproach(float DeltaSeconds)
{
    if (!Target.IsValid()) return;
    const FVector ToPlayer = Target->GetActorLocation() - GetActorLocation();
    const float Distance = ToPlayer.Size2D();
    const bool bSight = HasSightTo(Target->GetActorLocation(), Target.Get());
    if (Kind == EDBEnemyKind::Caster)
    {
        if (bNeedsReposition)
        {
            if (!bRepositioning) ChooseRepositionTarget();
            RepositionTime += DeltaSeconds;
            if (FVector::DistSquared2D(GetActorLocation(), RepositionTarget) > FMath::Square(85.f) && RepositionTime < 2.4f)
            { MoveToward(RepositionTarget, DeltaSeconds); return; }
            bNeedsReposition = bRepositioning = false;
        }
        if (Cooldown <= 0.f && Distance >= 380.f && Distance < 1250.f && bSight)
        { BeginTell(EAttack::Bolt, 1.18f); return; }
        if (Distance < 500.f)
        {
            bNeedsReposition = true;
            ChooseRepositionTarget();
            MoveToward(RepositionTarget, DeltaSeconds);
        }
        else if (Distance > 1000.f || !bSight) MoveToward(Target->GetActorLocation(), DeltaSeconds);
        else MoveDirection(FVector::CrossProduct(ToPlayer.GetSafeNormal2D(), FVector::UpVector) * AvoidanceSide, DeltaSeconds, 0.55f);
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
            if (InterceptionCooldown <= 0.f && Distance > 300.f && Distance < 1700.f)
                BeginTell(EAttack::Intercept, 1.f);
            else if (Distance < 420.f) BeginTell(EAttack::Slam, 1.1f);
            else if (BossAttackIndex % 3 == 0) BeginTell(EAttack::Salvo, 1.35f);
            else BeginTell(EAttack::Ground, 1.5f);
            BossAttackIndex++;
            return;
        }
        MoveToward(Target->GetActorLocation(), DeltaSeconds, 0.85f);
    }
    else
    {
        if (Distance < 360.f && bSight)
        {
            MeleeSetupTime += DeltaSeconds;
            if (Cooldown <= 0.f && MeleeSetupTime >= 0.32f && Distance < 225.f)
            { BeginTell(EAttack::Swing, 0.88f); return; }
            const FVector Toward = ToPlayer.GetSafeNormal2D();
            const FVector Circle = FVector::CrossProduct(Toward, FVector::UpVector) * AvoidanceSide;
            MoveDirection(Toward * FMath::Clamp((Distance - 180.f) / 85.f, -0.65f, 1.f) + Circle * 0.65f,
                DeltaSeconds, 0.7f);
        }
        else { MeleeSetupTime = 0.f; MoveToward(Target->GetActorLocation(), DeltaSeconds); }
    }
}

bool ADBEnemy::TryInterceptShieldPiece(FVector IncomingDirection)
{
    if (bDead || bPracticeTarget || Kind != EDBEnemyKind::Boss || !IsRoomActive()
        || Phase != EDBEnemyPhase::Attack || Attack != EAttack::Intercept || !bShieldInterceptionReady
        || IncomingDirection.ContainsNaN()) return false;
    const FVector Incoming = IncomingDirection.GetSafeNormal();
    if (Incoming.IsNearlyZero() || FVector::DotProduct(-Incoming, GetActorForwardVector()) < 0.45f) return false;
    // Consume before returning to the flight callback: simultaneous later pieces pass normally.
    bShieldInterceptionReady = false;
    ADBCombatEffect::SpawnBurst(GetWorld(), GetActorLocation() + GetActorForwardVector()
        * (GetCapsuleComponent()->GetScaledCapsuleRadius() + 25.f), EDBElement::Neutral, RoomId, 0.85f);
    if (BodyImpactSound) UGameplayStatics::PlaySoundAtLocation(this, BodyImpactSound, GetActorLocation(), 0.85f, 0.6f);
    BeginRecovery(1.8f);
    Telegraph = TEXT("INTERCEPT SPENT - EXPOSED");
    if (ADBGameMode* Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode()))
        Mode->NotifyEvent(TEXT("ONE PIECE BROKEN - SENTINEL EXPOSED"), InterceptColor);
    return true;
}

void ADBEnemy::BeginTell(EAttack InAttack, float Duration)
{
    Attack = InAttack;
    Phase = EDBEnemyPhase::Telegraph;
    TellTime = TellDuration = Duration;
    bVulnerable = false;
    bShieldInterceptionReady = false;
    MeleeSetupTime = 0.f;
    bRepositioning = false;
    GetCharacterMovement()->StopMovementImmediately();
    LockedDirection = Target.IsValid() ? (Target->GetActorLocation() - ShotOrigin()).GetSafeNormal() : GetActorForwardVector();
    bAimLocked = InAttack == EAttack::Ground || InAttack == EAttack::Slam;
    TellTarget = Target.IsValid() ? Target->GetActorLocation() : GetActorLocation() + GetActorForwardVector() * 300.f;
    TellRadius = 0.f;
    switch (Attack)
    {
    case EAttack::Swing: Telegraph = TEXT("HEAVY SWING - PARRY / STEP BACK"); TellRadius = 190.f; break;
    case EAttack::Bolt: Telegraph = TEXT("CHARGING VOLLEY - MOVE / RETURN SHIELD"); break;
    case EAttack::Lunge: Telegraph = TEXT("LUNGE - SIDESTEP / DEFLECT"); break;
    case EAttack::Salvo: Telegraph = TEXT("AIMED SALVO - LEAVE THE LANES"); break;
    case EAttack::Intercept:
        Telegraph = TEXT("INTERCEPT WINDUP - WAIT / FLANK / STRIKE CLOSE");
        InterceptionCooldown = 8.f;
        TellRadius = 125.f;
        break;
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
            Mode->NotifyEvent(Telegraph, Attack == EAttack::Intercept ? InterceptColor
                : Attack == EAttack::Slam || Attack == EAttack::Ground ? GroundColor : DangerColor);
    }
    else if (EnemyTellSound) UGameplayStatics::PlaySoundAtLocation(this, EnemyTellSound, GetActorLocation(),
        Kind == EDBEnemyKind::Caster ? 0.8f : 0.65f, Kind == EDBEnemyKind::Caster ? 1.08f : 0.82f);
}

void ADBEnemy::UpdateTell(float DeltaSeconds)
{
    if (!Target.IsValid()) { BeginRecovery(0.4f); return; }
    GetCharacterMovement()->StopMovementImmediately();
    if (!bAimLocked)
    {
        LockedDirection = (Target->GetActorLocation() - ShotOrigin()).GetSafeNormal();
        TellTarget = Target->GetActorLocation();
        if (TellTime <= (Attack == EAttack::Swing ? 0.37f : TellDuration * 0.5f)) bAimLocked = true;
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
    ShotsRemaining = Attack == EAttack::Salvo ? 3 : Attack == EAttack::Bolt ? 2 : 1;
    bHitAttempted = false;
    EnemyAttackSequence = (EnemyAttackSequence + 1u) & 0x7fffffffu;
    ActiveAttackId = 0x80000000u | EnemyAttackSequence;
    if (Attack == EAttack::Intercept)
    {
        bShieldInterceptionReady = true;
        Telegraph = TEXT("INTERCEPT READY - BREAKS ONE FRONTAL PIECE");
    }
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
    AttackKick = 1.f;
    if (EnemyFireSound) UGameplayStatics::PlaySoundAtLocation(this, EnemyFireSound, ShotOrigin(), 0.78f,
        Kind == EDBEnemyKind::Boss ? 0.82f : 1.f);
    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (ADBProjectile* Bolt = GetWorld()->SpawnActor<ADBProjectile>(ShotOrigin(), Direction.Rotation(), Params))
        Bolt->Initialize(Direction, Kind == EDBEnemyKind::Boss ? 900.f : 690.f, Damage, false, this, Color);
}

bool ADBEnemy::TryMeleeHit(float Range, float ConeCosine, float Damage, bool bUnblockable)
{
    if (!Target.IsValid() || Target->bDead) return false;
    const FVector Difference = Target->GetActorLocation() - GetActorLocation();
    if (Difference.Size2D() > Range || FMath::Abs(Difference.Z) > 165.f) return false;
    if (FVector::DotProduct(Difference.GetSafeNormal2D(), LockedDirection.GetSafeNormal2D()) < ConeCosine) return false;
    if (!HasSightTo(Target->GetActorLocation(), Target.Get())) return false;
    Target->ReceiveAttack(Damage, GetActorLocation(), bUnblockable, this, ActiveAttackId);
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
        Target->ReceiveAttack(AttackDamage * (Attack == EAttack::Slam ? 1.6f : 1.25f), TellTarget, true, this, ActiveAttackId);
}

void ADBEnemy::UpdateAttack(float DeltaSeconds)
{
    AttackElapsed += DeltaSeconds;
    if (Attack == EAttack::Intercept)
    {
        GetCharacterMovement()->StopMovementImmediately();
        if (AttackElapsed >= 0.75f) BeginRecovery(1.8f);
    }
    else if (Attack == EAttack::Bolt || Attack == EAttack::Salvo)
    {
        while (ShotsRemaining > 0 && AttackElapsed >= NextShotTime)
        {
            const float Spread = Attack == EAttack::Salvo ? (2 - ShotsRemaining) * 7.f : (ShotsRemaining == 2 ? -3.f : 3.f);
            FireBolt(LockedDirection.RotateAngleAxis(Spread, FVector::UpVector), AttackDamage, DangerColor);
            ShotsRemaining--;
            NextShotTime += Attack == EAttack::Salvo ? 0.2f : 0.27f;
        }
        if (AttackElapsed >= (Attack == EAttack::Salvo ? 0.65f : 0.64f))
            BeginRecovery(Kind == EDBEnemyKind::Boss ? 1.7f : 1.15f);
    }
    else if (Attack == EAttack::Lunge)
    {
        // Commitment follows the visible locked lane; CharacterMovement stops on solid cover.
        const FVector Next = GetActorLocation() + LockedDirection * 100.f;
        if (FVector::DistSquared2D(Next, KeepInsideArena(Next)) < 1.f)
            AddMovementInput(LockedDirection, 1.f, true);
        if (!bHitAttempted && TryMeleeHit(150.f, 0.15f, AttackDamage)) bHitAttempted = true;
        if (Phase != EDBEnemyPhase::Attack || bDead) return;
        if (AttackElapsed >= 0.43f || (AttackElapsed > 0.15f && GetVelocity().Size2D() < 30.f)) BeginRecovery(1.05f);
    }
    else if (Attack == EAttack::Swing)
    {
        if (AttackElapsed < 0.19f)
        {
            const FVector Step = LockedDirection.GetSafeNormal2D();
            const FVector Next = GetActorLocation() + Step * 70.f;
            if (FVector::DistSquared2D(Next, KeepInsideArena(Next)) < 1.f)
            {
                GetCharacterMovement()->MaxWalkSpeed = 360.f * (1.f - ChillStacks * 0.16f);
                AddMovementInput(Step, 1.f, true);
            }
        }
        else GetCharacterMovement()->StopMovementImmediately();
        if (!bHitAttempted && AttackElapsed >= 0.14f && AttackElapsed <= 0.28f)
            bHitAttempted = TryMeleeHit(190.f, 0.4f, AttackDamage);
        if (Phase != EDBEnemyPhase::Attack || bDead) return;
        if (AttackElapsed > 0.55f) BeginRecovery(1.2f);
    }
    else
    {
        if (!bHitAttempted && AttackElapsed >= 0.16f) { DetonateGround(); bHitAttempted = true; }
        if (AttackElapsed > 0.44f) BeginRecovery(1.85f);
    }
}

void ADBEnemy::BeginRecovery(float Duration)
{
    Phase = EDBEnemyPhase::Recovery;
    PhaseTime = Duration;
    RecoveryDuration = Duration;
    bVulnerable = true;
    bShieldInterceptionReady = false;
    Telegraph = TEXT("EXPOSED - COUNTERATTACK");
    TellTime = 0.f;
    GetCharacterMovement()->MaxAcceleration = 2200.f;
    GetCharacterMovement()->StopMovementImmediately();
    Cooldown = 0.35f;
    MeleeSetupTime = 0.f;
    if (Kind == EDBEnemyKind::Caster) { bNeedsReposition = true; bRepositioning = false; }
}

void ADBEnemy::Stagger(float Duration)
{
    if (bDead) return;
    // The guardian retains committed tells but still takes damage and every elemental status.
    if (Kind == EDBEnemyKind::Boss && (Phase == EDBEnemyPhase::Telegraph || Phase == EDBEnemyPhase::Attack)) return;
    Phase = EDBEnemyPhase::Staggered;
    bShieldInterceptionReady = false;
    PhaseTime = FMath::Min(1.2f, FMath::Max(PhaseTime, Duration));
    RecoveryDuration = PhaseTime;
    bVulnerable = true;
    Telegraph = TEXT("STAGGERED - COUNTERATTACK");
    TellTime = 0.f;
    ShotsRemaining = 0;
    bAimLocked = false;
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->MaxAcceleration = 2200.f;
    if (Kind == EDBEnemyKind::Caster) { bNeedsReposition = true; bRepositioning = false; }
}

void ADBEnemy::ApplyHitReaction(const FDBHit& Hit)
{
    FVector Away = Hit.Direction;
    if (Away.ContainsNaN() || Away.IsNearlyZero()) Away = GetActorLocation() - Hit.Source;
    Away = Away.GetSafeNormal2D();
    if (Away.IsNearlyZero()) Away = -GetActorForwardVector();
    LastHitDirection = Away;
    ReactionLocalDirection = GetActorRotation().UnrotateVector(Away);
    const bool bFrostPiece = Hit.Element == EDBElement::Frost && !Hit.bSecondary;
    const bool bPhysicalContact = Hit.bImpact || bFrostPiece;
    ReactionDuration = bPhysicalContact ? 0.48f : 0.26f;
    ReactionTime = ReactionDuration;
    ReactionStrength = bPhysicalContact ? FMath::Clamp(0.75f + Hit.Damage / 160.f, 0.8f, 1.45f) : 0.38f;
    if (Kind == EDBEnemyKind::Boss) ReactionStrength *= 0.65f;
    if (bPhysicalContact)
    {
        // A physical echo/parry may move a body even though it cannot recursively proc statuses.
        Stagger(Kind == EDBEnemyKind::Boss ? 0.32f : !Hit.bImpact ? 0.32f : Hit.bSecondary ? 0.62f : 0.72f);
        if (bPracticeTarget) return;
        KnockbackTime = Kind == EDBEnemyKind::Boss ? 0.16f : 0.24f;
        GetCharacterMovement()->GroundFriction = 2.5f;
        GetCharacterMovement()->BrakingDecelerationWalking = 450.f;
        GetCharacterMovement()->AddImpulse(Away * (Kind == EDBEnemyKind::Boss ? 175.f
            : !Hit.bImpact ? 320.f : Hit.bSecondary ? 380.f : 560.f), true);
    }
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
            BurnPulse = 0.28f;
            DealHealthDamage(BurnTickDamage);
        }
    }
}

void ADBEnemy::ApplyCombatHit(const FDBHit& Hit)
{
    if (bDead || !IsRoomActive() || !FMath::IsFinite(Hit.Damage) || Hit.Damage <= 0.f) return;
    const int32 PreviousChill = ChillStacks;
    const int32 PreviousStorm = StormMarks;
    const bool bWasBurning = BurnRemaining > 0.f;
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
        UGameplayStatics::PlaySoundAtLocation(this, EnemyHitSound, GetActorLocation(), Hit.bImpact ? 0.92f : 0.55f,
            Hit.bImpact ? 0.82f : 1.f);
        HitSoundCooldown = 0.085f;
    }
    const float Damage = Hit.Damage * (bVulnerable ? 1.35f : 1.f) + ShatterBonus;
    // Store the impact direction before a lethal hit creates its collapse pose.
    LastHitDirection = Hit.Direction.GetSafeNormal2D();
    if (LastHitDirection.IsNearlyZero()) LastHitDirection = (GetActorLocation() - Hit.Source).GetSafeNormal2D();
    if (LastHitDirection.IsNearlyZero()) LastHitDirection = -GetActorForwardVector();
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
            BurnPulse = 0.3f;
            BurnTickDamage = FMath::Max(BurnTickDamage, FMath::Clamp(Hit.Damage * 0.22f, 3.f, 14.f));
            BurnInstigator = Hit.InstigatorActor;
            if (!bWasBurning) ADBCombatEffect::SpawnBurst(GetWorld(), GetActorLocation(), EDBElement::Ember, RoomId, 0.6f);
        }
        else if (Hit.Element == EDBElement::Storm)
        {
            StormMarks = FMath::Min(3, StormMarks + 1);
            StormRemaining = 5.f;
        }
        // Apply the physical impulse after status stagger, which clears prior velocity.
        ApplyHitReaction(Hit);
    }
    if (bFracture)
        ChainToNearby(Hit.Damage * 0.55f + ConsumedChill * 6.f + ConsumedStorm * 8.f, 3, 720.f, Hit.InstigatorActor);
    else if (ConsumedStorm > 0)
        ChainToNearby(Hit.Damage * 0.42f + ConsumedStorm * 6.f, 2, 620.f, Hit.InstigatorActor);
    if (ShatterBonus > 0.f)
    {
        ADBCombatEffect::SpawnBurst(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 25.f),
            EDBElement::Frost, RoomId, 0.8f + ConsumedChill * 0.15f);
        if (BodyImpactSound && ElementSoundCooldown <= 0.f)
        {
            UGameplayStatics::PlaySoundAtLocation(this, BodyImpactSound, GetActorLocation(), 0.8f, 1.45f);
            ElementSoundCooldown = 0.16f;
        }
        if (ADBGameMode* Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode())) Mode->NotifyEvent(TEXT("SHATTER - CHILL CONSUMED"), FrostColor);
    }
}

void ADBEnemy::ChainToNearby(float Damage, int32 MaxTargets, float Radius, AActor* HitInstigator)
{
    TArray<TPair<float, ADBEnemy*>> Candidates;
    const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 20.f);
    ADBCombatEffect::SpawnBurst(GetWorld(), Start, EDBElement::Storm, RoomId, 0.55f);
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
        ADBCombatEffect::SpawnArc(GetWorld(), Start, End, RoomId);
        Other->ApplyCombatHit(Secondary);
    }
}

void ADBEnemy::DealHealthDamage(float Damage)
{
    if (bDead || Damage <= 0.f) return;
    Health = FMath::Max(0.f, Health - Damage);
    if (Health <= 0.f)
    {
        if (bPracticeTarget)
        {
            Health = MaxHealth;
            HitFlash = 0.3f;
            const EDBElement Element = BurnRemaining > 0.f ? EDBElement::Ember
                : ChillStacks > 0 ? EDBElement::Frost : StormMarks > 0 ? EDBElement::Storm : EDBElement::Neutral;
            ADBCombatEffect::SpawnBurst(GetWorld(), GetActorLocation(), Element, RoomId, 0.8f);
        }
        else Die();
    }
}

void ADBEnemy::Die()
{
    if (bDead) return;
    bDead = true;
    bShieldInterceptionReady = false;
    Phase = EDBEnemyPhase::Dead;
    bVulnerable = false;
    DeathTime = 0.f;
    bDeathLanded = false;
    DeathStartPose.Reset();
    for (USceneComponent* Joint : { BodyPivot.Get(), HeadPivot.Get(), LeftArmPivot.Get(),
        RightArmPivot.Get(), LeftLegPivot.Get(), RightLegPivot.Get() }) DeathStartPose.Add(Joint->GetRelativeTransform());
    DeathLocalDirection = GetActorRotation().UnrotateVector(LastHitDirection);
    if (DeathLocalDirection.IsNearlyZero()) DeathLocalDirection = -FVector::ForwardVector;
    if (EnemyDefeatSound) UGameplayStatics::PlaySoundAtLocation(this, EnemyDefeatSound, GetActorLocation(), 0.95f,
        Kind == EDBEnemyKind::Boss ? 0.7f : 0.92f);
    ChargePart->SetVisibility(false);
    Telegraph.Empty();
    WarningMarks->ClearInstances();
    ClearElementVisuals();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetCharacterMovement()->DisableMovement();
    if (!bDeathNotified)
    {
        bDeathNotified = true;
        if (ADBGameMode* Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode())) Mode->NotifyEnemyKilled(this);
    }
}

void ADBEnemy::UpdateDeath(float DeltaSeconds)
{
    DeathTime += DeltaSeconds;
    const float Kneel = FMath::SmoothStep(0.f, 0.48f, DeathTime);
    const float Fall = FMath::SmoothStep(0.25f, 0.92f, DeathTime);
    const float Settle = FMath::Exp(-FMath::Max(0.f, DeathTime - 0.65f) * 8.f)
        * FMath::Sin(FMath::Max(0.f, DeathTime - 0.65f) * 20.f);
    const float Side = DeathLocalDirection.Y >= 0.f ? 1.f : -1.f;
    const float Back = DeathLocalDirection.X >= 0.f ? -1.f : 1.f;
    const float Pelvis = FMath::Lerp(94.f, 38.f, Kneel);
    BodyPivot->SetRelativeLocation(FVector(Fall * -Back * 12.f, Side * Fall * 10.f, Pelvis));
    BodyPivot->SetRelativeRotation(FRotator(Back * (15.f * Kneel + 66.f * Fall + Settle * 3.f),
        Side * Fall * 14.f, Side * Fall * 16.f));
    HeadPivot->SetRelativeRotation(FRotator(-Back * (Kneel * 18.f - Fall * 8.f), Side * 9.f * Fall, Side * 14.f * Fall));
    LeftArmPivot->SetRelativeRotation(FRotator(Back * (65.f * Kneel - Fall * 24.f), -18.f * Fall, -20.f - 28.f * Fall));
    RightArmPivot->SetRelativeRotation(FRotator(Back * (40.f * Kneel + Fall * 14.f), 24.f * Fall, 15.f + 34.f * Fall));
    LeftLegPivot->SetRelativeLocation(FVector(0.f, -18.f, Pelvis));
    RightLegPivot->SetRelativeLocation(FVector(8.f * Fall, 18.f, Pelvis));
    LeftLegPivot->SetRelativeRotation(FRotator(Back * 69.f * Kneel, -10.f * Fall, -5.f * Fall));
    RightLegPivot->SetRelativeRotation(FRotator(Back * 73.f * Kneel, 14.f * Fall, 9.f * Fall));
    if (DeathTime < 0.18f && DeathStartPose.Num() == 6)
    {
        const float Blend = FMath::SmoothStep(0.f, 0.18f, DeathTime);
        int32 Index = 0;
        for (USceneComponent* Joint : { BodyPivot.Get(), HeadPivot.Get(), LeftArmPivot.Get(),
            RightArmPivot.Get(), LeftLegPivot.Get(), RightLegPivot.Get() })
        {
            const FTransform StartPose = DeathStartPose[Index++];
            Joint->SetRelativeLocation(FMath::Lerp(StartPose.GetLocation(), Joint->GetRelativeLocation(), Blend));
            Joint->SetRelativeRotation(FQuat::Slerp(StartPose.GetRotation(), Joint->GetRelativeRotation().Quaternion(), Blend));
        }
    }
    WarningMarks->ClearInstances();
    EffectMarks->ClearInstances();
    if (CoreMaterial) CoreMaterial->SetVectorParameterValue(TEXT("Color"), DangerColor * FMath::Max(0.015f, 1.f - DeathTime * 1.5f));
    if (!bDeathLanded && DeathTime >= 0.64f)
    {
        bDeathLanded = true;
        if (BodyImpactSound) UGameplayStatics::PlaySoundAtLocation(this, BodyImpactSound, FeetLocation(), 0.58f, 0.62f);
    }
    // Keep the fallen body visible through the immediate follow-up/catch and reward beat.
    const bool bFar = !Target.IsValid() || FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation()) > FMath::Square(950.f);
    if (DeathTime > 18.f || (DeathTime > 9.f && bFar)) Destroy();
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
    if (Phase != EDBEnemyPhase::Telegraph && !bShieldInterceptionReady && GroundPulseTime <= 0.f) return;
    const float Progress = TellDuration > 0.f ? 1.f - TellTime / TellDuration : 1.f;
    if (Attack == EAttack::Intercept && (Phase == EDBEnemyPhase::Telegraph || bShieldInterceptionReady))
    {
        if (WarningMaterial)
        {
            WarningMaterial->SetVectorParameterValue(TEXT("Color"), InterceptColor * (bShieldInterceptionReady ? 1.8f : 0.8f));
            WarningMaterial->SetVectorParameterValue(TEXT("Tint"), InterceptColor);
        }
        const FVector Center = GetActorLocation() + GetActorForwardVector()
            * (GetCapsuleComponent()->GetScaledCapsuleRadius() + 22.f) + FVector(0.f, 0.f, 25.f);
        const FVector Side = GetActorRightVector() * (52.f * VisualScale);
        const FVector Up(0.f, 0.f, 49.f * VisualScale);
        AddLine(WarningMarks, Center - Side, Center + Up, 5.f);
        AddLine(WarningMarks, Center + Up, Center + Side, 5.f);
        AddLine(WarningMarks, Center + Side, Center - Up, 5.f);
        AddLine(WarningMarks, Center - Up, Center - Side, 5.f);
        const float Cross = bShieldInterceptionReady ? 0.75f : 0.2f + Progress * 0.5f;
        AddLine(WarningMarks, Center - (Side + Up) * Cross, Center + (Side + Up) * Cross, bShieldInterceptionReady ? 8.f : 4.f);
        AddLine(WarningMarks, Center - (Side - Up) * Cross, Center + (Side - Up) * Cross, bShieldInterceptionReady ? 8.f : 4.f);
        const FVector Floor = FeetLocation() + FVector(0.f, 0.f, 6.f);
        for (int32 Index = -4; Index < 4; ++Index)
        {
            const FVector A = GetActorForwardVector().RotateAngleAxis(Index * 15.f, FVector::UpVector);
            const FVector B = GetActorForwardVector().RotateAngleAxis((Index + 0.8f) * 15.f, FVector::UpVector);
            AddLine(WarningMarks, Floor + A * 185.f, Floor + B * 185.f, 6.f);
        }
        return;
    }
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
            AddLine(WarningMarks, Center + A * TellRadius, Center + B * TellRadius, 5.f);
        }
    }
    else
    {
        const FVector Start = ShotOrigin();
        const float Range = Attack == EAttack::Lunge ? 490.f : 1600.f;
        const int32 Lanes = Attack == EAttack::Salvo ? 3 : Attack == EAttack::Bolt ? 2 : 1;
        for (int32 Lane = 0; Lane < Lanes; ++Lane)
        {
            const float Spread = Lanes == 3 ? (Lane - 1) * 7.f : Lanes == 2 ? (Lane == 0 ? -3.f : 3.f) : 0.f;
            FVector Direction = LockedDirection.RotateAngleAxis(Spread, FVector::UpVector);
            FVector End = Start + Direction * Range;
            FHitResult Hit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(DBWarningLane), false, this);
            if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params)) End = Hit.ImpactPoint;
            if (Attack == EAttack::Bolt)
            {
                // The held charge is the first tell; the locked volley shows short separated lanes.
                if (bAimLocked)
                    for (float Distance = 85.f; Distance < FVector::Dist(Start, End); Distance += 180.f)
                        AddLine(WarningMarks, Start + Direction * Distance,
                            Start + Direction * FMath::Min(Distance + 48.f, FVector::Dist(Start, End)), 2.5f);
            }
            else AddLine(WarningMarks, Start, End, bAimLocked ? 3.8f : 2.f);
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
    AttackKick = FMath::Max(0.f, AttackKick - DeltaSeconds * 5.5f);
    ReactionTime = FMath::Max(0.f, ReactionTime - DeltaSeconds);
    const bool bCaster = Kind == EDBEnemyKind::Caster;
    const bool bHunter = Kind == EDBEnemyKind::Hunter;
    const float Speed = GetVelocity().Size2D();
    const bool bWalking = Phase == EDBEnemyPhase::Approach || (Phase == EDBEnemyPhase::Attack && Attack == EAttack::Lunge);
    const float Walking = bWalking ? FMath::Clamp(Speed / FMath::Max(1.f, BaseSpeed), 0.f, 1.f) : 0.f;
    GaitBlend = FMath::FInterpTo(GaitBlend, Walking, DeltaSeconds, 12.f);
    // Travel advances the gait; a blocked or stationary body never marches in place.
    if (Speed > 4.f && bWalking) GaitPhase += Speed * DeltaSeconds * (2.f * PI / (bHunter ? 185.f : 170.f));
    const float Stride = FMath::Sin(GaitPhase) * GaitBlend;
    const float LegPitch = Stride * (bCaster ? 21.f : 27.f);
    const float Pelvis = 94.f * FMath::Cos(FMath::DegreesToRadians(LegPitch));
    const float Lift = FMath::Cos(GaitPhase) * 6.f * GaitBlend;
    const FVector LocalVelocity = GetActorRotation().UnrotateVector(GetVelocity()) / FMath::Max(1.f, BaseSpeed);
    const float TellProgress = Phase == EDBEnemyPhase::Telegraph && TellDuration > 0.f
        ? FMath::Clamp(1.f - TellTime / TellDuration, 0.f, 1.f) : 0.f;
    const float Windup = FMath::SmoothStep(0.f, 1.f, TellProgress);
    const float Recover = (Phase == EDBEnemyPhase::Recovery || Phase == EDBEnemyPhase::Staggered)
        ? FMath::Clamp(PhaseTime / FMath::Max(0.01f, RecoveryDuration), 0.f, 1.f) : 0.f;
    float LeftArmPitch = bCaster ? 38.f - Stride * 9.f : -Stride * 23.f;
    float RightArmPitch = bCaster ? 48.f + Stride * 9.f : 12.f + Stride * 23.f;
    float LeftArmRoll = bCaster ? 14.f : -10.f;
    float RightArmRoll = bCaster ? -14.f : 13.f;
    float BodyPitch = (bHunter ? -13.f : -3.f) - FMath::Clamp(LocalVelocity.X, -1.f, 1.f) * GaitBlend * 5.f;
    float BodyYaw = Stride * (bCaster ? 2.f : 5.f);
    float BodyRoll = FMath::Clamp(LocalVelocity.Y, -1.f, 1.f) * GaitBlend * 6.f;
    float BodyDrop = 0.f;
    float HeadPitch = -BodyPitch * 0.65f;
    if (Phase == EDBEnemyPhase::Telegraph)
    {
        if (Attack == EAttack::Intercept)
        {
            LeftArmPitch = 50.f + Windup * 40.f;
            RightArmPitch = 60.f + Windup * 33.f;
            LeftArmRoll = 28.f; RightArmRoll = -28.f;
            BodyPitch = -5.f; BodyYaw = 0.f; BodyDrop = Windup * 5.f;
            HeadPitch = 5.f;
        }
        else if (Attack == EAttack::Swing)
        {
            RightArmPitch = FMath::Lerp(-40.f, -155.f, Windup);
            RightArmRoll = FMath::Lerp(16.f, 30.f, Windup);
            LeftArmPitch = 48.f + Windup * 12.f;
            BodyYaw = -32.f * Windup;
            BodyPitch = 6.f * Windup;
            BodyDrop = 5.f * Windup;
            HeadPitch = -8.f;
        }
        else if (Attack == EAttack::Ground || Attack == EAttack::Slam)
        {
            LeftArmPitch = RightArmPitch = -75.f - Windup * 88.f;
            LeftArmRoll = -20.f; RightArmRoll = 20.f;
            BodyPitch = 9.f * Windup;
            BodyDrop = 9.f * Windup;
            HeadPitch = -14.f;
        }
        else if (Attack == EAttack::Lunge)
        {
            LeftArmPitch = -42.f; RightArmPitch = 55.f;
            BodyPitch = -22.f - Windup * 10.f;
            BodyDrop = 10.f * Windup;
            HeadPitch = 18.f;
        }
        else
        {
            LeftArmPitch = FMath::Lerp(38.f, 70.f, Windup);
            RightArmPitch = FMath::Lerp(48.f, 82.f, Windup);
            LeftArmRoll = 19.f; RightArmRoll = -19.f;
            BodyPitch = -6.f; BodyDrop = 4.f * Windup;
            HeadPitch = 2.f;
        }
    }
    else if (Phase == EDBEnemyPhase::Attack)
    {
        if (Attack == EAttack::Intercept)
        {
            LeftArmPitch = 90.f; RightArmPitch = 93.f;
            LeftArmRoll = 28.f; RightArmRoll = -28.f;
            BodyPitch = -5.f; BodyYaw = 0.f; BodyDrop = 5.f; HeadPitch = 5.f;
        }
        else if (Attack == EAttack::Swing)
        {
            // The striking forearm crosses in front exactly as the active hit window opens.
            const float Strike = FMath::SmoothStep(0.f, 0.18f, AttackElapsed);
            RightArmPitch = FMath::Lerp(-155.f, 68.f, Strike);
            RightArmRoll = FMath::Lerp(30.f, -22.f, Strike);
            LeftArmPitch = 55.f - Strike * 35.f;
            BodyYaw = FMath::Lerp(-32.f, 29.f, Strike);
            BodyPitch = -14.f * Strike;
            BodyDrop = 5.f;
            HeadPitch = 8.f;
        }
        else if (Attack == EAttack::Slam || Attack == EAttack::Ground)
        {
            const float Strike = FMath::SmoothStep(0.f, 0.16f, AttackElapsed);
            LeftArmPitch = RightArmPitch = FMath::Lerp(-163.f, 32.f, Strike);
            BodyPitch = -22.f * Strike; BodyDrop = 12.f;
            HeadPitch = 12.f;
        }
        else if (Attack == EAttack::Bolt || Attack == EAttack::Salvo)
        {
            LeftArmPitch = 78.f - AttackKick * 16.f;
            RightArmPitch = 86.f - AttackKick * 20.f;
            LeftArmRoll = 19.f; RightArmRoll = -19.f;
            BodyPitch = -6.f + AttackKick * 13.f;
            HeadPitch = -AttackKick * 8.f;
        }
        else { BodyPitch = -30.f; LeftArmPitch = -25.f; RightArmPitch = 76.f; HeadPitch = 20.f; }
    }
    else if (Recover > 0.f)
    {
        LeftArmPitch = FMath::Lerp(LeftArmPitch, 12.f, Recover);
        RightArmPitch = FMath::Lerp(RightArmPitch, Attack == EAttack::Swing ? 55.f : 15.f, Recover);
        LeftArmRoll = -22.f * Recover;
        RightArmRoll = 27.f * Recover;
        BodyPitch = FMath::Lerp(BodyPitch, Attack == EAttack::Swing ? -13.f : 12.f, Recover);
        BodyYaw = Attack == EAttack::Swing ? 23.f * Recover : 0.f;
        HeadPitch = -9.f * Recover;
        BodyDrop = 6.f * Recover;
    }
    const float ReactionProgress = 1.f - ReactionTime / FMath::Max(0.01f, ReactionDuration);
    const float Recoil = ReactionTime > 0.f ? FMath::SmoothStep(0.f, 0.11f, ReactionProgress)
        * FMath::Square(1.f - ReactionProgress) * ReactionStrength : 0.f;
    BodyPitch -= ReactionLocalDirection.X * Recoil * 28.f;
    BodyRoll += ReactionLocalDirection.Y * Recoil * 24.f;
    HeadPitch += ReactionLocalDirection.X * Recoil * 17.f;
    LeftArmPitch -= Recoil * 23.f;
    RightArmPitch -= Recoil * 31.f;
    LeftArmRoll -= Recoil * 17.f;
    RightArmRoll += Recoil * 17.f;
    const FVector RecoilOffset = ReactionLocalDirection * Recoil * 10.f;
    BodyPivot->SetRelativeLocation(FVector(RecoilOffset.X, RecoilOffset.Y,
        Pelvis - BodyDrop + FMath::Sin(VisualTime * 2.3f) * 0.7f));
    BodyPivot->SetRelativeRotation(FRotator(BodyPitch, BodyYaw, BodyRoll));
    HeadPivot->SetRelativeRotation(FRotator(HeadPitch, -BodyYaw * 0.65f, -BodyRoll * 0.6f));
    LeftArmPivot->SetRelativeRotation(FRotator(LeftArmPitch, 0.f, LeftArmRoll));
    RightArmPivot->SetRelativeRotation(FRotator(RightArmPitch, 0.f, RightArmRoll));
    LeftLegPivot->SetRelativeLocation(FVector(0.f, -18.f, Pelvis + FMath::Max(0.f, Lift)));
    RightLegPivot->SetRelativeLocation(FVector(0.f, 18.f, Pelvis + FMath::Max(0.f, -Lift)));
    LeftLegPivot->SetRelativeRotation(FRotator(LegPitch, 0.f, -GaitBlend * 2.f));
    RightLegPivot->SetRelativeRotation(FRotator(-LegPitch, 0.f, GaitBlend * 2.f));
    const bool bCharging = (Attack == EAttack::Bolt || Attack == EAttack::Salvo)
        && (Phase == EDBEnemyPhase::Telegraph || (Phase == EDBEnemyPhase::Attack && ShotsRemaining > 0));
    ChargePart->SetVisibility(bCharging);
    if (bCharging)
    {
        const float Charge = Phase == EDBEnemyPhase::Telegraph ? 0.3f + Windup * 1.7f : 1.65f - AttackKick * 0.6f;
        ChargePart->SetRelativeScale3D(FVector(Charge * (1.f + FMath::Sin(VisualTime * 16.f) * 0.035f)));
        ChargePart->SetRelativeRotation(FRotator(0.f, 0.f, VisualTime * 75.f));
    }
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
    UpdateElementVisuals(DeltaSeconds);
}

void ADBEnemy::ClearElementVisuals()
{
    FrostMarks->ClearInstances();
    EmberMarks->ClearInstances();
    EffectMarks->ClearInstances();
}

void ADBEnemy::UpdateElementVisuals(float DeltaSeconds)
{
    ClearElementVisuals();
    BurnPulse = FMath::Max(0.f, BurnPulse - DeltaSeconds);
    const FTransform Frame = VisualRoot->GetComponentTransform();
    if (ChillStacks > 0)
    {
        // Static ice attaches to the silhouette; it remains blue during white hits and green recovery.
        const int32 Count = 6 + ChillStacks * 2;
        for (int32 Index = 0; Index < Count; ++Index)
        {
            const float Angle = Index * 2.f * PI / Count;
            const float Height = Index % 2 ? 42.f : 111.f;
            const FVector At = Frame.TransformPosition(FVector(FMath::Cos(Angle) * 36.f, FMath::Sin(Angle) * 40.f, Height));
            AddElementShard(FrostMarks, At, FRotator(12.f, FMath::RadiansToDegrees(Angle), Index % 2 ? 15.f : -15.f),
                FVector(10.f, 12.f, 23.f + ChillStacks * 5.f) * VisualScale);
        }
        const FVector Crown = Frame.TransformPosition(FVector(0.f, 0.f, 216.f));
        AddElementShard(FrostMarks, Crown, FRotator::ZeroRotator, FVector(11.f, 11.f, 29.f) * VisualScale);
        AddElementShard(FrostMarks, Crown, FRotator(90.f, GetActorRotation().Yaw, 0.f), FVector(7.f, 7.f, 30.f) * VisualScale);
    }
    if (BurnRemaining > 0.f)
    {
        for (int32 Index = 0; Index < 9; ++Index)
        {
            const float Rise = FMath::Fmod(VisualTime * 0.95f + Index * 0.137f, 1.f);
            const float Angle = Index * 2.f * PI / 9.f + FMath::Sin(VisualTime * 2.f + Index) * 0.12f;
            const FVector At = Frame.TransformPosition(FVector(FMath::Cos(Angle) * 32.f,
                FMath::Sin(Angle) * 36.f, 28.f + Rise * 133.f));
            const float Flame = FMath::Max(0.15f, 1.f - Rise) * (1.f + BurnPulse);
            AddElementShard(EmberMarks, At, FRotator(10.f * FMath::Sin(VisualTime * 7.f + Index), Index * 37.f, 0.f),
                FVector(13.f, 12.f, 43.f) * (VisualScale * Flame));
        }
    }
    for (int32 Mark = 0; Mark < StormMarks; ++Mark)
    {
        // A lightning zig-zag across the chest is distinct from ice and rising flame.
        const FVector Center(45.f, (Mark - (StormMarks - 1) * 0.5f) * 24.f, 128.f);
        const FVector A = Frame.TransformPosition(Center + FVector(0.f, -8.f, 17.f));
        const FVector B = Frame.TransformPosition(Center + FVector(0.f, 5.f, 2.f));
        const FVector C = Frame.TransformPosition(Center + FVector(0.f, -4.f, -2.f));
        const FVector D = Frame.TransformPosition(Center + FVector(0.f, 7.f, -17.f));
        AddLine(EffectMarks, A, B, 4.8f * VisualScale);
        AddLine(EffectMarks, B, C, 4.8f * VisualScale);
        AddLine(EffectMarks, C, D, 4.8f * VisualScale);
    }
}

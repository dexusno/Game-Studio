#include "DBEnemy.h"
#include "DBEnemyPerformance.h"
#include "DBOrganicFire.h"

#include "DBCharacter.h"
#include "DBCombatEffect.h"
#include "DBGameMode.h"
#include "DBProjectile.h"
#include "DBThrownShield.h"
#include "Components/CapsuleComponent.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
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
    // Original solid claw vertices 371712/35784, weight 1.0 in both the 16-
    // and 21-bone source FBXs. Offsets are reference actor-space centimetres.
    const FVector BriarhideClawOffsets[2] = {
        FVector(-17.5378f, -5.0008f, -23.7313f),
        FVector(-16.8672f, 4.4914f, -23.9927f)
    };
    uint32 EnemyAttackSequence = 0;

    // Exact critically damped response for a target held over this tick.
    // Arrival remains continuous across frame rates and a hitch cannot turn
    // a small carried-body reaction into an unstable Euler spring.
    void AdvanceOrganicResponse(FVector& Value, FVector& Velocity, const FVector& Target,
        float Frequency, float DeltaSeconds)
    {
        if (DeltaSeconds <= 0.f) return;
        const float Time = FMath::Min(DeltaSeconds, .25f);
        const float Omega = FMath::Max(.01f, Frequency);
        const FVector Error = Value - Target;
        const FVector Carry = Velocity + Error * Omega;
        const float Decay = FMath::Exp(-Omega * Time);
        Value = Target + (Error + Carry * Time) * Decay;
        Velocity = (Velocity - Carry * (Omega * Time)) * Decay;
    }

    void AddElementShard(UInstancedStaticMeshComponent* Component, FVector At, FRotator Rotation, FVector Size)
    {
        if (!Component->GetStaticMesh()) return;
        const FBoxSphereBounds Bounds = Component->GetStaticMesh()->GetBounds();
        const FVector MeshSize = Bounds.BoxExtent * 2.f;
        const FVector Scale(Size.X / FMath::Max(1.0, MeshSize.X), Size.Y / FMath::Max(1.0, MeshSize.Y),
            Size.Z / FMath::Max(1.0, MeshSize.Z));
        Component->AddInstance(FTransform(Rotation, At - Rotation.RotateVector(Bounds.Origin * Scale), Scale), true);
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
    OrganicMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("OrganicCreature"));
    OrganicMesh->SetupAttachment(VisualRoot);
    OrganicMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OrganicMesh->SetGenerateOverlapEvents(false);
    OrganicMesh->SetCanEverAffectNavigation(false);
    // The proven FBX import convention maps Blender -Y front to Unreal +Y.
    OrganicMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    OrganicMesh->SetBoundsScale(1.6f);
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
    OrganicFireCharge = Part(TEXT("OrganicFireCharge"), GetCapsuleComponent());
    OrganicFireCharge->SetCastShadow(false);
    OrganicFireCharge->SetVisibility(false);
    OrganicFireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("InternalFurnaceLight"));
    OrganicFireLight->SetupAttachment(GetCapsuleComponent());
    OrganicFireLight->SetIntensity(0.f);
    OrganicFireLight->SetAttenuationRadius(210.f);
    OrganicFireLight->SetCastShadows(true);
    OrganicFireLight->SetLightColor(FLinearColor(1.f,.22f,.025f));
    OrganicFurnaceAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("OrganicFurnaceAudio"));
    OrganicFurnaceAudio->SetupAttachment(GetCapsuleComponent());
    OrganicFurnaceAudio->bAutoActivate = false;
    OrganicIgnitionAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("OrganicIgnitionAudio"));
    OrganicIgnitionAudio->SetupAttachment(GetCapsuleComponent());
    OrganicIgnitionAudio->bAutoActivate = false;
    for (int32 Side=0; Side<2; ++Side)
    {
        UAudioComponent* Release=CreateDefaultSubobject<UAudioComponent>(
            *FString::Printf(TEXT("OrganicReleaseAudio%d"),Side));
        Release->SetupAttachment(GetCapsuleComponent());
        Release->bAutoActivate=false;
        OrganicReleaseAudio.Add(Release);
    }
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
    CasterIntent = ECasterIntent::Hunt;
    ObservedShields.Reset();
    SenseTime = CasterClock = DecisionTime = IncomingThreatTime = ThreatReactionTime = 0.f;
    EvadeCooldown = WithdrawCooldown = FlankCooldown = GuardPressureTime = RecentDamageTime = 0.f;
    bTargetVisible = bTargetGuarding = bHasTargetMemory = false;
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
    OrganicIdlePhase = static_cast<float>((PlacementKey >> 6u) & 1023u) * (17.f / 1024.f);
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
    OrganicMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    ResetOrganicLocomotion();
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
    const bool bLegacyRig = FParse::Param(FCommandLine::Get(), TEXT("DBLegacyCreatureRig"));
    const bool bAnatomyRig = !bLegacyRig && (FParse::Param(FCommandLine::Get(), TEXT("DBAnatomyCreatureRig"))
        || !FParse::Param(FCommandLine::Get(), TEXT("DBPerformanceCreatureRig")));
    const bool bDreadRig = !bLegacyRig && (FParse::Param(FCommandLine::Get(), TEXT("DBDreadCreatureRig"))
        || (!FParse::Param(FCommandLine::Get(), TEXT("DBAnatomyCreatureRig"))
            && !FParse::Param(FCommandLine::Get(), TEXT("DBPerformanceCreatureRig"))));
    const TCHAR* CreaturePath = bCaster
        ? (bLegacyRig ? TEXT("/Game/Art/OrganicEnemies/Meshes/SK_OE_MireSeer.SK_OE_MireSeer")
            : bDreadRig ? TEXT("/Game/Art/OrganicEnemies/Meshes/SK_OE_MireSeer_Dread.SK_OE_MireSeer_Dread")
            : bAnatomyRig ? TEXT("/Game/Art/OrganicEnemies/Meshes/SK_OE_MireSeer_Anatomy.SK_OE_MireSeer_Anatomy")
            : TEXT("/Game/Art/OrganicEnemies/Meshes/SK_OE_MireSeer_Performance.SK_OE_MireSeer_Performance"))
        : (bLegacyRig ? TEXT("/Game/Art/OrganicEnemies/Meshes/SK_OE_Briarhide.SK_OE_Briarhide")
            : bDreadRig ? TEXT("/Game/Art/OrganicEnemies/Meshes/SK_OE_Briarhide_Dread.SK_OE_Briarhide_Dread")
            : bAnatomyRig ? TEXT("/Game/Art/OrganicEnemies/Meshes/SK_OE_Briarhide_Anatomy.SK_OE_Briarhide_Anatomy")
            : TEXT("/Game/Art/OrganicEnemies/Meshes/SK_OE_Briarhide_Performance.SK_OE_Briarhide_Performance"));
    USkeletalMesh* Creature = LoadObject<USkeletalMesh>(nullptr, CreaturePath);
    OrganicMesh->SetSkinnedAssetAndUpdate(Creature);
    CoreMaterial = nullptr;
    OrganicReferencePose.Reset();
    OrganicReferenceComponentPose.Reset();
    OrganicBoneIndices.Reset();
    if (Creature)
    {
        const FReferenceSkeleton& Skeleton = Creature->GetRefSkeleton();
        OrganicReferencePose = Skeleton.GetRefBonePose();
        OrganicReferenceComponentPose.SetNum(OrganicReferencePose.Num());
        for (int32 Index = 0; Index < OrganicReferencePose.Num(); ++Index)
        {
            OrganicBoneIndices.Add(Skeleton.GetBoneName(Index), Index);
            const int32 Parent = Skeleton.GetParentIndex(Index);
            OrganicReferenceComponentPose[Index] = Parent == INDEX_NONE ? OrganicReferencePose[Index]
                : OrganicReferencePose[Index] * OrganicReferenceComponentPose[Parent];
        }
        UE_LOG(LogTemp, Display, TEXT("Organic performance mesh %s: %d bones, articulated torso %s"),
            *Creature->GetName(), Skeleton.GetNum(), OrganicBoneIndices.Contains(TEXT("chest")) ? TEXT("yes") : TEXT("no"));
        if (!bLegacyRig && (!OrganicBoneIndices.Contains(TEXT("spine_lower")) || !OrganicBoneIndices.Contains(TEXT("chest"))
            || !OrganicBoneIndices.Contains(TEXT("neck")) || !OrganicBoneIndices.Contains(TEXT("clavicle_l"))
            || !OrganicBoneIndices.Contains(TEXT("clavicle_r"))))
            UE_LOG(LogTemp, Error, TEXT("The selected performance mesh is missing its authored torso/shoulder hierarchy."));
        if (bAnatomyRig && !OrganicBoneIndices.Contains(TEXT("jaw")))
            UE_LOG(LogTemp, Error, TEXT("The selected anatomy mesh is missing its articulated jaw."));
        if (UMaterialInterface* Skin = OrganicMesh->GetMaterial(0))
        {
            const FLinearColor CreatureTint = bHunter ? FLinearColor(0.87f, 0.91f, 0.8f)
                : Kind == EDBEnemyKind::Boss ? FLinearColor(0.91f, 0.84f, 0.71f) : FLinearColor::White;
            CoreMaterial = UMaterialInstanceDynamic::Create(Skin, this);
            CoreMaterial->SetVectorParameterValue(TEXT("CreatureTint"), CreatureTint);
            OrganicMesh->SetMaterial(0, CoreMaterial);
            for (int32 Slot=1; Slot<OrganicMesh->GetNumMaterials(); ++Slot)
                if (UMaterialInterface* Face=OrganicMesh->GetMaterial(Slot))
                    if (Face->GetName().Contains(TEXT("M_OE_DreadSkin")))
                    {
                        UMaterialInstanceDynamic* FaceSkin=UMaterialInstanceDynamic::Create(Face,this);
                        FaceSkin->SetVectorParameterValue(TEXT("CreatureTint"),CreatureTint);
                        OrganicMesh->SetMaterial(Slot,FaceSkin);
                    }
        }
    }
    else UE_LOG(LogTemp, Error, TEXT("Organic enemy mesh is missing: %s. Complete the performance import before packaging."), CreaturePath);
    // Keep the proven motion pivots as a pose controller, with no old body art.
    for (UStaticMeshComponent* Part : { BodyPart.Get(), HeadPart.Get(), LeftArmPart.Get(),
        RightArmPart.Get(), LeftLegPart.Get(), RightLegPart.Get(), CorePart.Get() })
    {
        Part->SetStaticMesh(nullptr);
        Part->SetVisibility(false);
    }
    ChargePart->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    ChargePart->SetRelativeScale3D(FVector(0.17f));
    ChargePart->SetRelativeLocation(FVector(78.f, 0.f, 37.f));
    LeftArmPivot->SetRelativeLocation(FVector(0.f, bCaster ? -31.f : -39.f, 52.f));
    RightArmPivot->SetRelativeLocation(FVector(0.f, bCaster ? 31.f : 39.f, 52.f));
    UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Art/PreferredCombat/Materials/M_CombatGlow.M_CombatGlow"));
    if (Glow)
    {
        WarningMaterial = UMaterialInstanceDynamic::Create(Glow, this);
        WarningMarks->SetMaterial(0, WarningMaterial);
        for (int32 Index = 0; Index < ChargePart->GetNumMaterials(); ++Index) ChargePart->SetMaterial(Index, WarningMaterial);
        WarningMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 2.f);
        if (CoreMaterial) CoreMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.6f);
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
    UpdateOrganicPose();
    if (bCaster) PrepareOrganicFire();
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
    // Preserve the original gameplay origin under the legacy body controller.
    // The visible orb can stay with the release gesture while that controller
    // applies its post-shot kick, without moving the next projectile or LOS ray.
    if (bVisualsBuilt && Kind == EDBEnemyKind::Caster && BodyPivot)
        return BodyPivot->GetComponentTransform().TransformPosition(FVector(78.f, 0.f, 37.f));
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
    const ADBGameMode* FireMode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode());
    const bool bFirePaused = FireMode && (FireMode->bPaused || FireMode->bShowingBuild);
    OrganicFurnaceAudio->SetPaused(bFirePaused);
    OrganicIgnitionAudio->SetPaused(bFirePaused);
    for (UAudioComponent* Audio : OrganicReleaseAudio) Audio->SetPaused(bFirePaused);
    if (bFirePaused) return;
    if (bDead)
    {
        if (const ADBGameMode* Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode()))
            if (Mode->bPaused || Mode->bShowingBuild) return;
        UpdateDeath(Dt);
        return;
    }
    if (!IsRoomActive())
    {
        ObservedShields.Reset();
        IncomingThreatTime = GuardPressureTime = 0.f;
        bRepositioning = bTargetVisible = bHasTargetMemory = false;
        StopOrganicFire(true);
        if (Kind == EDBEnemyKind::Caster && Attack == EAttack::Bolt
            && (Phase == EDBEnemyPhase::Telegraph || Phase == EDBEnemyPhase::Attack))
        {
            // A deactivated encounter cannot later resume half of a silent cast.
            Attack=EAttack::None; Phase=EDBEnemyPhase::Dormant;
            ShotsRemaining=0; TellTime=TellDuration=AttackElapsed=PhaseTime=0.f;
            bAimLocked=bOrganicCastReleased=false;
            Cooldown=FMath::Max(Cooldown,.8f);
        }
        GetCharacterMovement()->StopMovementImmediately();
        ConsumeMovementInputVector();
        // Creatures waiting in another court remain alive and breathing.
        // Menus and pause still freeze the world, including cosmetic motion.
        const ADBGameMode* Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode());
        const bool bWorldRunning = !Mode || (!Mode->bPaused && !Mode->bChoosingReward && !Mode->bShowingBuild
            && !Mode->bTitle && !Mode->bWon && !Mode->bDefeated);
        if (bWorldRunning && Phase == EDBEnemyPhase::Dormant) UpdateVisuals(Dt);
        WarningMarks->ClearInstances();
        ClearElementVisuals();
        ChargePart->SetVisibility(false);
        return;
    }
    if (!Target.IsValid()) Target = Cast<ADBCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!Target.IsValid() || Target->bDead || FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation()) > FMath::Square(3600.f))
    {
        ObservedShields.Reset();
        IncomingThreatTime = GuardPressureTime = 0.f;
        bRepositioning = bTargetVisible = bHasTargetMemory = false;
        GetCharacterMovement()->StopMovementImmediately();
        Phase = EDBEnemyPhase::Dormant;
        UpdateVisuals(Dt);
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
    if (Kind == EDBEnemyKind::Caster) UpdateCasterSenses(Dt);
    switch (Phase)
    {
    case EDBEnemyPhase::Approach: UpdateApproach(Dt); break;
    case EDBEnemyPhase::Telegraph: UpdateTell(Dt); break;
    case EDBEnemyPhase::Attack: UpdateAttack(Dt); break;
    case EDBEnemyPhase::Recovery:
    case EDBEnemyPhase::Staggered:
    {
        PhaseTime -= Dt;
        // A struck body keeps its collision-resolved impulse during the first recovery beat.
        ConsumeMovementInputVector();
        const bool bHunterNeedsSpace = Phase == EDBEnemyPhase::Recovery && Kind == EDBEnemyKind::Hunter
            && Attack == EAttack::Lunge && KnockbackTime <= 0.f
            && RecoveryDuration - PhaseTime > .18f
            && FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation()) < FMath::Square(205.f);
        // Absorb the landing first, then take real supporting steps out of the
        // player's camera. CharacterMovement and the foot solver own this
        // retreat; neither the player nor the rendered mesh is teleported.
        if (bHunterNeedsSpace) MoveDirection(GetActorLocation() - Target->GetActorLocation(), Dt, .6f);
        else if (KnockbackTime <= 0.f) GetCharacterMovement()->StopMovementImmediately();
        if (PhaseTime <= 0.f) { Phase = EDBEnemyPhase::Approach; bVulnerable = false; Telegraph.Empty(); }
        break;
    }
    default: break;
    }
    // UE5.8 MovementComponent registers bTickBeforeOwner=true: translation has
    // already finished here. Evaluate after the AI's final actor yaw as well.
    // The movement-updated delegate is too early and let later turns rotate
    // both planted ankles away from their world contacts in the same frame.
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
    const bool bCasterTravel = Kind == EDBEnemyKind::Caster;
    const bool bCasterEvasion = bCasterTravel && bRepositioning && CasterIntent == ECasterIntent::Evade;
    const FVector Facing = Target.IsValid() && (Kind == EDBEnemyKind::Melee || (bRepositioning && !bCasterTravel)
        || (Kind == EDBEnemyKind::Hunter && Phase == EDBEnemyPhase::Recovery))
        && FVector::DistSquared2D(Start, Target->GetActorLocation()) < FMath::Square(1200.f)
        ? Target->GetActorLocation() - Start : Chosen;
    const FRotator Face(0.f, Facing.Rotation().Yaw, 0.f);
    SetActorRotation(bCasterTravel
        ? FMath::RInterpConstantTo(GetActorRotation(), Face, DeltaSeconds, bCasterEvasion ? 300.f : 165.f)
        : FMath::RInterpTo(GetActorRotation(), Face, DeltaSeconds, 7.f));
    // Let the carried body turn into a route before taking full forward steps.
    // Facing the player throughout every relocation produced a constant crab walk.
    const float TravelYaw = bOrganicFeetInitialized ? OrganicFacingYaw : GetActorRotation().Yaw;
    float TravelAlignment = bCasterTravel
        ? FMath::Clamp((FVector::DotProduct(FRotator(0.f,TravelYaw,0.f).Vector(),Chosen)-.5f)/.4f,0.f,1.f)
        : 1.f;
    // A single emergency step starts while turning; ordinary travel still
    // waits for forward alignment. Collision and planted-foot solving remain.
    if (bCasterEvasion) TravelAlignment = FMath::Max(.55f,TravelAlignment);
    GetCharacterMovement()->MaxWalkSpeed = BaseSpeed * SpeedMultiplier * TravelAlignment * (1.f - ChillStacks * 0.16f);
    if (TravelAlignment > .01f) AddMovementInput(Chosen, 1.f, true);
    else ConsumeMovementInputVector();
}

void ADBEnemy::UpdateCasterSenses(float DeltaSeconds)
{
    if (!Target.IsValid()) return;
    CasterClock += DeltaSeconds;
    SenseTime -= DeltaSeconds;
    DecisionTime = FMath::Max(0.f, DecisionTime - DeltaSeconds);
    EvadeCooldown = FMath::Max(0.f, EvadeCooldown - DeltaSeconds);
    WithdrawCooldown = FMath::Max(0.f, WithdrawCooldown - DeltaSeconds);
    FlankCooldown = FMath::Max(0.f, FlankCooldown - DeltaSeconds);
    RecentDamageTime = FMath::Max(0.f, RecentDamageTime - DeltaSeconds);
    IncomingThreatTime = FMath::Max(0.f, IncomingThreatTime - DeltaSeconds);
    ThreatReactionTime = FMath::Max(0.f, ThreatReactionTime - DeltaSeconds);
    GuardPressureTime = bTargetVisible && bTargetGuarding ? FMath::Min(3.f, GuardPressureTime + DeltaSeconds) : 0.f;
    if (SenseTime > 0.f) return;
    SenseTime = .12f;
    const FVector PlayerPosition = Target->GetActorLocation();
    bTargetVisible = HasSightTo(PlayerPosition, Target.Get());
    // Entry gives one last-known position. Thereafter walls break tracking;
    // only another sighting updates the memory.
    if (!bHasTargetMemory || bTargetVisible) LastSeenPlayer = PlayerPosition;
    bHasTargetMemory = true;
    bTargetGuarding = bTargetVisible && Target->bGuarding;
    SeenPlayerVelocity = bTargetVisible ? Target->GetVelocity().GetClampedToMaxSize(650.f) : FVector::ZeroVector;
    if (bTargetVisible) SeenPlayerAim = Target->GetAimDirection().GetSafeNormal2D();

    TArray<FObservedShield> VisibleShields;
    float EarliestThreat = 1.f;
    for (int32 Index = 0; Index < Target->GetShieldPieceCount(); ++Index)
    {
        ADBThrownShield* Piece = Target->GetPieceFlight(Index);
        if (!IsValid(Piece) || Target->GetPieceState(Index) != EDBShieldPieceState::Outbound) continue;
        const FVector Position = Piece->GetActorLocation();
        const FVector ToPiece = Position - GetActorLocation();
        if (ToPiece.SizeSquared() > FMath::Square(1600.f)
            || FVector::DotProduct(ToPiece.GetSafeNormal2D(), GetActorForwardVector()) < -.15f
            || !HasSightTo(Position, Piece)) continue;
        const FObservedShield* Previous = ObservedShields.FindByPredicate(
            [Piece](const FObservedShield& Sample) { return Sample.Actor.Get() == Piece; });
        if (Previous && CasterClock > Previous->Time)
        {
            // Manually moved shields have no AActor velocity. Observe their
            // actual visible travel; never dodge the player's button press.
            const FVector Velocity = (Position - Previous->Position) / (CasterClock - Previous->Time);
            const float SpeedSquared = Velocity.SizeSquared();
            if (SpeedSquared > FMath::Square(200.f))
            {
                const FVector ToBody = GetActorLocation() - Position;
                const float Arrival = FVector::DotProduct(ToBody, Velocity) / SpeedSquared;
                if (Arrival > .08f && Arrival < .9f && Arrival < EarliestThreat
                    && (ToBody - Velocity * Arrival).SizeSquared() < FMath::Square(130.f))
                {
                    EarliestThreat = Arrival;
                    IncomingShieldDirection = Velocity.GetSafeNormal2D();
                }
            }
        }
        FObservedShield Sample;
        Sample.Actor = Piece; Sample.Position = Position; Sample.Time = CasterClock;
        VisibleShields.Add(Sample);
    }
    ObservedShields = MoveTemp(VisibleShields);
    if (EarliestThreat < 1.f)
    {
        if (IncomingThreatTime <= 0.f) ThreatReactionTime = .20f;
        IncomingThreatTime = .27f;
    }
}

bool ADBEnemy::ChooseCasterPosition(ECasterIntent Intent)
{
    const FVector Start = GetActorLocation();
    const FVector Away = (Start - LastSeenPlayer).GetSafeNormal2D();
    const FVector Side = FVector::CrossProduct(
        Intent == ECasterIntent::Evade ? IncomingShieldDirection : Away, FVector::UpVector) * AvoidanceSide;
    const FVector Basis = Intent == ECasterIntent::Withdraw ? Away : Intent == ECasterIntent::Hunt ? -Away : Side;
    const float CurrentRange = FVector::Dist2D(Start, LastSeenPlayer);
    const float CurrentGuardAngle = FVector::DotProduct(Away, SeenPlayerAim);
    const bool bWounded = Health < MaxHealth * .45f;
    float BestScore = TNumericLimits<float>::Max();
    FVector Best = Start;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DBCasterPosition), false, this);
    Params.AddIgnoredActor(Target.Get());
    const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float Half = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    for (float Length : { Intent == ECasterIntent::Evade ? 220.f : 380.f,
        Intent == ECasterIntent::Evade ? 340.f : 650.f })
    for (float Angle : { 0.f, 35.f, -35.f, 70.f, -70.f, 110.f, -110.f, 180.f })
    {
        FVector Candidate = KeepInsideArena(Start + Basis.RotateAngleAxis(Angle, FVector::UpVector) * Length);
        const float Travel = FVector::Dist2D(Candidate, Start);
        if (Travel < 150.f) continue;
        FHitResult FloorHit, RouteHit, CoverHit;
        if (!GetWorld()->LineTraceSingleByChannel(FloorHit, Candidate + FVector(0.f,0.f,80.f),
            Candidate - FVector(0.f,0.f,Half + 85.f), ECC_WorldStatic, Params)
            || FloorHit.ImpactNormal.Z < .7f || FMath::Abs(FloorHit.ImpactPoint.Z - (Start.Z-Half)) > 75.f) continue;
        Candidate.Z = FloorHit.ImpactPoint.Z + Half + 2.f;
        if (GetWorld()->SweepSingleByChannel(RouteHit, Start + FVector(0.f,0.f,8.f),
            Candidate + FVector(0.f,0.f,8.f), FQuat::Identity, ECC_Visibility,
            FCollisionShape::MakeCapsule(Radius*.9f,Half-14.f), Params)) continue;
        const bool bCovered = GetWorld()->LineTraceSingleByChannel(CoverHit, Candidate + FVector(0.f,0.f,28.f),
            LastSeenPlayer, ECC_Visibility, Params);
        const float Range = FVector::Dist2D(Candidate, LastSeenPlayer);
        const float GuardAngle = FVector::DotProduct((Candidate-LastSeenPlayer).GetSafeNormal2D(), SeenPlayerAim);
        float Score = Travel * .15f;
        if (Intent == ECasterIntent::Evade)
        {
            const float Clearance = FMath::Abs(FVector::DotProduct(Candidate-Start,Side));
            if (Clearance < 150.f || Range < 240.f) continue;
            Score += FMath::Abs(Clearance-250.f) + FMath::Abs(Range-800.f)*.12f;
        }
        else if (Intent == ECasterIntent::Withdraw)
        {
            if (Range < CurrentRange+110.f) continue;
            Score += FMath::Abs(Range-(bWounded ? 1050.f : 800.f))*.7f + (bCovered ? 90.f : 0.f);
        }
        else if (Intent == ECasterIntent::Flank)
        {
            if (Range < 400.f || GuardAngle > CurrentGuardAngle-.12f || bCovered) continue;
            Score += FMath::Abs(Range-780.f)*.4f + FMath::Max(0.f,GuardAngle)*650.f;
        }
        else Score += FMath::Abs(Range-(bTargetVisible ? 850.f : 250.f)) + (bCovered ? 500.f : 0.f);
        if (Score < BestScore) { BestScore = Score; Best = Candidate; }
    }
    DecisionTime = .3f;
    if (BestScore == TNumericLimits<float>::Max()) return false;
    RepositionTarget = Best;
    RepositionTime = 0.f;
    CasterMoveDuration = Intent == ECasterIntent::Evade ? 1.55f : 3.1f;
    CasterIntent = Intent;
    SteeringTime = 0.f;
    bRepositioning = true;
    return true;
}

void ADBEnemy::UpdateCasterCombat(float DeltaSeconds)
{
    const FVector ToPlayer = LastSeenPlayer - GetActorLocation();
    const float Distance = ToPlayer.Size2D();
    // Only Approach makes these decisions. Gathering, throwing and recovering
    // remain commitments that cannot be cancelled to dodge a late attack.
    if (IncomingThreatTime > 0.f && ThreatReactionTime <= 0.f && EvadeCooldown <= 0.f)
    {
        EvadeCooldown = 4.5f;
        ChooseCasterPosition(ECasterIntent::Evade);
    }
    else if (!(bRepositioning && CasterIntent == ECasterIntent::Evade)
        && bTargetVisible && WithdrawCooldown <= 0.f
        && (Distance < 480.f || (bNeedsReposition && RecentDamageTime > 0.f && Distance < 1050.f)))
    {
        WithdrawCooldown = 6.5f;
        bNeedsReposition = false;
        ChooseCasterPosition(ECasterIntent::Withdraw);
    }
    if (bRepositioning)
    {
        RepositionTime += DeltaSeconds;
        const bool bReached = FVector::DistSquared2D(GetActorLocation(),RepositionTarget) < FMath::Square(65.f);
        const bool bEnoughSpace = CasterIntent == ECasterIntent::Withdraw && RepositionTime > .8f
            && Distance > (Health < MaxHealth*.45f ? 970.f : 760.f);
        if (!bReached && !bEnoughSpace && RepositionTime < CasterMoveDuration)
        {
            MoveToward(RepositionTarget,DeltaSeconds,CasterIntent == ECasterIntent::Evade ? 1.4f
                : CasterIntent == ECasterIntent::Withdraw ? 1.15f : 1.f);
            return;
        }
        bRepositioning = false;
        DecisionTime = .35f;
    }
    if (!bTargetVisible || Distance > 1120.f)
    {
        CasterIntent = ECasterIntent::Hunt;
        if (DecisionTime <= 0.f && ChooseCasterPosition(ECasterIntent::Hunt)) return;
        if (Distance > 180.f) { MoveToward(LastSeenPlayer,DeltaSeconds); return; }
    }
    else if (GuardPressureTime > 1.f && FlankCooldown <= 0.f && DecisionTime <= 0.f)
    {
        // One better angle, then fight from it even if the guard tracks us.
        // Holding a shield must not turn the encounter into endless circling.
        FlankCooldown = 6.f;
        if (ChooseCasterPosition(ECasterIntent::Flank)) return;
    }
    CasterIntent = ECasterIntent::Hold;
    ConsumeMovementInputVector();
    const FRotator FacePlayer(0.f,ToPlayer.Rotation().Yaw,0.f);
    SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(),FacePlayer,DeltaSeconds,150.f));
    const float FacingError = FMath::Abs(FMath::FindDeltaAngleDegrees(
        bOrganicFeetInitialized ? OrganicFacingYaw : GetActorRotation().Yaw,FacePlayer.Yaw));
    if (bTargetVisible && Cooldown <= 0.f && Distance < 1180.f && FacingError < 25.f
        && GetVelocity().SizeSquared2D() < FMath::Square(45.f)) BeginTell(EAttack::Bolt,1.35f);
}

void ADBEnemy::UpdateApproach(float DeltaSeconds)
{
    if (!Target.IsValid()) return;
    const FVector ToPlayer = Target->GetActorLocation() - GetActorLocation();
    const float Distance = ToPlayer.Size2D();
    const bool bSight = HasSightTo(Target->GetActorLocation(), Target.Get());
    if (Kind == EDBEnemyKind::Caster)
    {
        UpdateCasterCombat(DeltaSeconds);
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
    bOrganicCastReleased = false;
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
    case EAttack::Bolt: Telegraph = TEXT("FIRE GATHERING - DODGE / RAISE SHIELD"); break;
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
    else if (Kind == EDBEnemyKind::Caster)
    {
        OrganicIgnitionAudio->SetVolumeMultiplier(.70f);
        OrganicIgnitionAudio->Play();
        OrganicFurnaceAudio->SetVolumeMultiplier(.08f);
        OrganicFurnaceAudio->Play();
        bOrganicFireActive = true;
    }
    else if (EnemyTellSound) UGameplayStatics::PlaySoundAtLocation(this, EnemyTellSound, GetActorLocation(), .65f, .82f);
}

void ADBEnemy::UpdateTell(float DeltaSeconds)
{
    if (!Target.IsValid()) { BeginRecovery(0.4f); return; }
    GetCharacterMovement()->StopMovementImmediately();
    if (!bAimLocked)
    {
        FVector Aim = Target->GetActorLocation();
        if (Kind == EDBEnemyKind::Caster)
        {
            // Read visible travel before the existing halfway aim lock. Late
            // dodges still defeat the committed throw; cover breaks tracking.
            Aim = LastSeenPlayer;
            const float FlightTime = FMath::Clamp(FVector::Distance(Aim,ShotOrigin())/690.f,.15f,.8f);
            const FVector Lead = SeenPlayerVelocity.GetClampedToMaxSize(360.f)*FlightTime;
            if (bTargetVisible && HasSightTo(Aim+Lead,Target.Get())) Aim += Lead;
        }
        LockedDirection = (Aim - ShotOrigin()).GetSafeNormal();
        TellTarget = Aim;
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
    NextShotTime = Attack == EAttack::Bolt ? .03f : 0.f;
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
        bOrganicPounceBlocked = false;
        OrganicBlockedPounceTime = 0.f;
        OrganicPounceStart = GetActorLocation();
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
    const FVector Origin = ShotOrigin();
    if (Kind == EDBEnemyKind::Caster)
    {
        if (!bOrganicCastReleased) OrganicCastOrigin = Origin;
        bOrganicCastReleased = true;
        // Commit after the current tick's authored pose is solved. Fire leaves
        // the hand actually shown at the release, without a detached emitter.
        PendingOrganicBolts.Add({Direction,Color,Damage,ShotsRemaining == 2 ? 1 : 0});
        return;
    }
    if (EnemyFireSound) UGameplayStatics::PlaySoundAtLocation(this, EnemyFireSound, Origin, 0.78f,
        Kind == EDBEnemyKind::Boss ? 0.82f : 1.f);
    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (ADBProjectile* Bolt = GetWorld()->SpawnActor<ADBProjectile>(Origin, Direction.Rotation(), Params))
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
            NextShotTime += Attack == EAttack::Salvo ? 0.2f : 0.44f;
        }
        if (AttackElapsed >= (Attack == EAttack::Salvo ? 0.65f : 0.87f))
            BeginRecovery(Kind == EDBEnemyKind::Boss ? 1.7f : 1.22f);
    }
    else if (Attack == EAttack::Lunge)
    {
        // Commitment follows the visible locked lane; CharacterMovement stops on solid cover.
        const FVector Next = GetActorLocation() + LockedDirection * 100.f;
        if (FVector::DistSquared2D(Next, KeepInsideArena(Next)) < 1.f)
            AddMovementInput(LockedDirection, 1.f, true);
        // Two movement updates have had a chance to advance the pounce. If
        // solid contact or the arena boundary held it here, brace against that
        // stop instead of displaying a complete flight over a stationary body.
        // Damage, commitment duration and recovery remain gameplay-owned.
        if (!bOrganicPounceBlocked && AttackElapsed >= .06f && AttackElapsed <= .18f
            && FVector::DistSquared2D(GetActorLocation(),OrganicPounceStart) < FMath::Square(6.f)
            && GetVelocity().SizeSquared2D() < FMath::Square(30.f))
        {
            bOrganicPounceBlocked = true;
            OrganicBlockedPounceTime = 0.f;
            OrganicPhaseStartBonePose = OrganicMesh->BoneSpaceTransforms;
            OrganicPhaseBlendTime = 0.f;
        }
        if (!bHitAttempted && TryMeleeHit(150.f, 0.15f, AttackDamage)) bHitAttempted = true;
        if (Phase != EDBEnemyPhase::Attack || bDead) return;
        // Contact completes the forward commitment. Continuing to drive after
        // the hit forced the tiny collision capsules together and left the
        // creature's crown inside the player's view throughout recovery.
        if (bHitAttempted)
        {
            ConsumeMovementInputVector();
            GetCharacterMovement()->StopMovementImmediately();
        }
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
    // Keep a breathing/assessment beat between volleys now that the caster no
    // longer spends every recovery walking around an arbitrary ring.
    Cooldown = Kind == EDBEnemyKind::Caster ? Duration + 1.5f : .35f;
    MeleeSetupTime = 0.f;
    if (Kind == EDBEnemyKind::Caster) bRepositioning = false;
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
    RecentDamageTime = 4.f;
    bNeedsReposition = true;
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
    StopOrganicFire(false);
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
    OrganicDeathStartPose = OrganicMesh->BoneSpaceTransforms;
    OrganicDeathPelvisStart = OrganicMesh->GetBoneLocationByName(TEXT("pelvis"), EBoneSpaces::WorldSpace);
    const FRotator DeathFacing(0.f, OrganicFacingYaw, 0.f);
    DeathLocalDirection = DeathFacing.UnrotateVector(LastHitDirection);
    if (DeathLocalDirection.IsNearlyZero()) DeathLocalDirection = -FVector::ForwardVector;
    const float FallSide = DeathLocalDirection.Y >= 0.f ? 1.f : -1.f;
    const float FallBack = DeathLocalDirection.X >= 0.f ? -1.f : 1.f;
    const FVector ContactCandidate = OrganicDeathPelvisStart + DeathFacing.RotateVector(
        FVector(-FallBack * 29.f, FallSide * 20.f, 0.f)) * VisualScale;
    FVector PelvisGroundNormal;
    TraceOrganicFoot(ContactCandidate, (Kind == EDBEnemyKind::Caster ? 14.f : 22.f) * VisualScale,
        OrganicDeathPelvisContact, PelvisGroundNormal);
    for (int32 Side = 0; Side < 2; ++Side)
    {
        bOrganicDeathHandContact[Side] = false;
        const FName FootName(Side == 0 ? TEXT("foot_l") : TEXT("foot_r"));
        FOrganicFoot& Foot = OrganicFeet[Side];
        Foot.Position = OrganicMesh->GetBoneLocationByName(FootName, EBoneSpaces::WorldSpace);
        Foot.Rotation = OrganicMesh->GetBoneRotationByName(FootName, EBoneSpaces::WorldSpace).Quaternion();
        FVector GroundNormal;
        const float Sign = Side == 0 ? -1.f : 1.f;
        const bool bFailedSupport = Sign == FallSide;
        const FVector FootDrag = bFailedSupport
            ? DeathFacing.RotateVector(FVector(-12.f,-Sign * 8.f,0.f)) * VisualScale : FVector::ZeroVector;
        TraceOrganicFoot(Foot.Position + FootDrag, Foot.AnkleHeight, DeathFootTargets[Side], GroundNormal);
        DeathFootRotations[Side] = FQuat(GroundNormal, FMath::DegreesToRadians(bFailedSupport ? Sign * 17.f : 0.f))
            * FQuat::FindBetweenNormals(Foot.Normal, GroundNormal) * Foot.Rotation;
        Foot.bSwinging = false;
    }
    bOrganicFeetInitialized = bOrganicGrounded = false;
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
    OrganicPoseDelta = DeltaSeconds;
    DeathTime += DeltaSeconds;
    const float Kneel = FMath::SmoothStep(0.f, 0.26f, DeathTime);
    const float Fall = FMath::SmoothStep(0.18f, 0.70f, DeathTime);
    const float Settle = FMath::Exp(-FMath::Max(0.f, DeathTime - 0.70f) * 11.f)
        * FMath::Sin(FMath::Max(0.f, DeathTime - 0.70f) * 19.f);
    const float Side = DeathLocalDirection.Y >= 0.f ? 1.f : -1.f;
    const float Back = DeathLocalDirection.X >= 0.f ? -1.f : 1.f;
    const float Pelvis = FMath::Lerp(94.f, 38.f, Kneel);
    BodyPivot->SetRelativeLocation(FVector(Fall * -Back * 22.f, Side * Fall * 15.f, Pelvis));
    BodyPivot->SetRelativeRotation(FRotator(Back * (12.f * Kneel + 75.f * Fall + Settle * 1.5f),
        Side * Fall * 14.f, Side * Fall * 10.f));
    HeadPivot->SetRelativeRotation(FRotator(-Back * (Kneel * 18.f - Fall * 8.f), Side * 9.f * Fall, Side * 14.f * Fall));
    LeftArmPivot->SetRelativeRotation(FRotator(Back * (45.f * Kneel - Fall * 52.f), -18.f * Fall, -20.f - 18.f * Fall));
    RightArmPivot->SetRelativeRotation(FRotator(Back * (35.f * Kneel - Fall * 48.f), 24.f * Fall, 15.f + 20.f * Fall));
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
    UpdateOrganicPose();
    WarningMarks->ClearInstances();
    EffectMarks->ClearInstances();
    if (CoreMaterial) CoreMaterial->SetVectorParameterValue(TEXT("Color"), DangerColor * FMath::Max(0.015f, 1.f - DeathTime * 1.5f));
    if (!bDeathLanded && DeathTime >= 0.59f)
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

void ADBEnemy::ResetOrganicLocomotion()
{
    bOrganicFeetInitialized = false;
    bOrganicGrounded = false;
    bOrganicLungeNeedsLanding = bOrganicLanding = false;
    OrganicLandingTime = 0.f;
    OrganicLandingPelvisStart = FVector::ZeroVector;
    OrganicLastLocation = GetActorLocation();
    OrganicVelocity = OrganicPelvisOffset = OrganicSupportOffset = FVector::ZeroVector;
    OrganicMotionLean = OrganicMotionLeanVelocity = FVector::ZeroVector;
    OrganicGaze = OrganicGazeVelocity = FVector::ZeroVector;
    OrganicFacingVelocity = 0.f;
    bOrganicGazeInitialized = false;
    OrganicAttentionWeights = FVector(1.f, 1.f, 0.f);
    OrganicAttentionVelocity = FVector::ZeroVector;
    bOrganicCastReleased = false;
    OrganicCrestMotion = OrganicCrestVelocity = FVector::ZeroVector;
    OrganicFacingYaw = GetActorRotation().Yaw;
    OrganicSpeed = OrganicArmDrive = OrganicHipYaw = OrganicHipRoll = OrganicSupportDrop = OrganicTurnRate = OrganicStepCooldown = OrganicPerformanceCycle = 0.f;
    OrganicPerformanceStepSide = INDEX_NONE;
    bOrganicPounceBlocked = false;
    OrganicBlockedPounceTime = 0.f;
    OrganicPounceStart = GetActorLocation();
    for (int32 Side = 0; Side < 2; ++Side)
    {
        OrganicPerformancePlantTime[Side] = 1.f;
        bOrganicPerformanceFootSwinging[Side] = false;
        bOrganicDeathHandContact[Side] = false;
    }
    OrganicStepsSinceStop = 0;
    bOrganicSlamTargetsInitialized = false;
    OrganicPhaseBlendTime = 1.f;
    OrganicPreviousPhase = EDBEnemyPhase::Dormant;
    OrganicPhaseStartPose.Reset();
    OrganicPhaseStartBonePose.Reset();
    OrganicDeathStartPose.Reset();
    for (FOrganicFoot& Foot : OrganicFeet) Foot = FOrganicFoot();
}

bool ADBEnemy::TraceOrganicFoot(FVector Candidate, float AnkleHeight, FVector& Position, FVector& Normal) const
{
    const float GroundZ = FeetLocation().Z;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DBOrganicFootGround), false, this);
    Params.AddIgnoredActor(Target.Get());
    const FVector Start(Candidate.X, Candidate.Y, GroundZ + 115.f);
    const FVector End(Candidate.X, Candidate.Y, GroundZ - 160.f);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params)
        && Hit.ImpactNormal.Z > 0.6f)
    {
        Normal = Hit.ImpactNormal.GetSafeNormal();
        Position = Hit.ImpactPoint + Normal * AnkleHeight;
        return true;
    }
    Normal = FVector::UpVector;
    Position = FVector(Candidate.X, Candidate.Y, GroundZ + AnkleHeight);
    return false;
}

void ADBEnemy::UpdateOrganicLocomotion(float DeltaSeconds)
{
    if (!OrganicMesh->GetSkinnedAsset() || OrganicReferencePose.IsEmpty()) return;
    const FReferenceSkeleton& Skeleton = OrganicMesh->GetSkinnedAsset()->GetRefSkeleton();
    const int32 FootIndices[2] = { Skeleton.FindBoneIndex(TEXT("foot_l")), Skeleton.FindBoneIndex(TEXT("foot_r")) };
    const int32 HipIndices[2] = { Skeleton.FindBoneIndex(TEXT("thigh_l")), Skeleton.FindBoneIndex(TEXT("thigh_r")) };
    const int32 KneeIndices[2] = { Skeleton.FindBoneIndex(TEXT("shin_l")), Skeleton.FindBoneIndex(TEXT("shin_r")) };
    if (!OrganicReferenceComponentPose.IsValidIndex(FootIndices[0]) || !OrganicReferenceComponentPose.IsValidIndex(FootIndices[1])) return;
    const bool bCaster = Kind == EDBEnemyKind::Caster;
    const FVector Location = GetActorLocation();
    const FVector Travel = Location - OrganicLastLocation;
    const bool bTeleport = Travel.Size2D() > 200.f || FMath::Abs(Travel.Z) > 120.f;
    if (bTeleport) ResetOrganicLocomotion();
    const float Distance = bTeleport ? 0.f : Travel.Size2D();
    const bool bLunging = Phase == EDBEnemyPhase::Attack && Attack == EAttack::Lunge && !bOrganicPounceBlocked;
    const bool bStartingLanding = bOrganicLungeNeedsLanding && !bLunging
        && GetCharacterMovement()->IsMovingOnGround();
    const FVector MeasuredVelocity = DeltaSeconds > SMALL_NUMBER && !bTeleport ? Travel / DeltaSeconds : FVector::ZeroVector;
    const FVector PreviousVelocity = OrganicVelocity;
    OrganicVelocity = FMath::VInterpTo(OrganicVelocity, FVector(MeasuredVelocity.X, MeasuredVelocity.Y, 0.f), DeltaSeconds, 14.f);
    OrganicSpeed = OrganicVelocity.Size2D();
    OrganicLastLocation = Location;
    const float PreviousYaw = OrganicFacingYaw;
    const float TurnSpeed = Phase == EDBEnemyPhase::Attack ? 720.f : Phase == EDBEnemyPhase::Telegraph ? 390.f : bCaster ? 225.f : 165.f;
    // The head can acquire a new target bearing before the carried body turns.
    // A bounded angular response also keeps the trunk moving briefly after a
    // steering change, so it catches over the actual support steps.
    FVector Heading(OrganicFacingYaw, 0.f, 0.f), HeadingVelocity(OrganicFacingVelocity, 0.f, 0.f);
    const FVector HeadingTarget(OrganicFacingYaw + FMath::FindDeltaAngleDegrees(OrganicFacingYaw,
        GetActorRotation().Yaw), 0.f, 0.f);
    const float HeadingResponse = Phase == EDBEnemyPhase::Attack ? 45.f : Phase == EDBEnemyPhase::Telegraph ? 25.f
        : bCaster ? 12.f : Kind == EDBEnemyKind::Hunter ? 11.f : Kind == EDBEnemyKind::Boss ? 7.f : 8.f;
    AdvanceOrganicResponse(Heading, HeadingVelocity, HeadingTarget, HeadingResponse, DeltaSeconds);
    OrganicFacingYaw = FMath::UnwindDegrees(OrganicFacingYaw + FMath::Clamp(
        static_cast<float>(Heading.X) - OrganicFacingYaw, -TurnSpeed * DeltaSeconds, TurnSpeed * DeltaSeconds));
    OrganicFacingVelocity = FMath::Clamp(static_cast<float>(HeadingVelocity.X), -TurnSpeed, TurnSpeed);
    OrganicTurnRate = DeltaSeconds > SMALL_NUMBER ? FMath::FindDeltaAngleDegrees(PreviousYaw, OrganicFacingYaw) / DeltaSeconds : 0.f;
    const FRotator MotionFacing(0.f, OrganicFacingYaw, 0.f);
    const FVector Acceleration = DeltaSeconds > SMALL_NUMBER && !bTeleport
        ? MotionFacing.UnrotateVector((OrganicVelocity - PreviousVelocity) / DeltaSeconds) : FVector::ZeroVector;
    const float SpeedFraction = FMath::Clamp(OrganicSpeed / FMath::Max(1.f, BaseSpeed), 0.f, 1.f);
    const float TurnFraction = FMath::Clamp(OrganicTurnRate / 165.f, -1.f, 1.f);
    const float HeadingError = FMath::Clamp(FMath::FindDeltaAngleDegrees(OrganicFacingYaw,
        GetActorRotation().Yaw), -35.f, 35.f);
    const FVector LeanTarget(
        -FMath::Clamp(static_cast<float>(Acceleration.X / 1500.f), -1.f, 1.f) * 10.f,
        HeadingError * .20f,
        -FMath::Clamp(static_cast<float>(Acceleration.Y / 1500.f), -1.f, 1.f) * 6.f
            - TurnFraction * SpeedFraction * 5.f);
    AdvanceOrganicResponse(OrganicMotionLean, OrganicMotionLeanVelocity, LeanTarget,
        bCaster ? 13.f : Kind == EDBEnemyKind::Boss ? 8.f : 10.f, DeltaSeconds);
    // Only the skin turns with visual inertia. The capsule, warning direction,
    // and ChargePart/ShotOrigin retain their existing actor-space semantics.
    OrganicMesh->SetRelativeRotation(FRotator(0.f,
        FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, OrganicFacingYaw) - 90.f, 0.f));
    const FTransform Frame = OrganicMesh->GetComponentTransform();
    const float Scale = Frame.GetScale3D().GetAbsMax();
    FVector Nominal[2];
    float Lengths[2];
    for (int32 Side = 0; Side < 2; ++Side)
    {
        const FVector Reference = OrganicReferenceComponentPose[FootIndices[Side]].GetLocation();
        Nominal[Side] = Frame.TransformPosition(Reference);
        Lengths[Side] = Scale * ((OrganicReferenceComponentPose[HipIndices[Side]].GetLocation()
            - OrganicReferenceComponentPose[KneeIndices[Side]].GetLocation()).Size()
            + (OrganicReferenceComponentPose[KneeIndices[Side]].GetLocation() - Reference).Size());
    }
    if (!bOrganicFeetInitialized)
    {
        const USkeletalMesh* Asset = Cast<USkeletalMesh>(OrganicMesh->GetSkinnedAsset());
        const float Floor = Asset ? Asset->GetImportedBounds().Origin.Z - Asset->GetImportedBounds().BoxExtent.Z : 0.f;
        for (int32 Side = 0; Side < 2; ++Side)
        {
            FOrganicFoot& Foot = OrganicFeet[Side];
            Foot.AnkleHeight = FMath::Max(2.f, (OrganicReferenceComponentPose[FootIndices[Side]].GetLocation().Z - Floor) * Scale);
            TraceOrganicFoot(Nominal[Side], Foot.AnkleHeight, Foot.Anchor, Foot.Normal);
            Foot.Position = Foot.SwingStart = Foot.SwingEnd = Foot.Anchor;
            Foot.Rotation = FQuat::FindBetweenNormals(FVector::UpVector, Foot.Normal)
                * Frame.GetRotation() * OrganicReferenceComponentPose[FootIndices[Side]].GetRotation();
            Foot.StartRotation = Foot.LandingRotation = Foot.Rotation;
            Foot.FacingYaw = OrganicFacingYaw;
            Foot.bSwinging = false;
            Foot.Progress = 1.f;
            Foot.Velocity = Foot.SwingStartVelocity = FVector::ZeroVector;
            Foot.ToeRotation = FQuat::Identity;
            Foot.LiftPhaseStart = Foot.LiftVelocity = 0.f;
            Foot.bSettling = false;
            if (bStartingLanding)
            {
                // Inherit the last visible air pose, including an early
                // collision termination. Nominal ground anchors are the
                // destination, never a one-frame replacement for the feet.
                Foot.SwingStart = Foot.Position = OrganicMesh->GetBoneLocationByName(
                    Side == 0 ? TEXT("foot_l") : TEXT("foot_r"), EBoneSpaces::WorldSpace);
                Foot.StartRotation = Foot.Rotation = OrganicMesh->GetBoneRotationByName(
                    Side == 0 ? TEXT("foot_l") : TEXT("foot_r"), EBoneSpaces::WorldSpace).Quaternion();
                Foot.Progress = 0.f;
                Foot.LandingNormal = Foot.Normal;
                Foot.bSwinging = Foot.bSettling = true;
            }
        }
        bOrganicFeetInitialized = true;
        if (bStartingLanding)
        {
            OrganicLandingPelvisStart = OrganicPelvisOffset;
            OrganicLandingTime = 0.f;
            bOrganicLanding = true;
            bOrganicLungeNeedsLanding = false;
        }
    }
    bOrganicGrounded = GetCharacterMovement()->IsMovingOnGround() && !bLunging;
    if (!bOrganicGrounded)
    {
        bOrganicFeetInitialized = false;
        if (bLunging) bOrganicLungeNeedsLanding = true;
        bOrganicLanding = false;
        GaitBlend = FMath::FInterpTo(GaitBlend, 0.f, DeltaSeconds, 12.f);
        const float Leap = bLunging ? FMath::Sin(FMath::Clamp(AttackElapsed / .43f, 0.f, 1.f) * PI) : 0.f;
        OrganicPelvisOffset = FVector(0.f, 0.f, Leap * 16.f);
        OrganicArmDrive = 0.f;
        return;
    }
    const float Moving = bOrganicGrounded ? FMath::Clamp(OrganicSpeed / FMath::Max(BaseSpeed, 1.f), 0.f, 1.f) : 0.f;
    GaitBlend = FMath::FInterpTo(GaitBlend, Moving, DeltaSeconds, Moving > GaitBlend ? 6.f : 10.f);
    if (bOrganicLanding)
    {
        if (!bStartingLanding) OrganicLandingTime += DeltaSeconds;
        const float Land = FMath::SmoothStep(0.f, .12f, OrganicLandingTime);
        for (FOrganicFoot& Foot : OrganicFeet)
        {
            Foot.Position = FMath::Lerp(Foot.SwingStart, Foot.SwingEnd, Land);
            Foot.Rotation = FQuat::Slerp(Foot.StartRotation, Foot.LandingRotation, Land).GetNormalized();
            Foot.Progress = Land;
            if (Land >= 1.f)
            {
                Foot.Position = Foot.Anchor = Foot.SwingEnd;
                Foot.Normal = Foot.LandingNormal;
                Foot.Rotation = Foot.LandingRotation;
                Foot.bSwinging = false;
            }
        }
        OrganicPelvisOffset = FMath::Lerp(OrganicLandingPelvisStart, FVector(0.f, 0.f, -5.f), Land);
        OrganicSupportOffset = FVector::ZeroVector;
        OrganicArmDrive = 0.f;
        if (Land >= 1.f)
        {
            bOrganicLanding = false;
            OrganicSupportDrop = 5.f * Scale;
            OrganicStepsSinceStop = 0;
        }
        return;
    }
    const bool bStopping = GetVelocity().Size2D() < 15.f;
    const FVector PlanningVelocity = bStopping ? FVector::ZeroVector : FVector(MeasuredVelocity.X, MeasuredVelocity.Y, 0.f);
    const float PlanningSpeed = PlanningVelocity.Size2D();
    if (bStopping) OrganicStepsSinceStop = 0;
    auto PredictedYaw = [&](float Time)
    {
        return OrganicFacingYaw + (bStopping ? 0.f : OrganicTurnRate * Time);
    };
    auto PredictedTurn = [&](float Time)
    {
        return FQuat(FVector::UpVector, FMath::DegreesToRadians(PredictedYaw(Time) - OrganicFacingYaw));
    };
    auto PredictPoint = [&](FVector Point, float Time)
    {
        return Frame.GetLocation() + PlanningVelocity * Time
            + PredictedTurn(Time).RotateVector(Point - Frame.GetLocation());
    };
    auto LandingFor = [&](int32 Side, float RemainingTime, float LeadTime)
    {
        FVector Landing = PredictPoint(Nominal[Side], RemainingTime) + PlanningVelocity * LeadTime;
        const FVector FutureHip = PredictPoint(Frame.TransformPosition(
            OrganicReferenceComponentPose[HipIndices[Side]].GetLocation()), RemainingTime);
        FVector Radial = Landing - FutureHip;
        Radial.Z = 0.f;
        // Bound the landing around its future hip, at the established working
        // stance height. Actor-centred bounds let an accelerating, turning
        // foot land too wide and demand another deep crouch on flat paving.
        Radial = Radial.GetClampedToMaxSize(Lengths[Side] * .52f);
        Landing.X = FutureHip.X + Radial.X;
        Landing.Y = FutureHip.Y + Radial.Y;
        return Landing;
    };
    auto SwingRate = [&](const FOrganicFoot& Foot)
    {
        return Foot.bSettling || bStopping ? 1.f / FMath::Max(.01f, Foot.Duration)
            : .4f / FMath::Max(.01f, Foot.Duration) + PlanningSpeed * .6f / FMath::Max(1.f, Foot.ExpectedTravel);
    };
    auto StepUrgency = [&](int32 Side, FVector CandidateNominal, float CandidateYaw, bool bOtherSwinging, float OtherProgress)
    {
        const FOrganicFoot& Foot = OrganicFeet[Side];
        const float Gap = FVector::Dist2D(CandidateNominal, Foot.Anchor);
        // Scheduling step two is still the startup catch-up, not a completed
        // steady stride. Transfer the first short plant during that catch-up;
        // retaining it for a full swing forced a sudden deep support drop as
        // the creature accelerated and turned. The shared forecast uses this
        // same release gate, while mature weight-bearing cadence is unchanged.
        const float Transfer = OrganicStepsSinceStop == 2 ? .30f
            : OrganicStepsSinceStop < 2 ? .45f : bCaster ? .94f : 1.f;
        if (bOtherSwinging && Gap < Lengths[Side] * .55f && (PlanningSpeed < 120.f || OtherProgress < Transfer)) return 0.f;
        const float Threshold = OrganicStepsSinceStop == 0 ? 4.f * Scale
            : FMath::Max(8.f * Scale, Lengths[Side] * (bCaster ? .22f : .26f));
        const float Turn = FMath::Abs(FMath::FindDeltaAngleDegrees(Foot.FacingYaw, CandidateYaw));
        const FVector MotionDirection = PlanningVelocity.GetSafeNormal2D();
        const FVector MotionSide = FVector::CrossProduct(FVector::UpVector, MotionDirection);
        const FVector Offset = CandidateNominal - Foot.Anchor;
        const float Trailing = FVector::DotProduct(Offset, MotionDirection);
        const float Sideways = FMath::Abs(FVector::DotProduct(Offset, MotionSide));
        const float Placement = bStopping ? Gap / (8.f * Scale)
            : FMath::Max(Trailing / Threshold, Sideways / (Lengths[Side] * .33f));
        return FMath::Max(Placement, Turn / (bCaster ? 23.f : 29.f));
    };
    OrganicStepCooldown = FMath::Max(0.f, OrganicStepCooldown - DeltaSeconds);
    for (int32 Side = 0; Side < 2; ++Side)
    {
        FOrganicFoot& Foot = OrganicFeet[Side];
        if (!Foot.bSwinging) continue;
        const float RemainingTime = (1.f - Foot.Progress) / FMath::Max(.01f, SwingRate(Foot));
        // The body has already moved this tick; the foot's stored phase has
        // not advanced yet. Its remaining future body travel excludes this
        // tick, otherwise every retarget lands a movement frame too far ahead.
        const float FutureTravelTime = FMath::Max(0.f, RemainingTime - DeltaSeconds);
        const FVector ExpectedLanding = LandingFor(Side, FutureTravelTime, bStopping ? 0.f : Foot.LandingLeadTime);
        const bool bSettleNow = bStopping && !Foot.bSettling;
        const bool bCourseChanged = !bStopping
            && FVector::Dist2D(ExpectedLanding, Foot.SwingEnd) > Lengths[Side] * .04f;
        if (bSettleNow || bCourseChanged)
        {
            // Refit only the base path. Keep the original clearance phase and
            // peak separate, so an early retarget cannot erase toe clearance
            // and a later retarget cannot restart it above a raised foot.
            const float LiftPhase = FMath::Lerp(Foot.LiftPhaseStart, 1.f, Foot.Progress);
            const float CurrentLift = FMath::Square(FMath::Sin(LiftPhase * PI)) * Foot.LiftHeight * Scale;
            Foot.Duration = FMath::Max(DeltaSeconds, bStopping
                ? FMath::Min(RemainingTime, bCaster ? .17f : .20f) : RemainingTime);
            Foot.SwingStart = Foot.Position - FVector(0.f, 0.f, CurrentLift);
            Foot.SwingStartVelocity = (Foot.Velocity - FVector(0.f, 0.f, Foot.LiftVelocity)).GetClampedToMaxSize(650.f * Scale);
            Foot.StartRotation = (Foot.ToeRotation.Inverse() * Foot.Rotation).GetNormalized();
            Foot.LiftPhaseStart = LiftPhase;
            if (bStopping) Foot.LandingLeadTime = 0.f;
            const float NewFutureTravelTime = FMath::Max(0.f, Foot.Duration - DeltaSeconds);
            TraceOrganicFoot(LandingFor(Side, NewFutureTravelTime, Foot.LandingLeadTime), Foot.AnkleHeight, Foot.SwingEnd, Foot.LandingNormal);
            Foot.LandingRotation = FQuat::FindBetweenNormals(FVector::UpVector, Foot.LandingNormal)
                * PredictedTurn(NewFutureTravelTime) * Frame.GetRotation() * OrganicReferenceComponentPose[FootIndices[Side]].GetRotation();
            Foot.FacingYaw = PredictedYaw(NewFutureTravelTime);
            Foot.Progress = 0.f;
            Foot.ExpectedTravel = FMath::Max(1.f, PlanningVelocity.Size2D() * Foot.Duration);
            Foot.bSettling = bStopping;
        }
        // Travel controls most of the swing; a minimum timed finish allows a
        // stopped creature to put its lifted foot down without freezing it.
        const float Advance = Foot.bSettling || OrganicSpeed < 15.f ? DeltaSeconds / Foot.Duration
            : DeltaSeconds * .4f / Foot.Duration + Distance * .6f / FMath::Max(1.f, Foot.ExpectedTravel);
        Foot.Progress = FMath::Min(1.f, Foot.Progress + Advance);
        const float Blend = FMath::SmoothStep(0.f, 1.f, Foot.Progress);
        Foot.Position = FMath::Lerp(Foot.SwingStart, Foot.SwingEnd, Blend);
        const float P = Foot.Progress;
        Foot.Position += Foot.SwingStartVelocity * Foot.Duration * (P * P * P - 2.f * P * P + P);
        const float LiftPhase = FMath::Lerp(Foot.LiftPhaseStart, 1.f, P);
        const float LiftShape = FMath::Square(FMath::Sin(LiftPhase * PI));
        const float Lift = LiftShape * Foot.LiftHeight * Scale;
        Foot.Position.Z += Lift;
        const float Rate = DeltaSeconds > SMALL_NUMBER ? Advance / DeltaSeconds : 0.f;
        Foot.LiftVelocity = FMath::Sin(2.f * PI * LiftPhase) * PI * Foot.LiftHeight * Scale
            * (1.f - Foot.LiftPhaseStart) * Rate;
        Foot.Velocity = ((Foot.SwingEnd - Foot.SwingStart) * (6.f * P * (1.f - P))
            + Foot.SwingStartVelocity * Foot.Duration * (3.f * P * P - 4.f * P + 1.f)) * Rate
            + FVector(0.f, 0.f, Foot.LiftVelocity);
        Foot.Rotation = FQuat::Slerp(Foot.StartRotation, Foot.LandingRotation, Blend).GetNormalized();
        const FVector SwingRight = FRotator(0.f, OrganicFacingYaw, 0.f).RotateVector(FVector::RightVector);
        Foot.ToeRotation = FQuat(SwingRight, FMath::DegreesToRadians(-LiftShape * (bCaster ? 9.f : 13.f)));
        Foot.Rotation = (Foot.ToeRotation * Foot.Rotation).GetNormalized();
        if (Foot.Progress >= 1.f)
        {
            Foot.bSwinging = false;
            Foot.Anchor = Foot.Position = Foot.SwingEnd;
            Foot.Normal = Foot.LandingNormal;
            Foot.Rotation = Foot.LandingRotation;
            Foot.Velocity = FVector::ZeroVector;
            Foot.LiftVelocity = 0.f;
            Foot.ToeRotation = FQuat::Identity;
            // Transfer support on this landing update. A second cooldown here
            // kept the old support foot behind the moving hip for another tick.
            OrganicStepCooldown = 0.f;
        }
    }
    if (bOrganicGrounded && OrganicStepCooldown <= 0.f)
    {
        int32 Pick = INDEX_NONE;
        float Worst = 1.f;
        for (int32 Side = 0; Side < 2; ++Side)
        {
            FOrganicFoot& Foot = OrganicFeet[Side];
            const FOrganicFoot& Other = OrganicFeet[1 - Side];
            if (Foot.bSwinging) continue;
            const float Urgency = StepUrgency(Side, Nominal[Side], OrganicFacingYaw, Other.bSwinging, Other.Progress);
            if (Urgency > Worst) { Pick = Side; Worst = Urgency; }
        }
        if (Pick != INDEX_NONE)
        {
            FOrganicFoot& Foot = OrganicFeet[Pick];
            Foot.Duration = !bStopping ? FMath::Clamp(Lengths[Pick] * (bCaster ? .78f : .88f) / FMath::Max(120.f, PlanningSpeed),
                bCaster ? .18f : .20f, bCaster ? .35f : .43f) : (bCaster ? .22f : .29f);
            const bool bFirstStep = !bStopping && OrganicStepsSinceStop == 0;
            const bool bSecondStep = !bStopping && OrganicStepsSinceStop == 1;
            if (bFirstStep) Foot.Duration = FMath::Min(Foot.Duration, bCaster ? .15f : .17f);
            if (bSecondStep) Foot.Duration = FMath::Min(Foot.Duration, bCaster ? .18f : .20f);
            // The landing prediction uses the duration actually available to
            // the fixed-rate update. Otherwise the last fractional frame left
            // a 30 Hz landing almost ten centimetres behind its planned hip.
            if (!bStopping && !bFirstStep && DeltaSeconds > SMALL_NUMBER)
                Foot.Duration = FMath::CeilToFloat((Foot.Duration - .0001f) / DeltaSeconds) * DeltaSeconds;
            Foot.LandingLeadTime = bStopping || bFirstStep ? 0.f : Foot.Duration * .50f;
            Foot.LiftHeight = bStopping ? 2.f : bFirstStep ? (bCaster ? 5.f : 6.f)
                : bSecondStep ? (bCaster ? 7.f : 9.f) : (bCaster ? 9.f : 12.f);
            TraceOrganicFoot(LandingFor(Pick, Foot.Duration, Foot.LandingLeadTime), Foot.AnkleHeight, Foot.SwingEnd, Foot.LandingNormal);
            Foot.SwingStart = Foot.Position;
            Foot.SwingStartVelocity = FVector::ZeroVector;
            Foot.LiftPhaseStart = Foot.LiftVelocity = 0.f;
            Foot.ToeRotation = FQuat::Identity;
            Foot.bSettling = bStopping;
            Foot.StartRotation = Foot.Rotation;
            Foot.LandingRotation = FQuat::FindBetweenNormals(FVector::UpVector, Foot.LandingNormal)
                * PredictedTurn(Foot.Duration) * Frame.GetRotation() * OrganicReferenceComponentPose[FootIndices[Pick]].GetRotation();
            Foot.FacingYaw = PredictedYaw(Foot.Duration);
            Foot.Progress = Foot.Travel = 0.f;
            Foot.ExpectedTravel = FMath::Max(1.f, PlanningSpeed * Foot.Duration);
            Foot.bSwinging = true;
            // Cosmetic cycle follows the selected real foot; it does not
            // influence scheduling, clearance, reach or the landing deadline.
            OrganicPerformanceStepSide = Pick;
            if (!bStopping) ++OrganicStepsSinceStop;
            OrganicStepCooldown = .025f;
        }
    }
    FVector Support = (OrganicFeet[0].Position + OrganicFeet[1].Position) * .5f;
    if (OrganicFeet[0].bSwinging != OrganicFeet[1].bSwinging)
        Support = OrganicFeet[OrganicFeet[0].bSwinging ? 1 : 0].Anchor;
    Support = (Support - Frame.GetLocation()) * (bCaster ? .15f : .20f);
    Support.Z = 0.f;
    Support = Support.GetClampedToMaxSize((bCaster ? 4.5f : 7.f) * Scale);
    OrganicSupportOffset = FMath::VInterpTo(OrganicSupportOffset, Support, DeltaSeconds, bCaster ? 11.f : 7.f);
    float RequiredDrop = (bCaster ? 8.f : FMath::Lerp(3.f, 5.f, GaitBlend)) * Scale;
    float ForecastDrop = RequiredDrop;
    auto FutureFootPosition = [&](const FOrganicFoot& Foot, float LookAhead)
    {
        if (!Foot.bSwinging) return Foot.Position;
        const float P = FMath::Min(1.f, Foot.Progress + SwingRate(Foot) * LookAhead);
        FVector Position = FMath::Lerp(Foot.SwingStart, Foot.SwingEnd, FMath::SmoothStep(0.f, 1.f, P));
        Position += Foot.SwingStartVelocity * Foot.Duration * (P * P * P - 2.f * P * P + P);
        const float LiftPhase = FMath::Lerp(Foot.LiftPhaseStart, 1.f, P);
        Position.Z += FMath::Square(FMath::Sin(LiftPhase * PI)) * Foot.LiftHeight * Scale;
        return Position;
    };
    for (int32 Side = 0; Side < 2; ++Side)
    {
        const FVector Hip = Frame.TransformPosition(OrganicReferenceComponentPose[HipIndices[Side]].GetLocation()) + OrganicSupportOffset;
        const float Reach = Lengths[Side] * .97f;
        auto NeededDrop = [&](FVector HipPosition, FVector FootPosition)
        {
            const float Horizontal = FVector::Dist2D(HipPosition, FootPosition);
            const float Height = FMath::Sqrt(FMath::Max(0.f, Reach * Reach - Horizontal * Horizontal));
            return HipPosition.Z - FootPosition.Z - Height;
        };
        RequiredDrop = FMath::Max(RequiredDrop, NeededDrop(Hip, OrganicFeet[Side].Position));
        for (int32 Sample = 1; Sample <= 3; ++Sample)
        {
            float LookAhead = Sample * .035f;
            const FOrganicFoot& Foot = OrganicFeet[Side];
            const FOrganicFoot& Other = OrganicFeet[1 - Side];
            if (!Foot.bSwinging)
            {
                // Predict only the stance the scheduler will actually keep.
                // A long fixed-anchor extrapolation after an early support
                // release caused large false crouches late in the orbit path.
                const float Tick = FMath::Max(.001f, DeltaSeconds);
                for (float Probe = Tick; Probe <= LookAhead + .0001f; Probe += Tick)
                {
                    if (OrganicStepCooldown > Probe) continue;
                    const float OtherProgress = Other.bSwinging ? FMath::Min(1.f, Other.Progress + SwingRate(Other) * Probe) : 1.f;
                    if (StepUrgency(Side, PredictPoint(Nominal[Side], Probe), PredictedYaw(Probe),
                        Other.bSwinging && OtherProgress < 1.f, OtherProgress) > 1.f)
                    {
                        LookAhead = Probe;
                        break;
                    }
                }
            }
            const FVector FutureHip = PredictPoint(Frame.TransformPosition(
                OrganicReferenceComponentPose[HipIndices[Side]].GetLocation()), LookAhead) + OrganicSupportOffset;
            ForecastDrop = FMath::Max(ForecastDrop, NeededDrop(FutureHip, FutureFootPosition(Foot, LookAhead)));
        }
    }
    const float MaximumDrop = FMath::Min(Lengths[0], Lengths[1]) * .43f;
    RequiredDrop = FMath::Clamp(RequiredDrop, 0.f, MaximumDrop);
    ForecastDrop = FMath::Clamp(FMath::Max(ForecastDrop, RequiredDrop), 0.f, MaximumDrop);
    // Begin accepting weight before the descending claw reaches the floor.
    // A current-frame-only correction produced a 7-9 cm pelvis pop at every
    // longer stride. Future reach eases the descent; slower recovery avoids a
    // rebound as soon as the next knee begins to bend. Current reach remains
    // a hard bound, including an unexpected stop, reversal or terrain change.
    OrganicSupportDrop = FMath::FInterpTo(OrganicSupportDrop, ForecastDrop, DeltaSeconds,
        ForecastDrop > OrganicSupportDrop ? 14.f : 4.5f);
    OrganicSupportDrop = FMath::Max(OrganicSupportDrop, RequiredDrop);
    const FRotator Facing(0.f, OrganicFacingYaw, 0.f);
    const FVector SupportLocal = Facing.UnrotateVector(OrganicSupportOffset) / FMath::Max(.01f, Scale);
    OrganicPelvisOffset = FVector(SupportLocal.X, SupportLocal.Y, -OrganicSupportDrop / FMath::Max(.01f, Scale));
    const float FootSeparation = FVector::DotProduct(OrganicFeet[1].Position - OrganicFeet[0].Position, Facing.Vector());
    OrganicArmDrive = FMath::FInterpTo(OrganicArmDrive,
        FMath::Clamp(FootSeparation / FMath::Max(1.f, Lengths[0] * .60f), -1.f, 1.f), DeltaSeconds, bCaster ? 10.f : 8.f);
}

FDBEnemyAnimationDebug ADBEnemy::GetAnimationDebugState() const
{
    FDBEnemyAnimationDebug Result;
    Result.speed_cm_s = OrganicSpeed;
    Result.visible_yaw = OrganicFacingYaw;
    Result.movement_blend = GaitBlend;
    Result.attack_elapsed = AttackElapsed;
    Result.pounce_blocked = bOrganicPounceBlocked;
    Result.blocked_pounce_elapsed = OrganicBlockedPounceTime;
    Result.target_visible = bTargetVisible;
    Result.incoming_threat = IncomingThreatTime;
    if (Kind == EDBEnemyKind::Caster)
    {
        switch (CasterIntent)
        {
        case ECasterIntent::Hunt: Result.combat_intent = TEXT("FindShot"); break;
        case ECasterIntent::Flank: Result.combat_intent = TEXT("FlankShield"); break;
        case ECasterIntent::Withdraw: Result.combat_intent = TEXT("CreateSpace"); break;
        case ECasterIntent::Evade: Result.combat_intent = TEXT("EvadeThrow"); break;
        default: Result.combat_intent = TEXT("HoldShot"); break;
        }
        if (Phase == EDBEnemyPhase::Telegraph || Phase == EDBEnemyPhase::Attack) Result.combat_intent = TEXT("CommittedCast");
        if (Phase == EDBEnemyPhase::Recovery || Phase == EDBEnemyPhase::Staggered) Result.combat_intent = TEXT("Recover");
    }
    switch (Attack)
    {
    case EAttack::Swing: Result.attack_name = TEXT("Swing"); break;
    case EAttack::Bolt: Result.attack_name = TEXT("Bolt"); break;
    case EAttack::Lunge: Result.attack_name = TEXT("Lunge"); break;
    case EAttack::Salvo: Result.attack_name = TEXT("Salvo"); break;
    case EAttack::Slam: Result.attack_name = TEXT("Slam"); break;
    case EAttack::Ground: Result.attack_name = TEXT("Ground"); break;
    case EAttack::Intercept: Result.attack_name = TEXT("Intercept"); break;
    default: Result.attack_name = TEXT("None"); break;
    }
    Result.left_target_world = OrganicFeet[0].Position;
    Result.right_target_world = OrganicFeet[1].Position;
    Result.left_ground_normal = OrganicFeet[0].Normal;
    Result.right_ground_normal = OrganicFeet[1].Normal;
    Result.left_planted = bOrganicFeetInitialized && bOrganicGrounded && !OrganicFeet[0].bSwinging && !bDead;
    Result.right_planted = bOrganicFeetInitialized && bOrganicGrounded && !OrganicFeet[1].bSwinging && !bDead;
    Result.left_foot_world = OrganicMesh->GetBoneLocationByName(TEXT("foot_l"), EBoneSpaces::WorldSpace);
    Result.right_foot_world = OrganicMesh->GetBoneLocationByName(TEXT("foot_r"), EBoneSpaces::WorldSpace);
    Result.right_hand_world = OrganicMesh->GetBoneLocationByName(TEXT("hand_r"), EBoneSpaces::WorldSpace);
    Result.left_hand_world = OrganicMesh->GetBoneLocationByName(TEXT("hand_l"), EBoneSpaces::WorldSpace);
    Result.right_shoulder_world = OrganicMesh->GetBoneLocationByName(TEXT("upperarm_r"), EBoneSpaces::WorldSpace);
    Result.facing_forward = FRotator(0.f, OrganicFacingYaw, 0.f).Vector();
    Result.pelvis_world = OrganicMesh->GetBoneLocationByName(TEXT("pelvis"), EBoneSpaces::WorldSpace);
    Result.head_world = OrganicMesh->GetBoneLocationByName(TEXT("head"), EBoneSpaces::WorldSpace);
    Result.left_reach_error_cm = FVector::Dist(Result.left_target_world, Result.left_foot_world);
    Result.right_reach_error_cm = FVector::Dist(Result.right_target_world, Result.right_foot_world);
    if (!bDead && bOrganicSlamTargetsInitialized && Attack == EAttack::Slam
        && (Phase == EDBEnemyPhase::Attack || Phase == EDBEnemyPhase::Recovery))
    {
        Result.left_hand_target_world = OrganicSlamRequestedHands[0];
        Result.right_hand_target_world = OrganicSlamRequestedHands[1];
        Result.left_hand_reach_error_cm = FVector::Dist(Result.left_hand_target_world, Result.left_hand_world);
        Result.right_hand_reach_error_cm = FVector::Dist(Result.right_hand_target_world, Result.right_hand_world);
        const FQuat ActorToReferenceMesh = FRotator(0.f, 90.f, 0.f).Quaternion();
        for (int32 Side = 0; Side < 2; ++Side)
        {
            const FName Name(Side == 0 ? TEXT("hand_l") : TEXT("hand_r"));
            const int32* Index = OrganicBoneIndices.Find(Name);
            if (!Index || !OrganicReferenceComponentPose.IsValidIndex(*Index)) continue;
            const FTransform& ReferenceHand = OrganicReferenceComponentPose[*Index];
            const FVector ReferenceVertex = ReferenceHand.GetLocation()
                + ActorToReferenceMesh.RotateVector(BriarhideClawOffsets[Side]);
            const FVector BoneLocalVertex = ReferenceHand.InverseTransformPosition(ReferenceVertex);
            const FVector ActualVertex = OrganicMesh->GetBoneTransformByName(Name, EBoneSpaces::WorldSpace).TransformPosition(BoneLocalVertex);
            if (Side == 0) Result.left_claw_contact_world = ActualVertex;
            else Result.right_claw_contact_world = ActualVertex;
        }
    }
    if (!OrganicReferenceComponentPose.IsEmpty()) Result.parent_unit_scale = OrganicReferenceComponentPose[0].GetScale3D().GetAbsMax();
    return Result;
}

void ADBEnemy::UpdateVisuals(float DeltaSeconds)
{
    OrganicPoseDelta = DeltaSeconds;
    if (bOrganicPounceBlocked)
    {
        if (Attack == EAttack::Lunge && (Phase == EDBEnemyPhase::Attack || Phase == EDBEnemyPhase::Recovery))
            OrganicBlockedPounceTime += DeltaSeconds;
        else
        {
            bOrganicPounceBlocked = false;
            OrganicBlockedPounceTime = 0.f;
        }
    }
    UpdateOrganicLocomotion(DeltaSeconds);
    VisualTime += DeltaSeconds;
    HitFlash = FMath::Max(0.f, HitFlash - DeltaSeconds);
    HitSoundCooldown = FMath::Max(0.f, HitSoundCooldown - DeltaSeconds);
    GroundPulseTime = FMath::Max(0.f, GroundPulseTime - DeltaSeconds);
    AttackKick = FMath::Max(0.f, AttackKick - DeltaSeconds * 5.5f);
    ReactionTime = FMath::Max(0.f, ReactionTime - DeltaSeconds);
    const bool bCaster = Kind == EDBEnemyKind::Caster;
    const bool bHunter = Kind == EDBEnemyKind::Hunter;
    if (Attack != EAttack::Slam || (Phase != EDBEnemyPhase::Attack && Phase != EDBEnemyPhase::Recovery))
        bOrganicSlamTargetsInitialized = false;
    if (Phase != OrganicPreviousPhase)
    {
        OrganicPhaseStartPose.Reset();
        OrganicPhaseStartBonePose = OrganicMesh->BoneSpaceTransforms;
        for (USceneComponent* Joint : { BodyPivot.Get(), HeadPivot.Get(), LeftArmPivot.Get(), RightArmPivot.Get() })
            OrganicPhaseStartPose.Add(Joint->GetRelativeTransform());
        OrganicPhaseBlendTime = 0.f;
        OrganicPreviousPhase = Phase;
    }
    OrganicPhaseBlendTime += DeltaSeconds;
    const float Stride = OrganicArmDrive * GaitBlend;
    if (OrganicPerformanceStepSide != INDEX_NONE && bOrganicGrounded)
    {
        const FOrganicFoot& Step = OrganicFeet[OrganicPerformanceStepSide];
        // Replans reset Progress, but retain the original lift phase. The
        // performance therefore passes through a turn instead of restarting
        // its arms every time the horizontal foot curve is refitted.
        const float SwingPhase = FMath::Lerp(Step.LiftPhaseStart, 1.f, Step.Progress);
        const float DesiredCycle = (OrganicPerformanceStepSide == 0 ? 0.f : .5f) + SwingPhase * .5f;
        const float DeltaCycle = FMath::FindDeltaAngleDegrees(OrganicPerformanceCycle * 360.f, DesiredCycle * 360.f) / 360.f;
        OrganicPerformanceCycle += DeltaCycle * FMath::Min(1.f, DeltaSeconds * (Kind == EDBEnemyKind::Boss ? 17.f : 22.f));
        OrganicPerformanceCycle -= FMath::FloorToFloat(OrganicPerformanceCycle);
    }
    for (int32 Side = 0; Side < 2; ++Side)
    {
        OrganicPerformancePlantTime[Side] = bOrganicPerformanceFootSwinging[Side] && !OrganicFeet[Side].bSwinging
            ? 0.f : OrganicPerformancePlantTime[Side] + DeltaSeconds;
        bOrganicPerformanceFootSwinging[Side] = OrganicFeet[Side].bSwinging;
    }
    const bool bLocomotionPose = bOrganicGrounded && Phase == EDBEnemyPhase::Approach;
    OrganicHipYaw = FMath::FInterpTo(OrganicHipYaw, bLocomotionPose ? -Stride * (bCaster ? 3.5f : 7.f) : 0.f, DeltaSeconds, 8.f);
    OrganicHipRoll = FMath::FInterpTo(OrganicHipRoll, bLocomotionPose ? OrganicPelvisOffset.Y * .25f : 0.f, DeltaSeconds, 8.f);
    const FVector LocalVelocity = FRotator(0.f, OrganicFacingYaw, 0.f).UnrotateVector(OrganicVelocity) / FMath::Max(1.f, BaseSpeed);
    const float TellProgress = Phase == EDBEnemyPhase::Telegraph && TellDuration > 0.f
        ? FMath::Clamp(1.f - TellTime / TellDuration, 0.f, 1.f) : 0.f;
    const float Windup = FMath::SmoothStep(0.f, 1.f, TellProgress);
    const float Recover = (Phase == EDBEnemyPhase::Recovery || Phase == EDBEnemyPhase::Staggered)
        ? FMath::Clamp(PhaseTime / FMath::Max(0.01f, RecoveryDuration), 0.f, 1.f) : 0.f;
    float LeftArmPitch = bCaster ? 10.f - Stride * 8.f : -8.f - Stride * 32.f;
    float RightArmPitch = bCaster ? 14.f + Stride * 8.f : -4.f + Stride * 32.f;
    float LeftArmRoll = bCaster ? -4.f : -9.f;
    float RightArmRoll = bCaster ? 4.f : 9.f;
    float BodyPitch = (bCaster ? -9.f : bHunter ? -13.f : -5.f) - FMath::Clamp(LocalVelocity.X, -1.f, 1.f) * GaitBlend * 5.f;
    float BodyYaw = Stride * (bCaster ? 2.5f : 6.f) - FMath::Clamp(OrganicTurnRate / 70.f, -3.f, 3.f);
    float BodyRoll = -OrganicPelvisOffset.Y * .65f + FMath::Clamp(LocalVelocity.Y, -1.f, 1.f) * GaitBlend * 3.f;
    float BodyDrop = 0.f;
    float BodyForward = 0.f;
    float ElbowLeft = bCaster ? 38.f : -21.f - FMath::Abs(Stride) * 6.f;
    float ElbowRight = bCaster ? 44.f : -25.f - FMath::Abs(Stride) * 6.f;
    float WristLeft = bCaster ? -12.f : 5.f;
    float WristRight = bCaster ? -15.f : 7.f;
    float HeadPitch = -BodyPitch * 0.65f;
    if (Phase == EDBEnemyPhase::Telegraph)
    {
        if (Attack == EAttack::Intercept)
        {
            LeftArmPitch = -36.f - Windup * 30.f;
            RightArmPitch = -40.f - Windup * 28.f;
            LeftArmRoll = 28.f; RightArmRoll = -28.f;
            BodyPitch = -5.f; BodyYaw = 0.f; BodyDrop = Windup * 5.f;
            HeadPitch = 5.f;
            ElbowLeft = ElbowRight = -55.f;
        }
        else if (Attack == EAttack::Swing)
        {
            const float Gather = FMath::SmoothStep(0.f, .66f, TellProgress);
            const float Set = FMath::SmoothStep(.55f, 1.f, TellProgress);
            RightArmPitch = -32.f - Gather * 65.f - Set * 15.f;
            RightArmRoll = 12.f + Set * 12.f;
            LeftArmPitch = -24.f - Gather * 18.f;
            BodyYaw = 28.f * Gather;
            BodyPitch = 6.f * Windup;
            BodyDrop = 5.f * Windup;
            BodyForward = -6.f * Gather;
            HeadPitch = -8.f;
            ElbowRight = -45.f + Set * 18.f;
            ElbowLeft = -38.f;
            WristRight = -18.f * Set;
        }
        else if (Attack == EAttack::Ground || Attack == EAttack::Slam)
        {
            LeftArmPitch = RightArmPitch = -40.f - Windup * 84.f;
            LeftArmRoll = -20.f; RightArmRoll = 20.f;
            BodyPitch = 9.f * Windup;
            BodyDrop = 9.f * Windup;
            HeadPitch = -14.f;
            ElbowLeft = ElbowRight = -30.f + Windup * 16.f;
            BodyForward = -4.f * Windup;
        }
        else if (Attack == EAttack::Lunge)
        {
            LeftArmPitch = -55.f; RightArmPitch = -30.f;
            BodyPitch = -22.f - Windup * 10.f;
            BodyDrop = 10.f * Windup;
            HeadPitch = 18.f;
            ElbowLeft = -35.f; ElbowRight = -50.f;
        }
        else
        {
            LeftArmPitch = FMath::Lerp(-20.f, -68.f, Windup);
            RightArmPitch = FMath::Lerp(-25.f, -73.f, Windup);
            LeftArmRoll = 19.f; RightArmRoll = -19.f;
            BodyPitch = -6.f; BodyDrop = 4.f * Windup;
            HeadPitch = 2.f;
            ElbowLeft = ElbowRight = -40.f;
            WristLeft = WristRight = 18.f;
        }
    }
    else if (Phase == EDBEnemyPhase::Attack)
    {
        if (Attack == EAttack::Intercept)
        {
            LeftArmPitch = -66.f; RightArmPitch = -68.f;
            LeftArmRoll = 28.f; RightArmRoll = -28.f;
            BodyPitch = -5.f; BodyYaw = 0.f; BodyDrop = 5.f; HeadPitch = 5.f;
            ElbowLeft = ElbowRight = -55.f;
        }
        else if (Attack == EAttack::Swing)
        {
            // Continue over the crown (through -180 degrees), then forward.
            // Returning toward zero sent the claw down behind the hips in the
            // first captured build while damage occurred in front of the body.
            const float Strike = FMath::SmoothStep(.015f, .18f, AttackElapsed);
            const float Follow = FMath::SmoothStep(.19f, .48f, AttackElapsed);
            RightArmPitch = FMath::Lerp(-112.f, -295.f, Strike) - Follow * 45.f;
            RightArmRoll = FMath::Lerp(24.f, 34.f, Strike);
            LeftArmPitch = -42.f + Strike * 25.f;
            BodyYaw = FMath::Lerp(28.f, -24.f, Strike);
            BodyPitch = -14.f * Strike;
            BodyDrop = 5.f + 4.f * Strike;
            BodyForward = FMath::Lerp(-6.f, 7.f, Strike);
            HeadPitch = 8.f;
            ElbowRight = -27.f + Strike * 15.f - Follow * 22.f;
            ElbowLeft = -38.f;
            WristRight = FMath::Lerp(-18.f, 20.f, Strike);
        }
        else if (Attack == EAttack::Slam)
        {
            // This close attack physically drives both claws into the paving.
            // The ranged Ground gesture keeps its separate, upright cast.
            const float Strike = FMath::SmoothStep(.01f, .16f, AttackElapsed);
            // The complete stroke is solved from the outgoing windup hands
            // below. A late blend from a fast FK rotation skipped most of the
            // descending arc in the actual 30 Hz capture.
            LeftArmPitch = RightArmPitch = -124.f;
            LeftArmRoll = -20.f; RightArmRoll = 20.f;
            BodyPitch = FMath::Lerp(9.f, -62.f, Strike);
            BodyDrop = FMath::Lerp(9.f, 25.f, Strike);
            BodyForward = FMath::Lerp(-4.f, 8.f, Strike);
            HeadPitch = FMath::Lerp(-14.f, 27.f, Strike);
            ElbowLeft = ElbowRight = -14.f;
        }
        else if (Attack == EAttack::Ground)
        {
            const float Strike = FMath::SmoothStep(.01f, .16f, AttackElapsed);
            LeftArmPitch = RightArmPitch = FMath::Lerp(-124.f, -320.f, Strike);
            BodyPitch = -22.f * Strike; BodyDrop = 12.f;
            HeadPitch = 12.f;
            ElbowLeft = ElbowRight = -14.f - Strike * 12.f;
            WristLeft = WristRight = Strike * 24.f;
            BodyForward = Strike * 6.f;
        }
        else if (Attack == EAttack::Bolt || Attack == EAttack::Salvo)
        {
            LeftArmPitch = -68.f + AttackKick * 12.f;
            RightArmPitch = -73.f + AttackKick * 14.f;
            LeftArmRoll = 19.f; RightArmRoll = -19.f;
            BodyPitch = -6.f + AttackKick * 13.f;
            HeadPitch = -AttackKick * 8.f;
            ElbowLeft = -30.f - AttackKick * 14.f;
            ElbowRight = -35.f - AttackKick * 16.f;
            WristLeft = WristRight = 18.f - AttackKick * 24.f;
        }
        else if (Attack == EAttack::Lunge)
        {
            // Throw a leading claw and shoulder into the committed lane, then
            // fold the elbows to brace. Holding both arms behind the torso for
            // the whole leap read as a rigid wingspan in the Hunter capture.
            const float Drive = FMath::SmoothStep(0.f, .10f, AttackElapsed);
            const float Brace = FMath::SmoothStep(.25f, .43f, AttackElapsed);
            RightArmPitch = FMath::Lerp(-30.f, 105.f, Drive) - Brace * 62.f;
            LeftArmPitch = FMath::Lerp(-55.f, 70.f, Drive) - Brace * 42.f;
            LeftArmRoll = -10.f; RightArmRoll = 28.f;
            BodyPitch = -32.f + Brace * 8.f;
            BodyYaw = -12.f * Drive * (1.f - Brace * .5f);
            BodyForward = 10.f * Drive * (1.f - Brace);
            HeadPitch = 22.f - Brace * 8.f;
            ElbowRight = -18.f - Brace * 20.f;
            ElbowLeft = -20.f - Brace * 24.f;
            WristRight = 8.f + Brace * 12.f;
            WristLeft = -5.f + Brace * 10.f;
        }
        else { BodyPitch = -30.f; LeftArmPitch = -65.f; RightArmPitch = -58.f; HeadPitch = 20.f; }
    }
    else if (Phase == EDBEnemyPhase::Recovery && Attack == EAttack::Slam)
    {
        const float SinceImpactHold = FMath::Max(0.f, RecoveryDuration - PhaseTime);
        const float Load = 1.f - FMath::SmoothStep(.10f, .78f, SinceImpactHold);
        BodyPitch = FMath::Lerp(BodyPitch, -62.f, Load);
        BodyDrop = 25.f * Load;
        BodyForward = 8.f * Load;
        HeadPitch = 27.f * Load;
        LeftArmPitch = FMath::Lerp(LeftArmPitch, 45.f, Load);
        RightArmPitch = FMath::Lerp(RightArmPitch, 45.f, Load);
        LeftArmRoll = -20.f * Load; RightArmRoll = 20.f * Load;
        ElbowLeft = ElbowRight = -26.f - 8.f * Load;
        WristLeft = WristRight = 20.f * Load;
    }
    else if (Phase == EDBEnemyPhase::Recovery && Attack == EAttack::Lunge)
    {
        const float SinceLanding = FMath::Max(0.f, RecoveryDuration - PhaseTime);
        const float Load = 1.f - FMath::SmoothStep(.09f, .38f, SinceLanding);
        const float Compression = FMath::SmoothStep(0.f, .07f, SinceLanding) * Load;
        RightArmPitch = FMath::Lerp(RightArmPitch, 43.f, Load);
        LeftArmPitch = FMath::Lerp(LeftArmPitch, 28.f, Load);
        RightArmRoll = FMath::Lerp(RightArmRoll, 28.f, Load);
        LeftArmRoll = FMath::Lerp(LeftArmRoll, -10.f, Load);
        BodyPitch = FMath::Lerp(BodyPitch, -24.f, Load) - Compression * 5.f;
        BodyYaw = -6.f * Load;
        BodyDrop = Compression * 11.f;
        HeadPitch = 14.f * Load;
        ElbowRight = FMath::Lerp(ElbowRight, -38.f, Load);
        ElbowLeft = FMath::Lerp(ElbowLeft, -44.f, Load);
        WristRight = 20.f * Load;
        WristLeft = 5.f * Load;
    }
    else if (Recover > 0.f)
    {
        LeftArmPitch = FMath::Lerp(LeftArmPitch, -15.f, Recover);
        RightArmPitch = FMath::Lerp(RightArmPitch, Attack == EAttack::Swing ? 35.f : -12.f, Recover);
        LeftArmRoll = -22.f * Recover;
        RightArmRoll = 27.f * Recover;
        BodyPitch = FMath::Lerp(BodyPitch, Attack == EAttack::Swing ? -13.f : 12.f, Recover);
        BodyYaw = Attack == EAttack::Swing ? -23.f * Recover : 0.f;
        HeadPitch = -9.f * Recover;
        BodyDrop = 6.f * Recover;
        ElbowLeft = FMath::Lerp(ElbowLeft, -30.f, Recover);
        ElbowRight = FMath::Lerp(ElbowRight, -38.f, Recover);
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
    BodyPivot->SetRelativeLocation(FVector(0.f, 0.f, 94.f) + OrganicPelvisOffset
        + FVector(BodyForward, 0.f, -BodyDrop) + RecoilOffset);
    // Breathing expands the chest subtly; it never translates planted feet.
    BodyPitch += FMath::Sin(VisualTime * (bCaster ? 2.1f : 1.65f)) * .45f;
    BodyPivot->SetRelativeRotation(FRotator(BodyPitch, BodyYaw, BodyRoll));
    HeadPivot->SetRelativeRotation(FRotator(HeadPitch, -BodyYaw * 0.65f, -BodyRoll * 0.6f));
    LeftArmPivot->SetRelativeRotation(FRotator(LeftArmPitch, 0.f, LeftArmRoll));
    RightArmPivot->SetRelativeRotation(FRotator(RightArmPitch, 0.f, RightArmRoll));
    LeftLegPivot->SetRelativeRotation(FRotator::ZeroRotator);
    RightLegPivot->SetRelativeRotation(FRotator::ZeroRotator);
    if (OrganicPhaseStartPose.Num() == 4)
    {
        const float Duration = Phase == EDBEnemyPhase::Attack ? .035f : Phase == EDBEnemyPhase::Staggered ? .07f : .16f;
        const float Blend = FMath::SmoothStep(0.f, Duration, OrganicPhaseBlendTime);
        int32 Index = 0;
        for (USceneComponent* Joint : { BodyPivot.Get(), HeadPivot.Get(), LeftArmPivot.Get(), RightArmPivot.Get() })
        {
            const FTransform TargetPose = Joint->GetRelativeTransform();
            FTransform Smoothed;
            Smoothed.Blend(OrganicPhaseStartPose[Index++], TargetPose, Blend);
            Joint->SetRelativeTransform(Smoothed);
        }
    }
    const float JointSpeed = Phase == EDBEnemyPhase::Attack ? 40.f : 13.f;
    LeftElbowPitch = FMath::FInterpTo(LeftElbowPitch, ElbowLeft, DeltaSeconds, JointSpeed);
    RightElbowPitch = FMath::FInterpTo(RightElbowPitch, ElbowRight, DeltaSeconds, JointSpeed);
    LeftWristPitch = FMath::FInterpTo(LeftWristPitch, WristLeft, DeltaSeconds, JointSpeed);
    RightWristPitch = FMath::FInterpTo(RightWristPitch, WristRight, DeltaSeconds, JointSpeed);
    const bool bCharging = (Attack == EAttack::Bolt || Attack == EAttack::Salvo)
        && (Phase == EDBEnemyPhase::Telegraph || (Phase == EDBEnemyPhase::Attack && ShotsRemaining > 0));
    ChargePart->SetVisibility(bCharging && Kind != EDBEnemyKind::Caster);
    if (Kind == EDBEnemyKind::Caster)
        ChargePart->SetWorldLocation(bOrganicCastReleased ? OrganicCastOrigin : ShotOrigin());
    if (bCharging)
    {
        if (Kind == EDBEnemyKind::Boss) ChargePart->SetWorldLocation(ShotOrigin());
        const float Charge = Phase == EDBEnemyPhase::Telegraph ? 0.3f + Windup * 1.7f : 1.65f - AttackKick * 0.6f;
        ChargePart->SetRelativeScale3D(FVector(0.17f * Charge * (1.f + FMath::Sin(VisualTime * 16.f) * 0.035f)));
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
    UpdateOrganicPose();
    UpdateOrganicFire(DeltaSeconds);
    UpdateWarningGeometry();
    UpdateElementVisuals(DeltaSeconds);
}

void ADBEnemy::PrepareOrganicFire()
{
    StopOrganicFire(true);
    OrganicFireMaterial = DBOrganicFire::Prepare(OrganicFireCharge,this);
    OrganicFurnaceAudio->SetSound(LoadObject<USoundBase>(nullptr,
        *DBOrganicFire::SoundPath(TEXT("S_CasterFurnaceLoop"))));
    OrganicIgnitionAudio->SetSound(LoadObject<USoundBase>(nullptr,
        *DBOrganicFire::SoundPath(TEXT("S_CasterIgnite"))));
    OrganicFireReleases.Reset();
    for (const TCHAR* Name : { TEXT("S_CasterReleaseA"),TEXT("S_CasterReleaseB"),TEXT("S_CasterReleaseC") })
    {
        const FString Path=DBOrganicFire::SoundPath(Name);
        OrganicFireReleases.Add(LoadObject<USoundBase>(nullptr,*Path));
    }
}

void ADBEnemy::StopOrganicFire(bool bImmediate)
{
    PendingOrganicBolts.Reset();
    bOrganicFireActive=false;
    OrganicFireHeat=0.f;
    OrganicFireCharge->SetVisibility(false);
    OrganicFireLight->SetIntensity(0.f);
    if (CoreMaterial) CoreMaterial->SetScalarParameterValue(TEXT("InternalHeat"),0.f);
    for (UAudioComponent* Audio : {OrganicFurnaceAudio.Get(),OrganicIgnitionAudio.Get()})
        if (bImmediate) Audio->Stop();
        else if (Audio->IsPlaying()) Audio->FadeOut(.06f,0.f);
    for (UAudioComponent* Audio : OrganicReleaseAudio)
        if (bImmediate) Audio->Stop();
        else if (Audio->IsPlaying()) Audio->FadeOut(.06f,0.f);
}

FVector ADBEnemy::OrganicFireHand(int32 Side) const
{
    const FTransform Hand=OrganicMesh->GetBoneTransformByName(Side == 0 ? TEXT("hand_l") : TEXT("hand_r"),EBoneSpaces::WorldSpace);
    return Hand.GetLocation()+GetActorForwardVector()*(11.f*VisualScale)+FVector::UpVector*(4.f*VisualScale);
}

void ADBEnemy::UpdateOrganicFire(float DeltaSeconds)
{
    if (Kind != EDBEnemyKind::Caster || !OrganicMesh->GetSkinnedAsset()) return;
    const bool bTell=Phase == EDBEnemyPhase::Telegraph && Attack == EAttack::Bolt;
    const bool bAttack=Phase == EDBEnemyPhase::Attack && Attack == EAttack::Bolt;
    const bool bActive=!bDead && (bTell || (bAttack && ShotsRemaining>0));
    const float Action=bTell ? -TellTime : AttackElapsed;
    const FVector Forward=GetActorForwardVector(), Right=GetActorRightVector();
    const FTransform Chest=OrganicMesh->GetBoneTransformByName(TEXT("chest"),EBoneSpaces::WorldSpace);
    const FTransform Throat=OrganicMesh->GetBoneTransformByName(TEXT("throat"),EBoneSpaces::WorldSpace);
    const FVector Body=FMath::Lerp(Chest.GetLocation(),Throat.GetLocation(),.34f)+Forward*(10.f*VisualScale);
    float TargetHeat=0.f;
    if (bTell) TargetHeat=FMath::Lerp(.12f,1.f,FMath::SmoothStep(-1.35f,-.20f,Action));
    else if (bActive) TargetHeat=.45f+.55f*FMath::SmoothStep(.13f,.40f,Action);
    OrganicFireHeat=FMath::FInterpTo(OrganicFireHeat,TargetHeat,DeltaSeconds,bActive ? 14.f : 9.f);
    if (CoreMaterial)
    {
        CoreMaterial->SetVectorParameterValue(TEXT("HeatCenter"),DBOrganicFire::V(Body));
        CoreMaterial->SetVectorParameterValue(TEXT("HeatForward"),DBOrganicFire::V(Forward/VisualScale));
        CoreMaterial->SetVectorParameterValue(TEXT("HeatRight"),DBOrganicFire::V(Right/VisualScale));
        CoreMaterial->SetVectorParameterValue(TEXT("HeatUp"),DBOrganicFire::V(FVector::UpVector/VisualScale));
        CoreMaterial->SetScalarParameterValue(TEXT("InternalHeat"),OrganicFireHeat);
        CoreMaterial->SetScalarParameterValue(TEXT("FireTime"),VisualTime);
    }
    OrganicFurnaceAudio->SetWorldLocation(Body);
    OrganicIgnitionAudio->SetWorldLocation(Body);
    if (bActive) OrganicFurnaceAudio->SetVolumeMultiplier(.68f*FMath::Lerp(.12f,1.f,OrganicFireHeat));

    // The new visible flame follows the solved hand. ChargePart stays hidden
    // and stationary as the independent IK reference, avoiding pose feedback.
    const bool bFirst=bTell || (bAttack && ShotsRemaining==2);
    const bool bSecond=bAttack && ShotsRemaining==1 && Action>=.17f;
    const float Draw=bFirst ? FMath::SmoothStep(-.94f,-.65f,Action)
        : FMath::SmoothStep(.17f,.32f,Action);
    const FVector Hand=OrganicFireHand(bFirst ? 1 : 0);
    const FVector Charge=FMath::Lerp(Body,Hand,Draw);
    const float Strength=(bFirst || bSecond) ? FMath::SmoothStep(0.f,.35f,Draw) : 0.f;
    const float Size=FMath::Lerp(16.f,32.f,FMath::Sqrt(FMath::Max(0.f,Draw)))*VisualScale;
    DBOrganicFire::Draw(OrganicFireCharge,OrganicFireMaterial,Charge,GetActorRotation(),
        FVector(Size),VisualTime,Strength,0.f,static_cast<float>(GetUniqueID()%71));
    OrganicFireLight->SetWorldLocation(FMath::Lerp(Body+Forward*(14.f*VisualScale),Charge,Strength*.65f));
    OrganicFireLight->SetIntensity((180.f*OrganicFireHeat+380.f*Strength)*(1.f+.09f*FMath::Sin(VisualTime*19.f)));

    for (const FPendingOrganicBolt& Pending : PendingOrganicBolts)
    {
        if (!Target.IsValid() || Target->bDead || bDead) continue;
        const FVector Origin=OrganicFireHand(Pending.HandSide);
        FActorSpawnParameters Params;
        Params.Owner=this;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        if (ADBProjectile* Bolt=GetWorld()->SpawnActor<ADBProjectile>(Origin,Pending.Direction.Rotation(),Params))
            Bolt->Initialize(Pending.Direction,690.f,Pending.Damage,false,this,Pending.Color);
        const int32 Variant=(static_cast<int32>(ActiveAttackId&0x7fffffff)+Pending.HandSide)%3;
        if (OrganicFireReleases.IsValidIndex(Variant) && OrganicFireReleases[Variant])
        {
            UAudioComponent* Release=OrganicReleaseAudio[Pending.HandSide];
            Release->SetWorldLocation(Origin);
            Release->SetSound(OrganicFireReleases[Variant]);
            Release->SetVolumeMultiplier(.82f);
            Release->Play();
        }
        OrganicFireCharge->SetVisibility(false);
    }
    PendingOrganicBolts.Reset();
    if (!bActive && bOrganicFireActive)
    {
        OrganicFurnaceAudio->FadeOut(bAttack ? .035f : .06f,0.f);
        if (!bAttack) OrganicIgnitionAudio->FadeOut(.06f,0.f);
        bOrganicFireActive=false;
    }
}

void ADBEnemy::UpdateOrganicPose()
{
    if (!OrganicMesh->GetSkinnedAsset() || OrganicReferencePose.IsEmpty()) return;
    const FReferenceSkeleton& Skeleton = OrganicMesh->GetSkinnedAsset()->GetRefSkeleton();
    const bool bSlamPose = !bDead && Attack == EAttack::Slam
        && (Phase == EDBEnemyPhase::Attack || Phase == EDBEnemyPhase::Recovery);
    if (bSlamPose && !bOrganicSlamTargetsInitialized)
    {
        const TCHAR* Hands[2] = { TEXT("hand_l"), TEXT("hand_r") };
        const TCHAR* Shoulders[2] = { TEXT("upperarm_l"), TEXT("upperarm_r") };
        const TCHAR* Elbows[2] = { TEXT("forearm_l"), TEXT("forearm_r") };
        for (int32 Side = 0; Side < 2; ++Side)
        {
            // Capture the last visible windup before resetting the local pose.
            OrganicSlamStartHands[Side] = OrganicMesh->GetBoneTransformByName(Hands[Side], EBoneSpaces::WorldSpace);
            const FTransform ShoulderFrame = OrganicMesh->GetBoneTransformByName(Shoulders[Side], EBoneSpaces::WorldSpace);
            const FTransform ElbowFrame = OrganicMesh->GetBoneTransformByName(Elbows[Side], EBoneSpaces::WorldSpace);
            OrganicSlamUpperRotations[Side] = ShoulderFrame.GetRotation();
            OrganicSlamLowerRotations[Side] = ElbowFrame.GetRotation();
            const FVector Shoulder = ShoulderFrame.GetLocation();
            const FVector Elbow = ElbowFrame.GetLocation();
            OrganicSlamStartPoles[Side] = (Elbow - (Shoulder + OrganicSlamStartHands[Side].GetLocation()) * .5f).GetSafeNormal();
        }
    }
    OrganicMesh->BoneSpaceTransforms = OrganicReferencePose;
    // Authored angles are anatomical. Visual yaw inertia belongs to the mesh
    // component and must not rotate these angles back into capsule facing.
    const FQuat MeshToActor = FRotator(0.f, -90.f, 0.f).Quaternion();
    const FQuat ActorToMesh = MeshToActor.Inverse();
    using namespace DBEnemyPerformance;
    const ERole PerformanceRole = Kind == EDBEnemyKind::Caster ? ERole::Caster : Kind == EDBEnemyKind::Hunter ? ERole::Hunter
        : Kind == EDBEnemyKind::Boss ? ERole::Boss : ERole::Melee;
    const FVector MotionVelocity = FRotator(0.f, OrganicFacingYaw, 0.f).UnrotateVector(OrganicVelocity) / FMath::Max(1.f, BaseSpeed);
    auto PlantLoad = [&](int32 Side)
    {
        const float Age = OrganicPerformancePlantTime[Side];
        return FMath::SmoothStep(0.f, .045f, Age) * FMath::Exp(-Age * 9.f);
    };
    FPose Performance = Travel(PerformanceRole, GaitBlend, OrganicPerformanceCycle, OrganicPelvisOffset.Y,
        FMath::Clamp(OrganicTurnRate / 165.f, -1.f, 1.f), MotionVelocity, FVector2D(PlantLoad(0), PlantLoad(1)), VisualTime + OrganicIdlePhase);
    const bool bActionPerformance = !bDead && (Phase == EDBEnemyPhase::Telegraph
        || Phase == EDBEnemyPhase::Attack || Phase == EDBEnemyPhase::Recovery);
    if (bActionPerformance)
    {
        // One authored timeline crosses tell/contact/recovery. The gameplay
        // state still owns its duration, hit windows and projectile releases.
        const float ActionTime = Phase == EDBEnemyPhase::Telegraph ? -TellTime
            : Phase == EDBEnemyPhase::Attack ? AttackElapsed : AttackElapsed + RecoveryDuration - PhaseTime;
        switch (Attack)
        {
        case EAttack::Swing: Performance = Melee(ActionTime); break;
        case EAttack::Lunge: Performance = bOrganicPounceBlocked ? BlockedPounce(OrganicBlockedPounceTime) : Hunter(ActionTime); break;
        case EAttack::Bolt: Performance = Cast(ActionTime, false); break;
        case EAttack::Salvo: Performance = Cast(ActionTime, true); break;
        case EAttack::Intercept: Performance = Cast(ActionTime, true, true); break;
        case EAttack::Slam: Performance = Ground(ActionTime, true); break;
        case EAttack::Ground: Performance = Ground(ActionTime, false); break;
        default: break;
        }
    }
    if (!bDead)
    {
        const float CarriageTarget = Phase == EDBEnemyPhase::Approach || Phase == EDBEnemyPhase::Dormant ? 1.f
            : Phase == EDBEnemyPhase::Recovery ? .25f : .08f;
        const float AttentionTarget = Phase == EDBEnemyPhase::Approach || Phase == EDBEnemyPhase::Dormant ? 1.f
            : Phase == EDBEnemyPhase::Telegraph ? .45f : Phase == EDBEnemyPhase::Recovery ? .55f : .12f;
        const bool bWatchingTarget = Target.IsValid() && !Target->bDead && IsRoomActive()
            && FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation()) < FMath::Square(1800.f);
        // An action-to-recovery handoff must not switch a large held gaze
        // correction on in one frame. Regain attention and carried response
        // continuously while the authored action itself keeps its clock.
        AdvanceOrganicResponse(OrganicAttentionWeights, OrganicAttentionVelocity,
            FVector(CarriageTarget, AttentionTarget, bWatchingTarget ? 1.f : 0.f), 18.f, OrganicPoseDelta);
        const float Carriage = FMath::Clamp(static_cast<float>(OrganicAttentionWeights.X), 0.f, 1.f);
        const float Attention = FMath::Clamp(static_cast<float>(OrganicAttentionWeights.Y), 0.f, 1.f);
        // Acceleration loads the body before a travelling stance settles;
        // braking leaves the shoulders catching up. The actual movement,
        // rather than an independent idle clock, drives these small reactions.
        Performance.Joint[Hips] += FRotator(OrganicMotionLean.X * .25f, 0.f, OrganicMotionLean.Z * .20f) * Carriage;
        Performance.Joint[Lumbar] += FRotator(OrganicMotionLean.X * .25f, OrganicMotionLean.Y * .35f,
            OrganicMotionLean.Z * .25f) * Carriage;
        Performance.Joint[Chest] += FRotator(OrganicMotionLean.X * .50f, OrganicMotionLean.Y * .65f,
            OrganicMotionLean.Z * .55f) * Carriage;
        Performance.Joint[Neck].Pitch -= OrganicMotionLean.X * Carriage * .30f;
        Performance.Joint[Head].Roll -= OrganicMotionLean.Z * Carriage * .25f;

        if (!bOrganicGazeInitialized)
        {
            OrganicGaze = FVector(OrganicFacingYaw, 0.f, 0.f);
            OrganicGazeVelocity = FVector::ZeroVector;
            bOrganicGazeInitialized = true;
        }
        FVector GazeTarget(OrganicGaze.X + FMath::FindDeltaAngleDegrees(static_cast<float>(OrganicGaze.X),
            OrganicFacingYaw), 0.f, 0.f);
        if (bWatchingTarget)
        {
            const int32 HeadIndex = Skeleton.FindBoneIndex(TEXT("head"));
            const FVector HeadOrigin = OrganicReferenceComponentPose.IsValidIndex(HeadIndex)
                ? OrganicMesh->GetComponentTransform().TransformPosition(OrganicReferenceComponentPose[HeadIndex].GetLocation())
                : GetActorLocation();
            const FVector ToTarget = Target->GetPawnViewLocation() - HeadOrigin;
            GazeTarget.X = OrganicGaze.X + FMath::FindDeltaAngleDegrees(static_cast<float>(OrganicGaze.X), ToTarget.Rotation().Yaw);
            GazeTarget.Y = FMath::Clamp(static_cast<float>(ToTarget.Rotation().Pitch), -22.f, 20.f);
        }
        AdvanceOrganicResponse(OrganicGaze, OrganicGazeVelocity, GazeTarget,
            Kind == EDBEnemyKind::Hunter ? 27.f : Kind == EDBEnemyKind::Caster ? 24.f : 22.f, OrganicPoseDelta);
        const float TargetBearing = FMath::Clamp(FMath::FindDeltaAngleDegrees(OrganicFacingYaw,
            static_cast<float>(OrganicGaze.X)), -58.f, 58.f);
        const float InheritedYaw = Performance.Joint[Hips].Yaw + Performance.Joint[Lumbar].Yaw
            + Performance.Joint[Spine].Yaw + Performance.Joint[Chest].Yaw
            + Performance.Joint[Neck].Yaw + Performance.Joint[Head].Yaw;
        const float GazeYaw = FMath::Clamp(TargetBearing - InheritedYaw * .90f
            * static_cast<float>(OrganicAttentionWeights.Z), -64.f, 64.f);
        // Preserve target attention while the shoulders sway and turn beneath
        // it. Smoothing an angle in the moving torso frame made both arrive
        // together and allowed the stride's chest twist to carry the face off.
        Performance.Joint[Neck] += FRotator(OrganicGaze.Y * .60f, GazeYaw * .68f,
            -TargetBearing * .035f) * Attention;
        Performance.Joint[Head] += FRotator(OrganicGaze.Y * .40f, GazeYaw * .32f,
            -TargetBearing * .025f) * Attention;
    }
    if (!bDead && Phase == EDBEnemyPhase::Staggered)
    {
        Performance = Staggered(PerformanceRole, RecoveryDuration - PhaseTime, RecoveryDuration, ReactionLocalDirection);
    }
    if (!bDead && ReactionTime > 0.f)
    {
        const float Age = ReactionDuration - ReactionTime;
        auto Impulse = [&](float Delay)
        {
            const float T = FMath::Max(0.f, Age - Delay);
            return FMath::SmoothStep(0.f, .045f, T) * FMath::Exp(-T * 9.f) * ReactionStrength;
        };
        const float BodyHit = Impulse(0.f), HeadHit = Impulse(.035f);
        Performance.Offset += ReactionLocalDirection * BodyHit * 8.f;
        Performance.Joint[Hips] += FRotator(-ReactionLocalDirection.X * BodyHit * 9.f, 0.f, ReactionLocalDirection.Y * BodyHit * 6.f);
        Performance.Joint[Lumbar] += FRotator(-ReactionLocalDirection.X * BodyHit * 6.f, ReactionLocalDirection.Y * BodyHit * 5.f, 0.f);
        Performance.Joint[Chest] += FRotator(-ReactionLocalDirection.X * BodyHit * 13.f, ReactionLocalDirection.Y * BodyHit * 10.f, ReactionLocalDirection.Y * BodyHit * 8.f);
        Performance.Joint[Neck].Pitch += ReactionLocalDirection.X * HeadHit * 8.f;
        Performance.Joint[Head].Pitch += ReactionLocalDirection.X * HeadHit * 12.f;
        Performance.Joint[ClavicleL].Roll -= BodyHit * 5.f;
        Performance.Joint[ClavicleR].Roll += BodyHit * 5.f;
        Performance.Joint[ForearmL].Pitch -= BodyHit * 11.f;
        Performance.Joint[ForearmR].Pitch -= BodyHit * 15.f;
    }
    auto PoseRotation = [&](const TCHAR* Name, const FQuat& Rotation, const FVector& Offset)
    {
        const int32* Cached = OrganicBoneIndices.Find(FName(Name));
        const int32 Index = Cached ? *Cached : INDEX_NONE;
        if (!OrganicReferencePose.IsValidIndex(Index)) return;
        const int32 Parent = Skeleton.GetParentIndex(Index);
        const FTransform ParentFrame = OrganicReferenceComponentPose.IsValidIndex(Parent)
            ? OrganicReferenceComponentPose[Parent] : FTransform::Identity;
        const FQuat ParentBasis = ParentFrame.GetRotation();
        const FQuat MeshDelta = ActorToMesh * Rotation * MeshToActor;
        FTransform& Bone = OrganicMesh->BoneSpaceTransforms[Index];
        Bone.SetRotation((ParentBasis.Inverse() * MeshDelta * ParentBasis * Bone.GetRotation()).GetNormalized());
        // Motion offsets are centimetres in mesh space. The imported FBX root
        // can carry the metres-to-centimetres scale: inverse rotation alone
        // multiplied a sub-centimetre breath into a visible rise/sink.
        Bone.AddToTranslation(ParentFrame.InverseTransformVector(ActorToMesh.RotateVector(Offset)));
    };
    auto Pose = [&](const TCHAR* Name, FRotator Rotation, FVector Offset = FVector::ZeroVector)
    {
        PoseRotation(Name, Rotation.Quaternion(), Offset);
    };
    auto Anatomy = [&](const TCHAR* Name, const FVector& BlenderDegrees, float Channel)
    {
        const FVector Angles = BlenderDegrees * FMath::Clamp(Channel, 0.f, 1.f);
        // The source contract uses mesh-axis X, then Y, then Z rotations.
        // Map axial handedness explicitly; a bone's arbitrary local roll is
        // never treated as the jaw/digit hinge. Keep this order for the frill.
        const FQuat X = FRotator(-Angles.X, 0.f, 0.f).Quaternion();
        const FQuat Y = FRotator(0.f, 0.f, -Angles.Y).Quaternion();
        const FQuat Z = FRotator(0.f, -Angles.Z, 0.f).Quaternion();
        PoseRotation(Name, (Z * Y * X).GetNormalized(), FVector::ZeroVector);
    };
    // The old scene pivots remain the authoritative effect/ShotOrigin
    // controller. Skin performance is independent so a larger visual coil
    // cannot silently move the caster's existing projectile spawn or LOS ray.
    FVector PelvisOffset = OrganicPelvisOffset + Performance.Offset;
    if (bDead)
    {
        Performance = Dying(PerformanceRole, DeathTime, DeathLocalDirection);
        const float Fall = FallProgress(DeathTime);
        const int32 PelvisIndex = Skeleton.FindBoneIndex(TEXT("pelvis"));
        if (OrganicReferenceComponentPose.IsValidIndex(PelvisIndex))
        {
            const FTransform GroundFrame = OrganicMesh->GetComponentTransform();
            // Start at the actual struck hip. It first loses lateral support,
            // then falls into a traced contact instead of lowering fifty
            // centimetres into a symmetric squat before the torso tips.
            const float Yield = FMath::SmoothStep(0.f, .16f, DeathTime);
            const FVector Across = OrganicDeathPelvisContact - OrganicDeathPelvisStart;
            FVector PelvisWorld = OrganicDeathPelvisStart
                + FVector(Across.X, Across.Y, 0.f) * (Yield * .12f + Fall * .88f);
            const float FailedSide = DeathLocalDirection.Y >= 0.f ? 1.f : -1.f;
            PelvisWorld += FRotator(0.f,OrganicFacingYaw,0.f).RotateVector(FVector(0.f,FailedSide * 6.f,0.f))
                * VisualScale * Yield * (1.f - Fall);
            // First lose height over one buckling knee. The remaining support
            // briefly catches the weight before the trunk falls beyond it.
            PelvisWorld.Z = FMath::Lerp(OrganicDeathPelvisStart.Z, OrganicDeathPelvisContact.Z, .24f * Yield + .76f * Fall);
            const FVector LocalPelvis = GroundFrame.InverseTransformPosition(PelvisWorld);
            PelvisOffset = MeshToActor.RotateVector(LocalPelvis - OrganicReferenceComponentPose[PelvisIndex].GetLocation());
        }
    }
    const bool bArticulatedTorso = OrganicBoneIndices.Contains(TEXT("spine_lower"))
        && OrganicBoneIndices.Contains(TEXT("chest")) && OrganicBoneIndices.Contains(TEXT("neck"));
    Pose(TEXT("pelvis"), Performance.Joint[Hips], PelvisOffset);
    Pose(TEXT("spine_lower"), Performance.Joint[Lumbar]);
    Pose(TEXT("spine"), Performance.Joint[Spine] + (bArticulatedTorso ? FRotator::ZeroRotator
        : Performance.Joint[Lumbar] + Performance.Joint[Chest]));
    Pose(TEXT("chest"), Performance.Joint[Chest]);
    Pose(TEXT("neck"), Performance.Joint[Neck]);
    Pose(TEXT("head"), Performance.Joint[Head] + (bArticulatedTorso ? FRotator::ZeroRotator : Performance.Joint[Neck]));
    Pose(TEXT("clavicle_l"), Performance.Joint[ClavicleL]);
    Pose(TEXT("clavicle_r"), Performance.Joint[ClavicleR]);
    Pose(TEXT("upperarm_l"), Performance.Joint[ArmL] + (bArticulatedTorso ? FRotator::ZeroRotator : Performance.Joint[ClavicleL]));
    Pose(TEXT("upperarm_r"), Performance.Joint[ArmR] + (bArticulatedTorso ? FRotator::ZeroRotator : Performance.Joint[ClavicleR]));
    Pose(TEXT("forearm_l"), Performance.Joint[ForearmL]);
    Pose(TEXT("forearm_r"), Performance.Joint[ForearmR]);
    Pose(TEXT("hand_l"), Performance.Joint[HandL]);
    Pose(TEXT("hand_r"), Performance.Joint[HandR]);
    if (OrganicBoneIndices.Contains(TEXT("jaw")))
    {
        const bool bMire = Kind == EDBEnemyKind::Caster;
        Anatomy(TEXT("jaw"), FVector(bMire ? 14.f : 18.f, 0.f, 0.f), Performance.Jaw);
        static const TCHAR* DigitNames[2][4] = {
            { TEXT("digit_01_l"), TEXT("digit_02_l"), TEXT("digit_03_l"), TEXT("thumb_l") },
            { TEXT("digit_01_r"), TEXT("digit_02_r"), TEXT("digit_03_r"), TEXT("thumb_r") }
        };
        static const TCHAR* TipNames[2][4] = {
            { TEXT("digit_01_tip_l"), TEXT("digit_02_tip_l"), TEXT("digit_03_tip_l"), TEXT("thumb_tip_l") },
            { TEXT("digit_01_tip_r"), TEXT("digit_02_tip_r"), TEXT("digit_03_tip_r"), TEXT("thumb_tip_r") }
        };
        const FVector MireDigits[4] = { FVector(-28,0,0), FVector(-25,0,0), FVector(-18,-8,0), FVector(-12,-24,0) };
        const FVector MireTips[4] = { FVector(-18,0,0), FVector(-18,0,0), FVector(-14,0,0), FVector(0,-15,0) };
        const float BriarDigits[4] = { 23.f,25.f,21.f,-20.f };
        const float BriarTips[4] = { 18.f,19.f,16.f,-15.f };
        for (int32 Side = 0; Side < 2; ++Side)
        {
            const float Mirror = Side == 0 ? 1.f : -1.f;
            for (int32 Digit = 0; Digit < 4; ++Digit)
            {
                const FVector Proximal = bMire ? MireDigits[Digit] * FVector(1.f,Mirror,Mirror)
                    : FVector(0.f,BriarDigits[Digit] * Mirror,0.f);
                const FVector Distal = bMire ? MireTips[Digit] * FVector(1.f,Mirror,Mirror)
                    : FVector(0.f,BriarTips[Digit] * Mirror,0.f);
                Anatomy(DigitNames[Side][Digit], Proximal, Performance.Grip[Side]);
                Anatomy(TipNames[Side][Digit], Distal, Performance.Grip[Side]);
            }
        }
        if (bMire)
        {
            const float TurnLoad = FMath::Clamp(OrganicTurnRate * .0005f, -.09f, .09f);
            const FVector CrestTarget(FMath::Clamp(Performance.Crest + TurnLoad, 0.f, 1.f),
                FMath::Clamp(Performance.Crest - TurnLoad, 0.f, 1.f), 0.f);
            AdvanceOrganicResponse(OrganicCrestMotion, OrganicCrestVelocity, CrestTarget, 17.f, OrganicPoseDelta);
            Anatomy(TEXT("frill_l"), FVector(0,-8,14), Performance.Crest);
            Anatomy(TEXT("frill_r"), FVector(0,8,-14), Performance.Crest);
            // Membrane tips catch up with tension in the base, and load a
            // little differently across a turn. The bony crown stays rigid.
            Anatomy(TEXT("frill_tip_l"), FVector(0,5,-8), OrganicCrestMotion.X);
            Anatomy(TEXT("frill_tip_r"), FVector(0,-5,8), OrganicCrestMotion.Y);
        }
    }
    if (const int32* Throat = OrganicBoneIndices.Find(TEXT("throat")))
        OrganicMesh->BoneSpaceTransforms[*Throat].SetScale3D(OrganicReferencePose[*Throat].GetScale3D()
            * FVector(1.f + Performance.Throat * .08f, 1.f + Performance.Throat * .015f, 1.f + Performance.Throat * .08f));
    if (bDead)
    {
        // Preserve the complete struck pose, including wrists and lifted feet,
        // through the first part of the collapse rather than resetting to bind.
        if (OrganicDeathStartPose.Num() == OrganicReferencePose.Num())
        {
            const float Blend = FMath::SmoothStep(0.f, .16f, DeathTime);
            for (int32 Index = 0; Index < OrganicReferencePose.Num(); ++Index)
            {
                FTransform Blended;
                Blended.Blend(OrganicDeathStartPose[Index], OrganicMesh->BoneSpaceTransforms[Index], Blend);
                OrganicMesh->BoneSpaceTransforms[Index] = Blended;
            }
        }
    }
    else if (!bOrganicGrounded)
    {
        // The hunter covers its existing lunge distance in a short leap. Stance
        // locks release for that move; tucked legs extend again before landing.
        const float Tuck = Attack == EAttack::Lunge && Phase == EDBEnemyPhase::Attack
            ? FMath::Sin(FMath::Clamp(AttackElapsed / .43f, 0.f, 1.f) * PI) : .18f;
        Pose(TEXT("thigh_l"), FRotator(-Tuck * 44.f, 0.f, -Tuck * 5.f));
        Pose(TEXT("thigh_r"), FRotator(-Tuck * 35.f, 0.f, Tuck * 5.f));
        Pose(TEXT("shin_l"), FRotator(Tuck * 76.f, 0.f, 0.f));
        Pose(TEXT("shin_r"), FRotator(Tuck * 68.f, 0.f, 0.f));
        Pose(TEXT("foot_l"), FRotator(-Tuck * 24.f, 0.f, 0.f));
        Pose(TEXT("foot_r"), FRotator(-Tuck * 21.f, 0.f, 0.f));
    }
    if (!bDead && OrganicPhaseStartBonePose.Num() == OrganicReferencePose.Num())
    {
        // Enter from the actual solved performance, including every new torso
        // and shoulder joint. Authored action phases share a continuous clock;
        // blending those phases again would erase the fast contact stroke.
        const float Duration = bOrganicPounceBlocked && (Phase == EDBEnemyPhase::Attack || Phase == EDBEnemyPhase::Recovery) ? .10f
            : Phase == EDBEnemyPhase::Staggered ? .09f
            : Phase == EDBEnemyPhase::Telegraph ? .14f : Phase == EDBEnemyPhase::Approach ? .20f : 0.f;
        if (Duration > 0.f && OrganicPhaseBlendTime < Duration)
        {
            const float Release = FMath::SmoothStep(0.f, Duration, OrganicPhaseBlendTime);
            for (int32 Index = 0; Index < OrganicReferencePose.Num(); ++Index)
            {
                FTransform Blended;
                Blended.Blend(OrganicPhaseStartBonePose[Index], OrganicMesh->BoneSpaceTransforms[Index], Release);
                OrganicMesh->BoneSpaceTransforms[Index] = Blended;
            }
        }
    }

    TArray<FTransform, TInlineAllocator<48>> Components;
    Components.SetNum(OrganicReferencePose.Num());
    auto RebuildComponents = [&]()
    {
        for (int32 Index = 0; Index < Components.Num(); ++Index)
        {
            const int32 Parent = Skeleton.GetParentIndex(Index);
            Components[Index] = Parent == INDEX_NONE ? OrganicMesh->BoneSpaceTransforms[Index]
                : OrganicMesh->BoneSpaceTransforms[Index] * Components[Parent];
        }
    };
    auto SetComponentBone = [&](int32 Index, const FTransform& Desired)
    {
        const int32 Parent = Skeleton.GetParentIndex(Index);
        OrganicMesh->BoneSpaceTransforms[Index] = Parent == INDEX_NONE ? Desired
            : Desired.GetRelativeTransform(Components[Parent]);
        OrganicMesh->BoneSpaceTransforms[Index].NormalizeRotation();
        RebuildComponents();
    };
    RebuildComponents();
    const FTransform MeshFrame = OrganicMesh->GetComponentTransform();
    const FRotator Facing(0.f, OrganicFacingYaw, 0.f);
    const FVector Forward = Facing.Vector();
    const FVector Right = Facing.RotateVector(FVector::RightVector);
    auto SolveLimb = [&](const TCHAR* UpperName, const TCHAR* LowerName, const TCHAR* EndName,
        FVector WorldTarget, FQuat WorldRotation, FVector PoleDirection, float Weight, bool bLockEndRotation)
    {
        const int32 Upper = Skeleton.FindBoneIndex(FName(UpperName));
        const int32 Lower = Skeleton.FindBoneIndex(FName(LowerName));
        const int32 End = Skeleton.FindBoneIndex(FName(EndName));
        if (!Components.IsValidIndex(Upper) || !Components.IsValidIndex(Lower) || !Components.IsValidIndex(End) || Weight <= 0.f) return;
        const FTransform OldUpper = Components[Upper], OldLower = Components[Lower], OldEnd = Components[End];
        const FVector Root = OldUpper.GetLocation();
        const FVector TargetPosition = FMath::Lerp(OldEnd.GetLocation(), MeshFrame.InverseTransformPosition(WorldTarget), Weight);
        const float UpperLength = FVector::Dist(OrganicReferenceComponentPose[Upper].GetLocation(), OrganicReferenceComponentPose[Lower].GetLocation());
        const float LowerLength = FVector::Dist(OrganicReferenceComponentPose[Lower].GetLocation(), OrganicReferenceComponentPose[End].GetLocation());
        if (UpperLength < .01f || LowerLength < .01f) return;
        FVector Direction = (TargetPosition - Root).GetSafeNormal();
        if (Direction.IsNearlyZero()) Direction = FVector::DownVector;
        const float Distance = FMath::Clamp(static_cast<float>(FVector::Dist(Root, TargetPosition)),
            FMath::Abs(UpperLength - LowerLength) + .1f, (UpperLength + LowerLength) * .997f);
        FVector Bend = MeshFrame.InverseTransformVectorNoScale(PoleDirection);
        Bend = (Bend - Direction * FVector::DotProduct(Bend, Direction)).GetSafeNormal();
        if (Bend.IsNearlyZero())
        {
            Bend = FVector::CrossProduct(Direction, FVector::RightVector).GetSafeNormal();
            if (Bend.IsNearlyZero()) Bend = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal();
        }
        const float Along = (UpperLength * UpperLength - LowerLength * LowerLength + Distance * Distance) / (2.f * Distance);
        const float Away = FMath::Sqrt(FMath::Max(0.f, UpperLength * UpperLength - Along * Along));
        const FVector Joint = Root + Direction * Along + Bend * Away;
        const FVector Tip = Root + Direction * Distance;
        const FQuat UpperDelta = FQuat::FindBetweenNormals((OldLower.GetLocation() - Root).GetSafeNormal(), (Joint - Root).GetSafeNormal());
        const FQuat LowerDelta = FQuat::FindBetweenNormals((OldEnd.GetLocation() - OldLower.GetLocation()).GetSafeNormal(), (Tip - Joint).GetSafeNormal());
        FTransform NewUpper = OldUpper;
        NewUpper.SetRotation((UpperDelta * OldUpper.GetRotation()).GetNormalized());
        SetComponentBone(Upper, NewUpper);
        FTransform NewLower = OldLower;
        NewLower.SetLocation(Joint);
        NewLower.SetRotation((LowerDelta * OldLower.GetRotation()).GetNormalized());
        SetComponentBone(Lower, NewLower);
        FTransform NewEnd = OldEnd;
        NewEnd.SetLocation(Tip);
        NewEnd.SetRotation(bLockEndRotation
            ? FQuat::Slerp(OldEnd.GetRotation(), MeshFrame.InverseTransformRotation(WorldRotation), Weight).GetNormalized()
            : (LowerDelta * OldEnd.GetRotation()).GetNormalized());
        SetComponentBone(End, NewEnd);
    };

    if (bOrganicGrounded || bDead)
    {
        const TCHAR* UpperNames[2] = { TEXT("thigh_l"), TEXT("thigh_r") };
        const TCHAR* LowerNames[2] = { TEXT("shin_l"), TEXT("shin_r") };
        const TCHAR* FootNames[2] = { TEXT("foot_l"), TEXT("foot_r") };
        FVector Targets[2];
        FQuat Rotations[2];
        float ExtraDrop = 0.f;
        for (int32 Side = 0; Side < 2; ++Side)
        {
            const bool bFailedSupport = (DeathLocalDirection.Y >= 0.f ? 1 : 0) == Side;
            const float Landing = bFailedSupport ? FMath::SmoothStep(.04f, .29f, DeathTime)
                : FMath::SmoothStep(.24f, .55f, DeathTime);
            Targets[Side] = bDead ? FMath::Lerp(OrganicFeet[Side].Position, DeathFootTargets[Side], Landing) : OrganicFeet[Side].Position;
            Rotations[Side] = bDead ? FQuat::Slerp(OrganicFeet[Side].Rotation, DeathFootRotations[Side], Landing) : OrganicFeet[Side].Rotation;
            const int32 Upper = Skeleton.FindBoneIndex(UpperNames[Side]);
            const int32 Lower = Skeleton.FindBoneIndex(LowerNames[Side]);
            const int32 Foot = Skeleton.FindBoneIndex(FootNames[Side]);
            if (!Components.IsValidIndex(Upper) || !Components.IsValidIndex(Lower) || !Components.IsValidIndex(Foot)) continue;
            const float Length = (FVector::Dist(OrganicReferenceComponentPose[Upper].GetLocation(), OrganicReferenceComponentPose[Lower].GetLocation())
                + FVector::Dist(OrganicReferenceComponentPose[Lower].GetLocation(), OrganicReferenceComponentPose[Foot].GetLocation()))
                * MeshFrame.GetScale3D().GetAbsMax() * .992f;
            const FVector Hip = MeshFrame.TransformPosition(Components[Upper].GetLocation());
            const float Horizontal = FVector::Dist2D(Hip, Targets[Side]);
            const float Vertical = FMath::Sqrt(FMath::Max(0.f, Length * Length - Horizontal * Horizontal));
            ExtraDrop = FMath::Max(ExtraDrop, Hip.Z - Targets[Side].Z - Vertical);
        }
        // The phase blend also blends the pelvis. Correct any resulting reach
        // excess before IK so a planted ankle never gets pulled toward the hip.
        const int32 Pelvis = Skeleton.FindBoneIndex(TEXT("pelvis"));
        if (!bDead && ExtraDrop > 0.f && Components.IsValidIndex(Pelvis))
        {
            FTransform Supported = Components[Pelvis];
            Supported.AddToTranslation(MeshFrame.InverseTransformVector(FVector(0.f, 0.f, -FMath::Min(ExtraDrop, 35.f * VisualScale))));
            SetComponentBone(Pelvis, Supported);
        }
        for (int32 Side = 0; Side < 2; ++Side)
        {
            const float Sign = Side == 0 ? -1.f : 1.f;
            const bool bFailedSupport = bDead && (DeathLocalDirection.Y >= 0.f ? 1 : 0) == Side;
            const float KneeGive = bFailedSupport ? FMath::SmoothStep(0.f, .16f, DeathTime) : 0.f;
            SolveLimb(UpperNames[Side], LowerNames[Side], FootNames[Side], Targets[Side], Rotations[Side],
                Forward + Right * Sign * (.14f + KneeGive * .85f), 1.f, true);
        }
    }
    else if (Phase == EDBEnemyPhase::Attack && Attack == EAttack::Lunge)
    {
        // Reach under the body before touchdown. The small remaining sole
        // clearance belongs to the landing handoff, so an early collision
        // does not force a tucked air pose directly onto standing anchors.
        const float Reach = FMath::SmoothStep(.24f, .42f, AttackElapsed);
        const TCHAR* UpperNames[2] = { TEXT("thigh_l"), TEXT("thigh_r") };
        const TCHAR* LowerNames[2] = { TEXT("shin_l"), TEXT("shin_r") };
        const TCHAR* FootNames[2] = { TEXT("foot_l"), TEXT("foot_r") };
        for (int32 Side = 0; Side < 2; ++Side)
        {
            const int32 Foot = Skeleton.FindBoneIndex(FootNames[Side]);
            if (!OrganicReferenceComponentPose.IsValidIndex(Foot)) continue;
            FVector Landing, Normal;
            TraceOrganicFoot(MeshFrame.TransformPosition(OrganicReferenceComponentPose[Foot].GetLocation()),
                OrganicFeet[Side].AnkleHeight, Landing, Normal);
            Landing.Z += (4.f + (1.f - Reach) * 12.f) * VisualScale;
            const FQuat Orientation = FQuat::FindBetweenNormals(FVector::UpVector, Normal)
                * MeshFrame.GetRotation() * OrganicReferenceComponentPose[Foot].GetRotation();
            SolveLimb(UpperNames[Side], LowerNames[Side], FootNames[Side], Landing, Orientation,
                Forward + Right * (Side == 0 ? -.14f : .14f), Reach, true);
        }
    }

    if (bSlamPose)
    {
        const TCHAR* UpperNames[2] = { TEXT("upperarm_l"), TEXT("upperarm_r") };
        const TCHAR* LowerNames[2] = { TEXT("forearm_l"), TEXT("forearm_r") };
        const TCHAR* HandNames[2] = { TEXT("hand_l"), TEXT("hand_r") };
        const FVector AimForward = GetActorForwardVector();
        const FVector AimRight = GetActorRightVector();
        if (!bOrganicSlamTargetsInitialized)
        {
            // These are the two lowest fully hand-weighted Briarhide skin
            // vertices in this contact orientation, relative to each wrist
            // in reference actor-space centimetres. Subtract the transformed
            // skin offset from the traced surface, rather than treating a
            // wrist bone as a fingertip. FBX source analysis: left vertex
            // 371712, right 35784; both have hand weight 1.0.
            for (int32 Side = 0; Side < 2; ++Side)
            {
                const float Sign = Side == 0 ? -1.f : 1.f;
                FVector Contact, Normal;
                TraceOrganicFoot(FeetLocation() + (AimForward * 72.f + AimRight * Sign * 36.f) * VisualScale,
                    0.f, Contact, Normal);
                const int32 Hand = Skeleton.FindBoneIndex(HandNames[Side]);
                if (!OrganicReferenceComponentPose.IsValidIndex(Hand)) continue;
                const FQuat ContactDelta = FQuat::FindBetweenNormals(FVector::UpVector, Normal)
                    * FQuat(AimRight, FMath::DegreesToRadians(-28.f));
                const FVector SurfaceOffset = ContactDelta.RotateVector(MeshFrame.TransformVector(
                    ActorToMesh.RotateVector(BriarhideClawOffsets[Side])));
                OrganicSlamHandTargets[Side] = Contact - SurfaceOffset;
                OrganicSlamHandRotations[Side] = ContactDelta * MeshFrame.GetRotation()
                    * OrganicReferenceComponentPose[Hand].GetRotation();
            }
            bOrganicSlamTargetsInitialized = true;
        }
        for (int32 Side = 0; Side < 2; ++Side)
        {
            const int32 Upper = Skeleton.FindBoneIndex(UpperNames[Side]);
            const int32 Lower = Skeleton.FindBoneIndex(LowerNames[Side]);
            const int32 Hand = Skeleton.FindBoneIndex(HandNames[Side]);
            if (!Components.IsValidIndex(Upper) || !Components.IsValidIndex(Lower) || !Components.IsValidIndex(Hand)) continue;
            const FVector FreeHand = MeshFrame.TransformPosition(Components[Hand].GetLocation());
            const FVector FreePole = MeshFrame.TransformVectorNoScale(Components[Lower].GetLocation()
                - (Components[Upper].GetLocation() + Components[Hand].GetLocation()) * .5f).GetSafeNormal();
            const FQuat FreeUpper = MeshFrame.TransformRotation(Components[Upper].GetRotation());
            const FQuat FreeLower = MeshFrame.TransformRotation(Components[Lower].GetRotation());
            const float RestoreArm = Phase == EDBEnemyPhase::Recovery
                ? FMath::SmoothStep(.42f, .9f, RecoveryDuration - PhaseTime) : 0.f;
            // A fixed wrist and bend pole do not constrain axial arm twist.
            // Transport from the last solved arm, not a changing hidden FK
            // windup/recovery pose. Keep current shoulder translations and the
            // full bone transforms so this never changes the imported scale.
            FTransform ContinuousUpper = Components[Upper];
            ContinuousUpper.SetRotation(MeshFrame.InverseTransformRotation(FQuat::Slerp(
                OrganicSlamUpperRotations[Side], FreeUpper, RestoreArm).GetNormalized()));
            SetComponentBone(Upper, ContinuousUpper);
            FTransform ContinuousLower = Components[Lower];
            ContinuousLower.SetRotation(MeshFrame.InverseTransformRotation(FQuat::Slerp(
                OrganicSlamLowerRotations[Side], FreeLower, RestoreArm).GetNormalized()));
            SetComponentBone(Lower, ContinuousLower);
            const FVector RestingPole = (Right * (Side == 0 ? -1.f : 1.f) + FVector::UpVector * .35f).GetSafeNormal();
            FVector Pole = RestingPole;
            float PlantRotation = 1.f;
            if (Phase == EDBEnemyPhase::Attack)
            {
                // A continuous world-space arc clears the crown and reaches
                // the measured skin contacts at the unchanged .16 s impact.
                // IK owns the entire descent, including its first update.
                const float Stroke = FMath::Clamp(AttackElapsed / .16f, 0.f, 1.f);
                const float Remain = 1.f - Stroke;
                const FVector Start = OrganicSlamStartHands[Side].GetLocation();
                const FVector End = OrganicSlamHandTargets[Side];
                const FVector ControlA = Start + (AimForward * 70.f + FVector::UpVector * 12.f) * VisualScale;
                // The ground stops this stroke. A high second control point
                // kept the claws near the crown until the final update, then
                // dropped them 104cm in 33ms. Arrive with a zero end tangent.
                const FVector ControlB = End;
                OrganicSlamRequestedHands[Side] = Start * Remain * Remain * Remain
                    + ControlA * (3.f * Remain * Remain * Stroke)
                    + ControlB * (3.f * Remain * Stroke * Stroke) + End * Stroke * Stroke * Stroke;
                Pole = FMath::Lerp(OrganicSlamStartPoles[Side], RestingPole,
                    FMath::SmoothStep(0.f, .6f, Stroke)).GetSafeNormal();
                PlantRotation = FMath::SmoothStep(.4f, 1.f, Stroke);
            }
            else
            {
                const float Hold = 1.f - FMath::SmoothStep(.12f, .52f, RecoveryDuration - PhaseTime);
                OrganicSlamRequestedHands[Side] = FMath::Lerp(FreeHand, OrganicSlamHandTargets[Side], Hold);
                Pole = FMath::Lerp(RestingPole, FreePole, RestoreArm).GetSafeNormal();
                PlantRotation = Hold;
            }
            SolveLimb(UpperNames[Side], LowerNames[Side], HandNames[Side], OrganicSlamRequestedHands[Side],
                FQuat::Identity, Pole, 1.f, false);
            // Let each palm follow its forearm during the swing, then lay its
            // claws onto the traced normal. This avoids counter-rotating the
            // whole hand against the elbow through the middle of the arc.
            FTransform PlantedHand = Components[Hand];
            PlantedHand.SetRotation(FQuat::Slerp(PlantedHand.GetRotation(),
                MeshFrame.InverseTransformRotation(OrganicSlamHandRotations[Side]), PlantRotation).GetNormalized());
            SetComponentBone(Hand, PlantedHand);
            OrganicSlamUpperRotations[Side] = MeshFrame.TransformRotation(Components[Upper].GetRotation());
            OrganicSlamLowerRotations[Side] = MeshFrame.TransformRotation(Components[Lower].GetRotation());
        }
    }
    else if (!bDead && (!bActionPerformance || Attack == EAttack::Swing || Attack == EAttack::Lunge || Attack == EAttack::Ground))
    {
        const TCHAR* UpperNames[2] = { TEXT("upperarm_l"), TEXT("upperarm_r") };
        const TCHAR* LowerNames[2] = { TEXT("forearm_l"), TEXT("forearm_r") };
        const TCHAR* HandNames[2] = { TEXT("hand_l"), TEXT("hand_r") };
        const FVector AimForward = !bActionPerformance ? Forward : GetActorForwardVector();
        const FVector AimRight = FVector::CrossProduct(FVector::UpVector, AimForward).GetSafeNormal();
        for (int32 Side = 0; Side < 2; ++Side)
        {
            const int32 Upper = Skeleton.FindBoneIndex(UpperNames[Side]);
            const int32 Lower = Skeleton.FindBoneIndex(LowerNames[Side]);
            const int32 Hand = Skeleton.FindBoneIndex(HandNames[Side]);
            if (!Components.IsValidIndex(Upper) || !Components.IsValidIndex(Lower) || !Components.IsValidIndex(Hand)) continue;
            const float Reach = (FVector::Dist(OrganicReferenceComponentPose[Upper].GetLocation(), OrganicReferenceComponentPose[Lower].GetLocation())
                + FVector::Dist(OrganicReferenceComponentPose[Lower].GetLocation(), OrganicReferenceComponentPose[Hand].GetLocation()))
                * VisualScale * .96f;
            const FVector Local = Performance.HandTarget[Side];
            const FVector Offset = ((AimForward * Local.X + AimRight * Local.Y + FVector::UpVector * Local.Z) * VisualScale).GetClampedToMaxSize(Reach);
            const FVector Wrist = MeshFrame.TransformPosition(Components[Upper].GetLocation()) + Offset;
            const float Sign = Side == 0 ? -1.f : 1.f;
            // The outgoing solved arm survives an interruption. Release into
            // the new brace/carry over the same full-pose transition rather
            // than snapping its IK target on the first stunned update.
            const float EntryBlend = Phase == EDBEnemyPhase::Staggered
                ? FMath::SmoothStep(0.f, .11f, OrganicPhaseBlendTime)
                : Phase == EDBEnemyPhase::Approach ? FMath::SmoothStep(0.f, .20f, OrganicPhaseBlendTime) : 1.f;
            SolveLimb(UpperNames[Side], LowerNames[Side], HandNames[Side], Wrist, FQuat::Identity,
                AimRight * Sign + AimForward * .15f - FVector::UpVector * .50f, Performance.HandWeight[Side] * EntryBlend, false);
        }
    }
    else if (!bDead && (Kind == EDBEnemyKind::Caster || Kind == EDBEnemyKind::Boss)
        && (Attack == EAttack::Bolt || Attack == EAttack::Salvo || Attack == EAttack::Intercept))
    {
        const FVector AimForward = GetActorForwardVector();
        const FVector AimRight = GetActorRightVector();
        const bool bGuard = Attack == EAttack::Intercept;
        const FVector Charge = bGuard ? GetActorLocation() + AimForward
            * (GetCapsuleComponent()->GetScaledCapsuleRadius() + 22.f) + FVector(0.f, 0.f, 25.f)
            : ChargePart->GetComponentLocation();
        const TCHAR* UpperNames[2] = { TEXT("upperarm_l"), TEXT("upperarm_r") };
        const TCHAR* LowerNames[2] = { TEXT("forearm_l"), TEXT("forearm_r") };
        const TCHAR* HandNames[2] = { TEXT("hand_l"), TEXT("hand_r") };
        for (int32 Side = 0; Side < 2; ++Side)
        {
            const float Sign = Side == 0 ? -1.f : 1.f;
            // The two hands gather and discharge on distinct authored paths;
            // their reference remains the unchanged real projectile origin.
            const FVector Local = Performance.HandTarget[Side];
            const FVector Wrist = Charge + (AimForward * Local.X + AimRight * Local.Y + FVector::UpVector * Local.Z) * VisualScale;
            SolveLimb(UpperNames[Side], LowerNames[Side], HandNames[Side], Wrist, FQuat::Identity,
                Right * Sign + FVector(0.f, 0.f, -.55f), Performance.HandWeight[Side], false);
        }
    }
    else if (bDead)
    {
        const TCHAR* UpperNames[2] = { TEXT("upperarm_l"), TEXT("upperarm_r") };
        const TCHAR* LowerNames[2] = { TEXT("forearm_l"), TEXT("forearm_r") };
        const TCHAR* HandNames[2] = { TEXT("hand_l"), TEXT("hand_r") };
        for (int32 Side = 0; Side < 2; ++Side)
        {
            const int32 Hand = Skeleton.FindBoneIndex(HandNames[Side]);
            if (!Components.IsValidIndex(Hand)) continue;
            const bool bCatchSide = (DeathLocalDirection.Y >= 0.f ? 1 : 0) == Side;
            const float ReachStart = bCatchSide ? .25f : .57f;
            const float ContactTime = bCatchSide ? .51f : .93f;
            const float Contact = FMath::SmoothStep(ReachStart, ContactTime, DeathTime);
            if (DeathTime < ReachStart) continue;
            if (!bOrganicDeathHandContact[Side])
            {
                FVector GroundNormal;
                TraceOrganicFoot(MeshFrame.TransformPosition(Components[Hand].GetLocation()),
                    (Kind == EDBEnemyKind::Caster ? 7.f : 11.f) * VisualScale, OrganicDeathHandContacts[Side], GroundNormal);
                bOrganicDeathHandContact[Side] = true;
            }
            // The catching arm takes weight first. The other arm arrives after
            // torso contact; once a palm lands its world contact stays fixed.
            SolveLimb(UpperNames[Side], LowerNames[Side], HandNames[Side], OrganicDeathHandContacts[Side], FQuat::Identity,
                Right * (Side == 0 ? -1.f : 1.f) + FVector::UpVector * .35f, Contact, false);
        }
    }
    OrganicMesh->RefreshBoneTransforms();
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

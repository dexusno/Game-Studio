#include "FoundryRobot.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogFoundryRobot, Log, All);

AFoundryRobot::AFoundryRobot()
{
    PrimaryActorTick.bCanEverTick = true;
    Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CinderwallRobot"));
    SetRootComponent(Body);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    FlashMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttachedCosmeticCue"));
    FlashMesh->SetupAttachment(Body);
    FlashMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FlashMesh->SetCastShadow(false);
    FlashMesh->SetVisibility(false);
    FlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("AttachedCosmeticLight"));
    FlashLight->SetupAttachment(Body);
    FlashLight->SetIntensity(0);
    FlashLight->SetAttenuationRadius(280);
    FlashLight->SetCastShadows(false);
    SteamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttachedSteamCue"));
    SteamMesh->SetupAttachment(Body);
    SteamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SteamMesh->SetCastShadow(false);
    SteamMesh->SetVisibility(false);
}

void AFoundryRobot::Initialize(uint64 Id, bool bInRam, int32 InMaxHp)
{
    CoreId = Id;
    bRam = bInRam;
    MaxHp = InMaxHp;
    const FString Asset = bRam ? TEXT("SK_BreachRam") : TEXT("SK_RivetMite");
    USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Cinderwall/Robots/%s.%s"), *Asset, *Asset));
    checkf(Mesh, TEXT("Run tools/unreal.ps1 Art before the Cinderwall encounter"));
    Body->SetSkeletalMeshAsset(Mesh);
    const FString Prefix = bRam ? TEXT("BR_") : TEXT("RM_");
    for (const TCHAR* Name : {TEXT("idle"), TEXT("hit_light"), TEXT("hit_medium"), TEXT("hit_heavy"), TEXT("death"), TEXT("escape"), TEXT("attack")})
    {
        const FString Suffix = FString(Name) == TEXT("attack") ? (bRam ? TEXT("attack_blast") : TEXT("attack_rivet")) : FString(Name);
        const FString ClipName = Prefix + Suffix;
        UAnimSequence* Clip = LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("/Game/Cinderwall/Animations/%s.%s"), *ClipName, *ClipName));
        checkf(Clip, TEXT("Missing animation %s"), *ClipName);
        Clips.Add(Name, Clip);
    }
    if (bRam)
    {
        UAnimSequence* Clip = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Cinderwall/Animations/BR_charge.BR_charge"));
        check(Clip);
        Clips.Add(TEXT("charge"), Clip);
    }
    for (int32 Index = 0; Index < Body->GetNumMaterials(); ++Index)
    {
        UMaterialInstanceDynamic* Material = Body->CreateDynamicMaterialInstance(Index);
        check(Material);
        float Parameter = -1;
        checkf(Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Dissolve")), Parameter), TEXT("All robot slots require Dissolve"));
        Material->SetScalarParameterValue(TEXT("Dissolve"), 0);
        Materials.Add(Material);
    }
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UMaterialInterface* Emissive = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Cinderwall/Materials/MI_CW_ember.MI_CW_ember"));
    FlashMesh->SetStaticMesh(Sphere);
    FlashMaterial = UMaterialInstanceDynamic::Create(Emissive, this);
    FlashMesh->SetMaterial(0, FlashMaterial);
    SteamMesh->SetStaticMesh(Sphere);
    SteamMaterial = UMaterialInstanceDynamic::Create(Emissive, this);
    SteamMaterial->SetVectorParameterValue(TEXT("FlatColor"), FLinearColor(.45f, .50f, .52f));
    SteamMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(.03f, .04f, .05f));
    SteamMesh->SetMaterial(0, SteamMaterial);
    const FVector Bounds = Mesh->GetImportedBounds().BoxExtent * 2;
    Body->RefreshBoneTransforms();
    const FVector Muzzle = Body->GetSocketTransform(TEXT("muzzle"), RTS_Component).GetLocation();
    checkf(Body->DoesSocketExist(TEXT("muzzle")) && Body->DoesSocketExist(TEXT("core")) && Body->DoesSocketExist(TEXT("intent")) && Muzzle.X < -50, TEXT("Cinderwall socket/facing check failed"));
    UE_LOG(LogFoundryRobot, Display, TEXT("ART_ROBOT id=%llu asset=%s bounds_cm=%s bones=%d muzzle_local=%s dissolve_slots=%d"),
        CoreId, *Asset, *Bounds.ToString(), Mesh->GetRefSkeleton().GetNum(), *Muzzle.ToString(), Materials.Num());
    Play(TEXT("idle"));
}

bool AFoundryRobot::AreMaterialsSolid() const
{
    for (const UMaterialInstanceDynamic* Material : Materials)
    {
        float Value = -1;
        if (!Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Dissolve")), Value) || !FMath::IsNearlyZero(Value)) return false;
    }
    return Materials.Num() == (bRam ? 8 : 7);
}

FVector AFoundryRobot::GetImpactLocation() const { return Body->GetSocketLocation(TEXT("core")); }

void AFoundryRobot::Play(const FString& Clip, uint64 EventId)
{
    UAnimSequence* Sequence = Clips.FindChecked(Clip);
    Cue = Clip;
    Elapsed = 0;
    Duration = Sequence->GetPlayLength();
    bDisplayCueFired = false;
    bContactFired = false;
    bSteamFired = false;
    Body->bPauseAnims = false;
    Body->PlayAnimation(Sequence, Clip == TEXT("idle"));
    Body->SetPlayRate(1);
    UE_LOG(LogFoundryRobot, Display, TEXT("ART_CUE id=%llu event=%llu clip=%s duration=%.3f charged=%d"), CoreId, EventId, *Sequence->GetName(), Duration, bCharged);
}

void AFoundryRobot::EnemyAction(int32 Move, uint64 EventId)
{
    if (bTerminal) return;
    if (Move == 1 && bRam) { bCharged = true; Play(TEXT("charge"), EventId); }
    else if (Move == 0) { bCharged = false; Play(TEXT("attack"), EventId); }
    else if (Move == 2) { bCharged = false; Play(TEXT("idle"), EventId); }
}

void AFoundryRobot::NamedAction(const FString& ActionName, uint64 EventId)
{
    if (bTerminal) return;
    UE_LOG(LogFoundryRobot, Display, TEXT("ART_NAMED_ACTION id=%llu event=%llu action=%s"), CoreId, EventId, *ActionName);
    if (bRam && ActionName == TEXT("charge")) { bCharged = true; Play(TEXT("charge"), EventId); }
    else if ((bRam && ActionName == TEXT("blast")) || (!bRam && ActionName == TEXT("attack")))
    { bCharged = false; Play(TEXT("attack"), EventId); }
    else if (ActionName == TEXT("recover")) { bCharged = false; Play(TEXT("idle"), EventId); }
    // Escape is driven by its committed enemy_escape event; other robot actions
    // require their own art adapter and are not silently mapped to these two rigs.
}

void AFoundryRobot::Hit(int32 HpLoss, int32 ShieldLoss, uint64 EventId)
{
    if (bTerminal) return;
    const int64 Effective = static_cast<int64>(HpLoss) + ShieldLoss;
    if (Effective <= 0)
    {
        Flash(TEXT("core"), false, .25f);
        UE_LOG(LogFoundryRobot, Display, TEXT("ART_DEFLECT id=%llu event=%llu"), CoreId, EventId);
        return;
    }
    const FString Band = 100 * Effective < 5LL * MaxHp ? TEXT("light") : (100 * Effective < 20LL * MaxHp ? TEXT("medium") : TEXT("heavy"));
    UE_LOG(LogFoundryRobot, Display, TEXT("ART_REACTION id=%llu event=%llu effective_loss=%lld max_hp=%d band=%s"), CoreId, EventId, Effective, MaxHp, *Band);
    Play(TEXT("hit_") + Band, EventId); // Replaces short reactions; no growing queue.
    Flash(TEXT("core"), false, .5f);
}

void AFoundryRobot::Die(uint64 EventId)
{
    if (bTerminal) return;
    bTerminal = true;
    bCharged = false;
    Play(TEXT("death"), EventId);
    UE_LOG(LogFoundryRobot, Display, TEXT("ART_DEATH id=%llu event=%llu core_targetability=ended dissolve_slots=%d cleanup_seconds=2"), CoreId, EventId, Materials.Num());
}

void AFoundryRobot::Escape(uint64 EventId)
{
    if (bTerminal) return;
    bTerminal = true;
    bCharged = false;
    Play(TEXT("escape"), EventId);
}

void AFoundryRobot::HoldCharge()
{
    UAnimSequence* Sequence = Clips.FindChecked(TEXT("charge"));
    Body->PlayAnimation(Sequence, false);
    Body->SetPosition(Sequence->GetPlayLength() - .001f, false);
    Body->SetPlayRate(0);
    Cue = TEXT("charge_hold");
    UE_LOG(LogFoundryRobot, Display, TEXT("ART_CHARGE_HOLD id=%llu final_pose=1"), CoreId);
}

void AFoundryRobot::Flash(FName Socket, bool bBeam, float Strength)
{
    const FVector Location = Body->GetSocketLocation(Socket);
    FlashMesh->SetWorldLocation(Location);
    FlashMesh->SetWorldRotation(FRotator::ZeroRotator);
    FlashMesh->SetWorldScale3D(FVector(.28f * Strength));
    if (bBeam)
    {
        const FVector Gun(-180, 0, 165);
        const FVector Direction = Gun - Location;
        FlashMesh->SetWorldLocation((Gun + Location) * .5);
        FlashMesh->SetWorldRotation(Direction.Rotation());
        FlashMesh->SetWorldScale3D(FVector(Direction.Length() / 100, .035 * Strength, .035 * Strength));
    }
    FlashMaterial->SetScalarParameterValue(TEXT("Dissolve"), 0);
    FlashMaterial->SetScalarParameterValue(TEXT("Glow"), 4 * Strength);
    FlashMesh->SetVisibility(true);
    FlashLight->SetWorldLocation(Location);
    FlashLight->SetLightColor(FLinearColor(1, .25f, .035f));
    FlashLight->SetIntensity(15000 * Strength);
    FlashRemaining = .20f;
}

void AFoundryRobot::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Elapsed += DeltaSeconds;
    if (FlashRemaining > 0)
    {
        FlashRemaining = FMath::Max(0.f, FlashRemaining - DeltaSeconds);
        FlashMaterial->SetScalarParameterValue(TEXT("Dissolve"), 1 - FlashRemaining / .20f);
        if (FlashRemaining <= 0) { FlashMesh->SetVisibility(false); FlashLight->SetIntensity(0); }
    }
    if (SteamRemaining > 0)
    {
        SteamRemaining = FMath::Max(0.f, SteamRemaining - DeltaSeconds);
        const float Progress = 1 - SteamRemaining / .45f;
        SteamMesh->SetRelativeLocation(FVector(55, 0, 230 + Progress * 80));
        SteamMesh->SetRelativeScale3D(FVector(.18f + Progress * .6f, .18f + Progress * .6f, .25f + Progress * .9f));
        SteamMaterial->SetScalarParameterValue(TEXT("Dissolve"), Progress);
        if (SteamRemaining <= 0) SteamMesh->SetVisibility(false);
    }
    const float Glow = bCharged ? 2.3f + .4f * FMath::Sin(GetWorld()->GetTimeSeconds() * 8) : 1.f;
    for (UMaterialInstanceDynamic* Material : Materials) Material->SetScalarParameterValue(TEXT("Glow"), Glow);
    if (Cue == TEXT("death"))
    {
        if (!bDisplayCueFired && Elapsed >= .20f) { Flash(TEXT("core"), false, 2); bDisplayCueFired = true; }
        if (!bContactFired && Elapsed >= (bRam ? .85f : .60f))
        {
            bContactFired = true;
            UE_LOG(LogFoundryRobot, Display, TEXT("ART_COLLAPSE_CONTACT id=%llu time=%.3f"), CoreId, Elapsed);
        }
        const float Dissolve = FMath::Clamp((Elapsed - 1.10f) / .90f, 0.f, 1.f);
        for (UMaterialInstanceDynamic* Material : Materials) Material->SetScalarParameterValue(TEXT("Dissolve"), Dissolve);
        if (!bDissolveLogged && Dissolve > 0)
        {
            bDissolveLogged = true;
            UE_LOG(LogFoundryRobot, Display, TEXT("ART_DISSOLVE id=%llu time=%.3f all_slots=%d"), CoreId, Elapsed, Materials.Num());
        }
        if (Elapsed >= 2)
        {
            FlashMesh->SetVisibility(false);
            SteamMesh->SetVisibility(false);
            FlashLight->SetIntensity(0);
            UE_LOG(LogFoundryRobot, Display, TEXT("ART_CLEANUP id=%llu time=%.3f dissolve=1 slots=%d attached_fx=owned actor_destroy=1"), CoreId, Elapsed, Materials.Num());
            Destroy();
        }
        return;
    }
    if (Cue == TEXT("escape") && Elapsed >= Duration) { Destroy(); return; }
    if (Cue == TEXT("attack") && !bDisplayCueFired && Elapsed >= (bRam ? .30f : .33f))
    {
        Flash(TEXT("muzzle"), true, bRam ? 2.f : .65f);
        bDisplayCueFired = true;
        UE_LOG(LogFoundryRobot, Display, TEXT("ART_MUZZLE id=%llu clip=%s time=%.3f socket=muzzle world=%s cosmetic_only=1"), CoreId, bRam ? TEXT("BR_attack_blast") : TEXT("RM_attack_rivet"), Elapsed, *Body->GetSocketLocation(TEXT("muzzle")).ToString());
    }
    if (bRam && Cue == TEXT("attack") && !bSteamFired && Elapsed >= .45f)
    {
        SteamRemaining = .45f;
        SteamMesh->SetVisibility(true);
        SteamMaterial->SetScalarParameterValue(TEXT("Dissolve"), 0);
        bSteamFired = true;
        UE_LOG(LogFoundryRobot, Display, TEXT("ART_STEAM id=%llu time=%.3f cosmetic_only=1"), CoreId, Elapsed);
    }
    if (Cue == TEXT("charge") && Elapsed >= Duration) HoldCharge();
    else if ((Cue.StartsWith(TEXT("hit_")) || Cue == TEXT("attack")) && Elapsed >= Duration)
    {
        if (bCharged) HoldCharge(); else Play(TEXT("idle"));
    }
}

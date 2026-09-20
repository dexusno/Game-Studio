#include "FoundryMara.h"

#include "Animation/AnimSequence.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogFoundryMara, Log, All);

AFoundryMara::AFoundryMara()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("MaraPresentation")));
    Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MaraForgeGun"));
    Gun->SetupAttachment(RootComponent);
    Gun->SetRelativeLocation(FVector(-350, 0, 1.5));
    Claw = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MaraRearClaw"));
    Claw->SetupAttachment(RootComponent);
    Claw->SetRelativeLocation(FVector(-675, -100, 0));
    for (USkeletalMeshComponent* Rig : {Gun.Get(), Claw.Get()})
    {
        Rig->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Rig->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Rig->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    }
    Payload = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CosmeticGrabPayload"));
    Payload->SetupAttachment(Claw, TEXT("grab"));
    // FBX sockets retain the armature's metre unit scale. The static payload
    // already has centimetre render geometry and must not inherit that scale.
    Payload->SetAbsolute(false, false, true);
    Payload->SetWorldScale3D(FVector::OneVector);
    Payload->bUseAsOccluder = false;
    MuzzleFlash = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MaraMuzzleCue"));
    MuzzleFlash->SetupAttachment(RootComponent);
    ShotBeam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MaraShotCue"));
    ShotBeam->SetupAttachment(RootComponent);
    for (UStaticMeshComponent* Part : {Payload.Get(), MuzzleFlash.Get(), ShotBeam.Get()})
    {
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetVisibility(false);
    }
    MuzzleFlash->SetCastShadow(false);
    ShotBeam->SetCastShadow(false);
    ShotLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MaraShotLight"));
    ShotLight->SetupAttachment(RootComponent);
    ShotLight->SetIntensity(0);
    ShotLight->SetAttenuationRadius(280);
    ShotLight->SetCastShadows(false);
}

void AFoundryMara::Initialize()
{
    for (const bool bGun : {true, false})
    {
        const FString Name = bGun ? TEXT("MaraGun") : TEXT("MaraClaw");
        USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Cinderwall/Mara/Props/SK_%s.SK_%s"), *Name, *Name));
        checkf(Mesh, TEXT("Run MaraArt after generating and verifying Mara source exports"));
        USkeletalMeshComponent* Rig = bGun ? Gun.Get() : Claw.Get();
        Rig->SetSkeletalMeshAsset(Mesh);
        const TArray<FString> Names = bGun ? TArray<FString>{TEXT("idle"), TEXT("load"), TEXT("unload"), TEXT("fire"), TEXT("recovery")} : TArray<FString>{TEXT("idle"), TEXT("collect")};
        for (const FString& Cue : Names)
        {
            const FString Asset = (bGun ? TEXT("MG_") : TEXT("MC_")) + Cue;
            UAnimSequence* Clip = LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("/Game/Cinderwall/Mara/Animations/%s.%s"), *Asset, *Asset));
            checkf(Clip, TEXT("Missing Mara clip %s"), *Asset);
            (bGun ? GunClips : ClawClips).Add(Cue, Clip);
        }
        UE_LOG(LogFoundryMara, Display, TEXT("MARA_RIG asset=%s bounds_cm=%s bones=%d materials=%d"), *Name, *(Mesh->GetImportedBounds().BoxExtent * 2).ToString(), Mesh->GetRefSkeleton().GetNum(), Rig->GetNumMaterials());
    }
    for (const TCHAR* Socket : {TEXT("muzzle"), TEXT("load"), TEXT("eject"), TEXT("shield_attach"), TEXT("operator_attach")}) check(Gun->DoesSocketExist(Socket));
    check(Claw->DoesSocketExist(TEXT("grab")) && Claw->DoesSocketExist(TEXT("dump")));
    Payload->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Cinderwall/Mara/Stage/MaraStageExtension_SM_Mara_payload.MaraStageExtension_SM_Mara_payload")));
    check(Payload->GetStaticMesh());
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UMaterialInterface* Ember = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Cinderwall/Materials/MI_CW_ember.MI_CW_ember"));
    check(Sphere && Ember);
    FlashMaterial = UMaterialInstanceDynamic::Create(Ember, this);
    MuzzleFlash->SetStaticMesh(Sphere);
    ShotBeam->SetStaticMesh(Sphere);
    MuzzleFlash->SetMaterial(0, FlashMaterial);
    ShotBeam->SetMaterial(0, FlashMaterial);
    ResetPresentation();
}

void AFoundryMara::PlayGun(const FString& Name, uint64 EventId)
{
    GunCue = Name;
    GunElapsed = 0;
    Gun->PlayAnimation(GunClips.FindChecked(Name), Name == TEXT("idle"));
    UE_LOG(LogFoundryMara, Display, TEXT("MARA_CUE rig=gun event=%llu clip=MG_%s duration=%.3f"), EventId, *Name, GunClips.FindChecked(Name)->GetPlayLength());
}

void AFoundryMara::PlayClaw(const FString& Name, uint64 EventId)
{
    ClawCue = Name;
    ClawElapsed = 0;
    Claw->PlayAnimation(ClawClips.FindChecked(Name), Name == TEXT("idle"));
    UE_LOG(LogFoundryMara, Display, TEXT("MARA_CUE rig=claw event=%llu clip=MC_%s duration=%.3f"), EventId, *Name, ClawClips.FindChecked(Name)->GetPlayLength());
}

void AFoundryMara::ResetPresentation()
{
    bLoaded = bGrabbed = bDumped = false;
    FlashRemaining = 0;
    Payload->SetVisibility(false);
    Payload->AttachToComponent(Claw, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("grab"));
    MuzzleFlash->SetVisibility(false);
    ShotBeam->SetVisibility(false);
    ShotLight->SetIntensity(0);
    PlayGun(TEXT("idle"));
    PlayClaw(TEXT("idle"));
    UE_LOG(LogFoundryMara, Display, TEXT("MARA_RESET attachments_clear=1"));
}

bool AFoundryMara::HasPayload() const { return Payload->IsVisible(); }
bool AFoundryMara::IsReset() const { return !bLoaded && !Payload->IsVisible() && FlashRemaining == 0 && GunCue == TEXT("idle") && ClawCue == TEXT("idle"); }

void AFoundryMara::Collect(uint64 EventId, int32 ActualHaul)
{
    bGrabbed = bDumped = false;
    MinGrabHeight = 10000;
    Payload->SetVisibility(false);
    Payload->AttachToComponent(Claw, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("grab"));
    PlayClaw(TEXT("collect"), EventId);
    UE_LOG(LogFoundryMara, Display, TEXT("MARA_COLLECT event=%llu committed_haul=%d material_authority=core cosmetic_payload_only=1"), EventId, ActualHaul);
}

void AFoundryMara::Load(uint64 EventId, int32 ActualParts)
{
    bLoaded = true;
    PlayGun(TEXT("load"), EventId);
    UE_LOG(LogFoundryMara, Display, TEXT("MARA_LOAD event=%llu committed_parts=%d"), EventId, ActualParts);
}

void AFoundryMara::Unload(uint64 EventId)
{
    // End Turn emits unload even for an empty assembly. Avoid a false physical cue.
    if (bLoaded) PlayGun(TEXT("unload"), EventId);
    bLoaded = false;
}

void AFoundryMara::Fire(uint64 EventId, int32 ActualShot, const FVector& Target)
{
    bLoaded = false;
    ShotTarget = Target;
    ShotStrength = FMath::Clamp(.7f + ActualShot / 50.f, .7f, 1.8f);
    FireMuzzleStart = Gun->GetSocketLocation(TEXT("muzzle"));
    MaxRecoilMotion = 0;
    FlashRemaining = .13f;
    PlayGun(TEXT("fire"), EventId); // Rapid valid Fire replaces the short cosmetic cue.
    UE_LOG(LogFoundryMara, Display, TEXT("MARA_FIRE event=%llu committed_shot=%d target_world=%s"), EventId, ActualShot, *Target.ToString());
}

void AFoundryMara::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    GunElapsed += DeltaSeconds;
    ClawElapsed += DeltaSeconds;
    if (GunCue == TEXT("fire")) MaxRecoilMotion = FMath::Max(MaxRecoilMotion, static_cast<float>(FVector::Distance(FireMuzzleStart, Gun->GetSocketLocation(TEXT("muzzle")))));
    if (GunCue != TEXT("idle") && GunElapsed >= GunClips.FindChecked(GunCue)->GetPlayLength())
    {
        if (GunCue == TEXT("fire"))
        {
            UE_LOG(LogFoundryMara, Display, TEXT("MARA_RECOIL measured_muzzle_motion_cm=%.3f"), MaxRecoilMotion);
            PlayGun(TEXT("recovery"));
        }
        else PlayGun(TEXT("idle"));
    }
    if (ClawCue == TEXT("collect"))
    {
        MinGrabHeight = FMath::Min(MinGrabHeight, static_cast<float>(Claw->GetSocketLocation(TEXT("grab")).Z));
        if (!bGrabbed && ClawElapsed >= .43f)
        {
            bGrabbed = true;
            Payload->SetVisibility(true);
            check(Payload->GetComponentScale().Equals(FVector::OneVector, .001));
            UE_LOG(LogFoundryMara, Display, TEXT("MARA_GRAB elapsed=%.3f socket_world=%s payload_world_scale=%s"), ClawElapsed, *Claw->GetSocketLocation(TEXT("grab")).ToString(), *Payload->GetComponentScale().ToString());
        }
        if (!bDumped && ClawElapsed >= 1.06f)
        {
            bDumped = true;
            DropStart = Payload->GetComponentLocation();
            Payload->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            UE_LOG(LogFoundryMara, Display, TEXT("MARA_DUMP elapsed=%.3f cosmetic_drop=1 socket_world=%s"), ClawElapsed, *Claw->GetSocketLocation(TEXT("grab")).ToString());
        }
        if (bDumped)
        {
            const float Fall = FMath::Clamp((ClawElapsed - 1.06f) / .18f, 0.f, 1.f);
            Payload->SetWorldLocation(FMath::Lerp(DropStart, FVector(-580, -100, 130), Fall * Fall));
            if (Fall >= 1) Payload->SetVisibility(false);
        }
        if (ClawElapsed >= ClawClips.FindChecked(TEXT("collect"))->GetPlayLength())
        {
            Payload->SetVisibility(false);
            UE_LOG(LogFoundryMara, Display, TEXT("MARA_COLLECTION_COMPLETE min_grab_z_cm=%.3f payload_cleared=1 no_gameplay_callbacks=1"), MinGrabHeight);
            PlayClaw(TEXT("idle"));
        }
    }
    if (FlashRemaining > 0)
    {
        FlashRemaining = FMath::Max(0.f, FlashRemaining - DeltaSeconds);
        const FVector Muzzle = Gun->GetSocketLocation(TEXT("muzzle"));
        const FVector Delta = ShotTarget - Muzzle;
        const float Fade = FlashRemaining / .13f;
        MuzzleFlash->SetVisibility(FlashRemaining > 0);
        ShotBeam->SetVisibility(FlashRemaining > 0);
        MuzzleFlash->SetWorldLocation(Muzzle);
        MuzzleFlash->SetWorldScale3D(FVector(.46f, .19f, .19f) * ShotStrength * Fade);
        ShotBeam->SetWorldLocation((Muzzle + ShotTarget) * .5);
        ShotBeam->SetWorldRotation(Delta.Rotation());
        ShotBeam->SetWorldScale3D(FVector(Delta.Size() / 100, .024f * ShotStrength, .024f * ShotStrength));
        FlashMaterial->SetScalarParameterValue(TEXT("Glow"), 7.f * Fade);
        ShotLight->SetWorldLocation(Muzzle);
        ShotLight->SetLightColor(FLinearColor(1, .48f, .16f));
        ShotLight->SetIntensity(22000.f * Fade * ShotStrength);
    }
}

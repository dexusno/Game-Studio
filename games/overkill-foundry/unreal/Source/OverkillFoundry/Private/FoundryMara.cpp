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

namespace
{
bool IsBoneUnder(const FReferenceSkeleton& Skeleton, int32 Bone, int32 Root)
{
    for (int32 Parent = Bone; Parent != INDEX_NONE; Parent = Skeleton.GetParentIndex(Parent))
        if (Parent == Root) return true;
    return false;
}
}

void UFoundryAimedGun::FinalizeBoneTransform()
{
    // FinalizeBoneTransform flips the evaluated/editable buffer in UE 5.8.
    // Only modify a newly evaluated buffer, never accumulate onto an old pose.
    if (bNeedToFlipSpaceBaseBuffers) CosmeticAimDelta = FTransform::Identity;
    if (bAim && bNeedToFlipSpaceBaseBuffers && GetSkeletalMeshAsset())
    {
        auto& Pose = GetEditableComponentSpaceTransforms();
        const auto& Skeleton = GetSkeletalMeshAsset()->GetRefSkeleton();
        const int32 Cradle = GetBoneIndex(TEXT("cradle"));
        if (Pose.IsValidIndex(Cradle))
        {
            const FVector Pivot = Pose[Cradle].GetLocation();
            const FVector Delta = GetComponentTransform().InverseTransformPosition(TargetWorld) - Pivot;
            // The authored bore is 52 cm above the trunnion. Solve its offset
            // ray toward the selected robot, rather than aiming the pivot ray.
            const double Pitch = FMath::Atan2(Delta.Z, FVector2D(Delta.X, Delta.Y).Size()) - FMath::Asin(FMath::Clamp(52.0 / Delta.Size(), -1.0, 1.0));
            const FQuat Aim = FRotator(FMath::RadiansToDegrees(Pitch), FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)), 0).Quaternion();
            const FVector WorldPivot = GetComponentTransform().TransformPosition(Pivot);
            const FQuat WorldAim = GetComponentQuat() * Aim * GetComponentQuat().Inverse();
            CosmeticAimDelta = FTransform(WorldAim, WorldPivot - WorldAim.RotateVector(WorldPivot));
            for (int32 Bone = 0; Bone < Pose.Num(); ++Bone)
            {
                int32 Parent = Bone;
                while (Parent != INDEX_NONE && Parent != Cradle) Parent = Skeleton.GetParentIndex(Parent);
                if (Parent != Cradle) continue;
                Pose[Bone].SetLocation(Pivot + Aim.RotateVector(Pose[Bone].GetLocation() - Pivot));
                Pose[Bone].SetRotation(Aim * Pose[Bone].GetRotation());
            }
        }
    }
    Super::FinalizeBoneTransform();
}

void UFoundryAimedOperator::FinalizeBoneTransform()
{
    if (bNeedToFlipSpaceBaseBuffers && GetSkeletalMeshAsset() && GunSource.IsValid())
    {
        auto& Pose = GetEditableComponentSpaceTransforms();
        const auto& Skeleton = GetSkeletalMeshAsset()->GetRefSkeleton();
        const int32 Pelvis = GetBoneIndex(TEXT("pelvis")), Spine = GetBoneIndex(TEXT("spine"));
        const int32 Root = GetBoneIndex(TEXT("root")), LeftFoot = GetBoneIndex(TEXT("foot_l")), RightFoot = GetBoneIndex(TEXT("foot_r"));
        const int32 Upper[] = {GetBoneIndex(TEXT("upperarm_l")), GetBoneIndex(TEXT("upperarm_r"))};
        const int32 Fore[] = {GetBoneIndex(TEXT("forearm_l")), GetBoneIndex(TEXT("forearm_r"))};
        const int32 Hand[] = {GetBoneIndex(TEXT("hand_l")), GetBoneIndex(TEXT("hand_r"))};
        if (Pose.IsValidIndex(Pelvis) && Pose.IsValidIndex(Spine) && Pose.IsValidIndex(Root) && Pose.IsValidIndex(LeftFoot) && Pose.IsValidIndex(RightFoot) &&
            Pose.IsValidIndex(Upper[0]) && Pose.IsValidIndex(Upper[1]) &&
            Pose.IsValidIndex(Fore[0]) && Pose.IsValidIndex(Fore[1]) &&
            Pose.IsValidIndex(Hand[0]) && Pose.IsValidIndex(Hand[1]))
        {
            const int32 FixedBones[] = {Root, Pelvis, LeftFoot, RightFoot};
            FVector FixedPoints[4];
            for (int32 I = 0; I < 4; ++I) FixedPoints[I] = Pose[FixedBones[I]].GetLocation();
            const FTransform Aim = GunSource->GetCosmeticAimDelta();
            const FTransform Component = GetComponentTransform();
            const FQuat LocalAim = Component.GetRotation().Inverse() * Aim.GetRotation() * Component.GetRotation();
            FVector Targets[2]; FQuat Rotations[2]; double UpperLength[2], ForeLength[2];
            ReachErrorCm = BodyLeanDegrees = 0;
            double WorstExcess = 0;
            FVector LeanDirection = FVector::ForwardVector;
            for (int32 Side = 0; Side < 2; ++Side)
            {
                // Transform the current authored gesture, rather than pinning
                // every clip to the idle grip and erasing load/recoil movement.
                Targets[Side] = Component.InverseTransformPosition(Aim.TransformPosition(Component.TransformPosition(Pose[Hand[Side]].GetLocation())));
                Rotations[Side] = LocalAim * Pose[Hand[Side]].GetRotation();
                UpperLength[Side] = FVector::Distance(Pose[Upper[Side]].GetLocation(), Pose[Fore[Side]].GetLocation());
                ForeLength[Side] = FVector::Distance(Pose[Fore[Side]].GetLocation(), Pose[Hand[Side]].GetLocation());
                const FVector Reach = Targets[Side] - Pose[Upper[Side]].GetLocation();
                const double Excess = Reach.Size() - (UpperLength[Side] + ForeLength[Side] - .4);
                if (Excess > WorstExcess)
                {
                    WorstExcess = Excess;
                    LeanDirection = FVector(Reach.X, Reach.Y, 0).GetSafeNormal();
                }
            }
            const FVector Pivot = Pose[Pelvis].GetLocation();
            FQuat Lean = FQuat::Identity;
            if (WorstExcess > 0 && !LeanDirection.IsNearlyZero())
            {
                const double Azimuth = FMath::Clamp(FMath::Atan2(LeanDirection.Y, LeanDirection.X), -PI / 4, PI / 4);
                LeanDirection = FVector(FMath::Cos(Azimuth), FMath::Sin(Azimuth), 0);
                const FVector Axis = FVector::CrossProduct(FVector::UpVector, LeanDirection).GetSafeNormal();
                // A small shared torso lean supplies reach at the far aiming
                // edge. Pelvis, legs and feet keep their evaluated transforms.
                for (int32 Step = 1; Step <= 40; ++Step)
                {
                    BodyLeanDegrees = Step * .25f;
                    Lean = FQuat(Axis, FMath::DegreesToRadians(BodyLeanDegrees));
                    bool bReachable = true;
                    for (int32 Side = 0; Side < 2; ++Side)
                    {
                        const FVector Shoulder = Pivot + Lean.RotateVector(Pose[Upper[Side]].GetLocation() - Pivot);
                        if (FVector::Distance(Shoulder, Targets[Side]) > UpperLength[Side] + ForeLength[Side] - .4) bReachable = false;
                    }
                    if (bReachable) break;
                }
                for (int32 Bone = 0; Bone < Pose.Num(); ++Bone) if (IsBoneUnder(Skeleton, Bone, Spine))
                {
                    Pose[Bone].SetLocation(Pivot + Lean.RotateVector(Pose[Bone].GetLocation() - Pivot));
                    Pose[Bone].SetRotation(Lean * Pose[Bone].GetRotation());
                }
            }
            for (int32 Side = 0; Side < 2; ++Side)
            {
                const FVector Shoulder = Pose[Upper[Side]].GetLocation(), OldElbow = Pose[Fore[Side]].GetLocation();
                const FTransform OldHand = Pose[Hand[Side]];
                const FVector Reach = Targets[Side] - Shoulder;
                const FVector Direction = Reach.GetSafeNormal();
                const double A = UpperLength[Side], B = ForeLength[Side];
                if (A <= .01 || B <= .01 || Direction.IsNearlyZero()) continue;
                const double Distance = FMath::Clamp(Reach.Size(), FMath::Abs(A - B) + .01, A + B - .4);
                FVector Pole = (OldElbow - Shoulder) - Direction * FVector::DotProduct(OldElbow - Shoulder, Direction);
                if (!Pole.Normalize()) Pole = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal();
                const double Along = (A * A - B * B + Distance * Distance) / (2 * Distance);
                const FVector Elbow = Shoulder + Direction * Along + Pole * FMath::Sqrt(FMath::Max(0., A * A - Along * Along));
                const FVector Wrist = Shoulder + Direction * Distance;
                Pose[Upper[Side]].SetRotation(FQuat::FindBetweenVectors(OldElbow - Shoulder, Elbow - Shoulder) * Pose[Upper[Side]].GetRotation());
                Pose[Fore[Side]].SetLocation(Elbow);
                Pose[Fore[Side]].SetRotation(FQuat::FindBetweenVectors(OldHand.GetLocation() - OldElbow, Wrist - Elbow) * Pose[Fore[Side]].GetRotation());
                FTransform NewHand = OldHand; NewHand.SetLocation(Wrist); NewHand.SetRotation(Rotations[Side]);
                for (int32 Bone = 0; Bone < Pose.Num(); ++Bone) if (Bone != Hand[Side] && IsBoneUnder(Skeleton, Bone, Hand[Side]))
                    Pose[Bone] = Pose[Bone].GetRelativeTransform(OldHand) * NewHand;
                Pose[Hand[Side]] = NewHand;
                ReachErrorCm = FMath::Max(ReachErrorCm, static_cast<float>(FVector::Distance(Wrist, Targets[Side])));
            }
            LowerBodyShiftCm = 0;
            for (int32 I = 0; I < 4; ++I)
                LowerBodyShiftCm = FMath::Max(LowerBodyShiftCm, static_cast<float>(FVector::Distance(FixedPoints[I], Pose[FixedBones[I]].GetLocation())));
        }
    }
    Super::FinalizeBoneTransform();
}

FBoxSphereBounds UFoundryAimedOperator::CalcBounds(const FTransform& LocalToWorld) const
{
    // There is deliberately no physics asset. UE's fallback rest-pose box is
    // too narrow for the forward-reaching arms; include the current pose for
    // visibility and shadows without changing the imported geometry's scale.
    FBox Envelope = Super::CalcBounds(LocalToWorld).GetBox();
    const FVector Margin = LocalToWorld.GetScale3D().GetAbs() * 28;
    for (const FTransform& Bone : GetComponentSpaceTransforms())
    {
        const FVector Point = LocalToWorld.TransformPosition(Bone.GetLocation());
        Envelope += FBox(Point - Margin, Point + Margin);
    }
    return FBoxSphereBounds(Envelope);
}

AFoundryMara::AFoundryMara()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("MaraPresentation")));
    Gun = CreateDefaultSubobject<UFoundryAimedGun>(TEXT("MaraForgeGun"));
    Gun->SetupAttachment(RootComponent);
    Gun->SetRelativeLocation(FVector(-350, 0, 1.5));
    Claw = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MaraRearClaw"));
    Claw->SetupAttachment(RootComponent);
    Claw->SetRelativeLocation(FVector(-675, -100, 0));
    Operator = CreateDefaultSubobject<UFoundryAimedOperator>(TEXT("MaraOperator"));
    Operator->SetupAttachment(RootComponent);
    Operator->SetRelativeLocation(FVector(-496, 55, .7));
    Operator->GunSource = Gun.Get();
    Operator->AddTickPrerequisiteComponent(Gun);
    for (USkeletalMeshComponent* Rig : {static_cast<USkeletalMeshComponent*>(Gun.Get()), Claw.Get(), static_cast<USkeletalMeshComponent*>(Operator.Get())})
    {
        Rig->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Rig->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Rig->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    }
    Payload = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CosmeticGrabPayload"));
    Payload->SetupAttachment(Claw); // The real socket is attached after the rig loads.
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
        USkeletalMeshComponent* Rig = bGun ? static_cast<USkeletalMeshComponent*>(Gun.Get()) : Claw.Get();
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
    USkeletalMesh* OperatorMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Cinderwall/Operator/Meshes/SK_MaraOperator.SK_MaraOperator"));
    checkf(OperatorMesh, TEXT("Run OperatorArt after generating and verifying the original operator source"));
    Operator->SetSkeletalMeshAsset(OperatorMesh);
    for (const FString& Cue : {FString(TEXT("idle")), FString(TEXT("load")), FString(TEXT("fire")), FString(TEXT("recovery"))})
    {
        const FString Asset = TEXT("MO_") + Cue;
        UAnimSequence* Clip = LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("/Game/Cinderwall/Operator/Animations/%s.%s"), *Asset, *Asset));
        checkf(Clip, TEXT("Missing original operator clip %s"), *Asset);
        OperatorClips.Add(Cue, Clip);
    }
    check(Operator->DoesSocketExist(TEXT("grip_l")) && Operator->DoesSocketExist(TEXT("grip_r")) && Operator->DoesSocketExist(TEXT("portrait")));
    checkf(Operator->GetNumMaterials() == 1 && (OperatorMesh->GetImportedBounds().BoxExtent * 2).Equals(FVector(42.825, 97.32, 179.287), 2), TEXT("Original operator materials and centimetre bounds changed"));
    UE_LOG(LogFoundryMara, Display, TEXT("MARA_RIG asset=MaraOperator bounds_cm=%s bones=%d materials=%d clips=%d"), *(OperatorMesh->GetImportedBounds().BoxExtent * 2).ToString(), OperatorMesh->GetRefSkeleton().GetNum(), Operator->GetNumMaterials(), OperatorClips.Num());
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

void AFoundryMara::PlayOperator(const FString& Name, uint64 EventId)
{
    OperatorCue = Name;
    OperatorElapsed = 0;
    Operator->PlayAnimation(OperatorClips.FindChecked(Name), Name == TEXT("idle"));
    UE_LOG(LogFoundryMara, Display, TEXT("MARA_CUE rig=operator event=%llu clip=MO_%s duration=%.3f cosmetic_only=1"), EventId, *Name, OperatorClips.FindChecked(Name)->GetPlayLength());
}

void AFoundryMara::ResetPresentation()
{
    bLoaded = bGrabbed = bDumped = false;
    Gun->bAim = false;
    FlashRemaining = 0;
    Payload->SetVisibility(false);
    Payload->AttachToComponent(Claw, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("grab"));
    MuzzleFlash->SetVisibility(false);
    ShotBeam->SetVisibility(false);
    ShotLight->SetIntensity(0);
    PlayGun(TEXT("idle"));
    PlayClaw(TEXT("idle"));
    PlayOperator(TEXT("idle"));
    UE_LOG(LogFoundryMara, Display, TEXT("MARA_RESET attachments_clear=1"));
}

bool AFoundryMara::HasPayload() const { return Payload->IsVisible(); }
bool AFoundryMara::IsReset() const { return !bLoaded && !Payload->IsVisible() && FlashRemaining == 0 && GunCue == TEXT("idle") && ClawCue == TEXT("idle") && OperatorCue == TEXT("idle"); }

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
    PlayOperator(TEXT("load"), EventId);
    UE_LOG(LogFoundryMara, Display, TEXT("MARA_LOAD event=%llu committed_parts=%d"), EventId, ActualParts);
}

void AFoundryMara::Unload(uint64 EventId)
{
    // End Turn emits unload even for an empty assembly. Avoid a false physical cue.
    if (bLoaded) { PlayGun(TEXT("unload"), EventId); PlayOperator(TEXT("recovery"), EventId); }
    bLoaded = false;
}

void AFoundryMara::Fire(uint64 EventId, int32 ActualShot, const FVector& Target)
{
    AimAt(Target);
    Gun->RefreshBoneTransforms();
    Operator->RefreshBoneTransforms();
    UE_LOG(LogFoundryMara, Display, TEXT("MARA_OPERATOR_AIM event=%llu reach_residual_cm=%.3f torso_lean_degrees=%.3f lower_body_shift_cm=%.3f"), EventId, Operator->GetReachErrorCm(), Operator->GetBodyLeanDegrees(), Operator->GetLowerBodyShiftCm());
    const FVector Bore = (Gun->GetSocketLocation(TEXT("muzzle")) - Gun->GetSocketLocation(TEXT("recoil"))).GetSafeNormal();
    const FVector AimDirection = (Target - Gun->GetSocketLocation(TEXT("muzzle"))).GetSafeNormal();
    UE_LOG(LogFoundryMara, Display, TEXT("MARA_AIM event=%llu bore_target_degrees=%.3f"), EventId, FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Bore, AimDirection), -1.0, 1.0))));
    bLoaded = false;
    ShotTarget = Target;
    ShotStrength = FMath::Clamp(.7f + ActualShot / 50.f, .7f, 1.8f);
    FireMuzzleStart = Gun->GetSocketLocation(TEXT("muzzle"));
    MaxRecoilMotion = 0;
    FlashRemaining = .13f;
    PlayGun(TEXT("fire"), EventId); // Rapid valid Fire replaces the short cosmetic cue.
    PlayOperator(TEXT("fire"), EventId);
    UE_LOG(LogFoundryMara, Display, TEXT("MARA_FIRE event=%llu committed_shot=%d target_world=%s"), EventId, ActualShot, *Target.ToString());
}

void AFoundryMara::AimAt(const FVector& Target)
{
    Gun->TargetWorld = Target;
    Gun->bAim = true;
}

void AFoundryMara::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    GunElapsed += DeltaSeconds;
    ClawElapsed += DeltaSeconds;
    OperatorElapsed += DeltaSeconds;
    if (OperatorCue != TEXT("idle") && OperatorElapsed >= OperatorClips.FindChecked(OperatorCue)->GetPlayLength())
        PlayOperator(OperatorCue == TEXT("fire") ? TEXT("recovery") : TEXT("idle"));
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

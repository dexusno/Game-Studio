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
#include "FoundryRosterData.inl"

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

void AFoundryRobot::Initialize(uint64 Id, const FString& InDefinition, int32 InMaxHp, int32 Tiles, bool bInitiallyCharged)
{
    CoreId = Id;
    Definition = InDefinition;
    bRam = Definition == TEXT("C1-R02");
    MaxHp = InMaxHp;
    FString Asset;
    for (const auto& Row : FoundryVisualData::Robots) if (Definition == Row.Definition)
    {
        Asset = Row.Asset;
        ExpectedMaterialSlots = Row.MaterialSlots;
        break;
    }
    checkf(!Asset.IsEmpty(), TEXT("Enabled enemy has no original visual: %s"), *Definition);
    USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Cinderwall/Robots/%s.%s"), *Asset, *Asset));
    checkf(Mesh, TEXT("Missing %s; run tools/unreal.ps1 Art and RosterArt"), *Asset);
    Body->SetSkeletalMeshAsset(Mesh);
    for (const auto& Row : FoundryVisualData::Clips)
    {
        if (Definition != Row.Definition) continue;
        const FString ClipName = Row.Asset;
        UAnimSequence* Clip = LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("/Game/Cinderwall/Animations/%s.%s"), *ClipName, *ClipName));
        checkf(Clip, TEXT("Missing animation %s"), *ClipName);
        Clips.Add(Row.Key, Clip);
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
    checkf(Materials.Num() == ExpectedMaterialSlots, TEXT("Original robot material inventory changed"));
    Play(TEXT("idle"));
    SetTiles(Tiles);
    if (bInitiallyCharged && Clips.Contains(TEXT("charge"))) { bCharged = true; HoldCharge(); }
}

bool AFoundryRobot::AreMaterialsSolid() const
{
    for (const UMaterialInstanceDynamic* Material : Materials)
    {
        float Value = -1;
        if (!Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Dissolve")), Value) || !FMath::IsNearlyZero(Value)) return false;
    }
    return Materials.Num() == ExpectedMaterialSlots && ExpectedMaterialSlots > 0;
}

FString AFoundryRobot::GetVisualAssetName() const { return Body->GetSkeletalMeshAsset()->GetName(); }

bool AFoundryRobot::HasAction(const FString& Name) const
{
    return Clips.Contains(bRam && Name == TEXT("blast") ? TEXT("attack") : Name);
}

void AFoundryRobot::SetTiles(int32 Tiles)
{
    if (Definition != TEXT("C1-O02") || Tiles == VisibleTiles) return;
    VisibleTiles = FMath::Clamp(Tiles, 0, 3);
    for (int32 Index = 0; Index < 3; ++Index)
    {
        const FName Bone(*FString::Printf(TEXT("tile_%d"), Index));
        if (Index < VisibleTiles) Body->UnHideBoneByName(Bone);
        else Body->HideBoneByName(Bone, PBO_None);
    }
    UE_LOG(LogFoundryRobot, Display, TEXT("ART_TILES id=%llu remaining=%d committed_state_only=1"), CoreId, VisibleTiles);
}

int32 AFoundryRobot::GetVisibleTileCount() const
{
    if (Definition != TEXT("C1-O02")) return 0;
    int32 Count = 0;
    for (int32 Index = 0; Index < 3; ++Index)
        if (!Body->IsBoneHiddenByName(FName(*FString::Printf(TEXT("tile_%d"), Index)))) ++Count;
    return Count;
}

void AFoundryRobot::UpdateVisibility() { SetActorHiddenInGame(bSceneHidden || bAwaitingReveal || RevealRemaining > 0); }
void AFoundryRobot::SetSceneHidden(bool bInSceneHidden) { bSceneHidden = bInSceneHidden; UpdateVisibility(); }
void AFoundryRobot::AwaitReveal() { bAwaitingReveal = true; UpdateVisibility(); }
void AFoundryRobot::RevealAfter(float Seconds)
{
    bAwaitingReveal = false;
    RevealRemaining = Seconds;
    UpdateVisibility();
    UE_LOG(LogFoundryRobot, Display, TEXT("ART_SUMMON_WAIT id=%llu seconds=%.3f already_committed=1"), CoreId, Seconds);
}

void AFoundryRobot::CompletePendingReveal()
{
    if (!bAwaitingReveal && RevealRemaining <= 0) return;
    bAwaitingReveal = false;
    RevealRemaining = 0;
    UpdateVisibility();
    UE_LOG(LogFoundryRobot, Display, TEXT("ART_SUMMON_RECONCILE id=%llu committed_state_only=1"), CoreId);
}

FBox AFoundryRobot::GetBodyBounds() const { return Body->Bounds.GetBox(); }

FVector AFoundryRobot::GetImpactLocation() const { return Body->GetSocketLocation(TEXT("core")); }

void AFoundryRobot::Play(const FString& Clip, uint64 EventId)
{
    UAnimSequence* Sequence = Clips.FindChecked(Clip);
    Cue = Clip;
    ClipAsset = Sequence->GetName();
    CueEventId = EventId;
    Elapsed = 0;
    Duration = Sequence->GetPlayLength();
    DisplayedCues.Empty();
    Body->bPauseAnims = false;
    Body->PlayAnimation(Sequence, Clip == TEXT("idle"));
    Body->SetPlayRate(1);
    UE_LOG(LogFoundryRobot, Display, TEXT("ART_CUE id=%llu event=%llu clip=%s duration=%.3f charged=%d"), CoreId, EventId, *Sequence->GetName(), Duration, bCharged);
}

void AFoundryRobot::EnemyAction(int32 Move, uint64 EventId)
{
    if (bTerminal) return;
    if (Move == 1 && bRam) { bCharged = true; Play(TEXT("charge"), EventId); }
    else if (Move == 0 && Clips.Contains(TEXT("attack"))) { bCharged = false; CommittedHits = 1; Play(TEXT("attack"), EventId); }
    else if (Move == 2) { bCharged = false; Play(TEXT("idle"), EventId); }
}

void AFoundryRobot::NamedAction(const FString& ActionName, uint64 EventId, int32 InCommittedHits, bool bInCommittedSummon)
{
    if (bTerminal) return;
    UE_LOG(LogFoundryRobot, Display, TEXT("ART_NAMED_ACTION id=%llu event=%llu action=%s"), CoreId, EventId, *ActionName);
    if (ActionName == TEXT("escape")) return; // Its terminal event owns cleanup.
    const FString Clip = bRam && ActionName == TEXT("blast") ? TEXT("attack") : ActionName;
    checkf(Clips.Contains(Clip), TEXT("Unmapped authored robot action: %s / %s"), *Definition, *ActionName);
    CommittedHits = InCommittedHits;
    bCommittedSummon = bInCommittedSummon;
    bCharged = ActionName == TEXT("charge");
    Play(Clip, EventId);
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
    if (Cue != TEXT("idle") && Cue != TEXT("charge_hold") && !Cue.StartsWith(TEXT("hit_")) && Elapsed < Duration)
    {
        // A counter-hit must not erase a committed multi-hit attack. Keep at
        // most its strongest short reaction, then play it after that action.
        if (PendingReaction.IsEmpty() || Band == TEXT("heavy") || (Band == TEXT("medium") && PendingReaction != TEXT("hit_heavy")))
            PendingReaction = TEXT("hit_") + Band;
    }
    else Play(TEXT("hit_") + Band, EventId);
    Flash(TEXT("core"), false, .5f);
}

void AFoundryRobot::Die(uint64 EventId, bool bDeathRelease, bool bInCommittedSummon)
{
    if (bTerminal) return;
    bTerminal = true;
    bCharged = false;
    bCommittedSummon = bInCommittedSummon;
    DeathEventId = EventId;
    const FString DeathClip = bDeathRelease && Clips.Contains(TEXT("death_release")) ? TEXT("death_release") : TEXT("death");
    PendingReaction.Empty();
    if (Cue != TEXT("idle") && Cue != TEXT("charge_hold") && !Cue.StartsWith(TEXT("hit_")) && CueEventId && Elapsed < Duration)
    {
        PendingDeath = DeathClip;
        UE_LOG(LogFoundryRobot, Display, TEXT("ART_TERMINAL_QUEUED id=%llu event=%llu after=%s seconds=%.3f core_targetability=ended"), CoreId, EventId, *Cue, Duration - Elapsed);
    }
    else Play(DeathClip, EventId);
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

void AFoundryRobot::DisplayCue(const FString& Name, int32 HitIndex)
{
    const bool bHit = Name.Contains(TEXT("display_committed_hit")) || Name == TEXT("fouling_cable_spark");
    if (bHit)
    {
        if (HitIndex >= CommittedHits) return;
        Flash(TEXT("muzzle"), true, bRam || Definition == TEXT("C1-B01") ? 2.f : .65f);
        ++PresentedHits;
        UE_LOG(LogFoundryRobot, Display, TEXT("ART_MUZZLE id=%llu event=%llu clip=%s time=%.3f hit_index=%d committed_hits=%d socket=muzzle world=%s cosmetic_only=1"),
            CoreId, CueEventId, *ClipAsset, Elapsed, HitIndex, CommittedHits, *Body->GetSocketLocation(TEXT("muzzle")).ToString());
    }
    else if (Name == TEXT("steam_vent") || Name == TEXT("thermal_vent"))
    {
        SteamRemaining = .45f;
        SteamMesh->SetVisibility(true);
        SteamMaterial->SetScalarParameterValue(TEXT("Dissolve"), 0);
        UE_LOG(LogFoundryRobot, Display, TEXT("ART_STEAM id=%llu time=%.3f cosmetic_only=1"), CoreId, Elapsed);
    }
    else if (Name == TEXT("display_committed_summon") || Name == TEXT("display_committed_death_spawn"))
    {
        if (!bCommittedSummon) return;
        Flash(TEXT("muzzle"), false, 1.2f);
        ++PresentedSummons;
        UE_LOG(LogFoundryRobot, Display, TEXT("ART_SUMMON_CUE id=%llu event=%llu time=%.3f creates_gameplay_enemy=0"), CoreId, CueEventId, Elapsed);
    }
    else if (Name == TEXT("core_burst_cosmetic")) Flash(TEXT("core"), false, 2);
    else if (Name == TEXT("collapse_contact"))
    {
        UE_LOG(LogFoundryRobot, Display, TEXT("ART_COLLAPSE_CONTACT id=%llu time=%.3f"), CoreId, Elapsed);
    }
    else if (Name != TEXT("dissolve_start_all_components") && Name != TEXT("destroy_actor_and_fx") && Name != TEXT("despawn_without_death_fx"))
    {
        Flash(TEXT("core"), false, .8f);
        UE_LOG(LogFoundryRobot, Display, TEXT("ART_SUPPORT_CUE id=%llu event=%llu kind=%s time=%.3f cosmetic_only=1"), CoreId, CueEventId, *Name, Elapsed);
    }
}

void AFoundryRobot::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Elapsed += DeltaSeconds;
    if (RevealRemaining > 0)
    {
        RevealRemaining = FMath::Max(0.f, RevealRemaining - DeltaSeconds);
        if (RevealRemaining <= 0)
        {
            UpdateVisibility();
            UE_LOG(LogFoundryRobot, Display, TEXT("ART_SUMMON_REVEAL id=%llu"), CoreId);
        }
    }
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
        SteamMesh->SetWorldLocation(Body->GetSocketLocation(TEXT("core")) + FVector(55, 0, 80 + Progress * 80));
        SteamMesh->SetRelativeScale3D(FVector(.18f + Progress * .6f, .18f + Progress * .6f, .25f + Progress * .9f));
        SteamMaterial->SetScalarParameterValue(TEXT("Dissolve"), Progress);
        if (SteamRemaining <= 0) SteamMesh->SetVisibility(false);
    }
    const float Glow = bCharged ? 2.3f + .4f * FMath::Sin(GetWorld()->GetTimeSeconds() * 8) : 1.f;
    for (UMaterialInstanceDynamic* Material : Materials) Material->SetScalarParameterValue(TEXT("Glow"), Glow);
    // Metadata only schedules display. It never calls the rules engine.
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(FoundryVisualData::Cues); ++Index)
    {
        const auto& Row = FoundryVisualData::Cues[Index];
        if (ClipAsset == Row.Asset && Elapsed >= Row.At && !DisplayedCues.Contains(Index))
        {
            DisplayedCues.Add(Index);
            DisplayCue(Row.Event, Row.HitIndex);
        }
    }
    if (Cue == TEXT("death") || Cue == TEXT("death_release"))
    {
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
    if (Cue != TEXT("idle") && Cue != TEXT("charge_hold") && Elapsed >= Duration)
    {
        if (!PendingDeath.IsEmpty())
        {
            const FString Death = PendingDeath;
            PendingDeath.Empty();
            Play(Death, DeathEventId);
        }
        else if (!PendingReaction.IsEmpty())
        {
            const FString Reaction = PendingReaction;
            PendingReaction.Empty();
            Play(Reaction);
        }
        else if (bCharged) HoldCharge();
        else Play(TEXT("idle"));
    }
}

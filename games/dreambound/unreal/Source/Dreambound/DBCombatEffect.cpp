#include "DBCombatEffect.h"

#include "DBGameMode.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    FLinearColor EffectColor(EDBElement Element)
    {
        if (Element == EDBElement::Frost) return FLinearColor(0.2f, 0.8f, 1.f);
        if (Element == EDBElement::Ember) return FLinearColor(1.f, 0.2f, 0.025f);
        if (Element == EDBElement::Storm) return FLinearColor(0.6f, 0.3f, 1.f);
        return FLinearColor(1.f, 0.65f, 0.15f);
    }
}

ADBCombatEffect::ADBCombatEffect()
{
    PrimaryActorTick.bCanEverTick = true;
    EffectRoot = CreateDefaultSubobject<USceneComponent>(TEXT("EffectRoot"));
    SetRootComponent(EffectRoot);
    Shards = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ElementFragments"));
    Lines = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ElementLines"));
    for (UInstancedStaticMeshComponent* Component : { Shards.Get(), Lines.Get() })
    {
        Component->SetupAttachment(EffectRoot);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->SetCastShadow(false);
        Component->SetCanEverAffectNavigation(false);
    }
    // Resolve authored visuals on initialization so editor imports remain editable.
    Tags.Add(TEXT("DBTransientCombatEffect"));
}

ADBCombatEffect* ADBCombatEffect::Create(UWorld* World, FVector At)
{
    if (!World || At.ContainsNaN()) return nullptr;
    int32 Count = 0;
    for (TActorIterator<ADBCombatEffect> It(World); It; ++It) if (++Count >= 48) return nullptr;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    // No enemy owner or attachment: source death cannot remove an already-earned discharge.
    return World->SpawnActor<ADBCombatEffect>(At, FRotator::ZeroRotator, Params);
}

void ADBCombatEffect::SpawnBurst(UWorld* World, FVector At, EDBElement Element, int32 RoomId, float Scale)
{
    if (ADBCombatEffect* Effect = Create(World, At)) Effect->Initialize(Element, RoomId, Scale);
}

void ADBCombatEffect::SpawnArc(UWorld* World, FVector From, FVector To, int32 RoomId)
{
    if (To.ContainsNaN()) return;
    if (ADBCombatEffect* Effect = Create(World, From))
    {
        Effect->bArc = true;
        Effect->ArcEnd = To;
        Effect->Initialize(EDBElement::Storm, RoomId, 1.f);
    }
}

void ADBCombatEffect::Initialize(EDBElement Element, int32 RoomId, float Scale)
{
    Lines->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    EffectElement = Element;
    EffectRoomId = RoomId;
    EffectScale = FMath::Clamp(Scale, 0.5f, 2.f);
    Duration = bArc ? 0.55f : 0.65f;
    if (const ADBGameMode* Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode()))
    {
        Expedition = Mode->Expeditions;
        if (EffectRoomId == INDEX_NONE) EffectRoomId = Mode->CurrentRoomId;
    }
    if (UStaticMesh* Crystal = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/PreferredCombat/Meshes/SM_Crystal.SM_Crystal")))
        Shards->SetStaticMesh(Crystal);
    const TCHAR* MaterialPath = Element == EDBElement::Frost ? TEXT("/Game/Art/PreferredCombat/Materials/M_Frost.M_Frost")
        : Element == EDBElement::Ember ? TEXT("/Game/Art/PreferredCombat/Materials/M_Ember.M_Ember")
        : Element == EDBElement::Storm ? TEXT("/Game/Art/PreferredCombat/Materials/M_Storm.M_Storm")
        : TEXT("/Game/Art/PreferredCombat/Materials/M_Core.M_Core");
    if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, MaterialPath))
        for (int32 Index = 0; Index < Shards->GetNumMaterials(); ++Index) Shards->SetMaterial(Index, Material);
    if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Art/PreferredCombat/Materials/M_CombatGlow.M_CombatGlow")))
    {
        Glow = UMaterialInstanceDynamic::Create(Material, this);
        Lines->SetMaterial(0, Glow);
    }
    Rebuild();
}

void ADBCombatEffect::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (const ADBGameMode* Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode()))
    {
        if (Mode->bTitle || Mode->bDefeated || Mode->CurrentRoomId != EffectRoomId || Mode->Expeditions != Expedition)
        { Destroy(); return; }
        if (Mode->bPaused || Mode->bChoosingReward || Mode->bShowingBuild) return;
    }
    Age += DeltaSeconds;
    if (Age >= Duration) { Destroy(); return; }
    Rebuild();
}

void ADBCombatEffect::AddLine(FVector From, FVector To, float Width)
{
    const FVector Delta = To - From;
    if (Delta.IsNearlyZero()) return;
    Lines->AddInstance(FTransform(Delta.Rotation(), (From + To) * 0.5f,
        FVector(Delta.Size() / 100.f, Width / 100.f, Width / 100.f)), true);
}

void ADBCombatEffect::AddShard(FVector At, FRotator Rotation, FVector Size)
{
    if (!Shards->GetStaticMesh()) return;
    const FBoxSphereBounds Bounds = Shards->GetStaticMesh()->GetBounds();
    const FVector MeshSize = Bounds.BoxExtent * 2.f;
    const FVector Scale(Size.X / FMath::Max(1.0, MeshSize.X), Size.Y / FMath::Max(1.0, MeshSize.Y),
        Size.Z / FMath::Max(1.0, MeshSize.Z));
    Shards->AddInstance(FTransform(Rotation, At - Rotation.RotateVector(Bounds.Origin * Scale), Scale), true);
}

void ADBCombatEffect::Rebuild()
{
    Lines->ClearInstances(); Shards->ClearInstances();
    const float Progress = FMath::Clamp(Age / Duration, 0.f, 1.f);
    const float Fade = 1.f - Progress;
    if (Glow)
    {
        const FLinearColor Color = EffectColor(EffectElement) * (0.4f + Fade * 1.4f);
        Glow->SetVectorParameterValue(TEXT("Color"), Color);
        Glow->SetVectorParameterValue(TEXT("Tint"), Color);
    }
    const FVector Start = GetActorLocation();
    if (bArc)
    {
        const FVector Direction = (ArcEnd - Start).GetSafeNormal();
        FVector Side = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal();
        if (Side.IsNearlyZero()) Side = FVector::RightVector;
        FVector Previous = Start;
        for (int32 Index = 1; Index <= 8; ++Index)
        {
            const float Alpha = Index / 8.f;
            const FVector Jag = Index == 8 ? FVector::ZeroVector
                : Side * (Index % 2 ? 23.f : -19.f) + FVector::UpVector * (Index % 3 ? 12.f : -14.f);
            const FVector Point = FMath::Lerp(Start, ArcEnd, Alpha) + Jag * (0.8f + 0.2f * FMath::Sin(Age * 36.f));
            AddLine(Previous, Point, (3.f + Fade * 5.f));
            Previous = Point;
        }
        AddShard(ArcEnd, FRotator(Progress * 90.f, 30.f, 0.f), FVector(12.f, 12.f, 24.f) * Fade);
        return;
    }
    const float Travel = EffectScale * (12.f + Progress * 135.f);
    for (int32 Index = 0; Index < 14; ++Index)
    {
        const float Angle = Index * 2.f * PI / 14.f;
        const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.15f + (Index % 4) * 0.23f);
        const FVector Point = Start + Direction * Travel + FVector(0.f, 0.f, Age * 75.f - Age * Age * 145.f);
        AddShard(Point, FRotator(Index * 29.f + Progress * 140.f, Index * 37.f, Progress * 95.f),
            FVector(7.f, 9.f, 20.f) * (EffectScale * FMath::Max(0.02f, Fade)));
    }
    for (int32 Index = 0; Index < 24; ++Index)
    {
        const float Angle = Index * 2.f * PI / 24.f;
        const float Next = (Index + 0.72f) * 2.f * PI / 24.f;
        AddLine(Start + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Travel,
            Start + FVector(FMath::Cos(Next), FMath::Sin(Next), 0.f) * Travel, FMath::Max(0.4f, Fade * 4.f));
    }
}

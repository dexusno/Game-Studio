#include "FoundryHost.h"
#include "FoundrySession.h"
#include "FoundryRobot.h"
#include "FoundryMara.h"
#include "FoundryCampaign.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InputComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "StaticMeshResources.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "UObject/UObjectIterator.h"
#include <algorithm>
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogFoundryHost, Log, All);

namespace
{
AFoundryStage* FindStage(UWorld* World)
{
    for (TActorIterator<AFoundryStage> Stage(World); Stage; ++Stage)
    {
        return *Stage;
    }
    return nullptr;
}

AFoundryStage* KeyboardStage(UWorld* World)
{
    AFoundryStage* Stage=FindStage(World);
#if FOUNDRY_WITH_CAMPAIGN
    if(Stage && Stage->GetCampaign())
    {
        const auto& Campaign=*Stage->GetCampaign();
        if(Campaign.bPrecisionModal || Campaign.Page!=TEXT("combat") || !Campaign.Drawer.IsEmpty() || Stage->IsPresentationBusy())return nullptr;
        const auto* C=Campaign.Current();
        if(!C || !C->fight.upgradeChoices.empty() || std::any_of(C->upgradeOffers.begin(),C->upgradeOffers.end(),[](const auto& O){return !O.deferred;}))return nullptr;
    }
#endif
    return Stage;
}

void SynchronizeStaticRenderBounds(UWorld* World)
{
    // UE5.8's editor CalculateExtendedBounds prefers cached MeshDescription
    // bounds before BuildScale. Our pivot-preserving metre FBX imports have
    // centimetre render geometry: tiny stale bounds cause self-occlusion.
    // Match the actual render bounds, including their origin, before visibility.
    TSet<UStaticMesh*> Corrected;
    int32 Components = 0;
    for (TObjectIterator<UStaticMeshComponent> It; It; ++It)
    {
        UStaticMeshComponent* Component = *It;
        if (Component->GetWorld() != World) continue;
        UStaticMesh* Mesh = Component->GetStaticMesh();
        if (!Mesh || !Mesh->GetRenderData()) continue;
        const FBoxSphereBounds Actual(Mesh->GetRenderData()->Bounds);
        const FBoxSphereBounds Cached = Mesh->GetBounds();
        if (!Cached.BoxExtent.Equals(Actual.BoxExtent, .01) || !Cached.Origin.Equals(Actual.Origin, .01))
        {
            Mesh->SetExtendedBounds(Actual);
            Corrected.Add(Mesh);
        }
        if (Corrected.Contains(Mesh))
        {
            Component->UpdateBounds();
            Component->MarkRenderStateDirty();
            ++Components;
        }
    }
    UE_LOG(LogFoundryHost, Display, TEXT("ART_RENDER_BOUNDS_SYNC assets=%d components=%d"), Corrected.Num(), Components);
}

void ComposeCinderwallDepth(UWorld* World)
{
    // Keep the contact deck and nearby machinery in place. The factory layer
    // sits beyond the canal, leaving a visible horizon for the fixed far city.
    AStaticMeshActor* ContactDeck = nullptr;
    for (TActorIterator<AStaticMeshActor> Actor(World); Actor; ++Actor)
    {
        if (!Actor->ActorHasTag(TEXT("CinderwallSceneryV002"))) continue;
        UStaticMesh* Mesh = Actor->GetStaticMeshComponent()->GetStaticMesh();
        if (!Mesh || !Mesh->GetRenderData()) continue;
        if (Mesh->GetName().Contains(TEXT("combat_deck"))) ContactDeck = *Actor;
        if (Mesh->GetName().Contains(TEXT("outer_apron")))
        {
            // The lower industrial yard supports the distant buildings and
            // closes the view beneath the combat bridge.
            auto* Component = Actor->GetStaticMeshComponent();
            Component->SetMobility(EComponentMobility::Movable);
            Actor->AddActorWorldOffset(FVector(0, 0, -700));
            Component->SetMobility(EComponentMobility::Static);
            for (int32 Slot = 0; Slot < Component->GetNumMaterials(); ++Slot)
                if (auto* Ground = Component->CreateDynamicMaterialInstance(Slot))
                {
                    Ground->SetScalarParameterValue(TEXT("UseTexture"), 0);
                    Ground->SetVectorParameterValue(TEXT("FlatColor"), FLinearColor(.025f, .035f, .043f));
                    Ground->SetScalarParameterValue(TEXT("Roughness"), .94f);
                    Ground->SetScalarParameterValue(TEXT("Metallic"), .05f);
                    Ground->SetScalarParameterValue(TEXT("NormalStrength"), 0);
                }
        }
        else if (Mesh->GetName().Contains(TEXT("factory_")))
        {
            const FVector Center = Actor->GetActorTransform().TransformPosition(FVector(Mesh->GetRenderData()->Bounds.Origin));
            // Imported scenery is static. Permit this one startup placement,
            // then restore static mobility before the first visible frame.
            Actor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
            Actor->AddActorWorldOffset(FVector(Center.X * 1.2, Center.Y * 1.2, -700));
            Actor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Static);
            UE_LOG(LogFoundryHost, Display, TEXT("SCENERY_DEPTH asset=%s offset=%s"), *Mesh->GetName(), *Actor->GetActorLocation().ToString());
        }
    }
    if (ContactDeck)
    {
        // Continue the authored paving beneath the wider shooting view. These
        // adjacent static sections are scenery, with no collision or targeting.
        const FVector Offsets[] = {FVector(2300, 0, 0), FVector(0, 750, 0), FVector(2300, 750, 0)};
        auto* Source = ContactDeck->GetStaticMeshComponent();
        for (const FVector& Offset : Offsets)
        {
            auto* Extension = World->SpawnActor<AStaticMeshActor>(ContactDeck->GetActorLocation() + Offset, ContactDeck->GetActorRotation());
            auto* Component = Extension->GetStaticMeshComponent();
            Component->SetMobility(EComponentMobility::Movable);
            Component->SetStaticMesh(Source->GetStaticMesh());
            for (int32 Slot = 0; Slot < Source->GetNumMaterials(); ++Slot) Component->SetMaterial(Slot, Source->GetMaterial(Slot));
            Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Component->SetMobility(EComponentMobility::Static);
            Extension->Tags.Add(TEXT("CinderwallDepthExtension"));
            UE_LOG(LogFoundryHost, Display, TEXT("SCENERY_DECK_EXTENSION offset_cm=%s source=%s collision=0"), *Offset.ToString(), *Source->GetStaticMesh()->GetName());
        }
    }
}
}

AFoundryStage::AFoundryStage()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("StageRoot")));
}

AFoundryStage::~AFoundryStage() = default;

void AFoundryStage::EndPlay(const EEndPlayReason::Type Reason)
{
    if(Audio)Audio->StopAll();
    Super::EndPlay(Reason);
}

void AFoundryStage::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogFoundryHost, Display, TEXT("FOUNDRY_CORE_BUILD digest=%s review_snapshot=%d"), UTF8_TO_TCHAR(FOUNDRY_CORE_SOURCE_DIGEST), FOUNDRY_CORE_REVIEW_SNAPSHOT);
    uint64 Seed = 1;
    FParse::Value(FCommandLine::Get(), TEXT("FoundrySeed="), Seed);
    Session = MakeUnique<FFoundrySession>(Seed);
    Audio = MakeUnique<FFoundryAudio>(this);
    if (FParse::Param(FCommandLine::Get(), TEXT("FoundryInputProbe")))
    {
        FString Report;
        const bool bPassed = Session->RunInputProbe(Report);
        UE_LOG(LogFoundryHost, Display, TEXT("FOUNDRY_UI_INPUT_PROBE ok=%d %s"), bPassed, *Report);
        ensureAlwaysMsgf(bPassed, TEXT("Presentation command-path probe failed: %s"), *Report);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("FoundryCoreFixture")))
    {
        auto Fixture = overkill::Rules::teachingEncounter(1, false);
        const auto Result = overkill::playTeachingFixture(Fixture, Session->Rules);
        FString Json;
        for (const auto& Event : Result.events)
        {
            const FString Line = UTF8_TO_TCHAR(overkill::eventJson(Event).c_str());
            Json += Line + TEXT("\n");
            UE_LOG(LogFoundryHost, Display, TEXT("PARITY_EVENT %s"), *Line);
        }
        const FString Hash = UTF8_TO_TCHAR(overkill::stateHash(Fixture).c_str());
        Json += FString::Printf(TEXT("{\"fixture\":\"mite-ram-v1.1\",\"ok\":%s,\"hp\":%d,\"round\":%d,\"shots\":%d,\"state_hash\":\"%s\"}\n"),
            Result.ok ? TEXT("true") : TEXT("false"), Fixture.hp, Fixture.round, Fixture.shots, *Hash);
        const FString Folder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Parity"));
        IFileManager::Get().MakeDirectory(*Folder, true);
        const bool bWritten = FFileHelper::SaveStringToFile(Json, *FPaths::Combine(Folder, TEXT("core-fixture.jsonl")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        UE_LOG(LogFoundryHost, Display, TEXT("FOUNDRY_CORE_FIXTURE ok=%d hash=%s events=%llu written=%d"), Result.ok, *Hash, static_cast<uint64>(Result.events.size()), bWritten);
        ensureAlwaysMsgf(Result.ok && bWritten, TEXT("Shared core fixture failed."));
    }
    bSmokeTest = FParse::Param(FCommandLine::Get(), TEXT("FoundrySmoke"));
    bArtProbe = FParse::Param(FCommandLine::Get(), TEXT("FoundryArtProbe"));
    bRosterProbe = FParse::Param(FCommandLine::Get(), TEXT("FoundryRosterProbe"));
    bCampaignProbe = FParse::Param(FCommandLine::Get(), TEXT("FoundryCampaignProbe"));
#if FOUNDRY_WITH_CAMPAIGN
    bTechnicalMode = bSmokeTest || bArtProbe || bRosterProbe || FParse::Param(FCommandLine::Get(), TEXT("FoundryTeaching"));
    if (!bTechnicalMode)
    {
        FString SavePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Campaign/profile.ofsave"));
        const bool bExplicitSave = FParse::Value(FCommandLine::Get(), TEXT("FoundrySave="), SavePath);
        const bool bFixedSave = bExplicitSave || bCampaignProbe || FParse::Param(FCommandLine::Get(), TEXT("FoundryInspectSave"));
        Campaign = MakeUnique<FFoundryCampaign>(*Session, FPaths::ConvertRelativePathToFull(SavePath), bFixedSave);
#if !UE_BUILD_SHIPPING
        if(FParse::Param(FCommandLine::Get(),TEXT("FoundryInspectSave")) && Campaign->Current())
        {
            Campaign->Page=Campaign->PhasePage();++Campaign->SceneRevision;
            Campaign->Message=TEXT("Prepared UI fixture · inspection only");
        }
#endif
    }
#endif
    int32 StageActorCount = 0;
    int32 MaraStageCount = 0;
    int32 SceneryActorCount = 0;
    int32 BackdropActorCount = 0;
    TSet<UStaticMesh*> StageMeshes;
    for (TActorIterator<AStaticMeshActor> Actor(GetWorld()); Actor; ++Actor)
    {
        if (Actor->ActorHasTag(TEXT("MaraGenerated"))) ++MaraStageCount;
        if (Actor->ActorHasTag(TEXT("CinderwallSceneryV002"))) ++SceneryActorCount;
        if (Actor->ActorHasTag(TEXT("CinderwallBackdropV001"))) ++BackdropActorCount;
        if (!Actor->ActorHasTag(TEXT("CinderwallGenerated"))) continue;
        ++StageActorCount;
        UStaticMesh* Mesh = Actor->GetStaticMeshComponent()->GetStaticMesh();
        StageMeshes.Add(Mesh);
        if (Mesh->GetName().Contains(TEXT("rear_claw_mount"))) Actor->SetActorHiddenInGame(true);
        if (Mesh->GetName().Contains(TEXT("SM_CW_furnace_")))
        {
            // Background furnaces support the hero silhouettes rather than
            // presenting seven equally bright orange panels behind them.
            const int32 Bay = FMath::RoundToInt((Actor->GetActorLocation().X + 900) / 300);
            const float Glow[] = {.22f, .40f, .14f, .28f, .48f, .18f, .32f};
            for (int32 Slot = 0; Slot < Actor->GetStaticMeshComponent()->GetNumMaterials(); ++Slot)
                if (Actor->GetStaticMeshComponent()->GetMaterial(Slot)->GetName().Contains(TEXT("MI_CW_furnace")))
                    Actor->GetStaticMeshComponent()->CreateDynamicMaterialInstance(Slot)->SetScalarParameterValue(TEXT("Glow"), Glow[FMath::Clamp(Bay, 0, 6)]);
        }
        if (Mesh->GetName().Contains(TEXT("SM_CW_deck_")))
        {
            const FVector Extent = Mesh->GetRenderData()->Bounds.BoxExtent;
            checkf(FMath::IsNearlyEqual(Extent.X, 98.0, 1.0) && FMath::IsNearlyEqual(Extent.Y, 98.0, 1.0), TEXT("Cinderwall render geometry is not centimetre scale"));
        }
    }
    checkf((SceneryActorCount == 19 && StageActorCount == 0 && MaraStageCount == 1 && BackdropActorCount == 1) ||
        (SceneryActorCount == 0 && StageActorCount == 47 && StageMeshes.Num() == 18 && MaraStageCount == 10),
        TEXT("Stage must be complete legacy Art+MaraArt or SceneryArt+BackdropArt with its Mara hopper"));
    UE_LOG(LogFoundryHost, Display, TEXT("ART_STAGE legacy_placements=%d legacy_meshes=%d scenery_v002=%d mara_stage=%d"), StageActorCount, StageMeshes.Num(), SceneryActorCount, MaraStageCount);
    ComposeCinderwallDepth(GetWorld());
    Mara = GetWorld()->SpawnActor<AFoundryMara>();
    Mara->Initialize();
    if (bTechnicalMode) SpawnRobots();

    GetWorld()->SpawnActor<ASkyAtmosphere>();
    AExponentialHeightFog* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>(FVector(0, 0, -250), FRotator::ZeroRotator);
    Fog->GetComponent()->SetFogDensity(.018f);
    Fog->GetComponent()->SetFogHeightFalloff(.15f);
    Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(.18f, .24f, .30f));
    Fog->GetComponent()->SetFogMaxOpacity(.78f);
    Fog->GetComponent()->SetStartDistance(1200.f);
    ADirectionalLight* Key = GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-40, 35, 0));
    CastChecked<UDirectionalLightComponent>(Key->GetLightComponent())->SetAtmosphereSunLight(true);
    Key->GetLightComponent()->SetIntensity(3.3f);
    Key->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.82f, 0.64f));
    APointLight* Warm = GetWorld()->SpawnActor<APointLight>(FVector(-300, 230, 420), FRotator::ZeroRotator);
    Warm->PointLightComponent->SetIntensity(55000.0f);
    Warm->PointLightComponent->SetAttenuationRadius(1250.0f);
    Warm->PointLightComponent->SetLightColor(FLinearColor(1.0f, 0.50f, 0.27f));
    APointLight* Cool = GetWorld()->SpawnActor<APointLight>(FVector(650, -240, 380), FRotator::ZeroRotator);
    Cool->PointLightComponent->SetIntensity(50000.0f);
    Cool->PointLightComponent->SetAttenuationRadius(1700.0f);
    Cool->PointLightComponent->SetLightColor(FLinearColor(0.48f, 0.70f, 1.0f));
    ASkyLight* Fill = GetWorld()->SpawnActor<ASkyLight>();
    Fill->GetLightComponent()->SetIntensity(0.60f);
    Fill->GetLightComponent()->SetRealTimeCaptureEnabled(true);
    Fill->GetLightComponent()->RecaptureSky();

    const FVector PrepareLocation(-130, 2230, 530);
    const FVector PrepareFocus(-130, -60, 155);
    PreparationCamera = GetWorld()->SpawnActor<ACameraActor>(PrepareLocation, (PrepareFocus - PrepareLocation).Rotation());
    PreparationCamera->GetCameraComponent()->SetFieldOfView(52.0f);
    const FVector ActionLocation(-1250, 965, 340);
    const FVector ActionFocus(220, 0, 150);
    ActionCamera = GetWorld()->SpawnActor<ACameraActor>(ActionLocation, (ActionFocus - ActionLocation).Rotation());
    ActionCamera->GetCameraComponent()->SetFieldOfView(57.0f);
#if WITH_EDITOR
    // Editor -game may rebuild uncooked meshes, textures and animation data.
    // Start the visible encounter only after those loaded products are ready.
    FAssetCompilingManager::Get().FinishAllCompilation();
    UE_LOG(LogFoundryHost, Display, TEXT("ART_ASSET_WARMUP remaining=%d"), FAssetCompilingManager::Get().GetNumRemainingAssets());
#endif
    SynchronizeStaticRenderBounds(GetWorld());
    for (TActorIterator<AStaticMeshActor> Actor(GetWorld()); Actor; ++Actor)
    {
        if (Actor->ActorHasTag(TEXT("CinderwallBackdropV001")))
        {
            const auto* Component = Actor->GetStaticMeshComponent();
            const FVector Render = Component->GetStaticMesh()->GetRenderData()->Bounds.BoxExtent * 2;
            const FVector Expected(30598.37, 20694.95, 14428.06);
            checkf(Render.Equals(Expected, 2) && Component->Bounds.BoxExtent.Equals(Render * .5, 1), TEXT("Far-city source/render/culling dimensions must agree in centimetres"));
            UE_LOG(LogFoundryHost, Display, TEXT("BACKDROP_RUNTIME_BOUNDS render_cm=%s fixed_world=1 ok=1"), *Render.ToString());
        }
        if (Actor->ActorHasTag(TEXT("CinderwallGenerated")) && Actor->GetStaticMeshComponent()->GetStaticMesh()->GetName().Contains(TEXT("SM_CW_deck_")))
            checkf(FMath::IsNearlyEqual(Actor->GetStaticMeshComponent()->Bounds.BoxExtent.X, 98.0, 1.0), TEXT("Deck component culling bounds must match rendered centimetres"));
        if (Actor->ActorHasTag(TEXT("CinderwallSceneryV002")))
        {
            FVector Expected = FVector::ZeroVector;
            for (const FName& Tag : Actor->Tags)
            {
                FString Value = Tag.ToString();
                if (!Value.RemoveFromStart(TEXT("SceneryBoundsCm="))) continue;
                TArray<FString> Parts; Value.ParseIntoArray(Parts, TEXT(","));
                checkf(Parts.Num() == 3, TEXT("Malformed source scenery bounds tag"));
                Expected = FVector(FCString::Atod(*Parts[0]), FCString::Atod(*Parts[1]), FCString::Atod(*Parts[2]));
            }
            const auto* Component = Actor->GetStaticMeshComponent();
            const FVector Render = Component->GetStaticMesh()->GetRenderData()->Bounds.BoxExtent * 2;
            const FVector Culling = Component->Bounds.BoxExtent * 2;
            checkf(!Expected.IsNearlyZero() && Render.Equals(Expected, 1.5) && Culling.Equals(Render, 1.5), TEXT("Scenery source/render/culling centimetre bounds differ: %s"), *Actor->GetName());
            UE_LOG(LogFoundryHost, Display, TEXT("SCENERY_RUNTIME_BOUNDS asset=%s source_cm=%s render_cm=%s culling_cm=%s ok=1"), *Component->GetStaticMesh()->GetName(), *Expected.ToString(), *Render.ToString(), *Culling.ToString());
        }
    }
    SetActionView(false, true);
    if (bSmokeTest || bArtProbe || bRosterProbe) Session->bShowPanels = false;
    UE_LOG(LogFoundryHost, Display, TEXT("Host P13 ready. Cinderwall-v001 shared-core Mara teaching encounter. Smoke=%d ArtProbe=%d"), bSmokeTest, bArtProbe);
}

void AFoundryStage::SpawnRobots()
{
    for (AFoundryRobot* Robot : Robots) if (IsValid(Robot)) Robot->Destroy();
    Robots.Empty();
    int32 Index = 0;
    for (const auto& Enemy : Session->State.enemies)
    {
        const int32 PositionIndex = Index++;
        if (Enemy.dead || Enemy.escaped) continue;
        const FVector Position = RobotPosition(PositionIndex);
        AFoundryRobot* Robot = GetWorld()->SpawnActor<AFoundryRobot>(Position, FRotator::ZeroRotator);
        Robot->Initialize(Enemy.id, UTF8_TO_TCHAR(Enemy.definition.c_str()), Enemy.maxHp, Enemy.tiles, Enemy.robotAction == "blast");
        Robots.Add(Robot);
    }
    FrameRoster();
}

void AFoundryStage::SpawnMissingRobots()
{
    int32 Index = 0;
    for (const auto& Enemy : Session->State.enemies)
    {
        const int32 PositionIndex = Index++;
        if (Enemy.dead || Enemy.escaped) continue;
        bool Exists = false;
        for (const AFoundryRobot* Robot : Robots) if (IsValid(Robot) && Robot->GetCoreId() == Enemy.id) { Exists = true; break; }
        if (Exists) continue;
        auto* Robot = GetWorld()->SpawnActor<AFoundryRobot>(RobotPosition(PositionIndex), FRotator::ZeroRotator);
        Robot->Initialize(Enemy.id, UTF8_TO_TCHAR(Enemy.definition.c_str()), Enemy.maxHp, Enemy.tiles, Enemy.robotAction == "blast");
        const auto Pending = [&](const std::vector<overkill::Event>& Events)
        {
            for (const auto& Event : Events) if (Event.type == "robot_deployed" && Event.target == Enemy.id) return true;
            return false;
        };
        if (Pending(Session->CommittedEvents) || Pending(QueuedEvents)) Robot->AwaitReveal();
        Robots.Add(Robot);
        UE_LOG(LogFoundryHost, Display, TEXT("CAMPAIGN_ROBOT_SPAWN id=%llu definition=%s"), Enemy.id, UTF8_TO_TCHAR(Enemy.definition.c_str()));
    }
}

FVector AFoundryStage::RobotPosition(int32 Index) const
{
    // Visual staging only. Keep the single small Mite in the nearer lane and
    // separate its larger companions behind it. Dead entries retain their slot;
    // core IDs, formation order, targeting and hit rules do not change.
    if (Session && Index < static_cast<int32>(Session->State.enemies.size()) && Session->State.enemies[Index].definition == "C1-R01")
        return FVector(300, -180, 1.5);
    int32 LargeIndex = 0;
    if (Session) for (int32 I = 0; I < Index && I < static_cast<int32>(Session->State.enemies.size()); ++I)
        if (Session->State.enemies[I].definition != "C1-R01") ++LargeIndex;
    static const FVector Slots[] = {FVector(560, -20, 1.5), FVector(960, 220, 1.5), FVector(940, -230, 1.5)};
    return Slots[LargeIndex % UE_ARRAY_COUNT(Slots)] + FVector(160 * (LargeIndex / UE_ARRAY_COUNT(Slots)), -340 * (LargeIndex / UE_ARRAY_COUNT(Slots)), 0);
}

void AFoundryStage::FrameRoster()
{
    if (!PreparationCamera || !ActionCamera || !Session || Session->State.enemies.empty()) return;
    const bool Wide = Session->State.enemies.size() > 2;
    if (Wide == bWideRoster) return;
    bWideRoster = Wide;
    const FVector PrepLocation = Wide ? FVector(70, 2430, 580) : FVector(-130, 2230, 530);
    const FVector PrepFocus = Wide ? FVector(70, -60, 155) : FVector(-130, -60, 155);
    const FVector ShotLocation = Wide ? FVector(-1400, 1050, 370) : FVector(-1250, 965, 340);
    const FVector ShotFocus(220, 0, 150);
    PreparationCamera->SetActorLocationAndRotation(PrepLocation, (PrepFocus - PrepLocation).Rotation());
    ActionCamera->SetActorLocationAndRotation(ShotLocation, (ShotFocus - ShotLocation).Rotation());
    UE_LOG(LogFoundryHost, Display, TEXT("CAMERA_ROSTER wide=%d bodies=%d presentation_only=1"), Wide, static_cast<int32>(Session->State.enemies.size()));
}

#if FOUNDRY_WITH_CAMPAIGN
void AFoundryStage::RefreshCampaignWorld()
{
    if (!Campaign) return;
    if (SeenSceneRevision != Campaign->SceneRevision)
    {
        if(Audio)Audio->StopAll();
        SeenSceneRevision = Campaign->SceneRevision;
        for (AFoundryRobot* Robot : Robots) if (IsValid(Robot)) Robot->Destroy();
        Robots.Empty(); Mara->ResetPresentation(); ResetActionPresentation();
        const auto* C = Campaign->Current();
        if (C && C->phase == overkill::CityPhase::Fight && Campaign->Page != TEXT("title")) SpawnRobots();
        SetActionView(!Session->State.bullet.empty(), true);
    }
    if(const auto* C=Campaign->Current(); C && C->phase==overkill::CityPhase::Fight)
        SpawnMissingRobots();
    for (AFoundryRobot* Robot : Robots) if (IsValid(Robot)) Robot->SetSceneHidden(Campaign->Page == TEXT("title"));
    if (Campaign->Page == TEXT("title") && !bWasTitle)
    {
        ResetActionPresentation();
        Session->CommittedEvents.clear();
        if (Audio) Audio->StopAll();
        Mara->ResetPresentation();
        SetActionView(false, true);
    }
    bWasTitle = Campaign->Page == TEXT("title");
    FrameRoster();
}
#endif

void AFoundryStage::PresentCommittedEvents()
{
    if (!Session || Session->CommittedEvents.empty()) return;
    // The saved core result is already final. Only presentation waits for the
    // End Turn camera; every committed event retains its original order/ID.
    if (!QueuedEvents.empty())
    {
        QueuedEvents.insert(QueuedEvents.end(), Session->CommittedEvents.begin(), Session->CommittedEvents.end());
        Session->CommittedEvents.clear();
        return;
    }
    PresentEvents(Session->CommittedEvents);
    Session->CommittedEvents.clear();
}

void AFoundryStage::PresentEvents(const std::vector<overkill::Event>& Events)
{
    if(Audio) Audio->Present(Events);
    SpawnMissingRobots();
    TSet<uint64> Died;
    TSet<uint64> DeathReleasing;
    TSet<uint64> SummonParents;
    TSet<uint64> SummonSources;
    TMap<uint64, int32> CommittedHits;
    TSet<uint64> NamedActionParents;
    for (const auto& Event : Events) if (Event.type == "enemy_death") Died.Add(Event.target);
    if (!Died.IsEmpty())
    {
        // Utility/support kills need the same complete death/fade window as a
        // main shot. Keep the current viewpoint; only presentation/input waits.
        ReturnCameraAfter = FMath::Max(ReturnCameraAfter, 2.25f);
        if (Session->State.phase == overkill::Phase::Victory) ActionCaption = TEXT("Securing salvage…");
        else if (Session->State.phase == overkill::Phase::Defeat) ActionCaption = TEXT("Systems failing…");
        else if (ActionCaption != TEXT("Firing…") && ActionCaption != TEXT("Enemy turn")) ActionCaption = TEXT("Enemy destroyed");
#if FOUNDRY_WITH_CAMPAIGN
        if (Campaign) ++Campaign->ViewRevision;
#endif
    }
    for (const auto& Event : Events) if (Event.type.rfind("robot_action:", 0) == 0) NamedActionParents.Add(Event.parent);
    for (const auto& Event : Events)
    {
        if (Event.type == "death_release") DeathReleasing.Add(Event.subject);
        if (Event.type == "robot_deployed") { SummonParents.Add(Event.parent); SummonSources.Add(Event.subject); }
        if (Event.type == "player_damage") ++CommittedHits.FindOrAdd(Event.parent);
    }
    for (const auto& Event : Events)
    {
        UE_LOG(LogFoundryHost, Verbose, TEXT("PRESENT_EVENT id=%llu type=%s action_view=%d transition=%.3f time=%.3f"), Event.id, UTF8_TO_TCHAR(Event.type.c_str()), bActionView, CameraTransitionRemaining, GetWorld()->GetTimeSeconds());
        if (Event.type == "collected") Mara->Collect(Event.id, Event.amount);
        else if (Event.type == "loaded") Mara->Load(Event.id, Event.amount);
        else if (Event.type == "unload") Mara->Unload(Event.id);
        else if (Event.type == "fire")
        {
            UE_LOG(LogFoundryHost, Display, TEXT("SHOT_PRESENT event=%llu action_view=%d settled=%d time=%.3f"), Event.id, bActionView, CameraTransitionRemaining <= 0, GetWorld()->GetTimeSeconds());
            for (AFoundryRobot* Robot : Robots)
                if (IsValid(Robot) && Robot->GetCoreId() == Event.target) Mara->Fire(Event.id, Event.amount, Robot->GetImpactLocation());
        }
        for (AFoundryRobot* Robot : Robots)
        {
            if (!IsValid(Robot)) continue;
            const uint64 Id = Robot->GetCoreId();
            if (Event.type == "robot_deployed" && Event.target == Id)
            {
                float Delay = DeathReleasing.Contains(Event.subject) ? .42f : .66f;
                for (const AFoundryRobot* Source : Robots) if (IsValid(Source) && Source->GetCoreId() == Event.subject) Delay += Source->GetTerminalDelay();
                Robot->RevealAfter(Delay);
            }
            if (Event.type == "enemy_death" && Event.target == Id) Robot->Die(Event.id, DeathReleasing.Contains(Id), SummonSources.Contains(Id));
            // Suppress redundant flinches before a death, but preserve an enemy
            // action that actually happened before Burn or a counter killed it.
            else if (Died.Contains(Id) && (Event.type == "hit" || Event.type == "status_damage")) continue;
            else if ((Event.type == "hit" || Event.type == "status_damage") && Event.target == Id) Robot->Hit(Event.amount, Event.secondary, Event.id);
            else if (Event.type.rfind("robot_action:", 0) == 0 && Event.subject == Id)
                Robot->NamedAction(UTF8_TO_TCHAR(Event.type.substr(13).c_str()), Event.id, CommittedHits.FindRef(Event.parent), SummonParents.Contains(Event.parent));
            else if (Event.type == "enemy_action" && Event.subject == Id && !NamedActionParents.Contains(Event.id)) Robot->EnemyAction(Event.amount, Event.id);
            else if (Event.type == "enemy_escape" && Event.subject == Id) Robot->Escape(Event.id);
        }
    }
    for (AFoundryRobot* Robot : Robots) if (IsValid(Robot))
        for (const auto& Enemy : Session->State.enemies) if (Enemy.id == Robot->GetCoreId()) Robot->SetTiles(Enemy.tiles);
}

void AFoundryStage::PlayPresentationCue(const FString& Cue) { if (Audio) Audio->PlayCue(FName(*Cue)); }

void AFoundryStage::ResetActionPresentation()
{
    // Discarding cosmetic events on menu/restart must not strand a real helper
    // behind an animation reveal that can no longer be delivered.
    for (AFoundryRobot* Robot : Robots) if (IsValid(Robot)) Robot->CompletePendingReveal();
    QueuedEvents.clear();
    CameraTransitionRemaining = ReturnCameraAfter = 0;
}

bool AFoundryStage::HasTerminalPresentation() const
{
    for (const AFoundryRobot* Robot : Robots) if (IsValid(Robot) && Robot->IsTerminal()) return true;
    return false;
}

void AFoundryStage::AimAtSelectedTarget()
{
    if (!Mara || !Session || ReturnCameraAfter > 0) return;
    for (const AFoundryRobot* Robot : Robots)
        if (IsValid(Robot) && Robot->GetCoreId() == Session->Target) { Mara->AimAt(Robot->GetImpactLocation()); break; }
}

void AFoundryStage::Control(const FString& Command)
{
    if (!Session) return;
    if (IsPresentationBusy() && Command != TEXT("diagnostic") && Command != TEXT("restart")) return;
    bool bCommitted = false;
#if FOUNDRY_WITH_CAMPAIGN
    if (Campaign) bCommitted = Campaign->Control(Command);
    else
#endif
        bCommitted = Session->Control(Command);
    FinishCommandPresentation(Command, bCommitted);
}

#if FOUNDRY_WITH_CAMPAIGN
bool AFoundryStage::SubmitCampaignAction(const overkill::CampaignAction& Action, std::vector<overkill::Event>& OutEvents)
{
    OutEvents.clear();
    if (!Campaign || !Session || IsPresentationBusy() || !Session->CommittedEvents.empty()) return false;
    const auto* Current = Campaign->Current();
    // The UI adapter supplies these fields for ordinary controls. A typed
    // replay must already identify its exact transaction, rather than silently
    // accepting the adapter's replacement of an incorrect run/sequence.
    if (!Current || Action.runId != Current->runId || Action.sequence != Current->nextTransaction) return false;
    if (!Campaign->Apply(Action)) return false;
    OutEvents = Session->CommittedEvents;
    FString Command;
    if (Action.type == overkill::CampaignActionType::Combat)
    {
        switch (Action.combat.type)
        {
        case overkill::ActionType::Load: Command = TEXT("load"); break;
        case overkill::ActionType::Unload: Command = TEXT("unload"); break;
        case overkill::ActionType::Fire: Command = TEXT("fire"); break;
        case overkill::ActionType::EndTurn: Command = TEXT("end"); break;
        default: break;
        }
    }
    FinishCommandPresentation(Command, true);
    return true;
}
#endif

void AFoundryStage::FinishCommandPresentation(const FString& Command, bool bCommitted)
{
#if FOUNDRY_WITH_CAMPAIGN
    // A new encounter's actors must exist before its already-committed events
    // are displayed. This synchronization cannot mutate the rules state.
    if (Campaign && bCommitted) RefreshCampaignWorld();
#endif
    if (bCommitted && Command == TEXT("load"))
    {
        SetActionView(true);
        CameraTransitionRemaining = FMath::Max(CameraTransitionRemaining, .70f);
        ActionCaption = TEXT("Locking and loading…");
    }
    else if (bCommitted && Command == TEXT("unload")) SetActionView(false);
    else if (bCommitted && Command == TEXT("fire"))
    {
        ActionCaption = TEXT("Firing…");
        ReturnCameraAfter = 2.25f;
#if FOUNDRY_WITH_CAMPAIGN
        if (Campaign) ++Campaign->ViewRevision;
#endif
    }
    else if (bCommitted && Command == TEXT("end"))
    {
        SetActionView(true);
        ActionCaption = TEXT("Enemy turn");
        if (CameraTransitionRemaining > 0)
        {
            QueuedEvents = std::move(Session->CommittedEvents);
            Session->CommittedEvents.clear();
            UE_LOG(LogFoundryHost, Display, TEXT("ENEMY_PRESENT_QUEUED events=%d core_committed=1 time=%.3f"), static_cast<int32>(QueuedEvents.size()), GetWorld()->GetTimeSeconds());
        }
        else ReturnCameraAfter = 2.25f;
    }
    PresentCommittedEvents();
    AimAtSelectedTarget();
    if (bTechnicalMode && Command == TEXT("restart")) { if(Audio)Audio->StopAll(); SpawnRobots(); Mara->ResetPresentation(); ResetActionPresentation(); SetActionView(false, true); }
}

void AFoundryStage::SetActionView(bool bAction, bool bInstant)
{
    const bool Changed = bActionView != bAction;
    bActionView = bAction;
    CameraTransitionRemaining = bInstant || !Changed ? 0 : .65f;
    ActionCaption = bAction ? TEXT("Taking aim…") : TEXT("Returning to preparation…");
#if FOUNDRY_WITH_CAMPAIGN
    if (Campaign) { ++Campaign->ViewRevision; if (bAction) Campaign->Drawer.Empty(); }
#endif
    if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
    {
        Controller->SetViewTargetWithBlend(bAction ? ActionCamera.Get() : PreparationCamera.Get(), CameraTransitionRemaining,
                                           EViewTargetBlendFunction::VTBlend_Cubic);
    }
    UE_LOG(LogFoundryHost, Display, TEXT("Camera=%s transition=%.3f time=%.3f"), bAction ? TEXT("action") : TEXT("preparation"), CameraTransitionRemaining, GetWorld()->GetTimeSeconds());
}

void AFoundryStage::CaptureView()
{
    const FString Name = bActionView ? TEXT("host-action.png") : TEXT("host-preparation.png");
    CaptureNamed(Name);
}

void AFoundryStage::CaptureNamed(const FString& Name)
{
    RequestedCaptures.Add(Name);
    const FString ScreenshotPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"), Name));
    if(!bTechnicalMode)
    {
        FTimerHandle CaptureTimer;
        const float Delay=Name==TEXT("campaign-action-settled.png")?.85f:.10f;
        GetWorldTimerManager().SetTimer(CaptureTimer,FTimerDelegate::CreateWeakLambda(this,[ScreenshotPath](){FScreenshotRequest::RequestScreenshot(ScreenshotPath,true,false);}),Delay,false);
        UE_LOG(LogFoundryHost,Display,TEXT("Campaign capture queued after layout: %s"),*ScreenshotPath);
        return;
    }
    FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false);
    UE_LOG(LogFoundryHost, Display, TEXT("Rendered screenshot requested: %s"), *ScreenshotPath);
}

void AFoundryStage::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(Audio)
    {
        bool Active=bTechnicalMode;
#if FOUNDRY_WITH_CAMPAIGN
        if(Campaign)Active=Campaign->Page!=TEXT("title") && Campaign->Page!=TEXT("confirm-new");
#endif
        Audio->SetActive(Active);
        Audio->Tick(DeltaSeconds);
    }
#if FOUNDRY_WITH_CAMPAIGN
    if (Campaign) RefreshCampaignWorld();
#endif
    if (CameraTransitionRemaining > 0)
    {
        CameraTransitionRemaining = FMath::Max(0.f, CameraTransitionRemaining - DeltaSeconds);
        if (CameraTransitionRemaining <= 0)
        {
            UE_LOG(LogFoundryHost, Display, TEXT("CAMERA_SETTLED action=%d time=%.3f"), bActionView, GetWorld()->GetTimeSeconds());
            if (!QueuedEvents.empty())
            {
                PresentEvents(QueuedEvents);
                QueuedEvents.clear();
                ReturnCameraAfter = 2.25f;
                UE_LOG(LogFoundryHost, Display, TEXT("ENEMY_PRESENT_DISPATCH settled=1 time=%.3f"), GetWorld()->GetTimeSeconds());
            }
#if FOUNDRY_WITH_CAMPAIGN
            if (Campaign) ++Campaign->ViewRevision;
#endif
        }
    }
    else if (ReturnCameraAfter > 0.0f)
    {
        ReturnCameraAfter -= DeltaSeconds;
        if (ReturnCameraAfter <= 0.0f)
        {
            if (HasTerminalPresentation()) ReturnCameraAfter = .10f;
            else SetActionView(false);
        }
    }
    PresentCommittedEvents();
    AimAtSelectedTarget();
    if (bArtProbe) TickArtProbe(DeltaSeconds);
    if (bRosterProbe) TickRosterProbe(DeltaSeconds);
    if (bCampaignProbe) TickCampaignProbe(DeltaSeconds);
    if (!bSmokeTest) return;
    SmokeElapsed += DeltaSeconds;
    if (SmokeStep == 0 && SmokeElapsed >= 4.0f) { CaptureView(); ++SmokeStep; }
    else if (SmokeStep == 1 && SmokeElapsed >= 6.0f) { SetActionView(true); ++SmokeStep; }
    else if (SmokeStep == 2 && SmokeElapsed >= 9.0f) { CaptureView(); ++SmokeStep; }
    else if (SmokeStep == 3 && SmokeElapsed >= 11.0f) { SetActionView(false); ++SmokeStep; }
    else if (SmokeStep == 4 && SmokeElapsed >= 13.0f)
    {
        UE_LOG(LogFoundryHost, Display, TEXT("FOUNDRY_HOST_SMOKE_COMPLETE: rendered viewport, both camera poses, orderly exit."));
        FPlatformMisc::RequestExit(false);
        ++SmokeStep;
    }
}

AFoundryPlayerController::AFoundryPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    bAutoManageActiveCameraTarget = false;
}

void AFoundryPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AFoundryPlayerController::PreparationView);
    InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AFoundryPlayerController::ActionView);
    InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &AFoundryPlayerController::Screenshot);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AFoundryPlayerController::QuitHost);
    InputComponent->BindKey(EKeys::C, IE_Pressed, this, &AFoundryPlayerController::Collect);
    InputComponent->BindKey(EKeys::L, IE_Pressed, this, &AFoundryPlayerController::LoadParts);
    InputComponent->BindKey(EKeys::U, IE_Pressed, this, &AFoundryPlayerController::UnloadParts);
    InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AFoundryPlayerController::Fire);
    InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AFoundryPlayerController::EndTurn);
    InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AFoundryPlayerController::Restart);
    InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AFoundryPlayerController::Steer);
    InputComponent->BindKey(EKeys::P, IE_Pressed, this, &AFoundryPlayerController::Precision);
    InputComponent->BindKey(EKeys::H, IE_Pressed, this, &AFoundryPlayerController::TogglePanels);
    InputComponent->BindKey(EKeys::F3, IE_Pressed, this, &AFoundryPlayerController::Diagnostic);
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
}

void AFoundryPlayerController::PreparationView() { if (AFoundryStage* Stage = FindStage(GetWorld()); Stage && Stage->IsTechnicalMode()) Stage->SetActionView(false); }
void AFoundryPlayerController::ActionView() { if (AFoundryStage* Stage = FindStage(GetWorld()); Stage && Stage->IsTechnicalMode()) Stage->SetActionView(true); }
void AFoundryPlayerController::Screenshot() { if (AFoundryStage* Stage = FindStage(GetWorld())) Stage->CaptureView(); }
void AFoundryPlayerController::QuitHost() { if (AFoundryStage* Stage = FindStage(GetWorld()); Stage && !Stage->IsTechnicalMode()) Stage->Control(TEXT("back")); else ConsoleCommand(TEXT("quit")); }
void AFoundryPlayerController::Collect() { if (AFoundryStage* Stage = KeyboardStage(GetWorld())) Stage->Control(TEXT("collect")); }
void AFoundryPlayerController::LoadParts() { if (AFoundryStage* Stage = KeyboardStage(GetWorld())) Stage->Control(TEXT("load")); }
void AFoundryPlayerController::UnloadParts() { if (AFoundryStage* Stage = KeyboardStage(GetWorld())) Stage->Control(TEXT("unload")); }
void AFoundryPlayerController::Fire() { if (AFoundryStage* Stage = KeyboardStage(GetWorld())) Stage->Control(TEXT("fire")); }
void AFoundryPlayerController::EndTurn() { if (AFoundryStage* Stage = KeyboardStage(GetWorld())) Stage->Control(TEXT("end")); }
void AFoundryPlayerController::Restart() { if (AFoundryStage* Stage = KeyboardStage(GetWorld())) Stage->Control(TEXT("restart")); }
void AFoundryPlayerController::Steer() { if (AFoundryStage* Stage = KeyboardStage(GetWorld())) Stage->Control(TEXT("steer")); }
void AFoundryPlayerController::Precision() { if (AFoundryStage* Stage = KeyboardStage(GetWorld())) Stage->Control(TEXT("precision")); }
void AFoundryPlayerController::TogglePanels() { if (AFoundryStage* Stage = FindStage(GetWorld())) Stage->Control(TEXT("panels")); }
void AFoundryPlayerController::Diagnostic() { if (AFoundryStage* Stage = FindStage(GetWorld())) Stage->Control(TEXT("diagnostic")); }

AFoundryGameMode::AFoundryGameMode()
{
    DefaultPawnClass = nullptr;
    PlayerControllerClass = AFoundryPlayerController::StaticClass();
    HUDClass = AFoundryHUD::StaticClass();
}

void AFoundryGameMode::BeginPlay()
{
    Super::BeginPlay();
    GetWorld()->SpawnActor<AFoundryStage>();
}

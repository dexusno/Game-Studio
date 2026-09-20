#include "FoundryHost.h"
#include "FoundrySession.h"
#include "FoundryRobot.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/InputComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
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
#include "UnrealClient.h"

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
}

AFoundryStage::AFoundryStage()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("StageRoot")));
}

AFoundryStage::~AFoundryStage() = default;

void AFoundryStage::AddShape(const TCHAR* Label, UStaticMesh* Mesh, const FVector& Location,
                            const FVector& Scale, const FLinearColor& Color, const FRotator& Rotation)
{
    UStaticMeshComponent* Shape = NewObject<UStaticMeshComponent>(this, FName(Label));
    Shape->SetupAttachment(RootComponent);
    Shape->SetStaticMesh(Mesh);
    Shape->SetRelativeLocation(Location);
    Shape->SetRelativeScale3D(Scale);
    Shape->SetRelativeRotation(Rotation);
    Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (StageMaterial)
    {
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(StageMaterial, this);
        Material->SetVectorParameterValue(TEXT("Tint"), Color);
        Shape->SetMaterial(0, Material);
    }
    Shape->RegisterComponent();
    AddInstanceComponent(Shape);
}

void AFoundryStage::BeginPlay()
{
    Super::BeginPlay();
    uint64 Seed = 1;
    FParse::Value(FCommandLine::Get(), TEXT("FoundrySeed="), Seed);
    Session = MakeUnique<FFoundrySession>(Seed);
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
    int32 StageActorCount = 0;
    TSet<UStaticMesh*> StageMeshes;
    for (TActorIterator<AStaticMeshActor> Actor(GetWorld()); Actor; ++Actor)
    {
        if (!Actor->ActorHasTag(TEXT("CinderwallGenerated"))) continue;
        ++StageActorCount;
        UStaticMesh* Mesh = Actor->GetStaticMeshComponent()->GetStaticMesh();
        StageMeshes.Add(Mesh);
        if (Mesh->GetName().Contains(TEXT("SM_CW_deck_")))
        {
            const FVector Extent = Mesh->GetRenderData()->Bounds.BoxExtent;
            checkf(FMath::IsNearlyEqual(Extent.X, 98.0, 1.0) && FMath::IsNearlyEqual(Extent.Y, 98.0, 1.0), TEXT("Cinderwall render geometry is not centimetre scale"));
        }
    }
    checkf(StageActorCount == 47 && StageMeshes.Num() == 18, TEXT("Run Art to rebuild the complete Cinderwall map"));
    UE_LOG(LogFoundryHost, Display, TEXT("ART_STAGE placements=%d shared_meshes=%d"), StageActorCount, StageMeshes.Num());
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    StageMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Technical/M_HostMetal.M_HostMetal"));
    checkf(Cube && Cylinder && Sphere && StageMaterial, TEXT("Generate smoke content before launching the host."));

    const FLinearColor Iron(0.15f, 0.21f, 0.24f);
    const FLinearColor Brass(0.48f, 0.22f, 0.065f);
    const FLinearColor Dark(0.04f, 0.065f, 0.08f);
    // The cannon remains a clearly provisional host prop; this art package contains
    // the robots and stage, not Mara's production gun. Stage meshes live in the map.
    AddShape(TEXT("RigBase"), Cylinder, FVector(-300, 0, 50), FVector(2.8, 2.8, 1), Dark);
    AddShape(TEXT("RigBody"), Cube, FVector(-300, 0, 145), FVector(2.2, 1.8, 1.5), Brass);
    AddShape(TEXT("RigBarrel"), Cylinder, FVector(-150, 0, 180), FVector(0.72, 0.72, 2.6), Iron, FRotator(90, 0, 0));
    AddShape(TEXT("RigRing"), Cylinder, FVector(-35, 0, 180), FVector(0.97, 0.97, 0.22), Brass, FRotator(90, 0, 0));
    SpawnRobots();

    ADirectionalLight* Key = GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-40, 35, 0));
    Key->GetLightComponent()->SetIntensity(5.0f);
    Key->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.80f, 0.57f));
    APointLight* Warm = GetWorld()->SpawnActor<APointLight>(FVector(-300, 230, 420), FRotator::ZeroRotator);
    Warm->PointLightComponent->SetIntensity(100000.0f);
    Warm->PointLightComponent->SetAttenuationRadius(1250.0f);
    Warm->PointLightComponent->SetLightColor(FLinearColor(1.0f, 0.38f, 0.10f));
    APointLight* Cool = GetWorld()->SpawnActor<APointLight>(FVector(650, -240, 380), FRotator::ZeroRotator);
    Cool->PointLightComponent->SetIntensity(150000.0f);
    Cool->PointLightComponent->SetAttenuationRadius(1300.0f);
    Cool->PointLightComponent->SetLightColor(FLinearColor(0.20f, 0.60f, 1.0f));
    ASkyLight* Fill = GetWorld()->SpawnActor<ASkyLight>();
    Fill->GetLightComponent()->SetIntensity(0.45f);
    Fill->GetLightComponent()->SetRealTimeCaptureEnabled(false);
    Fill->GetLightComponent()->RecaptureSky();

    const FVector PrepareLocation(-60, 2400, 700);
    const FVector PrepareFocus(-60, -60, 225);
    PreparationCamera = GetWorld()->SpawnActor<ACameraActor>(PrepareLocation, (PrepareFocus - PrepareLocation).Rotation());
    PreparationCamera->GetCameraComponent()->SetFieldOfView(52.0f);
    const FVector ActionLocation(-540, 940, 345);
    const FVector ActionFocus(260, -20, 140);
    ActionCamera = GetWorld()->SpawnActor<ACameraActor>(ActionLocation, (ActionFocus - ActionLocation).Rotation());
    ActionCamera->GetCameraComponent()->SetFieldOfView(50.0f);
    SetActionView(false, true);
    if (bArtProbe) Session->bShowPanels = false;
    UE_LOG(LogFoundryHost, Display, TEXT("Host P13 ready. Cinderwall-v001 shared-core Mara teaching encounter. Smoke=%d ArtProbe=%d"), bSmokeTest, bArtProbe);
}

void AFoundryStage::SpawnRobots()
{
    for (AFoundryRobot* Robot : Robots) if (IsValid(Robot)) Robot->Destroy();
    Robots.Empty();
    for (const auto& Enemy : Session->State.enemies)
    {
        if (Enemy.dead || Enemy.escaped) continue;
        const bool bRam = Enemy.definition == "C1-R02";
        AFoundryRobot* Robot = GetWorld()->SpawnActor<AFoundryRobot>(bRam ? FVector(360, -65, 1.5) : FVector(130, 20, 1.5), FRotator::ZeroRotator);
        Robot->Initialize(Enemy.id, bRam, Enemy.maxHp);
        Robots.Add(Robot);
    }
}

void AFoundryStage::PresentCommittedEvents()
{
    if (!Session || Session->CommittedEvents.empty()) return;
    TSet<uint64> Died;
    TSet<uint64> NamedActionParents;
    for (const auto& Event : Session->CommittedEvents) if (Event.type == "enemy_death") Died.Add(Event.target);
    for (const auto& Event : Session->CommittedEvents) if (Event.type.rfind("robot_action:", 0) == 0) NamedActionParents.Add(Event.parent);
    for (const auto& Event : Session->CommittedEvents)
    {
        for (AFoundryRobot* Robot : Robots)
        {
            if (!IsValid(Robot)) continue;
            const uint64 Id = Robot->GetCoreId();
            if (Event.type == "enemy_death" && Event.target == Id) Robot->Die(Event.id);
            else if (Died.Contains(Id)) continue; // Death wins over every reaction in this committed action.
            else if ((Event.type == "hit" || Event.type == "status_damage") && Event.target == Id) Robot->Hit(Event.amount, Event.secondary, Event.id);
            else if (Event.type.rfind("robot_action:", 0) == 0 && Event.subject == Id) Robot->NamedAction(UTF8_TO_TCHAR(Event.type.substr(13).c_str()), Event.id);
            else if (Event.type == "enemy_action" && Event.subject == Id && !NamedActionParents.Contains(Event.id)) Robot->EnemyAction(Event.amount, Event.id);
            else if (Event.type == "enemy_escape" && Event.subject == Id) Robot->Escape(Event.id);
        }
    }
    Session->CommittedEvents.clear();
}

void AFoundryStage::Control(const FString& Command)
{
    if (!Session) return;
    const bool bCommitted = Session->Control(Command);
    PresentCommittedEvents();
    if (bCommitted && (Command == TEXT("fire") || Command == TEXT("end")))
    {
        SetActionView(true);
        ReturnCameraAfter = 2.25f;
    }
    if (Command == TEXT("restart")) { SpawnRobots(); ReturnCameraAfter = 0.0f; SetActionView(false); }
}

void AFoundryStage::SetActionView(bool bAction, bool bInstant)
{
    bActionView = bAction;
    if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
    {
        Controller->SetViewTargetWithBlend(bAction ? ActionCamera.Get() : PreparationCamera.Get(), bInstant ? 0.0f : 0.65f,
                                           EViewTargetBlendFunction::VTBlend_Cubic);
    }
    UE_LOG(LogFoundryHost, Display, TEXT("Camera=%s"), bAction ? TEXT("action") : TEXT("preparation"));
}

void AFoundryStage::CaptureView()
{
    const FString Name = bActionView ? TEXT("host-action.png") : TEXT("host-preparation.png");
    CaptureNamed(Name);
}

void AFoundryStage::CaptureNamed(const FString& Name)
{
    const FString ScreenshotPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"), Name));
    FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false);
    UE_LOG(LogFoundryHost, Display, TEXT("Rendered screenshot requested: %s"), *ScreenshotPath);
}

void AFoundryStage::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (ReturnCameraAfter > 0.0f)
    {
        ReturnCameraAfter -= DeltaSeconds;
        if (ReturnCameraAfter <= 0.0f) SetActionView(false);
    }
    PresentCommittedEvents();
    if (bArtProbe) TickArtProbe(DeltaSeconds);
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
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
}

void AFoundryPlayerController::PreparationView() { if (AFoundryStage* Stage = FindStage(GetWorld())) Stage->SetActionView(false); }
void AFoundryPlayerController::ActionView() { if (AFoundryStage* Stage = FindStage(GetWorld())) Stage->SetActionView(true); }
void AFoundryPlayerController::Screenshot() { if (AFoundryStage* Stage = FindStage(GetWorld())) Stage->CaptureView(); }
void AFoundryPlayerController::QuitHost() { ConsoleCommand(TEXT("quit")); }
void AFoundryPlayerController::Collect() { if (AFoundryStage* Stage = FindStage(GetWorld())) Stage->Control(TEXT("collect")); }
void AFoundryPlayerController::LoadParts() { if (AFoundryStage* Stage = FindStage(GetWorld())) Stage->Control(TEXT("load")); }
void AFoundryPlayerController::UnloadParts() { if (AFoundryStage* Stage = FindStage(GetWorld())) Stage->Control(TEXT("unload")); }
void AFoundryPlayerController::Fire() { if (AFoundryStage* Stage = FindStage(GetWorld())) Stage->Control(TEXT("fire")); }
void AFoundryPlayerController::EndTurn() { if (AFoundryStage* Stage = FindStage(GetWorld())) Stage->Control(TEXT("end")); }
void AFoundryPlayerController::Restart() { if (AFoundryStage* Stage = FindStage(GetWorld())) Stage->Control(TEXT("restart")); }
void AFoundryPlayerController::Steer() { if (AFoundryStage* Stage = FindStage(GetWorld())) Stage->Control(TEXT("steer")); }
void AFoundryPlayerController::Precision() { if (AFoundryStage* Stage = FindStage(GetWorld())) Stage->Control(TEXT("precision")); }
void AFoundryPlayerController::TogglePanels() { if (AFoundryStage* Stage = FindStage(GetWorld())) Stage->Control(TEXT("panels")); }

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

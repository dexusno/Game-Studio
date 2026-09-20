#include "FoundryHost.h"
#include "FoundrySession.h"

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
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    StageMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Technical/M_HostMetal.M_HostMetal"));
    checkf(Cube && Cylinder && Sphere && StageMaterial, TEXT("Generate smoke content before launching the host."));

    const FLinearColor Iron(0.15f, 0.21f, 0.24f);
    const FLinearColor Brass(0.48f, 0.22f, 0.065f);
    const FLinearColor Dark(0.04f, 0.065f, 0.08f);
    AddShape(TEXT("Deck"), Cube, FVector(0, 0, -38), FVector(26, 12, 0.75), Iron);
    AddShape(TEXT("BackWall"), Cube, FVector(0, -510, 260), FVector(28, 0.55, 6), Dark);
    for (int32 Index = 0; Index < 8; ++Index)
    {
        const double X = -1100.0 + 320.0 * Index;
        AddShape(*FString::Printf(TEXT("WallRib%d"), Index), Cube, FVector(X, -455, 290), FVector(0.55, 0.8, 6.2), Iron);
        AddShape(*FString::Printf(TEXT("DeckStrip%d"), Index), Cube, FVector(X, 0, 1), FVector(0.08, 11, 0.06), Brass);
    }
    // These original primitive assemblies are labels for future art, not enemy content.
    AddShape(TEXT("RigBase"), Cylinder, FVector(-300, 0, 50), FVector(2.8, 2.8, 1), Dark);
    AddShape(TEXT("RigBody"), Cube, FVector(-300, 0, 145), FVector(2.2, 1.8, 1.5), Brass);
    AddShape(TEXT("RigBarrel"), Cylinder, FVector(-150, 0, 180), FVector(0.72, 0.72, 2.6), Iron, FRotator(90, 0, 0));
    AddShape(TEXT("RigRing"), Cylinder, FVector(-35, 0, 180), FVector(0.97, 0.97, 0.22), Brass, FRotator(90, 0, 0));
    AddShape(TEXT("RearClawMast"), Cube, FVector(-820, 120, 205), FVector(0.38, 0.5, 4.3), Iron);
    AddShape(TEXT("RearClawArm"), Cube, FVector(-700, 120, 420), FVector(2.8, 0.45, 0.4), Brass);
    AddShape(TEXT("RearClawCable"), Cylinder, FVector(-575, 120, 340), FVector(0.055, 0.055, 1.6), Dark);
    AddShape(TEXT("RearClawLeft"), Cube, FVector(-600, 120, 270), FVector(0.15, 0.25, 0.7), Brass, FRotator(0, 0, -25));
    AddShape(TEXT("RearClawRight"), Cube, FVector(-550, 120, 270), FVector(0.15, 0.25, 0.7), Brass, FRotator(0, 0, 25));
    AddShape(TEXT("ScrapBin"), Cube, FVector(-610, 70, 22), FVector(3.2, 2.8, 0.42), Iron);
    AddShape(TEXT("ScrapA"), Cube, FVector(-675, 30, 70), FVector(0.62, 0.64, 0.68), Brass, FRotator(10, 22, 30));
    AddShape(TEXT("ScrapB"), Cylinder, FVector(-570, 70, 60), FVector(0.7, 0.7, 0.5), Iron, FRotator(24, 12, 45));
    AddShape(TEXT("TargetBase"), Cube, FVector(430, 20, 45), FVector(2.6, 2.0, 0.9), Dark);
    AddShape(TEXT("TargetBody"), Sphere, FVector(430, 20, 145), FVector(2.7, 2.1, 2.4), Iron);
    AddShape(TEXT("TargetRam"), Cube, FVector(295, 20, 150), FVector(0.55, 2.35, 1.35), Brass);
    AddShape(TEXT("TargetCore"), Sphere, FVector(430, 132, 165), FVector(0.58, 0.22, 0.58), FLinearColor(0.8f, 0.22f, 0.035f));
    AddShape(TEXT("MiteBody"), Sphere, FVector(160, 80, 65), FVector(0.95, 0.8, 0.7), Brass);
    AddShape(TEXT("MiteLeftLeg"), Cube, FVector(160, 30, 30), FVector(1.2, 0.12, 0.15), Iron, FRotator(0, -30, 0));
    AddShape(TEXT("MiteRightLeg"), Cube, FVector(160, 125, 30), FVector(1.2, 0.12, 0.15), Iron, FRotator(0, 30, 0));

    ADirectionalLight* Key = GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-40, 35, 0));
    Key->GetLightComponent()->SetIntensity(4.0f);
    Key->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.80f, 0.57f));
    APointLight* Warm = GetWorld()->SpawnActor<APointLight>(FVector(-300, 230, 420), FRotator::ZeroRotator);
    Warm->PointLightComponent->SetIntensity(70000.0f);
    Warm->PointLightComponent->SetAttenuationRadius(1250.0f);
    Warm->PointLightComponent->SetLightColor(FLinearColor(1.0f, 0.38f, 0.10f));
    APointLight* Cool = GetWorld()->SpawnActor<APointLight>(FVector(650, -240, 380), FRotator::ZeroRotator);
    Cool->PointLightComponent->SetIntensity(90000.0f);
    Cool->PointLightComponent->SetAttenuationRadius(1300.0f);
    Cool->PointLightComponent->SetLightColor(FLinearColor(0.20f, 0.60f, 1.0f));
    ASkyLight* Fill = GetWorld()->SpawnActor<ASkyLight>();
    Fill->GetLightComponent()->SetIntensity(0.45f);
    Fill->GetLightComponent()->SetRealTimeCaptureEnabled(false);
    Fill->GetLightComponent()->RecaptureSky();

    const FVector PrepareLocation(0, 1920, 695);
    const FVector PrepareFocus(-40, 30, 170);
    PreparationCamera = GetWorld()->SpawnActor<ACameraActor>(PrepareLocation, (PrepareFocus - PrepareLocation).Rotation());
    PreparationCamera->GetCameraComponent()->SetFieldOfView(52.0f);
    const FVector ActionLocation(-650, 700, 370);
    const FVector ActionFocus(340, 15, 145);
    ActionCamera = GetWorld()->SpawnActor<ACameraActor>(ActionLocation, (ActionFocus - ActionLocation).Rotation());
    ActionCamera->GetCameraComponent()->SetFieldOfView(61.0f);
    SetActionView(false, true);
    UE_LOG(LogFoundryHost, Display, TEXT("Host P13 ready. Shared-core Mara teaching encounter. Smoke=%d"), bSmokeTest);
}

void AFoundryStage::Control(const FString& Command)
{
    if (!Session) return;
    const bool bCommitted = Session->Control(Command);
    if (bCommitted && (Command == TEXT("fire") || Command == TEXT("end")))
    {
        SetActionView(true);
        ReturnCameraAfter = 1.25f;
    }
    if (Command == TEXT("restart")) { ReturnCameraAfter = 0.0f; SetActionView(false); }
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
    if (Session)
    {
        TInlineComponentArray<UStaticMeshComponent*> Shapes(this);
        for (UStaticMeshComponent* Shape : Shapes)
        {
            const bool bRam = Shape->GetName().StartsWith(TEXT("Target"));
            const bool bMite = Shape->GetName().StartsWith(TEXT("Mite"));
            if (!bRam && !bMite) continue;
            const size_t Index = bRam ? 1 : 0;
            const auto& Enemy = Session->State.enemies[Index];
            Shape->SetVisibility(!Enemy.dead && !Enemy.escaped);
        }
    }
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

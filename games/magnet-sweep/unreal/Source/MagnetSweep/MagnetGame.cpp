#include "MagnetGame.h"
#include "WorkbenchRuntime.h"
#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"
#include "Components/SceneComponent.h"
AMagnetWorkbench::AMagnetWorkbench() { PrimaryActorTick.bCanEverTick=true; RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root")); }
AMagnetWorkbench::~AMagnetWorkbench() { delete Demo; }
void AMagnetWorkbench::BeginPlay() { Super::BeginPlay(); Demo=new FWorkbenchImpl(this);Demo->Start(); }
void AMagnetWorkbench::Tick(float D) { Super::Tick(D);if(Demo)Demo->Tick(D); }
void AMagnetWorkbench::EndPlay(const EEndPlayReason::Type R) { if(Demo){Demo->Save();Demo->Stop();}Super::EndPlay(R); }
void AMagnetWorkbench::Paint(UCanvas* C) { if(Demo&&C)Demo->Paint(C); }
void AMagnetPlayerController::BeginPlay() { Super::BeginPlay(); bShowMouseCursor=true; FInputModeGameAndUI Mode; Mode.SetHideCursorDuringCapture(false); Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); SetInputMode(Mode); }
void AMagnetHUD::DrawHUD() { Super::DrawHUD(); for(TActorIterator<AMagnetWorkbench> It(GetWorld());It;++It){It->Paint(Canvas);break;} }
AMagnetGameMode::AMagnetGameMode() { DefaultPawnClass=nullptr; PlayerControllerClass=AMagnetPlayerController::StaticClass(); HUDClass=AMagnetHUD::StaticClass(); }
void AMagnetGameMode::StartPlay() { Super::StartPlay(); GetWorld()->SpawnActor<AMagnetWorkbench>(); }

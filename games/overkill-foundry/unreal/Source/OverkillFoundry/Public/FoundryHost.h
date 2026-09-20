#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "FoundrySession.h"
#include "FoundryCampaign.h"
#include "FoundryAudio.h"
#include "FoundryHost.generated.h"

class ACameraActor;
class UMaterialInterface;
class UStaticMesh;
class AFoundryRobot;
class AFoundryMara;
class FFoundryCampaign;
class SWidget;

// Technical presentation adapter. All combat rules live in the shared core.
UCLASS()
class OVERKILLFOUNDRY_API AFoundryStage : public AActor
{
    GENERATED_BODY()
public:
    AFoundryStage();
    virtual ~AFoundryStage() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;
    void SetActionView(bool bAction, bool bInstant = false);
    void CaptureView();
    bool IsActionView() const { return bActionView; }
    bool IsPresentationBusy() const { return CameraTransitionRemaining > 0 || ReturnCameraAfter > 0 || !QueuedEvents.empty(); }
    void PlayPresentationCue(const FString& Cue);
    const FString& GetActionCaption() const { return ActionCaption; }
    FFoundrySession& GetSession() { return *Session; }
    const FFoundrySession& GetSession() const { return *Session; }
    void Control(const FString& Command);
    bool IsTechnicalMode() const { return bTechnicalMode; }
#if FOUNDRY_WITH_CAMPAIGN
    FFoundryCampaign* GetCampaign() { return Campaign.Get(); }
    void RefreshCampaignWorld();
#endif
private:
    void SpawnRobots();
    void SpawnMissingRobots();
    bool HasTerminalPresentation() const;
    void PresentCommittedEvents();
    void PresentEvents(const std::vector<overkill::Event>& Events);
    void ResetActionPresentation();
    void AimAtSelectedTarget();
    FVector RobotPosition(int32 Index) const;
    void FrameRoster();
    void TickArtProbe(float DeltaSeconds);
    void TickRosterProbe(float DeltaSeconds);
    void CaptureNamed(const FString& Name);
    void TickCampaignProbe(float DeltaSeconds);
    UPROPERTY() TObjectPtr<ACameraActor> PreparationCamera;
    UPROPERTY() TObjectPtr<ACameraActor> ActionCamera;
    UPROPERTY() TArray<TObjectPtr<AFoundryRobot>> Robots;
    UPROPERTY() TObjectPtr<AFoundryMara> Mara;
    bool bActionView = false;
    FString ActionCaption = TEXT("Firing…");
    bool bSmokeTest = false;
    float SmokeElapsed = 0.0f;
    int32 SmokeStep = 0;
    TUniquePtr<FFoundrySession> Session;
    TUniquePtr<FFoundryAudio> Audio;
#if FOUNDRY_WITH_CAMPAIGN
    TUniquePtr<FFoundryCampaign> Campaign;
    uint64 SeenSceneRevision = 0;
#endif
    bool bTechnicalMode = true;
    bool bCampaignProbe = false;
    float CampaignProbeElapsed = 0;
    int32 CampaignProbeStep = 0;
    int32 CameraProbeStep = 0;
    float ReturnCameraAfter = 0.0f;
    float CameraTransitionRemaining = 0.0f;
    std::vector<overkill::Event> QueuedEvents;
    bool bWasTitle = true;
    bool bWideRoster = false;
    bool bArtProbe = false;
    bool bRosterProbe = false;
    bool bRosterProbeOk = true;
    float RosterElapsed = 0;
    int32 RosterCase = 0;
    int32 RosterStep = 0;
    int32 RosterExpectedHits = 0;
    FString RosterExpectedHash;
    bool bArtProbeOk = true;
    float ArtElapsed = 0;
    int32 ArtStep = 0;
    int32 MaraProbeStep = 0;
    TSet<FString> ArtCaptures;
    TSet<FString> RequestedCaptures;
};

UCLASS()
class OVERKILLFOUNDRY_API AFoundryPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    AFoundryPlayerController();
    virtual void SetupInputComponent() override;
private:
    void PreparationView();
    void ActionView();
    void Screenshot();
    void QuitHost();
    void Collect();
    void LoadParts();
    void UnloadParts();
    void Fire();
    void EndTurn();
    void Restart();
    void Steer();
    void Precision();
    void TogglePanels();
    void Diagnostic();
};

UCLASS()
class OVERKILLFOUNDRY_API AFoundryHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void NotifyHitBoxClick(FName BoxName) override;
    virtual void NotifyHitBoxBeginCursorOver(FName BoxName) override;
    virtual void NotifyHitBoxEndCursorOver(FName BoxName) override;
private:
    FString HoverCommand;
    TSharedPtr<SWidget> CampaignWidget;
};

UCLASS()
class OVERKILLFOUNDRY_API AFoundryGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AFoundryGameMode();
    virtual void BeginPlay() override;
};

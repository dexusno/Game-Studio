#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "FoundrySession.h"
#include "FoundryHost.generated.h"

class ACameraActor;
class UMaterialInterface;
class UStaticMesh;

// Technical presentation adapter. All combat rules live in the shared core.
UCLASS()
class OVERKILLFOUNDRY_API AFoundryStage : public AActor
{
    GENERATED_BODY()
public:
    AFoundryStage();
    virtual ~AFoundryStage() override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void SetActionView(bool bAction, bool bInstant = false);
    void CaptureView();
    bool IsActionView() const { return bActionView; }
    FFoundrySession& GetSession() { return *Session; }
    const FFoundrySession& GetSession() const { return *Session; }
    void Control(const FString& Command);
private:
    void AddShape(const TCHAR* Label, UStaticMesh* Mesh, const FVector& Location,
                  const FVector& Scale, const FLinearColor& Color, const FRotator& Rotation = FRotator::ZeroRotator);
    UPROPERTY() TObjectPtr<ACameraActor> PreparationCamera;
    UPROPERTY() TObjectPtr<ACameraActor> ActionCamera;
    UPROPERTY() TObjectPtr<UMaterialInterface> StageMaterial;
    bool bActionView = false;
    bool bSmokeTest = false;
    float SmokeElapsed = 0.0f;
    int32 SmokeStep = 0;
    TUniquePtr<FFoundrySession> Session;
    float ReturnCameraAfter = 0.0f;
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
};

UCLASS()
class OVERKILLFOUNDRY_API AFoundryHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    virtual void NotifyHitBoxClick(FName BoxName) override;
    virtual void NotifyHitBoxBeginCursorOver(FName BoxName) override;
    virtual void NotifyHitBoxEndCursorOver(FName BoxName) override;
private:
    FString HoverCommand;
};

UCLASS()
class OVERKILLFOUNDRY_API AFoundryGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AFoundryGameMode();
    virtual void BeginPlay() override;
};

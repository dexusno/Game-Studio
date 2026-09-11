#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Actor.h"
#include "MagnetGame.generated.h"

UCLASS()
class AMagnetPlayerController : public APlayerController {
 GENERATED_BODY()
public:
 virtual void BeginPlay() override;
};
UCLASS()
class AMagnetHUD : public AHUD {
 GENERATED_BODY()
public:
 virtual void DrawHUD() override;
};
struct FWorkbenchImpl;
UCLASS()
class AMagnetWorkbench : public AActor {
 GENERATED_BODY()
public:
 AMagnetWorkbench();
 virtual ~AMagnetWorkbench() override;
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 void Paint(UCanvas* Target);
private:
 FWorkbenchImpl* Demo = nullptr;
 UPROPERTY() TArray<TObjectPtr<UObject>> Resources;
 friend struct FWorkbenchImpl;
};
UCLASS()
class AMagnetGameMode : public AGameModeBase {
 GENERATED_BODY()
public:
 AMagnetGameMode();
 virtual void StartPlay() override;
};

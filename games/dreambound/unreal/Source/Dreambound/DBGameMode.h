#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/SaveGame.h"
#include "DBTypes.h"
#include "DBGameMode.generated.h"

class ADBCharacter; class ADBEnemy; class UHierarchicalInstancedStaticMeshComponent;
USTRUCT()
struct FDBRoom {
 GENERATED_BODY()
 FVector Center=FVector::ZeroVector;
 FString Name;
 int32 Parent=-1;
 bool bOptional=false;
 bool bTech=false;
 TArray<AActor*> Gates;
 AActor* Altar=nullptr;
};
USTRUCT()
struct FDBOffer {
 GENERATED_BODY()
 FName Id;
 FString Name;
 FString Description;
 FLinearColor Color=FLinearColor::White;
};
UCLASS()
class DREAMBOUND_API UDBSave : public USaveGame {
 GENERATED_BODY()
public:
 UPROPERTY() int32 Version=1;
 UPROPERTY() int32 Revision=0;
 UPROPERTY() int32 Seed=0;
 UPROPERTY() int32 CurrentRoom=0;
 UPROPERTY() TArray<int32> Cleared;
 UPROPERTY() TArray<int32> Claimed;
 UPROPERTY() TMap<FName,int32> Upgrades;
 UPROPERTY() TMap<int32,FName> RoomRewards;
 UPROPERTY() TArray<FName> Patterns;
 UPROPERTY() float Health=100.f;
 UPROPERTY() int32 Element=0;
 UPROPERTY() bool bActiveRun=false;
 UPROPERTY() bool bBossWon=false;
 UPROPERTY() int32 Expeditions=0;
 UPROPERTY() float Sensitivity=1.f;
 UPROPERTY() float SoundVolume=.85f;
};
UCLASS()
class DREAMBOUND_API ADBGameMode : public AGameModeBase {
 GENERATED_BODY()
 friend FIntPoint DBRunRuntimeChecks(ADBGameMode&,FString&);
 friend class ADBShieldCheckRunner;
public:
 ADBGameMode();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
 void NotifyEnemyKilled(ADBEnemy* Enemy);
 void NotifyPlayerDied();
 void NotifyEvent(const FString& Text,FLinearColor Color=FLinearColor::White);
 void Interact();
 void ChooseReward(int32 Index);
 void TogglePause();
 void ToggleBuild();
 void StartNewRun(bool bSameSeed=false);
 void AdjustSensitivity(float Delta);
 void AdjustSoundVolume(float Delta);
 void ResumeRun();
 void QuitGame(bool bWithoutSaving=false);
 void SetMenuInput(bool bEnable);
 void SaveProgress(bool bActive=true);
 void LoadProgress();
 void BuildWorld();
 void ActivateRoom(int32 RoomIndex);
 void FinishRoom(int32 RoomIndex);
 void ShowOffers(int32 RoomIndex);
 void ClaimReward(FName Id);
 void UpdateGates();
 void RunVerification();
 void BuildRecoveryCourtyard();
 void BeginRewardPractice(FName Reward);
 void ClearRewardPractice();
 FVector GetRoomEntryPoint(int32 RoomIndex) const;
 void TickMotionDemo(float DeltaSeconds);
 FDBOffer DescribeUpgrade(FName Id) const;
 FString ObjectiveText() const;
 FString InteractText() const;
 float GetSensitivity() const { return Sensitivity; }
 bool bPaused=false,bChoosingReward=false,bShowingBuild=false,bTitle=true,bWon=false,bDefeated=false;
 int32 CurrentRoomId=0,Seed=0,Kills=0,CurrentWave=0,Expeditions=0;
 float RunSeconds=0.f,Sensitivity=1.f,SoundVolume=.85f;
 bool bBossWon=false,bCanResume=false;
 bool bRecoverySlice=true,bSliceAwaitingStart=true;
 bool bSaveFailed=false;
 FString EventText;
 FString PracticeInstruction;
 FName PracticeReward=NAME_None;
 FString LayoutSignature;
 TMap<int32,int32> RoomLayoutVariants;
 FLinearColor EventColor=FLinearColor::White;
 float EventRemaining=0;
 TArray<FDBRoom> Rooms;
 TArray<FDBOffer> Offers;
 TArray<int32> ClearedRooms,ClaimedRooms;
 TMap<int32,FName> EarnedRoomRewards;
 TArray<FName> LearnedPatterns;
 FName StartingPattern=NAME_None;
 ADBCharacter* Player=nullptr;
 FString SaveNotice;
private:
 TArray<AActor*> Generated;
 UPROPERTY() TArray<TObjectPtr<ADBEnemy>> PracticeActors;
 TMap<FString,UHierarchicalInstancedStaticMeshComponent*> MeshBatches;
 TSet<int32> SpawnedRooms;
 TMap<int32,int32> RoomWaves;
 UPROPERTY() UDBSave* StoredSave=nullptr;
 int32 SaveRevision=0,RewardRoom=0;
 float PulseTime=0,CheckTimer=0;
 bool bEnding=false,bVerify=false,bCapture=false,bHadFocus=true,bInitialEntry=false,bCapturedFrame=false;
 bool bMotionCapture=false,bDemoRecording=false,bDemoStopped=false,bChecksRunning=false;
 float VictoryDelay=0.f;
 double DemoStartedAt=0;
 double DemoAudioStartedAt=0;
 FString DemoFrameTimes;
 float DemoTime=0.f;
 int32 DemoFrame=0,DemoAction=0;
 FString SlotBase="DreamboundSegments";
 FRandomStream Random;
 AActor* Mesh(const FString& Name,FVector Location,FRotator Rotation=FRotator::ZeroRotator,FVector Scale=FVector::OneVector,bool bCollision=true,const FString& Material="");
 void Instance(const FString& Name,FVector Location,FRotator Rotation=FRotator::ZeroRotator,FVector Scale=FVector::OneVector,const FString& Material="");
 void MakeRoom(int32 Index);
 void MakeCorridor(int32 From,int32 To);
 void SpawnWave(int32 RoomIndex,int32 Wave);
 void ResetActors();
 bool IsCleared(int32 RoomId) const { return ClearedRooms.Contains(RoomId); }
};

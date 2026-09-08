#include "DBGameMode.h"
#include "DBCharacter.h"
#include "DBEnemy.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AudioMixerBlueprintLibrary.h"
#include "AudioDevice.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/App.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/IConsoleManager.h"
#include "UnrealClient.h"
#include "Kismet/GameplayStatics.h"

// Scripted visual/audio evidence, not a natural playthrough or fun assessment.
void ADBGameMode::TickMotionDemo(float Dt)
{
 if(!Player||PulseTime<5.f)return;
 if(!bDemoRecording){
  bDemoRecording=true;DemoTime=0;DemoFrame=0;DemoAction=0;DemoStartedAt=FPlatformTime::Seconds();
  IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir()/TEXT("MotionCapture")),true);
  FApp::SetUnfocusedVolumeMultiplier(1.f);GEngine->SetMaxFPS(60.f);
  // Recording needs silent buffers too; submix auto-disable otherwise removes
  // the pauses between impacts and desynchronizes the exported soundtrack.
  if(auto* RenderSilence=IConsoleManager::Get().FindConsoleVariable(TEXT("au.NeverDisableSubmixes")))RenderSilence->Set(1,ECVF_SetByCode);
  DemoFrameTimes=TEXT("frame,wall_seconds,audio_seconds\n");
  if(auto Device=GetWorld()->GetAudioDevice())DemoAudioStartedAt=Device->GetAudioClock();
  UAudioMixerBlueprintLibrary::StartRecordingOutput(this,27.f);
 }
 DemoTime=static_cast<float>(FPlatformTime::Seconds()-DemoStartedAt);
 RunSeconds+=Dt;EventRemaining=FMath::Max(0.f,EventRemaining-Dt);
 if(DemoTime>26.f){
  if(!bDemoStopped){
   bDemoStopped=true;Player->SuspendCombatInput();Player->GetCharacterMovement()->StopMovementImmediately();
   UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,TEXT("CourtyardMix"),FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("MotionCapture")));
   FString Note=FString::Printf(TEXT("Scripted moving renderer/audio capture. No ordinary-input, complete-run or fun claim.\nframes=%d seconds=%.3f engine_fps_cap=60\nScreenshot requests stall rendering; this is not a performance measurement.\n"),DemoFrame,DemoTime);
   FFileHelper::SaveStringToFile(Note,*(FPaths::ProjectSavedDir()/TEXT("MotionCapture/Capture.txt")));
   FFileHelper::SaveStringToFile(DemoFrameTimes,*(FPaths::ProjectSavedDir()/TEXT("MotionCapture/Frames.csv")));
  }
  if(DemoTime>28.f)FGenericPlatformMisc::RequestExit(false);
  return;
 }
 auto* PC=Cast<APlayerController>(Player->GetController());if(!PC)return;
 FVector Aim(360,260,490);ADBEnemy* Nearest=nullptr;float Best=BIG_NUMBER;
 for(TActorIterator<ADBEnemy> It(GetWorld());It;++It)if(!It->bDead&&It->RoomId==CurrentRoomId){float D=FVector::DistSquared(It->GetActorLocation(),Player->GetActorLocation());if(D<Best){Best=D;Nearest=*It;}}
 if(Nearest&&DemoTime>6)Aim=Nearest->GetActorLocation()+FVector(0,0,35);
 const FRotator Target=(Aim-Player->GetPawnViewLocation()).Rotation();
 PC->SetControlRotation(FMath::RInterpTo(PC->GetControlRotation(),Target,Dt,DemoTime<6?1.5f:6.f));
 FVector Destination(-1180,-530,110);
 if(DemoTime>6&&DemoTime<11)Destination=FVector(-320,-460,110);
 else if(DemoTime>=11&&DemoTime<16)Destination=FVector(-230,-1160,110);
 else if(DemoTime>=16&&DemoTime<21)Destination=FVector(190,-670,110);
 else if(DemoTime>=21)Destination=FVector(-650,-330,110);
 if(FVector::Dist2D(Player->GetActorLocation(),Destination)>90&&!(DemoTime>9&&DemoTime<11))Player->AddMovementInput((Destination-Player->GetActorLocation()).GetSafeNormal2D(),.55f);
 struct FAction{float At;int32 Kind;};
 static const FAction Actions[]={{3,0},{7,1},{9.6f,2},{10,3},{10.10f,4},{11,3},{11.7f,4},{13,5},{14.5f,1},{16,2},{16.2f,6},{18,3},{18.75f,4},{20,5},{21,1},{22,2},{22.2f,3},{22.3f,4},{23.5f,6}};
 while(DemoAction<UE_ARRAY_COUNT(Actions)&&DemoTime>=Actions[DemoAction].At){
  switch(Actions[DemoAction].Kind){case 0:Interact();break;case 1:Player->PressGuard();break;case 2:Player->ReleaseGuard();break;case 3:Player->PressFire();break;case 4:Player->ReleaseFire();break;case 5:Player->RecallShield();break;case 6:Player->UseSpecial();break;}
  ++DemoAction;
 }
 if(!FScreenshotRequest::IsScreenshotRequested()){
  double AudioTime=0;if(auto Device=GetWorld()->GetAudioDevice())AudioTime=Device->GetAudioClock()-DemoAudioStartedAt;
  DemoFrameTimes+=FString::Printf(TEXT("%d,%.6f,%.6f\n"),DemoFrame,DemoTime,AudioTime);
  const FString Frame=FPaths::ProjectSavedDir()/TEXT("MotionCapture")/FString::Printf(TEXT("Frame_%05d.png"),DemoFrame++);
  FScreenshotRequest::RequestScreenshot(Frame,true,false);
 }
}

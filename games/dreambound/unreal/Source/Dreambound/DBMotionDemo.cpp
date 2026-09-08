#include "DBGameMode.h"
#include "DBCharacter.h"
#include "DBEnemy.h"
#include "DBThrownShield.h"
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
 FVector Aim(-280,-500,160);
 PC->SetControlRotation(FMath::RInterpTo(PC->GetControlRotation(),(Aim-Player->GetPawnViewLocation()).Rotation(),Dt,4.f));
 // This controlled fixture makes each state visible. Upgrades and incoming
 // damage here are scripted, independently of the earned-progression checks.
 if(DemoAction==0){
  Player->SetActorLocation(FVector(-1000,-500,110));Player->GetCharacterMovement()->StopMovementImmediately();
  Player->ApplyUpgrade("Frost");BeginRewardPractice("Frost");PracticeInstruction="SCRIPTED CAPTURE / partial throw, retained guard, independent rebuild and Frost return. This is a controlled fixture.";
  EventRemaining=0;DemoAction=1;
 }
 else if(DemoAction==1&&DemoTime>2){Player->PressFire();DemoAction=2;}
 else if(DemoAction==2&&Player->GetSelectedPieceCount()>=3){Player->ReleaseFire();DemoAction=3;}
 else if(DemoAction==3&&DemoTime>4){Player->PressGuard();DemoAction=4;}
 else if(DemoAction==4&&DemoTime>5){Player->ReceiveAttack(16,Player->GetPawnViewLocation()+Player->GetAimDirection()*500,false,nullptr,12001);DemoAction=5;}
 else if(DemoAction==5&&DemoTime>6.4f){Player->RecallShield();DemoAction=6;}
 else if(DemoAction==6&&DemoTime>9){Player->ReleaseGuard();Player->PressFire();DemoAction=7;}
 else if(DemoAction==7&&Player->GetSelectedPieceCount()>=2){Player->ReleaseFire();DemoAction=8;}
 else if(DemoAction==8&&DemoTime>11){for(int32 I=0;I<6;++I)if(auto* F=Player->GetPieceFlight(I)){F->DestroyPiece();break;}DemoAction=9;}
 else if(DemoAction==9&&DemoTime>12){Player->RecallShield();DemoAction=10;}
 else if(DemoAction==10&&DemoTime>15){ToggleBuild();DemoAction=11;}
 else if(DemoAction==11&&DemoTime>18){ToggleBuild();DemoAction=12;}
 else if(DemoAction==12&&DemoTime>19){Player->PressFire();DemoAction=13;}
 else if(DemoAction==13&&Player->GetSelectedPieceCount()==6){Player->ReleaseFire();DemoAction=14;}
 else if(DemoAction==14&&DemoTime>22){Player->RecallShield();DemoAction=15;}
 else if(DemoAction==15&&DemoTime>24){Player->PressGuard();DemoAction=16;}
 if(!FScreenshotRequest::IsScreenshotRequested()){
  double AudioTime=0;if(auto Device=GetWorld()->GetAudioDevice())AudioTime=Device->GetAudioClock()-DemoAudioStartedAt;
  DemoFrameTimes+=FString::Printf(TEXT("%d,%.6f,%.6f\n"),DemoFrame,DemoTime,AudioTime);
  const FString Frame=FPaths::ProjectSavedDir()/TEXT("MotionCapture")/FString::Printf(TEXT("Frame_%05d.png"),DemoFrame++);
  FScreenshotRequest::RequestScreenshot(Frame,true,false);
 }
}

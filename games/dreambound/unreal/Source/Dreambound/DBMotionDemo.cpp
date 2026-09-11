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
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/IConsoleManager.h"
#include "UnrealClient.h"
#include "Kismet/GameplayStatics.h"

// Scripted visual/audio evidence, not a natural playthrough or fun assessment.
void ADBGameMode::TickMotionDemo(float Dt)
{
 if(!Player||PulseTime<5.f)return;
 const FString CaptureFolder=FPaths::ProjectSavedDir()/(bMotionFrameStudy?TEXT("PlayerMotionStudy"):TEXT("CombatFeelCapture"));
 if(!bDemoRecording){
  bDemoRecording=true;DemoTime=0;DemoFrame=0;DemoAction=0;DemoMissedFrames=0;DemoNextFrameAt=0;DemoStartedAt=FPlatformTime::Seconds();
  IFileManager::Get().MakeDirectory(*CaptureFolder,true);
  FApp::SetUnfocusedVolumeMultiplier(1.f);GEngine->SetMaxFPS(60.f);
  // Recording needs silent buffers too; submix auto-disable otherwise removes
  // the pauses between impacts and desynchronizes the exported soundtrack.
  if(auto* RenderSilence=IConsoleManager::Get().FindConsoleVariable(TEXT("au.NeverDisableSubmixes")))RenderSilence->Set(1,ECVF_SetByCode);
  DemoFrameTimes=TEXT("frame,wall_seconds,audio_seconds,shield_expansion,selected,full_charge,charge_audio,speed_cm_s,stamina,sprinting,dashing,x,y,z,simulation_seconds,dt,attack_recovery,weapon_x,weapon_y,weapon_z,weapon_pitch,weapon_yaw,weapon_roll\n");
  if(bMotionFrameStudy){
   // Actor ticks run inside a task time context. Configure the engine clock,
   // not only that tick's inherited FApp time, so the next frame is fixed too.
   GEngine->bUseFixedFrameRate=true;GEngine->FixedFrameRate=30.f;GEngine->SetMaxFPS(30.f);
   return; // Begin sampling after the first complete fixed simulation step.
  }
  if(auto Device=GetWorld()->GetAudioDevice())DemoAudioStartedAt=Device->GetAudioClock();
  UAudioMixerBlueprintLibrary::StartRecordingOutput(this,37.f);
 }
 const double WallTime=FPlatformTime::Seconds()-DemoStartedAt;
 if(bMotionFrameStudy&&FMath::Abs(Dt-1.f/30.f)>.0001f){
  FFileHelper::SaveStringToFile(FString::Printf(TEXT("Invalid engine dt %.6f; frame study rejected.\n"),Dt),*(CaptureFolder/TEXT("TimingFailure.txt")));
  FGenericPlatformMisc::RequestExit(false);return;
 }
 if(!bMotionFrameStudy)DemoTime=static_cast<float>(WallTime);
 RunSeconds+=Dt;EventRemaining=FMath::Max(0.f,EventRemaining-Dt);
 if(DemoTime>36.f){
  if(!bDemoStopped){
   bDemoStopped=true;Player->SuspendCombatInput();Player->GetCharacterMovement()->StopMovementImmediately();
   if(!bMotionFrameStudy)UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,TEXT("CourtyardMix"),FPaths::ConvertRelativePathToFull(CaptureFolder));
   const bool bAudioOnly=FParse::Param(FCommandLine::Get(),TEXT("DBAudioOnly"));
   FString Note=FString::Printf(TEXT("Scripted engine capture. No ordinary-input, complete-run or fun claim.\nsamples=%d seconds=%.3f wall_seconds=%.3f fixed_30_hz=%d missed_frames=%d audio_recorded=%d screenshot_capture=%d\nThis fixture is not a performance measurement.\n"),DemoFrame,DemoTime,WallTime,bMotionFrameStudy?1:0,DemoMissedFrames,bMotionFrameStudy?0:1,bAudioOnly?0:1);
   FFileHelper::SaveStringToFile(Note,*(CaptureFolder/TEXT("Capture.txt")));
   FFileHelper::SaveStringToFile(DemoFrameTimes,*(CaptureFolder/TEXT("Frames.csv")));
  }
  if(bMotionFrameStudy||DemoTime>38.f)FGenericPlatformMisc::RequestExit(false);
  return;
 }
 auto* PC=Cast<APlayerController>(Player->GetController());if(!PC)return;
 FVector Aim(-500,-500,160);
 // Keep the walking/sprinting study level and facing along its route. A fixed
 // world-space look-at point pitches the view into the sky when passed.
 if(DemoTime>30.f)Aim=Player->GetPawnViewLocation()+FVector(1000,300,0);
 PC->SetControlRotation(FMath::RInterpTo(PC->GetControlRotation(),(Aim-Player->GetPawnViewLocation()).Rotation(),Dt,4.f));
 // This controlled fixture makes each state visible. Upgrades and incoming
 // damage here are scripted, independently of the earned-progression checks.
 if(DemoAction==0){
  Player->SetActorLocation(FVector(-1000,-500,110));Player->GetCharacterMovement()->StopMovementImmediately();
  BeginRewardPractice("Study");bSliceAwaitingStart=false;PracticeInstruction="SCRIPTED COMBAT STUDY / fold, block, partial launch, full charge and melee. Targets are staged and invulnerable.";
  EventRemaining=0;DemoAction=1;
 }
 else if(DemoAction==1&&DemoTime>2){Player->PressGuard();DemoAction=2;}
 else if(DemoAction==2&&DemoTime>4){Player->ReleaseGuard();DemoAction=3;}
 else if(DemoAction==3&&DemoTime>5){Player->PressFire();DemoAction=4;}
 else if(DemoAction==4&&Player->GetSelectedPieceCount()>=3){Player->ReleaseFire();DemoAction=5;}
 else if(DemoAction==5&&DemoTime>7.5f){Player->RecallShield();DemoAction=6;}
 else if(DemoAction==6&&DemoTime>10){Player->PressFire();DemoAction=7;}
 else if(DemoAction==7&&Player->IsFullChargeReady()&&DemoTime>12.3f){Player->ReleaseFire();DemoAction=8;}
 else if(DemoAction==8&&DemoTime>14.5f){Player->RecallShield();DemoAction=9;}
 else if(DemoAction==9&&DemoTime>17){Player->SetActorLocation(FVector(-775,-500,110));Player->PressFire();Player->ReleaseFire();DemoAction=10;}
 else if(DemoAction==10&&DemoTime>17.8f){Player->PressFire();Player->ReleaseFire();DemoAction=11;}
 else if(DemoAction==11&&DemoTime>18.6f){Player->PressFire();Player->ReleaseFire();DemoAction=12;}
 else if(DemoAction==12&&DemoTime>20){
  // Keep the three-hit chain continuous, then show the separate heavy stroke
  // from its own combat distance instead of inside the invulnerable dummy.
  Player->SetActorLocation(FVector(-775,-500,94.15f));Player->GetCharacterMovement()->StopMovementImmediately();
  Player->UseSpecial();DemoAction=13;
 }
 else if(DemoAction==13&&DemoTime>22){
  // Give this isolated evasion room to travel beside the threat. The melee
  // endpoint is against the target capsule and only demonstrates a blocked dash.
  Player->SetActorLocation(FVector(-1050,-600,94.15f));Player->GetCharacterMovement()->StopMovementImmediately();
  Player->AddMovementInput(FVector(0,1,0),1.f);Player->Dash();DemoAction=14;
 }
 else if(DemoAction==14&&DemoTime>24){Player->GetCharacterMovement()->StopMovementImmediately();Player->PressGuard();DemoAction=15;}
 else if(DemoAction==15&&DemoTime>25){Player->ReceiveAttack(16,Player->GetPawnViewLocation()+Player->GetAimDirection()*500,false,nullptr,12001);DemoAction=16;}
 else if(DemoAction==16&&DemoTime>28.5f){Player->ReleaseGuard();DemoAction=17;}
 else if(DemoAction==17&&DemoTime>30){Player->SetActorLocation(FVector(-1250,-850,110));DemoAction=18;}
 // The final approach contrasts walking, held sprint, a jump and landing.
 // Scripted movement uses the ordinary CharacterMovement input path.
 if(DemoTime>30.f&&DemoTime<33.f){
  if(DemoTime>31.2f&&DemoAction==18){Player->PressSprint();DemoAction=19;}
  const FVector Forward=PC->GetControlRotation().Vector().GetSafeNormal2D();
  Player->AddMovementInput(Forward,1.f);
 }
 if(DemoTime>=33.f&&DemoAction==19){Player->ReleaseSprint();Player->Jump();DemoAction=20;}
 if(DemoTime>=33.25f&&DemoAction==20){Player->StopJumping();DemoAction=21;}
 if((bMotionFrameStudy||DemoTime>=DemoNextFrameAt)&&!FScreenshotRequest::IsScreenshotRequested()){
  DemoNextFrameAt=DemoTime+.10f;
  double AudioTime=-1;if(!bMotionFrameStudy)if(auto Device=GetWorld()->GetAudioDevice())AudioTime=Device->GetAudioClock()-DemoAudioStartedAt;
  const FVector Location=Player->GetActorLocation();
  const FVector Weapon=Player->WeaponRoot->GetRelativeLocation();
  const FRotator WeaponRotation=Player->WeaponRoot->GetRelativeRotation();
  DemoFrameTimes+=FString::Printf(TEXT("%d,%.6f,%.6f,%.3f,%d,%d,%d,%.3f,%.3f,%d,%d,%.3f,%.3f,%.3f,%.6f,%.6f,%.6f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n"),DemoFrame,WallTime,AudioTime,Player->GetShieldExpansion(),Player->GetSelectedPieceCount(),Player->IsFullChargeReady()?1:0,Player->IsChargeLoopPlaying()?1:0,Player->GetVelocity().Size2D(),Player->Stamina,Player->IsSprinting()?1:0,Player->IsDashing()?1:0,Location.X,Location.Y,Location.Z,DemoTime,Dt,Player->AttackRecovery,Weapon.X,Weapon.Y,Weapon.Z,WeaponRotation.Pitch,WeaponRotation.Yaw,WeaponRotation.Roll);
  const FString Frame=CaptureFolder/FString::Printf(TEXT("Frame_%05d.png"),DemoFrame++);
  // Audio auditions retain event timing without hundreds of GPU readbacks.
  if(!FParse::Param(FCommandLine::Get(),TEXT("DBAudioOnly")))FScreenshotRequest::RequestScreenshot(Frame,true,false);
 }
 else if(bMotionFrameStudy)++DemoMissedFrames;
 if(bMotionFrameStudy)DemoTime+=Dt;
}

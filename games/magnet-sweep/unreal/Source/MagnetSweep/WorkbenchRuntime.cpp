#include "WorkbenchRuntime.h"
#include "MagnetGame.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "InputKeyEventArgs.h"
#include "UnrealClient.h"
#include "Sound/SoundBase.h"

using namespace MagnetSweep;
FWorkbenchImpl::FWorkbenchImpl(AMagnetWorkbench* InOwner):Owner(InOwner) {}
void FWorkbenchImpl::Start()
{
 PC=Owner->GetWorld()->GetFirstPlayerController();
 FParse::Value(FCommandLine::Get(),TEXT("DemoProfile="),Profile);
 for(int32 I=0;I<Profile.Len();++I)if(!FChar::IsAlnum(Profile[I])&&Profile[I]!=TCHAR('_'))Profile[I]=TCHAR('_');
 Profile=Profile.Left(40);if(Profile.IsEmpty())Profile=TEXT("player");
 bQA=FParse::Param(FCommandLine::Get(),TEXT("DemoQA"));
 SavePath=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Rework"),Profile+TEXT(".json"));
 CareerSavePath=SavePath;
 const bool Loaded=!FParse::Param(FCommandLine::Get(),TEXT("DemoFresh"))&&Load();
 if(!Loaded)Tutorial.Start();
 SetupScene();BuildPieces();DisplayUpgrade=Model.GetUpgradeLevel();MagnetTarget=Magnet;bReceipt=Model.IsJobEnded();bFailureShown=Model.IsJobEnded();if(Loaded&&Model.IsCargoUnsafe())bPaused=true;
 Tutorial.HoldFirstRisk(Model.IsCargoUnsafe());
 Toast(Loaded?TEXT("Back at the workbench"):TEXT("First contract: recover 120 credits"),Loaded?TEXT("Your cargo, fuel and career are saved."):TEXT("Four fuel charges. Carry up to 24 kg safely. Copper and alloy pay more."),8);
 Save();WriteTelemetry();
 if(GEngine&&GEngine->GameViewport)InputHandle=GEngine->GameViewport->OnInputKey().AddLambda([this](const FInputKeyEventArgs& E){
  if(!E.Viewport)return;
  if(E.Key==EKeys::RightMouseButton&&E.Event==IE_Pressed){UpdatePointer(FVector2D(E.Viewport->GetMouseX(),E.Viewport->GetMouseY()));Vent();return;}
  if(E.Key!=EKeys::LeftMouseButton||(E.Event!=IE_Pressed&&E.Event!=IE_Released))return;
  UpdatePointer(FVector2D(E.Viewport->GetMouseX(),E.Viewport->GetMouseY()));
  if(E.Event==IE_Pressed){bMouseWasDown=true;HandlePress();}else{HandleRelease();bMouseWasDown=false;}
 });
}
void FWorkbenchImpl::Stop()
{
 UE_LOG(LogTemp,Display,TEXT("REWORK_SESSION seconds=%.1f frame_average_ms=%.2f music_playing_observed=%d field_audio_observed=%d warning_audio_observed=%d smelt_audio=%d reward_audio=%d overload_audio=%d sounds_loaded=%d"),SessionSeconds,FrameAverage*1000,bObservedMusic,bObservedFieldSound,bObservedWarningSound,SmeltAudioEvents,RewardAudioEvents,OverloadAudioEvents,Sounds.Num());
 if(InputHandle.IsValid()&&GEngine&&GEngine->GameViewport)GEngine->GameViewport->OnInputKey().Remove(InputHandle);InputHandle.Reset();
 for(auto* A:{MusicAudio,MagnetAudio,WarningAudio,FurnaceAudio})if(A)A->Stop();for(auto& A:PlayingSounds)if(A.IsValid())A->Stop();
}
void FWorkbenchImpl::Play(FName Name,float Volume,float Pitch)
{
 if(bMuted)return;
 if(Name==TEXT("smelt"))SmeltAudioEvents++;
 if(Name==TEXT("payout")||Name==TEXT("upgrade"))RewardAudioEvents++;
 if(Name==TEXT("overload"))OverloadAudioEvents++;
 const bool Light=Name.ToString().StartsWith(TEXT("pickup_metal"));
 PickupVoices.RemoveAll([](const auto& A){return !A.IsValid()||!A->IsPlaying();});
 if(Light&&PickupVoices.Num()>=2){if(PickupVoices[0].IsValid())PickupVoices[0]->Stop();PickupVoices.RemoveAt(0);}
 if(Name==TEXT("pickup_heavy")&&HeavyVoice.IsValid()&&HeavyVoice->IsPlaying())return;
 if(auto* S=Sounds.Find(Name)){auto* A=UGameplayStatics::SpawnSound2D(Owner,*S,Volume*SfxVolume,Pitch);if(A){PlayingSounds.Add(A);if(Light)PickupVoices.Add(A);if(Name==TEXT("pickup_heavy"))HeavyVoice=A;if(bPaused)A->SetPaused(true);}}
}
void FWorkbenchImpl::UpdateAudio(float Delta)
{
 auto Loop=[&](UAudioComponent*& A,FName Name,float Gain){
  if(!A)if(auto* S=Sounds.Find(Name)){A=UGameplayStatics::SpawnSound2D(Owner,*S,0,1,0,nullptr,false,false);if(A)Owner->Resources.Add(A);}
  if(A){A->SetVolumeMultiplier(bMuted?0:Gain);A->SetPaused(bPaused);}
 };
 const bool Active=Action==EMagnetAction::Sweep&&!bWorkshop&&!bReceipt&&!bPaused;
 FieldIntensity=FMath::FInterpTo(FieldIntensity,Active?1.f:0.f,Delta,12.f);
 Loop(MusicAudio,TEXT("workshop_music"),MusicVolume*(Model.IsCargoUnsafe()?.55f:1.f));
 Loop(MagnetAudio,TEXT("magnet_loop"),FieldIntensity*SfxVolume*.6f);
 if(MagnetAudio)MagnetAudio->SetPitchMultiplier(.88f+.13f*Model.GetForceScale()+.15f*float(Model.GetCargoMass())/Model.GetCapacity());
 Loop(WarningAudio,TEXT("warning_loop"),Model.IsCargoUnsafe()&&!Tutorial.bRiskHeld&&!bWorkshop&&!bReceipt?SfxVolume*.65f:0.f);
 if(WarningAudio)WarningAudio->SetPitchMultiplier(1.f+.35f*Model.GetFuseElapsed()/Model.GetFuseDuration());
 Loop(FurnaceAudio,TEXT("furnace_loop"),SfxVolume*(PourTimer>0?.32f:.07f));
 bObservedMusic|=MusicAudio&&MusicAudio->IsPlaying()&&!bMuted&&MusicVolume>0;
 bObservedFieldSound|=MagnetAudio&&MagnetAudio->IsPlaying()&&!bMuted&&FieldIntensity>.5f;
 bObservedWarningSound|=WarningAudio&&WarningAudio->IsPlaying()&&!bMuted&&Model.IsCargoUnsafe();
}
void FWorkbenchImpl::Toast(const FString& Title,const FString& Body,float Duration){NoticeTitle=Title;NoticeBody=Body;NoticeTimer=Duration;}
FVector2D FWorkbenchImpl::Project(FVector V) const{FVector2D Result;PC->ProjectWorldLocationToScreen(V,Result);return Result;}
void FWorkbenchImpl::UpdatePointer(FVector2D EventPosition)
{
 float X=EventPosition.X,Y=EventPosition.Y;if((X<0||Y<0)&&!PC->GetMousePosition(X,Y)){bWorldHit=bInTray=bFurnaceHover=false;return;}
 Pointer={X,Y};FVector Origin,Direction;bWorldHit=PC->DeprojectScreenPositionToWorld(X,Y,Origin,Direction)&&FMath::Abs(Direction.Z)>.001;
 if(bWorldHit){const FVector V=Origin+Direction*((10-Origin.Z)/Direction.Z);RawWorld={V.X,V.Y};}
 bInTray=bWorldHit&&FSalvageModel::IsInsideTray(RawWorld);bFurnaceHover=bWorldHit&&RawWorld.X>548&&RawWorld.X<768&&FMath::Abs(RawWorld.Y)<140;HoverRing=INDEX_NONE;
 if(HitButton()!=INDEX_NONE){bInTray=false;bFurnaceHover=false;}
 if(!bPaused&&!bWorkshop&&!bReceipt&&!TutorialIntro()&&bWorldHit&&(bInTray||bFurnaceHover))MagnetTarget=RawWorld;
}
int32 FWorkbenchImpl::HitButton() const{for(const auto& B:Buttons)if(B.Enabled&&B.Rect.IsInside(Pointer))return B.Id;return INDEX_NONE;}
void FWorkbenchImpl::HandlePress()
{
 bHaulDrag=false;
 const int32 UI=HitButton();if(UI!=INDEX_NONE){Action=EMagnetAction::UI;Button(UI);return;}
 if(bPaused||bWorkshop||bReceipt||TutorialIntro()||PourTimer>0||ForgeTimer>0)return;
 if(bFurnaceHover){Action=EMagnetAction::Dump;Deposit();return;}
 if(bInTray&&!Tutorial.bRiskHeld&&!Model.IsJobEnded()&&!Model.IsJobFailed()){bHaulDrag=true;Action=EMagnetAction::Sweep;Play(TEXT("magnet_on"),.65f);LastEvent=TEXT("field_on");}
}
void FWorkbenchImpl::HandleRelease()
{
 const bool DropOnFurnace=bHaulDrag&&(bFurnaceHover||HitButton()==40); bHaulDrag=false;
 const bool WasSweeping=Action==EMagnetAction::Sweep;
 if(WasSweeping&&!bFieldLatched)Play(TEXT("magnet_off"),.4f);
 Action=bFieldLatched?EMagnetAction::Sweep:EMagnetAction::None;AimedRing=INDEX_NONE;Preview={};
 LastEvent=bFieldLatched?TEXT("field_latched"):TEXT("field_off");
 if(!bFieldLatched&&WasSweeping)TutorialChanged(Tutorial.ObserveFieldOff());
 if(DropOnFurnace&&Model.GetCargoMass()>0)Deposit();
 SaveTimer=.3f;
}
void FWorkbenchImpl::SweepPath() {} // No instantaneous swept-point capture.
void FWorkbenchImpl::UpdateAttraction(float Delta)
{
 const bool Active=Action==EMagnetAction::Sweep&&bWorldHit;FieldRadius=Model.GetFieldRadius()*(bPrecision?.48f:1.f);TSet<int32> Visited;
 for(const FSalvagePiece& P:Model.GetPieces())
 {
  if(P.State!=EPieceState::Available||Visited.Contains(P.Id))continue;
  const auto Group=Model.GetCaptureGroup(P.Id);if(Group.IsEmpty())continue;
  FVector2D Center=FVector2D::ZeroVector;int32 Mass=0;
  for(int32 Id:Group){Visited.Add(Id);const auto* Q=Model.FindPiece(Id);Center+=Q->Position*Q->Mass;Mass+=Q->Mass;}Center/=FMath::Max(1,Mass);
  const FVector2D Toward=Magnet-Center;const float Distance=Toward.Size();auto* First=Visuals.Find(Group[0]);if(!First)continue;
  const bool CanTake=CanCaptureForPlayer(P.Id);const bool GuardBlocked=Tutorial.IsTrainingGuardActive()&&!CanTake;const bool Pulling=Active&&Distance<FieldRadius&&First->CaptureDelay<=0&&!GuardBlocked;FVector2D Velocity=First->Velocity;
  if(Pulling){const float Falloff=1.f-FMath::Clamp(Distance/FieldRadius,0.f,1.f);const float Acceleration=(2000+9000*Falloff)*Model.GetForceScale()/FMath::Sqrt(float(Mass));Velocity+=Toward.GetSafeNormal()*Acceleration*Delta*(bPrecision?.8f:1.f);Velocity*=FMath::Exp(-Delta*3.f);if(!CanTake&&Distance<48)Velocity-=Toward.GetSafeNormal()*4000*Delta;}
  else Velocity*=FMath::Exp(-Delta*9.f);
  if(Velocity.Size()>650)Velocity=Velocity.GetSafeNormal()*650;FVector2D Shift=Velocity*Delta;
  for(int32 Id:Group){const auto* Q=Model.FindPiece(Id);const auto Clamped=FSalvageModel::ClampToTray(Q->Position+Shift);Shift=Clamped-Q->Position;}
  for(int32 Id:Group){const auto* Q=Model.FindPiece(Id);Model.MoveAvailablePiece(Id,Q->Position+Shift);if(auto* V=Visuals.Find(Id)){V->Velocity=Velocity;V->Attracted=GuardBlocked&&Active&&Distance<FieldRadius?-.35f:(Pulling?(CanTake?1.f:-1.f):0.f);V->CaptureDelay=FMath::Max(0.f,V->CaptureDelay-Delta);}}
  if(Pulling&&CanTake&&Distance<28){const auto PreviousStep=Tutorial.Step;OnRecovery(Model.CapturePieces(Group),Group.Num()>1,Group.Num());if(Tutorial.bRiskHeld||Tutorial.Step!=PreviousStep||Action!=EMagnetAction::Sweep)return;}
 }
}
void FWorkbenchImpl::OnRecovery(const FRecoveryResult& R,bool Bundle,int32 Count)
{
 if(!R.Succeeded())return;RecoveryTimer=FMath::Max(RecoveryTimer,.22f);Impact=FMath::Min(1.f,Impact+.08f*R.Mass);bool Rare=false;
 for(int32 Id:R.PieceIds)if(auto* V=Visuals.Find(Id)){V->From=V->Position;V->Progress=0;V->Duration=.2f;V->bPouring=false;V->Velocity=FVector2D::ZeroVector;const auto* P=Model.FindPiece(Id);if(P&&P->Material==EMaterial::Core&&!SeenRare.Contains(Id)){Rare=true;SeenRare.Add(Id);}AddBurst(V->Position,P&&P->Material==EMaterial::HotCell?FLinearColor(1,.16,.03):FLinearColor(.45,1,.8),4,70);}
 if(Rare){Play(TEXT("rare_find"),.8f);Toast(TEXT("Rare core secured — still at risk"),TEXT("Smelt it to keep the collectible and its payout."),3);}
 else if(Bundle||R.Mass>=8)Play(TEXT("pickup_heavy"),.8f,.96f);
 else if(PickupCooldown<=0){Play(FName(*FString::Printf(TEXT("pickup_metal%d"),1+Sweeps%3)),.7f,.94f+(Sweeps%4)*.03f);PickupCooldown=.055f;}
 Popups.Add({World(Magnet,125),FString::Printf(TEXT("%d cr in haul   %d kg"),R.Amount,R.Mass),Rare?FLinearColor(1,.78,.28):FLinearColor(.7,.96,.9),1.15f});
 const bool LessonChanged=Tutorial.ObserveCapture(Bundle,bPrecision,R.Amount>0,Model.GetCargoMass());
 const bool TeachingHold=Tutorial.HoldFirstRisk(Model.IsCargoUnsafe());
 TutorialChanged(LessonChanged||TeachingHold);
 if(Tutorial.IsTrainingGuardActive()&&(Model.GetCargoMass()>=Model.GetCapacity()||(Model.GetBanked()==0&&Model.GetCargo()>=36))){
  bFieldLatched=false;Action=EMagnetAction::None;
  Toast(TEXT("Practice load ready"),TEXT("Click SMELT HAUL by the furnace to turn this mixed load into credits."),5);
 }
 Sweeps++;if(Bundle)Pulls++;LastBurst=R.Amount;LastEvent=TEXT("physical_capture");SaveTimer=.3f;
}
void FWorkbenchImpl::SpillVisuals(const TArray<int32>& Released,const TArray<int32>& Destroyed,bool Failure)
{
 for(int32 Id:Released)if(auto* V=Visuals.Find(Id)){V->From=V->Position;V->Progress=0;V->Duration=.4f;V->CaptureDelay=.9f;V->Velocity=FVector2D::ZeroVector;V->bPouring=false;V->Mesh->SetVisibility(true);}
 for(int32 Id:Destroyed)if(auto* V=Visuals.Find(Id)){AddBurst(V->Position,FLinearColor(1,.22,.025),24,260);V->Mesh->SetVisibility(false);for(auto* D:V->Details)D->SetVisibility(false);}
 AddBurst(World(Magnet,65),Failure?FLinearColor(1,.3,.04):FLinearColor(.42,.86,1),Failure?50:18,Failure?330:200);Impact=Failure?1.4f:.4f;
}
void FWorkbenchImpl::Vent()
{
 if(bPaused||bWorkshop||bReceipt||TutorialIntro()||PourTimer>0||ForgeTimer>0)return;bFieldLatched=false;Action=EMagnetAction::None;const auto R=Model.VentCargo(Magnet);
 if(R.Changed()){SpillVisuals(R.ReleasedPieceIds,R.DestroyedPieceIds,false);Play(TEXT("vent"),.85f);Toast(TEXT("Haul dropped — fuel and salvage saved"),TEXT("Everything is back on the tray. Use precision to rebuild a safe load."),3);LastEvent=TEXT("drop_haul");TutorialChanged(Tutorial.ObserveDrop(true));Save();}
 else Toast(TEXT("Nothing to drop"),TEXT("Right-click releases your entire haul for recovery."),2);
}
void FWorkbenchImpl::Deposit(bool Next)
{
 if(bPaused||bWorkshop||bReceipt||TutorialIntro()||PourTimer>0||ForgeTimer>0)return;
 if(Tutorial.bRiskHeld){Toast(TEXT("Unsafe haul — drop it back onto the tray"),TEXT("RMB drops everything recoverably. Then collect a smaller safe load and click SMELT HAUL."),5);return;}
 FString Reason;if(!Model.CanSmelt(Reason)){Toast(TEXT("Cannot smelt this load"),Reason,3);Play(TEXT("ui_click"),.35f,.7f);return;}
 bFieldLatched=false;Action=EMagnetAction::None;
 if(RecoveryTimer>0){bPendingDeposit=true;bPendingNext=Next;return;}if(PourTimer>0||ForgeTimer>0)return;
 bPendingDeposit=false;LastBank=Model.BankCargo();if(LastBank.Amount<=0)return;
 bFieldLatched=false;Action=EMagnetAction::None;Deposits++;PourTimer=2.6f;FurnacePulse=1;Play(TEXT("smelt"),.7f);bPendingNext=Next;
 for(int32 Id:LastBank.PieceIds)if(auto* V=Visuals.Find(Id)){V->From=V->Position;V->Progress=0;V->Duration=.7f+.023f*(Id%17);V->bPouring=true;V->Mesh->SetVisibility(true);}
 LastEvent=TEXT("smelt_started");Toast(FString::Printf(TEXT("Smelting %d kg  /  +%d credits"),LastBank.Mass,LastBank.CashAwarded),FString::Printf(TEXT("%d / %d contract value  —  %d fuel charges remain"),Model.GetBanked(),Model.GetGoal(),Model.GetHeatsRemaining()),3.5f);Save();
}
void FWorkbenchImpl::NextDelivery()
{
 if(PourTimer>0||ForgeTimer>0){bPendingNext=true;return;}if(Model.GetCargo()>0){Deposit(true);return;}
 if(Model.FinishJob()||Model.IsJobEnded()||Model.IsJobFailed()){bReceipt=false;bWorkshop=true;Action=EMagnetAction::None;Save();}
}
void FWorkbenchImpl::BeginJob(int32 Job)
{
 const int32 Seed=12092026+Model.GetCompletedDeliveryCount()*977+Job*31;
 if(!Model.StartJob(Job,Seed)){Toast(TEXT("Finish the current contract first"),TEXT("Bank the target, or retry the current tray from the pause menu."),4);return;}
 BuildPieces();bWorkshop=bReceipt=bFailureShown=false;LastBank={};RecoveryTimer=PourTimer=ForgeTimer=0;Action=EMagnetAction::None;Magnet=MagnetTarget=FVector2D(-350,-245);MagnetVelocity=FVector2D::ZeroVector;
 Toast(Model.GetJob().Name,FString::Printf(TEXT("Recover %d credits with 4 fuel charges. Gold target: %d."),Model.GetGoal(),Model.GetGoldGoal()),5);Save();LastEvent=TEXT("job_started");
}
void FWorkbenchImpl::Pause(bool Value)
{
 bPaused=Value;bFieldLatched=false;bHaulDrag=false;Action=EMagnetAction::None;AimedRing=INDEX_NONE;Preview={};bConfirmRetry=bConfirmNew=false;LastEvent=Value?TEXT("paused"):TEXT("resumed");for(auto& A:PlayingSounds)if(A.IsValid())A->SetPaused(Value);Save();
}
void FWorkbenchImpl::Button(int32 Id)
{
 Play(TEXT("ui_click"),.65f);
 if(Id>=100&&Id<103){
  const bool GuardWasOn=Tutorial.IsTrainingGuardActive();
  if(Model.PurchaseUpgrade(static_cast<EUpgrade>(Id-100))){
   TutorialChanged(Tutorial.ObservePurchase(Id-100));DisplayUpgrade=Model.GetUpgradeLevel();ForgeTimer=.85f;Impact=.6f;Play(TEXT("upgrade"),.9f);
   Toast(GuardWasOn?TEXT("Upgrade fitted — training guard off"):TEXT("Upgrade fitted"),GuardWasOn?(Tutorial.bRiskLearned?TEXT("Red cells and overloads now run a real fuse. RMB returns the whole haul to the tray."):TEXT("Your first dangerous pickup will pause for rescue practice. Try your improved magnet.")):TEXT("Your permanent rig improvement is ready for the next haul."),7);Save();
  }return;
 }
 if(Id>=200&&Id<206){BeginJob(Id-200);return;}
 switch(Id){
 case 1:NextDelivery();break;
 case 2:Pause(true);bConfirmRetry=true;break;
 case 3:Pause(!bPaused);break;
 case 4:bMuted=!bMuted;for(auto& A:PlayingSounds)if(A.IsValid())A->SetVolumeMultiplier(bMuted?0:SfxVolume);Save();break;
 case 5:Pause(true);bConfirmNew=true;break;
 case 6:Save();UKismetSystemLibrary::QuitGame(Owner,PC,EQuitPreference::Quit,false);break;
 case 7:Pause(false);break;
 case 8:Model.RetryJob(false);Tutorial.HoldFirstRisk(false);BuildPieces();bWorkshop=bReceipt=bFailureShown=false;RecoveryTimer=PourTimer=ForgeTimer=0;bPendingDeposit=bPendingNext=false;Pause(false);Toast(TEXT("Contract restocked"),TEXT("Banked cash and upgrades are safe. Four new fuel charges."),3);break;
 case 9:Model=FSalvageModel();Tutorial.Start();BuildPieces();bWorkshop=bReceipt=bFailureShown=false;LastBank={};RecoveryTimer=PourTimer=ForgeTimer=0;bPendingDeposit=bPendingNext=false;Pause(false);break;
 case 10:MusicVolume=MusicVolume>=.99f?0:FMath::Min(1.f,MusicVolume+.25f);Save();break;
 case 11:SfxVolume=SfxVolume>=.99f?0:FMath::Min(1.f,SfxVolume+.25f);Save();break;
 case 20:if(Tutorial.bRiskHeld){Toast(TEXT("Practise Drop haul first"),TEXT("Right-click on the tray. This first warning is waiting safely."),3);break;}Pause(false);bWorkshop=true;Action=EMagnetAction::None;break;
 case 21:bWorkshop=false;break;
 case 30:bReceipt=false;bWorkshop=true;break;
 case 40:Deposit();break;
 case 31:Model.AbandonJob();bReceipt=false;bWorkshop=true;Pause(false);Save();break;
 case 450:bPaused=bWorkshop=bReceipt=false;TutorialChanged(Tutorial.Begin());break;
 case 451:if(Tutorial.bRiskHeld){const bool WasPaused=bPaused,WasWorkshop=bWorkshop,WasReceipt=bReceipt;bPaused=bWorkshop=bReceipt=false;Vent();bPaused=WasPaused;bWorkshop=WasWorkshop;bReceipt=WasReceipt;}TutorialChanged(Tutorial.Skip());break;
 case 452:TutorialChanged(Tutorial.Finish());Toast(TEXT("Ready for the next haul"),TEXT("Your credits, recovered cores and rig stay yours."),4);break;
 case 460:EnterTutorialPractice();break;
 case 461:ReturnFromTutorial();break;
 }
}
void FWorkbenchImpl::Tick(float Delta)
{
 if(!PC)return;int32 W,H;PC->GetViewportSize(W,H);Width=FMath::Max(W,1);Height=FMath::Max(H,1);UIScale=FMath::Max(.01f,FMath::Min(Width/1600.f,Height/900.f));UX=(Width-1600*UIScale)*.5f;UY=(Height-900*UIScale)*.5f;FrameAverage=FMath::Lerp(FrameAverage,Delta,.025f);
 bool Focused=true;if(GEngine&&GEngine->GameViewport&&GEngine->GameViewport->Viewport)Focused=GEngine->GameViewport->Viewport->IsForegroundWindow();if(!Focused&&bWasFocused)Pause(true);bWasFocused=Focused;
 UpdatePointer();bPrecision=bPrecisionLatched||PC->IsInputKeyDown(EKeys::LeftShift)||PC->IsInputKeyDown(EKeys::RightShift);
 if(Focused){
  if(PC->WasInputKeyJustPressed(EKeys::Escape)){if(bWorkshop)bWorkshop=false;else if(bReceipt)bReceipt=false;else Pause(!bPaused);}
  if(PC->WasInputKeyJustPressed(EKeys::M))Button(4);
  if(PC->WasInputKeyJustPressed(EKeys::E))Deposit();
  if(PC->WasInputKeyJustPressed(EKeys::Q)&&!bPaused&&!bWorkshop&&!bReceipt&&!TutorialIntro())bPrecisionLatched=!bPrecisionLatched;
  if(PC->WasInputKeyJustPressed(EKeys::SpaceBar)&&!bPaused&&!bWorkshop&&!bReceipt&&!TutorialIntro()&&!Tutorial.bRiskHeld&&PourTimer<=0&&ForgeTimer<=0&&!Model.IsJobEnded()&&!Model.IsJobFailed()){bFieldLatched=!bFieldLatched;Action=bFieldLatched?EMagnetAction::Sweep:EMagnetAction::None;Play(bFieldLatched?TEXT("magnet_on"):TEXT("magnet_off"),.5f);if(!bFieldLatched)TutorialChanged(Tutorial.ObserveFieldOff());}
  if(PC->WasInputKeyJustPressed(EKeys::Tab)&&!bPaused&&!TutorialIntro()&&!Tutorial.bRiskHeld&&PourTimer<=0){bWorkshop=!bWorkshop;bFieldLatched=false;Action=EMagnetAction::None;}
  if(PC->WasInputKeyJustPressed(EKeys::R)&&!TutorialIntro()&&PourTimer<=0&&ForgeTimer<=0){Pause(true);bConfirmRetry=true;}
  if(PC->WasInputKeyJustPressed(EKeys::F11)){auto* S=GEngine->GetGameUserSettings();if(S->GetFullscreenMode()==EWindowMode::Windowed){WindowedSize={W,H};S->SetFullscreenMode(EWindowMode::WindowedFullscreen);}else{S->SetFullscreenMode(EWindowMode::Windowed);S->SetScreenResolution(WindowedSize);}S->ApplySettings(false);}
  if(PC->WasInputKeyJustPressed(EKeys::F9))FScreenshotRequest::RequestScreenshot(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Screenshots"),TEXT("MagnetSweepRework.png")),true,true);
 }
 Delta=FMath::Min(Delta,.05f);UpdateAudio(Delta);PlayingSounds.RemoveAll([](const auto& A){return !A.IsValid();});if(bPaused||bWorkshop||bReceipt||TutorialIntro()){if(bQA)WriteTelemetry();return;}
 Time+=Delta;SessionSeconds+=Delta;PickupCooldown-=Delta;NoticeTimer-=Delta;Impact=FMath::Max(0.f,Impact-Delta*4);
 const FVector2D Difference=MagnetTarget-Magnet;FVector2D Desired=Difference*15;if(Desired.Size()>1000)Desired=Desired.GetSafeNormal()*1000;MagnetVelocity=FMath::Lerp(MagnetVelocity,Desired,1-FMath::Exp(-Delta*18));Magnet+=MagnetVelocity*Delta;if(Difference.Size()<.5)Magnet=MagnetTarget;
 RecoveryTimer=FMath::Max(0.f,RecoveryTimer-Delta);if(PourTimer<=0&&ForgeTimer<=0&&!Tutorial.bRiskHeld&&!Model.IsJobEnded()&&!Model.IsJobFailed())UpdateAttraction(Delta);
 UpdateTutorial();
 const auto Spill=Model.AdvanceRisk(Tutorial.bRiskHeld?0.f:Delta,Magnet);
 if(Spill.bTripped){bFieldLatched=false;Action=EMagnetAction::None;RecoveryTimer=1.1f;SpillVisuals(Spill.ReleasedPieceIds,Spill.DestroyedPieceIds,true);Play(TEXT("overload"),1.f);Toast(FString::Printf(TEXT("Overload — 1 fuel and %d credits lost"),Spill.LostValue),TEXT("Emergency quench used a fuel charge. Remaining scrap spilled back; banked earnings are safe."),5);LastEvent=TEXT("overload_failure");Save();}
 if(Model.IsCargoUnsafe()&&!bRiskWasActive)LastEvent=TEXT("unstable_load");bRiskWasActive=Model.IsCargoUnsafe();
 if(bPendingDeposit&&RecoveryTimer<=0)Deposit(bPendingNext);if(SaveTimer>0){SaveTimer-=Delta;if(SaveTimer<=0)Save();}
 if(PourTimer>0){PourTimer-=Delta;if(PourTimer<=0){
  Play(TEXT("payout"),.85f);Impact=.55f;AddBurst(World({650,0},115),FLinearColor(1,.62,.14),45,200);
  if(LastBank.bCompletedNow)Play(TEXT("contract_success"),.8f);
  if(LastBank.NewPlayerLevel>LastBank.PreviousPlayerLevel)Toast(FString::Printf(TEXT("Level %d — new upgrades unlocked"),LastBank.NewPlayerLevel),FString::Printf(TEXT("+%d XP. Open the workshop to fit your next improvement."),LastBank.XPAwarded),5);
  else if(LastBank.bGoldNow)Toast(TEXT("Gold contract achieved"),FString::Printf(TEXT("+%d credits paid. Your record is saved."),LastBank.CashAwarded),4);
  else if(LastBank.bCompletedNow)Toast(TEXT("Contract target reached"),TEXT("Finish now, or use remaining batches to chase the gold bonus."),5);
  if(Model.IsJobFailed()){Play(TEXT("contract_fail"),.75f);bReceipt=true;bFailureShown=true;}else if(Model.GetHeatsRemaining()==0){Model.FinishJob();bReceipt=true;}
  TutorialChanged(Tutorial.ObserveSmelt(Model.IsDeliveryCompleted()));LastEvent=TEXT("smelt_finished");Save();
 }}
 if(ForgeTimer>0)ForgeTimer=FMath::Max(0.f,ForgeTimer-Delta);
 if(bPendingNext&&!bPendingDeposit&&RecoveryTimer<=0&&PourTimer<=0&&ForgeTimer<=0){bPendingNext=false;NextDelivery();}
 if(Model.IsJobEnded()&&!bFailureShown&&PourTimer<=0&&RecoveryTimer<=0){bFailureShown=true;bReceipt=true;if(Model.IsJobFailed())Play(TEXT("contract_fail"),.75f);Save();}
 FurnacePulse=FMath::Max(0.f,FurnacePulse-Delta*.7f);UpdateVisuals(Delta);LastMagnet=Magnet;if(bQA)WriteTelemetry();
}

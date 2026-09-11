#include "WorkbenchRuntime.h"
#include "MagnetGame.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Styling/CoreStyle.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/SoundBase.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "UnrealClient.h"
#include "InputKeyEventArgs.h"

using namespace MagnetSweep;
namespace {
const FVector2D Furnace(650,0);
FLinearColor C(float R,float G,float B,float A=1) {return FLinearColor(R,G,B,A);}
float Ease(float T) {return 1-FMath::Pow(1-FMath::Clamp(T,0.f,1.f),3);}
}
FWorkbenchImpl::FWorkbenchImpl(AMagnetWorkbench* InOwner):Owner(InOwner) {}
void FWorkbenchImpl::Start() {
 PC=Owner->GetWorld()->GetFirstPlayerController();
 FParse::Value(FCommandLine::Get(),TEXT("DemoProfile="),Profile);
 for(int32 I=0;I<Profile.Len();++I) if(!FChar::IsAlnum(Profile[I]) && Profile[I]!=TCHAR('_')) Profile[I]=TCHAR('_');
 Profile=Profile.Left(40); if(Profile.IsEmpty()) Profile=TEXT("player");
 bQA=FParse::Param(FCommandLine::Get(),TEXT("DemoQA"));
 SavePath=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("ConceptDemo"),Profile+TEXT(".json"));
 const bool bLoaded=!FParse::Param(FCommandLine::Get(),TEXT("DemoFresh")) && Load();
 SetupScene();BuildPieces();DisplayUpgrade=Model.GetUpgradeLevel();
 Toast(bLoaded?TEXT("Welcome back to the workbench"):TEXT("A little scrap. A lot of possibility."),
  bLoaded?TEXT("Your haul and magnet are right where you left them."):TEXT("Hold to gather loose metal. Drag a copper ring to pull a tangle."),7);
 Save();WriteTelemetry();
 if(GEngine&&GEngine->GameViewport)InputHandle=GEngine->GameViewport->OnInputKey().AddLambda([this](const FInputKeyEventArgs& E){
  if(E.Key!=EKeys::LeftMouseButton||!E.Viewport)return;
  if(E.Event!=IE_Pressed&&E.Event!=IE_Released)return;
  UpdatePointer(FVector2D(E.Viewport->GetMouseX(),E.Viewport->GetMouseY()));
  if(E.Event==IE_Pressed){bMouseWasDown=true;HandlePress();}else {HandleRelease();bMouseWasDown=false;}
 });
}
void FWorkbenchImpl::Stop(){if(InputHandle.IsValid()&&GEngine&&GEngine->GameViewport)GEngine->GameViewport->OnInputKey().Remove(InputHandle);InputHandle.Reset();}

UStaticMesh* FWorkbenchImpl::Mesh(FName Name) {
 if(auto* Found=Meshes.Find(Name)) return *Found;
 FString Path;
 if(Name==TEXT("Cube")||Name==TEXT("Cylinder")||Name==TEXT("Sphere")) Path=TEXT("/Engine/BasicShapes/")+Name.ToString()+TEXT(".")+Name.ToString();
 else Path=TEXT("/Game/MagnetSweep/Meshes/")+Name.ToString()+TEXT(".")+Name.ToString();
 UStaticMesh* Result=LoadObject<UStaticMesh>(nullptr,*Path);
 if(!Result) {UE_LOG(LogTemp,Warning,TEXT("Magnet missing mesh %s"),*Path); Result=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));}
 Owner->Resources.Add(Result);Meshes.Add(Name,Result);return Result;
}
UMaterialInstanceDynamic* FWorkbenchImpl::Mat(FName Name,FLinearColor Color,float Metal,float Rough,float Glow,bool bOverlay) {
 auto* M=UMaterialInstanceDynamic::Create(bOverlay&&Overlay?Overlay:Surface,Owner);
 M->SetVectorParameterValue(TEXT("Color"),Color);M->SetScalarParameterValue(TEXT("Metallic"),Metal);M->SetScalarParameterValue(TEXT("Roughness"),Rough);M->SetScalarParameterValue(TEXT("Glow"),Glow);
 Materials.Add(Name,M);Owner->Resources.Add(M);return M;
}
void FWorkbenchImpl::Place(UStaticMeshComponent* S,FVector P,FVector Size,FRotator R) {
 if(!S||!S->GetStaticMesh()) return;
 const auto B=S->GetStaticMesh()->GetBounds();
 FVector Scale=Size/(B.BoxExtent*2).ComponentMax(FVector(.01));
 S->SetWorldTransform(FTransform(R,P-R.RotateVector(B.Origin*Scale),Scale));
}
UStaticMeshComponent* FWorkbenchImpl::Shape(FName MeshName,FVector P,FVector Size,FName Material,FRotator R,bool Static) {
 auto* S=NewObject<UStaticMeshComponent>(Owner);
 S->SetMobility(EComponentMobility::Movable);S->SetStaticMesh(Mesh(MeshName));
 if(Materials.Contains(Material)) S->SetMaterial(0,Materials[Material]);
 S->SetCollisionEnabled(ECollisionEnabled::NoCollision);S->SetGenerateOverlapEvents(false);
 Owner->AddInstanceComponent(S);S->RegisterComponent();Place(S,P,Size,R);
 if(Static) StaticShapes.Add(S);return S;
}
void FWorkbenchImpl::SetupScene() {
 RuntimeFont=NewObject<UFont>(Owner);RuntimeFont->FontCacheType=EFontCacheType::Runtime;RuntimeFont->CompositeFont=*FCoreStyle::GetDefaultFont();Owner->Resources.Add(RuntimeFont);
 Surface=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/MagnetSweep/Materials/M_Surface.M_Surface"));
 if(!Surface) Surface=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
 Owner->Resources.Add(Surface);
 Mat(TEXT("Background"),C(.0015,.003,.005),0,.85);
 Mat(TEXT("Base"),C(.004,.012,.017),.12,.7);
 Mat(TEXT("Tray"),C(.002,.006,.010),0,.87);
 Mat(TEXT("Grid"),C(.034,.067,.070),.15,.65);
 Mat(TEXT("Edge"),C(.13,.20,.21),.6,.3);
 Mat(TEXT("Dark"),C(.009,.016,.018),.2,.6);
 Mat(TEXT("Steel"),C(.42,.57,.58),.65,.24);
 Mat(TEXT("Copper"),C(.7,.255,.075),.65,.28);
 Mat(TEXT("Brass"),C(.85,.52,.15),.7,.24);
 Mat(TEXT("Ivory"),C(.78,.81,.69),.25,.4);
 Mat(TEXT("Magnet"),C(.055,.38,.37),.4,.28);
 Mat(TEXT("Hot"),C(.8,.09,.008),.15,.3,.8);
 Mat(TEXT("Selected"),C(.36,1,.79),.4,.2,.6);
 Mat(TEXT("Ring"),C(1,.58,.2),.6,.26,.2);
 Camera=NewObject<UCameraComponent>(Owner);Owner->AddInstanceComponent(Camera);Camera->RegisterComponent();
 Camera->SetWorldLocation(FVector(80,920,1500));Camera->SetWorldRotation((FVector(80,0,0)-Camera->GetComponentLocation()).Rotation());
 Camera->ProjectionMode=ECameraProjectionMode::Orthographic;Camera->OrthoWidth=2100;Camera->bOverrideAspectRatioAxisConstraint=true;Camera->AspectRatioAxisConstraint=EAspectRatioAxisConstraint::AspectRatio_MaintainXFOV;Camera->bConstrainAspectRatio=false;
 Camera->OrthoNearClipPlane=1;Camera->OrthoFarClipPlane=10000;
 PC->SetViewTarget(Owner);
 auto* Key=Owner->GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector,FRotator(-56,-40,0));
 Key->GetLightComponent()->SetIntensity(5.5);Key->GetLightComponent()->SetLightColor(C(1,.84,.67));
 auto* Fill=Owner->GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector,FRotator(-38,145,0));
 Fill->GetLightComponent()->SetIntensity(1.3);Fill->GetLightComponent()->SetLightColor(C(.5,.77,1));Fill->GetLightComponent()->SetCastShadows(false);
 auto* Sky=Owner->GetWorld()->SpawnActor<ASkyLight>();Sky->GetLightComponent()->SetIntensity(.35);Sky->GetLightComponent()->bLowerHemisphereIsBlack=false;Sky->GetLightComponent()->LowerHemisphereColor=C(.22,.3,.34);Sky->GetLightComponent()->RecaptureSky();
 auto* PP=Owner->GetWorld()->SpawnActor<APostProcessVolume>();PP->bUnbound=true;
 PP->Settings.bOverride_AutoExposureMethod=true;PP->Settings.AutoExposureMethod=AEM_Manual;
 PP->Settings.bOverride_AutoExposureBias=true;PP->Settings.AutoExposureBias=0;
 PP->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;PP->Settings.AutoExposureApplyPhysicalCameraExposure=false;
 PP->Settings.bOverride_BloomIntensity=true;PP->Settings.BloomIntensity=.2f;
 PP->Settings.bOverride_VignetteIntensity=true;PP->Settings.VignetteIntensity=.25f;
 PP->Settings.bOverride_AmbientOcclusionIntensity=true;PP->Settings.AmbientOcclusionIntensity=.65f;
 Shape(TEXT("Cube"),FVector(80,0,-95),FVector(2600,2000,65),TEXT("Background"));
 Shape(TEXT("Cube"),FVector(80,0,-40),FVector(1470,800,58),TEXT("Base"));
 Shape(TEXT("SM_Tray"),FVector(0,0,-6),FVector(1040,660,22),TEXT("Tray"));
 for(int I=-4;I<=4;I++) Shape(TEXT("Cube"),FVector(I*100,0,5.3),FVector(.7,615,.5),TEXT("Grid"));
 for(int I=-2;I<=2;I++) Shape(TEXT("Cube"),FVector(0,I*100,5.3),FVector(995,.7,.5),TEXT("Grid"));
 for(int Side:{-1,1}) {
  Shape(TEXT("Cube"),FVector(Side*530,0,12),FVector(32,695,34),TEXT("Edge"));
  Shape(TEXT("Cube"),FVector(0,Side*337,12),FVector(1030,25,34),TEXT("Edge"));
  for(int Y:{-1,1}) Shape(TEXT("Cylinder"),FVector(Side*531,Y*324,33),FVector(15,15,7),TEXT("Brass"));
 }
 for(int I=0;I<7;I++) Shape(TEXT("Cube"),FVector(-420+I*125,-372,-7),FVector(68,10,2),I%2?TEXT("Dark"):TEXT("Copper"));
 Shape(TEXT("Cube"),FVector(660,0,8),FVector(215,280,66),TEXT("Dark"));
 Shape(TEXT("Cylinder"),FVector(650,0,30),FVector(208,208,38),TEXT("Copper"));
 Shape(TEXT("Cylinder"),FVector(650,0,55),FVector(174,174,8),TEXT("Dark"));
 for(int Z=62;Z<=95;Z+=8)Shape(TEXT("SM_Ring"),FVector(650,0,Z),FVector(203,203,12),Z==94?TEXT("Brass"):TEXT("Copper"));
 FurnaceFill=Shape(TEXT("Cylinder"),FVector(650,0,62),FVector(142,142,3),TEXT("Hot"));
 for(int Side:{-1,1}) for(int I=0;I<5;I++) Shape(TEXT("Cube"),FVector(650+(I-2)*34,Side*123,43),FVector(20,14,4),I%2?TEXT("Dark"):TEXT("Brass"));
 auto* Lamp=Owner->GetWorld()->SpawnActor<APointLight>(FVector(650,0,120),FRotator::ZeroRotator);
 FurnaceLight=Lamp->PointLightComponent;FurnaceLight->SetIntensity(1800);FurnaceLight->SetAttenuationRadius(480);FurnaceLight->SetLightColor(C(1,.3,.065));FurnaceLight->SetCastShadows(false);
 auto AddMagnet=[&](FName MeshName,FVector Offset,FVector Size,FName Material){
  auto* S=Shape(MeshName,World(Magnet,80)+Offset,Size,Material,FRotator::ZeroRotator,false);
  MagnetShapes.Add(S);MagnetOffsets.Add(Offset);MagnetSizes.Add(Size);return S;};
 AddMagnet(TEXT("SM_MagnetBody"),FVector(0,0,0),FVector(80,70,20),TEXT("Magnet"));
 for(int Side:{-1,1}) {
  AddMagnet(TEXT("Cube"),FVector(Side*27,-29,0),FVector(22,17,21),TEXT("Ivory"));
  AddMagnet(TEXT("Cube"),FVector(Side*27,-18,0),FVector(24,5,22),TEXT("Copper"));
  for(int I=0;I<3;I++) {auto* Coil=AddMagnet(TEXT("SM_Ring"),FVector(Side*26,3+I*7,0),FVector(29,29,6),TEXT("Brass"));Coils.Add(Coil);}
  auto* Arm=AddMagnet(TEXT("Cube"),FVector(Side*48,20,0),FVector(28,14,14),TEXT("Copper"));ReachParts.Add(Arm);
 }
 RecoveredBlock=Shape(TEXT("SM_Plate"),FVector(650,-216,25),FVector(118,70,30),TEXT("Copper"));RecoveredBlock->SetVisibility(false);
 const TArray<FName> CueNames={TEXT("pickup_01"),TEXT("pickup_02"),TEXT("pickup_03"),TEXT("latch"),TEXT("tug"),TEXT("linked_release"),TEXT("deposit"),TEXT("forge"),TEXT("ui_click")};
 for(FName Name:CueNames){USoundBase* S=LoadObject<USoundBase>(nullptr,*(TEXT("/Game/MagnetSweep/Audio/")+Name.ToString()+TEXT(".")+Name.ToString()));if(S){Sounds.Add(Name,S);Owner->Resources.Add(S);}}
 UE_LOG(LogTemp,Display,TEXT("MAGNET_DEMO_BEGIN profile=%s sounds=%d layout=%d"),*Profile,Sounds.Num(),Model.GetLayoutIndex());
}
void FWorkbenchImpl::BuildPieces() {
 for(auto& Pair:Visuals){if(Pair.Value.Mesh)Pair.Value.Mesh->DestroyComponent();if(Pair.Value.Ring)Pair.Value.Ring->DestroyComponent();}
 Visuals.Empty();int Slot=0;
 for(const FSalvagePiece& P:Model.GetPieces()) {
  FPieceVisual V;const bool T=P.Kind==EPieceKind::Tangle;
  FName MeshName=T?TEXT("SM_Tangle"):(P.Id%3==0?TEXT("SM_Washer"):(P.Id%3==1?TEXT("SM_Bolt"):TEXT("SM_Plate")));
  V.Size=T?FVector(73,65,38):FVector(31+(P.Id%4)*3,28,8+(P.Id%3)*3);
  V.Position=World(P.Position,T?26:13);V.From=V.Position;V.Slot=Slot++;
  V.Mesh=Shape(MeshName,V.Position,V.Size,P.Id%4==0?TEXT("Copper"):TEXT("Steel"),FRotator(0,(P.Id*47)%360,0),false);
  if(T) V.Ring=Shape(TEXT("SM_Ring"),World(P.Position,58),FVector(49,49,8),TEXT("Ring"),FRotator::ZeroRotator,false);
  if(P.State==EPieceState::Banked){V.Mesh->SetVisibility(false);if(V.Ring)V.Ring->SetVisibility(false);}
  if(P.State==EPieceState::Cargo && V.Ring)V.Ring->SetVisibility(false);
  Visuals.Add(P.Id,V);
 }
 Preview={};AimedRing=INDEX_NONE;Action=EMagnetAction::None;
}
void FWorkbenchImpl::Play(FName Name,float Volume,float Pitch) {
 if(!bMuted) if(auto* S=Sounds.Find(Name)) { auto* A=UGameplayStatics::SpawnSound2D(Owner,*S,Volume,Pitch); if(A && Name!=TEXT("ui_click")) {PlayingSounds.Add(A);if(bPaused)A->SetPaused(true);} }
}
void FWorkbenchImpl::Toast(const FString& Title,const FString& Body,float Duration) {NoticeTitle=Title;NoticeBody=Body;NoticeTimer=Duration;}
FVector2D FWorkbenchImpl::Project(FVector V) const {FVector2D Result;PC->ProjectWorldLocationToScreen(V,Result);return Result;}
void FWorkbenchImpl::UpdatePointer(FVector2D EventPosition) {
 float X=EventPosition.X,Y=EventPosition.Y;if((X<0||Y<0)&&!PC->GetMousePosition(X,Y)){bWorldHit=bInTray=bFurnaceHover=false;HoverRing=INDEX_NONE;return;}Pointer=FVector2D(X,Y);
 FVector Origin,Direction;bWorldHit=PC->DeprojectScreenPositionToWorld(X,Y,Origin,Direction)&&FMath::Abs(Direction.Z)>.001;
 if(bWorldHit) {const FVector V=Origin+Direction*((10-Origin.Z)/Direction.Z);RawWorld=FVector2D(V.X,V.Y);}
 bInTray=bWorldHit&&FSalvageModel::IsInsideTray(RawWorld);
 bFurnaceHover=bWorldHit&&RawWorld.X>548&&RawWorld.X<758&&FMath::Abs(RawWorld.Y)<132;
 HoverRing=bInTray?Model.FindAvailableRing(RawWorld):INDEX_NONE;
 if(HitButton()!=INDEX_NONE){bInTray=false;bFurnaceHover=false;HoverRing=INDEX_NONE;}
 if(!bPaused && bWorldHit) {
  if(Action==EMagnetAction::Aim){Preview=Model.PreviewPull(AimedRing,RawWorld);Magnet=Preview.Endpoint;}
  else if(bInTray||bFurnaceHover) Magnet=RawWorld;
 }
}
int32 FWorkbenchImpl::HitButton() const {for(const auto& B:Buttons)if(B.Enabled&&B.Rect.IsInside(Pointer))return B.Id;return INDEX_NONE;}
void FWorkbenchImpl::HandlePress() {
 const int32 UI=HitButton();if(UI!=INDEX_NONE){Action=EMagnetAction::UI;Button(UI);return;}
 if(bPaused||PourTimer>0||ForgeTimer>0)return;
 if(bFurnaceHover&&Model.GetCargo()>0){Action=EMagnetAction::Dump;Deposit();return;}
 if(HoverRing!=INDEX_NONE){Action=EMagnetAction::Aim;AimedRing=HoverRing;Preview=Model.PreviewPull(AimedRing,RawWorld);Play(TEXT("latch"),.70f);LastEvent=TEXT("aim_started");return;}
 if(bInTray){Action=EMagnetAction::Sweep;LastSweepPoint=RawWorld;bLastSweepInTray=true;OnRecovery(Model.SweepAt(RawWorld),false);}
}
void FWorkbenchImpl::SweepPath(){
 if(!bWorldHit)return;
 // Sample the entire traveled segment, including an outward stroke. The model
 // ignores samples outside the tray; leaving the tray cannot skip its last scrap.
 const int Steps=FMath::Clamp(FMath::CeilToInt(FVector2D::Distance(LastSweepPoint,RawWorld)/32),1,128);
 FRecoveryResult Combined;
 for(int I=1;I<=Steps;I++){
  const auto R=Model.SweepAt(FMath::Lerp(LastSweepPoint,RawWorld,float(I)/Steps));
  Combined.PieceIds.Append(R.PieceIds);Combined.Amount+=R.Amount;
 }
 OnRecovery(Combined,false);LastSweepPoint=RawWorld;bLastSweepInTray=bInTray;
}
void FWorkbenchImpl::HandleRelease() {
 if(Action==EMagnetAction::Sweep&&!bPaused)SweepPath();
 if(Action==EMagnetAction::Aim&&!bPaused){Preview=Model.PreviewPull(AimedRing,RawWorld);int32 Tangled=0;for(int Id:Preview.PieceIds){auto* P=Model.FindPiece(Id);if(P&&P->Kind==EPieceKind::Tangle)Tangled++;}OnRecovery(Model.CommitPull(Preview),true,Tangled);}
 Action=EMagnetAction::None;AimedRing=INDEX_NONE;Preview={};
}
void FWorkbenchImpl::OnRecovery(const FRecoveryResult& R,bool bPull,int32 Tangled) {
 if(!R.Succeeded())return;
 RecoveryTimer=FMath::Max(RecoveryTimer,bPull?.5f:.28f);
 for(int Id:R.PieceIds)if(auto* V=Visuals.Find(Id)){V->From=V->Position;V->Progress=0;V->Duration=bPull?.48f:.25f;V->bPouring=false;if(V->Ring)V->Ring->SetVisibility(false);}
 if(bPull){Pulls++;LastBurst=R.Amount;Play(TEXT("tug"),.78f);if(Tangled>1)Play(TEXT("linked_release"),.82f);
 Toast(Tangled>1?FString::Printf(TEXT("%d tangles. One beautiful pull."),Tangled):TEXT("A satisfying little breakthrough."),FString::Printf(TEXT("+%d metal safely in your haul"),R.Amount),2.5f);LastEvent=TEXT("pull_committed");}
 else {Sweeps++;if(PickupCooldown<=0){Play(FName(*FString::Printf(TEXT("pickup_0%d"),1+Sweeps%3)),.55f,.97f+(Sweeps%4)*.035f);PickupCooldown=.065f;}LastEvent=TEXT("sweep_recovered");}
 SaveTimer=.25f;WriteTelemetry();
}
void FWorkbenchImpl::Deposit(bool Next) {
 if(RecoveryTimer>0){bPendingDeposit=true;bPendingNext|=Next;return;}
 bPendingDeposit=false;
 if(PourTimer>0||ForgeTimer>0){if(Next)bPendingNext=true;return;}
 if(Model.GetCargo()==0){if(Next)NextDelivery();return;}
 const auto R=Model.BankCargo();if(R.Amount<=0)return;
 Deposits++;bPendingNext=Next;PourTimer=1.02f;FurnacePulse=1;Play(TEXT("deposit"),.85f);
 for(int Id:R.PieceIds)if(auto* V=Visuals.Find(Id)){V->From=V->Position;V->Progress=0;V->Duration=.85f+.004f*(Id%20);V->bPouring=true;V->Mesh->SetVisibility(true);}
 LastEvent=TEXT("deposit_started");
 Toast(FString::Printf(TEXT("%d metal, all yours."),R.Amount),R.bCompletedNow?TEXT("Target reached. Your furnace is forging something good."):(Model.IsDeliveryCompleted()?TEXT("Extra metal recovered. The whole haul counts."):(Model.GetUpgradeLevel()>=2?TEXT("Every piece adds to your recovered metal block."):TEXT("Every piece brings your next improvement closer."))),3);
 Save();WriteTelemetry();
}
void FWorkbenchImpl::NextDelivery() {
 if(!Model.IsDeliveryCompleted())return;
 if(PourTimer>0||ForgeTimer>0){bPendingNext=true;return;}
 if(Model.GetCargo()>0){Deposit(true);return;}
 if(Model.AdvanceLayout()){BuildPieces();bPendingNext=false;LastEvent=TEXT("next_delivery");
 Toast(Model.GetUpgradeLevel()==2?TEXT("Full rig. Fresh possibilities."):TEXT("Fresh metal. Better magnet."),TEXT("Look for a useful direction through the linked rings."),4);Save();WriteTelemetry();}
}
void FWorkbenchImpl::Pause(bool Value) {
 for(auto& A:PlayingSounds)if(A.IsValid())A->SetPaused(Value);
 bPaused=Value;Action=EMagnetAction::None;AimedRing=INDEX_NONE;Preview={};bMouseWasDown=PC->IsInputKeyDown(EKeys::LeftMouseButton);
 bConfirmRetry=false;bConfirmNew=false;LastEvent=Value?TEXT("paused"):TEXT("resumed");Save();WriteTelemetry();
}
void FWorkbenchImpl::Button(int32 Id) {
 Play(TEXT("ui_click"),.65f);
 switch(Id){
 case 1:NextDelivery();break;
 case 2:Pause(true);bConfirmRetry=true;break;
 case 3:Pause(!bPaused);break;
 case 4:bMuted=!bMuted;Save();break;
 case 5:Pause(true);bConfirmNew=true;break;
 case 6:Save();UKismetSystemLibrary::QuitGame(Owner,PC,EQuitPreference::Quit,false);break;
 case 7:Pause(false);break;
 case 8:for(auto& A:PlayingSounds)if(A.IsValid())A->Stop();PlayingSounds.Empty();Model.ResetLayout();BuildPieces();RecoveryTimer=PourTimer=ForgeTimer=0;bPendingDeposit=bPendingNext=false;DisplayUpgrade=Model.GetUpgradeLevel();Pause(false);Toast(TEXT("Same tray. Another approach."),TEXT("Your magnet keeps its improvements."),3);break;
 case 9:for(auto& A:PlayingSounds)if(A.IsValid())A->Stop();PlayingSounds.Empty();Model=FSalvageModel();BuildPieces();DisplayUpgrade=0;RecoveryTimer=PourTimer=ForgeTimer=0;bPendingDeposit=bPendingNext=false;Pause(false);Toast(TEXT("A fresh start."),TEXT("Hold to sweep. Drag a ring to pull. Click the furnace to pour."),5);break;
 }
}
void FWorkbenchImpl::Tick(float Delta) {
 if(!PC)return;
 int32 W,H;PC->GetViewportSize(W,H);Width=FMath::Max(W,1);Height=FMath::Max(H,1);UIScale=FMath::Min(Width/1600.f,Height/900.f);UX=(Width-1600*UIScale)*.5f;UY=(Height-900*UIScale)*.5f;
 FrameAverage=FMath::Lerp(FrameAverage,Delta,.025f);
 bool Focused=true;if(GEngine&&GEngine->GameViewport&&GEngine->GameViewport->Viewport)Focused=GEngine->GameViewport->Viewport->IsForegroundWindow();
 if(!Focused&&bWasFocused)Pause(true);bWasFocused=Focused;
 UpdatePointer();
 if(Focused){
  if(PC->WasInputKeyJustPressed(EKeys::Escape))Pause(!bPaused);
  if(PC->WasInputKeyJustPressed(EKeys::M)){bMuted=!bMuted;Save();}
  if(PC->WasInputKeyJustPressed(EKeys::R)&&PourTimer<=0&&ForgeTimer<=0){Pause(true);bConfirmRetry=true;}
  if(PC->WasInputKeyJustPressed(EKeys::F11)){auto* S=GEngine->GetGameUserSettings();if(S->GetFullscreenMode()==EWindowMode::Windowed){WindowedSize=FIntPoint(W,H);S->SetFullscreenMode(EWindowMode::WindowedFullscreen);}else{S->SetFullscreenMode(EWindowMode::Windowed);S->SetScreenResolution(WindowedSize);}S->ApplySettings(false);}
  if(PC->WasInputKeyJustPressed(EKeys::F9))FScreenshotRequest::RequestScreenshot(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Screenshots"),TEXT("MagnetSweep.png")),true,true);

 }
 if(bPaused){if(bQA)WriteTelemetry();return;}
 Delta=FMath::Min(Delta,.1f);Time+=Delta;
 PlayingSounds.RemoveAll([](const auto& A){return !A.IsValid();});
 RecoveryTimer=FMath::Max(0.f,RecoveryTimer-Delta);
 if(bPendingDeposit && RecoveryTimer<=0)Deposit(bPendingNext);PickupCooldown-=Delta;NoticeTimer-=Delta;
 if(Action==EMagnetAction::Sweep)SweepPath();
 if(SaveTimer>0){SaveTimer-=Delta;if(SaveTimer<=0)Save();}
 if(PourTimer>0){PourTimer-=Delta;if(PourTimer<=0){
  LastEvent=TEXT("deposit_finished");
  if(DisplayUpgrade<Model.GetUpgradeLevel()){ForgeTimer=1.18f;Play(TEXT("forge"),.80f);DisplayUpgrade=Model.GetUpgradeLevel();
   Toast(DisplayUpgrade==1?TEXT("Breakaway coil forged."):TEXT("Long-reach arms forged."),DisplayUpgrade==1?TEXT("Aim through linked rings. One pull can free several tangles."):TEXT("Reach farther across the tray. Your rig is complete."),6);}
  else if(Model.IsDeliveryCompleted()){Toast(TEXT("A solid block of recovered metal."),TEXT("Keep gathering, try the same tray, or call in the next delivery."),5);}
 }}
 if(ForgeTimer>0){ForgeTimer-=Delta;if(ForgeTimer<=0)LastEvent=TEXT("forge_finished");}
 if(bPendingNext&&!bPendingDeposit&&RecoveryTimer<=0&&PourTimer<=0&&ForgeTimer<=0)NextDelivery();
 FurnacePulse=FMath::Max(0.f,FurnacePulse-Delta*.75f);
 UpdateVisuals(Delta);LastMagnet=Magnet;
 if(bQA)WriteTelemetry();
}
void FWorkbenchImpl::UpdateVisuals(float Delta) {
 const FVector MagnetPosition=World(Magnet,82+FMath::Sin(Time*2.5f)*1.2f);
 for(int I=0;I<MagnetShapes.Num();I++){
  auto* S=MagnetShapes[I];const bool Coil=Coils.Contains(S),Reach=ReachParts.Contains(S);
  S->SetVisibility((!Coil||DisplayUpgrade>=1)&&(!Reach||DisplayUpgrade>=2));
  Place(S,MagnetPosition+MagnetOffsets[I],MagnetSizes[I],Coil?FRotator(90,0,0):FRotator::ZeroRotator);
 }
 for(const auto& P:Model.GetPieces())if(auto* V=Visuals.Find(P.Id)) {
  if(P.State==EPieceState::Available){
   V->Position=World(P.Position,P.Kind==EPieceKind::Tangle?26:13);
   if(V->Ring){V->Ring->SetMaterial(0,Materials[(Preview.PieceIds.Contains(P.Id)||HoverRing==P.Id)?TEXT("Selected"):TEXT("Ring")]);Place(V->Ring,World(P.Position,58+FMath::Sin(Time*2+P.Id)*1.5f),FVector(49,49,8));}
   continue;
  }
  if(P.State==EPieceState::Banked&&!V->bPouring){V->Mesh->SetVisibility(false);continue;}
  V->Progress=FMath::Min(1.f,V->Progress+Delta/V->Duration);const float A=Ease(V->Progress);
  const float Angle=V->Slot*2.39996f;const float Radius=22+7*FMath::Sqrt(float(V->Slot+1));
  FVector Target=MagnetPosition+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,-26-(V->Slot%3)*6);
  if(V->bPouring)Target=World(Furnace,77);
  V->Position=FMath::Lerp(V->From,Target,A)+FVector(0,0,FMath::Sin(V->Progress*PI)*(V->bPouring?90:50));
  const float Shrink=V->bPouring?FMath::Max(.02f,1-V->Progress*.98f):FMath::Lerp(1.f,P.Kind==EPieceKind::Tangle?.48f:.72f,A);
  Place(V->Mesh,V->Position,V->Size*Shrink,FRotator(V->bPouring?V->Progress*130:0,P.Id*47+V->Progress*35,0));
  if(V->bPouring&&V->Progress>=1){V->bPouring=false;V->Mesh->SetVisibility(false);}
 }
 if(FurnaceFill)Place(FurnaceFill,World(Furnace,62),FVector(146,146,3+4*FMath::Clamp(float(Model.GetBanked())/Model.GetGoal(),0.f,1.f)));
 if(RecoveredBlock){const bool Show=Model.IsDeliveryCompleted()&&Model.GetCompletedDeliveryCount()>2&&PourTimer<=0;RecoveredBlock->SetVisibility(Show);if(Show)Place(RecoveredBlock,FVector(650,-216,25+FMath::Sin(Time*2)*.6f),FVector(118,70,30));}
 if(FurnaceLight)FurnaceLight->SetIntensity(1300+FurnacePulse*1700+100*FMath::Sin(Time*6));
}

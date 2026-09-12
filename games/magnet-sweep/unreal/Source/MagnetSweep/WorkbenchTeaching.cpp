#include "WorkbenchRuntime.h"
#include "MagnetGame.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Misc/Paths.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"

using namespace MagnetSweep;

bool FWorkbenchImpl::TutorialIntro() const
{
 return Tutorial.bEnabled && Tutorial.Step==ETutorialStep::Welcome;
}

void FWorkbenchImpl::TutorialChanged(bool Changed)
{
 if(!Changed)return;
 // Release is a real player action: keep the field running until they release it.
 if(Tutorial.Step!=ETutorialStep::Release || Tutorial.bRiskHeld){bFieldLatched=false;Action=EMagnetAction::None;}
 if(Tutorial.Step==ETutorialStep::Complete){Toast(TEXT("Tutorial complete — your rig is ready"),TEXT("Keep your earnings and upgrades. Chase gold, missing cores and the next contract."),7);Play(TEXT("contract_success"),.7f);}
 LastEvent=TEXT("tutorial_progress");Save();
}

void FWorkbenchImpl::UpdateTutorial()
{
 if(!Tutorial.bEnabled)return;
 bool Bundle=false;
 for(const auto& P:Model.GetPieces())if(P.State==EPieceState::Available&&Model.GetCaptureGroup(P.Id).Num()>1){Bundle=true;break;}
 bool Changed=Tutorial.HoldFirstRisk(Model.IsCargoUnsafe());
 if(PourTimer<=0&&Tutorial.Step==ETutorialStep::FirstSmelt&&Model.GetBanked()>0)Changed|=Tutorial.ObserveSmelt(Model.IsDeliveryCompleted());
 if(PourTimer<=0&&ForgeTimer<=0)Changed|=Tutorial.Reconcile(Model.IsDeliveryCompleted(),Bundle,Model.GetAvailableAmount()>0||Model.GetCargo()>0);
 TutorialChanged(Changed);
}

void FWorkbenchImpl::ResetTutorialScene()
{
 bPaused=bWorkshop=bReceipt=bConfirmRetry=bConfirmNew=false;
 bFieldLatched=bPrecisionLatched=bPrecision=false;
 bPendingDeposit=bPendingNext=bFailureShown=false;
 RecoveryTimer=PourTimer=ForgeTimer=NoticeTimer=0;LastBank={};
 Action=EMagnetAction::None;Magnet=MagnetTarget={0,-240};MagnetVelocity=FVector2D::ZeroVector;
 BuildPieces();DisplayUpgrade=Model.GetUpgradeLevel();
 bReceipt=bFailureShown=Model.IsJobEnded();
 if(Model.IsCargoUnsafe())bPaused=true;
 Tutorial.HoldFirstRisk(Model.IsCargoUnsafe());
}

void FWorkbenchImpl::EnterTutorialPractice()
{
 if(bTutorialPractice)return;
 if(!Save()){Toast(TEXT("Could not save your career"),TEXT("Practice has not started. Your current run is still here."),5);return;}
 SavePath=FPaths::Combine(FPaths::GetPath(FPaths::GetPath(CareerSavePath)),TEXT("Tutorial"),Profile+TEXT(".json"));
 bTutorialPractice=true;Model=FSalvageModel();Tutorial=FTutorialProgress();
 const bool Resumed=Load();
 if(!Resumed){Model=FSalvageModel();Tutorial.Start();}
 ResetTutorialScene();Save();
}

void FWorkbenchImpl::ReturnFromTutorial()
{
 if(!bTutorialPractice)return;
 if(!Save()){Toast(TEXT("Could not save practice"),TEXT("Your current practice remains open."),4);return;}
 const auto Practice=Model.GetSnapshot();const auto Teaching=Tutorial;const FString PracticePath=SavePath;
 SavePath=CareerSavePath;
 if(!Load()){
  FString Error;Model.RestoreSnapshot(Practice,Error);Tutorial=Teaching;SavePath=PracticePath;
  Toast(TEXT("Could not load the saved career"),TEXT("Neither run has been replaced. Practice remains open."),5);return;
 }
 bTutorialPractice=false;ResetTutorialScene();Save();
}

void FWorkbenchImpl::PaintTutorial(UCanvas* C)
{
 if(!C||bPaused||bConfirmRetry||bConfirmNew)return;
 const FLinearColor Paper(.90,.96,.93),Quiet(.62,.76,.76),Mint(.32,.96,.73),Gold(1,.72,.27),Dark(.008,.025,.029,.98);
 auto Button=[&](int32 Id,const FString& Label,float X,float Y,float W,float H,bool Primary=false){
  FDemoButton B;B.Id=Id;B.Label=Label;B.Enabled=true;B.Rect=FBox2D({UX+X*UIScale,UY+Y*UIScale},{UX+(X+W)*UIScale,UY+(Y+H)*UIScale});Buttons.Add(B);
  Rect(C,X,Y,W,H,Primary?FLinearColor(.08,.28,.21,1):FLinearColor(.04,.10,.12,1));Rect(C,X,Y,W,2,Primary?Mint:Quiet);
  DrawText(C,Label,X+12,Y+(H-18)/2,.77f,Paper);
 };
 auto Wrap=[&](const FString& Text,float X,float Y,float W,float Scale,FLinearColor Color){
  const FSlateFontInfo Font(RuntimeFont,FMath::Max(9.f,Scale*UIScale*16.f),TEXT("Regular"));
  const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
  TArray<FString> Words;Text.ParseIntoArray(Words,TEXT(" "),true);FString Row;
  for(const auto& Word:Words){const FString Next=Row.IsEmpty()?Word:Row+TEXT(" ")+Word;
   if(!Row.IsEmpty()&&Measure->Measure(Next,Font).X>W*UIScale){DrawText(C,Row,X,Y,Scale,Color);Y+=24;Row=Word;}else Row=Next;
  }
  if(!Row.IsEmpty()){DrawText(C,Row,X,Y,Scale,Color);Y+=24;}return Y;
 };
 if(!Tutorial.bEnabled){
  if(bTutorialPractice&&!bWorkshop&&!bReceipt){Rect(C,44,197,273,83,Dark);DrawText(C,TEXT("PRACTICE RUN"),58,207,.85f,Gold);Button(461,TEXT("Return to saved career"),57,239,246,32);}
  return;
 }
 if(TutorialIntro()){
  Buttons.Reset();Rect(C,0,0,1600,900,FLinearColor(.004,.014,.020,.84));
  Rect(C,365,218,870,460,Dark);Rect(C,365,218,870,3,Mint);
  DrawText(C,TEXT("LEARN THE WORKBENCH"),403,255,1.28f,Paper,true);
  float Y=Wrap(TEXT("Pull a valuable haul. Smelt it for credits. Build a better magnet."),405,313,785,1.04f,Paper);
  Y=Wrap(TEXT("We will guide your first order one action at a time. Your first dangerous pickup pauses so you can practise dropping the haul safely."),405,Y+20,785,.94f,Quiet);
  Wrap(bTutorialPractice?TEXT("This practice run has its own save. Return to your saved career at any time from Pause."):TEXT("Credits, XP and upgrades you earn here stay yours when the guidance ends."),405,Y+20,785,.94f,Mint);
  Button(450,TEXT("Start learning"),405,579,368,52,true);Button(451,TEXT("Play without guidance"),798,579,395,52);
  if(bTutorialPractice)Button(461,TEXT("Return to saved career"),405,639,788,30);
  return;
 }
 FString Title,Body,Task;int32 Target=INDEX_NONE;bool Furnace=false;
 const bool NeedsTray=Tutorial.NeedsFreshTray(Model.GetAvailableAmount()>0||Model.GetCargo()>0);
 const bool Failed=Model.IsJobFailed();
 const bool EndedAction=Model.IsJobEnded()&&Tutorial.Step!=ETutorialStep::Upgrade&&Tutorial.Step!=ETutorialStep::Complete;
 if(Tutorial.bRiskHeld){
  Title=TEXT("FIRST WARNING: PAUSED");Task=TEXT("Right-click to drop the whole haul.");
  Body=FString::Printf(TEXT("Red cells or excess weight start a %g-second fuse, even with attraction off. Expiry spends 1 fuel and destroys your most valuable unbanked piece, if any. Banked rewards stay safe. This first warning waits for you. Drop haul returns salvage to the tray."),Model.GetFuseDuration());
 }else if(Failed){
  Title=TEXT("TRY A RICHER LOAD");Task=TEXT("Retry this order to continue learning.");
  Body=TEXT("Four fuel charges ran out before the quota. Each smelt and each failed warning uses one charge. Banked credits, XP and upgrades stay yours. Copper and alloy pack more value into a load.");
 }else switch(Tutorial.Step){
  case ETutorialStep::Attract:Title=TEXT("1. ATTRACT SALVAGE");Task=TEXT("Collect at least 4 kg of iron.");Body=TEXT("Move beside the highlighted iron. Hold left mouse or press Space to switch the field on. Watch the pieces move into the magnet. Your order needs 120 credits of salvage.");break;
  case ETutorialStep::Release:Title=TEXT("2. KEEP YOUR HAUL");Task=(Action==EMagnetAction::Sweep||bFieldLatched)?TEXT("Release left mouse, or press Space off."):TEXT("Switch the field on, then off again.");Body=TEXT("Turning attraction off keeps your cargo attached. You can carry it safely while choosing your next pickup. Right-click is different: it drops the entire haul.");break;
  case ETutorialStep::Bundle:Title=TEXT("3. LINKED BUNDLES");Task=TEXT("Attract the linked copper rings.");Body=FString::Printf(TEXT("Connected pieces move together and count as one pickup: these three rings weigh 9 kg and pay 36 credits. Your safe load is %d kg. Check the incoming weight before pulling."),Model.GetCapacity());break;
  case ETutorialStep::FirstSmelt:Title=TEXT("4. YOUR FIRST PAYOUT");Task=Model.GetCargo()>0?TEXT("Click the furnace to smelt this haul."):TEXT("Gather valuable salvage for a useful load.");Body=FString::Printf(TEXT("Your haul is worth %d credits. One smelt spends one of four fuel charges, no matter how small the load. The metal pays spendable credits and XP. Reach 120 salvage value for an 80-credit bonus."),Model.GetCargo());Furnace=Model.GetCargo()>0;break;
  case ETutorialStep::Risk:Title=TEXT("5. PRACTISE A RESCUE");Task=TEXT("Pick up a highlighted red cell.");Body=TEXT("Red cells make even a light haul unstable. Try the red-cell pocket with a small haul. Your FIRST warning pauses before any loss. We will practise Drop haul; later warnings run in real time.");break;
  case ETutorialStep::Precision:Title=TEXT("6. PICK WITH PRECISION");Task=TEXT("Use Q or hold Shift and collect salvage.");Body=TEXT("A narrow field helps separate valuable salvage from red cells. Try the glowing core if it is still here: smelting it pays 40 credits and adds it to your collection. Rare cores are optional.");break;
  case ETutorialStep::Quota:Title=TEXT("7. COMPLETE YOUR ORDER");Task=FString::Printf(TEXT("Bank %d more credits of salvage."),FMath::Max(0,Model.GetGoal()-Model.GetBanked()));Body=TEXT("Choose useful loads of copper and alloy. Smelted value fills the order; the bonus adds extra credits and XP. Finish after the quota or use spare fuel to chase gold. The next danger warning has a real fuse.");Furnace=Model.GetCargo()>0&&Model.GetBanked()+Model.GetCargo()>=Model.GetGoal();break;
  case ETutorialStep::Upgrade:Title=TEXT("8. BUILD A BETTER RIG");Task=TEXT("Open Workshop and buy one upgrade.");Body=TEXT("Spend 150 credits on capacity, a stronger coil or more rescue time. XP unlocks ranks and further contracts; spending credits does not remove XP. You can fit a mod once your cargo is empty.");break;
  case ETutorialStep::TryUpgrade:Title=TEXT("9. FEEL THE UPGRADE");Task=TEXT("Try your upgraded magnet on another haul.");Body=FString::Printf(TEXT("Your rig now holds %d kg, reaches %g units and gives %g seconds to rescue an unsafe load. If this order is finished, choose another in Workshop. Your purchased improvement stays with you."),Model.GetCapacity(),Model.GetFieldRadius(),Model.GetFuseDuration());break;
  case ETutorialStep::Complete:Title=TEXT("YOU KNOW THE WORKBENCH");Task=TEXT("Ready for your next contract.");Body=TEXT("Choose a valuable load, watch its weight, drop danger in time and smelt for lasting rewards. Pursue gold marks, missing cores and new rig upgrades. Your progress stays with this run.");break;
  default:break;
 }
 if(NeedsTray&&!Failed&&Tutorial.Step!=ETutorialStep::Upgrade){Task=TEXT("Restock this tray or choose the next order.");Body+=TEXT(" There is no useful salvage left for this lesson. Retry keeps your banked earnings and learned controls.");}
 if(EndedAction&&!Failed){Title=TEXT("CONTINUE ON A FRESH TRAY");Task=TEXT("Choose another order in Workshop, or retry.");Body=TEXT("This order is finished, so its remaining pieces cannot be picked up. Your lesson continues on the next tray. Credits, upgrades and learned controls stay yours.");}
 if(PourTimer>0){Title=TEXT("SMELTING YOUR HAUL");Task=FString::Printf(TEXT("+%d credits and %d XP banked."),LastBank.CashAwarded,LastBank.XPAwarded);Body=TEXT("The furnace uses one fuel charge for the whole load. Watch the metal become an ingot; your next lesson starts when the pour finishes.");Furnace=false;}
 if(bWorkshop){
  Rect(C,146,800,1308,80,Dark);DrawText(C,Title,160,810,.88f,Mint);
  Wrap(Task,160,841,900,.81f,Paper);
  Button(451,Tutorial.bRiskHeld?TEXT("Drop haul & skip guidance"):TEXT("Skip guidance"),1115,820,317,37);
  return;
 }
 constexpr float X=43,W=275;
 Rect(C,X,199,W,530,Dark);Rect(C,X,199,W,3,Tutorial.bRiskHeld?Gold:Mint);
 DrawText(C,bTutorialPractice?TEXT("GUIDED PRACTICE"):TEXT("GUIDED FIRST ORDER"),X+16,215,.71f,Quiet);
 float Y=Wrap(Title,X+16,248,W-32,.98f,Tutorial.bRiskHeld?Gold:Mint);
 Y=Wrap(Task,X+16,Y+14,W-32,.94f,Paper);
 Wrap(Body,X+16,Y+22,W-32,.83f,Quiet);
 if(Tutorial.Step==ETutorialStep::Complete&&!Tutorial.bRiskHeld)Button(452,TEXT("Continue playing"),X+15,631,W-30,40,true);
 else if(PourTimer<=0&&(Failed||NeedsTray||EndedAction))Button(2,TEXT("Retry guided order"),X+15,631,W-30,40,true);
 Button(451,Tutorial.bRiskHeld?TEXT("Drop haul & skip guidance"):TEXT("Skip guidance"),X+15,683,W-30,32);

 // One marker points to an actual available target; no pretend magnetic captures.
 if(!Tutorial.bRiskHeld&&!Failed&&!EndedAction&&!bReceipt&&PourTimer<=0){
  for(const auto& P:Model.GetPieces()){
   if(P.State!=EPieceState::Available)continue;
   bool Match=false;
   if(Tutorial.Step==ETutorialStep::Attract)Match=P.Material==EMaterial::Iron;
   if(Tutorial.Step==ETutorialStep::Bundle)Match=Model.GetCaptureGroup(P.Id).Num()>1;
   if(Tutorial.Step==ETutorialStep::Risk)Match=P.Material==EMaterial::HotCell;
   if(Tutorial.Step==ETutorialStep::Precision)Match=P.Material!=EMaterial::HotCell;
   // The quota lesson asks the player to choose value per load; do not steer it toward the first cheap piece.
   if(Tutorial.Step==ETutorialStep::TryUpgrade||Tutorial.Step==ETutorialStep::FirstSmelt)Match=P.Amount>0&&Model.CanCaptureGroup(P.Id);
   if(Match&&(Target==INDEX_NONE||(Tutorial.Step==ETutorialStep::Precision&&P.Material==EMaterial::Core))){Target=P.Id;if(Tutorial.Step!=ETutorialStep::Precision||P.Material==EMaterial::Core)break;}
  }
  if(Furnace)WorldCircle(C,{650,0},112,Gold,3,96);
  else if(Target!=INDEX_NONE){
   const auto* P=Model.FindPiece(Target);const FVector2D S=Project(World(P->Position,24));
   WorldCircle(C,P->Position,34+3*FMath::Sin(Time*4),Gold,3,23);
   Line(C,{UX+318*UIScale,UY+280*UIScale},S,Gold*.72f,1.4f);
  }
 }
}

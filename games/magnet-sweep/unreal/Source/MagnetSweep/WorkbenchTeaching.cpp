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
 // First useful capture exposes smelting without cutting the sweep down to one cheap piece.
 if(Tutorial.Step!=ETutorialStep::FirstSmelt || Tutorial.bRiskHeld){bFieldLatched=false;Action=EMagnetAction::None;}
 if(Tutorial.Step==ETutorialStep::Complete){Toast(TEXT("Tutorial complete — your rig is ready"),TEXT("Keep your earnings and upgrades. Chase gold, missing cores and the next contract."),7);Play(TEXT("contract_success"),.7f);}
 LastEvent=TEXT("tutorial_progress");Save();
}

void FWorkbenchImpl::UpdateTutorial()
{
 if(!Tutorial.bEnabled)return;
 bool Bundle=false;
 for(const auto& P:Model.GetPieces())if(P.State==EPieceState::Available&&Model.GetCaptureGroup(P.Id).Num()>1){Bundle=true;break;}
 bool Changed=Tutorial.HoldFirstRisk(Model.IsCargoUnsafe());
 if(PourTimer<=0&&Model.GetBanked()>0)Changed|=Tutorial.ObserveSmelt(Model.IsDeliveryCompleted());
 if(PourTimer<=0&&ForgeTimer<=0)Changed|=Tutorial.Reconcile(Model.IsDeliveryCompleted(),Bundle,Model.GetAvailableAmount()>0||Model.GetCargo()>0,Model.GetCargo());
 TutorialChanged(Changed);
}

bool FWorkbenchImpl::CanCaptureForPlayer(int32 Id) const
{
 if(!Model.CanCaptureGroup(Id))return false;
 if(!Tutorial.IsTrainingGuardActive())return true;
 int32 IncomingMass=Model.GetCargoMass();
 for(int32 PieceId:Model.GetCaptureGroup(Id)){
  const auto* Piece=Model.FindPiece(PieceId);
  if(!Piece||Piece->Material==EMaterial::HotCell)return false;
  IncomingMass+=Piece->Mass;
 }
 return IncomingMass<=Model.GetCapacity();
}

void FWorkbenchImpl::ResetTutorialScene()
{
 bPaused=bWorkshop=bReceipt=bConfirmRetry=bConfirmNew=false;
 bFieldLatched=bPrecisionLatched=bPrecision=bHaulDrag=false;
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
   if(!Row.IsEmpty()&&Measure->Measure(Next,Font).X>W*UIScale){DrawText(C,Row,X,Y,Scale,Color);Y+=23;Row=Word;}else Row=Next;
  }
  if(!Row.IsEmpty()){DrawText(C,Row,X,Y,Scale,Color);Y+=23;}return Y;
 };
 if(TutorialIntro()){
  Buttons.Reset();Rect(C,0,0,1600,900,FLinearColor(.004,.014,.020,.84));
  Rect(C,330,173,940,554,Dark);Rect(C,330,173,940,3,Mint);
  DrawText(C,TEXT("BUILD YOUR FIRST MAGNET UPGRADE"),367,209,1.10f,Paper,true);
  float Y=Wrap(TEXT("Earn 150 credits, then fit a stronger coil, a larger basket or a safer stabilizer."),370,268,860,1.01f,Gold);
  Y=Wrap(TEXT("Hold left mouse near metal to attract it. Release keeps your haul. Click SMELT HAUL by the furnace to turn it into credits."),370,Y+17,860,.96f,Paper);
  Y=Wrap(TEXT("Iron, copper and alloy can be mixed freely. Every metal counts toward the order. Right-click drops the whole haul back onto the table; it does not smelt it."),370,Y+17,860,.94f,Paper);
  Y=Wrap(TEXT("Training guard: your magnet cannot overfill or pick up red cells until your first upgrade. Then we will practise risk with a paused first warning."),370,Y+17,860,.89f,Mint);
  Wrap(bTutorialPractice?TEXT("Separate practice save. Your main career stays safe."):TEXT("Keep your earnings. Finish orders, install all 9 rig improvements and recover 6 rare cores."),370,Y+14,860,.82f,Quiet);
  Button(450,TEXT("Start: collect and smelt"),370,626,421,51,true);Button(451,TEXT("Play without assistance"),812,626,417,51);
  if(bTutorialPractice)Button(461,TEXT("Return to saved career"),370,686,859,29);
  return;
 }
 const int32 Mods=Model.GetUpgradeLevel();
 auto DrawRigGoal=[&](){
  constexpr float X=43,W=275;
  Rect(C,X,199,W,530,Dark);Rect(C,X,199,W,3,Mint);
  DrawText(C,Mods==0?TEXT("FIRST BUILD: BETTER MAGNET"):TEXT("BUILD YOUR SALVAGE RIG"),X+16,213,.71f,Gold);
  if(Mods==0){
   DrawText(C,FString::Printf(TEXT("%d / 150 credits saved"),Model.GetWallet()),X+16,237,.91f,Paper);
   Rect(C,X+16,266,W-32,6,FLinearColor(.06,.12,.12));Rect(C,X+16,266,(W-32)*FMath::Clamp(Model.GetWallet()/150.f,0.f,1.f),6,Gold);
  }else{
   DrawText(C,FString::Printf(TEXT("%d / 9 improvements fitted"),Mods),X+16,237,.91f,Paper);
   for(int32 I=0;I<9;++I)Rect(C,X+16+I*27,266,22,6,I<Mods?Mint:FLinearColor(.06,.12,.12));
  }
  DrawText(C,Tutorial.IsTrainingGuardActive()?TEXT("TRAINING GUARD ON"):TEXT("SMELT  >  EARN  >  UPGRADE"),X+16,282,.67f,Tutorial.IsTrainingGuardActive()?Mint:Quiet);
 };
 if(!Tutorial.bEnabled){
  if(!bWorkshop&&!bReceipt){
   DrawRigGoal();float Y=Wrap(TEXT("Smelt mixed metal to earn credits. Buy permanent improvements in Workshop."),59,325,243,.93f,Paper);
   int32 Cleared=0;for(int32 I=0;I<6;++I)Cleared+=Model.IsJobCleared(I)?1:0;
   Y=Wrap(FString::Printf(TEXT("Orders completed: %d / 6. Rare cores recovered: %d / 6."),Cleared,Model.GetCollectedCores().Num()),59,Y+22,243,.88f,Mint);
   Wrap(TEXT("Choose valuable loads and chase gold targets with the rig you are building."),59,Y+20,243,.85f,Quiet);
   Button(20,TEXT("Workshop: improve rig"),58,621,245,40,true);
   if(bTutorialPractice)Button(461,TEXT("Return to saved career"),58,683,245,32);
  }
  return;
 }
 FString Title,Body,Task;int32 Target=INDEX_NONE;bool Furnace=Model.GetCargo()>0&&!Model.IsCargoUnsafe();
 const bool NeedsTray=Tutorial.NeedsFreshTray(Model.GetAvailableAmount()>0||Model.GetCargo()>0);
 const bool Failed=Model.IsJobFailed();
 const bool EndedAction=Model.IsJobEnded()&&Tutorial.Step!=ETutorialStep::Upgrade;
 if(Tutorial.bRiskHeld){
  Title=TEXT("DANGER: PAUSED FOR YOU");Task=TEXT("Right-click to return the entire haul to the table.");
  Body=TEXT("This is a rescue, not a deposit. A normal expired warning costs 1 fuel and your most valuable unbanked piece. Your banked credits and rig stay safe. This first warning waits.");
 }else if(Failed){
  Title=TEXT("YOUR CREDITS ARE SAFE");Task=FString::Printf(TEXT("Retry the order. Keep your %d credits."),Model.GetWallet());
  Body=TEXT("The four pours ran out. Copper and alloy give more credits per load. You keep your earned money and upgrades when you retry.");
 }else switch(Tutorial.Step){
  case ETutorialStep::Attract:
   Title=TEXT("COLLECT A MIXED LOAD");Task=TEXT("Hold left mouse beside metal, then release.");
   Body=TEXT("Iron, copper and alloy all count. Mixing them never fails the order. Try the copper rings for a useful load. Click SMELT HAUL when ready; RMB only drops it.");break;
  case ETutorialStep::Release:case ETutorialStep::Bundle:case ETutorialStep::FirstSmelt:
   Title=TEXT("TURN METAL INTO CREDITS");Task=Model.GetCargo()>0?FString::Printf(TEXT("Click SMELT HAUL by the furnace: +%d credits."),Model.GetCargo()):TEXT("Collect any metal, then click SMELT HAUL.");
   Body=TEXT("You can also click the glowing furnace or press E. Release left mouse to keep your load. Right-click does NOT deposit. Each pour uses 1 fuel, so useful mixed loads pay better.");break;
  case ETutorialStep::Quota:
   Title=TEXT("FUND YOUR FIRST UPGRADE");Task=FString::Printf(TEXT("Smelt %d more credits of any metal."),FMath::Max(0,Model.GetGoal()-Model.GetBanked()));
   Body=TEXT("Reach 120 order value for an extra 80 credits: enough for a 150-credit rig upgrade. Copper and alloy pay more than iron. Mix them freely. Only smelting pays your wallet.");break;
  case ETutorialStep::Upgrade:
   Title=TEXT("BUILD THE IMPROVEMENT");Task=Model.GetCargo()>0?TEXT("Smelt your carried metal, then open Workshop."):TEXT("Open Workshop and buy a 150-credit improvement.");
   Body=TEXT("A larger basket carries more per pour; a stronger coil pulls farther; a stabilizer gives more rescue time. These are permanent changes to this magnet.");break;
  case ETutorialStep::TryUpgrade:
   Title=TEXT("USE YOUR BETTER MAGNET");Task=TEXT("Return to the tray and collect another useful load.");
   Body=Tutorial.bRiskLearned?TEXT("Your upgrade stays yours. Training guard is now OFF: avoid red cells and overfilling, or drop the haul before the warning expires."):TEXT("Your upgrade stays yours. Training guard is now OFF. Your first dangerous pickup will pause so we can practise rescuing it.");break;
  case ETutorialStep::Risk:
   Title=TEXT("NOW PRACTISE A RESCUE");Task=Model.GetCargo()>0?TEXT("Smelt this safe haul, then try the marked red cell."):TEXT("Pick up the marked red cell with an empty magnet.");
   Body=TEXT("Your first upgrade is built. Now learn the risk: red cells or too much weight start a fuse. This first warning pauses before any loss. Later warnings run in real time.");break;
  case ETutorialStep::Precision:
   Title=TEXT("LEAVE DANGER BEHIND");Task=TEXT("Press Q or hold Shift, then collect useful metal.");
   Body=TEXT("Precision narrows the field so you can leave red cells behind. Try the glowing core: smelt it for 40 credits and a permanent collectible. Mixing ordinary metals is always fine.");break;
  default:break;
 }
 if(NeedsTray&&!Failed&&Tutorial.Step!=ETutorialStep::Upgrade){Task=TEXT("Restock this tray or choose the next order.");Body=TEXT("This lesson needs useful metal. Retry keeps your banked credits, installed upgrades and learned controls.");}
 if(EndedAction&&!Failed){Title=TEXT("CONTINUE ON A FRESH TRAY");Task=TEXT("Choose another order in Workshop, or retry.");Body=TEXT("Your banked earnings and improved rig stay yours. The lesson continues on the next tray.");}
 if(PourTimer>0){Title=TEXT("CREDITS PAID TO WALLET");Task=FString::Printf(TEXT("+%d credits and %d XP earned."),LastBank.CashAwarded,LastBank.XPAwarded);Body=Mods==0?FString::Printf(TEXT("%d / 150 credits toward your first rig improvement. One fuel charge paid for the whole mixed load."),Model.GetWallet()):TEXT("The whole mixed haul paid your wallet. Use those credits to keep improving your rig.");Furnace=false;}
 if(bWorkshop){
  Rect(C,146,800,1308,80,Dark);DrawText(C,Title,160,810,.88f,Mint);Wrap(Task,160,841,900,.81f,Paper);
  Button(451,Tutorial.bRiskHeld?TEXT("Drop haul & skip guide"):TEXT("Skip guide & guard"),1115,820,317,37);return;
 }
 DrawRigGoal();float Y=Wrap(Title,59,311,243,.98f,Tutorial.bRiskHeld?Gold:Mint);
 Y=Wrap(Task,59,Y+12,243,.94f,Paper);Wrap(Body,59,Y+16,243,.83f,Quiet);
 if(PourTimer<=0&&(Failed||NeedsTray||EndedAction))Button(2,TEXT("Retry order; keep credits"),58,631,245,40,true);
 Button(451,Tutorial.bRiskHeld?TEXT("Drop haul & skip guide"):TEXT("Skip guide & guard"),58,683,245,32);
 if(!Tutorial.bRiskHeld&&!Failed&&!EndedAction&&!bReceipt&&PourTimer<=0){
  for(const auto& P:Model.GetPieces()){
   if(P.State!=EPieceState::Available)continue;
   bool Match=false;
   if(Tutorial.Step==ETutorialStep::Attract)Match=P.Amount>0&&CanCaptureForPlayer(P.Id);
   if(Tutorial.Step==ETutorialStep::Risk&&!Furnace)Match=P.Material==EMaterial::HotCell;
   if(Tutorial.Step==ETutorialStep::Precision)Match=P.Material!=EMaterial::HotCell&&CanCaptureForPlayer(P.Id);
   if(Tutorial.Step==ETutorialStep::TryUpgrade||(Tutorial.Step==ETutorialStep::FirstSmelt&&!Furnace))Match=P.Amount>0&&CanCaptureForPlayer(P.Id);
   if(Match){
    const bool Preferred=(Tutorial.Step==ETutorialStep::Attract&&P.Material==EMaterial::Copper&&Model.GetCaptureGroup(P.Id).Num()>1)||(Tutorial.Step==ETutorialStep::Precision&&P.Material==EMaterial::Core);
    if(Target==INDEX_NONE||Preferred)Target=P.Id;
    if(Preferred)break;
   }
  }
  if(Furnace)WorldCircle(C,{650,0},112,Gold,3,96);
  else if(Target!=INDEX_NONE){const auto* P=Model.FindPiece(Target);const FVector2D S=Project(World(P->Position,24));WorldCircle(C,P->Position,34+3*FMath::Sin(Time*4),Gold,3,23);Line(C,{UX+318*UIScale,UY+327*UIScale},S,Gold*.72f,1.4f);}
 }
}

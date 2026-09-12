#include "WorkbenchRuntime.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GlobalRenderResources.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"

using namespace MagnetSweep;

namespace
{
const FLinearColor Ink(.018f,.035f,.044f,1.f);
const FLinearColor Panel(.009f,.024f,.030f,.93f);
const FLinearColor Edge(.12f,.25f,.27f,1.f);
const FLinearColor Paper(.89f,.94f,.90f,1.f);
const FLinearColor Quiet(.53f,.67f,.68f,1.f);
const FLinearColor Mint(.34f,.94f,.72f,1.f);
const FLinearColor Copper(1.f,.57f,.26f,1.f);
}

void FWorkbenchImpl::DrawText(UCanvas* C,const FString& Text,float X,float Y,float Scale,FLinearColor Color,bool Large) const
{
 if(!C || !GEngine) return;
 FCanvasTextItem Item(FVector2D(UX+X*UIScale,UY+Y*UIScale),FText::FromString(Text),
  FSlateFontInfo(RuntimeFont,FMath::Max(9.f,Scale*UIScale*(Large?24.f:16.f)),Large?TEXT("Bold"):TEXT("Regular")),Color);
 Item.Scale=FVector2D(1);
 Item.EnableShadow(FLinearColor(0,0,0,.5f),FVector2D(0,1));
 C->DrawItem(Item);
}

void FWorkbenchImpl::Rect(UCanvas* C,float X,float Y,float W,float H,FLinearColor Color) const
{
 if(!C || W<=0 || H<=0) return;
 FCanvasTileItem Item(FVector2D(UX+X*UIScale,UY+Y*UIScale),FVector2D(W*UIScale,H*UIScale),Color);
 Item.BlendMode=SE_BLEND_Translucent;
 C->DrawItem(Item);
}

void FWorkbenchImpl::Line(UCanvas* C,FVector2D A,FVector2D B,FLinearColor Color,float Thickness) const
{
 if(!C || !GEngine) return;
 FCanvasLineItem Item(A,B);
 Item.SetColor(Color);
 Item.LineThickness=FMath::Max(.75f,Thickness*UIScale);
 C->DrawItem(Item);
}

void FWorkbenchImpl::WorldCircle(UCanvas* C,FVector2D Center,float Radius,FLinearColor Color,float Thickness,float Z) const
{
 constexpr int32 Segments=48;
 FVector2D Last=Project(World(Center+FVector2D(Radius,0),Z));
 for(int32 I=1;I<=Segments;++I)
 {
  const float Angle=2.f*PI*I/Segments;
  const FVector2D Next=Project(World(Center+FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*Radius,Z));
  Line(C,Last,Next,Color,Thickness);
  Last=Next;
 }
}

void FWorkbenchImpl::Paint(UCanvas* C)
{
 if(!C || !GEngine) return;
 Width=C->ClipX; Height=C->ClipY;
 UIScale=FMath::Max(.01f,FMath::Min(Width/1600.f,Height/900.f));
 UX=(Width-1600.f*UIScale)*.5f; UY=(Height-900.f*UIScale)*.5f;
 Buttons.Reset();
 const FLinearColor Danger(1.f,.24f,.18f,1),Gold(1.f,.80f,.32f,1);
 auto Measure=[&](const FString& Text,float Scale,bool Large=false)->FVector2D
 {
  return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text,
   FSlateFontInfo(RuntimeFont,FMath::Max(9.f,Scale*UIScale*(Large?24.f:16.f)),Large?TEXT("Bold"):TEXT("Regular")))/UIScale;
 };
 auto FitText=[&](const FString& Text,float X,float Y,float W,float Scale,FLinearColor Color,bool Large=false)
 {
  const float M=Measure(Text,Scale,Large).X;
  DrawText(C,Text,X,Y,M>W?Scale*W/M:Scale,Color,Large);
 };
 auto PanelBox=[&](float X,float Y,float W,float H,FLinearColor Accent)
 {
  Rect(C,X+4,Y+6,W,H,FLinearColor(0,0,0,.22f));
  Rect(C,X,Y,W,H,Panel);Rect(C,X,Y,W,2,Accent);
 };
 auto Meter=[&](float X,float Y,float W,float Fraction,FLinearColor Color,float H=7.f)
 {
  Rect(C,X,Y,W,H,Ink);Rect(C,X,Y,W*FMath::Clamp(Fraction,0.f,1.f),H,Color);
 };
 auto AddButton=[&](int32 Id,const FString& Label,float X,float Y,float W,float H,bool Enabled=true,bool Primary=false)
 {
  FDemoButton B;B.Id=Id;B.Label=Label;B.Enabled=Enabled;
  B.Rect=FBox2D(FVector2D(UX+X*UIScale,UY+Y*UIScale),FVector2D(UX+(X+W)*UIScale,UY+(Y+H)*UIScale));
  Buttons.Add(B);
  const bool Hover=Enabled&&B.Rect.IsInside(Pointer);
  const FLinearColor Base=Primary?FLinearColor(.10f,.34f,.27f,1):FLinearColor(.05f,.11f,.13f,1);
  Rect(C,X,Y,W,H,Enabled?(Hover?FLinearColor(Base.R+.045f,Base.G+.065f,Base.B+.055f,1):Base):FLinearColor(.035f,.060f,.070f,1));
  Rect(C,X,Y,W,1,Enabled?(Primary?Mint:(Hover?Paper:Edge)):Edge*.6f);
  const float Scale=FMath::Min(.93f,(W-22.f)/FMath::Max(1.f,Measure(Label,1.f).X));
  const FVector2D Size=Measure(Label,Scale);
  DrawText(C,Label,X+(W-Size.X)*.5f,Y+(H-Size.Y)*.5f-1,Scale,Enabled?Paper:Quiet*.72f);
 };
 auto Veil=[&]()
 {
  Buttons.Reset();
  FCanvasTileItem Item(FVector2D::ZeroVector,FVector2D(Width,Height),FLinearColor(.003f,.012f,.018f,.91f));
  Item.BlendMode=SE_BLEND_Translucent;C->DrawItem(Item);
 };
 const FJobDefinition& Job=Model.GetJob();
 const bool Unsafe=Model.IsCargoUnsafe();
 const bool FieldOn=bFieldLatched||Action==EMagnetAction::Sweep;
 const bool Modal=bPaused||bWorkshop||bReceipt||bConfirmRetry||bConfirmNew;
 const bool Ready=PourTimer<=0&&ForgeTimer<=0&&!bPendingDeposit&&!bPendingNext;
 const int32 Rank=Model.GetPlayerLevel();
 const int32 RankStart=FSalvageModel::XPForLevel(Rank);
 const int32 RankEnd=FSalvageModel::XPForLevel(Rank+1);
 const float RankFraction=RankEnd>RankStart?float(Model.GetXP()-RankStart)/(RankEnd-RankStart):1.f;
 const bool Complete=Model.IsDeliveryCompleted(),Failed=Model.IsJobFailed();
 const bool GuideBuy=Tutorial.bEnabled&&Tutorial.Step==ETutorialStep::Upgrade;

 PaintEffects(C);

 // Goal, opportunity budget and permanent progress stay visible together.
 PanelBox(28,22,1544,118,Edge);Rect(C,28,22,4,118,Copper);
 DrawText(C,TEXT("MAGNET SWEEP"),50,42,1.07f,Paper,true);
 DrawText(C,FString::Printf(TEXT("YOUR RIG  /  %d OF 9 UPGRADES"),Model.GetUpgradeLevel()),52,91,.71f,Quiet);
 Rect(C,319,43,1,75,Edge);
 FitText(FString::Printf(TEXT("ORDER %02d  /  %s"),Model.GetLayoutIndex()+1,*Job.Name.ToUpper()),342,39,417,.76f,Quiet);
 const int32 Target=Complete&&!GuideBuy?Job.GoldGoal:Job.Quota;
 const FString GoalText=GuideBuy?TEXT("Order paid — upgrade ready"):(Failed?TEXT("Order missed"):(Model.IsGoldAwarded()?TEXT("Gold order complete"):
  FString::Printf(TEXT("%d / %d cr %s"),Model.GetBanked(),Target,Complete?TEXT("for gold"):TEXT("banked"))));
 DrawText(C,GoalText,342,61,1.13f,Failed?Danger:(Complete?Mint:Paper));
 constexpr float GoalWidth=417;
 Meter(342,98,GoalWidth,float(Model.GetBanked()+Model.GetCargo())/FMath::Max(1,Target),FLinearColor(.22f,.40f,.35f,1),9);
 Rect(C,342,98,GoalWidth*FMath::Clamp(float(Model.GetBanked())/FMath::Max(1,Target),0.f,1.f),9,Model.IsGoldAwarded()?Gold:Mint);
 if(Model.GetCargo()>0&&!Unsafe)DrawText(C,FString::Printf(TEXT("Next pour +%d cr"),Model.GetCargo()),342,115,.65f,Mint);
 else DrawText(C,GuideBuy?TEXT("Open Workshop to fit your first improvement"):(Complete?FString::Printf(TEXT("Gold bonus +%d cr"),Job.GoldBonus):FString::Printf(TEXT("Order bonus +%d cr"),Job.CompletionBonus)),342,115,.65f,Quiet);
 DrawText(C,TEXT("FURNACE FUEL"),799,39,.72f,Quiet);
 for(int32 I=0;I<FSalvageModel::HeatsPerJob;++I)
 {
  const bool Full=I<Model.GetHeatsRemaining();
  Rect(C,799+I*34.f,68,26,29,Full?FLinearColor(.58f,.25f,.07f,1):Ink);
  Rect(C,803+I*34.f,72,18,17,Full?Copper:Edge*.7f);
  Rect(C,807+I*34.f,66,10,3,Full?Gold:Edge);
 }
 DrawText(C,FString::Printf(TEXT("%d fuel charges"),Model.GetHeatsRemaining()),799,110,.69f,Model.GetHeatsRemaining()==1?Copper:Paper);
 Rect(C,962,43,1,75,Edge);
 if(PourTimer>0)Rect(C,973,34,174,94,FLinearColor(.36f,.22f,.04f,.35f+.15f*FMath::Sin(Time*10)));
 DrawText(C,TEXT("BANKED CREDITS"),986,39,.72f,Quiet);
 DrawText(C,FString::Printf(TEXT("%d cr"),Model.GetWallet()),986,64,1.05f,Gold,true);
 DrawText(C,FString::Printf(TEXT("RANK %d"),Rank),1164,39,.76f,Mint);
 DrawText(C,FString::Printf(TEXT("%d XP"),Model.GetXP()),1164,65,.95f,Paper);
 Meter(1164,100,190,RankFraction,Mint,5);
 DrawText(C,RankEnd>RankStart?FString::Printf(TEXT("%d to next rank"),FMath::Max(0,RankEnd-Model.GetXP())):TEXT("Veteran rig"),1164,113,.64f,Quiet);
 AddButton(20,TEXT("Workshop"),1400,41,148,38,Ready&&!TutorialIntro()&&!Tutorial.bRiskHeld,GuideBuy);
 AddButton(3,TEXT("Pause"),1400,91,148,29);

 if(!Modal)
 {
  DrawText(C,TEXT("IRON  2kg / 4cr"),47,157,.69f,Quiet);
  DrawText(C,TEXT("COPPER  3kg / 12cr"),224,157,.69f,Copper);
  DrawText(C,TEXT("ALLOY  4kg / 24cr"),437,157,.69f,Paper);
  DrawText(C,Tutorial.IsTrainingGuardActive()?TEXT("RED CELLS BLOCKED IN TRAINING"):TEXT("RED CELLS = DANGER"),643,157,.69f,Tutorial.IsTrainingGuardActive()?Mint:Danger);
  const FVector2D Screen=Project(World(Magnet,105));
  const float ChipWidth=Unsafe?284.f:214.f,ChipHeight=Unsafe?85.f:62.f;
  const float MX=FMath::Clamp(float((Screen.X-UX)/UIScale)+42.f,46.f,1546.f-ChipWidth);
  const float MY=FMath::Clamp(float((Screen.Y-UY)/UIScale)-25.f,238.f,694.f);
  if(Model.GetCargoMass()>0||FieldOn)
  {
   Rect(C,MX,MY,ChipWidth,ChipHeight,Panel);Rect(C,MX,MY,3,ChipHeight,Unsafe?Danger:Mint);
   DrawText(C,FString::Printf(TEXT("%d / %d kg"),Model.GetCargoMass(),Model.GetCapacity()),MX+13,MY+8,1.f,Unsafe?Danger:Paper);
   DrawText(C,Unsafe?(Tutorial.bRiskHeld?TEXT("Training: fuse paused"):FString::Printf(TEXT("%.1fs - lose 1 fuel"),Model.GetFuseRemaining())):FString::Printf(TEXT("Haul worth %d cr"),Model.GetCargo()),MX+13,MY+35,.79f,Unsafe?Danger:Mint);
   if(Unsafe)
   {
    const FSalvagePiece* Risk=Model.GetAtRiskPiece();
    FitText(Risk?FString::Printf(TEXT("+ %s (%d cr)"),*Model.PieceName(Risk->Id),Risk->Amount):TEXT("RMB: Drop haul"),MX+13,MY+58,ChipWidth-26,.75f,Risk?Danger:Quiet);
   }
   Meter(MX+13,MY+ChipHeight-4,ChipWidth-26,Unsafe?Model.GetFuseRemaining()/Model.GetFuseDuration():float(Model.GetCargoMass())/Model.GetCapacity(),Unsafe?Danger:Mint,3);
  }
  const FSalvagePiece* Nearest=nullptr;double Closest=48.0*48.0;
  if(bInTray&&!FieldOn)
   for(const FSalvagePiece& P:Model.GetPieces())
    if(P.State==EPieceState::Available&&(P.Position-RawWorld).SizeSquared()<Closest)
    {Nearest=&P;Closest=(P.Position-RawWorld).SizeSquared();}
  if(Nearest)
  {
   const TArray<int32> Group=Model.GetCaptureGroup(Nearest->Id);
   int32 Mass=0,Value=0;bool Cell=false;
   for(int32 Id:Group)if(const FSalvagePiece* P=Model.FindPiece(Id)){Mass+=P->Mass;Value+=P->Amount;Cell|=P->Material==EMaterial::HotCell;}
   const bool Fits=CanCaptureForPlayer(Nearest->Id),WouldOverload=Model.GetCargoMass()+Mass>Model.GetCapacity()||Cell;
   const FLinearColor Color=WouldOverload?(Tutorial.IsTrainingGuardActive()?Gold:Danger):(Nearest->Material==EMaterial::Core?Gold:Copper);
   const float HX=FMath::Clamp(float((Pointer.X-UX)/UIScale)+25.f,42.f,1255.f);
   const float HY=FMath::Clamp(float((Pointer.Y-UY)/UIScale)+26.f,199.f,669.f);
   Rect(C,HX,HY,300,99,Panel);Rect(C,HX,HY,3,99,Color);
   FitText(Group.Num()>1?FString::Printf(TEXT("LINKED BUNDLE  /  %d pieces"),Group.Num()):Model.PieceName(Nearest->Id),HX+13,HY+9,276,.89f,Paper);
   DrawText(C,FString::Printf(TEXT("%d kg   /   %d cr"),Mass,Value),HX+13,HY+37,1.02f,Color);
   FitText(!Fits?(Tutorial.IsTrainingGuardActive()?(Cell?TEXT("Training guard: red cell stays on tray"):TEXT("Training guard: smelt your current load")):TEXT("Too heavy for this rig")):(Cell?TEXT("Discharge costs 1 fuel. RMB drops haul."):
    FString::Printf(TEXT("Incoming %d / %d kg%s"),Model.GetCargoMass()+Mass,Model.GetCapacity(),WouldOverload?TEXT(" - unstable"):TEXT(""))),HX+13,HY+69,275,.77f,Color);
  }
  FString SmeltReason;const bool CanSmelt=Model.CanSmelt(SmeltReason);
  FString Status=TEXT("Uses 1 fuel. Pays your banked credits.");
  if(PourTimer>0)Status=TEXT("Paying credits — pouring metal...");
  else if(Model.IsJobEnded()||Failed)Status=TEXT("Order ended. Open Workshop for another.");
  else if(Model.GetCargoMass()==0)Status=TEXT("Collect any metal first. Mixing is fine.");
  else if(Model.HasHotCell())Status=TEXT("Red cell onboard. RMB drops the haul.");
  else if(Unsafe)Status=TEXT("Over capacity. RMB drops the haul.");
  Rect(C,1262,548,286,178,Panel);
  DrawText(C,TEXT("FURNACE: ALL METALS ACCEPTED"),1276,561,.72f,Copper);
  FitText(Status,1276,585,258,.73f,Unsafe?Danger:Paper);
  AddButton(40,CanSmelt?FString::Printf(TEXT("SMELT HAUL  +%d cr"),Model.GetCargo()):TEXT("SMELT HAUL"),1274,614,262,46,Ready&&CanSmelt&&!Tutorial.bRiskHeld,true);
  DrawText(C,TEXT("Click furnace or press E to smelt."),1276,673,.70f,Quiet);
  DrawText(C,TEXT("RMB puts everything back on the tray."),1276,698,.66f,Quiet);
 }

 if(NoticeTimer>0&&!NoticeTitle.IsEmpty()&&!Modal&&!Unsafe)
 {
  const float Fade=FMath::Min(1.f,NoticeTimer*2.f);
  Rect(C,460,186,680,73,FLinearColor(.023f,.063f,.067f,.97f*Fade));Rect(C,460,186,3,73,FLinearColor(.34f,.94f,.72f,Fade));
  FitText(NoticeTitle,480,197,642,1.03f,FLinearColor(.89f,.94f,.90f,Fade));
  FitText(NoticeBody,480,230,642,.79f,FLinearColor(.66f,.78f,.74f,Fade));
 }
 PanelBox(28,786,1544,92,Unsafe?Danger:Edge);
 FString Hint,Detail;
 if(Unsafe)
 {
  const FSalvagePiece* Risk=Model.GetAtRiskPiece();
  Hint=Tutorial.bRiskHeld?TEXT("FIRST WARNING PAUSED  -  RMB: Drop haul"):FString::Printf(TEXT("UNSTABLE  %.1fs  -  RMB: Drop haul"),Model.GetFuseRemaining());
  Detail=Risk?FString::Printf(TEXT("Expiry: lose 1 fuel + %s (%d cr). Drop releases the whole haul; all dropped pieces stay recoverable."),*Model.PieceName(Risk->Id),Risk->Amount):TEXT("Expiry: lose 1 fuel. Drop releases the whole haul recoverably and switches the field off.");
 }
 else if(PourTimer>0||ForgeTimer>0){Hint=TEXT("Smelting your haul");Detail=TEXT("Metal becomes credits, rank progress and recovered loot.");}
 else if(Failed){Hint=TEXT("Order missed. Your banked earnings are safe.");Detail=TEXT("Try a richer load or improve your rig before the next contract.");}
 else if(GuideBuy){Hint=TEXT("Your first upgrade is funded — open Workshop");Detail=TEXT("Fit a permanent 150-credit improvement. Optional gold targets can wait until your rig is better.");}
 else if(Complete){Hint=Model.IsGoldAwarded()?TEXT("Gold secured. Finish this order when ready."):TEXT("Order secured. Bank more for gold, or finish safely.");Detail=TEXT("LMB hold / Space toggle field  |  Shift hold / Q toggle precision  |  RMB drops whole haul");}
 else if(Model.GetCargo()>0){Hint=FString::Printf(TEXT("%d cr carried — click SMELT HAUL to get paid"),Model.GetCargo());Detail=TEXT("Any metal mix is accepted. Release LMB keeps your load. RMB returns the entire haul to the tray.");}
 else {Hint=TEXT("Collect metal > smelt for credits > build your better magnet");Detail=Tutorial.IsTrainingGuardActive()?FString::Printf(TEXT("TRAINING GUARD: %d kg maximum; red cells stay on the tray until your first upgrade."),Model.GetCapacity()):TEXT("LMB / Space attracts; release keeps cargo. E / furnace smelts. RMB drops everything back on the tray.");}
 FitText(Hint,49,801,1080,1.06f,Unsafe?Danger:Paper);
 FitText(Detail,49,839,1080,.79f,Unsafe?FLinearColor(1,.66f,.54f,1):Quiet);
 AddButton(20,TEXT("Workshop"),1372,809,172,43,Ready&&!Tutorial.bRiskHeld,GuideBuy);
 if(Complete&&!Model.IsJobEnded()&&!GuideBuy)AddButton(1,TEXT("Finish order"),1180,809,176,43,Ready&&Model.GetCargoMass()==0,true);
 else if(Model.IsJobEnded()||Failed)AddButton(30,TEXT("Choose next order"),1180,809,176,43,Ready,true);
 else DrawText(C,FString::Printf(TEXT("%dkg RIG"),Model.GetCapacity()),1209,824,.88f,Quiet);

 if(bConfirmRetry||bConfirmNew)
 {
  Veil();PanelBox(452,227,696,434,Copper);
  DrawText(C,bConfirmNew?(bTutorialPractice?TEXT("Restart this practice run?"):TEXT("Start a new career?")):TEXT("Retry this contract?"),490,263,1.36f,Paper,true);
  FitText(bConfirmNew?(bTutorialPractice?TEXT("This replaces only practice progress. Your saved career stays safe."):TEXT("This replaces your current save, wallet, rig and collection.")):TEXT("The current tray and unbanked cargo will be replaced."),491,340,618,.93f,Paper);
  FitText(bConfirmNew?TEXT("This cannot be undone."):TEXT("Banked credits, XP, upgrades and collected cores stay yours."),491,379,618,.90f,bConfirmNew?Danger:Mint);
  AddButton(bConfirmNew?9:8,bConfirmNew?TEXT("Start over"):TEXT("Retry contract"),490,484,280,54,true,true);
  AddButton(7,TEXT("Cancel"),794,484,280,54);
  return;
 }

 if(bWorkshop)
 {
  Veil();
  DrawText(C,TEXT("THE WORKSHOP"),150,86,1.55f,Paper,true);
  DrawText(C,TEXT("Earn better pulls. Choose what your rig does next."),151,132,.95f,Quiet);
  DrawText(C,FString::Printf(TEXT("%d cr  /  RANK %d"),Model.GetWallet(),Rank),1104,92,1.05f,Gold);
  AddButton(21,TEXT("Back to tray"),1264,133,186,38);
  const TCHAR* Names[]={TEXT("DEEP BASKET"),TEXT("POWER COIL"),TEXT("STABILIZER")};
  const TCHAR* Roles[]={TEXT("Bigger valuable batches"),TEXT("Stronger, wider attraction"),TEXT("More time to rescue a risky haul")};
  const EUpgrade Types[]={EUpgrade::Capacity,EUpgrade::Coil,EUpgrade::Stabilizer};
  for(int32 I=0;I<3;++I)
  {
   const float X=150+I*441.f;const int32 Tier=Model.GetUpgradeTier(Types[I]);
   PanelBox(X,197,416,263,I==0?Copper:(I==1?Mint:Gold));
   DrawText(C,Names[I],X+22,219,1.06f,Paper,true);
   DrawText(C,Roles[I],X+23,257,.81f,Quiet);
   for(int32 J=0;J<3;++J)Rect(C,X+24+J*29,292,21,6,J<Tier?Mint:Edge);
   DrawText(C,FString::Printf(TEXT("TIER %d / 3"),Tier),X+132,285,.70f,Quiet);
   FString Effect;
   if(I==0)Effect=Tier<3?FString::Printf(TEXT("%d kg  >  %d kg capacity"),Model.GetCapacity(),Model.GetCapacity()+8):FString::Printf(TEXT("%d kg capacity"),Model.GetCapacity());
   else if(I==1)Effect=Tier<3?FString::Printf(TEXT("%.0f  >  %.0f field radius"),Model.GetFieldRadius(),Model.GetFieldRadius()+35):FString::Printf(TEXT("%.0f field radius"),Model.GetFieldRadius());
   else Effect=Tier<3?FString::Printf(TEXT("%.0fs  >  %.0fs warning fuse"),Model.GetFuseDuration(),Model.GetFuseDuration()+1):FString::Printf(TEXT("%.0fs warning fuse"),Model.GetFuseDuration());
   FitText(Effect,X+23,320,370,.99f,Paper);
   DrawText(C,I==1?TEXT("+25% force per tier. Shift / Q for precision."):(I==0?TEXT("Carry more value in each furnace pour."):TEXT("Expiry costs 1 fuel and your best piece.")),X+23,351,.76f,Quiet);
   FString Reason;const bool CanBuy=Model.CanPurchaseUpgrade(Types[I],Reason);
   const FString BuyLabel=Tier>=3?TEXT("Fully upgraded"):(CanBuy?FString::Printf(TEXT("Upgrade  /  %d cr"),Model.GetUpgradePrice(Types[I])):Reason);
   AddButton(100+I,BuyLabel.IsEmpty()?TEXT("Unavailable"):BuyLabel,X+23,395,370,43,CanBuy,CanBuy);
  }
  DrawText(C,TEXT("CONTRACTS"),150,483,.93f,Paper);
  const bool CanLeave=Model.IsJobEnded()||Failed;
  FitText(CanLeave?TEXT("Choose a contract. Each new delivery changes the salvage arrangement."):TEXT("Finish or retry the active order before starting a different contract."),352,484,1096,.78f,Quiet);
  for(int32 I=0;I<FSalvageModel::LayoutCount;++I)
  {
   const float X=150+I*220.f;const FJobDefinition& D=FSalvageModel::JobDefinition(I);
   const bool Unlocked=Model.IsJobUnlocked(I),Current=I==Model.GetLayoutIndex();
   PanelBox(X,516,208,116,Model.IsJobGold(I)?Gold:(Current?Mint:Edge));
   FitText(FString::Printf(TEXT("%02d  %s"),I+1,*D.Name),X+12,530,184,.81f,Unlocked?Paper:Quiet);
   DrawText(C,FString::Printf(TEXT("%d cr target"),D.Quota),X+12,557,.75f,Quiet);
   const FString Status=!Unlocked?FString::Printf(TEXT("Rank %d required"),D.RequiredLevel):(CanLeave?TEXT("Start order"):(Current?TEXT("Active order"):TEXT("Finish active order")));
   AddButton(200+I,Status,X+12,590,184,30,Unlocked&&CanLeave,Unlocked&&CanLeave);
  }
  DrawText(C,TEXT("RECOVERED CORES"),150,662,.91f,Paper);
  DrawText(C,FString::Printf(TEXT("%d / 6  -  Bank rare salvage to keep it"),Model.GetCollectedCores().Num()),382,664,.77f,Quiet);
  for(int32 I=0;I<6;++I)
  {
   const float X=150+I*220.f;const bool Owned=Model.GetCollectedCores().Contains(I);
   Rect(C,X,700,208,84,Panel);Rect(C,X+12,716,28,35,Owned?Gold:Edge*.6f);
   Rect(C,X+19,725,14,17,Owned?FLinearColor(.98f,.98f,.72f,1):Ink);
   FitText(Owned?FSalvageModel::CoreName(I):TEXT("Unknown core"),X+51,715,147,.75f,Owned?Paper:Quiet);
   DrawText(C,Owned?TEXT("RECOVERED"):TEXT("Find and smelt"),X+51,747,.65f,Owned?Mint:Quiet);
  }
  if(NoticeTimer>0&&!NoticeTitle.IsEmpty())FitText(NoticeTitle+TEXT("  ")+NoticeBody,150,814,1300,.85f,Mint);
  PaintTutorial(C);return;
 }

 if(bReceipt)
 {
  Veil();PanelBox(380,164,840,577,Failed?Danger:(Model.IsGoldAwarded()?Gold:Mint));
  const FString Result=Failed?TEXT("ORDER MISSED"):(Model.IsGoldAwarded()?TEXT("GOLD RECOVERED"):TEXT("ORDER COMPLETE"));
  DrawText(C,Result,426,201,.85f,Failed?Danger:(Model.IsGoldAwarded()?Gold:Mint));
  DrawText(C,Job.Name,423,239,1.60f,Paper,true);
  FitText(Failed?TEXT("The banked haul stays yours. Try a different extraction."):TEXT("Your salvage is banked. Put it to work on the next rig."),426,297,748,.90f,Quiet);
  const int32 Bonus=(Complete?Job.CompletionBonus:0)+(Model.IsGoldAwarded()?Job.GoldBonus:0);
  Rect(C,426,347,748,1,Edge);
  DrawText(C,TEXT("SALVAGE BANKED"),427,370,.78f,Quiet);
  DrawText(C,FString::Printf(TEXT("%d cr"),Model.GetBanked()),908,366,1.08f,Paper);
  DrawText(C,TEXT("ORDER / GOLD BONUSES"),427,412,.78f,Quiet);
  DrawText(C,FString::Printf(TEXT("+%d cr"),Bonus),908,407,1.08f,Gold);
  DrawText(C,TEXT("TOTAL ORDER XP"),427,454,.78f,Quiet);
  DrawText(C,FString::Printf(TEXT("+%d XP"),Model.GetJobEarnings()),908,449,1.08f,Mint);
  DrawText(C,TEXT("FUEL CHARGES USED"),427,496,.78f,Quiet);
  DrawText(C,FString::Printf(TEXT("%d / 4 charges"),Model.GetHeatsUsed()),908,491,1.02f,Paper);
  const FString Loot=LastBank.NewCoreIds.IsEmpty()?FString::Printf(TEXT("Collection: %d of 6 rare cores recovered"),Model.GetCollectedCores().Num()):FString::Printf(TEXT("New core: %s"),*FSalvageModel::CoreName(LastBank.NewCoreIds[0]));
  FitText(Loot,427,543,745,.87f,LastBank.NewCoreIds.IsEmpty()?Quiet:Gold);
  AddButton(30,TEXT("Workshop & next order"),427,616,463,53,true,true);
  AddButton(2,TEXT("Retry this order"),912,616,262,53,true);
  PaintTutorial(C);return;
 }

 if(bPaused)
 {
  Veil();PanelBox(458,130,684,684,Mint);
  DrawText(C,TEXT("PAUSED"),498,196,1.35f,Paper,true);
  DrawText(C,TEXT("Your cargo, fuel and progress are saved."),499,247,.92f,Quiet);
  AddButton(3,TEXT("Resume"),498,305,604,49,true,true);
  AddButton(20,TEXT("Workshop"),498,370,291,43,Ready&&!Tutorial.bRiskHeld);
  AddButton(2,TEXT("Retry contract"),811,370,291,43,Ready);
  AddButton(10,FString::Printf(TEXT("Music  %d%%"),FMath::RoundToInt(MusicVolume*100)),498,434,291,43);
  AddButton(11,FString::Printf(TEXT("Effects  %d%%"),FMath::RoundToInt(SfxVolume*100)),811,434,291,43);
  AddButton(4,bMuted?TEXT("Master sound: OFF"):TEXT("Master sound: ON"),498,498,604,39);
  AddButton(5,bTutorialPractice?TEXT("Restart practice"):TEXT("New career"),498,561,291,43);
  AddButton(6,TEXT("Save & quit"),811,561,291,43);
  AddButton(bTutorialPractice?461:460,bTutorialPractice?TEXT("Return to saved career"):TEXT("Practice tutorial (separate save)"),498,622,604,42);
  FitText(TEXT("LMB hold / Space toggle field  |  Shift hold / Q toggle precision"),499,693,603,.75f,Quiet);
  FitText(TEXT("RMB drops the whole haul recoverably and switches the field off."),499,719,603,.74f,Quiet);
  DrawText(C,TEXT("E smelt  /  ESC resume  /  F11 fullscreen  /  M mute"),499,747,.71f,Quiet);
  DrawText(C,TEXT("Music and effects buttons cycle their volume."),499,773,.68f,Quiet);
 }
 PaintTutorial(C);
}

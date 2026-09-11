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
const FLinearColor Panel(.024f,.053f,.061f,.96f);
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

 auto AddButton=[&](int32 Id,const FString& Label,float X,float Y,float W,float H,bool Enabled=true,bool Primary=false)
 {
  FDemoButton B;
  B.Id=Id; B.Label=Label; B.Enabled=Enabled;
  B.Rect=FBox2D(FVector2D(UX+X*UIScale,UY+Y*UIScale),FVector2D(UX+(X+W)*UIScale,UY+(Y+H)*UIScale));
  Buttons.Add(B);
  const bool Hover=Enabled && B.Rect.IsInside(Pointer);
  const FLinearColor Base=Primary?FLinearColor(.11f,.39f,.32f,1):FLinearColor(.055f,.12f,.14f,1);
  Rect(C,X,Y,W,H,Enabled?(Hover?Base*1.35f:Base):FLinearColor(.04f,.075f,.084f,1));
  Rect(C,X,Y,W,1,Primary?Mint:(Hover?Quiet:Edge));
  const float TextScale=.94f;
  float TextW=0,TextH=0;
  const FVector2D Measured=FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Label,FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),TextScale*UIScale*16.f));
  TextW=Measured.X/UIScale;TextH=Measured.Y/UIScale;
  DrawText(C,Label,X+FMath::Max(10.f,(W-TextW)*.5f),Y+(H-TextH)*.5f,TextScale,Enabled?Paper:Quiet*.55f);
 };

 // Connections are a property of the board. A preview emphasizes only the links
 // from the chosen ring, so it cannot imply a recursive chain reaction.
 if(!bPaused)
 {
  for(const FSalvagePiece& P:Model.GetPieces())
  {
   if(P.Kind!=EPieceKind::Tangle || P.State!=EPieceState::Available) continue;
   for(int32 LinkId:P.DirectLinks)
   {
    const FSalvagePiece* Q=Model.FindPiece(LinkId);
    if(!Q || Q->State!=EPieceState::Available || P.Id>=Q->Id) continue;
    const FVector2D Direction=(Q->Position-P.Position).GetSafeNormal();
    const bool InPreview=Action==EMagnetAction::Aim && Preview.IsValid() &&
     ((P.Id==AimedRing && Preview.PieceIds.Contains(Q->Id)) || (Q->Id==AimedRing && Preview.PieceIds.Contains(P.Id)));
    const FLinearColor LinkColor=InPreview?Mint:FLinearColor(.66f,.53f,.32f,.5f);
    Line(C,Project(World(P.Position+Direction*29,45)),Project(World(Q->Position-Direction*29,45)),LinkColor,InPreview?3.f:1.4f);
   }
  }

  if(Action==EMagnetAction::Aim && Preview.IsValid())
  {
   const FSalvagePiece* Ring=Model.FindPiece(AimedRing);
   if(Ring)
   {
    const FVector2D Start=Ring->Position;
    const FVector2D End=Preview.Endpoint;
    const FVector2D Direction=(End-Start).GetSafeNormal();
    const float Angle=Direction.IsNearlyZero()?0.f:FMath::Atan2(Direction.Y,Direction.X);
    TArray<FVector2D> Boundary;
    constexpr int32 ArcSteps=24;
    for(int32 I=0;I<=ArcSteps;++I)
    {
     const float A=Angle+PI*.5f+PI*I/ArcSteps;
     Boundary.Add(Project(World(Start+FVector2D(FMath::Cos(A),FMath::Sin(A))*FSalvageModel::PullRadius,14)));
    }
    for(int32 I=0;I<=ArcSteps;++I)
    {
     const float A=Angle-PI*.5f+PI*I/ArcSteps;
     Boundary.Add(Project(World(End+FVector2D(FMath::Cos(A),FMath::Sin(A))*FSalvageModel::PullRadius,14)));
    }
    const FVector2D Center=Project(World((Start+End)*.5f,14));
    for(int32 I=0;I<Boundary.Num();++I)
    {
     const FVector2D A=Boundary[I],B=Boundary[(I+1)%Boundary.Num()];
     FCanvasTriangleItem Fill(Center,A,B,GWhiteTexture);
     Fill.SetColor(FLinearColor(.18f,.85f,.61f,.075f));
     Fill.BlendMode=SE_BLEND_Translucent;
     C->DrawItem(Fill);
     Line(C,A,B,FLinearColor(.35f,.94f,.72f,.8f),2.f);
    }
    Line(C,Project(World(Start,62)),Project(World(End,64)),Mint,2.f);
    for(int32 Id:Preview.PieceIds)
    {
     const FSalvagePiece* P=Model.FindPiece(Id);
     if(P && P->State==EPieceState::Available)
      WorldCircle(C,P->Position,P->Kind==EPieceKind::Tangle?37.f:20.f,Mint,P->Kind==EPieceKind::Tangle?2.5f:1.2f,P->Kind==EPieceKind::Tangle?58.f:19.f);
    }
   }
  }
  else if(Action==EMagnetAction::Sweep)
  {
   WorldCircle(C,Magnet,FSalvageModel::SweepRadius,FLinearColor(.34f,.94f,.72f,.45f),1.4f,12);
  }
  else if(HoverRing!=INDEX_NONE)
  {
   if(const FSalvagePiece* P=Model.FindPiece(HoverRing))
    WorldCircle(C,P->Position,40,Copper,2.5f,58);
  }

  if(bFurnaceHover && Model.GetCargo()>0)
   WorldCircle(C,FVector2D(650,0),112,Copper,3.f,77);
  const FVector2D FurnaceLabel=Project(World(FVector2D(650,180),30));
  const float FX=(FurnaceLabel.X-UX)/UIScale-74.f;
  const float FY=(FurnaceLabel.Y-UY)/UIScale;
  Rect(C,FX-10,FY-5,172,50,Panel);
  DrawText(C,TEXT("FURNACE"),FX,FY,.81f,Copper);
  DrawText(C,Model.GetCargo()>0?TEXT("Click to pour your haul"):TEXT("Bring your haul here"),FX,FY+21,.72f,Paper);
 }

 // A narrow instrument strip keeps the entire workbench available to play.
 Rect(C,32,24,1536,124,Panel);
 Rect(C,32,24,5,124,Copper);
 DrawText(C,TEXT("MAGNET SWEEP"),56,42,1.15f,Paper,true);
 DrawText(C,TEXT("CONCEPT DEMO  /  TAKE YOUR TIME"),58,94,.77f,Quiet);

 const FString GoalLabel=DisplayUpgrade==0?TEXT("FORGE A BREAKAWAY COIL"):
  DisplayUpgrade==1?TEXT("FORGE LONG REACH"):TEXT("RECOVER A METAL BLOCK");
 DrawText(C,GoalLabel,465,43,.8f,Model.IsDeliveryCompleted()?Mint:Quiet);
 const FString Progress=Model.IsDeliveryCompleted()?TEXT("Delivery complete"):
  FString::Printf(TEXT("%d / %d metal banked"),Model.GetBanked(),Model.GetGoal());
 DrawText(C,Progress,465,68,1.04f,Paper);
 constexpr float BarWidth=475;
 const float BankedFraction=FMath::Clamp(float(Model.GetBanked())/Model.GetGoal(),0.f,1.f);
 const float WithCargoFraction=FMath::Clamp(float(Model.GetBanked()+Model.GetCargo())/Model.GetGoal(),0.f,1.f);
 Rect(C,465,108,BarWidth,8,Ink);
 Rect(C,465,108,BarWidth*WithCargoFraction,8,FLinearColor(.20f,.40f,.35f,1));
 Rect(C,465,108,BarWidth*BankedFraction,8,Model.IsDeliveryCompleted()?Mint:Copper);
 if(!Model.IsDeliveryCompleted() && Model.GetCargo()>0)
  Rect(C,465+BarWidth*WithCargoFraction-1,105,2,14,Mint);

 DrawText(C,TEXT("YOUR HAUL"),977,43,.76f,Quiet);
 DrawText(C,FString::Printf(TEXT("%d"),Model.GetCargo()),977,67,1.2f,Model.GetCargo()>0?Mint:Paper,true);
 DrawText(C,TEXT("safe on the magnet"),977,108,.67f,Quiet);
 Rect(C,1160,46,1,70,Edge);
 DrawText(C,TEXT("YOUR RIG"),1185,43,.76f,Quiet);
 const FString Rig=DisplayUpgrade==0?TEXT("Starter magnet"):
  DisplayUpgrade==1?TEXT("Breakaway coil"):TEXT("Complete rig");
 DrawText(C,Rig,1185,69,.98f,Paper);
 DrawText(C,DisplayUpgrade==0?TEXT("A stronger pull is ahead"):
  DisplayUpgrade==1?TEXT("Linked rings can break free"):TEXT("Breakaway + extended reach"),1185,108,.67f,DisplayUpgrade>0?Mint:Quiet);
 AddButton(3,TEXT("Pause"),1454,45,90,40);
 AddButton(4,bMuted?TEXT("Sound off"):TEXT("Sound on"),1454,94,90,30);
 DrawText(C,Model.GetLayoutName().ToUpper(),48,163,.79f,Quiet);

 // Positive feedback concerns this action only; there is no score to protect.
 if(NoticeTimer>0 && !NoticeTitle.IsEmpty() && !bPaused)
 {
  const float Fade=FMath::Min(1.f,NoticeTimer*2.f);
  Rect(C,470,166,670,67,FLinearColor(.025f,.07f,.075f,.94f*Fade));
  Rect(C,470,166,3,67,FLinearColor(.34f,.94f,.72f,Fade));
  DrawText(C,NoticeTitle,490,175,1.f,FLinearColor(.89f,.94f,.90f,Fade));
  DrawText(C,NoticeBody,490,205,.77f,FLinearColor(.60f,.76f,.73f,Fade));
 }

 Rect(C,32,782,1536,94,Panel);
 Rect(C,32,782,1536,1,Edge);
 FString HintTitle,HintBody;
 if(PourTimer>0 || ForgeTimer>0 || bPendingNext)
 {
  HintTitle=ForgeTimer>0?TEXT("Making your magnet stronger"):TEXT("A whole haul, safely recovered");
  HintBody=TEXT("Enjoy the pour. Your next delivery will be ready when it finishes.");
 }
 else if(Action==EMagnetAction::Aim && Preview.IsValid())
 {
  int32 Tangles=0;
  for(int32 Id:Preview.PieceIds) if(const FSalvagePiece* P=Model.FindPiece(Id))
   if(P->Kind==EPieceKind::Tangle) ++Tangles;
  HintTitle=FString::Printf(TEXT("Release to pull %d metal%s"),Preview.Amount,Tangles>1?*FString::Printf(TEXT(" from %d tangles"),Tangles):TEXT(""));
  HintBody=Model.HasBreakaway()?TEXT("The lit pieces will come with you. Only directly linked rings can join this pull."):
   TEXT("The lit pieces will come with you. The selected tangle always breaks free.");
 }
 else if(Action==EMagnetAction::Sweep)
 {
  HintTitle=TEXT("Sweep through the loose scrap");
  HintBody=TEXT("Release whenever you like. Everything collected stays on your magnet.");
 }
 else if(bFurnaceHover && Model.GetCargo()>0)
 {
  HintTitle=FString::Printf(TEXT("Click to pour %d metal"),Model.GetCargo());
  HintBody=TEXT("The entire haul counts. There is no need to aim precisely.");
 }
 else if(HoverRing!=INDEX_NONE)
 {
  HintTitle=TEXT("Press this ring, drag, then release");
  HintBody=Model.HasBreakaway()?TEXT("Aim through lit links and loose scrap for a bigger breakaway."):
   TEXT("Choose a direction to gather loose scrap along your pull.");
 }
 else if(Model.IsDeliveryCompleted())
 {
  HintTitle=TEXT("Delivery complete. Take another satisfying sweep?");
  HintBody=TEXT("Keep collecting if you want, or bring your earned rig to the next arrangement.");
 }
 else if(Model.GetCargo()>0 && Model.GetCargo()+Model.GetBanked()>=Model.GetGoal())
 {
  HintTitle=TEXT("That haul can finish the forge");
  HintBody=TEXT("Click the furnace to pour it in, or keep collecting. Your cargo has no limit.");
 }
 else
 {
  HintTitle=TEXT("Hold the mouse button and sweep to collect");
  HintBody=TEXT("Drag from a ring for a pull. Click the furnace to pour. Releasing keeps your cargo.");
 }
 DrawText(C,HintTitle,57,800,1.04f,Paper);
 DrawText(C,HintBody,57,837,.8f,Quiet);
 const bool Ready=PourTimer<=0 && ForgeTimer<=0 && !bPendingNext;
 if(Model.IsDeliveryCompleted())
  AddButton(1,Model.GetCargo()>0?TEXT("Pour & next delivery"):TEXT("Next delivery"),1220,802,270,49,Ready,true);
 else
  AddButton(2,TEXT("Retry tray"),1350,802,140,49,Ready);

 if(bPaused || bConfirmRetry || bConfirmNew)
 {
  Buttons.Reset();
  FCanvasTileItem Veil(FVector2D::ZeroVector,FVector2D(Width,Height),FLinearColor(.006f,.015f,.022f,.85f));
  Veil.BlendMode=SE_BLEND_Translucent;
  C->DrawItem(Veil);
  const bool Confirm=bConfirmRetry || bConfirmNew;
  Rect(C,480,225,640,450,Panel);
  Rect(C,480,225,640,3,Confirm?Copper:Mint);
  DrawText(C,Confirm?(bConfirmNew?TEXT("Restart the demo?"):TEXT("Retry this tray?")):TEXT("Take a breather"),520,269,1.3f,Paper,true);
  if(Confirm)
  {
   DrawText(C,bConfirmNew?TEXT("Start again with the starter magnet and a fresh tray."):
    TEXT("Restock this tray and clear its current haul."),520,346,.96f,Paper);
   DrawText(C,bConfirmNew?TEXT("This replaces your saved demo progress."):
    TEXT("All earned rig improvements stay with you."),520,382,.9f,bConfirmNew?Copper:Mint);
   AddButton(bConfirmNew?9:8,bConfirmNew?TEXT("Restart demo"):TEXT("Retry tray"),520,470,250,55,true,true);
   AddButton(7,TEXT("Keep playing"),790,470,250,55);
  }
  else
  {
   DrawText(C,TEXT("Your cargo and progress are kept."),520,336,.95f,Quiet);
   AddButton(3,TEXT("Resume"),520,391,540,53,true,true);
   AddButton(2,TEXT("Retry this tray"),520,460,260,48);
   AddButton(5,TEXT("Restart demo"),800,460,260,48);
   AddButton(4,bMuted?TEXT("Turn sound on"):TEXT("Turn sound off"),520,525,260,48);
   AddButton(6,TEXT("Save & quit"),800,525,260,48);
   DrawText(C,TEXT("ESC to return to the workbench"),520,617,.78f,Quiet);
  }
 }
}

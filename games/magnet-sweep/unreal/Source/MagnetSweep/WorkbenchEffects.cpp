#include "WorkbenchRuntime.h"
#include "MagnetGame.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "GlobalRenderResources.h"
using namespace MagnetSweep;
namespace {
FName PieceMaterial(const FSalvagePiece& P){switch(P.Material){case EMaterial::Copper:return TEXT("Copper");case EMaterial::Alloy:return TEXT("Alloy");case EMaterial::Core:return TEXT("Brass");case EMaterial::HotCell:return TEXT("Hazard");default:return TEXT("Steel");}}
float Ease(float T){return 1-FMath::Pow(1-FMath::Clamp(T,0.f,1.f),3);}
}
void FWorkbenchImpl::BuildPieces()
{
 for(auto& Pair:Visuals){auto& V=Pair.Value;if(V.Mesh)V.Mesh->DestroyComponent();if(V.Ring)V.Ring->DestroyComponent();for(auto* D:V.Details)D->DestroyComponent();}
 Visuals.Empty();Particles.Empty();Popups.Empty();SeenRare.Empty();int Slot=0;
 for(const FSalvagePiece& P:Model.GetPieces())
 {
  FPieceVisual V;V.Slot=Slot++;V.Spin=P.Id*47;
  const bool Core=P.Material==EMaterial::Core,Cell=P.Material==EMaterial::HotCell;
  FName Name=P.Material==EMaterial::Alloy?TEXT("SM_Plate"):(P.Id%2?TEXT("SM_Bolt"):TEXT("SM_Washer"));
  V.Size=P.Material==EMaterial::Alloy?FVector(49,37,16):FVector(35,31,15);
  if(P.Material==EMaterial::Copper){Name=TEXT("SM_Ring");V.Size=FVector(40,40,19);}
  if(Core){Name=TEXT("Cylinder");V.Size=FVector(48,48,34);}
  if(Cell){Name=TEXT("Cube");V.Size=FVector(34,49,27);}
  V.Position=World(P.Position,Cell?22:19);V.From=V.Position;
  V.Mesh=Shape(Name,V.Position,V.Size,PieceMaterial(P),FRotator(0,V.Spin,0),false);
  if(Core){V.Details.Add(Shape(TEXT("SM_Ring"),V.Position+FVector(0,0,20),FVector(48,48,9),TEXT("Copper"),FRotator::ZeroRotator,false));V.Details.Add(Shape(TEXT("Sphere"),V.Position+FVector(0,0,22),FVector(26,26,26),TEXT("Core"),FRotator::ZeroRotator,false));}
  if(Cell){V.Details.Add(Shape(TEXT("Cube"),V.Position+FVector(0,0,17),FVector(29,9,4),TEXT("Warning"),FRotator::ZeroRotator,false));V.Details.Add(Shape(TEXT("Cylinder"),V.Position+FVector(0,-14,18),FVector(11,11,6),TEXT("Steel"),FRotator::ZeroRotator,false));}
  const bool Visible=P.State==EPieceState::Available||P.State==EPieceState::Cargo;V.Mesh->SetVisibility(Visible);for(auto* D:V.Details)D->SetVisibility(Visible);
  Visuals.Add(P.Id,V);if(P.Material==EMaterial::Core&&P.State!=EPieceState::Available)SeenRare.Add(P.Id);
 }
 Preview={};AimedRing=INDEX_NONE;bFieldLatched=false;Action=EMagnetAction::None;
}
void FWorkbenchImpl::AddBurst(FVector Position,FLinearColor Color,int32 Count,float Force)
{
 for(int32 I=0;I<Count&&Particles.Num()<400;I++)
 {
  const float A=(I*2.39996f)+Time*2;const float V=Force*(.5f+.5f*FMath::Abs(FMath::Sin(I*3.17f)));
  FForceParticle P;P.Position=Position;P.Velocity=FVector(FMath::Cos(A)*V,FMath::Sin(A)*V,Force*(.4f+.6f*FMath::Abs(FMath::Cos(I*5.1f))));P.Color=Color;P.Life=P.Maximum=.35f+.6f*FMath::Abs(FMath::Sin(I*1.83f));P.Size=1.4f+(I%3);Particles.Add(P);
 }
}
void FWorkbenchImpl::UpdateVisuals(float Delta)
{
 const bool Unsafe=Model.IsCargoUnsafe();
 const float Shake=Unsafe?1.6f+Model.GetFuseElapsed():0.f;
 const FVector MagnetPosition=World(Magnet,84+FMath::Sin(Time*3)*.7f)+FVector(FMath::Sin(Time*57)*Shake,FMath::Cos(Time*39)*Shake,Impact*4);
 MagnetAngle=FMath::FInterpTo(MagnetAngle,FMath::Clamp(float(MagnetVelocity.X)*.011f,-9.f,9.f),Delta,10);
 for(int I=0;I<MagnetShapes.Num();I++)
 {
  auto* S=MagnetShapes[I];const bool Coil=Coils.Contains(S),Basket=ReachParts.Contains(S),Stabilizer=StabilizerParts.Contains(S);
  const int32 CoilIndex=Coils.Find(S);
  S->SetVisibility((!Coil||CoilIndex%3<Model.GetUpgradeTier(EUpgrade::Coil))&&(!Basket||Model.GetUpgradeTier(EUpgrade::Capacity)>0)&&(!Stabilizer||Model.GetUpgradeTier(EUpgrade::Stabilizer)>0));
  FVector Size=MagnetSizes[I]*(1.f+Impact*.035f);if(Basket)Size.X*=1+.22f*Model.GetUpgradeTier(EUpgrade::Capacity);
  const FRotator R=Coil?FRotator(90,MagnetAngle,0):FRotator(0,MagnetAngle,0);
  Place(S,MagnetPosition+FRotator(0,MagnetAngle,0).RotateVector(MagnetOffsets[I]),Size,R);
 }
 if(Materials.Contains(TEXT("Magnet")))Materials[TEXT("Magnet")]->SetScalarParameterValue(TEXT("Glow"),FieldIntensity*.14f);
 if(Materials.Contains(TEXT("Warning")))Materials[TEXT("Warning")]->SetScalarParameterValue(TEXT("Glow"),1.3f+.9f*FMath::Sin(Time*8));
 int CargoIndex=0;
 for(const auto& P:Model.GetPieces())if(auto* V=Visuals.Find(P.Id))
 {
  const bool Dead=P.State==EPieceState::Lost||(P.State==EPieceState::Banked&&!V->bPouring);
  V->Mesh->SetVisibility(!Dead);for(auto* D:V->Details)D->SetVisibility(!Dead);if(Dead)continue;
  float Scale=1;FRotator Rotation(0,V->Spin,0);
  if(P.State==EPieceState::Available)
  {
   const FVector Target=World(P.Position,(P.Material==EMaterial::HotCell?22:19)+FMath::Abs(V->Attracted)*FMath::Min(19.f,float(V->Velocity.Size())*.04f));
   if(V->Progress<1){V->Progress=FMath::Min(1.f,V->Progress+Delta/V->Duration);V->Position=FMath::Lerp(V->From,Target,Ease(V->Progress))+FVector(0,0,FMath::Sin(V->Progress*PI)*65);}
   else V->Position=Target;
   V->Spin+=Delta*V->Velocity.Size()*.17f;Rotation=FRotator(FMath::Sin(Time*18+P.Id)*FMath::Abs(V->Attracted)*5,V->Spin,0);
   V->Mesh->SetMaterial(0,Materials[PieceMaterial(P)]);
  }
  else
  {
   V->Progress=FMath::Min(1.f,V->Progress+Delta/V->Duration);const float A=Ease(V->Progress);
   const float Angle=CargoIndex*2.39996f;const float Radius=10+7*FMath::Sqrt(float(CargoIndex+1));CargoIndex++;
   FVector Target=MagnetPosition+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius-7,-29-(CargoIndex%3)*7);
   if(V->bPouring)Target=World({650,0},65);
   V->Position=FMath::Lerp(V->From,Target,A)+FVector(0,0,FMath::Sin(V->Progress*PI)*(V->bPouring?105:14));
   Scale=V->bPouring?FMath::Max(.02f,1-V->Progress*.98f):FMath::Lerp(1.f,.67f,A);
   Rotation=FRotator(V->bPouring?V->Progress*220:0,V->Spin+V->Progress*35,0);
   if(V->bPouring&&V->Progress>=1){V->bPouring=false;V->Mesh->SetVisibility(false);for(auto* D:V->Details)D->SetVisibility(false);}
  }
  Place(V->Mesh,V->Position,V->Size*Scale,Rotation);
  for(int32 I=0;I<V->Details.Num();I++)
  {
   const bool Core=P.Material==EMaterial::Core;
   const FVector Offset=Core?FVector(0,0,I?22:20):FVector(0,I?-14:0,I?18:17);
   const FVector DetailSize=Core?(I?FVector(26,26,26):FVector(48,48,9)):(I?FVector(11,11,6):FVector(29,9,4));
   Place(V->Details[I],V->Position+Rotation.RotateVector(Offset*Scale),DetailSize*Scale,Rotation);
  }
 }
 for(auto& P:Particles){P.Life-=Delta;P.Position+=P.Velocity*Delta;P.Velocity.Z-=260*Delta;P.Velocity*=FMath::Exp(-Delta*.65f);}
 Particles.RemoveAll([](const auto& P){return P.Life<=0;});
 for(auto& P:Popups){P.Life-=Delta;P.Position.Z+=Delta*40;}Popups.RemoveAll([](const auto& P){return P.Life<=0;});
 if(FurnaceFill)Place(FurnaceFill,World({650,0},62),FVector(146,146,3+5*FMath::Clamp(float(Model.GetBanked())/Model.GetGoal(),0.f,1.f)));
 if(RecoveredBlock){RecoveredBlock->SetVisibility(Model.GetBanked()>0&&PourTimer<=0);if(Model.GetBanked()>0)Place(RecoveredBlock,FVector(650,-216,28),FVector(118,70,26+14*FMath::Min(1.f,float(Model.GetBanked())/Model.GetGoal())));}
 if(FurnaceLight)FurnaceLight->SetIntensity(1450+FurnacePulse*6000+100*FMath::Sin(Time*6));
 if(Camera)Camera->SetWorldLocation(FVector(80+FMath::Sin(Time*67)*Impact*2,920,1500+FMath::Sin(Time*51)*Impact*1.3));
}
void FWorkbenchImpl::PaintEffects(UCanvas* C)
{
 if(!C)return;
 // Visible chain segments are the actual atomic capture group, not a hidden cascade graph.
 for(const auto& P:Model.GetPieces())
 {
  if(P.State!=EPieceState::Available)continue;
  const auto* V=Visuals.Find(P.Id);if(!V)continue;
  for(int32 Link:P.DirectLinks)
  {
   const auto* Q=Model.FindPiece(Link);const auto* W=Visuals.Find(Link);if(!Q||!W||P.Id>=Link||Q->State!=EPieceState::Available)continue;
   const FVector A=V->Position+FVector(0,0,6),B=W->Position+FVector(0,0,6);const int N=FMath::Max(2,FMath::RoundToInt(FVector::Distance(A,B)/13));
   for(int I=1;I<N;I++)
   {
    const FVector T=FMath::Lerp(A,B,float(I)/N);const FVector2D S=Project(T);FCanvasTileItem Dot(S-FVector2D(2,2),FVector2D(5,4),FLinearColor(.7,.55,.3,.85));Dot.BlendMode=SE_BLEND_Translucent;C->DrawItem(Dot);
   }
   Line(C,Project(A),Project(B),FLinearColor(.48,.37,.18,.8),1.2f);
  }
  if(P.Material==EMaterial::HotCell)
  {
   const FVector2D S=Project(V->Position+FVector(0,0,37));DrawText(C,TEXT("!"),(S.X-UX)/UIScale-4,(S.Y-UY)/UIScale-9,.9f,FLinearColor(1,.4,.15));
  }
  if(P.Material==EMaterial::Core)
  {
   WorldCircle(C,P.Position,31,FLinearColor(1,.67,.13,.35f+.15f*FMath::Sin(Time*3)),1.6f,12);
   if(((int)(Time*15)+P.Id)%13==0&&Particles.Num()<120)AddBurst(V->Position+FVector(0,0,28),FLinearColor(1,.8,.22),1,20);
  }
 }
 if(FieldIntensity>.03f&&!bPaused&&!bWorkshop&&!bReceipt)
 {
  const FLinearColor Color=Model.IsCargoUnsafe()?FLinearColor(1,.19,.04):FLinearColor(.13,.88,.69);
  for(int Arm=0;Arm<12;Arm++)
  {
   FVector2D Last;for(int Step=0;Step<=12;Step++)
   {
    const float T=float(Step)/12;const float Radius=22+(FieldRadius-22)*T;const float Angle=Arm*PI/6+.4f*(1-T)+Time*.05f;
    const auto Point=Project(World(Magnet+FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*Radius,15+12*(1-T)));
    if(Step)Line(C,Last,Point,FLinearColor(Color.R,Color.G,Color.B,FieldIntensity*.17f),1);Last=Point;
   }
   const float T=1-FMath::Frac(Time*1.1f+Arm*.137f);const float R=22+(FieldRadius-22)*T;const float A=Arm*PI/6+.4f*(1-T)+Time*.05f;
   auto S=Project(World(Magnet+FVector2D(FMath::Cos(A),FMath::Sin(A))*R,20));FCanvasTileItem Dot(S,FVector2D(2.3,2.3),FLinearColor(Color.R,Color.G,Color.B,FieldIntensity*.55f));Dot.BlendMode=SE_BLEND_Additive;C->DrawItem(Dot);
  }
  WorldCircle(C,Magnet,FieldRadius,FLinearColor(Color.R,Color.G,Color.B,FieldIntensity*.18f),1,10);
 }
 for(const auto& P:Particles)
 {
  const FVector2D S=Project(P.Position);const float Alpha=FMath::Clamp(P.Life/P.Maximum,0.f,1.f);FLinearColor Color=P.Color;Color.A=Alpha;
  Line(C,S,Project(P.Position-P.Velocity*.026f),Color,P.Size*.6f);FCanvasTileItem Dot(S-FVector2D(P.Size*.5f),FVector2D(P.Size),Color);Dot.BlendMode=SE_BLEND_Additive;C->DrawItem(Dot);
 }
 for(const auto& P:Popups)
 {const auto S=Project(P.Position);FLinearColor Color=P.Color;Color.A=FMath::Min(1.f,P.Life*2);DrawText(C,P.Text,(S.X-UX)/UIScale-40,(S.Y-UY)/UIScale,.88f,Color);}
 if(PourTimer>0)WorldCircle(C,{650,0},102+(2.6f-PourTimer)*22,FLinearColor(1,.48,.05,FMath::Max(0.f,PourTimer/2.6f)*.6f),2,12);
}

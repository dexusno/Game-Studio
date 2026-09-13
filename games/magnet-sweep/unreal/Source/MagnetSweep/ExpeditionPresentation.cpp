#include "ExpeditionRuntime.h"
#include "ExpeditionRig.h"
#include "ExpeditionWorld.h"
#include "WorkbenchRuntime.h"
#include "MagnetGame.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/DateTime.h"

using namespace MagnetSweep;
namespace
{
const FLinearColor Paper(.89f,.96f,.91f), Quiet(.59f,.74f,.72f), Mint(.31f,.94f,.69f);
const FLinearColor Copper(1.f,.59f,.25f), Red(1.f,.28f,.18f), Panel(.008f,.025f,.029f,.96f);
FString NameOf(FName Id){const auto* M=FExpeditionRig::FindModule(Id);return M?M->Name:TEXT("Empty socket");}
FName MaterialOf(const FExpeditionBody& B)
{
    if(B.bHot) return TEXT("Hazard"); if(B.bGoal) return TEXT("Core");
    if(B.Material==EExpeditionMaterial::Copper) return TEXT("Copper");
    if(B.Material==EExpeditionMaterial::Alloy) return TEXT("Alloy");
    return B.Material==EExpeditionMaterial::Mechanism?FName(TEXT("Brass")):FName(TEXT("Steel"));
}
bool IsVisible(const FExpeditionBody& B){return B.State==EExpeditionBodyState::Available||B.State==EExpeditionBodyState::Pulling||B.State==EExpeditionBodyState::Cargo||
    (B.Role==TEXT("power_frame") && B.State==EExpeditionBodyState::Banked);}
FString LabelOf(const FExpeditionBody& B)
{
    if(B.Role==TEXT("power_frame") && B.State==EExpeditionBodyState::Banked)return TEXT("POWER FRAME / INSTALLED");
    FString Name=B.Role.ToString();Name.ReplaceInline(TEXT("_"),TEXT(" "));
    return FString::Printf(TEXT("%s  %.0f kg%s%s%s"),*Name,B.Mass,B.bHot?TEXT("  HOT"):TEXT(""),
        B.bFunctional?TEXT("  WORKING"):TEXT(""),B.bBrittle?TEXT("  BRITTLE"):TEXT(""));
}
}
void FExpeditionRuntime::Text(UCanvas* C,const FString& S,float X,float Y,float Scale,FLinearColor Color,bool Large)
{if(Display)Display->DrawText(C,S,X,Y,Scale,Color,Large);}
void FExpeditionRuntime::Wrap(UCanvas* C,const FString& S,float X,float Y,int32 Characters,float Scale,FLinearColor Color)
{
    TArray<FString> Words;S.ParseIntoArrayWS(Words);FString Line;
    for(const auto& Word:Words)
    {
        if(Line.Len()+Word.Len()+1>Characters && !Line.IsEmpty())
        {Text(C,Line,X,Y,Scale,Color);Y+=22*Scale;Line.Empty();}
        if(!Line.IsEmpty())Line+=TEXT(" ");Line+=Word;
    }
    if(!Line.IsEmpty())Text(C,Line,X,Y,Scale,Color);
}
void FExpeditionRuntime::AddButton(UCanvas* C,int32 Id,const FString& Label,float X,float Y,float W,float H,bool Enabled)
{
    if(!Display)return;
    const FBox2D Box(FVector2D(Display->UX+X*Display->UIScale,Display->UY+Y*Display->UIScale),
                     FVector2D(Display->UX+(X+W)*Display->UIScale,Display->UY+(Y+H)*Display->UIScale));
    const bool Hover=Box.IsInside(Pointer);
    Display->Rect(C,X,Y,W,H,Enabled?(Hover?FLinearColor(.10f,.30f,.27f):FLinearColor(.035f,.14f,.14f)):FLinearColor(.035f,.06f,.065f));
    Display->Rect(C,X,Y,3,H,Enabled?Mint:Quiet*.35f);
    Text(C,Label,X+12,Y+H*.5f-10,.9f,Enabled?Paper:Quiet*.7f);
    Buttons.Add({Box,Label,Id,Enabled});
}
void FExpeditionRuntime::Click(int32 Id)
{
    Input.Cancel();ClearPreparation();
    if(Id==50){ReceiveFrame();return;}
    if(Screen==EExpeditionScreen::Site && !bPaused && Id>=60 && Id<68)
    {ToolOperation[(Id-60)/4]=(Id-60)%4;return;}
    if(Screen==EExpeditionScreen::Site && !bPaused && Id>=2000)
    {
        const auto* B=World->FindBody(Id-2000);
        if(B && (B->State==EExpeditionBodyState::Cargo || World->GetState().CycloneIds.Contains(B->Id)) && !B->bGoal)
        {
            SelectedCargoId=B->Id;
            if(bPrepareWeld && B->State==EExpeditionBodyState::Cargo && B->Material==EExpeditionMaterial::Iron && !B->bHot)
            {if(WeldSelection.Contains(B->Id))WeldSelection.Remove(B->Id);else WeldSelection.Add(B->Id);}
        }
        return;
    }
    if(Screen==EExpeditionScreen::Site && !bPaused && (Id==40 || Id==41))
    {
        bPrepareWeld=Id==41 && Rig->Has(TEXT("slug_press"));
        Show(bPrepareWeld?TEXT("Select two or more iron pieces below. Aim with the Rail tool, then release to weld for 4 battery."):
             TEXT("Select one held body to launch. C cycles ammunition. Aim with the Rail tool, then release to fire."));return;
    }
    if(Id>=100 && Id<106)
    {
        const TArray<FName> Starters={TEXT("extraction_coil"),TEXT("rail_impeller"),TEXT("arc_driver"),TEXT("anchor_winch"),TEXT("vector_emitter"),TEXT("relay_projector")};
        const FName Starter=Starters[Id-100];
        if(!Rig->IsStarterUnlocked(Starter)||!FExpeditionWorld::IsModuleImplemented(Starter))return;
        SiteIndex=0;const int32 Seed=FMath::Max(1,int32(FDateTime::UtcNow().GetTicks()%(MAX_int32-4)));
        World->StartSite(0,Seed);TArray<FName> Implemented;
        for(const auto& M:FExpeditionRig::Catalog())if(FExpeditionWorld::IsModuleImplemented(M.Id))Implemented.Add(M.Id);
        Rig->SetShopContext(World->GetOpportunityTags(),Implemented);
        if(!Rig->NewRun(Starter,Seed)){Show(TEXT("That starter is not available."));return;}
        FString Reason;Rig->GenerateOffers(Reason);
        Screen=EExpeditionScreen::Depot;bPaused=false;bSaveAllowed=true;SiteEntry.Reset();
        SelectedCargoId=INDEX_NONE;WeldSelection.Reset();bPrepareWeld=false;ToolOperation[0]=ToolOperation[1]=0;
        Show(Reason.IsEmpty()?TEXT("Choose a useful support or another active tool. Purchases can be fitted here for free."):Reason);
        BuildMechanismVisuals();Save();return;
    }
    if(Id>=1000 && Id<1000+FExpeditionRig::Catalog().Num()){SelectedModule=Id-1000;return;}
    FString Reason;
    const auto& Catalog=FExpeditionRig::Catalog();
    const auto* Selected=Catalog.IsValidIndex(SelectedModule)?&Catalog[SelectedModule]:nullptr;
    if(Id==10)Depart();
    else if(Id==11){Show(Rig->Precharge(Reason)?TEXT("Next site battery: 120. This service does not stack."):Reason);Save();}
    else if(Id==12 && Selected)
    {
        if(Rig->Buy(Selected->Id,Reason))
        {
            FString FitReason;const bool Fitted=Rig->Fit(Selected->Id,FitReason);
            Show(Fitted?Selected->Name+TEXT(" installed. ")+Selected->Description:
                 Selected->Name+TEXT(" purchased. Refit below: ")+FitReason,TEXT("upgrade"));
        }else Show(Reason);
        Save();
    }
    else if(Id==13 && Selected){Show(Rig->Fit(Selected->Id,Reason)?Selected->Name+TEXT(" fitted."):Reason);Save();}
    else if(Id==14 && Selected){Show(Rig->Unfit(Selected->Id,Reason)?Selected->Name+TEXT(" removed from the rig; still owned."):Reason);Save();}
    else if(Id==15 && Selected)
    {
        const int32 Refund=Rig->GetResaleValue(Selected->Id);
        Show(Rig->Sell(Selected->Id,Reason)?FString::Printf(TEXT("Sold %s for %d credits."),*Selected->Name,Refund):Reason);Save();
    }
    else if(Id==20)SetPaused(false);
    else if(Id==21)RetrySite();
    else if(Id==22)
    {
        World->Evacuate();Screen=EExpeditionScreen::Retreated;bPaused=false;SiteEntry.Reset();
        Show(TEXT("Expedition ended. Earlier discoveries remain; this site's unfinished reward is lost."));Save();
    }
    else if(Id==23)
    {
        bContinuousField=!bContinuousField;
        Show(bContinuousField?TEXT("Continuous comparison: each new valid capture batch costs 6 battery."):
             TEXT("Deliberate field: one press buys one capture batch for 6 battery."));Save();
    }
    else if(Id==30)
    {
        if(!bConfirmNew){bConfirmNew=true;Show(TEXT("Start fresh equipment and credits? Discoveries and archived rigs remain. Click again to confirm."));}
        else {bConfirmNew=false;Screen=EExpeditionScreen::Starter;bPaused=false;SiteEntry.Reset();SelectedModule=INDEX_NONE;}
    }
    else if(Id==31){Save();if(Owner)UKismetSystemLibrary::QuitGame(Owner,PC,EQuitPreference::Quit,false);}
}

void FExpeditionRuntime::BuildMechanismVisuals()
{
    if(!Display)return;
    for(auto* S:MechanismShapes)if(S)S->DestroyComponent();MechanismShapes.Reset();
    auto Mark=[&](FVector2D P,FVector Size,FName Mat)
    {MechanismShapes.Add(Display->Shape(TEXT("Cube"),Display->World(P,18),Size,Mat,FRotator::ZeroRotator,false));};
    const auto& Definition=World->GetSiteDefinition();
    for(const auto& Marker:Definition.Markers)
    {
        if(Marker.Id==TEXT("furnace"))continue; // Existing furnace mesh remains the shared receiver for scrap.
        Mark(Marker.Position,FVector(Marker.Radius*2,Marker.Radius*2,6),Marker.Id==TEXT("circuit")?TEXT("Copper"):TEXT("Dark"));
        if(Marker.Id==TEXT("frame_receiver"))
            MechanismShapes.Add(Display->Shape(TEXT("SM_Ring"),Display->World(Marker.Position,23),FVector(Marker.Radius*2,Marker.Radius*2,5),TEXT("Glow"),FRotator::ZeroRotator,false));
        if(Marker.Id==TEXT("receiver"))
            MechanismShapes.Add(Display->Shape(TEXT("SM_Ring"),Display->World(Marker.Position,38),FVector(110,110,12),TEXT("Glow"),FRotator::ZeroRotator,false));
    }
    for(const auto& O:World->GetObstacles())
        Mark(O.Center,FVector(O.HalfSize.X*2,O.HalfSize.Y*2,30),O.Role.ToString().Contains(TEXT("press"))?FName(TEXT("Hazard")):FName(TEXT("Edge")));
    PressVisual=nullptr;ArmVisual=nullptr;
    if(Definition.bHasPress)
    {
        PressVisual=Display->Shape(TEXT("Cube"),Display->World(Definition.PressCenter,120),
            FVector(Definition.PressHalfSize.X*2,Definition.PressHalfSize.Y*2,18),TEXT("Hazard"),FRotator::ZeroRotator,false);
        MechanismShapes.Add(PressVisual);
    }
    if(Definition.bHasArm)
    {
        ArmVisual=Display->Shape(TEXT("Cube"),Display->World(Definition.ArmPivot,52),FVector(90,15,16),TEXT("Brass"),FRotator::ZeroRotator,false);
        MechanismShapes.Add(ArmVisual);
    }
    for(auto& P:Visuals){if(P.Value.Mesh)P.Value.Mesh->DestroyComponent();if(P.Value.Detail)P.Value.Detail->DestroyComponent();}Visuals.Reset();
}
void FExpeditionRuntime::UpdateVisuals(float Delta)
{
    if(!Display)return;
    Display->Time=Elapsed;Display->Magnet=Magnet;
    const auto& Definition=World->GetSiteDefinition();
    if(PressVisual)Display->Place(PressVisual,Display->World(Definition.PressCenter,World->IsPressSafe()?125:55),
        FVector(Definition.PressHalfSize.X*2,Definition.PressHalfSize.Y*2,18));
    if(ArmVisual)
    {
        const FVector2D Pivot=Definition.ArmPivot,Tip=World->GetArmTip(),Direction=Tip-Pivot;
        Display->Place(ArmVisual,Display->World((Pivot+Tip)*.5,52),FVector(Direction.Size(),15,16),
            FRotator(0,FMath::RadiansToDegrees(FMath::Atan2(Direction.Y,Direction.X)),0));
    }
    const float Shake=World->IsUnsafe() && !bPaused?2.f:0.f;
    const FVector MagnetPosition=Display->World(Magnet,78)+FVector(FMath::Sin(Elapsed*61)*Shake,FMath::Cos(Elapsed*49)*Shake,Impact*5);
    for(int32 I=0;I<Display->MagnetShapes.Num();++I)
    {
        Display->MagnetShapes[I]->SetVisibility(Screen==EExpeditionScreen::Site);
        Display->Place(Display->MagnetShapes[I],MagnetPosition+Display->MagnetOffsets[I],Display->MagnetSizes[I]*(1+Impact*.025f));
    }
    for(const auto& B:World->GetBodies())
    {
        auto* Existing=Visuals.Find(B.Id);
        if(!Existing)
        {
            FExpeditionVisual V;V.Spin=B.Id*43.f;V.Previous=Display->World(B.Position,B.State==EExpeditionBodyState::Cargo?47.f:B.bGoal?35.f:24.f);
            FName Mesh=B.bGoal?FName(TEXT("Cylinder")):B.Material==EExpeditionMaterial::Iron?FName(TEXT("SM_Bolt")):FName(TEXT("SM_Plate"));
            if(B.Role==TEXT("power_frame") || B.bFloorContact)Mesh=TEXT("Cube");
            if(B.Role==TEXT("welded_slug"))Mesh=TEXT("Cylinder");
            V.Mesh=Display->Shape(Mesh,V.Previous,FVector(B.Radius*2,B.Radius*2,B.bGoal?40:18),MaterialOf(B),FRotator::ZeroRotator,false);
            if(B.bGoal)V.Detail=Display->Shape(TEXT("SM_Ring"),V.Previous+FVector(0,0,25),FVector(B.Radius*2.1f,B.Radius*2.1f,10),TEXT("Glow"),FRotator::ZeroRotator,false);
            else if(B.Role==TEXT("power_frame"))V.Detail=Display->Shape(TEXT("Cylinder"),V.Previous+FVector(0,0,27),FVector(B.Radius*1.4f,B.Radius*1.4f,32),TEXT("Brass"),FRotator::ZeroRotator,false);
            Visuals.Add(B.Id,V);Existing=Visuals.Find(B.Id);
        }
        auto& V=*Existing;const bool Visible=IsVisible(B);
        V.Mesh->SetVisibility(Visible);if(V.Detail)V.Detail->SetVisibility(Visible);if(!Visible)continue;
        const bool Cargo=B.State==EExpeditionBodyState::Cargo;
        const bool Frame=B.Role==TEXT("power_frame");
        const float Z=Cargo?47.f:B.bFloorContact?19.f:B.bGoal?35.f:Frame?30.f:24.f;
        const FVector Target=Display->World(B.Position,Z);
        V.Previous=FMath::VInterpTo(V.Previous,Target,Delta,24.f);
        if(Delta>0 && !Cargo && !Frame && !B.bFloorContact)V.Spin+=B.Velocity.Size()*Delta*.15f;
        const float Scale=Cargo?.8f:1.f;
        Display->Place(V.Mesh,V.Previous,FVector(B.Radius*2,B.Radius*2,B.bFloorContact?2:Frame?28:B.bGoal?42:18)*Scale,FRotator(0,Frame||B.bFloorContact?0:V.Spin,0));
        if(auto* M=Display->Materials.Find(MaterialOf(B)))V.Mesh->SetMaterial(0,*M);
        if(V.Detail)Display->Place(V.Detail,V.Previous+FVector(0,0,(Frame?27:25)*Scale),
            (Frame?FVector(B.Radius*1.4f,B.Radius*1.4f,32):FVector(B.Radius*2.1f,B.Radius*2.1f,10))*Scale);
    }
    for(auto& P:Display->Particles){P.Life-=Delta;P.Position+=P.Velocity*Delta;P.Velocity.Z-=260*Delta;}
    Display->Particles.RemoveAll([](const auto& P){return P.Life<=0;});
}
void FExpeditionRuntime::RenderWorld(UCanvas* C)
{
    if(!Display)return;
    const auto& State=World->GetState();
    const auto& Definition=World->GetSiteDefinition();
    const auto Objectives=World->GetObjectives();
    if(Definition.bHasPress)
    {
    const FVector2D Press=Definition.PressCenter,Half=Definition.PressHalfSize;
    const TArray<FVector2D> Corners={Press-Half,Press+FVector2D(Half.X,-Half.Y),Press+Half,Press+FVector2D(-Half.X,Half.Y)};
    for(int32 I=0;I<4;++I)Display->Line(C,Display->Project(Display->World(Corners[I],27)),Display->Project(Display->World(Corners[(I+1)%4],27)),World->IsPressSafe()?Copper:Red,3);
    const FVector2D PressLabel=Display->Project(Display->World(Press+FVector2D(0,-Half.Y-25),38));
    const float Phase=FMath::Fmod(State.WorldTime,5.5f);
    const FString PressHint=World->IsPressSafe()?FString::Printf(TEXT("PRESS CLOSES %.1fs"),3.4f-Phase):FString::Printf(TEXT("PRESS OPENS %.1fs"),5.5f-Phase);
    Text(C,PressHint,(PressLabel.X-Display->UX)/Display->UIScale-62,(PressLabel.Y-Display->UY)/Display->UIScale,.77f,World->IsPressSafe()?Copper:Red);
    }
    for(const auto& B:World->GetBodies())if(IsVisible(B))
    {
        const bool Installed=B.Role==TEXT("power_frame") && B.State==EExpeditionBodyState::Banked;
        if(B.Charge>0 && !Installed)
        {
            Display->WorldCircle(C,B.Position,B.Radius+3,Mint,2,20);
            const FVector2D P=Display->Project(Display->World(B.Position,75));
            Text(C,FString::Printf(TEXT("%d CHARGE"),B.Charge),(P.X-Display->UX)/Display->UIScale-25,(P.Y-Display->UY)/Display->UIScale-17,.65f,Mint);
        }
        if(!B.FlowDirection.IsNearlyZero())
        {
            const FVector2D Dir=B.FlowDirection.GetSafeNormal(),Across(-Dir.Y,Dir.X),Tip=B.Position+Dir*28;
            const auto Start=Display->Project(Display->World(B.Position-Dir*23,53)),End=Display->Project(Display->World(Tip,53));
            Display->Line(C,Start,End,Mint,2);
            for(float Side:{-1.f,1.f})Display->Line(C,End,Display->Project(Display->World(Tip-Dir*11+Across*Side*7,53)),Mint,2);
        }
        for(int32 Link:B.Links)if(B.Id<Link)
            if(const auto* Other=World->FindBody(Link))if(IsVisible(*Other))
                Display->Line(C,Display->Project(Display->World(B.Position,40)),Display->Project(Display->World(Other->Position,40)),Copper,3);
        if(B.bGoal || B.bAnchored || B.bHot || B.bFunctional || Installed)
        {
            const FLinearColor BodyColor=B.bHot?Red:B.bGoal?Mint:Copper;
            Display->WorldCircle(C,B.Position,B.Radius+7,BodyColor,2,20);
            const FVector2D P=Display->Project(Display->World(B.Position,65));
            FString BodyLabel=LabelOf(B);
            if(!B.bGoal && B.Appraisal>0 && !Installed)BodyLabel+=FString::Printf(TEXT(" / %d value"),B.Appraisal);
            Display->DrawText(C,BodyLabel,(P.X-Display->UX)/Display->UIScale-45,(P.Y-Display->UY)/Display->UIScale,.72f,BodyColor);
        }
    }
    auto Mark=[&](FVector2D Pos,float Radius,const FString& Label,bool Done)
    {
        Display->WorldCircle(C,Pos,Radius,Done?Mint:Copper,2,25);
        const FVector2D P=Display->Project(Display->World(Pos,48));
        Text(C,Label,(P.X-Display->UX)/Display->UIScale-45,(P.Y-Display->UY)/Display->UIScale,.73f,Done?Mint:Copper);
    };
    for(const auto& Marker:Definition.Markers)
    {
        if(Marker.Id==TEXT("furnace"))continue;
        const bool FrameMarker=Marker.Id.ToString().StartsWith(TEXT("frame_"));
        if(const auto* Payload=World->FindBody(Marker.LinkedBodyId);Payload&&IsVisible(*Payload)&&Payload->bAnchored&&
            (!FrameMarker||Marker.Id==TEXT("frame_support")))
            Display->Line(C,Display->Project(Display->World(Marker.Position,31)),Display->Project(Display->World(Payload->Position,31)),
                Quiet.CopyWithNewOpacity(.6f),2);
        const auto* Objective=Objectives.FindByPredicate([&](const auto& Item){return Item.Id==Marker.Id;});
        const bool Done=Marker.Id==TEXT("frame_receiver")?State.bFrameReceived:Objective?Objective->bSatisfied:Marker.Id==TEXT("receiver")&&World->IsCoreReleased();
        FString Label=Marker.Label;
        if(Marker.Id==TEXT("frame_contact"))Label=World->IsManualTargetExposed(63)?TEXT("HOIST CONTACT / EXPOSED"):TEXT("HOIST CONTACT / UNDER FRAME");
        if(Marker.Id==TEXT("frame_receiver"))Label=State.bFrameReceived?TEXT("FRAME INSTALLED"):TEXT("FRAME DOCK / E: RECOVER");
        Mark(Marker.Position,Marker.Radius,Label,Done);
    }
    if(Definition.LayoutId==TEXT("counterweight_exchange"))
    {
        const auto* Dock=World->FindMarker(TEXT("frame_receiver"));const auto* Counterbalance=World->FindMarker(TEXT("counterbalance"));
        if(Dock&&Counterbalance)Display->Line(C,Display->Project(Display->World(Dock->Position,23)),
            Display->Project(Display->World(Counterbalance->Position,23)),State.bFrameReceived?Mint:Quiet.CopyWithNewOpacity(.5f),3);
    }
    const FVector2D FP=Display->Project(Display->World(FExpeditionWorld::FurnacePosition(),100));
    Text(C,TEXT("E: SMELT SCRAP"),(FP.X-Display->UX)/Display->UIScale-60,(FP.Y-Display->UY)/Display->UIScale,.85f,Copper);
    if(State.TetherBody!=INDEX_NONE)if(const auto* B=World->FindBody(State.TetherBody))
        Display->Line(C,Display->Project(Display->World(State.TetherAnchor,45)),Display->Project(Display->World(B->Position,35)),Mint,4);
    if(State.SecondTetherBody!=INDEX_NONE)if(const auto* B=World->FindBody(State.SecondTetherBody))
        Display->Line(C,Display->Project(Display->World(State.SecondTetherAnchor,45)),Display->Project(Display->World(B->Position,35)),Copper,3);
    for(const auto& Link:State.Constraints)if(Link.bActive)
    {
        const auto* A=World->FindBody(Link.BodyA);const auto* B=World->FindBody(Link.BodyB);
        if(A && B)
        {
            Display->Line(C,Display->Project(Display->World(A->Position,40)),Display->Project(Display->World(Link.Anchor,58)),Copper,3);
            Display->Line(C,Display->Project(Display->World(Link.Anchor,58)),Display->Project(Display->World(B->Position,40)),Mint,3);
        }
    }
    for(int32 I=1;I<State.GuidePoints.Num();++I)
        Display->Line(C,Display->Project(Display->World(State.GuidePoints[I-1],43)),Display->Project(Display->World(State.GuidePoints[I],43)),
            State.GuideUses>0?Mint:Quiet,4);
    if(State.ShearRemaining>0)
    {
        const FVector2D Tangent(-State.ShearNormal.Y,State.ShearNormal.X);
        Display->Line(C,Display->Project(Display->World(State.ShearOrigin-Tangent*75,38)),Display->Project(Display->World(State.ShearOrigin+Tangent*75,38)),Red,3);
    }
    if(State.GroundBody!=INDEX_NONE)if(const auto* B=World->FindBody(State.GroundBody))
        Display->WorldCircle(C,B->Position,B->Radius+10,FLinearColor(.28f,.65f,1.f),3,24);
    if(State.DeferredSensor!=INDEX_NONE && State.DeferredTerminal!=INDEX_NONE)
    {
        const auto* A=World->FindBody(State.DeferredSensor);const auto* B=World->FindBody(State.DeferredTerminal);
        if(A && B)Display->Line(C,Display->Project(Display->World(A->Position,48)),Display->Project(Display->World(B->Position,48)),Copper,2);
    }
    if(!State.CycloneIds.IsEmpty())Display->WorldCircle(C,State.CycloneCenter,58,Mint,2,32);
    if(RelaySource!=INDEX_NONE)if(const auto* B=World->FindBody(RelaySource))
        Display->Line(C,Display->Project(Display->World(B->Position,38)),Display->Project(Display->World(Aim,38)),Mint,2);
    if(PreparedPartner!=INDEX_NONE)if(const auto* B=World->FindBody(PreparedPartner))
        Display->WorldCircle(C,B->Position,B->Radius+8,Copper,2,20);
    if(PreparationStage>0 && PreparedSlot!=INDEX_NONE)
    {
        const auto Prepared=CommandFor(PreparedSlot);
        if(Prepared.Action==EExpeditionAction::LayGuide || (Prepared.Action==EExpeditionAction::Relay && PreparationStage>1))
            Display->WorldCircle(C,PreparedPoint,22,Copper,2,20);
        if(Prepared.Action==EExpeditionAction::LayGuide && PreparationStage>1)
            Display->Line(C,Display->Project(Display->World(PreparedPoint,36)),Display->Project(Display->World(PreparedBend,36)),Mint,2);
    }
    for(const auto& P:Display->Particles)
    {
        const FVector2D S=Display->Project(P.Position);const float Alpha=P.Life/P.Maximum;
        Display->Line(C,S,S+FVector2D(P.Size*2,0),P.Color.CopyWithNewOpacity(Alpha),P.Size);
    }
    if(Input.ArmedSlot!=INDEX_NONE)
    {
        Display->Line(C,Display->Project(Display->World(Magnet,82)),Display->Project(Display->World(Aim,25)),Mint,2);
        const auto Cmd=CommandFor(Input.ArmedSlot);const auto P=World->Preview(Cmd,*Rig);
        if(Cmd.Action==EExpeditionAction::Arc && P.bAllowed)
            for(int32 I=1;I<P.BodyIds.Num();++I)
            {
                const auto* From=World->FindBody(P.BodyIds[I-1]);const auto* To=World->FindBody(P.BodyIds[I]);
                if(From&&To)Display->Line(C,Display->Project(Display->World(From->Position,48)),
                    Display->Project(Display->World(To->Position,48)),Mint,3);
            }
        for(int32 I=1;I<P.PathPoints.Num();++I)
            Display->Line(C,Display->Project(Display->World(P.PathPoints[I-1],39)),Display->Project(Display->World(P.PathPoints[I],39)),Mint,3);
        const FString Preparation=PreparationHint(Input.ArmedSlot);
        const FLinearColor PreviewColor=!P.bAllowed?Copper:P.bUnsafe?Red:Mint;
        Display->WorldCircle(C,Aim,25,PreviewColor,2,30);
        Text(C,!Preparation.IsEmpty()?Preparation:P.bAllowed?FString::Printf(TEXT("Release: %d battery / %.0f kg / %d value%s"),P.BatteryCost,P.Mass,P.Value,
            P.bUnsafe?TEXT(" / UNSAFE: drop within 3s or spill +12 battery"):TEXT("")):P.Reason,365,695,.8f,PreviewColor);
    }
    else
    {
        Display->WorldCircle(C,Magnet,bPrecision?42:110,Input.bField?Mint:FLinearColor(.20f,.42f,.39f,.55f),Input.bField?2:1,18);
        const auto Preview=World->Preview(FieldCommand(),*Rig);
        if(Preview.bAllowed)
        {
            for(int32 Id:Preview.BodyIds)if(const auto* B=World->FindBody(Id))
                Display->WorldCircle(C,B->Position,B->Radius+4,Preview.bUnsafe?Red:Mint,1,16);
            Text(C,FString::Printf(TEXT("LMB: %d battery / +%.0f kg = %.0f kg held / %d value%s"),Preview.BatteryCost,Preview.Mass,
                World->GetCargoMass()+Preview.Mass,Preview.Value,Preview.bUnsafe?TEXT(" / UNSAFE: 3s to drop"):TEXT("")),365,664,.79f,Preview.bUnsafe?Red:Mint);
        }
        const int32 Target=World->FindBodyAt(Aim,22);
        if(const auto* B=World->FindBody(Target))
        {
            FString Detail=LabelOf(*B);
            if(!B->bGoal && B->Appraisal>0)Detail+=FString::Printf(TEXT("  / %d refining value"),B->Appraisal);
            if(B->Links.Num())Detail+=TEXT("  welded: pulling takes the connected bundle");
            Text(C,Detail,365,695,.9f,B->bHot?Red:Paper);
        }
    }
}
void FExpeditionRuntime::RenderDepot(UCanvas* C)
{
    Display->Rect(C,230,150,1150,585,Panel);
    Text(C,SiteIndex==0?TEXT("OUTFIT YOUR FIRST RIG"):TEXT("WORKSHOP — CHOOSE YOUR NEXT CAPABILITY"),260,174,1,Paper,true);
    Text(C,FString::Printf(TEXT("%d credits    Tools %d/2    Passive sockets %d/4"),Rig->GetCash(),Rig->GetUsedSlots(EExpeditionModuleKind::Active),Rig->GetUsedSlots(EExpeditionModuleKind::Passive)),260,213,1,Mint);
    const auto& Catalog=FExpeditionRig::Catalog();int32 I=0;
    for(FName Offer:Rig->GetOffers())
    {
        const auto* M=FExpeditionRig::FindModule(Offer);if(!M)continue;
        const int32 Index=Catalog.IndexOfByPredicate([&](const auto& V){return V.Id==Offer;});
        const float X=260+(I%2)*535,Y=255+(I/2)*110;++I;
        AddButton(C,1000+Index,FString::Printf(TEXT("%s   %d credits   %d socket%s"),*M->Name,M->Price,M->Slots,M->Slots==1?TEXT(""):TEXT("s")),X,Y,505,36);
        Wrap(C,M->Description,X+8,Y+44,65,.79f);
    }
    if(!Rig->HasVerifiedShopPair())Wrap(C,Rig->GetShopNotice(),260,490,125,.85f,Copper);
    float X=260,Y=510;Text(C,TEXT("OWNED — click to inspect, fit or sell"),X,Y,.85f,Quiet);Y+=28;
    for(const auto& Owned:Rig->GetInventory())
    {
        const int32 Index=Catalog.IndexOfByPredicate([&](const auto& V){return V.Id==Owned.Id;});
        const bool Fitted=Rig->GetFittedActives().Contains(Owned.Id)||Rig->GetFittedPassives().Contains(Owned.Id);
        const FString Label=(Fitted?(Rig->Has(Owned.Id)?TEXT("[FITTED] "):TEXT("[INACTIVE] ")):TEXT(""))+NameOf(Owned.Id);
        AddButton(C,1000+Index,Label,X,Y,245,32);X+=263;if(X>1200){X=260;Y+=38;}
    }
    FString Reason;
    const bool CanDepart=Rig->CanDepart(Reason);
    AddButton(C,10,TEXT("DEPART — recover the next machine"),960,684,380,36,CanDepart);
    if(!CanDepart)Wrap(C,Reason,700,646,76,.78f,Copper);
    AddButton(C,11,TEXT("Precharge next site +20 battery / 4 cr"),260,684,410,36,!Rig->IsPrecharged()&&Rig->GetCash()>=4);
    if(Catalog.IsValidIndex(SelectedModule))
    {
        const auto& M=Catalog[SelectedModule];Display->Rect(C,230,741,1150,112,Panel);
        Text(C,M.Name,254,751,.98f,Paper);Wrap(C,M.Description,254,780,89,.82f);
        FString BuyReason,FitReason;const bool CanBuy=Rig->CanBuy(M.Id,BuyReason),CanFit=Rig->CanFit(M.Id,FitReason);
        AddButton(C,12,TEXT("BUY"),1020,748,150,32,CanBuy);
        AddButton(C,13,TEXT("FIT"),1185,748,160,32,CanFit);
        AddButton(C,14,TEXT("UNFIT"),1020,792,150,32,Rig->GetFittedActives().Contains(M.Id)||Rig->GetFittedPassives().Contains(M.Id));
        AddButton(C,15,FString::Printf(TEXT("SELL / %d cr"),Rig->GetResaleValue(M.Id)),1185,792,160,32,Rig->Owns(M.Id));
        const bool Fitted=Rig->GetFittedActives().Contains(M.Id)||Rig->GetFittedPassives().Contains(M.Id);
        const FString Block=Rig->Owns(M.Id)?(!Fitted&&!CanFit?FitReason:!Rig->Has(M.Id)&&Fitted?TEXT("Inactive: fit the active tool required by this support."):TEXT("")):!CanBuy?BuyReason:TEXT("");
        if(!Block.IsEmpty())Wrap(C,Block,254,836,126,.71f,Copper);
    }
}
void FExpeditionRuntime::Paint(UCanvas* C)
{
    if(!C || !Display)return;
    Display->Width=C->ClipX;Display->Height=C->ClipY;
    Display->UIScale=FMath::Min(C->ClipX/1600.f,C->ClipY/900.f);
    Display->UX=(C->ClipX-1600*Display->UIScale)*.5f;Display->UY=(C->ClipY-900*Display->UIScale)*.5f;
    Buttons.Reset();Display->Rect(C,0,0,1600,102,Panel);
    Text(C,TEXT("MAGNET SWEEP  /  EXPEDITION LAB"),26,19,1,Paper,true);
    if(Screen==EExpeditionScreen::Site || Screen==EExpeditionScreen::Depot)
        Wrap(C,World->GetSiteDefinition().Name+TEXT(" — ")+World->GetSiteDefinition().Summary,28,57,147,.79f,Quiet);
    else Text(C,TEXT("Recover machinery. Earn new capabilities. Bring the final core home."),28,60,.92f,Quiet);
    Text(C,FString::Printf(TEXT("SITE %d / 4       %d CREDITS"),SiteIndex+1,Rig->GetCash()),1170,32,1,Mint);
    if(Screen==EExpeditionScreen::Starter)
    {
        Display->Rect(C,285,155,1030,588,Panel);
        Text(C,TEXT("CHOOSE YOUR STARTING TOOL"),322,185,1,Paper,true);
        Wrap(C,TEXT("Run equipment and credits start fresh. Discoveries and archived winning rigs stay. Your previous career is separate."),322,226,110,.95f);
        const TArray<FName> Starters={TEXT("extraction_coil"),TEXT("rail_impeller"),TEXT("arc_driver"),TEXT("anchor_winch"),TEXT("vector_emitter"),TEXT("relay_projector")};
        for(int32 I=0;I<Starters.Num();++I)
        {
            const auto* M=FExpeditionRig::FindModule(Starters[I]);if(!M)continue;
            const float X=322+(I%2)*486,Y=303+(I/2)*125;
            const bool Enabled=Rig->IsStarterUnlocked(M->Id)&&FExpeditionWorld::IsModuleImplemented(M->Id);
            AddButton(C,100+I,M->Name+(Enabled?TEXT(" / FREE STARTER"):TEXT(" / DISCOVER TO UNLOCK")),X,Y,455,38,Enabled);
            Wrap(C,M->Description,X+6,Y+48,54,.85f);
        }
    }
    else if(Screen==EExpeditionScreen::Depot)RenderDepot(C);
    else if(Screen==EExpeditionScreen::Site)
    {
        RenderWorld(C);Display->Rect(C,18,128,307,455,Panel);
        Text(C,TEXT("YOUR RECOVERY"),37,149,.94f,Mint,true);
        Wrap(C,World->GetGoalText(),37,193,32,.9f,Paper);
        const auto& S=World->GetState();
        bool NextShown=false;int32 Step=0;
        for(const auto& Objective:World->GetObjectives())if(Objective.bRequired)
        {
            const bool Next=!Objective.bSatisfied&&!NextShown;NextShown|=Next;
            Text(C,(Objective.bSatisfied?TEXT("DONE  "):Next?TEXT("NEXT  "):TEXT("      "))+Objective.Label,
                37,290+Step*25,.75f,Objective.bSatisfied?Mint:Next?Paper:Quiet);++Step;
        }
        Text(C,FString::Printf(TEXT("BATTERY   %d"),World->GetBattery()),37,402,1,Mint);
        Text(C,FString::Printf(TEXT("CARGO   %.0f / 24 kg"),World->GetCargoMass()),37,435,1,World->IsUnsafe()?Red:Paper);
        if(World->IsUnsafe())Text(C,FString::Printf(TEXT("DROP NOW — %.1f s / lose 12 energy"),FMath::Max(0.f,3-S.UnsafeElapsed)),37,468,.77f,Red);
        else Text(C,TEXT("RMB drops. Field-off keeps cargo."),37,469,.76f,Quiet);
        const int32 Out=World->GetOutput();int32 Next=0;
        for(int32 M=0;M<2;++M){const int32 T=FExpeditionRig::RefiningThreshold(SiteIndex,M);if(Out<T){Next=T;break;}}
        Text(C,Next>0?FString::Printf(TEXT("REFINE %d / %d  = +2 credits"),Out,Next):SiteIndex==3?TEXT("Final recovery archives your winning rig."):TEXT("Refining bonuses earned; recover the core."),37,518,.73f,Copper);
        Text(C,SiteIndex<3?FString::Printf(TEXT("DELIVER CORE  = +%d credits"),10+SiteIndex*2):TEXT("DELIVER CORE  = expedition complete"),37,548,.73f,Mint);
        Display->Rect(C,18,594,307,139,Panel);
        Wrap(C,RecoveryHint(),33,608,36,.78f,Paper);
        if(const auto* Frame=World->FindFrameBody();Frame && !bPaused)
        {
            Display->Rect(C,990,178,326,167,Panel);
            Text(C,S.bFrameReceived?TEXT("POWER FRAME INSTALLED"):TEXT("OPTIONAL POWER FRAME / 40 kg"),1004,191,.78f,S.bFrameReceived?Mint:Copper);
            FString Reason;const auto* Dock=World->FindMarker(TEXT("frame_receiver"));
            const bool Ready=World->CanReceiveFrame(Reason);
            const bool Near=Dock && FVector2D::Distance(Magnet,Dock->Position)<=95;
            Wrap(C,S.bFrameReceived?(SiteIndex==3?TEXT("Output recorded. The installed frame supplies the final counterbalance. Recover the core."):
                TEXT("Output recorded. Continue the core recovery.")):Ready?TEXT("Frame and receiving support ready. Press E to install and collect its appraisal."):Reason,
                1004,217,39,.73f,Paper);
            AddButton(C,50,S.bFrameReceived?TEXT("INSTALLED"):FString::Printf(TEXT("E: RECOVER FRAME / +%d OUTPUT"),Frame->Appraisal),1004,304,298,31,Ready&&Near);
        }
        if(Rig->Has(TEXT("rail_impeller")) && !bPaused)
        {
            Display->Rect(C,1330,182,252,550,Panel);
            Text(C,TEXT("RAIL PREPARATION"),1344,197,.82f,Mint);
            AddButton(C,40,bPrepareWeld?TEXT("LAUNCH"):TEXT("[LAUNCH]"),1344,230,108,32);
            AddButton(C,41,bPrepareWeld?TEXT("[WELD]"):TEXT("WELD"),1462,230,106,32,Rig->Has(TEXT("slug_press")));
            Wrap(C,bPrepareWeld?TEXT("Check 2+ iron pieces. Only checked pieces are welded. Other cargo stays."):
                 TEXT("Choose one body. C cycles. Aim + release the Rail tool to fire."),1344,274,28,.75f);
            float CY=350;int32 Count=0;
            for(const auto& B:World->GetBodies())if((B.State==EExpeditionBodyState::Cargo || S.CycloneIds.Contains(B.Id)) && !B.bGoal)
            {
                if(++Count>18)break;
                const bool Chosen=bPrepareWeld?WeldSelection.Contains(B.Id):SelectedCargoId==B.Id;
                const bool Allowed=!bPrepareWeld||(B.State==EExpeditionBodyState::Cargo&&B.Material==EExpeditionMaterial::Iron&&!B.bHot);
                AddButton(C,2000+B.Id,FString::Printf(TEXT("%s #%d  %s  %.0f kg"),Chosen?TEXT("[x]"):TEXT("[ ]"),B.Id,
                    B.Material==EExpeditionMaterial::Iron?TEXT("iron"):B.bHot?TEXT("HOT"):TEXT("metal"),B.Mass),1344,CY,224,19,Allowed);
                CY+=21;
            }
            if(Count==0)Wrap(C,TEXT("Collect scrap to load the tool. Shift narrows ordinary attraction."),1344,350,28,.78f,Quiet);
        }
        Display->Rect(C,18,745,1564,137,Panel);
        const auto& Active=Rig->GetFittedActives();
        for(int32 I=0;I<2;++I)
        {
            const float X=42+I*510;const FString Name=Active.IsValidIndex(I)?NameOf(Active[I]):TEXT("Empty tool socket");
            Text(C,FString::Printf(TEXT("HOLD %s TO AIM / RELEASE TO USE"),I==0?TEXT("Q"):TEXT("F")),X,762,.74f,Quiet);
            Text(C,Name,X,791,1.05f,Mint);
            if(Active.IsValidIndex(I))
            {
                const FName Tool=Active[I];
                TArray<FString> Modes;TArray<bool> Enabled;
                if(Tool==TEXT("arc_driver")){Modes={TEXT("PULSE"),TEXT("GROUND"),TEXT("ARM RELAY")};Enabled={true,Rig->Has(TEXT("ground_clip")),Rig->Has(TEXT("escapement_relay"))};}
                else if(Tool==TEXT("anchor_winch") && Rig->Has(TEXT("twin_anchor"))){Modes={TEXT("TOW"),TEXT("SWITCH")};Enabled={true,true};}
                else if(Tool==TEXT("vector_emitter")){Modes={TEXT("PUSH"),TEXT("HEAT SINK"),TEXT(""),TEXT("REACTION")};Enabled={true,Rig->Has(TEXT("heat_sink_mould")),false,Rig->Has(TEXT("reaction_frame"))};}
                else if(Tool==TEXT("relay_projector")){Modes={TEXT("TRANSFER"),TEXT("HEAT SINK"),TEXT("LAY GUIDE"),TEXT("SPLIT")};Enabled={true,Rig->Has(TEXT("heat_sink_mould")),Rig->Has(TEXT("field_loom")),Rig->Has(TEXT("flow_splitter"))};}
                if(Modes.Num()>1)
                {
                    int32 VisibleMode=0;for(int32 Op=0;Op<Modes.Num();++Op)if(Enabled[Op])
                        AddButton(C,60+I*4+Op,(ToolOperation[I]==Op?TEXT("> "):TEXT(""))+Modes[Op],X+(VisibleMode++)*119,822,113,26,!bPaused);
                    Text(C,TEXT("Choose operation, then aim with your tool key."),X,858,.68f,Quiet);
                }
                else if(const auto* M=FExpeditionRig::FindModule(Tool))Wrap(C,M->Description,X,822,62,.76f);
            }
        }
        Text(C,TEXT("LMB / SPACE attract   SHIFT precise"),1090,765,.75f,Paper);
        Text(C,TEXT("RMB  drop whole haul"),1090,798,.88f,Paper);
        Text(C,TEXT("E  interact / bank / deliver"),1090,830,.88f,Paper);
        Text(C,TEXT("ESC  planning pause     M  sound"),1090,860,.72f,Quiet);
        if(bPaused)
        {
            Display->Rect(C,442,236,716,380,Panel);Text(C,TEXT("PLANNING PAUSE — ALL DANGER FROZEN"),475,264,.96f,Paper,true);
            Wrap(C,TEXT("Inspect your plan. Resume keeps the same energy, material and hazard timing. Retrying restores this entire site's entry and rolls back its rewards."),476,312,74,.95f);
            AddButton(C,20,TEXT("RESUME"),477,409,308,43);
            AddButton(C,21,TEXT("RETRY SITE / roll back this site"),802,409,308,43,SiteEntry.IsValid());
            AddButton(C,22,TEXT("RETREAT / end this expedition"),477,470,308,43);
            AddButton(C,23,bContinuousField?TEXT("FIELD: continuous / compare"):TEXT("FIELD: deliberate / compare"),802,470,308,43);
            AddButton(C,31,TEXT("SAVE AND QUIT"),477,536,633,43);
        }
    }
    else
    {
        Display->Rect(C,320,250,960,405,Panel);
        Text(C,Screen==EExpeditionScreen::Victory?TEXT("CORE RECOVERED"):TEXT("EXPEDITION ENDED"),365,290,1.5f,Mint,true);
        Wrap(C,Screen==EExpeditionScreen::Victory?TEXT("You built this rig and brought the machine home. Your winning loadout is archived. Which capability would you choose differently next time?"):
             TEXT("Dispatched discoveries remain. Start another expedition with a different tool, or return later."),365,352,96,1,Paper);
        int32 TotalOutput=0;for(int32 I=0;I<4;++I)TotalOutput+=Rig->GetOutput(I);
        Text(C,FString::Printf(TEXT("Refined output: %d    Completed expeditions: %d"),TotalOutput,Rig->GetRecords().RunsWon),365,434,.9f,Copper);
        if(Screen==EExpeditionScreen::Victory)
        {
            FString Build=TEXT("Your recovered rig: ");
            for(FName Id:Rig->GetRecords().LastWinningRig){if(!Build.EndsWith(TEXT(": ")))Build+=TEXT(" + ");Build+=NameOf(Id);}
            Wrap(C,Build,365,477,102,.91f,Mint);
        }
        AddButton(C,30,bConfirmNew?TEXT("CONFIRM NEW EXPEDITION"):TEXT("TRY A DIFFERENT RIG"),365,580,425,48);
        AddButton(C,31,TEXT("SAVE AND QUIT"),816,580,420,48);
    }
    if(NoticeLife>0)
    {
        Display->Rect(C,340,106,1218,60,FLinearColor(.025f,.065f,.064f,.96f));
        Wrap(C,Notice,359,119,142,.88f,Paper);
    }
}

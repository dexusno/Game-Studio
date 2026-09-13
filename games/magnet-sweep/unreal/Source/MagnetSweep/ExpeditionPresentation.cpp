#include "ExpeditionRuntime.h"
#include "ExpeditionRig.h"
#include "ExpeditionWorld.h"
#include "WorkbenchRuntime.h"
#include "ExpeditionUIVisuals.h"
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
const FLinearColor Paper(.94f,.91f,.80f), Quiet(.60f,.68f,.69f), Mint(.29f,.85f,.76f);
const FLinearColor Copper(.91f,.65f,.31f), Red(1.f,.29f,.20f), Panel(.025f,.046f,.060f,.985f);
FString NameOf(FName Id){const auto* M=FExpeditionRig::FindModule(Id);return M?M->Name:TEXT("Empty socket");}
bool FindCompanionPlan(const FExpeditionRig& Rig,FName Support,FName& Tool,FExpeditionPairPlan& Plan)
{
    if(Rig.IsCompatible(Support) || Rig.Owns(Support))return false;
    TArray<FName> Candidates=Rig.GetOffers();
    for(const auto& Owned:Rig.GetInventory())Candidates.AddUnique(Owned.Id);
    for(FName Id:Candidates)
    {
        auto Candidate=Rig.PreviewPair(Id,Support);
        if(Candidate.bPossible){Tool=Id;Plan=MoveTemp(Candidate);return true;}
    }
    return false;
}
FString ModuleNames(const TArray<FName>& Ids)
{
    FString Names;
    for(FName Id:Ids){if(!Names.IsEmpty())Names+=TEXT(", ");Names+=NameOf(Id);}
    return Names;
}
FString RequiredTools(const FExpeditionModule& Support)
{
    auto NamesFor=[](const TArray<FName>& Tags,const TCHAR* Separator)
    {
        FString Names;
        for(FName Tag:Tags)for(const auto& Tool:FExpeditionRig::Catalog())
            if(Tool.Kind==EExpeditionModuleKind::Active && Tool.Provides.Contains(Tag))
            {if(!Names.IsEmpty())Names+=Separator;Names+=Tool.Name;}
        return Names;
    };
    FString Result=NamesFor(Support.RequiresAll,TEXT(" + "));
    const FString Any=NamesFor(Support.RequiresAny,TEXT(" or "));
    if(!Any.IsEmpty()){if(!Result.IsEmpty())Result+=TEXT(" + ");Result+=Any;}
    return Result;
}
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
{ExpeditionUI::Text(Display.Get(),C,S,X,Y,Scale,Color,Large);}
void FExpeditionRuntime::Wrap(UCanvas* C,const FString& S,float X,float Y,int32 Characters,float Scale,FLinearColor Color)
{
    ExpeditionUI::Paragraph(Display.Get(),C,S,X,Y,Characters*7.5f*Scale,Scale,Color);
}
void FExpeditionRuntime::AddButton(UCanvas* C,int32 Id,const FString& Label,float X,float Y,float W,float H,bool Enabled)
{
    if(!Display)return;
    const FBox2D Box(FVector2D(Display->UX+X*Display->UIScale,Display->UY+Y*Display->UIScale),
                     FVector2D(Display->UX+(X+W)*Display->UIScale,Display->UY+(Y+H)*Display->UIScale));
    const bool Hover=Box.IsInside(Pointer);
    const bool Selected=(Id>=1000&&Id==1000+SelectedModule);
    const bool Primary=Id==10||Id==12||Id==20||Id==30||Id==50;
    const FLinearColor Accent=Primary?Copper:Mint;
    Display->Rect(C,X+2,Y+3,W,H,FLinearColor(0,0,0,.27f));
    Display->Rect(C,X,Y,W,H,Enabled?(Hover||Selected?Accent:ExpeditionUI::Edge):FLinearColor(.08f,.12f,.14f));
    Display->Rect(C,X+1,Y+1,W-2,H-2,Enabled?(Hover?FLinearColor(.11f,.20f,.22f):Selected?FLinearColor(.065f,.14f,.15f):FLinearColor(.045f,.078f,.093f)):FLinearColor(.035f,.047f,.055f));
    Display->Rect(C,X+1,Y+1,W-2,2,Enabled?Accent.CopyWithNewOpacity(Hover?.9f:.45f):ExpeditionUI::Edge);
    const bool Compact=H<30;
    const float Scale=Compact?.64f:.81f;
    ExpeditionUI::FitText(Display.Get(),C,Label,X+12,Y+(H-(Compact?16:21))*.5f-1,W-30,Scale,Enabled?Paper:Quiet.CopyWithNewOpacity(.72f),true);
    if(Primary&&Enabled)Display->Rect(C,X+W-9,Y+H*.5f-3,3,6,Accent);
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
    {MechanismShapes.Add(Display->Shape(TEXT("SM_VBarrier"),Display->World(P,18),Size,Mat,FRotator::ZeroRotator,false));};
    const auto& Definition=World->GetSiteDefinition();
    for(const auto& Marker:Definition.Markers)
    {
        if(Marker.Id==TEXT("furnace"))continue; // Existing furnace mesh remains the shared receiver for scrap.
        if(Marker.Id==TEXT("receiver"))MechanismShapes.Add(Display->Shape(TEXT("SM_VDock"),Display->World(Marker.Position,15),FVector(Marker.Radius*2,Marker.Radius*2,18),TEXT("Dark"),FRotator::ZeroRotator,false));
        else MechanismShapes.Add(Display->Shape(TEXT("SM_VPad"),Display->World(Marker.Position,19),FVector(Marker.Radius*2,Marker.Radius*2,10),TEXT("Dark"),FRotator::ZeroRotator,false));
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
        PressVisual=Display->Shape(TEXT("SM_VPress"),Display->World(Definition.PressCenter,120),
            FVector(Definition.PressHalfSize.X*2,Definition.PressHalfSize.Y*2,18),TEXT("Hazard"),FRotator::ZeroRotator,false);
        MechanismShapes.Add(PressVisual);
    }
    if(Definition.bHasArm)
    {
        ArmVisual=Display->Shape(TEXT("SM_VArm"),Display->World(Definition.ArmPivot,52),FVector(90,15,16),TEXT("Brass"),FRotator::ZeroRotator,false);
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
            FName Mesh=B.bGoal?FName(TEXT("SM_VCore")):B.Material==EExpeditionMaterial::Iron?(B.Id%2?FName(TEXT("SM_VGear")):FName(TEXT("SM_VBearing"))):B.Material==EExpeditionMaterial::Copper?FName(TEXT("SM_VCoil")):FName(TEXT("SM_VHeatSink"));
            if(B.Role==TEXT("power_frame"))Mesh=TEXT("SM_VFrame");
            if(B.bFloorContact)Mesh=TEXT("Cube");
            if(B.Role==TEXT("welded_slug"))Mesh=TEXT("Cylinder");
            V.Mesh=Display->Shape(Mesh,V.Previous,FVector(B.Radius*2,B.Radius*2,B.bGoal?40:18),MaterialOf(B),FRotator::ZeroRotator,false);
            if(B.bGoal)V.Detail=Display->Shape(TEXT("SM_Ring"),V.Previous+FVector(0,0,25),FVector(B.Radius*2.1f,B.Radius*2.1f,10),TEXT("Glow"),FRotator::ZeroRotator,false);
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
    const auto* NextObjective=Objectives.FindByPredicate([](const auto& Item){return !Item.bSatisfied;});
    TArray<FBox2D> LabelBounds;
    FName InspectedFitting;float FittingDistance=70;
    for(const auto& Marker:Definition.Markers)if(Marker.Id.ToString().StartsWith(TEXT("frame_"))&&Marker.Id!=TEXT("frame_receiver"))
    {
        const float Distance=FVector2D::Distance(Aim,Marker.Position);
        if(Distance<FittingDistance){FittingDistance=Distance;InspectedFitting=Marker.Id;}
    }
    auto Plate=[&](FVector Position,const FString& Label,float Scale,FLinearColor Color)
    {
        const FVector2D P=Display->Project(Position);
        const float W=ExpeditionUI::TextWidth(Display.Get(),Label,Scale),X=(P.X-Display->UX)/Display->UIScale-W*.5f,OriginalY=(P.Y-Display->UY)/Display->UIScale;
        float Y=OriginalY;
        for(float Offset:{0.f,20.f,-20.f,40.f,-40.f,60.f,-60.f})
        {
            const float CandidateY=OriginalY+Offset;
            if(CandidateY<155||CandidateY+22*Scale+4>716)continue;
            const FBox2D Candidate(FVector2D(X-7,CandidateY-4),FVector2D(X+W+7,CandidateY+22*Scale+4));
            if(!LabelBounds.ContainsByPredicate([&](const auto& Existing){return Existing.Intersect(Candidate);}))
            {Y=CandidateY;break;}
        }
        LabelBounds.Add(FBox2D(FVector2D(X-7,Y-4),FVector2D(X+W+7,Y+22*Scale+4)));
        if(!FMath::IsNearlyEqual(Y,OriginalY))Display->Line(C,P,FVector2D(P.X,Display->UY+(Y+6)*Display->UIScale),Color.CopyWithNewOpacity(.4f),1);
        Display->Rect(C,X-5,Y-2,W+10,22*Scale+4,FLinearColor(.005f,.012f,.016f,.92f));
        Text(C,Label,X,Y,Scale,Color);
    };
    if(Definition.bHasPress)
    {
    const FVector2D Press=Definition.PressCenter,Half=Definition.PressHalfSize;
    const TArray<FVector2D> Corners={Press-Half,Press+FVector2D(Half.X,-Half.Y),Press+Half,Press+FVector2D(-Half.X,Half.Y)};
    for(int32 I=0;I<4;++I)Display->Line(C,Display->Project(Display->World(Corners[I],27)),Display->Project(Display->World(Corners[(I+1)%4],27)),World->IsPressSafe()?Copper:Red,3);
    const float Phase=FMath::Fmod(State.WorldTime,5.5f);
    const FString PressHint=World->IsPressSafe()?FString::Printf(TEXT("PRESS CLOSES %.1fs"),3.4f-Phase):FString::Printf(TEXT("PRESS OPENS %.1fs"),5.5f-Phase);
    Plate(Display->World(Press+FVector2D(0,Half.Y+35),38),PressHint,.70f,World->IsPressSafe()?Copper:Red);
    }
    for(const auto& B:World->GetBodies())if(IsVisible(B))
    {
        const bool Installed=B.Role==TEXT("power_frame") && B.State==EExpeditionBodyState::Banked;
        if(B.Charge>0 && !Installed)
        {
            Display->WorldCircle(C,B.Position,B.Radius+3,Mint,2,20);
            Plate(Display->World(B.Position+FVector2D(0,-B.Radius-13),65),FString::Printf(TEXT("%d CHARGE"),B.Charge),.60f,Mint);
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
        const bool Inspecting=FVector2D::Distance(Aim,B.Position)<=B.Radius+26;
        if(B.bGoal || B.bHot || Installed || (Inspecting&&(B.bAnchored||B.bFunctional)))
        {
            const FLinearColor BodyColor=B.bHot?Red:B.bGoal?Mint:Copper;
            Display->WorldCircle(C,B.Position,B.Radius+7,BodyColor,2,20);
            FString BodyLabel=LabelOf(B);
            if(!B.bGoal && B.Appraisal>0 && !Installed)BodyLabel+=FString::Printf(TEXT(" / %d value"),B.Appraisal);
            Plate(Display->World(B.Position+FVector2D(0,-B.Radius-13),70),BodyLabel,.67f,BodyColor);
        }
    }
    auto Mark=[&](FVector2D Pos,float Radius,const FString& Label,bool Done,bool Focus,bool Above,bool ShowLabel)
    {
        const FLinearColor Accent=Done?Mint:Focus?Copper:Quiet;
        Display->WorldCircle(C,Pos,Radius,Accent.CopyWithNewOpacity(Focus?.75f:.28f),Focus?2:1,32);
        if(ShowLabel)Plate(Display->World(Pos+FVector2D(0,(Radius+17)*(Above?-1:1)),28),Label,.65f,Accent);
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
        const bool Focus=(NextObjective&&NextObjective->Id==Marker.Id)||Marker.Id==InspectedFitting||(!FrameMarker&&FVector2D::Distance(Aim,Marker.Position)<Marker.Radius);
        Mark(Marker.Position,Marker.Radius,Label,Done,Focus,Marker.Id==TEXT("collar")||Marker.Id==TEXT("receiver"),!FrameMarker||Marker.Id==TEXT("frame_receiver")||Marker.Id==InspectedFitting);
    }
    if(Definition.LayoutId==TEXT("counterweight_exchange"))
    {
        const auto* Dock=World->FindMarker(TEXT("frame_receiver"));const auto* Counterbalance=World->FindMarker(TEXT("counterbalance"));
        if(Dock&&Counterbalance)Display->Line(C,Display->Project(Display->World(Dock->Position,23)),
            Display->Project(Display->World(Counterbalance->Position,23)),State.bFrameReceived?Mint:Quiet.CopyWithNewOpacity(.5f),3);
    }
    const FVector2D FP=Display->Project(Display->World(FExpeditionWorld::FurnacePosition()+FVector2D(0,130),35));
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
    using namespace ExpeditionUI;
    Text(C,TEXT("THE OUTFITTER"),48,125,1.35f,Paper,true);
    Text(C,SiteIndex==0?TEXT("Choose what your rig can do."):TEXT("Refit for the next recovery."),310,140,.90f,Quiet);
    Text(C,TEXT("01 / AVAILABLE EQUIPMENT"),54,184,.78f,Copper);
    const auto& Catalog=FExpeditionRig::Catalog();int32 OfferNumber=0;
    for(FName Offer:Rig->GetOffers())
    {
        const auto* M=FExpeditionRig::FindModule(Offer);if(!M)continue;
        const int32 Index=Catalog.IndexOfByPredicate([&](const auto& V){return V.Id==Offer;});
        const float X=44+(OfferNumber%2)*517,Y=213+(OfferNumber/2)*174;++OfferNumber;
        const FLinearColor Accent=ModuleColor(M->Id);
        ExpeditionUI::Panel(Display.Get(),C,X,Y,499,159,SelectedModule==Index?Accent:Edge);
        Display->Rect(C,X+14,Y+16,65,67,Ink);
        Icon(Display.Get(),C,M->Id,X+22,Y+23,49,Accent);
        AddButton(C,1000+Index,M->Name,X+91,Y+15,315,34);
        Text(C,FString::Printf(TEXT("%d"),M->Price),X+424,Y+14,1.30f,Copper,true);
        Text(C,TEXT("CR"),X+469,Y+24,.65f,Quiet);
        Text(C,M->Kind==EExpeditionModuleKind::Active?TEXT("ACTIVE TOOL"):M->Price>=24?TEXT("KEYSTONE SUPPORT"):TEXT("RIG SUPPORT"),X+94,Y+58,.68f,Accent);
        for(int32 Slot=0;Slot<M->Slots;++Slot)Socket(Display.Get(),C,X+451-Slot*17,Y+61,true,Accent);
        FString Detail;
        FName PairTool;FExpeditionPairPlan Pair;
        if(FindCompanionPlan(*Rig,M->Id,PairTool,Pair))
            Detail=FString::Printf(TEXT("PAIR / %s + support: %d cr"),*NameOf(PairTool),Pair.Cost);
        else if(!Rig->IsCompatible(M->Id))Detail=TEXT("REQUIRES / ")+RequiredTools(*M);
        if(!Detail.IsEmpty())FitText(Display.Get(),C,Detail,X+16,Y+91,464,.77f,Copper);
        Paragraph(Display.Get(),C,M->Description,X+16,Y+(Detail.IsEmpty()?92:113),466,Detail.IsEmpty()?.84f:.74f,Paper);
    }
    if(!Rig->HasVerifiedShopPair())Text(C,TEXT("Limited matches: refit, keep your rig, or save your credits."),54,562,.77f,Copper);
    Text(C,TEXT("02 / YOUR EQUIPMENT"),54,585,.78f,Copper);
    Text(C,TEXT("Click to inspect. Lit labels are fitted; inactive supports show a dash."),284,587,.76f,Quiet);
    const int32 InventoryRows=FMath::Max(1,FMath::DivideAndRoundUp(Rig->GetInventory().Num(),4));
    const float InventoryHeight=FMath::Min(32.f,106.f/InventoryRows);
    int32 OwnedNumber=0;
    for(const auto& Owned:Rig->GetInventory())
    {
        const int32 Index=Catalog.IndexOfByPredicate([&](const auto& V){return V.Id==Owned.Id;});
        const bool Fitted=Rig->GetFittedActives().Contains(Owned.Id)||Rig->GetFittedPassives().Contains(Owned.Id);
        const float X=54+(OwnedNumber%4)*253,Y=615+(OwnedNumber/4)*InventoryHeight;++OwnedNumber;
        const FString Label=(Fitted?(Rig->Has(Owned.Id)?TEXT("+ "):TEXT("- ")):TEXT(""))+NameOf(Owned.Id);
        AddButton(C,1000+Index,Label,X,Y,239,InventoryHeight-3);
        if(Fitted)Display->Rect(C,X,Y+3,3,InventoryHeight-9,Rig->Has(Owned.Id)?ModuleColor(Owned.Id):Quiet);
    }
    ExpeditionUI::Panel(Display.Get(),C,1081,177,477,548,Copper);
    Text(C,TEXT("EQUIPMENT INSPECTOR"),1103,195,.8f,Quiet);
    if(Catalog.IsValidIndex(SelectedModule))
    {
        const auto& M=Catalog[SelectedModule];const FLinearColor Accent=ModuleColor(M.Id);
        Display->Rect(C,1104,236,74,74,Ink);Icon(Display.Get(),C,M.Id,1114,247,53,Accent);
        FitText(Display.Get(),C,M.Name,1196,237,337,1.15f,Paper,true);
        Text(C,FString::Printf(TEXT("%s / %d SOCKET%s"),M.Kind==EExpeditionModuleKind::Active?TEXT("TOOL"):TEXT("SUPPORT"),M.Slots,M.Slots==1?TEXT(""):TEXT("S")),1197,275,.77f,Accent);
        Paragraph(Display.Get(),C,M.Description,1105,328,428,.94f,Paper);
        FString BuyReason,FitReason;const bool CanBuy=Rig->CanBuy(M.Id,BuyReason),CanFit=Rig->CanFit(M.Id,FitReason);
        const auto Purchase=Rig->PreviewPurchase(M.Id);
        AddButton(C,12,CanBuy?(Purchase.bFitsNow?TEXT("BUY + FIT"):TEXT("BUY / STORE")):TEXT("BUY"),1105,424,202,37,CanBuy);
        AddButton(C,13,TEXT("FIT TO RIG"),1320,424,214,37,CanFit);
        AddButton(C,14,TEXT("UNFIT"),1105,472,202,33,Rig->GetFittedActives().Contains(M.Id)||Rig->GetFittedPassives().Contains(M.Id));
        AddButton(C,15,FString::Printf(TEXT("SELL / %d CR"),Rig->GetResaleValue(M.Id)),1320,472,214,33,Rig->Owns(M.Id));
        const bool Fitted=Rig->GetFittedActives().Contains(M.Id)||Rig->GetFittedPassives().Contains(M.Id);
        FString Block=Rig->Owns(M.Id)?(!Fitted&&!CanFit?FitReason:!Rig->Has(M.Id)&&Fitted?TEXT("Inactive: fit the active tool required by this support."):TEXT("")):!CanBuy?BuyReason:TEXT("");
        if(!Rig->Owns(M.Id)&&CanBuy)
            Block=Purchase.bFitsNow?FString::Printf(TEXT("%d credits remain. Installs now; your equipment stays fitted."),Purchase.CreditsAfter):
                FString::Printf(TEXT("%d credits remain. Stored until you free %d %s socket%s. Nothing is removed automatically."),Purchase.CreditsAfter,
                    Purchase.SlotsToFree,M.Kind==EExpeditionModuleKind::Active?TEXT("tool"):TEXT("passive"),Purchase.SlotsToFree==1?TEXT(""):TEXT("s"));
        FName PairTool;FExpeditionPairPlan Pair;
        if(FindCompanionPlan(*Rig,M.Id,PairTool,Pair))
        {
            Block=FString::Printf(TEXT("Requires %s fitted. %d credits to %s; %d remain."),*NameOf(PairTool),Pair.Cost,
                Rig->Owns(PairTool)?TEXT("complete the pair"):TEXT("buy both"),Pair.CreditsAfter);
            TArray<FName> Removed=Pair.UnfitActives;Removed.Append(Pair.UnfitPassives);
            Block+=Removed.IsEmpty()?TEXT(" Both fit without removing equipment."):
                TEXT(" One valid refit: unfit ")+ModuleNames(Removed)+TEXT(" (kept in inventory).");
            Block+=Rig->Owns(PairTool)?TEXT(" Fit your tool, then buy and fit this support."):
                TEXT(" Buy and fit the tool, then this support.");
        }
        else if(!Rig->IsCompatible(M.Id))Block=TEXT("Requires fitted tools: ")+RequiredTools(M)+TEXT(". ")+Block;
        const auto Disabled=Rig->DisabledByUnfit(M.Id);
        if(!Disabled.IsEmpty())
        {
            if(!Block.IsEmpty())Block+=TEXT(" ");Block+=TEXT("Unfitting disables: ");
            Block+=ModuleNames(Disabled)+TEXT(". Those parts remain owned.");
        }
        Rule(Display.Get(),C,1105,525,429,Edge);
        Text(C,TEXT("BEFORE YOU COMMIT"),1105,540,.73f,Copper);
        Paragraph(Display.Get(),C,Block.IsEmpty()?TEXT("Equipment changes are free here. Your current rig stays intact until you choose an action."):Block,1105,564,428,.82f,Copper);
    }
    else
    {
        Icon(Display.Get(),C,Rig->GetFittedActives().IsEmpty()?NAME_None:Rig->GetFittedActives()[0],1235,262,146,Teal);
        Text(C,TEXT("BUILD YOUR NEXT CAPABILITY"),1114,464,1.08f,Paper,true);
        Paragraph(Display.Get(),C,TEXT("Select equipment to compare its purpose, socket cost and exact refit requirements before spending."),1114,512,414,.95f,Quiet);
    }
    ExpeditionUI::Panel(Display.Get(),C,44,746,1514,132,Teal);
    Text(C,TEXT("RIG STATUS"),64,762,.77f,Quiet);
    Text(C,FString::Printf(TEXT("%d / 2 TOOLS"),Rig->GetUsedSlots(EExpeditionModuleKind::Active)),254,762,.81f,Paper);
    for(int32 Slot=0;Slot<2;++Slot)Socket(Display.Get(),C,384+Slot*19,766,Slot<Rig->GetUsedSlots(EExpeditionModuleKind::Active),Teal);
    Text(C,FString::Printf(TEXT("%d / 4 SUPPORT SOCKETS"),Rig->GetUsedSlots(EExpeditionModuleKind::Passive)),477,762,.81f,Paper);
    for(int32 Slot=0;Slot<4;++Slot)Socket(Display.Get(),C,692+Slot*19,766,Slot<Rig->GetUsedSlots(EExpeditionModuleKind::Passive),Copper);
    FString Reason;const bool CanDepart=Rig->CanDepart(Reason);
    AddButton(C,11,Rig->IsPrecharged()?TEXT("BATTERY PRECHARGED / 120"):TEXT("PRECHARGE / +20 BATTERY / 4 CR"),64,815,448,41,!Rig->IsPrecharged()&&Rig->GetCash()>=4);
    Text(C,TEXT("Keep your rig, refit, or save for a later purchase."),544,825,.83f,Quiet);
    AddButton(C,10,TEXT("DEPART / RECOVER THE NEXT MACHINE"),1087,812,447,44,CanDepart);
    if(!CanDepart)FitText(Display.Get(),C,Reason,1087,778,443,.78f,Copper);
}
void FExpeditionRuntime::Paint(UCanvas* C)
{
    if(!C || !Display)return;
    Display->Width=C->ClipX;Display->Height=C->ClipY;
    Display->UIScale=FMath::Min(C->ClipX/1600.f,C->ClipY/900.f);
    Display->UX=(C->ClipX-1600*Display->UIScale)*.5f;Display->UY=(C->ClipY-900*Display->UIScale)*.5f;
    auto PaddedCopy=[&](const FString& Copy,float X,float Y,float W,float H,float Scale,FLinearColor Color)
    {
        // Measure with the real font/word wrapping before drawing; long recovery
        // instructions retain their full meaning and the panel's bottom padding.
        for(int32 Pass=0;Pass<8&&Scale>.69f;++Pass)
        {
            if(ExpeditionUI::Paragraph(Display.Get(),nullptr,Copy,X,Y,W,Scale,Color)-Y<=H)break;
            Scale=FMath::Max(.69f,Scale-.02f);
        }
        ExpeditionUI::Paragraph(Display.Get(),C,Copy,X,Y,W,Scale,Color);
    };
    Buttons.Reset();
    // Keep the physical workbench visible, with clear separation from menu copy.
    if(Screen==EExpeditionScreen::Starter||Screen==EExpeditionScreen::Depot)
        Display->Rect(C,0,0,1600,900,FLinearColor(.003f,.009f,.014f,.54f));
    ExpeditionUI::Panel(Display.Get(),C,18,12,1564,96,Copper);
    ExpeditionUI::Icon(Display.Get(),C,TEXT("extraction_coil"),35,25,42,Copper);
    Text(C,TEXT("MAGNET SWEEP"),92,23,1.35f,Paper,true);
    Text(C,TEXT("INDUSTRIAL RECOVERY DIVISION"),369,39,.66f,Quiet);
    if(NoticeLife<=0)
    {
        if(Screen==EExpeditionScreen::Site||Screen==EExpeditionScreen::Depot)
            ExpeditionUI::FitText(Display.Get(),C,World->GetSiteDefinition().Name+TEXT(" / ")+World->GetSiteDefinition().Summary,38,75,1490,.86f,Quiet);
        else Text(C,TEXT("Recover machinery. Build new capabilities. Bring the final core home."),38,74,.87f,Quiet);
    }
    for(int32 Site=0;Site<4;++Site)
    {
        const float X=807+Site*91;
        ExpeditionUI::Socket(Display.Get(),C,X,34,Site<=SiteIndex,Site<SiteIndex?Mint:Copper);
        Text(C,FString::Printf(TEXT("%02d"),Site+1),X+22,29,.9f,Site==SiteIndex?Paper:Quiet,true);
        if(Site<3)ExpeditionUI::Rule(Display.Get(),C,X+53,40,25,ExpeditionUI::Edge);
    }
    Text(C,TEXT("CREDITS"),1230,35,.73f,Quiet);
    Text(C,FString::Printf(TEXT("%03d"),Rig->GetCash()),1339,20,1.5f,Copper,true);
    Text(C,TEXT("CR"),1438,37,.8f,Copper);
    if(Screen==EExpeditionScreen::Starter)
    {
        Text(C,TEXT("BUILD A DIFFERENT KIND OF POWER"),47,137,1.52f,Paper,true);
        Text(C,TEXT("CHOOSE YOUR STARTING TOOL"),1123,153,.82f,Copper);
        ExpeditionUI::Paragraph(Display.Get(),C,TEXT("Equipment and credits start fresh. Discoveries and your last winning rig stay. Recover a core while owning a new tool to unlock it as a free starter next run."),49,188,1460,.97f,Quiet);
        const TArray<FName> Starters={TEXT("extraction_coil"),TEXT("rail_impeller"),TEXT("arc_driver"),TEXT("anchor_winch"),TEXT("vector_emitter"),TEXT("relay_projector")};
        for(int32 StarterIndex=0;StarterIndex<Starters.Num();++StarterIndex)
        {
            const auto* M=FExpeditionRig::FindModule(Starters[StarterIndex]);if(!M)continue;
            const float X=44+(StarterIndex%3)*512,Y=252+(StarterIndex/3)*295;
            const bool Enabled=Rig->IsStarterUnlocked(M->Id)&&FExpeditionWorld::IsModuleImplemented(M->Id);
            const FLinearColor Accent=Enabled?ExpeditionUI::ModuleColor(M->Id):Quiet;
            ExpeditionUI::Panel(Display.Get(),C,X,Y,488,269,Accent);
            Display->Rect(C,X+18,Y+18,104,104,ExpeditionUI::Ink);
            ExpeditionUI::Icon(Display.Get(),C,M->Id,X+30,Y+30,80,Accent);
            Text(C,FString::Printf(TEXT("TOOL / %02d"),StarterIndex+1),X+144,Y+25,.75f,Accent);
            ExpeditionUI::FitText(Display.Get(),C,M->Name,X+143,Y+53,320,1.28f,Paper,true);
            Text(C,Enabled?TEXT("AVAILABLE / FREE"):TEXT("LOCKED / RECOVER TO DISCOVER"),X+145,Y+99,.73f,Enabled?Mint:Quiet);
            ExpeditionUI::Paragraph(Display.Get(),C,M->Description,X+23,Y+142,441,.95f,Paper);
            AddButton(C,100+StarterIndex,Enabled?TEXT("EQUIP AS STARTER"):TEXT("DISCOVER TO UNLOCK"),X+22,Y+214,444,36,Enabled);
        }
    }
    else if(Screen==EExpeditionScreen::Depot)RenderDepot(C);
    else if(Screen==EExpeditionScreen::Site)
    {
        RenderWorld(C);
        ExpeditionUI::Panel(Display.Get(),C,18,128,307,455,Mint);
        Text(C,TEXT("RECOVERY ORDER"),37,148,1.02f,Paper,true);
        Text(C,FString::Printf(TEXT("CONTRACT / %02d"),SiteIndex+1),38,180,.7f,Copper);
        ExpeditionUI::Paragraph(Display.Get(),C,World->GetGoalText(),37,212,267,.92f,Paper);
        const auto& S=World->GetState();
        bool NextShown=false;int32 Step=0;
        for(const auto& Objective:World->GetObjectives())if(Objective.bRequired)
        {
            const bool Next=!Objective.bSatisfied&&!NextShown;NextShown|=Next;
            const float RowY=290+Step*25;
            ExpeditionUI::Socket(Display.Get(),C,38,RowY+3,Objective.bSatisfied,Objective.bSatisfied?Mint:Next?Copper:Quiet);
            ExpeditionUI::FitText(Display.Get(),C,(Next?TEXT("NEXT / "):TEXT(""))+Objective.Label,60,RowY,242,.81f,Objective.bSatisfied?Mint:Next?Paper:Quiet);
            ++Step;
        }
        ExpeditionUI::Rule(Display.Get(),C,37,389,267,ExpeditionUI::Edge);
        Text(C,TEXT("BATTERY"),37,406,.74f,Quiet);
        Text(C,FString::Printf(TEXT("%03d"),World->GetBattery()),240,393,1.05f,World->GetBattery()<20?Red:Mint,true);
        ExpeditionUI::Meter(Display.Get(),C,37,431,266,World->GetBattery(),Rig->IsPrecharged()?120:100,World->GetBattery()<20?Red:Mint);
        Text(C,TEXT("CARGO / SAFE LIMIT"),37,457,.70f,Quiet);
        Text(C,FString::Printf(TEXT("%.0f / 24 KG"),World->GetCargoMass()),214,450,.80f,World->IsUnsafe()?Red:Paper,true);
        ExpeditionUI::Meter(Display.Get(),C,37,485,266,World->GetCargoMass(),24,World->IsUnsafe()?Red:Copper,12);
        if(World->IsUnsafe())ExpeditionUI::FitText(Display.Get(),C,FString::Printf(TEXT("DROP NOW / %.1f s / lose 12 energy"),FMath::Max(0.f,3-S.UnsafeElapsed)),37,503,266,.79f,Red);
        else Text(C,TEXT("RMB drops. Field-off keeps cargo."),37,503,.78f,Quiet);
        const int32 Out=World->GetOutput();int32 Next=0;
        for(int32 Milestone=0;Milestone<2;++Milestone){const int32 Threshold=FExpeditionRig::RefiningThreshold(SiteIndex,Milestone);if(Out<Threshold){Next=Threshold;break;}}
        ExpeditionUI::FitText(Display.Get(),C,Next>0?FString::Printf(TEXT("REFINE %d / %d  = +2 CR"),Out,Next):SiteIndex==3?TEXT("Recover the final core to archive this rig."):TEXT("Bonuses earned. Recover the core."),37,538,266,.79f,Copper);
        ExpeditionUI::FitText(Display.Get(),C,SiteIndex<3?FString::Printf(TEXT("CORE RECOVERY  = +%d CR"),10+SiteIndex*2):TEXT("CORE RECOVERY = EXPEDITION COMPLETE"),37,559,266,.77f,Mint);
        ExpeditionUI::Panel(Display.Get(),C,18,592,307,141,Copper);
        Text(C,TEXT("FIELD NOTES"),35,603,.75f,Copper);
        PaddedCopy(RecoveryHint(),35,631,270,90,.83f,Paper);
        if(const auto* Frame=World->FindFrameBody();Frame && !bPaused)
        {
            ExpeditionUI::Panel(Display.Get(),C,1270,572,312,161,Copper);
            ExpeditionUI::FitText(Display.Get(),C,S.bFrameReceived?TEXT("POWER FRAME INSTALLED"):TEXT("OPTIONAL POWER FRAME / 40 kg"),1284,585,284,.78f,S.bFrameReceived?Mint:Copper);
            FString Reason;const auto* Dock=World->FindMarker(TEXT("frame_receiver"));
            const bool Ready=World->CanReceiveFrame(Reason);
            const bool Near=Dock && FVector2D::Distance(Magnet,Dock->Position)<=95;
            FString FrameCopy=S.bFrameReceived?(SiteIndex==3?TEXT("Output recorded. The installed frame supplies the final counterbalance. Recover the core."):
                TEXT("Output recorded. Continue the core recovery.")):Ready?TEXT("Frame and receiving support ready. Press E to install and collect its appraisal."):Reason;
            if(!S.bFrameReceived)FrameCopy+=TEXT(" Hover fittings for labels.");
            PaddedCopy(FrameCopy,1284,611,284,74,.78f,Paper);
            AddButton(C,50,S.bFrameReceived?TEXT("INSTALLED"):FString::Printf(TEXT("E: RECOVER FRAME / +%d OUTPUT"),Frame->Appraisal),1284,694,284,28,Ready&&Near);
        }
        if(Rig->Has(TEXT("rail_impeller")) && !bPaused)
        {
            const bool CompactRail=World->FindFrameBody()!=nullptr;
            ExpeditionUI::Panel(Display.Get(),C,1270,182,312,CompactRail?378:550,Mint);
            Text(C,TEXT("RAIL PREPARATION"),1284,197,.82f,Mint);
            AddButton(C,40,bPrepareWeld?TEXT("LAUNCH"):TEXT("[LAUNCH]"),1284,230,136,32);
            AddButton(C,41,bPrepareWeld?TEXT("[WELD]"):TEXT("WELD"),1430,230,138,32,Rig->Has(TEXT("slug_press")));
            ExpeditionUI::Paragraph(Display.Get(),C,bPrepareWeld?TEXT("Check 2+ iron pieces. Only checked pieces are welded. Other cargo stays."):
                 TEXT("Choose one body. C cycles. Aim + release the Rail tool to fire."),1284,274,284,.75f,Paper);
            int32 Count=0;
            for(const auto& B:World->GetBodies())if((B.State==EExpeditionBodyState::Cargo || S.CycloneIds.Contains(B.Id)) && !B.bGoal)
            {
                if(++Count>18)break;
                const bool Chosen=bPrepareWeld?WeldSelection.Contains(B.Id):SelectedCargoId==B.Id;
                const bool Allowed=!bPrepareWeld||(B.State==EExpeditionBodyState::Cargo&&B.Material==EExpeditionMaterial::Iron&&!B.bHot);
                const int32 Columns=CompactRail?2:1;
                const float CargoX=1284+((Count-1)%Columns)*146,CargoY=350+((Count-1)/Columns)*21;
                AddButton(C,2000+B.Id,FString::Printf(TEXT("%s #%d  %s  %.0f kg"),Chosen?TEXT("[x]"):TEXT("[ ]"),B.Id,
                    B.Material==EExpeditionMaterial::Iron?TEXT("iron"):B.bHot?TEXT("HOT"):TEXT("metal"),B.Mass),CargoX,CargoY,CompactRail?138:284,19,Allowed);
            }
            if(Count==0)ExpeditionUI::Paragraph(Display.Get(),C,TEXT("Collect scrap to load the tool. Shift narrows ordinary attraction."),1284,350,284,.78f,Quiet);
        }
        ExpeditionUI::Panel(Display.Get(),C,18,745,514,137,Mint);
        ExpeditionUI::Panel(Display.Get(),C,544,745,514,137,Mint);
        ExpeditionUI::Panel(Display.Get(),C,1070,745,512,137,Copper);
        const auto& Active=Rig->GetFittedActives();
        for(int32 I=0;I<2;++I)
        {
            const float X=92+I*526;const FString Name=Active.IsValidIndex(I)?NameOf(Active[I]):TEXT("Empty tool socket");
            Text(C,I==0?TEXT("Q"):TEXT("F"),X-56,757,1.1f,Copper,true);
            Text(C,TEXT("HOLD TO AIM / RELEASE TO USE"),X,762,.75f,Quiet);
            ExpeditionUI::Icon(Display.Get(),C,Active.IsValidIndex(I)?Active[I]:NAME_None,X-57,792,40,Active.IsValidIndex(I)?ExpeditionUI::ModuleColor(Active[I]):Quiet);
            ExpeditionUI::FitText(Display.Get(),C,Name,X,781,409,1.08f,Paper,true);
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
                        AddButton(C,60+I*4+Op,(ToolOperation[I]==Op?TEXT("> "):TEXT(""))+Modes[Op],X+(VisibleMode++)*102,825,96,26,!bPaused);
                    Text(C,TEXT("Choose operation, then aim with your tool key."),X,852,.68f,Quiet);
                }
                else if(const auto* M=FExpeditionRig::FindModule(Tool))ExpeditionUI::Paragraph(Display.Get(),C,M->Description,X,816,416,.80f,Quiet);
            }
        }
        Text(C,TEXT("LMB / SPACE attract   SHIFT precise"),1090,765,.75f,Paper);
        Text(C,TEXT("RMB  drop whole haul"),1090,798,.88f,Paper);
        Text(C,TEXT("E  interact / bank / deliver"),1090,830,.88f,Paper);
        Text(C,TEXT("ESC  planning pause     M  sound"),1090,852,.72f,Quiet);
        if(bPaused)
        {
            Display->Rect(C,0,111,1600,789,FLinearColor(.005f,.012f,.018f,.72f));
            ExpeditionUI::Panel(Display.Get(),C,442,236,716,380,Copper,true);Text(C,TEXT("PLANNING PAUSE / ALL DANGER FROZEN"),475,264,1.10f,Paper,true);
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
        ExpeditionUI::Panel(Display.Get(),C,320,250,960,405,Copper,true);
        Text(C,Screen==EExpeditionScreen::Victory?TEXT("CORE RECOVERED"):TEXT("EXPEDITION ENDED"),365,290,1.5f,Mint,true);
        Wrap(C,Screen==EExpeditionScreen::Victory?TEXT("You built this rig and brought the machine home. Your winning loadout is archived. Which capability would you choose differently next time?"):
             TEXT("Dispatched discoveries remain. Start another expedition with a different tool, or return later."),365,352,96,1,Paper);
        int32 TotalOutput=0;for(int32 I=0;I<4;++I)TotalOutput+=Rig->GetOutput(I);
        Text(C,FString::Printf(TEXT("Recovered output: %d    Completed expeditions: %d"),TotalOutput,Rig->GetRecords().RunsWon),365,434,.9f,Copper);
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
        Display->Rect(C,31,65,1537,35,ExpeditionUI::Ink);
        Display->Rect(C,31,65,3,35,Mint);
        ExpeditionUI::Paragraph(Display.Get(),C,Notice,44,68,1505,.78f,Paper);
    }
}

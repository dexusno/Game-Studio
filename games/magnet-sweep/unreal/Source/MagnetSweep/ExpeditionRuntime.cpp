#include "ExpeditionRuntime.h"
#include "VisualCaptureProbe.h"
#include "ExpeditionRig.h"
#include "ExpeditionWorld.h"
#include "WorkbenchRuntime.h"
#include "MagnetGame.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "InputKeyEventArgs.h"
#include "UnrealClient.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

using namespace MagnetSweep;
namespace
{
const FLinearColor Paper(.89f,.96f,.91f), Quiet(.59f,.74f,.72f), Mint(.31f,.94f,.69f);
const FLinearColor Copper(1.f,.59f,.25f), Red(1.f,.28f,.18f), Panel(.008f,.025f,.029f,.97f);
FString ToolName(FName Id)
{
    const auto* M = FExpeditionRig::FindModule(Id); return M ? M->Name : TEXT("Empty tool socket");
}
TArray<FName> ImplementedModules()
{
    TArray<FName> Result;
    for (const auto& M : FExpeditionRig::Catalog())
        if (FExpeditionWorld::IsModuleImplemented(M.Id)) Result.Add(M.Id);
    return Result;
}
bool VisibleBody(const FExpeditionBody& B)
{
    return B.State != EExpeditionBodyState::Banked && B.State != EExpeditionBodyState::Consumed
        && B.State != EExpeditionBodyState::Dispatched;
}
int32 ElectricalTarget(const FExpeditionWorld& World,const FVector2D& Aim)
{
    // A marked floor contact can sit under its payload. Select the intended
    // contact so the shared exposure check explains the physical obstruction.
    for(const auto& Body:World.GetBodies())
        if(Body.bFloorContact && VisibleBody(Body) && FVector2D::Distance(Aim,Body.Position)<=Body.Radius)
            return Body.Id;
    return World.FindBodyAt(Aim,24);
}
FName BodyMaterial(const FExpeditionBody& B)
{
    if (B.bHot) return TEXT("Hazard");
    if (B.bGoal) return TEXT("Core");
    switch (B.Material)
    {
    case EExpeditionMaterial::Copper: return TEXT("Copper");
    case EExpeditionMaterial::Alloy: return TEXT("Alloy");
    case EExpeditionMaterial::Mechanism: return TEXT("Brass");
    default: return TEXT("Steel");
    }
}
FString BodyLabel(const FExpeditionBody& B)
{
    FString Name = B.Role.ToString(); Name.ReplaceInline(TEXT("_"), TEXT(" "));
    return FString::Printf(TEXT("%s  %.0f kg%s"), *Name, B.Mass, B.bHot ? TEXT("  HOT") : TEXT(""));
}
}

FExpeditionRuntime::FExpeditionRuntime(AMagnetWorkbench* InOwner) : Owner(InOwner)
{
    Rig = MakeUnique<FExpeditionRig>(); World = MakeUnique<FExpeditionWorld>();
    if (Owner) Display = MakeUnique<FWorkbenchImpl>(Owner);
}
FExpeditionRuntime::~FExpeditionRuntime() = default;

void FExpeditionRuntime::Start()
{
    PC = Owner ? Owner->GetWorld()->GetFirstPlayerController() : nullptr;
    FParse::Value(FCommandLine::Get(), TEXT("ExpeditionProfile="), Profile);
    for (int32 I=0; I<Profile.Len(); ++I)
        if (!FChar::IsAlnum(Profile[I]) && Profile[I] != TCHAR('_')) Profile[I]=TCHAR('_');
    Profile=Profile.Left(40); if(Profile.IsEmpty()) Profile=TEXT("expedition_preview");
    const bool bVisualAudit=InitializeVisualCaptureProbe(*this);
    SavePath=bVisualAudit?FString():FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Expedition"),Profile+TEXT(".json"));
    // DemoProfile and DemoFresh belong to the old career and are deliberately ignored.
    const bool bLoaded=bVisualAudit?false:Load();
    if(Display)
    {
        Display->PC=PC; Display->Profile=TEXT("expedition_assets");Display->bMuted=bMuted;Display->bPaused=bPaused;
        Display->SetupScene(); // Asset/scene helpers only. Never legacy Start, Load or Save.
        BuildMechanismVisuals();
        if(auto* S=Display->Sounds.Find(TEXT("workshop_music")))
            Music=UGameplayStatics::SpawnSound2D(Owner,*S,bMuted?0:.25f,1,0,nullptr,false,false);
        if(auto* S=Display->Sounds.Find(TEXT("magnet_loop")))
            FieldAudio=UGameplayStatics::SpawnSound2D(Owner,*S,0,1,0,nullptr,false,false);
    }
    if(bLoaded) { bPaused=Screen==EExpeditionScreen::Site; Show(TEXT("Expedition restored. Your rig, energy and worksite are preserved.")); }
    else if(bSaveAllowed) Show(TEXT("Build a rig. Recover the machinery. Bring the core home."));
    if(Music.IsValid())Music->SetPaused(bPaused);
    if(GEngine && GEngine->GameViewport)
        InputHandle=GEngine->GameViewport->OnInputKey().AddLambda([this](const FInputKeyEventArgs& E)
        {
            if(E.Event!=IE_Pressed && E.Event!=IE_Released && E.Event!=IE_Repeat) return;
            UpdatePointer(); Key(E.Key,E.Event!=IE_Released,E.Event==IE_Repeat);
        });
    Save();
}
void FExpeditionRuntime::Stop()
{
    if(InputHandle.IsValid() && GEngine && GEngine->GameViewport)
        GEngine->GameViewport->OnInputKey().Remove(InputHandle);
    InputHandle.Reset();
    if(Music.IsValid()) Music->Stop();
    if(FieldAudio.IsValid()) FieldAudio->Stop();
    if(Display) Display->Stop();
}
void FExpeditionRuntime::Show(const FString& Value,FName Cue)
{
    Notice=Value; NoticeLife=6;
    if(Display && !Cue.IsNone()) Display->Play(Cue,.7f);
}
void FExpeditionRuntime::SetPaused(bool bValue)
{
    bPaused=bValue; Input.Cancel(); ClearPreparation();
    if(Display){Display->bPaused=bValue;for(auto& Sound:Display->PlayingSounds)if(Sound.IsValid())Sound->SetPaused(bValue);}
    if(FieldAudio.IsValid()) FieldAudio->SetVolumeMultiplier(0);
    if(Music.IsValid()) Music->SetPaused(bValue);
    Save();
}
void FExpeditionRuntime::UpdatePointer()
{
    if(!PC) return;
    float X=0,Y=0;
    if(!PC->GetMousePosition(X,Y)){bWorldHit=false; return;}
    Pointer={X,Y}; FVector O,D;
    bWorldHit=PC->DeprojectScreenPositionToWorld(X,Y,O,D) && FMath::Abs(D.Z)>.001f;
    if(bWorldHit)
    {
        const FVector P=O+D*((18-O.Z)/D.Z); Aim={P.X,P.Y};
        bWorldHit=Aim.X>=-510 && Aim.X<=770 && Aim.Y>=-310 && Aim.Y<=310;
    }
    for(const auto& B:Buttons) if(B.Rect.IsInside(Pointer)){bWorldHit=false; break;}
}
void FExpeditionRuntime::StartField()
{
    if(Screen!=EExpeditionScreen::Site || bPaused || !bWorldHit || Input.ArmedSlot!=INDEX_NONE) return;
    ClearPreparation();
    const auto Result=World->Execute(FieldCommand(),*Rig);
    Input.bField=Result.bSucceeded || bContinuousField;
    if(!Result.bSucceeded && !bContinuousField) Show(Result.Message);
}
FExpeditionCommand FExpeditionRuntime::FieldCommand() const
{
    FExpeditionCommand C;C.Action=EExpeditionAction::Attract;C.Magnet=Magnet;C.Aim=Magnet;C.bPrecision=bPrecision;return C;
}
void FExpeditionRuntime::RefreshCargoSelection()
{
    auto IsCargo=[&](int32 Id){const auto* B=World->FindBody(Id);return B && (B->State==EExpeditionBodyState::Cargo || World->GetState().CycloneIds.Contains(Id)) && !B->bGoal;};
    if(!IsCargo(SelectedCargoId))
    {
        SelectedCargoId=INDEX_NONE;
        for(const auto& B:World->GetBodies())if(IsCargo(B.Id)){SelectedCargoId=B.Id;break;}
    }
    WeldSelection.RemoveAll([&](int32 Id){const auto* B=World->FindBody(Id);return !B||B->State!=EExpeditionBodyState::Cargo||B->Material!=EExpeditionMaterial::Iron||B->bHot;});
}
void FExpeditionRuntime::CycleCargo()
{
    if(bPrepareWeld){Show(TEXT("Weld mode: use the checkboxes to choose the iron pieces. Switch to Launch to cycle ammunition."));return;}
    RefreshCargoSelection();TArray<int32> Cargo;
    for(const auto& B:World->GetBodies())if((B.State==EExpeditionBodyState::Cargo || World->GetState().CycloneIds.Contains(B.Id)) && !B.bGoal)Cargo.Add(B.Id);
    if(Cargo.IsEmpty())return;
    SelectedCargoId=Cargo[(Cargo.IndexOfByKey(SelectedCargoId)+1)%Cargo.Num()];
    Show(FString::Printf(TEXT("Selected ammunition #%d. Only this body will launch."),SelectedCargoId));
}
FString FExpeditionRuntime::RecoveryHint() const
{
    if(World->IsUnsafe())return TEXT("Right-click now to drop the whole haul safely. A spill at fuse expiry also costs 12 battery. Dropped scrap can be recovered.");
    if(const auto* Frame=World->FindFrameBody();Frame && !World->GetState().bFrameReceived)
    {
        const auto* Dock=World->FindMarker(TEXT("frame_receiver"));
        if(FVector2D::Distance(Aim,Frame->Position)<Frame->Radius+40 ||
            (Dock && FVector2D::Distance(Magnet,Dock->Position)<100))return FrameRecoveryHint();
    }
    if(FVector2D::Distance(Magnet,FExpeditionWorld::FurnacePosition())<115 && World->GetCargoValue()>0)
        return TEXT("Press E to smelt settled scrap here. Iron, copper and alloy can be mixed. The mission core is protected and goes to the receiver.");
    for(const auto& Objective:World->GetObjectives())
        if(Objective.bRequired && !Objective.bSatisfied)return Objective.Hint;
    const auto* Core=World->FindBody(0);
    if(Core && Core->State==EExpeditionBodyState::Cargo)
        return SiteIndex==3?TEXT("Carry the core to MACHINE RECEIVER on the right. Wait for it to settle, then press E to complete the expedition and archive this rig."):
            TEXT("Carry the core to MACHINE RECEIVER on the right. Wait for it to settle, then press E to earn this site's recovery credits.");
    return FString::Printf(TEXT("Make room for the %.0f kg core; you hold %.0f of 24 safe kg. Collect the glowing core, carry it to MACHINE RECEIVER and press E once settled."),
        Core?Core->Mass:8.f+4.f*SiteIndex,World->GetCargoMass());
}
FString FExpeditionRuntime::FrameRecoveryHint() const
{
    const auto* Frame=World->FindFrameBody();
    if(!Frame)return TEXT("");
    if(World->GetState().bFrameReceived)return TEXT("Power frame installed. Its appraisal is recorded in this expedition's output.");
    FString Reason;
    if(World->CanReceiveFrame(Reason))return FString::Printf(TEXT("Frame ready. Move the magnet to FRAME DOCK and press E to recover +%d output. The frame stays installed here."),Frame->Appraisal);
    if(World->GetState().bFrameHoistActive)return TEXT("Hoist moving the frame. Let it settle at FRAME DOCK, then move the magnet there and press E to recover it.");
    if(Frame->bAnchored)return TEXT("Optional 40 kg frame exceeds the 36 kg pickup limit. Support its mount to move it, or power the exposed loop input. Recover it at FRAME DOCK with E.");
    return Reason;
}
void FExpeditionRuntime::ClearPreparation()
{
    RelaySource=INDEX_NONE;PreparedPartner=INDEX_NONE;PreparedSlot=INDEX_NONE;PreparationStage=0;
    PreparedPoint=FVector2D::ZeroVector;PreparedBend=FVector2D::ZeroVector;
}
FString FExpeditionRuntime::PreparationHint(int32 Slot) const
{
    const auto C=CommandFor(Slot);
    if(C.Action==EExpeditionAction::ArmRelay)
        return PreparationStage==0?TEXT("Release on a moving sensor body to mark it (free); then choose the receiver terminal."):TEXT("");
    if(C.Action==EExpeditionAction::TransferHeat)
        return PreparationStage==0?TEXT("Release on a hot body to mark it (free); then choose a separate iron heat sink."):TEXT("");
    if(C.Action==EExpeditionAction::LayGuide)
        return PreparationStage==0?TEXT("Release to mark the guide entrance (free). Next mark its bend and exit."):
               PreparationStage==1?TEXT("Release to mark the bend (free). The exit commits the 16-battery guide."):TEXT("");
    if(C.Action==EExpeditionAction::Relay)
        return PreparationStage==0?TEXT("Release on a loose body to mark the source (free). Next choose its destination."):
               PreparationStage==1 && ToolOperation[Slot]==3 && Rig->Has(TEXT("flow_splitter"))?TEXT("Release to mark receiver A (free). Next choose receiver B to commit."):TEXT("");
    if(C.Action==EExpeditionAction::Vector && ToolOperation[Slot]==3 && Rig->Has(TEXT("reaction_frame")))
        return PreparationStage==0?TEXT("Release on the body to push (free). Next choose its reaction partner."):
               PreparationStage==1?TEXT("Release on a separate supported reaction body (free). Next aim the impulse."):TEXT("");
    return TEXT("");
}
bool FExpeditionRuntime::PrepareAction(int32 Slot)
{
    if(PreparedSlot!=Slot){ClearPreparation();PreparedSlot=Slot;}
    const auto C=CommandFor(Slot);
    if(PreparationHint(Slot).IsEmpty())return false;
    if(C.Action==EExpeditionAction::LayGuide)
    {
        if(PreparationStage==0)PreparedPoint=Aim;else PreparedBend=Aim;
    }
    else if(C.Action==EExpeditionAction::Relay && PreparationStage==1)PreparedPoint=Aim;
    else
    {
        const int32 Id=World->FindBodyAt(Aim,24);
        if(Id==INDEX_NONE){Show(TEXT("Aim at a visible body to mark this stage. No battery spent."));return true;}
        if(PreparationStage==0)RelaySource=Id;else PreparedPartner=Id;
    }
    ++PreparationStage;
    const FString Next=PreparationHint(Slot);
    Show(Next.IsEmpty()?C.Action==EExpeditionAction::ArmRelay?TEXT("Sensor marked. Aim at the receiver terminal and release to reserve its charge."):
        C.Action==EExpeditionAction::TransferHeat?TEXT("Hot source marked. Aim at an available iron body and release to transfer the heat."):
        C.Action==EExpeditionAction::Vector?TEXT("Bodies marked. Aim the direction and release to commit both reactions."):
        TEXT("Preparation marked. Aim at the final destination and release to pay and commit."):Next);
    return true;
}
FExpeditionCommand FExpeditionRuntime::CommandFor(int32 Slot) const
{
    FExpeditionCommand C; C.Magnet=Magnet; C.Aim=Aim; C.Destination=Aim;
    const auto& Actives=Rig->GetFittedActives();
    if(!Actives.IsValidIndex(Slot)) return C;
    const FName Tool=Actives[Slot];
    if(Tool==TEXT("extraction_coil")) C.Action=EExpeditionAction::Extract;
    else if(Tool==TEXT("arc_driver")) C.Action=ToolOperation[Slot]==1 && Rig->Has(TEXT("ground_clip"))?EExpeditionAction::Ground:
        ToolOperation[Slot]==2 && Rig->Has(TEXT("escapement_relay"))?EExpeditionAction::ArmRelay:EExpeditionAction::Arc;
    else if(Tool==TEXT("anchor_winch")){C.Action=ToolOperation[Slot]==1 && Rig->Has(TEXT("twin_anchor"))?EExpeditionAction::SwitchAnchor:EExpeditionAction::Winch; C.Destination=Magnet;}
    else if(Tool==TEXT("vector_emitter")) C.Action=ToolOperation[Slot]==1 && Rig->Has(TEXT("heat_sink_mould"))?EExpeditionAction::TransferHeat:EExpeditionAction::Vector;
    else if(Tool==TEXT("relay_projector")) C.Action=ToolOperation[Slot]==1 && Rig->Has(TEXT("heat_sink_mould"))?EExpeditionAction::TransferHeat:
        ToolOperation[Slot]==2 && Rig->Has(TEXT("field_loom"))?EExpeditionAction::LayGuide:EExpeditionAction::Relay;
    else if(Tool==TEXT("rail_impeller"))
    {
        if(bPrepareWeld && Rig->Has(TEXT("slug_press")))
        { C.Action=EExpeditionAction::Weld; C.BodyIds=WeldSelection; }
        else
        {
            C.Action=EExpeditionAction::Launch; C.TargetId=SelectedCargoId;
            if(SelectedCargoId!=INDEX_NONE) C.BodyIds.Add(SelectedCargoId);
        }
        return C;
    }
    if(C.Action==EExpeditionAction::ArmRelay || C.Action==EExpeditionAction::TransferHeat)
    {C.TargetId=RelaySource;C.SecondaryId=C.Action==EExpeditionAction::ArmRelay?ElectricalTarget(*World,Aim):World->FindBodyAt(Aim,24);return C;}
    if(C.Action==EExpeditionAction::LayGuide)
    {C.Aim=PreparedPoint;C.SecondaryPoint=PreparedBend;C.bHasSecondaryPoint=PreparationStage>=2;return C;}
    if(C.Action==EExpeditionAction::Relay)
    {
        C.TargetId=RelaySource;
        if(ToolOperation[Slot]==3 && Rig->Has(TEXT("flow_splitter")) && PreparationStage>=2){C.Destination=PreparedPoint;C.SecondaryPoint=Aim;C.bHasSecondaryPoint=true;}
        return C;
    }
    if(C.Action==EExpeditionAction::Vector && ToolOperation[Slot]==3 && Rig->Has(TEXT("reaction_frame")))
    {C.TargetId=RelaySource;C.SecondaryId=PreparedPartner;return C;}
    C.TargetId=C.Action==EExpeditionAction::Arc?ElectricalTarget(*World,Aim):World->FindBodyAt(Aim,24);
    return C;
}
void FExpeditionRuntime::Act(int32 Slot)
{
    if(Screen!=EExpeditionScreen::Site || bPaused || !bWorldHit || !Rig->GetFittedActives().IsValidIndex(Slot)) return;
    RefreshCargoSelection();
    if(PrepareAction(Slot))return;
    const auto C=CommandFor(Slot); const auto P=World->Preview(C,*Rig);
    if(!P.bAllowed){Show(P.Reason); return;}
    const auto R=World->Execute(C,*Rig); Show(R.Message);
    if(R.bSucceeded)
    {
        ClearPreparation();
        if(C.Action==EExpeditionAction::Weld)
        { bPrepareWeld=false;WeldSelection.Reset();if(!R.BodyIds.IsEmpty())SelectedCargoId=R.BodyIds[0]; }
    }
    Save();
}
void FExpeditionRuntime::Key(const FKey& K,bool bPressed,bool bRepeat)
{
    if(bRepeat) return;
    if(K==EKeys::LeftShift || K==EKeys::RightShift){bPrecision=bPressed;return;}
    const int32 Slot=K==EKeys::Q?0:K==EKeys::F?1:INDEX_NONE;
    if(Slot!=INDEX_NONE)
    {
        if(bPressed)
        {
            if(Screen==EExpeditionScreen::Site && !bPaused && Rig->GetFittedActives().IsValidIndex(Slot))
            { if(Input.BeginAim(Slot)){if(PreparedSlot!=Slot)ClearPreparation();} }
        }
        else { const int32 Commit=Input.EndAim(Slot); if(Commit!=INDEX_NONE) Act(Commit); }
        return;
    }
    if(!bPressed)
    {
        if(K==EKeys::LeftMouseButton && Input.bField && !bPaused){Input.bField=false; World->CancelPull();}
        return;
    }
    if(K==EKeys::Escape){if(Screen==EExpeditionScreen::Site) SetPaused(!bPaused); return;}
    if(K==EKeys::M)
    {
        bMuted=!bMuted; if(Display) Display->bMuted=bMuted;
        if(Music.IsValid()) Music->SetVolumeMultiplier(bMuted?0:.25f); Save(); return;
    }
    if(K==EKeys::LeftMouseButton)
    {
        for(const auto& B:Buttons) if(B.Rect.IsInside(Pointer)){if(B.bEnabled) Click(B.Id); return;}
        StartField(); return;
    }
    if(Screen!=EExpeditionScreen::Site) return;
    if(K==EKeys::RightMouseButton)
    {
        Input.Cancel(); ClearPreparation();
        if(!bPaused){Show(World->Drop().Message); Save();} return;
    }
    if(bPaused) return;
    if(K==EKeys::C){CycleCargo();return;}
    if(K==EKeys::SpaceBar)
    {
        if(Input.bField){Input.Cancel(); World->CancelPull();} else StartField();
    }
    else if(K==EKeys::E) BankOrDeliver();
    else if(K==EKeys::R) { SetPaused(true); Show(TEXT("Retry restores the entire site entry, including energy and rewards.")); }
}
void FExpeditionRuntime::BankOrDeliver()
{
    if(Screen!=EExpeditionScreen::Site || bPaused) return;
    if(const auto* Dock=World->FindMarker(TEXT("frame_receiver"));Dock && FVector2D::Distance(Magnet,Dock->Position)<=95)
    {ReceiveFrame();return;}
    Input.Cancel(); ClearPreparation(); World->CancelPull();
    if(FVector2D::Distance(Magnet,FExpeditionWorld::ReceiverPosition())<100)
    {
        const auto R=World->Dispatch(); Show(R.Message);
        if(R.bSucceeded)
        {
            const auto PreviousStarters=Rig->GetRecords().UnlockedStarters;
            const int32 Pay=Rig->AwardSite(SiteIndex);
            FString Unlocks;
            for(FName Starter:Rig->GetRecords().UnlockedStarters)if(!PreviousStarters.Contains(Starter))
                Unlocks+=TEXT(" ")+ToolName(Starter)+TEXT(" unlocked as a free starter.");
            if(SiteIndex==3){Screen=EExpeditionScreen::Victory; Show(TEXT("CORE RECOVERED. Your winning rig is archived.")+Unlocks,TEXT("contract_success"));}
            else
            {
                ++SiteIndex; World->StartSite(SiteIndex,Rig->GetSeed()+SiteIndex);
                Rig->SetShopContext(World->GetOpportunityTags(),ImplementedModules());
                FString Reason;
                if(!Rig->NextDepot(Reason)) Show(Reason);
                else Show(FString::Printf(TEXT("Recovery complete: +%d credits. Choose the next rig improvement."),Pay)+Unlocks,TEXT("contract_success"));
                Screen=EExpeditionScreen::Depot; BuildMechanismVisuals();
            }
            SiteEntry.Reset();
        }
    }
    else if(FVector2D::Distance(Magnet,FExpeditionWorld::FurnacePosition())<115)
    {
        const auto R=World->Bank(); Show(R.Message);
        if(R.bSucceeded)
        {
            const int32 Paid=Rig->AwardOutput(SiteIndex,World->GetOutput());
            if(Paid>0) Show(FString::Printf(TEXT("Refining milestone: +%d credits. Output %d."),Paid,World->GetOutput()),TEXT("payout"));
        }
    }
    else
    {
        FExpeditionCommand C; C.Action=EExpeditionAction::Interact; C.Magnet=Magnet; C.Aim=Aim;
        C.TargetId=World->FindBodyAt(Aim,30); Show(World->Execute(C,*Rig).Message);
    }
    Save();
}
void FExpeditionRuntime::ReceiveFrame()
{
    if(Screen!=EExpeditionScreen::Site || bPaused)return;
    const auto* Dock=World->FindMarker(TEXT("frame_receiver"));
    if(!Dock)return;
    if(FVector2D::Distance(Magnet,Dock->Position)>95)
    {Show(TEXT("Move the magnet to FRAME DOCK, then press E to recover the settled frame."));return;}
    Input.Cancel();ClearPreparation();World->CancelPull();
    const auto Result=World->ReceiveFrame();Show(Result.Message);
    if(Result.bSucceeded)
    {
        const int32 Paid=Rig->AwardOutput(SiteIndex,World->GetOutput());
        if(Paid>0)Show(FString::Printf(TEXT("Frame installed. Output %d / refining reward +%d credits."),World->GetOutput(),Paid),TEXT("payout"));
        else Show(Result.Message,TEXT("contract_success"));
    }
    Save();
}
void FExpeditionRuntime::Depart()
{
    if(Screen!=EExpeditionScreen::Depot) return;
    FString Reason; const int32 Battery=FMath::RoundToInt(Rig->GetStartingBattery());
    if(!Rig->CanDepart(Reason)){Show(Reason); return;}
    const auto& PreviewState=World->GetState();
    auto NextWorld=MakeUnique<FExpeditionWorld>();
    if(!NextWorld->StartSite(SiteIndex,Rig->GetSeed()+SiteIndex,Battery,PreviewState.LayoutId,PreviewState.LayoutRevision))
    {Show(TEXT("The saved worksite definition is unavailable. Your depot is preserved."));return;}
    if(!Rig->Depart(Reason)){Show(Reason);return;}
    World=MoveTemp(NextWorld);
    Screen=EExpeditionScreen::Site; bPaused=false; Magnet=FVector2D(0,-240);
    Input.Cancel(); ClearPreparation(); ToolOperation[0]=ToolOperation[1]=0;World->Tick(.001f,Magnet,*Rig);
    SiteEntry=MakeState(false); BuildMechanismVisuals();
    Show(World->GetSiteDefinition().Summary);
    Save();
}
void FExpeditionRuntime::RetrySite()
{
    if(!SiteEntry.IsValid()) {Show(TEXT("No active site checkpoint.")); return;}
    const auto Checkpoint=SiteEntry; FString Error;
    if(!RestoreState(Checkpoint,Error,false)){Show(TEXT("Could not restore site: ")+Error); return;}
    SiteEntry=Checkpoint; bPaused=true; Input.LoseFocus(); ClearPreparation();
    BuildMechanismVisuals(); Show(TEXT("Site entry restored. Energy, scrap and rewards rolled back together. Resume when ready.")); Save();
}

void FExpeditionRuntime::Tick(float Delta)
{
    const float D=FMath::Clamp(Delta,0.f,.1f); Elapsed+=D; NoticeLife=FMath::Max(0.f,NoticeLife-D);
    if(Owner && GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
    {
        const bool Focused=GEngine->GameViewport->Viewport->HasFocus();
        if(!Focused && bWasFocused){Input.LoseFocus(); bPrecision=false; if(Screen==EExpeditionScreen::Site) SetPaused(true);}
        bWasFocused=Focused;
    }
    UpdatePointer();
    if(Screen==EExpeditionScreen::Site && !bPaused)
    {
        if(bWorldHit && Input.ArmedSlot==INDEX_NONE) Magnet=FMath::Vector2DInterpTo(Magnet,Aim,D,15.f);
        World->Tick(D,Magnet,*Rig);
        if(Input.bField && World->GetState().PullRemaining<=0)
        {
            if(bContinuousField) StartField(); else Input.bField=false;
        }
        for(const auto& Event:World->DrainEvents())
        {
            FName Cue; FLinearColor Color=Mint; int32 Count=10;
            switch(Event.Kind)
            {
            case EExpeditionEventKind::Captured: Cue=TEXT("pickup_heavy"); Impact=.5f; break;
            case EExpeditionEventKind::Severed: Cue=TEXT("pickup_metal2"); Color=Copper; break;
            case EExpeditionEventKind::Impact: Cue=TEXT("pickup_metal3"); Impact=.8f; break;
            case EExpeditionEventKind::Welded: Cue=TEXT("smelt"); Color=Copper; Count=24; break;
            case EExpeditionEventKind::Discharge: Cue=TEXT("rare_find"); Count=30; break;
            case EExpeditionEventKind::Mechanism: Cue=TEXT("upgrade"); Show(Event.Message); break;
            case EExpeditionEventKind::Quench: Cue=TEXT("overload"); Color=Red; Count=36; Input.Cancel(); Show(Event.Message); break;
            case EExpeditionEventKind::Dropped: Cue=TEXT("vent"); break;
            case EExpeditionEventKind::Banked:
                if(const auto* Frame=World->FindFrameBody();Frame && Event.BodyIds.Contains(Frame->Id))
                {Cue=TEXT("pickup_heavy");Impact=.6f;Color=Mint;Count=24;}
                else {Cue=TEXT("smelt");Color=Copper;Count=32;}
                break;
            default: break;
            }
            if(Display)
            {
                if(!Cue.IsNone()) Display->Play(Cue,.65f);
                Display->AddBurst(Display->World(Event.Position,35),Color,Count,140);
            }
        }
    }
    Impact=FMath::FInterpTo(Impact,0.f,D,7.f);
    if(FieldAudio.IsValid()) FieldAudio->SetVolumeMultiplier(!bMuted && !bPaused && (Input.bField || World->GetState().PullRemaining>0)?.3f:0.f);
    RefreshCargoSelection();UpdateVisuals(bPaused?0.f:D);
    SaveElapsed+=D; if(SaveElapsed>=2){SaveElapsed=0; Save();}
    TickVisualCaptureProbe(*this,Delta);
}

TSharedPtr<FJsonObject> FExpeditionRuntime::MakeState(bool bIncludeCheckpoint) const
{
    auto J=MakeShared<FJsonObject>(); J->SetStringField(TEXT("mode"),TEXT("salvage_expedition"));
    J->SetNumberField(TEXT("schema"),1); J->SetNumberField(TEXT("screen"),int32(Screen));
    J->SetNumberField(TEXT("site"),SiteIndex); J->SetNumberField(TEXT("magnet_x"),Magnet.X); J->SetNumberField(TEXT("magnet_y"),Magnet.Y);
    J->SetBoolField(TEXT("muted"),bMuted); J->SetBoolField(TEXT("continuous_field"),bContinuousField);
    J->SetObjectField(TEXT("rig"),Rig->ToJson()); J->SetObjectField(TEXT("world"),World->ToJson());
    if(bIncludeCheckpoint && SiteEntry.IsValid()) J->SetObjectField(TEXT("site_entry"),SiteEntry);
    return J;
}
bool FExpeditionRuntime::RestoreState(const TSharedPtr<FJsonObject>& J,FString& Error,bool bRestoreCheckpoint)
{
    FString Mode; double Schema=0, ScreenValue=0, Site=0, X=0,Y=0;
    const TSharedPtr<FJsonObject>* RigJson=nullptr; const TSharedPtr<FJsonObject>* WorldJson=nullptr;
    if(!J.IsValid() || !J->TryGetStringField(TEXT("mode"),Mode) || Mode!=TEXT("salvage_expedition")
        || !J->TryGetNumberField(TEXT("schema"),Schema) || Schema!=1
        || !J->TryGetNumberField(TEXT("screen"),ScreenValue) || ScreenValue<0 || ScreenValue>4 || ScreenValue!=int32(ScreenValue)
        || !J->TryGetNumberField(TEXT("site"),Site) || Site<0 || Site>3 || Site!=int32(Site)
        || !J->TryGetNumberField(TEXT("magnet_x"),X) || !J->TryGetNumberField(TEXT("magnet_y"),Y)
        || !FMath::IsFinite(X) || !FMath::IsFinite(Y) || FMath::Abs(X)>800 || FMath::Abs(Y)>400
        || !J->TryGetObjectField(TEXT("rig"),RigJson) || !J->TryGetObjectField(TEXT("world"),WorldJson))
    {Error=TEXT("This is not a valid expedition save. Legacy careers use a separate format."); return false;}
    auto NewRig=MakeUnique<FExpeditionRig>(); auto NewWorld=MakeUnique<FExpeditionWorld>();
    if(!NewRig->FromJson(**RigJson,Error) || !NewWorld->FromJson(*WorldJson,Error)) return false;
    // Capabilities follow the exact saved worksite; refreshing them never rerolls its existing stock.
    // This also adds newly explicit opportunity tags to a compatible older E1 depot.
    NewRig->SetShopContext(NewWorld->GetOpportunityTags(),ImplementedModules());
    const auto& NewState=NewWorld->GetState();
    const int64 SiteSeed=int64(NewRig->GetSeed())+int32(Site);
    if(NewRig->GetSiteIndex()!=int32(Site) || NewState.SiteIndex!=int32(Site)
        || SiteSeed>MAX_int32 || NewState.Seed!=SiteSeed || NewWorld->GetOutput()!=NewRig->GetOutput(int32(Site)))
    {Error=TEXT("Rig and worksite checkpoints disagree."); return false;}
    const auto NewScreen=EExpeditionScreen(int32(ScreenValue));
    const bool Won=NewRig->IsRunWon(), Awarded=NewRig->IsSiteAwarded(int32(Site));
    bool PhaseValid=false;
    if(NewScreen==EExpeditionScreen::Depot)
        PhaseValid=NewRig->IsAtDepot()&&!Won&&!Awarded&&!NewWorld->IsEnded()&&NewWorld->GetOutput()==0&&NewState.ActionSerial==0;
    else if(NewScreen==EExpeditionScreen::Site)
        PhaseValid=!NewRig->IsAtDepot()&&!Won&&!Awarded&&!NewWorld->IsEnded();
    else if(NewScreen==EExpeditionScreen::Victory)
        PhaseValid=int32(Site)==3&&Won&&Awarded&&NewState.bDispatched&&!NewState.bEvacuated;
    else if(NewScreen==EExpeditionScreen::Retreated)
        PhaseValid=!Won&&!Awarded&&NewState.bEvacuated&&!NewRig->IsAtDepot();
    if(!PhaseValid){Error=TEXT("Expedition screen disagrees with its actual progress.");return false;}
    TSharedPtr<FJsonObject> Checkpoint;
    if(bRestoreCheckpoint && NewScreen==EExpeditionScreen::Site)
    {
        const TSharedPtr<FJsonObject>* P=nullptr;
        if(!J->TryGetObjectField(TEXT("site_entry"),P)){Error=TEXT("Active expedition is missing its retry checkpoint."); return false;}
        FExpeditionRuntime Candidate(nullptr);
        if(!Candidate.RestoreState(*P,Error,false) || Candidate.SiteIndex!=int32(Site) || Candidate.Screen!=EExpeditionScreen::Site)
        {Error=TEXT("Invalid site-entry checkpoint: ")+Error; return false;}
        const auto& Entry=*Candidate.Rig; const auto& Before=Entry.GetRecords(); const auto& Now=NewRig->GetRecords();
        const auto& EntryWorld=Candidate.World->GetState();
        bool Matching=Entry.GetSeed()==NewRig->GetSeed() && Before.RunsStarted==Now.RunsStarted
            && Entry.GetFittedActives()==NewRig->GetFittedActives() && Entry.GetFittedPassives()==NewRig->GetFittedPassives()
            && Entry.IsPrecharged()==NewRig->IsPrecharged() && Entry.GetOutput(int32(Site))==0
            && EntryWorld.ActionSerial==0 && EntryWorld.Battery==FMath::RoundToInt(Entry.GetStartingBattery())
            && EntryWorld.LayoutId==NewState.LayoutId && EntryWorld.LayoutRevision==NewState.LayoutRevision
            && Before.RunsWon==Now.RunsWon && Before.BestOutput==Now.BestOutput
            && Before.DiscoveredModules==Now.DiscoveredModules && Before.UnlockedStarters==Now.UnlockedStarters
            && Before.LastWinningRig==Now.LastWinningRig;
        const auto& EntryInventory=Entry.GetInventory();const auto& CurrentInventory=NewRig->GetInventory();
        Matching &= Entry.GetOffers()==NewRig->GetOffers() && EntryInventory.Num()==CurrentInventory.Num();
        for(const auto& Item:EntryInventory)
            Matching &= CurrentInventory.ContainsByPredicate([&](const auto& Other){return Other.Id==Item.Id && Other.Paid==Item.Paid;});
        for(int32 I=0;I<int32(Site);++I)
            Matching &= Entry.GetOutput(I)==NewRig->GetOutput(I) && Entry.IsSiteAwarded(I)==NewRig->IsSiteAwarded(I);
        int32 Earned=0;
        if(int32(Site)<3)for(int32 M=0;M<2;++M)
            if(NewRig->GetOutput(int32(Site))>=FExpeditionRig::RefiningThreshold(int32(Site),M))Earned+=2;
        Matching &= Entry.GetCash()+Earned==NewRig->GetCash();
        if(!Matching){Error=TEXT("Retry checkpoint belongs to different equipment, worksite, rewards or run history.");return false;}
        Checkpoint=*P;
    }
    // Publish only after rig, world and checkpoint all validate.
    Rig=MoveTemp(NewRig); World=MoveTemp(NewWorld); SiteIndex=int32(Site); Screen=NewScreen; Magnet={X,Y}; Aim=Magnet;
    if(bRestoreCheckpoint) SiteEntry=Checkpoint;
    J->TryGetBoolField(TEXT("muted"),bMuted); J->TryGetBoolField(TEXT("continuous_field"),bContinuousField);
    bPaused=Screen==EExpeditionScreen::Site; Input.LoseFocus(); ClearPreparation(); SelectedCargoId=INDEX_NONE;WeldSelection.Reset();bPrepareWeld=false;bPrecision=false;Error.Empty(); return true;
}
FString FExpeditionRuntime::EncodeSave() const
{
    FString Text; auto W=TJsonWriterFactory<>::Create(&Text); FJsonSerializer::Serialize(MakeState(true).ToSharedRef(),W); return Text;
}
bool FExpeditionRuntime::DecodeSave(const FString& Text,FString& Error)
{
    TSharedPtr<FJsonObject> J; const auto R=TJsonReaderFactory<>::Create(Text);
    if(!FJsonSerializer::Deserialize(R,J)){Error=TEXT("Invalid JSON."); return false;}
    return RestoreState(J,Error,true);
}
bool FExpeditionRuntime::Load()
{
    if(SavePath.IsEmpty()) return false;
    FString Text,Error;
    if(FFileHelper::LoadFileToString(Text,*SavePath) && DecodeSave(Text,Error)) return true;
    if(FFileHelper::LoadFileToString(Text,*(SavePath+TEXT(".bak"))) && DecodeSave(Text,Error))
    {bLoadedBackup=true; return true;}
    if(IFileManager::Get().FileExists(*SavePath))
    {bSaveAllowed=false; Show(TEXT("Expedition save could not be restored. It is preserved; start a new run explicitly to replace it."));}
    return false;
}
bool FExpeditionRuntime::Save()
{
    if(SavePath.IsEmpty() || !bSaveAllowed || Screen==EExpeditionScreen::Starter) return false;
    const FString Text=EncodeSave(); FString Error; FExpeditionRuntime Check(nullptr);
    if(!Check.DecodeSave(Text,Error)){UE_LOG(LogTemp,Error,TEXT("EXPEDITION_SAVE_REFUSED %s"),*Error); return false;}
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(SavePath),true);
    const FString Temp=SavePath+TEXT(".tmp");
    if(!FFileHelper::SaveStringToFile(Text,*Temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) return false;
    if(!bLoadedBackup && IFileManager::Get().FileExists(*SavePath))
        if(IFileManager::Get().Copy(*(SavePath+TEXT(".bak")),*SavePath,true,true)!=COPY_OK) return false;
    const bool Success=IFileManager::Get().Move(*SavePath,*Temp,true,true,false,true);
    if(Success) bLoadedBackup=false; return Success;
}

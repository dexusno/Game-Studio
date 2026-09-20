#include "FoundryHost.h"
#if FOUNDRY_WITH_CAMPAIGN
#include "FoundryCampaign.h"
#include "FoundryCampaignUI.h"
#include "FoundryRecipeControlProbe.h"
#include "FoundryProfileProbe.h"
#include "FoundryPrecisionProbe.h"
#include "FoundryCityProbe.h"
#include "Widgets/SWidget.h"
#include "overkill/robots.hpp"
#include <algorithm>

DEFINE_LOG_CATEGORY_STATIC(LogFoundryCampaignProbe, Log, All);
void AFoundryStage::TickCampaignProbe(float DeltaSeconds)
{
    if(TickFoundryCityProbe(*this,[this](const FString& Name){CaptureNamed(Name);}))return;
    if(!Campaign) return;
    CampaignProbeElapsed+=DeltaSeconds;
    if(CampaignProbeElapsed<1.0f || IsPresentationBusy()) return;
    CampaignProbeElapsed=0;
    auto Check=[&](bool Ok,const TCHAR* Label)
    {
        UE_LOG(LogFoundryCampaignProbe,Display,TEXT("CAMPAIGN_CHECK ok=%d %s"),Ok,Label);
        if(!Ok){UE_LOG(LogFoundryCampaignProbe,Error,TEXT("FOUNDRY_CAMPAIGN_PROBE_COMPLETE ok=0 step=%d reason=%s"),CampaignProbeStep,*Campaign->Message);FPlatformMisc::RequestExit(false);}
        return Ok;
    };
    auto Act=[&](overkill::CampaignActionType Type,overkill::Id Subject=0,std::string Choice={})
    {overkill::CampaignAction A;A.type=Type;A.subject=Subject;A.choice=std::move(Choice);return Campaign->Apply(A);};
    using CT=overkill::CampaignActionType;
    switch(CampaignProbeStep)
    {
    case 0: {FString Report;bool Ok=RunFoundryProfileProbe(Report);if(!Check(Ok,*Report))return;Ok=RunFoundryRecipeControlProbe(Report);if(!Check(Ok,*Report))return;Ok=RunFoundryPrecisionPersistenceProbe(Report);if(!Check(Ok,*Report))return;CaptureNamed(TEXT("campaign-title.png"));break;}
    case 1: if(!Check(Campaign->StartNew(false,20260920),TEXT("New Game persisted")))return;break;
    case 2: CaptureNamed(TEXT("campaign-mayor.png"));break;
    case 3:
    {
        // Select a saved offer with no nested choice for this short UI route.
        // This is scripted QA selection, never a production policy.
        const auto Original=*Campaign->Current();std::string Selected;
        for(const auto& Id:Original.mayorOffers)
        {
            auto Candidate=Original;overkill::CampaignAction A;A.type=CT::ChooseMayor;A.runId=Candidate.runId;A.sequence=Candidate.nextTransaction;A.choice=Id;
            const auto R=Campaign->Rules.apply(Candidate,A);
            if(R.ok && Candidate.fight.upgradeChoices.empty() && Candidate.upgradeOffers.empty()){Selected=Id;break;}
        }
        if(!Check(!Selected.empty() && Act(CT::ChooseMayor,0,Selected),TEXT("Saved Mayor choice applied with production hooks")))return;break;
    }
    case 4: CaptureNamed(TEXT("campaign-route.png"));break;
    case 5: Campaign->Navigate(TEXT("shop"));break;
    case 6: CaptureNamed(TEXT("campaign-shop.png"));break;
    case 7:
    {
        const auto Product=Campaign->Current()->shop.front();const auto Before=Campaign->Current()->fight.credits;
        if(!Check(Act(CT::Buy,Product.id),TEXT("Finite shop purchase saved")))return;
        if(!Check(Campaign->Current()->shop.front().quantity==Product.quantity-1 && Campaign->Current()->fight.credits==Before-Campaign->Hooks.productPrice(*Campaign->Current(),Product),TEXT("Stock and authoritative price update together")))return;
        Campaign->Close();break;
    }
    case 8:
    {
        const auto Offers=overkill::routeOffers(Campaign->Current()->route);overkill::Id Id=0;
        for(const auto& O:Offers)if(O.formation=="C1-F-MITE-RAM")Id=O.id;
        if(!Id)for(const auto& O:Offers)if(O.kind==overkill::EncounterKind::Regular){Id=O.id;break;}
        if(!Check(Id && Act(CT::EnterOffer,Id),TEXT("Route enters actual formation")))return;break;
    }
    case 9: CaptureNamed(TEXT("campaign-collection.png"));break;
    case 10:
    {
        const auto EntryHash=overkill::stateHash(Session->State);
        const auto CampaignHash=overkill::campaignHash(*Campaign->Current());
        if(!Check(Campaign->BeginPrecision(),TEXT("Precision opens from a legal Collect preview")))return;
        Campaign->Control(TEXT("collect"));Campaign->Control(TEXT("back"));Campaign->Control(TEXT("panels"));
        if(!Check(Campaign->bPrecisionModal && Campaign->Page==TEXT("combat") && Campaign->Drawer.IsEmpty() && overkill::campaignHash(*Campaign->Current())==CampaignHash,TEXT("Precision modal blocks underlying commands without a transaction")))return;
        Campaign->CancelPrecision(Campaign->PrecisionAttempt);
        if(!Check(!Campaign->bPrecisionModal && overkill::campaignHash(*Campaign->Current())==CampaignHash,TEXT("Before-start Precision cancellation leaves the campaign unchanged")))return;
        Session->Steering=3;
        if(!Check(Campaign->Control(TEXT("collect")),TEXT("Ordinary haul uses campaign combat action")))return;
        if(!Check(Campaign->Continue() && overkill::stateHash(Session->State)==EntryHash,TEXT("Continue restores original fight entry exactly")))return;
        break;
    }
    case 11: Control(TEXT("collect"));Campaign->Drawer=TEXT("recipes");++Campaign->ViewRevision;break;
    case 12: CaptureNamed(TEXT("campaign-recipes.png"));break;
    case 13:
    {
        Campaign->Drawer.Empty();++Campaign->ViewRevision;
        if(!Check(!Campaign->Control(TEXT("fire")),TEXT("Invalid Fire is rejected without a speculative save")))return;
        const auto Before=overkill::campaignHash(*Campaign->Current());Control(TEXT("load"));
        if(!Check(!IsActionView()&&!IsPresentationBusy()&&overkill::campaignHash(*Campaign->Current())==Before,TEXT("Invalid Lock and load leaves preparation and save unchanged")))return;
        const auto Widget=MakeFoundryCampaignUI(this);
        const auto Reply=Widget->OnKeyDown(FGeometry(),FKeyEvent(EKeys::Enter,FModifierKeysState(),0,true,0,0));
        if(!Check(Reply.IsEventHandled()&&overkill::campaignHash(*Campaign->Current())==Before,TEXT("Slate repeat Enter is consumed without End Turn after modal focus return")))return;
        break;
    }
    case 14:
    {
        const auto& State=Session->State;
        if(Campaign->Current()->phase==overkill::CityPhase::Rewards){CaptureNamed(TEXT("campaign-victory.png"));break;}
        if(!Check(Campaign->Current()->phase==overkill::CityPhase::Fight && State.round<=16,TEXT("Legal fight remains live and bounded")))return;
        if(State.phase==overkill::Phase::Collection){Control(TEXT("collect"));return;}
        if(!State.bullet.empty())
        {
            if(!Check(IsActionView()&&!IsPresentationBusy(),TEXT("Loaded shooting view is settled and interactive before Fire")))return;
            if(CameraProbeStep==0){CameraProbeStep=1;CaptureNamed(TEXT("campaign-locked-and-loaded.png"));return;}
            if(CameraProbeStep==1){Campaign->Drawer=TEXT("recipes");++Campaign->ViewRevision;CameraProbeStep=2;return;}
            if(CameraProbeStep==2){Campaign->Close();if(!Check(IsActionView()&&Campaign->Drawer.IsEmpty(),TEXT("Closing Recipes returns to loaded shooting view")))return;CameraProbeStep=3;return;}
            if(CameraProbeStep==3){Campaign->Navigate(TEXT("shop"));CameraProbeStep=4;return;}
            if(CameraProbeStep==4){Campaign->Close();if(!Check(IsActionView()&&Campaign->Page==TEXT("combat"),TEXT("Closing shop returns to loaded shooting view")))return;CameraProbeStep=5;return;}
            if(CameraProbeStep==5){Control(TEXT("unload"));if(!Check(!IsActionView()&&Session->State.bullet.empty(),TEXT("Unload returns to preparation with unfired parts")))return;CameraProbeStep=6;return;}
            if(CameraProbeStep==6){CameraProbeStep=7;CaptureNamed(TEXT("campaign-reloaded.png"));return;}
            int32 Best=MAX_int32;for(const auto& E:State.enemies)if(!E.dead&&!E.escaped&&E.hp<Best){Best=E.hp;Session->Target=E.id;}
            const auto Revision=Campaign->Store.revision();Control(TEXT("fire"));
            if(!Check(Campaign->Store.revision()==Revision+1,TEXT("Loaded shot committed through camera-aware control")))return;
            CaptureNamed(TEXT("campaign-action.png"));CaptureNamed(TEXT("campaign-action-settled.png"));return;
        }
        for(const auto& P:State.parts)if(P.kind==overkill::Kind::Shield&&P.place==overkill::Place::Reserve)
        {if(!Check(Campaign->Control(FString::Printf(TEXT("part:%llu"),P.id)),TEXT("Shield installed through campaign")))return;return;}
        for(const char* Name:{"SH004","SH001","SH003","SH002"})for(const auto& Copy:State.memory)if(Copy.recipe==Name)
            if(Session->Preview(overkill::Action::craft(Copy.id)).result.ok)
            {Control(FString::Printf(TEXT("craft:%llu"),Copy.id));return;}
        Session->Selection.clear();for(const auto& P:State.parts)if(P.kind==overkill::Kind::Ammo&&P.place==overkill::Place::Reserve)Session->Selection.push_back(P.id);
        if(!Session->Selection.empty())
        {
            if(!RequestedCaptures.Contains(TEXT("campaign-parts.png"))){Campaign->Drawer=TEXT("parts");++Campaign->ViewRevision;CaptureNamed(TEXT("campaign-parts.png"));return;}
            Control(TEXT("load"));if(!Check(IsActionView()&&!Session->State.bullet.empty(),TEXT("Lock and load starts shooting camera without firing")))return;
            const auto LoadedHash=overkill::campaignHash(*Campaign->Current());Control(TEXT("fire"));
            if(!Check(overkill::campaignHash(*Campaign->Current())==LoadedHash,TEXT("Fire input is locked only during camera/load transition")))return;return;
        }
        Control(TEXT("end"));CaptureNamed(TEXT("campaign-enemy-turn.png"));return;
    }
    case 15: if(!Check(Act(CT::RequestAdvance) && Campaign->Current()->skipConfirmation,TEXT("Unclaimed rewards require explicit abandonment")))return;break;
    case 16: CaptureNamed(TEXT("campaign-skip-confirm.png"));break;
    case 17:
    {
        if(!Check(Act(CT::CancelAdvance),TEXT("Cancel preserves rewards")))return;
        const auto Rewards=Campaign->Current()->rewards;
        for(const auto& R:Rewards)if(R.kind==overkill::RewardKind::Cores)if(!Check(Act(CT::ClaimReward,R.id),TEXT("Energy cores claimed once")))return;
        for(const auto& R:Rewards)if(R.kind==overkill::RewardKind::Recipe){if(!Check(Act(CT::OpenRecipes,R.id),TEXT("Opening recipe offer records discoveries")))return;break;}
        break;
    }
    case 18:CaptureNamed(TEXT("campaign-recipe-reward.png"));break;
    case 19:
    {
        const auto Rewards=Campaign->Current()->rewards;
        for(const auto& R:Rewards)if(R.kind==overkill::RewardKind::Recipe && R.id==Campaign->Current()->recipeWindow)
        {if(!Check(Act(CT::ClaimReward,R.id,R.choices.front()),TEXT("Selected recipe enters memory")))return;break;}
        if(!Check(Act(CT::RequestAdvance) && Campaign->Current()->phase==overkill::CityPhase::Between,TEXT("Reward completion reaches next route position")))return;
        break;
    }
    case 20:CaptureNamed(TEXT("campaign-next-route.png"));break;
    case 21:
    {
        overkill::CampaignSession Reload(std::filesystem::path(*Campaign->SavePath),Campaign->Rules);const auto Loaded=Reload.load();
        if(!Check(Loaded.ok && overkill::campaignHash(Reload.state())==overkill::campaignHash(*Campaign->Current()),TEXT("Disk reload equals displayed committed campaign")))return;
        FFoundrySession ClosedCombat(1);FFoundryCampaign ClosedCampaign(ClosedCombat,Campaign->SavePath);ClosedCampaign.Close();
        if(!Check(ClosedCampaign.Page==TEXT("title") && !ClosedCampaign.bCanResumeInMemory,TEXT("Fresh startup Escape cannot bypass Continue")))return;
        UE_LOG(LogFoundryCampaignProbe,Display,TEXT("FOUNDRY_CAMPAIGN_PROBE_COMPLETE ok=1 position=%d hp=%d receipts=%llu revision=%llu hash=%s screenshots=%d physical_input=0"),
            Campaign->Current()->route.position,Campaign->Current()->fight.hp,static_cast<uint64>(Campaign->Current()->receipts.size()),static_cast<uint64>(Campaign->Store.revision()),UTF8_TO_TCHAR(overkill::campaignHash(*Campaign->Current()).c_str()),RequestedCaptures.Num());
        FPlatformMisc::RequestExit(false);break;
    }
    }
    ++CampaignProbeStep;
}
#else
void AFoundryStage::TickCampaignProbe(float) {}
#endif

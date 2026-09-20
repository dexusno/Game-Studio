#include "FoundryRecipeControlProbe.h"
#if FOUNDRY_WITH_CAMPAIGN
#include "FoundryCampaign.h"
#include "HAL/FileManager.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include <algorithm>
#include <stdexcept>

DEFINE_LOG_CATEGORY_STATIC(LogFoundryRecipeProbe,Log,All);
bool RunFoundryRecipeControlProbe(FString& Report)
{
    int32 Checks=0;
    try
    {
        using namespace overkill;
        auto Verify=[&](bool Ok,const char* Label)
        {++Checks;UE_LOG(LogFoundryRecipeProbe,Display,TEXT("RECIPE_CONTROL_CHECK ok=%d %s"),Ok,UTF8_TO_TCHAR(Label));if(!Ok)throw std::runtime_error(Label);};
        FFoundrySession Controls(20260920);
        CampaignRules Setup(Controls.Rules,cinderwallUpgradeHooks(Controls.Rules));
        Campaign Entry;
        for(uint64 Seed=1;Seed<512;++Seed)
        {Entry=Setup.newGame(Seed,"prepared-recipe-controls");if(std::find(Entry.mayorOffers.begin(),Entry.mayorOffers.end(),"MY1-17")!=Entry.mayorOffers.end())break;}
        auto Prepare=[&](Campaign& C,CampaignAction A)
        {A.runId=C.runId;A.sequence=C.nextTransaction;const auto R=Setup.apply(C,A);if(!R.ok)throw std::runtime_error(R.reason);};
        auto Fight=[&](Campaign& C,const Action& A)
        {CampaignAction Command;Command.type=CampaignActionType::Combat;Command.combat=A;Prepare(C,Command);};
        CampaignAction A;A.type=CampaignActionType::ChooseMayor;A.choice="MY1-17";Prepare(Entry,A);
        A={};A.type=CampaignActionType::EnterOffer;
        for(const auto& O:routeOffers(Entry.route))if(O.formation=="C1-F-MITE-RAM"){A.subject=O.id;break;}
        Verify(A.subject!=0,"Prepared fixture starts from a real Mite/Ram route offer");Prepare(Entry,A);
        auto Grant=[&](Campaign& C,const char* Recipe)
        {const auto First=C.fight.nextId;const auto R=Controls.Rules.grantPart(C.fight,Recipe);if(!R.ok)throw std::runtime_error(R.reason);for(const auto& P:C.fight.parts)if(P.id>=First&&P.recipe==Recipe)return P.id;throw std::runtime_error("Prepared part missing");};
        auto Protection=[&](Campaign& C)
        {const auto R=Controls.Rules.grantPlainPart(C.fight,Kind::Shield,80,"Prepared command probe",true);if(!R.ok)throw std::runtime_error(R.reason);};
        const FString Directory=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("RecipeControlProbe"),FGuid::NewGuid().ToString(EGuidFormats::Digits));
        IFileManager::Get().MakeDirectory(*Directory,true);
        auto Save=[&](const Campaign& C,const TCHAR* Name)
        {const FString Path=FPaths::ConvertRelativePathToFull(FPaths::Combine(Directory,Name));SaveStore Disk{std::filesystem::path(*Path)};const auto R=Disk.commit(serializeCampaign(C),"");if(!R.ok)throw std::runtime_error(R.error);return Path;};
        auto Ready=Entry;Fight(Ready,Action::collect(3));Ready.fight.materials={100,100,100,100,100};Ready.fight.heat=8;Protection(Ready);
        // Deliberately enlarged health confines this transaction probe to a
        // live encounter. It is neither a rendered formation nor balance proof.
        for(auto& E:Ready.fight.enemies)E.hp=E.maxHp=1000;
        const Id Sacrifice=Grant(Ready,"SH001"),Clamp=Grant(Ready,"MA022"),Variable=Grant(Ready,"MA096");
        const Id Modifier=Grant(Ready,"SH085"),Support=Grant(Ready,"SH017"),Spread=Grant(Ready,"SH110"),Payment=Grant(Ready,"SH002");
        {
            FFoundryCampaign Live(Controls,Save(Ready,TEXT("builder.ofsave")));Live.Page=Live.PhasePage();
            auto Use=Action{};Use.type=ActionType::Activate;Use.subject=Modifier;Controls.Drafts.set(Use,"amount",{15});
            auto Expected=Controls.Preview(Controls.PartAction(Modifier));const auto FirstRevision=Live.Store.revision();
            Verify(Expected.result.ok&&Live.Control(FString::Printf(TEXT("part:%llu"),Modifier)),"Shared modifier control submits the explicit amount");
            Verify(stateHash(Controls.State)==stateHash(Expected.state)&&Live.Store.revision()==FirstRevision+1,"Configured modifier preview equals one saved transaction");
            Controls.Drafts.set(Action::install(Clamp),"sacrifice",{Sacrifice});Expected=Controls.Preview(Controls.PartAction(Clamp));
            Verify(Expected.result.ok&&Live.Control(FString::Printf(TEXT("part:%llu"),Clamp)),"Shared install control submits the selected victim");
            Verify(stateHash(Controls.State)==stateHash(Expected.state),"Selected install sacrifice matches preview");
            Controls.Drafts.set(Action::install(Variable),"amount",{6});Expected=Controls.Preview(Controls.PartAction(Variable));
            Verify(Expected.result.ok&&Live.Control(FString::Printf(TEXT("part:%llu"),Variable)),"Variable Heat installation uses the same builder");
            Verify(stateHash(Controls.State)==stateHash(Expected.state),"Variable installation matches the quoted payment");
            Controls.Selection={Support,Spread};Controls.Drafts.set(Action::load(Controls.Selection),"sacrifice-"+std::to_string(Spread),{Payment});
            Expected=Controls.Preview(Controls.LoadAction());
            Verify(Expected.result.ok&&Live.Control(TEXT("load")),"Shared Load control preserves physical Shield reservation");
            Verify(stateHash(Controls.State)==stateHash(Expected.state),"Reserved Load matches preview");
            Verify(Live.Control(TEXT("unload")),"Unload returns the reserved payment");
            Controls.Selection={Spread,Support};Expected=Controls.Preview(Controls.LoadAction());
            Verify(Expected.result.ok&&Live.Control(TEXT("load"))&&stateHash(Controls.State)==stateHash(Expected.state),"Reordered reload keeps payment assigned to its source part");
            Verify(foundry_controls::loadedDisplayOrder(Controls.State)==std::vector<Id>({Spread,Payment,Support}),"Loaded display follows reversed firing order and nests its reserved payment");
            Controls.Target=Controls.State.enemies.back().id;Controls.Drafts.set(Action::fire(Controls.Target),"extra-"+std::to_string(Support),{Controls.State.enemies.front().id});
            Expected=Controls.Preview(Controls.FireAction());const auto Round=Controls.State.round;
            Verify(Expected.result.ok&&Live.Control(TEXT("fire")),"Shared Fire control submits per-part support target");
            Verify(stateHash(Controls.State)==stateHash(Expected.state)&&Controls.State.round==Round,"Configured Fire matches preview and preserves the player turn");
            CampaignSession Reload(std::filesystem::path(*Live.SavePath),Live.Rules);const auto Loaded=Reload.load();
            Verify(Loaded.ok&&campaignHash(Reload.state())==campaignHash(*Live.Current()),"Configured requests survive production SaveStore reload");
        }
        auto Heavy=Entry;Fight(Heavy,Action::collect(3));Protection(Heavy);const auto Magnet=Grant(Heavy,"SH105");
        Action Fit;Fit.type=ActionType::Activate;Fit.subject=Magnet;Fight(Heavy,Fit);Fight(Heavy,Action::endTurn());
        {
            FFoundryCampaign Live(Controls,Save(Heavy,TEXT("heavy.ofsave")));Live.Page=Live.PhasePage();Controls.Steering=3;
            const auto Before=campaignHash(*Live.Current());const auto Revision=Live.Store.revision();
            Verify(!Live.Control(TEXT("collect"))&&Live.bCollectionChoice&&!Live.bPrecisionModal,"Ordinary Heavy Magnet opens choices before collecting");
            Live.CancelCollectionChoice();Verify(!Live.bCollectionChoice&&campaignHash(*Live.Current())==Before,"Ordinary discard cancellation has no transaction");
            Verify(Live.BeginPrecision(),"Heavy Magnet allows the actual Precision widget path");const auto Attempt=Live.PrecisionAttempt;
            // A synthetic callback tests persistence/control guards only. The
            // native timing pass separately measures real keyboard input.
            Verify(!Live.FinishPrecision(Attempt,2)&&Live.bPrecisionModal&&Live.bCollectionChoice,"Measured Precision waits for the required discard");
            Verify(campaignHash(*Live.Current())==Before&&Live.Store.revision()==Revision,"Measurement does not award or save an incomplete haul");
            Live.CancelPrecision(Attempt);Live.CancelCollectionChoice();Live.Control(TEXT("back"));Live.Control(TEXT("end"));Live.Close();Live.Navigate(TEXT("shop"));
            Verify(Live.bPrecisionModal&&Live.bCollectionChoice&&Live.Page==TEXT("combat")&&!Live.Continue()&&!Live.BeginPrecision(),"Measured discard blocks cancel, reopen and underlying actions");
            Verify(!Live.FinishPrecision(Attempt,0)&&Live.PrecisionResult==2,"A duplicate timing callback cannot replace the measured result");
            Controls.Steering=0;const auto Chosen=Live.CollectionOptions.back();
            Verify(Chosen.action.precision==2&&Chosen.action.steering==3,"Discard choice retains the original steering and measured category");
            Verify(Live.CommitCollectionChoice(static_cast<int32>(Live.CollectionOptions.size()-1)),"One completed measured collection is saved");
            Verify(stateHash(Controls.State)==stateHash(Chosen.preview.state)&&Live.Store.revision()==Revision+1,"Completed measured collection matches its exact core preview");
            const auto Saved=campaignHash(*Live.Current());
            Verify(!Live.CommitCollectionChoice(0)&&!Live.FinishPrecision(Attempt,2)&&campaignHash(*Live.Current())==Saved,"No duplicate haul or award after confirmation");
            Verify(Live.Continue()&&stateHash(Controls.State)==stateHash(Entry.fight),"Continue retains the original entry restart contract");
        }
        auto UtilityKill=Ready;for(auto& Enemy:UtilityKill.fight.enemies)Enemy.hp=1;
        RecipeCopy Vent;Vent.id=UtilityKill.fight.nextId++;Vent.recipe="MA058";UtilityKill.fight.memory.push_back(Vent);
        {
            FFoundryCampaign Live(Controls,Save(UtilityKill,TEXT("utility-kill.ofsave")));Live.Page=Live.PhasePage();Controls.CommittedEvents.clear();
            const auto Before=Live.Store.revision();const auto Preview=Controls.Preview(Controls.CraftAction(Vent.id));
            Verify(Preview.result.ok&&Live.Control(FString::Printf(TEXT("craft:%llu"),Vent.id)),"Real MA058 Utility Use goes through the shared saved recipe builder");
            Verify(Preview.state.phase==Phase::Victory&&Live.Current()->phase==CityPhase::Rewards&&Live.Store.revision()==Before+1,"Killing Utility request publishes one reward transition after canonical fight cleanup");
            Verify(std::count_if(Controls.CommittedEvents.begin(),Controls.CommittedEvents.end(),[](const Event& E){return E.type=="enemy_death";})==2&&std::none_of(Controls.CommittedEvents.begin(),Controls.CommittedEvents.end(),[](const Event& E){return E.type=="fire";}),"Utility emits both deaths without a Fire event; rendered hold is a separate Stage check");
        }
        Report=FString::Printf(TEXT("Prepared recipe builder and measured collection contracts: %d checks; no native timing or rendered-layout claim."),Checks);
        UE_LOG(LogFoundryRecipeProbe,Display,TEXT("RECIPE_CONTROL_PROBE_COMPLETE ok=1 checks=%d prepared=1 saves=%s"),Checks,*Directory);return true;
    }
    catch(const std::exception& Error)
    {Report=UTF8_TO_TCHAR(Error.what());UE_LOG(LogFoundryRecipeProbe,Error,TEXT("RECIPE_CONTROL_PROBE_COMPLETE ok=0 checks=%d reason=%s"),Checks,*Report);return false;}
}
#endif

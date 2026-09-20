#include "FoundryPrecisionProbe.h"
#if FOUNDRY_WITH_CAMPAIGN
#include "FoundryCampaign.h"
#include "HAL/FileManager.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>

DEFINE_LOG_CATEGORY_STATIC(LogFoundryPrecisionProbe,Log,All);

// This opt-in campaign preflight exercises the real UI model and Windows
// SaveStore. Results are synthetic callbacks, not measured player timing.
bool RunFoundryPrecisionPersistenceProbe(FString& Report)
{
    int32 Checks=0;
    try
    {
        using namespace overkill;
        auto Verify=[&](bool Ok,const char* Label)
        {++Checks;UE_LOG(LogFoundryPrecisionProbe,Display,TEXT("PRECISION_SAVE_CHECK ok=%d %s"),Ok,UTF8_TO_TCHAR(Label));if(!Ok)throw std::runtime_error(Label);};
        FFoundrySession Controls(20260920);
        CampaignRules Setup(Controls.Rules,cinderwallUpgradeHooks(Controls.Rules));
        Campaign Entry;
        for(uint64 Seed=1;Seed<512;++Seed)
        {Entry=Setup.newGame(Seed,"prepared-precision-persistence");if(std::find(Entry.mayorOffers.begin(),Entry.mayorOffers.end(),"MY1-17")!=Entry.mayorOffers.end())break;}
        auto Prepare=[&](Campaign& C,CampaignAction A)
        {A.runId=C.runId;A.sequence=C.nextTransaction;const auto R=Setup.apply(C,A);if(!R.ok)throw std::runtime_error(R.reason);};
        auto Fight=[&](Campaign& C,const Action& A)
        {CampaignAction Command;Command.type=CampaignActionType::Combat;Command.combat=A;Prepare(C,Command);};
        CampaignAction A;A.type=CampaignActionType::ChooseMayor;A.choice="MY1-17";Prepare(Entry,A);
        A={};A.type=CampaignActionType::EnterOffer;
        for(const auto& O:routeOffers(Entry.route))if(O.formation=="C1-F-MITE-RAM"){A.subject=O.id;break;}
        Verify(A.subject!=0,"Prepared persistence fixture has a real Mite/Ram entry and retry Mayor");Prepare(Entry,A);

        auto Retry=Entry;Fight(Retry,Action::collect(3,0));
        Verify(Retry.fight.upgradeChoices.size()==1&&Retry.fight.upgradeChoices.front().kind==UpgradeChoiceKind::PrecisionRetry,"Actual Miss produces the saved retry choice");
        auto Heavy=Entry;Fight(Heavy,Action::collect(3));
        Verify(Controls.Rules.grantPlainPart(Heavy.fight,Kind::Shield,80,"Prepared persistence protection",true).ok,"Prepared protection isolates discard persistence");
        const auto FirstPart=Heavy.fight.nextId;
        Verify(Controls.Rules.grantPart(Heavy.fight,"SH105").ok,"Prepared Heavy Magnet materializes through the production factory");
        Id Magnet=0;for(const auto& Part:Heavy.fight.parts)if(Part.id>=FirstPart&&Part.recipe=="SH105")Magnet=Part.id;
        Verify(Magnet!=0,"Prepared Heavy Magnet has a physical identity");
        Action Fit;Fit.type=ActionType::Activate;Fit.subject=Magnet;Fight(Heavy,Fit);Fight(Heavy,Action::endTurn());
        Verify(foundry_controls::heavyLiftPending(Heavy.fight),"Actual activation and End Turn produce the next collection choice");

        const FString Directory=FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("PrecisionPersistenceProbe"),FGuid::NewGuid().ToString(EGuidFormats::Digits)));
        IFileManager::Get().MakeDirectory(*Directory,true);
        auto Save=[&](const Campaign& C,const TCHAR* Name)
        {const FString Path=FPaths::Combine(Directory,Name);SaveStore Disk{std::filesystem::path(*Path)};const auto R=Disk.commit(serializeCampaign(C),"");if(!R.ok)throw std::runtime_error(R.error);return Path;};
        auto Bytes=[](const FString& Path)
        {std::ifstream File(std::filesystem::path(*Path),std::ios::binary);if(!File)throw std::runtime_error("Read probe save bytes");return std::string(std::istreambuf_iterator<char>(File),std::istreambuf_iterator<char>());};

        for(int32 Mode=0;Mode<3;++Mode)
        {
            const Campaign& Prepared=Mode==1?Retry:Mode==2?Heavy:Entry;
            const TCHAR* Name=Mode==1?TEXT("retry.ofsave"):Mode==2?TEXT("heavy.ofsave"):TEXT("normal.ofsave");
            const FString Path=Save(Prepared,Name);
            FFoundryCampaign Live(Controls,Path);Live.Page=Live.PhasePage();Controls.Steering=3;Controls.CommittedEvents.clear();
            Verify(Live.Current()!=nullptr&&Live.Page==TEXT("combat"),"Production model loads the prepared saved encounter");
            const auto StateBefore=serializeCampaign(*Live.Current());
            const auto BytesBefore=Bytes(Path);const auto RevisionBefore=Live.Store.revision();
            const Id Choice=Mode==1?Live.Current()->fight.upgradeChoices.front().id:0;
            Verify(Live.BeginPrecision(Choice),"Ordinary, saved retry or Heavy Precision opens through the production model");
            const auto Attempt=Live.PrecisionAttempt;
            Verify(!Live.FinishPrecision(Attempt-1,2)&&!Live.FinishPrecision(Attempt,3)&&Live.PrecisionResult==-1,"Stale and invalid callbacks cannot submit a measured category");
            Live.CancelPrecision(Attempt-1);
            Verify(Live.bPrecisionModal&&Live.Store.revision()==RevisionBefore,"Stale cancellation leaves the active attempt and disk untouched");
            Controls.Steering=0;
            Verify(Live.PrecisionSteering==3,"The original material steering is fixed before measuring");
            if(Mode==2)
            {
                Verify(!Live.FinishPrecision(Attempt,2)&&Live.bCollectionChoice&&Live.PrecisionResult==2,"Heavy Precision fixes the category before requesting discards");
                Verify(Bytes(Path)==BytesBefore&&serializeCampaign(*Live.Current())==StateBefore,"Measured Heavy category has not awarded or saved an incomplete haul");
                Verify(!Live.CollectionOptions.empty()&&Live.CollectionOptions.back().action.precision==2&&Live.CollectionOptions.back().action.steering==3,"Discard candidates retain the measured category and original steering");
            }
            {
                // Holding any read handle prevents SaveStore's zero-sharing
                // lock acquisition on Windows. It does not lock the save data,
                // so the failed writer can reconcile the unchanged envelope.
                std::ifstream Held(std::filesystem::path(*(Path+TEXT(".lock"))),std::ios::binary);
                Verify(Held.is_open(),"A real Windows handle holds the isolated save lock");
                const bool Saved=Mode==2?Live.CommitCollectionChoice(static_cast<int32>(Live.CollectionOptions.size()-1)):Live.FinishPrecision(Attempt,2);
                Verify(!Saved&&Live.Message.Contains(TEXT("Lock profile save")),"Actual SaveStore lock conflict rejects the measured transaction");
            }
            Verify(Live.bPrecisionModal&&!Live.bCollectionChoice&&Live.PrecisionResult==2,"Failed save retains one frozen measured result");
            Verify(Bytes(Path)==BytesBefore&&serializeCampaign(*Live.Current())==StateBefore&&Live.Store.revision()==RevisionBefore,"Failed award preserves exact disk bytes, campaign state and revision");
            Verify(Controls.CommittedEvents.empty(),"Failed award publishes no committed presentation or reward events");
            CampaignSession Reload(std::filesystem::path(*Path),Live.Rules);
            Verify(Reload.load().ok&&serializeCampaign(Reload.state())==StateBefore,"Independent production reload sees the original haul and receipt state");
            Verify(!Live.FinishPrecision(Attempt,0)&&!Live.FinishPrecision(Attempt+1,2)&&!Live.BeginPrecision(Choice),"Repeated callbacks and reopening cannot reroll the failed result");
            Verify(!Live.CommitCollectionChoice(0)&&!Live.Control(TEXT("collect"))&&!Live.Control(TEXT("end"))&&!Live.Continue()&&!Live.StartNew(true,999),"Failed result blocks extra collections, turns, Continue and replacement underneath the modal");
            CampaignAction Underlying;Underlying.type=CampaignActionType::Combat;Underlying.combat=Action::collect(3,2);
            Verify(!Live.Apply(Underlying),"Typed campaign commands cannot bypass the failed-result modal");
            Live.CancelPrecision(Attempt);Live.CancelCollectionChoice();Live.Close();Live.Navigate(TEXT("shop"));
            Verify(Live.bPrecisionModal&&Live.Page==TEXT("combat")&&Live.Drawer.IsEmpty()&&Live.PrecisionResult==2,"Cancel, Back and navigation retain the failed measured category");
            Verify(Bytes(Path)==BytesBefore&&serializeCampaign(*Live.Current())==StateBefore&&Controls.CommittedEvents.empty(),"Blocked attempts leave both state and emitted events untouched");
            Live.LeaveFailedPrecision();
            Verify(!Live.bPrecisionModal&&Live.Page==TEXT("title")&&!Live.bCanResumeInMemory&&Live.Store.revision()==RevisionBefore,"The failure exit returns to title without granting resources or bypassing Continue");
            Verify(Live.Continue()&&Live.Page==TEXT("combat")&&stateHash(Controls.State)==stateHash(Entry.fight),"Continue after failure restores the exact original encounter entry");
            Verify(!Live.FinishPrecision(Attempt,2),"A callback from the closed failed attempt cannot change restarted play");
            Controls.Steering=3;Controls.CommittedEvents.clear();
            Verify(Live.BeginPrecision()&&Live.PrecisionAttempt>Attempt,"A new attempt is possible only after the ordinary encounter restart");
            const auto NewAttempt=Live.PrecisionAttempt;const auto BeforeSuccess=Live.Store.revision();
            const auto Expected=Controls.Preview(Live.PrecisionAction(2));
            Verify(Expected.result.ok&&Live.FinishPrecision(NewAttempt,2),"Once the real lock is released the normal saved result succeeds");
            Verify(stateHash(Controls.State)==stateHash(Expected.state)&&Live.Store.revision()==BeforeSuccess+1&&!Live.bPrecisionModal,"Successful award equals the authoritative preview and one disk transaction");
            const auto SavedBytes=Bytes(Path);const auto EventCount=Controls.CommittedEvents.size();
            Verify(EventCount>0&&!Live.FinishPrecision(NewAttempt,2)&&!Live.FinishPrecision(Attempt,2)&&Bytes(Path)==SavedBytes&&Controls.CommittedEvents.size()==EventCount,"Successful measured result and its events cannot be awarded twice");
            CampaignSession FinalReload(std::filesystem::path(*Path),Live.Rules);
            Verify(FinalReload.load().ok&&serializeCampaign(FinalReload.state())==serializeCampaign(*Live.Current()),"Successful measured haul and receipts survive a fresh disk reload");
        }
        Report=FString::Printf(TEXT("%d Precision persistence checks passed; three real Windows lock failures; prepared callbacks, no native timing or layout claim."),Checks);
        UE_LOG(LogFoundryPrecisionProbe,Display,TEXT("PRECISION_PERSISTENCE_PROBE_COMPLETE ok=1 checks=%d failures=3 prepared=1 saves=%s"),Checks,*Directory);
        return true;
    }
    catch(const std::exception& Error)
    {Report=UTF8_TO_TCHAR(Error.what());UE_LOG(LogFoundryPrecisionProbe,Error,TEXT("PRECISION_PERSISTENCE_PROBE_COMPLETE ok=0 checks=%d reason=%s"),Checks,*Report);return false;}
}
#endif

#if FOUNDRY_WITH_CAMPAIGN
#include "FoundryCampaign.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include <algorithm>

DEFINE_LOG_CATEGORY_STATIC(LogFoundryCampaign, Log, All);
namespace { FString Text(const std::string& S) { return UTF8_TO_TCHAR(S.c_str()); } }

FFoundryCampaign::FFoundryCampaign(FFoundrySession& InCombat, const FString& InSavePath)
    : Combat(InCombat), Hooks(overkill::cinderwallUpgradeHooks(Combat.Rules)), Rules(Combat.Rules, Hooks),
      Store(std::filesystem::path(*InSavePath), Rules), SavePath(InSavePath)
{
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(SavePath), true);
    if (IFileManager::Get().FileExists(*SavePath) || IFileManager::Get().FileExists(*(SavePath + TEXT(".previous"))))
    {
        const auto Loaded = Store.load();
        if (!Loaded.ok) { bSaveUnavailable = true; Message = TEXT("Save could not be opened: ") + Text(Loaded.reason); }
        else { ReadCommitted(); Message = Loaded.recoveredPrevious ? TEXT("Recovered the previous valid save.") : TEXT("Campaign ready to continue."); }
    }
    Combat.CampaignSubmit = [this](const overkill::Action& Action, const FString& Description)
    {
        overkill::CampaignAction Command;
        Command.type = overkill::CampaignActionType::Combat;
        Command.combat = Action;
        Combat.LastAction = Description;
        return Apply(Command);
    };
}
FFoundryCampaign::~FFoundryCampaign() { Combat.CampaignSubmit = {}; }
const overkill::Campaign* FFoundryCampaign::Current() const { return Store.loaded() ? &Store.state() : nullptr; }
bool FFoundryCampaign::HasActiveSave() const
{
    const auto* C = Current();
    return C && C->phase != overkill::CityPhase::Defeated && C->phase != overkill::CityPhase::Complete;
}
FString FFoundryCampaign::PhasePage() const
{
    const auto* C = Current();
    if (!C) return TEXT("title");
    switch (C->phase)
    {
    case overkill::CityPhase::Arrival: return TEXT("arrival");
    case overkill::CityPhase::Between: return TEXT("route");
    case overkill::CityPhase::Fight: return TEXT("combat");
    case overkill::CityPhase::Rewards: return TEXT("rewards");
    case overkill::CityPhase::Mystery: return TEXT("mystery");
    case overkill::CityPhase::Defeated: return TEXT("defeat");
    default: return TEXT("complete");
    }
}
void FFoundryCampaign::ReadCommitted()
{
    if (!Store.loaded()) return;
    Combat.State = Store.state().fight;
    Combat.Precision = -1;
    Combat.FixSelection();
}
bool FFoundryCampaign::Handle(const overkill::SessionResult& Result)
{
    ++ViewRevision;
    if (Store.loaded()) ReadCommitted();
    else bSaveUnavailable = true;
    Message = Result.ok ? (Result.recoveredPrevious ? TEXT("Recovered and saved.") : TEXT("Saved.")) : Text(Result.reason);
    Combat.Message = Message;
    UE_LOG(LogFoundryCampaign, Display, TEXT("CAMPAIGN_TRANSACTION ok=%d replayed=%d reconciled=%d revision=%llu reason=%s"),
        Result.ok, Result.replayed, Result.reconciled, static_cast<uint64>(Store.revision()), *Text(Result.reason));
    if (!Result.ok) return false;
    if (!Result.replayed)
    {
        Combat.CommittedEvents.insert(Combat.CommittedEvents.end(), Result.events.begin(), Result.events.end());
        for (const auto& Event : Result.events)
        {
            const FString Json = Text(overkill::eventJson(Event));
            Combat.RecentEvents.Add(Json);
            UE_LOG(LogFoundryCampaign, Display, TEXT("CAMPAIGN_EVENT %s"), *Json);
        }
        while (Combat.RecentEvents.Num() > 12) Combat.RecentEvents.RemoveAt(0);
    }
    return true;
}
bool FFoundryCampaign::StartNew(bool bConfirmed, uint64 Seed)
{
    if (bPrecisionModal || bCollectionChoice) return false;
    if (bSaveUnavailable) { ++ViewRevision; return false; }
    if (HasActiveSave() && !bConfirmed) { Page = TEXT("confirm-new"); ++ViewRevision; return false; }
    if (!Seed) Seed = static_cast<uint64>(FDateTime::UtcNow().GetTicks());
    const FString RunId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    if (!Handle(Store.newGame(Seed, TCHAR_TO_UTF8(*RunId)))) return false;
    bCanResumeInMemory = true;
    Combat.Selection.clear(); Combat.SpreadTargets.Empty(); Combat.CommittedEvents.clear(); Combat.Drafts.clear();
    Drawer.Empty(); FocusRecipe = 0; ++SceneRevision; Page = PhasePage();
    Message = TEXT("Mara arrives at Cinderwall. Choose one permanent Mayor gift.");
    return true;
}
bool FFoundryCampaign::Continue()
{
    if (bPrecisionModal || bCollectionChoice) return false;
    const auto* C = Current();
    if (!C || C->phase == overkill::CityPhase::Defeated) return false;
    if (C->phase == overkill::CityPhase::Fight)
    {
        overkill::CampaignAction Action; Action.type = overkill::CampaignActionType::Continue;
        if (!Apply(Action)) return false;
        Message = TEXT("Returned to this encounter's original entry. Your discoveries remain.");
    }
    else { ReadCommitted(); Page = PhasePage(); ++ViewRevision; }
    Drawer.Empty(); Combat.Selection.clear(); Combat.SpreadTargets.Empty(); Combat.Drafts.clear(); ++SceneRevision;
    bCanResumeInMemory = true;
    return true;
}
bool FFoundryCampaign::Apply(overkill::CampaignAction Action)
{
    if (bPrecisionModal || bCollectionChoice) return false;
    const auto* C = Current();
    if (!C) { Message = TEXT("Open a campaign first."); ++ViewRevision; return false; }
    const auto BeforePhase = C->phase;
    Action.runId = C->runId; Action.sequence = C->nextTransaction;
    const bool bTiming = FParse::Param(FCommandLine::Get(), TEXT("FoundryTransactionTiming"));
    double RulesMilliseconds = -1, SaveTimes[4] = {-1,-1,-1,-1};
    if (bTiming)
    {
        // Diagnostic evaluation on a copy. Only Store.apply can publish state.
        auto Candidate = *C;
        const double RulesStart = FPlatformTime::Seconds();
        Rules.apply(Candidate, Action);
        RulesMilliseconds = (FPlatformTime::Seconds() - RulesStart) * 1000;
    }
    const double Start = FPlatformTime::Seconds();
    const auto Result = bTiming ? Store.apply(Action, [&](overkill::SavePoint Point)
        { SaveTimes[static_cast<int32>(Point)] = (FPlatformTime::Seconds() - Start) * 1000; }) : Store.apply(Action);
    const double TotalMilliseconds = (FPlatformTime::Seconds() - Start) * 1000;
    if (bTiming)
        UE_LOG(LogFoundryCampaign, Display, TEXT("CAMPAIGN_TIMING type=%d combat=%d ok=%d total_ms=%.3f rules_copy_ms=%.3f written_ms=%.3f temporary_flush_ms=%.3f replace_ms=%.3f commit_flush_ms=%.3f verify_ms=%.3f"),
            static_cast<int32>(Action.type), static_cast<int32>(Action.combat.type), Result.ok, TotalMilliseconds, RulesMilliseconds,
            SaveTimes[0], SaveTimes[1]<0?-1:SaveTimes[1]-SaveTimes[0], SaveTimes[2]<0?-1:SaveTimes[2]-SaveTimes[1],
            SaveTimes[3]<0?-1:SaveTimes[3]-SaveTimes[2], SaveTimes[3]<0?-1:TotalMilliseconds-SaveTimes[3]);
    if (!Handle(Result)) return false;
    const auto* After = Current();
    if (!After) return false;
    if (After->phase != BeforePhase || Page == TEXT("title")) { Page = PhasePage(); Drawer.Empty(); }
    if (Action.type == overkill::CampaignActionType::EnterOffer || Action.type == overkill::CampaignActionType::EnterPatrol || Action.type == overkill::CampaignActionType::Continue)
    { ++SceneRevision; Combat.Selection.clear(); Combat.SpreadTargets.Empty(); Combat.Drafts.clear(); Combat.FixSelection(); }
    return true;
}
void FFoundryCampaign::Navigate(const FString& Destination)
{
    if (bPrecisionModal || bCollectionChoice) return;
    if (Destination == TEXT("shop") || Destination == TEXT("event-shop"))
    {
        overkill::CampaignAction Action; Action.type = overkill::CampaignActionType::OpenShop; Action.eventShop = Destination == TEXT("event-shop");
        if (!Apply(Action)) return;
    }
    if (Page != Destination) ReturnPage = Page;
    Page = Destination; ++ViewRevision;
}
void FFoundryCampaign::Close()
{
    if (bPrecisionModal) return;
    if (bCollectionChoice) { CancelCollectionChoice(); return; }
    if (!Drawer.IsEmpty()) Drawer.Empty();
    else if (Page == TEXT("confirm-new")) Page = TEXT("title");
    else if (Page == TEXT("shop") || Page == TEXT("event-shop") || Page == TEXT("memory") || Page == TEXT("upgrades")) { Page = ReturnPage.IsEmpty() ? PhasePage() : ReturnPage; ReturnPage=PhasePage(); }
    else if (Page == TEXT("title")) { if (bCanResumeInMemory && HasActiveSave()) Page=PhasePage(); }
    else Page = TEXT("title");
    ++ViewRevision;
}
bool FFoundryCampaign::Control(const FString& Command)
{
    if (bPrecisionModal) return false;
    if (bCollectionChoice) { if(Command==TEXT("back"))CancelCollectionChoice(); return false; }
    if (Command == TEXT("back")) { Close(); return false; }
    if (Command == TEXT("diagnostic")) { bDiagnostic = !bDiagnostic; ++ViewRevision; return false; }
    if (Command == TEXT("panels")) { Drawer = Drawer.IsEmpty() ? TEXT("recipes") : TEXT(""); ++ViewRevision; return false; }
    if (Page != TEXT("combat") || !Current() || Current()->phase != overkill::CityPhase::Fight) return false;
    if (Command == TEXT("restart")) { Message = TEXT("Use the title menu to leave. Continue restores this encounter's original entry."); ++ViewRevision; return false; }
    if (Command == TEXT("precision")) { BeginPrecision(); return false; }
    if (Command == TEXT("collect") && foundry_controls::heavyLiftPending(Combat.State)) { BeginCollectionChoice(); return false; }
    if (Command == TEXT("end") && Drawer!=TEXT("turn-choices") && !Combat.Drafts.choices(Combat.Rules,Combat.State,overkill::Action::endTurn()).empty())
    { Drawer=TEXT("turn-choices");++ViewRevision;return false; }
    const bool Result = Combat.Control(Command);
    if (!Result) { Message = Combat.Message; ++ViewRevision; }
    return Result;
}
overkill::Action FFoundryCampaign::PrecisionAction(int32 Result) const
{
    if (!PrecisionChoice) return overkill::Action::collect(PrecisionSteering, Result);
    overkill::Action Action;
    Action.type = overkill::ActionType::ResolveUpgradeChoice;
    Action.upgradeChoice.choice = PrecisionChoice;
    Action.upgradeChoice.values = {Result};
    return Action;
}
bool FFoundryCampaign::BeginPrecision(overkill::Id RetryChoice)
{
    const auto* C = Current();
    if (bPrecisionModal || bCollectionChoice || !C || C->phase != overkill::CityPhase::Fight || Page != TEXT("combat") || !Drawer.IsEmpty()) return false;
    PrecisionChoice = RetryChoice;
    PrecisionSteering = Combat.Steering;
    if (RetryChoice)
    {
        if (C->fight.upgradeChoices.empty()) return false;
        const auto& Choice = C->fight.upgradeChoices.front();
        if (Choice.id != RetryChoice || Choice.kind != overkill::UpgradeChoiceKind::PrecisionRetry || Choice.values.size() != 1) return false;
        PrecisionSteering = Choice.values.front(); // Saved original steering; the retry cannot change it.
    }
    for (int32 Outcome = 0; Outcome < 3; ++Outcome)
    {
        if(!RetryChoice&&foundry_controls::heavyLiftPending(Combat.State))
        {
            if(foundry_controls::collectionOptions(Combat.Rules,Combat.State,PrecisionSteering,Outcome).empty())
            {Message=TEXT("This collection has no legal discard choice.");++ViewRevision;return false;}
        }
        else
        {
            const auto Preview = Combat.Preview(PrecisionAction(Outcome));
            if (!Preview.result.ok) { Message = Text(Preview.result.reason); ++ViewRevision; return false; }
        }
    }
    bPrecisionModal = true; PrecisionResult = -1; ++PrecisionAttempt; ++ViewRevision;
    UE_LOG(LogFoundryCampaign, Display, TEXT("PRECISION_OPEN attempt=%llu choice=%llu steering=%d width=%d"),
        PrecisionAttempt, PrecisionChoice, PrecisionSteering, overkill::upgradePrecisionWidthPercent(C->fight));
    return true;
}
void FFoundryCampaign::CancelPrecision(uint64 Attempt)
{
    if (!bPrecisionModal || Attempt != PrecisionAttempt || PrecisionResult >= 0) return;
    bPrecisionModal = false; ++ViewRevision;
    UE_LOG(LogFoundryCampaign, Display, TEXT("PRECISION_CANCEL attempt=%llu no_transaction=1"), Attempt);
}
bool FFoundryCampaign::FinishPrecision(uint64 Attempt, int32 Result)
{
    if (!bPrecisionModal || Attempt != PrecisionAttempt || PrecisionResult >= 0 || Result < 0 || Result > 2) return false;
    PrecisionResult = Result;
    if(!PrecisionChoice&&foundry_controls::heavyLiftPending(Combat.State))
    {
        BeginCollectionChoice(Result);
        return false; // The measured category is fixed; no award until a discard is confirmed.
    }
    overkill::CampaignAction Command;
    Command.type = PrecisionChoice ? overkill::CampaignActionType::ResolveUpgradeChoice : overkill::CampaignActionType::Combat;
    Command.combat = PrecisionAction(Result);
    Combat.LastAction = PrecisionChoice ? TEXT("Precision retry") : TEXT("Precision collect");
    bPrecisionModal = false;
    const bool Saved = Apply(Command);
    if (Saved)
    {
        const TCHAR* Labels[] = {TEXT("Miss"), TEXT("Good"), TEXT("Perfect")};
        Message = FString::Printf(TEXT("Precision: %s. Saved."), Labels[Result]);
    }
    else bPrecisionModal = true; // Keep the measured outcome frozen; never silently reopen an attempt.
    ++ViewRevision;
    UE_LOG(LogFoundryCampaign, Display, TEXT("PRECISION_COMMIT attempt=%llu choice=%llu result=%d ok=%d"), Attempt, PrecisionChoice, Result, Saved);
    return Saved;
}

bool FFoundryCampaign::BeginCollectionChoice(int32 Result)
{
    if(bCollectionChoice || !Current() || Current()->phase!=overkill::CityPhase::Fight)return false;
    const int32 Steering=bPrecisionModal?PrecisionSteering:Combat.Steering;
    CollectionOptions=foundry_controls::collectionOptions(Combat.Rules,Combat.State,Steering,Result);
    if(CollectionOptions.empty()){Message=TEXT("No legal collection choice is available.");++ViewRevision;return false;}
    CollectionChoiceHash=overkill::campaignHash(*Current());
    bCollectionChoice=true;++ViewRevision;
    UE_LOG(LogFoundryCampaign,Display,TEXT("COLLECTION_CHOICE_OPEN measured=%d result=%d steering=%d options=%d no_transaction=1"),bPrecisionModal,Result,Steering,static_cast<int32>(CollectionOptions.size()));
    return true;
}
void FFoundryCampaign::CancelCollectionChoice()
{
    if(!bCollectionChoice || bPrecisionModal)return;
    bCollectionChoice=false;CollectionOptions.clear();CollectionChoiceHash.clear();++ViewRevision;
}
bool FFoundryCampaign::CommitCollectionChoice(int32 Index)
{
    if(!bCollectionChoice || Index<0 || static_cast<std::size_t>(Index)>=CollectionOptions.size())return false;
    const bool Measured=bPrecisionModal;
    if(!Current() || overkill::campaignHash(*Current())!=CollectionChoiceHash){Message=TEXT("Collection changed before this choice could be saved.");bCollectionChoice=false;CollectionOptions.clear();++ViewRevision;return false;}
    overkill::CampaignAction A;A.type=overkill::CampaignActionType::Combat;A.combat=CollectionOptions[Index].action;
    bPrecisionModal=false;bCollectionChoice=false;CollectionOptions.clear();CollectionChoiceHash.clear();
    const bool Saved=Apply(A);
    if(!Saved&&Measured)bPrecisionModal=true;
    if(Saved)Message=Measured?TEXT("Precision collection saved."):TEXT("Collection saved.");
    UE_LOG(LogFoundryCampaign,Display,TEXT("COLLECTION_CHOICE_COMMIT measured=%d result=%d ok=%d"),Measured,A.combat.precision,Saved);
    return Saved;
}
void FFoundryCampaign::LeaveFailedPrecision()
{
    if (!bPrecisionModal || PrecisionResult < 0) return;
    bPrecisionModal = false; Page = TEXT("title"); Drawer.Empty(); bCanResumeInMemory = false; ++ViewRevision;
}
FString FFoundryCampaign::ItemName(const std::string& Id, bool bUpgrade) const
{
    const auto* Item = overkill::cityItem(bUpgrade ? Rules.content().upgrades : Rules.content().recipes, Id);
    return Item ? Text(Item->name) : Text(Id);
}
FString FFoundryCampaign::ItemDescription(const std::string& Id, bool bUpgrade) const
{
    const auto* Item = overkill::cityItem(bUpgrade ? Rules.content().upgrades : Rules.content().recipes, Id);
    return Item ? Text(Item->description) : TEXT("No catalogue description is available.");
}
FString FFoundryCampaign::CopyName(overkill::Id Id) const
{
    for (const auto& Copy : Combat.State.memory) if (Copy.id == Id)
        return ItemName(Copy.recipe) + FString::Printf(TEXT("  [copy %llu]"), Id);
    return Combat.NameFor(Id);
}
#endif

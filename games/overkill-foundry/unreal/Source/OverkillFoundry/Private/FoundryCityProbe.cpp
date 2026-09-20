#if FOUNDRY_WITH_CAMPAIGN
#include "FoundryCityProbe.h"
#include "FoundryHost.h"
#include "FoundryCampaign.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/WeakObjectPtr.h"
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>

// The exact same parser/preflight runs in the graphics-free preparation. It
// calls shared CampaignRules; no runner policy or combat arithmetic lives here.
#include "../../../Tools/city-probe/replay.cpp"

DEFINE_LOG_CATEGORY_STATIC(LogFoundryCityProbe, Log, All);
namespace {
namespace fs=std::filesystem;
FString Text(const std::string& Value) { return UTF8_TO_TCHAR(Value.c_str()); }
void Need(bool Value,const std::string& Reason) { if(!Value)throw std::runtime_error(Reason); }
std::string ReadBytes(const fs::path& Path)
{
    std::ifstream File(Path,std::ios::binary);Need(static_cast<bool>(File),"Could not read replay input");
    return {std::istreambuf_iterator<char>(File),{}};
}
void WriteBytes(const fs::path& Path,const std::string& Bytes)
{
    std::ofstream File(Path,std::ios::binary);File<<Bytes;Need(static_cast<bool>(File),"Could not write replay evidence");
}
bool Within(const fs::path& Path,const fs::path& Directory)
{
    const auto Relative=fs::weakly_canonical(Path).lexically_relative(fs::weakly_canonical(Directory));
    return !Relative.empty()&&!Relative.is_absolute()&&*Relative.begin()!=L"..";
}
class FCityReplay
{
public:
    explicit FCityReplay(AFoundryStage& InStage):Stage(InStage){}
    void Tick(TFunctionRef<void(const FString&)> Capture)
    {
        if(bDone)return;
        try
        {
            if(!bStarted){Start();return;}
            const double Now=FPlatformTime::Seconds();
            Need(Now-LastProgress<120.0,"Presentation/capture watchdog expired; no command was substituted");
            if(!ActiveCapture.IsEmpty())
            {
                const auto Source=fs::path(*FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Screenshots"),ActiveCapture));
                if(fs::exists(Source)&&fs::file_size(Source)>128)
                {
                    fs::copy_file(Source,Output/L"screenshots"/fs::path(*ActiveCapture));
                    CaptureRows<<"{\"file\":"<<foundry_city_probe::jsonQuote(TCHAR_TO_UTF8(*ActiveCapture))<<",\"step\":"<<Index
                        <<",\"hash\":"<<foundry_city_probe::jsonQuote(overkill::campaignHash(*Campaign().Current()))
                        <<",\"action_view\":"<<(Stage.IsActionView()?"true":"false")<<",\"busy\":"<<(Stage.IsPresentationBusy()?"true":"false")<<"}\n";
                    Need(static_cast<bool>(CaptureRows),"Capture index write failed");CaptureRows.flush();
                    ++CaptureCount;ActiveCapture.Empty();LastProgress=Now;ReadyAt=Now+.12;
                }
                return;
            }
            // Two sparse action captures use the existing shot hold. They do
            // not delay, replay or derive any committed damage event.
            if(!DuringHoldCapture.IsEmpty())
            {
                RequestCapture(DuringHoldCapture,Capture);DuringHoldCapture.Empty();return;
            }
            if(Stage.IsPresentationBusy()||Now<ReadyAt)return;
            if(bCheckPresentation)
            {
                const auto& Prior=Plan.steps[Index-1];
                if(Prior.action.type==overkill::CampaignActionType::Combat)
                {
                    const auto Type=Prior.action.combat.type;
                    if(Type==overkill::ActionType::Load)Need(Stage.IsActionView(),"Successful Lock and load did not settle in action view");
                    if(Type==overkill::ActionType::Fire||Type==overkill::ActionType::EndTurn||Type==overkill::ActionType::Unload)
                        Need(!Stage.IsActionView(),"Completed Fire/End Turn/Unload did not return to preparation");
                }
                bCheckPresentation=false;
            }
            if(!PendingCapture.IsEmpty()){RequestCapture(PendingCapture,Capture);PendingCapture.Empty();return;}
            if(Index==Plan.steps.size()){Finish();return;}
            ApplyNext();
        }
        catch(const std::exception& Error){Fail(Error.what());}
    }
private:
    AFoundryStage& Stage;
    foundry_city_probe::Plan Plan;
    fs::path Output,Save;
    std::ofstream Rows,CaptureRows,EventRows;
    std::size_t Index=0,CaptureCount=0;
    bool bStarted=false,bDone=false,bOwnOutput=false,bCheckPresentation=false,bShopSeen=false,bMemorySeen=false;
    double LastProgress=0,ReadyAt=0,StartedAt=0;
    FString Prefix,PendingCapture,DuringHoldCapture,ActiveCapture;
    std::set<int> LoadedPositions,ShotPositions;
    FFoundryCampaign& Campaign(){auto* Value=Stage.GetCampaign();Need(Value!=nullptr,"City replay requires the production campaign host");return *Value;}
    void Start()
    {
        FString TracePath,OutputPath,ExplicitSave;
        Need(FParse::Value(FCommandLine::Get(),TEXT("FoundryCityTrace="),TracePath),"Missing city trace");
        Need(FParse::Value(FCommandLine::Get(),TEXT("FoundryCityOutput="),OutputPath),"Pass a fresh -FoundryCityOutput directory");
        Need(FParse::Value(FCommandLine::Get(),TEXT("FoundrySave="),ExplicitSave),"Replay requires an explicit isolated -FoundrySave");
        auto& C=Campaign();Save=fs::path(*C.SavePath);Output=fs::absolute(fs::path(*OutputPath));
        const fs::path Saved(*FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()));
        Need(Within(Save,Saved)&&Within(Output,Saved),"Replay save/evidence must remain beneath this project's ignored Saved directory");
        Need(C.bFixedSavePath&&!C.Current()&&!C.Store.hasSave()&&!C.bSaveUnavailable,"City replay refuses to replace or recover an existing campaign");
        Need(!fs::exists(Output),"City evidence directory already exists; use a new run directory");
        const auto Bytes=ReadBytes(fs::path(*TracePath));std::istringstream Input(Bytes);
        Plan=foundry_city_probe::readPlan(Input,C.Rules);
        // Full input/rules preflight precedes the first save. START bytes are
        // comparison data only, never injected into the live campaign.
        fs::create_directories(Output/L"screenshots");bOwnOutput=true;WriteBytes(Output/L"input.oftrace",Bytes);
        Rows.open(Output/L"transactions.jsonl",std::ios::binary);CaptureRows.open(Output/L"captures.jsonl",std::ios::binary);EventRows.open(Output/L"events.jsonl",std::ios::binary);
        Need(Rows&&CaptureRows&&EventRows,"Cannot create city evidence streams");
        Prefix=FString::Printf(TEXT("city%llu-"),static_cast<uint64>(Plan.seed))+FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"))+FString::Printf(TEXT("-%u"),FPlatformProcess::GetCurrentProcessId());
        const auto Fresh=C.Store.newGame(Plan.seed,Plan.runId);
        Need(Fresh.ok&&!Fresh.replayed&&C.Store.revision()==1&&overkill::serializeCampaign(C.Store.state())==Plan.initialBytes,"Production New Game differs from the canonical historical start");
        Need(C.Continue(),"Canonical Arrival could not be displayed");Stage.RefreshCampaignWorld();
        Need(overkill::serializeCampaign(*C.Current())==Plan.initialBytes&&C.Store.revision()==1,"Arrival display caused an unexpected durable transaction");
        bStarted=true;StartedAt=LastProgress=FPlatformTime::Seconds();ReadyAt=StartedAt+.75;PendingCapture=TEXT("arrival");
        UE_LOG(LogFoundryCityProbe,Display,TEXT("CITY_REPLAY_START seed=%llu policy=%s precision=%s commands=%llu core_digest=%s review_snapshot=%d save=%s output=%s canonical_newgame=1"),
            static_cast<uint64>(Plan.seed),*Text(Plan.policy),*Text(Plan.precision),static_cast<uint64>(Plan.steps.size()),UTF8_TO_TCHAR(FOUNDRY_CORE_SOURCE_DIGEST),FOUNDRY_CORE_REVIEW_SNAPSHOT,*C.SavePath,*OutputPath);
    }
    void RequestCapture(const FString& Label,TFunctionRef<void(const FString&)> Capture)
    {
        ActiveCapture=Prefix+FString::Printf(TEXT("-%03llu-"),static_cast<uint64>(Index))+Label+TEXT(".png");
        const FString Path=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Screenshots"),ActiveCapture);
        Need(!IFileManager::Get().FileExists(*Path),"Screenshot identity already exists");
        Capture(ActiveCapture);
    }
    void ApplyNext()
    {
        auto& C=Campaign();const auto& Step=Plan.steps[Index];
        const auto ExpectedBefore=Index?Plan.steps[Index-1].expectedBytes:Plan.initialBytes;
        Need(C.Current()&&overkill::serializeCampaign(*C.Current())==ExpectedBefore,"Campaign changed outside the recorded input stream");
        Need(Stage.GetSession().CommittedEvents.empty(),"Undrained presentation events before recorded command");
        // These are view selections, not fabricated CampaignActions. Shop and
        // recipe discovery still happen only through their recorded commands.
        C.Page=C.PhasePage();C.Drawer.Empty();
        if(Step.action.type==overkill::CampaignActionType::Combat)
        {
            if(Step.action.combat.type==overkill::ActionType::Craft){C.FocusRecipe=Step.action.combat.subject;C.Drawer=TEXT("recipes");}
            if(Step.action.combat.type==overkill::ActionType::Fire)Stage.GetSession().Target=Step.action.combat.target;
            // Make the naturally chosen next shot target readable during its
            // preceding loaded view, without changing any rules state.
            if(Step.action.combat.type==overkill::ActionType::Load)
                for(std::size_t Next=Index+1;Next<Plan.steps.size();++Next)
                {
                    const auto& Action=Plan.steps[Next].action;
                    if(Action.type!=overkill::CampaignActionType::Combat)break;
                    if(Action.combat.type==overkill::ActionType::Fire){Stage.GetSession().Target=Action.combat.target;break;}
                    if(Action.combat.type==overkill::ActionType::EndTurn||Action.combat.type==overkill::ActionType::Unload)break;
                }
        }
        ++C.ViewRevision;
        std::vector<overkill::Event> Events;
        const bool Ok=Stage.SubmitCampaignAction(Step.action,Events);
        const auto* Actual=C.Current();Need(Actual!=nullptr,"Committed campaign disappeared");
        const auto Reason=Actual->receipts.empty()?std::string(TCHAR_TO_UTF8(*C.Message)):Actual->receipts.back().message;
        foundry_city_probe::verifyCommitted(Step,*Actual,Ok,Reason,Events);
        Need(C.Store.revision()==Index+2,"Recorded action did not produce exactly one durable revision");
        overkill::CampaignSession Disk(Save,C.Rules);const auto Reload=Disk.load();
        Need(Reload.ok&&!Reload.recoveredPrevious&&Disk.revision()==C.Store.revision()&&overkill::serializeCampaign(Disk.state())==Step.expectedBytes,"Disk envelope differs after recorded command");
        ++Index;Rows<<foundry_city_probe::stepJson(Index,Step,C.Store.revision())<<'\n';Rows.flush();Need(static_cast<bool>(Rows),"Transaction report write failed");
        for(std::size_t EventIndex=0;EventIndex<Events.size();++EventIndex)
            EventRows<<"{\"step\":"<<Index<<",\"index\":"<<EventIndex<<",\"event\":"<<overkill::eventJson(Events[EventIndex])<<"}\n";
        EventRows.flush();Need(static_cast<bool>(EventRows),"Event report write failed");
        bCheckPresentation=true;LastProgress=FPlatformTime::Seconds();ReadyAt=LastProgress+.12;
        using CT=overkill::CampaignActionType;using AT=overkill::ActionType;
        if(Step.action.type==CT::ChooseMayor)PendingCapture=TEXT("mayor-committed-route");
        else if(Step.action.type==CT::OpenShop)
        {
            C.Page=Step.action.eventShop?TEXT("event-shop"):TEXT("shop");++C.ViewRevision;
            if(!bShopSeen){PendingCapture=TEXT("finite-shop");bShopSeen=true;}
        }
        else if(Step.action.type==CT::OpenRecipes)
        {
            C.Page=TEXT("memory");++C.ViewRevision;
            if(!bMemorySeen){PendingCapture=TEXT("earned-memory");bMemorySeen=true;}
        }
        else if(Step.action.type==CT::EnterOffer)PendingCapture=FString::Printf(TEXT("position-%02d-%s"),Step.beforePosition,Step.afterPhase==overkill::CityPhase::Fight?TEXT("encounter"):TEXT("mystery"));
        else if(Step.action.type==CT::Combat&&Step.action.combat.type==AT::Load&&LoadedPositions.insert(Step.beforePosition).second)
            PendingCapture=FString::Printf(TEXT("position-%02d-loaded"),Step.beforePosition);
        else if(Step.action.type==CT::Combat&&Step.action.combat.type==AT::Fire&&(Step.beforePosition==1||Step.beforePosition==12)&&ShotPositions.insert(Step.beforePosition).second)
            DuringHoldCapture=FString::Printf(TEXT("position-%02d-committed-shot"),Step.beforePosition);
        if(Step.beforePhase==overkill::CityPhase::Fight&&Step.afterPhase==overkill::CityPhase::Rewards)
            PendingCapture=FString::Printf(TEXT("position-%02d-rewards-after-presentation"),Step.beforePosition);
        if(Step.afterPhase==overkill::CityPhase::Complete)PendingCapture=TEXT("cinderwall-complete");
        UE_LOG(LogFoundryCityProbe,Display,TEXT("CITY_TRANSACTION ok=1 step=%llu sequence=%llu action=%s position=%d phase=%s hash=%s revision=%llu events=%llu disk_reload=1 receipts_exact=1"),
            static_cast<uint64>(Index),static_cast<uint64>(Step.action.sequence),*Text(foundry_city_probe::actionName(Step.action)),Step.afterPosition,*Text(foundry_city_probe::phaseName(Step.afterPhase)),*Text(Step.hash),static_cast<uint64>(C.Store.revision()),static_cast<uint64>(Events.size()));
    }
    void Finish()
    {
        const auto& C=Campaign();Need(C.Current()&&overkill::serializeCampaign(*C.Current())==Plan.finalBytes,"Final rendered campaign bytes differ");
        Need(C.Current()->phase==overkill::CityPhase::Complete&&C.Current()->route.position==13,"City did not complete all twelve positions");
        Need(!Stage.IsPresentationBusy()&&Stage.GetSession().CommittedEvents.empty(),"Presentation not drained at completion");
        Rows.close();CaptureRows.close();EventRows.close();fs::copy_file(Save,Output/L"final.ofsave");
        std::ostringstream Result;Result<<"{\"ok\":true,\"evidence_kind\":\"automated Unreal production campaign replay; not human play or visual approval\",\"seed\":"<<Plan.seed
            <<",\"commands\":"<<Index<<",\"captures\":"<<CaptureCount<<",\"final_hash\":"<<foundry_city_probe::jsonQuote(Plan.finalHash)
            <<",\"revision\":"<<C.Store.revision()<<",\"position\":"<<C.Current()->route.position<<",\"hp\":"<<C.Current()->fight.hp
            <<",\"core_digest\":"<<foundry_city_probe::jsonQuote(FOUNDRY_CORE_SOURCE_DIGEST)<<",\"review_snapshot\":"<<FOUNDRY_CORE_REVIEW_SNAPSHOT
            <<",\"seconds\":"<<FPlatformTime::Seconds()-StartedAt<<"}\n";
        WriteBytes(Output/L"summary.json",Result.str());bDone=true;
        UE_LOG(LogFoundryCityProbe,Display,TEXT("FOUNDRY_CITY_PROBE_COMPLETE ok=1 commands=%llu captures=%llu hash=%s position=13 hp=%d revision=%llu"),
            static_cast<uint64>(Index),static_cast<uint64>(CaptureCount),*Text(Plan.finalHash),C.Current()->fight.hp,static_cast<uint64>(C.Store.revision()));
        FPlatformMisc::RequestExit(false);
    }
    void Fail(const std::string& Reason)
    {
        bDone=true;
        UE_LOG(LogFoundryCityProbe,Error,TEXT("FOUNDRY_CITY_PROBE_COMPLETE ok=0 completed_commands=%llu reason=%s"),static_cast<uint64>(Index),*Text(Reason));
        if(bOwnOutput)
            try{WriteBytes(Output/L"failure.json","{\"ok\":false,\"completed_commands\":"+std::to_string(Index)+",\"reason\":"+foundry_city_probe::jsonQuote(Reason)+"}\n");}catch(const std::exception&){}
        FPlatformMisc::RequestExit(false);
    }
};
TWeakObjectPtr<AFoundryStage> ProbeOwner;
TUniquePtr<FCityReplay> CityReplay;
}
bool TickFoundryCityProbe(AFoundryStage& Stage,TFunctionRef<void(const FString&)> Capture)
{
    FString Trace;
    if(!FParse::Value(FCommandLine::Get(),TEXT("FoundryCityTrace="),Trace))return false;
    if(ProbeOwner.Get()!=&Stage){ProbeOwner=&Stage;CityReplay=MakeUnique<FCityReplay>(Stage);}
    CityReplay->Tick(Capture);return true;
}
#endif

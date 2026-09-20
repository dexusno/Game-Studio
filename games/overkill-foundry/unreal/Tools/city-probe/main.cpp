#include "replay.hpp"
#include "city_runner.hpp"
#include "overkill/campaign_session.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace {
namespace fs=std::filesystem;
using namespace overkill;
using foundry_city_probe::jsonQuote;
std::size_t checks=0;
std::ostream* branchEvidence=nullptr;
std::string branchName;
void need(bool value,const std::string& label){++checks;if(!value)throw std::runtime_error(label);}
std::string read(const fs::path& path){std::ifstream file(path,std::ios::binary);need(static_cast<bool>(file),"Cannot read input");return {std::istreambuf_iterator<char>(file),{}};}
void write(const fs::path& path,const std::string& text){std::ofstream file(path,std::ios::binary);file<<text;need(static_cast<bool>(file),"Cannot write report");}
std::string mapJson(const std::map<std::string,std::size_t>& values){std::ostringstream out;out<<'{';bool comma=false;for(const auto& item:values){if(comma)out<<',';comma=true;out<<jsonQuote(item.first)<<':'<<item.second;}out<<'}';return out.str();}
std::string setJson(const std::set<std::string>& values){std::ostringstream out;out<<'[';bool comma=false;for(const auto& item:values){if(comma)out<<',';comma=true;out<<jsonQuote(item);}out<<']';return out.str();}
void codecContracts(const foundry_city_probe::Plan& plan){
    for(const auto& step:plan.steps)need(runner::encodeAction(step.action)==step.encodedAction,"Historical codec differs at sequence "+std::to_string(step.action.sequence));
    CampaignAction all;all.type=CampaignActionType::MoveRecipeCopy;all.runId="escaped \\\"identity";all.sequence=57;all.subject=8;all.exchange=9;all.target=10;all.choice="choice with spaces";all.quantity=3;all.eventShop=true;all.decline=true;all.material=4;
    auto& b=all.combat;b.type=ActionType::ActivateUpgrade;b.subject=12;b.target=13;b.steering=2;b.precision=1;b.amount=15;
    b.parts={16,17};b.spreadTargets={{16,18},{17,19}};b.choices={20,21};b.targets={22,23};b.sacrifices={24,25};b.partTargets={{26,27}};b.partChoices={{28,29}};b.discarded={1,2,3,4,5};
    b.upgradeChoice.choice=30;b.upgradeChoice.objects={31,32};b.upgradeChoice.values={-1,0,2};b.upgradeChoice.option="option \\\"quoted";b.upgradeChoice.removed={5,4,3,2,1};b.upgradeChoice.added={0,1,0,2,0};b.upgradeChoice.decline=true;b.discount={0,1,2,3,4};b.upgrade="UGS-TEST";
    const auto encoded=runner::encodeAction(all);
    need(runner::encodeAction(foundry_city_probe::decodeAction(encoded))==encoded,"All optional action fields preserve the historical codec");
    for(const auto& bad:std::vector<std::string>{encoded+" trailing",encoded.substr(0,encoded.size()-4),"99 "+encoded.substr(encoded.find(' ')+1)}){
        bool rejected=false;try{foundry_city_probe::decodeAction(bad);}catch(const std::exception&){rejected=true;}need(rejected,"Malformed command must reject");
    }
}
void corruptContracts(const std::string& original,const CampaignRules& rules){
    const auto replace=[](std::string source,const std::string& from,const std::string& to){const auto at=source.find(from);need(at!=std::string::npos,"Negative-test marker exists");source.replace(at,from.size(),to);return source;};
    std::vector<std::string> bad;
    bad.push_back(replace(original,"baseline-5","baseline-99"));
    bad.push_back(replace(original,"OFCITY2","OFCITY9"));
    bad.push_back(replace(original,"START ","START z"));
    bad.push_back(replace(original,"E {","E {\"unearned\":true,"));
    bad.push_back(replace(original,"R 1 ","R 0 "));
    bad.push_back(original+"trailing\n");
    bad.push_back(original.substr(0,original.rfind("FINAL ")));
    for(const auto& text:bad){bool rejected=false;try{std::istringstream input(text);foundry_city_probe::readPlan(input,rules);}catch(const std::exception&){rejected=true;}need(rejected,"Corrupted historical witness must reject before writing a save");}
}
CampaignAction action(const Campaign& state,CampaignActionType type){CampaignAction out;out.type=type;out.runId=state.runId;out.sequence=state.nextTransaction;return out;}
SessionResult exactApply(CampaignSession& session,Campaign& reference,const CampaignRules& rules,const CampaignAction& command){
    const auto expected=rules.apply(reference,command);const auto result=session.apply(command);
    need(result.ok&&expected.ok&&!result.replayed,"Branch transaction must succeed exactly once: "+foundry_city_probe::actionName(command)+" sequence "+std::to_string(command.sequence)+"; "+result.reason);
    need(result.reason==expected.reason&&serializeCampaign(session.state())==serializeCampaign(reference),"Branch save/result matches rules reference");
    need(result.events.size()==expected.events.size(),"Branch event count matches");
    for(std::size_t i=0;i<result.events.size();++i)need(eventJson(result.events[i])==eventJson(expected.events[i]),"Branch ordered event matches");
    if(branchEvidence){
        *branchEvidence<<"{\"branch\":"<<jsonQuote(branchName)<<",\"action\":"<<jsonQuote(runner::encodeAction(command))<<",\"hash\":"<<jsonQuote(campaignHash(reference))<<",\"revision\":"<<session.revision()<<",\"events\":[";
        for(std::size_t i=0;i<result.events.size();++i){if(i)*branchEvidence<<',';*branchEvidence<<eventJson(result.events[i]);}*branchEvidence<<"]}\n";
        need(static_cast<bool>(*branchEvidence),"Branch action/event evidence written");
    }
    return result;
}
void beginBranch(CampaignSession& session,Campaign& reference,const CampaignRules& rules,const foundry_city_probe::Plan& plan,bool throughCollect){
    const auto fresh=session.newGame(plan.seed,plan.runId);need(fresh.ok,"Separate branch starts with production New Game");reference=rules.newGame(plan.seed,plan.runId);
    for(const auto& step:plan.steps){exactApply(session,reference,rules,step.action);if(throughCollect){if(step.action.type==CampaignActionType::Combat&&step.action.combat.type==ActionType::Collect)return;}else if(reference.phase==CityPhase::Fight)return;}
    throw std::runtime_error("Missing natural branch point");
}
std::string branchContracts(const fs::path& directory,const CampaignRules& rules,const foundry_city_probe::Plan& plan){
    // Separate legal branches: the successful historical route below is never
    // altered to manufacture a defeat or an encounter restart.
    std::ofstream branchRows(directory/"separate-branches.jsonl",std::ios::binary);branchEvidence=&branchRows;branchName="continue";
    CampaignSession restart(directory/"restart-branch.ofsave",rules);Campaign reference;
    beginBranch(restart,reference,rules,plan,true);
    const auto gatheredHash=campaignHash(restart.state());const auto entry=restart.state().entry;const auto discoveries=restart.state().profile.recipes;
    need(!entry.empty(),"Earned fight stores an original entry");
    CampaignSession reopened(directory/"restart-branch.ofsave",rules);need(reopened.load().ok&&campaignHash(reopened.state())==gatheredHash,"Midfight load is exact before explicit Continue");
    exactApply(reopened,reference,rules,action(reference,CampaignActionType::Continue));
    need(reference.phase==CityPhase::Fight&&reference.entry==entry&&reference.profile.recipes==discoveries,"Continue restarts the same fight and retains discoveries");
    need(campaignHash(reference)!=gatheredHash&&reference.fight.round==1&&reference.fight.phase==Phase::Collection,"Continue resets earned midfight collection, rather than resuming it");
    const auto restartHash=campaignHash(reference);

    branchName="defeat";CampaignSession defeat(directory/"defeat-branch.ofsave",rules);beginBranch(defeat,reference,rules,plan,false);
    std::size_t endTurns=0,deathEvents=0;
    // A probe watchdog is a test failure, not an in-game turn cap.
    while(reference.phase==CityPhase::Fight&&endTurns<100){
        if(reference.fight.phase==Phase::Collection){auto gather=action(reference,CampaignActionType::Combat);gather.combat=Action::collect(3);exactApply(defeat,reference,rules,gather);}
        auto command=action(reference,CampaignActionType::Combat);command.combat=Action::endTurn();const auto result=exactApply(defeat,reference,rules,command);++endTurns;for(const auto& event:result.events)if(event.type=="defeat")++deathEvents;
    }
    need(reference.phase==CityPhase::Defeated&&reference.fight.hp==0,"Ordinary enemy turns produce a genuine defeat");
    const auto defeatHash=campaignHash(reference);const auto retained=reference.profile.recipes;
    fs::copy_file(directory/"defeat-branch.ofsave",directory/"defeat-before-restart.ofsave");
    CampaignSession deadReload(directory/"defeat-branch.ofsave",rules);need(deadReload.load().ok&&campaignHash(deadReload.state())==defeatHash,"Defeat persists across load");
    const auto blocked=deadReload.apply(action(deadReload.state(),CampaignActionType::Continue));need(!blocked.ok&&campaignHash(deadReload.state())==defeatHash,"Defeated campaign cannot Continue into combat");
    const auto again=deadReload.newGame(plan.seed+1,"city-probe/separate-defeat-restart");need(again.ok&&!again.replayed,"New Game after defeat commits once");
    need(deadReload.state().phase==CityPhase::Arrival&&deadReload.state().fight.hp==80&&deadReload.state().profile.recipes==retained,"Restart restores Mara and retains exact discovered recipe facts");
    const auto againHash=campaignHash(deadReload.state());const auto revision=deadReload.revision();const auto duplicate=deadReload.newGame(plan.seed+1,"city-probe/separate-defeat-restart");
    need(duplicate.ok&&duplicate.replayed&&deadReload.revision()==revision&&campaignHash(deadReload.state())==againHash,"Duplicate New Game identity cannot create a second restart");
    branchEvidence=nullptr;branchRows.close();
    std::ostringstream out;out<<"{\"source\":\"separate earned prefix; Auto Collect(3) then End Turn, no attacks or grants\",\"continue_hash\":"<<jsonQuote(restartHash)<<",\"defeat_hash\":"<<jsonQuote(defeatHash)<<",\"defeat_end_turns\":"<<endTurns<<",\"defeat_named_events\":"<<deathEvents<<",\"restart_hash\":"<<jsonQuote(againHash)<<",\"retained_recipes\":"<<retained.size()<<'}';return out.str();
}
}
int wmain(int argc,wchar_t** argv){try{
    need(argc==3,"Usage: foundry_city_prepare TRACE NEW_OUTPUT_DIRECTORY");const fs::path trace(argv[1]),directory(argv[2]);
    need(!fs::exists(directory),"Output directory already exists; never replace earlier evidence or a save");
    Rules fights;CampaignRules rules(fights,cinderwallUpgradeHooks(fights));const auto original=read(trace);std::istringstream input(original);
    auto plan=foundry_city_probe::readPlan(input,rules);codecContracts(plan);corruptContracts(original,rules);
    const auto historical=runner::replayTrace(trace.string(),rules);need(historical.find(plan.finalHash)!=std::string::npos,"Original runner independently verifies selected trace");
    fs::create_directories(directory);write(directory/"input.oftrace",original);
    const auto save=directory/"campaign.ofsave";CampaignSession session(save,rules);const auto fresh=session.newGame(plan.seed,plan.runId);
    need(fresh.ok&&!fresh.replayed&&session.revision()==1&&serializeCampaign(session.state())==plan.initialBytes,"Actual production New Game equals canonical historical start");
    std::ofstream rows(directory/"transactions.jsonl",std::ios::binary);std::map<std::string,std::size_t> commands,events,robotActions;std::set<std::string> encounters,mysteries,upgrades,recipes;
    std::size_t memoryExchanges=0,fightEntries=0,mysteryEntries=0;const auto started=std::chrono::steady_clock::now();
    for(std::size_t i=0;i<plan.steps.size();++i){const auto& step=plan.steps[i];const auto result=session.apply(step.action);
        foundry_city_probe::verifyCommitted(step,session.state(),result.ok,result.reason,result.events);++checks;
        need(!result.replayed&&session.revision()==i+2,"Every new recorded command has one durable revision");
        CampaignSession disk(save,rules);need(disk.load().ok&&disk.revision()==session.revision()&&serializeCampaign(disk.state())==step.expectedBytes,"Reloaded envelope matches full state/receipts after every transaction");
        const auto before=read(save);const auto duplicate=session.apply(step.action);foundry_city_probe::verifyCommitted(step,session.state(),duplicate.ok,duplicate.reason,duplicate.events);
        need(duplicate.replayed&&session.revision()==i+2&&read(save)==before,"Duplicate transaction is an exact receipt replay without any save write");
        rows<<foundry_city_probe::stepJson(i+1,step,session.revision())<<'\n';
        ++commands[foundry_city_probe::actionName(step.action)];if(step.action.exchange)++memoryExchanges;
        if(step.action.type==CampaignActionType::EnterOffer){if(step.afterPhase==CityPhase::Fight){++fightEntries;encounters.insert(step.encounter);}else if(step.afterPhase==CityPhase::Mystery){++mysteryEntries;mysteries.insert(step.mystery);}}
        for(const auto& event:result.events){++events[event.type];if(event.type.rfind("robot_action:",0)==0)++robotActions[event.type.substr(13)];if(event.type=="recipe_used")recipes.insert(event.source);}
        for(const auto& upgrade:session.state().fight.upgrades)upgrades.insert(upgrade.id);
    }
    need(static_cast<bool>(rows),"Complete transaction report written");rows.close();
    need(serializeCampaign(session.state())==plan.finalBytes&&session.state().phase==CityPhase::Complete&&session.state().route.position==13,"All twelve positions and final boss complete with exact historical final bytes");
    const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
    const auto branches=branchContracts(directory,rules,plan);
    std::ostringstream summary;summary<<"{\"ok\":true,\"evidence_kind\":\"new CPU production CampaignSession replay; no Unreal runtime execution\",\"seed\":"<<plan.seed<<",\"policy\":"<<jsonQuote(plan.policy)<<",\"precision\":"<<jsonQuote(plan.precision)<<",\"commands\":"<<plan.steps.size()<<",\"revision\":"<<session.revision()<<",\"final_hash\":"<<jsonQuote(plan.finalHash)<<",\"hp\":"<<session.state().fight.hp<<",\"position\":"<<session.state().route.position<<",\"fight_entries\":"<<fightEntries<<",\"noncombat_entries\":"<<mysteryEntries<<",\"memory_exchanges\":"<<memoryExchanges<<",\"checks\":"<<checks<<",\"seconds\":"<<seconds<<",\"original_codec_replay\":"<<jsonQuote(historical)<<",\"command_counts\":"<<mapJson(commands)<<",\"event_counts\":"<<mapJson(events)<<",\"robot_action_counts\":"<<mapJson(robotActions)<<",\"encounters\":"<<setJson(encounters)<<",\"mysteries\":"<<setJson(mysteries)<<",\"upgrades\":"<<setJson(upgrades)<<",\"recipes_used\":"<<setJson(recipes)<<",\"separate_branches\":"<<branches<<"}\n";
    write(directory/"summary.json",summary.str());std::cout<<summary.str();return 0;
}catch(const std::exception& error){std::cerr<<"CITY_PREPARE_FAILED "<<error.what()<<'\n';return 1;}}

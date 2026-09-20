#include "replay.hpp"
#include <algorithm>
#include <iomanip>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>

namespace foundry_city_probe {
namespace {
void require(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}
void endRow(std::istream& row,const std::string& message){require(static_cast<bool>(row),message);row>>std::ws;require(row.eof(),message);}
template<class T> void readVector(std::istream& row,std::vector<T>& values,std::size_t inputSize){
    std::size_t count=0;row>>count;require(row&&count<=inputSize,"Invalid command vector length");
    values.resize(count);for(auto& value:values)row>>value;
}
void readMaterials(std::istream& row,overkill::Materials& values){
    std::size_t count=0;row>>count;require(row&&count==values.size(),"Wrong command material vector length");
    for(auto& value:values)row>>value;
}
void readPairs(std::istream& row,std::vector<overkill::SpreadTarget>& values,std::size_t inputSize){
    std::size_t count=0;row>>count;require(row&&count<=inputSize,"Invalid command pair length");
    values.resize(count);for(auto& value:values)row>>value.part>>value.enemy;
}
void ordinaryEarnedInput(const overkill::Campaign& state){
    // This chosen witness avoids unresolved haul composition completely. Do
    // not substitute another amount, action or route if one appears.
    for(const auto& upgrade:state.fight.upgrades)
        require(upgrade.id!="UGS-141"&&upgrade.id!="UGS-011"&&upgrade.id!="UGS-018"&&upgrade.id!="UGS-126",
                "This bounded city witness excludes held-composition upgrade families");
}
}
std::string unhex(const std::string& text){
    require(text.size()%2==0,"Odd hexadecimal trace length");
    const auto digit=[](char value){if(value>='0'&&value<='9')return value-'0';if(value>='a'&&value<='f')return value-'a'+10;throw std::runtime_error("Invalid hexadecimal trace byte");};
    std::string result;result.reserve(text.size()/2);
    for(std::size_t i=0;i<text.size();i+=2)result+=static_cast<char>((digit(text[i])<<4)|digit(text[i+1]));
    return result;
}
overkill::CampaignAction decodeAction(const std::string& text){
    // OFCITY2's field order is the existing baseline-5 runner protocol. The
    // CPU contract compares this adapter to that original codec for every
    // recorded command and an all-fields round trip; no policy is linked in UE.
    overkill::CampaignAction action;std::istringstream row(text);int type=0,combatType=0;
    row>>type>>std::quoted(action.runId)>>action.sequence>>action.subject>>action.exchange>>action.target
       >>std::quoted(action.choice)>>action.quantity>>action.eventShop>>action.decline>>action.material;
    auto& combat=action.combat;
    row>>combatType>>combat.subject>>combat.target>>combat.steering>>combat.precision>>combat.amount;
    require(row&&type>=0&&type<=static_cast<int>(overkill::CampaignActionType::MoveRecipeCopy)
        &&combatType>=0&&combatType<=static_cast<int>(overkill::ActionType::ActivateUpgrade),"Invalid command enum");
    action.type=static_cast<overkill::CampaignActionType>(type);combat.type=static_cast<overkill::ActionType>(combatType);
    readVector(row,combat.parts,text.size());readPairs(row,combat.spreadTargets,text.size());
    readVector(row,combat.choices,text.size());readVector(row,combat.targets,text.size());readVector(row,combat.sacrifices,text.size());
    readPairs(row,combat.partTargets,text.size());readPairs(row,combat.partChoices,text.size());readMaterials(row,combat.discarded);
    row>>combat.upgradeChoice.choice;readVector(row,combat.upgradeChoice.objects,text.size());readVector(row,combat.upgradeChoice.values,text.size());
    row>>std::quoted(combat.upgradeChoice.option);readMaterials(row,combat.upgradeChoice.removed);readMaterials(row,combat.upgradeChoice.added);
    row>>combat.upgradeChoice.decline;readMaterials(row,combat.discount);row>>std::quoted(combat.upgrade);
    endRow(row,"Truncated or trailing command fields");return action;
}
void verifyCommitted(const Step& expected,const overkill::Campaign& actual,bool ok,
                     const std::string& reason,const std::vector<overkill::Event>& events){
    const auto where=" at sequence "+std::to_string(expected.action.sequence);
    require(ok,"Rejected command"+where+": "+reason);
    require(reason==expected.reason,"Result reason differs"+where);
    require(overkill::campaignHash(actual)==expected.hash,"Campaign hash differs"+where);
    require(expected.expectedBytes.empty()||overkill::serializeCampaign(actual)==expected.expectedBytes,"Complete campaign/receipt bytes differ"+where);
    require(events.size()==expected.events.size(),"Event count differs"+where);
    for(std::size_t i=0;i<events.size();++i)require(overkill::eventJson(events[i])==expected.events[i],"Ordered event differs"+where+" index "+std::to_string(i));
    require(!actual.receipts.empty()&&actual.receipts.back().sequence==expected.action.sequence,"Receipt identity differs"+where);
    if(!expected.receiptCommand.empty())require(actual.receipts.back().command==expected.receiptCommand,"Receipt command differs"+where);
    require(actual.nextTransaction==expected.action.sequence+1,"Next transaction differs"+where);
}
Plan readPlan(std::istream& input,const overkill::CampaignRules& rules){
    Plan plan;std::string line,error;
    require(static_cast<bool>(std::getline(input,line))&&line=="OFCITY2","Expected baseline-5 OFCITY2 witness");
    require(static_cast<bool>(std::getline(input,line))&&line.rfind("META ",0)==0,"Missing witness metadata");
    std::istringstream meta(line.substr(5));std::string budget;
    meta>>std::quoted(plan.rulesVersion)>>std::quoted(plan.contentVersion)>>std::quoted(plan.manifest)
        >>std::quoted(plan.policyVersion)>>std::quoted(plan.policy)>>std::quoted(plan.precision)>>budget;
    endRow(meta,"Invalid witness metadata");
    require(plan.rulesVersion==overkill::RulesVersion&&plan.contentVersion==overkill::ContentVersion
        &&plan.manifest==rules.content().manifestHash,"Witness rules/content do not match this compiled core");
    require(plan.policyVersion=="baseline-5"&&(plan.policy=="aggressive"||plan.policy=="defensive")&&plan.precision=="auto",
        "This bounded rendered witness requires baseline-5 Auto inputs");
    require(!budget.empty()&&std::all_of(budget.begin(),budget.end(),[](char c){return c>='0'&&c<='9';}),"Invalid witness preview budget");
    plan.searchBudget=static_cast<std::size_t>(std::stoull(budget));require(plan.searchBudget>0,"Zero witness preview budget");
    require(static_cast<bool>(std::getline(input,line))&&line.rfind("START ",0)==0,"Missing canonical start");
    plan.initialBytes=unhex(line.substr(6));overkill::Campaign original;
    require(overkill::deserializeCampaign(plan.initialBytes,original,error),"Invalid initial campaign: "+error);
    plan.seed=original.seed;plan.runId=original.runId;
    require(plan.runId=="runner/"+std::to_string(plan.seed)+"/"+plan.policy+"/"+plan.precision,"Witness identity disagrees with metadata");
    auto state=rules.newGame(plan.seed,plan.runId);
    require(plan.initialBytes==overkill::serializeCampaign(state),"Witness starts with edited state instead of canonical Mara New Game");
    std::set<int> visited;
    while(std::getline(input,line)){
        if(line.rfind("FINAL ",0)==0){
            std::istringstream row(line.substr(6));std::string outcome,encoded;row>>plan.finalHash>>outcome>>encoded;endRow(row,"Invalid final witness record");
            plan.finalBytes=unhex(encoded);
            require(outcome=="city_complete"&&state.phase==overkill::CityPhase::Complete&&state.route.position==13,"Witness did not complete Cinderwall");
            require(plan.finalHash==overkill::campaignHash(state)&&plan.finalBytes==overkill::serializeCampaign(state),"Final witness bytes differ");
            require(!std::getline(input,line),"Trailing data after final witness");
            require(visited.size()==12,"Witness did not enter all twelve positions");
            return plan;
        }
        require(line.rfind("A ",0)==0,"Expected recorded action");Step step;step.encodedAction=unhex(line.substr(2));step.action=decodeAction(step.encodedAction);
        require(step.action.runId==state.runId&&step.action.sequence==state.nextTransaction,"Nonmonotonic or wrong-run recorded action");
        if(step.action.type==overkill::CampaignActionType::Combat&&step.action.combat.type==overkill::ActionType::Collect)
            require(step.action.combat.precision==-1,"Synthetic manual Precision is not allowed in this Auto witness");
        step.beforePhase=state.phase;step.beforePosition=state.route.position;
        if(step.action.type==overkill::CampaignActionType::EnterOffer)visited.insert(state.route.position);
        const auto result=rules.apply(state,step.action);
        require(static_cast<bool>(std::getline(input,line))&&line.rfind("R ",0)==0,"Missing recorded result");
        bool expectedOk=false;std::size_t count=0;std::istringstream row(line.substr(2));row>>expectedOk>>std::quoted(step.reason)>>step.hash>>count;endRow(row,"Invalid recorded result");
        require(expectedOk&&result.ok&&!result.replayed,"Witness contains a failed/replayed action; no substituted command is permitted");
        require(count==result.events.size(),"Recorded event count differs");
        for(std::size_t i=0;i<count;++i){require(static_cast<bool>(std::getline(input,line))&&line.rfind("E ",0)==0,"Missing recorded event");step.events.push_back(line.substr(2));}
        verifyCommitted(step,state,result.ok,result.reason,result.events);ordinaryEarnedInput(state);
        step.expectedBytes=overkill::serializeCampaign(state);step.receiptCommand=state.receipts.back().command;
        step.afterPhase=state.phase;step.afterPosition=state.route.position;step.round=state.fight.round;step.hp=state.fight.hp;step.encounter=state.fight.encounter;
        if(const auto* offer=overkill::selectedRouteOffer(state.route))step.mystery=offer->mystery;
        plan.steps.push_back(std::move(step));
    }
    throw std::runtime_error("Witness is truncated before final state");
}
std::string actionName(const overkill::CampaignAction& action){
    static const char* campaign[]={"mayor","enter-offer","enter-patrol","combat","open-recipes","back-recipes","claim-reward","request-advance","cancel-advance","confirm-advance","buy","sell-core","sell-part","calibration","leave-mystery","continue","open-shop","upgrade-choice","upgrade-offer","reroll-reward","preview-route","replace-route","cancel-exchange","move-copy"};
    static const char* combat[]={"collect","craft","install","remove","load","unload","fire","end-turn","activate","combat-upgrade-choice","activate-upgrade"};
    const auto c=static_cast<std::size_t>(action.type);
    if(action.type==overkill::CampaignActionType::Combat){const auto a=static_cast<std::size_t>(action.combat.type);return a<std::size(combat)?combat[a]:"unknown-combat";}
    return c<std::size(campaign)?campaign[c]:"unknown-campaign";
}
std::string phaseName(overkill::CityPhase phase){
    static const char* names[]={"arrival","between","fight","rewards","mystery","defeated","complete"};
    const auto index=static_cast<std::size_t>(phase);return index<std::size(names)?names[index]:"unknown";
}
std::string jsonQuote(const std::string& text){
    std::ostringstream out;out<<'"';for(const auto value:text){const auto byte=static_cast<unsigned char>(value);if(value=='"'||value=='\\')out<<'\\'<<value;else if(byte<32)out<<"\\u"<<std::hex<<std::setw(4)<<std::setfill('0')<<static_cast<unsigned>(byte);else out<<value;}out<<'"';return out.str();
}
std::string stepJson(std::size_t index,const Step& step,std::uint64_t revision){
    std::ostringstream out;out<<"{\"step\":"<<index<<",\"sequence\":"<<step.action.sequence<<",\"action\":"<<jsonQuote(actionName(step.action))
        <<",\"before_position\":"<<step.beforePosition<<",\"position\":"<<step.afterPosition<<",\"phase\":"<<jsonQuote(phaseName(step.afterPhase))
        <<",\"round\":"<<step.round<<",\"hp\":"<<step.hp<<",\"events\":"<<step.events.size()<<",\"hash\":"<<jsonQuote(step.hash)
        <<",\"revision\":"<<revision<<",\"receipt_command\":"<<jsonQuote(step.receiptCommand)<<",\"encounter\":"<<jsonQuote(step.encounter)<<",\"mystery\":"<<jsonQuote(step.mystery)<<'}';return out.str();
}
} // namespace foundry_city_probe

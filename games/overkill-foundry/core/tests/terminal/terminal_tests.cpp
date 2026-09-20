#include "overkill/campaign.hpp"
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace overkill;
namespace {
void need(bool v,const std::string& text){if(!v)throw std::runtime_error(text);}
Amount count(const std::vector<Event>& events,const std::string& type){return static_cast<Amount>(std::count_if(events.begin(),events.end(),[&](const Event& e){return e.type==type;}));}
Id exhaust(State& s){const Id id=s.nextId++;s.memory.push_back({id,"MA058",0,0,0});s.phase=Phase::Preparation;s.heat=5;s.materials={0,1,1,1,0};for(auto& e:s.enemies){e.hp=1;e.armor=e.shield=e.tiles=e.mesh=0;}return id;}
CampaignAction command(const Campaign& c,CampaignActionType type){CampaignAction a;a.type=type;a.runId=c.runId;a.sequence=c.nextTransaction;return a;}
CampaignResult apply(const CampaignRules& r,Campaign& c,const CampaignAction& a){auto result=r.apply(c,a);need(result.ok,result.reason);return result;}
}
int main(){try{
    Rules fights;auto raw=Rules::teachingEncounter(1,true);const auto copy=exhaust(raw);const auto unchanged=serialize(raw);const auto preview=fights.preview(raw,Action::craft(copy));need(preview.result.ok,preview.result.reason);need(serialize(raw)==unchanged,"Preview mutated live state");auto result=fights.apply(raw,Action::craft(copy));need(result.ok,result.reason);need(serialize(raw)==serialize(preview.state),"Preview and committed terminal state diverged");
    std::cout<<"CORE MA058 kills="<<raw.kills<<" victory_events="<<count(result.events,"victory")<<" ids=";for(const auto& e:result.events)if(e.type=="victory")std::cout<<e.id<<',';std::cout<<'\n';
    CampaignRules rules(fights,cinderwallUpgradeHooks(fights));auto c=rules.newGame(1,"duplicate-terminal");c.mayorOffers={"MY1-02","MY1-03","MY1-01"};auto a=command(c,CampaignActionType::ChooseMayor);a.choice="MY1-02";apply(rules,c,a);
    for(const auto* id:{"UGS-033","UGS-036","UGS-065","UGS-072","UGS-129"}){auto acquired=fights.acquireUpgrade(c.fight,id);need(acquired.ok,acquired.reason);c.everAcquired.push_back(id);}
    const auto offers=routeOffers(c.route);auto offer=std::find_if(offers.begin(),offers.end(),[](const RouteOffer& o){return o.kind==EncounterKind::Regular;});need(offer!=offers.end(),"No Regular fixture");a=command(c,CampaignActionType::EnterOffer);a.subject=offer->id;apply(rules,c,a);
    const auto recipe=exhaust(c.fight);c.fight.hp=30;const auto credits=c.fight.credits,maxHp=c.fight.maxHp;const auto receiptCount=c.receipts.size();const auto transaction=c.nextTransaction;
    a=command(c,CampaignActionType::Combat);a.combat=Action::craft(recipe);const auto completed=apply(rules,c,a);
    const auto entitlement=upgradeCounter(*ownedUpgrade(c.fight,"UGS-033"),"entitlement");const auto charges=ownedUpgrade(c.fight,"UGS-065")->charges;
    std::cout<<"CAMPAIGN victory_events="<<count(completed.events,"victory")<<" upgrade_victory_dispatches="<<std::count_if(completed.events.begin(),completed.events.end(),[](const Event& e){return e.type=="upgrade_campaign_event"&&e.amount==static_cast<Amount>(UpgradeEventKind::Victory);})<<" hp=30->"<<c.fight.hp<<" maxHp_delta="<<c.fight.maxHp-maxHp<<" credits_delta="<<c.fight.credits-credits<<" entitlement="<<entitlement<<" charges="<<charges<<" receipt_delta="<<c.receipts.size()-receiptCount<<" sequence_delta="<<c.nextTransaction-transaction<<'\n';
    need(c.fight.hp==43&&c.fight.maxHp==maxHp+1&&c.fight.credits==credits+10&&entitlement==1&&charges==0,"Duplicate gameplay effect detected");
    const auto bytes=serializeCampaign(c);auto replay=apply(rules,c,a);need(replay.replayed&&serializeCampaign(c)==bytes,"Receipt replay mutated state");
    auto dead=Rules::teachingEncounter(1,true);dead.enemies.resize(1);const auto deathCopy=exhaust(dead);dead.hp=1;dead.enemies.front().mesh=2;const auto death=fights.apply(dead,Action::craft(deathCopy));need(death.ok,death.reason);need(dead.kills==1&&dead.phase==Phase::Defeat,"Last-enemy lethal recoil did not give defeat priority");std::cout<<"CORE lethal recoil defeat_events="<<count(death.events,"defeat")<<" hp="<<dead.hp<<'\n';
    bool boundary=true;
    for(const auto phase:{Phase::Victory,Phase::Escaped}){
        // Controlled saved continuation checks the terminal boundary directly.
        State finished;finished.phase=phase;finished.kills=phase==Phase::Victory?1:0;UpgradeResolution frame;frame.after=UpgradeContinuation::Collection;finished.upgradeResolution.push_back(frame);
        auto repeated=fights.resumeUpgrades(finished);need(repeated.ok,repeated.reason);boundary=boundary&&count(repeated.events,"victory")==0&&count(repeated.events,"escape_complete")==0&&finished.phase==phase;
        std::cout<<"REPEATED phase="<<static_cast<int>(phase)<<" terminal_events="<<count(repeated.events,"victory")+count(repeated.events,"escape_complete")<<'\n';
    }
    State provisional;provisional.phase=Phase::Victory;provisional.kills=1;provisional.hp=0;UpgradeResolution frame;frame.after=UpgradeContinuation::Collection;provisional.upgradeResolution.push_back(frame);const auto lost=fights.resumeUpgrades(provisional);need(lost.ok,lost.reason);need(provisional.phase==Phase::Defeat&&count(lost.events,"defeat")==1,"A provisional Victory prevented final death");
    const bool unique=count(result.events,"victory")==1&&count(completed.events,"victory")==1&&count(death.events,"defeat")==1&&boundary;
    std::cout<<(unique?"PASS":"FAIL")<<" terminal notifications are unique; campaign completion effects and receipt were exactly once\n";return unique?0:1;
}catch(const std::exception& e){std::cerr<<"SETUP/ASSERTION FAILURE "<<e.what()<<'\n';return 2;}}

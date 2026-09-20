#include "overkill/campaign.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// Explicitly prepared route boundaries/resources isolate campaign contracts.
// All commands use production hooks; this is not an earned city/balance run.
using namespace overkill;
namespace {
int checks = 0;
void need(bool value, const std::string& why) { ++checks; if (!value) throw std::runtime_error(why); }
bool has(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}
CampaignAction command(const Campaign& c, CampaignActionType type, Id subject = 0, std::string choice = {}) {
    CampaignAction a; a.runId=c.runId; a.sequence=c.nextTransaction; a.type=type;
    a.subject=subject; a.choice=std::move(choice); return a;
}
CampaignResult act(Campaign& c, const CampaignRules& rules, const CampaignAction& a) {
    Campaign restored; std::string error;
    need(deserializeCampaign(serializeCampaign(c),restored,error), "Input round trip: "+error);
    auto result=rules.apply(c,a); need(result.ok,"Production command: "+result.reason);
    auto replay=rules.apply(restored,a);
    need(replay.ok && serializeCampaign(restored)==serializeCampaign(c),"Serialized replay changed full campaign");
    need(result.events.size()==replay.events.size(),"Replay event count");
    for (std::size_t i=0;i<result.events.size();++i)
        need(eventJson(result.events[i])==eventJson(replay.events[i]),"Replay ordered event payload");
    const auto committed=serializeCampaign(c); const auto duplicate=rules.apply(c,a);
    need(duplicate.ok && duplicate.replayed && serializeCampaign(c)==committed,"Exact receipt retry changed state");
    need(duplicate.events.size()==result.events.size(),"Receipt lost events");
    for (std::size_t i=0;i<result.events.size();++i)
        need(eventJson(duplicate.events[i])==eventJson(result.events[i]),"Receipt changed event identity/payload");
    return result;
}
void deny(Campaign& c,const CampaignRules& rules,const CampaignAction& a) {
    const auto before=serializeCampaign(c);const auto r=rules.apply(c,a);
    need(!r.ok && r.events.empty() && serializeCampaign(c)==before,"Rejected campaign command mutated state");
}
void combat(Campaign& c,const CampaignRules& rules,Action a) {
    auto transaction=command(c,CampaignActionType::Combat);transaction.combat=std::move(a);act(c,rules,transaction);
}
Campaign start(const CampaignRules& rules,std::uint64_t seed=1) {
    auto c=rules.newGame(seed,"prepared-lifecycle/"+std::to_string(seed));
    // Use one real Mayor implementation with no sale-price modifier. This
    // controlled choice is not evidence of a natural Mayor offer for every seed.
    c.mayorOffers={"MY1-M1","MY1-03","MY1-01"};act(c,rules,command(c,CampaignActionType::ChooseMayor,0,"MY1-M1"));return c;
}
Id product(const Campaign& c,ProductKind kind,const std::string& id) {
    for (const auto& p:c.shop) if(p.kind==kind && p.definition==id)return p.id;
    throw std::runtime_error("Missing real shop product");
}
void enterRam(Campaign& c,const CampaignRules& rules) {
    for(const auto& offer:routeOffers(c.route))if(offer.formation=="C1-F-RAM"){
        act(c,rules,command(c,CampaignActionType::EnterOffer,offer.id));return;
    }
    throw std::runtime_error("No Ram at prepared opening");
}
Id part(const Campaign& c,const std::string& recipe) {
    for(const auto& p:c.fight.parts)if(p.recipe==recipe)return p.id;
    throw std::runtime_error("Missing physical part");
}
void win(Campaign& c,const CampaignRules& rules,const Rules& fights) {
    if(c.fight.phase==Phase::Collection)combat(c,rules,Action::collect(0));
    // Clearly labelled terminal ammunition keeps lifecycle fixtures short.
    while(c.phase==CityPhase::Fight){
        const auto enemy=std::find_if(c.fight.enemies.begin(),c.fight.enemies.end(),[](const Enemy& e){return !e.dead&&!e.escaped&&e.hp>0;});
        need(enemy!=c.fight.enemies.end(),"Fight without live body");const Id target=enemy->id,first=c.fight.nextId;
        need(fights.grantPlainPart(c.fight,Kind::Ammo,500,"Prepared lifecycle terminal shot").ok,"Terminal fixture grant");
        Id ammo=0;for(const auto& p:c.fight.parts)if(p.id>=first)ammo=p.id;
        combat(c,rules,Action::load({ammo}));combat(c,rules,Action::fire(target));
    }
    need(c.phase==CityPhase::Rewards,"Victory did not reach Rewards");
}
Campaign boundary(const CampaignRules& rules,const std::string& mystery,bool boss=false) {
    for(std::uint64_t seed=1;seed<=100;++seed){auto c=start(rules,seed);
        while(c.route.position<=12){
            for(const auto& offer:routeOffers(c.route))if((boss&&offer.kind==EncounterKind::Boss)||(!boss&&offer.mystery==mystery)){
                act(c,rules,command(c,CampaignActionType::EnterOffer,offer.id));return c;
            }
            // Only the route prefix is prepared, not falsely simulated combat.
            std::string error;need(selectRouteOffer(c.route,routeOffers(c.route).front().id,error),error);
            need(completeRouteNode(c.route,error),error);
        }
    }
    throw std::runtime_error("Prepared route boundary not found");
}
Id reward(const Campaign& c,RewardKind kind) {
    for(const auto& r:c.rewards)if(r.kind==kind)return r.id;
    throw std::runtime_error("Missing expected reward");
}
void leaveRewards(Campaign& c,const CampaignRules& rules) {
    act(c,rules,command(c,CampaignActionType::RequestAdvance));
    if(c.skipConfirmation)act(c,rules,command(c,CampaignActionType::ConfirmAdvance));
}
}
int main(){try{
    const Rules fights;const CampaignRules rules(fights,cinderwallUpgradeHooks(fights));
    {
        auto c=start(rules);enterRam(c,rules);win(c,rules,fights);
        const auto balance=c.fight.credits;const Id cores=reward(c,RewardKind::Cores);
        act(c,rules,command(c,CampaignActionType::ClaimReward,cores));
        need(c.cores.size()==1 && c.cores[0].robot=="C1-R02" && c.cores[0].baseValue==15,"Killed Ram must allocate one15-credit core");
        need(c.fight.credits==balance,"Taking core cannot sell it");const Id id=c.cores[0].id;
        act(c,rules,command(c,CampaignActionType::SellCore,id));
        need(c.fight.credits==balance+15 && c.cores.empty(),"Core sale value/ownership");
        deny(c,rules,command(c,CampaignActionType::SellCore,id));
        deny(c,rules,command(c,CampaignActionType::ClaimReward,cores));
        std::cout<<"PASS actual body core identity, manual sale, exact receipt replay and no resell\n";
    }
    {
        auto c=start(rules);const auto buyId=product(c,ProductKind::Part,"SH002");
        auto buy=command(c,CampaignActionType::Buy,buyId);buy.quantity=2;act(c,rules,buy);
        std::vector<Id> ids;for(const auto& p:c.fight.parts)if(p.recipe=="SH002")ids.push_back(p.id);
        need(ids.size()==2,"Actual ready-part purchase must produce two identities");
        const auto balance=c.fight.credits;
        act(c,rules,command(c,CampaignActionType::SellPart,ids[0]));
        need(c.fight.credits==balance+4 && c.fight.parts.size()==1,"First9-credit material reference floors to4");
        act(c,rules,command(c,CampaignActionType::SellPart,ids[1]));
        need(c.fight.credits==balance+8 && c.fight.parts.empty(),"Two individual floors total8, not9");
        deny(c,rules,command(c,CampaignActionType::SellPart,ids[0]));
        std::cout<<"PASS purchased physical identities and canonical per-part rounding\n";
    }
    {
        auto c=start(rules);enterRam(c,rules);combat(c,rules,Action::collect(0));
        need(fights.grantPart(c.fight,"SH001",false).ok && fights.grantPart(c.fight,"SH002",false).ok,"Prepared standard parts");
        const Id ammo=part(c,"SH001"),shield=part(c,"SH002");
        combat(c,rules,Action::load({ammo}));deny(c,rules,command(c,CampaignActionType::SellPart,ammo));
        Action unload;unload.type=ActionType::Unload;combat(c,rules,unload);
        const auto balance=c.fight.credits;act(c,rules,command(c,CampaignActionType::SellPart,ammo));
        need(c.fight.credits==balance+2,"Unloaded unused Slug retains one-Iron reference");
        combat(c,rules,Action::install(shield));deny(c,rules,command(c,CampaignActionType::SellPart,shield));
        combat(c,rules,Action::remove(shield));act(c,rules,command(c,CampaignActionType::SellPart,shield));
        need(c.fight.credits==balance+6,"Untouched removed ordinary Shield stays saleable at4");
        need(fights.grantPart(c.fight,"SH002",false).ok && fights.grantPart(c.fight,"SH001",false).ok,"Second standard part fixture");
        const Id spent=part(c,"SH002");combat(c,rules,Action::install(spent));
        c.fight.enemies[0].mesh=2; // Controlled reaction spends actual protection.
        combat(c,rules,Action::load({part(c,"SH001")}));combat(c,rules,Action::fire(c.fight.enemies[0].id));
        combat(c,rules,Action::remove(spent));deny(c,rules,command(c,CampaignActionType::SellPart,spent));
        std::cout<<"PASS loaded and installed sale rejection, unload/removal eligibility and spent-Shield exclusion\n";
    }
    {
        auto c=start(rules);need(c.profile.recipes.size()==12,"NewGame discovers twelve starters");
        const auto memory=c.fight.memory.size();const auto balance=c.fight.credits;
        std::vector<std::string> offered;for(const auto& p:c.shop)if(p.kind==ProductKind::Recipe)offered.push_back(p.definition);
        need(std::any_of(offered.begin(),offered.end(),[&](const std::string& id){return !has(c.profile.recipes,id);}),"Shop has an unseen recipe");
        act(c,rules,command(c,CampaignActionType::OpenShop));
        for(const auto& id:offered)need(has(c.profile.recipes,id),"Viewing a shop offer discovers without buying");
        need(c.fight.memory.size()==memory && c.fight.credits==balance,"Seeing an offer cannot buy it");
        const auto known=c.profile.recipes;act(c,rules,command(c,CampaignActionType::OpenShop));
        need(c.profile.recipes==known,"Repeated shop viewing duplicates discovery");
        enterRam(c,rules);win(c,rules,fights);const Id id=reward(c,RewardKind::Recipe);
        auto options=c.rewards[1].choices;for(const auto& r:c.rewards)if(r.id==id)options=r.choices;
        act(c,rules,command(c,CampaignActionType::OpenRecipes,id));
        for(const auto& option:options)need(has(c.profile.recipes,option),"Seeing reward candidates discovers all alternatives");
        const auto discoveries=c.profile.recipes;act(c,rules,command(c,CampaignActionType::BackRecipes));leaveRewards(c,rules);
        need(c.profile.recipes==discoveries && c.fight.memory.size()==memory,"Skipping preserves seen facts without granting recipes");
        std::cout<<"PASS seen shop/reward alternatives, repeated discovery and skipped-choice persistence\n";
    }
    {
        auto c=boundary(rules,"C1-M-PATROL");const auto position=c.route.position,stock=c.shopGeneration;
        deny(c,rules,command(c,CampaignActionType::LeaveMystery));
        act(c,rules,command(c,CampaignActionType::EnterPatrol));need(c.route.position==position,"Patrol entry cannot complete the node");
        const auto entry=stateHash(c.fight);act(c,rules,command(c,CampaignActionType::Continue));
        need(stateHash(c.fight)==entry && c.shopGeneration==stock,"Patrol Continue preserves original entry and stock");
        win(c,rules,fights);need(c.route.position==position+1 && c.shopGeneration==stock+1,"Patrol victory progresses/restocks exactly once");
        const auto normal=std::count_if(c.rewards.begin(),c.rewards.end(),[](const RewardEntry& r){return r.kind==RewardKind::Recipe&&r.pool=="Regular";});
        need(normal==1 && reward(c,RewardKind::Cores)>0,"Patrol grants normal recipe choice and legitimate killed cores");
        leaveRewards(c,rules);need(c.phase==CityPhase::Between && c.route.position==position+1,"Leaving rewards cannot advance twice");
        deny(c,rules,command(c,CampaignActionType::EnterPatrol));
        std::cout<<"PASS Patrol compulsory combat, unchanged Continue, normal rewards and single progression\n";
    }
    {
        auto c=boundary(rules,{},true);need(!has(c.profile.unlocked,"Ivo"),"Unfinished boss cannot unlock Ivo");
        win(c,rules,fights);need(c.route.position==13 && has(c.profile.unlocked,"Ivo") && c.profile.cityClearReceipts.size()==1,"Actual boss victory unlocks Ivo once");
        need(!has(c.profile.unlocked,"Ada") && !has(c.profile.unlocked,"Noor"),"One Mara city cannot skip character order");
        leaveRewards(c,rules);need(c.phase==CityPhase::Complete,"Boss reward completion reaches city Complete");
        const auto facts=c.profile;c.cores.push_back({c.nextId++,"C1-R02",15});c.fight.credits=777;
        const auto fresh=rules.newGame(991,"replacement-lifecycle",facts);
        need(fresh.phase==CityPhase::Arrival && fresh.fight.hp==80 && fresh.fight.maxHp==80 && fresh.fight.credits==100,"New Game resets campaign economy/HP");
        need(fresh.cores.empty() && fresh.fight.parts.empty() && fresh.fight.materials==Materials{} && fresh.fight.memory.size()==12,"New Game clears supplies and restores starter memory");
        need(fresh.profile.recipes==facts.recipes && fresh.profile.unlocked==facts.unlocked && fresh.profile.cityClearReceipts==facts.cityClearReceipts,"New Game preserves discoveries and earned access");
        std::cout<<"PASS real boss result, single Ivo unlock, completion and profile facts across New Game\n";
    }
    std::cout<<"CAMPAIGN_LIFECYCLE_OK groups=6 assertions="<<checks<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<"FAIL after "<<checks<<": "<<error.what()<<'\n';return 1;}}

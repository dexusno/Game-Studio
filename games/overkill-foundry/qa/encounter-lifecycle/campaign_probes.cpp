#include "overkill/campaign.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>

// Prepared route prefixes, terminal ammunition and explicit late-fight damage
// states isolate lifecycle boundaries. No disk persistence or earned build claim.
using namespace overkill;
namespace {
int checks=0,passed=0,failed=0;
void require(bool b,const std::string& message){++checks;if(!b)throw std::runtime_error(message);}
void group(const char* name,const std::function<void()>& test){try{test();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cout<<"FAIL "<<name<<": "<<e.what()<<'\n';}}
bool contains(const std::vector<std::string>& a,const std::string& b){return std::find(a.begin(),a.end(),b)!=a.end();}
CampaignAction command(const Campaign& c,CampaignActionType kind,Id subject=0,const std::string& choice={}){
    CampaignAction a;a.runId=c.runId;a.sequence=c.nextTransaction;a.type=kind;a.subject=subject;a.choice=choice;return a;
}
CampaignResult act(const CampaignRules& rules,Campaign& c,const CampaignAction& a){
    Campaign copy;std::string error;require(deserializeCampaign(serializeCampaign(c),copy,error),"snapshot accepted: "+error);
    const auto r=rules.apply(c,a);require(r.ok,r.reason);const auto replay=rules.apply(copy,a);
    require(replay.ok && serializeCampaign(copy)==serializeCampaign(c),"full campaign replay");
    require(replay.events.size()==r.events.size(),"event count replay");
    for(std::size_t i=0;i<r.events.size();++i)require(eventJson(r.events[i])==eventJson(replay.events[i]),"ordered event replay");
    return r;
}
void deny(const CampaignRules& rules,Campaign& c,const CampaignAction& a){const auto bytes=serializeCampaign(c);const auto r=rules.apply(c,a);require(!r.ok && r.events.empty() && serializeCampaign(c)==bytes,"rejected action is atomic");}
CampaignResult fight(const CampaignRules& rules,Campaign& c,const Action& a){auto cmd=command(c,CampaignActionType::Combat);cmd.combat=a;return act(rules,c,cmd);}
Campaign fresh(const CampaignRules& rules,std::uint64_t seed=1){
    auto c=rules.newGame(seed,"independent-lifecycle/"+std::to_string(seed));
    c.mayorOffers={"MY1-M1","MY1-01","MY1-03"};act(rules,c,command(c,CampaignActionType::ChooseMayor,0,"MY1-M1"));return c;
}
void enter(const CampaignRules& rules,Campaign& c,const std::string& formation){
    for(const auto& o:routeOffers(c.route))if(o.formation==formation){act(rules,c,command(c,CampaignActionType::EnterOffer,o.id));return;}
    throw std::runtime_error("required real opening formation absent");
}
Campaign atBoundary(const CampaignRules& rules,bool boss){
    for(std::uint64_t seed=1;seed<=100;++seed){auto c=fresh(rules,seed);
        while(c.route.position<=12){
            for(const auto& o:routeOffers(c.route))if(boss?o.kind==EncounterKind::Boss:o.mystery=="C1-M-PATROL"){
                act(rules,c,command(c,CampaignActionType::EnterOffer,o.id));return c;
            }
            std::string error;require(selectRouteOffer(c.route,routeOffers(c.route).front().id,error),error);require(completeRouteNode(c.route,error),error);
        }
    }
    throw std::runtime_error("prepared route boundary not found");
}
Id plain(const Rules& fights,Campaign& c,Amount n){
    const Id first=c.fight.nextId;const auto r=fights.grantPlainPart(c.fight,Kind::Ammo,n,"independent terminal fixture");require(r.ok,r.reason);
    for(const auto& p:c.fight.parts)if(p.id>=first)return p.id;throw std::runtime_error("physical grant missing");
}
void win(const Rules& fights,const CampaignRules& rules,Campaign& c){
    if(c.fight.phase==Phase::Collection)fight(rules,c,Action::collect(0));
    while(c.phase==CityPhase::Fight){
        const auto e=std::find_if(c.fight.enemies.begin(),c.fight.enemies.end(),[](const Enemy& v){return v.hp>0&&!v.dead&&!v.escaped;});
        require(e!=c.fight.enemies.end(),"nonterminal fight has live target");const Id target=e->id,ammo=plain(fights,c,500);
        fight(rules,c,Action::load({ammo}));fight(rules,c,Action::fire(target));
    }
    require(c.phase==CityPhase::Rewards,"fixture produces actual victory");
}
Id copy(State& s,const std::string& id){const Id n=s.nextId++;s.memory.push_back({n,id,0,0,0});return n;}
Id reserve(const State& s,const std::string& id){for(const auto& p:s.parts)if(p.recipe==id&&p.place==Place::Reserve)return p.id;throw std::runtime_error("reserve part missing");}
}
int main(){
    const Rules fights;const CampaignRules rules(fights,cinderwallUpgradeHooks(fights));
    group("L01 simultaneous final boss death and lethal recoil cannot unlock Ivo or commit rewards",[&]{
        auto c=atBoundary(rules,true);fight(rules,c,Action::collect(0));const auto position=c.route.position;
        require(c.fight.enemies.size()==1 && c.fight.enemies[0].definition=="C1-B01","real boss body");
        // This is an explicitly prepared last-hit/recoil boundary, not an earned mesh boss.
        c.fight.hp=1;c.fight.enemies[0].mesh=2;const Id target=c.fight.enemies[0].id;
        fight(rules,c,Action::load({plain(fights,c,500)}));const auto r=fight(rules,c,Action::fire(target));
        require(std::any_of(r.events.begin(),r.events.end(),[](const Event& e){return e.type=="enemy_death";}),"robot really dies in resolution");
        require(c.phase==CityPhase::Defeated && c.fight.hp==0 && c.route.position==position,"death wins without route completion");
        require(!contains(c.profile.unlocked,"Ivo") && c.profile.cityClearReceipts.empty() && c.rewards.empty() && c.cores.empty(),"no unlock, receipt or loot from defeat");
        deny(rules,c,command(c,CampaignActionType::Continue));deny(rules,c,command(c,CampaignActionType::RequestAdvance));
    });
    group("L02 Patrol defeat ends campaign without a noncombat payout or route completion",[&]{
        auto c=atBoundary(rules,false);const auto before=c.fight.credits,position=c.route.position,stock=c.shopGeneration;
        deny(rules,c,command(c,CampaignActionType::LeaveMystery));act(rules,c,command(c,CampaignActionType::EnterPatrol));
        require(c.fight.encounterClass==EncounterClass::Regular,"Patrol uses Regular fight class");
        fight(rules,c,Action::collect(0));c.fight.hp=1;c.fight.burn=1;fight(rules,c,Action::endTurn());
        require(c.phase==CityPhase::Defeated && c.fight.credits==before && c.route.position==position && c.shopGeneration==stock,"no noncombat ten Credits, advancement or restock on defeat");
        require(c.rewards.empty() && c.cores.empty(),"no Patrol defeat rewards");deny(rules,c,command(c,CampaignActionType::EnterPatrol));
    });
    group("L03 actual purchase rollback keeps seen recipe but restores money memory and stock on Continue",[&]{
        auto c=atBoundary(rules,false);act(rules,c,command(c,CampaignActionType::EnterPatrol));
        const auto oldMoney=c.fight.credits;const auto oldMemory=c.fight.memory.size();
        const auto it=std::find_if(c.shop.begin(),c.shop.end(),[&](const Product& p){return p.kind==ProductKind::Recipe && p.price<=oldMoney && !contains(c.profile.recipes,p.definition);});
        require(it!=c.shop.end(),"actual unseen affordable stock for boundary");const auto product=*it;
        act(rules,c,command(c,CampaignActionType::Buy,product.id));
        require(contains(c.profile.recipes,product.definition) && c.fight.memory.size()==oldMemory+1 && c.fight.credits<oldMoney,"buy without opening shop discovers and pays");
        act(rules,c,command(c,CampaignActionType::Continue));
        require(contains(c.profile.recipes,product.definition) && c.fight.memory.size()==oldMemory && c.fight.credits==oldMoney,"facts persist while attempt economy rolls back");
        const auto restored=std::find_if(c.shop.begin(),c.shop.end(),[&](const Product& p){return p.id==product.id;});
        require(restored!=c.shop.end() && restored->quantity==product.quantity,"original stock and identity restored");
        const auto profile=c.profile.recipes;act(rules,c,command(c,CampaignActionType::Continue));require(c.profile.recipes==profile,"repeated Continue does not duplicate discoveries");
    });
    group("L04 paid source copied parts and generated equivalent sell by the current configured one-part reference",[&]{
        auto data=cinderwallContent();data.materialPrices={5,9,7,11,13};
        const CampaignRules priceRules(fights,cinderwallUpgradeHooks(fights,data),data);auto c=fresh(priceRules);enter(priceRules,c,"C1-F-RAM");fight(priceRules,c,Action::collect(0));
        c.fight.materials={20,20,20,20,20};fight(priceRules,c,Action::craft(copy(c.fight,"SH012")));
        const Id original=reserve(c.fight,"SH012");auto duplicate=Action::craft(copy(c.fight,"SH102"));duplicate.parts={original};fight(priceRules,c,duplicate);
        fight(priceRules,c,Action::endTurn());fight(priceRules,c,Action::collect(0));
        std::vector<Id> ids;for(const auto& p:c.fight.parts)if(p.recipe=="SH012")ids.push_back(p.id);
        require(ids.size()==3 && ids[0]!=ids[1] && ids[0]!=ids[2] && ids[1]!=ids[2],"paid source and two delivered physical copies");
        const auto money=c.fight.credits;for(Id id:ids)act(priceRules,c,command(c,CampaignActionType::SellPart,id));
        require(c.fight.credits==money+15,"each two-Iron source/copy reference sells for five, independent of utility cost");
        const Id generated=plain(fights,c,6);act(priceRules,c,command(c,CampaignActionType::SellPart,generated));
        require(c.fight.credits==money+17,"free plain-six equivalent uses one five-credit Iron, floors to two");
    });
    group("L05 unseen reward alternatives are not discovered by main skip or cancellation",[&]{
        auto c=fresh(rules,3);enter(rules,c,"C1-F-RAM");const auto known=c.profile.recipes;win(fights,rules,c);
        bool unseen=false;for(const auto& reward:c.rewards)for(const auto& id:reward.choices)unseen=unseen||!contains(known,id);
        require(unseen && c.profile.recipes==known,"generating unviewed rewards does not discover them");
        act(rules,c,command(c,CampaignActionType::RequestAdvance));act(rules,c,command(c,CampaignActionType::CancelAdvance));
        require(c.profile.recipes==known && c.phase==CityPhase::Rewards,"cancel main skip does not reveal alternatives");
        act(rules,c,command(c,CampaignActionType::RequestAdvance));act(rules,c,command(c,CampaignActionType::ConfirmAdvance));
        require(c.profile.recipes==known && c.phase==CityPhase::Between,"abandoning unviewed recipes never grants Collection facts");
    });
    group("L06 two actual killed bodies produce distinct manually sold cores and stale sales cannot pay twice",[&]{
        auto c=fresh(rules);enter(rules,c,"C1-F-MITE-RAM");win(fights,rules,c);Id reward=0;
        for(const auto& item:c.rewards)if(item.kind==RewardKind::Cores)reward=item.id;require(reward!=0,"core reward available");
        const auto money=c.fight.credits;act(rules,c,command(c,CampaignActionType::ClaimReward,reward));
        require(c.cores.size()==2 && c.cores[0].id!=c.cores[1].id && c.fight.credits==money,"taking two cores is ownership only");
        const auto first=c.cores[0],second=c.cores[1];require(first.baseValue+second.baseValue==20,"Mite five plus Ram fifteen");
        auto sale=command(c,CampaignActionType::SellCore,first.id);act(rules,c,sale);const auto bytes=serializeCampaign(c);
        const auto retry=rules.apply(c,sale);require(retry.ok && retry.replayed && serializeCampaign(c)==bytes,"same sale receipt pays once");
        deny(rules,c,command(c,CampaignActionType::SellCore,first.id));act(rules,c,command(c,CampaignActionType::SellCore,second.id));
        require(c.fight.credits==money+20 && c.cores.empty(),"both cores sell once at their assigned values");
    });
    group("L07 defeat preserves actually seen recipes and a fresh independent profile receives no leaked facts",[&]{
        auto c=fresh(rules);enter(rules,c,"C1-F-RAM");fight(rules,c,Action::collect(0));
        act(rules,c,command(c,CampaignActionType::OpenShop));const auto seen=c.profile.recipes;
        require(seen.size()>12,"actual shop viewing added discoveries");
        c.fight.hp=1;c.fight.burn=1;fight(rules,c,Action::endTurn());
        require(c.phase==CityPhase::Defeated && c.profile.recipes==seen && !contains(c.profile.unlocked,"Ivo"),"terminal death preserves discoveries but creates no unlock");
        const auto replacement=rules.newGame(42,"independent replacement",c.profile);
        const auto separate=rules.newGame(43,"independent empty profile");
        require(replacement.profile.recipes==seen && separate.profile.recipes.size()==12,"explicit ProfileFacts copy persists; separate value is isolated");
        require(replacement.fight.memory.size()==12 && replacement.cores.empty() && replacement.fight.parts.empty(),"seen history never restores old campaign ownership");
    });
    std::cout<<"INDEPENDENT_LIFECYCLE groups="<<passed<<" failed="<<failed<<" assertions="<<checks<<'\n';return failed?1:0;
}

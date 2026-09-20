#include "overkill/campaign.hpp"
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

using namespace overkill;
namespace {
int checks=0,passed=0,failed=0;
const Rules rules;
void require(bool x,const std::string& reason){++checks;if(!x)throw std::runtime_error(reason);}
void group(const char* name,const std::function<void()>& body){try{body();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cout<<"FAIL "<<name<<": "<<e.what()<<'\n';}}
State scene(std::uint64_t seed=17){State s;s.hp=50;s.hotBarrel=false;s.phase=Phase::Preparation;s.encounter="qa-upgrade-contract";s.rng=Rng::seeded(seed,s.encounter);s.materials={50,50,50,50,50};for(int i=0;i<2;++i){Enemy e;e.id=s.nextId++;e.definition="fixture";e.name="Controlled target";e.hp=e.maxHp=100;e.pattern={{Move::Recover,0,0}};e.intent=e.pattern.front();s.enemies.push_back(e);}return s;}
Result act(State& s,const Action& a){auto r=rules.apply(s,a);require(r.ok,r.reason);return r;}
void own(State& s,const std::string& id){const auto r=rules.acquireUpgrade(s,id);require(r.ok,r.reason);}
void begin(State& s){const auto r=rules.startUpgrades(s,EncounterClass::Regular);require(r.ok,r.reason);}
Id store(State& s,const std::string& id){RecipeCopy c;c.id=s.nextId++;c.recipe=id;s.memory.push_back(c);return c.id;}
Id make(State& s,const std::string& id,Action a={}){a.type=ActionType::Craft;a.subject=store(s,id);const auto before=s.nextId;act(s,a);for(const auto& p:s.parts)if(p.id>=before&&p.creator==id)return p.id;return 0;}
std::size_t madeBy(const State& s,const std::string& id){return std::count_if(s.parts.begin(),s.parts.end(),[&](const Part& p){return p.creator==id;});}
std::string events(const Result& r){std::string v;for(const auto& e:r.events)v+=eventJson(e)+"\n";return v;}
void reload(State& s){const auto bytes=serialize(s);State copy;std::string error;require(deserialize(bytes,copy,error),error);require(serialize(copy)==bytes,"State reload differs");s=copy;}
void materialChoice(State& s,Amount value){require(!s.upgradeChoices.empty(),"Expected source material choice");Action a;a.type=ActionType::ResolveUpgradeChoice;a.upgradeChoice.choice=s.upgradeChoices.front().id;a.upgradeChoice.values={value};act(s,a);}
CampaignAction command(const Campaign& c,CampaignActionType type,Id id=0,const std::string& choice={}){CampaignAction a;a.type=type;a.subject=id;a.choice=choice;a.sequence=c.nextTransaction;a.runId=c.runId;return a;}
CampaignResult transact(const CampaignRules& cr,Campaign& c,CampaignAction a){auto r=cr.apply(c,a);require(r.ok,r.reason);return r;}
void combat(const CampaignRules& cr,Campaign& c,const Action& a){auto x=command(c,CampaignActionType::Combat);x.combat=a;transact(cr,c,x);}
Campaign campaign(const CampaignRules& cr,std::uint64_t seed=7){auto c=cr.newGame(seed,"qa-contract-"+std::to_string(seed));c.mayorOffers={"MY1-02","MY1-01","MY1-03"};transact(cr,c,command(c,CampaignActionType::ChooseMayor,0,"MY1-02"));return c;}
Id stock(Campaign& c,const std::string& id,Amount price){const auto key=c.nextId++;c.shop.push_back({key,ProductKind::Upgrade,id,0,1,price});return key;}
void buy(const CampaignRules& cr,Campaign& c,const std::string& id,Amount price=150){c.fight.credits=1000;transact(cr,c,command(c,CampaignActionType::Buy,stock(c,id,price)));}
void enter(const CampaignRules& cr,Campaign& c){const auto offers=routeOffers(c.route);for(const auto& o:offers)if(o.kind==EncounterKind::Regular){transact(cr,c,command(c,CampaignActionType::EnterOffer,o.id));return;}throw std::runtime_error("No Regular entry");}
void win(const CampaignRules& cr,Campaign& c){
    // Explicit low-HP/known-recipe/material controls. Damage, payment and reward
    // dispatch remain production code; this is not natural campaign evidence.
    enter(cr,c);combat(cr,c,Action::collect(0));c.fight.materials={50,50,50,50,50};
    for(auto& e:c.fight.enemies)e.hp=1;
    std::size_t fired=0;
    while(c.phase==CityPhase::Fight){require(++fired<=5,"Controlled fight did not finish");Id enemy=0;for(const auto& e:c.fight.enemies)if(!e.dead&&!e.escaped){enemy=e.id;break;}
        const auto copy=store(c.fight,"SH109");const auto before=c.fight.nextId;combat(cr,c,Action::craft(copy));Id part=0;for(const auto& p:c.fight.parts)if(p.id>=before&&p.creator=="SH109"){part=p.id;break;}
        require(part!=0,"Paid terminal recipe output missing");combat(cr,c,Action::load({part}));combat(cr,c,Action::fire(enemy));
        // Remove only the controlled extra recipe copy after its real paid Use;
        // it never participates in the reward acceptance being tested.
        c.fight.memory.erase(std::remove_if(c.fight.memory.begin(),c.fight.memory.end(),[&](const RecipeCopy& r){return r.id==copy;}),c.fight.memory.end());
    }
    require(c.phase==CityPhase::Rewards,"Expected actual victory rewards");
}
RewardEntry& normal(Campaign& c){for(auto& r:c.rewards)if(r.kind==RewardKind::Recipe&&r.normalOffer&&r.claim==ClaimState::Pending)return r;throw std::runtime_error("Missing normal reward");}
void leave(const CampaignRules& cr,Campaign& c){transact(cr,c,command(c,CampaignActionType::RequestAdvance));if(c.skipConfirmation)transact(cr,c,command(c,CampaignActionType::ConfirmAdvance));}
}

int main(){
group("Q01 six source spreading Modifiers retain typed outputs and deterministic paid Shield reward",[]{
    for(const auto* id:{"SH005","SH051","SH082","SH110","MA041","MA111"}){auto s=scene();own(s,"UGS-095");begin(s);act(s,Action::collect(0));const auto copy=store(s,id);const auto before=serialize(s);const auto preview=rules.preview(s,Action::craft(copy));require(preview.result.ok,preview.result.reason);require(serialize(s)==before,"Preview mutated authoritative state");reload(s);const auto result=act(s,Action::craft(copy));require(serialize(s)==serialize(preview.state)&&events(result)==events(preview.result),"Preview/reload/action differ");require(Rules::shield(s)==7&&madeBy(s,"UGS-095")==1,"Missing exactly one Shield7");auto p=std::find_if(s.parts.begin(),s.parts.end(),[&](const Part& x){return x.creator==id;});require(p!=s.parts.end()&&p->kind==Kind::Spread&&p->place==Place::Reserve,"Source category repair changed physical placement");const auto beforeGrant=serialize(s);auto granted=rules.grantPart(s,id);require(granted.ok,granted.reason);require(madeBy(s,"UGS-095")==1&&serialize(s)!=beforeGrant,"Free physical grant incorrectly triggered reward");}
});
group("Q02 fully discounted Spread Use does not consume first resource-paid Modifier allowance",[]{
    bool witnessed=false;for(std::uint64_t seed=1;seed<=64&&!witnessed;++seed){auto s=scene(seed);own(s,"UGS-095");own(s,"MY1-09");own(s,"UGS-116");begin(s);if(ownedUpgrade(s,"UGS-116")->values.front()!=1)continue;witnessed=true;act(s,Action::collect(0));for(int i=0;i<3;++i)make(s,"MA001");const auto before=s.materials;Action discount;discount.discount={2,0,0,0,0};require(make(s,"SH005",discount)!=0,"Free Spread output absent");require(s.materials==before&&Rules::shield(s)==0,"Zero actual resources triggered paid-only reward");make(s,"SH036");require(Rules::shield(s)==7,"Free Spread consumed later paid planning-Modifier allowance");}require(witnessed,"Copper discount witness missing");
});
group("Q03 paid Spread reward protects later real Fire recoil without changing support damage",[]{
    auto s=scene();own(s,"UGS-095");begin(s);act(s,Action::collect(0));const auto spread=make(s,"SH005"),ammo=make(s,"SH001");s.enemies[0].mesh=2;const auto hp=s.hp;act(s,Action::load({ammo,spread}));Action fire=Action::fire(s.enemies[0].id);fire.spreadTargets={{spread,s.enemies[1].id}};act(s,fire);require(s.hp==hp&&Rules::shield(s)==5,"Fresh Shield7 failed to absorb later recoil2");require(s.enemies[0].hp==94&&s.enemies[1].hp==97,"Spread no longer contributes its source-defined separate50percent hit");require(!std::any_of(s.parts.begin(),s.parts.end(),[&](const Part& p){return p.id==ammo||p.id==spread;}),"Fired Ammo/Spread were not consumed");
});
group("Q04 second-victory Shared Modifier filter includes all four spread rarities and excludes Mara",[]{
    for(const auto* selected:{"SH005","SH051","SH082","SH110","MA041"}){auto content=cinderwallContent();const auto* target=rules.recipe(selected);require(target!=nullptr,"Source recipe unavailable");std::vector<std::string> pool;for(const auto& r:content.recipes){const auto* def=rules.recipe(r.id);if(def&&r.id.substr(0,2)=="SH"&&def->rarity==target->rarity&&def->kind!=Kind::Modifier&&def->kind!=Kind::Spread&&pool.size()<4)pool.push_back(r.id);}require(pool.size()==4,"Four non-Modifier controls required");pool.push_back(selected);for(auto& p:content.recipePools)if(p.name=="Regular"){p.ids=pool;p.weights={};p.weights[static_cast<std::size_t>(target->rarity)]=1;}
        const CampaignRules cr(rules,cinderwallUpgradeHooks(rules,content),content);bool witnessed=false;
        for(std::uint64_t seed=1;seed<=24&&!witnessed;++seed){auto c=campaign(cr,seed);buy(cr,c,"UGS-062");win(cr,c);require(normal(c).choices.size()==3,"First victory incorrectly gained fourth option");leave(cr,c);win(cr,c);const auto choices=normal(c).choices;require(std::count(choices.begin(),choices.end(),selected)<=1,"Additional option duplicated existing card");if(std::string(selected).substr(0,2)=="MA"){require(choices.size()==3,"Shared-only fourth option admitted Mara Spread");witnessed=true;}else if(choices.size()==4){require(choices.back()==selected,"Only eligible source Modifier was not the added option");witnessed=true;}else require(choices.size()==3&&std::find(choices.begin(),choices.end(),selected)!=choices.end(),"Eligible unused Spread silently omitted");}
        require(witnessed,"No independently executed fourth-option witness");
    }
});
group("Q05 Bond uses original price gate and exact discounted affordability before all mutations",[]{
    const CampaignRules cr(rules,cinderwallUpgradeHooks(rules));
    for(Amount original:{99,100,101,102}){auto c=campaign(cr);buy(cr,c,"UGS-075");const auto product=stock(c,"UGS-085",original);const Amount payable=original*80/100;c.fight.credits=payable-1;auto a=command(c,CampaignActionType::Buy,product);auto before=serializeCampaign(c);auto r=cr.apply(c,a);require(!r.ok&&r.events.empty()&&serializeCampaign(c)==before,"Unaffordable/gated purchase mutated state");c.fight.credits=payable;a=command(c,CampaignActionType::Buy,product);before=serializeCampaign(c);r=cr.apply(c,a);if(original<=100){require(!r.ok&&r.events.empty()&&serializeCampaign(c)==before,"Original price<=100 bypassed gate under discount");}else{require(r.ok&&c.fight.credits==100&&ownedUpgrade(c.fight,"UGS-085"),"Exact affordable discounted purchase failed or credited wrong amount");const auto committed=serializeCampaign(c);std::string receiptEvents;for(const auto& e:r.events)receiptEvents+=eventJson(e);r=cr.apply(c,a);std::string replayEvents;for(const auto& e:r.events)replayEvents+=eventJson(e);require(r.ok&&r.replayed&&replayEvents==receiptEvents&&serializeCampaign(c)==committed,"Receipt replay changed acquisition state or canonical recorded events");const auto duplicate=stock(c,"UGS-085",150);c.fight.credits=1000;const auto saved=serializeCampaign(c);r=cr.apply(c,command(c,CampaignActionType::Buy,duplicate));require(!r.ok&&r.events.empty()&&serializeCampaign(c)==saved,"Second stock identity duplicated permanent acquisition");}}
    auto s=scene();const auto credits=s.credits;own(s,"UGS-085");require(s.credits==credits+100,"Nonpurchase acquisition improperly requires shop context");
});
group("Q06 Cooling Dividend grants fresh Copper when a real discount removed all Copper payment",[]{
    bool witnessed=false;for(std::uint64_t seed=1;seed<=64&&!witnessed;++seed){auto s=scene(seed);own(s,"MY2-01");own(s,"UGS-116");begin(s);if(ownedUpgrade(s,"UGS-116")->values.front()!=1)continue;witnessed=true;act(s,Action::collect(0));const auto cooling=store(s,"SH051");act(s,Action::craft(cooling));act(s,Action::endTurn());act(s,Action::collect(0));s.materials[1]=0;const auto glass=s.materials[3];const auto priorCooldown=std::find_if(s.memory.begin(),s.memory.end(),[&](const RecipeCopy& c){return c.id==cooling;})->cooldown;require(priorCooldown>0,"Cooling control must have a live prior-round cooldown");reload(s);make(s,"SH007");require(s.materials[1]==1&&s.materials[3]==glass-1,"Fresh Copper was capped to zero actually paid Copper");const auto item=std::find_if(s.memory.begin(),s.memory.end(),[&](const RecipeCopy& c){return c.id==cooling;});require(item!=s.memory.end()&&item->cooldown==priorCooldown-1,"Real prior-round cooldown did not reduce by one");}require(witnessed,"Copper-discount source witness missing");
});
group("Q07 actual recipe damage and recoil finish before refund or third-use material reward",[]{
    for(const auto* id:{"MY1-08","MY1-11"})for(Amount hp:{1,3}){auto s=scene();own(s,id);begin(s);materialChoice(s,2);act(s,Action::collect(0));if(std::string(id)=="MY1-11"){make(s,"MA001");make(s,"MA001");}s.hp=hp;s.heat=5;s.enemies[0].mesh=2;const auto carbon=s.materials[2];make(s,"MA058");const Amount paid=rules.recipe("MA058")->cost[2];const Amount reward=hp==1?0:(std::string(id)=="MY1-08"?std::min(3,paid):5);require(s.materials[2]==carbon-paid+reward,"Carbon awarded before lethal recoil or missing after survival");require((hp==1)==(s.phase==Phase::Defeat),"Expected actual recoil survival boundary");}
});
group("Q08 fifth and tenth actual victory acceptances heal independently across persisted exchanges",[]{
    const CampaignRules cr(rules,cinderwallUpgradeHooks(rules));auto c=campaign(cr);buy(cr,c,"UGS-016");c.fight.hp=40;
    for(int n=1;n<=10;++n){win(cr,c);auto& r=normal(c);r.choices={"SH001","SH002","SH003"};const auto id=r.id;transact(cr,c,command(c,CampaignActionType::OpenRecipes,id));auto a=command(c,CampaignActionType::ClaimReward,id,"SH001");if(c.fight.memory.size()>=20)a.exchange=c.fight.memory.front().id;transact(cr,c,a);require(c.fight.hp==40+8*(n/5),"Repeated fifth-acceptance cadence differs");const auto saved=serializeCampaign(c);Campaign restored;std::string error;require(deserializeCampaign(saved,restored,error),error);require(serializeCampaign(restored)==saved,"Saved reward/exchange state differs");c=restored;const auto replay=cr.apply(c,a);require(replay.ok&&replay.replayed&&serializeCampaign(c)==saved,"Acceptance replay repeated counter/heal");leave(cr,c);}
});
group("Q09 Jig bonus is unique across Lease copies in either acquisition order and preserves other modifiers",[]{
    std::vector<std::string> mismatches;
    for(bool withOther:{false,true})for(bool jigFirst:{false,true}){auto s=scene();const auto copy=store(s,"SH001");if(withOther){own(s,"UGS-046");Action selected;selected.type=ActionType::ResolveUpgradeChoice;selected.upgradeChoice.choice=s.upgradeChoices.front().id;selected.upgradeChoice.objects={copy};act(s,selected);}own(s,jigFirst?"UGS-121":"UGS-015");own(s,jigFirst?"UGS-015":"UGS-121");begin(s);act(s,Action::collect(0));const auto before=s.nextId;act(s,Action::craft(copy));std::vector<Id> parts;Amount bonus=0;for(const auto& p:s.parts)if(p.id>=before&&(p.creator=="SH001"||p.creator=="UGS-015")){parts.push_back(p.id);bonus+=p.upgradeDamage;}require(parts.size()==2,"Paid lease must produce exactly two slugs");reload(s);const auto hp=s.enemies[0].hp;act(s,Action::load(parts));act(s,Action::fire(s.enemies[0].id));const Amount expectedBonus=withOther?4:2,expectedDamage=12+expectedBonus;if(bonus!=expectedBonus||hp-s.enemies[0].hp!=expectedDamage)mismatches.push_back("jigFirst="+std::to_string(jigFirst)+", other="+std::to_string(withOther)+", bonus="+std::to_string(bonus)+", damage="+std::to_string(hp-s.enemies[0].hp)+", expected="+std::to_string(expectedDamage));}
    std::string message;for(const auto& mismatch:mismatches)message+=mismatch+"; ";require(mismatches.empty(),message);
});
group("Q10 later printed-effect duplicates omit Jig and attached damage bonuses",[]{
    auto s=scene();const auto copy=store(s,"SH001");own(s,"UGS-046");Action selected;selected.type=ActionType::ResolveUpgradeChoice;selected.upgradeChoice.choice=s.upgradeChoices.front().id;selected.upgradeChoice.objects={copy};act(s,selected);own(s,"UGS-121");begin(s);act(s,Action::collect(0));const auto before=s.nextId;act(s,Action::craft(copy));Id original=0;for(const auto& p:s.parts)if(p.id>=before&&p.creator=="SH001")original=p.id;require(original!=0,"Original paid slug required");Action duplicate;duplicate.parts={original};make(s,"SH118",duplicate);reload(s);act(s,Action::endTurn());act(s,Action::collect(0));std::vector<Id> parts{original};for(const auto& p:s.parts)if(p.creator=="SH118"){require(p.upgradeDamage==0,"Printed-only copy retained later bonuses");parts.push_back(p.id);}require(parts.size()==3,"Original plus two delayed copies required");act(s,Action::load(parts));const auto hp=s.enemies[0].hp;act(s,Action::fire(s.enemies[0].id));require(hp-s.enemies[0].hp==21,"Expected original9 plus two printed6 copies");
});
std::cout<<"SUMMARY "<<passed<<" passed, "<<failed<<" failed, "<<checks<<" assertions\n";return failed?1:0;
}

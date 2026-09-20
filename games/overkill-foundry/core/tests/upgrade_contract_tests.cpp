#include "overkill/campaign.hpp"
#include "overkill/upgrades.hpp"
#include "overkill/robots.hpp"
#include "../../presentation/precision.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
using namespace overkill;
namespace {
int assertions=0,passed=0,failed=0;
Rules fights;
CampaignRules campaigns(fights,cinderwallUpgradeHooks(fights));
std::map<std::string,std::pair<std::string,std::function<void()>>> cases;
void check(bool value,const std::string& why){++assertions;if(!value)throw std::runtime_error(why);}
void add(const std::string& id,const std::string& title,std::function<void()> test){check(cases.emplace(id,std::make_pair(title,std::move(test))).second,"Duplicate semantic group");}
Result act(State& s,const Action& action){auto r=fights.apply(s,action);check(r.ok,r.reason);return r;}
void reject(State& s,const Action& action){const auto before=serialize(s);const auto r=fights.apply(s,action);check(!r.ok&&r.events.empty()&&serialize(s)==before,"Invalid action was not rejected atomically");}
void snapshot(State& s){const auto bytes=serialize(s);State loaded;std::string error;check(deserialize(bytes,loaded,error),error);check(serialize(loaded)==bytes,"Canonical reload changed state");s=std::move(loaded);}
Id memory(State& s,const std::string& id){const auto value=s.nextId++;s.memory.push_back({value,id,0,0,0});return value;}
RecipeCopy& copy(State& s,Id id){for(auto& c:s.memory)if(c.id==id)return c;throw std::runtime_error("Missing recipe copy");}
Part& part(State& s,Id id){for(auto& p:s.parts)if(p.id==id)return p;throw std::runtime_error("Missing physical part");}
bool exists(const State& s,Id id){return std::any_of(s.parts.begin(),s.parts.end(),[&](const Part& p){return p.id==id;});}
std::vector<Id> outputs(const State& s,const std::string& source){std::vector<Id> out;for(const auto& p:s.parts)if(p.creator==source)out.push_back(p.id);return out;}
State arena(){State s;s.phase=Phase::Preparation;s.hotBarrel=false;s.hp=40;s.materials={100,100,100,100,100};s.rng=Rng::seeded(43,"upgrade-contract");s.encounter="upgrade-contract";for(int i=0;i<3;++i){Enemy e;e.id=s.nextId++;e.definition="fixture";e.name="Quiet target";e.hp=e.maxHp=10000;e.pattern={{Move::Recover,0,0}};e.intent=e.pattern.front();s.enemies.push_back(e);}return s;}
void acquire(State& s,const std::string& id){auto r=fights.acquireUpgrade(s,id);check(r.ok,r.reason);const auto saved=serialize(s);r=fights.acquireUpgrade(s,id);check(!r.ok&&r.events.empty()&&serialize(s)==saved,"Duplicate upgrade acquisition mutated its ledger");}
Result start(State& s,EncounterClass kind=EncounterClass::Regular){auto r=fights.startUpgrades(s,kind);check(r.ok,r.reason);return r;}
void collect(State& s,Amount precision=-1,Amount steering=0){act(s,Action::collect(steering,precision));}
Result end(State& s){return act(s,Action::endTurn());}
void next(State& s){end(s);check(s.phase==Phase::Collection,"Controlled fight ended early");if(s.upgradeChoices.empty()&&s.upgradeRequests.empty())collect(s);}
void answer(State& s,UpgradeAnswer a){check(!s.upgradeChoices.empty(),"Missing typed decision");a.choice=s.upgradeChoices.front().id;Action action;action.type=ActionType::ResolveUpgradeChoice;action.upgradeChoice=a;act(s,action);reject(s,action);}
void selectMaterial(State& s,Amount value){UpgradeAnswer a;a.values={value};answer(s,a);}
void selectCopies(State& s,std::vector<Id> ids){UpgradeAnswer a;a.objects=std::move(ids);answer(s,a);}
std::vector<Id> craft(State& s,Id recipe,Action a={}){const auto before=s.nextId;a.type=ActionType::Craft;a.subject=recipe;act(s,a);std::vector<Id> out;for(const auto& p:s.parts)if(p.id>=before)out.push_back(p.id);return out;}
std::vector<Id> use(State& s,const std::string& recipe,Action a={}){return craft(s,memory(s,recipe),a);}
Id grant(State& s,const std::string& recipe){const auto before=s.nextId;const auto r=fights.grantPart(s,recipe);check(r.ok,r.reason);for(const auto& p:s.parts)if(p.id>=before)return p.id;throw std::runtime_error("Grant produced no physical part");}
Id plain(State& s,Kind kind,Amount n,bool installed=false){const auto before=s.nextId;const auto r=fights.grantPlainPart(s,kind,n,"controlled-upgrade-fixture",installed);check(r.ok,r.reason);for(const auto& p:s.parts)if(p.id>=before)return p.id;throw std::runtime_error("Plain fixture part missing");}
Result fire(State& s,std::vector<Id> ids,Id target=0,Action a={}){act(s,Action::load(std::move(ids)));a.type=ActionType::Fire;a.target=target?target:s.enemies[0].id;return act(s,a);}
Amount shot(State& s,const std::string& recipe="SH001",Id target=0){if(!target)target=s.enemies[0].id;Amount hp=0;for(const auto& e:s.enemies)if(e.id==target)hp=e.hp;const auto ids=use(s,recipe);fire(s,ids,target);for(const auto& e:s.enemies)if(e.id==target)return hp-e.hp;throw std::runtime_error("Shot target disappeared");}
void activate(State& s,Id id,Action a={}){a.type=ActionType::Activate;a.subject=id;act(s,a);}
Materials delta(const Materials& after,const Materials& before){Materials d{};for(std::size_t i=0;i<5;++i)d[i]=after[i]-before[i];return d;}
Amount total(const Materials& v){return std::accumulate(v.begin(),v.end(),Amount{});}
Amount eventAmount(const Result& result,const std::string& type,const std::string& source={}){Amount sum=0;for(const auto& e:result.events)if(e.type==type&&(source.empty()||e.source==source))sum+=e.amount;return sum;}
void attacks(State& s,Amount damage,Amount hits=1){for(auto& e:s.enemies)e.intent=e.pattern[0]={Move::Attack,damage,hits};}
State renewed(const State& old){auto s=arena();s.nextId=old.nextId;s.nextOrder=old.nextOrder;s.upgrades=old.upgrades;s.memory=old.memory;s.fightSerial=old.fightSerial;for(auto& e:s.enemies)e.id=s.nextId++;for(auto& c:s.memory){c.cooldown=c.usedRound=c.usesThisRound=0;}return s;}
CampaignAction command(const Campaign& c,CampaignActionType type,Id subject=0,const std::string& choice={}){CampaignAction a;a.type=type;a.subject=subject;a.choice=choice;a.runId=c.runId;a.sequence=c.nextTransaction;return a;}
CampaignResult run(Campaign& c,CampaignAction a){const auto r=campaigns.apply(c,a);check(r.ok,r.reason);return r;}
void reject(Campaign& c,CampaignAction a){const auto before=serializeCampaign(c);const auto r=campaigns.apply(c,a);check(!r.ok&&r.events.empty()&&serializeCampaign(c)==before,"Campaign rejection changed state");}
void snapshot(Campaign& c){const auto bytes=serializeCampaign(c);Campaign loaded;std::string error;check(deserializeCampaign(bytes,loaded,error),error);check(serializeCampaign(loaded)==bytes,"Campaign reload changed bytes");c=std::move(loaded);}
Campaign campaign(const std::string& mayor="MY1-02",std::uint64_t seed=7){auto c=campaigns.newGame(seed,"upgrade-contract-"+std::to_string(seed));c.mayorOffers={mayor};for(const auto* other:{"MY1-01","MY1-02","MY1-03"})if(c.mayorOffers.size()<3&&mayor!=other)c.mayorOffers.push_back(other);run(c,command(c,CampaignActionType::ChooseMayor,0,mayor));return c;}
void buy(Campaign& c,ProductKind kind,const std::string& id,Amount price=10,Id exchange=0){c.fight.credits=std::max(c.fight.credits,10000);const auto product=c.nextId++;c.shop.push_back({product,kind,id,0,1,price});auto a=command(c,CampaignActionType::Buy,product);a.exchange=exchange;run(c,a);const auto bytes=serializeCampaign(c);const auto replay=campaigns.apply(c,a);check(replay.ok&&replay.replayed&&serializeCampaign(c)==bytes,"Purchase receipt replay repeated acquisition/payment");}
void combat(Campaign& c,Action a){auto cmd=command(c,CampaignActionType::Combat);cmd.combat=std::move(a);run(c,cmd);}
void enter(Campaign& c){const auto offers=routeOffers(c.route);for(const auto& o:offers)if(o.kind==EncounterKind::Regular){run(c,command(c,CampaignActionType::EnterOffer,o.id));return;}throw std::runtime_error("No controlled regular route");}
void finish(Campaign& c,bool escape=false,bool defeat=false){
    if(c.fight.phase==Phase::Collection)combat(c,Action::collect(0));
    if(escape){for(auto& e:c.fight.enemies){e.robotAction.clear();e.pattern={{Move::Escape,0,0}};e.intent=e.pattern.front();}combat(c,Action::endTurn());}
    else{if(defeat){c.fight.hp=1;c.fight.enemies.front().mesh=1000;}
        while(c.phase==CityPhase::Fight){Id target=0;for(const auto& e:c.fight.enemies)if(!e.dead&&!e.escaped){target=e.id;break;}check(target!=0,"Missing terminal target");const auto ammo=plain(c.fight,Kind::Ammo,500);combat(c,Action::load({ammo}));combat(c,Action::fire(target));}}
    check(c.phase==(defeat?CityPhase::Defeated:CityPhase::Rewards),"Wrong terminal fixture outcome");
}
void leave(Campaign& c){run(c,command(c,CampaignActionType::RequestAdvance));if(c.skipConfirmation)run(c,command(c,CampaignActionType::ConfirmAdvance));}
RewardEntry& reward(Campaign& c,RewardKind kind=RewardKind::Recipe,bool normal=true){for(auto& r:c.rewards)if(r.kind==kind&&r.claim==ClaimState::Pending&&(kind!=RewardKind::Recipe||r.normalOffer==normal))return r;throw std::runtime_error("Missing pending reward");}
Id acceptRecipe(Campaign& c,const std::string& id,Id exchange=0,Id rewardId=0){if(!rewardId)rewardId=reward(c).id;run(c,command(c,CampaignActionType::OpenRecipes,rewardId));auto a=command(c,CampaignActionType::ClaimReward,rewardId,id);a.exchange=exchange;const auto before=c.fight.nextId;run(c,a);for(const auto& copy:c.fight.memory)if(copy.id>=before)return copy.id;return 0;}
void controlledRecipes(Campaign& c,const std::vector<std::string>& ids,Id rewardId=0){if(!rewardId)rewardId=reward(c).id;for(auto& r:c.rewards)if(r.id==rewardId){r.choices=ids;r.lightTouch.clear();return;}throw std::runtime_error("Missing controlled reward");}
std::size_t recipeRewards(const Campaign& c,bool normal=true){return static_cast<std::size_t>(std::count_if(c.rewards.begin(),c.rewards.end(),[=](const RewardEntry& r){return r.kind==RewardKind::Recipe&&r.normalOffer==normal;}));}
void enterAvailable(Campaign& c,bool officer){const auto offers=routeOffers(c.route);for(const auto& o:offers)if((officer&&o.kind==EncounterKind::Officer)||o.kind==EncounterKind::Boss){run(c,command(c,CampaignActionType::EnterOffer,o.id));return;}enter(c);}
void controlledCityBoundary(Campaign& c){UpgradeEvent event;event.kind=UpgradeEventKind::CityStart;const auto result=cinderwallUpgradeHooks(fights).notifyUpgrade(c,event);check(result.ok,result.reason);snapshot(c);}
#include "upgrade-contracts/campaign_cases.inl"
#include "upgrade-contracts/rewards.inl"
#include "upgrade-contracts/edge_clauses.inl"
#include "upgrade-contracts/entry_turn.inl"
#include "upgrade-contracts/end_turn.inl"
#include "upgrade-contracts/production.inl"
#include "upgrade-contracts/combat.inl"
}
int main(){entryTurnCases();endTurnCases();productionCases();combatCases();campaignCases();rewardCases();edgeCases();for(const auto& [id,test]:cases){try{test.second();++passed;std::cout<<"PASS upgrade:"<<id<<" "<<test.first<<'\n';}catch(const std::exception& error){++failed;std::cerr<<"FAIL upgrade:"<<id<<": "<<error.what()<<'\n';}}std::cout<<"SUMMARY "<<passed<<" passed, "<<failed<<" failed, "<<assertions<<" assertions. Held UGS-011/018/126/141 are not proposed.\n";return failed?1:0;}

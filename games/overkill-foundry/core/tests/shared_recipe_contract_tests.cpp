#include "overkill/campaign.hpp"
#include "overkill/upgrades.hpp"
#include "overkill/robots.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
using namespace overkill;
namespace {
int assertions=0,passed=0,failed=0;
Rules rules;
std::map<std::string,std::pair<std::string,std::function<void()>>> cases;
#include "shared-recipes/source-costs.inc"
void check(bool value,const std::string& why){++assertions;if(!value)throw std::runtime_error(why);}
void add(const std::string& id,const std::string& title,std::function<void()> test){check(cases.emplace(id,std::make_pair(title,std::move(test))).second,"Duplicate named case");}
State arena(int enemies=3){State s;s.phase=Phase::Preparation;s.hotBarrel=false;s.hp=s.maxHp=1000;s.materials={100,100,100,100,100};s.encounter="shared-recipe-contracts";s.rng=Rng::seeded(43,s.encounter);for(int i=0;i<enemies;++i){Enemy e;e.id=s.nextId++;e.definition="controlled";e.name="Controlled target";e.hp=e.maxHp=10000;e.intent={Move::Recover,0,0};e.pattern={e.intent};s.enemies.push_back(e);}return s;}
Result act(State& s,const Action& a){const auto before=serialize(s);const auto preview=rules.preview(s,a);check(serialize(s)==before,"Preview mutated live state");auto r=rules.apply(s,a);check(r.ok,r.reason);check(preview.result.ok&&serialize(preview.state)==serialize(s),"Preview and committed state differ");check(preview.result.events.size()==r.events.size(),"Preview event count");for(std::size_t i=0;i<r.events.size();++i)check(eventJson(r.events[i])==eventJson(preview.result.events[i]),"Preview event differs");return r;}
void reject(State& s,const Action& a){const auto before=serialize(s);const auto r=rules.apply(s,a);check(!r.ok&&r.events.empty()&&serialize(s)==before,"Invalid action must reject atomically");}
void snapshot(State& s){const auto before=serialize(s);State restored;std::string error;check(deserialize(before,restored,error),error);check(serialize(restored)==before,"Reload identity");s=std::move(restored);}
Id memory(State& s,const std::string& id){const Id n=s.nextId++;s.memory.push_back({n,id,0,0,0});return n;}
Part& part(State& s,Id id){for(auto& p:s.parts)if(p.id==id)return p;throw std::runtime_error("Missing physical part");}
bool exists(const State& s,Id id){return std::any_of(s.parts.begin(),s.parts.end(),[&](const Part& p){return p.id==id;});}
RecipeCopy& copy(State& s,Id id){for(auto& c:s.memory)if(c.id==id)return c;throw std::runtime_error("Missing recipe copy");}
Materials delta(const Materials& a,const Materials& b){Materials out{};for(std::size_t i=0;i<5;++i)out[i]=a[i]-b[i];return out;}
Amount total(const Materials& a){return std::accumulate(a.begin(),a.end(),Amount{});}
std::vector<Id> created(const State& s,Id since){std::vector<Id> out;for(const auto& p:s.parts)if(p.id>=since)out.push_back(p.id);return out;}
std::vector<Id> outputs(const State& s,const std::string& source){std::vector<Id> out;for(const auto& p:s.parts)if(p.creator==source)out.push_back(p.id);return out;}
Result paid(State& s,Id recipe,Action a={},Materials discount={}){a.type=ActionType::Craft;a.subject=recipe;const auto id=copy(s,recipe).recipe;Materials cost=printedCosts.at(id);for(std::size_t i=0;i<5;++i)cost[i]-=discount[i];for(std::size_t i=0;i<5;++i)if(cost[i]>0){auto poor=s;poor.materials[i]=cost[i]-1;reject(poor,a);}const auto before=s.spentThisRound;auto r=act(s,a);check(delta(s.spentThisRound,before)==cost,id+" actual paid materials");int uses=0;for(const auto& event:r.events)if(event.type=="recipe_used"){++uses;check(event.subject==recipe&&event.source==id&&event.paid==cost,id+" source copy and payment event");}check(uses==1,id+" exactly one paid Use");const auto printed=printedCooldowns.at(id);Amount expected=printed;if(id=="SH067")expected=std::max(0,printed-2);if(id=="SH068"||id=="SH100"||id=="SH007")expected=std::max(0,printed-1);if(id=="SH099"||id=="SH116")expected=0;check(copy(s,recipe).cooldown==expected&&copy(s,recipe).usedRound==s.round,id+" source cooldown assigned at paid Use");if(printed==0||expected>0){auto repeated=s;repeated.materials={1000,1000,1000,1000,1000};const auto rejected=rules.apply(repeated,a);check(!rejected.ok&&(rejected.reason.find("already been used")!=std::string::npos||rejected.reason.find("cooling")!=std::string::npos),id+" exact physical copy cannot repeat before ready");}return r;}
std::vector<Id> use(State& s,const std::string& id,Action a={},Materials discount={}){const Id recipe=memory(s,id),first=s.nextId;paid(s,recipe,a,discount);return created(s,first);}
Id make(State& s,const std::string& id,Action a={}){const auto p=use(s,id,a);check(p.size()==1,id+" single physical output");check(part(s,p[0]).sourceRecipeCopy!=0&&part(s,p[0]).origin==PartOrigin::Produced,id+" paid production provenance");return p[0];}
Result end(State& s){return act(s,Action::endTurn());}
void collect(State& s,Amount precision=-1,Amount steering=0,Materials discarded={}){auto a=Action::collect(steering,precision);a.discarded=discarded;act(s,a);}
void next(State& s){end(s);check(s.phase==Phase::Collection,"Controlled fight unexpectedly ended");collect(s);}
Result fire(State& s,std::vector<Id> ids,Id target=0,Action a={}){act(s,Action::load(ids));a.type=ActionType::Fire;a.target=target?target:s.enemies.front().id;auto result=act(s,a);for(Id id:ids)check(!exists(s,id),"Fired physical output consumed");return result;}
void activate(State& s,Id id,Action a={}){a.type=ActionType::Activate;a.subject=id;act(s,a);check(!exists(s,id),"Planning output consumed on activation");}
void install(State& s,Id id){act(s,Action::install(id));}
void remove(State& s,Id id){act(s,Action::remove(id));}
Amount firedAmount(const Result& result){for(const auto& e:result.events)if(e.type=="fire")return e.amount;throw std::runtime_error("No main Fire event");}
Amount eventAmount(const Result& result,const std::string& type){Amount n=0;for(const auto& e:result.events)if(e.type==type)n+=e.amount;return n;}
std::size_t events(const Result& result,const std::string& type){return static_cast<std::size_t>(std::count_if(result.events.begin(),result.events.end(),[&](const Event& e){return e.type==type;}));}
void attack(State& s,std::size_t index,Amount damage,Amount hits=1){s.enemies[index].intent={Move::Attack,damage,hits};s.enemies[index].pattern={s.enemies[index].intent};}
Id fixedShield(State& s,Amount n,bool installed=true){const Id first=s.nextId;auto r=rules.grantPlainPart(s,Kind::Shield,n,"controlled-fixed-shield",installed);check(r.ok,r.reason);const auto ids=created(s,first);check(ids.size()==1,"Controlled physical Shield grant");return ids[0];}
void savedValue(State& s,Id id,Amount expected){check(part(s,id).shield==expected,"First-install source value");remove(s,id);snapshot(s);install(s,id);check(part(s,id).shield==expected,"Reinstall preserves snapshot");}
#include "shared-recipes/ammo.inl"
#include "shared-recipes/shield.inl"
#include "shared-recipes/utility.inl"
#include "shared-recipes/collection.inl"
#include "shared-recipes/boundaries.inl"
}
int main(){ammoCases();shieldCases();utilityCases();collectionCases();boundaryCases();for(const auto& [id,test]:cases){try{test.second();++passed;std::cout<<"PASS recipe:"<<id<<" "<<test.first<<'\n';}catch(const std::exception& e){++failed;std::cerr<<"FAIL recipe:"<<id<<": "<<e.what()<<'\n';}}std::cout<<"SUMMARY "<<passed<<" passed, "<<failed<<" failed, "<<assertions<<" assertions.\n";return failed?1:0;}

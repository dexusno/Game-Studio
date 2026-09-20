#include "overkill/core.hpp"
#include "overkill/campaign.hpp"
#include "overkill/upgrades.hpp"
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
struct Contract {std::string coverage;std::function<void()> test;};
std::map<std::string,Contract> contracts;
#include "mara-recipes/source-costs.inc"
void check(bool value,const std::string& why){++assertions;if(!value)throw std::runtime_error(why);}
void add(const std::string& id,const std::string& coverage,std::function<void()> test){check(assignedIds.count(id)==1,"Case outside assigned107: "+id);check(contracts.emplace(id,Contract{coverage,std::move(test)}).second,"Duplicate case "+id);}
void extend(const std::string& id,const std::string& coverage,std::function<void()> test){auto& contract=contracts.at(id);auto original=contract.test;contract.coverage+="; "+coverage;contract.test=[original,test]{original();test();};}
State arena(int enemies=3){State s;s.phase=Phase::Preparation;s.hotBarrel=false;s.hp=s.maxHp=1000;s.materials={100,100,100,100,100};s.encounter="mara-semantic-contracts";s.rng=Rng::seeded(107,s.encounter);for(int i=0;i<enemies;++i){Enemy e;e.id=s.nextId++;e.definition="CONTROL";e.name="Controlled target";e.hp=e.maxHp=10000;e.intent={Move::Recover,0,0};e.pattern={e.intent};s.enemies.push_back(e);}return s;}
Part& part(State& s,Id id){for(auto& p:s.parts)if(p.id==id)return p;throw std::runtime_error("Missing part "+std::to_string(id));}
bool exists(const State& s,Id id){return std::any_of(s.parts.begin(),s.parts.end(),[&](const Part& p){return p.id==id;});}
RecipeCopy& copy(State& s,Id id){for(auto& c:s.memory)if(c.id==id)return c;throw std::runtime_error("Missing recipe copy");}
Result act(State& s,const Action& action){check(s.heat>=0&&s.heat<=upgradeHeatCap(s),"Fixture Heat must respect actual capacity");const auto before=serialize(s);const auto preview=rules.preview(s,action);check(serialize(s)==before,"Preview mutated live state");auto result=rules.apply(s,action);check(result.ok,result.reason);check(s.heat>=0&&s.heat<=upgradeHeatCap(s),"Committed Heat stays within actual capacity");check(preview.result.ok&&serialize(preview.state)==serialize(s),"Preview differs from paid commit");check(preview.result.events.size()==result.events.size(),"Preview event count differs");for(std::size_t i=0;i<result.events.size();++i)check(eventJson(result.events[i])==eventJson(preview.result.events[i]),"Preview ordered event differs");return result;}
void reject(State& s,const Action& action){const auto before=serialize(s);const auto result=rules.apply(s,action);check(!result.ok&&result.events.empty()&&serialize(s)==before,"Rejected action must preserve complete state, IDs and RNG");}
void snapshot(State& s){const auto bytes=serialize(s);State restored;std::string error;check(deserialize(bytes,restored,error),error);check(serialize(restored)==bytes,"Snapshot identity");s=std::move(restored);}
Id memory(State& s,const std::string& id){const auto value=s.nextId++;s.memory.push_back({value,id,0,0,0});return value;}
Materials delta(const Materials& a,const Materials& b){Materials result{};for(std::size_t i=0;i<5;++i)result[i]=a[i]-b[i];return result;}
Amount shield(const State& s){return Rules::shield(s);}
std::vector<Id> created(const State& s,Id first){std::vector<Id> ids;for(const auto& p:s.parts)if(p.id>=first)ids.push_back(p.id);return ids;}
Result paid(State& s,Id recipe,Action action={}){action.type=ActionType::Craft;action.subject=recipe;const auto id=copy(s,recipe).recipe;const auto cost=printedCosts.at(id);for(std::size_t i=0;i<5;++i)if(cost[i]>0){auto poor=s;poor.materials[i]=cost[i]-1;reject(poor,action);}const auto before=s.spentThisRound;auto result=act(s,action);check(delta(s.spentThisRound,before)==cost,id+" exact printed material payment");int uses=0;for(const auto& e:result.events)if(e.type=="recipe_used"){++uses;check(e.subject==recipe&&e.source==id&&e.paid==cost,id+" exact source/payment receipt");}check(uses==1,id+" one recipe Use");return result;}
Result use(State& s,const std::string& id,Action action={}){return paid(s,memory(s,id),action);}
Id make(State& s,const std::string& id,Action action={}){const auto first=s.nextId;use(s,id,action);const auto outputs=created(s,first);check(outputs.size()==1,id+" one paid physical output");check(part(s,outputs[0]).sourceRecipeCopy!=0&&part(s,outputs[0]).origin==PartOrigin::Produced,id+" crafted provenance");return outputs[0];}
Result activate(State& s,Id id,Action action={}){action.type=ActionType::Activate;action.subject=id;auto result=act(s,action);check(!exists(s,id),"Activation consumes planning part");return result;}
Result install(State& s,Id id,Action action={}){action.type=ActionType::Install;action.subject=id;return act(s,action);}
void remove(State& s,Id id){act(s,Action::remove(id));}
Result fire(State& s,std::vector<Id> ids,Id target=0,Action action={}){act(s,Action::load(ids));action.type=ActionType::Fire;action.target=target?target:s.enemies[0].id;auto result=act(s,action);for(auto id:ids)check(!exists(s,id),"Fire consumes exact loaded part");return result;}
Result shot(State& s,const std::string& id,Action action={}){return fire(s,{make(s,id)},0,action);}
Result end(State& s){return act(s,Action::endTurn());}
void collect(State& s){act(s,Action::collect(0));}
void next(State& s){end(s);check(s.phase==Phase::Collection,"Controlled encounter ended unexpectedly");collect(s);}
Amount amount(const Result& result,const std::string& type){Amount out=0;for(const auto& event:result.events)if(event.type==type)out+=event.amount;return out;}
int count(const Result& result,const std::string& type){return static_cast<int>(std::count_if(result.events.begin(),result.events.end(),[&](const Event& event){return event.type==type;}));}
Amount damage(const Result& result){for(const auto& event:result.events)if(event.type=="fire")return event.amount;throw std::runtime_error("Missing Fire event");}
void attack(State& s,std::size_t i,Amount value,Amount hits=1){s.enemies[i].intent={Move::Attack,value,hits};s.enemies[i].pattern={s.enemies[i].intent};}
void recover(State& s,std::size_t i){s.enemies[i].intent={Move::Recover,0,0};s.enemies[i].pattern={s.enemies[i].intent};}
Id plate(State& s,const std::string& id="SH002"){const auto p=make(s,id);install(s,p);return p;}
void retainedValue(State& s,Id id,Amount expected){check(part(s,id).shield==expected,"First-install calculated Shield value");remove(s,id);snapshot(s);install(s,id);check(part(s,id).shield==expected,"Reinstall keeps first-install value");}
#include "mara-recipes/ammo.inl"
#include "mara-recipes/shield.inl"
#include "mara-recipes/modifiers.inl"
#include "mara-recipes/utility.inl"
#include "mara-recipes/boundaries.inl"
}
int main(int argc,char** argv){try{ammoCases();shieldCases();modifierCases();utilityCases();boundaryCases();check(contracts.size()==assignedIds.size(),"Every assigned ID needs an explicit semantic contract");std::string selected;if(argc!=1){check(argc==3&&std::string(argv[1])=="--case","Usage: mara_recipe_contract_tests [--case MA###]");selected=argv[2];check(contracts.count(selected)==1,"Unknown recipe case");}for(const auto& [id,contract]:contracts){if(!selected.empty()&&selected!=id)continue;const auto before=assertions;try{contract.test();++passed;std::cout<<"PASS recipe:"<<id<<" assertions="<<assertions-before<<" "<<contract.coverage<<'\n';}catch(const std::exception& e){++failed;std::cerr<<"FAIL recipe:"<<id<<" assertions="<<assertions-before<<": "<<e.what()<<'\n';}}std::cout<<"SUMMARY "<<passed<<" passed, "<<failed<<" failed, "<<assertions<<" assertions.\n";return failed?1:0;}catch(const std::exception& e){std::cerr<<"HARNESS_FAILURE "<<e.what()<<'\n';return 2;}}

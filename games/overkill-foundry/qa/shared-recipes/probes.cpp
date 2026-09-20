#include "overkill/core.hpp"
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
const Rules rules;
int checks=0,passed=0,failed=0;
void need(bool value,const std::string& why){++checks;if(!value)throw std::runtime_error(why);}
void test(const char* name,const std::function<void()>& body){try{body();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cout<<"FAIL "<<name<<": "<<e.what()<<'\n';}}
State scene(std::uint64_t seed=37){State s;s.hp=s.maxHp=80;s.phase=Phase::Preparation;s.hotBarrel=false;s.materials={200,200,200,200,200};s.encounter="independent-shared";s.rng=Rng::seeded(seed,s.encounter);for(int i=0;i<2;++i){Enemy e;e.id=s.nextId++;e.definition="controlled";e.name="Quiet test target";e.hp=e.maxHp=100;e.intent={Move::Recover,0,0};e.pattern={e.intent};s.enemies.push_back(e);}return s;}
std::string eventBytes(const Result& r){std::string result;for(const auto& e:r.events)result+=eventJson(e)+"\n";return result;}
Result act(State& s,const Action& a){const auto before=serialize(s);const auto p=rules.preview(s,a);need(serialize(s)==before,"Preview changed authoritative state");const auto r=rules.apply(s,a);need(r.ok,r.reason);need(p.result.ok&&serialize(p.state)==serialize(s)&&eventBytes(p.result)==eventBytes(r),"Preview/action bytes or events differ");return r;}
void reject(State& s,const Action& a){const auto before=serialize(s);const auto r=rules.apply(s,a);need(!r.ok&&r.events.empty()&&serialize(s)==before,"Rejected action changed state or emitted effects");}
void reload(State& s){const auto bytes=serialize(s);State loaded;std::string error;need(deserialize(bytes,loaded,error),error);need(serialize(loaded)==bytes,"Snapshot mismatch");s=std::move(loaded);}
Id store(State& s,const std::string& recipe){RecipeCopy copy;copy.id=s.nextId++;copy.recipe=recipe;s.memory.push_back(copy);return copy.id;}
Id make(State& s,const std::string& recipe,Action a={}){a.type=ActionType::Craft;a.subject=store(s,recipe);const auto before=s.nextId;act(s,a);for(const auto& p:s.parts)if(p.id>=before&&p.creator==recipe)return p.id;return 0;}
Part& part(State& s,Id id){for(auto& p:s.parts)if(p.id==id)return p;throw std::runtime_error("Missing physical source");}
Amount cooldown(const State& s,Id id){for(const auto& c:s.memory)if(c.id==id)return c.cooldown;throw std::runtime_error("Missing recipe copy");}
std::size_t scheduled(const State& s,const std::string& source){return std::count_if(s.deliveries.begin(),s.deliveries.end(),[&](const Delivery& d){return d.source==source;});}
void next(State& s){act(s,Action::endTurn());need(s.phase==Phase::Collection,"Expected continuing fight");act(s,Action::collect(0));}
Id startCold(State& s){const auto slug=make(s,"SH001");const auto copy=store(s,"SH118");auto a=Action::craft(copy);a.parts={slug};act(s,a);need(cooldown(s,copy)==3,"Actual source cooldown3 required");return copy;}
void attack(State& s,std::size_t index,Amount amount){s.enemies[index].intent={Move::Attack,amount,1};s.enemies[index].pattern={s.enemies[index].intent};}
Result fire(State& s,const std::vector<Id>& parts){act(s,Action::load(parts));return act(s,Action::fire(s.enemies[0].id));}
Amount fireAmount(const Result& r){for(const auto& e:r.events)if(e.type=="fire")return e.amount;throw std::runtime_error("No main Fire amount");}
void activate(State& s,Id part){Action a;a.type=ActionType::Activate;a.subject=part;act(s,a);}
}

int main(){
test("S01 aged reserve first installation schedules once; detached and later reinstalled sources cannot rearm",[]{
    for(const auto* id:{"SH092","SH113"}){auto s=scene();const auto source=make(s,id);next(s);next(s);need(s.round==3&&scheduled(s,id)==0&&Rules::shield(s)==0,"Reserve age must not start delivery");const auto cold=startCold(s);act(s,Action::install(source));need(part(s,source).firstInstallRound==3&&scheduled(s,id)==1,"First installation owns exactly one schedule");act(s,Action::remove(source));act(s,Action::install(source));act(s,Action::remove(source));reload(s);need(scheduled(s,id)==1,"Same-round movement duplicated delivery");next(s);const Amount delivered=std::string(id)=="SH092"?15:20;need(Rules::shield(s)==delivered&&cooldown(s,cold)==3,"Detached source cancelled fixed delivery or granted conditional cooling");act(s,Action::install(source));reload(s);next(s);need(Rules::shield(s)==0&&scheduled(s,id)==0&&cooldown(s,cold)==2,"Old source/generated delivery repeated original-round hook");}
});
test("S02 committed Shield delivery remains separate from the installed first-attacker Mark hook",[]{
    auto s=scene();const auto source=make(s,"SH092");act(s,Action::install(source));act(s,Action::remove(source));reload(s);act(s,Action::install(source));attack(s,0,0);attack(s,1,2);next(s);need(s.enemies[0].mark==0&&s.enemies[1].mark==4&&Rules::shield(s)==15,"First positive Shield-removing attack must receive Mark4, then independent15 arrives");s.enemies[1].mark=0;next(s);need(s.enemies[1].mark==0&&Rules::shield(s)==0,"Fresh generic delivery inherited source reactive hook");
});
test("S03 SH113 pre-reset reader keeps first-binding order through removal and reinstallation",[]{
    for(bool discardFirst:{false,true}){auto s=scene();const auto cold=startCold(s);Id wall=0,discard=0;if(discardFirst){discard=make(s,"SH060");act(s,Action::install(discard));}wall=make(s,"SH113");act(s,Action::install(wall));if(!discardFirst){discard=make(s,"SH060");act(s,Action::install(discard));}const auto order=part(s,wall).bindingOrder;act(s,Action::remove(wall));act(s,Action::install(wall));need(part(s,wall).bindingOrder==order&&part(s,wall).installOrder>part(s,discard).installOrder,"Movement must change protection order only");reload(s);next(s);need(Rules::shield(s)==20,"Committed20 must survive both reset orders");need(cooldown(s,cold)==(discardFirst?3:2),"Reader must see actual ordered pre-reset protection, not later delivery or current installation order");}
});
test("S04 scheduled damage and recoil respect delivery order and final death",[]{
    for(const auto* id:{"SH092","SH113"})for(bool shieldFirst:{false,true}){auto s=scene();Id wall=0;if(shieldFirst){wall=make(s,id);act(s,Action::install(wall));}s.enemies[0].mesh=6;fire(s,{make(s,"SH056")});if(!shieldFirst){wall=make(s,id);act(s,Action::install(wall));}act(s,Action::remove(wall));s.hp=1;reload(s);act(s,Action::endTurn());if(shieldFirst){need(s.phase==Phase::Collection&&s.hp==1&&Rules::shield(s)==(std::string(id)=="SH092"?9:14),"Earlier fresh Shield must absorb delayed hit recoil6");}else{need(s.phase==Phase::Defeat&&s.hp==0&&Rules::shield(s)==0,"Earlier lethal delayed recoil must interrupt later ordinary Shield grant");}}
});
test("S05 delivery cooling uses actual per-copy clocks and cannot refresh a used None recipe",[]{
    auto s=scene();const auto earlier=store(s,"SH051");act(s,Action::craft(earlier));next(s);need(cooldown(s,earlier)==2,"Use round must skip natural tick");const auto later=startCold(s);const auto noCooldown=store(s,"SH002");act(s,Action::craft(noCooldown));act(s,Action::install(make(s,"SH113")));reload(s);next(s);need(cooldown(s,earlier)==0&&cooldown(s,later)==2,"Old copy ticks then cools; new copy skips tick then cools once");act(s,Action::craft(noCooldown));const auto newer=store(s,"SH113");act(s,Action::craft(newer));Id wall=0;for(const auto& p:s.parts)if(p.sourceRecipeCopy==newer)wall=p.id;act(s,Action::install(wall));make(s,"SH067");reject(s,Action::craft(noCooldown));
});
test("S06 different physical delayed Shields retain both fixed schedules after partial depletion",[]{
    auto s=scene();const auto first=make(s,"SH092"),second=make(s,"SH113");act(s,Action::install(first));act(s,Action::install(second));s.enemies[0].mesh=7;fire(s,{make(s,"SH001")});need(part(s,first).shield==8&&part(s,second).shield==38,"Actual recoil drains oldest current installed part");act(s,Action::remove(first));act(s,Action::remove(second));reload(s);next(s);need(Rules::shield(s)==35,"Two fixed future values remain15+20 despite depleted or detached originals");need(part(s,first).shield==8&&part(s,second).shield==38,"Promised grants must not refill stored sources");next(s);need(Rules::shield(s)==0,"Fresh ordinary grants do not retain or recursively schedule");
});
test("S07 same-name Magnet reapplication refreshes choices while finite ordinary stock is conserved",[]{
    auto s=scene();Action first;first.choices={0,3};activate(s,make(s,"SH119",first));Action last;last.choices={1,4};activate(s,make(s,"SH119",last));act(s,Action::endTurn());s.finitePile=true;s.pile={5,3,1,1,2};const auto before=s.materials;reload(s);act(s,Action::collect(0));const Materials expected={5,3,1,1,2};for(std::size_t i=0;i<5;++i)need(s.materials[i]-before[i]==expected[i],"Refresh must use final two choices, once, without creating missing stock");need(std::accumulate(s.pile.begin(),s.pile.end(),Amount{0})==0,"Finite pile conservation");
});

// Observation only: the selected row does not settle printed versus actually
// paid material types after discounts. Do not call either amount accepted.
try{bool found=false;for(std::uint64_t seed=1;seed<=64&&!found;++seed){auto s=scene(seed);need(rules.acquireUpgrade(s,"UGS-116").ok,"Discount acquisition");need(rules.startUpgrades(s,EncounterClass::Regular).ok,"Discount start");if(ownedUpgrade(s,"UGS-116")->values.front()!=3)continue;found=true;act(s,Action::collect(0));const auto copy=store(s,"SH084"),first=s.nextId;const auto used=act(s,Action::craft(copy));Materials paid{};for(const auto& e:used.events)if(e.type=="recipe_used")paid=e.paid;const auto types=std::count_if(paid.begin(),paid.end(),[](Amount n){return n>0;});need(paid[3]==0&&types==2,"Glass discount must remove one actual material type");Id output=0;for(const auto& p:s.parts)if(p.id>=first&&p.creator=="SH084")output=p.id;const auto damage=fireAmount(fire(s,{output}));std::cout<<"OBS O01 SH084 actual-paid-types="<<types<<" actual-Glass="<<paid[3]<<" main-damage="<<damage<<"; actual-paid interpretation=18, printed-types interpretation=21; unresolved\n";}need(found,"Deterministic Glass-discount witness");}catch(const std::exception& e){++failed;std::cout<<"FAIL O01 observation setup: "<<e.what()<<'\n';}
std::cout<<"SUMMARY "<<passed<<" passed, "<<failed<<" failed, "<<checks<<" assertions; O01 is an unresolved source observation, not semantic acceptance\n";
return failed?1:0;
}

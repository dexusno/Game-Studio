#include "overkill/core.hpp"
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace overkill;
namespace {
const Rules rules;
int checks=0,passed=0,failed=0;
std::string selected;
void need(bool value,const std::string& why){++checks;if(!value)throw std::runtime_error(why);}
void test(const char* id,const char* name,const std::function<void()>& body){if(!selected.empty()&&selected!=id)return;try{body();++passed;std::cout<<"PASS "<<id<<" "<<name<<'\n';}catch(const std::exception& e){++failed;std::cout<<"FAIL "<<id<<" "<<name<<": "<<e.what()<<'\n';}}
State scene(){State s;s.hp=s.maxHp=80;s.phase=Phase::Preparation;s.hotBarrel=false;s.materials={200,200,200,200,200};s.encounter="independent-mara-contract";s.rng=Rng::seeded(37,s.encounter);for(int i=0;i<2;++i){Enemy e;e.id=s.nextId++;e.name=e.definition="Controlled quiet target";e.hp=e.maxHp=1000;e.intent={Move::Recover,0,0};e.pattern={e.intent};s.enemies.push_back(e);}return s;}
std::string events(const Result& r){std::string text;for(const auto& e:r.events)text+=eventJson(e)+"\n";return text;}
Result act(State& s,const Action& a){need(s.heat<=upgradeHeatCap(s),"Heat fixture exceeds actual acquired capacity");const auto before=serialize(s);const auto preview=rules.preview(s,a);need(serialize(s)==before,"Preview mutated authoritative state");const auto r=rules.apply(s,a);need(r.ok,r.reason);need(preview.result.ok&&serialize(preview.state)==serialize(s)&&events(preview.result)==events(r),"Preview/action state or ordered events differ");need(s.heat<=upgradeHeatCap(s),"Committed Heat exceeds actual capacity");return r;}
void reload(State& s){const auto bytes=serialize(s);State restored;std::string error;need(deserialize(bytes,restored,error),error);need(serialize(restored)==bytes,"Canonical snapshot differs");s=std::move(restored);}
void reject(State& s,const Action& a){const auto before=serialize(s);const auto r=rules.apply(s,a);need(!r.ok&&r.events.empty()&&serialize(s)==before,"Rejected action committed state or partial events");}
Id store(State& s,const std::string& recipe){RecipeCopy copy;copy.id=s.nextId++;copy.recipe=recipe;s.memory.push_back(copy);return copy.id;}
Result use(State& s,const std::string& recipe,Action a={}){a.type=ActionType::Craft;a.subject=store(s,recipe);return act(s,a);}
Part& part(State& s,Id id){for(auto& p:s.parts)if(p.id==id)return p;throw std::runtime_error("Missing physical source");}
Id make(State& s,const std::string& recipe){const auto first=s.nextId;use(s,recipe);std::vector<Id> ids;for(const auto& p:s.parts)if(p.id>=first&&p.creator==recipe)ids.push_back(p.id);need(ids.size()==1,"One actual paid physical output required");return ids[0];}
Id plate(State& s,const std::string& recipe="SH002"){const auto id=make(s,recipe);act(s,Action::install(id));return id;}
void activate(State& s,Id id,Action a={}){a.type=ActionType::Activate;a.subject=id;act(s,a);}
void next(State& s){act(s,Action::endTurn());need(s.phase==Phase::Collection,"Quiet fixture unexpectedly ended");act(s,Action::collect(0));}
Result fire(State& s,const std::vector<Id>& ids,Action a={}){act(s,Action::load(ids));a.type=ActionType::Fire;a.target=s.enemies[0].id;return act(s,a);}
Amount amount(const Result& r,const std::string& type){Amount total=0;for(const auto& e:r.events)if(e.type==type)total+=e.amount;return total;}
Amount shot(State& s){return amount(fire(s,{make(s,"SH001")}),"fire");}
void own(State& s,const std::string& id){const auto r=rules.acquireUpgrade(s,id);need(r.ok,r.reason);}
void attack(State& s,Amount value){s.enemies[0].intent={Move::Attack,value,1};s.enemies[0].pattern={s.enemies[0].intent};}
}

int main(int argc,char** argv){
if(argc==2)selected=argv[1];else if(argc!=1)return 2;
test("M01","MA082 caps each added Burn payload using legal pre-payment Fire Heat, not remaining Heat or total stacks",[]{
    for(Amount heat:{9,10,11,14})for(int copies:{1,2}){auto s=scene();own(s,"MAU-02");need(s.heat==0&&upgradeHeatCap(s)==14,"Capacity upgrade must not fill Heat");for(int i=0;i<5;++i)use(s,"MA001");need(s.heat==14,"Actual Fuel Brick payment fills legal14");
        Action cool;cool.target=s.enemies[1].id;if(heat==9){use(s,"MA004");use(s,"MA026",cool);}else if(heat==10)use(s,"MA074",cool);else if(heat==11)use(s,"MA026",cool);need(s.heat==heat,"Exact paid/loss-derived Heat boundary");s.enemies[0].burn=3;s.enemies[1].burn=0;
        std::vector<Id> bullet;for(int i=0;i<copies;++i)bullet.push_back(make(s,"MA082"));const auto spread=make(s,"MA041");bullet.push_back(spread);Action a;a.spreadTargets={{spread,s.enemies[1].id}};reload(s);const auto r=fire(s,bullet,a);
        const Amount extra=copies*std::min(10,heat);need(s.enemies[0].burn==3+extra,"Burn per-output cap10 or additive stack contract failed: actual="+std::to_string(s.enemies[0].burn)+" expected="+std::to_string(3+extra));need(s.heat==heat-4&&amount(r,"fire")==12*copies,"Separate Fire payment rewrote snapshot or printed damage");need(s.enemies[1].burn==2&&amount(r,"burn_applied")==extra+2,"Main Burn payload incorrectly copied or receipt differs");
    }
});
test("M02","MA098 excludes paid or recoil-depleted saved Shield and preserves untouched remove/reinstall eligibility",[]{
    for(int mode:{0,1,2,3}){auto s=scene();const auto shield=plate(s);if(mode==1||mode==2)use(s,"MA037");if(mode==2){plate(s);use(s,"MA037");}if(mode==3){s.enemies[0].mesh=1;shot(s);s.enemies[0].mesh=0;}const Amount expectedRemaining=mode==0?6:(mode==1?1:(mode==2?0:5));need(part(s,shield).shield==expectedRemaining,"Actual payment/recoil setup did not consume expected protection");
        act(s,Action::remove(shield));reload(s);act(s,Action::install(shield));act(s,Action::remove(shield));need(part(s,shield).shield==expectedRemaining,"Reinstallation changed saved protection");activate(s,make(s,"MA098"));const auto actual=shot(s);need(actual==(mode==0?9:6),"Unused reserve distinction failed: mode="+std::to_string(mode)+" actual="+std::to_string(actual));need(part(s,shield).place==Place::Reserve&&part(s,shield).shield==expectedRemaining,"Reserve count consumed or changed the physical source");
    }
});
test("M03","MA055 records post-attack remaining protection in original pre-reset binding order",[]{
    for(int order:{0,1,2}){auto s=scene();if(order==1)plate(s,"SH060");const auto reader=plate(s,"MA055");if(order==2)plate(s,"SH060");plate(s);act(s,Action::remove(reader));act(s,Action::install(reader));attack(s,Rules::shield(s)-9);const auto iron=s.materials[0];reload(s);act(s,Action::endTurn());need(s.hp==80&&Rules::shield(s)==0,"Attack should leave9 before normal reset");need(s.materials[0]==iron+(order==1?0:1),"Iron reader used pre-attack, later discard, or current installation ordering");
    }
});
test("M04","removed and previous-round MA015/MA102 hooks do not retain Heat under real reduced-decay upgrade",[]{
    auto s=scene();own(s,"MAU-04");for(int i=0;i<3;++i)use(s,"MA001");need(s.heat==9&&upgradeHeatDecay(s)==1,"Actual9 Heat and reduced normal decay");const auto skip=plate(s,"MA015");act(s,Action::remove(skip));reload(s);next(s);need(s.heat==8,"Removed insulation skipped normal decay");act(s,Action::install(skip));next(s);need(s.heat==7,"Old reinstall rearmed original-round skip");
    s=scene();own(s,"MAU-04");for(int i=0;i<3;++i)use(s,"MA001");const auto high=plate(s,"MA102");Action cool;cool.target=s.enemies[1].id;use(s,"MA026",cool);use(s,"MA026",cool);const auto low=plate(s,"MA102");act(s,Action::remove(high));reload(s);next(s);need(s.heat==3,"Detached remembered8 overrode installed remembered3");act(s,Action::install(high));next(s);need(s.heat==2,"Saved original seal rearmed its remembered value");need(std::none_of(s.parts.begin(),s.parts.end(),[&](const Part& p){return p.id==low;}),"Installed low seal must clear normally");
});
test("M05","MA091 records Burn after all pre-hit effects in either Ammo order and stops transfer after lethal recoil",[]{
    for(bool reverse:{false,true})for(bool lethal:{false,true}){auto s=scene();s.enemies[0].hp=30;s.enemies[0].burn=10;s.enemies[0].mesh=lethal?2:0;if(lethal)s.hp=1;const auto remover=make(s,"MA042"),transfer=make(s,"MA091");Action a;a.partTargets={{transfer,s.enemies[1].id}};reload(s);fire(s,reverse?std::vector<Id>{transfer,remover}:std::vector<Id>{remover,transfer},a);need(s.enemies[0].dead,"Actual main hit must kill its target");need(s.enemies[1].burn==(lethal?0:4),"Transfer used old Burn or ran after final player death");need((s.phase==Phase::Defeat)==lethal,"Controlled recoil death boundary");}
});
test("M06","MA076 exact5 paid part Heat qualifies while4 does not; rejected cost leaves all state unchanged",[]{
    for(Amount payment:{4,5}){auto s=scene();for(int i=0;i<2;++i)use(s,"MA001");const auto modifier=make(s,"MA045");Action a;a.type=ActionType::Activate;a.subject=modifier;a.amount=7;reject(s,a);a.amount=payment;activate(s,modifier,a);s.enemies[0].shield=100;reload(s);const auto r=fire(s,{make(s,"MA076")});const Amount main=10+3*payment;need(amount(r,"fire")==main&&s.enemies[0].shield==100-main-(payment==5?10:0),"Inclusive5 threshold differs from actual paid physical cost");}
});
test("M07","MA066 timed Heat occurs after the exact part effect and is interrupted by fatal main-hit recoil",[]{
    for(bool lethal:{false,true}){auto s=scene();const auto ammo=make(s,"MA023");Action a;a.parts={ammo};use(s,"MA066",a);next(s);reload(s);s.hp=lethal?1:2;s.enemies[0].mesh=1;const auto r=fire(s,{ammo});need(s.heat==(lethal?0:6)&&amount(r,"heat")== (lethal?0:6),"Heat own2/attached4 resolved before lethal reaction or was lost on survival");need(s.enemies[0].hp==993&&(s.phase==Phase::Defeat)==lethal,"Actual main hit and recoil control");}
});
if(!selected.empty()&&passed+failed==0){std::cout<<"Unknown case\n";return 2;}
std::cout<<"SUMMARY "<<passed<<" passed, "<<failed<<" failed, "<<checks<<" assertions\n";return failed?1:0;
}

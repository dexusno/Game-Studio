#include "overkill/core.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>
using namespace overkill;
namespace {
int checks=0;
void check(bool ok,const std::string& reason){++checks;if(!ok)throw std::runtime_error(reason);}
void act(const Rules& rules,State& s,const Action& a){auto r=rules.apply(s,a);check(r.ok,r.reason);}
Id copy(State& s,const std::string& recipe){const auto id=s.nextId++;s.memory.push_back({id,recipe,0,0,0});return id;}
Id recipeCopy(const State& s,const std::string& recipe){for(const auto& c:s.memory)if(c.recipe==recipe)return c.id;throw std::runtime_error("missing copy "+recipe);}
Id part(const State& s,const std::string& recipe){for(auto it=s.parts.rbegin();it!=s.parts.rend();++it)if(it->recipe==recipe && it->place==Place::Reserve)return it->id;throw std::runtime_error("missing part "+recipe);}
State prepared(const Rules& rules){auto s=Rules::teachingEncounter(77,true);act(rules,s,Action::collect(0));s.materials={200,200,200,200,200};s.enemies.resize(1);auto& e=s.enemies[0];e.hp=e.maxHp=1000;e.pattern={{Move::Charge,0,0}};e.intent=e.pattern[0];return s;}
void shot(const Rules& rules,State& s,const std::string& recipe="SH001") {const auto id=copy(s,recipe);act(rules,s,Action::craft(id));act(rules,s,Action::load({part(s,recipe)}));act(rules,s,Action::fire(s.enemies[0].id));}
void next(const Rules& rules,State& s){act(rules,s,Action::endTurn());act(rules,s,Action::collect(0));}
Id install(const Rules& rules,State& s,const std::string& recipe){act(rules,s,Action::craft(copy(s,recipe)));const auto id=part(s,recipe);act(rules,s,Action::install(id));return id;}
const Part& getPart(const State& s,Id id){for(const auto& p:s.parts)if(p.id==id)return p;throw std::runtime_error("part not present");}
void unchanged(const Rules& rules,State& s,const Action& a){const auto before=serialize(s);auto r=rules.apply(s,a);check(!r.ok,"expected rejected action");check(serialize(s)==before,"rejected action changed state/RNG/IDs");}
void run(const char* name,const std::function<void()>& fn){fn();std::cout<<"PASS "<<name<<'\n';}
}
int main(){try{
    Rules rules;
    run("authored Mite/Ram complete fight, multiple shots, saved Shield and integer Armor",[&]{
        auto s=Rules::teachingEncounter();auto result=playTeachingFixture(s,rules);check(result.ok,result.reason);
        check(s.hp==80 && s.round==3 && s.shots==4 && s.kills==2,"teaching outcome");
        std::vector<Amount> ramDamage;bool blocked=false;
        for(const auto& e:result.events){if(e.type=="hit" && e.target==s.enemies[1].id)ramDamage.push_back(e.amount);if(e.type=="player_damage" && e.secondary==18 && e.amount==0)blocked=true;}
        check(ramDamage==std::vector<Amount>({3,17,4}),"Ram 24 -> 21 -> 4 -> 0");check(blocked,"20 Shield must absorb Blast 18");
    });
    run("T01 installed Shield absorbs recoil across repeated shots without refill",[&]{
        auto s=prepared(rules);s.hp=40;install(rules,s,"SH002");install(rules,s,"SH006");Rules::spendShield(s,2);
        s.enemies[0].mesh=3;shot(rules,s);check(s.hp==40 && Rules::shield(s)==5,"first recoil");
        s.enemies[0].mesh=7;shot(rules,s);check(s.hp==38 && Rules::shield(s)==0,"second recoil");check(s.round==1,"Fire advanced round");
    });
    run("T02 lethal killing-hit recoil takes precedence over victory",[&]{
        auto s=prepared(rules);s.hp=2;s.enemies[0].hp=1;s.enemies[0].mesh=4;install(rules,s,"SH002");Rules::spendShield(s,5);
        shot(rules,s);check(s.phase==Phase::Defeat && s.hp==0 && s.kills==1,"simultaneous defeat precedence");
        unchanged(rules,s,Action::endTurn());
    });
    run("T03 Reserve Heart Pump intercepts lethal recoil before final death",[&]{
        auto s=prepared(rules);s.hp=2;s.enemies[0].hp=1;s.enemies[0].mesh=4;s.reserveHeartPump=true;
        install(rules,s,"SH002");Rules::spendShield(s,5);shot(rules,s);
        check(s.hp==20 && !s.reserveHeartPump && s.phase==Phase::Victory,"rescue and surviving victory");
    });
    run("T04 first-install Heat cost and depletion survive remove/reinstall",[&]{
        auto s=prepared(rules);s.heat=3;const auto id=install(rules,s,"MA017");check(s.heat==0,"first Heat payment");
        Rules::spendShield(s,4);act(rules,s,Action::remove(id));check(Rules::shield(s)==0,"reserve must not protect");
        act(rules,s,Action::install(id));check(s.heat==0 && Rules::shield(s)==6 && getPart(s,id).paidHeat==3,"reinstall refilled or repaid");
        const auto first=getPart(s,id).bindingOrder;act(rules,s,Action::remove(id));next(rules,s);act(rules,s,Action::install(id));
        check(Rules::shield(s)==6 && getPart(s,id).bindingOrder==first,"saved installation history");
    });
    run("installation payment atomicity and one-time Boiler Jacket Heat",[&]{
        auto s=prepared(rules);act(rules,s,Action::craft(copy(s,"MA017")));unchanged(rules,s,Action::install(part(s,"MA017")));
        const auto id=install(rules,s,"MA003");check(s.heat==1,"Boiler Jacket installation Heat");
        act(rules,s,Action::remove(id));act(rules,s,Action::install(id));check(s.heat==1,"Boiler Jacket repeated gain");
    });
    run("installed Shield payment order and reinstall priority",[&]{
        auto s=prepared(rules);const auto a=install(rules,s,"SH002"),b=install(rules,s,"SH006");
        check(Rules::spendShield(s,5,true)==5 && getPart(s,a).shield==1 && getPart(s,b).shield==4,"6 then 4 drained by 5");
        act(rules,s,Action::remove(a));act(rules,s,Action::install(a));Rules::spendShield(s,3,true);
        check(getPart(s,a).shield==1 && getPart(s,b).shield==1,"reinstallation must append protection order");
    });
    run("T07 retention allowances allocate one shared remaining balance",[&]{
        auto s=prepared(rules);install(rules,s,"SH002");install(rules,s,"SH002");install(rules,s,"SH006");install(rules,s,"SH006");
        s.retentionAllowances={3,6};next(rules,s);check(Rules::shield(s)==9 && s.parts.empty(),"retention of 20 must keep 9");
        s.protection[0].amount=5;next(rules,s);check(Rules::shield(s)==5,"retention cannot duplicate remaining 5");
    });
    run("T08 next-round fresh Shield survives old reset and source removal",[&]{
        auto s=prepared(rules);act(rules,s,Action::craft(copy(s,"SH026")));check(Rules::shield(s)==4 && s.deliveries.size()==1,"immediate scheduled Shield");
        const Id source=s.parts[0].id;act(rules,s,Action::remove(source));next(rules,s);
        check(Rules::shield(s)==6 && s.deliveries.empty(),"fresh delivery reset or cancelled by removal");
        check(getPart(s,source).shield==4,"saved source part changed");
    });
    run("T09 cooldown 1 blocks following full round; cooling permits reuse",[&]{
        auto s=prepared(rules);const Id id=recipeCopy(s,"SH005");act(rules,s,Action::craft(id));
        next(rules,s);unchanged(rules,s,Action::craft(id));next(rules,s);act(rules,s,Action::craft(id));
        act(rules,s,Action::craft(recipeCopy(s,"SH007")));act(rules,s,Action::craft(id));
        unchanged(rules,s,Action::craft(recipeCopy(s,"SH007")));
    });
    run("independent duplicate recipe copies and atomic material/HP costs",[&]{
        auto s=prepared(rules);act(rules,s,Action::craft(recipeCopy(s,"SH001")));unchanged(rules,s,Action::craft(recipeCopy(s,"SH001")));
        act(rules,s,Action::craft(copy(s,"SH001")));s.hp=3;const Id blood=copy(s,"MA019");unchanged(rules,s,Action::craft(blood));
        s.hp=4;act(rules,s,Action::craft(blood));check(s.hp==1 && s.heat==5,"cost-plus-1");
        s.materials={0,0,0,0,0};unchanged(rules,s,Action::craft(copy(s,"SH002")));
    });
    run("zero base gun damage and retained Mara Hot Barrel per shot",[&]{
        auto s=prepared(rules);s.heat=6;const auto before=s.enemies[0].hp;shot(rules,s);shot(rules,s);
        check(before-s.enemies[0].hp==18 && s.heat==6 && s.round==1,"Hot Barrel should add 3 twice without consuming Heat");
        s.hotBarrel=false;const auto hp=s.enemies[0].hp;shot(rules,s);check(hp-s.enemies[0].hp==6,"ordinary gun base must be zero");
        unchanged(rules,s,Action::load({}));
    });
    run("combined integer percentages, separate spread hits and lost dead-target hits",[&]{
        auto s=prepared(rules);s.hotBarrel=false;Enemy second=s.enemies[0];second.id=s.nextId++;second.hp=second.maxHp=5;second.mesh=2;s.enemies.push_back(second);
        const auto slug=copy(s,"SH001"),powder=copy(s,"SH003");act(rules,s,Action::craft(slug));act(rules,s,Action::craft(powder));
        act(rules,s,Action::craft(copy(s,"SH005")));const auto spread1=part(s,"SH005");act(rules,s,Action::craft(copy(s,"SH005")));const auto spread2=part(s,"SH005");
        s.nextPercent=60;act(rules,s,Action::load({part(s,"SH001"),part(s,"SH003"),spread1,spread2}));
        auto result=rules.apply(s,Action::fire(s.enemies[0].id,{{spread1,second.id},{spread2,second.id}}));check(result.ok,result.reason);
        check(s.enemies[0].hp==986 && s.hp==78,"9 with +60% is 14; one killing spread reaction");
        check(std::count_if(result.events.begin(),result.events.end(),[](const Event& e){return e.type=="hit_lost";})==1,"later dead-target hit should be lost");
    });
    run("main-target Burn never copied by Split Outlet",[&]{
        auto s=prepared(rules);Enemy second=s.enemies[0];second.id=s.nextId++;s.enemies.push_back(second);
        act(rules,s,Action::craft(copy(s,"SH013")));act(rules,s,Action::craft(recipeCopy(s,"SH005")));const auto nozzle=part(s,"SH005");
        act(rules,s,Action::load({part(s,"SH013"),nozzle}));act(rules,s,Action::fire(s.enemies[0].id,{{nozzle,second.id}}));
        check(s.enemies[0].burn==2 && s.enemies[1].burn==0,"status copied to spread");
    });
    run("T10 four full escape warning rounds and no departure attack",[&]{
        auto s=prepared(rules);s.enemies[0].departureRound=6;s.enemies[0].pattern={{Move::Attack,1,1}};s.enemies[0].intent=s.enemies[0].pattern[0];
        for(int r=1;r<=5;++r){check(s.round==r,"round sequence");if(r>=2)check(Rules::intentText(s.enemies[0],s.round).find("Escape in "+std::to_string(6-r))!=std::string::npos,"escape countdown");next(rules,s);}
        check(s.hp==75 && s.enemies[0].intent.move==Move::Escape,"four full warning turns");act(rules,s,Action::endTurn());
        check(s.phase==Phase::Escaped && s.hp==75 && s.kills==0,"departure damage or false kill");
    });
    run("Burn ignores Armor and Tiles; Corrosion kills before a committed attack",[&]{
        auto s=prepared(rules);s.enemies[0].armor=9;s.enemies[0].tiles=3;s.enemies[0].burn=4;
        next(rules,s);check(s.enemies[0].hp==996 && s.enemies[0].tiles==3 && s.enemies[0].burn==3,"Burn was reduced by direct-hit defence");
        s.enemies[0].hp=2;s.enemies[0].corrosion=2;s.enemies[0].intent={Move::Attack,100,1};
        act(rules,s,Action::endTurn());check(s.phase==Phase::Victory && s.hp==80,"Corrosion death failed to cancel attack");
    });
    run("one collection per round, Precision once per fight, Magnet next-haul scope",[&]{
        auto s=Rules::teachingEncounter();act(rules,s,Action::collect(4,2));check(s.materials==Materials({3,2,1,1,5}),"Perfect haul");
        unchanged(rules,s,Action::collect(0));s.materials={30,30,30,30,30};s.enemies.resize(1);s.enemies[0].pattern={{Move::Charge,0,0}};s.enemies[0].intent=s.enemies[0].pattern[0];
        act(rules,s,Action::craft(recipeCopy(s,"SH008")));Action fit;fit.type=ActionType::Activate;fit.subject=part(s,"SH008");act(rules,s,fit);
        act(rules,s,Action::endTurn());const auto before=std::accumulate(s.materials.begin(),s.materials.end(),0);
        unchanged(rules,s,Action::collect(0,0));act(rules,s,Action::collect(0));check(std::accumulate(s.materials.begin(),s.materials.end(),0)-before==12,"Magnet haul quantity");
    });
    run("Quick Patch's fight healing cap is shared by duplicates",[&]{
        auto s=prepared(rules);s.hp=60;for(int i=0;i<3;++i)act(rules,s,Action::craft(copy(s,"SH074")));
        check(s.hp==68 && s.quickPatchHealing==8,"Quick Patch duplicate cap");
    });
    run("removed Insulating Pad cannot prevent Burn or rearm in another round",[&]{
        auto s=prepared(rules);const auto pad=install(rules,s,"SH006");act(rules,s,Action::remove(pad));s.burn=3;
        next(rules,s);check(s.hp==77,"removed source still prevented Burn");
        act(rules,s,Action::install(pad));Rules::spendShield(s,4);s.burn=3;next(rules,s);check(s.hp==74,"old first-install ward was rearmed");
        install(rules,s,"SH006");Rules::spendShield(s,4);s.burn=3;next(rules,s);check(s.hp==74,"fresh installed ward failed");
    });
    run("same named Modifiers refresh; different names combine",[&]{
        auto definitions=Rules::starterContent();definitions.push_back({"FIXTURE20","Distinct fixture bonus","Fixture part",Kind::Modifier,{0,0,0,0,0},0,{{Op::PercentDamage,Timing::Activate,20,0}}});Rules fixtureRules(definitions);
        auto s=prepared(fixtureRules);s.hotBarrel=false;
        const auto activate=[&](const std::string& id){act(fixtureRules,s,Action::craft(copy(s,id)));Action a;a.type=ActionType::Activate;a.subject=part(s,id);act(fixtureRules,s,a);};
        activate("SH050");activate("SH050");activate("FIXTURE20");
        act(fixtureRules,s,Action::craft(recipeCopy(s,"SH001")));act(fixtureRules,s,Action::craft(recipeCopy(s,"SH003")));
        act(fixtureRules,s,Action::load({part(s,"SH001"),part(s,"SH003")}));act(fixtureRules,s,Action::fire(s.enemies[0].id));
        check(s.enemies[0].hp==984 && s.hp==72,"9 with refreshed60 + different20 must be16; both legal costs paid");check(s.shotBonuses.empty(),"next-shot bonuses persisted after Fire");
    });
    run("identical Magnet copies refresh the next haul",[&]{
        auto s=prepared(rules);
        for(int i=0;i<2;++i){act(rules,s,Action::craft(copy(s,"SH008")));Action a;a.type=ActionType::Activate;a.subject=part(s,"SH008");act(rules,s,a);}
        check(s.deliveries.size()==1,"duplicate scheduled Magnet");act(rules,s,Action::endTurn());const auto before=std::accumulate(s.materials.begin(),s.materials.end(),0);
        act(rules,s,Action::collect(0));check(std::accumulate(s.materials.begin(),s.materials.end(),0)-before==12,"duplicate Magnet stacked");
    });
    run("content rejects unknown opcode and unsupported timing before play",[&]{
        for(const auto effect:std::vector<Effect>{{static_cast<Op>(255),Timing::Use,1,0},{Op::FlatDamage,Timing::Use,3,0},{Op::Burn,Timing::Use,3,0},{Op::FlatDamage,Timing::Assembly,3,0}}){
            Recipe broken{"BROKEN","Broken","",Kind::Utility,{1,0,0,0,0},0,{effect}};
            bool rejected=false;try{Rules bad(std::vector<Recipe>{broken});}catch(const std::exception&){rejected=true;}
            check(rejected,"unsupported content accepted");
        }
        auto invalidOutput=Rules::starterContent()[0];invalidOutput.automaticOutput=true;
        bool rejected=false;try{Rules bad(std::vector<Recipe>{invalidOutput});}catch(const std::exception&){rejected=true;}
        check(rejected,"absent automatic output still accepted assembly effect");
    });
    run("preview and rejected loaded Fire preserve all authoritative state",[&]{
        auto s=prepared(rules);act(rules,s,Action::craft(recipeCopy(s,"SH001")));act(rules,s,Action::load({part(s,"SH001")}));
        const auto before=serialize(s);const auto preview=rules.preview(s,Action::fire(s.enemies[0].id));check(preview.result.ok && preview.state.shots==1,"preview result");
        check(serialize(s)==before,"preview changed live state");unchanged(rules,s,Action::fire(99999));
        act(rules,s,Action::craft(recipeCopy(s,"MA001")));check(s.bullet.size()==1,"crafting while loaded changed reservation");
        act(rules,s,Action::endTurn());check(s.bullet.empty() && !s.parts.empty() && s.parts[0].place==Place::Reserve,"unfired parts were consumed");
    });
    run("no fixed bullet or Shield part cap",[&]{
        auto s=prepared(rules);std::vector<Id> bullet;
        for(int i=0;i<32;++i){act(rules,s,Action::craft(copy(s,"SH001")));bullet.push_back(part(s,"SH001"));install(rules,s,"SH002");}
        check(Rules::shield(s)==192,"32 Shield parts");act(rules,s,Action::load(bullet));act(rules,s,Action::fire(s.enemies[0].id));check(s.enemies[0].hp==808,"32-part shot");
    });
    run("versioned state roundtrip includes loaded parts, schedule, RNG and installation history",[&]{
        auto s=prepared(rules);install(rules,s,"MA003");act(rules,s,Action::craft(copy(s,"SH026")));act(rules,s,Action::craft(recipeCopy(s,"SH001")));act(rules,s,Action::load({part(s,"SH001")}));
        State decoded;std::string error;const auto bytes=serialize(s);check(deserialize(bytes,decoded,error),error);check(serialize(decoded)==bytes,"snapshot roundtrip");
        act(rules,decoded,Action::fire(decoded.enemies[0].id));act(rules,s,Action::fire(s.enemies[0].id));check(stateHash(s)==stateHash(decoded),"roundtrip continuation divergence");
        auto damaged=bytes;damaged.back()^=1;const auto before=serialize(decoded);check(!deserialize(damaged,decoded,error),"corrupt snapshot accepted");check(serialize(decoded)==before,"failed load mutated output");
        check(!deserialize(bytes.substr(0,bytes.size()/2),decoded,error),"truncated save accepted");
        auto future=s;future.contentVersion="future";check(!deserialize(serialize(future),decoded,error),"incompatible content accepted");
    });
    run("separate seeded RNG domains and repeatable full action/event replay",[&]{
        auto a=Rng::seeded(37,"encounter/a"),b=a;for(int i=0;i<50;++i)a.below(Domain::Shop,11);check(a.below(Domain::Robot,113)==b.below(Domain::Robot,113),"shop RNG perturbed robot RNG");
        auto first=Rules::teachingEncounter(99),second=Rules::teachingEncounter(99);auto r1=playTeachingFixture(first,rules),r2=playTeachingFixture(second,rules);
        check(r1.ok && r2.ok && stateHash(first)==stateHash(second) && r1.events.size()==r2.events.size(),"deterministic replay");
        for(std::size_t i=0;i<r1.events.size();++i)check(eventJson(r1.events[i])==eventJson(r2.events[i]),"event divergence");
    });
    std::cout<<"All "<<checks<<" behavioral assertions passed. No complete-city, durable-save, graphical, balance or human-quality claim.\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}

// Independent P08 semantic probes, selected from source before the first freeze.
// Controlled arenas isolate exact behavior; they are not full-city playtests.
#include "overkill/core.hpp"
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>
using namespace overkill;
namespace {
int passed=0,failed=0;
void expect(bool value,const std::string& why){if(!value)throw std::runtime_error(why);}
void equal(Amount actual,Amount expected,const std::string& why){expect(actual==expected,why+" expected="+std::to_string(expected)+" actual="+std::to_string(actual));}
Result accepted(Result out){expect(out.ok,out.reason);return out;}
Result act(const Rules&r,State&s,const Action&a){return accepted(r.apply(s,a));}
State arena(Amount damage=0,Amount hits=0){State s;s.seed=8821;s.encounter="independent-upgrades";s.rng=Rng::seeded(s.seed,s.encounter);s.phase=Phase::Collection;s.hotBarrel=false;s.materials={500,500,500,500,500};Enemy e;e.id=s.nextId++;e.definition="QA";e.hp=e.maxHp=10000;e.intent={damage?Move::Attack:Move::Recover,damage,hits};e.pattern={e.intent};s.enemies={e};return s;}
Id memory(State&s,const std::string&id){auto n=s.nextId++;s.memory.push_back({n,id,0,0,0});return n;}
RecipeCopy& copy(State&s,Id id){auto it=std::find_if(s.memory.begin(),s.memory.end(),[&](const RecipeCopy&c){return c.id==id;});expect(it!=s.memory.end(),"copy missing");return *it;}
Part& part(State&s,Id id){auto it=std::find_if(s.parts.begin(),s.parts.end(),[&](const Part&p){return p.id==id;});expect(it!=s.parts.end(),"part missing");return *it;}
void acquire(const Rules&r,State&s,const std::string&id){accepted(r.acquireUpgrade(s,id));}
void begin(const Rules&r,State&s){accepted(r.startUpgrades(s,EncounterClass::Regular));}
void collect(const Rules&r,State&s){act(r,s,Action::collect(0));}
void start(const Rules&r,State&s){begin(r,s);collect(r,s);}
void choose(const Rules&r,State&s,std::vector<Id>objects){expect(!s.upgradeChoices.empty(),"expected pending choice");Action a;a.type=ActionType::ResolveUpgradeChoice;a.upgradeChoice.choice=s.upgradeChoices.front().id;a.upgradeChoice.objects=std::move(objects);act(r,s,a);}
std::vector<Id> made(const Result&r){std::vector<Id>ids;for(const auto&e:r.events)if(e.type=="part_created")ids.push_back(e.subject);return ids;}
Id first(const Result&r){auto ids=made(r);expect(!ids.empty(),"expected physical output");return ids.front();}
Result use(const Rules&r,State&s,Id id,Action a={}){a.type=ActionType::Craft;a.subject=id;return act(r,s,a);}
Result use(const Rules&r,State&s,const std::string&id,Action a={}){return use(r,s,memory(s,id),a);}
Id craft(const Rules&r,State&s,const std::string&id){return first(use(r,s,id));}
Id grant(const Rules&r,State&s,const std::string&id){return first(accepted(r.grantPart(s,id)));}
Id plain(const Rules&r,State&s,Kind kind,Amount value,bool installed=false){return first(accepted(r.grantPlainPart(s,kind,value,"independent-setup",installed)));}
void install(const Rules&r,State&s,Id id){act(r,s,Action::install(id));}
void activate(const Rules&r,State&s,Id id){Action a;a.type=ActionType::Activate;a.subject=id;act(r,s,a);}
Result fire(const Rules&r,State&s,std::vector<Id>ids){act(r,s,Action::load(std::move(ids)));return act(r,s,Action::fire(s.enemies.front().id));}
void next(const Rules&r,State&s){act(r,s,Action::endTurn());if(s.phase==Phase::Collection)collect(r,s);}
void rejected(const Rules&r,State&s,const Action&a){const auto before=serialize(s);expect(!r.apply(s,a).ok,"expected rejection");expect(before==serialize(s),"rejection changed authoritative state");}
void roundtrip(State&s){auto before=serialize(s);State decoded;std::string error;expect(deserialize(before,decoded,error),error);expect(serialize(decoded)==before,"snapshot not canonical");s=std::move(decoded);}
std::string events(const Result&r){std::string out;for(const auto&e:r.events)out+=eventJson(e);return out;}
void run(const char*name,const std::function<void()>&fn){try{fn();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception&e){++failed;std::cout<<"FAIL "<<name<<": "<<e.what()<<'\n';}}
}
int main(){Rules r;
run("U01 calibration acquisition is nonlethal, atomic, once and reloadable",[&]{
 auto s=arena();auto ammo=memory(s,"MA038"),shield=memory(s,"SH002");s.hp=8;auto before=serialize(s);expect(!r.acquireUpgrade(s,"UGS-039").ok,"8HP acquisition accepted");expect(serialize(s)==before,"rejected acquisition mutated state");s.hp=9;acquire(r,s,"UGS-039");equal(s.hp,1,"pay8 once");expect(s.upgradeChoices.size()==1,"choice not retained");rejected(r,s,Action::collect(0));roundtrip(s);choose(r,s,{ammo,shield});equal(s.hp,1,"choice recharged acquisition HP");before=serialize(s);expect(!r.acquireUpgrade(s,"UGS-039").ok,"duplicate ID accepted");expect(serialize(s)==before,"duplicate acquisition mutated state");
});
run("U02 acquisition maxHP does not repeat on reload or fight start",[&]{
 auto s=arena();s.hp=40;acquire(r,s,"UGS-071");equal(s.maxHp,84,"maxHP gain");equal(s.hp,44,"gain heals4");roundtrip(s);start(r,s);equal(s.maxHp,84,"start reacquired maxHP");equal(s.hp,44,"start reacquired healing");
});
run("U03 permanent tuning is per copy and first physical output only",[&]{
 auto s=arena();auto chosen=memory(s,"MA038"),other=memory(s,"MA038"),shield=memory(s,"SH002");acquire(r,s,"UGS-039");choose(r,s,{chosen,shield});start(r,s);auto chosenParts=made(use(r,s,chosen)),otherParts=made(use(r,s,other));expect(chosenParts.size()==2&&otherParts.size()==2,"bundle output count");for(auto id:chosenParts)expect(part(s,id).sourceRecipeCopy==chosen,"selected source-copy provenance lost");fire(r,s,chosenParts);equal(s.enemies[0].hp,9990,"chosen6+4");fire(r,s,otherParts);equal(s.enemies[0].hp,9982,"untuned4+4");
});
run("U04 cold-start production allowance excludes free grants and Utilities",[&]{
 auto s=arena();acquire(r,s,"UGS-009");start(r,s);grant(r,s,"SH001");use(r,s,"MA001");auto bundle=made(use(r,s,"MA038"));auto slug=craft(r,s,"SH001"),plate=craft(r,s,"SH002"),fourth=craft(r,s,"SH001");install(r,s,plate);equal(Rules::shield(s),8,"third qualifying Use6+2Shield");fire(r,s,bundle);equal(s.enemies[0].hp,9990,"first output6, second4");fire(r,s,{slug,fourth});equal(s.enemies[0].hp,9976,"second8 plus fourth6");
});
run("U05 Trial Fabricator copies committed first output without recursion",[&]{
 auto s=arena();auto ammo=memory(s,"MA038"),shield=memory(s,"SH002");acquire(r,s,"UGS-039");choose(r,s,{ammo,shield});acquire(r,s,"UGS-015");start(r,s);auto ids=made(use(r,s,ammo));equal(static_cast<Amount>(ids.size()),3,"two original parts plus one finite copy");equal(s.heat,0,"copy replayed after-hit effects before firing");roundtrip(s);fire(r,s,ids);equal(s.enemies[0].hp,9984,"6+4+6 committed damage");equal(s.heat,3,"one printed on-hit Heat per fired part");
});
run("U06 Ammo stamp excludes None and is independent per physical copy",[&]{
 auto s=arena();auto a=memory(s,"SH012"),b=memory(s,"SH012"),none=memory(s,"SH001");acquire(r,s,"UGS-145");const auto&offers=s.upgradeChoices.front().objects;expect(std::find(offers.begin(),offers.end(),none)==offers.end(),"None recipe offered");choose(r,s,{a,b});start(r,s);use(r,s,a);equal(copy(s,a).cooldown,0,"first numeric1 assigned0");roundtrip(s);use(r,s,a);equal(copy(s,a).cooldown,1,"second assignment has normal1");rejected(r,s,Action::craft(a));use(r,s,b);equal(copy(s,b).cooldown,0,"other copy has independent first assignment");use(r,s,b);equal(copy(s,b).cooldown,1,"other copy second assignment normal");
});
run("U07 Utility stamp does not waive HP or reset another copy",[&]{
 auto s=arena();auto a=memory(s,"MA019"),b=memory(s,"MA019"),none=memory(s,"MA001");acquire(r,s,"UGS-144");const auto&offers=s.upgradeChoices.front().objects;expect(std::find(offers.begin(),offers.end(),none)==offers.end(),"None Utility offered");choose(r,s,{a,b});start(r,s);use(r,s,a);use(r,s,a);equal(s.hp,74,"both ordinary3HP costs paid");equal(copy(s,a).cooldown,1,"later Use normal cooldown");use(r,s,b);equal(s.hp,71,"independent copy fullHP cost");equal(copy(s,b).cooldown,0,"independent copy first reduction");
});
run("U08 Batch Permit changes only selected None copy on turn one",[&]{
 auto s=arena();auto a=memory(s,"SH001"),b=memory(s,"SH001");acquire(r,s,"MY1-21");begin(r,s);choose(r,s,{a});collect(r,s);auto iron=s.materials[0];use(r,s,a);use(r,s,a);equal(s.materials[0],iron-2,"both Uses paid Iron");rejected(r,s,Action::craft(a));use(r,s,b);rejected(r,s,Action::craft(b));next(r,s);use(r,s,a);rejected(r,s,Action::craft(a));
});
run("U09 Ballistic Starter survives zero-shot turns and expires after one Fire",[&]{
 auto s=arena();acquire(r,s,"UGS-001");start(r,s);next(r,s);roundtrip(s);next(r,s);fire(r,s,{grant(r,s,"SH001")});equal(s.enemies[0].hp,9988,"first fight shot6+6 after two zero-shot turns");fire(r,s,{grant(r,s,"SH001")});equal(s.enemies[0].hp,9982,"later shot6");
});
run("U10 Alignment adds before percentage and only on first Fire that turn",[&]{
 auto s=arena();acquire(r,s,"UGS-139");start(r,s);activate(r,s,craft(r,s,"SH050"));fire(r,s,{grant(r,s,"SH001")});equal(s.enemies[0].hp,9989,"floor((6+1)*1.6)=11");fire(r,s,{grant(r,s,"SH001")});equal(s.enemies[0].hp,9983,"later same-turn6");next(r,s);fire(r,s,{grant(r,s,"SH001")});equal(s.enemies[0].hp,9976,"next-turn7");
});
run("U11 named retention sums but cannot create Shield",[&]{
 for(Amount input:{5,20}){auto s=arena();acquire(r,s,"UGS-123");acquire(r,s,"MY3-03");start(r,s);plain(r,s,Kind::Shield,input,true);next(r,s);equal(Rules::shield(s),std::min(input,9),"bounded3+6 retention");expect(s.parts.empty(),"retention preserved reusable old physical payload");roundtrip(s);next(r,s);equal(Rules::shield(s),std::min(input,9),"retained balance duplicated");}
});
run("U12 Shield reader and spender use binding order before real retention",[&]{
 for(bool readerFirst:{true,false}){auto s=arena();acquire(r,s,"UGS-123");acquire(r,s,"MY3-03");start(r,s);if(readerFirst)install(r,s,craft(r,s,"MA055"));activate(r,s,craft(r,s,"SH066"));if(!readerFirst)install(r,s,craft(r,s,"MA055"));plain(r,s,Kind::Shield,2,true);const auto iron=s.materials[0];next(r,s);equal(Rules::shield(s),3,"12 minus complete9 payment leaves3");equal(s.materials[0]-iron,readerFirst?10:8,"haul5+Sweep3+reader2or0");}
});
run("U13 Vent Fan counts actual Heat payment, once, and excludes decay",[&]{
 auto s=arena();acquire(r,s,"MAU-03");start(r,s);s.heat=10;activate(r,s,craft(r,s,"MA010"));equal(Rules::shield(s),3,"3Heat part payment grants3Shield");use(r,s,"MA035");equal(Rules::shield(s),3,"second payment same turn gives no extra");next(r,s);equal(Rules::shield(s),0,"normal Heat decay must not grant3Shield");s.heat=2;use(r,s,"MA035");equal(Rules::shield(s),3,"next-turn2Heat Utility payment qualifies");
});
run("U14 later Suture heal cannot finance own HP cost",[&]{
 auto s=arena();acquire(r,s,"MAU-05");start(r,s);auto recipe=memory(s,"MA116");s.hp=6;rejected(r,s,Action::craft(recipe));s.hp=7;use(r,s,recipe);equal(s.hp,3,"7-6 then2 Suture");auto other=memory(s,"MA019");rejected(r,s,Action::craft(other));equal(s.hp,3,"same-turn future healing cannot fund3HP");
});
run("U15 first recipe healing excludes acquisition healing",[&]{
 auto s=arena();s.hp=40;acquire(r,s,"UGS-106");acquire(r,s,"UGS-071");equal(s.hp,44,"maxHP gain heals its4 only");start(r,s);use(r,s,"SH074");equal(s.hp,51,"first actual recipe4+3");roundtrip(s);use(r,s,"SH074");equal(s.hp,55,"later Quick Patch ordinary4");
});
run("U16 cooldown tie selects memory position rather than smallest ID",[&]{
 auto s=arena();s.memory={{200,"SH074",5,0,0},{100,"SH074",5,0,0}};s.nextId=201;acquire(r,s,"UGS-094");start(r,s);next(r,s);next(r,s);equal(copy(s,200).cooldown,2,"first slot auto5-2-port1");equal(copy(s,100).cooldown,3,"second slot auto5-2");
});
run("U17 Momentum uses producing-copy identity, one shot and consecutive turns",[&]{
 auto s=arena();auto selected=memory(s,"MA038"),other=memory(s,"MA038");acquire(r,s,"UGS-102");choose(r,s,{selected});start(r,s);auto a=made(use(r,s,selected)),b=made(use(r,s,other));fire(r,s,{b.front()});equal(s.enemies[0].hp,9996,"other physical copy gets no bonus");fire(r,s,{a.front()});equal(s.enemies[0].hp,9990,"first qualifying4+2");fire(r,s,{a.back()});equal(s.enemies[0].hp,9986,"same-turn next shot4 only");next(r,s);auto c=made(use(r,s,selected));fire(r,s,{c.front()});equal(s.enemies[0].hp,9978,"second consecutive turn4+4");next(r,s);next(r,s);roundtrip(s);auto d=made(use(r,s,selected));fire(r,s,{d.front()});equal(s.enemies[0].hp,9972,"missing turn resets to4+2");
});
run("U18 Toolhead claims after three earlier Uses and free Use is not paid",[&]{
 auto s=arena();acquire(r,s,"MY1-09");acquire(r,s,"UGS-136");start(r,s);for(int i=0;i<3;++i)use(r,s,"MA001");auto fourth=memory(s,"MA001");Action discount;discount.discount[2]=1;const auto carbon=s.materials[2];use(r,s,fourth,discount);equal(s.materials[2],carbon,"chosen full material discount");equal(Rules::shield(s),0,"zero-material Use counted as fourth paid Utility");use(r,s,"MA001");equal(Rules::shield(s),5,"next actual paid Utility is fourth");
});
run("U19 Toolhead cannot invent ingredients or waive remaining HP cost",[&]{
 auto s=arena();acquire(r,s,"MY1-09");start(r,s);for(int i=0;i<3;++i)use(r,s,"MA001");auto recipe=memory(s,"MA116");Action a=Action::craft(recipe);a.discount[3]=1;rejected(r,s,a);a.discount={0,0,2,0,0};s.hp=6;rejected(r,s,a);s.hp=7;const auto before=s.materials;act(r,s,a);equal(s.hp,1,"ordinary6HP cost retained");equal(s.materials[2],before[2],"discounted Carbon2");equal(s.materials[0],before[0]-1,"Iron1 still paid");
});
run("U20 five-material threshold uses actual payment after discounts",[&]{
 auto s=arena();acquire(r,s,"MY1-09");acquire(r,s,"UGS-055");start(r,s);for(int i=0;i<3;++i)use(r,s,"MA001");Action discount;discount.discount[2]=2;use(r,s,"MA116",discount);equal(Rules::shield(s),24,"3actualmaterials must not grant extra3");use(r,s,"MA116");equal(Rules::shield(s),51,"5actualmaterials grants recipe24 plus brace3");
});
run("U21 paid Utility counter continues between turns",[&]{
 auto s=arena();acquire(r,s,"UGS-136");start(r,s);use(r,s,"MA001");use(r,s,"MA001");next(r,s);use(r,s,"MA001");equal(Rules::shield(s),0,"third paid Utility premature");use(r,s,"MA001");equal(Rules::shield(s),5,"fourth across turn boundary");
});
run("U22 counterbattery responds to each actual attack hit",[&]{
 auto s=arena(3,2);acquire(r,s,"UGS-019");start(r,s);plain(r,s,Kind::Shield,2,true);next(r,s);equal(s.hp,76,"two3hits less2Shield");equal(s.enemies[0].hp,9996,"two separate2damage counters");
});
run("U23 lethal Utility Arc recoil interrupts healing and later damage siblings",[&]{
 auto s=arena();auto other=s.enemies[0];other.id=s.nextId++;s.enemies.push_back(other);s.enemies[0].mesh=2;acquire(r,s,"UGS-066");acquire(r,s,"MAU-05");start(r,s);use(r,s,"MA001");use(r,s,"MA001");s.hp=4;use(r,s,"MA019");expect(s.phase==Phase::Defeat,"1remainingHP must die to2recoil; actual HP="+std::to_string(s.hp)+" second target HP="+std::to_string(s.enemies[1].hp));equal(s.hp,0,"later ordinary healing resurrected player");equal(s.enemies[1].hp,10000,"AoE continued after final death");
});
run("U24 turn-start grants precede Pulse recoil even with later acquisition",[&]{
 auto s=arena();s.enemies[0].mesh=4;acquire(r,s,"UGS-076");acquire(r,s,"UGS-053");start(r,s);s.hp=2;next(r,s);equal(s.hp,2,"fresh10Shield must protect from turn-start4recoil");equal(Rules::shield(s),6,"fresh10 less recoil4");equal(s.enemies[0].hp,9998,"turn2Pulse2damage");
});
run("U25 nested first-turn choices serialize and preview without replay",[&]{
 auto s=arena();auto slug=memory(s,"SH001");memory(s,"SH002");acquire(r,s,"MY1-21");acquire(r,s,"MY1-23");begin(r,s);expect(s.upgradeChoices.size()==1,"first nested choice absent");rejected(r,s,Action::collect(0));Action answer;answer.type=ActionType::ResolveUpgradeChoice;answer.upgradeChoice.choice=s.upgradeChoices.front().id;answer.upgradeChoice.objects={slug};const auto original=serialize(s);auto one=r.preview(s,answer),two=r.preview(s,answer);expect(one.result.ok&&two.result.ok,"nested choice preview rejected");expect(serialize(s)==original,"preview changed source");expect(serialize(one.state)==serialize(two.state)&&events(one.result)==events(two.result),"preview changed candidate/cursor");roundtrip(s);auto actual=act(r,s,answer);expect(serialize(s)==serialize(one.state)&&events(actual)==events(one.result),"reloaded resolution differs");expect(s.upgradeChoices.size()==1,"second choice lost");choose(r,s,{slug});collect(r,s);auto a=first(use(r,s,slug)),b=first(use(r,s,slug));fire(r,s,{a});equal(s.enemies[0].hp,9989,"selected first output6+5");fire(r,s,{b});equal(s.enemies[0].hp,9983,"second output normal6");
});
run("U26 copying eligibility requires permanent memory and delegates saved selection",[&]{
 auto s=arena();auto borrowed=memory(s,"SH012");copy(s,borrowed).storage=MemoryKind::Borrowed;copy(s,borrowed).borrowedFrom="UGS-133";expect(!upgradeEligible(r,s,"UGS-032"),"Borrowed-only memory enables permanent copying");memory(s,"SH001");expect(upgradeEligible(r,s,"UGS-032"),"permanent memory incorrectly excluded");acquire(r,s,"UGS-032");expect(s.upgradeRequests.size()==1&&s.upgradeRequests.front().kind==UpgradeRequestKind::CopyRecipe,"saved campaign copy request missing");roundtrip(s);
});
run("U27 shop reductions are product-scoped and sale bonus floors per core",[&]{
 auto s=arena();equal(upgradeShopPrice(s,PurchaseKind::Upgrade,15),15,"unowned discount applies to itself");acquire(r,s,"UGS-075");equal(upgradeShopPrice(s,PurchaseKind::Material,19),15,"floor19x0.8 once");equal(upgradeShopPrice(s,PurchaseKind::Recipe,1),1,"priced minimum1");equal(upgradeShopPrice(s,PurchaseKind::Part,19),19,"Part purchase outside discount");acquire(r,s,"UGS-017");equal(upgradeCoreSaleValue(s,15)+upgradeCoreSaleValue(s,15),36,"two separate15core bonuses3each");
});
run("U28 Opening Laminator saves its bonus and fixed resale reference",[&]{
 auto s=arena();acquire(r,s,"UGS-140");start(r,s);auto p=craft(r,s,"SH002");const auto reference=part(s,p).resaleReference;const auto basis=part(s,p).materialBasis;install(r,s,p);equal(Rules::shield(s),10,"first6Shield output+4");act(r,s,Action::remove(p));roundtrip(s);next(r,s);install(r,s,p);equal(Rules::shield(s),10,"saved physical bonus preserved");expect(part(s,p).resaleReference==reference&&part(s,p).materialBasis==basis,"saving or reinstall repriced output");auto later=craft(r,s,"SH002");install(r,s,later);equal(Rules::shield(s),16,"later Shield production normal6");
});
run("U29 Vent payment protects before recoil and combined affordability stays atomic",[&]{
 auto s=arena();s.enemies[0].mesh=4;auto other=s.enemies[0];other.id=s.nextId++;other.mesh=0;s.enemies.push_back(other);acquire(r,s,"MAU-03");start(r,s);s.hp=2;s.heat=7;auto a=grant(r,s,"MA041"),b=grant(r,s,"MA041");act(r,s,Action::load({grant(r,s,"SH001"),a,b}));Action shot=Action::fire(s.enemies[0].id);shot.partTargets={{a,other.id},{b,other.id}};rejected(r,s,shot);equal(Rules::shield(s),0,"rejected aggregate cost granted protection");s.heat=8;act(r,s,shot);equal(s.hp,1,"payment's3Shield protects before4recoil");equal(s.heat,0,"both physical4Heat costs paid");equal(Rules::shield(s),0,"once-per-turn3Shield fully consumed");equal(s.enemies[1].hp,9997,"named spread hits only once");
});
run("U30 generated Shield bonus never changes original value or printed-only copies",[&]{
 auto s=arena();acquire(r,s,"UGS-140");start(r,s);auto original=craft(r,s,"SH026");equal(Rules::shield(s),8,"immediate4 plus Laminator4");expect(part(s,original).materialBasis==Materials({1,1,0,0,0}),"4Shield grant repriced after bonus");act(r,s,Action::remove(original));Action chosen;chosen.parts={original};use(r,s,"SH118",chosen);roundtrip(s);next(r,s);std::vector<Id> clones;for(const auto&p:s.parts)if(p.id!=original&&p.place==Place::Reserve)clones.push_back(p.id);equal(static_cast<Amount>(clones.size()),2,"fresh copies arrive in reserve");for(Id id:clones)install(r,s,id);equal(Rules::shield(s),14,"regular6 delivery plus two printed4 copies");for(const auto&p:s.parts)if(p.id!=original)expect(p.materialBasis==Materials({1,1,0,0,0}),"delivery or copy repriced original grant");install(r,s,original);equal(Rules::shield(s),22,"saved original keeps its own8");
});
run("U31 Cooling Authorization is free but never refreshes None-use flags",[&]{
 auto s=arena();auto none=memory(s,"SH001"),numeric=memory(s,"SH074");acquire(r,s,"MY1-10");start(r,s);use(r,s,none);use(r,s,numeric);Action action;action.type=ActionType::ActivateUpgrade;action.upgrade="MY1-10";action.subject=none;rejected(r,s,action);action.subject=numeric;const auto cost=s.materials;act(r,s,action);expect(s.materials==cost,"equipment charged materials");equal(copy(s,numeric).cooldown,0,"numeric cooldown was not cleared");rejected(r,s,Action::craft(none));use(r,s,numeric);rejected(r,s,action);
});
run("U32 chosen-resource return is capped by actual payment and consumed once",[&]{
 auto s=arena();acquire(r,s,"MY1-08");begin(r,s);expect(s.upgradeChoices.size()==1,"material choice absent");Action chosen;chosen.type=ActionType::ResolveUpgradeChoice;chosen.upgradeChoice.choice=s.upgradeChoices.front().id;chosen.upgradeChoice.values={0};act(r,s,chosen);collect(r,s);const auto before=s.materials;use(r,s,"MA038");equal(s.materials[0],before[0],"return only2Iron actually paid");equal(s.materials[2],before[2]-1,"Carbon not refunded");use(r,s,"MA038");equal(s.materials[0],before[0]-2,"second Use no extra return");
});
run("U33 Plain Slug return is a Fire trigger rather than paid production",[&]{
 auto s=arena();acquire(r,s,"UGS-044");start(r,s);const auto iron=s.materials[0];auto free=grant(r,s,"SH001");act(r,s,Action::load({free}));equal(s.materials[0],iron,"loading granted return");Action unload;unload.type=ActionType::Unload;act(r,s,unload);equal(s.materials[0],iron,"unloading granted return");fire(r,s,{free});equal(s.materials[0],iron+1,"named Fire trigger returns1 even for granted Slug");fire(r,s,{grant(r,s,"SH001")});equal(s.materials[0],iron+1,"second same-turn return");
});
run("U34 sacrifice upgrades count individual payments but exclude Fire",[&]{
 auto s=arena();acquire(r,s,"UGS-038");acquire(r,s,"UGS-056");start(r,s);fire(r,s,{grant(r,s,"SH001")});equal(s.enemies[0].hp,9994,"Fire was treated as sacrifice");auto a=grant(r,s,"SH001"),b=grant(r,s,"SH001"),c=grant(r,s,"SH001");Action pair;pair.parts={a,b};pair.choices={0,1};use(r,s,"SH101",pair);equal(s.enemies[0].hp,9988,"two individual3damage sacrifice triggers");const auto glass=s.materials[3];Action third;third.parts={c};third.choices={2};use(r,s,"SH101",third);equal(s.enemies[0].hp,9988,"third same-turn sacrifice exceeded two-trigger cap");equal(s.materials[3],glass,"third sacrifice returns1Glass after Utility pays1Glass");
});
run("U35 seeded material discount persists and reduces only eligible actual ingredient",[&]{
 auto s=arena();acquire(r,s,"UGS-116");start(r,s);auto* upgrade=ownedUpgrade(s,"UGS-116");expect(upgrade&&upgrade->values.size()==1,"selected material not visible");const auto kind=upgrade->values[0];expect(kind>=0&&kind<5,"invalid material");const std::string recipe=kind==0?"SH001":kind==1?"SH002":kind==2?"MA001":"SH074";const auto at=static_cast<std::size_t>(kind);auto before=s.materials;use(r,s,recipe);equal(s.materials[at],before[at],"printed1 selected material becomes0");roundtrip(s);expect(ownedUpgrade(s,"UGS-116")->values[0]==kind,"reload rerolled material");use(r,s,recipe);equal(s.materials[at],before[at]-1,"second Use pays selected material normally");
});
run("U36 Mara capacity and decay exceptions never discount explicit payments",[&]{
 auto s=arena();acquire(r,s,"MAU-02");equal(s.heat,0,"extra capacity should not fill itself");equal(upgradeHeatCap(s),14,"base10 plus4 capacity");acquire(r,s,"MAU-01");acquire(r,s,"MAU-04");start(r,s);equal(s.heat,2,"fresh Primer2");for(int i=0;i<5;++i)use(r,s,"MA001");equal(s.heat,14,"new cap14");use(r,s,"MA035");equal(s.heat,12,"explicit2Heat cost not reduced by Seal");next(r,s);equal(s.heat,11,"ordinary2decay reduced to1");
});
std::cout<<"SUMMARY "<<passed<<" passed, "<<failed<<" failed\n";return failed?1:0;
}

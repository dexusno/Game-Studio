#include "overkill/core.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
using namespace overkill;
namespace {
int assertions=0,failures=0;std::set<std::string> exercised;
void check(bool b,const std::string& label){++assertions;if(!b)throw std::runtime_error(label);}
void act(const Rules& r,State& s,const Action& a){const auto result=r.apply(s,a);check(result.ok,result.reason);}
Id memory(State& s,const std::string& id){const Id n=s.nextId++;s.memory.push_back({n,id,0,0,0});return n;}
State fixture(const Rules& rules){State s;s.rng=Rng::seeded(8,"recipes");s.encounter="recipe-contracts";s.phase=Phase::Preparation;s.hotBarrel=false;s.hp=60;s.heat=8;s.materials={100,100,100,100,100};for(int i=0;i<3;++i){Enemy e;e.id=s.nextId++;e.definition="TEST";e.name="Target";e.hp=e.maxHp=1000;e.burn=i==0?4:2;e.corrosion=i==0?2:0;e.weaken=i==0?3:0;e.intent={Move::Attack,10,1};e.pattern={e.intent};s.enemies.push_back(e);}const auto result=rules.grantPlainPart(s,Kind::Shield,80,"test-protection",true);check(result.ok,result.reason);return s;}
Id findPart(const State& s,const std::string& source){for(auto i=s.parts.rbegin();i!=s.parts.rend();++i)if(i->recipe==source&&i->place==Place::Reserve)return i->id;throw std::runtime_error("No reserve part: "+source);}
Id grant(const Rules&r,State&s,const std::string&id){const auto result=r.grantPart(s,id);check(result.ok,result.reason);return findPart(s,id);}
Id craft(const Rules&r,State&s,const std::string&id,Action a=Action{}){a.type=ActionType::Craft;a.subject=memory(s,id);act(r,s,a);return findPart(s,id);}
void rejection(const Rules&r,State&s,const Action&a){const auto before=serialize(s);const auto result=r.apply(s,a);check(!result.ok,"Expected rejected action");check(before==serialize(s),"Rejected action changed authoritative state");}
void round(const Rules&r,State&s){act(r,s,Action::endTurn());if(s.phase==Phase::Collection)act(r,s,Action::collect(0));}
void shot(const Rules&r,State&s,Id part,Id extra=0){act(r,s,Action::load({part}));Action a=Action::fire(s.enemies[0].id);if(extra)a.partTargets={{part,extra}};act(r,s,a);}
void run(const char*name,const std::function<void()>&fn){try{fn();std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failures;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}}
}
int main(){try{Rules r;
run("full246 exact catalogue registrations and source costs",[&]{check(r.content().size()==246,"Expected all246 Shared/Mara definitions");check(Rules::implementedRecipeIds().size()==246,"Runtime registration count");check(r.recipe("MA038")->outputCount==2,"Rivet Bundle creates two physical parts");check(r.recipe("SH121")->rarity==Rarity::Rare&&r.recipe("SH124")->rarity==Rarity::Legendary,"Manual grab rarities");});
run("every Ammo's main damage in a controlled source-derived reference state",[&]{
    const std::map<std::string,Amount> expected{
        {"SH001",6},{"SH003",3},{"SH004",8},{"SH009",8},{"SH010",8},{"SH011",3},{"SH012",7},{"SH013",4},{"SH014",2},{"SH016",5},{"SH017",5},{"SH018",0},
        {"SH041",8},{"SH042",8},{"SH044",8},{"SH045",5},{"SH046",7},{"SH047",5},{"SH048",8},{"SH052",7},{"SH053",5},{"SH054",8},{"SH055",8},{"SH056",7},{"SH075",8},
        {"SH081",14},{"SH084",21},{"SH086",16},{"SH087",14},{"SH088",25},{"SH089",24},{"SH090",10},{"SH109",36},{"SH112",24},
        {"MA002",6},{"MA005",14},{"MA006",7},{"MA007",6},{"MA008",5},{"MA009",8},{"MA011",4},{"MA012",5},{"MA023",7},{"MA025",9},{"MA032",8},{"MA033",12},{"MA036",7},{"MA038",4},{"MA039",7},
        {"MA042",17},{"MA047",8},{"MA051",6},{"MA060",6},{"MA068",12},{"MA073",12},{"MA076",10},{"MA077",7},{"MA082",12},{"MA084",10},{"MA091",16},{"MA097",10},{"MA103",14},{"MA109",24},{"MA120",18}};
    for(const auto& pair:expected){auto s=fixture(r);const auto p=craft(r,s,pair.first);shot(r,s,p,s.enemies[1].id);check(s.enemies[0].hp==1000-pair.second,pair.first+" main/support damage expected "+std::to_string(pair.second)+" got "+std::to_string(1000-s.enemies[0].hp));check(s.shots==1&&s.round==1,pair.first+" Fire lifecycle");exercised.insert(pair.first);}
});
run("conditional Ammo snapshots, percentages, prehit order, independent saved copies",[&]{
    auto s=fixture(r);s.enemies[0].shield=10;auto a=craft(r,s,"SH010"),b=craft(r,s,"SH011");act(r,s,Action::load({b,a}));act(r,s,Action::fire(s.enemies[0].id));check(s.enemies[0].hp==989&&s.enemies[0].shield==0,"Heavy Casting snapshots Shield before Cutting Edge removes6;15 damage hits remaining4");
    s=fixture(r);auto old=craft(r,s,"SH090");round(r,s);auto fresh=grant(r,s,"SH090");shot(r,s,old);check(s.enemies[0].hp==976,"Stored Momentum18 plus first Burn4");shot(r,s,fresh);check(s.enemies[0].hp==966,"fresh copy starts age0");
    s=fixture(r);s.enemies[0].hp=200;auto finisher=craft(r,s,"SH012");shot(r,s,finisher);check(s.enemies[0].hp==188,"quarter HP condition");
    s=fixture(r);s.materials={0,0,0,0,0};auto last=grant(r,s,"SH053");shot(r,s,last);check(s.enemies[0].hp==983,"Last Scrap exact zero pool");
});
run("all Utility uses and required choices execute without omitted operation",[&]{
    for(const auto& recipe:r.content())if(recipe.kind==Kind::Utility){auto s=fixture(r);Action a;a.type=ActionType::Craft;a.subject=memory(s,recipe.id);a.target=s.enemies[0].id;a.targets={s.enemies[1].id};
        const Id ammo=grant(r,s,"SH009"),shield=grant(r,s,"SH002");
        if(recipe.id=="SH032"||recipe.id=="SH065"||recipe.id=="SH071"||recipe.id=="SH102"||recipe.id=="SH118"||recipe.id=="MA030"||recipe.id=="MA066")a.parts={ammo};
        if(recipe.id=="SH076"||recipe.id=="MA089")a.parts={shield};
        if(recipe.id=="SH069")a.choices={1};if(recipe.id=="SH101"){a.parts={ammo,shield};a.choices={0,1};}
        if(recipe.id=="MA095")s.heat=0;
        act(r,s,a);State restored;std::string error;check(deserialize(serialize(s),restored,error),recipe.id+" snapshot: "+error);check(serialize(restored)==serialize(s),recipe.id+" canonical snapshot");
        round(r,s);exercised.insert(recipe.id);
    }
});
run("all Shield installation values have a source-derived reference result",[&]{
    const std::map<std::string,Amount> values{{"SH002",6},{"SH006",4},{"SH019",10},{"SH020",6},{"SH021",7},{"SH022",6},{"SH023",7},{"SH025",5},{"SH027",4},{"SH028",9},{"SH058",9},{"SH059",7},{"SH060",16},{"SH061",7},{"SH062",6},{"SH063",8},{"SH091",24},{"SH092",15},{"SH093",13},{"SH094",16},{"SH096",12},{"SH097",14},{"SH098",10},{"SH113",38},{"SH114",25},
        {"MA003",6},{"MA013",11},{"MA014",5},{"MA015",8},{"MA016",5},{"MA017",10},{"MA018",6},{"MA022",12},{"MA024",12},{"MA028",7},{"MA031",12},{"MA034",8},{"MA040",7},{"MA044",10},{"MA048",10},{"MA054",18},{"MA055",10},{"MA057",8},{"MA059",7},{"MA062",6},{"MA065",10},{"MA070",16},{"MA071",6},{"MA072",9},{"MA075",18},{"MA079",12},{"MA083",34},{"MA086",12},{"MA096",24},{"MA099",22},{"MA102",16},{"MA104",12},{"MA108",18},{"MA115",24}};
    for(const auto& pair:values){auto s=fixture(r);const Id p=craft(r,s,pair.first);Action a=Action::install(p);a.target=s.enemies[0].id;if(pair.first=="MA022")a.sacrifices={grant(r,s,"SH001")};if(pair.first=="MA083")a.amount=8;if(pair.first=="MA096")a.amount=6;act(r,s,a);const auto it=std::find_if(s.parts.begin(),s.parts.end(),[p](const Part&x){return x.id==p;});check(it!=s.parts.end()&&it->shield==pair.second,pair.first+" Shield expected "+std::to_string(pair.second)+(it==s.parts.end()?" missing":" got "+std::to_string(it->shield)));round(r,s);exercised.insert(pair.first);}
    for(const auto* id:{"SH026","SH057"}){auto s=fixture(r);Action a=Action::craft(memory(s,id));act(r,s,a);round(r,s);exercised.insert(id);}
});
run("all planning Modifiers and Magnet effects have a complete action path",[&]{
    for(const auto& recipe:r.content())if(recipe.kind==Kind::Modifier||recipe.kind==Kind::Magnet){auto s=fixture(r);Action craftChoice;craftChoice.choices={0};if(recipe.id=="SH080"||recipe.id=="SH119"||recipe.id=="SH122")craftChoice.choices={0,1};const Id part=craft(r,s,recipe.id,craftChoice);
        Action a;a.type=ActionType::Activate;a.subject=part;a.target=s.enemies[0].id;if(recipe.id=="SH036")a.parts={grant(r,s,"SH001")};if(recipe.id=="MA052")a.parts={grant(r,s,"SH001"),grant(r,s,"SH003")};if(recipe.id=="MA045")a.amount=4;if(recipe.id=="MA061")a.amount=8;
        act(r,s,a);
        if(recipe.kind==Kind::Magnet){act(r,s,Action::endTurn());Action collect=Action::collect(0,2);if(recipe.id=="SH105")collect.discarded={2,0,0,0,0};act(r,s,collect);}
        else {const Id ammo=grant(r,s,"SH001");act(r,s,Action::load({ammo}));Action fire=Action::fire(s.enemies[0].id);if(recipe.id=="SH072")fire.partChoices={{0,ammo}};act(r,s,fire);round(r,s);}
        exercised.insert(recipe.id);
    }
});
run("all physical spread parts validate costs, own targets and resolution",[&]{for(const auto& recipe:r.content())if(recipe.kind==Kind::Spread){auto s=fixture(r);const Id p=craft(r,s,recipe.id),ammo=grant(r,s,"SH001");Action load=Action::load({ammo,p});if(recipe.id=="SH110")load.sacrifices={grant(r,s,"SH002")};act(r,s,load);Action a=Action::fire(s.enemies[0].id);if(recipe.id=="SH005"||recipe.id=="MA041")a.spreadTargets={{p,s.enemies[1].id}};if(recipe.id=="SH051")a.spreadTargets={{p,s.enemies[1].id},{p,s.enemies[2].id}};act(r,s,a);check(s.enemies[1].hp<1000,"Spread failed "+recipe.id);exercised.insert(recipe.id);}});
run("finite stock preserves baseline when Precision or named bonus stock is missing",[&]{
    auto s=fixture(r);s.phase=Phase::Collection;s.finitePile=true;s.pile={5,2,1,1,1};const auto old=s.materials;act(r,s,Action::collect(0,2));check(s.precisionSpent,"Attempt must be spent even when bonus stock is unavailable");check(s.materials[0]-old[0]==5,"Missing bonus must not remove guaranteed Iron");check(s.pile==Materials{},"Exactly10 units consumed");
    s=fixture(r);Action option;option.choices={1};const auto p=craft(r,s,"SH121",option);Action fit;fit.type=ActionType::Activate;fit.subject=p;act(r,s,fit);act(r,s,Action::endTurn());s.finitePile=true;s.pile={5,3,1,1,1};const auto before=s.materials;act(r,s,Action::collect(0,2));check(s.materials[1]-before[1]==3,"Copper bonus saturates remaining stock separately");check(s.materials[0]-before[0]==5,"No ordinary Perfect Iron was available");
});
run("fixed Shield recasting and copying preserve generated identity while dropping later bonuses",[&]{
    auto s=fixture(r);const Id plate=grant(r,s,"SH091");Action recast;recast.parts={plate};const Id slug=craft(r,s,"SH076",recast);auto get=[&](Id id)->const Part&{return *std::find_if(s.parts.begin(),s.parts.end(),[id](const Part& p){return p.id==id;});};
    check(get(slug).originalValue==14&&get(slug).materialBasis==Materials({3,0,0,0,0}),"Generated14 damage locks its pure-ammo ingredient basis");const auto reference=get(slug).resaleReference;
    const Id fuse=craft(r,s,"SH036");Action attach;attach.type=ActionType::Activate;attach.subject=fuse;attach.parts={slug};act(r,s,attach);
    Action copy;copy.parts={slug};copy.type=ActionType::Craft;copy.subject=memory(s,"SH118");act(r,s,copy);act(r,s,Action::endTurn());Amount copies=0;for(const auto& p:s.parts)if(p.recipe=="SH076"&&p.id!=slug){++copies;check(p.attachments.empty()&&p.originalValue==14&&p.resaleReference==reference,"Fresh generated copy keeps identity/value, not added Fuse");check(p.createdRound==2,"Copied age restarts");}check(copies==2,"Exact Duplication grants2 subparts");
});
run("first-install shot check expires on an unsuccessful or removed-source next shot",[&]{
    auto s=fixture(r);const Id catchPlate=craft(r,s,"SH062");act(r,s,Action::install(catchPlate));act(r,s,Action::remove(catchPlate));shot(r,s,grant(r,s,"SH001"));act(r,s,Action::install(catchPlate));const auto count=s.parts.size();shot(r,s,grant(r,s,"SH001"));check(s.parts.size()==count,"Reinstallation must not rearm consumed next-shot check");
});
run("part-only Mara costs exclude Utility costs and copying/Use are separate events",[&]{
    auto s=fixture(r);Action use=Action::craft(memory(s,"MA093"));act(r,s,use);use=Action::craft(memory(s,"MA064"));act(r,s,use);check(s.heatPaidRound==3&&s.partHeatPaidRound==0,"Utility Heat is telemetry but not a physical part payment");check(std::none_of(s.deliveries.begin(),s.deliveries.end(),[](const Delivery& d){return d.source=="MA093";}),"Utility must not earn Furnace Ledger part-only reward");
    act(r,s,Action::craft(memory(s,"MA019")));check(s.hpPaidFight==3&&s.partHpPaidFight==0,"Blood Fuel recipe Use is not a stored-part activation");const Id grate=craft(r,s,"MA054");act(r,s,Action::install(grate));check(s.partHpPaidFight==3,"Actual Shield part HP payment counted once");
});
run("prehit changes cannot rewrite ordinary Fire-condition snapshots",[&]{
    auto s=fixture(r);s.enemies[0].shield=6;const Id cut=craft(r,s,"SH011"),entry=craft(r,s,"SH086");act(r,s,Action::load({cut,entry}));act(r,s,Action::fire(s.enemies[0].id));check(s.enemies[0].corrosion==2,"Clean Entry condition reads original Shield, not earlier prehit removal");
    s=fixture(r);s.enemies[0].shield=12;s.heat=10;const Id spike=craft(r,s,"MA076"),splitter=craft(r,s,"MA111"),slug=grant(r,s,"SH001");act(r,s,Action::load({spike,slug,splitter}));act(r,s,Action::fire(s.enemies[0].id));check(s.enemies[0].hp==996,"Heat paid by this Fire cannot satisfy an earlier Fire-start condition");
});
run("HP-first Shield prevention and total attack reduction obey whole-attack facts",[&]{
    auto s=fixture(r);s.parts.clear();s.enemies.resize(1);s.enemies[0].weaken=0;s.enemies[0].burn=s.enemies[0].corrosion=0;s.enemies[0].intent={Move::Attack,6,2};s.enemies[0].pattern={s.enemies[0].intent};s.mark=5;
    const Id damper=craft(r,s,"SH024");Action fit;fit.type=ActionType::Activate;fit.subject=damper;act(r,s,fit);const Id brace=craft(r,s,"MA086");Action install=Action::install(brace);install.target=s.enemies[0].id;act(r,s,install);Rules::spendShield(s,12);const auto before=s.hp;act(r,s,Action::endTurn());check(before-s.hp==5,"(6+6+5Mark-6flat)/2 floors to5 total");
    s=fixture(r);s.parts.clear();s.enemies.resize(1);s.enemies[0].weaken=0;s.enemies[0].intent={Move::Attack,25,1};s.enemies[0].pattern={s.enemies[0].intent};const Id spring=craft(r,s,"MA044"),alarm=craft(r,s,"SH058");act(r,s,Action::install(spring));act(r,s,Action::install(alarm));const auto result=r.apply(s,Action::endTurn());check(result.ok,result.reason);check(std::any_of(result.events.begin(),result.events.end(),[](const Event& e){return e.type=="weaken_applied"&&e.amount==3;}),"Earlier recovered Shield must not hide the just-completed break from Alarm Plate");
});
run("Refuge Walls share restoration and Kiln Seals retain greatest remembered Heat",[&]{
    auto s=fixture(r);for(int i=0;i<2;++i){const auto p=craft(r,s,"MA115");act(r,s,Action::install(p));}auto result=r.apply(s,Action::endTurn());check(result.ok,result.reason);Amount restoration=0;for(const auto& e:result.events)if(e.type=="part_created")restoration+=e.amount;check(restoration==24,"Duplicate Refuge Walls must share24 total recovery, without replaying each attack");
    s=fixture(r);s.heat=8;auto p=craft(r,s,"MA102");act(r,s,Action::install(p));s.heat=3;p=craft(r,s,"MA102");act(r,s,Action::install(p));act(r,s,Action::endTurn());check(s.heat==8,"Later lower Kiln Seal must not replace highest8");
});
run("later killed-target support hit is lost without undoing earlier committed hits",[&]{
    auto s=fixture(r);s.enemies[1].hp=4;const auto one=craft(r,s,"SH017"),two=craft(r,s,"SH017");act(r,s,Action::load({one,two}));Action fire=Action::fire(s.enemies[0].id);fire.partTargets={{one,s.enemies[1].id},{two,s.enemies[1].id}};const auto result=r.apply(s,fire);check(result.ok,result.reason);check(s.enemies[1].dead&&s.enemies[0].hp==990,"Second support hit is lost, no transactional rejection");check(std::count_if(result.events.begin(),result.events.end(),[](const Event& e){return e.type=="hit_lost";})==1,"One lost support hit");
});
run("expanded snapshot preserves choices and rejects unsafe persisted indices/counters",[&]{
    auto s=fixture(r);Action choices;choices.choices={1,3};const auto p=craft(r,s,"SH122",choices);Action fit;fit.type=ActionType::Activate;fit.subject=p;act(r,s,fit);State restored;std::string error;check(deserialize(serialize(s),restored,error),error);const auto before=serialize(restored);act(r,s,Action::endTurn());act(r,restored,Action::endTurn());act(r,s,Action::collect(0,2));act(r,restored,Action::collect(0,2));check(serialize(s)==serialize(restored),"Stored collection choices diverged after reload");
    check(deserialize(before,restored,error),error);State bad=restored;bad.bindings[0].choices={9,3};State unchanged=restored;check(!deserialize(serialize(bad),unchanged,error),"Invalid material index accepted");check(serialize(unchanged)==serialize(restored),"Invalid load mutated live state");bad=restored;bad.partHeatPaidRound=-1;check(!deserialize(serialize(bad),unchanged,error),"Negative source-payment counter accepted");
});
run("combined Fire affordability precedes each physical part's payment trigger",[&]{
    auto s=fixture(r);act(r,s,Action::craft(memory(s,"MA093")));const Id a=craft(r,s,"MA041"),b=craft(r,s,"MA041"),ammo=grant(r,s,"SH001");act(r,s,Action::load({ammo,a,b}));Action fire=Action::fire(s.enemies[0].id,{{a,s.enemies[1].id},{b,s.enemies[1].id}});s.heat=7;rejection(r,s,fire);s.heat=8;act(r,s,fire);Amount earned=0;for(const auto& d:s.deliveries)if(d.source=="MA093")earned+=d.materials[0];check(earned==2,"Two4Heat physical payments must claim the first two Furnace Ledger triggers");check(s.heat==0,"Both physical costs paid once despite spread nonstacking");
});
run("Pressure Release honors optional chosen Shield amount without forced maximum",[&]{
    auto s=fixture(r);s.parts.clear();const Id old=grant(r,s,"SH002"),newer=grant(r,s,"SH006");act(r,s,Action::install(old));act(r,s,Action::install(newer));const Id collar=craft(r,s,"SH085");Action a;a.type=ActionType::Activate;a.subject=collar;a.amount=11;rejection(r,s,a);a.amount=4;act(r,s,a);const auto oldPart=std::find_if(s.parts.begin(),s.parts.end(),[old](const Part&p){return p.id==old;});const auto newPart=std::find_if(s.parts.begin(),s.parts.end(),[newer](const Part&p){return p.id==newer;});check(oldPart->shield==2&&newPart->shield==4,"Chosen4 is spent in installation order");shot(r,s,grant(r,s,"SH001"));check(s.enemies[0].hp==986,"Chosen4 gives8 damage, added to6Slug");
});
run("source IDs present in tested execution, no metadata-only omission",[&]{for(const auto& recipe:r.content())check(exercised.count(recipe.id)>0,"Untested recipe action path: "+recipe.id);});
check(failures==0,std::to_string(failures)+" recipe contract groups failed");
std::cout<<"All "<<assertions<<" recipe assertions passed; "<<exercised.size()<<" source recipes have executable action paths. Interaction coverage remains explicit.\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL recipe assertion "<<assertions<<": "<<e.what()<<'\n';return 1;}}

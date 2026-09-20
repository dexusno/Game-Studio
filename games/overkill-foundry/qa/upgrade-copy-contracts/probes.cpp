#include "overkill/core.hpp"
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

using namespace overkill;
namespace {
const Rules rules;
int checks=0,passed=0,failed=0;
void need(bool value,const std::string& reason){++checks;if(!value)throw std::runtime_error(reason);}
void test(const char* name,const std::function<void()>& body){try{body();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cout<<"FAIL "<<name<<": "<<e.what()<<'\n';}}
std::string events(const Result& r){std::string text;for(const auto& e:r.events)text+=eventJson(e)+"\n";return text;}
State scene(std::uint64_t seed=37){State s;s.hp=s.maxHp=80;s.hotBarrel=false;s.phase=Phase::Preparation;s.materials={100,100,100,100,100};s.encounter="independent-copy-contract";s.rng=Rng::seeded(seed,s.encounter);Enemy e;e.id=s.nextId++;e.hp=e.maxHp=1000;e.definition=e.name="Controlled target";e.intent={Move::Recover,0,0};e.pattern={e.intent};s.enemies.push_back(e);return s;}
Result act(State& s,const Action& a){const auto before=serialize(s);const auto preview=rules.preview(s,a);need(serialize(s)==before,"Preview changed authoritative state");const auto r=rules.apply(s,a);need(r.ok,r.reason);need(preview.result.ok&&serialize(s)==serialize(preview.state)&&events(r)==events(preview.result),"Preview/action state or events differ");return r;}
void own(State& s,const std::string& id){const auto r=rules.acquireUpgrade(s,id);need(r.ok,r.reason);}
void reload(State& s){const auto bytes=serialize(s);State restored;std::string error;need(deserialize(bytes,restored,error),error);need(serialize(restored)==bytes,"Canonical reload differs");s=std::move(restored);}
void begin(State& s){const auto r=rules.startUpgrades(s,EncounterClass::Regular);need(r.ok,r.reason);act(s,Action::collect(0));}
void next(State& s){act(s,Action::endTurn());act(s,Action::collect(0));}
Id store(State& s,const std::string& recipe){RecipeCopy copy;copy.id=s.nextId++;copy.recipe=recipe;s.memory.push_back(copy);return copy.id;}
Part& part(State& s,Id id){for(auto& p:s.parts)if(p.id==id)return p;throw std::runtime_error("Missing physical output");}
std::size_t markers(const Part& p){return static_cast<std::size_t>(std::count_if(p.attachments.begin(),p.attachments.end(),[](const Attachment& a){return a.source=="UGS-121";}));}
std::vector<Id> use(State& s,Id copy,Action a={}){const auto first=s.nextId;a.type=ActionType::Craft;a.subject=copy;const auto r=act(s,a);need(std::count_if(r.events.begin(),r.events.end(),[](const Event& e){return e.type=="recipe_used";})==1,"A copy repeated recipe Use");std::vector<Id> ids;for(const auto& p:s.parts)if(p.id>=first)ids.push_back(p.id);return ids;}
std::vector<Id> use(State& s,const std::string& recipe,Action a={}){return use(s,store(s,recipe),a);}
void activate(State& s,Id id,Action a={}){a.type=ActionType::Activate;a.subject=id;act(s,a);}
Amount fire(State& s,const std::vector<Id>& ids){act(s,Action::load(ids));const auto before=s.enemies[0].hp;act(s,Action::fire(s.enemies[0].id));for(const Id id:ids)need(std::none_of(s.parts.begin(),s.parts.end(),[&](const Part& p){return p.id==id;}),"Fired part not consumed");return before-s.enemies[0].hp;}
void tag(State& s,Id copy){need(s.upgradeChoices.size()==1,"One actual recipe tag choice");Action a;a.type=ActionType::ResolveUpgradeChoice;a.upgradeChoice.choice=s.upgradeChoices.front().id;a.upgradeChoice.objects={copy};act(s,a);}
}

int main(int argc,char** argv){
test("C01 all24 source orders preserve only earlier ordinary bonuses in the lease copy, never Jig",[]{
    std::vector<std::string> order{"MY1-01","UGS-015","UGS-046","UGS-121"};int permutations=0;
    do{auto s=scene();const Id recipe=store(s,"SH001");for(const auto& id:order){own(s,id);if(id=="UGS-046")tag(s,recipe);}begin(s);activate(s,use(s,"SH103").at(0));
        const auto ids=use(s,recipe);need(ids.size()==2,"Actual license original/copy pair required");
        const auto pos=[&](const std::string& id){return std::find(order.begin(),order.end(),id)-order.begin();};
        const Amount copiedBonus=(pos("MY1-01")<pos("UGS-015")?2:0)+(pos("UGS-046")<pos("UGS-015")?1:0);
        need(part(s,ids[0]).upgradeDamage==5&&part(s,ids[1]).upgradeDamage==copiedBonus,"Incorrect single-output exclusion or ordinary committed bonus");
        need(markers(part(s,ids[0]))==1&&markers(part(s,ids[1]))==0,"Attribution copied or duplicated");
        for(Id id:ids){const auto& p=part(s,id);need(p.sourceRecipeCopy==recipe&&p.canonicalRecipe=="SH001"&&p.resaleReference=="SH001"&&p.materialBasis==Materials{1,0,0,0,0},"Copy provenance/resale changed");need(std::count_if(p.attachments.begin(),p.attachments.end(),[](const Attachment& a){return a.source=="SH103"&&a.damage==8;})==1,"Ordinary committed attachment lost");}
        reload(s);next(s);need(fire(s,ids)==33+copiedBonus,"Real saved Fire violates acquisition order or one-physical Jig rule");++permutations;
    }while(std::next_permutation(order.begin(),order.end()));need(permutations==24,"All four-source orderings executed");
});
test("C02 later per-part damage and timed Heat attachments remain independent from Jig attribution",[]{
    auto s=scene();own(s,"UGS-121");own(s,"UGS-015");begin(s);const auto ids=use(s,"SH001");need(ids.size()==2,"Actual original/copy pair");
    Action original;original.parts={ids[0]};use(s,"MA030",original);Action duplicate;duplicate.parts={ids[1]};activate(s,use(s,"SH036").at(0),duplicate);use(s,"MA066",duplicate);
    need(markers(part(s,ids[0]))==1&&markers(part(s,ids[1]))==0,"Later attachments changed attribution");reload(s);next(s);const auto heat=s.heat;
    need(fire(s,{ids[0]})==16&&s.heat==heat,"Saved original must keep Jig2 and actual next-round rack8 only");
    need(fire(s,{ids[1]})==10&&s.heat==heat+4,"Independent copy must keep attached4 and one next-round Heat4");
});
test("C03 fully discounted actual Use consumes neither first resource-paid Jig nor lease allowance",[]{
    bool witnessed=false;for(std::uint64_t seed=1;seed<=64&&!witnessed;++seed){auto s=scene(seed);own(s,"UGS-116");own(s,"UGS-121");own(s,"UGS-015");begin(s);if(ownedUpgrade(s,"UGS-116")->values.at(0)!=0)continue;witnessed=true;
        const auto recipe=store(s,"SH001");const auto before=s.materials;const auto free=use(s,recipe);need(free.size()==1&&s.materials==before&&part(s,free[0]).upgradeDamage==0&&markers(part(s,free[0]))==0,"Zero resource payment incorrectly earned or consumed bonus");
        reload(s);const auto paid=use(s,"SH001");need(paid.size()==2&&s.materials[0]==before[0]-1&&part(s,paid[0]).upgradeDamage==2&&part(s,paid[1]).upgradeDamage==0,"First real paid Use not preserved");need(fire(s,{free[0],paid[0],paid[1]})==20,"Actual free6, paid8 and copied6 shot");
    }need(witnessed,"Actual Iron discount witness not found");
});
test("C04 adversarial delayed-copy failure rolls back earlier deliveries and the entire round after reload",[]{
    auto s=scene();own(s,"UGS-121");begin(s);const auto original=use(s,"SH001").at(0);Part corrupt=part(s,original);corrupt.origin=PartOrigin::Copied;corrupt.upgradeDamage=1;
    Delivery material;material.id=s.nextId++;material.source="controlled-earlier-delivery";material.dueRound=s.round+1;material.kind=DeliveryKind::Material;material.materials={3,0,0,0,0};s.deliveries.push_back(material);
    Delivery invalid;invalid.id=s.nextId++;invalid.source="controlled-invalid-copy";invalid.dueRound=s.round+1;invalid.kind=DeliveryKind::PartCopy;invalid.parts={corrupt};s.deliveries.push_back(invalid);reload(s);
    const auto before=serialize(s);const auto preview=rules.preview(s,Action::endTurn());need(!preview.result.ok&&preview.result.events.empty()&&serialize(s)==before,"Rejected preview mutated live state or exposed partial effects");const auto result=rules.apply(s,Action::endTurn());need(!result.ok&&result.reason.find("attribution")!=std::string::npos&&result.events.empty()&&serialize(s)==before,"Earlier delivery/clock/RNG mutation escaped rejected transaction");
});
test("C05 valid nested copy control removes one attributed component and leaves no marker for descendants",[]{
    // Controlled pending-delivery representation tests the common receive boundary;
    // this is not claimed as a naturally available additional copy recipe.
    auto s=scene();const auto recipe=store(s,"SH001");own(s,"UGS-046");tag(s,recipe);own(s,"UGS-121");begin(s);const auto original=use(s,recipe).at(0);Action attached;attached.parts={original};activate(s,use(s,"SH036").at(0),attached);
    Part copy=part(s,original);copy.origin=PartOrigin::Copied;copy.creator="controlled-committed-copy";Delivery d;d.id=s.nextId++;d.source=copy.creator;d.dueRound=s.round+1;d.kind=DeliveryKind::PartCopy;d.parts={copy,copy};s.deliveries.push_back(d);reload(s);next(s);
    std::vector<Id> copies;for(const auto& p:s.parts)if(p.creator=="controlled-committed-copy"){need(p.upgradeDamage==1&&markers(p)==0,"A received copy retained Jig or removed ordinary tag");copies.push_back(p.id);}need(copies.size()==2&&part(s,original).upgradeDamage==3&&markers(part(s,original))==1,"Copying mutated original attribution");
    Part child=part(s,copies[0]);child.origin=PartOrigin::Copied;child.creator="controlled-descendant";d.id=s.nextId++;d.source=child.creator;d.dueRound=s.round+1;d.parts={child};s.deliveries.push_back(d);reload(s);next(s);
    for(const auto& p:s.parts)if(p.creator==child.creator){need(p.upgradeDamage==1&&markers(p)==0,"Descendant subtracted unrelated bonus a second time");copies.push_back(p.id);}need(copies.size()==3,"Descendant required");copies.insert(copies.begin(),original);need(fire(s,copies)==46,"Original13 plus three ordinary committed11 copies");
});
test("C06 actual pre-fix saves load without inventing attribution; known historical Jig excess remains",[&]{
    need(argc==2,"Provide directory containing separately emitted historical jig/mayor saves");
    for(const auto* name:{"jig.ofcore","mayor.ofcore"}){std::ifstream file(std::string(argv[1])+"/"+name,std::ios::binary);need(static_cast<bool>(file),"Missing historical state");const std::string bytes((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());State s;std::string error;need(deserialize(bytes,s,error),error);need(serialize(s)==bytes,"New reader rewrote historical aggregate state");std::vector<Id> ids;for(const auto& p:s.parts){need(markers(p)==0&&p.upgradeDamage==2,"Historical representation unexpectedly attributed");ids.push_back(p.id);}need(ids.size()==2&&fire(s,ids)==16,"Unmarked state was heuristically reinterpreted");
        std::cout<<"OBS LEGACY "<<name<<" unmarked original/copy remain16; "<<(std::string(name)=="jig.ofcore"?"fresh corrected Jig pair is14; migration held":"ordinary Mayor pair correctly remains16")<<'\n';
    }
});
std::cout<<"SUMMARY "<<passed<<" passed, "<<failed<<" failed, "<<checks<<" assertions; C06 documents an unresolved legacy compatibility limit\n";return failed?1:0;
}

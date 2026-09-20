#include "overkill/campaign.hpp"
#include "overkill/upgrades.hpp"
#include "overkill/robots.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace overkill;
namespace {
struct Case {
    const char* id;
    const char* recipe;
    Kind kind;
    Rarity rarity;
    Materials cost, basis;
    Amount count, reference;
};
const std::vector<Case> cases={
#include "parts/cases.inc"
};
int assertions=0, passed=0, failed=0;
void check(bool value,const std::string& why){++assertions;if(!value)throw std::runtime_error(why);}
Result act(const Rules& rules,State& state,const Action& action){auto result=rules.apply(state,action);check(result.ok,result.reason);return result;}
void reject(const Rules& rules,State& state,const Action& action){const auto before=serialize(state);auto result=rules.apply(state,action);check(!result.ok,"Expected rejected action");check(result.events.empty()&&serialize(state)==before,"Rejected action changed state or emitted effects");}
void snapshot(State& state){const auto before=serialize(state);State saved;std::string error;check(deserialize(before,saved,error),error);check(serialize(saved)==before,"Canonical physical snapshot changed bytes");state=std::move(saved);}
const Part& part(const State& state,Id id){const auto it=std::find_if(state.parts.begin(),state.parts.end(),[id](const Part& p){return p.id==id;});check(it!=state.parts.end(),"Expected physical part missing");return *it;}
bool exists(const State& state,Id id){return std::any_of(state.parts.begin(),state.parts.end(),[id](const Part& p){return p.id==id;});}
Id memory(State& state,const std::string& recipe){const auto id=state.nextId++;state.memory.push_back({id,recipe,0,0,0});return id;}
Id granted(const Rules& rules,State& state,const std::string& recipe){const auto before=state.nextId;auto result=rules.grantPart(state,recipe);check(result.ok,result.reason);for(const auto& p:state.parts)if(p.id>=before&&p.recipe==recipe)return p.id;throw std::runtime_error("No granted part");}
State arena(const Rules& rules){
    State state;state.rng=Rng::seeded(8,"part-lifecycle");state.encounter="part-lifecycle";state.phase=Phase::Preparation;
    state.hotBarrel=false;state.hp=60;state.heat=8;state.materials={100,100,100,100,100};
    for(int i=0;i<3;++i){Enemy e;e.id=state.nextId++;e.definition="TEST";e.name="Target";e.hp=e.maxHp=1000;e.burn=i==0?4:2;e.corrosion=i==0?2:0;e.weaken=i==0?3:0;e.intent={Move::Attack,10,1};e.pattern={e.intent};state.enemies.push_back(e);}
    const auto result=rules.grantPlainPart(state,Kind::Shield,80,"test-protection",true);check(result.ok,result.reason);return state;
}
Action creationOptions(const std::string& recipe){Action a;if(recipe=="SH080"||recipe=="SH119")a.choices={2,3};if(recipe=="SH122")a.choices={2,3};if(recipe=="SH106"||recipe=="SH108"||recipe=="SH121")a.choices={2};return a;}
std::vector<Id> produce(const Rules& rules,State& state,const Case& row){
    const Id copy=memory(state,row.recipe);const auto before=state.materials;
    auto craft=creationOptions(row.recipe);craft.type=ActionType::Craft;craft.subject=copy;
    const auto result=act(rules,state,craft);std::vector<Id> ids;
    for(const auto& p:state.parts)if(p.sourceRecipeCopy==copy&&p.origin==PartOrigin::Produced)ids.push_back(p.id);
    check(ids.size()==static_cast<std::size_t>(row.count),"Wrong physical output count");
    check(std::set<Id>(ids.begin(),ids.end()).size()==ids.size(),"Batch outputs reused a physical instance ID");
    for(std::size_t i=0;i<5;++i)check(before[i]-state.materials[i]==row.cost[i],"Paid Use did not pay authored material vector");
    check(std::count_if(result.events.begin(),result.events.end(),[&](const Event& e){return e.type=="recipe_used"&&e.subject==copy&&e.source==row.recipe&&e.paid==row.cost;})==1,"One recipe Use must identify actual full batch payment");
    for(Id id:ids)check(std::any_of(result.events.begin(),result.events.end(),[&](const Event& e){return e.type=="part_created"&&e.subject==id&&e.target==copy&&e.source==row.recipe;}),"Materialization event lacks physical/copy/source identity");
    return ids;
}
void identity(const Part& p,const Case& row){
    check(p.recipe==row.recipe&&p.canonicalRecipe==row.recipe,"Recipe-backed canonical identity changed");
    check(p.kind==row.kind&&p.rarity==row.rarity,"Kind or producer rarity changed");
    check(p.materialBasis==row.basis,"Resale basis differs from authored one-part reference");
    check(p.resaleReference==(std::string(row.id)=="warm-rivet"?"warm-rivet":row.recipe),"Immutable resale reference changed");
}
bool equalEffects(const Part& a,const Part& b){
    if(a.effects.size()!=b.effects.size())return false;
    for(std::size_t i=0;i<a.effects.size();++i){const auto& x=a.effects[i];const auto& y=b.effects[i];if(x.op!=y.op||x.timing!=y.timing||x.amount!=y.amount||x.threshold!=y.threshold)return false;}
    return true;
}
void next(const Rules& rules,State& state){act(rules,state,Action::endTurn());check(state.phase==Phase::Collection,"Controlled lifecycle arena ended unexpectedly");act(rules,state,Action::collect(0));}
Action fireAction(const State& state,Id id){auto a=Action::fire(state.enemies[0].id);a.partTargets={{id,state.enemies[1].id}};return a;}
void fire(const Rules& rules,State& state,Id id){act(rules,state,Action::load({id}));act(rules,state,fireAction(state,id));}
void install(const Rules& rules,State& state,Id id,const Case& row){
    auto a=Action::install(id);a.target=state.enemies[0].id;
    if(std::string(row.recipe)=="MA022")a.sacrifices={granted(rules,state,"SH001")};
    if(std::string(row.recipe)=="MA083")a.amount=8;
    if(std::string(row.recipe)=="MA096")a.amount=6;
    act(rules,state,a);
}
Amount price(const Materials& basis,const Materials& prices){std::int64_t sum=0;for(std::size_t i=0;i<5;++i)sum+=static_cast<std::int64_t>(basis[i])*prices[i];return static_cast<Amount>(sum/2);}
void valuation(const State& state,Id id,const Case& row){
    for(const auto& prices:std::vector<Materials>{{4,4,4,5,6},{1,3,4,7,11},{0,0,0,0,1}})
        check(upgradePartSaleValue(state,part(state,id),prices)==price(row.basis,prices),"Value must floor current one-part prices, never the paid batch cost");
}
void materializeAndUse(const Rules& rules,const Case& row){
    auto state=arena(rules);const auto ids=produce(rules,state,row);const Id id=ids.front();
    for(Id physical:ids){const auto& p=part(state,physical);identity(p,row);check(p.place==Place::Reserve&&!p.everInstalled&&p.createdRound==1&&p.creator==row.recipe,"Fresh production has stale placement or provenance");valuation(state,physical,row);}
    const auto usedRound=state.memory.back().usedRound;const auto cooldown=state.memory.back().cooldown;const auto resources=state.materials;
    auto grantState=state;const auto gift=granted(rules,grantState,row.recipe);identity(part(grantState,gift),row);
    check(part(grantState,gift).origin==PartOrigin::Granted&&part(grantState,gift).sourceRecipeCopy==0,"Free grant counted as paid production");
    check(grantState.materials==resources&&grantState.memory.back().usedRound==usedRound&&grantState.memory.back().cooldown==cooldown,"Grant paid or refreshed a recipe");
    snapshot(state);
    if(row.kind==Kind::Ammo){
        act(rules,state,Action::load({id}));check(upgradePartSaleValue(state,part(state,id),{4,4,4,5,6})==-1,"Loaded Ammo is saleable");
        Action copy=Action::craft(memory(state,"SH118"));copy.parts={id};reject(rules,state,copy);
        Action unload;unload.type=ActionType::Unload;act(rules,state,unload);identity(part(state,id),row);valuation(state,id,row);
        const auto hp=state.enemies[0].hp;fire(rules,state,id);check(!exists(state,id),"Fire retained the consumed physical source");
        check(hp-state.enemies[0].hp==row.reference,"Fresh source-derived damage reference changed");
        if(row.count==2){check(exists(state,ids[1])&&isUnusedPart(part(state,ids[1])),"Firing one Warm Rivet consumed its batch sibling");check(state.heat==9,"One Warm Rivet must grant exactly one postimpact Heat");}
    }else{
        install(rules,state,id,row);check(part(state,id).shield==row.reference,"Fresh source-derived installed Shield changed");
        const auto first=part(state,id);identity(first,row);check(upgradePartSaleValue(state,first,{4,4,4,5,6})==-1,"Installed Shield is saleable");
        if(std::string(row.recipe)=="SH002"){
            auto untouched=state;act(rules,untouched,Action::remove(id));check(isUnusedPart(part(untouched,id)),"Untouched pure Shield became used merely by installation");valuation(untouched,id,row);
            auto copy=Action::craft(memory(untouched,"SH118"));copy.parts={id};act(rules,untouched,copy);next(rules,untouched);
            check(part(untouched,id).shield==6&&part(untouched,id).place==Place::Reserve,"Stored untouched pure Shield lost its original balance");
            check(std::count_if(untouched.parts.begin(),untouched.parts.end(),[](const Part& p){return p.creator=="SH118"&&p.recipe=="SH002";})==2,"Untouched removed pure Shield did not permit two fresh copies");
        }
        Rules::spendShield(state,Rules::shield(state,true),true);check(part(state,id).shield==0,"Shield allocation left paid balance on this part");
        act(rules,state,Action::remove(id));check(!isUnusedPart(part(state,id)),"Depleted/used Shield became unused on removal");
        check(upgradePartSaleValue(state,part(state,id),{4,4,4,5,6})==-1,"Used Shield is saleable");
        Action copy=Action::craft(memory(state,"SH118"));copy.parts={id};reject(rules,state,copy);
        const auto hp=state.hp,heat=state.heat;const auto deliveries=state.deliveries.size();snapshot(state);install(rules,state,id,row);
        check(part(state,id).shield==0&&state.hp==hp&&state.heat==heat&&state.deliveries.size()==deliveries,"Reinstall refilled, repaid or repeated a first-install delivery");
        check(part(state,id).bindingOrder==first.bindingOrder&&part(state,id).installOrder>first.installOrder,"Binding priority renewed on reinstall");identity(part(state,id),row);
        next(rules,state);check(!exists(state,id),"Used installed source survived ordinary enemy-phase reset");
    }
    state=arena(rules);const auto saved=produce(rules,state,row).front();const auto original=part(state,saved);
    if(row.kind==Kind::Ammo)act(rules,state,Action::load({saved}));
    next(rules,state);snapshot(state);identity(part(state,saved),row);valuation(state,saved,row);
    check(part(state,saved).place==Place::Reserve&&part(state,saved).createdRound==original.createdRound&&isUnusedPart(part(state,saved)),"Unused reserve/load-unloaded source did not survive with its original age");
}
void printedCopies(const Rules& rules,const Case& row){
    for(const auto* copier:{"SH071","SH102","SH118"}){
        auto state=arena(rules);
        if(row.kind==Kind::Shield){auto result=rules.acquireUpgrade(state,"UGS-140");check(result.ok,result.reason);result=rules.startUpgrades(state,EncounterClass::Regular);check(result.ok,result.reason);act(rules,state,Action::collect(0));}
        const auto id=produce(rules,state,row).front();
        if(row.kind==Kind::Ammo){const auto fuse=granted(rules,state,"SH036");Action fit;fit.type=ActionType::Activate;fit.subject=fuse;fit.parts={id};act(rules,state,fit);check(part(state,id).attachments.size()==1,"Real Fuse attachment missing");}
        else check(part(state,id).upgradeShield==4,"Paid original did not receive actual Laminator bonus");
        const auto original=part(state,id);valuation(state,id,row);
        const bool common=row.rarity==Rarity::Common;
        const bool allowed=std::string(copier)=="SH118"?row.rarity<=Rarity::Rare:(common&&(row.kind==Kind::Ammo||std::string(copier)=="SH102"));
        auto action=Action::craft(memory(state,copier));action.parts={id};
        if(!allowed){reject(rules,state,action);continue;}
        const auto before=state.nextId;act(rules,state,action);
        check(std::none_of(state.parts.begin(),state.parts.end(),[before](const Part& p){return p.id>=before;}),"Printed copy arrived before next turn");
        snapshot(state);next(rules,state);std::vector<Id> clones;
        for(const auto& p:state.parts)if(p.origin==PartOrigin::Copied&&p.creator==copier)clones.push_back(p.id);
        check(clones.size()==(std::string(copier)=="SH071"?1U:2U),"Wrong next-turn printed copy count");
        check(std::set<Id>(clones.begin(),clones.end()).size()==clones.size(),"Printed copies reused a physical instance ID");
        for(Id clone:clones){const auto& p=part(state,clone);identity(p,row);check(p.id!=id&&p.sourceRecipeCopy==original.sourceRecipeCopy,"Copy lost source-copy identity");
            check(p.createdRound==2&&p.place==Place::Reserve&&!p.everInstalled&&p.bindingOrder==0&&p.installOrder==0&&p.paidHeat==0&&p.paidHp==0,"Printed copy retained old installation/age/payment history");
            check(p.upgradeDamage==0&&p.upgradeShield==0&&p.attachments.empty()&&equalEffects(p,original),"Printed copy kept a later bonus or lost original effects");valuation(state,clone,row);}
        check(part(state,id).attachments.size()==original.attachments.size()&&part(state,id).upgradeShield==original.upgradeShield,"Copying stripped the original's earned property");
    }
}
void committedCopy(const Rules& rules,const Case& row){
    if(row.kind!=Kind::Ammo)return;
    auto state=arena(rules);auto acquired=rules.acquireUpgrade(state,"UGS-015");check(acquired.ok,acquired.reason);auto started=rules.startUpgrades(state,EncounterClass::Regular);check(started.ok,started.reason);act(rules,state,Action::collect(0));
    const auto mould=granted(rules,state,"SH103");Action fit;fit.type=ActionType::Activate;fit.subject=mould;act(rules,state,fit);
    const auto ids=produce(rules,state,row);const auto original=part(state,ids.front());check(original.attachments.size()==1&&original.attachments[0].damage==8,"Fine Mould did not alter first committed output");
    std::vector<Id> copied;for(const auto& p:state.parts)if(p.origin==PartOrigin::Copied)copied.push_back(p.id);
    check(copied.size()==1,"Trial copy recursed or omitted its one committed copy");const auto& clone=part(state,copied.front());identity(clone,row);
    check(clone.creator=="UGS-015"&&clone.sourceRecipeCopy==original.sourceRecipeCopy&&clone.attachments.size()==1&&clone.attachments[0].damage==8&&equalEffects(clone,original),"Trial copy lost committed effects/provenance");
    valuation(state,clone.id,row);snapshot(state);
}
void returnDelivery(const Rules& rules,const Case& row){
    if(row.kind!=Kind::Ammo)return;
    auto state=arena(rules);const auto id=produce(rules,state,row).front();
    const auto fuse=granted(rules,state,"SH036");Action attach;attach.type=ActionType::Activate;attach.subject=fuse;attach.parts={id};act(rules,state,attach);
    const auto original=part(state,id);const auto catchPart=granted(rules,state,"SH072");Action activate;activate.type=ActionType::Activate;activate.subject=catchPart;act(rules,state,activate);
    act(rules,state,Action::load({id}));auto shot=fireAction(state,id);shot.partChoices={{0,id}};
    if(row.rarity>Rarity::Common){reject(rules,state,shot);return;}
    const auto fired=act(rules,state,shot);check(!exists(state,id),"Return Delivery retained the consumed original");
    check(std::none_of(state.parts.begin(),state.parts.end(),[](const Part& p){return p.creator=="SH072";}),"Return Delivery arrived in the firing turn");
    const auto scheduled=std::find_if(state.deliveries.begin(),state.deliveries.end(),[](const Delivery& d){return d.source=="SH072";});
    check(scheduled!=state.deliveries.end()&&scheduled->kind==DeliveryKind::PartCopy&&scheduled->dueRound==2&&scheduled->parts.size()==1,"Return Delivery scheduled other than one next-turn physical copy");
    check(std::count_if(fired.events.begin(),fired.events.end(),[&](const Event& e){return e.type=="delivery_scheduled"&&e.subject==scheduled->id;})==1,"Return Delivery scheduled-event identity is missing");
    snapshot(state);next(rules,state);std::vector<Id> returned;for(const auto& p:state.parts)if(p.creator=="SH072")returned.push_back(p.id);
    check(returned.size()==1,"Return Delivery produced other than one next-turn part");const auto& clone=part(state,returned.front());identity(clone,row);
    check(clone.origin==PartOrigin::Copied&&clone.sourceRecipeCopy==original.sourceRecipeCopy&&clone.createdRound==2&&clone.place==Place::Reserve&&isUnusedPart(clone),"Return Delivery lost fresh consumed-copy provenance");
    check(clone.attachments.empty()&&clone.upgradeDamage==0&&equalEffects(clone,original),"Return Delivery retained Fuse or lost printed payload");valuation(state,clone.id,row);
    fire(rules,state,clone.id);next(rules,state);check(std::none_of(state.parts.begin(),state.parts.end(),[](const Part& p){return p.creator=="SH072";}),"Return Delivery repeated after its single next-shot trigger");
}
CampaignAction command(const Campaign& c,CampaignActionType type,Id id=0,const std::string& choice={}){CampaignAction a;a.runId=c.runId;a.sequence=c.nextTransaction;a.type=type;a.subject=id;a.choice=choice;return a;}
void campaignAct(const CampaignRules& rules,Campaign& c,const CampaignAction& action){auto result=rules.apply(c,action);check(result.ok,result.reason);}
void combat(const CampaignRules& rules,Campaign& c,const Action& a){auto cmd=command(c,CampaignActionType::Combat);cmd.combat=a;campaignAct(rules,c,cmd);}
Campaign campaignArena(const CampaignRules& rules){
    auto c=rules.newGame(1,"part-lifecycle-campaign");
    // Controlled Mayor option and supplies isolate lifecycle mechanics; no balance claim.
    c.mayorOffers={"MY1-02","MY1-03","MY1-01"};campaignAct(rules,c,command(c,CampaignActionType::ChooseMayor,0,"MY1-02"));
    const auto offers=routeOffers(c.route);const auto found=std::find_if(offers.begin(),offers.end(),[](const RouteOffer& o){return o.kind==EncounterKind::Regular;});
    check(found!=offers.end(),"No controlled Regular encounter");campaignAct(rules,c,command(c,CampaignActionType::EnterOffer,found->id));combat(rules,c,Action::collect(0));
    c.fight.materials={100,100,100,100,100};c.fight.heat=8;return c;
}
void saleAndCleanup(const Rules& fights,const Case& row){
    CampaignRules rules(fights,cinderwallUpgradeHooks(fights));auto c=campaignArena(rules);
    auto ids=produce(fights,c.fight,row);const auto memoryBefore=c.fight.memory.size();const auto money=c.fight.credits;
    const auto sale=command(c,CampaignActionType::SellPart,ids.front());campaignAct(rules,c,sale);
    check(c.fight.credits==money+price(row.basis,{4,4,4,5,6})&&!exists(c.fight,ids.front())&&c.fight.memory.size()==memoryBefore,"Actual sale failed immutable one-part price/removal/memory separation");
    const auto committed=serializeCampaign(c);auto replay=rules.apply(c,sale);check(replay.ok&&replay.replayed&&serializeCampaign(c)==committed,"Sale receipt paid a second time");
    auto again=command(c,CampaignActionType::SellPart,ids.front());auto rejected=rules.apply(c,again);check(!rejected.ok&&serializeCampaign(c)==committed,"Consumed physical ID sold again");
    for(bool defeat:{false,true}){
        c=campaignArena(rules);ids=produce(fights,c.fight,row);const auto sourceCopy=part(c.fight,ids.front()).sourceRecipeCopy;
        if(defeat){c.fight.hp=1;c.fight.enemies[0].mesh=100;}
        while(c.phase==CityPhase::Fight){
            Id target=0;for(const auto& e:c.fight.enemies)if(!e.dead&&!e.escaped){target=e.id;break;}check(target!=0,"Terminal fixture lost its robot");
            const auto before=c.fight.nextId;const auto result=fights.grantPlainPart(c.fight,Kind::Ammo,500,"lifecycle-terminal-fixture");check(result.ok,result.reason);
            Id slug=0;for(const auto& p:c.fight.parts)if(p.id>=before)slug=p.id;
            combat(rules,c,Action::load({slug}));combat(rules,c,Action::fire(target));
        }
        check(c.phase==(defeat?CityPhase::Defeated:CityPhase::Rewards),"Wrong controlled terminal outcome");
        check(c.fight.parts.empty()&&c.fight.bullet.empty()&&c.fight.deliveries.empty()&&c.fight.materials==Materials{},"Fight result retained temporary physical inventory/delivery");
        check(std::any_of(c.fight.memory.begin(),c.fight.memory.end(),[sourceCopy](const RecipeCopy& copy){return copy.id==sourceCopy;}),"Fight-end part cleanup deleted its recipe memory");
        const auto bytes=serializeCampaign(c);Campaign restored;std::string error;check(deserializeCampaign(bytes,restored,error),error);check(serializeCampaign(restored)==bytes,"Terminal cleanup snapshot changed bytes");
    }
}
#include "parts/extra_lifecycle.inl"
}
int main(){
    Rules rules;
    for(const auto& row:cases){try{if(row.kind==Kind::Ammo||row.kind==Kind::Shield){materializeAndUse(rules,row);printedCopies(rules,row);committedCopy(rules,row);returnDelivery(rules,row);}else{planningLifecycle(rules,row);if(row.kind==Kind::Magnet)magnetSemantics(rules,row);else if(row.kind==Kind::Spread)spreadSemantics(rules,row);else modifierSemantics(rules,row);}saleAndCleanup(rules,row);++passed;std::cout<<"PASS part:"<<row.id<<" materialization/copy/resale/place/lifetime\n";}
        catch(const std::exception& e){++failed;std::cerr<<"FAIL part:"<<row.id<<": "<<e.what()<<'\n';}}
    for(const auto& row:plainCases){try{plainSemantics(rules,row);++passed;std::cout<<"PASS part:"<<row.id<<" materialization/copy/resale/place/lifetime\n";}catch(const std::exception& e){++failed;std::cerr<<"FAIL part:"<<row.id<<": "<<e.what()<<'\n';}}
    std::cout<<"SUMMARY "<<passed<<" passed, "<<failed<<" failed, "<<assertions<<" assertions; 0 explicitly untested output types. No full recipe-effect or balance claim.\n";
    return failed?1:0;
}

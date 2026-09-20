#include "city_runner.hpp"
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <set>

namespace overkill::runner {
namespace {
bool alive(const Enemy& e){return e.hp>0&&!e.dead&&!e.escaped;}
template<class T> const T* find(const std::vector<T>& xs,Id id){for(const auto& x:xs)if(x.id==id)return &x;return nullptr;}
double announced(const State& s){double n=0;for(const auto& e:s.enemies)if(alive(e)&&e.bornRound<s.round&&e.intent.move==Move::Attack)n+=std::max(0,e.intent.damage+e.drive-e.weaken)*e.intent.hits;return n+s.burn;}
CampaignAction command(const Campaign& c,CampaignActionType type,Id subject=0,const std::string& choice={}){CampaignAction a;a.type=type;a.runId=c.runId;a.sequence=c.nextTransaction;a.subject=subject;a.choice=choice;return a;}
Decision ready(CampaignAction a){return {true,std::move(a),{}};}
double sumMaterials(const Materials& xs){return std::accumulate(xs.begin(),xs.end(),0.0);}
// Credit necessary, immediately previewed transitions. A boss's new Shield or
// a dead Chassis's released helper does not undo damage to the old body. The
// replacement is fully valued as a living target on the following decision.
double transitionProgress(const State& before,const State& after){double value=0;for(const auto& e:after.enemies){const auto* old=find(before.enemies,e.id);if(old&&!old->bossTransitioned&&e.bossTransitioned)value+=std::max(0,e.shield-old->shield)*0.75;if(!old&&alive(e)&&after.kills>before.kills)value+=e.hp+e.shield*0.75;}return value;}
}
Policy::Policy(const Rules& r,const CampaignRules& c,Options options):rules_(r),campaign_(c),options_(std::move(options)),executionRandom_(options_.seed^0xcb66a4d93e517029ULL){}
double Policy::hpWeight()const{return options_.policy=="defensive"?3.2:2.0;}
double Policy::resourceWeight()const{return options_.policy=="defensive"?0.35:0.12;}
Preview Policy::preview(const State& s,const Action& a){if(decisionPreviews_>=options_.searchBudget)return {{false,"Policy preview budget",{}},s};++decisionPreviews_;++totalPreviews_;return rules_.preview(s,a);}
Amount Policy::precisionResult(){if(options_.precision=="auto")return -1;executionRandom_+=0x9e3779b97f4a7c15ULL;auto x=executionRandom_;x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;x=(x^(x>>27))*0x94d049bb133111ebULL;x^=x>>31;const auto roll=x%100;const auto miss=options_.precision=="learning"?55:options_.precision=="practised"?20:5;const auto perfect=options_.precision=="learning"?10:options_.precision=="practised"?35:70;return roll<static_cast<unsigned>(miss)?0:roll>=static_cast<unsigned>(100-perfect)?2:1;}
double Policy::recipeRank(const State& s,const std::string& id)const{
    const auto* r=rules_.recipe(id);if(!r)return -1000;const double cost=sumMaterials(r->cost);const auto copies=std::count_if(s.memory.begin(),s.memory.end(),[&](const RecipeCopy& c){return c.recipe==id;});
    double value=14+static_cast<int>(r->rarity)*2-cost*0.8-r->cooldown;
    if(r->kind==Kind::Ammo)value+=options_.policy=="aggressive"?8:3;
    if(r->kind==Kind::Shield)value+=options_.policy=="defensive"?8:2;
    if(r->kind==Kind::Utility)value+=3;
    if(id=="SH074"||id=="SH104")value+=25;
    if(id=="SH001"||id=="SH002"||id=="MA001"||id=="MA003")value+=3;
    return value-copies*8;
}
double Policy::upgradeRank(const State& s,const std::string& id)const{
    if(!upgradeEligible(rules_,s,id))return -1000;
    // These are declared policy preferences for visible item IDs, not altered effects.
    const std::set<std::string> recovery{"UGS-013","UGS-014","UGS-064","UGS-065","UGS-070","UGS-071","UGS-074","UGS-097","UGS-106","UGS-120","UGS-129","MY1-02","MY1-05","MY1-M3","MAU-05"};
    const std::set<std::string> damage{"MY1-01","MY1-03","MY1-15","MY1-M1","MY1-M2","UGS-001","UGS-019","UGS-037","UGS-060","UGS-066","UGS-076","UGS-093","UGS-121","UGS-139"};
    const std::set<std::string> defence{"MY1-20","MY2-02","MY3-02","MY3-03","UGS-030","UGS-047","UGS-053","UGS-088","UGS-123","UGS-140","MAU-03"};
    double score=12;if(recovery.count(id))score+=options_.policy=="defensive"?16:10;if(damage.count(id))score+=options_.policy=="aggressive"?17:9;if(defence.count(id))score+=options_.policy=="defensive"?14:6;
    if(id=="UGS-011"||id=="UGS-018"||id=="UGS-126")score-=8; // Printed opening resource drawbacks.
    if(id=="MY1-06"||id=="MY2-03")score+=3;
    return score;
}
Id Policy::exchange(const Campaign& c,const std::string& incoming,Id excluded)const{
    const auto* recipe=rules_.recipe(incoming);Id selected=0;double weakest=std::numeric_limits<double>::max();
    for(const auto& copy:c.fight.memory){if(copy.id==excluded||copy.storage==MemoryKind::Borrowed||(copy.storage==MemoryKind::Utility&&recipe&&recipe->kind!=Kind::Utility))continue;const auto value=recipeRank(c.fight,copy.recipe)+copy.tags.size()*2;if(value<weakest){weakest=value;selected=copy.id;}}
    return selected;
}
std::vector<Action> Policy::choices(const State& s,Action base,const std::string& definition)const{
    std::vector<Id> enemies,parts,ammo,shields;for(const auto& e:s.enemies)if(alive(e))enemies.push_back(e.id);
    for(const auto& p:s.parts)if(p.place==Place::Reserve&&isUnusedPart(p)){parts.push_back(p.id);if(p.kind==Kind::Ammo)ammo.push_back(p.id);if(p.kind==Kind::Shield)shields.push_back(p.id);}
    if(!enemies.empty())base.target=enemies.front();base.targets=enemies;
    if(definition=="SH080"||definition=="SH119"||definition=="SH122")base.choices={0,1};
    if(definition=="SH106"||definition=="SH108"||definition=="SH121")base.choices={0};
    if(definition=="SH069")base.choices={2};
    if(definition=="SH101")base.choices={};
    std::vector<Action> out{base};
    const auto* recipe=rules_.recipe(definition);
    if((base.type==ActionType::Craft&&recipe&&recipe->kind==Kind::Utility)||definition=="MA086")for(Id target:enemies){auto a=base;a.target=target;out.push_back(a);}
    if(definition=="MA105"){out.clear();for(Id from:enemies)for(Id to:enemies)if(from!=to){auto a=base;a.target=from;a.targets={to};out.push_back(a);}}
    const bool variable=definition=="SH085"||definition=="MA045"||definition=="MA061"||definition=="MA083"||definition=="MA096";
    if(base.type!=ActionType::Craft&&variable)for(Amount amount:{1,2,3,4,5,6,8,12,15,s.heat,Rules::shield(s,true)})if(amount>=0){auto a=base;a.amount=amount;out.push_back(a);}
    const std::set<std::string> needsPart{"SH032","SH036","SH065","SH071","SH076","SH102","SH118","MA030","MA052","MA066","MA089"};
    if(needsPart.count(definition))for(Id part:parts){auto a=base;a.parts={part};out.push_back(a);}
    if(definition=="MA052"&&ammo.size()>=2){auto a=base;a.parts={ammo[0],ammo[1]};out.push_back(a);}
    if(definition=="MA052"&&ammo.size()>=3){auto a=base;a.parts={ammo[0],ammo[1],ammo[2]};out.push_back(a);}
    if(definition=="MA022"&&base.type==ActionType::Install)for(Id id:ammo){auto a=base;a.sacrifices={id};out.push_back(a);}
    if(definition=="SH101"&&!parts.empty()){auto a=base;a.parts={parts.front()};a.choices={0};out.push_back(a);if(parts.size()>1){a.parts.push_back(parts[1]);a.choices.push_back(0);out.push_back(a);}}
    if(const auto* u=ownedUpgrade(s,"MY1-09");u&&upgradeCounter(*u,"paid_uses")>=3&&upgradeCounter(*u,"discount")==0){
        const auto* r=rules_.recipe(definition);if(r){const auto old=out;for(auto a:old){Amount remain=2;for(std::size_t i=0;i<5;++i){a.discount[i]=std::min(remain,r->cost[i]);remain-=a.discount[i];}out.push_back(a);}}
    }
    return out;
}
std::vector<Action> Policy::shots(const State& s)const{
    std::vector<Action> out;std::vector<Id> enemies;for(const auto& e:s.enemies)if(alive(e))enemies.push_back(e.id);
    for(Id main:enemies){Action a=Action::fire(main);std::vector<Id> others;for(Id id:enemies)if(id!=main)others.push_back(id);std::size_t next=0;
        for(Id id:s.bullet){const auto* p=find(s.parts,id);if(!p)continue;const auto other=others.empty()?0:others[next++%others.size()];
            if(p->kind==Kind::Spread&&other&&p->recipe!="SH082"&&p->recipe!="SH110"&&p->recipe!="MA111")a.spreadTargets.push_back({id,other});
            if(other&&(p->recipe=="SH017"||p->recipe=="SH042"||p->recipe=="SH087"||p->recipe=="MA012"||p->recipe=="MA077"||p->recipe=="MA091"))a.partTargets.push_back({id,other});
        }
        for(const auto& b:s.bindings)if(b.source=="SH072"&&b.clock==BindingClock::Shot)for(Id id:s.bullet){const auto* p=find(s.parts,id);if(p&&p->kind==Kind::Ammo&&p->rarity<=Rarity::Common){a.partChoices={{0,id}};break;}}
        out.push_back(a);
    }return out;
}
double Policy::simpleValue(const State& s)const{
    if(s.phase==Phase::Defeat)return -1000000;if(s.phase==Phase::Victory)return 100000+s.hp*hpWeight();
    double value=s.hp*hpWeight()+s.maxHp*0.3+s.heat*(options_.policy=="aggressive"?1.0:0.65);
    value+=sumMaterials(s.materials)*resourceWeight();
    // Penalize exposed current damage. Rewarding Shield against surviving
    // attackers instead would make eliminating those attackers look costly.
    const auto shield=Rules::shield(s);value-=std::max<double>(0,announced(s)-shield)*hpWeight()*0.82;
    if(ownedUpgrade(s,"MY3-03")||ownedUpgrade(s,"UGS-123"))value+=std::min<double>(shield,6)*0.25;
    double bonusOpportunity=0;
    for(const auto& e:s.enemies)if(alive(e)){
        const auto healthValue=e.hp+e.shield*0.75;
        value-=healthValue+e.tiles*2.0;
        if(e.bornRound<s.round)value-=e.intent.move==Move::Attack?std::max(0,e.intent.damage+e.drive-e.weaken)*e.intent.hits*0.45:1;
        // Pending damage helps remove this body's remaining HP. Surplus Mark,
        // Burn and Corrosion must not make preserving a nearly dead enemy worth
        // more than killing it. Weaken is already included in current threat.
        // Mark and shot bonuses cannot add damage through a remaining tile:
        // the next direct hit spends those bonuses while stripping the tile.
        const auto pending=std::min(e.hp*0.9,e.burn*0.65+e.corrosion*0.75+(e.tiles>0?0:e.mark*0.4));
        value+=pending;
        if(e.tiles==0)bonusOpportunity=std::max(bonusOpportunity,healthValue*0.9-pending);
    }
    value-=s.burn*1.2+s.corrosion*1.5+s.weaken*0.4+s.mark*0.6;
    for(const auto& c:s.memory)value-=std::min(c.cooldown,4)*0.12;
    double bonusValue=0;for(const auto& b:s.shotBonuses)bonusValue+=b.flat*0.5+b.percent*0.025;
    // Stored statuses and a stored shot bonus share the same damage opportunity;
    // valuing them independently can make consuming both on a kill look costly.
    value+=std::min(bonusValue,std::max(0.0,bonusOpportunity));
    for(const auto& d:s.deliveries){if(d.kind==DeliveryKind::Material)value+=sumMaterials(d.materials)*0.6;else if(d.kind==DeliveryKind::ShieldPart)value+=d.amount*0.22;else if(d.kind==DeliveryKind::Heat)value+=d.amount*0.3;else if(d.kind==DeliveryKind::PartCopy)value+=d.parts.size()*2;}
    for(const auto& b:s.bindings)if(b.clock==BindingClock::Collection)value+=1.4+std::max(0,b.amount)*0.3;
    return value;
}
double Policy::partPotential(const State& s,Id firstNewPart){
    if(!s.bullet.empty())return 0;double total=0;const auto before=simpleValue(s);const auto incoming=announced(s);
    for(const auto& p:s.parts){if(p.place!=Place::Reserve||p.id<firstNewPart)continue;double best=0;
        if(p.kind==Kind::Ammo||p.kind==Kind::Spread){auto load=Action::load({p.id});if(p.recipe=="SH110")for(const auto& q:s.parts)if(q.place==Place::Reserve&&q.kind==Kind::Shield&&isUnusedPart(q)){load.sacrifices={q.id};break;}auto loaded=preview(s,load);if(loaded.result.ok)for(const auto& fire:shots(loaded.state)){const auto result=preview(loaded.state,fire);if(result.result.ok)best=std::max(best,std::min(100.0,simpleValue(result.state)-before+transitionProgress(s,result.state)));}}
        else if(p.kind==Kind::Shield){for(const auto& a:choices(s,Action::install(p.id),p.recipe)){const auto result=preview(s,a);if(result.result.ok){const auto gained=std::max(0,Rules::shield(result.state)-Rules::shield(s));best=std::max(best,simpleValue(result.state)-before+std::max(0.0,gained-incoming)*0.08);}}}
        else{Action activate;activate.type=ActionType::Activate;activate.subject=p.id;for(const auto& a:choices(s,activate,p.recipe)){const auto result=preview(s,a);if(result.result.ok)best=std::max(best,simpleValue(result.state)-before+transitionProgress(s,result.state));}}
        total+=std::max(0.0,best)*0.9;
    }return total;
}
Decision Policy::nested(const Campaign& c){
    if(!c.fight.upgradeChoices.empty()){const auto& q=c.fight.upgradeChoices.front();auto a=command(c,CampaignActionType::ResolveUpgradeChoice);a.combat.type=ActionType::ResolveUpgradeChoice;auto& answer=a.combat.upgradeChoice;answer.choice=q.id;
        if(q.kind==UpgradeChoiceKind::Material){answer.values={q.values.front()};if(std::find(q.values.begin(),q.values.end(),0)!=q.values.end())answer.values={0};}
        else if(q.kind==UpgradeChoiceKind::Pack)answer.option=options_.policy=="aggressive"?"assault":"defence";
        else if(q.kind==UpgradeChoiceKind::Enemy){Id best=q.objects.front();for(Id id:q.objects){const auto* e=find(c.fight.enemies,id);const auto* old=find(c.fight.enemies,best);if(e&&old&&e->hp>old->hp)best=id;}answer.objects={best};}
        else if(q.kind==UpgradeChoiceKind::PrecisionRetry){const auto result=precisionResult();if(result<0)answer.decline=true;else answer.values={result};}
        else if(q.kind==UpgradeChoiceKind::HaulExchange)answer.decline=true;
        else {auto copies=q.objects;std::stable_sort(copies.begin(),copies.end(),[&](Id x,Id y){const auto* aCopy=find(c.fight.memory,x);const auto* bCopy=find(c.fight.memory,y);return aCopy&&bCopy&&recipeRank(c.fight,aCopy->recipe)>recipeRank(c.fight,bCopy->recipe);});
            if(q.source=="UGS-039"){for(auto kind:{Kind::Ammo,Kind::Shield})for(Id id:copies){const auto* copy=find(c.fight.memory,id);if(copy&&rules_.recipe(copy->recipe)->kind==kind){answer.objects.push_back(id);break;}}}
            else{const auto count=q.minimum==0?std::min<std::size_t>(copies.size(),static_cast<std::size_t>(q.maximum)):static_cast<std::size_t>(q.minimum);for(std::size_t i=0;i<count&&i<copies.size();++i)answer.objects.push_back(copies[i]);}
        }return ready(a);
    }
    const auto current=std::find_if(c.upgradeOffers.begin(),c.upgradeOffers.end(),[](const CampaignOffer& o){return !o.deferred;});if(current==c.upgradeOffers.end())return {};
    const auto& offer=*current;auto a=command(c,CampaignActionType::ResolveUpgradeOffer,offer.id);
    if(offer.kind==UpgradeRequestKind::CopyRecipe||offer.kind==UpgradeRequestKind::Subscription){if(offer.copies.empty())return {false,{},"No offered recipe copies for nested decision"};Id best=offer.copies.front();for(Id id:offer.copies){const auto* copy=find(c.fight.memory,id);const auto* old=find(c.fight.memory,best);if(copy&&old&&recipeRank(c.fight,copy->recipe)>recipeRank(c.fight,old->recipe))best=id;}a.target=best;const auto* copy=find(c.fight.memory,best);if(!copy)return {false,{},"Selected nested copy missing"};if(offer.kind==UpgradeRequestKind::Subscription){const auto* r=rules_.recipe(copy->recipe);a.material=static_cast<Amount>(std::max_element(r->cost.begin(),r->cost.end())-r->cost.begin());}else if(!campaign_.hasFreeMemory(c,copy->recipe))a.exchange=exchange(c,copy->recipe,best);}
    else {if(offer.index<0||static_cast<std::size_t>(offer.index)>=offer.candidates.size()||offer.candidates[static_cast<std::size_t>(offer.index)].empty())return {false,{},"Nested offer has no candidates"};const auto& candidates=offer.candidates[static_cast<std::size_t>(offer.index)];a.choice=candidates.front();for(const auto& id:candidates){const auto score=offer.kind==UpgradeRequestKind::UpgradeOffer?upgradeRank(c.fight,id):recipeRank(c.fight,id);const auto old=offer.kind==UpgradeRequestKind::UpgradeOffer?upgradeRank(c.fight,a.choice):recipeRank(c.fight,a.choice);if(score>old)a.choice=id;}if(offer.kind==UpgradeRequestKind::RecipeOffer&&!campaign_.hasFreeMemory(c,a.choice,offer.storage))a.exchange=exchange(c,a.choice);}
    return ready(a);
}
Decision Policy::shop(const Campaign& c,bool eventStock){
    const auto* selected=selectedRouteOffer(c.route);const auto shopKey=eventStock?"event/"+std::to_string(selected?selected->id:0):"regular/"+std::to_string(c.shopGeneration);
    if(inspectedShops_.insert(shopKey).second){auto a=command(c,CampaignActionType::OpenShop);a.eventShop=eventStock;return ready(a);}
    if(!c.cores.empty())return ready(command(c,CampaignActionType::SellCore,c.cores.front().id));
    const auto& stock=eventStock?c.eventShop:c.shop;const Product* best=nullptr;double bestScore=0;
    for(const auto& p:stock){if(p.quantity<=0)continue;const auto price=upgradeShopPrice(c.fight,static_cast<PurchaseKind>(p.kind),p.price);if(price>c.fight.credits)continue;double score=-1000;
        if(p.kind==ProductKind::Recipe){const bool owns=std::any_of(c.fight.memory.begin(),c.fight.memory.end(),[&](const RecipeCopy& x){return x.recipe==p.definition;});if(owns)continue;score=recipeRank(c.fight,p.definition)-price*0.22;if(!campaign_.hasFreeMemory(c,p.definition))score-=8;}
        if(p.kind==ProductKind::Upgrade)score=upgradeRank(c.fight,p.definition)-price*0.18;
        if(score>bestScore){bestScore=score;best=&p;}}
    if(!best)return {};auto a=command(c,CampaignActionType::Buy,best->id);a.eventShop=eventStock;if(best->kind==ProductKind::Recipe&&!campaign_.hasFreeMemory(c,best->definition))a.exchange=exchange(c,best->definition);return ready(a);
}
Decision Policy::combat(const Campaign& c){
    const auto& s=c.fight;auto submit=[&](Action a){auto out=command(c,CampaignActionType::Combat);out.combat=std::move(a);return ready(out);};
    if(s.phase==Phase::Collection){Materials demand{};for(const auto& copy:s.memory){const auto* r=rules_.recipe(copy.recipe);if(r&&copy.cooldown==0&&(r->kind==Kind::Ammo||r->kind==Kind::Shield||r->id=="MA001"))for(std::size_t i=0;i<5;++i)demand[i]+=r->cost[i];}Amount steering=0;double best=-1;for(Amount i=0;i<5;++i){const auto score=demand[static_cast<std::size_t>(i)]-s.materials[static_cast<std::size_t>(i)]-(i==0?3:i==1?2:1);if(score>best){best=score;steering=i;}}return submit(Action::collect(steering,s.precisionSpent?-1:precisionResult()));}
    const auto base=simpleValue(s);if(!s.bullet.empty()){double best=-std::numeric_limits<double>::max();Action selected;bool found=false;for(const auto& a:shots(s)){const auto result=preview(s,a);if(result.result.ok&&simpleValue(result.state)>best){best=simpleValue(result.state);selected=a;found=true;}}if(found)return submit(selected);return submit(Action{ActionType::Unload});}
    Action bestAction=Action::endTurn();double best=0.03;
    // Value each candidate's new physical outputs once. Revaluing the whole
    // reserve for every candidate spent the search budget on old Shields and
    // compared late candidates against an incompletely evaluated baseline.
    const auto consider=[&](const Action& a,bool production){const auto result=preview(s,a);if(!result.result.ok)return;double score=simpleValue(result.state)-base+transitionProgress(s,result.state);if(production)score+=partPotential(result.state,s.nextId)*(options_.policy=="aggressive"?1.35:1.15);if(score>best){best=score;bestAction=a;}};
    for(const auto& p:s.parts)if(p.place==Place::Reserve){if(p.kind==Kind::Shield)for(const auto& a:choices(s,Action::install(p.id),p.recipe))consider(a,false);else if(p.kind==Kind::Modifier||p.kind==Kind::Magnet){Action activation;activation.type=ActionType::Activate;activation.subject=p.id;for(const auto& a:choices(s,activation,p.recipe))consider(a,false);}}
    if(ownedUpgrade(s,"MY1-10"))for(const auto& copy:s.memory)if(copy.cooldown>0){Action a;a.type=ActionType::ActivateUpgrade;a.upgrade="MY1-10";a.subject=copy.id;consider(a,false);}

    // Multi-part bullet candidates include every available part; search bounds
    // limit policy enumeration, never inventory size or a legal game action.
    std::vector<Id> ammunition,spreads,shieldPayments;for(const auto& p:s.parts)if(p.place==Place::Reserve){if(p.kind==Kind::Ammo)ammunition.push_back(p.id);if(p.kind==Kind::Spread)spreads.push_back(p.id);if(p.kind==Kind::Shield&&isUnusedPart(p))shieldPayments.push_back(p.id);}
    std::vector<std::vector<Id>> assemblies;if(!ammunition.empty()){assemblies.push_back(ammunition);auto all=ammunition;all.insert(all.end(),spreads.begin(),spreads.end());assemblies.push_back(all);for(Id id:ammunition)assemblies.push_back({id});for(std::size_t n=2;n<ammunition.size();++n)assemblies.emplace_back(ammunition.begin(),ammunition.begin()+static_cast<std::ptrdiff_t>(n));}
    else for(Id id:spreads)assemblies.push_back({id});
    for(const auto& assembly:assemblies){auto a=Action::load(assembly);std::size_t payment=0;for(Id id:assembly){const auto* part=find(s.parts,id);if(part&&part->recipe=="SH110"&&payment<shieldPayments.size())a.sacrifices.push_back(shieldPayments[payment++]);}const auto loaded=preview(s,a);if(!loaded.result.ok)continue;for(const auto& fire:shots(loaded.state)){const auto result=preview(loaded.state,fire);if(!result.result.ok)continue;const auto score=simpleValue(result.state)-base+transitionProgress(s,result.state)-assembly.size()*0.05;if(score>best){best=score;bestAction=a;}}}
    for(const auto& copy:s.memory)if(copy.cooldown==0){const auto* r=rules_.recipe(copy.recipe);if(!r)continue;for(const auto& a:choices(s,Action::craft(copy.id),copy.recipe))consider(a,true);}
    return submit(bestAction);
}
Decision Policy::choose(const Campaign& c){decisionPreviews_=0;if(!c.fight.upgradeChoices.empty()||std::any_of(c.upgradeOffers.begin(),c.upgradeOffers.end(),[](const CampaignOffer& o){return !o.deferred;}))return nested(c);
    if(c.phase==CityPhase::Arrival){if(c.mayorOffers.empty())return {false,{},"No Mayor offer"};auto id=c.mayorOffers.front();for(const auto& candidate:c.mayorOffers)if(upgradeRank(c.fight,candidate)>upgradeRank(c.fight,id))id=candidate;return ready(command(c,CampaignActionType::ChooseMayor,0,id));}
    if(c.phase==CityPhase::Fight)return combat(c);
    if(c.phase==CityPhase::Between){if(auto action=shop(c);action.available)return action;const auto& offered=routeOffers(c.route);if(offered.empty())return {false,{},"No route options"};const RouteOffer* best=&offered.front();double bestScore=-1000;for(const auto& o:offered){double score=o.kind==EncounterKind::Boss?100:o.kind==EncounterKind::Officer?(options_.policy=="aggressive"?30:-10):o.kind==EncounterKind::Mystery?(options_.policy=="defensive"?25:5):15;const auto revealed=campaign_.revealedMysteryCategory(c,o.id);if(revealed=="combat"&&options_.policy=="defensive")score-=15;if(score>bestScore){bestScore=score;best=&o;}}return ready(command(c,CampaignActionType::EnterOffer,best->id));}
    if(c.phase==CityPhase::Rewards){for(const auto& reward:c.rewards)if(reward.claim==ClaimState::Pending){if(reward.kind==RewardKind::Recipe&&c.recipeWindow!=reward.id)return ready(command(c,CampaignActionType::OpenRecipes,reward.id));auto a=command(c,CampaignActionType::ClaimReward,reward.id);if(reward.kind==RewardKind::Recipe||reward.kind==RewardKind::Upgrade){if(reward.choices.empty())continue;a.choice=reward.choices.front();for(const auto& id:reward.choices)if((reward.kind==RewardKind::Recipe?recipeRank(c.fight,id):upgradeRank(c.fight,id))>(reward.kind==RewardKind::Recipe?recipeRank(c.fight,a.choice):upgradeRank(c.fight,a.choice)))a.choice=id;if(reward.kind==RewardKind::Recipe&&!campaign_.hasFreeMemory(c,a.choice))a.exchange=exchange(c,a.choice);}return ready(a);}return ready(command(c,c.skipConfirmation?CampaignActionType::ConfirmAdvance:CampaignActionType::RequestAdvance));}
    if(c.phase==CityPhase::Mystery){const auto* selected=selectedRouteOffer(c.route);if(!selected)return {false,{},"Missing selected Mystery"};if(selected->mystery=="C1-M-PATROL")return ready(command(c,CampaignActionType::EnterPatrol));if(selected->mystery=="C1-M-EXCHANGE")if(auto a=shop(c,true);a.available)return a;if(selected->mystery=="C1-M-TECH"&&!c.rewards.empty()&&c.fight.hp>24){const auto& choices=c.rewards.front().choices;if(!choices.empty()){auto id=choices.front();for(const auto& candidate:choices)if(upgradeRank(c.fight,candidate)>upgradeRank(c.fight,id))id=candidate;if(upgradeRank(c.fight,id)>15)return ready(command(c,CampaignActionType::AcceptCalibration,0,id));}}return ready(command(c,CampaignActionType::LeaveMystery));}
    return {false,{},"Terminal campaign"};
}
} // namespace overkill::runner

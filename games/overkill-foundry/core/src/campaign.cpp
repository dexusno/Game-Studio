#include "overkill/campaign.hpp"
#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace overkill {
namespace {
void need(bool condition,const std::string& message){if(!condition)throw std::runtime_error(message);}
Amount plus(Amount a,Amount b){const auto value=static_cast<std::int64_t>(a)+b;need(value>=0 && value<=std::numeric_limits<Amount>::max(),"Campaign quantity exceeds integer range.");return static_cast<Amount>(value);}
bool has(const std::vector<std::string>& v,const std::string& x){return std::find(v.begin(),v.end(),x)!=v.end();}
void remember(std::vector<std::string>& v,const std::string& x){if(!has(v,x)){v.push_back(x);std::sort(v.begin(),v.end());}}
template<class T> T* find(std::vector<T>& v,Id id){const auto it=std::find_if(v.begin(),v.end(),[&](const T& x){return x.id==id;});return it==v.end()?nullptr:&*it;}
bool ownsRecipe(const Campaign& c,const std::string& id){return std::any_of(c.fight.memory.begin(),c.fight.memory.end(),[&](const RecipeCopy& r){return r.recipe==id;});}
void accepted(const Result& result){need(result.ok,result.reason.empty()?"Effect integration failed.":result.reason);}
bool pending(const Campaign& c){return std::any_of(c.rewards.begin(),c.rewards.end(),[](const RewardEntry& r){return r.claim==ClaimState::Pending;});}
void canShop(const Campaign& c){need(c.phase==CityPhase::Between || c.phase==CityPhase::Rewards || c.phase==CityPhase::Mystery || c.phase==CityPhase::Fight,"The shop is not available in this phase.");if(c.phase==CityPhase::Fight)need(c.fight.phase==Phase::Collection || c.fight.phase==Phase::Preparation,"The fight result must commit before shopping.");}
std::string commandKey(const CampaignAction& a){
    std::ostringstream out;out<<std::quoted(a.runId)<<' '<<static_cast<int>(a.type)<<' '<<a.subject<<' '<<a.exchange<<' '<<std::quoted(a.choice)<<' '<<a.quantity<<' '<<a.eventShop;
    if(a.type==CampaignActionType::Combat){
        const auto& b=a.combat;out<<' '<<static_cast<int>(b.type)<<' '<<b.subject<<' '<<b.target<<' '<<b.steering<<' '<<b.precision<<' '<<b.amount;
        auto ids=[&](const auto& values){out<<' '<<values.size();for(const auto& value:values)out<<' '<<value;};
        auto pairs=[&](const auto& values){out<<' '<<values.size();for(const auto& value:values)out<<' '<<value.part<<' '<<value.enemy;};
        ids(b.parts);pairs(b.spreadTargets);ids(b.choices);ids(b.targets);ids(b.sacrifices);pairs(b.partTargets);pairs(b.partChoices);ids(b.discarded);
    }
    return out.str();
}
void clearFight(Campaign& c){
    State clean;clean.hp=c.fight.hp;clean.maxHp=c.fight.maxHp;clean.credits=c.fight.credits;clean.memory=c.fight.memory;
    clean.nextId=c.fight.nextId;clean.nextOrder=c.fight.nextOrder;clean.nextEvent=c.fight.nextEvent;
    clean.seed=c.seed;clean.rng=Rng::seeded(c.seed,"Mara/cinderwall/between");
    for(auto& copy:clean.memory){copy.cooldown=0;copy.usedRound=0;copy.usesThisRound=0;}
    if(clean.hp==0)clean.phase=Phase::Defeat;
    c.fight=std::move(clean);c.entry.clear();
}
void finishRewards(Campaign& c,bool abandon){
    if(abandon)for(auto& r:c.rewards)if(r.claim==ClaimState::Pending)r.claim=ClaimState::Abandoned;
    need(!pending(c),"Unclaimed rewards need confirmation before leaving.");
    c.recipeWindow=0;c.pendingRecipeChoice.clear();c.skipConfirmation=false;c.phase=c.route.position==13?CityPhase::Complete:CityPhase::Between;
}
void pricePayment(Campaign& c,Amount price,Amount quantity){
    need(price>=0 && quantity>0,"Invalid price or quantity.");const auto total=static_cast<std::int64_t>(price)*quantity;
    need(total<=c.fight.credits,"Not enough Credits.");c.fight.credits-=static_cast<Amount>(total);
}
} // namespace
CampaignRules::CampaignRules(const Rules& fights,CampaignHooks hooks,CityContent content):fights_(fights),hooks_(std::move(hooks)),content_(std::move(content)){}
Campaign CampaignRules::newGame(std::uint64_t seed,const std::string& runId,const ProfileFacts& profile) const {
    need(!runId.empty(),"A stable new campaign identity is required.");
    Campaign c;c.seed=seed;c.runId=runId;c.manifestHash=content_.manifestHash;c.profile=profile;remember(c.profile.unlocked,"Mara");
    c.rng=Rng::seeded(seed,"Mara/cinderwall/campaign");c.route=makeCinderwallRoute(seed);c.memorySlots=content_.memorySlots;
    c.fight.seed=seed;c.fight.rng=Rng::seeded(seed,"Mara/cinderwall/arrival");c.fight.credits=content_.startingCredits;
    for(const auto& id:content_.starters){need(fights_.recipe(id)!=nullptr,"Missing starter runtime: "+id);c.fight.memory.push_back({c.fight.nextId++,id,0,0,0});remember(c.profile.recipes,id);}
    for(const auto& lane:{"infrastructure","tactical","mercenary"}){
        const auto offer=drawCityOffer(content_.upgrades,content_.mayorPools,lane,c.rng,Domain::Choice,1);
        need(offer.size()==1,"Mayor lane is empty.");c.mayorOffers.push_back(offer.front());
    }
    restock(c);return c;
}
void CampaignRules::restock(Campaign& c) const {
    c.shop.clear();c.shopGeneration=plus(c.shopGeneration,1);
    for(Amount i=0;i<5;++i)c.shop.push_back({c.nextId++,ProductKind::Material,{},i,content_.materialStock[static_cast<std::size_t>(i)],content_.materialPrices[static_cast<std::size_t>(i)]});
    std::vector<std::string> recipes;
    if(!ownsRecipe(c,"SH074"))recipes.push_back("SH074");
    auto random=drawCityOffer(content_.recipes,content_.recipePools,"Shop",c.rng,Domain::Shop,content_.recipeSlots-static_cast<Amount>(recipes.size()),recipes);
    recipes.insert(recipes.end(),random.begin(),random.end());
    for(const auto& id:recipes){const auto* item=cityItem(content_.recipes,id);need(item!=nullptr,"Unknown shop recipe.");c.shop.push_back({c.nextId++,ProductKind::Recipe,id,0,1,item->price});}
    for(const auto& id:drawCityOffer(content_.upgrades,content_.upgradePools,"Shop",c.rng,Domain::Shop,content_.upgradeSlots,c.everAcquired)){
        const auto* item=cityItem(content_.upgrades,id);c.shop.push_back({c.nextId++,ProductKind::Upgrade,id,0,1,item->price});
    }
    // Fixed initial beta products: printed main-recipe material value + 2.
    for(const auto& id:{"SH001","SH002"}){
        const auto* recipe=fights_.recipe(id);need(recipe!=nullptr,"Missing ready-part definition.");Amount price=2;
        for(std::size_t i=0;i<5;++i)price=plus(price,recipe->cost[i]*content_.materialPrices[i]);
        c.shop.push_back({c.nextId++,ProductKind::Part,id,0,2,price});
    }
}
void CampaignRules::acquireRecipe(Campaign& c,const std::string& recipe,Id exchange) const {
    need(cityItem(content_.recipes,recipe)!=nullptr && fights_.recipe(recipe)!=nullptr,"This recipe has no runtime implementation.");
    if(static_cast<Amount>(c.fight.memory.size())>=c.memorySlots){
        const auto old=std::find_if(c.fight.memory.begin(),c.fight.memory.end(),[&](const RecipeCopy& x){return x.id==exchange;});
        need(old!=c.fight.memory.end(),"Select an owned recipe copy to exchange, or cancel.");c.fight.memory.erase(old);
    }else need(exchange==0,"An exchange is only available when memory is full.");
    c.fight.memory.push_back({c.fight.nextId++,recipe,0,0,0});remember(c.profile.recipes,recipe);
}
void CampaignRules::acquireUpgrade(Campaign& c,const std::string& upgrade,const CampaignAction& action) const {
    need(cityItem(content_.upgrades,upgrade)!=nullptr,"Unknown upgrade.");need(!has(c.everAcquired,upgrade),"This upgrade was already acquired in this campaign.");
    need(static_cast<bool>(hooks_.acquireUpgrade),"Permanent-upgrade effect integration is not ready.");
    c.upgrades.push_back({upgrade,c.fight.nextOrder++,0,0,0,0,0,{}});remember(c.everAcquired,upgrade);
    accepted(hooks_.acquireUpgrade(c,upgrade,action));
    need(c.fight.hp>0,"Acquisition costs must leave at least 1 HP.");
}
void CampaignRules::finishCombat(Campaign& c) const {
    need(c.phase==CityPhase::Fight,"No active fight.");
    need(c.fight.phase==Phase::Victory || c.fight.phase==Phase::Defeat || c.fight.phase==Phase::Escaped,"Fight has not finished.");
    if(c.fight.phase==Phase::Defeat){clearFight(c);c.phase=CityPhase::Defeated;c.rewards.clear();return;}
    const auto* active=selectedRouteOffer(c.route);need(active!=nullptr,"Missing committed encounter.");const auto offer=*active;
    if(!c.upgrades.empty())need(static_cast<bool>(hooks_.combatCompleted),"Upgrade result hooks are not integrated.");
    if(hooks_.combatCompleted)accepted(hooks_.combatCompleted(c));
    need(c.fight.hp>0,"A post-combat effect cannot revive a terminal defeat.");
    c.rewards.clear();c.recipeWindow=0;c.pendingRecipeChoice.clear();c.skipConfirmation=false;
    if(c.fight.phase==Phase::Victory){
        RewardEntry cores;cores.id=c.nextId++;cores.kind=RewardKind::Cores;
        for(const auto& enemy:c.fight.enemies)if(enemy.dead && !enemy.escaped){
            const auto value=std::find_if(content_.coreValues.begin(),content_.coreValues.end(),[&](const CoreValue& x){return x.robot==enemy.definition;});
            need(value!=content_.coreValues.end(),"Missing destroyed robot's core value.");cores.cores.push_back({c.nextId++,enemy.definition,value->credits});
        }
        if(!cores.cores.empty())c.rewards.push_back(std::move(cores));
        const std::string source=offer.kind==EncounterKind::Boss?"Boss":offer.kind==EncounterKind::Officer?"Officer":"Regular";
        auto choices=drawCityOffer(content_.recipes,content_.recipePools,source,c.rng,Domain::Reward,3);
        if(!choices.empty())c.rewards.push_back({c.nextId++,RewardKind::Recipe,ClaimState::Pending,std::move(choices),{},0});
        if(offer.kind==EncounterKind::Officer){
            auto upgrades=drawCityOffer(content_.upgrades,content_.upgradePools,"Officer",c.rng,Domain::Reward,3,c.everAcquired);
            if(upgrades.empty())c.rewards.push_back({c.nextId++,RewardKind::Credits,ClaimState::Pending,{},{},30});
            else c.rewards.push_back({c.nextId++,RewardKind::Upgrade,ClaimState::Pending,std::move(upgrades),{},0});
        }
        if(offer.kind==EncounterKind::Boss){remember(c.profile.unlocked,"Ivo");remember(c.profile.cityClearReceipts,c.runId+"/cinderwall/"+std::to_string(offer.id));}
    }
    std::string error;need(completeRouteNode(c.route,error),error);clearFight(c);
    if(c.route.position<=12)restock(c);
    c.phase=CityPhase::Rewards;
}
CampaignResult CampaignRules::apply(Campaign& original,const CampaignAction& action) const {
    try{
        need(original.rulesVersion==RulesVersion && original.contentVersion==ContentVersion && original.manifestHash==content_.manifestHash,"Incompatible campaign content; an explicit migration is required.");
        need(!original.preStart,"Internal fight-entry snapshots must be initialized before gameplay.");
        need(!action.runId.empty() && action.runId==original.runId,"This command belongs to another campaign; refresh the active run.");
        const auto key=commandKey(action);
        for(const auto& receipt:original.receipts)if(receipt.sequence==action.sequence){need(receipt.command==key,"Transaction identity was reused for a different action.");return {true,true,receipt.message,receipt.events};}
        need(action.sequence==original.nextTransaction && action.sequence!=0,"Campaign changed; reload before another transaction.");
        Campaign c=original;std::vector<Event> events;std::string message="Committed.";bool captureEntry=false,initialize=false;
        switch(action.type){
        case CampaignActionType::ChooseMayor:
            need(c.phase==CityPhase::Arrival && has(c.mayorOffers,action.choice),"Choose one of the saved Mayor offers.");
            acquireUpgrade(c,action.choice,action);c.mayor=action.choice;c.phase=CityPhase::Between;break;
        case CampaignActionType::EnterOffer:{
            need(c.phase==CityPhase::Between,"Finish the current encounter or rewards first.");std::string error;need(selectRouteOffer(c.route,action.subject,error),error);
            const auto* offer=selectedRouteOffer(c.route);need(offer!=nullptr,"Missing chosen encounter.");
            if(offer->kind==EncounterKind::Mystery){
                c.phase=CityPhase::Mystery;c.eventShop.clear();c.rewards.clear();
                if(offer->mystery=="C1-M-EXCHANGE"){
                    for(Amount rarity:{2,3}){
                        auto pools=content_.recipePools;for(auto& pool:pools)if(pool.name=="Shop"){pool.weights={};pool.weights[static_cast<std::size_t>(rarity)]=1;}
                        const auto ids=drawCityOffer(content_.recipes,pools,"Shop",c.rng,Domain::Choice,1);
                        if(!ids.empty()){const auto* item=cityItem(content_.recipes,ids.front());c.eventShop.push_back({c.nextId++,ProductKind::Recipe,item->id,0,1,item->price});}
                    }
                    auto pools=content_.upgradePools;for(auto& pool:pools)if(pool.name=="Shop")pool.weights={0,0,70,30,0};
                    const auto ids=drawCityOffer(content_.upgrades,pools,"Shop",c.rng,Domain::Choice,1,c.everAcquired);
                    if(!ids.empty()){const auto* item=cityItem(content_.upgrades,ids.front());c.eventShop.push_back({c.nextId++,ProductKind::Upgrade,item->id,0,1,item->price});}
                }else if(offer->mystery=="C1-M-TECH"){
                    auto ids=drawCityOffer(content_.upgrades,content_.upgradePools,"Mystery",c.rng,Domain::Choice,3,c.everAcquired);
                    if(!ids.empty())c.rewards.push_back({c.nextId++,RewardKind::Upgrade,ClaimState::Pending,std::move(ids),{},0});
                }
            }else{c.phase=CityPhase::Fight;captureEntry=true;initialize=true;}
            break;}
        case CampaignActionType::EnterPatrol:{
            const auto* offer=selectedRouteOffer(c.route);need(c.phase==CityPhase::Mystery && offer && offer->mystery=="C1-M-PATROL","No patrol awaits entry.");
            c.phase=CityPhase::Fight;captureEntry=true;initialize=true;break;}
        case CampaignActionType::Combat:{
            need(c.phase==CityPhase::Fight,"No active fight.");const auto result=fights_.apply(c.fight,action.combat);accepted(result);events=result.events;
            if(c.fight.phase==Phase::Victory || c.fight.phase==Phase::Defeat || c.fight.phase==Phase::Escaped)finishCombat(c);
            break;}
        case CampaignActionType::OpenRecipes:{
            need(c.phase==CityPhase::Rewards,"No victory rewards are open.");auto* reward=find(c.rewards,action.subject);
            need(reward && reward->kind==RewardKind::Recipe && reward->claim==ClaimState::Pending,"This recipe reward is unavailable.");
            c.recipeWindow=reward->id;c.pendingRecipeChoice.clear();c.skipConfirmation=false;for(const auto& id:reward->choices)remember(c.profile.recipes,id);break;}
        case CampaignActionType::BackRecipes:
            need(c.phase==CityPhase::Rewards && c.recipeWindow!=0,"No recipe window is open.");c.recipeWindow=0;c.pendingRecipeChoice.clear();break;
        case CampaignActionType::ClaimReward:{
            need(c.phase==CityPhase::Rewards,"No victory rewards are open.");auto* reward=find(c.rewards,action.subject);
            need(reward && reward->claim==ClaimState::Pending,"This reward has already been handled.");
            if(reward->kind==RewardKind::Cores)c.cores.insert(c.cores.end(),reward->cores.begin(),reward->cores.end());
            else if(reward->kind==RewardKind::Credits)c.fight.credits=plus(c.fight.credits,reward->credits);
            else {
                need(has(reward->choices,action.choice),"Choose one of this reward's saved candidates.");
                if(reward->kind==RewardKind::Recipe){
                    need(c.recipeWindow==reward->id,"Open the recipe reward first.");
                    need(c.pendingRecipeChoice.empty() || c.pendingRecipeChoice==action.choice,"Cancel the pending exchange before choosing another recipe.");
                    if(static_cast<Amount>(c.fight.memory.size())>=c.memorySlots && action.exchange==0){c.pendingRecipeChoice=action.choice;message="Select a memory copy to exchange, or cancel.";break;}
                    acquireRecipe(c,action.choice,action.exchange);c.recipeWindow=0;c.pendingRecipeChoice.clear();
                }
                else acquireUpgrade(c,action.choice,action);
            }
            // Acquisition hooks may append other rewards, invalidating pointers.
            find(c.rewards,action.subject)->claim=ClaimState::Claimed;c.skipConfirmation=false;break;}
        case CampaignActionType::RequestAdvance:
            need(c.phase==CityPhase::Rewards,"No rewards await completion.");c.recipeWindow=0;c.pendingRecipeChoice.clear();
            if(pending(c)){c.skipConfirmation=true;message="Confirm abandonment of the remaining rewards.";}else finishRewards(c,false);break;
        case CampaignActionType::CancelAdvance:
            need(c.phase==CityPhase::Rewards && c.skipConfirmation,"No abandonment confirmation is open.");c.skipConfirmation=false;break;
        case CampaignActionType::ConfirmAdvance:
            need(c.phase==CityPhase::Rewards && c.skipConfirmation,"Request abandonment before confirming it.");finishRewards(c,true);break;
        case CampaignActionType::OpenShop:{
            canShop(c);const auto& stock=action.eventShop?c.eventShop:c.shop;
            if(action.eventShop){const auto* offer=selectedRouteOffer(c.route);need(c.phase==CityPhase::Mystery && offer && offer->mystery=="C1-M-EXCHANGE","No event merchant is open.");}
            for(const auto& item:stock)if(item.kind==ProductKind::Recipe && item.quantity>0)remember(c.profile.recipes,item.definition);break;}
        case CampaignActionType::Buy:{
            canShop(c);if(action.eventShop){const auto* offer=selectedRouteOffer(c.route);need(c.phase==CityPhase::Mystery && offer && offer->mystery=="C1-M-EXCHANGE","No event merchant is open.");}
            auto& stock=action.eventShop?c.eventShop:c.shop;auto* item=find(stock,action.subject);need(item && action.quantity>0 && item->quantity>=action.quantity,"Selected shop stock is unavailable.");
            const auto product=*item;const auto price=hooks_.productPrice?hooks_.productPrice(c,product):product.price;
            pricePayment(c,price,action.quantity);
            if(product.kind==ProductKind::Material){need(product.material>=0 && product.material<5,"Invalid material.");auto& n=c.fight.materials[static_cast<std::size_t>(product.material)];n=plus(n,action.quantity);}
            else if(product.kind==ProductKind::Recipe){need(action.quantity==1,"Recipe slots contain one copy.");acquireRecipe(c,product.definition,action.exchange);}
            else if(product.kind==ProductKind::Upgrade){need(action.quantity==1,"Upgrade slots contain one item.");acquireUpgrade(c,product.definition,action);}
            else {need(static_cast<bool>(hooks_.grantPart),"Ready-part creation is not integrated.");for(Amount i=0;i<action.quantity;++i)accepted(hooks_.grantPart(c,product.definition));}
            auto& updatedStock=action.eventShop?c.eventShop:c.shop;auto* updated=find(updatedStock,action.subject);need(updated!=nullptr,"Acquisition replaced its own shop transaction.");updated->quantity-=action.quantity;break;}
        case CampaignActionType::SellCore:{
            canShop(c);const auto* core=find(c.cores,action.subject);need(core!=nullptr,"This core is not owned.");const auto income=hooks_.coreSaleValue?hooks_.coreSaleValue(c,*core):core->baseValue;
            need(income>=0,"Invalid core sale value.");c.fight.credits=plus(c.fight.credits,income);c.cores.erase(std::remove_if(c.cores.begin(),c.cores.end(),[&](const EnergyCore& x){return x.id==action.subject;}),c.cores.end());break;}
        case CampaignActionType::SellPart:{
            canShop(c);const auto* part=find(c.fight.parts,action.subject);need(part && part->place==Place::Reserve,"Only an eligible reserve part can be sold.");
            need(static_cast<bool>(hooks_.partSaleValue),"Canonical part-sale valuation is not integrated.");const auto income=hooks_.partSaleValue(c,*part);need(income>=0,"This part cannot be sold.");
            c.fight.credits=plus(c.fight.credits,income);c.fight.parts.erase(std::remove_if(c.fight.parts.begin(),c.fight.parts.end(),[&](const Part& x){return x.id==action.subject;}),c.fight.parts.end());break;}
        case CampaignActionType::AcceptCalibration:
        case CampaignActionType::LeaveMystery:{
            const auto* offer=selectedRouteOffer(c.route);need(c.phase==CityPhase::Mystery && offer && offer->mystery!="C1-M-PATROL","No noncombat Mystery is open.");
            if(action.type==CampaignActionType::AcceptCalibration){
                need(offer->mystery=="C1-M-TECH" && c.rewards.size()==1 && has(c.rewards.front().choices,action.choice),"Select an offered calibration.");
                need(c.fight.hp>8,"Calibration must leave at least 1 HP after all costs.");c.fight.hp-=8;acquireUpgrade(c,action.choice,action);
            }
            if(!c.upgrades.empty())need(static_cast<bool>(hooks_.noncombatCompleted),"Mystery upgrade hooks are not integrated.");
            if(hooks_.noncombatCompleted)accepted(hooks_.noncombatCompleted(c));else c.fight.credits=plus(c.fight.credits,10);
            std::string error;need(completeRouteNode(c.route,error),error);c.rewards.clear();c.eventShop.clear();c.phase=CityPhase::Between;break;}
        case CampaignActionType::Continue:{
            need(c.phase==CityPhase::Fight && !c.entry.empty(),"No unfinished fight checkpoint is available.");
            Campaign checkpoint;std::string error;need(deserializeCampaign(c.entry,checkpoint,error),error);
            need(checkpoint.phase==CityPhase::Fight && checkpoint.preStart && checkpoint.entry.empty() && checkpoint.runId==c.runId,"Invalid original fight-entry snapshot.");
            const auto profile=c.profile;const auto entry=c.entry;const auto sequence=c.nextTransaction;
            c=std::move(checkpoint);c.profile=profile;c.entry=entry;c.nextTransaction=sequence;c.preStart=false;initialize=true;break;}
        default:throw std::runtime_error("Unsupported campaign action.");
        }
        need(c.nextTransaction<std::numeric_limits<Id>::max(),"Campaign transaction identity exhausted.");
        c.receipts.push_back({action.sequence,key,message,events});++c.nextTransaction;
        if(captureEntry){need(c.entry.empty(),"A previous fight checkpoint was not closed.");c.preStart=true;c.entry=serializeCampaign(c);c.preStart=false;}
        if(initialize){
            const auto* offer=selectedRouteOffer(c.route);need(offer && !offer->formation.empty(),"Missing committed combat formation.");
            need(static_cast<bool>(hooks_.initializeFight),"Encounter initialization is not integrated.");
            c.fight.encounter=offer->encounterKey;c.fight.seed=offer->encounterSeed;c.fight.rng=Rng::seeded(offer->encounterSeed,offer->encounterKey);
            c.fight.phase=Phase::Collection;accepted(hooks_.initializeFight(c,*offer));
        }
        original=std::move(c);return {true,false,message,std::move(events)};
    }catch(const std::exception& e){return {false,false,e.what(),{}};}
}
} // namespace overkill

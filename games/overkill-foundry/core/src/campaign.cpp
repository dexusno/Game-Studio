#include "overkill/campaign.hpp"
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace overkill {
namespace {
void need(bool condition,const std::string& message){if(!condition)throw std::runtime_error(message);}
Amount plus(Amount a,Amount b){const auto value=static_cast<std::int64_t>(a)+b;need(value>=0 && value<=std::numeric_limits<Amount>::max(),"Campaign quantity exceeds integer range.");return static_cast<Amount>(value);}
Amount times(Amount a,Amount b){const auto value=static_cast<std::int64_t>(a)*b;need(a>=0 && b>=0 && value<=std::numeric_limits<Amount>::max(),"Campaign price exceeds integer range.");return static_cast<Amount>(value);}
bool has(const std::vector<std::string>& v,const std::string& x){return std::find(v.begin(),v.end(),x)!=v.end();}
void remember(std::vector<std::string>& v,const std::string& x){if(!has(v,x)){v.push_back(x);std::sort(v.begin(),v.end());}}
template<class T> T* find(std::vector<T>& v,Id id){const auto it=std::find_if(v.begin(),v.end(),[&](const T& x){return x.id==id;});return it==v.end()?nullptr:&*it;}
bool ownsRecipe(const Campaign& c,const std::string& id){return std::any_of(c.fight.memory.begin(),c.fight.memory.end(),[&](const RecipeCopy& r){return r.recipe==id;});}
void accepted(const Result& result,std::vector<Event>& events){need(result.ok,result.reason.empty()?"Effect integration failed.":result.reason);events.insert(events.end(),result.events.begin(),result.events.end());}
bool pending(const Campaign& c){return std::any_of(c.rewards.begin(),c.rewards.end(),[](const RewardEntry& r){return r.claim==ClaimState::Pending;});}
void canShop(const Campaign& c){need(c.phase==CityPhase::Between || c.phase==CityPhase::Rewards || c.phase==CityPhase::Mystery || c.phase==CityPhase::Fight,"The shop is not available in this phase.");if(c.phase==CityPhase::Fight)need(c.fight.phase==Phase::Collection || c.fight.phase==Phase::Preparation,"The fight result must commit before shopping.");}
std::string commandKey(const CampaignAction& a){
    std::ostringstream out;out<<std::quoted(a.runId)<<' '<<static_cast<int>(a.type)<<' '<<a.subject<<' '<<a.exchange<<' '<<std::quoted(a.choice)<<' '<<a.quantity<<' '<<a.eventShop<<' '<<a.decline<<' '<<a.target<<' '<<a.material;
    if(a.type==CampaignActionType::Combat || a.type==CampaignActionType::ResolveUpgradeChoice || a.type==CampaignActionType::ResolveUpgradeOffer){
        const auto& b=a.combat;out<<' '<<static_cast<int>(b.type)<<' '<<b.subject<<' '<<b.target<<' '<<b.steering<<' '<<b.precision<<' '<<b.amount;
        auto ids=[&](const auto& values){out<<' '<<values.size();for(const auto& value:values)out<<' '<<value;};
        auto pairs=[&](const auto& values){out<<' '<<values.size();for(const auto& value:values)out<<' '<<value.part<<' '<<value.enemy;};
        ids(b.parts);pairs(b.spreadTargets);ids(b.choices);ids(b.targets);ids(b.sacrifices);pairs(b.partTargets);pairs(b.partChoices);ids(b.discarded);
        const auto& choice=b.upgradeChoice;out<<' '<<choice.choice;ids(choice.objects);ids(choice.values);out<<' '<<std::quoted(choice.option);ids(choice.removed);ids(choice.added);out<<' '<<choice.decline;ids(b.discount);out<<' '<<std::quoted(b.upgrade);
    }
    return out.str();
}
void clearFight(Campaign& c){
    State clean;clean.hp=c.fight.hp;clean.maxHp=c.fight.maxHp;clean.credits=c.fight.credits;clean.memory=c.fight.memory;
    clean.nextId=c.fight.nextId;clean.nextOrder=c.fight.nextOrder;clean.nextEvent=c.fight.nextEvent;
    clean.upgrades=c.fight.upgrades;clean.upgradeResolution=c.fight.upgradeResolution;clean.upgradeChoices=c.fight.upgradeChoices;clean.upgradeRequests=c.fight.upgradeRequests;
    clean.fightSerial=c.fight.fightSerial;clean.encounterClass=c.fight.encounterClass;
    clean.precisionGoodFight=c.fight.precisionGoodFight;clean.precisionFailedFight=c.fight.precisionFailedFight;
    clean.memory.erase(std::remove_if(clean.memory.begin(),clean.memory.end(),[](const RecipeCopy& x){return x.storage==MemoryKind::Borrowed;}),clean.memory.end());
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
    for(const auto& id:drawCityOffer(content_.upgrades,content_.upgradePools,"Shop",c.rng,Domain::Shop,content_.upgradeSlots,c.everAcquired,[&](const std::string& id){return upgradeEligible(fights_,c.fight,id);})){
        const auto* item=cityItem(content_.upgrades,id);c.shop.push_back({c.nextId++,ProductKind::Upgrade,id,0,1,item->price});
    }
    // Fixed initial beta products: printed main-recipe material value + 2.
    for(const auto& id:{"SH001","SH002"}){
        const auto* recipe=fights_.recipe(id);need(recipe!=nullptr,"Missing ready-part definition.");Amount price=2;
        for(std::size_t i=0;i<5;++i)price=plus(price,recipe->cost[i]*content_.materialPrices[i]);
        c.shop.push_back({c.nextId++,ProductKind::Part,id,0,2,price});
    }
}
void CampaignRules::acquireUpgrade(Campaign& c,const std::string& upgrade,const CampaignAction& action,std::vector<Event>& events) const {
    need(cityItem(content_.upgrades,upgrade)!=nullptr,"Unknown upgrade.");need(!has(c.everAcquired,upgrade),"This upgrade was already acquired in this campaign.");
    need(static_cast<bool>(hooks_.acquireUpgrade),"Permanent-upgrade effect integration is not ready.");
    remember(c.everAcquired,upgrade);
    accepted(hooks_.acquireUpgrade(c,upgrade,action),events);
    need(ownedUpgrade(c.fight,upgrade)!=nullptr,"Acquisition did not insert its upgrade ledger entry.");
    c.route.safeMysteries=ownedUpgrade(c.fight,"UGS-057")!=nullptr;
    need(c.fight.hp>0,"Acquisition costs must leave at least 1 HP.");
}
void CampaignRules::finishCombat(Campaign& c,std::vector<Event>& events) const {
    need(c.phase==CityPhase::Fight,"No active fight.");
    need(c.fight.phase==Phase::Victory || c.fight.phase==Phase::Defeat || c.fight.phase==Phase::Escaped,"Fight has not finished.");
    if(c.fight.phase==Phase::Defeat){clearFight(c);c.phase=CityPhase::Defeated;c.rewards.clear();return;}
    const auto* active=selectedRouteOffer(c.route);need(active!=nullptr,"Missing committed encounter.");const auto offer=*active;
    if(!c.fight.upgrades.empty())need(static_cast<bool>(hooks_.combatCompleted),"Upgrade result hooks are not integrated.");
    c.rewards.clear();c.rewardAdjustments.clear();c.recipeWindow=0;c.pendingRecipeChoice.clear();c.skipConfirmation=false;
    if(c.fight.phase==Phase::Victory){
        RewardEntry cores;cores.id=c.nextId++;cores.kind=RewardKind::Cores;
        for(const auto& enemy:c.fight.enemies)if(enemy.dead && !enemy.escaped){
            const auto value=std::find_if(content_.coreValues.begin(),content_.coreValues.end(),[&](const CoreValue& x){return x.robot==enemy.definition;});
            need(value!=content_.coreValues.end(),"Missing destroyed robot's core value.");cores.cores.push_back({c.nextId++,enemy.definition,value->credits});
        }
        if(!cores.cores.empty())c.rewards.push_back(std::move(cores));
        const std::string source=offer.kind==EncounterKind::Boss?"Boss":offer.kind==EncounterKind::Officer?"Officer":"Regular";
        auto choices=normalRecipeOffer(c,offer);
        if(!choices.empty()){RewardEntry r;r.id=c.nextId++;r.kind=RewardKind::Recipe;r.choices=std::move(choices);r.pool=source;c.rewards.push_back(std::move(r));}
        if(offer.kind==EncounterKind::Officer){
            auto upgrades=drawCityOffer(content_.upgrades,content_.upgradePools,"Officer",c.rng,Domain::Reward,3,c.everAcquired,[&](const std::string& id){return upgradeEligible(fights_,c.fight,id);});
            if(upgrades.empty())c.rewards.push_back({c.nextId++,RewardKind::Credits,ClaimState::Pending,{},{},30});
            else c.rewards.push_back({c.nextId++,RewardKind::Upgrade,ClaimState::Pending,std::move(upgrades),{},0});
        }
        if(offer.kind==EncounterKind::Boss){remember(c.profile.unlocked,"Ivo");remember(c.profile.cityClearReceipts,c.runId+"/cinderwall/"+std::to_string(offer.id));}
    }
    if(hooks_.combatCompleted)accepted(hooks_.combatCompleted(c),events);
    need(c.fight.hp>0,"A post-combat effect cannot revive a terminal defeat.");
    prepareUpgradeOffers(c,events);
    std::string error;need(completeRouteNode(c.route,error),error);c.routePreviews.clear();clearFight(c);
    c.revealNextReward=false;
    if(c.route.position<=12){restock(c);UpgradeEvent e;e.kind=UpgradeEventKind::ShopRestocked;notifyUpgrade(c,e,events);}
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
        const bool upgradePending=!c.fight.upgradeChoices.empty() || !c.fight.upgradeRequests.empty() || std::any_of(c.upgradeOffers.begin(),c.upgradeOffers.end(),[](const CampaignOffer& o){return !o.deferred;});
        if(upgradePending)need(action.type==CampaignActionType::ResolveUpgradeChoice || action.type==CampaignActionType::ResolveUpgradeOffer || action.type==CampaignActionType::CancelUpgradeExchange || action.type==CampaignActionType::Continue,"Resolve the saved upgrade decision before another action.");
        switch(action.type){
        case CampaignActionType::ChooseMayor:
            need(c.phase==CityPhase::Arrival && has(c.mayorOffers,action.choice),"Choose one of the saved Mayor offers.");
            acquireUpgrade(c,action.choice,action,events);c.mayor=action.choice;c.phase=CityPhase::Between;break;
        case CampaignActionType::EnterOffer:{
            need(c.phase==CityPhase::Between,"Finish the current encounter or rewards first.");std::string error;need(selectRouteOffer(c.route,action.subject,error),error);
            c.routePreviews.clear();
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
                    const auto ids=drawCityOffer(content_.upgrades,pools,"Shop",c.rng,Domain::Choice,1,c.everAcquired,[&](const std::string& id){return upgradeEligible(fights_,c.fight,id);});
                    if(!ids.empty()){const auto* item=cityItem(content_.upgrades,ids.front());c.eventShop.push_back({c.nextId++,ProductKind::Upgrade,item->id,0,1,item->price});}
                }else if(offer->mystery=="C1-M-TECH"){
                    auto ids=drawCityOffer(content_.upgrades,content_.upgradePools,"Mystery",c.rng,Domain::Choice,3,c.everAcquired,[&](const std::string& id){return upgradeEligible(fights_,c.fight,id);});
                    if(!ids.empty())c.rewards.push_back({c.nextId++,RewardKind::Upgrade,ClaimState::Pending,std::move(ids),{},0});
                }
            }else{c.phase=CityPhase::Fight;captureEntry=true;initialize=true;}
            break;}
        case CampaignActionType::EnterPatrol:{
            const auto* offer=selectedRouteOffer(c.route);need(c.phase==CityPhase::Mystery && offer && offer->mystery=="C1-M-PATROL","No patrol awaits entry.");
            c.phase=CityPhase::Fight;captureEntry=true;initialize=true;break;}
        case CampaignActionType::Combat:{
            need(c.phase==CityPhase::Fight,"No active fight.");accepted(fights_.apply(c.fight,action.combat),events);
            if(c.fight.phase==Phase::Victory || c.fight.phase==Phase::Defeat || c.fight.phase==Phase::Escaped)finishCombat(c,events);
            break;}
        case CampaignActionType::ResolveUpgradeChoice:
            need(action.combat.type==ActionType::ResolveUpgradeChoice,"Use an explicit upgrade-choice answer.");
            accepted(fights_.apply(c.fight,action.combat),events);
            if(c.phase==CityPhase::Fight && (c.fight.phase==Phase::Victory || c.fight.phase==Phase::Defeat || c.fight.phase==Phase::Escaped))finishCombat(c,events);
            break;
        case CampaignActionType::ResolveUpgradeOffer:resolveUpgradeOffer(c,action,events);break;
        case CampaignActionType::CancelUpgradeExchange:{
            auto* offer=find(c.upgradeOffers,action.subject);need(offer && !offer->selected.empty(),"No nested memory exchange is open.");offer->selected.clear();break;}
        case CampaignActionType::MoveRecipeCopy:{
            need(c.phase!=CityPhase::Fight && c.phase!=CityPhase::Defeated,"Recipe slots can only be rearranged outside fights.");
            auto* copy=find(c.fight.memory,action.subject);need(copy && copy->storage!=MemoryKind::Borrowed,"Select a permanent recipe copy.");
            need(action.choice=="general" || action.choice=="utility","Choose general or Utility memory.");const auto storage=action.choice=="utility"?MemoryKind::Utility:MemoryKind::General;
            need(copy->storage!=storage,"This copy already occupies that slot type.");const auto* recipe=fights_.recipe(copy->recipe);need(recipe && (storage!=MemoryKind::Utility || recipe->kind==Kind::Utility),"Dedicated memory only accepts Utility recipes.");
            const auto capacity=storage==MemoryKind::Utility?upgradeUtilityMemoryBonus(c.fight):c.memorySlots+upgradeGeneralMemoryBonus(c.fight);
            const auto used=static_cast<Amount>(std::count_if(c.fight.memory.begin(),c.fight.memory.end(),[storage](const RecipeCopy& r){return r.storage==storage;}));need(used<capacity,"No free slot of that type remains.");copy->storage=storage;break;}
        case CampaignActionType::PreviewRouteReplacement:{
            need(c.phase==CityPhase::Between,"Route alternatives are available between encounters.");const auto* pass=ownedUpgrade(c.fight,"MY1-12");need(pass && pass->charges>0,"No Forked Route Pass charge remains.");
            if(!find(c.routePreviews,action.subject)){RouteOffer replacement;std::string error;need(previewRouteReplacement(c.route,action.subject,replacement,error),error);c.routePreviews.push_back(replacement);}break;}
        case CampaignActionType::ReplaceRouteOffer:{
            need(c.phase==CityPhase::Between,"Route alternatives are available between encounters.");auto* pass=ownedUpgrade(c.fight,"MY1-12");need(pass && pass->charges>0,"No Forked Route Pass charge remains.");
            const auto* preview=find(c.routePreviews,action.subject);need(preview!=nullptr,"Preview the saved alternative before committing it.");const auto committed=*preview;std::string error;need(replaceRouteOffer(c.route,committed,error),error);--pass->charges;c.routePreviews={committed};break;}
        case CampaignActionType::RerollRecipeReward:{
            need(c.phase==CityPhase::Rewards,"No victory recipe reward is open.");auto* source=ownedUpgrade(c.fight,"MY1-14");need(source && upgradeCounter(*source,"reward_reroll")==0,"Reward Spectrometer was already used for this victory.");
            auto* reward=find(c.rewards,action.subject);need(reward && reward->kind==RewardKind::Recipe && reward->claim==ClaimState::Pending && c.recipeWindow==reward->id,"Open an unclaimed recipe offer before replacing it.");
            auto pools=content_.recipePools;const auto pool=reward->pool.empty()?"Regular":reward->pool;
            if(reward->fixedRarity>=0)for(auto& p:pools)if(p.name==pool){p.weights={};p.weights[static_cast<std::size_t>(reward->fixedRarity)]=1;}
            const auto replacement=drawCityOffer(content_.recipes,pools,pool,c.rng,Domain::Reward,static_cast<Amount>(reward->choices.size()));need(!replacement.empty(),"No eligible replacement reward exists.");
            for(const auto& id:reward->choices)remember(c.profile.recipes,id);reward->choices=replacement;reward->lightTouch.clear();for(const auto& id:reward->choices)remember(c.profile.recipes,id);
            c.pendingRecipeChoice.clear();bool set=false;for(auto& counter:source->counters)if(counter.key=="reward_reroll"){counter.value=1;set=true;}if(!set)source->counters.push_back({"reward_reroll",UpgradeScope::Fight,1});break;}
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
                    if(!hasFreeMemory(c,action.choice) && action.exchange==0){c.pendingRecipeChoice=action.choice;message="Select a memory copy to exchange, or cancel.";break;}
                    UpgradeEvent e;e.kind=UpgradeEventKind::RecipeAccepted;e.origin=AcquisitionSource::Victory;e.reward=reward->id;e.definition=action.choice;e.normalOffer=reward->normalOffer;e.lightTouch=has(reward->lightTouch,action.choice);
                    e.subject=acquireRecipe(c,action.choice,action.exchange);c.recipeWindow=0;c.pendingRecipeChoice.clear();notifyUpgrade(c,e,events);
                }
                else acquireUpgrade(c,action.choice,action,events);
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
            need(product.kind!=ProductKind::Upgrade || product.definition!="UGS-085" || product.price>100,"Civic Credit Bond requires an undiscounted shop price above 100 Credits.");
            UpgradeEvent purchase;purchase.kind=UpgradeEventKind::Purchase;purchase.product=static_cast<PurchaseKind>(product.kind);purchase.definition=product.definition;purchase.subject=product.id;
            purchase.snapshotListeners=true;auto priorListeners=c.fight.upgrades;std::sort(priorListeners.begin(),priorListeners.end(),[](const OwnedUpgrade& a,const OwnedUpgrade& b){return a.order<b.order;});for(const auto& u:priorListeners)purchase.listeners.push_back(u.id);
            purchase.baseValue=times(product.price,action.quantity);purchase.actualValue=times(price,action.quantity);
            pricePayment(c,price,action.quantity);
            if(product.kind==ProductKind::Material){need(product.material>=0 && product.material<5,"Invalid material.");auto& n=c.fight.materials[static_cast<std::size_t>(product.material)];n=plus(n,action.quantity);}
            else if(product.kind==ProductKind::Recipe){need(action.quantity==1,"Recipe slots contain one copy.");UpgradeEvent e;e.kind=UpgradeEventKind::RecipeAccepted;e.origin=AcquisitionSource::Shop;e.definition=product.definition;e.normalOffer=false;e.subject=acquireRecipe(c,product.definition,action.exchange);notifyUpgrade(c,e,events);}
            else if(product.kind==ProductKind::Upgrade){need(action.quantity==1,"Upgrade slots contain one item.");acquireUpgrade(c,product.definition,action,events);}
            else {need(static_cast<bool>(hooks_.grantPart),"Ready-part creation is not integrated.");for(Amount i=0;i<action.quantity;++i)accepted(hooks_.grantPart(c,product.definition),events);}
            auto& updatedStock=action.eventShop?c.eventShop:c.shop;auto* updated=find(updatedStock,action.subject);need(updated!=nullptr,"Acquisition replaced its own shop transaction.");updated->quantity-=action.quantity;
            notifyUpgrade(c,purchase,events);break;}
        case CampaignActionType::SellCore:{
            canShop(c);const auto* core=find(c.cores,action.subject);need(core!=nullptr,"This core is not owned.");const auto income=hooks_.coreSaleValue?hooks_.coreSaleValue(c,*core):core->baseValue;
            UpgradeEvent e;e.kind=UpgradeEventKind::CoreSold;e.subject=core->id;e.definition=core->robot;e.baseValue=core->baseValue;e.actualValue=income;
            need(income>=0,"Invalid core sale value.");c.fight.credits=plus(c.fight.credits,income);c.cores.erase(std::remove_if(c.cores.begin(),c.cores.end(),[&](const EnergyCore& x){return x.id==action.subject;}),c.cores.end());notifyUpgrade(c,e,events);break;}
        case CampaignActionType::SellPart:{
            canShop(c);const auto* part=find(c.fight.parts,action.subject);need(part && part->place==Place::Reserve,"Only an eligible reserve part can be sold.");
            need(static_cast<bool>(hooks_.partSaleValue),"Canonical part-sale valuation is not integrated.");const auto income=hooks_.partSaleValue(c,*part);need(income>=0,"This part cannot be sold.");
            UpgradeEvent e;e.kind=UpgradeEventKind::PartSold;e.subject=part->id;e.definition=part->recipe;e.baseValue=income;e.actualValue=income;
            c.fight.credits=plus(c.fight.credits,income);c.fight.parts.erase(std::remove_if(c.fight.parts.begin(),c.fight.parts.end(),[&](const Part& x){return x.id==action.subject;}),c.fight.parts.end());notifyUpgrade(c,e,events);break;}
        case CampaignActionType::AcceptCalibration:
        case CampaignActionType::LeaveMystery:{
            const auto* offer=selectedRouteOffer(c.route);need(c.phase==CityPhase::Mystery && offer && offer->mystery!="C1-M-PATROL","No noncombat Mystery is open.");
            if(action.type==CampaignActionType::AcceptCalibration){
                need(offer->mystery=="C1-M-TECH" && c.rewards.size()==1 && has(c.rewards.front().choices,action.choice),"Select an offered calibration.");
                need(c.fight.hp>8,"Calibration must leave at least 1 HP after all costs.");c.fight.hp-=8;acquireUpgrade(c,action.choice,action,events);
            }
            if(!c.fight.upgrades.empty())need(static_cast<bool>(hooks_.noncombatCompleted),"Mystery upgrade hooks are not integrated.");
            if(hooks_.noncombatCompleted)accepted(hooks_.noncombatCompleted(c),events);else c.fight.credits=plus(c.fight.credits,10);
            std::string error;need(completeRouteNode(c.route,error),error);c.routePreviews.clear();c.rewards.clear();c.eventShop.clear();c.phase=CityPhase::Between;break;}
        case CampaignActionType::Continue:{
            need(c.phase==CityPhase::Fight && !c.entry.empty(),"No unfinished fight checkpoint is available.");
            Campaign checkpoint;std::string error;need(deserializeCampaign(c.entry,checkpoint,error),error);
            need(checkpoint.phase==CityPhase::Fight && checkpoint.preStart && checkpoint.entry.empty() && checkpoint.runId==c.runId,"Invalid original fight-entry snapshot.");
            const auto profile=c.profile;const auto entry=c.entry;const auto sequence=c.nextTransaction;
            c=std::move(checkpoint);c.profile=profile;c.entry=entry;c.nextTransaction=sequence;c.preStart=false;initialize=true;break;}
        default:throw std::runtime_error("Unsupported campaign action.");
        }
        prepareUpgradeOffers(c,events);
        if(c.phase==CityPhase::Fight && (c.fight.phase==Phase::Victory || c.fight.phase==Phase::Defeat || c.fight.phase==Phase::Escaped)){finishCombat(c,events);prepareUpgradeOffers(c,events);}
        need(c.nextTransaction<std::numeric_limits<Id>::max(),"Campaign transaction identity exhausted.");
        c.receipts.push_back({action.sequence,key,message,events});++c.nextTransaction;
        if(captureEntry){need(c.entry.empty(),"A previous fight checkpoint was not closed.");c.preStart=true;c.entry=serializeCampaign(c);c.preStart=false;}
        if(initialize){
            const auto* offer=selectedRouteOffer(c.route);need(offer && !offer->formation.empty(),"Missing committed combat formation.");
            need(static_cast<bool>(hooks_.initializeFight),"Encounter initialization is not integrated.");
            c.fight.encounter=offer->encounterKey;c.fight.seed=offer->encounterSeed;c.fight.rng=Rng::seeded(offer->encounterSeed,offer->encounterKey);
            c.fight.phase=Phase::Collection;accepted(hooks_.initializeFight(c,*offer),events);
            prepareUpgradeOffers(c,events);
            if(c.fight.phase==Phase::Victory || c.fight.phase==Phase::Defeat || c.fight.phase==Phase::Escaped)finishCombat(c,events);
        }
        c.receipts.back().events=events;
        original=std::move(c);return {true,false,message,std::move(events)};
    }catch(const std::exception& e){return {false,false,e.what(),{}};}
}
} // namespace overkill

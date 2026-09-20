#include "overkill/campaign.hpp"
#include "overkill/robots.hpp"
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>

namespace overkill {
namespace {
void need(bool condition,const std::string& why){if(!condition)throw std::runtime_error(why);}
void accept(const Result& result,std::vector<Event>& events){need(result.ok,result.reason);events.insert(events.end(),result.events.begin(),result.events.end());}
bool has(const std::vector<std::string>& items,const std::string& id){return std::find(items.begin(),items.end(),id)!=items.end();}
void remember(std::vector<std::string>& items,const std::string& id){if(!has(items,id)){items.push_back(id);std::sort(items.begin(),items.end());}}
void credits(Campaign& c,Amount amount){const auto total=static_cast<std::int64_t>(c.fight.credits)+amount;need(amount>=0 && total<=std::numeric_limits<Amount>::max(),"Credit amount exceeds the integer range.");c.fight.credits=static_cast<Amount>(total);}
template<class T> T* byId(std::vector<T>& values,Id id){for(auto& x:values)if(x.id==id)return &x;return nullptr;}
std::string rewardPool(EncounterClass kind){return kind==EncounterClass::Boss?"Boss":kind==EncounterClass::Officer?"Officer":"Regular";}
std::string rewardPool(const RouteOffer& offer){return offer.kind==EncounterKind::Boss?"Boss":offer.kind==EncounterKind::Officer?"Officer":"Regular";}
std::vector<Id> subscriptionCopies(const Campaign& c){
    std::vector<Id> ids;
    for(const auto& copy:c.fight.memory)if(copy.storage!=MemoryKind::Borrowed && std::none_of(copy.tags.begin(),copy.tags.end(),[](const RecipeTag& t){return t.kind==RecipeTagKind::Subscription;}))ids.push_back(copy.id);
    return ids;
}
Amount usedMemory(const Campaign& c,MemoryKind kind){return static_cast<Amount>(std::count_if(c.fight.memory.begin(),c.fight.memory.end(),[kind](const RecipeCopy& r){return r.storage==kind;}));}
MemoryKind preferredStorage(const Campaign& c,const Recipe& recipe,MemoryKind requested){
    if(requested==MemoryKind::Borrowed)return requested;
    if(recipe.kind==Kind::Utility && usedMemory(c,MemoryKind::Utility)<upgradeUtilityMemoryBonus(c.fight))return MemoryKind::Utility;
    return MemoryKind::General;
}
std::vector<std::string> drawRecipes(const Rules& fights,const CityContent& content,Campaign& c,const UpgradeRequest& request,const std::string& pool,const std::vector<std::string>& excluded={}){
    auto pools=content.recipePools;
    if(request.rarity>=0)for(auto& p:pools)if(p.name==pool){p.weights={};p.weights[static_cast<std::size_t>(request.rarity)]=1;}
    return drawCityOffer(content.recipes,pools,pool,c.rng,Domain::Choice,request.count,excluded,[&](const std::string& id){
        const auto* recipe=fights.recipe(id);return recipe && (!request.sharedOnly || id.substr(0,2)=="SH") && (!request.filterKind || recipe->kind==request.recipeKind || (request.recipeKind==Kind::Modifier&&recipe->kind==Kind::Spread));
    });
}
std::vector<std::string> drawUpgrades(const Rules& fights,const CityContent& content,Campaign& c,const UpgradeRequest& request){
    CityPool pool;pool.name="Nested";pool.weights={0,1,1,1,1};
    for(const auto& ordinary:content.upgradePools)for(const auto& id:ordinary.ids)remember(pool.ids,id);
    if(request.rarity>=0){pool.weights={};pool.weights[static_cast<std::size_t>(request.rarity)]=1;}
    return drawCityOffer(content.upgrades,{pool},pool.name,c.rng,Domain::Choice,request.count,c.everAcquired,[&](const std::string& id){return (!request.sharedOnly || id.substr(0,4)=="UGS-") && upgradeEligible(fights,c.fight,id);});
}
}

CampaignHooks cinderwallUpgradeHooks(const Rules& fights,CityContent content){
    CampaignHooks hooks;
    hooks.acquireUpgrade=[&fights](Campaign& c,const std::string& id,const CampaignAction&){return fights.acquireUpgrade(c.fight,id);};
    hooks.initializeFight=[&fights](Campaign& c,const RouteOffer& offer){
        c.fight.enemies=makeCinderwallFormation(offer.formation,offer.encounterSeed,offer.encounterKey,c.fight.nextId,offer.binderDefaultSeen);
        const auto kind=offer.kind==EncounterKind::Officer?EncounterClass::Officer:offer.kind==EncounterKind::Boss?EncounterClass::Boss:EncounterClass::Regular;
        return fights.startUpgrades(c.fight,kind);
    };
    hooks.grantPart=[&fights](Campaign& c,const std::string& id){return fights.grantPart(c.fight,id);};
    hooks.combatCompleted=[&fights](Campaign& c){
        UpgradeEvent e;e.kind=c.fight.phase==Phase::Victory?UpgradeEventKind::Victory:UpgradeEventKind::Escaped;e.baseValue=c.fight.kills;
        return fights.upgradeEvent(c.fight,e);
    };
    hooks.noncombatCompleted=[&fights](Campaign& c){c.pendingMysteryCredits=10;UpgradeEvent e;e.kind=UpgradeEventKind::Noncombat;return fights.upgradeEvent(c.fight,e);};
    hooks.notifyUpgrade=[&fights](Campaign& c,const UpgradeEvent& e){return fights.upgradeEvent(c.fight,e);};
    hooks.resumeUpgrade=[&fights](Campaign& c){return fights.resumeUpgrades(c.fight);};
    hooks.productPrice=[](const Campaign& c,const Product& p){return upgradeShopPrice(c.fight,static_cast<PurchaseKind>(p.kind),p.price);};
    hooks.coreSaleValue=[](const Campaign& c,const EnergyCore& core){return upgradeCoreSaleValue(c.fight,core.baseValue);};
    hooks.partSaleValue=[prices=content.materialPrices](const Campaign& c,const Part& part){return upgradePartSaleValue(c.fight,part,prices);};
    return hooks;
}

bool CampaignRules::hasFreeMemory(const Campaign& c,const std::string& id,MemoryKind storage) const {
    const auto* recipe=fights_.recipe(id);if(!recipe)return false;
    storage=preferredStorage(c,*recipe,storage);
    if(storage==MemoryKind::Borrowed)return usedMemory(c,storage)==0;
    if(storage==MemoryKind::Utility)return usedMemory(c,storage)<upgradeUtilityMemoryBonus(c.fight);
    return usedMemory(c,storage)<c.memorySlots+upgradeGeneralMemoryBonus(c.fight);
}
Id CampaignRules::acquireRecipe(Campaign& c,const std::string& id,Id exchange,MemoryKind storage,const std::string& borrowedFrom) const {
    const auto* recipe=fights_.recipe(id);need(recipe && cityItem(content_.recipes,id),"This recipe has no runtime implementation.");
    const bool free=hasFreeMemory(c,id,storage);storage=preferredStorage(c,*recipe,storage);
    if(!free){
        need(storage!=MemoryKind::Borrowed,"Only one borrowed recipe slot is available.");
        auto* old=byId(c.fight.memory,exchange);need(old && old->storage!=MemoryKind::Borrowed,"Select an owned permanent copy to exchange.");
        need(old->storage!=MemoryKind::Utility || recipe->kind==Kind::Utility,"This memory slot only accepts Utility recipes.");
        storage=old->storage;
        c.fight.memory.erase(std::remove_if(c.fight.memory.begin(),c.fight.memory.end(),[exchange](const RecipeCopy& x){return x.id==exchange;}),c.fight.memory.end());
    }else need(exchange==0,"An exchange is only available when eligible memory is full.");
    RecipeCopy copy;copy.id=c.fight.nextId++;copy.recipe=id;copy.storage=storage;copy.borrowedFrom=borrowedFrom;
    c.fight.memory.push_back(copy);remember(c.profile.recipes,id);return copy.id;
}
void CampaignRules::notifyUpgrade(Campaign& c,const UpgradeEvent& event,std::vector<Event>& events) const {
    if(hooks_.notifyUpgrade)accept(hooks_.notifyUpgrade(c,event),events);
}
std::vector<std::string> CampaignRules::normalRecipeOffer(const Campaign& c,const RouteOffer& offer) const {
    // A route-specific reward substream lets information effects inspect future
    // candidates without consuming or reserving a reward, including after a
    // noncombat Mystery. Shopping and optional nested offers cannot reroll it.
    auto rng=Rng::seeded(offer.encounterSeed,offer.encounterKey+"/normal-victory-recipes");
    (void)c;
    return drawCityOffer(content_.recipes,content_.recipePools,rewardPool(offer),rng,Domain::Reward,3);
}
std::string CampaignRules::revealedRecipe(const Campaign& c,const RouteOffer& offer) const {
    if(!c.revealNextReward || offer.formation.empty())return {};
    const auto choices=normalRecipeOffer(c,offer);std::vector<std::string> shared;
    for(const auto& id:choices)if(id.substr(0,2)=="SH")shared.push_back(id);
    if(shared.empty())return {};
    auto rng=Rng::seeded(offer.encounterSeed,offer.encounterKey+"/UGS-131");
    return shared[rng.below(Domain::Choice,static_cast<std::uint32_t>(shared.size()))];
}
std::string CampaignRules::revealedMysteryCategory(const Campaign& c,Id id) const {
    if(!ownedUpgrade(c.fight,"UGS-149") || c.phase!=CityPhase::Between)return {};
    Amount shown=0;for(const auto& o:routeOffers(c.route))if(o.kind==EncounterKind::Mystery){
        if(++shown>2)break;if(o.id==id)return o.mystery=="C1-M-PATROL"?"combat":o.mystery=="C1-M-EXCHANGE"?"merchant":"calibration";
    }
    return {};
}

void CampaignRules::prepareUpgradeOffers(Campaign& c,std::vector<Event>& events) const {
    // Earned subscriptions survive an empty eligible-copy set. Only their
    // decision is deferred; they cannot hold the engine's other event frames.
    for(auto& o:c.upgradeOffers)if(o.kind==UpgradeRequestKind::Subscription){
        o.copies=subscriptionCopies(c);o.deferred=o.copies.empty();
        if(o.deferred && o.request){
            need(acknowledgeUpgradeRequest(c.fight,o.request),"The deferred subscription request disappeared.");o.request=0;
            need(static_cast<bool>(hooks_.resumeUpgrade),"Upgrade continuation is not integrated.");accept(hooks_.resumeUpgrade(c),events);
        }
    }
    while(!c.fight.upgradeRequests.empty()){
        const auto request=c.fight.upgradeRequests.front();
        if(std::any_of(c.upgradeOffers.begin(),c.upgradeOffers.end(),[&](const CampaignOffer& o){return o.request==request.id;}))break;
        bool automatic=false;
        if(request.kind==UpgradeRequestKind::AddRewardOption){
            c.rewardAdjustments.push_back(request);
            // Apply to each normal offer, including the double Regular offer.
            for(auto& reward:c.rewards)if(reward.kind==RewardKind::Recipe && reward.normalOffer && reward.claim==ClaimState::Pending){
                const auto choices=drawRecipes(fights_,content_,c,request,reward.pool.empty()?rewardPool(c.fight.encounterClass):reward.pool,reward.choices);
                reward.choices.insert(reward.choices.end(),choices.begin(),choices.end());
            }
            automatic=true;
        }else if(request.kind==UpgradeRequestKind::AddRewardOffer){
            RewardEntry reward;reward.id=c.nextId++;reward.kind=RewardKind::Recipe;reward.normalOffer=request.normalOffer;
            reward.fixedRarity=request.rarity;
            reward.pool=request.source=="UGS-101"?"Regular":rewardPool(c.fight.encounterClass);
            reward.choices=drawRecipes(fights_,content_,c,request,reward.pool);
            // Previously resolved modifiers apply to a later normal offer.
            // Later listeners will visit it when their own request resolves.
            if(reward.normalOffer)for(const auto& extra:c.rewardAdjustments){
                auto choices=drawRecipes(fights_,content_,c,extra,reward.pool,reward.choices);reward.choices.insert(reward.choices.end(),choices.begin(),choices.end());
            }
            if(!reward.choices.empty())c.rewards.push_back(std::move(reward));automatic=true;
        }else if(request.kind==UpgradeRequestKind::RevealReward){c.revealNextReward=true;automatic=true;}
        else {
            CampaignOffer offer;offer.id=c.nextId++;offer.request=request.id;offer.source=request.source;offer.kind=request.kind;offer.optional=request.optional;offer.sourceCopy=request.recipeCopy;
            if(request.kind==UpgradeRequestKind::Subscription){
                offer.copies=subscriptionCopies(c);offer.optional=false;
                if(offer.copies.empty()){offer.deferred=true;offer.request=0;automatic=true;}
            }else if(request.kind==UpgradeRequestKind::CopyRecipe){
                if(request.recipeCopy){const auto* copy=byId(c.fight.memory,request.recipeCopy);need(copy && copy->storage!=MemoryKind::Borrowed,"The copied permanent recipe is unavailable.");offer.copies={copy->id};}
                else for(const auto& copy:c.fight.memory)if(copy.storage!=MemoryKind::Borrowed)offer.copies.push_back(copy.id);
                if(offer.copies.empty())automatic=true;
            }else{
                if(request.source=="UGS-133")offer.storage=MemoryKind::Borrowed;
                for(Amount i=0;i<request.offers;++i){
                    auto choices=request.kind==UpgradeRequestKind::UpgradeOffer?drawUpgrades(fights_,content_,c,request):drawRecipes(fights_,content_,c,request,"Regular");
                    if(!choices.empty())offer.candidates.push_back(std::move(choices));
                }
                if(request.source=="UGS-113"){offer.creditAlternative=c.pendingMysteryCredits;c.pendingMysteryCredits=0;}
                if(offer.candidates.empty()){
                    if(request.source=="UGS-150")credits(c,30);else credits(c,offer.creditAlternative);
                    automatic=true;
                }
            }
            const bool present=offer.deferred || !automatic;
            if(present){
                // All fixed recipe candidates have actually been offered now.
                if(offer.kind==UpgradeRequestKind::RecipeOffer)for(const auto& id:offer.candidates.front())remember(c.profile.recipes,id);
                c.upgradeOffers.push_back(std::move(offer));
            }
        }
        if(!automatic)break;
        need(acknowledgeUpgradeRequest(c.fight,request.id),"The completed request disappeared.");
        need(static_cast<bool>(hooks_.resumeUpgrade),"Upgrade continuation is not integrated.");accept(hooks_.resumeUpgrade(c),events);
    }
    if(c.pendingMysteryCredits && std::none_of(c.fight.upgradeResolution.begin(),c.fight.upgradeResolution.end(),[](const UpgradeResolution& f){return f.event.kind==UpgradeEventKind::Noncombat;})){
        credits(c,c.pendingMysteryCredits);c.pendingMysteryCredits=0;
    }
    if(c.fight.upgradeResolution.empty() && c.fight.upgradeRequests.empty() && c.fight.upgradeChoices.empty() && ownedUpgrade(c.fight,"UGS-148"))
        for(auto& reward:c.rewards)if(reward.kind==RewardKind::Recipe && reward.normalOffer && reward.claim==ClaimState::Pending && reward.lightTouch.empty() && !reward.choices.empty())
            reward.lightTouch.push_back(reward.choices[c.rng.below(Domain::Choice,static_cast<std::uint32_t>(reward.choices.size()))]);
}

void CampaignRules::resolveUpgradeOffer(Campaign& c,const CampaignAction& action,std::vector<Event>& events) const {
    need(c.fight.upgradeChoices.empty(),"Resolve the current engine choice first.");
    const auto current=std::find_if(c.upgradeOffers.begin(),c.upgradeOffers.end(),[](const CampaignOffer& o){return !o.deferred;});
    need(current!=c.upgradeOffers.end() && current->id==action.subject,"Choose the current saved upgrade offer.");
    const auto offer=*current;std::string recipe;RecipeCopy originalCopy;bool copying=false;
    if(action.decline)need(offer.optional,"This upgrade requires a selection.");
    else if(offer.kind==UpgradeRequestKind::Subscription){
        need(action.material>=0 && action.material<5,"Choose one material for the subscription.");
        need(std::find(offer.copies.begin(),offer.copies.end(),action.target)!=offer.copies.end(),"Choose an offered permanent recipe copy.");
        const auto* copy=byId(c.fight.memory,action.target);need(copy && copy->storage!=MemoryKind::Borrowed && std::none_of(copy->tags.begin(),copy->tags.end(),[](const RecipeTag& t){return t.kind==RecipeTagKind::Subscription;}),"This recipe is not eligible for a subscription.");
    }else if(offer.kind==UpgradeRequestKind::CopyRecipe){
        const Id target=offer.sourceCopy?offer.sourceCopy:action.target;
        need(std::find(offer.copies.begin(),offer.copies.end(),target)!=offer.copies.end(),"Choose an offered permanent recipe copy.");
        const auto* copy=byId(c.fight.memory,target);need(copy && copy->storage!=MemoryKind::Borrowed,"Cannot copy a borrowed or missing recipe.");
        need(offer.source!="UGS-032" || action.exchange!=target,"Exchange a different recipe copy.");
        originalCopy=*copy;recipe=copy->recipe;copying=true;
        need(offer.selected.empty() || offer.selected==std::to_string(target),"Cancel the pending copy selection before choosing another source.");
        if(!hasFreeMemory(c,recipe) && action.exchange==0){current->selected=std::to_string(target);return;}
    }else{
        need(offer.index>=0 && offer.index<static_cast<Amount>(offer.candidates.size()) && has(offer.candidates[static_cast<std::size_t>(offer.index)],action.choice),"Choose one of this offer's saved candidates.");
        need(offer.selected.empty() || offer.selected==action.choice,"Cancel the pending exchange before selecting another recipe.");
        if(offer.kind==UpgradeRequestKind::RecipeOffer){recipe=action.choice;if(!hasFreeMemory(c,recipe,offer.storage) && action.exchange==0){current->selected=recipe;return;}}
    }
    const bool finished=offer.kind==UpgradeRequestKind::CopyRecipe || offer.kind==UpgradeRequestKind::Subscription || offer.index+1>=static_cast<Amount>(offer.candidates.size());
    // Remove the parent before nested acquisition. The surrounding campaign
    // transaction rolls back the acknowledgement together with any failed cost.
    if(finished){
        c.upgradeOffers.erase(current);
        if(offer.request)need(acknowledgeUpgradeRequest(c.fight,offer.request),"The parent upgrade request is unavailable.");
    }else{++current->index;current->selected.clear();if(current->kind==UpgradeRequestKind::RecipeOffer)for(const auto& id:current->candidates[static_cast<std::size_t>(current->index)])remember(c.profile.recipes,id);}
    if(action.decline)credits(c,offer.creditAlternative);
    else if(offer.kind==UpgradeRequestKind::Subscription){
        auto* copy=byId(c.fight.memory,action.target);const auto* source=ownedUpgrade(c.fight,offer.source);need(copy && source,"Subscription source was lost.");
        copy->tags.push_back({offer.source,source->order,RecipeTagKind::Subscription,1,action.material,0});
    }else if(offer.kind==UpgradeRequestKind::UpgradeOffer)acquireUpgrade(c,action.choice,action,events);
    else{
        const auto id=acquireRecipe(c,recipe,action.exchange,offer.storage,offer.storage==MemoryKind::Borrowed?offer.source:"");
        if(copying){auto* copy=byId(c.fight.memory,id);copy->tags=originalCopy.tags;for(auto& t:copy->tags)t.usedFight=0;}
        UpgradeEvent event;event.kind=UpgradeEventKind::RecipeAccepted;event.origin=offer.storage==MemoryKind::Borrowed?AcquisitionSource::Borrowed:AcquisitionSource::Upgrade;
        event.subject=id;event.definition=recipe;event.source=offer.source;event.normalOffer=false;notifyUpgrade(c,event,events);
    }
    if(finished){need(static_cast<bool>(hooks_.resumeUpgrade),"Upgrade continuation is not integrated.");accept(hooks_.resumeUpgrade(c),events);}
}
} // namespace overkill

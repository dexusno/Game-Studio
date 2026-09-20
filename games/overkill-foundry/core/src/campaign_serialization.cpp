#include "overkill/campaign.hpp"
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace overkill {
namespace {
void need(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
std::uint64_t hashBytes(const std::string& bytes){std::uint64_t hash=14695981039346656037ULL;for(unsigned char c:bytes){hash^=c;hash*=1099511628211ULL;}return hash;}
struct Writer {
    static constexpr bool reading=false;std::string data;
    template<class T> void number(T& value){using U=std::make_unsigned_t<T>;const auto u=static_cast<U>(value);for(std::size_t i=0;i<sizeof(T);++i)data.push_back(static_cast<char>((u>>(8*i))&255U));}
    void raw(const char* dataIn,std::size_t length){data.append(dataIn,length);}
};
struct Reader {
    static constexpr bool reading=true;const std::string& data;std::size_t at=0;
    void require(std::size_t count){need(count<=data.size()-at,"Truncated campaign snapshot.");}
    template<class T> void number(T& value){require(sizeof(T));std::uint64_t u=0;for(std::size_t i=0;i<sizeof(T);++i)u|=static_cast<std::uint64_t>(static_cast<unsigned char>(data[at++]))<<(8*i);value=static_cast<T>(u);}
    void raw(char* output,std::size_t length){require(length);std::memcpy(output,data.data()+at,length);at+=length;}
};
template<class A,class T,std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T,bool>,int> =0>void field(A& a,T& x){a.number(x);}
template<class A>void field(A& a,bool& x){std::uint8_t n=x?1:0;a.number(n);if constexpr(A::reading){need(n<=1,"Invalid campaign boolean.");x=n!=0;}}
template<class A,class T,std::enable_if_t<std::is_enum_v<T>,int> =0>void field(A& a,T& x){auto n=static_cast<std::underlying_type_t<T>>(x);a.number(n);if constexpr(A::reading)x=static_cast<T>(n);}
template<class A>void field(A& a,std::string& x){
    need(x.size()<=std::numeric_limits<std::uint32_t>::max(),"Campaign string exceeds encoding range.");std::uint32_t n=static_cast<std::uint32_t>(x.size());a.number(n);
    if constexpr(A::reading){a.require(n);x.resize(n);a.raw(x.data(),n);}else a.raw(x.data(),n);
}
template<class A>void field(A&,RouteOffer&);template<class A>void field(A&,RouteNode&);template<class A>void field(A&,CityRoute&);
template<class A>void field(A&,EnergyCore&);template<class A>void field(A&,Product&);template<class A>void field(A&,RewardEntry&);
template<class A>void field(A&,CampaignOffer&);
template<class A>void field(A&,UpgradeRequest&);
template<class A>void field(A&,ProfileFacts&);template<class A>void field(A&,CampaignReceipt&);template<class A>void field(A&,Event&);
template<class A,class T>void field(A& a,std::vector<T>& x){
    need(x.size()<=std::numeric_limits<std::uint32_t>::max(),"Campaign vector exceeds encoding range.");std::uint32_t n=static_cast<std::uint32_t>(x.size());a.number(n);
    if constexpr(A::reading){a.require(n);x.resize(n);}for(auto& item:x)field(a,item);
}
template<class A,class T,std::size_t N>void field(A& a,std::array<T,N>& x){for(auto& item:x)field(a,item);}
template<class A,class...T>void fields(A& a,T&... values){(field(a,values),...);}
template<class A>void field(A& a,State& x){std::string bytes;if constexpr(!A::reading)bytes=serialize(x);field(a,bytes);if constexpr(A::reading){std::string error;if(!deserialize(bytes,x,error))throw std::runtime_error("Invalid campaign fight state: "+error);}}
template<class A>void field(A& a,RouteOffer& x){fields(a,x.id,x.kind,x.formation,x.mystery,x.district,x.encounterKey,x.encounterSeed,x.binderDefaultSeen,x.replaced,x.surveyed);}
template<class A>void field(A& a,RouteNode& x){fields(a,x.offers,x.selected,x.resolved);}
template<class A>void field(A& a,CityRoute& x){fields(a,x.seed,x.rng.state,x.nextOfferId,x.position,x.officer,x.mystery,x.nodes,x.officerOrder,x.district,x.previousFormation,x.binderDefaultSeen,x.safeMysteries);}
template<class A>void field(A& a,EnergyCore& x){fields(a,x.id,x.robot,x.baseValue);}
template<class A>void field(A& a,Product& x){fields(a,x.id,x.kind,x.definition,x.material,x.quantity,x.price);}
template<class A>void field(A& a,RewardEntry& x){fields(a,x.id,x.kind,x.claim,x.choices,x.cores,x.credits,x.normalOffer,x.lightTouch,x.pool,x.fixedRarity);}
template<class A>void field(A& a,CampaignOffer& x){fields(a,x.id,x.request,x.source,x.kind,x.candidates,x.index,x.creditAlternative,x.storage,x.optional,x.selected,x.copies,x.sourceCopy,x.deferred);}
template<class A>void field(A& a,UpgradeRequest& x){fields(a,x.id,x.source,x.kind,x.offers,x.count,x.rarity,x.recipeKind,x.filterKind,x.sharedOnly,x.optional,x.normalOffer,x.recipeCopy,x.reward,x.definition);}
template<class A>void field(A& a,ProfileFacts& x){fields(a,x.recipes,x.unlocked,x.cityClearReceipts);}
template<class A>void field(A& a,Event& x){fields(a,x.id,x.parent,x.subject,x.target,x.type,x.amount,x.secondary,x.source,x.round,x.paid);}
template<class A>void field(A& a,CampaignReceipt& x){fields(a,x.sequence,x.command,x.message,x.events);}
template<class A>void visit(A& a,Campaign& x){
    fields(a,x.rulesVersion,x.contentVersion,x.manifestHash,x.runId,x.seed,x.nextId,x.nextTransaction,x.phase,x.rng.state,x.profile,x.fight,x.route,x.routePreviews,
           x.memorySlots,x.shopGeneration,x.cores,x.everAcquired,x.mayorOffers,x.mayor,x.shop,x.eventShop,x.rewards,x.rewardAdjustments,x.upgradeOffers,x.pendingMysteryCredits,x.revealNextReward,x.recipeWindow,x.pendingRecipeChoice,x.skipConfirmation,x.preStart,x.entry,x.receipts);
}
void unique(const std::vector<std::string>& values){std::set<std::string> seen;for(const auto& x:values)need(!x.empty() && seen.insert(x).second,"Duplicate or empty campaign identity.");}
Campaign parse(const std::string& bytes,bool allowEntry);
void validate(const Campaign& c,bool allowEntry){
    need(c.rulesVersion==RulesVersion && c.contentVersion==ContentVersion,"Unsupported campaign rules/content version.");
    need(c.manifestHash.size()==64 && c.manifestHash.find_first_not_of("0123456789abcdef")==std::string::npos,"Invalid content manifest identity.");
    need(!c.runId.empty() && c.nextId>0 && c.nextTransaction>0 && c.phase<=CityPhase::Complete,"Invalid campaign identity or phase.");
    need(c.memorySlots>0 && c.shopGeneration>0 && c.pendingMysteryCredits>=0,"Invalid campaign capacity or stock generation.");
    Amount general=0,utility=0,borrowed=0;static const Rules recipes;
    for(const auto& copy:c.fight.memory){
        const auto* recipe=recipes.recipe(copy.recipe);need(recipe!=nullptr,"Unknown campaign recipe.");
        if(copy.storage==MemoryKind::General)++general;
        else if(copy.storage==MemoryKind::Utility){++utility;need(recipe->kind==Kind::Utility,"Non-Utility recipe occupies a dedicated slot.");}
        else if(copy.storage==MemoryKind::Borrowed){++borrowed;need(c.phase==CityPhase::Fight && copy.borrowedFrom=="UGS-133" && ownedUpgrade(c.fight,"UGS-133"),"Orphaned borrowed recipe.");}
        else need(false,"Invalid memory storage kind.");
    }
    need(general<=c.memorySlots+upgradeGeneralMemoryBonus(c.fight) && utility<=upgradeUtilityMemoryBonus(c.fight) && borrowed<=1,"Campaign memory exceeds its eligible slots.");
    std::string error;need(validateRoute(c.route,error),"Invalid campaign route.");need(c.route.seed==c.seed,"Route seed changed.");
    need(c.route.safeMysteries==(ownedUpgrade(c.fight,"UGS-057")!=nullptr),"Route survey ownership changed.");
    for(const auto& preview:c.routePreviews){
        need(c.phase==CityPhase::Between && ownedUpgrade(c.fight,"MY1-12") && preview.replaced,"Invalid route preview.");
        const auto& offers=routeOffers(c.route);const auto found=std::find_if(offers.begin(),offers.end(),[&](const RouteOffer& o){return o.id==preview.id;});need(found!=offers.end(),"Route preview belongs to another screen.");
        RouteOffer expected;if(found->replaced)expected=*found;else need(previewRouteReplacement(c.route,preview.id,expected,error),"Invalid route replacement preview.");
        need(expected.encounterKey==preview.encounterKey && expected.encounterSeed==preview.encounterSeed && expected.kind==preview.kind && expected.formation==preview.formation && expected.mystery==preview.mystery && expected.district==preview.district && expected.binderDefaultSeen==preview.binderDefaultSeen && expected.surveyed==preview.surveyed,"Saved route preview was changed.");
    }
    const auto* active=selectedRouteOffer(c.route);
    if(c.phase==CityPhase::Defeated){
        need(c.fight.hp==0 && c.fight.phase==Phase::Defeat && active && !active->formation.empty(),"Defeated campaign has no terminal defeat.");
    }else{
        need(c.fight.hp>0 && c.fight.phase!=Phase::Defeat && c.fight.phase!=Phase::Victory && c.fight.phase!=Phase::Escaped,"Live campaign has a terminal fight state.");
        if(c.phase!=CityPhase::Fight)need(c.fight.phase==Phase::Collection,"Closed encounter retained an active preparation phase.");
    }
    if(c.phase==CityPhase::Arrival || c.phase==CityPhase::Between || c.phase==CityPhase::Rewards || c.phase==CityPhase::Complete)
        need(active==nullptr,"Closed encounter has a committed next offer.");
    if(c.phase==CityPhase::Arrival)need(c.route.position==1,"Arrival moved past the opening route position.");
    if(c.phase==CityPhase::Between)need(c.route.position<=12,"Completed route still offers another encounter.");
    if(c.phase==CityPhase::Rewards)need(c.route.position>=2,"Victory rewards precede any completed encounter.");
    if(c.phase==CityPhase::Complete)need(c.route.position==13,"City marked complete before its final encounter.");
    if(c.phase==CityPhase::Mystery)need(active && active->kind==EncounterKind::Mystery,"Mystery phase lost its committed outcome.");
    unique(c.profile.recipes);unique(c.profile.unlocked);unique(c.profile.cityClearReceipts);unique(c.everAcquired);unique(c.mayorOffers);
    need(c.mayorOffers.size()==3,"Invalid Mayor offer count.");
    need(c.phase!=CityPhase::Arrival || c.mayor.empty(),"Arrival already acquired a Mayor gift.");
    std::set<Id> productIds,rewardIds,coreIds,offerIds,requests,orders,receipts;std::set<std::string> upgradeIds;
    const auto identity=[&](Id id){need(id>0 && id<c.nextId,"Invalid campaign object identity.");};
    for(const auto* stock:{&c.shop,&c.eventShop})for(const auto& item:*stock){identity(item.id);need(productIds.insert(item.id).second && item.kind<=ProductKind::Part && item.quantity>=0 && item.price>=0,"Invalid shop product.");if(item.kind==ProductKind::Material)need(item.material>=0 && item.material<5,"Invalid shop material.");else need(!item.definition.empty(),"Missing product definition.");}
    for(const auto& core:c.cores){identity(core.id);need(coreIds.insert(core.id).second && !core.robot.empty() && core.baseValue>=0,"Invalid owned core.");}
    bool recipeFound=c.recipeWindow==0;
    for(const auto& reward:c.rewards){
        identity(reward.id);need(rewardIds.insert(reward.id).second && reward.kind<=RewardKind::Credits && reward.claim<=ClaimState::Abandoned && reward.credits>=0,"Invalid reward entry.");unique(reward.choices);
        unique(reward.lightTouch);for(const auto& id:reward.lightTouch)need(std::find(reward.choices.begin(),reward.choices.end(),id)!=reward.choices.end(),"Light-Touch marks a missing option.");
        if(reward.kind==RewardKind::Recipe)need((reward.pool.empty() || reward.pool=="Regular" || reward.pool=="Officer" || reward.pool=="Boss") && reward.fixedRarity>=-1 && reward.fixedRarity<=4,"Invalid reward pool.");
        std::set<Id> within;for(const auto& core:reward.cores){identity(core.id);need(within.insert(core.id).second && core.baseValue>=0 && !core.robot.empty(),"Invalid reward core.");}
        if(reward.id==c.recipeWindow){recipeFound=true;need(reward.kind==RewardKind::Recipe && reward.claim==ClaimState::Pending,"Invalid recipe-choice cursor.");need(c.pendingRecipeChoice.empty() || std::find(reward.choices.begin(),reward.choices.end(),c.pendingRecipeChoice)!=reward.choices.end(),"Invalid exchange-choice cursor.");}
    }
    need(recipeFound && (c.recipeWindow==0 || c.phase==CityPhase::Rewards) && (!c.skipConfirmation || c.phase==CityPhase::Rewards),"Choice cursor belongs to another phase.");
    need(c.pendingRecipeChoice.empty() || c.recipeWindow!=0,"Orphaned recipe exchange.");
    for(const auto& modifier:c.rewardAdjustments)need(modifier.kind==UpgradeRequestKind::AddRewardOption && modifier.count>0 && modifier.rarity>=-1 && modifier.rarity<=4 && ownedUpgrade(c.fight,modifier.source),"Invalid reward-option modifier.");
    for(const auto& offer:c.upgradeOffers){
        identity(offer.id);need(offerIds.insert(offer.id).second && !offer.source.empty() && ownedUpgrade(c.fight,offer.source),"Invalid upgrade offer identity/source.");
        need(offer.kind<=UpgradeRequestKind::Subscription && offer.storage<=MemoryKind::Borrowed && offer.creditAlternative>=0,"Invalid saved upgrade offer.");
        if(offer.request){
            need(requests.insert(offer.request).second,"Duplicate request offer.");
            need(std::any_of(c.fight.upgradeRequests.begin(),c.fight.upgradeRequests.end(),[&](const UpgradeRequest& r){return r.id==offer.request && r.source==offer.source && r.kind==offer.kind;}),"Offer lost its engine request.");
        }else need(offer.kind==UpgradeRequestKind::Subscription,"Only an earned subscription can defer its request.");
        if(offer.kind==UpgradeRequestKind::RecipeOffer || offer.kind==UpgradeRequestKind::UpgradeOffer){
            need(!offer.deferred && offer.index>=0 && offer.index<static_cast<Amount>(offer.candidates.size()),"Invalid nested-offer cursor.");
            for(const auto& choices:offer.candidates){need(!choices.empty(),"Empty nested offer.");unique(choices);}
            const auto& choices=offer.candidates[static_cast<std::size_t>(offer.index)];need(offer.selected.empty() || std::find(choices.begin(),choices.end(),offer.selected)!=choices.end(),"Invalid nested exchange cursor.");
        }else{
            std::set<Id> ids;for(Id id:offer.copies)need(id>0 && id<c.fight.nextId && ids.insert(id).second,"Invalid copy selection.");
            need(offer.deferred?offer.kind==UpgradeRequestKind::Subscription && offer.copies.empty():!offer.copies.empty(),"Invalid deferred copy decision.");
        }
    }
    for(const auto& upgrade:c.fight.upgrades){
        need(!upgrade.id.empty() && upgradeIds.insert(upgrade.id).second && orders.insert(upgrade.order).second && upgrade.order>0 && upgrade.order<c.fight.nextOrder,"Invalid upgrade identity/order.");
        need(std::find(c.everAcquired.begin(),c.everAcquired.end(),upgrade.id)!=c.everAcquired.end(),"Lost ever-acquired upgrade ledger.");
        need(upgrade.charges>=0 && upgrade.campaignCount>=0 && upgrade.fightCount>=0 && upgrade.roundCount>=0,"Invalid upgrade counters.");
    }
    for(const auto& receipt:c.receipts)need(receipt.sequence>0 && receipt.sequence<c.nextTransaction && receipts.insert(receipt.sequence).second && !receipt.command.empty(),"Invalid transaction receipt.");
    if(c.phase==CityPhase::Fight){
        need(active && !active->formation.empty() && (active->kind!=EncounterKind::Mystery || active->mystery=="C1-M-PATROL"),"Active fight has no committed combat offer.");
        need(c.preStart?c.entry.empty():!c.entry.empty(),"Missing or recursive fight-entry checkpoint.");
        need(c.fight.phase==Phase::Collection || c.fight.phase==Phase::Preparation,"Unreconciled terminal fight in campaign snapshot.");
        if(!c.preStart)need(c.fight.seed==active->encounterSeed && c.fight.encounter==active->encounterKey,"Active fight changed its committed seed or encounter.");
    }else need(c.entry.empty() && !c.preStart,"Closed fight retained an active checkpoint.");
    if(!c.entry.empty()){
        need(allowEntry,"Nested fight checkpoints are not supported.");const auto entry=parse(c.entry,false);
        need(entry.preStart && entry.phase==CityPhase::Fight && entry.runId==c.runId && entry.seed==c.seed && entry.manifestHash==c.manifestHash,"Checkpoint does not belong to this campaign.");
        const auto* old=selectedRouteOffer(entry.route);const auto* current=selectedRouteOffer(c.route);
        need(old && current && old->id==current->id && old->encounterSeed==current->encounterSeed && old->encounterKey==current->encounterKey &&
             old->kind==current->kind && old->formation==current->formation && old->mystery==current->mystery && old->district==current->district &&
             old->binderDefaultSeen==current->binderDefaultSeen && old->replaced==current->replaced && old->surveyed==current->surveyed,"Unfinished fight changed its entry identity.");
    }
}
Campaign parse(const std::string& bytes,bool allowEntry){
    need(bytes.size()>=20,"Truncated campaign envelope.");need(bytes.compare(0,8,"OFCAMP01")==0,"Unsupported campaign envelope.");
    const auto body=bytes.substr(0,bytes.size()-8);Reader tail{bytes,bytes.size()-8};std::uint64_t checksum=0;tail.number(checksum);need(checksum==hashBytes(body),"Campaign integrity check failed.");
    Reader reader{body,8};std::uint32_t schema=0;reader.number(schema);need(schema==3,"Unsupported campaign schema.");Campaign c;visit(reader,c);
    need(reader.at==body.size(),"Unexpected campaign fields.");validate(c,allowEntry);return c;
}
} // namespace
std::string serializeCampaign(const Campaign& campaign){
    validate(campaign,true);Campaign copy=campaign;Writer writer;writer.data="OFCAMP01";std::uint32_t schema=3;writer.number(schema);visit(writer,copy);
    auto checksum=hashBytes(writer.data);writer.number(checksum);return writer.data;
}
bool deserializeCampaign(const std::string& bytes,Campaign& campaign,std::string& error){try{auto c=parse(bytes,true);campaign=std::move(c);return true;}catch(const std::exception& e){error=e.what();return false;}}
std::string campaignHash(const Campaign& campaign){std::ostringstream out;out<<std::hex<<std::setw(16)<<std::setfill('0')<<hashBytes(serializeCampaign(campaign));return out.str();}
} // namespace overkill

#pragma once
#include "overkill/city_content.hpp"
#include "overkill/route.hpp"

namespace overkill {
enum class CityPhase : std::uint8_t { Arrival, Between, Fight, Rewards, Mystery, Defeated, Complete };
enum class ProductKind : std::uint8_t { Material, Recipe, Upgrade, Part };
enum class RewardKind : std::uint8_t { Cores, Recipe, Upgrade, Credits };
enum class ClaimState : std::uint8_t { Pending, Claimed, Abandoned };
struct EnergyCore { Id id=0;std::string robot;Amount baseValue=0; };
struct Product {
    Id id=0;ProductKind kind=ProductKind::Material;std::string definition;
    Amount material=0,quantity=0,price=0;
};
struct RewardEntry {
    Id id=0;RewardKind kind=RewardKind::Cores;ClaimState claim=ClaimState::Pending;
    std::vector<std::string> choices;
    std::vector<EnergyCore> cores;
    Amount credits=0;
    bool normalOffer=true;
    std::vector<std::string> lightTouch;
    std::string pool;
    Amount fixedRarity=-1;
};
struct CampaignOffer {
    Id id=0,request=0;
    std::string source;
    UpgradeRequestKind kind=UpgradeRequestKind::RecipeOffer;
    std::vector<std::vector<std::string>> candidates;
    Amount index=0,creditAlternative=0;
    MemoryKind storage=MemoryKind::General;
    bool optional=true;
    std::string selected;
    std::vector<Id> copies;
    Id sourceCopy=0;
    bool deferred=false;
};
struct ProfileFacts {
    std::vector<std::string> recipes,unlocked{"Mara"};
    std::vector<std::string> cityClearReceipts;
};
struct CampaignReceipt {
    Id sequence=0;std::string command,message;std::vector<Event> events;
};
struct Campaign {
    std::string rulesVersion=RulesVersion,contentVersion=ContentVersion,manifestHash,runId;
    std::uint64_t seed=1;
    Id nextId=1,nextTransaction=1;
    CityPhase phase=CityPhase::Arrival;
    Rng rng;
    ProfileFacts profile;
    State fight;
    CityRoute route;
    std::vector<RouteOffer> routePreviews;
    Amount memorySlots=20,shopGeneration=0;
    std::vector<EnergyCore> cores;
    std::vector<std::string> everAcquired,mayorOffers;
    std::string mayor;
    std::vector<Product> shop,eventShop;
    std::vector<RewardEntry> rewards;
    std::vector<UpgradeRequest> rewardAdjustments;
    std::vector<CampaignOffer> upgradeOffers;
    Amount pendingMysteryCredits=0;
    bool revealNextReward=false;
    Id recipeWindow=0;
    std::string pendingRecipeChoice;
    bool skipConfirmation=false,preStart=false;
    // Exact serialized pre-start campaign. Its own entry is empty, so this is
    // one level, not a recursive mid-fight save. Profile discoveries survive it.
    std::string entry;
    std::vector<CampaignReceipt> receipts;
};
enum class CampaignActionType : std::uint8_t {
    ChooseMayor,EnterOffer,EnterPatrol,Combat,OpenRecipes,BackRecipes,ClaimReward,
    RequestAdvance,CancelAdvance,ConfirmAdvance,Buy,SellCore,SellPart,
    AcceptCalibration,LeaveMystery,Continue,OpenShop,ResolveUpgradeChoice,ResolveUpgradeOffer,
    RerollRecipeReward,PreviewRouteReplacement,ReplaceRouteOffer,CancelUpgradeExchange,MoveRecipeCopy
};
struct CampaignAction {
    CampaignActionType type=CampaignActionType::RequestAdvance;
    std::string runId; // Together with sequence identifies this run's command.
    Id sequence=0,subject=0,exchange=0;
    Id target=0;
    std::string choice;
    Amount quantity=1;
    bool eventShop=false;
    bool decline=false;
    Amount material=0;
    Action combat;
};
struct CampaignResult {
    bool ok=false,replayed=false;std::string reason;std::vector<Event> events;
};
// Explicit integration boundaries. Missing effect implementations reject their
// action; metadata presence alone never grants an inert upgrade or physical part.
struct CampaignHooks {
    std::function<Result(Campaign&,const std::string&,const CampaignAction&)> acquireUpgrade;
    std::function<Result(Campaign&,const RouteOffer&)> initializeFight;
    std::function<Result(Campaign&,const std::string&)> grantPart;
    std::function<Result(Campaign&)> combatCompleted;
    std::function<Result(Campaign&)> noncombatCompleted;
    std::function<Result(Campaign&,const UpgradeEvent&)> notifyUpgrade;
    std::function<Result(Campaign&)> resumeUpgrade;
    std::function<Amount(const Campaign&,const Product&)> productPrice;
    std::function<Amount(const Campaign&,const EnergyCore&)> coreSaleValue;
    std::function<Amount(const Campaign&,const Part&)> partSaleValue;
};
class CampaignRules {
public:
    explicit CampaignRules(const Rules& fights,CampaignHooks hooks={},CityContent content=cinderwallContent());
    Campaign newGame(std::uint64_t seed,const std::string& runId,const ProfileFacts& profile={}) const;
    CampaignResult apply(Campaign& campaign,const CampaignAction& action) const;
    const CityContent& content() const {return content_;}
    bool hasFreeMemory(const Campaign& campaign,const std::string& recipe,MemoryKind storage=MemoryKind::General) const;
    std::string revealedRecipe(const Campaign& campaign,const RouteOffer& offer) const;
    std::string revealedMysteryCategory(const Campaign& campaign,Id offer) const;
private:
    const Rules& fights_;CampaignHooks hooks_;CityContent content_;
    void restock(Campaign& campaign) const;
    Id acquireRecipe(Campaign& campaign,const std::string& recipe,Id exchange,MemoryKind storage=MemoryKind::General,const std::string& borrowedFrom={}) const;
    void acquireUpgrade(Campaign& campaign,const std::string& upgrade,const CampaignAction& action,std::vector<Event>& events) const;
    void finishCombat(Campaign& campaign,std::vector<Event>& events) const;
    void notifyUpgrade(Campaign& campaign,const UpgradeEvent& event,std::vector<Event>& events) const;
    void prepareUpgradeOffers(Campaign& campaign,std::vector<Event>& events) const;
    void resolveUpgradeOffer(Campaign& campaign,const CampaignAction& action,std::vector<Event>& events) const;
    std::vector<std::string> normalRecipeOffer(const Campaign& campaign,const RouteOffer& offer) const;
};
// Production hooks use the same Rules engine. Test-only hooks remain explicit.
CampaignHooks cinderwallUpgradeHooks(const Rules& fights,CityContent content=cinderwallContent());
std::string serializeCampaign(const Campaign& campaign);
bool deserializeCampaign(const std::string& bytes,Campaign& campaign,std::string& error);
std::string campaignHash(const Campaign& campaign);
} // namespace overkill

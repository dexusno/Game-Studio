#pragma once
#include "overkill/core.hpp"
#include <functional>

namespace overkill {
// Offer metadata is derived from the manifest; it never executes an effect.
struct CityItem {
    std::string id,name,description;
    Amount rarity=0,price=0; // Base=0, Common=1, Uncommon=2, Rare=3, Legendary=4.
};
struct CityPool {
    std::string name;
    std::vector<std::string> ids;
    std::array<Amount,5> weights{};
};
struct CoreValue { std::string robot;Amount credits=0; };
struct CityContent {
    std::string manifestHash;
    std::vector<CityItem> recipes,upgrades;
    std::vector<CityPool> recipePools,upgradePools,mayorPools;
    std::vector<std::string> starters;
    Materials materialPrices{},materialStock{};
    std::vector<CoreValue> coreValues;
    Amount memorySlots=20,startingCredits=100,recipeSlots=4,upgradeSlots=2;
};
CityContent cinderwallContent();
const CityItem* cityItem(const std::vector<CityItem>& items,const std::string& id);
// Rarity first, then a uniform remaining eligible ID; no duplicates within an
// offer. Owned recipe IDs stay eligible; callers exclude ever-acquired upgrades.
std::vector<std::string> drawCityOffer(const std::vector<CityItem>& items,
    const std::vector<CityPool>& pools,const std::string& source,Rng& rng,Domain domain,
    Amount count,const std::vector<std::string>& excluded={},
    const std::function<bool(const std::string&)>& eligible={});
} // namespace overkill

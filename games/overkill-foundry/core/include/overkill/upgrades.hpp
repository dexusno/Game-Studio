#pragma once
#include "overkill/core.hpp"

namespace overkill {
// Exact eligible Cinderwall IDs. Runtime coverage is reported by semantic tests,
// not by the presence of these registrations.
std::vector<std::string> eligibleUpgradeIds();
bool upgradeEligible(const Rules& rules,const State& state,const std::string& id);
const OwnedUpgrade* ownedUpgrade(const State& state,const std::string& id);
OwnedUpgrade* ownedUpgrade(State& state,const std::string& id);
Amount upgradeCounter(const OwnedUpgrade& upgrade,const std::string& key);
bool isUnusedPart(const Part& part);
Amount upgradeHeatCap(const State& state);
Amount upgradeHeatDecay(const State& state);
Amount upgradePrecisionWidthPercent(const State& state); // 100 = authored width.
Amount upgradeGeneralMemoryBonus(const State& state);
Amount upgradeUtilityMemoryBonus(const State& state);
Amount upgradeShopPrice(const State& state,PurchaseKind kind,Amount listedPrice);
Amount upgradeCoreSaleValue(const State& state,Amount baseValue);
Amount upgradePartSaleValue(const State& state,const Part& part,const Materials& materialPrices);

// Root adapters consume requests in saved order, generating candidate offers
// with the campaign Choice/Reward stream exactly once. They store the candidates
// and their nested exchange cursor before presentation. RecipeOffer is an
// acquisition offer; AddRewardOption/Offer modifies the identified victory's
// reward. CopyRecipe targets the exact ready copy. Subscription attaches one
// RecipeTagKind::Subscription to a selected eligible permanent copy.
// UGS-089 requests three sequential RecipeOffers; MY1-06 requests one shared
// Uncommon; UGS-133 requests one shared Common and must install Borrowed storage.
// UGS-113/150 request eligible, ever-unacquired upgrades of stated rarity;
// UGS-113 declining keeps the Mystery Credit reward; UGS-150 exhaustion pays30.
// UGS-031/034/062/101/147 add the authored option/offer; UGS-148 marks a saved
// normal-offer option and reports lightTouch on RecipeAccepted.
// Requests are acknowledged only after their result/exchange commits. This
// helper is idempotent only at the surrounding campaign receipt boundary.
bool acknowledgeUpgradeRequest(State& state,Id id);

// Route information/replacement and reward-reroll actions use the named ledger
// and counters: MY1-12 charges=3; MY1-14 'reward_reroll' (Fight) is consumed once
// per completed victory; UGS-057/149 are pure owned predicates. The adapter
// persists deterministic alternatives/previews, never redrawing on reopen.
} // namespace overkill

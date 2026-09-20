#include "overkill/campaign.hpp"
#include "overkill/robots.hpp"
#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>
using namespace overkill;
namespace {
int checks=0;
void check(bool ok,const std::string& why){++checks;if(!ok)throw std::runtime_error(why);}
CampaignAction command(const Campaign& c,CampaignActionType type,Id subject=0,std::string choice={}){CampaignAction a;a.runId=c.runId;a.sequence=c.nextTransaction;a.type=type;a.subject=subject;a.choice=std::move(choice);return a;}
CampaignResult run(Campaign& c,const CampaignRules& rules,CampaignAction a){auto result=rules.apply(c,a);check(result.ok,result.reason);return result;}
void combat(Campaign& c,const CampaignRules& rules,Action action){auto a=command(c,CampaignActionType::Combat);a.combat=std::move(action);run(c,rules,a);}
CampaignHooks transactionFixtureHooks(const Rules& fights){
    CampaignHooks hooks;
    // Deliberately test-only payload: this verifies lifecycle/atomic receipts,
    // not any Mayor or permanent-upgrade effect. P08 remains separately gated.
    hooks.acquireUpgrade=[](Campaign& c,const std::string& id,const CampaignAction&){OwnedUpgrade u;u.id=id;u.order=c.fight.nextOrder++;u.charges=1;c.fight.upgrades.push_back(u);return Result{true,{},{}};};
    hooks.initializeFight=[](Campaign& c,const RouteOffer& offer){c.fight.enemies=makeCinderwallFormation(offer.formation,offer.encounterSeed,offer.encounterKey,c.fight.nextId,offer.binderDefaultSeen);return Result{true,{},{}};};
    hooks.grantPart=[&fights](Campaign& c,const std::string& id){return fights.grantPart(c.fight,id,false);};
    hooks.combatCompleted=[](Campaign&){return Result{true,{},{}};};
    hooks.noncombatCompleted=[](Campaign& c){c.fight.credits+=10;return Result{true,{},{}};};
    hooks.partSaleValue=[](const Campaign&,const Part& p){return p.recipe=="SH001"?2:p.recipe=="SH002"?4:-1;};
    return hooks;
}
Campaign start(const CampaignRules& rules,std::uint64_t seed=1){auto c=rules.newGame(seed,"transaction-fixture-"+std::to_string(seed));c.mayorOffers={"MY1-12","MY1-03","MY1-01"};run(c,rules,command(c,CampaignActionType::ChooseMayor,0,c.mayorOffers[0]));return c;}
Id product(const Campaign& c,ProductKind kind,const std::string& definition={},Amount material=0){for(const auto& p:c.shop)if(p.kind==kind && p.definition==definition && (kind!=ProductKind::Material || p.material==material))return p.id;throw std::runtime_error("Missing fixture product.");}
Id reward(const Campaign& c,RewardKind kind){for(const auto& r:c.rewards)if(r.kind==kind)return r.id;throw std::runtime_error("Missing fixture reward.");}
const RewardEntry& entry(const Campaign& c,Id id){for(const auto& r:c.rewards)if(r.id==id)return r;throw std::runtime_error("Missing reward entry.");}
void enterRam(Campaign& c,const CampaignRules& rules){Id id=0;for(const auto& offer:routeOffers(c.route))if(offer.formation=="C1-F-RAM")id=offer.id;check(id!=0,"Opening lacks Ram choice.");run(c,rules,command(c,CampaignActionType::EnterOffer,id));}
void winControlledRam(Campaign& c,const CampaignRules& rules,const Rules& fights){
    // A controlled 50-damage grant shortens transaction fixtures; this is not a
    // balance policy or a claim that a normal run receives such ammunition.
    const auto grant=fights.grantPlainPart(c.fight,Kind::Ammo,50,"campaign-test-shot",false);check(grant.ok,grant.reason);
    Id ammo=0;for(const auto& p:c.fight.parts)if(p.kind==Kind::Ammo && p.originalValue==50)ammo=p.id;
    check(ammo!=0,"Test ammunition not materialized.");combat(c,rules,Action::collect(0));combat(c,rules,Action::load({ammo}));combat(c,rules,Action::fire(c.fight.enemies[0].id));
    check(c.phase==CityPhase::Rewards && c.route.position==2,"Terminal Fire did not commit the result/rewards.");
}
Campaign withRewards(const CampaignRules& rules,const Rules& fights,std::uint64_t seed=1){auto c=start(rules,seed);enterRam(c,rules);winControlledRam(c,rules,fights);return c;}
void roundtrip(Campaign& c){const auto bytes=serializeCampaign(c);Campaign restored;std::string error;check(deserializeCampaign(bytes,restored,error),error);check(serializeCampaign(restored)==bytes,"Campaign roundtrip changed bytes.");c=std::move(restored);}
void rejectedSnapshot(const Campaign& c){bool rejected=false;try{const auto bytes=serializeCampaign(c);Campaign parsed;std::string error;rejected=!deserializeCampaign(bytes,parsed,error);}catch(const std::exception&){rejected=true;}check(rejected,"Invalid campaign phase/entry identity was accepted.");}
Campaign mysteryFixture(const CampaignRules& rules,const std::string& outcome){
    // Route-only traversal constructs the transaction boundary. It deliberately
    // does not simulate the intervening fights or claim a balanced city clear.
    for(std::uint64_t seed=1;seed<100;++seed){auto c=start(rules,seed);
        while(c.route.position<12){
            for(const auto& offer:routeOffers(c.route))if(offer.mystery==outcome){run(c,rules,command(c,CampaignActionType::EnterOffer,offer.id));return c;}
            std::string error;check(selectRouteOffer(c.route,routeOffers(c.route).front().id,error),error);check(completeRouteNode(c.route,error),error);
        }
    }
    throw std::runtime_error("Mystery fixture seed search exhausted.");
}
}
int main(){try{
    Rules fights;CampaignRules rules(fights,transactionFixtureHooks(fights));
    const auto data=cinderwallContent();check(data.recipes.size()==246 && data.upgrades.size()==152,"Manifest projection changed content scope.");
    for(const auto& id:{"SH121","SH122","SH123"})check(cityItem(data.recipes,id)->rarity==3,"Suffixed source rarity was lost.");
    for(std::uint64_t seed=0;seed<100;++seed){
        auto rng=Rng::seeded(seed,"offer-fixture"),same=rng;const auto before=rng.state;
        const auto a=drawCityOffer(data.recipes,data.recipePools,"Regular",rng,Domain::Reward,3);
        const auto b=drawCityOffer(data.recipes,data.recipePools,"Regular",same,Domain::Reward,3);
        check(a==b && a.size()==3 && std::set<std::string>(a.begin(),a.end()).size()==3,"Offer replay/uniqueness.");
        for(const auto& id:a)check(cityItem(data.recipes,id)->rarity>0 && cityItem(data.recipes,id)->rarity<4,"Regular source leaked forbidden rarity.");
        check(rng.state[static_cast<std::size_t>(Domain::Shop)]==before[static_cast<std::size_t>(Domain::Shop)],"Reward draw changed Shop RNG.");
    }
    {
        auto c=start(rules);check(c.fight.memory.size()==12 && c.memorySlots==20 && c.shopGeneration==1,"New Game inventory/stock.");roundtrip(c);
        const auto heal=product(c,ProductKind::Recipe,"SH074");const auto hp=c.fight.hp;
        auto buy=command(c,CampaignActionType::Buy,heal);run(c,rules,buy);check(c.fight.credits==25 && c.fight.hp==hp && c.fight.memory.size()==13,"Quick Patch purchase healed or charged incorrectly.");
        const auto committed=serializeCampaign(c);auto duplicate=rules.apply(c,buy);check(duplicate.ok && duplicate.replayed && serializeCampaign(c)==committed,"Duplicate purchase changed state.");
        buy.quantity=2;check(!rules.apply(c,buy).ok && serializeCampaign(c)==committed,"Reused operation identity changed payload.");
        Id expensive=0;for(const auto& item:c.shop)if(item.kind==ProductKind::Upgrade)expensive=item.id;
        check(expensive!=0 && !rules.apply(c,command(c,CampaignActionType::Buy,expensive)).ok && serializeCampaign(c)==committed,"Unaffordable purchase changed campaign.");
        std::cout<<"PASS campaign purchase: fixed inventory, idempotent receipt and unaffordable rejection\n";
    }
    {
        auto c=start(rules);auto buy=command(c,CampaignActionType::Buy,product(c,ProductKind::Material));buy.quantity=2;run(c,rules,buy);
        check(c.fight.materials[0]==2 && c.fight.credits==92,"Between-fight purchase not staged.");
        enterRam(c,rules);check(c.fight.materials[0]==2 && !c.entry.empty(),"Fight entry cleared purchased supplies.");
        const auto entryHash=stateHash(c.fight);const auto stock=c.shopGeneration;const auto ironItem=product(c,ProductKind::Material);
        buy=command(c,CampaignActionType::Buy,ironItem);run(c,rules,buy);c.fight.upgrades[0].charges=0;
        run(c,rules,command(c,CampaignActionType::OpenShop));const auto seen=c.profile.recipes;
        check(c.fight.materials[0]==3 && c.fight.credits==88,"In-fight purchase failed.");roundtrip(c);
        run(c,rules,command(c,CampaignActionType::Continue));check(stateHash(c.fight)==entryHash,"Continue did not reconstruct original fight input/seed.");
        check(c.fight.materials[0]==2 && c.fight.credits==92 && c.fight.upgrades[0].charges==1 && c.shopGeneration==stock,"Continue leaked abandoned inventory/charge/stock state.");
        check(c.profile.recipes==seen,"Continue lost profile discoveries.");roundtrip(c);
        run(c,rules,command(c,CampaignActionType::Continue));check(stateHash(c.fight)==entryHash,"Repeated Continue duplicated entry effects.");
        std::cout<<"PASS campaign Continue: original entry, purchase rollback and persistent discoveries\n";
    }
    {
        auto c=withRewards(rules,fights);check(c.fight.materials==Materials{} && c.fight.parts.empty() && c.entry.empty() && c.shopGeneration==2,"Result boundary failed clearing/restock/checkpoint.");
        const auto rid=reward(c,RewardKind::Recipe),cores=reward(c,RewardKind::Cores);const auto fixed=entry(c,rid).choices;const auto money=c.fight.credits;
        run(c,rules,command(c,CampaignActionType::OpenRecipes,rid));run(c,rules,command(c,CampaignActionType::BackRecipes));
        check(entry(c,rid).claim==ClaimState::Pending,"Inner Skip abandoned recipes.");
        const auto claim=command(c,CampaignActionType::ClaimReward,cores);run(c,rules,claim);check(c.cores.size()==1 && c.cores[0].baseValue==15 && c.fight.credits==money,"Core acceptance auto-sold or changed value.");
        run(c,rules,command(c,CampaignActionType::OpenRecipes,rid));roundtrip(c);check(entry(c,rid).choices==fixed && c.recipeWindow==rid,"T19 reload rerolled/lost pending recipe cursor.");
        const auto committed=serializeCampaign(c);check(rules.apply(c,claim).replayed && serializeCampaign(c)==committed,"Reload duplicated claimed cores.");
        run(c,rules,command(c,CampaignActionType::RequestAdvance));check(c.skipConfirmation && c.phase==CityPhase::Rewards,"Pending loot did not require confirmation.");
        run(c,rules,command(c,CampaignActionType::CancelAdvance));check(!c.skipConfirmation && entry(c,rid).claim==ClaimState::Pending && c.cores.size()==1,"Cancel changed rewards.");
        run(c,rules,command(c,CampaignActionType::RequestAdvance));const auto confirm=command(c,CampaignActionType::ConfirmAdvance);run(c,rules,confirm);
        check(c.phase==CityPhase::Between && c.route.position==2 && c.cores.size()==1 && entry(c,rid).claim==ClaimState::Abandoned,"Confirmed skip lost claims or advanced twice.");
        const auto after=serializeCampaign(c);check(rules.apply(c,confirm).replayed && serializeCampaign(c)==after,"Confirmed skip not idempotent.");
        auto buy=command(c,CampaignActionType::Buy,product(c,ProductKind::Material));buy.quantity=2;c.fight.credits=50;
        for(auto& p:c.shop)if(p.id==buy.subject)p.price=10;run(c,rules,buy);roundtrip(c);
        check(c.fight.materials[0]==2 && c.fight.credits==30,"T11 bought supplies lost after result.");
        std::cout<<"PASS campaign rewards: owned cores, fixed choices, cancellation and once-only advance\n";
    }
    {
        auto c=withRewards(rules,fights,7);while(c.fight.memory.size()<20)c.fight.memory.push_back({c.fight.nextId++,"SH001",0,0,0});
        const auto rid=reward(c,RewardKind::Recipe);const auto choices=entry(c,rid).choices;const auto original=c.fight.memory;
        run(c,rules,command(c,CampaignActionType::OpenRecipes,rid));run(c,rules,command(c,CampaignActionType::ClaimReward,rid,choices[1]));
        check(c.pendingRecipeChoice==choices[1] && c.fight.memory.size()==20 && entry(c,rid).claim==ClaimState::Pending,"Full-memory selection acquired early.");
        roundtrip(c);check(c.pendingRecipeChoice==choices[1],"Exchange cursor did not survive save.");
        run(c,rules,command(c,CampaignActionType::BackRecipes));check(c.fight.memory.front().id==original.front().id && entry(c,rid).claim==ClaimState::Pending,"Exchange cancellation mutated memory.");
        run(c,rules,command(c,CampaignActionType::OpenRecipes,rid));auto exchange=command(c,CampaignActionType::ClaimReward,rid,choices[1]);exchange.exchange=c.fight.memory.front().id;run(c,rules,exchange);
        check(c.fight.memory.size()==20 && c.fight.memory.back().recipe==choices[1] && entry(c,rid).claim==ClaimState::Claimed,"Atomic memory exchange failed.");
        run(c,rules,command(c,CampaignActionType::ClaimReward,reward(c,RewardKind::Cores)));run(c,rules,command(c,CampaignActionType::RequestAdvance));
        check(c.phase==CityPhase::Between && !c.skipConfirmation,"Empty reward set requested discard confirmation.");roundtrip(c);
        std::cout<<"PASS campaign memory: saved pending exchange, cancellation and atomic replacement\n";
    }
    {
        auto c=withRewards(rules,fights,11);run(c,rules,command(c,CampaignActionType::RequestAdvance));run(c,rules,command(c,CampaignActionType::ConfirmAdvance));
        check(c.cores.empty() && c.phase==CityPhase::Between && c.fight.memory.size()==12,"T22 abandoned set granted something.");
        auto empty=withRewards(rules,fights,12);empty.rewards.clear();run(empty,rules,command(empty,CampaignActionType::RequestAdvance));check(empty.phase==CityPhase::Between && !empty.skipConfirmation,"Empty reward screen did not advance directly.");
        const auto good=serializeCampaign(c);auto bad=good;bad[bad.size()/2]^=1;const auto prior=campaignHash(c);std::string error;
        check(!deserializeCampaign(bad,c,error) && campaignHash(c)==prior,"Corrupt campaign changed live state.");
        auto unsupported=good;unsupported[7]='2';check(!deserializeCampaign(unsupported,c,error),"Unsupported campaign envelope accepted.");
        std::cout<<"PASS campaign persistence: empty rewards, abandonment and corrupt snapshot rejection\n";
    }
    {
        CampaignRules missing(fights);auto c=missing.newGame(1,"missing-upgrade-hook");const auto before=serializeCampaign(c);
        check(!missing.apply(c,command(c,CampaignActionType::ChooseMayor,0,c.mayorOffers[0])).ok && serializeCampaign(c)==before,"Missing upgrade hook silently granted inert item.");
        std::cout<<"PASS campaign missing executor: acquisition rejects without an inert grant\n";
    }
    {
        auto c=mysteryFixture(rules,"C1-M-EXCHANGE");const auto generation=c.shopGeneration,position=c.route.position,money=c.fight.credits;
        check(c.eventShop.size()==3,"Night Exchange did not prepare its finite three products.");
        check(cityItem(data.recipes,c.eventShop[0].definition)->rarity==2 && cityItem(data.recipes,c.eventShop[1].definition)->rarity==3,"Night Exchange recipe tiers changed.");
        const auto stock=c.eventShop;auto open=command(c,CampaignActionType::OpenShop);open.eventShop=true;run(c,rules,open);roundtrip(c);
        check(c.eventShop[0].definition==stock[0].definition && c.eventShop[1].definition==stock[1].definition && c.eventShop[2].definition==stock[2].definition,"Reopening Mystery rerolled products.");
        const auto leave=command(c,CampaignActionType::LeaveMystery);run(c,rules,leave);const auto bytes=serializeCampaign(c);
        check(c.phase==CityPhase::Between && c.route.position==position+1 && c.shopGeneration==generation && c.fight.credits==money+10,"T18 noncombat Mystery applied combat restock/reward or missed salvage.");
        check(rules.apply(c,leave).replayed && serializeCampaign(c)==bytes,"Repeated Mystery completion duplicated salvage/progress.");
        open=command(c,CampaignActionType::OpenShop);open.eventShop=true;check(!rules.apply(c,open).ok && serializeCampaign(c)==bytes,"Closed Mystery merchant reopened.");
        std::cout<<"PASS campaign Night Exchange: finite stock, fixed tiers and noncombat completion\n";
    }
    {
        auto c=mysteryFixture(rules,"C1-M-TECH");check(c.rewards.size()==1 && c.rewards[0].choices.size()==3,"Calibration lacks fixed choices.");
        const auto selected=c.rewards[0].choices[0];c.fight.hp=8;const auto before=serializeCampaign(c);
        check(!rules.apply(c,command(c,CampaignActionType::AcceptCalibration,0,selected)).ok && serializeCampaign(c)==before,"Calibration paid lethal base cost.");
        run(c,rules,command(c,CampaignActionType::LeaveMystery));check(c.fight.hp==8,"Declining calibration changed HP.");
        auto hooks=transactionFixtureHooks(fights);hooks.acquireUpgrade=[](Campaign& state,const std::string& id,const CampaignAction& a){
            if(a.type==CampaignActionType::AcceptCalibration){if(state.fight.hp<=14)return Result{false,"Test-only additional acquisition cost.",{}};state.fight.hp-=14;}OwnedUpgrade u;u.id=id;u.order=state.fight.nextOrder++;state.fight.upgrades.push_back(u);return Result{true,{},{}};
        };
        CampaignRules costRules(fights,hooks);c=mysteryFixture(costRules,"C1-M-TECH");c.fight.hp=20;
        const auto offer=c.rewards[0].choices[0];const auto old=serializeCampaign(c);
        check(!costRules.apply(c,command(c,CampaignActionType::AcceptCalibration,0,offer)).ok && serializeCampaign(c)==old,"Calibration ignored item cost after its own eight HP.");
        c.fight.hp=23;run(c,costRules,command(c,CampaignActionType::AcceptCalibration,0,offer));check(c.fight.hp==1 && c.fight.upgrades.size()==2,"Combined legal calibration costs were not atomic.");roundtrip(c);
        std::cout<<"PASS campaign Technician: fixed offers, refusal and combined nonlethal costs\n";
    }
    {
        auto c=mysteryFixture(rules,"C1-M-PATROL");const auto position=c.route.position,stock=c.shopGeneration;
        run(c,rules,command(c,CampaignActionType::EnterPatrol));check(c.phase==CityPhase::Fight && !c.entry.empty() && c.route.position==position,"Patrol counted before combat.");
        const auto hash=stateHash(c.fight);roundtrip(c);run(c,rules,command(c,CampaignActionType::Continue));check(stateHash(c.fight)==hash && c.shopGeneration==stock,"Patrol Continue rerolled/restocked entry.");
        Campaign internal;std::string error;check(deserializeCampaign(c.entry,internal,error),error);const auto bytes=serializeCampaign(internal);
        check(!rules.apply(internal,command(internal,CampaignActionType::OpenShop)).ok && serializeCampaign(internal)==bytes,"Internal checkpoint was treated as playable state.");
        std::cout<<"PASS campaign Patrol: encounter entry, Continue and internal-checkpoint rejection\n";
    }
    {
        auto c=start(rules);enterRam(c,rules);c.fight.hp=1;combat(c,rules,Action::collect(0));combat(c,rules,Action::endTurn());
        if(c.phase==CityPhase::Fight){combat(c,rules,Action::collect(0));combat(c,rules,Action::endTurn());}
        check(c.phase==CityPhase::Defeated && c.route.position==1 && c.rewards.empty() && c.entry.empty() && c.cores.empty(),"Defeat granted loot, progressed or retained a Continue checkpoint.");roundtrip(c);
        std::cout<<"PASS campaign defeat: no loot, advancement or Continue checkpoint\n";
    }
    {
        auto old=start(rules,9);auto queued=command(old,CampaignActionType::Buy,product(old,ProductKind::Material));
        auto fresh=rules.newGame(9,"replacement-run");run(fresh,rules,command(fresh,CampaignActionType::ChooseMayor,0,fresh.mayorOffers[0]));
        const auto before=serializeCampaign(fresh);check(queued.sequence==fresh.nextTransaction,"Cross-run regression lacks matching sequence.");
        check(!rules.apply(fresh,queued).ok && serializeCampaign(fresh)==before,"Queued command crossed a New Game boundary.");
        queued.runId.clear();check(!rules.apply(fresh,queued).ok && serializeCampaign(fresh)==before,"Missing campaign command identity was accepted.");
        auto invalid=fresh;invalid.fight.hp=0;invalid.fight.phase=Phase::Defeat;rejectedSnapshot(invalid);
        invalid=fresh;invalid.phase=CityPhase::Defeated;rejectedSnapshot(invalid);
        enterRam(fresh,rules);invalid=fresh;++invalid.fight.seed;invalid.fight.encounter="uncommitted-encounter";rejectedSnapshot(invalid);
        invalid=fresh;auto& node=invalid.route.nodes[0];const auto selected=node.selected;
        std::size_t chosen=0,other=0;for(std::size_t i=0;i<node.offers.size();++i){if(node.offers[i].id==selected)chosen=i;else other=i;}
        std::swap(node.offers[chosen].formation,node.offers[other].formation);rejectedSnapshot(invalid);
        std::cout<<"PASS campaign identity: stale run commands and incompatible phase or formation reject\n";
    }
    std::cout<<"PASS "<<checks<<" campaign/offer/receipt assertions. Uses controlled test-only upgrade payloads and terminal ammo; no full-city balance, P08 or disk-durability claim.\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}

#include "overkill/campaign.hpp"
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>
using namespace overkill;
namespace {
int checks=0;
void check(bool value,const std::string& why){++checks;if(!value)throw std::runtime_error(why);}
CampaignAction command(const Campaign& c,CampaignActionType type,Id subject=0,const std::string& choice={}){CampaignAction a;a.runId=c.runId;a.sequence=c.nextTransaction;a.type=type;a.subject=subject;a.choice=choice;return a;}
CampaignResult run(Campaign& c,const CampaignRules& rules,const CampaignAction& a){auto result=rules.apply(c,a);check(result.ok,result.reason);return result;}
void roundtrip(Campaign& c){const auto bytes=serializeCampaign(c);Campaign restored;std::string error;check(deserializeCampaign(bytes,restored,error),error);check(serializeCampaign(restored)==bytes,"Reload changed the saved campaign.");c=std::move(restored);}
Campaign start(const CampaignRules& rules,std::uint64_t seed=1,const std::string& mayor="MY1-12"){auto c=rules.newGame(seed,"production-upgrades-"+std::to_string(seed));c.mayorOffers={mayor,"MY1-03","MY1-01"};run(c,rules,command(c,CampaignActionType::ChooseMayor,0,mayor));return c;}
void buy(Campaign& c,const CampaignRules& rules,ProductKind kind,const std::string& id,Id exchange=0){
    // The product is injected at zero price to isolate real acquisition effects.
    // This is not a naturally rolled shop or an economic/balance simulation.
    const Id product=c.nextId++;c.shop.push_back({product,kind,id,0,1,0});auto a=command(c,CampaignActionType::Buy,product);a.exchange=exchange;run(c,rules,a);
}
void fullMemory(Campaign& c){while(c.fight.memory.size()<20)c.fight.memory.push_back({c.fight.nextId++,"SH001",0,0,0});}
Id enter(Campaign& c,const CampaignRules& rules){for(const auto& o:routeOffers(c.route))if(o.kind==EncounterKind::Regular && o.formation=="C1-F-RAM"){const auto id=o.id;run(c,rules,command(c,CampaignActionType::EnterOffer,id));return id;}for(const auto& o:routeOffers(c.route))if(o.kind==EncounterKind::Regular){const auto id=o.id;run(c,rules,command(c,CampaignActionType::EnterOffer,id));return id;}throw std::runtime_error("No Regular fight.");}
void combat(Campaign& c,const CampaignRules& rules,const Action& a){auto commandValue=command(c,CampaignActionType::Combat);commandValue.combat=a;run(c,rules,commandValue);}
void win(Campaign& c,const CampaignRules& rules,const Rules& fights){
    if(c.fight.phase==Phase::Collection)combat(c,rules,Action::collect(0));
    while(c.phase==CityPhase::Fight){
        Id enemy=0;for(const auto& e:c.fight.enemies)if(!e.dead && !e.escaped){enemy=e.id;break;}check(enemy!=0,"Controlled win lost its target.");
        const auto before=c.fight.nextId;const auto grant=fights.grantPlainPart(c.fight,Kind::Ammo,500,"campaign-upgrade-test");check(grant.ok,grant.reason);
        Id ammo=0;for(const auto& p:c.fight.parts)if(p.id>=before && p.kind==Kind::Ammo)ammo=p.id;
        combat(c,rules,Action::load({ammo}));combat(c,rules,Action::fire(enemy));
    }
    check(c.phase==CityPhase::Rewards,"Controlled fight did not reach rewards.");
}
void leaveRewards(Campaign& c,const CampaignRules& rules){run(c,rules,command(c,CampaignActionType::RequestAdvance));if(c.skipConfirmation)run(c,rules,command(c,CampaignActionType::ConfirmAdvance));}
void pick(Campaign& c,const CampaignRules& rules,Id exchange=0){check(!c.upgradeOffers.empty(),"No upgrade offer to select.");const auto offer=c.upgradeOffers.front();auto a=command(c,CampaignActionType::ResolveUpgradeOffer,offer.id,offer.candidates[static_cast<std::size_t>(offer.index)].front());a.exchange=exchange;run(c,rules,a);}
}
int main(){try{
    Rules fights;CampaignRules rules(fights,cinderwallUpgradeHooks(fights));
    {
        auto c=start(rules,1,"MY1-06");check(c.fight.upgrades.size()==1 && c.upgradeOffers.size()==1,"Mayor acquisition did not create one saved nested offer.");
        check(c.upgradeOffers[0].candidates[0].size()==3,"Blueprint Annex lost its three choices.");for(const auto& id:c.upgradeOffers[0].candidates[0])check(id.substr(0,2)=="SH" && cityItem(rules.content().recipes,id)->rarity==2,"Mayor recipe filter changed.");
        roundtrip(c);pick(c,rules);check(c.fight.memory.size()==13 && upgradeGeneralMemoryBonus(c.fight)==4 && c.upgradeOffers.empty(),"Mayor memory/recipe result failed.");roundtrip(c);
    }
    {
        auto c=start(rules,2);buy(c,rules,ProductKind::Upgrade,"UGS-089");const auto fixed=c.upgradeOffers[0].candidates;check(fixed.size()==3,"Archive Cache lacks three sequential offers.");roundtrip(c);
        auto a=command(c,CampaignActionType::ResolveUpgradeOffer,c.upgradeOffers[0].id,fixed[0][0]);run(c,rules,a);const auto saved=serializeCampaign(c);
        check(rules.apply(c,a).replayed && serializeCampaign(c)==saved,"Nested-offer receipt granted twice.");a.decline=true;check(!rules.apply(c,a).ok && serializeCampaign(c)==saved,"Changed decline payload reused a receipt.");
        check(c.upgradeOffers[0].index==1 && c.upgradeOffers[0].candidates==fixed,"Nested advancement rerolled candidates.");pick(c,rules);roundtrip(c);pick(c,rules);check(c.fight.memory.size()==15 && c.upgradeOffers.empty() && c.fight.upgradeRequests.empty(),"Archive Cache stopped before all choices.");
    }
    {
        auto c=start(rules,3);fullMemory(c);buy(c,rules,ProductKind::Upgrade,"UGS-100");buy(c,rules,ProductKind::Recipe,"SH074");
        check(c.fight.memory.size()==21 && c.fight.memory.back().storage==MemoryKind::Utility,"Dedicated Utility slot was not used.");roundtrip(c);
        const Id product=c.nextId++;c.shop.push_back({product,ProductKind::Recipe,"SH001",0,1,0});auto a=command(c,CampaignActionType::Buy,product);a.exchange=c.fight.memory.back().id;const auto before=serializeCampaign(c);
        check(!rules.apply(c,a).ok && serializeCampaign(c)==before,"Ammo overwrote a Utility-only slot.");a.exchange=c.fight.memory.front().id;run(c,rules,a);check(c.fight.memory.size()==21,"General memory exchange grew inventory.");roundtrip(c);
        Id utility=0;for(const auto& copy:c.fight.memory)if(copy.storage==MemoryKind::General && fights.recipe(copy.recipe)->kind==Kind::Utility){utility=copy.id;break;}
        check(utility!=0,"Missing existing Utility fixture copy.");run(c,rules,command(c,CampaignActionType::MoveRecipeCopy,utility,"utility"));
        check(rules.hasFreeMemory(c,"SH001"),"Moving an existing Utility failed to free a general slot.");roundtrip(c);enter(c,rules);
        const auto during=serializeCampaign(c);check(!rules.apply(c,command(c,CampaignActionType::MoveRecipeCopy,utility,"general")).ok && serializeCampaign(c)==during,"Memory slots were rearranged during a fight.");
    }
    {
        auto c=start(rules,4);fullMemory(c);const auto source=c.fight.memory[0].id;const auto definition=c.fight.memory[0].recipe;const auto other=c.fight.memory[1].id;buy(c,rules,ProductKind::Upgrade,"UGS-032");
        auto a=command(c,CampaignActionType::ResolveUpgradeOffer,c.upgradeOffers[0].id);a.target=source;run(c,rules,a);check(c.fight.memory.size()==20 && !c.upgradeOffers[0].selected.empty(),"Copy exchange was not staged.");roundtrip(c);
        run(c,rules,command(c,CampaignActionType::CancelUpgradeExchange,c.upgradeOffers[0].id));check(c.upgradeOffers[0].selected.empty() && c.fight.memory.size()==20,"Cancelling exchange acquired or abandoned the payload.");a.sequence=c.nextTransaction;run(c,rules,a);
        a.sequence=c.nextTransaction;a.exchange=source;const auto before=serializeCampaign(c);check(!rules.apply(c,a).ok && serializeCampaign(c)==before,"Carbon Copier exchanged its own selected source.");
        a.exchange=other;run(c,rules,a);check(c.fight.memory.size()==20 && c.fight.memory.back().recipe==definition && c.fight.memory.back().cooldown==0 && c.upgradeOffers.empty(),"Ready-copy exchange failed.");roundtrip(c);
    }
    {
        auto c=start(rules,5);buy(c,rules,ProductKind::Upgrade,"UGS-133");enter(c,rules);check(c.upgradeOffers.size()==1,"Borrowed fight-start offer absent.");const auto choices=c.upgradeOffers[0].candidates;
        for(const auto& id:choices[0])check(id.substr(0,2)=="SH" && cityItem(rules.content().recipes,id)->rarity==1,"Borrowed filter changed.");pick(c,rules);check(c.fight.memory.back().storage==MemoryKind::Borrowed && c.fight.memory.size()==13,"Borrowed slot not separate.");roundtrip(c);
        run(c,rules,command(c,CampaignActionType::Continue));check(c.upgradeOffers[0].candidates==choices && c.fight.memory.size()==12,"Continue retained or rerolled borrowed acquisition.");pick(c,rules);win(c,rules,fights);check(c.fight.memory.size()==12,"Borrowed copy survived fight completion.");roundtrip(c);
    }
    for(bool reverse:{false,true}){
        auto c=start(rules,reverse?7:6);buy(c,rules,ProductKind::Upgrade,reverse?"UGS-101":"UGS-031");buy(c,rules,ProductKind::Upgrade,reverse?"UGS-031":"UGS-101");buy(c,rules,ProductKind::Upgrade,"UGS-148");enter(c,rules);win(c,rules,fights);
        int count=0;Id selected=0;std::string marked;for(const auto& r:c.rewards)if(r.kind==RewardKind::Recipe){++count;check(r.choices.size()==4 && r.lightTouch.size()==1,"Normal offer modifier depends on upgrade ownership order.");if(!selected){selected=r.id;marked=r.lightTouch[0];}}
        check(count==2,"Double Regular reward lost an independent offer.");roundtrip(c);run(c,rules,command(c,CampaignActionType::OpenRecipes,selected));run(c,rules,command(c,CampaignActionType::ClaimReward,selected,marked));
        check(std::any_of(c.fight.memory.back().tags.begin(),c.fight.memory.back().tags.end(),[](const RecipeTag& t){return t.kind==RecipeTagKind::LightTouch;}),"Marked acceptance failed to tag its physical copy.");roundtrip(c);
    }
    {
        auto c=start(rules,8);buy(c,rules,ProductKind::Upgrade,"UGS-131");enter(c,rules);win(c,rules,fights);check(c.revealNextReward,"Post-fight stock did not reveal upcoming information.");leaveRewards(c,rules);
        RouteOffer next;for(const auto& o:routeOffers(c.route))if(o.kind==EncounterKind::Regular && !rules.revealedRecipe(c,o).empty()){next=o;break;}
        check(next.id!=0,"Fixture has no shared upcoming option.");const auto before=serializeCampaign(c);const auto shown=rules.revealedRecipe(c,next);check(rules.revealedRecipe(c,next)==shown && serializeCampaign(c)==before,"Reward information query changed state.");
        run(c,rules,command(c,CampaignActionType::EnterOffer,next.id));win(c,rules,fights);bool found=false;for(const auto& r:c.rewards)if(r.kind==RewardKind::Recipe && r.normalOffer)found=std::find(r.choices.begin(),r.choices.end(),shown)!=r.choices.end();check(found,"Preview did not match the actual next reward.");
    }
    {
        auto c=start(rules,9);const auto offer=routeOffers(c.route).front();const auto rng=c.route.rng.state;
        run(c,rules,command(c,CampaignActionType::PreviewRouteReplacement,offer.id));const auto saved=c.routePreviews.front();check(ownedUpgrade(c.fight,"MY1-12")->charges==3 && c.route.rng.state==rng,"Route preview spent a charge or RNG.");roundtrip(c);
        run(c,rules,command(c,CampaignActionType::ReplaceRouteOffer,offer.id));check(ownedUpgrade(c.fight,"MY1-12")->charges==2 && routeOffers(c.route).front().formation==saved.formation,"Route commitment did not install the preview.");
        const auto before=serializeCampaign(c);check(!rules.apply(c,command(c,CampaignActionType::ReplaceRouteOffer,offer.id)).ok && serializeCampaign(c)==before,"A route pass rerolled its spent option.");roundtrip(c);
    }
    {
        auto c=start(rules,10,"MY1-14");enter(c,rules);win(c,rules,fights);Id reward=0;for(const auto& r:c.rewards)if(r.kind==RewardKind::Recipe)reward=r.id;
        run(c,rules,command(c,CampaignActionType::OpenRecipes,reward));const auto seen=c.profile.recipes;run(c,rules,command(c,CampaignActionType::RerollRecipeReward,reward));
        for(const auto& id:seen)check(std::find(c.profile.recipes.begin(),c.profile.recipes.end(),id)!=c.profile.recipes.end(),"Reward replacement erased Collection knowledge.");roundtrip(c);
        const auto before=serializeCampaign(c);check(!rules.apply(c,command(c,CampaignActionType::RerollRecipeReward,reward)).ok && serializeCampaign(c)==before,"Spectrometer rolled twice for one victory.");
    }
    {
        auto c=start(rules,11);buy(c,rules,ProductKind::Upgrade,"UGS-073");enter(c,rules);win(c,rules,fights);leaveRewards(c,rules);c.fight.hp=50;
        buy(c,rules,ProductKind::Upgrade,"UGS-072");check(c.fight.hp==53,"Prior buyer entitlement did not heal on upgrade purchase.");
        check(upgradeCounter(*ownedUpgrade(c.fight,"UGS-072"),"disabled")==0,"New Shop Embargo heard its own past purchase.");
        buy(c,rules,ProductKind::Recipe,"SH001");check(upgradeCounter(*ownedUpgrade(c.fight,"UGS-072"),"disabled")==1,"Existing Shop Embargo missed a later purchase.");roundtrip(c);
    }
    {
        auto c=start(rules,12);buy(c,rules,ProductKind::Upgrade,"UGS-057");buy(c,rules,ProductKind::Upgrade,"UGS-149");check(c.route.safeMysteries,"Safe Signal Survey did not bind route generation.");
        while(c.route.position<12){
            for(const auto& offer:routeOffers(c.route))if(offer.kind==EncounterKind::Mystery){const auto before=serializeCampaign(c);const auto category=rules.revealedMysteryCategory(c,offer.id);check(category=="merchant" || category=="calibration","Badge exposed the wrong authored category.");check(serializeCampaign(c)==before,"Badge information changed the route.");}
            // Route traversal here isolates informational effects; no combat claim.
            std::string error;check(selectRouteOffer(c.route,routeOffers(c.route).front().id,error),error);check(completeRouteNode(c.route,error),error);
        }
        roundtrip(c);
    }
    {
        auto c=start(rules,13,"MY1-13");const auto* source=ownedUpgrade(c.fight,"MY1-13");check(source!=nullptr,"Missing subscription source.");
        for(auto& copy:c.fight.memory)copy.tags.push_back({"MY1-13",source->order,RecipeTagKind::Subscription,1,0,0});
        for(int i=0;i<2;++i){CampaignOffer earned;earned.id=c.nextId++;earned.source="MY1-13";earned.kind=UpgradeRequestKind::Subscription;earned.optional=false;earned.deferred=true;c.upgradeOffers.push_back(earned);}roundtrip(c);
        buy(c,rules,ProductKind::Recipe,"SH001");check(!c.upgradeOffers[0].deferred && !c.upgradeOffers[1].deferred,"Earned selections did not wake for a new copy.");
        auto a=command(c,CampaignActionType::ResolveUpgradeOffer,c.upgradeOffers.front().id);a.target=c.fight.memory.back().id;a.material=0;run(c,rules,a);
        check(c.upgradeOffers.size()==1 && c.upgradeOffers[0].deferred && c.upgradeOffers[0].copies.empty(),"Second entitlement was trapped on the already subscribed copy.");roundtrip(c);
        buy(c,rules,ProductKind::Recipe,"SH002");a=command(c,CampaignActionType::ResolveUpgradeOffer,c.upgradeOffers.front().id);a.target=c.fight.memory.back().id;a.material=1;run(c,rules,a);check(c.upgradeOffers.empty(),"Retained subscription could not resolve later.");roundtrip(c);
    }
    std::cout<<"PASS "<<checks<<" production campaign-upgrade assertions; controlled shop grants and terminal ammo, not city balance.\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL after "<<checks<<": "<<e.what()<<'\n';return 1;}}

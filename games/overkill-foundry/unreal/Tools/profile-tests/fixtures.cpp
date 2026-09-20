#include "overkill/campaign_session.hpp"
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>

// Prepared native-interface inspection only. The 1-HP fight and complete
// Collection are not earned progression. Never writes to a player profile.
int main(int argc,char** argv){try{
    if(argc!=2)throw std::runtime_error("Usage: foundry_profile_fixtures NEW_OUTPUT_DIRECTORY");
    const std::filesystem::path directory=argv[1];
    if(std::filesystem::exists(directory))throw std::runtime_error("Choose a new output directory; existing saves are never replaced.");
    std::filesystem::create_directories(directory);
    overkill::Rules fights;overkill::CampaignRules rules(fights,overkill::cinderwallUpgradeHooks(fights));
    auto campaign=rules.newGame(20260920,"prepared-profile-interface");
    auto apply=[&](overkill::Campaign& state,overkill::CampaignAction action){
        action.runId=state.runId;action.sequence=state.nextTransaction;const auto result=rules.apply(state,action);
        if(!result.ok)throw std::runtime_error(result.reason);
    };
    auto write=[&](const std::string& name,const overkill::Campaign& state){
        const auto path=directory/(name+".ofsave");
        const auto saved=overkill::SaveStore(path).commit(overkill::serializeCampaign(state),{});
        if(!saved.ok)throw std::runtime_error(saved.error);
        overkill::CampaignSession loaded(path,rules);const auto result=loaded.load();
        if(!result.ok)throw std::runtime_error(result.reason);
        std::cout<<name<<" hash="<<overkill::campaignHash(loaded.state())<<" discovered="<<state.profile.recipes.size()<<" memory="<<state.fight.memory.size()<<" phase="<<static_cast<int>(state.phase)<<'\n';
    };
    // This seeded offer includes the already verified non-nested Engine gift.
    overkill::CampaignAction action;action.type=overkill::CampaignActionType::ChooseMayor;action.choice="MY1-13";
    apply(campaign,action);
    action={};action.type=overkill::CampaignActionType::OpenShop;apply(campaign,action);
    write("seen-shop",campaign);
    auto collection=campaign;
    for(const auto& recipe:fights.content())if(std::find(collection.profile.recipes.begin(),collection.profile.recipes.end(),recipe.id)==collection.profile.recipes.end())collection.profile.recipes.push_back(recipe.id);
    write("collection-all-prepared",collection);
    action={};action.type=overkill::CampaignActionType::EnterOffer;
    for(const auto& offer:overkill::routeOffers(campaign.route))if(offer.formation=="C1-F-MITE-RAM"){action.subject=offer.id;break;}
    if(!action.subject)for(const auto& offer:overkill::routeOffers(campaign.route))if(offer.kind==overkill::EncounterKind::Regular){action.subject=offer.id;break;}
    apply(campaign,action);
    action={};action.type=overkill::CampaignActionType::Combat;action.combat=overkill::Action::collect(3);apply(campaign,action);
    campaign.fight.hp=1;write("defeat-ready-prepared",campaign);
    action.combat=overkill::Action::endTurn();apply(campaign,action);
    if(campaign.phase!=overkill::CityPhase::Defeated)throw std::runtime_error("Prepared actual enemy-turn action did not end this campaign.");
    write("defeat-ended",campaign);
    std::cout<<"PREPARED_PROFILE_FIXTURES_OK earned_progression=0\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}

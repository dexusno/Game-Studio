#include "overkill/campaign_session.hpp"
#include "overkill/upgrades.hpp"
#include "overkill/robots.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>

// Deliberately prepared UI datasets, not campaign/balance evidence. They use
// production metadata, validation and save envelopes; they never touch a player
// profile. Rendering/loading these files must be labelled fixture inspection.
int main(int argc,char** argv){
 try{
    if(argc!=2)throw std::runtime_error("Usage: foundry_ui_fixtures NEW_OUTPUT_DIRECTORY");
    const std::filesystem::path dir=argv[1];
    if(std::filesystem::exists(dir))throw std::runtime_error("Use a new output directory; fixture generation never replaces saves.");
    std::filesystem::create_directories(dir);
    overkill::Rules fights;overkill::CampaignRules rules(fights,overkill::cinderwallUpgradeHooks(fights));
    auto base=[&](const std::string& mode){return rules.newGame(20260920,"ui-fixture-"+mode);};
    auto apply=[&](overkill::Campaign& c,overkill::CampaignAction a){a.runId=c.runId;a.sequence=c.nextTransaction;const auto r=rules.apply(c,a);if(!r.ok)throw std::runtime_error(r.reason);};
    auto write=[&](const std::string& name,const overkill::Campaign& c){
        const auto path=dir/(name+".ofsave");overkill::SaveStore store(path);const auto saved=store.commit(overkill::serializeCampaign(c),"");if(!saved.ok)throw std::runtime_error(saved.error);
        overkill::CampaignSession session(path,rules);const auto loaded=session.load();if(!loaded.ok)throw std::runtime_error(loaded.reason);
        std::cout<<name<<" "<<overkill::campaignHash(session.state())<<" memory="<<c.fight.memory.size()<<" parts="<<c.fight.parts.size()<<"\n";
    };
    auto arrival=base("arrival");write("arrival",arrival);
    auto pack=base("pack");overkill::CampaignAction a;a.type=overkill::CampaignActionType::ChooseMayor;a.choice="MY1-15";apply(pack,a);write("pack-choice",pack);
    auto precision=base("precision");
    for(std::uint64_t seed=1;std::find(precision.mayorOffers.begin(),precision.mayorOffers.end(),"MY1-17")==precision.mayorOffers.end() && seed<512;++seed)
        precision=rules.newGame(seed,"ui-fixture-precision");
    a.choice="MY1-17";apply(precision,a);
    a={};a.type=overkill::CampaignActionType::EnterOffer;
    for(const auto& o:overkill::routeOffers(precision.route))if(o.formation=="C1-F-MITE-RAM"){a.subject=o.id;break;}
    if(!a.subject)for(const auto& o:overkill::routeOffers(precision.route))if(o.kind==overkill::EncounterKind::Regular){a.subject=o.id;break;}
    apply(precision,a);write("precision-entry",precision);
    auto composition=precision;
    composition.fight.enemies=overkill::makeCinderwallFormation("C1-F-MITE-RAM",composition.fight.seed,"prepared-camera-composition",composition.fight.nextId);
    auto extra=overkill::makeCinderwallFormation("C1-F-MITE-RAM",composition.fight.seed,"prepared-camera-composition",composition.fight.nextId);
    // Synthetic layout stress only, not a roster claim. Keep the source rule
    // that only one live Mite can exist: the third body is another Ram.
    composition.fight.enemies.push_back(extra.back());
    a={};a.type=overkill::CampaignActionType::Combat;a.combat=overkill::Action::collect(3);apply(composition,a);
    for(int i=0;i<6;++i){const auto r=fights.grantPlainPart(composition.fight,overkill::Kind::Ammo,4,"Prepared visual fixture");if(!r.ok)throw std::runtime_error(r.reason);}
    write("camera-three-robots",composition);
    auto precisionRetry=precision;
    a={};a.type=overkill::CampaignActionType::Combat;a.combat=overkill::Action::collect(3,0);apply(precisionRetry,a);write("precision-retry",precisionRetry);
    auto full=base("memory");a={};a.type=overkill::CampaignActionType::ChooseMayor;a.choice="MY1-13";apply(full,a);
    while(full.fight.memory.size()<20){overkill::RecipeCopy c;c.id=full.fight.nextId++;c.recipe="SH001";full.fight.memory.push_back(c);}
    write("full-memory",full);
    // A zero-price injected product is confined to this prepared test save.
    // Acquisition still traverses the ordinary canonical campaign request path.
    auto nested=full;nested.runId="ui-fixture-nested";
    nested.fight.credits=500;
    overkill::Product p;p.id=nested.nextId++;p.kind=overkill::ProductKind::Upgrade;p.definition="UGS-089";p.quantity=1;p.price=0;nested.shop.push_back(p);
    a={};a.type=overkill::CampaignActionType::Buy;a.subject=p.id;apply(nested,a);write("nested-offer",nested);
    auto stress=full;stress.runId="ui-fixture-parts";
    a={};a.type=overkill::CampaignActionType::EnterOffer;for(const auto& o:overkill::routeOffers(stress.route))if(o.kind==overkill::EncounterKind::Regular){a.subject=o.id;break;}apply(stress,a);
    a={};a.type=overkill::CampaignActionType::Combat;a.combat=overkill::Action::collect(3);apply(stress,a);
    for(int i=0;i<24;++i){auto r=fights.grantPlainPart(stress.fight,overkill::Kind::Ammo,4,"UI fixture");if(!r.ok)throw std::runtime_error(r.reason);r=fights.grantPlainPart(stress.fight,overkill::Kind::Shield,4,"UI fixture",true);if(!r.ok)throw std::runtime_error(r.reason);}
    std::vector<overkill::Id> bullet;for(const auto& p:stress.fight.parts)if(p.kind==overkill::Kind::Ammo)bullet.push_back(p.id);
    const auto loaded=fights.apply(stress.fight,overkill::Action::load(bullet));if(!loaded.ok)throw std::runtime_error(loaded.reason);
    write("parts-24-each",stress);
    std::cout<<"PREPARED_UI_FIXTURES_OK production_run=0\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}

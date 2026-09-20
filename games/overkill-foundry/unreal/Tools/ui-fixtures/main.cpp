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
    // Advanced-control saves deliberately provide resources and recipe copies.
    // They prove UI requests and source validation, not earned progression.
    auto combat=[&](overkill::Campaign& c,const overkill::Action& action){overkill::CampaignAction command;command.type=overkill::CampaignActionType::Combat;command.combat=action;apply(c,command);};
    auto grant=[&](overkill::Campaign& c,const std::string& recipe){const auto first=c.fight.nextId;const auto result=fights.grantPart(c.fight,recipe);if(!result.ok)throw std::runtime_error(result.reason);for(const auto& part:c.fight.parts)if(part.id>=first&&part.recipe==recipe)return part.id;throw std::runtime_error("Missing prepared part "+recipe);};
    auto remember=[&](overkill::Campaign& c,const std::vector<std::string>& recipes){c.fight.memory.clear();for(const auto& recipe:recipes){overkill::RecipeCopy copy;copy.id=c.fight.nextId++;copy.recipe=recipe;c.fight.memory.push_back(copy);}};
    auto protect=[&](overkill::Campaign& c){const auto result=fights.grantPlainPart(c.fight,overkill::Kind::Shield,80,"Prepared inspection protection",true);if(!result.ok)throw std::runtime_error(result.reason);};
    auto options=precision;
    combat(options,overkill::Action::collect(3));options.fight.materials={100,100,100,100,100};options.fight.heat=8;
    remember(options,{"SH080","SH101","MA105","SH103","MA038","SH002","SH026","SH057","SH121","SH032","SH071","SH122","MA030","MA066","MA095","MA100","MA117","SH069","SH106","SH108"});
    protect(options);options.fight.enemies.front().burn=4;
    for(const char* recipe:{"SH001","SH009","SH002","SH002","MA096","MA022","SH085","MA061","SH036","MA052","SH103"})grant(options,recipe);
    write("recipe-options",options);
    auto heavy=precision;combat(heavy,overkill::Action::collect(3));protect(heavy);
    const auto magnet=grant(heavy,"SH105");overkill::Action activate;activate.type=overkill::ActionType::Activate;activate.subject=magnet;combat(heavy,activate);combat(heavy,overkill::Action::endTurn());
    write("recipe-heavy",heavy);
    auto shot=precision;
    shot.fight.enemies=overkill::makeCinderwallFormation("C1-F-MITE-RAM",shot.fight.seed,"prepared-recipe-shot",shot.fight.nextId);
    auto extraShot=overkill::makeCinderwallFormation("C1-F-MITE-RAM",shot.fight.seed,"prepared-recipe-shot",shot.fight.nextId);shot.fight.enemies.push_back(extraShot.back());
    combat(shot,overkill::Action::collect(3));shot.fight.materials={100,100,100,100,100};shot.fight.heat=8;protect(shot);
    for(const char* recipe:{"SH017","SH051","SH110","SH002","SH002","SH001"})grant(shot,recipe);
    const auto delivery=grant(shot,"SH072");activate.subject=delivery;combat(shot,activate);
    const auto holdfast=grant(shot,"MA048");combat(shot,overkill::Action::install(holdfast));
    write("recipe-shot-synthetic-three",shot);
    auto discount=base("discount");
    for(std::uint64_t seed=1;std::find(discount.mayorOffers.begin(),discount.mayorOffers.end(),"MY1-09")==discount.mayorOffers.end()&&seed<512;++seed)discount=rules.newGame(seed,"ui-fixture-discount");
    a={};a.type=overkill::CampaignActionType::ChooseMayor;a.choice="MY1-09";apply(discount,a);
    a={};a.type=overkill::CampaignActionType::EnterOffer;for(const auto& offer:overkill::routeOffers(discount.route))if(offer.formation=="C1-F-MITE-RAM"){a.subject=offer.id;break;}
    if(!a.subject)for(const auto& offer:overkill::routeOffers(discount.route))if(offer.kind==overkill::EncounterKind::Regular){a.subject=offer.id;break;}apply(discount,a);
    combat(discount,overkill::Action::collect(3));discount.fight.materials={100,100,100,100,100};
    remember(discount,{"SH013","SH004","SH019","SH002","SH001"});
    for(std::size_t i=0;i<3;++i)combat(discount,overkill::Action::craft(discount.fight.memory[i].id));
    write("recipe-discount",discount);
    std::cout<<"PREPARED_UI_FIXTURES_OK production_run=0\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}

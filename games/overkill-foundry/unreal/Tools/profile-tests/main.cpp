#include "FoundryProfiles.h"
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
int checks=0;
void check(bool ok,const std::string& label){++checks;if(!ok)throw std::runtime_error(label);}
std::string id(int value){std::ostringstream out;out<<std::hex<<std::setw(32)<<std::setfill('0')<<value;return out.str();}
void corrupt(const std::filesystem::path& path){std::ofstream out(path,std::ios::binary|std::ios::trunc);out<<"deliberately corrupt QA envelope";}
std::string bytes(const std::filesystem::path& path){std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
overkill::CampaignAction command(const foundry_profiles::Store& store,overkill::CampaignActionType type){
    overkill::CampaignAction action;action.type=type;action.runId=store.state().runId;action.sequence=store.state().nextTransaction;return action;
}
}
int main(){try{
    namespace fs=std::filesystem;using foundry_profiles::Store;using overkill::SaveStore;
    const auto root=fs::current_path()/"profile-test-artifacts"/std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    fs::create_directories(root);
    overkill::Rules fights;overkill::CampaignRules rules(fights,overkill::cinderwallUpgradeHooks(fights));
    const auto defaultPath=root/"profile.ofsave";
    Store store(defaultPath,rules,false);
    check(!store.fixed()&&!store.loaded()&&store.activeId()=="default","fresh default selection");
    check(store.profiles().size()==1&&!store.hasSave(),"fresh profile browser does not create a run");
    check(store.newGame(3001,"default-first").ok,"legacy default New Game");
    const auto defaultBytes=bytes(defaultPath);
    check(store.create(id(1),"  Klaus / test  ").ok,"names are labels, not path components");
    check(!store.create(id(1),"Duplicate identity").ok,"duplicate identity never overwrites");
    check(!store.create("../escape","Escape").ok,"path traversal identity rejected");
    check(!store.create(id(2),"   ").ok,"blank profile name rejected");
    check(!store.create(id(2),"line\nname").ok,"control character name rejected");
    check(!store.create(id(2),"Name \xc2\x80").ok,"UTF-8 U+0080 C1 control rejected");
    check(!fs::exists(root/"Profiles"/id(2)),"U+0080 rejection creates no directory");
    check(!store.create(id(2),"Name \xc2\x9f").ok,"UTF-8 U+009F C1 control rejected");
    check(!fs::exists(root/"Profiles"/id(2)),"U+009F rejection creates no directory");
    check(!store.create(id(2),std::string("\xc0\xaf",2)).ok,"malformed UTF-8 name rejected");
    check(!store.create(id(2),std::string(161,'x')).ok,"oversized profile name rejected");
    check(store.select(id(1)).ok&&!store.loaded()&&!store.hasSave(),"select empty profile does not begin or overwrite a run");
    check(store.activeName()=="Klaus / test","trimmed display name persists");
    check(bytes(defaultPath)==defaultBytes,"creating/selecting an independent profile leaves default untouched");
    check(store.newGame(3002,"profile-one").ok,"new independent campaign");
    const auto starters=store.state().profile.recipes;
    auto mayor=command(store,overkill::CampaignActionType::ChooseMayor);bool chose=false;
    for(const auto& choice:store.state().mayorOffers){
        auto candidate=store.state();mayor.choice=choice;
        const auto result=rules.apply(candidate,mayor);
        if(result.ok&&candidate.fight.upgradeChoices.empty()&&candidate.upgradeOffers.empty()){chose=store.apply(mayor).ok;break;}
    }
    check(chose,"choose actual saved Mayor offer with production hooks");
    check(store.apply(command(store,overkill::CampaignActionType::OpenShop)).ok,"discover finite shop recipes through production command");
    const auto discovered=store.state().profile.recipes;
    check(discovered.size()>starters.size(),"Collection gains actually seen recipes");
    const auto firstProfilePath=store.path();
    const auto beforeSwitch=bytes(firstProfilePath);
    check(store.select("default").ok&&store.state().runId=="default-first","independent default campaign restored");
    check(store.state().profile.recipes==starters,"profile discoveries do not leak across campaigns");
    check(bytes(firstProfilePath)==beforeSwitch,"selection does not mutate departing campaign");
    check(store.select(id(1)).ok&&store.state().profile.recipes==discovered,"profile retains discoveries after switch");
    check(store.newGame(3003,"profile-one-restart").ok&&store.state().profile.recipes==discovered,"replacement New Game preserves saved discovery facts");
    Store reopened(defaultPath,rules,false);
    check(reopened.activeId()==id(1)&&!reopened.loaded(),"last selected identity survives process construction without auto-start");
    check(reopened.load().ok&&reopened.state().runId=="profile-one-restart","selected campaign survives reopening");
    check(reopened.state().profile.recipes==discovered,"Collection survives load/replacement");

    // No hard profile-count cap. Every created record remains independently selectable.
    for(int i=2;i<=33;++i)check(store.create(id(i),"Profile "+std::to_string(i)).ok,"create profile beyond a fixed slot limit");
    check(store.profiles().size()==34,"all 34 profiles remain visible");
    check(store.select(id(33)).ok&&!store.loaded(),"profile beyond first page is selectable");
    check(store.create(id(34),"Mara \xc3\xa6\xc3\xb8\xc3\xa5 \xe2\x9a\x99").ok,"UTF-8 display name supported");
    check(store.select(id(34)).ok&&store.activeName()=="Mara \xc3\xa6\xc3\xb8\xc3\xa5 \xe2\x9a\x99","UTF-8 profile name round-trips");

    // Fixed paths used by probes never read remembered choices or expose profiles.
    Store fixed(defaultPath,rules);
    check(fixed.fixed()&&fixed.path()==fs::absolute(defaultPath)&&fixed.profiles().size()==1,"fixed path bypasses profile preference");
    check(!fixed.create(id(90),"No").ok&&!fixed.select(id(1)).ok,"fixed path cannot create/switch profiles");
    check(fixed.load().ok&&fixed.state().runId=="default-first","fixed path loads exact requested campaign");

    // Corrupt campaigns cannot displace a usable active profile.
    check(store.select("default").ok,"select known working profile");
    const auto brokenPath=root/"Profiles"/id(2)/"profile.ofsave";corrupt(brokenPath);
    check(!store.select(id(2)).ok&&store.activeId()=="default"&&store.state().runId=="default-first","invalid candidate leaves active campaign untouched");
    // Metadata damage does not make an otherwise valid campaign unreachable.
    const auto namePath=root/"Profiles"/id(1)/"name.ofmeta";corrupt(namePath);
    auto profiles=store.profiles();auto found=std::find_if(profiles.begin(),profiles.end(),[](const auto& p){return p.id==id(1);});
    check(found!=profiles.end()&&found->name=="Recovered profile 00000000"&&!found->note.empty(),"corrupt name has a visible fallback");
    check(store.select(id(1)).ok&&store.state().profile.recipes==discovered,"missing name cannot erase valid discovery facts");

    // A corrupt primary recovers the previous envelope without a speculative write.
    check(store.newGame(3004,"profile-one-latest").ok,"prepare second valid recovery checkpoint");
    corrupt(firstProfilePath);
    check(store.select("default").ok&&store.select(id(1)).ok,"profile selection uses standard previous-save recovery");
    check(store.state().runId=="profile-one-restart"&&store.notice().find("Recovered")!=std::string::npos,"recovered run and notice are accurate");
    check(store.newGame(3005,"profile-one-repaired").ok,"ordinary replacement repairs recovered profile");

    Store concurrent(defaultPath,rules,false);check(concurrent.load().ok,"concurrent session loads same commit");
    check(store.newGame(3006,"winner").ok,"first writer commits");
    check(!concurrent.newGame(3007,"stale-writer").ok&&concurrent.state().runId=="winner","profile wrapper preserves session CAS conflict/reconciliation");
    // Name creation uses the same atomic storage and reconciles only its exact payload.
    for(int point=0;point<4;++point){
        const int value=100+point;
        const auto created=store.create(id(value),"Fault "+std::to_string(point),[point](overkill::SavePoint at){if(static_cast<int>(at)==point)throw std::runtime_error("Injected metadata write failure");});
        check(created.ok==(point>=2),"metadata boundary outcome reconciles exact committed payload");
        profiles=store.profiles();found=std::find_if(profiles.begin(),profiles.end(),[&](const auto& p){return p.id==id(value);});
        check((found!=profiles.end())==(point>=2),"metadata fault does not expose an uncommitted name");
    }
    // Damaged last-selection preferences never prevent browsing existing profiles.
    corrupt(root/"Profiles"/"selected.ofmeta");corrupt(root/"Profiles"/"selected.ofmeta.previous");
    Store selectionDamaged(defaultPath,rules,false);
    check(selectionDamaged.activeId()=="default"&&!selectionDamaged.notice().empty(),"damaged last choice falls back with notice");
    check(selectionDamaged.select(id(1)).ok&&selectionDamaged.state().runId=="winner","selection preference failure does not block valid campaign");
    check(selectionDamaged.notice().find("could not be saved")!=std::string::npos,"preference persistence failure remains visible");
    check(store.create(id(200),"Name \xc2\xa0").ok,"Adjacent U+00A0 noncontrol character is permitted");
    check(fs::exists(root/"Profiles"/id(200)),"Valid adjacent codepoint creates ordinary metadata directory");
    std::cout<<"PASS "<<checks<<" profile assertions using production CampaignSession/SaveStore.\nEvidence: "<<root.string()<<"\n";
    return 0;
}catch(const std::exception& e){std::cerr<<"FAIL after "<<checks<<" assertions: "<<e.what()<<'\n';return 1;}}

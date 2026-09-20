#include "FoundryProfiles.h"
#include <Windows.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs=std::filesystem;
using namespace overkill;
using foundry_profiles::Store;
namespace {
int checks=0,passed=0,failed=0;
void expect(bool ok,const std::string& why){++checks;if(!ok)throw std::runtime_error(why);}
void run(const char* name,const std::function<void()>& fn){try{fn();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cout<<"FAIL "<<name<<": "<<e.what()<<'\n';}}
std::string bytes(const fs::path& p){std::ifstream in(p,std::ios::binary);expect(static_cast<bool>(in),"Read controlled evidence file");return {std::istreambuf_iterator<char>(in),{}};}
std::string id(char digit){return std::string(32,digit);}
Campaign load(const fs::path& p){auto saved=SaveStore(p).read();expect(saved.ok,saved.error);Campaign c;std::string error;expect(deserializeCampaign(saved.payload,c,error),error);return c;}
void metadata(const fs::path& p,const std::string& data){SaveStore disk(p);auto old=disk.read();auto wrote=disk.commit(data,old.ok?old.token:std::string{});expect(wrote.ok,wrote.error);}
struct Locked {HANDLE value;explicit Locked(const fs::path& path):value(CreateFileW(path.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr)){expect(value!=INVALID_HANDLE_VALUE,"Open exclusive metadata test lock");}~Locked(){if(value!=INVALID_HANDLE_VALUE)CloseHandle(value);}Locked(const Locked&)=delete;Locked& operator=(const Locked&)=delete;};
}
int main(int argc,char** argv){
    if(argc!=4){std::cerr<<"Usage: independent_profiles NEW_CASE_DIRECTORY ARCHIVED_NATIVE_DIRECTORY VERIFIED_PREPARED_DEFEAT\n";return 2;}
    const fs::path root=fs::absolute(argv[1]),archive=fs::absolute(argv[2]);
    const Rules fights;const CampaignRules rules(fights,cinderwallUpgradeHooks(fights));
    run("P01 actual directory junction cannot alias another campaign",[&]{
        // The runner prepares this real NTFS junction inside its private case root.
        const auto alias=root/"Profiles"/id('9'),target=root/"alias-target";
        expect(fs::exists(alias)&&fs::weakly_canonical(alias)==fs::weakly_canonical(target),"Expected prepared junction");
        CampaignSession external(target/"profile.ofsave",rules);expect(external.newGame(700,"external-alias-target").ok,"Create independent alias target");
        metadata(target/"name.ofmeta","FOUNDRY-PROFILE-NAME-1\nExternal target");
        Store store(root/"profile.ofsave",rules,false);expect(store.newGame(701,"default-kept").ok,"Create test Default");
        const auto kept=bytes(root/"profile.ofsave"),outside=bytes(target/"profile.ofsave");
        const auto listed=store.profiles();expect(std::none_of(listed.begin(),listed.end(),[](const auto& p){return p.id==id('9');}),"Directory alias was offered as independent profile");
        expect(!store.select(id('9')).ok&&store.activeId()=="default"&&store.state().runId=="default-kept","Alias selection displaced current campaign");
        expect(!store.create(id('9'),"Overwrite target").ok,"Alias create was accepted");
        expect(bytes(root/"profile.ofsave")==kept&&bytes(target/"profile.ofsave")==outside,"Alias operation modified a campaign");
    });
    run("P02 exclusive metadata locks preserve successful selection and campaign",[&]{
        const auto folder=root/"locked";Store store(folder/"profile.ofsave",rules,false);
        expect(store.create(id('1'),"One").ok&&store.create(id('2'),"Two").ok,"Create two controlled profiles");
        expect(store.select(id('1')).ok&&store.newGame(710,"one").ok,"Start One");const auto one=store.path();
        expect(store.select(id('2')).ok&&store.newGame(711,"two").ok,"Start Two");const auto two=store.path();
        const auto beforeOne=bytes(one),beforeTwo=bytes(two);const auto preference=folder/"Profiles/selected.ofmeta";
        const auto beforePreference=bytes(preference);
        {
            Locked selected(preference);Locked previous(fs::path(preference).concat(L".previous"));
            Locked name(folder/"Profiles"/id('1')/"name.ofmeta");
            const auto result=store.select(id('1'));expect(result.ok&&store.state().runId=="one","Metadata sharing violation prevented valid campaign selection");
            expect(store.notice().find("could not be saved")!=std::string::npos,"Selection preference failure was hidden");
            expect(store.activeName().find("Recovered profile")==0,"Unreadable name did not give fallback label");
        }
        expect(bytes(preference)==beforePreference&&bytes(one)==beforeOne&&bytes(two)==beforeTwo,"Read/selection error changed campaign or preference bytes");
        expect(store.newGame(712,"one-replaced").ok,"Recovered metadata condition poisoned independent campaign saving");
    });
    run("P03 metadata version failure and previous-only names remain browsable",[&]{
        const auto folder=root/"metadata";Store store(folder/"profile.ofsave",rules,false);
        expect(store.create(id('3'),"Three").ok&&store.select(id('3')).ok&&store.newGame(720,"three").ok,"Create Three");
        const auto savedPath=store.path(),name=folder/"Profiles"/id('3')/"name.ofmeta";const auto before=bytes(savedPath);
        metadata(name,"FOUNDRY-PROFILE-NAME-99\nUnsupported name");
        auto profiles=store.profiles();const auto found=std::find_if(profiles.begin(),profiles.end(),[](const auto& p){return p.id==id('3');});
        expect(found!=profiles.end()&&!found->note.empty()&&found->hasSave,"Unsupported display metadata hid campaign");
        expect(store.select(id('3')).ok&&bytes(savedPath)==before,"Metadata fallback changed campaign");
        fs::rename(name,folder/"unsupported-name-kept.ofmeta"); // Only valid v1 previous remains.
        Store reopened(folder/"profile.ofsave",rules,false);expect(reopened.activeName()=="Three"&&reopened.load().ok&&reopened.state().runId=="three","Previous-only valid name/campaign not restored");
        expect(bytes(savedPath)==before,"Recovery rewrote the campaign");
    });
    run("P04 Unicode control characters cannot become profile names",[&]{
        Store store(root/"unicode/profile.ofsave",rules,false);
        for(const auto& code:std::vector<std::pair<char,std::string>>{{'4',std::string("\xc2\x80",2)},{'5',std::string("\xc2\x85",2)},{'6',std::string("\xc2\x9f",2)}}){
            const auto result=store.create(id(code.first),"Alpha "+code.second+" Beta");
            expect(!result.ok,"Accepted UTF-8 C1 control in display name ("+id(code.first).substr(0,1)+")");
            expect(!fs::exists(root/"unicode/Profiles"/id(code.first)),"Rejected name allocated a profile directory");
        }
        const std::string adjacent="Alpha "+std::string("\xc2\xa0",2)+" Beta";
        expect(store.create(id('7'),adjacent).ok,"Valid adjacent U+00A0 was rejected");
        expect(fs::exists(root/"unicode/Profiles"/id('7')/"name.ofmeta"),"Accepted name metadata missing");
        expect(store.select(id('7')).ok&&store.activeName()==adjacent&&!store.loaded()&&!store.hasSave(),"Accepted name did not round-trip without creating a campaign");
    });
    run("P05 archived campaign isolation and exactly one real EndTurn defeat",[&]{
        const auto alpha=load(archive/"alpha-before-browse.ofsave"),beta=load(archive/"beta-independent-arrival.ofsave");
        expect(bytes(archive/"alpha-before-browse.ofsave")==bytes(archive/"alpha-after-relaunch.ofsave"),"Alpha browsing/cancellation/relaunch changed bytes");
        expect(alpha.runId!=beta.runId&&alpha.phase==CityPhase::Between&&beta.phase==CityPhase::Arrival&&alpha.profile.recipes.size()==16&&beta.profile.recipes.size()==12,"Archived Alpha/Beta run/discovery isolation");
        auto before=load(argv[3]),defeated=load(archive/"defeat-after-endturn.ofsave"),fresh=load(archive/"defeat-new-run.ofsave");
        expect(before.phase==CityPhase::Fight&&before.fight.hp==1,"Expected original prepared HP1 fight");
        CampaignAction end;end.type=CampaignActionType::Combat;end.runId=before.runId;end.sequence=before.nextTransaction;end.combat=Action::endTurn();
        const auto result=rules.apply(before,end);expect(result.ok,result.reason);expect(serializeCampaign(before)==serializeCampaign(defeated),"One production EndTurn does not reproduce native persisted defeat");
        expect(defeated.phase==CityPhase::Defeated&&defeated.fight.hp==0&&defeated.profile.recipes.size()==16,"Persisted defeat status/discoveries");
        expect(fresh.phase==CityPhase::Arrival&&fresh.fight.hp==80&&fresh.fight.credits==100&&fresh.runId!=defeated.runId&&fresh.profile.recipes==defeated.profile.recipes,"Post-defeat fresh run did not retain only persistent facts");
        auto reference=load(archive/"collection-inspection.ofsave");expect(reference.profile.recipes.size()==246&&reference.fight.memory.size()==12,"Prepared discoveries were mistaken for owned memory");
    });
    std::cout<<"SUMMARY "<<passed<<" passed, "<<failed<<" failed, "<<checks<<" assertions\n";return failed?1:0;
}

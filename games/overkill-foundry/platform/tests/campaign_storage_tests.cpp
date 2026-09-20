#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "overkill/campaign_session.hpp"
#include "overkill/robots.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace overkill;
namespace {
int checks=0;
void check(bool ok,const std::string& message){++checks;if(!ok)throw std::runtime_error(message);}
CampaignHooks fixtureHooks(const Rules& fights){
    CampaignHooks h;
    // Transaction fixtures only. The P08 upgrade executor is a separate gate.
    h.acquireUpgrade=[](Campaign& c,const std::string&,const CampaignAction&){c.upgrades.back().charges=1;return Result{true,{},{}};};
    h.initializeFight=[](Campaign& c,const RouteOffer& o){c.fight.enemies=makeCinderwallFormation(o.formation,o.encounterSeed,o.encounterKey,c.fight.nextId,o.binderDefaultSeen);return Result{true,{},{}};};
    h.combatCompleted=[](Campaign&){return Result{true,{},{}};};
    h.grantPart=[&fights](Campaign& c,const std::string& id){return fights.grantPart(c.fight,id);};return h;
}
CampaignAction action(const Campaign& c,CampaignActionType type,Id subject=0,std::string choice={}){
    CampaignAction a;a.type=type;a.sequence=c.nextTransaction;a.subject=subject;a.choice=std::move(choice);return a;
}
void run(Campaign& c,const CampaignRules& rules,const CampaignAction& a){const auto r=rules.apply(c,a);check(r.ok,r.reason);}
Id iron(const Campaign& c){for(const auto& p:c.shop)if(p.kind==ProductKind::Material && p.material==0)return p.id;throw std::runtime_error("Missing Iron stock.");}
CampaignAction purchase(const Campaign& c){auto a=action(c,CampaignActionType::Buy,iron(c));a.quantity=2;return a;}
Campaign initial(const CampaignRules& rules){auto c=rules.newGame(73,"disk-transaction-fixture");run(c,rules,action(c,CampaignActionType::ChooseMayor,0,c.mayorOffers[0]));return c;}
void enter(Campaign& c,const CampaignRules& rules){for(const auto& o:routeOffers(c.route))if(o.formation=="C1-F-RAM"){run(c,rules,action(c,CampaignActionType::EnterOffer,o.id));return;}throw std::runtime_error("Missing opening Ram.");}
Campaign rewardFixture(const CampaignRules& rules,const Rules& fights){
    auto c=initial(rules);enter(c,rules);const auto grant=fights.grantPlainPart(c.fight,Kind::Ammo,50,"disk-test-only");check(grant.ok,grant.reason);
    Id ammo=0;for(const auto& p:c.fight.parts)if(p.originalValue==50)ammo=p.id;
    for(const auto& command:{Action::collect(0),Action::load({ammo}),Action::fire(c.fight.enemies.front().id)}){
        auto a=action(c,CampaignActionType::Combat);a.combat=command;run(c,rules,a);
    }
    check(c.phase==CityPhase::Rewards,"Fixture did not finish with rewards.");return c;
}
CampaignAction forMode(const Campaign& c,int mode){
    if(mode==0)return purchase(c);
    if(mode==1){for(const auto& r:c.rewards)if(r.kind==RewardKind::Cores)return action(c,CampaignActionType::ClaimReward,r.id);throw std::runtime_error("No core claim.");}
    return action(c,CampaignActionType::Continue);
}
void seedFile(const std::filesystem::path& path,const Campaign& c){SaveStore store(path);const auto r=store.commit(serializeCampaign(c),"");check(r.ok,r.error);}
}
int wmain(int argc,wchar_t** argv){try{
    Rules fights;CampaignRules rules(fights,fixtureHooks(fights));
    if(argc==5 && std::wstring(argv[1])==L"--crash"){
        CampaignSession session(argv[2],rules);check(session.load().ok,"Child cannot load baseline.");
        const int point=std::stoi(argv[3]),mode=std::stoi(argv[4]);
        const auto r=session.apply(forMode(session.state(),mode),[point](SavePoint p){if(static_cast<int>(p)==point)ExitProcess(75);});
        return r.ok?0:3;
    }
    const auto root=std::filesystem::current_path()/"campaign-storage-artifacts"/std::to_string(GetCurrentProcessId());std::filesystem::create_directories(root);
    {
        CampaignSession session(root/"new-game.ofsave",rules);check(!session.loaded(),"Unloaded session exposed state.");
        auto r=session.newGame(3,"new-game-1");check(r.ok && session.revision()==1,r.reason);
        r=session.newGame(3,"new-game-1");check(r.ok && r.replayed && session.revision()==1,"Duplicate New Game wrote again.");
        check(!session.newGame(4,"new-game-1").ok,"New Game accepted another seed for same identity.");
        const auto profile=session.state().profile.recipes;r=session.newGame(4,"new-game-2");check(r.ok && session.state().profile.recipes==profile,"New Game lost profile discoveries.");
        CampaignSession replacement(root/"new-game.ofsave",rules);r=replacement.newGame(5,"unsafe-overwrite");
        check(!r.ok && r.reconciled && replacement.state().runId=="new-game-2","Unloaded New Game overwrote an existing save.");
    }
    {
        const auto path=root/"ambiguous.ofsave";seedFile(path,initial(rules));CampaignSession session(path,rules);check(session.load().ok,"Load failed.");
        const auto before=serializeCampaign(session.state());auto a=purchase(session.state());
        auto r=session.apply(a,[](SavePoint p){if(p==SavePoint::TemporaryFlushed)throw std::runtime_error("injected before replacement");});
        check(!r.ok && r.reconciled && serializeCampaign(session.state())==before,"Failed write exposed speculative purchase.");
        r=session.apply(a,[](SavePoint p){if(p==SavePoint::Replaced)throw std::runtime_error("injected after replacement");});
        check(r.ok && r.reconciled && session.state().fight.materials[0]==2 && session.state().fight.credits==92,"Committed purchase not reconciled after ambiguous failure.");
        const auto revision=session.revision();r=session.apply(a);check(r.ok && r.replayed && session.revision()==revision && session.state().fight.materials[0]==2,"Reconciled purchase repeated.");
    }
    {
        const auto path=root/"writers.ofsave";seedFile(path,initial(rules));CampaignSession first(path,rules),second(path,rules);
        check(first.load().ok && second.load().ok,"Writer baseline load.");const auto a=purchase(first.state());auto b=a;b.quantity=1;
        check(first.apply(a).ok,"First writer failed.");const auto r=second.apply(b);
        check(!r.ok && r.reconciled && second.state().fight.materials[0]==2 && second.state().fight.credits==92,"Stale writer overwrote or misreported different command.");
        check(second.apply(a).replayed,"Reloaded receipt was not reusable.");
    }
    std::vector<wchar_t> executable(32768);const auto length=GetModuleFileNameW(nullptr,executable.data(),static_cast<DWORD>(executable.size()));check(length>0,"Executable path unavailable.");
    for(int mode=0;mode<3;++mode){
        auto before=mode==1?rewardFixture(rules,fights):initial(rules);
        if(mode==2){run(before,rules,purchase(before));enter(before,rules);auto a=action(before,CampaignActionType::Buy,iron(before));run(before,rules,a);before.upgrades.front().charges=0;run(before,rules,action(before,CampaignActionType::OpenShop));}
        const auto command=forMode(before,mode);auto after=before;run(after,rules,command);
        const auto beforeBytes=serializeCampaign(before),afterBytes=serializeCampaign(after);
        for(int point=0;point<4;++point){
            const auto path=root/("crash-"+std::to_string(mode)+"-"+std::to_string(point)+".ofsave");seedFile(path,before);
            std::wstring cli=L"\""+std::wstring(executable.data(),length)+L"\" --crash \""+path.wstring()+L"\" "+std::to_wstring(point)+L" "+std::to_wstring(mode);
            STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;PROCESS_INFORMATION child{};
            check(CreateProcessW(nullptr,cli.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child)!=0,"Crash subprocess launch failed.");
            const auto wait=WaitForSingleObject(child.hProcess,10000);check(wait==WAIT_OBJECT_0,"Crash subprocess timed out.");DWORD code=0;GetExitCodeProcess(child.hProcess,&code);CloseHandle(child.hThread);CloseHandle(child.hProcess);check(code==75,"Requested crash boundary was not reached.");
            CampaignSession recovered(path,rules);const auto loaded=recovered.load();check(loaded.ok,loaded.reason);
            check(serializeCampaign(recovered.state())==(point<2?beforeBytes:afterBytes),"Crash recovered a partial campaign transaction.");
            const auto retried=recovered.apply(command);check(retried.ok && retried.replayed==(point>=2),"Recovered receipt/retry did not match durable boundary.");
            check(serializeCampaign(recovered.state())==afterBytes,"Retry duplicated a purchase, reward or Continue effect.");
            std::cout<<"PASS campaign crash mode "<<mode<<" at boundary "<<point<<"; entire "<<(point<2?"prior":"new")<<" transaction recovered.\n";
        }
    }
    {
        auto c=initial(rules);enter(c,rules);const auto path=root/"internal-checkpoint.ofsave";
        SaveStore store(path);check(store.commit(c.entry,"").ok,"Checkpoint negative fixture failed.");CampaignSession session(path,rules);
        check(!session.load().ok && !session.loaded(),"Internal pre-start checkpoint accepted as active save.");
    }
    std::cout<<"PASS "<<checks<<" campaign storage assertions and 12 actual process exits (purchase, reward claim, Continue). Controlled fixture effects; no power-loss or full P08 claim.\nEvidence: "<<root.u8string()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}

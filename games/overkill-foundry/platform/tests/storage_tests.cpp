#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "overkill/save_store.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
void check(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
const std::string oldPayload="credits=50;stock=2;owned=0;receipt=0";
const std::string newPayload="credits=30;stock=1;owned=1;receipt=1";
}
int wmain(int argc,wchar_t** argv){try{
    if(argc==4 && std::wstring(argv[1])==L"--crash"){
        overkill::SaveStore store(argv[2]);const int point=std::stoi(argv[3]);
        const auto result=store.commit(newPayload,store.read().token,[point](overkill::SavePoint p){if(static_cast<int>(p)==point)ExitProcess(75);});
        return result.ok?0:3;
    }
    const auto root=std::filesystem::current_path()/"storage-test-artifacts"/std::to_string(GetCurrentProcessId());
    std::filesystem::create_directories(root);
    overkill::SaveStore store(root/"profile.ofsave");
    auto r=store.commit(oldPayload,"");check(r.ok && r.revision==1,r.error);const auto first=r;
    r=store.commit(newPayload,r.token);check(r.ok && r.revision==2,r.error);
    auto stale=store.commit("bad",first.token);check(!stale.ok && store.read().payload==newPayload,"stale writer changed committed state");
    r=store.commit("third",r.token);check(r.ok && r.revision==3,r.error);const auto staleThird=r;
    {std::ofstream out(root/"profile.ofsave",std::ios::binary|std::ios::trunc);out<<"corrupt";}
    r=store.read();check(r.ok && r.recoveredPrevious && r.revision==2 && r.payload==newPayload,"last valid backup recovery");
    r=store.commit("repaired",r.token);check(r.ok && r.revision==3,r.error);
    stale=store.commit("stale overwrite",staleThird.token);check(!stale.ok && store.read().payload=="repaired","recovery reused stale commit identity");
    std::vector<wchar_t> executable(32768);const DWORD n=GetModuleFileNameW(nullptr,executable.data(),static_cast<DWORD>(executable.size()));check(n>0,"executable path");
    for(int point=0;point<4;++point){
        const auto path=root/("crash-"+std::to_string(point)+".ofsave");overkill::SaveStore crashStore(path);
        r=crashStore.commit(oldPayload,"");check(r.ok,r.error);
        std::wstring command=L"\""+std::wstring(executable.data(),n)+L"\" --crash \""+path.wstring()+L"\" "+std::to_wstring(point);
        STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;PROCESS_INFORMATION process{};
        check(CreateProcessW(nullptr,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process)!=0,"crash probe process launch");
        const DWORD wait=WaitForSingleObject(process.hProcess,10000);check(wait==WAIT_OBJECT_0,"crash probe did not finish");
        DWORD code=0;GetExitCodeProcess(process.hProcess,&code);CloseHandle(process.hThread);CloseHandle(process.hProcess);check(code==75,"fault probe was not reached");
        const auto loaded=crashStore.read();check(loaded.ok,loaded.error);
        check(loaded.payload==(point<2?oldPayload:newPayload),"crash recovered a partial or wrong transaction");
        check(loaded.revision==(point<2?1U:2U),"crash revision mismatch");
        std::cout<<"PASS actual process exit at save boundary "<<point<<" recovered revision "<<loaded.revision<<'\n';
    }
    std::cout<<"PASS Windows save replacement, repeated backup, stale writer, corruption recovery and four crash boundaries.\nEvidence: "<<root.u8string()<<"\nThis tests storage envelopes, not yet campaign/Continue integration or power-loss hardware.\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}

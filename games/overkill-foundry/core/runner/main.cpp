#include "overkill/core.hpp"
#include <iostream>
#include <string>
int main(int argc,char** argv){
    if(argc<2 || std::string(argv[1])!="--fixture"){
        std::cout<<"Overkill Foundry shared-core runner "<<overkill::RulesVersion<<"\nUse --fixture to replay the approved Mite/Ram encounter. Complete-city policies are not implemented yet.\n";return argc<2?0:2;
    }
    overkill::Rules rules;auto state=overkill::Rules::teachingEncounter();
    const auto result=overkill::playTeachingFixture(state,rules);
    for(const auto& event:result.events)std::cout<<overkill::eventJson(event)<<'\n';
    std::cout<<"{\"fixture\":\"mite-ram-v1.1\",\"ok\":"<<(result.ok?"true":"false")<<",\"hp\":"<<state.hp<<",\"round\":"<<state.round<<",\"shots\":"<<state.shots<<",\"state_hash\":\""<<overkill::stateHash(state)<<"\"}\n";
    if(!result.ok)std::cerr<<result.reason<<'\n';return result.ok?0:1;
}

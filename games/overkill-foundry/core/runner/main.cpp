#include "city_runner.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
int main(int argc,char** argv){try{
    using namespace overkill;using namespace overkill::runner;
    if(argc<2){std::cout<<"Overkill Foundry "<<RulesVersion<<"\n--fixture\n--city --seed N --policy aggressive|defensive --precision auto|learning|practised|expert --output DIR\n--batch SEED,COUNT,POLICY,PRECISION --output DIR\n--replay TRACE\nOptional --timeout SECONDS and --search-budget PREVIEWS bound the runner, never the game.\n";return 0;}
    overkill::Rules rules;auto state=overkill::Rules::teachingEncounter();
    if(std::string(argv[1])!="--fixture"){
        CampaignRules campaign(rules,cinderwallUpgradeHooks(rules));Options options;std::size_t count=1;std::string output="games/overkill-foundry/core/build/city-runner";bool city=false;
        const auto next=[&](int& i){if(i+1>=argc)throw std::runtime_error("Missing option argument");return std::string(argv[++i]);};
        const auto number=[](const std::string& text){if(text.empty()||!std::all_of(text.begin(),text.end(),[](char c){return c>='0'&&c<='9';}))throw std::runtime_error("Expected an unsigned decimal number");return std::stoull(text);};
        for(int i=1;i<argc;++i){const std::string arg=argv[i];if(arg=="--replay"){const auto file=next(i);if(i+1!=argc)throw std::runtime_error("Unexpected options after replay path");std::cout<<replayTrace(file,campaign)<<'\n';return 0;}if(arg=="--city")city=true;else if(arg=="--batch"){city=true;auto spec=next(i);std::vector<std::string> fields;std::istringstream input(spec);std::string field;while(std::getline(input,field,','))fields.push_back(field);if(fields.size()!=4||spec.back()==',')throw std::runtime_error("Batch requires SEED,COUNT,POLICY,PRECISION");options.seed=number(fields[0]);count=number(fields[1]);options.policy=fields[2];options.precision=fields[3];}
            else if(arg=="--seed")options.seed=number(next(i));else if(arg=="--policy")options.policy=next(i);else if(arg=="--precision")options.precision=next(i);else if(arg=="--output")output=next(i);else if(arg=="--timeout"){const auto value=next(i);std::size_t consumed=0;options.timeoutSeconds=std::stod(value,&consumed);if(consumed!=value.size())throw std::runtime_error("Invalid timeout number");}else if(arg=="--search-budget")options.searchBudget=number(next(i));else if(arg!="--city")throw std::runtime_error("Unknown option: "+arg);}
        if(!city||count==0||options.timeoutSeconds<=0||!std::isfinite(options.timeoutSeconds)||options.searchBudget==0||count-1>std::numeric_limits<std::uint64_t>::max()-options.seed)throw std::runtime_error("Invalid city runner options");if(options.policy!="aggressive"&&options.policy!="defensive")throw std::runtime_error("Unknown policy");if(options.precision!="auto"&&options.precision!="learning"&&options.precision!="practised"&&options.precision!="expert")throw std::runtime_error("Unknown Precision model");
        std::filesystem::create_directories(output);const auto report=std::filesystem::path(output)/(options.policy+"-"+options.precision+"-"+std::to_string(options.seed)+"-"+std::to_string(count)+".jsonl");std::ofstream summary(report,std::ios::binary);if(!summary)throw std::runtime_error("Cannot create batch report");bool failed=false;const auto first=options.seed;
        for(std::size_t i=0;i<count;++i){options.seed=first+i;options.trace=(std::filesystem::path(output)/(options.policy+"-"+options.precision+"-"+std::to_string(options.seed)+".oftrace")).string();const auto result=runCity(options,rules,campaign);const auto json=summaryJson(result);summary<<json<<'\n';summary.flush();std::cout<<json<<'\n';if(result.outcome!="city_complete"&&result.outcome!="defeat")failed=true;}
        return failed?2:0;
    }
    const auto result=overkill::playTeachingFixture(state,rules);
    for(const auto& event:result.events)std::cout<<overkill::eventJson(event)<<'\n';
    std::cout<<"{\"fixture\":\"mite-ram-v1.1\",\"ok\":"<<(result.ok?"true":"false")<<",\"hp\":"<<state.hp<<",\"round\":"<<state.round<<",\"shots\":"<<state.shots<<",\"state_hash\":\""<<overkill::stateHash(state)<<"\"}\n";
    if(!result.ok)std::cerr<<result.reason<<'\n';return result.ok?0:1;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}

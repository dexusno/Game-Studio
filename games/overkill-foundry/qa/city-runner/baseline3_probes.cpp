// Independent, finite baseline-3 review. No production writes or game grants.
#include "city_runner.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace overkill;
using namespace overkill::runner;
namespace {
int passed=0, failed=0;
void require(bool condition,const std::string& message) {
    if(!condition) throw std::runtime_error(message);
}
void check(const std::string& name,const std::function<void()>& test) {
    try { test(); ++passed; std::cout<<"PASS "<<name<<'\n'; }
    catch(const std::exception& e) { ++failed; std::cout<<"FAIL "<<name<<": "<<e.what()<<'\n'; }
}
Campaign readState(const std::string& encoded) {
    Campaign state; std::string error;
    require(deserializeCampaign(unhex(encoded),state,error),error);
    return state;
}
Campaign finalState(const std::string& path) {
    std::ifstream input(path,std::ios::binary); require(static_cast<bool>(input),"missing stopped trace");
    std::string line;
    while(std::getline(input,line)) if(line.rfind("FINAL ",0)==0) {
        std::istringstream fields(line.substr(6)); std::string hash,outcome,encoded;
        fields>>hash>>outcome>>encoded; require(outcome=="policy_timeout","fixture was not a timeout");
        auto state=readState(encoded); require(hash==campaignHash(state),"stopped state hash differs");
        return state;
    }
    throw std::runtime_error("stopped trace lacks final state");
}
// Fixture extraction only uses the authored codec. The separate unchanged
// probes.cpp independently decodes and verifies every full regression trace.
Campaign firstState(const std::string& path,const CampaignRules& rules,
                    const std::function<bool(const Campaign&)>& predicate) {
    std::ifstream input(path,std::ios::binary); require(static_cast<bool>(input),"missing ordinary trace");
    Campaign state; std::string line,error; bool started=false;
    while(std::getline(input,line)) {
        if(line.rfind("START ",0)==0) { state=readState(line.substr(6)); started=true; }
        else if(line.rfind("A ",0)==0) {
            require(started,"action before start"); CampaignAction action;
            require(decodeAction(unhex(line.substr(2)),action,error),error);
            const auto result=rules.apply(state,action); require(result.ok,result.reason);
        }
        else continue;
        if(predicate(state)) return state;
    }
    throw std::runtime_error("ordinary trace lacks requested observation state");
}
std::string choose(const Rules& fights,const CampaignRules& rules,const Options& options,const Campaign& state) {
    Policy policy(fights,rules,options); const auto before=serializeCampaign(state);
    const auto result=policy.choose(state); require(result.available,result.reason);
    require(serializeCampaign(state)==before,"policy mutated observed state");
    return encodeAction(result.action);
}
std::string routeChoice(const Rules& fights,const CampaignRules& rules,const Campaign& state) {
    Options options; options.seed=state.seed; options.policy="defensive";
    Policy policy(fights,rules,options); const auto before=serializeCampaign(state);
    auto result=policy.choose(state);
    if(result.available&&result.action.type==CampaignActionType::OpenShop) result=policy.choose(state);
    require(result.available&&result.action.type==CampaignActionType::EnterOffer,"expected route choice");
    require(serializeCampaign(state)==before,"route observation mutated");
    return encodeAction(result.action);
}
}
int main(int argc,char** argv) {
    if(argc!=8) { std::cerr<<"Expected Miss Good Perfect ordinary-trace and three stopped traces\n"; return 2; }
    Rules fights; CampaignRules rules(fights,cinderwallUpgradeHooks(fights));
    std::array<int,3> expected{std::stoi(argv[1]),std::stoi(argv[2]),std::stoi(argv[3])};
    const auto collection=firstState(argv[4],rules,[](const Campaign& c) {
        return c.phase==CityPhase::Fight&&c.fight.phase==Phase::Collection&&c.fight.upgradeChoices.empty()
            &&std::none_of(c.upgradeOffers.begin(),c.upgradeOffers.end(),[](const auto& o){return !o.deferred;});
    });
    check("B301 canonical Practised categories from selected beta data",[&] {
        require(expected==std::array<int,3>{20,45,35},"selected data changed; review expectation");
        require(!collection.fight.precisionSpent,"sample state already spent Precision");
        std::array<int,3> observed{};
        for(std::uint64_t seed=100001;seed<=110000;++seed) {
            Options options; options.seed=seed; options.precision="practised";
            Policy policy(fights,rules,options); const auto decision=policy.choose(collection);
            require(decision.available&&decision.action.type==CampaignActionType::Combat
                &&decision.action.combat.type==ActionType::Collect,"sample did not choose collection");
            const auto category=decision.action.combat.precision;
            require(category>=0&&category<=2,"invalid Precision category");
            ++observed[static_cast<std::size_t>(category)];
        }
        for(std::size_t i=0;i<3;++i)
            require(std::abs(observed[i]-expected[i]*100)<=150,"category outside 1.5 percentage-point tolerance");
        std::cout<<"PRACTISED samples=10000 miss="<<observed[0]<<" good="<<observed[1]
            <<" perfect="<<observed[2]<<" expected_percent=20/45/35\n";
        auto spent=collection; spent.fight.precisionSpent=true;
        Options options; options.precision="practised"; Policy policy(fights,rules,options);
        require(policy.choose(spent).action.combat.precision==-1,"spent Precision sampled again");
    });

    std::vector<Campaign> stopped;
    for(int i=5;i<8;++i) stopped.push_back(finalState(argv[i]));
    for(std::size_t i=0;i<stopped.size();++i) check("B30"+std::to_string(i+2)+" progress from actual stopped state",[&,i] {
        auto state=stopped[i]; const auto round=state.fight.round; const auto parts=state.fight.parts.size();
        Options options; options.seed=state.seed; options.policy=i==2?"defensive":"aggressive";
        if(i<2) {
            auto witness=state.fight;
            if(witness.phase==Phase::Collection) {
                const auto collected=fights.preview(witness,Action::collect(0));
                require(collected.result.ok,collected.result.reason); witness=collected.state;
            }
            const auto copy=std::find_if(witness.memory.begin(),witness.memory.end(),[](const auto& c){return c.recipe=="SH001";});
            require(copy!=witness.memory.end(),"Plain Slug copy missing");
            const auto made=fights.preview(witness,Action::craft(copy->id)); require(made.result.ok,made.result.reason);
            const auto part=std::find_if(made.state.parts.begin(),made.state.parts.end(),[&](const auto& p){return p.id>=witness.nextId&&p.recipe=="SH001";});
            require(part!=made.state.parts.end(),"new Plain Slug part missing");
            const auto loaded=fights.preview(made.state,Action::load({part->id})); require(loaded.result.ok,loaded.result.reason);
            const auto enemy=std::find_if(witness.enemies.begin(),witness.enemies.end(),[](const auto& e){return !e.dead&&!e.escaped&&e.hp>0;});
            require(enemy!=witness.enemies.end(),"no live witness target");
            const auto fired=fights.preview(loaded.state,Action::fire(enemy->id)); require(fired.result.ok,fired.result.reason);
            require(fired.state.kills>witness.kills,"legal Plain Slug did not kill stalled body");
            std::cout<<"LEGAL_WITNESS seed="<<state.seed<<" recipe=SH001 target="<<enemy->definition
                <<" target_hp="<<enemy->hp<<" target_mark="<<enemy->mark
                <<" kills_before="<<witness.kills<<" kills_after="<<fired.state.kills<<'\n';
        }
        Policy policy(fights,rules,options); int commands=0;
        while(state.phase==CityPhase::Fight&&commands<64) {
            const auto before=serializeCampaign(state); const auto previews=policy.previews(); const auto decision=policy.choose(state);
            require(decision.available,decision.reason); require(serializeCampaign(state)==before,"progress preview mutated state");
            if(commands<12)std::cout<<"RESUME_ACTION seed="<<state.seed<<" index="<<commands
                <<" type="<<static_cast<int>(decision.action.combat.type)<<" subject="<<decision.action.combat.subject
                <<" heat="<<state.fight.heat<<" shield="<<Rules::shield(state.fight)
                <<" previews="<<(policy.previews()-previews)<<'\n';
            const auto result=rules.apply(state,decision.action); require(result.ok,result.reason); ++commands;
        }
        std::cout<<"RESUME policy="<<options.policy<<" seed="<<state.seed<<" start_round="<<round
            <<" start_parts="<<parts<<" commands="<<commands<<" end_phase="<<static_cast<int>(state.phase)
            <<" end_round="<<state.fight.round<<" kills="<<state.fight.kills<<" hp="<<state.fight.hp<<'\n';
        require(state.phase==CityPhase::Rewards,"did not finish stalled encounter within 64 review commands");
    });
    check("B305 stopped-state decisions ignore hidden RNG and future enemy packets",[&] {
        int comparisons=0;
        for(std::size_t i=0;i<stopped.size();++i) {
            Options options; options.seed=stopped[i].seed; options.policy=i==2?"defensive":"aggressive";
            const auto expectedAction=choose(fights,rules,options,stopped[i]);
            for(std::uint64_t seed=600;seed<620;++seed) {
                auto hidden=stopped[i]; hidden.rng=Rng::seeded(seed,"qa-hidden-campaign");
                hidden.fight.rng=Rng::seeded(seed,"qa-hidden-combat");
                hidden.route.rng=Rng::seeded(seed,"qa-hidden-route");
                for(auto& enemy:hidden.fight.enemies) {
                    for(auto& future:enemy.pattern)future={Move::Attack,999,9};
                    enemy.patternRandom=seed;
                }
                require(choose(fights,rules,options,hidden)==expectedAction,"hidden future changed current decision");
                ++comparisons;
            }
        }
        std::cout<<"HIDDEN_COMBAT comparisons="<<comparisons<<'\n';
    });
    check("B306 concealed current Mystery and future RNG do not change route ranking",[&] {
        auto visible=firstState(argv[4],rules,[&](const Campaign& c) {
            if(c.phase!=CityPhase::Between) return false;
            const auto& offers=routeOffers(c.route);
            return std::any_of(offers.begin(),offers.end(),[&](const auto& o) {
                return o.kind==EncounterKind::Mystery&&rules.revealedMysteryCategory(c,o.id).empty();
            });
        });
        // Isolate a read-only route decision; these observation fixtures are
        // not ordinary run results and never grant gameplay resources.
        visible.shop.clear(); visible.cores.clear();
        const auto expectedAction=routeChoice(fights,rules,visible);
        const auto& currentOffers=routeOffers(visible.route);
        const auto regular=std::find_if(currentOffers.begin(),currentOffers.end(),[](const auto& offer){return offer.kind==EncounterKind::Regular;});
        require(regular!=currentOffers.end(),"no legal current formation for concealed Patrol");
        for(std::uint64_t seed=900;seed<920;++seed) {
            auto hidden=visible; hidden.rng=Rng::seeded(seed,"qa-unseen-offers");
            hidden.route.rng=Rng::seeded(seed,"qa-unseen-route");
            const auto current=static_cast<std::size_t>(hidden.route.position-1);
            for(auto& offer:hidden.route.nodes[current].offers) if(offer.kind==EncounterKind::Mystery) {
                offer.mystery=seed%2?"C1-M-TECH":"C1-M-PATROL";
                offer.formation=seed%2?"":regular->formation;
            }
            require(routeChoice(fights,rules,hidden)==expectedAction,"concealed route payload affected ranking");
        }
        std::cout<<"HIDDEN_ROUTE comparisons=20 position="<<visible.route.position<<'\n';
    });
    std::cout<<"SUMMARY "<<passed<<" passed, "<<failed<<" failed\n";
    return failed?1:0;
}

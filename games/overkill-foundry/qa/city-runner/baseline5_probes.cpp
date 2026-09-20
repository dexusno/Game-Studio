// Independent additions for baseline-5. Reuse only QA fixture readers/helpers;
// the earlier six groups are executed separately, without changed semantics.
#define main preserved_baseline3_main
#include "baseline3_probes.cpp"
#undef main
#include "overkill/upgrades.hpp"

int main(int argc,char** argv) {
    if(argc!=12) {std::cerr<<"Expected ordinary trace, stopped57 trace, nine selected percentages\n";return 2;}
    Rules fights;CampaignRules rules(fights,cinderwallUpgradeHooks(fights));
    const auto ordinary=firstState(argv[1],rules,[](const Campaign& c){
        return c.phase==CityPhase::Fight&&c.fight.phase==Phase::Collection&&c.fight.upgradeChoices.empty()
            &&std::none_of(c.upgradeOffers.begin(),c.upgradeOffers.end(),[](const auto& o){return !o.deferred;});
    });
    const std::array<std::string,3> models{{"learning","practised","expert"}};
    for(std::size_t m=0;m<models.size();++m)check("B50"+std::to_string(m+1)+" selected "+models[m]+" categories",[&,m]{
        std::array<int,3> expected{},observed{};
        for(std::size_t c=0;c<3;++c)expected[c]=std::stoi(argv[3+m*3+c]);
        require(expected[0]+expected[1]+expected[2]==100,"source percentages invalid");
        const auto before=serializeCampaign(ordinary);
        for(std::uint64_t seed=100001;seed<=110000;++seed){
            Options options;options.seed=seed;options.precision=models[m];Policy policy(fights,rules,options);
            const auto choice=policy.choose(ordinary);
            require(choice.available&&choice.action.type==CampaignActionType::Combat&&choice.action.combat.type==ActionType::Collect,"model did not collect");
            const auto category=choice.action.combat.precision;require(category>=0&&category<=2,"invalid category");
            ++observed[static_cast<std::size_t>(category)];
        }
        require(serializeCampaign(ordinary)==before,"sampling mutated caller");
        for(std::size_t c=0;c<3;++c)require(std::abs(observed[c]-expected[c]*100)<=150,"category outside 1.5 percentage-point tolerance");
        std::cout<<"MODEL "<<models[m]<<" samples=10000 observed="<<observed[0]<<'/'<<observed[1]<<'/'<<observed[2]
            <<" expected="<<expected[0]<<'/'<<expected[1]<<'/'<<expected[2]<<'\n';
        Options options;options.precision=models[m];Policy first(fights,rules,options),second(fights,rules,options);
        require(encodeAction(first.choose(ordinary).action)==encodeAction(second.choose(ordinary).action),"same model seed differs");
        auto spent=ordinary;spent.fight.precisionSpent=true;
        require(first.choose(spent).action.combat.precision==-1,"spent Precision resampled");
    });
    check("B504 Auto is an ordinary haul without spending Precision",[&]{
        auto state=ordinary;Options options;options.precision="auto";Policy policy(fights,rules,options);
        const auto decision=policy.choose(state);require(decision.available&&decision.action.combat.precision==-1,"Auto invented a timing measurement");
        const auto result=rules.apply(state,decision.action);require(result.ok,result.reason);
        require(!state.fight.precisionSpent,"Auto spent the manual opportunity");
        require(state.fight.phase==Phase::Preparation,"ordinary collection did not finish");
    });
    const auto stopped=finalState(argv[2]);
    require(stopped.seed==57&&stopped.phase==CityPhase::Fight,"wrong stopped57 fixture");
    check("B505 actual retained Foil Warden resumes without grants",[&]{
        auto state=stopped;const auto original=serializeCampaign(state);
        const auto target=std::find_if(state.fight.enemies.begin(),state.fight.enemies.end(),[](const auto& e){return !e.dead&&!e.escaped&&e.tiles>0;});
        require(target!=state.fight.enemies.end(),"stopped fixture lacks finite tiles");
        require(ownedUpgrade(state.fight,"UGS-001")!=nullptr&&!state.fight.shotBonuses.empty(),"first-shot asset absent");
        std::cout<<"WARDEN_START round="<<state.fight.round<<" hp="<<target->hp<<" tiles="<<target->tiles<<" mark="<<target->mark<<" parts="<<state.fight.parts.size()<<'\n';
        Options options;options.seed=57;Policy policy(fights,rules,options);int commands=0,shots=0;
        while(state.phase==CityPhase::Fight&&commands<64){
            const auto before=serializeCampaign(state);const auto decision=policy.choose(state);
            require(decision.available,decision.reason);require(serializeCampaign(state)==before,"retained-state preview mutated caller");
            if(decision.action.type==CampaignActionType::Combat&&decision.action.combat.type==ActionType::Fire)++shots;
            const auto result=rules.apply(state,decision.action);require(result.ok,result.reason);++commands;
        }
        require(serializeCampaign(stopped)==original,"original stopped fixture changed");
        std::cout<<"WARDEN_RECOVERY commands="<<commands<<" shots="<<shots<<" phase="<<static_cast<int>(state.phase)<<" hp="<<state.fight.hp<<'\n';
        require(state.phase==CityPhase::Rewards&&shots>0,"no encounter completion within 64 review commands");
    });
    check("B506 Foil Warden decision ignores hidden future RNG and packets",[&]{
        Options options;options.seed=57;const auto expected=choose(fights,rules,options,stopped);
        for(std::uint64_t n=1200;n<1220;++n){auto hidden=stopped;
            hidden.rng=Rng::seeded(n,"qa-campaign");hidden.route.rng=Rng::seeded(n,"qa-route");hidden.fight.rng=Rng::seeded(n,"qa-combat");
            for(auto& e:hidden.fight.enemies){e.pattern={{Move::Attack,900,9},{Move::Escape,0,0}};e.patternRandom=n;}
            require(choose(fights,rules,options,hidden)==expected,"hidden future affected current tile choice");
        }
        std::cout<<"WARDEN_HIDDEN comparisons=20\n";
    });
    check("B507 held composition remains a rejected core command",[&]{
        // Controlled rule-boundary fixture, not a naturally acquired run. Both
        // drawbacks are acquired through real effects; no payoff is substituted.
        auto state=ordinary;
        for(const std::string id:{"UGS-011","UGS-018"})if(!ownedUpgrade(state.fight,id)){
            const auto acquired=fights.acquireUpgrade(state.fight,id);require(acquired.ok,acquired.reason);
            state.everAcquired.push_back(id); // Keep the controlled campaign fixture schema-coherent.
        }
        require(state.fight.round==1&&state.fight.phase==Phase::Collection,"held fixture not at first haul");
        const auto before=serializeCampaign(state);Options options;Policy policy(fights,rules,options);
        const auto decision=policy.choose(state);require(decision.available,"held core rule misreported as unsupported choice");
        const auto result=rules.apply(state,decision.action);
        require(!result.ok&&result.reason.find("owner decision")!=std::string::npos,"held rule not explicitly rejected");
        require(serializeCampaign(state)==before,"held command mutated state");
        std::cout<<"HELD available="<<decision.available<<" ok="<<result.ok<<" reason="<<result.reason<<'\n';
    });
    std::cout<<"SUMMARY "<<passed<<" passed, "<<failed<<" failed\n";
    return failed?1:0;
}

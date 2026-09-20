#include "overkill/robots.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>

// Independent prepared-state challenges. Mixed formations and direct balances
// below are controlled boundaries, not claims of naturally offered encounters.
using namespace overkill;
namespace {
int checks=0, passed=0, failed=0;
void require(bool condition,const std::string& message) { ++checks;if(!condition)throw std::runtime_error(message); }
void group(const char* name,const std::function<void()>& test) {
    try{test();++passed;std::cout<<"PASS "<<name<<'\n';}
    catch(const std::exception& e){++failed;std::cout<<"FAIL "<<name<<": "<<e.what()<<'\n';}
}
State arena(const std::string& formation) {
    State s;s.encounter="independent/"+formation;s.phase=Phase::Preparation;s.hp=s.maxHp=500;s.hotBarrel=false;
    s.rng=Rng::seeded(91,s.encounter);s.enemies=makeCinderwallFormation(formation,91,s.encounter,s.nextId);return s;
}
Result action(const Rules& rules,State& s,const Action& a) {
    const auto before=serialize(s);const auto preview=rules.preview(s,a);
    require(serialize(s)==before,"preview altered input");
    State restored;std::string error;require(deserialize(before,restored,error),"valid source snapshot: "+error);
    auto r=rules.apply(s,a);require(r.ok,r.reason);
    const auto replay=rules.apply(restored,a);
    require(replay.ok && serialize(s)==serialize(restored) && serialize(s)==serialize(preview.state),"replay/preview full state");
    require(r.events.size()==replay.events.size() && r.events.size()==preview.result.events.size(),"event count parity");
    for(std::size_t i=0;i<r.events.size();++i)
        require(eventJson(r.events[i])==eventJson(replay.events[i]) && eventJson(r.events[i])==eventJson(preview.result.events[i]),"ordered event parity");
    return r;
}
Result end(const Rules& rules,State& s) { auto r=action(rules,s,Action::endTurn());if(s.phase==Phase::Collection)action(rules,s,Action::collect(0));return r; }
void deny(const Rules& rules,State& s,const Action& a) { const auto before=serialize(s);const auto r=rules.apply(s,a);require(!r.ok && r.events.empty() && before==serialize(s),"atomic rejection"); }
Id recipe(State& s,const std::string& id) { const auto n=s.nextId++;s.memory.push_back({n,id,0,0,0});return n; }
Id plain(const Rules& rules,State& s,Kind kind,Amount n,bool installed=false) {
    const Id first=s.nextId;const auto r=rules.grantPlainPart(s,kind,n,"independent prepared grant",installed);require(r.ok,r.reason);
    for(const auto& p:s.parts)if(p.id>=first && p.kind==kind)return p.id;
    throw std::runtime_error("grant has no physical identity");
}
Id grant(const Rules& rules,State& s,const std::string& id) {
    const Id first=s.nextId;const auto r=rules.grantPart(s,id);require(r.ok,r.reason);
    for(const auto& p:s.parts)if(p.id>=first && p.recipe==id)return p.id;
    throw std::runtime_error("recipe grant has no physical identity");
}
int events(const Result& r,const std::string& type,Id subject=0) {
    return static_cast<int>(std::count_if(r.events.begin(),r.events.end(),[&](const Event& e){return e.type==type && (!subject || subject==e.subject);}));
}
std::vector<Amount> damage(const Result& r) {std::vector<Amount> v;for(const auto& e:r.events)if(e.type=="player_damage")v.push_back(e.amount);return v;}
}
int main() {
    const Rules rules;
    group("E01 Binder actual paid Ammo surcharge excludes Shield and stored Fire; unaffordable use is atomic",[&]{
        auto s=arena("C1-F-BINDER");end(rules,s);end(rules,s);
        require(s.recipeFouling==1 && s.foulingRound==s.round,"actual next-turn Foul application");
        const auto saved=plain(rules,s,Kind::Ammo,6);action(rules,s,Action::load({saved}));action(rules,s,Action::fire(s.enemies[0].id));
        require(s.recipeFouling==1,"stored part/Load/Fire must not consume Fouling");
        s.materials={1,1,0,0,0};action(rules,s,Action::craft(recipe(s,"SH002")));
        require(s.recipeFouling==1 && s.materials==Materials{},"Shield pays printed Iron+Copper only");
        const auto slug=recipe(s,"SH001");s.materials={1,0,0,0,0};deny(rules,s,Action::craft(slug));
        require(s.recipeFouling==1,"unaffordable Ammo cannot consume penalty");
        s.materials={2,0,0,0,0};action(rules,s,Action::craft(slug));
        require(s.recipeFouling==0 && s.materials==Materials{},"one paid Ammo consumes precisely one extra Iron/use");
        auto stacked=arena("C1-F-BINDER");
        for(int i=0;i<2;++i){auto extra=makeCinderwallFormation("C1-F-BINDER",92+static_cast<std::uint64_t>(i),stacked.encounter,stacked.nextId);stacked.enemies.push_back(extra.front());}
        end(rules,stacked);end(rules,stacked);require(stacked.recipeFouling==2,"three controlled sources cap at two uses");
        end(rules,stacked);require(stacked.recipeFouling==0,"unused penalty expires at following End Turn");
    });
    group("E02 Press Leak drains assembled parts once and excludes direct protection even when no parts exist",[&]{
        for(bool parts:{false,true}){
            auto s=arena("C1-F-PRESS");end(rules,s);end(rules,s);require(s.shieldLeak==2,"actual double-punch Leak");
            s.protection.push_back({s.nextId++,s.nextOrder++,10}); // Explicitly controlled active upgrade-style balance.
            if(parts){plain(rules,s,Kind::Shield,2,true);plain(rules,s,Kind::Shield,3,true);}
            const auto hp=s.hp;const auto r=end(rules,s);
            require(events(r,"shield_leak")==1 && s.shieldLeak==0,"one consumed Leak at this End Turn");
            for(const auto& e:r.events)if(e.type=="shield_leak")require(e.amount==(parts?2:0),"only installed total loses two");
            require(s.hp==hp-(parts?0:3),"13-damage slam respects direct ten plus remaining part three");
        }
    });
    group("E03 two spread hits cannot retarget a destroyed Chassis to its newly born Mite",[&]{
        auto s=arena("C1-F-CHASSIS");const Id chassis=s.enemies[0].id;
        auto ram=makeCinderwallFormation("C1-F-RAM",93,s.encounter,s.nextId);s.enemies.push_back(ram.front());
        const auto ammo=plain(rules,s,Kind::Ammo,96),left=grant(rules,s,"SH005"),right=grant(rules,s,"SH005");
        action(rules,s,Action::load({ammo,left,right}));const auto r=action(rules,s,Action::fire(ram.front().id,{{left,chassis},{right,chassis}}));
        require(s.kills==2 && s.phase==Phase::Preparation,"death-spawn prevents premature victory");
        require(events(r,"death_release",chassis)==1 && events(r,"robot_deployed",chassis)==1 && events(r,"core_dropped")==2,"one release and distinct body cores");
        require(events(r,"hit_lost",right)==1,"second scheduled carrier hit is lost");
        const auto mite=std::find_if(s.enemies.begin(),s.enemies.end(),[](const Enemy& e){return e.definition=="C1-R01"&&!e.dead;});
        require(mite!=s.enemies.end() && mite->hp==7 && mite->bornRound==s.round,"replacement is untouched new entity");
        const auto first=end(rules,s);require(damage(first).empty(),"player-phase newborn cannot act this round");
        require(damage(end(rules,s))==std::vector<Amount>{5},"new entity starts next round");
    });
    group("E04 Nest death ends remaining supply without deleting its already living helper",[&]{
        auto s=arena("C1-F-NEST");const Id nest=s.enemies[0].id;end(rules,s);
        require(s.enemies.size()==2 && s.enemies[1].hp==7,"one actual opening helper");
        auto ammo=plain(rules,s,Kind::Ammo,28);action(rules,s,Action::load({ammo}));const auto killed=action(rules,s,Action::fire(nest));
        require(events(killed,"core_dropped",nest)==1 && s.enemies[0].summonsRemaining==0,"dead Nest has no remaining supply");
        require(s.phase==Phase::Preparation && !s.enemies[1].dead,"existing child survives independently");
        for(int i=0;i<3;++i){const auto r=end(rules,s);require(events(r,"robot_deployed")==0 && damage(r)==std::vector<Amount>{5},"dead Nest cannot deploy/attack; living helper can");}
    });
    group("E05 Drive Fault clamps each hit before first-hit Mark and decays only once",[&]{
        auto s=arena("C1-F-BINDER");s.enemies[0].weaken=6;s.mark=7;
        require(damage(end(rules,s))==std::vector<Amount>({7,0}),"two base-four hits clamp then one player Mark");
        require(s.enemies[0].weaken==5 && s.mark==0,"not per-hit decay or repeated Mark");
        require(damage(end(rules,s))==std::vector<Amount>{1},"next Foul uses persistent five-point Weaken");
        require(s.enemies[0].weaken==4,"one decay after whole next enemy phase");
    });
    group("E06 nonlethal enemy Acid Etch bypasses defenses and decays through attack and support turns",[&]{
        auto s=arena("C1-F-CASK");s.enemies[0].corrosion=3;s.enemies[0].shield=100;s.enemies[0].tiles=3;
        require(damage(end(rules,s))==std::vector<Amount>{8},"first surviving acid victim attacks");
        require(s.enemies[0].hp==21 && s.enemies[0].corrosion==2,"first three-point direct HP tick and one decay");
        require(damage(end(rules,s)).empty(),"second tick precedes nonattack pressure");
        require(s.enemies[0].hp==19 && s.enemies[0].corrosion==1,"second two-point tick on support turn");
        require(damage(end(rules,s))==std::vector<Amount>{10},"third surviving tick precedes strengthened strike");
        require(s.enemies[0].hp==18 && s.enemies[0].corrosion==0 && s.enemies[0].shield==100 && s.enemies[0].tiles==3,"final one-point tick, no defense consumption, expires at zero");
    });
    std::cout<<"INDEPENDENT_ENCOUNTERS groups="<<passed<<" failed="<<failed<<" assertions="<<checks<<'\n';return failed?1:0;
}

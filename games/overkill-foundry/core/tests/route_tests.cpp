#include "overkill/route.hpp"
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
using namespace overkill;
namespace {
int checks=0;
void check(bool ok,const std::string& why){++checks;if(!ok)throw std::runtime_error(why);}
std::string signature(const CityRoute& r){
    std::ostringstream out;out<<r.position<<r.district<<r.previousFormation<<r.binderDefaultSeen;
    for(auto n:r.rng.state)out<<n<<';';
    for(bool b:r.officer)out<<b;for(bool b:r.mystery)out<<b;
    for(const auto& n:r.nodes){out<<n.selected<<n.resolved;for(const auto& o:n.offers)out<<o.id<<static_cast<int>(o.kind)<<o.formation<<o.mystery<<o.district<<o.encounterKey<<o.encounterSeed<<o.binderDefaultSeen;}
    return out.str();
}
}
int main(){try{
    std::set<std::string> schedules,firstOfficers;std::set<EncounterKind> kinds;
    for(std::uint64_t seed=0;seed<1000;++seed){
        auto a=makeCinderwallRoute(seed),b=makeCinderwallRoute(seed);std::string error;
        std::string schedule;for(bool n:a.officer)schedule+=n?'1':'0';for(bool n:a.mystery)schedule+=n?'1':'0';schedules.insert(schedule);
        for(int pos=1;pos<=12;++pos){
            check(validateRoute(a,error),error);check(signature(a)==signature(b),"Same seed/choices changed route.");
            const auto stable=signature(a);const auto offers=routeOffers(a);
            for(int revisit=0;revisit<4;++revisit)(void)routeOffers(a);
            check(signature(a)==stable,"Reading offers consumed RNG or changed saved choices.");
            for(const auto& offer:offers){kinds.insert(offer.kind);if(offer.kind==EncounterKind::Officer && pos==4)firstOfficers.insert(offer.formation);}
            check(!selectRouteOffer(a,999999,error) && signature(a)==stable,"Invalid choice mutated route.");
            // Vary actual district/category choices, including patrol and noncombat Mysteries.
            const auto chosen=offers[static_cast<std::size_t>((seed+static_cast<std::uint64_t>(pos))%offers.size())].id;
            check(selectRouteOffer(a,chosen,error),error);check(selectRouteOffer(b,chosen,error),error);
            check(selectRouteOffer(a,chosen,error),"Repeated same choice should be stable.");
            if(offers.size()>1){const auto other=offers[0].id==chosen?offers[1].id:offers[0].id;const auto prior=signature(a);check(!selectRouteOffer(a,other,error) && signature(a)==prior,"Changed an already committed choice.");}
            const auto* selected=selectedRouteOffer(a);check(selected && selected->id==chosen,"Lost selected offer.");
            const auto district=selected->district;
            check(completeRouteNode(a,error),error);check(completeRouteNode(b,error),error);
            if(pos==4 || pos==5 || pos==8 || pos==9)check(a.district==district,"Committed district ended too early.");
            if(pos==6 || pos==10)check(a.district.empty(),"Committed district leaked past gate.");
        }
        check(a.position==13 && routeOffers(a).empty(),"Boss did not complete city.");check(validateRoute(a,error),error);
        const auto prior=signature(a);check(!completeRouteNode(a,error) && signature(a)==prior,"Completed city advanced twice.");
        auto corrupt=a;corrupt.nodes[0].offers[0].encounterSeed^=1;check(!validateRoute(corrupt,error),"Accepted rerolled encounter identity.");
    }
    check(schedules.size()>400,"Schedule sampling did not exercise expected variety.");check(kinds.size()==4,"Missing category coverage.");check(firstOfficers==std::set<std::string>{"C1-F-PURSUER"},"Preferred first Officer changed.");
    std::cout<<"PASS route schedules: 1000 seeded complete choices, gates and immutable encounter identities\n";
    for(std::uint64_t seed=0;seed<100;++seed){
        auto route=makeCinderwallRoute(seed);route.safeMysteries=true;std::string error;std::set<std::string> observedOfficers;Amount passes=3;
        while(route.position<=12){
            const auto offers=routeOffers(route);const auto rng=route.rng.state;
            for(const auto& offer:offers){
                if(offer.kind==EncounterKind::Mystery)check(offer.mystery!="C1-M-PATROL","Safe Survey generated a hidden fight.");
                if(passes>0 && offer.kind==EncounterKind::Officer){
                    RouteOffer a,b;if(previewRouteReplacement(route,offer.id,a,error)){
                        check(previewRouteReplacement(route,offer.id,b,error) && a.formation==b.formation && a.encounterSeed==b.encounterSeed,"Repeated route preview changed alternative.");check(route.rng.state==rng,"Route preview consumed a stream.");
                        check(replaceRouteOffer(route,a,error),error);--passes;check(!replaceRouteOffer(route,a,error),"Spent Officer alternative rerolled.");check(validateRoute(route,error),error);
                    }
                }
            }
            auto chosen=routeOffers(route).front().id;for(const auto& offer:routeOffers(route))if(offer.kind==EncounterKind::Officer){check(observedOfficers.insert(offer.formation).second,"Officer pass caused a later duplicate.");chosen=offer.id;}
            check(selectRouteOffer(route,chosen,error),error);check(completeRouteNode(route,error),error);check(validateRoute(route,error),error);
        }
        check(observedOfficers.size()==3,"Route pass skipped Officer progression.");
    }
    std::cout<<"PASS route replacements: 100 Safe Survey paths, deterministic previews and Officer progression\n";
    std::cout<<"PASS "<<checks<<" route assertions over 1,000 complete choice paths; "<<schedules.size()<<" schedules observed. This is route validation, not combat simulation.\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}

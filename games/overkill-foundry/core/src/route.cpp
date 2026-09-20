#include "overkill/route.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>

namespace overkill {
namespace {
const std::vector<std::string> lanes={"C1-D-FOUNDRY","C1-D-PATROL","C1-D-WORKSHOP"};
const std::vector<std::string> opening={"C1-F-MITE-RAM","C1-F-RAM","C1-F-CASK"};
std::uint64_t encounterSeed(std::uint64_t seed,const std::string& key){
    std::uint64_t h=14695981039346656037ULL;
    for(unsigned char c:key){h^=c;h*=1099511628211ULL;}
    h^=seed;h+=0x9e3779b97f4a7c15ULL;
    h=(h^(h>>30))*0xbf58476d1ce4e5b9ULL;h=(h^(h>>27))*0x94d049bb133111ebULL;
    return h^(h>>31);
}
bool contains(const std::vector<std::string>& values,const std::string& value){return std::find(values.begin(),values.end(),value)!=values.end();}
std::vector<std::string> legalRegular(Amount position){
    auto out=opening;
    if(position>=4){out.push_back("C1-F-PRESS");out.push_back("C1-F-BINDER");}
    if(position>=7 && position<=10)out.push_back("C1-F-NEST");
    return out;
}
bool favoured(const std::string& district,const std::string& formation){
    if(district==lanes[0])return formation=="C1-F-RAM" || formation=="C1-F-CASK" || formation=="C1-F-PRESS";
    if(district==lanes[1])return formation=="C1-F-MITE-RAM" || formation=="C1-F-NEST";
    if(district==lanes[2])return formation=="C1-F-BINDER" || formation=="C1-F-PRESS";
    return false;
}
std::string regular(CityRoute& r,const std::string& district,bool simple,const std::vector<std::string>& used,std::size_t regularCount){
    auto pool=simple?opening:legalRegular(r.position);
    const auto all=pool;
    pool.erase(std::remove_if(pool.begin(),pool.end(),[&](const std::string& f){return contains(used,f);}),pool.end());
    if(pool.empty())pool=all;
    // Prior exclusion is conditional on enough distinct templates remaining.
    if(all.size()>regularCount && pool.size()>1){
        pool.erase(std::remove(pool.begin(),pool.end(),r.previousFormation),pool.end());
    }
    std::uint32_t total=0;for(const auto& f:pool)total+=favoured(district,f)?2U:1U;
    auto roll=r.rng.below(Domain::Formation,total);
    for(const auto& f:pool){const auto weight=favoured(district,f)?2U:1U;if(roll<weight)return f;roll-=weight;}
    throw std::logic_error("Empty legal formation pool.");
}
void prepareOffers(CityRoute& r){
    if(r.position>12)return;
    auto& node=r.nodes[static_cast<std::size_t>(r.position-1)];
    if(!node.offers.empty())return;
    if(r.position==7 || r.position==11)r.district.clear();
    std::vector<EncounterKind> kinds;
    const auto index=static_cast<std::size_t>(r.position-1);
    if(r.position==12)kinds={EncounterKind::Boss};
    else {
        kinds.push_back(EncounterKind::Regular);
        kinds.push_back(r.officer[index]?EncounterKind::Officer:EncounterKind::Regular);
        kinds.push_back(r.mystery[index]?EncounterKind::Mystery:EncounterKind::Regular);
        // Category/route-lane pairings are fixed by the route stream, not a UI refresh.
        for(std::size_t i=kinds.size();i>1;--i)std::swap(kinds[i-1],kinds[r.rng.below(Domain::Route,static_cast<std::uint32_t>(i))]);
    }
    std::vector<std::string> used;bool simplePending=true;
    const auto regularCount=static_cast<std::size_t>(std::count(kinds.begin(),kinds.end(),EncounterKind::Regular));
    for(std::size_t i=0;i<kinds.size();++i){
        RouteOffer offer;offer.id=r.nextOfferId++;offer.kind=kinds[i];
        offer.district=(r.position==4 || r.position==8)?lanes[i]:r.district;
        offer.encounterKey="Mara/cinderwall/position/"+std::to_string(r.position)+"/offer/"+std::to_string(offer.id);
        offer.encounterSeed=encounterSeed(r.seed,offer.encounterKey);offer.binderDefaultSeen=r.binderDefaultSeen;
        if(offer.kind==EncounterKind::Regular){
            offer.formation=regular(r,offer.district,simplePending,used,regularCount);
            used.push_back(offer.formation);simplePending=false;
        }else if(offer.kind==EncounterKind::Officer){
            const auto previous=std::count(r.officer.begin(),r.officer.begin()+index,true);
            offer.formation=r.officerOrder[static_cast<std::size_t>(previous)];
        }else if(offer.kind==EncounterKind::Boss)offer.formation="C1-F-GATEBREAKER";
        else {
            const auto draw=r.rng.below(Domain::Route,100);
            offer.mystery=draw<40?"C1-M-PATROL":draw<70?"C1-M-EXCHANGE":"C1-M-TECH";
            if(offer.mystery=="C1-M-PATROL")offer.formation=regular(r,offer.district,false,{},0);
        }
        node.offers.push_back(std::move(offer));
    }
}
} // namespace
CityRoute makeCinderwallRoute(std::uint64_t seed){
    CityRoute r;r.seed=seed;r.rng=Rng::seeded(seed,"Mara/cinderwall/route");
    std::vector<std::array<int,3>> choices;
    for(int a=4;a<=10;++a)for(int b=a+2;b<=10;++b)for(int c=b+2;c<=10;++c)choices.push_back({a,b,c});
    for(int p:choices[r.rng.below(Domain::Route,static_cast<std::uint32_t>(choices.size()))])r.officer[static_cast<std::size_t>(p-1)]=true;
    choices.clear();
    for(int a=4;a<=11;++a)for(int b=a+1;b<=11;++b)for(int c=b+1;c<=11;++c)choices.push_back({a,b,c});
    for(int p:choices[r.rng.below(Domain::Route,static_cast<std::uint32_t>(choices.size()))])r.mystery[static_cast<std::size_t>(p-1)]=true;
    r.officerOrder={"C1-F-PURSUER","C1-F-WARDEN","C1-F-CHASSIS"};
    if(r.rng.below(Domain::Route,2))std::swap(r.officerOrder[1],r.officerOrder[2]);
    prepareOffers(r);return r;
}
const std::vector<RouteOffer>& routeOffers(const CityRoute& r){
    static const std::vector<RouteOffer> empty;
    return r.position>=1 && r.position<=12?r.nodes[static_cast<std::size_t>(r.position-1)].offers:empty;
}
const RouteOffer* selectedRouteOffer(const CityRoute& r){
    if(r.position<1 || r.position>12)return nullptr;
    const auto& node=r.nodes[static_cast<std::size_t>(r.position-1)];
    for(const auto& offer:node.offers)if(offer.id==node.selected)return &offer;
    return nullptr;
}
bool selectRouteOffer(CityRoute& r,Id offer,std::string& error){
    if(r.position<1 || r.position>12){error="The city route is complete.";return false;}
    auto& node=r.nodes[static_cast<std::size_t>(r.position-1)];
    if(node.selected){if(node.selected==offer)return true;error="This encounter choice has already committed.";return false;}
    const auto it=std::find_if(node.offers.begin(),node.offers.end(),[&](const RouteOffer& o){return o.id==offer;});
    if(it==node.offers.end()){error="Select a current route offer.";return false;}
    node.selected=offer;if(r.position==4 || r.position==8)r.district=it->district;return true;
}
bool completeRouteNode(CityRoute& r,std::string& error){
    const auto* selected=selectedRouteOffer(r);
    if(!selected){error="No encounter is active.";return false;}
    auto& node=r.nodes[static_cast<std::size_t>(r.position-1)];
    if(node.resolved){error="The encounter is already complete.";return false;}
    if(selected->formation=="C1-F-BINDER")r.binderDefaultSeen=true;
    if(!selected->formation.empty())r.previousFormation=selected->formation;
    node.resolved=true;++r.position;prepareOffers(r);return true;
}
bool validateRoute(const CityRoute& r,std::string& error){
    auto fail=[&](const char* why){error=why;return false;};
    if(r.position<1 || r.position>13 || r.nextOfferId<1)return fail("Invalid city position or identity.");
    if(std::count(r.officer.begin(),r.officer.end(),true)!=3 || std::count(r.mystery.begin(),r.mystery.end(),true)!=3)return fail("Invalid opportunity count.");
    std::set<std::string> officers(r.officerOrder.begin(),r.officerOrder.end());
    if(r.officerOrder.size()!=3 || r.officerOrder.front()!="C1-F-PURSUER" || officers!=std::set<std::string>{"C1-F-PURSUER","C1-F-WARDEN","C1-F-CHASSIS"})return fail("Invalid Officer schedule identities.");
    std::set<Id> ids;std::string committedDistrict,previousFormation;bool binderSeen=false;
    for(std::size_t i=0;i<12;++i){
        const auto pos=static_cast<Amount>(i+1);const auto& node=r.nodes[i];
        if(r.officer[i] && (pos<4 || pos>10 || (i>0 && r.officer[i-1])))return fail("Invalid Officer gate.");
        if(r.mystery[i] && (pos<4 || pos>11))return fail("Invalid Mystery gate.");
        if(pos>r.position){if(!node.offers.empty() || node.selected || node.resolved)return fail("Future route offers are not yet resolved.");continue;}
        if(node.offers.size()!=(pos==12?1U:3U))return fail("Invalid offer count.");
        if(pos==12 && node.offers.front().kind!=EncounterKind::Boss)return fail("Final encounter must be the sole boss.");
        if((pos<r.position)!=node.resolved || (node.resolved && !node.selected))return fail("Invalid route completion history.");
        if(pos==7 || pos==11)committedDistrict.clear();
        bool selectionFound=node.selected==0,simple=false;int regulars=0,off=0,mystery=0;
        std::set<std::string> forkLanes;
        for(const auto& o:node.offers){
            if(o.id==0 || o.id>=r.nextOfferId || !ids.insert(o.id).second)return fail("Invalid route-offer identity.");
            if(o.id==node.selected)selectionFound=true;
            if(o.kind>EncounterKind::Boss || o.encounterKey!="Mara/cinderwall/position/"+std::to_string(pos)+"/offer/"+std::to_string(o.id) || o.encounterSeed!=encounterSeed(r.seed,o.encounterKey))return fail("Invalid encounter identity or seed.");
            if(!o.district.empty() && !contains(lanes,o.district))return fail("Unknown district.");
            if(pos!=4 && pos!=8 && o.district!=committedDistrict)return fail("Offer escaped the committed district.");
            if(o.binderDefaultSeen!=binderSeen)return fail("Robot exposure history changed.");
            if(pos==4 || pos==8)forkLanes.insert(o.district);
            if(o.kind==EncounterKind::Regular){++regulars;if(!contains(legalRegular(pos),o.formation) || !o.mystery.empty())return fail("Illegal Regular formation.");simple=simple || contains(opening,o.formation);}
            else if(o.kind==EncounterKind::Officer){++off;const auto ordinal=static_cast<std::size_t>(std::count(r.officer.begin(),r.officer.begin()+i,true));if(!r.officer[i] || ordinal>=r.officerOrder.size() || o.formation!=r.officerOrder[ordinal] || !o.mystery.empty())return fail("Illegal Officer offer.");}
            else if(o.kind==EncounterKind::Mystery){++mystery;if(!r.mystery[i] || (o.mystery!="C1-M-PATROL" && o.mystery!="C1-M-EXCHANGE" && o.mystery!="C1-M-TECH"))return fail("Illegal Mystery offer.");if(o.mystery=="C1-M-PATROL"?!contains(legalRegular(pos),o.formation):!o.formation.empty())return fail("Illegal Mystery formation.");}
            else if(pos!=12 || o.formation!="C1-F-GATEBREAKER" || !o.mystery.empty())return fail("Illegal boss offer.");
        }
        if(!selectionFound)return fail("Unknown committed route choice.");
        if(pos<12 && (!simple || off!=(r.officer[i]?1:0) || mystery!=(r.mystery[i]?1:0) || regulars!=3-off-mystery))return fail("Invalid ordinary category mix.");
        if((pos==4 || pos==8) && forkLanes!=std::set<std::string>(lanes.begin(),lanes.end()))return fail("Missing district fork choice.");
        for(const auto& o:node.offers)if(o.id==node.selected){
            if(pos==4 || pos==8)committedDistrict=o.district;
            if(node.resolved){if(o.formation=="C1-F-BINDER")binderSeen=true;if(!o.formation.empty())previousFormation=o.formation;}
        }
    }
    if(r.district!=committedDistrict || r.previousFormation!=previousFormation || r.binderDefaultSeen!=binderSeen)return fail("Route history does not match committed nodes.");
    return true;
}
} // namespace overkill

#pragma once
#include "overkill/core.hpp"
#include <array>

namespace overkill {
enum class EncounterKind : std::uint8_t { Regular, Officer, Mystery, Boss };
struct RouteOffer {
    Id id=0;
    EncounterKind kind=EncounterKind::Regular;
    std::string formation, mystery, district, encounterKey;
    std::uint64_t encounterSeed=0;
    bool binderDefaultSeen=false;
    bool replaced=false,surveyed=false;
};
struct RouteNode {
    std::vector<RouteOffer> offers;
    Id selected=0;
    bool resolved=false;
};
struct CityRoute {
    std::uint64_t seed=1;
    Rng rng;
    Id nextOfferId=1;
    Amount position=1;
    std::array<bool,12> officer{}, mystery{};
    std::array<RouteNode,12> nodes;
    std::vector<std::string> officerOrder;
    std::string district, previousFormation;
    bool binderDefaultSeen=false;
    bool safeMysteries=false;
};

// Cinderwall's complete schedule is fixed at creation. Current offers are generated
// once from that schedule/history; UI reads and shopping never consume route RNG.
CityRoute makeCinderwallRoute(std::uint64_t seed);
const std::vector<RouteOffer>& routeOffers(const CityRoute& route);
const RouteOffer* selectedRouteOffer(const CityRoute& route);
bool selectRouteOffer(CityRoute& route,Id offer,std::string& error);
bool completeRouteNode(CityRoute& route,std::string& error);
// Pure deterministic preview, followed by an atomic explicit commitment. A
// spent option cannot be replaced again. Officer replacements swap an unseen
// scheduled Officer so the three opportunities still use distinct identities.
bool previewRouteReplacement(const CityRoute& route,Id offer,RouteOffer& replacement,std::string& error);
bool replaceRouteOffer(CityRoute& route,const RouteOffer& replacement,std::string& error);
bool validateRoute(const CityRoute& route,std::string& error);
} // namespace overkill

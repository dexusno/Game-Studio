#include "overkill/campaign_session.hpp"
#include "overkill/robots.hpp"
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>

// A deliberately synthetic pose/composition fixture. This is not an authored
// campaign formation, an earned boss visit, or difficulty/balance evidence.
int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("Usage: foundry_operator_fixtures NEW_OUTPUT_DIRECTORY");
        const std::filesystem::path dir = argv[1];
        if (std::filesystem::exists(dir)) throw std::runtime_error("Use a new directory; never replace saves.");
        overkill::Rules fights;
        overkill::CampaignRules rules(fights, overkill::cinderwallUpgradeHooks(fights));
        auto campaign = rules.newGame(1, "prepared-operator-targets");
        for (std::uint64_t seed = 2; seed < 512 &&
             std::find(campaign.mayorOffers.begin(), campaign.mayorOffers.end(), "MY1-17") == campaign.mayorOffers.end(); ++seed)
            campaign = rules.newGame(seed, "prepared-operator-targets");
        auto apply = [&](overkill::CampaignAction action) {
            action.runId = campaign.runId;
            action.sequence = campaign.nextTransaction;
            const auto result = rules.apply(campaign, action);
            if (!result.ok) throw std::runtime_error(result.reason);
        };
        overkill::CampaignAction action;
        action.type = overkill::CampaignActionType::ChooseMayor;
        action.choice = "MY1-17";
        apply(action);
        action = {};
        action.type = overkill::CampaignActionType::EnterOffer;
        for (const auto& offer : overkill::routeOffers(campaign.route))
            if (offer.kind == overkill::EncounterKind::Regular) { action.subject = offer.id; break; }
        apply(action);
        campaign.fight.enemies = overkill::makeCinderwallFormation("C1-F-MITE-RAM", campaign.fight.seed,
            "prepared-operator-targets", campaign.fight.nextId);
        auto boss = overkill::makeCinderwallFormation("C1-F-GATEBREAKER", campaign.fight.seed,
            "prepared-operator-targets", campaign.fight.nextId);
        // Mite uses its near lane; Ram uses large slot0; Gatebreaker uses far slot1.
        campaign.fight.enemies.push_back(boss.front());
        action = {};
        action.type = overkill::CampaignActionType::Combat;
        action.combat = overkill::Action::collect(3);
        apply(action);
        for (int i = 0; i < 6; ++i) {
            const auto result = fights.grantPlainPart(campaign.fight, overkill::Kind::Ammo, 4,
                "Prepared operator pose fixture");
            if (!result.ok) throw std::runtime_error(result.reason);
        }
        const auto shield = fights.grantPlainPart(campaign.fight, overkill::Kind::Shield, 80,
            "Prepared operator pose fixture", true);
        if (!shield.ok) throw std::runtime_error(shield.reason);
        const auto bytes = overkill::serializeCampaign(campaign);
        std::filesystem::create_directories(dir);
        const auto path = dir / "operator-three-targets.ofsave";
        overkill::SaveStore store(path);
        const auto saved = store.commit(bytes, "");
        if (!saved.ok) throw std::runtime_error(saved.error);
        overkill::CampaignSession session(path, rules);
        const auto loaded = session.load();
        if (!loaded.ok || overkill::serializeCampaign(session.state()) != bytes)
            throw std::runtime_error("Prepared save failed canonical round trip");
        std::cout << "PREPARED_OPERATOR_FIXTURE_OK production_run=0 enemies=" << campaign.fight.enemies.size()
                  << " parts=" << campaign.fight.parts.size() << " hash=" << overkill::campaignHash(session.state()) << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

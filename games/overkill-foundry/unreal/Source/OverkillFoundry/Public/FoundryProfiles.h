#pragma once
#if FOUNDRY_WITH_CAMPAIGN
#include "overkill/campaign_session.hpp"
#include <memory>
#include <vector>

// Profile identity and display metadata only. Every campaign, discovery and
// unlock remains in its ordinary CampaignSession/SaveStore envelope.
namespace foundry_profiles {
struct Profile {
    std::string id, name, note;
    std::filesystem::path path;
    bool hasSave=false;
};
struct Result { bool ok=false; std::string reason; };
class Store {
public:
    Store(std::filesystem::path defaultPath,const overkill::CampaignRules& rules,bool fixedPath=true);
    bool fixed() const { return fixed_; }
    const std::filesystem::path& path() const { return path_; }
    const std::string& activeId() const { return activeId_; }
    const std::string& activeName() const { return activeName_; }
    const std::string& notice() const { return notice_; }
    bool hasSave() const;
    std::vector<Profile> profiles() const;
    Result create(const std::string& id,const std::string& name,const std::function<void(overkill::SavePoint)>& faultProbe={});
    Result select(const std::string& id);
    // The existing session API is intentionally forwarded unchanged, including
    // fault/recovery/CAS behavior and the retained ProfileFacts on New Game.
    overkill::SessionResult load() { return session_->load(); }
    overkill::SessionResult newGame(std::uint64_t seed,const std::string& runId,const std::function<void(overkill::SavePoint)>& probe={}) { return session_->newGame(seed,runId,probe); }
    overkill::SessionResult apply(const overkill::CampaignAction& action,const std::function<void(overkill::SavePoint)>& probe={}) { return session_->apply(action,probe); }
    bool loaded() const { return session_->loaded(); }
    const overkill::Campaign& state() const { return session_->state(); }
    std::uint64_t revision() const { return session_->revision(); }
private:
    const overkill::CampaignRules& rules_;
    std::filesystem::path defaultPath_, directory_, path_;
    bool fixed_=true;
    std::string activeId_="default", activeName_="Default", notice_;
    std::unique_ptr<overkill::CampaignSession> session_;
    std::filesystem::path profileDirectory(const std::string& id) const;
    Result rememberSelection(const std::string& id) const;
};
}
#endif

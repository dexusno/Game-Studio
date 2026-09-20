#pragma once
#include "overkill/campaign.hpp"
#include "overkill/save_store.hpp"

namespace overkill {
struct SessionResult {
    bool ok=false,replayed=false,reconciled=false,recoveredPrevious=false;
    std::string reason;
    std::vector<Event> events;
};
// Owns the committed view shown by either client. Rules operate on a copy; a
// failed/ambiguous write cannot publish that speculative copy to the UI.
class CampaignSession {
public:
    CampaignSession(std::filesystem::path path,const CampaignRules& rules):store_(std::move(path)),rules_(rules){}
    SessionResult load();
    // A new save uses an empty token. Existing saves must be loaded first so a
    // restart preserves profile facts and cannot overwrite a concurrent writer.
    SessionResult newGame(std::uint64_t seed,const std::string& runId,
                          const std::function<void(SavePoint)>& faultProbe={});
    SessionResult apply(const CampaignAction& action,const std::function<void(SavePoint)>& faultProbe={});
    bool loaded() const {return loaded_;}
    const Campaign& state() const;
    std::uint64_t revision() const {return revision_;}
private:
    SaveStore store_;
    const CampaignRules& rules_;
    Campaign campaign_;
    bool loaded_=false;
    std::string token_;
    std::uint64_t revision_=0;
    bool accept(const StoredSave& stored,std::string& error);
    SessionResult persist(Campaign candidate,CampaignResult result,const CampaignAction* action,
                          const std::function<void(SavePoint)>& faultProbe);
};
} // namespace overkill

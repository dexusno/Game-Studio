#pragma once
#include "overkill/campaign.hpp"
#include <chrono>
#include <iosfwd>
#include <set>
#include <string>
#include <vector>

namespace overkill::runner {
inline constexpr const char* PolicyVersion="baseline-4";
struct Options { std::uint64_t seed=1;std::string policy="aggressive",precision="auto",trace;double timeoutSeconds=30;std::size_t searchBudget=1200; };
struct FightRecord {std::string encounter,kind,outcome;Amount hpStart=0,hpEnd=0,rounds=0,shots=0,kills=0;std::vector<std::string> recipes,upgrades,choices;};
struct RunRecord {std::uint64_t seed=0;std::string policy,precision,outcome,reason,hash,trace;Amount hp=0,position=0;std::size_t actions=0,previews=0;double seconds=0;std::int64_t income=0,spending=0,healing=0,hpLost=0,hpPaid=0;Materials spent{},gained{};std::vector<FightRecord> fights;std::vector<std::string> choices;};
std::string encodeAction(const CampaignAction& action);
bool decodeAction(const std::string& text,CampaignAction& action,std::string& error);
std::string hex(const std::string& bytes);
std::string unhex(const std::string& encoded);
std::string summaryJson(const RunRecord& result);
std::string replayTrace(const std::string& file,const CampaignRules& rules);

struct Decision {bool available=false;CampaignAction action;std::string reason;};
class Policy {
public:
    Policy(const Rules& rules,const CampaignRules& campaign,Options options);
    Decision choose(const Campaign& state);
    std::size_t previews()const{return totalPreviews_;}
private:
    const Rules& rules_;const CampaignRules& campaign_;Options options_;
    std::size_t decisionPreviews_=0,totalPreviews_=0;
    std::uint64_t executionRandom_=0;
    std::set<std::string> inspectedShops_;
    double hpWeight()const;
    double resourceWeight()const;
    Preview preview(const State& s,const Action& a);
    std::vector<Action> choices(const State& s,Action base,const std::string& definition={})const;
    std::vector<Action> shots(const State& s)const;
    double simpleValue(const State& s)const;
    double partPotential(const State& s,Id firstNewPart);
    double recipeRank(const State& s,const std::string& id)const;
    double upgradeRank(const State& s,const std::string& id)const;
    Id exchange(const Campaign& s,const std::string& incoming,Id excluded=0)const;
    Amount precisionResult();
    Decision combat(const Campaign& c);
    Decision nested(const Campaign& c);
    Decision shop(const Campaign& c,bool eventStock=false);
};
RunRecord runCity(const Options& options,const Rules& fights,const CampaignRules& rules);
} // namespace overkill::runner

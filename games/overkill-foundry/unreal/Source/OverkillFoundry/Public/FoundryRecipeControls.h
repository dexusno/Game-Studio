#pragma once
#if FOUNDRY_WITH_CAMPAIGN
#include "overkill/core.hpp"
#include <map>
#include <string>
#include <utility>
#include <vector>

// Presentation metadata and reversible selections only. Rules::preview/apply
// remain the sole eligibility, price, damage, haul and transaction authority.
namespace foundry_controls {
enum class Field { Amount, Target, Parts, Material, Sacrifice, ExtraTarget, PartChoice, Discount };
struct Option { overkill::Id value=0; std::string label, recipe; };
struct Choice {
    std::string key,label,detail;
    Field field=Field::Target;
    overkill::Id source=0;
    int index=0,minimum=1,maximum=1;
    std::vector<Option> options;
};
using Selections=std::map<std::string,std::vector<overkill::Id>>;
class Drafts {
public:
    const Selections& selections(const overkill::Action& base) const;
    const std::vector<overkill::Id>& selected(const overkill::Action& base,const std::string& key) const;
    void choose(const overkill::Action& base,const Choice& choice,overkill::Id value);
    void set(const overkill::Action& base,const std::string& key,std::vector<overkill::Id> values);
    void reset(const overkill::Action& base);
    void clear();
    std::vector<Choice> choices(const overkill::Rules& rules,const overkill::State& state,const overkill::Action& base) const;
    overkill::Action build(const overkill::Rules& rules,const overkill::State& state,overkill::Action base) const;
private:
    std::map<std::pair<int,overkill::Id>,Selections> drafts;
};
struct CollectionOption { overkill::Action action; overkill::Preview preview; };
// Loaded rows follow the authoritative bullet order. Each reserved payment is
// shown directly below its source and does not acquire a firing-order number.
std::vector<overkill::Id> loadedDisplayOrder(const overkill::State& state);
bool heavyLiftPending(const overkill::State& state);
// Enumerates complete two-unit discard requests and asks the core which are
// legal. It never reconstructs collection RNG or edits a previewed haul.
std::vector<CollectionOption> collectionOptions(const overkill::Rules& rules,const overkill::State& state,int steering,int precision);
}
#endif

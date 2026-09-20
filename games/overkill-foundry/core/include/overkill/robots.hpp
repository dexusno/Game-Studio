#pragma once
#include "overkill/core.hpp"
#include <functional>

namespace overkill {

// Robot damage/statuses pass back through Engine so player protection, rescue and
// immutable event IDs remain authoritative in one place. A callback may append
// enemies; callers must not retain pointers into State::enemies across it.
enum class RobotStatus : std::uint8_t { Burn, Corrosion, Mark, Weaken, RecipeFouling, ShieldLeak };
struct RobotCallbacks {
    std::function<Id(const std::string&, Id, Id, Amount, Amount, Id)> emit;
    std::function<void(Amount, Id, Id, bool)> playerDamage;
    std::function<void(RobotStatus, Amount, Id, Id)> playerStatus;
};
struct RobotDefinition {
    const char* id;
    const char* name;
    Amount hp, armor, tiles, coreValue;
};
const std::vector<RobotDefinition>& cinderwallRobotDefinitions();
std::vector<std::string> cinderwallFormationIds();
bool isCinderwallRobot(const Enemy& enemy);

// Uses the authored allowlist only. Opening intentions are committed for round 1.
// Invalid formation IDs throw without modifying nextId. The encounter key should
// include mercenary/city/route position/offer identity; caller stores it in State.
std::vector<Enemy> makeCinderwallFormation(const std::string& formationId,
    std::uint64_t seed, const std::string& encounterKey, Id& nextId,
    bool binderDefaultSeen = false);

// Return false for a legacy/test enemy outside the Cinderwall registry, allowing
// Engine's generic pattern path. Repeated commitment in a round cannot reroll.
bool commitRobotIntent(State& state, Id enemy, const RobotCallbacks& callbacks);
bool executeRobotIntent(State& state, Id enemy, const RobotCallbacks& callbacks);
void finishRobotTurn(State& state, Id enemy, const RobotCallbacks& callbacks);
// HP-loss hook: after the hit event, before dead-actor consequences/recoil.
void onRobotHpLoss(State& state, Id enemy, const RobotCallbacks& callbacks);
// Death hook: the Engine has set dead and counted the kill already. This emits a
// unique core allocation and resolves the explicitly announced death deployment.
// It grants no Credits or ordinary on-kill payload before compulsory recoil.
void onRobotDeath(State& state, Id enemy, const RobotCallbacks& callbacks);
std::string robotIntentText(const Enemy& enemy, Amount round);

// No Cinderwall robot has an innate escape schedule. Explicit encounter effects
// may announce one; four complete player turns precede the departure action.
bool announceRobotEscape(State& state, Id enemy, const RobotCallbacks& callbacks);

} // namespace overkill

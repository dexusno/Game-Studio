#include "overkill/robots.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace overkill {
namespace {
bool alive(const Enemy& e) { return !e.dead && !e.escaped && e.hp > 0; }
Enemy* find(State& s, Id id) {
    const auto it = std::find_if(s.enemies.begin(), s.enemies.end(), [id](const Enemy& e) { return e.id == id; });
    return it == s.enemies.end() ? nullptr : &*it;
}
Amount add(Amount a, Amount b) {
    const auto value = static_cast<std::int64_t>(a) + b;
    if (value < 0 || value > std::numeric_limits<Amount>::max()) throw std::overflow_error("Robot quantity exceeds integer range.");
    return static_cast<Amount>(value);
}
Id allocate(State& s) {
    if (s.nextId == 0 || s.nextId == std::numeric_limits<Id>::max()) throw std::overflow_error("Robot identity space exhausted.");
    return s.nextId++;
}
Id emit(const RobotCallbacks& cb, const std::string& type, Id source = 0, Id target = 0,
        Amount amount = 0, Amount secondary = 0, Id parent = 0) {
    return cb.emit ? cb.emit(type, source, target, amount, secondary, parent) : 0;
}
const RobotDefinition* definition(const std::string& id) {
    for (const auto& d : cinderwallRobotDefinitions()) if (id == d.id) return &d;
    return nullptr;
}
std::uint64_t mix(std::uint64_t value) {
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}
std::uint64_t hashText(const std::string& value) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char c : value) { hash ^= c; hash *= 1099511628211ULL; }
    return hash;
}
// A documented SplitMix64 stream per body; route, collection and reward draws
// never change a saved robot packet. Two equiprobable packets need one fair bit.
Amount packetDraw(Enemy& e) {
    e.patternRandom += 0x9e3779b97f4a7c15ULL;
    return static_cast<Amount>((mix(e.patternRandom) >> 32) & 1U);
}
bool liveMite(const State& s) {
    return std::any_of(s.enemies.begin(), s.enemies.end(), [](const Enemy& e) {
        return e.definition == "C1-R01" && alive(e);
    });
}
Enemy makeEnemy(State& s, const std::string& id, bool binderSeen, Amount bornRound) {
    const auto* d = definition(id);
    if (!d) throw std::invalid_argument("Robot is outside the Cinderwall registry.");
    Enemy e;
    e.id = allocate(s); e.definition = d->id; e.name = d->name;
    e.robotAction = "entry"; // Explicit factory marker; legacy teaching fixtures remain generic.
    e.hp = d->hp; e.maxHp = d->hp; e.armor = d->armor; e.tiles = d->tiles; e.coreValue = d->coreValue;
    e.bornRound = bornRound; e.binderVariants = binderSeen;
    e.patternRandom = mix(s.seed ^ hashText("overkill/robot/v1/" + s.encounter + "/" + std::to_string(e.id)));
    if (id == "C1-R07") e.summonsRemaining = 4;
    if (id == "C1-O03") e.summonsRemaining = 1;
    return e;
}
void setIntent(Enemy& e, const char* name, Move move, Amount damage = 0, Amount hits = 0) {
    e.robotAction = name; e.intent = {move, damage, hits};
}
void spawnMite(State& s, Id parentId, const RobotCallbacks& cb, Id eventParent) {
    auto* parent = find(s, parentId);
    if (!parent || parent->summonsRemaining <= 0 || liveMite(s)) {
        emit(cb, "deploy_blocked", parentId, 0, 0, parent ? parent->summonsRemaining : 0, eventParent);
        return;
    }
    --parent->summonsRemaining;
    const Amount remaining = parent->summonsRemaining;
    auto helper = makeEnemy(s, "C1-R01", false, s.round);
    const Id helperId = helper.id;
    s.enemies.push_back(std::move(helper));
    emit(cb, "robot_deployed", parentId, helperId, 7, remaining, eventParent);
    commitRobotIntent(s, helperId, cb);
}
void expireBrace(State& s, Id id, const RobotCallbacks& cb, Id parent) {
    auto* e = find(s, id);
    if (e && e->temporaryArmor > 0 && s.round >= e->armorExpiresRound) {
        const Amount lost = e->temporaryArmor;
        e->armor = std::max(0, e->armor - lost); e->temporaryArmor = 0; e->armorExpiresRound = 0;
        emit(cb, "armor_expired", id, 0, lost, 0, parent);
    }
}
} // namespace

const std::vector<RobotDefinition>& cinderwallRobotDefinitions() {
    static const std::vector<RobotDefinition> definitions{
        {"C1-R01", "Rivet Mite", 7, 0, 0, 5},
        {"C1-R02", "Breach Ram", 24, 3, 0, 15},
        {"C1-R04", "Cable Binder", 22, 0, 0, 15},
        {"C1-R05", "Pressure Cask", 24, 1, 0, 15},
        {"C1-R07", "Coil Nest", 28, 0, 0, 18},
        {"C1-R08", "Knuckle Press", 28, 0, 0, 18},
        {"C1-O01", "Redline Pursuer", 52, 0, 0, 40},
        {"C1-O02", "Foil Warden", 44, 0, 3, 40},
        {"C1-O03", "Split Chassis", 48, 0, 0, 35},
        {"C1-B01", "Gatebreaker Prime", 92, 0, 0, 100}
    };
    return definitions;
}
std::vector<std::string> cinderwallFormationIds() {
    return {"C1-F-MITE-RAM", "C1-F-RAM", "C1-F-CASK", "C1-F-PRESS", "C1-F-BINDER",
        "C1-F-NEST", "C1-F-PURSUER", "C1-F-WARDEN", "C1-F-CHASSIS", "C1-F-GATEBREAKER"};
}
bool isCinderwallRobot(const Enemy& e) { return !e.robotAction.empty() && definition(e.definition) != nullptr; }

std::vector<Enemy> makeCinderwallFormation(const std::string& id, std::uint64_t seed,
        const std::string& encounterKey, Id& nextId, bool binderDefaultSeen) {
    std::vector<std::string> bodies;
    if (id == "C1-F-MITE-RAM") bodies = {"C1-R01", "C1-R02"};
    else if (id == "C1-F-RAM") bodies = {"C1-R02"};
    else if (id == "C1-F-CASK") bodies = {"C1-R05"};
    else if (id == "C1-F-PRESS") bodies = {"C1-R08"};
    else if (id == "C1-F-BINDER") bodies = {"C1-R04"};
    else if (id == "C1-F-NEST") bodies = {"C1-R07"};
    else if (id == "C1-F-PURSUER") bodies = {"C1-O01"};
    else if (id == "C1-F-WARDEN") bodies = {"C1-O02"};
    else if (id == "C1-F-CHASSIS") bodies = {"C1-O03"};
    else if (id == "C1-F-GATEBREAKER") bodies = {"C1-B01"};
    else throw std::invalid_argument("Formation is outside the Cinderwall allowlist.");
    State s; s.seed = seed; s.encounter = encounterKey; s.nextId = nextId;
    for (const auto& body : bodies) s.enemies.push_back(makeEnemy(s, body, binderDefaultSeen, 0));
    // These ten authored combinations contain neither two initial mites nor a
    // combination of initial mite and a death supplier. No generic mix-and-match.
    for (const auto& e : s.enemies) commitRobotIntent(s, e.id, {});
    nextId = s.nextId;
    return std::move(s.enemies);
}

bool commitRobotIntent(State& s, Id id, const RobotCallbacks& cb) {
    auto* e = find(s, id);
    if (!e || !isCinderwallRobot(*e)) return false;
    if (!alive(*e) || e->intentRound == s.round) return true;
    if (e->departureRound > 0 && s.round >= e->departureRound) setIntent(*e, "escape", Move::Escape);
    else if (e->recoveryPending) setIntent(*e, "recover", Move::Recover);
    else if (e->definition == "C1-R01") setIntent(*e, "attack", Move::Attack, 5, 1);
    else if (e->definition == "C1-R02") {
        if (e->patternCursor % 2 == 0) setIntent(*e, "charge", Move::Charge);
        else setIntent(*e, "blast", Move::Attack, 18, 1);
    } else if (e->definition == "C1-R04") {
        const Amount step = e->patternCursor % 3;
        if (step == 0) {
            e->packetVariant = e->binderVariants ? packetDraw(*e) : 0;
            setIntent(*e, "spray", Move::Attack, 4, 2);
        } else if ((step == 1) == (e->packetVariant == 0)) setIntent(*e, "foul", Move::Attack, 6, 1);
        else setIntent(*e, "punch", Move::Attack, 12, 1);
    } else if (e->definition == "C1-R05") {
        if (e->patternCursor % 2 == 0) setIntent(*e, "attack", Move::Attack, 8, 1);
        else setIntent(*e, "pressurize", Move::Support);
    } else if (e->definition == "C1-R07") {
        if (e->patternCursor != 1 && e->summonsRemaining > 0 && !liveMite(s)) setIntent(*e, "deploy", Move::Deploy);
        else setIntent(*e, "attack", Move::Attack, 6, 1);
    } else if (e->definition == "C1-R08") {
        if (e->patternCursor % 3 == 0) setIntent(*e, "brace", Move::Brace);
        else if (e->patternCursor % 3 == 1) setIntent(*e, "double_punch", Move::Attack, 4, 2);
        else setIntent(*e, "slam", Move::Attack, 13, 1);
    } else if (e->definition == "C1-O01") {
        if (e->patternCursor % 2 == 0) setIntent(*e, "strike", Move::Attack, 13, 1);
        else setIntent(*e, "triple_strike", Move::Attack, 3, 3);
    } else if (e->definition == "C1-O02") {
        if (e->patternCursor % 3 == 0) setIntent(*e, "attack_9", Move::Attack, 9, 1);
        else if (e->patternCursor % 3 == 1) setIntent(*e, "gain_drive", Move::Support);
        else setIntent(*e, "attack_12", Move::Attack, 12, 1);
    } else if (e->definition == "C1-O03") {
        if (e->patternCursor % 3 == 0) setIntent(*e, "double_strike", Move::Attack, 5, 2);
        else if (e->patternCursor % 3 == 1) setIntent(*e, "thermal_runaway", Move::Support);
        else setIntent(*e, "triple_strike", Move::Attack, 4, 3);
    } else if (!e->bossTransitioned) setIntent(*e, "phase_one_strike", Move::Attack, 14, 1);
    else if (e->patternCursor % 3 == 0) setIntent(*e, "quad_strike", Move::Attack, 4, 4);
    else if (e->patternCursor % 3 == 1) setIntent(*e, "charge", Move::Charge);
    else setIntent(*e, "blast", Move::Attack, 20, 1);
    e->intentRound = s.round;
    emit(cb, "intent", id, 0, static_cast<Amount>(e->intent.move), e->intent.damage);
    return true;
}

bool executeRobotIntent(State& s, Id id, const RobotCallbacks& cb) {
    auto* e = find(s, id);
    if (!e || !isCinderwallRobot(*e)) return false;
    if (!alive(*e) || e->bornRound >= s.round || e->actionCompletedRound == s.round || s.phase == Phase::Defeat) return true;
    if (!cb.emit || !cb.playerDamage || !cb.playerStatus) throw std::invalid_argument("Cinderwall execution needs Engine damage, status and event callbacks.");
    if (e->intentRound != s.round) throw std::logic_error("Cinderwall intent must be committed before the player phase.");
    e->actionCompletedRound = s.round;
    const auto actionName = e->robotAction;
    const auto intent = e->intent;
    const Id action = emit(cb, "enemy_action", id, 0, static_cast<Amount>(intent.move), intent.hits);
    // The next commitment can replace robotAction before a client animates this
    // result. Preserve the performed action's exact authored identity in events.
    emit(cb, "robot_action:" + actionName, id, 0, intent.damage, intent.hits, action);
    expireBrace(s, id, cb, action);
    e = find(s, id);
    if (intent.move == Move::Escape) {
        e->escaped = true; e->summonsRemaining = 0;
        emit(cb, "enemy_escape", id, 0, 0, 0, action);
        return true;
    }
    if (intent.move == Move::Attack) {
        for (Amount hit = 0; hit < intent.hits && s.phase != Phase::Defeat; ++hit) {
            e = find(s, id); if (!e || !alive(*e)) break;
            const Amount damage = std::max(0, add(intent.damage, e->drive) - e->weaken);
            cb.playerDamage(damage, id, action, false);
        }
    }
    e = find(s, id);
    if (!e || !alive(*e) || s.phase == Phase::Defeat) return true;
    if (actionName == "foul") cb.playerStatus(RobotStatus::RecipeFouling, 1, id, action);
    else if (actionName == "double_punch") cb.playerStatus(RobotStatus::ShieldLeak, 2, id, action);
    else if (actionName == "thermal_runaway") cb.playerStatus(RobotStatus::Burn, 2, id, action);
    else if (actionName == "pressurize" || actionName == "gain_drive") {
        e->drive = add(e->drive, 2); emit(cb, "drive_gained", id, 0, 2, e->drive, action);
    } else if (actionName == "brace") {
        e->armor = add(e->armor, 2); e->temporaryArmor = add(e->temporaryArmor, 2); e->armorExpiresRound = add(s.round, 1);
        emit(cb, "armor_gained", id, 0, 2, e->armorExpiresRound, action);
    } else if (actionName == "deploy") spawnMite(s, id, cb, action);
    e = find(s, id);
    if (!e || !alive(*e)) return true;
    if (actionName == "recover") {
        e->recoveryPending = false; e->patternCursor = 0;
    } else if (e->definition == "C1-R07") e->patternCursor = e->patternCursor == 1 ? 2 : 1;
    else if (e->definition == "C1-R02" || e->definition == "C1-R05" || e->definition == "C1-O01") e->patternCursor = (e->patternCursor + 1) % 2;
    else if (e->definition == "C1-R04" || e->definition == "C1-R08" || e->definition == "C1-O02" || e->definition == "C1-O03") e->patternCursor = (e->patternCursor + 1) % 3;
    else if (e->definition == "C1-B01" && e->bossTransitioned && !e->recoveryPending) e->patternCursor = (e->patternCursor + 1) % 3;
    return true;
}

void finishRobotTurn(State& s, Id id, const RobotCallbacks& cb) {
    auto* e = find(s, id);
    if (!e || !isCinderwallRobot(*e) || !alive(*e) || s.phase == Phase::Defeat
        || e->actionCompletedRound != s.round || e->turnCompletedRound == s.round) return;
    e->turnCompletedRound = s.round;
    if (e->definition == "C1-O01" || (e->definition == "C1-B01" && !e->bossTransitioned)) {
        e->drive = add(e->drive, 1); emit(cb, "drive_gained", id, 0, 1, e->drive);
    }
}

void onRobotHpLoss(State& s, Id id, const RobotCallbacks& cb) {
    auto* e = find(s, id);
    if (!e || !isCinderwallRobot(*e) || e->definition != "C1-B01" || !alive(*e) || e->bossTransitioned || e->hp > 46) return;
    const Amount cleared = e->drive;
    e->bossTransitioned = true; e->recoveryPending = true; e->drive = 0; e->patternCursor = 0;
    setIntent(*e, "recover", Move::Recover); e->intentRound = s.round;
    const Id transition = emit(cb, "recovery_joint", id, 0, 46, cleared);
    emit(cb, "intent", id, 0, static_cast<Amount>(Move::Recover), 0, transition);
}

void onRobotDeath(State& s, Id id, const RobotCallbacks& cb) {
    auto* e = find(s, id);
    if (!e || !isCinderwallRobot(*e) || !e->dead || e->hp != 0 || e->escaped) return;
    if (!e->coreRecorded) {
        e->coreRecorded = true;
        emit(cb, "core_dropped", id, 0, e->coreValue);
    }
    e = find(s, id);
    if (e->definition == "C1-O03" && !e->deathReleased) {
        e->deathReleased = true;
        const Id release = emit(cb, "death_release", id, 0, 1);
        spawnMite(s, id, cb, release);
        e = find(s, id); e->summonsRemaining = 0;
    } else if (e->definition != "C1-O03") e->summonsRemaining = 0;
}

std::string robotIntentText(const Enemy& e, Amount round) {
    if (!isCinderwallRobot(e)) return {};
    std::string text;
    if (e.intent.move == Move::Attack) {
        const auto damage = std::max<std::int64_t>(0, static_cast<std::int64_t>(e.intent.damage) + e.drive - e.weaken);
        text = "Attack " + std::to_string(damage);
        if (e.intent.hits > 1) text += " x " + std::to_string(e.intent.hits);
        if (e.robotAction == "foul") text += " + Recipe Fouling 1 next turn";
        if (e.robotAction == "double_punch") text += " + Shield Leak 2 next turn";
        if (e.robotAction == "phase_one_strike") text += " | Gain 1 Drive afterward";
    } else if (e.intent.move == Move::Charge) text = e.definition == "C1-R02" ? "Charge | Blast 18 next turn" : "Charge | Blast 20 next turn";
    else if (e.intent.move == Move::Escape) text = "Escape";
    else if (e.intent.move == Move::Recover) text = "Recover";
    else if (e.intent.move == Move::Deploy) text = "Deploy Rivet Mite (7 HP) | Supply " + std::to_string(e.summonsRemaining) + "; only if none lives";
    else if (e.intent.move == Move::Brace) text = "Brace | Armor 2 through next player turn";
    else if (e.robotAction == "thermal_runaway") text = "Apply Thermal Runaway 2";
    else text = "Gain 2 Drive";
    if (e.departureRound > round) text += " | Escape in " + std::to_string(e.departureRound - round);
    if (e.bornRound >= round) text += " | First action next round";
    return text;
}

bool announceRobotEscape(State& s, Id id, const RobotCallbacks& cb) {
    auto* e = find(s, id);
    if (!e || !alive(*e) || e->departureRound > 0) return false;
    e->departureRound = add(s.round, 4);
    emit(cb, "escape_announced", id, 0, 4, e->departureRound);
    return true;
}

} // namespace overkill

#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

// No Unreal types, IO, clocks, or rendering RNG are permitted in this library.
namespace overkill {
using Id = std::uint64_t;
using Amount = std::int32_t;
using Materials = std::array<Amount, 5>; // Iron, Copper, Carbon, Glass, Circuit.
inline constexpr const char* RulesVersion = "of-core-0.2";
inline constexpr const char* ContentVersion = "cinderwall-staged-0.1";

enum class Kind : std::uint8_t { Ammo, Shield, Spread, Utility, Magnet, Modifier };
enum class Place : std::uint8_t { Reserve, Installed, Loaded, Fitted };
enum class Phase : std::uint8_t { Collection, Preparation, Victory, Defeat, Escaped };
enum class Timing : std::uint8_t { Use, Install, Assembly, AfterHit, Activate };
enum class Op : std::uint8_t {
    FlatDamage, AttackIntentBonus, PercentDamage, ShieldValue, Heat, HeatCost,
    HpCost, Cool, ShieldGrant, DelayedShield, DelayedHeat, DelayedHaul,
    BurnIfHeat, Burn, Heal, SpreadPercent, BurnWard
};
struct Effect {
    Op op = Op::FlatDamage;
    Timing timing = Timing::Assembly;
    Amount amount = 0;
    Amount threshold = 0;
};
struct Recipe {
    std::string id, name, output;
    Kind kind = Kind::Ammo;
    Materials cost{};
    Amount cooldown = 0; // 0 = None; numeric recipes carry positive counters.
    std::vector<Effect> effects;
    bool automaticOutput = false;
};
struct RecipeCopy {
    Id id = 0;
    std::string recipe;
    Amount cooldown = 0, usedRound = 0, usesThisRound = 0;
};
struct Part {
    Id id = 0;
    std::string recipe, output, resaleReference;
    Kind kind = Kind::Ammo;
    Place place = Place::Reserve;
    Amount createdRound = 1, originalValue = 0, shield = 0;
    Amount firstInstallRound = 0, paidHeat = 0, paidHp = 0;
    Id bindingOrder = 0, installOrder = 0;
    bool everInstalled = false;
    std::vector<Effect> effects;
};
struct Protection { Id id = 0, order = 0; Amount amount = 0; };
enum class Move : std::uint8_t { Attack, Charge, Recover, Escape };
struct Intent { Move move = Move::Attack; Amount damage = 0, hits = 1; };
struct Enemy {
    Id id = 0;
    std::string definition, name;
    Amount hp = 1, maxHp = 1, shield = 0, armor = 0, mesh = 0, tiles = 0;
    Amount burn = 0, corrosion = 0, mark = 0, weaken = 0, drive = 0;
    Amount bornRound = 0, departureRound = 0, patternCursor = 0;
    bool dead = false, escaped = false;
    Intent intent;
    std::vector<Intent> pattern;
};
enum class DeliveryKind : std::uint8_t { ShieldPart, Heat, Haul };
struct Delivery {
    Id id = 0;
    std::string source;
    Amount dueRound = 0, amount = 0;
    DeliveryKind kind = DeliveryKind::ShieldPart;
};
struct ShotBonus { std::string source; Amount flat = 0, percent = 0; };
enum class Domain : std::uint8_t { Route, Formation, Robot, Collection, Reward, Shop, Choice, Count };
struct Rng {
    std::array<std::uint64_t, static_cast<std::size_t>(Domain::Count)> state{};
    static Rng seeded(std::uint64_t seed, std::string encounter);
    std::uint32_t below(Domain domain, std::uint32_t exclusiveMaximum);
};
struct State {
    std::string rulesVersion = RulesVersion, contentVersion = ContentVersion, encounter = "teaching";
    std::uint64_t seed = 1;
    Rng rng;
    Id nextId = 1, nextOrder = 1, nextEvent = 1;
    Phase phase = Phase::Collection;
    Amount round = 1, hp = 80, maxHp = 80, heat = 0, credits = 100;
    Amount shots = 0, kills = 0, burn = 0, corrosion = 0, weaken = 0;
    Amount nextFlat = 0, nextPercent = 0, haulBonus = 0, quickPatchHealing = 0;
    bool hotBarrel = true, precisionSpent = false, burnWardSpent = false, reserveHeartPump = false;
    Materials materials{};
    std::vector<RecipeCopy> memory;
    std::vector<Part> parts;
    std::vector<Protection> protection;
    std::vector<Enemy> enemies;
    std::vector<Delivery> deliveries;
    std::vector<ShotBonus> shotBonuses;
    std::vector<Id> bullet;
    std::vector<Amount> retentionAllowances; // Explicit testable upgrade hooks; no default retention.
};
enum class ActionType : std::uint8_t { Collect, Craft, Install, Remove, Load, Unload, Fire, EndTurn, Activate };
struct SpreadTarget { Id part = 0, enemy = 0; };
struct Action {
    ActionType type = ActionType::EndTurn;
    Id subject = 0, target = 0;
    std::vector<Id> parts;
    std::vector<SpreadTarget> spreadTargets;
    Amount steering = 0;
    // -1 = automatic haul; 0/1/2 = recorded Precision miss/good/perfect.
    Amount precision = -1;
    static Action collect(Amount material, Amount precisionResult = -1);
    static Action craft(Id copy);
    static Action install(Id part);
    static Action remove(Id part);
    static Action load(std::vector<Id> parts);
    static Action fire(Id target, std::vector<SpreadTarget> spreads = {});
    static Action endTurn();
};
struct Event {
    Id id = 0, parent = 0, subject = 0, target = 0;
    std::string type;
    Amount amount = 0, secondary = 0;
};
struct Result { bool ok = false; std::string reason; std::vector<Event> events; };
struct Preview { Result result; State state; };

class Rules {
public:
    explicit Rules(std::vector<Recipe> recipes = starterContent());
    static std::vector<Recipe> starterContent();
    static State teachingEncounter(std::uint64_t seed = 1, bool mara = false);
    const std::vector<Recipe>& content() const { return recipes_; }
    const Recipe* recipe(const std::string& id) const;
    Result apply(State& state, const Action& action) const;
    Preview preview(const State& state, const Action& action) const;
    std::vector<Action> legalActions(const State& state) const;
    static Amount shield(const State& state, bool installedOnly = false);
    static Amount spendShield(State& state, Amount amount, bool installedOnly = false);
    static std::string intentText(const Enemy& enemy, Amount round);
private:
    std::vector<Recipe> recipes_;
    Result execute(State& state, const Action& action) const;
};

// Versioned, checksummed, canonical little-endian snapshots. Parsing never mutates its output on failure.
std::string serialize(const State& state);
bool deserialize(const std::string& bytes, State& state, std::string& error);
std::string stateHash(const State& state);
std::string eventJson(const Event& event);
// Executes the authored route via public actions; used by tests, runner and graphical parity checks.
Result playTeachingFixture(State& state, const Rules& rules);
} // namespace overkill

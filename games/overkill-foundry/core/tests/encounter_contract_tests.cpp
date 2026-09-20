#include "overkill/robots.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// Boundary fixtures intentionally prepare HP, statuses and ordinary ammunition.
// They exercise real Rules actions; they are not earned campaign/balance runs.
using namespace overkill;
namespace {
int assertions = 0;
void check(bool ok, const std::string& why) { ++assertions; if (!ok) throw std::runtime_error(why); }
bool living(const Enemy& e) { return !e.dead && !e.escaped && e.hp > 0; }
Enemy& body(State& s, Id id) {
    for (auto& e : s.enemies) if (e.id == id) return e;
    throw std::runtime_error("Missing prepared enemy");
}
int count(const Result& result, const std::string& type, Id subject = 0) {
    return static_cast<int>(std::count_if(result.events.begin(), result.events.end(), [&](const Event& e) {
        return e.type == type && (!subject || e.subject == subject);
    }));
}
std::vector<Amount> hits(const Result& result) {
    std::vector<Amount> values;
    for (const auto& e : result.events) if (e.type == "player_damage") values.push_back(e.amount);
    return values;
}
State arena(const std::string& formation, std::uint64_t seed = 44, bool binderSeen = false) {
    State s;
    s.seed = seed; s.encounter = "prepared-contract/" + formation;
    s.rng = Rng::seeded(seed, s.encounter); s.phase = Phase::Preparation;
    s.hp = s.maxHp = 1000; s.hotBarrel = false;
    s.enemies = makeCinderwallFormation(formation, seed, s.encounter, s.nextId, binderSeen);
    return s;
}
Result apply(const Rules& rules, State& state, const Action& action) {
    State restored; std::string error;
    check(deserialize(serialize(state), restored, error), "State round trip before action: " + error);
    const auto result = rules.apply(state, action);
    check(result.ok, "Public action: " + result.reason);
    const auto replay = rules.apply(restored, action);
    check(replay.ok && serialize(restored) == serialize(state), "Saved action replay differs");
    check(result.events.size() == replay.events.size(), "Saved event count differs");
    for (std::size_t i = 0; i < result.events.size(); ++i)
        check(eventJson(result.events[i]) == eventJson(replay.events[i]), "Saved ordered event differs");
    return result;
}
Result end(const Rules& rules, State& state) {
    const auto result = apply(rules, state, Action::endTurn());
    if (state.phase == Phase::Collection) apply(rules, state, Action::collect(0));
    return result;
}
Result shoot(const Rules& rules, State& state, Id target, Amount damage) {
    const Id first = state.nextId;
    check(rules.grantPlainPart(state, Kind::Ammo, damage, "Prepared encounter boundary").ok, "Plain ammo grant");
    Id part = 0;
    for (const auto& p : state.parts) if (p.id >= first && p.kind == Kind::Ammo) part = p.id;
    check(part != 0, "Physical ammo identity");
    apply(rules, state, Action::load({part}));
    return apply(rules, state, Action::fire(target));
}
void entry(const State& state, const std::string& definition, Amount hp, Amount armor, Amount core) {
    check(state.enemies.size() == 1, "Authored single-body formation");
    const auto& e = state.enemies.front();
    check(e.definition == definition && e.hp == hp && e.maxHp == hp && e.armor == armor,
        "Authored body identity, full HP and Armor");
    check(e.tiles == 0 && e.mesh == 0 && e.shield == 0 && e.coreValue == core && e.departureRound == 0,
        "No invented defence/escape; exact assigned core value");
    check(e.intentRound == 1 && e.bornRound == 0, "Initial intent and birth clock");
}
void coreDrop(const Result& result, Id id, Amount value) {
    check(count(result, "core_dropped", id) == 1, "Exactly one body core allocation");
    for (const auto& e : result.events) if (e.type == "core_dropped" && e.subject == id)
        check(e.amount == value, "Authored per-body core value");
}
void denied(const Rules& rules, State& state, const Action& action) {
    const auto before = serialize(state); const auto result = rules.apply(state, action);
    check(!result.ok && result.events.empty() && serialize(state) == before, "Rejected command mutated state or emitted events");
}
void run(const char* name, const std::function<void()>& fn) { fn(); std::cout << "PASS " << name << '\n'; }
}

int main() {
    try {
        const Rules rules;
        run("C1-R04 Cable Binder exact body, default and veteran packets, Fouling clock, core identity", [&] {
            auto s = arena("C1-F-BINDER"); entry(s, "C1-R04", 22, 0, 15);
            const Id id = s.enemies[0].id;
            check(s.enemies[0].robotAction == "spray", "Default Spray opener");
            check(hits(end(rules, s)) == std::vector<Amount>({4, 4}), "Spray separate hits");
            check(hits(end(rules, s)) == std::vector<Amount>({6}), "Foul damage");
            check(s.recipeFouling == 1 && s.foulingRound == s.round, "Next-player-turn Fouling");
            check(hits(end(rules, s)) == std::vector<Amount>({12}) && s.recipeFouling == 0, "Punch and unused Fouling expiry");
            check(s.enemies[0].robotAction == "spray", "Default packet renews");
            coreDrop(shoot(rules, s, id, 22), id, 15);
            denied(rules, s, Action::fire(id));
            bool first = false, second = false;
            for (std::uint64_t seed = 1; seed <= 24; ++seed) {
                auto veteran = arena("C1-F-BINDER", seed, true);
                for (int packet = 0; packet < 2; ++packet) {
                    std::vector<std::string> actions; Amount total = 0;
                    for (int step = 0; step < 3; ++step) {
                        actions.push_back(veteran.enemies[0].robotAction);
                        for (Amount damage : hits(end(rules, veteran))) total += damage;
                    }
                    check(actions[0] == "spray" && total == 26, "Veteran opener and packet damage budget");
                    check(std::count(actions.begin(), actions.end(), "foul") == 1 &&
                        std::count(actions.begin(), actions.end(), "punch") == 1, "One Foul and Punch per packet");
                    first = first || actions[1] == "foul"; second = second || actions[1] == "punch";
                }
            }
            check(first && second, "Both saved veteran packet variants exercised");
        });
        run("C1-R05 Pressure Cask exact body, alternating pressure, armour and one core", [&] {
            auto s = arena("C1-F-CASK"); entry(s, "C1-R05", 24, 1, 15); const Id id = s.enemies[0].id;
            check(hits(end(rules, s)) == std::vector<Amount>({8}), "Cask first strike");
            check(hits(end(rules, s)).empty() && s.enemies[0].drive == 2, "Pressure is nonattack Drive2");
            check(Rules::intentText(s.enemies[0], s.round).find("Attack 10") != std::string::npos, "Next visible pressure damage");
            check(hits(end(rules, s)) == std::vector<Amount>({10}), "Pressure strengthens next attack");
            check(hits(end(rules, s)).empty() && s.enemies[0].drive == 4, "Second pressure gain");
            check(hits(end(rules, s)) == std::vector<Amount>({12}), "Pressure persists across cycles");
            shoot(rules, s, id, 1); check(s.enemies[0].hp == 24, "Armor blocks low direct hit");
            coreDrop(shoot(rules, s, id, 25), id, 15); denied(rules, s, Action::endTurn());
        });
        run("C1-R07 Coil Nest finite four helpers, birth timing, locked support, distinct cores", [&] {
            auto s = arena("C1-F-NEST"); entry(s, "C1-R07", 28, 0, 18); const Id nest = s.enemies[0].id;
            check(s.enemies[0].summonsRemaining == 4 && s.enemies[0].robotAction == "deploy", "Finite opening supply");
            int deployed = 0; int helperCores = 0;
            for (int turn = 0; turn < 12; ++turn) {
                const auto result = end(rules, s);
                const int made = count(result, "robot_deployed"); deployed += made;
                check(made <= 1, "At most one helper per action");
                int aliveMites = 0;
                for (const auto& e : s.enemies) if (living(e) && e.definition == "C1-R01") {
                    ++aliveMites;
                    if (made) check(count(result, "player_damage", e.id) == 0, "Newborn helper cannot attack in birth round");
                }
                check(aliveMites <= 1, "One live helper limit");
                const auto found = std::find_if(s.enemies.begin(), s.enemies.end(), [](const Enemy& e) {
                    return living(e) && e.definition == "C1-R01";
                });
                if (found != s.enemies.end()) {
                    const Id mite = found->id; const auto killed = shoot(rules, s, mite, 7);
                    coreDrop(killed, mite, 5); ++helperCores;
                }
            }
            check(deployed == 4 && helperCores == 4 && body(s, nest).summonsRemaining == 0, "Supply cannot replenish or farm indefinitely");
            check(body(s, nest).robotAction == "attack", "Exhausted deployment slots attack");
            coreDrop(shoot(rules, s, nest, 28), nest, 18);
            check(s.kills == 5 && s.phase == Phase::Victory, "Nest and four distinct killed helpers end encounter");
            auto locked = arena("C1-F-NEST"); end(rules, locked); end(rules, locked);
            check(locked.enemies[0].robotAction == "attack" && locked.enemies[0].summonsRemaining == 3, "Living helper commits alternate attack");
            shoot(rules, locked, locked.enemies.back().id, 7);
            const auto result = end(rules, locked);
            check(count(result, "robot_deployed") == 0 && hits(result) == std::vector<Amount>({6}), "Killing helper cannot reroll a committed attack");
        });
        run("C1-R08 Knuckle Press exact body, brace expiry, two hits, one next-turn Leak", [&] {
            auto s = arena("C1-F-PRESS"); entry(s, "C1-R08", 28, 0, 18); const Id id = s.enemies[0].id;
            check(hits(end(rules, s)).empty() && s.enemies[0].armor == 2, "Brace grants next-player-turn Armor2");
            shoot(rules, s, id, 2); check(s.enemies[0].hp == 28, "Brace actually protects incoming hit");
            const auto punch = end(rules, s);
            check(hits(punch) == std::vector<Amount>({4, 4}) && s.enemies[0].armor == 0 && count(punch, "armor_expired", id) == 1,
                "Armor expires before ordered two-hit attack");
            check(s.shieldLeak == 2 && s.shieldLeakRound == s.round, "Leak applies to following player turn");
            check(rules.grantPlainPart(s, Kind::Shield, 10, "Prepared Leak boundary", true).ok, "Install ordinary Shield10");
            const auto heavy = end(rules, s);
            check(hits(heavy) == std::vector<Amount>({5}) && s.shieldLeak == 0, "Leak drains two once before Attack13");
            check(s.enemies[0].robotAction == "brace", "Three-action pattern repeats");
            coreDrop(shoot(rules, s, id, 28), id, 18);
        });
        run("C1-O01 and FX03 Pursuer hit scaling, once per completed turn, no post-death motor", [&] {
            auto s = arena("C1-F-PURSUER"); entry(s, "C1-O01", 52, 0, 40); const Id id = s.enemies[0].id;
            const std::vector<std::vector<Amount>> expected{{13}, {4,4,4}, {15}, {6,6,6}};
            for (std::size_t i = 0; i < expected.size(); ++i) {
                const auto result = end(rules, s);
                check(hits(result) == expected[i] && s.enemies[0].drive == static_cast<Amount>(i + 1), "Per-hit scaling and one completed-turn gain");
                check(count(result, "drive_gained", id) == 1, "One motor event per completed turn");
            }
            coreDrop(shoot(rules, s, id, 52), id, 40);
            for (bool beforeAction : {false, true}) {
                auto lethal = arena("C1-F-PURSUER");
                if (beforeAction) lethal.enemies[0].corrosion = 52; else lethal.enemies[0].burn = 52;
                const auto result = end(rules, lethal);
                check(count(result, "drive_gained") == 0 && lethal.enemies[0].drive == 0, "Killed robot gets no completion motor");
                check(hits(result).size() == (beforeAction ? 0u : 1u), "Pre/post-action status boundary");
            }
            auto support = arena("C1-F-PURSUER");
            support.enemies[0].robotAction = "recover"; support.enemies[0].intent = {Move::Recover,0,0};
            check(hits(end(rules, support)).empty() && support.enemies[0].drive == 1, "Prepared nonattack still completes motor turn");
            auto fatal = arena("C1-F-PURSUER"); fatal.hp = 1;
            const auto death = end(rules, fatal);
            check(fatal.phase == Phase::Defeat && count(death, "drive_gained") == 0, "Player death interrupts completion gain");
        });
        run("C1-O03 Split Chassis three-action cycle, Burn timing, one death release and two cores", [&] {
            auto s = arena("C1-F-CHASSIS"); entry(s, "C1-O03", 48, 0, 35);
            check(hits(end(rules, s)) == std::vector<Amount>({5,5}), "Chassis double strike");
            check(hits(end(rules, s)).empty() && s.burn == 2, "Thermal action applies Burn without immediate player tick");
            check(hits(end(rules, s)) == std::vector<Amount>({2,4,4,4}) && s.burn == 1, "Player Burn precedes triple strike");
            check(hits(end(rules, s)) == std::vector<Amount>({1,5,5}) && s.burn == 0, "Pattern renewal and final Burn tick");
            auto released = arena("C1-F-CHASSIS"); const Id carrier = released.enemies[0].id;
            const auto kill = shoot(rules, released, carrier, 48); coreDrop(kill, carrier, 35);
            check(count(kill, "death_release", carrier) == 1 && count(kill, "robot_deployed") == 1 && released.enemies.size() == 2,
                "One compulsory visible survivor from death");
            const Id mite = released.enemies.back().id;
            check(released.phase == Phase::Preparation && released.enemies.back().hp == 7, "Helper prevents premature victory");
            check(hits(end(rules, released)).empty(), "Death helper waits past its birth round");
            check(hits(end(rules, released)) == std::vector<Amount>({5}), "Death helper acts the next enemy round");
            coreDrop(shoot(rules, released, mite, 7), mite, 5);
            check(released.phase == Phase::Victory && released.kills == 2 && released.enemies.size() == 2, "Both identities die exactly once");
            denied(rules, released, Action::fire(carrier));
        });
        run("Thermal Runaway mirrors: post-action enemy tick, pre-enemy player tick, Shield and lethal stops", [&] {
            auto enemyBurn = arena("C1-F-RAM");
            enemyBurn.enemies[0].shield = 2; enemyBurn.enemies[0].tiles = 2; enemyBurn.enemies[0].burn = 4;
            end(rules, enemyBurn);
            check(enemyBurn.enemies[0].hp == 22 && enemyBurn.enemies[0].shield == 0 && enemyBurn.enemies[0].burn == 3 &&
                enemyBurn.enemies[0].tiles == 2, "Burn ignores Armor/Tiles, spends Shield then HP and decays once");
            const auto blast = end(rules, enemyBurn);
            check(hits(blast) == std::vector<Amount>({18}) && enemyBurn.enemies[0].hp == 19 && enemyBurn.enemies[0].burn == 2,
                "Enemy completes action before later Burn");
            auto playerBurn = arena("C1-F-RAM"); playerBurn.burn = 4;
            check(rules.grantPlainPart(playerBurn, Kind::Shield, 3, "Prepared Burn protection", true).ok, "Burn Shield fixture");
            const auto tick = end(rules, playerBurn);
            check(hits(tick) == std::vector<Amount>({1}) && playerBurn.hp == 999 && playerBurn.burn == 3, "Installed Shield absorbs player Burn before enemy charge");
            auto fatal = arena("C1-F-PURSUER"); fatal.hp = 1; fatal.burn = 1;
            const auto stopped = end(rules, fatal);
            const bool anyRobotAction = std::any_of(stopped.events.begin(), stopped.events.end(), [](const Event& e) {
                return e.type.rfind("robot_action:", 0) == 0;
            });
            check(fatal.phase == Phase::Defeat && !anyRobotAction && fatal.enemies[0].drive == 0,
                "Lethal player Burn stops enemy actions");
        });
        run("Acid Etch mirrors: pre-action bypass, player post-reset pre-delivery tick and lethal interruption", [&] {
            auto enemyAcid = arena("C1-F-BINDER"); enemyAcid.enemies[0].hp = 2;
            enemyAcid.enemies[0].corrosion = 2; enemyAcid.enemies[0].armor = 40;
            enemyAcid.enemies[0].shield = 40; enemyAcid.enemies[0].tiles = 3;
            const auto died = end(rules, enemyAcid);
            check(enemyAcid.phase == Phase::Victory && hits(died).empty(), "Corrosion bypasses all protection and kills before robot action");
            check(enemyAcid.enemies[0].shield == 40 && enemyAcid.enemies[0].tiles == 3, "Corrosion consumes neither Shield nor Tiles");
            for (Amount hp : {3,4}) {
                auto playerAcid = arena("C1-F-RAM"); playerAcid.hp = hp; playerAcid.corrosion = 3;
                playerAcid.protection.push_back({playerAcid.nextId++,playerAcid.nextOrder++,100});
                playerAcid.retentionAllowances = {50};
                Delivery delivery; delivery.id = playerAcid.nextId++; delivery.source = "prepared-status-timing";
                delivery.dueRound = 2; delivery.kind = DeliveryKind::ShieldPart; delivery.amount = 10; playerAcid.deliveries.push_back(delivery);
                const auto result = end(rules, playerAcid);
                check(playerAcid.hp == hp - 3 && playerAcid.corrosion == 2 && playerAcid.round == 2, "Player Corrosion bypass after old Shield reset");
                if (hp == 3) check(playerAcid.phase == Phase::Defeat && count(result, "delivery") == 0 && Rules::shield(playerAcid) == 50,
                    "Lethal hostile Corrosion stops fresh grants");
                else check(playerAcid.phase == Phase::Preparation && count(result, "delivery") == 1 && Rules::shield(playerAcid) == 60,
                    "Surviving Corrosion precedes fresh Shield delivery");
            }
        });
        run("Drive Fault mirrors: repeated main shots, target bonus order, multi-hit reduction and no-attack decay", [&] {
            auto player = arena("C1-F-BINDER"); player.weaken = 5; player.enemies[0].mark = 3; const Id id = player.enemies[0].id;
            shoot(rules, player, id, 4);
            check(player.enemies[0].hp == 19 && player.weaken == 5, "Player Weaken clamps main damage before target Mark");
            shoot(rules, player, id, 10);
            check(player.enemies[0].hp == 14 && player.weaken == 5 && player.round == 1, "Repeated shot reuses unconsumed Weaken");
            end(rules, player); check(player.weaken == 4, "Player action-phase decay once");
            auto enemy = arena("C1-F-BINDER"); enemy.enemies[0].weaken = 3;
            check(hits(end(rules, enemy)) == std::vector<Amount>({1,1}) && enemy.enemies[0].weaken == 2, "Enemy Weaken affects every hit without per-hit consumption");
            check(hits(end(rules, enemy)) == std::vector<Amount>({4}) && enemy.enemies[0].weaken == 1, "Next enemy turn retains reduced amount");
            auto charge = arena("C1-F-RAM"); charge.enemies[0].weaken = 1;
            check(hits(end(rules, charge)).empty() && charge.enemies[0].weaken == 0, "Nonattack enemy turn still decays to zero");
            charge.weaken = 1; end(rules, charge); check(charge.weaken == 0, "No player shot still decays to zero");
        });
        std::cout << "ENCOUNTER_CONTRACTS_OK groups=9 assertions=" << assertions << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL after " << assertions << " assertions: " << e.what() << '\n'; return 1;
    }
}

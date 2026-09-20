#include "overkill/robots.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace overkill;
namespace {
int assertions = 0;
void check(bool condition, const std::string& label) {
    ++assertions;
    if (!condition) throw std::runtime_error(label);
}
bool alive(const Enemy& e) { return e.hp > 0 && !e.dead && !e.escaped; }
Enemy& enemy(State& s, Id id) {
    for (auto& e : s.enemies) if (e.id == id) return e;
    throw std::runtime_error("Test enemy missing.");
}
struct Harness {
    State state;
    std::vector<Event> events;
    std::vector<Amount> hits;
    RobotCallbacks cb;
    explicit Harness(const std::string& formation, std::uint64_t seed = 1, bool binderSeen = false) {
        state.seed = seed; state.encounter = "Mara/cinderwall/node4/" + formation;
        state.hp = 10000; state.maxHp = 10000; state.phase = Phase::Preparation;
        state.enemies = makeCinderwallFormation(formation, seed, state.encounter, state.nextId, binderSeen);
        cb.emit = [&](const std::string& type, Id source, Id target, Amount amount, Amount secondary, Id parent) {
            const Id id = state.nextEvent++;
            events.push_back({id, parent, source, target, type, amount, secondary}); return id;
        };
        cb.playerDamage = [&](Amount amount, Id, Id, bool) {
            hits.push_back(amount); state.hp = std::max(0, state.hp - amount);
            if (state.hp == 0) state.phase = Phase::Defeat;
        };
        cb.playerStatus = [&](RobotStatus type, Amount amount, Id, Id) {
            if (type == RobotStatus::Burn) state.burn += amount;
            else if (type == RobotStatus::RecipeFouling) { state.recipeFouling = std::min(2, state.recipeFouling + amount); state.foulingRound = state.round + 1; }
            else if (type == RobotStatus::ShieldLeak) { state.shieldLeak += amount; state.shieldLeakRound = state.round + 1; }
        };
    }
    Id first() const { return state.enemies.front().id; }
    void damage(Id id, Amount amount) {
        auto& e = enemy(state, id); e.hp = std::max(0, e.hp - amount);
        onRobotHpLoss(state, id, cb);
        if (e.hp == 0 && !e.dead) {
            e.dead = true; ++state.kills; onRobotDeath(state, id, cb);
        }
    }
    void next() {
        ++state.round;
        std::vector<Id> ids; for (const auto& e : state.enemies) ids.push_back(e.id);
        for (Id id : ids) commitRobotIntent(state, id, cb);
    }
    void action(Id id) { executeRobotIntent(state, id, cb); finishRobotTurn(state, id, cb); }
    void round() {
        std::vector<Id> actors;
        for (const auto& e : state.enemies) if (alive(e) && e.bornRound < state.round) actors.push_back(e.id);
        for (Id id : actors) action(id);
        next();
    }
    int count(const std::string& type) const {
        return static_cast<int>(std::count_if(events.begin(), events.end(), [&](const Event& event) { return event.type == type; }));
    }
};

void registryAndFactories() {
    check(cinderwallRobotDefinitions().size() == 10, "Ten selected robot definitions.");
    const auto formations = cinderwallFormationIds();
    check(formations.size() == 10, "Ten selected authored formations.");
    for (const auto& formation : formations) {
        Id next = 70;
        const auto bodies = makeCinderwallFormation(formation, 9, "Mara/cinderwall/node8/offer2", next);
        check(!bodies.empty() && next == 70 + bodies.size(), "Factory allocates unique per-body IDs.");
        int mites = 0;
        for (const auto& body : bodies) {
            check(body.intentRound == 1 && body.bornRound == 0, "Entry intent before first collection.");
            check(body.hp == body.maxHp && body.coreValue > 0, "HP and finite core allocation.");
            check(body.departureRound == 0, "No invented escape schedule.");
            if (body.definition == "C1-R01") ++mites;
        }
        check(mites <= 1 && (mites == 0 || bodies.size() > 1), "Mite initial companion and live limit.");
    }
    Id next = 20; bool threw = false;
    try { (void)makeCinderwallFormation("two-mites", 1, "invalid", next); } catch (const std::invalid_argument&) { threw = true; }
    check(threw && next == 20, "Invalid formation atomicity.");
    Harness pair("C1-F-MITE-RAM");
    check(pair.state.enemies[0].hp == 7 && pair.state.enemies[1].hp == 24 && pair.state.enemies[1].armor == 3, "Exact revised Mite/Ram fixture.");
    check(pair.state.enemies[1].intent.move == Move::Charge, "Ram cannot open on Blast.");
    pair.round();
    check(pair.hits == std::vector<Amount>{5}, "Mite attacks while Ram charges.");
    check(pair.count("robot_action:attack") == 1 && pair.count("robot_action:charge") == 1, "Immutable events preserve performed action identity.");
    check(pair.state.enemies[1].intent.damage == 18, "Ram promised Blast after Charge.");
    pair.round();
    check(pair.hits == std::vector<Amount>({5, 5, 18}), "Separate ordered Ram round hits.");
    check(pair.state.enemies[1].intent.move == Move::Charge, "No consecutive Ram Blast.");
}

void binderPackets() {
    bool sawDefault = false, sawAlternate = false;
    for (std::uint64_t seed = 1; seed <= 64; ++seed) {
        Harness first("C1-F-BINDER", seed, false), veteran("C1-F-BINDER", seed, true), replay("C1-F-BINDER", seed, true);
        const auto untouched = first.state.enemies[0].patternRandom;
        for (int packet = 0; packet < 3; ++packet) {
            std::vector<std::string> actions;
            Amount printedDamage = 0;
            for (int step = 0; step < 3; ++step) {
                auto& original = first.state.enemies[0];
                check(original.robotAction == (step == 0 ? "spray" : step == 1 ? "foul" : "punch"), "First-exposure default packet.");
                auto& varied = veteran.state.enemies[0];
                const auto savedRandom = varied.patternRandom;
                const auto savedAction = varied.robotAction;
                commitRobotIntent(veteran.state, varied.id, veteran.cb);
                check(varied.patternRandom == savedRandom && varied.robotAction == savedAction, "Repeated preview cannot reroll committed packet.");
                check(varied.robotAction == replay.state.enemies[0].robotAction && varied.patternRandom == replay.state.enemies[0].patternRandom, "Seeded packet replay.");
                actions.push_back(varied.robotAction);
                printedDamage += varied.intent.damage * varied.intent.hits;
                // Other streams cannot shift robot packet selection.
                veteran.state.rng.state[static_cast<std::size_t>(Domain::Reward)] += seed;
                veteran.round(); replay.round(); first.round();
            }
            check(actions[0] == "spray" && printedDamage == 26, "Fixed Spray opener and 26 printed damage per packet.");
            check(std::count(actions.begin(), actions.end(), "foul") == 1 && std::count(actions.begin(), actions.end(), "punch") == 1, "Exactly one Foul and Punch per packet.");
            sawDefault = sawDefault || actions[1] == "foul"; sawAlternate = sawAlternate || actions[1] == "punch";
        }
        check(first.state.enemies[0].patternRandom == untouched, "First exposure consumes no variant draws.");
        check(first.state.recipeFouling <= 2, "Fouling cap exposed to callback.");
    }
    check(sawDefault && sawAlternate, "Both eligible authored variants selected across seeds.");
}

void pressureAndOfficers() {
    Harness cask("C1-F-CASK");
    check(cask.state.enemies[0].armor == 1, "Cask Armor 1.");
    cask.round(); cask.round();
    check(cask.hits == std::vector<Amount>{8} && cask.state.enemies[0].drive == 2, "Pressurize gains Drive without attacking.");
    check(robotIntentText(cask.state.enemies[0], cask.state.round).find("Attack 10") != std::string::npos, "Pressure shown in forecast.");
    cask.round(); check(cask.hits.back() == 10, "Stored Drive strengthens next hit.");
    Harness pursuer("C1-F-PURSUER");
    pursuer.round();
    check(pursuer.state.enemies[0].drive == 1, "Runaway Motor after first completed turn.");
    pursuer.state.enemies[0].weaken = 2;
    pursuer.action(pursuer.first());
    check(pursuer.hits == std::vector<Amount>({13, 2, 2, 2}), "Drive and Fault apply to every separate hit.");
    check(pursuer.state.enemies[0].weaken == 2, "Attack does not consume Drive Fault.");
    finishRobotTurn(pursuer.state, pursuer.first(), pursuer.cb);
    check(pursuer.state.enemies[0].drive == 2, "Repeated completion cannot double Runaway Motor.");
    Harness warden("C1-F-WARDEN");
    check(warden.state.enemies[0].tiles == 3 && warden.state.enemies[0].mesh == 0, "Warden finite Tiles, no invented Mesh.");
    warden.round(); warden.round(); warden.round();
    check(warden.hits == std::vector<Amount>({9, 14}), "Warden Attack9, Drive2, Attack12 plus Drive.");
    check(warden.state.enemies[0].tiles == 3, "Robot actions do not recharge or consume Tiles.");
    Harness chassis("C1-F-CHASSIS");
    chassis.round(); chassis.round();
    check(chassis.hits == std::vector<Amount>({5, 5}) && chassis.state.burn == 2, "Chassis Thermal Runaway is a separate non-attack action.");
    chassis.round(); check(chassis.hits == std::vector<Amount>({5, 5, 4, 4, 4}), "Chassis triple strike.");
    Harness fatal("C1-F-PURSUER"); fatal.round(); fatal.state.hp = 1;
    const auto beforeFatal = fatal.hits.size(); fatal.action(fatal.first());
    check(fatal.state.phase == Phase::Defeat && fatal.hits.size() == beforeFatal + 1, "Final player death stops remaining multi-hit attacks.");
    check(fatal.state.enemies[0].drive == 1, "Final player death stops ordinary completion gains.");
}

void braceAndLockedSupport() {
    Harness press("C1-F-PRESS");
    press.round();
    check(press.hits.empty() && press.state.enemies[0].armor == 2 && press.state.enemies[0].armorExpiresRound == 2, "Brace protects following player turn.");
    check(press.state.enemies[0].robotAction == "double_punch", "Brace commits next double punch.");
    press.round();
    check(press.state.enemies[0].armor == 0 && press.count("armor_expired") == 1, "Temporary Armor expires before double punch.");
    check(press.hits == std::vector<Amount>({4, 4}) && press.state.shieldLeak == 2 && press.state.shieldLeakRound == 3, "Two hits and next-turn Shield Leak.");
    press.round(); check(press.hits.back() == 13, "Press heavy follow-up.");

    Harness blocked("C1-F-NEST");
    const Id nest = blocked.first();
    auto mitePair = makeCinderwallFormation("C1-F-MITE-RAM", 8, "injected-condition", blocked.state.nextId);
    blocked.state.enemies.push_back(mitePair.front());
    blocked.action(nest);
    check(blocked.hits.empty() && blocked.count("deploy_blocked") == 1 && enemy(blocked.state, nest).summonsRemaining == 4, "Committed Deploy cannot become Attack or consume unavailable slot.");
    blocked.next(); blocked.action(nest); blocked.next();
    check(enemy(blocked.state, nest).robotAction == "attack", "Occupied Mite slot commits Attack.");
    blocked.damage(mitePair.front().id, 7);
    const auto hits = blocked.hits.size(); blocked.action(nest);
    check(blocked.hits.size() == hits + 1 && blocked.count("robot_deployed") == 0, "Killing helper cannot change committed Attack into Deploy.");
}

void finiteSummonsAndDeathIdentity() {
    Harness nest("C1-F-NEST"); const Id parent = nest.first();
    for (int supply = 4; supply > 0; --supply) {
        if (enemy(nest.state, parent).robotAction != "deploy") nest.round();
        check(enemy(nest.state, parent).robotAction == "deploy", "Finite deployment selected when space exists.");
        const auto hitsBefore = nest.hits.size(); nest.round();
        check(nest.hits.size() == hitsBefore, "New helper does not act in deployment round.");
        Id child = 0;
        for (const auto& e : nest.state.enemies) if (e.definition == "C1-R01" && alive(e)) child = e.id;
        check(child != 0 && enemy(nest.state, parent).summonsRemaining == supply - 1, "Each spawn consumes exactly one supply.");
        check(enemy(nest.state, child).bornRound == nest.state.round - 1 && enemy(nest.state, child).intent.damage == 5, "Born-round and visible intent persisted.");
        nest.damage(child, 7);
    }
    for (int i = 0; i < 12; ++i) nest.round();
    check(nest.count("robot_deployed") == 4 && enemy(nest.state, parent).summonsRemaining == 0, "Four lifetime mites, never infinite supply.");
    check(nest.count("core_dropped") == 4, "Only actually destroyed helpers receive unique cores.");
    Harness lone("C1-F-NEST"); const Id maker = lone.first(); lone.round();
    const Id child = lone.state.enemies.back().id; lone.damage(maker, 28);
    check(alive(enemy(lone.state, child)) && enemy(lone.state, maker).summonsRemaining == 0, "Summoner death cancels future supply, preserves live helper.");
    lone.action(child); check(lone.hits.back() == 5, "Lone survivor remains active without overtime.");

    Harness chassis("C1-F-CHASSIS"); const Id carrier = chassis.first();
    chassis.damage(carrier, 48);
    check(chassis.state.enemies.size() == 2 && chassis.state.enemies.back().definition == "C1-R01", "Announced death release.");
    const Id parasite = chassis.state.enemies.back().id;
    check(parasite != carrier && enemy(chassis.state, parasite).coreValue == 5 && enemy(chassis.state, carrier).coreValue == 35, "Carrier and helper separate core identities total40.");
    chassis.action(parasite); check(chassis.hits.empty(), "Preparation death helper cannot act in its birth round.");
    onRobotDeath(chassis.state, carrier, chassis.cb);
    check(chassis.state.enemies.size() == 2 && chassis.count("core_dropped") == 1, "Repeated death consequence is idempotent.");
    chassis.next(); chassis.action(parasite); check(chassis.hits == std::vector<Amount>{5}, "Death helper first acts next round.");
    chassis.damage(parasite, 7); check(chassis.count("core_dropped") == 2, "Death helper core earned only on its own death.");
}

void recoveryJointAndEscapes() {
    Harness boss("C1-F-GATEBREAKER"); const Id id = boss.first();
    boss.round(); check(boss.hits == std::vector<Amount>{14} && enemy(boss.state, id).drive == 1, "Phase-one Attack14 and Drive gain.");
    boss.damage(id, 46);
    check(enemy(boss.state, id).hp == 46 && enemy(boss.state, id).intent.move == Move::Recover && enemy(boss.state, id).drive == 0, "Any surviving loss to46 clears Drive and replaces pending attack.");
    boss.round(); check(boss.hits.size() == 1 && enemy(boss.state, id).robotAction == "quad_strike", "One Recover then phase-two quad strike.");
    boss.round(); boss.round(); boss.round();
    check(boss.hits == std::vector<Amount>({14, 4, 4, 4, 4, 20}) && enemy(boss.state, id).drive == 0, "Phase-two Quad/Charge/Blast and no phase-one scaling.");
    boss.damage(id, 1); check(boss.count("recovery_joint") == 1 && !enemy(boss.state, id).recoveryPending, "One transition only.");
    Harness after("C1-F-GATEBREAKER"); const Id afterId = after.first();
    executeRobotIntent(after.state, afterId, after.cb);
    after.damage(afterId, 46); finishRobotTurn(after.state, afterId, after.cb); after.next();
    check(enemy(after.state, afterId).intent.move == Move::Recover, "Threshold after performed action reserves following Recover.");
    after.round(); check(enemy(after.state, afterId).robotAction == "quad_strike", "Deferred Recover consumed once.");
    Harness lethal("C1-F-GATEBREAKER"); lethal.damage(lethal.first(), 1000);
    check(lethal.count("recovery_joint") == 0 && lethal.state.enemies[0].dead, "Lethal hit has no artificial threshold survival.");

    Harness escape("C1-F-PURSUER"); escape.next(); const Id runner = escape.first();
    check(announceRobotEscape(escape.state, runner, escape.cb), "Explicit warning can be scheduled.");
    check(!announceRobotEscape(escape.state, runner, escape.cb), "Repeated announcement cannot defer departure.");
    for (int countdown = 4; countdown > 0; --countdown) {
        check(robotIntentText(enemy(escape.state, runner), escape.state.round).find("Escape in " + std::to_string(countdown)) != std::string::npos, "Four full warning turns shown.");
        check(enemy(escape.state, runner).intent.move == Move::Attack, "Normal warning-turn attack remains.");
        escape.round();
    }
    check(escape.state.round == 6 && enemy(escape.state, runner).intent.move == Move::Escape, "Round2 warning departs in round6.");
    const auto hits = escape.hits.size(); escape.action(runner);
    check(enemy(escape.state, runner).escaped && escape.hits.size() == hits && escape.count("core_dropped") == 0, "Escape replaces attack and grants no core.");
}

#ifdef OVERKILL_ROBOT_INTEGRATION
State integrated(const std::string& formation) {
    State s; s.seed = 44; s.encounter = "Mara/cinderwall/integration/" + formation;
    s.rng = Rng::seeded(s.seed, s.encounter); s.phase = Phase::Preparation; s.hotBarrel = false;
    s.enemies = makeCinderwallFormation(formation, s.seed, s.encounter, s.nextId);
    return s;
}
Result apply(Rules& rules, State& s, const Action& action) {
    auto result = rules.apply(s, action); check(result.ok, "Integrated action: " + result.reason); return result;
}
void nextRound(Rules& rules, State& s) { apply(rules, s, Action::endTurn()); if (s.phase == Phase::Collection) apply(rules, s, Action::collect(0)); }
Result shoot(Rules& rules, State& s, Id target, Amount damage) {
    Part p; p.id = s.nextId++; p.kind = Kind::Ammo; p.recipe = "test-robot-payload";
    p.effects = {{Op::FlatDamage, Timing::Assembly, damage, 0}}; s.parts.push_back(p);
    apply(rules, s, Action::load({p.id})); return apply(rules, s, Action::fire(target));
}
void integrationContracts() {
    Rules rules;
    auto ram = integrated("C1-F-RAM"); const Id ramId = ram.enemies[0].id;
    shoot(rules, ram, ramId, 6); check(ram.enemies[0].hp == 21, "Engine applies Ram Armor3 once per direct hit.");
    nextRound(rules, ram); check(ram.hp == 80 && ram.enemies[0].intent.damage == 18, "Engine Ram Charge and committed Blast.");
    ram.protection.push_back({ram.nextId++, ram.nextOrder++, 18});
    nextRound(rules, ram); check(ram.hp == 80, "Engine Shield absorbs actual Blast.");

    auto warden = integrated("C1-F-WARDEN"); const Id wardenId = warden.enemies[0].id;
    shoot(rules, warden, wardenId, 0); check(warden.enemies[0].tiles == 3, "Zero hit consumes no tile.");
    for (int i = 0; i < 3; ++i) shoot(rules, warden, wardenId, 6);
    check(warden.enemies[0].hp == 41 && warden.enemies[0].tiles == 0, "Three positive hits each become1 and exhaust tiles.");
    shoot(rules, warden, wardenId, 6); check(warden.enemies[0].hp == 35, "Fourth hit keeps ordinary damage.");

    auto chassis = integrated("C1-F-CHASSIS"); const Id carrier = chassis.enemies[0].id;
    shoot(rules, chassis, carrier, 48);
    check(chassis.phase == Phase::Preparation && chassis.enemies.size() == 2 && chassis.enemies[0].dead, "Engine death release prevents premature victory.");
    const Id helper = chassis.enemies.back().id;
    nextRound(rules, chassis); check(chassis.hp == 80 && chassis.round == 2, "Engine defers preparation-born helper action.");
    shoot(rules, chassis, helper, 7); check(chassis.phase == Phase::Victory && chassis.kills == 2, "Engine requires helper defeat and counts two actual kills.");
    auto etched = integrated("C1-F-CHASSIS"); etched.enemies[0].hp = 1; etched.enemies[0].corrosion = 1;
    nextRound(rules, etched);
    check(etched.hp == 80 && etched.enemies.size() == 2 && etched.enemies[0].dead && etched.enemies[1].bornRound == 1, "Engine pre-action death spawn survives vector growth and defers newborn.");

    auto nest = integrated("C1-F-NEST");
    nextRound(rules, nest); check(nest.enemies.size() == 2 && nest.hp == 80, "Engine deployment does not act immediately.");
    nextRound(rules, nest); check(nest.hp == 69, "Engine next phase orders Nest6 then Mite5.");

    auto boss = integrated("C1-F-GATEBREAKER");
    boss.enemies[0].drive = 9; boss.enemies[0].corrosion = 46;
    nextRound(rules, boss);
    check(boss.hp == 80 && boss.enemies[0].hp == 46 && boss.enemies[0].drive == 0 && boss.enemies[0].robotAction == "quad_strike", "Engine pre-action Corrosion substitutes Recover in same phase.");
    auto after = integrated("C1-F-GATEBREAKER"); after.enemies[0].burn = 46;
    nextRound(rules, after);
    check(after.hp == 66 && after.enemies[0].hp == 46 && after.enemies[0].intent.move == Move::Recover, "Engine post-action Burn schedules next Recover.");
    auto instant = integrated("C1-F-GATEBREAKER"); shoot(rules, instant, instant.enemies[0].id, 100);
    check(instant.phase == Phase::Victory && instant.enemies[0].hp == 0, "Engine overkill bypasses artificial boss survival.");

    auto debuff = integrated("C1-F-BINDER"); nextRound(rules, debuff); nextRound(rules, debuff);
    check(debuff.recipeFouling == 1 && debuff.foulingRound == debuff.round, "Engine Binder Fouling applies to following player turn.");
    Recipe surcharge; surcharge.id = "robot-fouling-test"; surcharge.name = "Test Ammo"; surcharge.output = "Test Ammo";
    surcharge.kind = Kind::Ammo; surcharge.cost = {1, 0, 0, 0, 0}; surcharge.effects = {{Op::FlatDamage, Timing::Assembly, 1, 0}};
    Rules crafting({surcharge}); const Id copy = debuff.nextId++; debuff.memory.push_back({copy, surcharge.id}); debuff.materials = {1, 0, 0, 0, 0};
    const auto before = serialize(debuff); auto denied = crafting.apply(debuff, Action::craft(copy));
    check(!denied.ok && serialize(debuff) == before, "Fouling unaffordable surcharge is atomic.");
    debuff.materials[0] = 2; apply(crafting, debuff, Action::craft(copy));
    check(debuff.materials[0] == 0 && debuff.recipeFouling == 0, "Fouling consumes exactly one extra Iron and one affected Use.");
    auto stacked = integrated("C1-F-BINDER");
    for (int i = 0; i < 2; ++i) {
        auto more = makeCinderwallFormation("C1-F-BINDER", stacked.seed, "synthetic-stacking-probe", stacked.nextId);
        stacked.enemies.push_back(more[0]);
    }
    // Synthetic simultaneous sources verify the global cap; this combination is
    // deliberately absent from the production formation allowlist.
    for (auto& e : stacked.enemies) { e.robotAction = "foul"; e.intent = {Move::Attack, 6, 1}; e.patternCursor = 1; }
    nextRound(rules, stacked);
    check(stacked.recipeFouling == 2, "Engine caps Fouling at two uses across sources.");
    for (const auto kind : {Kind::Shield, Kind::Utility}) {
        Recipe unaffected; unaffected.id = kind == Kind::Shield ? "unaffected-shield" : "unaffected-utility";
        unaffected.name = unaffected.output = unaffected.id; unaffected.kind = kind;
        unaffected.effects = kind == Kind::Shield ? std::vector<Effect>{{Op::ShieldValue, Timing::Install, 1, 0}}
            : std::vector<Effect>{{Op::Heal, Timing::Use, 0, 0}};
        Rules unaffectedRules({unaffected}); auto unaffectedState = stacked;
        const Id unaffectedCopy = unaffectedState.nextId++; unaffectedState.memory.push_back({unaffectedCopy, unaffected.id});
        unaffectedState.materials = {};
        apply(unaffectedRules, unaffectedState, Action::craft(unaffectedCopy));
        check(unaffectedState.recipeFouling == 2 && unaffectedState.materials[0] == 0, "Fouling neither taxes nor counts non-Ammo Use.");
    }

    auto press = integrated("C1-F-PRESS"); nextRound(rules, press); nextRound(rules, press);
    check(press.shieldLeak == 2 && press.shieldLeakRound == press.round, "Engine Press Leak is next-turn penalty.");
    Part plate; plate.id = press.nextId++; plate.kind = Kind::Shield; plate.place = Place::Installed; plate.everInstalled = true;
    plate.shield = 5; plate.installOrder = press.nextOrder++; press.parts.push_back(plate);
    press.protection.push_back({press.nextId++, press.nextOrder++, 8});
    const Amount hp = press.hp; nextRound(rules, press);
    check(press.hp == hp - 2 && press.shieldLeak == 0, "Leak drains installed total once and preserves direct upgrade Shield.");
    auto directLeak = integrated("C1-F-PRESS"); nextRound(rules, directLeak); nextRound(rules, directLeak);
    directLeak.protection.push_back({directLeak.nextId++, directLeak.nextOrder++, 13});
    const Amount directHp = directLeak.hp; const auto leakResult = apply(rules, directLeak, Action::endTurn());
    check(directLeak.hp == directHp && directLeak.shieldLeak == 0, "Leak with no installed parts consumes itself and cannot drain direct Shield.");
    const auto leaked = std::find_if(leakResult.events.begin(), leakResult.events.end(), [](const Event& e) { return e.type == "shield_leak"; });
    check(leaked != leakResult.events.end() && leaked->amount == 0, "Leak event reports only installed protection actually removed.");

    auto saved = integrated("C1-F-BINDER"); saved.enemies[0].binderVariants = true;
    nextRound(rules, saved); nextRound(rules, saved); nextRound(rules, saved);
    State restored; std::string error;
    check(deserialize(serialize(saved), restored, error), "Robot state round-trip: " + error);
    const auto beforePreview = serialize(saved); const auto preview = rules.preview(saved, Action::endTurn());
    check(preview.result.ok && serialize(saved) == beforePreview, "Enemy-phase preview preserves committed live state and RNG.");
    const auto original = rules.apply(saved, Action::endTurn()); const auto replay = rules.apply(restored, Action::endTurn());
    check(original.ok && replay.ok && serialize(saved) == serialize(restored), "Saved packet stream, action and timed fields replay identically.");
    check(serialize(saved) == serialize(preview.state), "Committed enemy phase matches its exact preview.");
}
#endif
} // namespace

int main() {
    try {
        registryAndFactories();
        std::cout << "PASS registryAndFactories: formation identities and Mite/Ram actions\n";
        binderPackets();
        std::cout << "PASS binderPackets: fixed introduction and saved seeded variants\n";
        pressureAndOfficers();
        std::cout << "PASS pressureAndOfficers: Drive, multi-hit actions and final death\n";
        braceAndLockedSupport();
        std::cout << "PASS braceAndLockedSupport: temporary Armor, Leak and committed Nest action\n";
        finiteSummonsAndDeathIdentity();
        std::cout << "PASS finiteSummonsAndDeathIdentity: bounded supply, newborn delay and unique cores\n";
        recoveryJointAndEscapes();
        std::cout << "PASS recoveryJointAndEscapes: boss threshold and explicit departure clocks\n";
#ifdef OVERKILL_ROBOT_INTEGRATION
        integrationContracts();
        std::cout << "PASS integrationContracts: actual Engine defense, status, spawning and replay\n";
        std::cout << "Cinderwall robot module + Engine integration: " << assertions << " assertions passed.\n";
#else
        std::cout << "Cinderwall robot module: " << assertions << " assertions passed (mocked Engine callbacks; integration not established).\n";
#endif
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Robot assertion " << assertions << " failed: " << error.what() << '\n'; return 1;
    }
}

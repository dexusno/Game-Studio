// Independent QA fixtures. This file changes no production behavior.
#include "overkill/core.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace overkill;
namespace {
int passed = 0, failed = 0;
void expect(bool value, const std::string& message) { if (!value) throw std::runtime_error(message); }
Result act(const Rules& rules, State& s, const Action& a) {
    auto r = rules.apply(s, a); expect(r.ok, r.reason); return r;
}
State arena(const Rules& rules) {
    auto s = Rules::teachingEncounter(612, false);
    act(rules, s, Action::collect(0));
    s.enemies.resize(1);
    auto& e = s.enemies.front(); e.hp = e.maxHp = 10000;
    e.intent = {Move::Recover, 0, 0}; e.pattern = {e.intent};
    s.materials = {2000, 2000, 2000, 2000, 2000};
    return s;
}
Id known(const State& s, const std::string& recipe) {
    for (const auto& c : s.memory) if (c.recipe == recipe) return c.id;
    throw std::runtime_error("missing recipe " + recipe);
}
Id grantCopy(State& s, const std::string& recipe) {
    Id id = s.nextId++; s.memory.push_back({id, recipe, 0, 0, 0}); return id;
}
Id craftPart(const Rules& rules, State& s, Id copy) {
    const auto r = act(rules, s, Action::craft(copy));
    for (const auto& e : r.events) if (e.type == "part_created") return e.subject;
    throw std::runtime_error("no part created");
}
void activate(const Rules& rules, State& s, Id part) {
    Action a; a.type = ActionType::Activate; a.subject = part; act(rules, s, a);
}
void next(const Rules& rules, State& s) {
    act(rules, s, Action::endTurn()); act(rules, s, Action::collect(0));
}
void run(const std::string& name, const std::function<void()>& fn) {
    try { fn(); ++passed; std::cout << "PASS " << name << '\n'; }
    catch (const std::exception& e) { ++failed; std::cout << "FAIL " << name << ": " << e.what() << '\n'; }
}
}
int main() {
    Rules rules;
    run("SH050 duplicate named modifiers refresh", [&] {
        auto s = arena(rules);
        const Id first = grantCopy(s, "SH050"), second = grantCopy(s, "SH050");
        activate(rules, s, craftPart(rules, s, first));
        activate(rules, s, craftPart(rules, s, second));
        const auto slug = craftPart(rules, s, known(s, "SH001"));
        const auto powder = craftPart(rules, s, known(s, "SH003"));
        const auto before = s.enemies[0].hp;
        act(rules, s, Action::load({slug, powder}));
        act(rules, s, Action::fire(s.enemies[0].id));
        const auto actual = before - s.enemies[0].hp;
        expect(s.hp == 72, "both 4 HP costs must be paid");
        expect(actual == 14, "same named Modifier refreshes +60%: expected 14; observed " + std::to_string(actual));
    });
    run("different named percentage modifiers combine", [&] {
        auto content = Rules::starterContent();
        content.push_back({"QA-percent", "distinct percentage modifier", "test", Kind::Modifier, {}, 0,
            {{Op::PercentDamage, Timing::Activate, 20, 0}}});
        Rules controlled(content); auto s = arena(controlled);
        activate(controlled, s, craftPart(controlled, s, grantCopy(s, "SH050")));
        activate(controlled, s, craftPart(controlled, s, grantCopy(s, "QA-percent")));
        const auto slug = craftPart(controlled, s, known(s, "SH001"));
        const auto powder = craftPart(controlled, s, known(s, "SH003"));
        const auto before = s.enemies[0].hp;
        act(controlled, s, Action::load({slug, powder}));
        State decoded; std::string error;
        expect(deserialize(serialize(s), decoded, error), error);
        const auto forecast = controlled.preview(decoded, Action::fire(decoded.enemies[0].id));
        act(controlled, s, Action::fire(s.enemies[0].id));
        const auto actual = before - s.enemies[0].hp;
        expect(actual == 16, "9 + floor(9 * 80 / 100) expected 16; observed " + std::to_string(actual));
        expect(serialize(s) == serialize(forecast.state), "named bonuses changed across snapshot/preview");
        const auto nextSlug = craftPart(controlled, s, grantCopy(s, "SH001"));
        act(controlled, s, Action::load({nextSlug})); act(controlled, s, Action::fire(s.enemies[0].id));
        expect(before - s.enemies[0].hp == 22, "used named bonuses leaked onto later shot");
    });
    run("identical SH008 Magnet copies refresh one next-haul bonus", [&] {
        auto s = arena(rules);
        activate(rules, s, craftPart(rules, s, known(s, "SH008")));
        State decoded; std::string error;
        expect(deserialize(serialize(s), decoded, error), error); s = decoded;
        activate(rules, s, craftPart(rules, s, grantCopy(s, "SH008")));
        act(rules, s, Action::endTurn());
        const auto r = act(rules, s, Action::collect(0));
        const auto collected = std::find_if(r.events.begin(), r.events.end(), [](const Event& e) { return e.type == "collected"; });
        expect(collected != r.events.end(), "missing collection event");
        expect(collected->amount == 12, "same named Magnet refreshes +2: expected haul12; observed " + std::to_string(collected->amount));
    });
    run("SH006 removed source does not ward Burn", [&] {
        auto s = arena(rules); s.burn = 3;
        const auto pad = craftPart(rules, s, known(s, "SH006"));
        act(rules, s, Action::install(pad)); act(rules, s, Action::remove(pad));
        act(rules, s, Action::endTurn());
        expect(s.hp == 77, "no installed insulation or protection: expected HP 77; observed " + std::to_string(s.hp));
    });
    run("unknown effect opcode is rejected before content use", [&] {
        bool rejected = false;
        try {
            Recipe r; r.id = "QA-unknown"; r.kind = Kind::Utility; r.cost[0] = 1;
            r.effects = {{static_cast<Op>(255), Timing::Use, 9, 0}};
            Rules invalid({r});
            auto s = arena(rules); const auto before = s.materials[0];
            const auto result = invalid.apply(s, Action::craft(grantCopy(s, r.id)));
            if (!result.ok) rejected = true;
            else std::cout << "  observed unknown effect accepted; Iron paid=" << before - s.materials[0] << '\n';
        } catch (const std::exception&) { rejected = true; }
        expect(rejected, "unknown opcode was accepted and silently ignored");
    });
    run("unsupported valid effect timing is rejected", [&] {
        bool rejected = false;
        try {
            Recipe r; r.id = "QA-unsupported-clock"; r.kind = Kind::Utility; r.cost[0] = 1;
            r.effects = {{Op::FlatDamage, Timing::Use, 9, 0}};
            Rules invalid({r});
            auto s = arena(rules); const auto result = invalid.apply(s, Action::craft(grantCopy(s, r.id)));
            rejected = !result.ok;
        } catch (const std::exception&) { rejected = true; }
        expect(rejected, "accepted a valid opcode on an unimplemented clock without rejection");
    });
    run("effect clock unreachable for the recipe kind is rejected", [&] {
        bool rejected = false;
        try {
            Recipe r; r.id = "QA-unreachable-clock"; r.kind = Kind::Utility; r.cost[0] = 1;
            r.effects = {{Op::FlatDamage, Timing::Assembly, 9, 0}};
            Rules invalid({r});
            auto s = arena(rules); const auto before = s.materials[0];
            const auto result = invalid.apply(s, Action::craft(grantCopy(s, r.id)));
            rejected = !result.ok;
            if (!rejected) std::cout << "  observed Utility/Assembly accepted; Iron paid=" << before - s.materials[0] << '\n';
        } catch (const std::exception&) { rejected = true; }
        expect(rejected, "Utility can never execute Assembly but definition was accepted");
    });
    run("lethal first spread recoil prevents later spread and ordinary heal", [&] {
        auto content = Rules::starterContent();
        content.push_back({"QA-heal", "controlled healing shot", "test", Kind::Ammo, {}, 0,
            {{Op::FlatDamage, Timing::Assembly, 6, 0}, {Op::Heal, Timing::AfterHit, 30, 0}}});
        Rules controlled(content); auto s = arena(controlled); s.hp = 2;
        auto extra = s.enemies[0]; extra.id = s.nextId++; extra.hp = extra.maxHp = 2; extra.mesh = 50;
        s.enemies.push_back(extra);
        auto third = s.enemies[0]; third.id = s.nextId++; s.enemies.push_back(third);
        const auto slug = craftPart(controlled, s, grantCopy(s, "QA-heal"));
        const auto nozzle1 = craftPart(controlled, s, known(s, "SH005"));
        const auto nozzle2 = craftPart(controlled, s, grantCopy(s, "SH005"));
        act(controlled, s, Action::load({slug, nozzle1, nozzle2}));
        const auto r = act(controlled, s, Action::fire(s.enemies[0].id, {{nozzle1, extra.id}, {nozzle2, third.id}}));
        // Main-hit healing is legitimately earlier than the first spread's recoil.
        expect(s.phase == Phase::Defeat && s.hp == 0 && s.enemies[1].dead, "spread recoil must kill");
        expect(s.enemies[2].hp == 10000, "later spread must not resolve after final death");
        const auto death = std::find_if(r.events.begin(), r.events.end(), [](const Event& e) { return e.type == "defeat"; });
        expect(death != r.events.end() && std::next(death) == r.events.end(), "ordinary events followed final death");
    });
    run("lethal main recoil stops after-hit healing", [&] {
        auto content = Rules::starterContent();
        content.push_back({"QA-heal", "controlled healing shot", "test", Kind::Ammo, {}, 0,
            {{Op::FlatDamage, Timing::Assembly, 6, 0}, {Op::Heal, Timing::AfterHit, 30, 0}}});
        Rules controlled(content); auto s = arena(controlled); s.hp = 2; s.enemies[0].hp = 1; s.enemies[0].mesh = 4;
        const auto p = craftPart(controlled, s, grantCopy(s, "QA-heal"));
        act(controlled, s, Action::load({p})); const auto r = act(controlled, s, Action::fire(s.enemies[0].id));
        expect(s.phase == Phase::Defeat && s.hp == 0 && s.kills == 1, "lethal main-hit priority");
        expect(std::none_of(r.events.begin(), r.events.end(), [](const Event& e) { return e.type == "heal" || e.type == "victory"; }), "heal/victory followed lethal recoil");
    });
    run("saved depleted Shield survives serialization and next-round reinstall", [&] {
        auto s = arena(rules); s.heat = 3;
        const auto frame = craftPart(rules, s, grantCopy(s, "MA017")); act(rules, s, Action::install(frame));
        Rules::spendShield(s, 10); act(rules, s, Action::remove(frame));
        State loaded; std::string error;
        expect(deserialize(serialize(s), loaded, error), error); next(rules, loaded);
        act(rules, loaded, Action::install(frame));
        expect(Rules::shield(loaded) == 0 && loaded.heat == 0, "spent Shield recharged or original Heat charged twice");
        const auto p = std::find_if(loaded.parts.begin(), loaded.parts.end(), [&](const Part& x) { return x.id == frame; });
        expect(p != loaded.parts.end() && p->paidHeat == 3 && p->firstInstallRound == 1, "installation history changed");
    });
    run("cooling in blocked round permits use and restarts its exemption", [&] {
        auto s = arena(rules); const auto copy = known(s, "SH005");
        craftPart(rules, s, copy); next(rules, s);
        const auto original = serialize(s); expect(!rules.apply(s, Action::craft(copy)).ok, "cooldown-1 ready too early");
        expect(serialize(s) == original, "failed cooldown action mutated state");
        act(rules, s, Action::craft(known(s, "SH007"))); craftPart(rules, s, copy); next(rules, s);
        expect(!rules.apply(s, Action::craft(copy)).ok, "reused round's automatic tick cleared new cooldown");
        next(rules, s); craftPart(rules, s, copy);
    });
    run("enemy Weaken reduces each hit and decays during a nonattack", [&] {
        auto s = arena(rules); auto& enemy = s.enemies[0]; enemy.weaken = 3;
        enemy.intent = {Move::Attack, 6, 2}; enemy.pattern = {{Move::Attack, 6, 2}, {Move::Recover, 0, 0}};
        next(rules, s); expect(s.hp == 74 && s.enemies[0].weaken == 2, "expected 3+3 damage and Weaken 2");
        next(rules, s); expect(s.hp == 74 && s.enemies[0].weaken == 1, "nonattack must decay Weaken once");
    });
    run("Armor and Tiles apply per direct hit; Burn uses Shield before HP", [&] {
        auto s = arena(rules); auto& enemy = s.enemies[0]; enemy.armor = 3; enemy.tiles = 2; enemy.shield = 4; enemy.burn = 5;
        const auto target = enemy.id;
        const auto powder = craftPart(rules, s, known(s, "SH003")); act(rules, s, Action::load({powder}));
        act(rules, s, Action::fire(target)); expect(s.enemies[0].tiles == 2, "Armor-zero hit spent a Tile");
        next(rules, s); expect(s.enemies[0].hp == 9999 && s.enemies[0].shield == 0 && s.enemies[0].tiles == 2 && s.enemies[0].burn == 4, "Burn defence or decay incorrect");
    });
    run("forty rounds of legal stockpiling permit forty-part assemblies and repeated Fire", [&] {
        auto s = arena(rules); std::vector<Id> ammo, shields;
        for (int i = 0; i < 40; ++i) {
            ammo.push_back(craftPart(rules, s, known(s, "SH001")));
            shields.push_back(craftPart(rules, s, known(s, "SH002")));
            if (i != 39) next(rules, s);
        }
        for (const auto id : shields) act(rules, s, Action::install(id));
        expect(Rules::shield(s) == 240, "forty-part Shield blocked");
        for (int i = 0; i < 6; ++i) { act(rules, s, Action::load({ammo.back()})); ammo.pop_back(); act(rules, s, Action::fire(s.enemies[0].id)); }
        act(rules, s, Action::load(ammo)); act(rules, s, Action::fire(s.enemies[0].id));
        expect(s.round == 40 && s.shots == 7 && s.enemies[0].hp == 9760, "stockpiled parts or repeated Fire restricted");
    });
    run("preview, snapshot reload and replay preserve RNG and event sequence", [&] {
        auto s = arena(rules); const auto p = craftPart(rules, s, known(s, "SH008")); activate(rules, s, p);
        act(rules, s, Action::endTurn()); const auto before = serialize(s); const auto a = Action::collect(4, 2);
        const auto p1 = rules.preview(s, a), p2 = rules.preview(s, a);
        expect(serialize(s) == before && serialize(p1.state) == serialize(p2.state), "preview changed source or differed");
        State loaded; std::string error; expect(deserialize(before, loaded, error), error);
        const auto r = act(rules, loaded, a);
        expect(serialize(loaded) == serialize(p1.state) && r.events.size() == p1.result.events.size(), "snapshot continuation differs from preview");
        for (std::size_t i = 0; i < r.events.size(); ++i) expect(eventJson(r.events[i]) == eventJson(p1.result.events[i]), "event trace differs");
        auto damaged = before; damaged[damaged.size() - 2] ^= 1; const auto committed = serialize(loaded);
        expect(!deserialize(damaged, loaded, error) && serialize(loaded) == committed, "corrupt save changed destination");
    });
    std::cout << "Independent cases: " << passed << " passed, " << failed << " failed.\n";
    return failed == 0 ? 0 : 1;
}

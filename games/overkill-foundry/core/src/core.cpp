#include "overkill/core.hpp"
#include <algorithm>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace overkill {
namespace {
struct Invalid : std::runtime_error { using std::runtime_error::runtime_error; };
void require(bool value, const char* reason) { if (!value) throw Invalid(reason); }
Amount checked(std::int64_t value) {
    require(value >= 0 && value <= std::numeric_limits<Amount>::max(), "Quantity exceeds supported integer range.");
    return static_cast<Amount>(value);
}
Amount add(Amount a, Amount b) { return checked(static_cast<std::int64_t>(a) + b); }
bool alive(const Enemy& e) { return !e.dead && !e.escaped && e.hp > 0; }
bool supported(const Effect& e) {
    const bool immediate=e.timing==Timing::Use || e.timing==Timing::Install || e.timing==Timing::AfterHit || e.timing==Timing::Activate;
    switch(e.op) {
    case Op::FlatDamage: case Op::PercentDamage: return e.timing==Timing::Assembly || e.timing==Timing::Activate;
    case Op::AttackIntentBonus: case Op::SpreadPercent: return e.timing==Timing::Assembly;
    case Op::ShieldValue: case Op::BurnWard: return e.timing==Timing::Install;
    case Op::HeatCost: case Op::HpCost: return e.timing==Timing::Use || e.timing==Timing::Install || e.timing==Timing::Assembly || e.timing==Timing::Activate;
    case Op::Burn: case Op::BurnIfHeat: return e.timing==Timing::AfterHit;
    case Op::Heat: case Op::Cool: case Op::ShieldGrant: case Op::DelayedShield:
    case Op::DelayedHeat: case Op::DelayedHaul: case Op::Heal: return immediate;
    default: return false;
    }
}
template<class T> T* byId(std::vector<T>& items, Id id) {
    auto it = std::find_if(items.begin(), items.end(), [id](const T& x) { return x.id == id; });
    return it == items.end() ? nullptr : &*it;
}
template<class T> const T* byId(const std::vector<T>& items, Id id) {
    auto it = std::find_if(items.begin(), items.end(), [id](const T& x) { return x.id == id; });
    return it == items.end() ? nullptr : &*it;
}
std::uint64_t mix(std::uint64_t x) {
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}
std::uint64_t textHash(const std::string& text) {
    std::uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : text) { h ^= c; h *= 1099511628211ULL; }
    return h;
}
struct Engine {
    State& s;
    const Rules& rules;
    std::vector<Event> events;
    Id root = 0;
    Id emit(const std::string& type, Id subject = 0, Id target = 0, Amount amount = 0, Amount secondary = 0, Id parent = 0) {
        const Id id = s.nextEvent++;
        events.push_back({id, parent ? parent : root, subject, target, type, amount, secondary});
        return id;
    }
    void heat(Amount n) { const auto old = s.heat; s.heat = std::min(10, add(s.heat, n)); emit("heat", 0, 0, s.heat-old); }
    void terminal() {
        if (s.hp <= 0) { s.phase = Phase::Defeat; emit("defeat"); return; }
        if (std::none_of(s.enemies.begin(), s.enemies.end(), alive)) {
            s.phase = s.kills > 0 ? Phase::Victory : Phase::Escaped;
            emit(s.kills > 0 ? "victory" : "escape_complete");
        }
    }
    void playerDamage(Amount amount, Id source, Id parent, bool bypass = false) {
        const Amount absorbed = bypass ? 0 : Rules::spendShield(s, amount);
        const Amount lost = std::min(s.hp, amount - absorbed);
        s.hp -= lost;
        const Id hit = emit("player_damage", source, 0, lost, absorbed, parent);
        if (s.hp == 0 && s.reserveHeartPump) {
            s.reserveHeartPump = false;
            s.hp = std::max(1, s.maxHp / 4);
            emit("reserve_heart_pump", 0, 0, s.hp, 0, hit);
        }
        if (s.hp == 0) { s.phase = Phase::Defeat; emit("defeat", 0, 0, 0, 0, hit); }
    }
    void enemyDamage(Id target, Amount amount, bool direct, bool bypass = false, Id source = 0) {
        auto* e = byId(s.enemies, target);
        if (!e || !alive(*e)) { emit("hit_lost", source, target); return; }
        Amount damage = direct && !bypass ? std::max(0, amount - e->armor) : amount;
        if (direct && e->tiles > 0 && damage > 0) { --e->tiles; damage = std::min(damage, 1); }
        const Amount shieldLoss = bypass ? 0 : std::min(e->shield, damage);
        e->shield -= shieldLoss;
        const Amount hpLoss = std::min(e->hp, damage - shieldLoss);
        e->hp -= hpLoss;
        const Amount recoil = direct && shieldLoss + hpLoss > 0 ? e->mesh : 0;
        const Id hit = emit(direct ? "hit" : "status_damage", source, target, hpLoss, shieldLoss);
        if (e->hp == 0) { e->dead = true; ++s.kills; emit("enemy_death", source, target, 0, 0, hit); }
        // Capture the response before death, then resolve it ahead of payloads and sibling hits.
        if (recoil > 0) { const auto response = emit("recoil", target, 0, recoil, 0, hit); playerDamage(recoil, target, response); }
    }
    void pay(const std::vector<Effect>& effects, Timing timing, Part* part = nullptr) {
        Amount hp = 0, heatCost = 0;
        for (const auto& e : effects) if (e.timing == timing) {
            if (e.op == Op::HpCost) hp = add(hp, e.amount);
            if (e.op == Op::HeatCost) heatCost = add(heatCost, e.amount);
        }
        require(s.hp > hp, "Own HP cost must leave at least 1 HP.");
        require(s.heat >= heatCost, "Not enough Heat for this action.");
        s.hp -= hp; s.heat -= heatCost;
        if (part) { part->paidHp = hp; part->paidHeat = heatCost; }
        if (hp) emit("hp_cost", part ? part->id : 0, 0, hp);
        if (heatCost) emit("heat_cost", part ? part->id : 0, 0, heatCost);
    }
    Id shieldPart(Amount value, const std::string& source, bool installed) {
        Part p;
        p.id = s.nextId++; p.recipe = source; p.output = "Shield";
        p.resaleReference = "generated:shield:" + std::to_string(value);
        p.kind = Kind::Shield; p.createdRound = s.round; p.originalValue = value;
        p.effects = {{Op::ShieldValue, Timing::Install, value, 0}};
        s.parts.push_back(p);
        emit("part_created", p.id, 0, value);
        if (installed) install(p.id);
        return p.id;
    }
    void schedule(DeliveryKind kind, Amount amount, const std::string& source) {
        if(kind==DeliveryKind::Haul) {
            for(auto& delivery:s.deliveries) if(delivery.kind==kind && delivery.source==source && delivery.dueRound==s.round+1) {
                delivery.amount=amount; emit("delivery_refreshed",delivery.id,0,amount,delivery.dueRound); return;
            }
        }
        const Id id = s.nextId++;
        s.deliveries.push_back({id, source, s.round + 1, amount, kind});
        emit("delivery_scheduled", id, 0, amount, s.round + 1);
    }
    void effects(const std::vector<Effect>& list, Timing timing, Id target, Amount heatAtFire, const std::string& source) {
        Amount activatedFlat=0,activatedPercent=0;bool hasShotBonus=false;
        for (const auto& effect : list) {
            if (s.phase == Phase::Defeat) break;
            if (effect.timing != timing) continue;
            switch (effect.op) {
            case Op::Heat: heat(effect.amount); break;
            case Op::Cool:
                for (auto& copy : s.memory) {
                    const auto before = copy.cooldown;
                    copy.cooldown = std::max(0, copy.cooldown - effect.amount);
                    if (before != copy.cooldown) emit("cooled", copy.id, 0, before-copy.cooldown);
                }
                break;
            case Op::ShieldGrant: shieldPart(effect.amount, source, true); break;
            case Op::DelayedShield: schedule(DeliveryKind::ShieldPart, effect.amount, source); break;
            case Op::DelayedHeat: schedule(DeliveryKind::Heat, effect.amount, source); break;
            case Op::DelayedHaul: schedule(DeliveryKind::Haul, effect.amount, source); break;
            case Op::BurnWard: break; // Armed on the physical source; checked while installed at Burn's clock.
            case Op::Heal: {
                Amount restored = std::min(effect.amount, s.maxHp - s.hp);
                // threshold is a recipe-wide fight cap; all duplicate copies share it.
                if (effect.threshold) restored = std::min(restored, std::max(0, effect.threshold-s.quickPatchHealing));
                s.hp += restored; if (effect.threshold) s.quickPatchHealing += restored;
                emit("heal", 0, 0, restored); break;
            }
            case Op::BurnIfHeat: case Op::Burn: {
                auto* enemy = byId(s.enemies, target);
                if (enemy && alive(*enemy) && (effect.op == Op::Burn || heatAtFire >= effect.threshold)) {
                    enemy->burn = add(enemy->burn, effect.amount); emit("burn_applied", 0, target, effect.amount);
                }
                break;
            }
            case Op::FlatDamage: if (timing == Timing::Activate) {activatedFlat=add(activatedFlat,effect.amount);hasShotBonus=true;} break;
            case Op::PercentDamage: if (timing == Timing::Activate) {activatedPercent=add(activatedPercent,effect.amount);hasShotBonus=true;} break;
            default: break; // Costs and assembly values have their own explicit resolution boundaries.
            }
        }
        if(hasShotBonus && s.phase!=Phase::Defeat) {
            auto old=std::find_if(s.shotBonuses.begin(),s.shotBonuses.end(),[&](const ShotBonus& b){return b.source==source;});
            if(old==s.shotBonuses.end())s.shotBonuses.push_back({source,activatedFlat,activatedPercent});
            else {*old={source,activatedFlat,activatedPercent};}
        }
    }
    void install(Id id) {
        auto* part = byId(s.parts, id);
        require(part && part->kind == Kind::Shield && part->place == Place::Reserve, "Select a reserved Shield part.");
        if (!part->everInstalled) {
            pay(part->effects, Timing::Install, part);
            Amount value = 0;
            for (const auto& e : part->effects) if (e.op == Op::ShieldValue && e.timing == Timing::Install) value = add(value,e.amount);
            part->shield = value; part->originalValue = value;
            part->everInstalled = true; part->firstInstallRound = s.round; part->bindingOrder = s.nextOrder++;
            part->place = Place::Installed; part->installOrder = s.nextOrder++;
            // Copy the hook list before payloads can append to the physical inventory.
            const auto list = part->effects; const auto source = part->recipe;
            emit("first_install", id, 0, value);
            effects(list, Timing::Install, 0, s.heat, source);
        } else {
            part->place = Place::Installed; part->installOrder = s.nextOrder++;
            emit("reinstall", id, 0, part->shield);
        }
    }
    void craft(Id id) {
        auto* copy = byId(s.memory, id);
        require(copy != nullptr, "This recipe is not in memory.");
        const auto* recipe = rules.recipe(copy->recipe);
        require(recipe != nullptr, "This recipe has no verified runtime implementation.");
        require(copy->cooldown == 0, "This recipe is cooling.");
        require(recipe->cooldown > 0 || copy->usedRound != s.round, "This copy has already been used this round.");
        for (std::size_t i=0; i<5; ++i) require(s.materials[i] >= recipe->cost[i], "Not enough materials.");
        pay(recipe->effects, Timing::Use);
        for (std::size_t i=0; i<5; ++i) s.materials[i] -= recipe->cost[i];
        copy->cooldown = recipe->cooldown; copy->usedRound = s.round; ++copy->usesThisRound;
        emit("recipe_used", id);
        if (recipe->kind != Kind::Utility && !recipe->automaticOutput) {
            Part p; p.id = s.nextId++; p.recipe = recipe->id; p.output = recipe->output;
            p.resaleReference = recipe->id; p.kind = recipe->kind; p.effects = recipe->effects; p.createdRound = s.round;
            s.parts.push_back(p); emit("part_created", p.id);
        }
        effects(recipe->effects, Timing::Use, 0, s.heat, recipe->id);
    }
    void unload() {
        for (Id id : s.bullet) if (auto* p = byId(s.parts,id)) p->place = Place::Reserve;
        s.bullet.clear(); emit("unload");
    }
    void fire(const Action& action) {
        require(!s.bullet.empty(), "Load a nonempty bullet first.");
        const auto* target = byId(s.enemies,action.target);
        require(target && alive(*target), "Select a living main target.");
        const bool attacking = target->intent.move == Move::Attack;
        const Amount heatAtFire = s.heat;
        Amount base = s.nextFlat, percent = s.nextPercent;
        for(const auto& bonus:s.shotBonuses){base=add(base,bonus.flat);percent=add(percent,bonus.percent);}
        std::vector<Part> fired;
        std::vector<Effect> combinedCosts;
        std::vector<std::pair<Id,Amount>> spreads;
        std::set<Id> spreadIds;
        for (const Id id : s.bullet) {
            const auto* p = byId(s.parts,id);
            require(p && p->place == Place::Loaded, "Loaded part is missing.");
            fired.push_back(*p);
            for (const auto& effect : p->effects) {
                if (effect.timing != Timing::Assembly) continue;
                combinedCosts.push_back(effect);
                if (effect.op == Op::FlatDamage) base = add(base,effect.amount);
                if (effect.op == Op::AttackIntentBonus && attacking) base = add(base,effect.amount);
                if (effect.op == Op::PercentDamage) percent = add(percent,effect.amount);
                if (effect.op == Op::SpreadPercent) {
                    const auto match = std::find_if(action.spreadTargets.begin(),action.spreadTargets.end(),[id](const SpreadTarget& a){ return a.part == id; });
                    require(match != action.spreadTargets.end(), "Choose another enemy for every spreading part.");
                    const auto* other = byId(s.enemies,match->enemy);
                    require(other && alive(*other) && other->id != action.target, "Spread target must be another living enemy.");
                    spreadIds.insert(id); spreads.emplace_back(other->id,effect.amount);
                }
            }
        }
        require(spreadIds.size() == action.spreadTargets.size(), "Duplicate or unused spread targeting.");
        pay(combinedCosts, Timing::Assembly);
        if (s.hotBarrel) base = add(base,heatAtFire/2);
        const Amount shot = std::max(0, checked(static_cast<std::int64_t>(base) + static_cast<std::int64_t>(base)*percent/100) - s.weaken);
        const Amount main = add(shot,target->mark);
        byId(s.enemies,action.target)->mark = 0;
        s.parts.erase(std::remove_if(s.parts.begin(),s.parts.end(),[&](const Part& p){ return p.place == Place::Loaded; }),s.parts.end());
        s.bullet.clear(); ++s.shots; s.nextFlat = 0; s.nextPercent = 0; s.shotBonuses.clear();
        emit("fire", 0, action.target, shot, heatAtFire);
        enemyDamage(action.target, main, true);
        if (s.phase == Phase::Defeat) return;
        for (const auto& p : fired) effects(p.effects,Timing::AfterHit,action.target,heatAtFire,p.recipe);
        for (const auto& spread : spreads) {
            if (s.phase == Phase::Defeat) break;
            enemyDamage(spread.first,checked(static_cast<std::int64_t>(shot)*spread.second/100),true);
        }
        if (s.phase != Phase::Defeat) terminal();
    }
    void commitIntents() {
        for (auto& e : s.enemies) if (alive(e)) {
            if (e.departureRound > 0 && s.round >= e.departureRound) e.intent = {Move::Escape,0,0};
            else if (!e.pattern.empty()) e.intent = e.pattern[static_cast<std::size_t>(e.patternCursor) % e.pattern.size()];
            emit("intent", e.id, 0, static_cast<Amount>(e.intent.move), e.intent.damage);
        }
    }
    void endTurn() {
        unload(); s.nextFlat = 0; s.nextPercent = 0; s.shotBonuses.clear();
        emit("end_turn", 0, 0, s.round);
        if (s.burn > 0) {
            const bool ward=!s.burnWardSpent && std::any_of(s.parts.begin(),s.parts.end(),[&](const Part& p){
                return p.place==Place::Installed && p.firstInstallRound==s.round && std::any_of(p.effects.begin(),p.effects.end(),[](const Effect& f){return f.op==Op::BurnWard;});
            });
            const Amount tick=s.burn--;
            if(!ward)playerDamage(tick,0,root);else {s.burnWardSpent=true;emit("burn_prevented",0,0,tick);}
        }
        if (s.phase == Phase::Defeat) return;
        s.weaken = std::max(0,s.weaken-1);
        std::vector<Id> actors; for (const auto& e : s.enemies) if (alive(e) && e.bornRound < s.round) actors.push_back(e.id);
        for (Id id : actors) {
            auto* e = byId(s.enemies,id); if (!e || !alive(*e)) continue;
            if (e->corrosion > 0) { const auto tick = e->corrosion--; enemyDamage(id,tick,false,true); }
            e = byId(s.enemies,id); if (!e || !alive(*e)) continue;
            const auto intent = e->intent;
            const auto action = emit("enemy_action",id,0,static_cast<Amount>(intent.move));
            if (intent.move == Move::Escape) { e->escaped = true; emit("enemy_escape",id); continue; }
            if (intent.move == Move::Attack) {
                const Amount damage = std::max(0,add(intent.damage,e->drive)-e->weaken);
                for (Amount hit=0; hit<intent.hits && s.phase != Phase::Defeat; ++hit) playerDamage(damage,id,action);
            }
            if (s.phase == Phase::Defeat) return;
            e = byId(s.enemies,id);
            if (alive(*e) && e->burn > 0) { const auto tick=e->burn--; enemyDamage(id,tick,false); }
            ++e->patternCursor;
        }
        terminal(); if (s.phase == Phase::Victory || s.phase == Phase::Escaped || s.phase == Phase::Defeat) return;
        Amount available = Rules::shield(s), retained=0;
        for (Amount allowance : s.retentionAllowances) {
            const auto actual = std::min(available,allowance); available -= actual; retained = add(retained,actual);
            emit("shield_retained",0,0,actual);
        }
        s.parts.erase(std::remove_if(s.parts.begin(),s.parts.end(),[](const Part& p){ return p.place == Place::Installed || p.place == Place::Fitted; }),s.parts.end());
        s.protection.clear(); if (retained) s.protection.push_back({s.nextId++,s.nextOrder++,retained});
        for (auto& e : s.enemies) if (alive(e)) e.weaken = std::max(0,e.weaken-1);
        for (auto& copy : s.memory) { if (copy.usedRound != s.round) copy.cooldown = std::max(0,copy.cooldown-1); copy.usesThisRound = 0; }
        s.heat = std::max(0,s.heat-2); s.burnWardSpent = false;
        ++s.round;
        if (s.corrosion > 0) { const auto tick=s.corrosion--; playerDamage(tick,0,root,true); }
        if (s.phase == Phase::Defeat) return;
        std::vector<Delivery> pending;
        for (const auto& delivery : s.deliveries) {
            if (delivery.dueRound > s.round) { pending.push_back(delivery); continue; }
            emit("delivery",delivery.id,0,delivery.amount);
            if (delivery.kind == DeliveryKind::ShieldPart) shieldPart(delivery.amount,delivery.source,true);
            if (delivery.kind == DeliveryKind::Heat) heat(delivery.amount);
            if (delivery.kind == DeliveryKind::Haul) s.haulBonus = add(s.haulBonus,delivery.amount);
        }
        s.deliveries = std::move(pending);
        s.phase = Phase::Collection; commitIntents();
    }
};
Effect fx(Op op, Timing timing, Amount n, Amount threshold = 0) { return {op,timing,n,threshold}; }
} // namespace

Rng Rng::seeded(std::uint64_t seed, std::string encounter) {
    static const std::array<const char*,7> names{{"route","formation","robot","collection","reward","shop","choice"}};
    Rng r;
    for (std::size_t i=0;i<names.size();++i) r.state[i]=mix(seed ^ textHash("overkill/v1/"+std::string(names[i])+"/"+encounter));
    return r;
}
std::uint32_t Rng::below(Domain domain, std::uint32_t maximum) {
    require(maximum>0,"Random range must be positive.");
    auto& stream=state.at(static_cast<std::size_t>(domain));
    const auto threshold=(std::numeric_limits<std::uint32_t>::max()-maximum+1U)%maximum;
    for (;;) { stream += 0x9e3779b97f4a7c15ULL; const auto n=static_cast<std::uint32_t>(mix(stream)>>32); if(n>=threshold) return n%maximum; }
}
Action Action::collect(Amount material, Amount precisionResult) { Action a; a.type=ActionType::Collect; a.steering=material; a.precision=precisionResult; return a; }
Action Action::craft(Id copy) { Action a; a.type=ActionType::Craft; a.subject=copy; return a; }
Action Action::install(Id part) { Action a; a.type=ActionType::Install; a.subject=part; return a; }
Action Action::remove(Id part) { Action a; a.type=ActionType::Remove; a.subject=part; return a; }
Action Action::load(std::vector<Id> parts) { Action a; a.type=ActionType::Load; a.parts=std::move(parts); return a; }
Action Action::fire(Id target,std::vector<SpreadTarget> spreads) { Action a; a.type=ActionType::Fire; a.target=target; a.spreadTargets=std::move(spreads); return a; }
Action Action::endTurn() { return {}; }

Rules::Rules(std::vector<Recipe> recipes): recipes_(std::move(recipes)) {
    std::set<std::string> ids;
    for (const auto& r:recipes_) {
        require(!r.id.empty() && ids.insert(r.id).second,"Duplicate or empty recipe ID.");
        require(r.kind<=Kind::Modifier,"Unsupported recipe kind.");
        require(r.cooldown>=0,"Negative recipe cooldown.");
        require(!r.effects.empty(),"Recipe has no implemented effects.");
        for (Amount n:r.cost) require(n>=0,"Negative material cost.");
        for (const auto& e:r.effects) {
            require(e.amount>=0 && e.threshold>=0,"Negative effect quantity.");
            require(supported(e),"Unsupported effect opcode or timing.");
            bool reachable=e.timing==Timing::Use;
            if(!r.automaticOutput) {
                reachable=reachable || (e.timing==Timing::Install && r.kind==Kind::Shield);
                reachable=reachable || ((e.timing==Timing::Assembly || e.timing==Timing::AfterHit) && (r.kind==Kind::Ammo || r.kind==Kind::Spread));
                reachable=reachable || (e.timing==Timing::Activate && (r.kind==Kind::Modifier || r.kind==Kind::Magnet));
            }
            require(reachable,"Recipe kind cannot execute the declared effect timing.");
        }
    }
}
const Recipe* Rules::recipe(const std::string& id) const {
    auto it=std::find_if(recipes_.begin(),recipes_.end(),[&](const Recipe& r){ return r.id==id; }); return it==recipes_.end()?nullptr:&*it;
}
std::vector<Recipe> Rules::starterContent() {
    using T=Timing; using O=Op; using K=Kind;
    return {
        {"SH001","Solid Casting","Plain Slug",K::Ammo,{1,0,0,0,0},0,{fx(O::FlatDamage,T::Assembly,6)}},
        {"SH002","Flat Plate","Basic Shield Plate",K::Shield,{1,1,0,0,0},0,{fx(O::ShieldValue,T::Install,6)}},
        {"SH003","Powder Packing","Powder Cap",K::Ammo,{0,0,1,0,0},0,{fx(O::FlatDamage,T::Assembly,3)}},
        {"SH004","Simple Sighting","Aim Fin",K::Ammo,{1,0,0,1,0},0,{fx(O::FlatDamage,T::Assembly,4),fx(O::AttackIntentBonus,T::Assembly,4)}},
        {"SH005","Split Outlet","Forked Nozzle",K::Spread,{2,1,0,0,0},1,{fx(O::SpreadPercent,T::Assembly,50)}},
        {"SH006","Basic Insulation","Insulating Pad",K::Shield,{0,1,0,1,0},0,{fx(O::ShieldValue,T::Install,4),fx(O::BurnWard,T::Install,1)}},
        {"SH007","Cooling Mix","Coolant Plug",K::Utility,{0,1,0,1,0},0,{fx(O::Cool,T::Use,1)}},
        {"SH008","Extra Lift","Magnet Lift Ring",K::Magnet,{0,1,0,0,1},1,{fx(O::DelayedHaul,T::Activate,2)}},
        {"MA001","Fuel Brick","Fuel Briquette",K::Utility,{0,0,1,0,0},0,{fx(O::Heat,T::Use,3)}},
        {"MA002","Hot Cast","Fresh Slug",K::Ammo,{1,0,1,0,0},0,{fx(O::FlatDamage,T::Assembly,6),fx(O::BurnIfHeat,T::AfterHit,2,4)}},
        {"MA003","Boiler Jacket","Wrapped Plate",K::Shield,{1,1,0,0,0},0,{fx(O::ShieldValue,T::Install,6),fx(O::Heat,T::Install,1)}},
        {"MA004","Quick Vent","Vent Valve",K::Utility,{0,1,0,0,0},0,{fx(O::HeatCost,T::Use,2),fx(O::ShieldGrant,T::Use,5)}},
        {"SH013","Hot Filling","Ember Capsule",K::Ammo,{1,0,1,0,0},0,{fx(O::FlatDamage,T::Assembly,4),fx(O::Burn,T::AfterHit,2)}},
        {"SH026","Folding Brace","Regular Shield",K::Shield,{2,0,0,1,0},1,{fx(O::ShieldGrant,T::Use,4),fx(O::DelayedShield,T::Use,6)},true},
        {"SH050","Risky Packing","Overpacked Charge",K::Modifier,{1,0,2,0,0},1,{fx(O::HpCost,T::Activate,4),fx(O::PercentDamage,T::Activate,60)}},
        {"SH074","Quick Patch","Small Repair Kit",K::Utility,{1,0,0,1,1},2,{fx(O::Heal,T::Use,4,8)}},
        {"MA017","Quench Ribs","Cooling Frame",K::Shield,{1,1,0,0,0},0,{fx(O::HeatCost,T::Install,3),fx(O::ShieldValue,T::Install,10)}},
        {"MA019","Blood Fuel","Emergency Fuel Cell",K::Utility,{0,1,1,0,0},1,{fx(O::HpCost,T::Use,3),fx(O::Heat,T::Use,5)}},
        {"MA020","Overfeed","Slow Fuel Cake",K::Utility,{0,0,2,0,0},0,{fx(O::Heat,T::Use,1),fx(O::DelayedHeat,T::Use,5)}}
    };
}
State Rules::teachingEncounter(std::uint64_t seed,bool mara) {
    State s; s.seed=seed; s.rng=Rng::seeded(seed,s.encounter); s.hotBarrel=mara;
    const auto content=starterContent();
    for (std::size_t i=0;i<12;++i) s.memory.push_back({s.nextId++,content[i].id,0,0,0});
    Enemy mite; mite.id=s.nextId++; mite.definition="C1-R01"; mite.name="Rivet Mite"; mite.hp=mite.maxHp=7; mite.pattern={{Move::Attack,5,1}}; mite.intent=mite.pattern[0];
    Enemy ram; ram.id=s.nextId++; ram.definition="C1-R02"; ram.name="Breach Ram"; ram.hp=ram.maxHp=24; ram.armor=3; ram.pattern={{Move::Charge,0,0},{Move::Attack,18,1}}; ram.intent=ram.pattern[0];
    s.enemies={mite,ram}; return s;
}
Amount Rules::shield(const State& s,bool installedOnly) {
    Amount total=0; for(const auto& p:s.parts) if(p.place==Place::Installed) total=add(total,p.shield);
    if(!installedOnly) for(const auto& p:s.protection) total=add(total,p.amount);
    return total;
}
Amount Rules::spendShield(State& s,Amount amount,bool installedOnly) {
    require(amount>=0,"Negative Shield payment.");
    struct Entry { Id order,id; bool part; }; std::vector<Entry> entries;
    for(const auto& p:s.parts) if(p.place==Place::Installed) entries.push_back({p.installOrder,p.id,true});
    if(!installedOnly) for(const auto& p:s.protection) entries.push_back({p.order,p.id,false});
    std::sort(entries.begin(),entries.end(),[](const Entry& a,const Entry& b){ return a.order<b.order; });
    Amount left=amount;
    for(const auto& e:entries) {
        Amount& value=e.part?byId(s.parts,e.id)->shield:byId(s.protection,e.id)->amount;
        const Amount take=std::min(left,value); value-=take; left-=take;
        if(left==0) break;
    }
    return amount-left;
}
Result Rules::apply(State& state,const Action& action) const {
    State candidate=state;
    try { auto result=execute(candidate,action); if(result.ok) state=std::move(candidate); return result; }
    catch(const Invalid& e) { return {false,e.what(),{}}; }
}
Preview Rules::preview(const State& state,const Action& action) const { Preview p; p.state=state; p.result=apply(p.state,action); return p; }
Result Rules::execute(State& s,const Action& a) const {
    require(s.rulesVersion==RulesVersion && s.contentVersion==ContentVersion,"Incompatible rules or content version.");
    require(s.phase==Phase::Collection || s.phase==Phase::Preparation,"This fight is finished.");
    Engine e{s,*this,{},0}; e.root=e.emit("action",0,0,static_cast<Amount>(a.type));
    if(a.type==ActionType::Collect) {
        require(s.phase==Phase::Collection,"This round has already been collected.");
        require(a.steering>=0 && a.steering<5,"Choose one of the five materials.");
        require(a.precision>=-1 && a.precision<=2,"Invalid Precision result.");
        require(a.precision<0 || !s.precisionSpent,"Precision has already been used this fight.");
        Materials haul{3,2,1,1,1}; haul[static_cast<std::size_t>(a.steering)]+=2;
        const Amount haulBonus=s.haulBonus;
        if(a.precision>=0) { s.precisionSpent=true; haul[static_cast<std::size_t>(a.steering)]+=a.precision; }
        const std::array<std::size_t,8> mixBag{{0,0,0,1,1,2,3,4}};
        for(Amount i=0;i<s.haulBonus;++i) ++haul[mixBag[s.rng.below(Domain::Collection,8)]];
        s.haulBonus=0; for(std::size_t i=0;i<5;++i) s.materials[i]=add(s.materials[i],haul[i]);
        s.parts.erase(std::remove_if(s.parts.begin(),s.parts.end(),[&](const Part& p){return p.kind==Kind::Magnet && p.createdRound<s.round;}),s.parts.end());
        s.phase=Phase::Preparation; e.emit("collected",0,0,10+std::max(0,a.precision)+haulBonus);
    } else {
        require(s.phase==Phase::Preparation,"Collect this round's materials first.");
        switch(a.type) {
        case ActionType::Craft: e.craft(a.subject); break;
        case ActionType::Install: e.install(a.subject); break;
        case ActionType::Remove: {
            auto* p=byId(s.parts,a.subject); require(p && p->place==Place::Installed,"Select an installed part.");
            p->place=Place::Reserve; e.emit("part_removed",p->id,0,p->shield); break;
        }
        case ActionType::Load: {
            require(s.bullet.empty(),"Unload the current bullet before changing it.");
            require(!a.parts.empty(),"A bullet needs at least one part."); std::set<Id> seen;
            for(Id id:a.parts) { auto* p=byId(s.parts,id); require(p && p->place==Place::Reserve && (p->kind==Kind::Ammo || p->kind==Kind::Spread),"Select reserved bullet parts."); require(seen.insert(id).second,"A physical part can only be loaded once."); p->place=Place::Loaded; }
            s.bullet=a.parts; e.emit("loaded",0,0,checked(static_cast<std::int64_t>(a.parts.size()))); break;
        }
        case ActionType::Unload: e.unload(); break;
        case ActionType::Fire: e.fire(a); break;
        case ActionType::EndTurn: e.endTurn(); break;
        case ActionType::Activate: {
            auto* p=byId(s.parts,a.subject); require(p && p->place==Place::Reserve && (p->kind==Kind::Magnet || p->kind==Kind::Modifier),"Select a reserved planning part.");
            require(p->kind!=Kind::Magnet || p->createdRound==s.round,"Magnet parts must be fitted in their crafting round.");
            const auto part=*p; e.pay(part.effects,Timing::Activate);
            s.parts.erase(std::remove_if(s.parts.begin(),s.parts.end(),[&](const Part& x){return x.id==a.subject;}),s.parts.end());
            e.effects(part.effects,Timing::Activate,a.target,s.heat,part.recipe); e.emit("part_used",part.id); break;
        }
        default: throw Invalid("Unsupported action.");
        }
    }
    return {true,{},std::move(e.events)};
}
std::vector<Action> Rules::legalActions(const State& s) const {
    std::vector<Action> candidates,legal;
    if(s.phase==Phase::Collection) { for(Amount i=0;i<5;++i)candidates.push_back(Action::collect(i)); }
    if(s.phase==Phase::Preparation) {
        for(const auto& c:s.memory)candidates.push_back(Action::craft(c.id));
        for(const auto& p:s.parts) {
            if(p.kind==Kind::Shield) candidates.push_back(p.place==Place::Installed?Action::remove(p.id):Action::install(p.id));
            if(p.kind==Kind::Ammo || p.kind==Kind::Spread) candidates.push_back(Action::load({p.id}));
            if(p.kind==Kind::Magnet || p.kind==Kind::Modifier) {Action a;a.type=ActionType::Activate;a.subject=p.id;candidates.push_back(a);}
        }
        for(const auto& enemy:s.enemies)candidates.push_back(Action::fire(enemy.id));
        Action unload;unload.type=ActionType::Unload;candidates.push_back(unload);candidates.push_back(Action::endTurn());
    }
    // Convenience suggestions, not an exhaustive combinatorial list. apply/preview validates any assembly.
    for(const auto& a:candidates)if(preview(s,a).result.ok)legal.push_back(a);
    return legal;
}
std::string Rules::intentText(const Enemy& e,Amount round) {
    std::string result;
    if(e.intent.move==Move::Attack)result="Attack "+std::to_string(std::max(0,e.intent.damage+e.drive-e.weaken))+(e.intent.hits>1?" x "+std::to_string(e.intent.hits):"");
    else if(e.intent.move==Move::Charge)result="Charge — Blast 18 next turn";
    else if(e.intent.move==Move::Escape)result="Escape";
    else result="Recover";
    if(e.departureRound>round)result+=" | Escape in "+std::to_string(e.departureRound-round);
    return result;
}
std::string eventJson(const Event& e) {
    std::ostringstream o; o<<"{\"id\":"<<e.id<<",\"parent\":"<<e.parent<<",\"type\":\""<<e.type<<"\",\"subject\":"<<e.subject<<",\"target\":"<<e.target<<",\"amount\":"<<e.amount<<",\"secondary\":"<<e.secondary<<"}";return o.str();
}
Result playTeachingFixture(State& s,const Rules& rules) {
    Result total{true,{}, {}};
    const auto submit=[&](Action a){auto r=rules.apply(s,a);if(!r.ok)throw Invalid(r.reason);total.events.insert(total.events.end(),r.events.begin(),r.events.end());};
    const auto craft=[&](const std::string& name){for(const auto& copy:s.memory)if(copy.recipe==name){submit(Action::craft(copy.id));return;}throw Invalid("Fixture recipe missing.");};
    const auto find=[&](const std::string& name){for(const auto& p:s.parts)if(p.recipe==name && p.place==Place::Reserve)return p.id;throw Invalid("Fixture part missing.");};
    const auto five=[&](){for(const auto* id:{"SH001","SH002","SH003","SH004","SH006"})craft(id);};
    try {
        require(s.enemies.size()==2,"Teaching fixture requires its original pair.");const auto mite=s.enemies[0].id,ram=s.enemies[1].id;
        submit(Action::collect(3));five();submit(Action::load({find("SH004")}));submit(Action::fire(mite));submit(Action::load({find("SH001")}));submit(Action::fire(ram));submit(Action::endTurn());
        submit(Action::collect(3));five();std::vector<Id> shot,shield;
        for(const auto& p:s.parts){if(p.kind==Kind::Ammo)shot.push_back(p.id);if(p.kind==Kind::Shield)shield.push_back(p.id);}
        submit(Action::load(shot));submit(Action::fire(ram));for(Id id:shield)submit(Action::install(id));submit(Action::endTurn());
        submit(Action::collect(3));craft("SH001");craft("SH004");submit(Action::load({find("SH001"),find("SH004")}));submit(Action::fire(ram));
        require(s.phase==Phase::Victory && s.hp==80,"Teaching route did not reproduce the authored outcome.");
    } catch(const Invalid& error){total.ok=false;total.reason=error.what();}
    return total;
}
} // namespace overkill

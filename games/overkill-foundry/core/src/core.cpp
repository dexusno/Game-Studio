#include "overkill/core.hpp"
#include "overkill/robots.hpp"
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <iomanip>
#include <limits>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace overkill {
namespace {
struct Invalid : std::runtime_error { using std::runtime_error::runtime_error; };
struct FinalDeath {};
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
    case Op::Catalogue: return (e.amount>=1&&e.amount<=126)||(e.amount>=1001&&e.amount<=1120);
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
    struct ScopedSource { std::string& current;std::string previous;ScopedSource(std::string& value,const std::string& source):current(value),previous(value){current=source;}~ScopedSource(){current=previous;} };
#include "recipe_effects.inl"
#include "upgrade_effects.inl"
    Id producingCopy=0;
    std::string producingRecipe;
    Amount useCoolingRemoved=0;
    bool useReadied=false;
    Amount attackBudget = -1, attackFlatRemaining = 0;
    bool inEnemyAttack = false;
    Amount attackLost = 0, attackAbsorbed = 0;
    RobotCallbacks callbacks() {
        return { [this](const std::string& type,Id subject,Id target,Amount amount,Amount secondary,Id parent){return emit(type,subject,target,amount,secondary,parent);},
            [this](Amount amount,Id source,Id parent,bool bypass){playerDamage(amount,source,parent,bypass,inEnemyAttack,true);},
            [this](RobotStatus kind,Amount amount,Id source,Id parent){
                if(auto* b=binding("SH064",BindingClock::Round); b && b->amount>0 && kind<=RobotStatus::Weaken){b->amount=0;emit("status_prevented",source,0,amount,static_cast<Amount>(kind),parent);return;}
                const Amount before=kind==RobotStatus::Burn?s.burn:kind==RobotStatus::Corrosion?s.corrosion:kind==RobotStatus::Mark?s.mark:kind==RobotStatus::Weaken?s.weaken:kind==RobotStatus::RecipeFouling?s.recipeFouling:s.shieldLeak;
                switch(kind){case RobotStatus::Burn:s.burn=add(s.burn,amount);break;case RobotStatus::Corrosion:s.corrosion=add(s.corrosion,amount);break;case RobotStatus::Mark:s.mark=add(s.mark,amount);break;case RobotStatus::Weaken:s.weaken=add(s.weaken,amount);break;case RobotStatus::RecipeFouling:s.recipeFouling=std::min(2,add(s.recipeFouling,amount));s.foulingRound=s.round+1;break;case RobotStatus::ShieldLeak:s.shieldLeak=add(s.shieldLeak,amount);s.shieldLeakRound=s.round+1;break;}
                emit("player_status",source,0,amount,static_cast<Amount>(kind),parent);if(before==0&&amount>0)enemyDebuffUpgrade(kind); } };
    }
    Id emit(const std::string& type, Id subject = 0, Id target = 0, Amount amount = 0, Amount secondary = 0, Id parent = 0) {
        const Id id = s.nextEvent++;
        events.push_back({id, parent ? parent : root, subject, target, type, amount, secondary,upgradeSource,s.round,{}});
        return id;
    }
    void heat(Amount n) { const auto old = s.heat; s.heat = std::min(upgradeHeatCap(s), add(s.heat, n)); emit("heat", 0, 0, s.heat-old); }
    void terminal() {
        if (s.hp <= 0) { s.phase = Phase::Defeat; emit("defeat"); return; }
        if (std::none_of(s.enemies.begin(), s.enemies.end(), alive)) {
            s.phase = s.kills > 0 ? Phase::Victory : Phase::Escaped;
            emit(s.kills > 0 ? "victory" : "escape_complete");
        }
    }
    void playerDamage(Amount amount,Id source,Id parent,bool bypass=false,bool attackHit=false,bool enemyCause=false) {
        if(attackHit)beforeEnemyHit(amount);
        if(attackHit&&!bypass){if(s.mark>0){amount=add(amount,s.mark);s.mark=0;}const auto reduced=std::min(amount,attackFlatRemaining);amount-=reduced;attackFlatRemaining-=reduced;}
        const Amount absorbed=bypass?0:Rules::spendShield(s,amount);Amount lost=amount-absorbed;if(lost>0)lost=hpPrevention(lost);lost=std::min(s.hp,lost);s.hp-=lost;s.hpLostRound=add(s.hpLostRound,lost);
        if(attackHit){attackLost=add(attackLost,lost);attackAbsorbed=add(attackAbsorbed,absorbed);}
        const Id hit=emit("player_damage",source,0,lost,absorbed,parent);
        if(s.hp==0&&s.reserveHeartPump){s.reserveHeartPump=false;s.hp=std::max(1,s.maxHp/4);emit("reserve_heart_pump",0,0,s.hp,0,hit);}
        if(s.hp==0){s.phase=Phase::Defeat;emit("defeat",0,0,0,0,hit);}
        playerLostUpgrades(source,lost,absorbed,attackHit,enemyCause||source!=0);
    }
    void enemyDamage(Id target,Amount amount,bool direct,bool bypass=false,Id source=0,Amount shieldBypass=0,Amount armorBypass=0) {
        auto* e=byId(s.enemies,target);if(!e||!alive(*e)){emit("hit_lost",source,target);return;}
        const auto armor=std::max(0,e->armor-armorBypass);const Amount reducedByArmor=direct&&!bypass?std::min(amount,armor):0;
        Amount damage=amount-reducedByArmor;if(direct&&e->tiles>0&&damage>0){--e->tiles;damage=std::min(damage,1);}
        const Amount redirected=bypass?damage:std::min(damage,shieldBypass);const Amount shieldLoss=std::min(e->shield,damage-redirected);e->shield-=shieldLoss;
        const Amount hpLoss=std::min(e->hp,damage-shieldLoss);e->hp-=hpLoss;e->hpLostFight=add(e->hpLostFight,hpLoss);const Amount recoil=direct&&shieldLoss+hpLoss>0?e->mesh:0;
        const Id hit=emit(direct?"hit":"status_damage",source,target,hpLoss,shieldLoss);
        if(hpLoss>0)onRobotHpLoss(s,target,callbacks());e=byId(s.enemies,target);
        if(e&&e->hp==0){e->dead=true;++s.kills;emit("enemy_death",source,target,0,0,hit);onRobotDeath(s,target,callbacks());}
        if(recoil>0){const auto response=emit("recoil",target,0,recoil,0,hit);playerDamage(recoil,target,response);}
        if(s.phase!=Phase::Defeat&&direct&&hpLoss>0)if(auto* b=binding("MA100",BindingClock::Round,target);b&&b->count<4){++b->count;status(target,3,2);}
        e=byId(s.enemies,target);if(direct)enemyHitUpgrades(target,reducedByArmor,e&&e->dead);
    }
    void pay(const std::vector<Effect>& list,Timing timing,Part* part=nullptr) {
        Amount hp=0,heatCost=0;for(const auto& f:list)if(f.timing==timing){if(f.op==Op::HpCost)hp=add(hp,f.amount);if(f.op==Op::HeatCost)heatCost=add(heatCost,f.amount);}
        payment(hp,heatCost,0,timing!=Timing::Use,part);
    }
    Id shieldPart(Amount value,const std::string& source,bool installed) {if(value==0)return 0;return receive(plain(Kind::Shield,value,source),installed);}
    void schedule(DeliveryKind kind, Amount amount, const std::string& source) {
        if(kind==DeliveryKind::Haul) {
            auto& boost=bind(source,BindingClock::Collection,amount);boost.round=s.round+1;
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
                    if (before != copy.cooldown) {emit("cooled", copy.id, 0, before-copy.cooldown);if(inRecipeUse){useCoolingRemoved=add(useCoolingRemoved,before-copy.cooldown);if(copy.cooldown==0)useReadied=true;}}
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
                emit("heal", 0, 0, restored);recipeHealed(restored); break;
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
    void install(Id id,const Action& action=Action{}) {
        auto* pointer=byId(s.parts,id);require(pointer&&pointer->kind==Kind::Shield&&pointer->place==Place::Reserve,"Select a reserved Shield part.");
        Part part=*pointer;ScopedSource sourceScope(upgradeSource,part.creator.empty()?part.recipe:part.creator);
        if(!part.everInstalled){
            pay(part.effects,Timing::Install,&part);Amount value=0;
            if(catalogue(part.effects))value=catalogueShield(part,action);else for(const auto& f:part.effects)if(f.op==Op::ShieldValue&&f.timing==Timing::Install)value=add(value,f.amount);
            if(auto* b=binding("SH035",BindingClock::Round)){value=add(value,b->amount);b->amount=0;}
            value=add(value,part.upgradeShield);part.shield=value;part.originalValue=value;part.everInstalled=true;part.firstInstallRound=s.round;part.bindingOrder=s.nextOrder++;part.place=Place::Installed;part.installOrder=s.nextOrder++;++s.firstInstallsRound;
            *byId(s.parts,id)=part;emit("first_install",id,0,value);effects(part.effects,Timing::Install,action.target,s.heat,part.recipe);
            if(catalogue(part.effects))catalogueInstalled(part,action);
            for(const auto& b:part.attachments)if(b.heat&&b.dueRound==s.round)heat(b.heat);
        }else{pointer->place=Place::Installed;pointer->installOrder=s.nextOrder++;emit("reinstall",id,0,pointer->shield);}
    }
    Part recipePart(const Recipe& recipe,const Action& a) {
        Part p;p.recipe=recipe.id;p.output=recipe.output;p.resaleReference=recipe.id;p.kind=recipe.kind;p.effects=recipe.effects;p.rarity=recipe.rarity;p.materialBasis=recipe.cost;p.choices=a.choices;p.creator=recipe.id;p.canonicalRecipe=recipe.id;
        if(recipe.id=="MA038"){p.resaleReference="warm-rivet";p.materialBasis={1,0,1,0,0};}
        return p;
    }
    void craft(const Action& a) {
        auto* copy=byId(s.memory,a.subject);require(copy,"This recipe is not in memory.");const auto* recipe=rules.recipe(copy->recipe);require(recipe,"This recipe has no verified runtime implementation.");
        require(copy->cooldown==0,"This recipe is cooling.");require(recipe->cooldown>0||copy->usedRound!=s.round||extraRecipeUse(*copy,*recipe),"This copy has already been used this round.");craftOptions(*recipe,a);if(recipe->kind==Kind::Ammo&&binding("SH103",BindingClock::Round))require(a.amount>=0&&a.amount<recipe->outputCount,"Choose one crafted Ammo output for Fine Mould.");
        ScopedSource sourceScope(upgradeSource,recipe->id);Materials cost=discountedCost(*copy,*recipe,a);Binding* saver=binding("SH070",BindingClock::Round);if(saver&&saver->amount>0&&cost[0]>=2){--cost[0];saver->amount=0;}
        if(recipe->kind==Kind::Ammo&&s.recipeFouling>0&&s.foulingRound==s.round)cost[0]=add(cost[0],1);
        for(std::size_t i=0;i<5;++i)require(s.materials[i]>=cost[i],"Not enough materials.");inRecipeUse=true;producingCopy=copy->id;producingRecipe=recipe->id;useCoolingRemoved=0;useReadied=false;pay(recipe->effects,Timing::Use);
        for(std::size_t i=0;i<5;++i){s.materials[i]-=cost[i];s.spentThisRound[i]=add(s.spentThisRound[i],cost[i]);}
        if(recipe->kind==Kind::Ammo&&s.recipeFouling>0&&s.foulingRound==s.round)--s.recipeFouling;
        copy=byId(s.memory,a.subject);copy->cooldown=assignedCooldown(*copy,recipe->cooldown);copy->usedRound=s.round;++copy->usesThisRound;emit("recipe_used",a.subject);events.back().source=recipe->id;events.back().paid=cost;const auto usedCopy=*copy;const Id firstOutput=s.nextId;
        if(auto* b=binding("MA069",BindingClock::Round))if(std::find(b->seen.begin(),b->seen.end(),recipe->id)==b->seen.end())b->seen.push_back(recipe->id);
        if(recipe->kind!=Kind::Utility&&!recipe->automaticOutput){for(Amount i=0;i<recipe->outputCount;++i){const Id id=receive(recipePart(*recipe,a));if(recipe->kind==Kind::Ammo)if(auto* b=binding("SH103",BindingClock::Round);b&&b->amount>0&&i==a.amount){attach(*byId(s.parts,id),"SH103",b->amount,0,0);b->amount=0;}}}
        effects(recipe->effects,Timing::Use,a.target,s.heat,recipe->id);
        if(catalogue(recipe->effects)&&(recipe->kind==Kind::Utility||recipe->automaticOutput))catalogueUtility(*recipe,a);
        inRecipeUse=false;producingCopy=0;producingRecipe.clear();if(s.phase==Phase::Defeat)return;
        std::vector<Id> outputs;for(const auto& p:s.parts)if(p.id>=firstOutput&&p.sourceRecipeCopy==a.subject&&p.origin==PartOrigin::Produced)outputs.push_back(p.id);
        producedUpgrades(usedCopy,*recipe,cost,outputs);resolvedUseUpgrades(usedCopy,*recipe,cost,useCoolingRemoved,useReadied);terminal();
    }
    void unload() {
        for (Id id : s.bullet) if (auto* p = byId(s.parts,id)) p->place = Place::Reserve;
        for(auto& p:s.parts)if(p.place==Place::Payment){p.place=Place::Reserve;p.reservedBy=0;}
        s.bullet.clear(); emit("unload");
    }
    void fire(const Action& action) {completeFire(action);}
    void commitIntents() {
        for(Id id:living()) {if(commitRobotIntent(s,id,callbacks()))continue;auto* e=byId(s.enemies,id);
            if(e->departureRound>0&&s.round>=e->departureRound)e->intent={Move::Escape,0,0};
            else if(!e->pattern.empty())e->intent=e->pattern[static_cast<std::size_t>(e->patternCursor)%e->pattern.size()];
            emit("intent",e->id,0,static_cast<Amount>(e->intent.move),e->intent.damage);
        }
    }
    void endTurn(const Action& choice) {
        if(!keepLoadedUpgrade())unload();endTurnHooks(choice);finishPaidUpgrades();terminal();if(s.phase==Phase::Victory||s.phase==Phase::Defeat||s.phase==Phase::Escaped)return;s.bindings.erase(std::remove_if(s.bindings.begin(),s.bindings.end(),[](const Binding& b){return b.clock==BindingClock::Shot;}),s.bindings.end());
        s.nextFlat=s.nextPercent=0;s.shotBonuses.erase(std::remove_if(s.shotBonuses.begin(),s.shotBonuses.end(),[](const ShotBonus& b){return !b.fightLifetime;}),s.shotBonuses.end());emit("end_turn",0,0,s.round);
        if(s.shieldLeak>0&&s.shieldLeakRound==s.round){const Amount lost=Rules::spendShield(s,s.shieldLeak,true);emit("shield_leak",0,0,lost);s.shieldLeak=0;}
        if(s.recipeFouling>0&&s.foulingRound==s.round)s.recipeFouling=0;
        inEnemyPhase=true;if(s.burn>0){const bool ward=!s.burnWardSpent&&std::any_of(s.parts.begin(),s.parts.end(),[&](const Part& p){return p.place==Place::Installed&&p.firstInstallRound==s.round&&std::any_of(p.effects.begin(),p.effects.end(),[](const Effect& f){return f.op==Op::BurnWard;});});const auto tick=s.burn--;if(!ward)playerDamage(tick,0,root,false,false,true);else{s.burnWardSpent=true;emit("burn_prevented",0,0,tick);}}
        if(s.phase==Phase::Defeat)return;s.weaken=std::max(0,s.weaken-1);
        const auto actors=living();Amount shelter=0;
        for(Id id:actors){auto* e=byId(s.enemies,id);if(!e||!alive(*e)||e->bornRound>=s.round)continue;
            if(e->corrosion>0){const auto tick=e->corrosion--;enemyDamage(id,tick,false,true);}e=byId(s.enemies,id);if(!e||!alive(*e))continue;
            const auto intent=e->intent;const auto shieldBefore=Rules::shield(s);const bool burned=e->burn>0;
            if(intent.move==Move::Attack){const Amount total=checked(static_cast<std::int64_t>(std::max(0,add(intent.damage,e->drive)-e->weaken))*intent.hits+s.mark);const auto reduced=attackReduction(id,total);attackFlatRemaining=total-reduced;attackLost=attackAbsorbed=0;inEnemyAttack=true;}
            if(!executeRobotIntent(s,id,callbacks())){const auto event=emit("enemy_action",id,0,static_cast<Amount>(intent.move));if(intent.move==Move::Escape){byId(s.enemies,id)->escaped=true;emit("enemy_escape",id);}else if(intent.move==Move::Attack){e=byId(s.enemies,id);const auto damage=std::max(0,add(intent.damage,e->drive)-e->weaken);for(Amount hit=0;hit<intent.hits&&s.phase!=Phase::Defeat;++hit){e=byId(s.enemies,id);if(!e||!alive(*e))break;playerDamage(damage,id,event,false,true,true);}}++byId(s.enemies,id)->patternCursor;}
            inEnemyAttack=false;if(s.phase==Phase::Defeat)return;if(intent.move==Move::Attack)afterEnemyAttack(id,attackLost,attackAbsorbed,shieldBefore,burned);if(s.phase==Phase::Defeat)return;
            e=byId(s.enemies,id);if(e&&alive(*e)&&e->burn>0){const auto tick=e->burn;if(e->burnHoldTicks>0)--e->burnHoldTicks;else --e->burn;enemyDamage(id,tick,false);if(active("MA090")&&shelter<6){shieldPart(2,"MA090",true);shelter+=2;}}
            finishRobotTurn(s,id,callbacks());
        }
        terminal();if(s.phase==Phase::Victory||s.phase==Phase::Escaped||s.phase==Phase::Defeat)return;
        preReset();terminal();if(s.phase==Phase::Victory||s.phase==Phase::Defeat)return;
        bool skipHeat=false;for(const auto& p:hooks())if(code(p.recipe)==1015)skipHeat=true;
        Amount restoreHeat=0;for(const auto& b:s.bindings)if(b.source=="MA102"&&b.clock==BindingClock::Round&&installedBinding(b))restoreHeat=std::max(restoreHeat,b.amount);
        Amount available=Rules::shield(s),retained=0;std::vector<std::pair<std::string,Amount>> allowances;for(Amount n:s.retentionAllowances)allowances.push_back({{},n});for(const auto& id:upgradeOrder()){if(id=="MY3-03")allowances.push_back({id,6});if(id=="UGS-123")allowances.push_back({id,3});}for(const auto& allowance:allowances){ScopedSource sourceScope(upgradeSource,allowance.first);const auto actual=std::min(available,allowance.second);available-=actual;retained=add(retained,actual);emit("shield_retained",0,0,actual);}
        s.parts.erase(std::remove_if(s.parts.begin(),s.parts.end(),[](const Part& p){return p.place==Place::Installed||p.place==Place::Fitted;}),s.parts.end());s.protection.clear();if(retained)s.protection.push_back({s.nextId++,s.nextOrder++,retained});
        for(auto& e:s.enemies)if(alive(e))e.weaken=std::max(0,e.weaken-1);
        for(auto& c:s.memory){if(c.usedRound!=s.round)c.cooldown=std::max(0,c.cooldown-1);c.usesThisRound=0;}
        if(!skipHeat)s.heat=std::max(0,s.heat-upgradeHeatDecay(s));s.heat=std::max(s.heat,restoreHeat);s.burnWardSpent=false;
        previousHeatPaid=s.partHeatPaidRound;s.partHeatPaidRound=0;s.previousEnemyAttackHp=s.enemyAttackHpRound;s.enemyAttackHpRound=0;s.hpLostRound=0;s.heatPaidRound=0;s.sacrificesRound=0;s.firstInstallsRound=0;s.spentThisRound={};++s.round;
        clearUpgradeScope(UpgradeScope::Round);inEnemyPhase=false;if(s.corrosion>0){const auto tick=s.corrosion--;playerDamage(tick,0,root,true,false,true);}if(s.phase==Phase::Defeat)return;
        s.bindings.erase(std::remove_if(s.bindings.begin(),s.bindings.end(),[&](const Binding& b){return b.clock==BindingClock::Round||b.clock==BindingClock::EndPhase||(b.clock==BindingClock::Collection&&b.round<s.round);}),s.bindings.end());
        auto deliveries=std::move(s.deliveries);s.deliveries.clear();const Amount beforeStartHeat=s.heat;
        for(const auto& d:deliveries){if(d.dueRound>s.round){s.deliveries.push_back(d);continue;}emit("delivery",d.id,0,d.amount);if(d.kind==DeliveryKind::ShieldPart)shieldPart(d.amount,d.source,true);if(d.kind==DeliveryKind::Heat)heat(d.amount);if(d.kind==DeliveryKind::Haul&&!binding(d.source,BindingClock::Collection))s.haulBonus=add(s.haulBonus,d.amount);extendedDelivery(d,beforeStartHeat);if(s.phase==Phase::Defeat)return;}
        if(active("MA056"))heat(1);terminal();if(s.phase==Phase::Victory||s.phase==Phase::Defeat)return;s.phase=Phase::Collection;UpgradeEvent turn;turn.kind=UpgradeEventKind::TurnStart;queueUpgradeEvent(turn,UpgradeContinuation::Collection);drainUpgrades();
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
            if(e.op==Op::Catalogue)require(e.amount==r.catalogueCode&&e.amount==Engine::code(r.id),"Catalogue effect identity mismatch.");
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
    catch(const std::exception& e) { return {false,e.what(),{}}; }
}
Result Rules::grantPart(State& state,const std::string& id,bool installed) const {
    State candidate=state;try{const auto* r=recipe(id);require(r&&r->kind!=Kind::Utility&&!r->automaticOutput,"Recipe has no directly grantable physical output.");Engine e{candidate,*this,{},0};e.root=e.emit("part_grant");e.receive(e.recipePart(*r,Action{}),installed);state=std::move(candidate);return {true,{},std::move(e.events)};}catch(const Invalid& error){return {false,error.what(),{}};}
}
Result Rules::grantPlainPart(State& state,Kind kind,Amount value,const std::string& source,bool installed) const {
    State candidate=state;try{require((kind==Kind::Ammo||kind==Kind::Shield)&&value>=0,"Plain grant needs Ammo or Shield and nonnegative value.");Engine e{candidate,*this,{},0};e.root=e.emit("part_grant");e.receive(e.plain(kind,value,source),installed);state=std::move(candidate);return {true,{},std::move(e.events)};}catch(const Invalid& error){return {false,error.what(),{}};}
}
Preview Rules::preview(const State& state,const Action& action) const { Preview p; p.state=state; p.result=apply(p.state,action); return p; }
Result Rules::acquireUpgrade(State& state,const std::string& id) const {
    State candidate=state;try{require(upgradeEligible(*this,candidate,id),"This upgrade is unavailable or its required choices cannot resolve.");Engine e{candidate,*this,{},0};e.upgradeSource=id;e.root=e.emit("upgrade_acquired");OwnedUpgrade u;u.id=id;u.order=candidate.nextOrder++;candidate.upgrades.push_back(u);UpgradeEvent event;event.kind=UpgradeEventKind::Acquire;event.snapshotListeners=true;event.listeners={id};e.queueUpgradeEvent(event);e.drainUpgrades();state=std::move(candidate);return {true,{},std::move(e.events)};}catch(const std::exception& error){return {false,error.what(),{}};}
}
Result Rules::startUpgrades(State& state,EncounterClass encounter) const {
    State candidate=state;try{require(candidate.upgradeChoices.empty()&&candidate.upgradeRequests.empty()&&candidate.upgradeResolution.empty(),"Resolve pending upgrade decisions before fight entry.");Engine e{candidate,*this,{},0};e.root=e.emit("upgrade_fight_start");candidate.encounterClass=encounter;candidate.fightSerial=add(candidate.fightSerial,1);candidate.round=1;candidate.shots=candidate.kills=0;candidate.precisionSpent=false;candidate.precisionGoodFight=candidate.precisionFailedFight=0;candidate.phase=Phase::Collection;e.clearUpgradeScope(UpgradeScope::Fight);e.clearUpgradeScope(UpgradeScope::Round);for(auto& u:candidate.upgrades){u.fightCount=u.roundCount=0;if(u.id=="UGS-029")u.seen.clear();}UpgradeEvent event;event.kind=UpgradeEventKind::FightStart;e.queueUpgradeEvent(event,UpgradeContinuation::FirstTurn);e.drainUpgrades();state=std::move(candidate);return {true,{},std::move(e.events)};}catch(const std::exception& error){return {false,error.what(),{}};}
}
Result Rules::upgradeEvent(State& state,const UpgradeEvent& event) const {
    State candidate=state;try{Engine e{candidate,*this,{},0};e.root=e.emit("upgrade_campaign_event",event.subject,0,static_cast<Amount>(event.kind));if(event.kind==UpgradeEventKind::CityStart)e.clearUpgradeScope(UpgradeScope::City);e.queueUpgradeEvent(event);e.drainUpgrades();state=std::move(candidate);return {true,{},std::move(e.events)};}catch(const std::exception& error){return {false,error.what(),{}};}
}
Result Rules::resumeUpgrades(State& state) const {State candidate=state;try{Engine e{candidate,*this,{},0};e.root=e.emit("upgrade_resume");e.drainUpgrades();state=std::move(candidate);return {true,{},std::move(e.events)};}catch(const std::exception& error){return {false,error.what(),{}};}}
Result Rules::execute(State& s,const Action& a) const {
    require(s.rulesVersion==RulesVersion && s.contentVersion==ContentVersion,"Incompatible rules or content version.");
    Engine e{s,*this,{},0}; e.root=e.emit("action",0,0,static_cast<Amount>(a.type));
    try {
    if(a.type==ActionType::ResolveUpgradeChoice){e.answerUpgrade(a.upgradeChoice);return {true,{},std::move(e.events)};}
    require(s.phase==Phase::Collection || s.phase==Phase::Preparation,"This fight is finished.");
    require(s.upgradeChoices.empty()&&s.upgradeRequests.empty()&&s.upgradeResolution.empty(),"Finish the pending upgrade resolution first.");
    if(a.type==ActionType::Collect) {
        require(s.phase==Phase::Collection,"This round has already been collected.");
        require(a.steering>=0 && a.steering<5,"Choose one of the five materials.");
        require(a.precision>=-1 && a.precision<=2,"Invalid Precision result.");
        require(a.precision<0 || !s.precisionSpent,"Precision has already been used this fight.");
        Materials base=e.upgradeBaseHaul();Materials haul=base;haul[static_cast<std::size_t>(a.steering)]+=2;
        if(a.precision>=0)s.precisionSpent=true;
        e.collectionEffects(haul,a,&base); for(std::size_t i=0;i<5;++i) s.materials[i]=add(s.materials[i],haul[i]);
        s.parts.erase(std::remove_if(s.parts.begin(),s.parts.end(),[&](const Part& p){return p.kind==Kind::Magnet && p.createdRound<s.round;}),s.parts.end());
        s.phase=Phase::Preparation;Amount gathered=0;for(Amount n:haul)gathered=add(gathered,n);e.emit("collected",0,0,gathered);
        if(a.precision>0)++s.precisionGoodFight;if(a.precision==0)++s.precisionFailedFight;
        UpgradeEvent collected;collected.kind=UpgradeEventKind::Collect;collected.steering=a.steering;collected.precision=a.precision;collected.baseHaul=base;e.queueUpgradeEvent(collected,UpgradeContinuation::Preparation);e.drainUpgrades();
    } else {
        require(s.phase==Phase::Preparation,"Collect this round's materials first.");
        switch(a.type) {
        case ActionType::Craft: e.craft(a); break;
        case ActionType::Install: e.install(a.subject,a); break;
        case ActionType::Remove: {
            auto* p=byId(s.parts,a.subject); require(p && p->place==Place::Installed,"Select an installed part.");
            p->place=Place::Reserve; e.emit("part_removed",p->id,0,p->shield); break;
        }
        case ActionType::Load: {
            require(s.bullet.empty(),"Unload the current bullet before changing it.");
            require(!a.parts.empty(),"A bullet needs at least one part."); std::set<Id> seen;
            for(Id id:a.parts) { auto* p=byId(s.parts,id); require(p && p->place==Place::Reserve && (p->kind==Kind::Ammo || p->kind==Kind::Spread),"Select reserved bullet parts."); require(seen.insert(id).second,"A physical part can only be loaded once."); p->place=Place::Loaded; }
            std::size_t paymentIndex=0;
            for(Id id:a.parts){const auto* p=byId(s.parts,id);if(p->kind==Kind::Spread)require(!e.active("MA067",BindingClock::Shot),"Barrel Weight cannot combine with spreading parts.");if(Engine::code(p->recipe)==110){require(paymentIndex<a.sacrifices.size(),"Reserve one unused Shield part for each Full-Spread Outlet.");auto* paid=byId(s.parts,a.sacrifices[paymentIndex++]);require(paid&&paid->place==Place::Reserve&&Engine::unused(*paid)&&paid->kind==Kind::Shield,"Choose an unused Shield payment.");paid->place=Place::Payment;paid->reservedBy=id;}}
            require(paymentIndex==a.sacrifices.size(),"Unused sacrifice selection.");s.bullet=a.parts; e.emit("loaded",0,0,checked(static_cast<std::int64_t>(a.parts.size()))); break;
        }
        case ActionType::Unload: e.unload(); break;
        case ActionType::Fire: e.fire(a); break;
        case ActionType::EndTurn: e.endTurn(a); break;
        case ActionType::Activate: {
            auto* p=byId(s.parts,a.subject); require(p && p->place==Place::Reserve && (p->kind==Kind::Magnet || p->kind==Kind::Modifier),"Select a reserved planning part.");
            require(p->kind!=Kind::Magnet || p->createdRound==s.round,"Magnet parts must be fitted in their crafting round.");
            auto part=*p;Engine::ScopedSource sourceScope(e.upgradeSource,part.creator.empty()?part.recipe:part.creator);e.pay(part.effects,Timing::Activate,&part);
            s.parts.erase(std::remove_if(s.parts.begin(),s.parts.end(),[&](const Part& x){return x.id==a.subject;}),s.parts.end());
            e.effects(part.effects,Timing::Activate,a.target,s.heat,part.recipe);if(Engine::catalogue(part.effects))e.catalogueActivate(part,a);for(const auto& b:part.attachments)if(b.heat&&b.dueRound==s.round)e.heat(b.heat); e.emit("part_used",part.id); break;
        }
        case ActionType::ActivateUpgrade:e.equipmentAction(a);break;
        default: throw Invalid("Unsupported action.");
        }
    }
    e.finishPaidUpgrades();
    } catch(const FinalDeath&) {}
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
    const auto robotText=robotIntentText(e,round);if(!robotText.empty())return robotText;
    if(e.intent.move==Move::Attack)result="Attack "+std::to_string(std::max(0,e.intent.damage+e.drive-e.weaken))+(e.intent.hits>1?" x "+std::to_string(e.intent.hits):"");
    else if(e.intent.move==Move::Charge)result="Charge — Blast 18 next turn";
    else if(e.intent.move==Move::Escape)result="Escape";
    else result="Recover";
    if(e.departureRound>round)result+=" | Escape in "+std::to_string(e.departureRound-round);
    return result;
}
std::string eventJson(const Event& e) {
    const auto quoted=[](const std::string& value){std::ostringstream out;out<<'"';for(unsigned char c:value){if(c=='"'||c=='\\')out<<'\\'<<c;else if(c<32)out<<"\\u"<<std::hex<<std::setw(4)<<std::setfill('0')<<static_cast<unsigned>(c);else out<<c;}out<<'"';return out.str();};
    std::ostringstream o;o<<"{\"id\":"<<e.id<<",\"parent\":"<<e.parent<<",\"type\":"<<quoted(e.type)<<",\"subject\":"<<e.subject<<",\"target\":"<<e.target<<",\"amount\":"<<e.amount<<",\"secondary\":"<<e.secondary<<",\"source\":"<<quoted(e.source)<<",\"round\":"<<e.round<<",\"paid\":[";for(std::size_t i=0;i<e.paid.size();++i){if(i)o<<',';o<<e.paid[i];}o<<"]}";return o.str();
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

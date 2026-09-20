#include "overkill/core.hpp"
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace overkill {
namespace {
std::uint64_t checksum(const std::string& s) {
    std::uint64_t h=14695981039346656037ULL;
    for(unsigned char c:s){h^=c;h*=1099511628211ULL;}return h;
}
struct Writer {
    static constexpr bool reading=false;
    std::string data;
    template<class T> void number(T value) {
        using U=std::make_unsigned_t<T>; U u=static_cast<U>(value);
        for(std::size_t i=0;i<sizeof(T);++i)data.push_back(static_cast<char>((u>>(8*i))&255U));
    }
    void raw(const char* bytes,std::size_t n){data.append(bytes,n);}
};
struct Reader {
    static constexpr bool reading=true;
    const std::string& data; std::size_t position=0;
    void need(std::size_t n){if(n>data.size()-position)throw std::runtime_error("Truncated snapshot.");}
    template<class T> void number(T& value) {
        need(sizeof(T)); std::uint64_t u=0;
        for(std::size_t i=0;i<sizeof(T);++i)u|=static_cast<std::uint64_t>(static_cast<unsigned char>(data[position++]))<<(8*i);
        value=static_cast<T>(u);
    }
    void raw(char* bytes,std::size_t n){need(n);std::memcpy(bytes,data.data()+position,n);position+=n;}
};
template<class A,class T,std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T,bool>,int> =0>
void field(A& a,T& x){a.number(x);}
template<class A> void field(A& a,bool& value){std::uint8_t x=value?1:0;a.number(x);if constexpr(A::reading){if(x>1)throw std::runtime_error("Invalid boolean.");value=x!=0;}}
template<class A,class T,std::enable_if_t<std::is_enum_v<T>,int> =0>
void field(A& a,T& x){auto n=static_cast<std::underlying_type_t<T>>(x);a.number(n);if constexpr(A::reading)x=static_cast<T>(n);}
template<class A> void field(A& a,std::string& value){
    if(value.size()>std::numeric_limits<std::uint32_t>::max())throw std::runtime_error("Snapshot string too large.");
    std::uint32_t n=static_cast<std::uint32_t>(value.size());a.number(n);
    if constexpr(A::reading){a.need(n);value.resize(n);a.raw(value.data(),n);}else a.raw(value.data(),n);
}
template<class A> void field(A&,Effect&);
template<class A> void field(A&,RecipeCopy&);
template<class A> void field(A&,Part&);
template<class A> void field(A&,Protection&);
template<class A> void field(A&,Intent&);
template<class A> void field(A&,Enemy&);
template<class A> void field(A&,Delivery&);
template<class A> void field(A&,ShotBonus&);
template<class A> void field(A&,Attachment&);
template<class A> void field(A&,Binding&);
template<class A> void field(A&,RecipeTag&);template<class A> void field(A&,UpgradeCounter&);template<class A> void field(A&,OwnedUpgrade&);
template<class A> void field(A&,UpgradeEvent&);template<class A> void field(A&,UpgradeChoice&);template<class A> void field(A&,UpgradeRequest&);template<class A> void field(A&,UpgradeResolution&);
template<class A,class T> void field(A& a,std::vector<T>& value){
    if(value.size()>std::numeric_limits<std::uint32_t>::max())throw std::runtime_error("Snapshot vector too large.");
    std::uint32_t n=static_cast<std::uint32_t>(value.size());a.number(n);
    if constexpr(A::reading){a.need(n);value.resize(n);} // Each supported entry consumes at least one byte.
    for(auto& x:value)field(a,x);
}
template<class A,class T,std::size_t N> void field(A& a,std::array<T,N>& value){for(auto& x:value)field(a,x);}
template<class A,class... T> void fields(A& a,T&... values){(field(a,values),...);}
template<class A> void field(A& a,Effect& x){fields(a,x.op,x.timing,x.amount,x.threshold);}
template<class A> void field(A& a,RecipeTag& x){fields(a,x.source,x.order,x.kind,x.amount,x.material,x.usedFight);}
template<class A> void field(A& a,RecipeCopy& x){fields(a,x.id,x.recipe,x.cooldown,x.usedRound,x.usesThisRound,x.storage,x.borrowedFrom,x.tags);}
template<class A> void field(A& a,Part& x){fields(a,x.id,x.recipe,x.output,x.resaleReference,x.kind,x.place,x.createdRound,x.originalValue,x.shield,x.firstInstallRound,x.paidHeat,x.paidHp,x.bindingOrder,x.installOrder,x.everInstalled,x.effects,x.rarity,x.materialBasis,x.choices,x.attachments,x.reservedBy,x.hookUses,x.sourceRecipeCopy,x.origin,x.creator,x.canonicalRecipe,x.upgradeDamage,x.upgradeShield);}
template<class A> void field(A& a,Protection& x){fields(a,x.id,x.order,x.amount);}
template<class A> void field(A& a,Intent& x){fields(a,x.move,x.damage,x.hits);}
template<class A> void field(A& a,Enemy& x){fields(a,x.id,x.definition,x.name,x.hp,x.maxHp,x.shield,x.armor,x.mesh,x.tiles,x.burn,x.corrosion,x.mark,x.weaken,x.drive,x.bornRound,x.departureRound,x.patternCursor,x.dead,x.escaped,x.intent,x.pattern,x.burnHoldTicks,x.hpLostFight,x.robotAction,x.patternRandom,x.packetVariant,x.summonsRemaining,x.temporaryArmor,x.armorExpiresRound,x.coreValue,x.actionCompletedRound,x.intentRound,x.turnCompletedRound,x.recoveryPending,x.binderVariants,x.bossTransitioned,x.deathReleased,x.coreRecorded);}
template<class A> void field(A& a,Delivery& x){fields(a,x.id,x.source,x.dueRound,x.amount,x.kind,x.target,x.detail,x.materials,x.parts);}
template<class A> void field(A& a,ShotBonus& x){fields(a,x.source,x.flat,x.percent,x.fightLifetime);}
template<class A> void field(A& a,Attachment& x){fields(a,x.source,x.damage,x.heat,x.dueRound);}
template<class A> void field(A& a,Binding& x){fields(a,x.id,x.order,x.target,x.part,x.source,x.clock,x.round,x.amount,x.count,x.choices,x.seen);}
template<class A> void field(A& a,UpgradeCounter& x){fields(a,x.key,x.scope,x.value);}
template<class A> void field(A& a,OwnedUpgrade& x){fields(a,x.id,x.order,x.recipeCopy,x.charges,x.campaignCount,x.fightCount,x.roundCount,x.values,x.recipes,x.target,x.materials,x.counters,x.seen);}
template<class A> void field(A& a,UpgradeEvent& x){fields(a,x.kind,x.origin,x.product,x.subject,x.reward,x.definition,x.source,x.baseValue,x.actualValue,x.steering,x.precision,x.baseHaul,x.normalOffer,x.lightTouch,x.snapshotListeners,x.listeners);}
template<class A> void field(A& a,UpgradeChoice& x){fields(a,x.id,x.source,x.kind,x.minimum,x.maximum,x.objects,x.values,x.options,x.baseHaul,x.optional);}
template<class A> void field(A& a,UpgradeRequest& x){fields(a,x.id,x.source,x.kind,x.offers,x.count,x.rarity,x.recipeKind,x.filterKind,x.sharedOnly,x.optional,x.normalOffer,x.recipeCopy,x.reward,x.definition);}
template<class A> void field(A& a,UpgradeResolution& x){fields(a,x.event,x.sources,x.cursor,x.after);}
template<class A> void visit(A& a,State& x){
    fields(a,x.rulesVersion,x.contentVersion,x.encounter,x.seed,x.rng.state,x.nextId,x.nextOrder,x.nextEvent,x.phase,x.round,x.hp,x.maxHp,x.heat,x.credits,x.shots,x.kills,x.burn,x.corrosion,x.weaken,x.nextFlat,x.nextPercent,x.haulBonus,x.quickPatchHealing,x.hotBarrel,x.precisionSpent,x.burnWardSpent,x.reserveHeartPump,x.materials,x.memory,x.parts,x.protection,x.enemies,x.deliveries,x.shotBonuses,x.bullet,x.retentionAllowances,x.bindings,x.spentThisRound,x.pile,x.finitePile,x.partHpPaidFight,x.partHeatPaidRound,x.hpPaidFight,x.heatPaidRound,x.sacrificesRound,x.firstInstallsRound,x.recastConsumed,x.hpLostRound,x.enemyAttackHpRound,x.previousEnemyAttackHp,x.mark,x.carefulHealing,x.recipeFouling,x.foulingRound,x.shieldLeak,x.shieldLeakRound);
    fields(a,x.upgrades,x.encounterClass,x.fightSerial,x.upgradeResolution,x.upgradeChoices,x.upgradeRequests,x.precisionGoodFight,x.precisionFailedFight);
}
void validate(const State& s){
    auto valid=[](bool b,const char* message){if(!b)throw std::runtime_error(message);};
    valid(s.rulesVersion==RulesVersion && s.contentVersion==ContentVersion,"Incompatible rules or content version; migration required.");
    valid(s.phase<=Phase::Escaped && s.round>=1 && s.hp>=0 && s.hp<=s.maxHp && s.maxHp>=1 && s.heat>=0 && s.heat<=upgradeHeatCap(s),"Invalid player state.");
    valid(s.credits>=0 && s.shots>=0 && s.kills>=0 && s.burn>=0 && s.corrosion>=0 && s.weaken>=0 && s.nextFlat>=0 && s.nextPercent>=0 && s.haulBonus>=0 && s.quickPatchHealing>=0,"Negative state quantity.");
    for(auto n:s.materials)valid(n>=0,"Negative inventory.");
    for(auto n:s.spentThisRound)valid(n>=0,"Negative material-use ledger.");
    for(auto n:s.pile)valid(n>=0,"Negative pile stock.");
    valid(s.partHpPaidFight>=0&&s.partHeatPaidRound>=0&&s.hpPaidFight>=s.partHpPaidFight&&s.heatPaidRound>=s.partHeatPaidRound&&s.sacrificesRound>=0&&s.firstInstallsRound>=0&&s.recastConsumed>=0&&s.hpLostRound>=0&&s.enemyAttackHpRound>=0&&s.previousEnemyAttackHp>=0&&s.mark>=0&&s.carefulHealing>=0&&s.carefulHealing<=16,"Invalid recipe counters.");
    valid(s.recipeFouling>=0&&s.recipeFouling<=2&&s.foulingRound>=0&&s.shieldLeak>=0&&s.shieldLeakRound>=0,"Invalid robot penalty.");
    const auto partData=[&](const Part& p){
        valid(p.rarity<=Rarity::Legendary&&p.hookUses>=0&&p.reservedBy<s.nextId,"Invalid physical-part metadata.");
        valid(p.origin<=PartOrigin::Copied&&p.sourceRecipeCopy<s.nextId&&p.upgradeDamage>=0&&p.upgradeShield>=0,"Invalid part provenance or upgrade value.");
        for(auto n:p.materialBasis)valid(n>=0,"Negative canonical part basis.");
        for(auto n:p.choices)valid(n>=0&&n<=4,"Invalid stored material choice.");
        for(const auto& b:p.attachments)valid(!b.source.empty()&&b.damage>=0&&b.heat>=0&&b.dueRound>=0,"Invalid part attachment.");
        for(const auto& e:p.effects){valid(e.op<=Op::Catalogue&&e.timing<=Timing::Activate&&e.amount>=0&&e.threshold>=0,"Unsupported or invalid part effect.");if(e.op==Op::Catalogue)valid((e.amount>=1&&e.amount<=126)||(e.amount>=1001&&e.amount<=1120),"Invalid catalogue recipe code.");}
        if(p.kind==Kind::Magnet){std::size_t n=0;if(p.recipe=="SH080"||p.recipe=="SH119"||p.recipe=="SH122")n=2;if(p.recipe=="SH106"||p.recipe=="SH108"||p.recipe=="SH121")n=1;valid(p.choices.size()==n,"Missing stored gathering choices.");}
    };
    std::set<Id> identities,loaded;
    const auto identity=[&](Id id){valid(id>0 && id<s.nextId && identities.insert(id).second,"Invalid or duplicate instance identity.");};
    for(const auto& c:s.memory){identity(c.id);valid(!c.recipe.empty() && c.cooldown>=0 && c.usedRound>=0 && c.usesThisRound>=0&&c.storage<=MemoryKind::Borrowed,"Invalid recipe-copy state.");valid((c.storage==MemoryKind::Borrowed)==!c.borrowedFrom.empty(),"Invalid borrowed copy attribution.");for(const auto& t:c.tags)valid(!t.source.empty()&&t.order>0&&t.order<s.nextOrder&&t.kind<=RecipeTagKind::Cooldown&&t.amount>=0&&t.material>=0&&t.material<5&&t.usedFight>=0&&t.usedFight<=std::max(1,s.fightSerial),"Invalid recipe-copy upgrade tag.");}
    for(const auto& p:s.parts){
        identity(p.id);partData(p);valid(p.kind<=Kind::Modifier && p.place<=Place::Payment && p.createdRound>=1 && p.shield>=0 && p.originalValue>=0 && p.paidHp>=0 && p.paidHeat>=0,"Invalid part state.");
        valid(p.place!=Place::Installed || (p.kind==Kind::Shield && p.everInstalled && p.installOrder>0),"Invalid installed part.");
        for(const auto& e:p.effects)valid(e.op<=Op::Catalogue && e.timing<=Timing::Activate && e.amount>=0 && e.threshold>=0,"Unsupported or invalid part effect.");
        valid(p.place!=Place::Payment||(p.kind==Kind::Shield&&p.reservedBy>0),"Invalid reserved part payment.");
        if(p.place==Place::Loaded)loaded.insert(p.id);
    }
    std::set<Id> bullet;
    for(Id id:s.bullet)valid(loaded.count(id)>0 && bullet.insert(id).second,"Invalid bullet reservation.");
    valid(loaded==bullet,"Loaded inventory does not match bullet.");
    for(const auto& p:s.parts)if(p.place==Place::Payment)valid(bullet.count(p.reservedBy)>0,"Payment does not belong to a loaded part.");
    for(const auto& p:s.protection){identity(p.id);valid(p.amount>=0 && p.order>0,"Invalid protection.");}
    for(const auto& e:s.enemies){
        identity(e.id);valid(e.hp>=0 && e.hp<=e.maxHp && e.maxHp>0 && e.armor>=0 && e.shield>=0 && e.mesh>=0 && e.tiles>=0 && e.burn>=0 && e.corrosion>=0 && e.mark>=0 && e.weaken>=0 && e.drive>=0 && e.patternCursor>=0,"Invalid enemy state.");
        valid(e.burnHoldTicks>=0&&e.burnHoldTicks<=2&&e.hpLostFight>=0&&e.packetVariant>=0&&e.packetVariant<=1&&e.summonsRemaining>=0&&e.temporaryArmor>=0&&e.temporaryArmor<=e.armor&&e.armorExpiresRound>=0&&e.coreValue>=0,"Invalid robot counters.");
        valid(e.intentRound>=0&&e.intentRound<=s.round&&e.actionCompletedRound>=0&&e.actionCompletedRound<=s.round&&e.turnCompletedRound>=0&&e.turnCompletedRound<=s.round,"Invalid robot clocks.");
        valid(!e.recoveryPending||e.bossTransitioned,"Unexpected boss recovery.");
        valid(e.dead==(e.hp==0),"Enemy death flag does not match HP.");
        valid(e.intent.move<=Move::Brace && e.intent.damage>=0 && e.intent.hits>=0,"Invalid committed intent.");
        for(const auto& i:e.pattern)valid(i.move<=Move::Brace && i.damage>=0 && i.hits>=0,"Invalid enemy pattern.");
    }
    for(const auto& d:s.deliveries){identity(d.id);valid(d.dueRound>0 && d.amount>=0 && d.kind<=DeliveryKind::Custom&&d.target<s.nextId&&d.detail>=0,"Invalid delivery.");for(auto n:d.materials)valid(n>=0,"Negative delivery materials.");for(const auto& p:d.parts)partData(p);}
    for(const auto& b:s.bindings){identity(b.id);valid(b.clock<=BindingClock::EndPhase&&b.round>=0&&b.amount>=0&&b.count>=0&&b.order>0&&b.order<s.nextOrder&&b.part<s.nextId&&b.target<s.nextId,"Invalid effect binding.");for(auto n:b.choices)valid(n>=0&&n<=4,"Invalid effect choice.");if(b.clock==BindingClock::Collection){std::size_t n=0;if(b.source=="SH080"||b.source=="SH119"||b.source=="SH122")n=2;if(b.source=="SH106"||b.source=="SH108"||b.source=="SH121")n=1;valid(b.choices.size()==n,"Missing collection choices.");}}
    for(Amount n:s.retentionAllowances)valid(n>=0,"Negative retention allowance.");
    std::set<std::string> bonusSources;for(const auto& b:s.shotBonuses)valid(!b.source.empty() && bonusSources.insert(b.source).second && b.flat>=0 && b.percent>=0,"Invalid next-shot bonus.");
    valid(s.encounterClass<=EncounterClass::Boss&&s.fightSerial>=0&&s.precisionGoodFight>=0&&s.precisionFailedFight>=0,"Invalid upgrade fight clock.");
    std::set<std::string> upgrades;std::set<Id> orders;const auto eligible=eligibleUpgradeIds();
    for(const auto& u:s.upgrades){valid(std::binary_search(eligible.begin(),eligible.end(),u.id)&&upgrades.insert(u.id).second&&u.order>0&&u.order<s.nextOrder&&orders.insert(u.order).second,"Invalid upgrade identity/order.");valid(u.charges>=0&&u.campaignCount>=0&&u.fightCount>=0&&u.roundCount>=0&&u.target<s.nextId&&u.recipeCopy<s.nextId,"Invalid upgrade state.");for(Id id:u.recipes)valid(id>0&&id<s.nextId,"Invalid selected recipe identity.");for(Amount n:u.materials)valid(n>=0,"Negative upgrade shipment.");std::set<std::string> keys;for(const auto& c:u.counters)valid(!c.key.empty()&&keys.insert(c.key).second&&c.scope<=UpgradeScope::Round&&c.value>=0,"Invalid scoped upgrade counter.");}
    for(const auto& c:s.upgradeChoices){identity(c.id);valid(upgrades.count(c.source)>0&&c.kind<=UpgradeChoiceKind::PrecisionRetry&&c.minimum>=0&&c.maximum>=c.minimum,"Invalid saved upgrade choice.");for(Id id:c.objects)valid(id>0&&id<s.nextId,"Invalid choice object.");for(Amount n:c.baseHaul)valid(n>=0,"Negative saved haul.");}
    for(const auto& r:s.upgradeRequests){identity(r.id);valid(upgrades.count(r.source)>0&&r.kind<=UpgradeRequestKind::RevealReward&&r.offers>0&&r.count>0&&r.rarity>=-1&&r.rarity<=4&&r.recipeKind<=Kind::Modifier&&r.recipeCopy<s.nextId,"Invalid campaign upgrade request.");}
    for(const auto& r:s.upgradeResolution){valid(r.cursor>=0&&static_cast<std::size_t>(r.cursor)<=r.sources.size()&&r.after<=UpgradeContinuation::Preparation&&r.event.kind<=UpgradeEventKind::CityStart&&r.event.origin<=AcquisitionSource::Borrowed&&r.event.product<=PurchaseKind::Part,"Invalid upgrade continuation.");for(const auto& id:r.sources)valid(upgrades.count(id)>0,"Continuation lost its upgrade listener.");}
}
} // namespace
std::string serialize(const State& state){
    State copy=state;Writer body;visit(body,copy);
    Writer envelope;envelope.raw("OFSTATE1",8);envelope.number(std::uint32_t{4});
    envelope.number(static_cast<std::uint64_t>(body.data.size()));envelope.number(checksum(body.data));envelope.data+=body.data;return envelope.data;
}
bool deserialize(const std::string& bytes,State& state,std::string& error){
    try{
        Reader r{bytes};char magic[8];r.raw(magic,8);
        if(std::string(magic,8)!="OFSTATE1")throw std::runtime_error("Not an Overkill Foundry snapshot.");
        std::uint32_t version=0;std::uint64_t length=0,hash=0;r.number(version);r.number(length);r.number(hash);
        if(version!=4)throw std::runtime_error("Unsupported snapshot schema.");
        if(length!=bytes.size()-r.position)throw std::runtime_error("Snapshot length mismatch.");
        const auto body=bytes.substr(r.position);if(checksum(body)!=hash)throw std::runtime_error("Snapshot integrity check failed.");
        State candidate;Reader data{body};visit(data,candidate);
        if(data.position!=body.size())throw std::runtime_error("Unexpected trailing snapshot data.");
        validate(candidate);state=std::move(candidate);error.clear();return true;
    }catch(const std::exception& e){error=e.what();return false;}
}
std::string stateHash(const State& state){std::ostringstream out;out<<std::hex<<std::setfill('0')<<std::setw(16)<<checksum(serialize(state));return out.str();}
} // namespace overkill

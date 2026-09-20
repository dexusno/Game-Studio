#include "overkill/core.hpp"
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
template<class A,class T> void field(A& a,std::vector<T>& value){
    if(value.size()>std::numeric_limits<std::uint32_t>::max())throw std::runtime_error("Snapshot vector too large.");
    std::uint32_t n=static_cast<std::uint32_t>(value.size());a.number(n);
    if constexpr(A::reading){a.need(n);value.resize(n);} // Each supported entry consumes at least one byte.
    for(auto& x:value)field(a,x);
}
template<class A,class T,std::size_t N> void field(A& a,std::array<T,N>& value){for(auto& x:value)field(a,x);}
template<class A,class... T> void fields(A& a,T&... values){(field(a,values),...);}
template<class A> void field(A& a,Effect& x){fields(a,x.op,x.timing,x.amount,x.threshold);}
template<class A> void field(A& a,RecipeCopy& x){fields(a,x.id,x.recipe,x.cooldown,x.usedRound,x.usesThisRound);}
template<class A> void field(A& a,Part& x){fields(a,x.id,x.recipe,x.output,x.resaleReference,x.kind,x.place,x.createdRound,x.originalValue,x.shield,x.firstInstallRound,x.paidHeat,x.paidHp,x.bindingOrder,x.installOrder,x.everInstalled,x.effects);}
template<class A> void field(A& a,Protection& x){fields(a,x.id,x.order,x.amount);}
template<class A> void field(A& a,Intent& x){fields(a,x.move,x.damage,x.hits);}
template<class A> void field(A& a,Enemy& x){fields(a,x.id,x.definition,x.name,x.hp,x.maxHp,x.shield,x.armor,x.mesh,x.tiles,x.burn,x.corrosion,x.mark,x.weaken,x.drive,x.bornRound,x.departureRound,x.patternCursor,x.dead,x.escaped,x.intent,x.pattern);}
template<class A> void field(A& a,Delivery& x){fields(a,x.id,x.source,x.dueRound,x.amount,x.kind);}
template<class A> void field(A& a,ShotBonus& x){fields(a,x.source,x.flat,x.percent);}
template<class A> void visit(A& a,State& x){
    fields(a,x.rulesVersion,x.contentVersion,x.encounter,x.seed,x.rng.state,x.nextId,x.nextOrder,x.nextEvent,x.phase,x.round,x.hp,x.maxHp,x.heat,x.credits,x.shots,x.kills,x.burn,x.corrosion,x.weaken,x.nextFlat,x.nextPercent,x.haulBonus,x.quickPatchHealing,x.hotBarrel,x.precisionSpent,x.burnWardSpent,x.reserveHeartPump,x.materials,x.memory,x.parts,x.protection,x.enemies,x.deliveries,x.shotBonuses,x.bullet,x.retentionAllowances);
}
void validate(const State& s){
    auto valid=[](bool b,const char* message){if(!b)throw std::runtime_error(message);};
    valid(s.rulesVersion==RulesVersion && s.contentVersion==ContentVersion,"Incompatible rules or content version; migration required.");
    valid(s.phase<=Phase::Escaped && s.round>=1 && s.hp>=0 && s.hp<=s.maxHp && s.maxHp>=1 && s.heat>=0 && s.heat<=10,"Invalid player state.");
    valid(s.credits>=0 && s.shots>=0 && s.kills>=0 && s.burn>=0 && s.corrosion>=0 && s.weaken>=0 && s.nextFlat>=0 && s.nextPercent>=0 && s.haulBonus>=0 && s.quickPatchHealing>=0,"Negative state quantity.");
    for(auto n:s.materials)valid(n>=0,"Negative inventory.");
    std::set<Id> identities,loaded;
    const auto identity=[&](Id id){valid(id>0 && id<s.nextId && identities.insert(id).second,"Invalid or duplicate instance identity.");};
    for(const auto& c:s.memory){identity(c.id);valid(!c.recipe.empty() && c.cooldown>=0 && c.usedRound>=0 && c.usesThisRound>=0,"Invalid recipe-copy state.");}
    for(const auto& p:s.parts){
        identity(p.id);valid(p.kind<=Kind::Modifier && p.place<=Place::Fitted && p.createdRound>=1 && p.shield>=0 && p.originalValue>=0 && p.paidHp>=0 && p.paidHeat>=0,"Invalid part state.");
        valid(p.place!=Place::Installed || (p.kind==Kind::Shield && p.everInstalled && p.installOrder>0),"Invalid installed part.");
        for(const auto& e:p.effects)valid(e.op<=Op::BurnWard && e.timing<=Timing::Activate && e.amount>=0 && e.threshold>=0,"Unsupported or invalid part effect.");
        if(p.place==Place::Loaded)loaded.insert(p.id);
    }
    std::set<Id> bullet;
    for(Id id:s.bullet)valid(loaded.count(id)>0 && bullet.insert(id).second,"Invalid bullet reservation.");
    valid(loaded==bullet,"Loaded inventory does not match bullet.");
    for(const auto& p:s.protection){identity(p.id);valid(p.amount>=0 && p.order>0,"Invalid protection.");}
    for(const auto& e:s.enemies){
        identity(e.id);valid(e.hp>=0 && e.hp<=e.maxHp && e.maxHp>0 && e.armor>=0 && e.shield>=0 && e.mesh>=0 && e.tiles>=0 && e.burn>=0 && e.corrosion>=0 && e.mark>=0 && e.weaken>=0 && e.drive>=0 && e.patternCursor>=0,"Invalid enemy state.");
        valid(e.dead==(e.hp==0),"Enemy death flag does not match HP.");
        valid(e.intent.move<=Move::Escape && e.intent.damage>=0 && e.intent.hits>=0,"Invalid committed intent.");
        for(const auto& i:e.pattern)valid(i.move<=Move::Escape && i.damage>=0 && i.hits>=0,"Invalid enemy pattern.");
    }
    for(const auto& d:s.deliveries){identity(d.id);valid(d.dueRound>0 && d.amount>=0 && d.kind<=DeliveryKind::Haul,"Invalid delivery.");}
    for(Amount n:s.retentionAllowances)valid(n>=0,"Negative retention allowance.");
    std::set<std::string> bonusSources;for(const auto& b:s.shotBonuses)valid(!b.source.empty() && bonusSources.insert(b.source).second && b.flat>=0 && b.percent>=0,"Invalid next-shot bonus.");
}
} // namespace
std::string serialize(const State& state){
    State copy=state;Writer body;visit(body,copy);
    Writer envelope;envelope.raw("OFSTATE1",8);envelope.number(std::uint32_t{2});
    envelope.number(static_cast<std::uint64_t>(body.data.size()));envelope.number(checksum(body.data));envelope.data+=body.data;return envelope.data;
}
bool deserialize(const std::string& bytes,State& state,std::string& error){
    try{
        Reader r{bytes};char magic[8];r.raw(magic,8);
        if(std::string(magic,8)!="OFSTATE1")throw std::runtime_error("Not an Overkill Foundry snapshot.");
        std::uint32_t version=0;std::uint64_t length=0,hash=0;r.number(version);r.number(length);r.number(hash);
        if(version!=2)throw std::runtime_error("Unsupported snapshot schema.");
        if(length!=bytes.size()-r.position)throw std::runtime_error("Snapshot length mismatch.");
        const auto body=bytes.substr(r.position);if(checksum(body)!=hash)throw std::runtime_error("Snapshot integrity check failed.");
        State candidate;Reader data{body};visit(data,candidate);
        if(data.position!=body.size())throw std::runtime_error("Unexpected trailing snapshot data.");
        validate(candidate);state=std::move(candidate);error.clear();return true;
    }catch(const std::exception& e){error=e.what();return false;}
}
std::string stateHash(const State& state){std::ostringstream out;out<<std::hex<<std::setfill('0')<<std::setw(16)<<checksum(serialize(state));return out.str();}
} // namespace overkill

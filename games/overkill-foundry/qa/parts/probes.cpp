#include "overkill/campaign.hpp"
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

// Independent source-derived counterexamples. No author matrix/oracle includes.
// The abundant stock and synthetic recoverers are controlled test inputs.
using namespace overkill;
namespace {
int assertions=0, passed=0, failed=0;
const Rules rules;
void expect(bool condition,const std::string& why){++assertions;if(!condition)throw std::runtime_error(why);}
Amount total(const Materials& m){return std::accumulate(m.begin(),m.end(),Amount{0});}
Materials minus(const Materials& a,const Materials& b){Materials out{};for(std::size_t i=0;i<5;++i)out[i]=a[i]-b[i];return out;}
std::string events(const Result& r){std::string out;for(const auto& e:r.events)out+=eventJson(e)+"\n";return out;}
State arena(){
    State s;s.rng=Rng::seeded(720,"qa-parts");s.encounter="qa-parts";s.phase=Phase::Preparation;
    s.hp=s.maxHp=80;s.hotBarrel=false;s.heat=10;s.materials={80,80,80,80,80};
    for(int i=0;i<3;++i){Enemy e;e.id=s.nextId++;e.definition="QA";e.name="Recoverer";e.hp=e.maxHp=300;e.intent={Move::Recover,0,0};e.pattern={e.intent};s.enemies.push_back(e);}
    return s;
}
Result apply(State& s,const Action& a){auto r=rules.apply(s,a);expect(r.ok,r.reason);return r;}
void reject(State& s,const Action& a){auto bytes=serialize(s);auto r=rules.apply(s,a);expect(!r.ok,"Action unexpectedly accepted");expect(r.events.empty()&&serialize(s)==bytes,"Rejected transaction was not atomic");}
void roundTrip(State& s){auto bytes=serialize(s);State copy;std::string error;expect(deserialize(bytes,copy,error),error);expect(serialize(copy)==bytes,"Snapshot round trip changed state");s=std::move(copy);}
Result previewApply(State& s,const Action& a){auto bytes=serialize(s);auto preview=rules.preview(s,a);expect(preview.result.ok,preview.result.reason);expect(serialize(s)==bytes,"Preview mutated authoritative state");State restored;std::string error;expect(deserialize(bytes,restored,error),error);auto replay=rules.apply(restored,a);auto result=apply(s,a);expect(serialize(preview.state)==serialize(s)&&serialize(restored)==serialize(s),"Preview/reload state mismatch");expect(events(result)==events(preview.result)&&events(result)==events(replay),"Preview/reload event mismatch");return result;}
Id remember(State& s,const std::string& recipe){RecipeCopy c;c.id=s.nextId++;c.recipe=recipe;s.memory.push_back(c);return c.id;}
std::vector<Id> craft(State& s,const std::string& recipe,Action a={}){const Id copy=remember(s,recipe);a.type=ActionType::Craft;a.subject=copy;apply(s,a);std::vector<Id> ids;for(const auto& p:s.parts)if(p.sourceRecipeCopy==copy)ids.push_back(p.id);return ids;}
Id craftOne(State& s,const std::string& recipe,Action a={}){auto ids=craft(s,recipe,a);expect(ids.size()==1,"Expected one paid physical output: "+recipe);return ids.front();}
const Part& part(const State& s,Id id){for(const auto& p:s.parts)if(p.id==id)return p;throw std::runtime_error("Missing part "+std::to_string(id));}
bool exists(const State& s,Id id){return std::any_of(s.parts.begin(),s.parts.end(),[id](const Part& p){return p.id==id;});}
Id plain(State& s,Kind kind,Amount value,bool installed=false){const Id start=s.nextId;auto r=rules.grantPlainPart(s,kind,value,"qa-controlled",installed);expect(r.ok,r.reason);expect(exists(s,start),"Controlled source not created");return start;}
void activate(State& s,Id id,Action a={}){a.type=ActionType::Activate;a.subject=id;apply(s,a);}
void next(State& s){apply(s,Action::endTurn());expect(s.phase==Phase::Collection,"Unexpected controlled terminal outcome");apply(s,Action::collect(0));}
std::vector<Id> createdBy(const State& s,const std::string& creator){std::vector<Id> ids;for(const auto& p:s.parts)if(p.creator==creator)ids.push_back(p.id);return ids;}
Amount fire(State& s,std::vector<Id> ids,Action a={}){apply(s,Action::load(std::move(ids)));a.type=ActionType::Fire;a.target=s.enemies.front().id;auto hp=s.enemies.front().hp;previewApply(s,a);return hp-s.enemies.front().hp;}
void upgrade(State& s,const std::string& id){auto r=rules.acquireUpgrade(s,id);expect(r.ok,r.reason);}
void run(const std::string& name,const std::function<void()>& fn){try{fn();++passed;std::cout<<"PASS "<<name<<"\n";}catch(const std::exception& e){++failed;std::cout<<"FAIL "<<name<<": "<<e.what()<<"\n";}}

void paidRivets(){
    auto s=arena();s.heat=7;auto before=s.materials;auto ids=craft(s,"MA038");expect(ids.size()==2&&ids[0]!=ids[1],"Paid bundle must make two different objects");
    expect(minus(before,s.materials)==Materials({2,0,1,0,0}),"Bundle did not pay 2 Iron and 1 Carbon");
    const auto original=part(s,ids[0]);expect(original.sourceRecipeCopy==part(s,ids[1]).sourceRecipeCopy&&original.origin==PartOrigin::Produced,"Batch source-copy provenance");
    for(Id id:ids)expect(upgradePartSaleValue(s,part(s,id),{3,1,2,4,5})==2,"Each Warm Rivet floors (3+2)/2 independently");
    Action attached;attached.parts={ids[0]};activate(s,craftOne(s,"SH036"),attached);
    activate(s,craftOne(s,"SH072"));Action shot;shot.partChoices={{0,ids[0]}};
    expect(fire(s,{ids[0]},shot)==8,"Fuse first shot should deal 4+4");expect(exists(s,ids[1])&&!exists(s,ids[0]),"One fired rivet consumed its sibling");
    expect(s.heat==8,"First Warm Rivet should grant one Heat below the ten-Heat cap");roundTrip(s);next(s);
    auto copies=createdBy(s,"SH072");expect(copies.size()==1,"Consumed-source return should deliver exactly one copy");auto copy=part(s,copies[0]);
    expect(copy.recipe=="MA038"&&copy.resaleReference=="warm-rivet"&&copy.materialBasis==Materials({1,0,1,0,0}),"Return changed immutable output identity/value");
    expect(copy.createdRound==2&&part(s,ids[1]).createdRound==1&&copy.sourceRecipeCopy==original.sourceRecipeCopy&&copy.attachments.empty(),"Return should refresh age, strip Fuse, preserve paid source");
    expect(fire(s,{copies[0]})==4,"Return copied the generator or attached Fuse");next(s);expect(createdBy(s,"SH072").empty(),"Return rearmed itself");
}

void paidShieldHistory(){
    for(Amount pay:{3,8}){
        auto s=arena();auto id=craftOne(s,"MA096");auto install=Action::install(id);install.amount=pay;s.heat=pay-1;reject(s,install);s.heat=pay;
        previewApply(s,install);expect(s.heat==0&&part(s,id).shield==6+3*pay&&part(s,id).paidHeat==pay,"First-install chosen Heat/value");auto order=part(s,id).bindingOrder;
        Rules::spendShield(s,5,true);apply(s,Action::remove(id));expect(!isUnusedPart(part(s,id)),"Paid/depleted Shield became unused");
        auto copy=Action::craft(remember(s,"SH118"));copy.parts={id};reject(s,copy);roundTrip(s);next(s);
        expect(exists(s,id)&&part(s,id).createdRound==1,"Saved paid Shield disappeared or reset age");
        auto before=s.heat;install.amount=0;apply(s,install);expect(s.heat==before&&part(s,id).shield==1+3*pay&&part(s,id).bindingOrder==order&&part(s,id).firstInstallRound==1,"Reinstall repaid/refilled/rebound saved Shield");
    }
    auto s=arena();auto id=craftOne(s,"MA054");s.hp=3;reject(s,Action::install(id));s.hp=4;previewApply(s,Action::install(id));expect(s.hp==1&&part(s,id).paidHp==3&&part(s,id).shield==18,"Own HP payment must leave exactly one at boundary");
    apply(s,Action::remove(id));apply(s,Action::install(id));expect(s.hp==1&&part(s,id).paidHp==3,"Reinstallation repeated own HP payment");
}

void generatedRecast(){
    for(const auto& item:std::vector<std::pair<std::string,Amount>>{{"SH058",9},{"SH113",14}}){
        auto s=arena();upgrade(s,"UGS-140");auto source=craftOne(s,item.first);expect(part(s,source).upgradeShield==4,"Controlled paid Shield did not receive Laminator");
        Action conversion;conversion.parts={source};auto slug=craftOne(s,"SH076",conversion);expect(!exists(s,source),"Recasting did not consume its exact input");auto p=part(s,slug);
        expect(p.originalValue==item.second&&p.effects.size()==1&&p.rarity==Rarity::Uncommon,"Conversion inherited tuned value or source Shield payload");
        const Materials basis{item.second==9?2:3,0,0,0,0};expect(p.materialBasis==basis&&p.resaleReference=="generated:ammo:"+std::to_string(item.second),"Recast fixed-value price basis");
        Action fit;fit.parts={slug};activate(s,craftOne(s,"SH036"),fit);Action copy;copy.parts={slug};craft(s,"SH118",copy);roundTrip(s);next(s);
        auto copies=createdBy(s,"SH118");expect(copies.size()==2,"Recast printed-copy count");auto cp=part(s,copies[0]);expect(cp.materialBasis==basis&&cp.sourceRecipeCopy==p.sourceRecipeCopy&&cp.effects.size()==1&&cp.attachments.empty(),"Recast copies inherited costs/bonus or lost provenance");
        expect(fire(s,{copies[0]})==item.second,"Recast copy has generator side effect or wrong capped damage");
    }
    auto s=arena();auto variable=craftOne(s,"MA096");Action conversion=Action::craft(remember(s,"SH076"));conversion.parts={variable};reject(s,conversion);
}

void generatedShieldCopy(){
    auto s=arena();craft(s,"SH026");auto immediate=createdBy(s,"SH026");expect(immediate.size()==1&&part(s,immediate[0]).originalValue==4,"Folding Brace immediate output");
    auto original=part(s,immediate[0]);apply(s,Action::remove(original.id));expect(isUnusedPart(part(s,original.id)),"Pristine auto-installed Shield should remain eligible after removal");
    Action copy;copy.parts={original.id};craft(s,"SH102",copy);next(s);auto clones=createdBy(s,"SH102");expect(clones.size()==2,"Common generated Shield copies missing");
    expect(createdBy(s,"SH026").size()==2,"Original delayed six-Shield source missing or repeated");auto before=s.deliveries.size();
    for(Id id:clones){apply(s,Action::install(id));expect(part(s,id).shield==4&&part(s,id).materialBasis==Materials({1,1,0,0,0})&&part(s,id).sourceRecipeCopy==original.sourceRecipeCopy,"Generated copy should be plain four-Shield");}
    expect(s.deliveries.size()==before,"Installing plain copies replayed Folding Brace delivery");
}

void modifierRefresh(){
    auto s=arena();for(int i=0;i<2;++i)activate(s,craftOne(s,"SH050"));expect(s.hp==72,"Two physical Risky Packing activations each cost four HP");
    activate(s,craftOne(s,"MA010"));expect(s.heat==7,"Different named Heat modifier paid once");
    auto slug=craftOne(s,"SH001");expect(fire(s,{slug})==25,"Same-name +60% should refresh; distinct +10 combines: floor(16*1.6)=25");
    expect(fire(s,{craftOne(s,"SH001")})==6,"Consumed named modifiers repeated on another shot");
}

void spread(const std::string& id){
    auto s=arena();auto out=craftOne(s,id);auto ammo=craftOne(s,"SH013"); // Main Burn must not be copied to extra targets.
    auto second=craftOne(s,"SH003"),third=craftOne(s,"SH001");auto load=Action::load({ammo,second,third,out});Id sacrifice=0;
    if(id=="SH110"){sacrifice=craftOne(s,"SH002");load.sacrifices={sacrifice};}
    apply(s,load);if(sacrifice){expect(part(s,sacrifice).place==Place::Payment,"Shield sacrifice not reserved");reject(s,Action::install(sacrifice));Action unload;unload.type=ActionType::Unload;apply(s,unload);expect(part(s,sacrifice).place==Place::Reserve,"Unload did not return payment");apply(s,load);}
    // Two player Weaken makes 4+3+6 become 11: catches integer factors.
    s.weaken=2;s.enemies[0].mark=9;s.enemies[1].mark=17;s.enemies[1].armor=1;s.enemies[1].shield=1;
    Action shot=Action::fire(s.enemies[0].id);if(id=="SH005"||id=="MA041")shot.spreadTargets={{out,s.enemies[1].id}};if(id=="SH051")shot.spreadTargets={{out,s.enemies[1].id},{out,s.enemies[2].id}};
    const Amount cost=id=="MA041"?4:id=="MA111"?7:0;if(cost){s.heat=cost-1;reject(s,shot);s.heat=cost;}
    previewApply(s,shot);const Amount extra=id=="SH110"?11:id=="MA111"?6:id=="SH051"?4:5;
    expect(s.enemies[0].hp==280&&s.enemies[0].burn==2,"Main shot should add only its own Mark to 11, then apply Ember Burn");expect(s.enemies[1].hp==300-(extra-2)&&s.enemies[1].mark==17,"Extra hit must floor pre-target amount, ignore Mark, and use own Armor/Shield");
    expect(!exists(s,out)&&!exists(s,ammo)&&!exists(s,second)&&!exists(s,third),"Fire kept physical inputs");if(cost)expect(s.heat==0,"Spread Heat spent early, omitted or duplicated");
    expect(s.enemies[1].burn==(id=="MA041"?2:0)&&s.enemies[1].weaken==(id=="SH110"?3:0),"Spread-specific payload mismatch");if(sacrifice)expect(!exists(s,sacrifice),"Fire did not consume reserved Shield");
}

struct Magnet {const char* id;Materials extra;std::vector<Amount> choices;};
const std::vector<Magnet> magnets={
    {"SH008",{},{}},{"SH037",{2,0,0,0,0},{}},{"SH038",{0,2,0,0,0},{}},{"SH039",{0,0,2,0,0},{}},{"SH040",{0,0,0,2,0},{}},
    {"SH077",{1,1,0,0,0},{}},{"SH078",{0,0,1,1,0},{}},{"SH079",{0,0,0,0,1},{}},{"SH080",{}, {3,4}},
    {"SH105",{},{}},{"SH106",{0,0,0,0,4},{4}},{"SH107",{1,1,1,1,0},{}},{"SH108",{}, {4}},
    {"SH119",{3,0,0,0,3},{4,0}},{"SH120",{},{}},{"SH121",{0,0,0,1,0},{3}},{"SH122",{0,0,0,1,0},{3,0}},
    {"SH123",{0,0,0,1,0},{}},{"SH124",{0,1,0,0,0},{}},{"SH125",{1,1,0,0,0},{}},{"SH126",{1,0,0,0,0},{}}
};
void magnet(const Magnet& row){
    const std::string id=row.id;Materials single{};
    for(bool duplicate:{false,true}){
        auto s=arena();for(int n=0;n<(duplicate?2:1);++n){Action craftChoice;craftChoice.choices=row.choices;auto p=craftOne(s,id,craftChoice);activate(s,p);expect(!exists(s,p),"Fit did not consume exact Magnet source");}
        apply(s,Action::endTurn());roundTrip(s);auto stock=s.materials;Action collect=Action::collect(4,1);if(id=="SH105")collect.discarded={2,0,0,0,0};previewApply(s,collect);auto haul=minus(s.materials,stock);
        const Materials base{3,2,1,1,4};
        if(id=="SH008"||id=="SH105"||id=="SH120")expect(total(haul)==11+(id=="SH008"?2:id=="SH105"?3:4),"Normal-mix bonus/discard count");
        else if(id=="SH080")expect(total(haul)==11&&haul[3]+haul[4]==8,"Three preference positions changed haul size or preferred category count");
        else if(id=="SH108")expect(haul==Materials({0,1,1,1,8})&&Rules::shield(s)==6,"Four preferred positions/post-haul Shield");
        else expect(minus(haul,base)==row.extra,"Good/Circuit steering activated wrong unconditional addition");
        if(duplicate)expect(haul==single,"Identical fitted sources stacked");else single=haul;
        if(id=="SH126")expect(createdBy(s,id).empty(),"Good must not create Perfect-only parts");
        apply(s,Action::endTurn());stock=s.materials;apply(s,Action::collect(4));expect(minus(s.materials,stock)==Materials({3,2,1,1,3}),"Magnet repeated after its designated collection");
    }
    // Partially depleted optional stock; unlike the author baseline-only pile,
    // some bonuses can pay while other named materials are exhausted.
    auto s=arena();Action opts;opts.choices=row.choices;activate(s,craftOne(s,id,opts));apply(s,Action::endTurn());s.finitePile=true;s.pile={3,3,1,2,5};const auto pile=s.pile,stock=s.materials;
    Action collect=Action::collect(4,1);if(id=="SH105")collect.discarded={2,0,0,0,0};previewApply(s,collect);auto haul=minus(s.materials,stock);
    for(std::size_t i=0;i<5;++i)expect(haul[i]+s.pile[i]==pile[i]&&s.pile[i]>=0,"Partial finite-pile material conservation");
    if(id!="SH008"&&id!="SH105"&&id!="SH120"&&id!="SH080"&&id!="SH108"){
        Materials expected{3,2,1,1,4};for(std::size_t i=0;i<5;++i)expected[i]+=std::min(row.extra[i],pile[i]-expected[i]);expect(haul==expected,"Typed bonus ignored partial remaining stock");
    }
}

void perfectAndExpiry(){
    auto s=arena();activate(s,craftOne(s,"SH124"));Action opts;opts.choices={1};activate(s,craftOne(s,"SH121",opts));activate(s,craftOne(s,"SH126"));apply(s,Action::endTurn());
    // Baseline Copper4 + ordinary Perfect2 leaves3 Copper: amplifier's +1
    // and copied bundle2 consume it before Jaws. Generated outputs cost no pile.
    s.finitePile=true;s.pile={4,9,1,1,1};auto stock=s.materials;previewApply(s,Action::collect(1,2));auto haul=minus(s.materials,stock);
    expect(haul==Materials({4,9,1,1,1}),"Perfect extras consumed baseline or fabricated scarce Copper");auto out=createdBy(s,"SH126");expect(out.size()==2,"Perfect should create one slug and one plate");
    expect(std::count_if(out.begin(),out.end(),[&](Id id){return part(s,id).kind==Kind::Ammo;})==1&&std::count_if(out.begin(),out.end(),[&](Id id){return part(s,id).kind==Kind::Shield;})==1,"Perfect must grant different Ammo/Shield output kinds");
    for(Id id:out){const auto& p=part(s,id);expect(p.place==Place::Reserve&&p.originalValue==10&&p.rarity==Rarity::Legendary&&p.sourceRecipeCopy==0,"Salvage outputs lost fixed grant/provenance");expect(p.materialBasis==Materials({2,p.kind==Kind::Shield?1:0,0,0,0}),"Salvage output inherited Magnet cost instead of plain-part value");}
    s=arena();auto expired=craftOne(s,"SH126");apply(s,Action::endTurn());Action late;late.type=ActionType::Activate;late.subject=expired;reject(s,late);apply(s,Action::collect(0,2));expect(!exists(s,expired)&&createdBy(s,"SH126").empty(),"Unfitted enhancement was stored or triggered late");
    s=arena();s.precisionSpent=true;activate(s,craftOne(s,"SH126"));apply(s,Action::endTurn());stock=s.materials;apply(s,Action::collect(0));expect(minus(s.materials,stock)==Materials({6,2,1,1,1})&&createdBy(s,"SH126").empty(),"Already-spent Precision disabled ordinary bonus or made parts");
}

void ephemeralShield(){
    auto s=arena();upgrade(s,"UGS-028");upgrade(s,"MY3-03");craftOne(s,"SH003");
    auto r=previewApply(s,Action::endTurn());Id generated=0;for(const auto& e:r.events)if(e.type=="part_created"&&e.source=="UGS-028"){generated=e.subject;expect(e.amount==1,"Exactly one reserve part should make Shield1");}
    expect(generated!=0&&!exists(s,generated)&&Rules::shield(s)==1,"Retention should carry balance without preserving ephemeral source part");
    apply(s,Action::collect(0));reject(s,Action::remove(generated));roundTrip(s);
    s=arena();upgrade(s,"UGS-028");craftOne(s,"SH003");s.enemies[0].intent=s.enemies[0].pattern[0]={Move::Attack,2,1};r=previewApply(s,Action::endTurn());
    expect(s.hp==79&&Rules::shield(s)==0,"Ephemeral Shield must protect before enemy damage and reset");expect(std::none_of(s.parts.begin(),s.parts.end(),[](const Part& p){return p.creator=="UGS-028";}),"Ephemeral physical object survived reset");
}

void sweep(){
    for(Amount amount:{0,1,2,3,4,5,8,9,10,11,14,15,16,20}){
        auto s=arena();upgrade(s,"MY3-03");if(amount)plain(s,Kind::Shield,amount,true);auto reserve=plain(s,Kind::Shield,40);activate(s,craftOne(s,"SH066"));roundTrip(s);auto stock=s.materials;auto r=previewApply(s,Action::endTurn());
        const Amount iron=std::min(3,amount/3);expect(minus(s.materials,stock)==Materials({iron,0,0,0,0})&&Rules::shield(s)==std::min(6,amount-3*iron),"Grouped automatic payment/remainder/retention cap");expect(exists(s,reserve)&&part(s,reserve).place==Place::Reserve,"Sweep spent reserve Shield");
        Id lastEnemy=0,schedule=0,retained=0,delivery=0;for(const auto& e:r.events){if(e.type=="enemy_action")lastEnemy=e.id;if(e.type=="delivery_scheduled")schedule=e.id;if(e.type=="shield_retained")retained=e.id;if(e.type=="delivery")delivery=e.id;}
        expect(lastEnemy<schedule&&schedule<retained&&retained<delivery,"Sweep event order is not enemies/payment/retention/next-turn delivery");
    }
    for(bool sweepFirst:{false,true}){
        auto s=arena();upgrade(s,"MY3-03");if(sweepFirst)activate(s,craftOne(s,"SH066"));auto reader=craftOne(s,"MA055");apply(s,Action::install(reader));plain(s,Kind::Shield,7,true);activate(s,craftOne(s,"SH066"));
        s.enemies[0].intent=s.enemies[0].pattern[0]={Move::Attack,3,1};auto stock=s.materials;previewApply(s,Action::endTurn());
        expect(minus(s.materials,stock)==Materials({sweepFirst?4:5,0,0,0,0})&&Rules::shield(s)==5,"Reader earns one per five: sees14 before Sweep or5 after it; duplicate keeps first priority");
        apply(s,Action::collect(0));stock=s.materials;plain(s,Kind::Shield,9,true);apply(s,Action::endTurn());expect(s.materials==stock,"Sweep binding repeated next round");
    }
    for(bool lossFirst:{false,true}){
        auto s=arena();upgrade(s,"MY3-03");if(!lossFirst)activate(s,craftOne(s,"SH066"));apply(s,Action::install(craftOne(s,"SH060")));if(lossFirst)activate(s,craftOne(s,"SH066"));auto stock=s.materials;previewApply(s,Action::endTurn());
        expect(s.materials[0]-stock[0]==(lossFirst?0:3)&&Rules::shield(s)==0,"Earlier Shield loss versus already-recorded reward");
    }
}

CampaignAction command(const Campaign& c,CampaignActionType type,Id subject=0,const std::string& choice={}){CampaignAction a;a.runId=c.runId;a.sequence=c.nextTransaction;a.type=type;a.subject=subject;a.choice=choice;return a;}
void transaction(const CampaignRules& engine,Campaign& c,const CampaignAction& a){auto r=engine.apply(c,a);expect(r.ok,r.reason);}
void saleAndEnd(){
    CampaignRules engine(rules,cinderwallUpgradeHooks(rules));auto c=engine.newGame(91,"qa-parts-sales");c.mayorOffers={"MY1-02","MY1-01","MY1-03"};transaction(engine,c,command(c,CampaignActionType::ChooseMayor,0,"MY1-02"));
    auto offers=routeOffers(c.route);auto entry=std::find_if(offers.begin(),offers.end(),[](const RouteOffer& o){return o.kind==EncounterKind::Regular;});expect(entry!=offers.end(),"No initial regular route");transaction(engine,c,command(c,CampaignActionType::EnterOffer,entry->id));
    auto collect=command(c,CampaignActionType::Combat);collect.combat=Action::collect(0);transaction(engine,c,collect);c.fight.materials={30,30,30,30,30};auto ids=craft(c.fight,"MA038");auto credits=c.fight.credits;
    for(Id id:ids){auto sale=command(c,CampaignActionType::SellPart,id);transaction(engine,c,sale);auto bytes=serializeCampaign(c);auto duplicate=engine.apply(c,sale);expect(duplicate.ok&&duplicate.replayed&&serializeCampaign(c)==bytes,"Replay paid a sale twice");}
    expect(c.fight.credits==credits+8,"Two Warm Rivets use independent normal one-part bases, not paid batch value");
    auto generated=craftOne(c.fight,"SH126");activate(c.fight,generated);auto saved=plain(c.fight,Kind::Ammo,7);for(auto& e:c.fight.enemies){e.robotAction.clear();e.intent={Move::Escape,0,0};e.pattern={e.intent};}
    auto end=command(c,CampaignActionType::Combat);end.combat=Action::endTurn();transaction(engine,c,end);expect(c.phase==CityPhase::Rewards&&!exists(c.fight,saved)&&c.fight.parts.empty()&&c.fight.bindings.empty()&&c.fight.deliveries.empty(),"Campaign result retained saved part or armed Magnet");
    auto bytes=serializeCampaign(c);Campaign restored;std::string error;expect(deserializeCampaign(bytes,restored,error)&&serializeCampaign(restored)==bytes,"Result cleanup changed on reload");
}
}
int main(){
    run("P01 paid Warm Rivet / consumed return / immutable per-part floor",paidRivets);
    run("P02 variable Heat and HP first-install history",paidShieldHistory);
    run("P03 capped generated conversion / tuned source / fresh copies",generatedRecast);
    run("P04 pristine generated Shield copies do not replay generator",generatedShieldCopy);
    run("P05 same-name refresh versus separate payments and distinct bonuses",modifierRefresh);
    for(const auto* id:{"SH005","SH051","SH082","SH110","MA041","MA111"})run(std::string("P06 odd spread / target defence / payment ")+id,[id]{spread(id);});
    for(const auto& row:magnets)run(std::string("P07 Good Circuit / duplicate / partial pile ")+row.id,[&row]{magnet(row);});
    run("P08 Perfect scarcity / unfitted expiry / spent Precision",perfectAndExpiry);
    run("P09 ephemeral Shield1 with actual retention and enemy hit",ephemeralShield);
    run("P10 automatic Sweep / post-enemy ordered reads / remainders",sweep);
    run("P11 real sale receipts and armed campaign cleanup",saleAndEnd);
    std::cout<<"SUMMARY "<<passed<<" passed, "<<failed<<" failed, "<<assertions<<" assertions\n";return failed?1:0;
}

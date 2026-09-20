// Additional physical-type contracts. Source-specific numbers are explicit
// oracles; this file does not interpret catalogue prose at runtime.
struct PlainCase {const char* id;Kind kind;Amount value;};
const std::vector<PlainCase> plainCases={
#include "plain-cases.inc"
};
State quiet(const Rules& rules){auto s=arena(rules);for(auto& e:s.enemies){e.burn=e.corrosion=e.weaken=e.mark=0;e.intent=e.pattern[0]={Move::Recover,0,0};}return s;}
Id plainAmmo(const Rules& rules,State& s,Amount value){const auto before=s.nextId;auto result=rules.grantPlainPart(s,Kind::Ammo,value,"lifecycle-ammo-fixture");check(result.ok,result.reason);for(const auto& p:s.parts)if(p.id>=before)return p.id;throw std::runtime_error("No fixture Ammo");}
Result activate(const Rules& rules,State& s,Id id,Action a={}){a.type=ActionType::Activate;a.subject=id;return act(rules,s,a);}
Amount shotValue(const Rules& rules,State& s,std::vector<Id> parts,Action a={}){const auto hp=s.enemies[0].hp;act(rules,s,Action::load(std::move(parts)));a.type=ActionType::Fire;a.target=s.enemies[0].id;act(rules,s,a);return hp-s.enemies[0].hp;}
Amount sum(const Materials& values){Amount n=0;for(auto v:values)n+=v;return n;}
Materials difference(const Materials& a,const Materials& b){Materials out{};for(std::size_t i=0;i<5;++i)out[i]=a[i]-b[i];return out;}
void planningLifecycle(const Rules& rules,const Case& row){
    auto s=quiet(rules);const auto id=produce(rules,s,row).front();const auto original=part(s,id);identity(original,row);valuation(s,id,row);
    check(original.origin==PartOrigin::Produced&&original.place==Place::Reserve&&original.createdRound==1,"Planning output lost paid physical identity");
    if(creationOptions(row.recipe).choices.empty()){
        auto gifted=s;const auto resources=gifted.materials;const auto copy=gifted.memory.back();const auto gift=granted(rules,gifted,row.recipe);identity(part(gifted,gift),row);valuation(gifted,gift,row);
        check(gift!=id&&part(gifted,gift).origin==PartOrigin::Granted&&part(gifted,gift).sourceRecipeCopy==0&&gifted.materials==resources&&gifted.memory.back().usedRound==copy.usedRound&&gifted.memory.back().cooldown==copy.cooldown,"Planning grant paid, recrafted, or reused the original identity");
    }
    reject(rules,s,Action::install(id));if(row.kind!=Kind::Spread)reject(rules,s,Action::load({id}));
    for(const auto* copier:{"SH071","SH102","SH118"}){auto copy=Action::craft(memory(s,copier));copy.parts={id};reject(rules,s,copy);}snapshot(s);
    if(row.kind==Kind::Magnet){
        auto invalid=creationOptions(row.recipe);invalid.type=ActionType::Craft;invalid.subject=memory(s,row.recipe);
        if(!invalid.choices.empty()){invalid.choices={5};reject(rules,s,invalid);if(creationOptions(row.recipe).choices.size()==2){invalid.choices={2,2};reject(rules,s,invalid);}}
        act(rules,s,Action::endTurn());check(exists(s,id),"Unfitted Magnet disappeared before the next collection boundary");Action late;late.type=ActionType::Activate;late.subject=id;reject(rules,s,late);act(rules,s,Action::collect(0));check(!exists(s,id),"Expired unfitted Magnet survived next collection");reject(rules,s,late);
    }else{
        if(row.kind==Kind::Spread){auto load=Action::load({id});if(std::string(row.recipe)=="SH110")load.sacrifices={granted(rules,s,"SH002")};act(rules,s,load);}next(rules,s);identity(part(s,id),row);valuation(s,id,row);
        check(part(s,id).place==Place::Reserve&&part(s,id).createdRound==1&&isUnusedPart(part(s,id)),"Stored unused planning/spread part lost age or reserve state");
    }
}
Materials magnetExpected(const std::string& id,Amount precision){
    Materials v{5+std::max(0,precision),2,1,1,1};
    if(id=="SH037")v[0]+=2;else if(id=="SH038")v[1]+=2;else if(id=="SH039")v[2]+=2;else if(id=="SH040")v[3]+=2;
    else if(id=="SH077"){++v[0];++v[1];}else if(id=="SH078"){++v[2];++v[3];}else if(id=="SH079")++v[4];
    else if(id=="SH080"){v[0]-=3;v[2]+=2;++v[3];}else if(id=="SH106")v[2]+=4;
    else if(id=="SH107")for(std::size_t i=0;i<4;++i)++v[i];else if(id=="SH108"){v[0]-=4;v[2]+=4;}
    else if(id=="SH119"){v[2]+=3;v[3]+=3;}else if(id=="SH121")v[2]+=precision==2?3:1;
    else if(id=="SH122"){v[2]+=precision==2?2:1;if(precision==2)v[3]+=2;}
    else if(id=="SH123"){++v[3];if(precision==2)v[4]+=2;}else if(id=="SH124"){++v[1];if(precision==2)v[0]+=2;}
    else if(id=="SH125"){++v[0];++v[1];if(precision==2)for(auto& n:v)++n;}else if(id=="SH126")++v[0];return v;
}
void magnetSemantics(const Rules& rules,const Case& row){
    const std::string name=row.recipe;
    for(Amount precision:{-1,0,1,2})for(bool duplicate:{false,true}){
        auto s=quiet(rules);const auto p=produce(rules,s,row).front();const auto old=s.materials;activate(rules,s,p);check(!exists(s,p)&&s.materials==old,"Fitting retained its physical source or granted the future haul immediately");
        Action again;again.type=ActionType::Activate;again.subject=p;reject(rules,s,again);
        if(duplicate){const auto q=produce(rules,s,row).front();activate(rules,s,q);}
        snapshot(s);act(rules,s,Action::endTurn());auto a=Action::collect(0,precision);if(name=="SH105")a.discarded={2,0,0,0,0};
        if(name=="SH105"){auto bad=a;bad.discarded={};reject(rules,s,bad);}else{auto bad=a;bad.discarded={1,0,0,0,0};reject(rules,s,bad);}
        const auto before=s.materials;const auto saved=serialize(s);const auto preview=rules.preview(s,a);check(preview.result.ok,preview.result.reason);check(serialize(s)==saved,"Magnet collection preview paid, drew or committed live state");act(rules,s,a);check(serialize(s)==serialize(preview.state),"Magnet collection preview diverged");const auto haul=difference(s.materials,before);
        if(name=="SH008"||name=="SH105"||name=="SH120"){
            const Amount extra=name=="SH008"?2:name=="SH105"?3:4;check(sum(haul)==10+std::max(0,precision)+extra,"Normal-mix extra count or Heavy Lift discard is wrong");
            if(name=="SH120")for(std::size_t i=0;i<5;++i)check(haul[i]>0,"Balanced Haul missed an available named type");
        }else check(haul==magnetExpected(name,precision),"Typed next-round haul differs from authored amount/priority/Precision condition");
        if(name=="SH108")check(Rules::shield(s)==6,"Sorting's postcollection six-Shield grant missing");
        if(name=="SH126"){std::vector<Part> reward;for(const auto& part:s.parts)if(part.creator==name)reward.push_back(part);check(reward.size()==(precision==2?2U:0U),"Salvage parts repeated or triggered without Perfect");for(const auto& part:reward)check(part.place==Place::Reserve&&part.originalValue==10&&part.origin==PartOrigin::Granted&&part.sourceRecipeCopy==0,"Salvage part was auto-installed or treated as another paid Use");}
        act(rules,s,Action::endTurn());const auto stock=s.materials;act(rules,s,Action::collect(0));check(difference(s.materials,stock)==Materials({5,2,1,1,1}),"A fitted Magnet repeated beyond its single following collection");
    }
    auto s=quiet(rules);activate(rules,s,produce(rules,s,row).front());act(rules,s,Action::endTurn());s.finitePile=true;s.pile={5,2,1,1,1};const auto before=s.materials;auto a=Action::collect(0,2);if(name=="SH105")a.discarded={2,0,0,0,0};act(rules,s,a);const auto gained=difference(s.materials,before);
    check(sum(gained)==(name=="SH105"?8:10),"Missing optional stock reduced guaranteed haul or created bonus materials");
    check(sum(s.pile)==(name=="SH105"?2:0),"Finite pile/discard conservation changed");
}
void spreadSemantics(const Rules& rules,const Case& row){
    const std::string name=row.recipe;auto s=quiet(rules);s.heat=10;const auto p=produce(rules,s,row).front();Action bad;bad.type=ActionType::Activate;bad.subject=p;reject(rules,s,bad);
    const auto ammo=granted(rules,s,"SH013");auto load=Action::load({ammo,p});Id paid=0;
    if(name=="SH110"){paid=granted(rules,s,"SH002");load.sacrifices={paid};}
    const auto heat=s.heat;act(rules,s,load);check(s.heat==heat,"Load paid the Fire-time Heat");check(upgradePartSaleValue(s,part(s,p),{4,4,4,5,6})==-1,"Loaded spread remains saleable");
    if(paid){check(part(s,paid).place==Place::Payment&&part(s,paid).reservedBy==p,"Full-Spread Shield payment not reserved to exact part");check(upgradePartSaleValue(s,part(s,paid),{4,4,4,5,6})==-1,"Reserved Shield payment remains saleable");reject(rules,s,Action::install(paid));}
    Action unload;unload.type=ActionType::Unload;act(rules,s,unload);if(paid)check(part(s,paid).place==Place::Reserve&&part(s,paid).reservedBy==0,"Unload did not release exact Shield payment");act(rules,s,load);snapshot(s);
    Action shot=Action::fire(s.enemies[0].id);if(name=="SH005"||name=="MA041")shot.spreadTargets={{p,s.enemies[1].id}};if(name=="SH051")shot.spreadTargets={{p,s.enemies[1].id},{p,s.enemies[2].id}};
    if(!shot.spreadTargets.empty()){auto invalid=shot;invalid.spreadTargets[0].enemy=s.enemies[0].id;reject(rules,s,invalid);}
    const Amount cost=name=="MA041"?4:name=="MA111"?7:0;
    if(cost){s.heat=cost-1;reject(rules,s,shot);s.heat=10;}
    act(rules,s,shot);check(!exists(s,p)&&!exists(s,ammo)&&s.bullet.empty(),"Fire did not consume exact loaded spread/ammo identities");check(s.heat==10-cost,"Wrong committed spread Heat payment");
    check(s.enemies[0].hp==996&&s.enemies[0].burn==2,"Spread changed main-target payload");const Amount damage=name=="SH051"?1:name=="SH110"?4:2;
    check(s.enemies[1].hp==1000-damage,"Spread integer factor differs from source");check(s.enemies[1].burn==(name=="MA041"?2:0),"Spread copied main payload or lost its own Burn");
    if(name=="SH005"||name=="MA041")check(s.enemies[2].hp==1000,"Single-target spread hit an unselected target");else check(s.enemies[2].hp==1000-damage,"Selected/all-target spread skipped second target");
    if(name=="SH110"){check(!exists(s,paid),"Full-Spread did not consume its reserved Shield");check(s.enemies[1].weaken==3&&s.enemies[2].weaken==3,"Full-Spread lost its own aftermath");}
    if(name=="MA041"||name=="MA111"){
        s=quiet(rules);auto acquired=rules.acquireUpgrade(s,"MAU-02");check(acquired.ok,acquired.reason);s.heat=14;const auto first=produce(rules,s,row).front(),second=produce(rules,s,row).front(),slug=plainAmmo(rules,s,10);act(rules,s,Action::load({slug,first,second}));shot=Action::fire(s.enemies[0].id);if(name=="MA041")shot.spreadTargets={{first,s.enemies[1].id},{second,s.enemies[1].id}};act(rules,s,shot);
        check(s.enemies[1].hp==1000-(name=="MA041"?5:6),"Identical heat spreads repeated the extra hit");check(s.heat==14-2*cost,"Two physical spread payments were collapsed");
    }
}
State sweepArena(const Rules& rules){
    auto s=quiet(rules);s.parts.clear();const auto acquired=rules.acquireUpgrade(s,"MY3-03");check(acquired.ok,acquired.reason);return s;
}
Id fixedShield(const Rules& rules,State& s,Amount amount,bool installed=true){
    const auto before=s.nextId;const auto granted=rules.grantPlainPart(s,Kind::Shield,amount,"sweep-shield-fixture",installed);check(granted.ok,granted.reason);
    for(const auto& p:s.parts)if(p.id>=before)return p.id;throw std::runtime_error("Missing controlled Shield part");
}
void armSweep(const Rules& rules,State& s,const Case& row){
    const auto physical=produce(rules,s,row).front();const auto resources=s.materials;const auto shield=Rules::shield(s);activate(rules,s,physical);
    check(!exists(s,physical)&&s.materials==resources&&Rules::shield(s)==shield,"Sweep activation paid Shield or granted Iron before its boundary");
    Action reused;reused.type=ActionType::Activate;reused.subject=physical;reject(rules,s,reused);
}
Result resolveSweep(const Rules& rules,State& s,Amount iron,Amount retained){
    const auto before=s.materials;const auto round=s.round;const auto saved=serialize(s);const auto preview=rules.preview(s,Action::endTurn());check(preview.result.ok,preview.result.reason);check(serialize(s)==saved,"Sweep preview changed live state");
    const auto result=act(rules,s,Action::endTurn());check(serialize(s)==serialize(preview.state),"Sweep preview/reload arithmetic diverged");
    check(s.phase==Phase::Collection&&s.round==round+1,"Sweep did not reach next-turn delivery boundary");
    check(difference(s.materials,before)==Materials({iron,0,0,0,0})&&Rules::shield(s)==retained,"Automatic Sweep payment, remainder, cap, reader or retention is wrong");
    Id lastEnemy=0,firstSchedule=0,lastSchedule=0,retention=0,firstDelivery=0;std::vector<Id> scheduled,delivered;
    for(const auto& e:result.events){
        if(e.type=="enemy_action")lastEnemy=e.id;
        if(e.type=="delivery_scheduled"){if(!firstSchedule)firstSchedule=e.id;lastSchedule=e.id;scheduled.push_back(e.subject);}
        if(e.type=="shield_retained")retention=e.id;
        if(e.type=="delivery"){if(!firstDelivery)firstDelivery=e.id;delivered.push_back(e.subject);}
        if(e.type=="material_granted")check(e.id>firstDelivery&&firstDelivery!=0,"Sweep resources arrived before next-turn delivery");
    }
    check(lastEnemy>0&&firstSchedule>lastEnemy&&retention>lastSchedule&&firstDelivery>retention,"Sweep did not resolve after enemies, before retention/reset, then deliver next turn");
    check(scheduled==delivered&&!scheduled.empty()&&s.deliveries.empty(),"Recorded Sweep/reader delivery was missing, duplicated or left pending");
    check(std::none_of(s.bindings.begin(),s.bindings.end(),[](const Binding& b){return b.source=="SH066";}),"Consumed Sweep binding survived its one round");snapshot(s);return result;
}
void sweepSemantics(const Rules& rules,const Case& row){
    // Section3's automatic grouped conversion: all remainders below/above
    // each threshold, plus amounts above the three-Iron cap. No Action choice.
    for(Amount available=0;available<=14;++available){
        auto s=sweepArena(rules);if(available)fixedShield(rules,s,available);armSweep(rules,s,row);snapshot(s);
        const Amount iron=std::min(3,available/3);resolveSweep(rules,s,iron,available-3*iron);
        act(rules,s,Action::collect(0));fixedShield(rules,s,12);const auto before=s.materials;act(rules,s,Action::endTurn());check(s.materials==before,"Sweep repeated on a later round without another physical activation");
    }
    // Reserve supplies do not pay active-Shield conversions. Two active points
    // cannot buy one complete group and remain available for real retention.
    {
        auto s=sweepArena(rules);const auto reserve=fixedShield(rules,s,20,false);fixedShield(rules,s,2);armSweep(rules,s,row);resolveSweep(rules,s,0,2);check(exists(s,reserve)&&part(s,reserve).place==Place::Reserve&&isUnusedPart(part(s,reserve)),"Sweep spent reserve Shield");
    }
    // The amount is measured after the actual enemy hit, not at activation.
    {
        auto s=sweepArena(rules);fixedShield(rules,s,12);s.enemies[0].intent=s.enemies[0].pattern[0]={Move::Attack,4,1};armSweep(rules,s,row);const auto hp=s.hp;const auto result=resolveSweep(rules,s,2,2);Amount absorbed=0;for(const auto& e:result.events)if(e.type=="player_damage")absorbed+=e.secondary;check(s.hp==hp&&absorbed==4,"Sweep spent protection before the enemy action");
    }
    // Eligible Mara analogue of T05: MA055 reads12->2Iron before Sweep,
    // or reads the remaining3->0Iron after Sweep. Actual Vault retains3.
    for(bool readerFirst:{false,true}){
        auto s=sweepArena(rules);if(!readerFirst)armSweep(rules,s,row);const auto reader=granted(rules,s,"MA055");act(rules,s,Action::install(reader));fixedShield(rules,s,2);armSweep(rules,s,row);
        check(std::count_if(s.bindings.begin(),s.bindings.end(),[](const Binding& b){return b.source=="SH066";})==1,"Same-name Sweep stacked instead of refreshing");resolveSweep(rules,s,readerFirst?5:3,3);
    }
    // An earlier real pre-reset Shield loss leaves nothing for Sweep; reversing
    // binding order preserves the Iron already recorded before that loss.
    for(bool staysFirst:{false,true}){
        auto s=sweepArena(rules);if(!staysFirst)armSweep(rules,s,row);const auto stays=granted(rules,s,"SH060");act(rules,s,Action::install(stays));if(staysFirst)armSweep(rules,s,row);resolveSweep(rules,s,staysFirst?0:3,0);
    }
    {
        CampaignRules campaigns(rules,cinderwallUpgradeHooks(rules));auto c=campaignArena(campaigns);armSweep(rules,c.fight,row);fixedShield(rules,c.fight,12);
        for(auto& enemy:c.fight.enemies){enemy.robotAction.clear();enemy.pattern={{Move::Escape,0,0}};enemy.intent=enemy.pattern.front();}
        combat(campaigns,c,Action::endTurn());check(c.phase==CityPhase::Rewards&&c.fight.parts.empty()&&c.fight.bindings.empty()&&c.fight.deliveries.empty()&&c.fight.materials==Materials{},"Armed Sweep escaped fight-end cleanup");
    }
}
void modifierSemantics(const Rules& rules,const Case& row){
    const std::string name=row.recipe;if(name=="SH066"){sweepSemantics(rules,row);return;}auto s=quiet(rules);const auto p=produce(rules,s,row).front();Action a;a.target=s.enemies[0].id;Id chosenAmmo=0;
    if(name=="SH036"){chosenAmmo=granted(rules,s,"SH001");a.parts={chosenAmmo};}
    if(name=="MA052")a.parts={granted(rules,s,"SH001"),granted(rules,s,"SH001")};
    if(name=="MA045")a.amount=3;if(name=="MA061")a.amount=8;if(name=="SH085")a.amount=4;
    if(name=="SH095"){s.enemies[0].intent=s.enemies[0].pattern[0]={Move::Attack,10,1};}
    if(name=="SH064"){s.enemies=makeCinderwallFormation("C1-F-CHASSIS",1,"lifecycle-status",s.nextId);next(rules,s);}
    auto invalid=a;invalid.type=ActionType::Activate;invalid.subject=p;bool selection=false;
    if(name=="MA045"){invalid.amount=1;selection=true;}if(name=="MA061"){invalid.amount=3;selection=true;}if(name=="SH085"){invalid.amount=16;selection=true;}if(name=="MA052"||name=="SH036"){invalid.parts={};selection=true;}
    if(selection)reject(rules,s,invalid);
    const auto hp=s.hp,heat=s.heat,shield=Rules::shield(s);activate(rules,s,p,a);check(!exists(s,p),"Modifier activation retained reusable physical source");snapshot(s);
    Action twice=a;twice.type=ActionType::Activate;twice.subject=p;reject(rules,s,twice);
    if(name=="SH024"){
        for(auto& e:s.enemies)e.intent=e.pattern[0]={Move::Attack,10,1};const auto result=act(rules,s,Action::endTurn());Amount damage=0;for(const auto& e:result.events)if(e.type=="player_damage")damage+=e.amount+e.secondary;check(damage==24,"Guard Lining did not reduce only the first complete attack by six");return;
    }
    if(name=="SH035"){auto first=granted(rules,s,"SH002"),second=granted(rules,s,"SH002");act(rules,s,Action::install(first));act(rules,s,Action::install(second));check(part(s,first).shield==10&&part(s,second).shield==6,"Shield booster repeated or missed its next-part trigger");return;}
    if(name=="SH036"){next(rules,s);check(shotValue(rules,s,{chosenAmmo})==10,"Attached Fuse did not survive reserve storage until consumed");return;}
    if(name=="SH043"){s.enemies[0].shield=40;s.enemies[0].armor=2;check(shotValue(rules,s,{plainAmmo(rules,s,30)})==12&&s.enemies[0].shield==24,"Shield bypass exceeded12 or added damage");check(shotValue(rules,s,{plainAmmo(rules,s,30)})==4&&s.enemies[0].shield==0,"Shield bypass repeated on the second shot");return;}
    if(name=="SH064"){
        check(Rules::shield(s)==shield+5,"Status screen did not grant its separate five-Shield part");const auto result=act(rules,s,Action::endTurn());check(s.burn==0&&std::any_of(result.events.begin(),result.events.end(),[](const Event& e){return e.type=="status_prevented"&&e.amount==2;}),"First actual robot Burn application was not prevented");return;
    }
    if(name=="SH070"){next(rules,s);const auto iron=s.materials[0];act(rules,s,Action::craft(memory(s,"SH001")));check(s.materials[0]==iron-1,"One-Iron use incorrectly took discount");act(rules,s,Action::craft(memory(s,"SH104")));check(s.materials[0]==iron-2,"First two-Iron Utility did not receive the discount");act(rules,s,Action::craft(memory(s,"SH104")));check(s.materials[0]==iron-4,"Discount repeated");return;}
    if(name=="SH072"){const auto slug=granted(rules,s,"SH001");Action shot;shot.partChoices={{0,slug}};check(shotValue(rules,s,{slug},shot)==6,"Return Delivery changed source shot");next(rules,s);check(std::count_if(s.parts.begin(),s.parts.end(),[](const Part& x){return x.creator=="SH072"&&x.origin==PartOrigin::Copied;})==1,"Paid Delivery Catch missed next-turn physical copy");return;}
    if(name=="SH095"){check(s.enemies[0].weaken==8&&s.enemies[1].weaken==0&&Rules::shield(s)==shield+10,"Intent partition created wrong Weaken or Shield");return;}
    if(name=="SH103"){auto craft=Action::craft(memory(s,"MA038"));craft.amount=1;act(rules,s,craft);std::vector<Part> outputs;for(const auto& x:s.parts)if(x.recipe=="MA038")outputs.push_back(x);check(outputs.size()==2&&outputs[0].attachments.empty()&&outputs[1].attachments.size()==1&&outputs[1].attachments[0].damage==8,"Mould did not choose exactly one batch output");return;}
    if(name=="SH083"||name=="SH111"||name=="MA063"||name=="MA078"||name=="MA098"){
        std::vector<Id> ammo;Amount expected=0;
        if(name=="SH083"){for(int i=0;i<11;++i)ammo.push_back(plainAmmo(rules,s,6));expected=118;}
        if(name=="SH111"){for(int i=0;i<10;++i)ammo.push_back(plainAmmo(rules,s,1));expected=20;}
        if(name=="MA063"){ammo={granted(rules,s,"SH001"),granted(rules,s,"SH001"),granted(rules,s,"SH003")};expected=21;}
        if(name=="MA078"){for(int i=0;i<6;++i)ammo.push_back(plainAmmo(rules,s,1));expected=22;}
        if(name=="MA098"){for(int i=0;i<7;++i)granted(rules,s,"SH001");ammo={plainAmmo(rules,s,1)};expected=19;}
        check(shotValue(rules,s,ammo)==expected,"Counted/capped shot bonus differs from source");if(name=="SH111")check(Rules::shield(s)==shield+15,"Ten-Ammo Shield grant missing");if(name=="MA078")check(s.heat==heat-3&&Rules::shield(s)==shield+6,"Five-Ammo aftermath missing");check(shotValue(rules,s,{plainAmmo(rules,s,6)})==6,"Counted bonus repeated on next shot");return;
    }
    if(name=="MA094"){check(s.heat==heat-6,"Shield bypass Heat not paid at activation");s.enemies[0].shield=50;check(shotValue(rules,s,{plainAmmo(rules,s,6)})==6&&s.enemies[0].shield==50,"Main damage did not bypass Shield");check(shotValue(rules,s,{plainAmmo(rules,s,6)})==0&&s.enemies[0].shield==44,"Main bypass repeated");return;}
    if(name=="MA106"){s.enemies[0].hp=10;check(shotValue(rules,s,{plainAmmo(rules,s,6)})==10,"Recoil Store main shot missing8");next(rules,s);const auto target=s.enemies[1].id;act(rules,s,Action::load({plainAmmo(rules,s,6)}));const auto before=s.enemies[1].hp;act(rules,s,Action::fire(target));check(before-s.enemies[1].hp==22,"Recoil Store did not deliver next-round16");return;}
    Amount extra=0;
    if(name=="SH050"){check(s.hp==hp-4,"Overpacking HP payment");check(shotValue(rules,s,{plainAmmo(rules,s,10)})==16,"Overpacking60percent");}
    else{
        if(name=="MA010"){extra=10;check(s.heat==heat-3,"Thermal Sleeve payment");}
        else if(name=="MA027"){extra=12;check(Rules::shield(s)==shield-6,"Shield pusher payment");}
        else if(name=="MA045"){extra=9;check(s.heat==heat-3,"Variable Heat payment");}
        else if(name=="MA052"){extra=14;for(auto id:a.parts)check(!exists(s,id),"Consumed reserve Ammo survived sacrifice");}
        else if(name=="MA053"){extra=8;check(s.enemies[0].mark==4&&s.heat==heat,"Hot sight mark or Heat condition");}
        else if(name=="MA061"){extra=16;check(Rules::shield(s)==shield-8&&s.heat==std::min(10,heat+2),"Variable Shield/Heat payment");}
        else if(name=="MA067"){extra=18;auto spread=granted(rules,s,"SH005");reject(rules,s,Action::load({spread}));}
        else if(name=="SH085"){extra=8;check(Rules::shield(s)==shield-4,"Chosen up-to Shield amount ignored");}
        else if(name=="MA113"){extra=30;check(s.heat==0&&Rules::shield(s)==shield+20,"Winter Sleeve all-Heat/Shield commitment");}
        else if(name=="MA118"){extra=28;check(Rules::shield(s)==shield-12,"Brace Cannon Shield payment");}
        else throw std::runtime_error("Missing semantic planning fixture for "+name);
        check(shotValue(rules,s,{plainAmmo(rules,s,6)})==6+extra+(name=="MA053"?4:0),"Fixed/selected main-shot payload mismatch");if(name=="MA067")check(s.heat==heat-4,"Barrel Weight postimpact Heat loss");
    }
    check(shotValue(rules,s,{plainAmmo(rules,s,6)})==6,"Consumed next-shot Modifier bonus repeated");
    if(name=="MA118"){next(rules,s);check(Rules::shield(s)==24,"Brace Cannon did not deliver fresh24 after reset");}
}

struct Origin {const char* id;Rarity rarity;Materials cost;};
const Origin& originDefinition(const std::string& id){
    static const std::vector<Origin> sources={
#include "origin-rarity.inc"
    };
    for(const auto& source:sources)if(source.id==id)return source;throw std::runtime_error("Missing authored generated-origin definition: "+id);
}
Rarity originRarity(const std::string& id){return originDefinition(id).rarity;}
std::string generatedReference(const PlainCase& row){return std::string("generated:")+(row.kind==Kind::Ammo?"ammo:":"shield:")+std::to_string(row.value);}
Materials generatedBasis(const PlainCase& row){return {(row.value+5)/6,row.kind==Kind::Shield?1:0,0,0,0};}
void generatedIdentity(const Part& p,const PlainCase& row,const std::string& originalSource){
    check(p.kind==row.kind&&p.originalValue==row.value&&p.rarity==originRarity(originalSource),"Generated kind/fixed value/source rarity mismatch");
    check(p.recipe==originalSource&&p.canonicalRecipe==(originalSource.rfind("SH",0)==0||originalSource.rfind("MA",0)==0?originalSource:row.kind==Kind::Ammo?"SH001":"SH002"),"Generated source identity confused with ordinary behavior");
    check(p.materialBasis==generatedBasis(row)&&p.resaleReference==generatedReference(row),"Generated value inherited source recipe's own sale price");
    check(p.effects.size()==1&&p.effects[0].amount==row.value&&p.effects[0].op==(row.kind==Kind::Ammo?Op::FlatDamage:Op::ShieldValue)&&p.effects[0].timing==(row.kind==Kind::Ammo?Timing::Assembly:Timing::Install),"Generated part inherited source costs or secondary payload");
}
Id useSource(const Rules& rules,State& s,const std::string& id,Action a={}){
    const Id copy=memory(s,id);const auto before=s.materials;a.type=ActionType::Craft;a.subject=copy;const auto result=act(rules,s,a);
    const auto& cost=originDefinition(id).cost;check(difference(before,s.materials)==cost,"Generated-output producer did not pay its authored one Use cost");
    check(std::count_if(result.events.begin(),result.events.end(),[&](const Event& e){return e.type=="recipe_used"&&e.subject==copy&&e.source==id&&e.paid==cost;})==1,"Generated source omitted its one paid Use event");return copy;
}
Id sourcePart(const State& s,const std::string& source){for(auto it=s.parts.rbegin();it!=s.parts.rend();++it)if(it->creator==source&&it->resaleReference.rfind("generated:",0)!=0)return it->id;throw std::runtime_error("Missing source body");}
Id materializePlain(const Rules& rules,State& s,const PlainCase& row,std::string source={}){
    if(source.empty()){
        if(row.kind==Kind::Ammo)source="SH076";
        else switch(row.value){case 2:source="SH045";break;case 3:source="SH033";break;case 4:source="SH026";break;case 5:source="MA004";break;case 6:source="SH026";break;case 7:source="SH062";break;case 8:source="SH034";break;case 10:source="SH065";break;case 12:source="SH096";break;case 14:source="MA099";break;case 15:source="SH092";break;case 18:source="SH115";break;case 20:source="MA113";break;case 24:source="MA116";break;default:throw std::runtime_error("Ephemeral producer requires separate timing fixture");}
    }
    const auto before=s.nextId;
    if(source=="SH076"){
        // Controlled fixed printed Shield inputs exercise the declared full
        // conversion domain, not natural acquisition of every input amount.
        const auto inputBefore=s.nextId;auto grant=rules.grantPlainPart(s,Kind::Shield,row.value==14?24:row.value,"fixed-printed-shield-fixture");check(grant.ok,grant.reason);Id input=0;for(const auto& p:s.parts)if(p.id>=inputBefore)input=p.id;
        auto a=Action{};a.parts={input};useSource(rules,s,source,a);check(!exists(s,input),"Recasting retained its unused source Shield");
    }else if(source.rfind("MY",0)==0||source.rfind("UGS-",0)==0){
        auto acquired=rules.acquireUpgrade(s,source);check(acquired.ok,acquired.reason);auto started=rules.startUpgrades(s,EncounterClass::Regular);check(started.ok,started.reason);
        if(source=="MY1-22"){check(s.upgradeChoices.size()==1,"Drawer selection missing");Action a;a.type=ActionType::ResolveUpgradeChoice;a.upgradeChoice.choice=s.upgradeChoices.front().id;a.upgradeChoice.option=row.kind==Kind::Ammo?"assault":"defence";act(rules,s,a);}
        act(rules,s,Action::collect(0));if(source=="MY1-19"||source=="UGS-053")next(rules,s);
        if(source=="MY1-M1"){
            check(std::none_of(s.parts.begin(),s.parts.end(),[&](const Part& p){return p.creator==source;}),"Ignition Bank granted Shield before a Heat payment");
            useSource(rules,s,"MA010");activate(rules,s,sourcePart(s,"MA010"));
        }
    }else if(source=="SH126"){
        useSource(rules,s,source);activate(rules,s,sourcePart(s,source));act(rules,s,Action::endTurn());act(rules,s,Action::collect(0,2));
    }else if(source=="MA119"){
        useSource(rules,s,source);check(std::none_of(s.parts.begin(),s.parts.end(),[&](const Part& p){return p.creator==source;}),"Scheduled foundry output arrived immediately");next(rules,s);
    }else if(source=="SH045"){
        useSource(rules,s,source);fire(rules,s,sourcePart(s,source));
    }else if(source=="SH062"){
        useSource(rules,s,source);act(rules,s,Action::install(sourcePart(s,source)));fire(rules,s,granted(rules,s,"SH001"));
    }else if(source=="SH065"){
        const auto input=granted(rules,s,"SH001");Action a;a.parts={input};useSource(rules,s,source,a);check(!exists(s,input),"Emergency Shield Forge retained its Ammo payment");
    }else if(source=="SH092"||source=="SH096"||source=="MA099"){
        if(source=="MA099")s.heat=0;useSource(rules,s,source);act(rules,s,Action::install(sourcePart(s,source)));next(rules,s);
    }else if(source=="MA113"){
        s.heat=8;useSource(rules,s,source);activate(rules,s,sourcePart(s,source));check(s.heat==0,"Sleeve's generated grant did not pay all Heat");
    }else{
        useSource(rules,s,source);if(source=="SH034"||source=="SH115"||(source=="SH026"&&row.value==6))next(rules,s);
    }
    for(const auto& p:s.parts)if(p.id>=before&&p.creator==source&&p.resaleReference==generatedReference(row)){
        generatedIdentity(p,row,source);check(p.id!=0&&p.createdRound==s.round,"Generated ID/arrival age missing");
        const bool paid=source=="SH076"||source=="SH033"||(source=="SH026"&&row.value==4)||source=="SH065"||source=="MA004"||source=="MA116";
        check(p.origin==(paid?PartOrigin::Produced:PartOrigin::Granted),"Generated origin confuses paid production with later/free grant");
        check(paid?(p.sourceRecipeCopy!=0&&std::any_of(s.memory.begin(),s.memory.end(),[&](const RecipeCopy& copy){return copy.id==p.sourceRecipeCopy&&copy.recipe==source;})):p.sourceRecipeCopy==0,"Generated source recipe-copy attribution is wrong");return p.id;
    }
    throw std::runtime_error("Authored producer did not create "+std::string(row.id)+" via "+source);
}
void ephemeralOneShield(const Rules& rules,const PlainCase& row){
    for(bool terminal:{false,true}){
        auto s=quiet(rules);s.parts.clear();auto acquired=rules.acquireUpgrade(s,"UGS-028");check(acquired.ok,acquired.reason);auto started=rules.startUpgrades(s,EncounterClass::Regular);check(started.ok,started.reason);act(rules,s,Action::collect(0));granted(rules,s,"SH001");const auto hp=s.hp;
        if(terminal)for(auto& e:s.enemies)e.intent=e.pattern[0]={Move::Escape,0,0};
        else s.enemies[0].intent=s.enemies[0].pattern[0]={Move::Attack,3,1};
        const auto result=act(rules,s,Action::endTurn());Id generated=0;for(const auto& e:result.events)if(e.type=="part_created"&&e.source=="UGS-028"){check(e.amount==1,"Reserve Resonator ignored exact one reserve part");generated=e.subject;}
        check(generated!=0,"End Turn omitted ephemeral one-Shield part");
        if(terminal){generatedIdentity(part(s,generated),row,"UGS-028");check(part(s,generated).place==Place::Installed&&part(s,generated).origin==PartOrigin::Granted&&part(s,generated).sourceRecipeCopy==0&&upgradePartSaleValue(s,part(s,generated),{4,4,4,5,6})==-1,"Ephemeral grant entered saleable reserve or paid provenance");snapshot(s);reject(rules,s,Action::remove(generated));auto copy=Action::craft(memory(s,"SH118"));copy.parts={generated};reject(rules,s,copy);}
        else check(!exists(s,generated)&&s.hp==hp-2,"Ephemeral grant failed to absorb one damage before normal reset");
    }
    CampaignRules campaigns(rules,cinderwallUpgradeHooks(rules));auto c=campaignArena(campaigns);auto acquired=rules.acquireUpgrade(c.fight,"UGS-028");check(acquired.ok,acquired.reason);c.everAcquired.push_back("UGS-028");granted(rules,c.fight,"SH001");for(auto& e:c.fight.enemies){e.robotAction.clear();e.pattern={{Move::Escape,0,0}};e.intent=e.pattern.front();}
    combat(campaigns,c,Action::endTurn());check(c.phase==CityPhase::Rewards&&c.fight.parts.empty(),"Campaign escape did not clear ephemeral grant");
}
void generatedCleanup(const Rules& fights,const PlainCase& row){
    CampaignRules campaigns(fights,cinderwallUpgradeHooks(fights));
    for(bool defeat:{false,true}){
        auto c=campaignArena(campaigns);c.fight.hotBarrel=false;const auto generated=materializePlain(fights,c.fight,row);const auto original=part(c.fight,generated);
        if(defeat){c.fight.hp=1;c.fight.enemies[0].mesh=100;}
        while(c.phase==CityPhase::Fight){
            Id target=0;for(const auto& enemy:c.fight.enemies)if(!enemy.dead&&!enemy.escaped){target=enemy.id;break;}check(target!=0,"Generated cleanup fixture lost its living target");
            const auto slug=plainAmmo(fights,c.fight,500);combat(campaigns,c,Action::load({slug}));combat(campaigns,c,Action::fire(target));
        }
        check(c.phase==(defeat?CityPhase::Defeated:CityPhase::Rewards),"Generated cleanup reached wrong controlled result");
        check(c.fight.parts.empty()&&c.fight.bullet.empty()&&c.fight.deliveries.empty()&&c.fight.materials==Materials{},"Generated part or pending source delivery escaped fight-end cleanup");
        if(original.sourceRecipeCopy)check(std::any_of(c.fight.memory.begin(),c.fight.memory.end(),[&](const RecipeCopy& copy){return copy.id==original.sourceRecipeCopy;}),"Generated cleanup erased its paid source recipe memory");
        const auto bytes=serializeCampaign(c);Campaign loaded;std::string error;check(deserializeCampaign(bytes,loaded,error),error);check(serializeCampaign(loaded)==bytes,"Generated cleanup changed on campaign reload");
    }
}
void generatedReturn(const Rules& rules,const State& original,Id id,const PlainCase& row){
    if(row.kind!=Kind::Ammo)return;
    auto s=original;const auto source=part(s,id);const auto fuse=granted(rules,s,"SH036");Action a;a.parts={id};activate(rules,s,fuse,a);activate(rules,s,granted(rules,s,"SH072"));
    act(rules,s,Action::load({id}));a=fireAction(s,id);a.partChoices={{0,id}};
    if(source.rarity>Rarity::Common){reject(rules,s,a);return;}
    act(rules,s,a);check(!exists(s,id),"Generated return retained consumed original");
    check(std::none_of(s.parts.begin(),s.parts.end(),[](const Part& p){return p.creator=="SH072";}),"Generated return arrived before next turn");snapshot(s);next(rules,s);
    std::vector<Id> returned;for(const auto& p:s.parts)if(p.creator=="SH072")returned.push_back(p.id);check(returned.size()==1,"Generated return did not produce exactly one physical copy");
    const auto clone=part(s,returned.front());generatedIdentity(clone,row,source.recipe);check(clone.id!=id&&clone.origin==PartOrigin::Copied&&clone.sourceRecipeCopy==source.sourceRecipeCopy&&clone.attachments.empty()&&clone.createdRound==s.round&&clone.place==Place::Reserve,"Generated consumed copy retained bonus/history or lost provenance");
    check(upgradePartSaleValue(s,clone,{4,4,4,5,6})==price(generatedBasis(row),{4,4,4,5,6}),"Generated return repriced the original part");check(shotValue(rules,s,{clone.id})==row.value,"Generated return repeated Fuse bonus or producer payload");next(rules,s);
    check(std::none_of(s.parts.begin(),s.parts.end(),[](const Part& p){return p.creator=="SH072";}),"Generated return repeated its single-shot trigger");
}
void plainSemantics(const Rules& rules,const PlainCase& row){
    if(row.kind==Kind::Shield&&row.value==1){ephemeralOneShield(rules,row);return;}
    std::vector<std::string> sources={""};
    if(row.kind==Kind::Ammo){if(row.value==4)sources.push_back("MY1-22");if(row.value==5)sources.push_back("MY1-19");if(row.value==6)sources.push_back("MY1-03");if(row.value==10){sources.push_back("SH126");sources.push_back("MA119");}}
    else{if(row.value==4)sources.push_back("MY1-22");if(row.value==6)sources.push_back("MY1-03");if(row.value==8)sources.push_back("MY1-M1");if(row.value==10){sources.push_back("UGS-053");sources.push_back("SH126");sources.push_back("MA119");}}
    for(const auto& source:sources){auto original=quiet(rules);const auto id=materializePlain(rules,original,row,source);const auto prototype=part(original,id);const auto creator=prototype.creator;
        if(prototype.place==Place::Installed)act(rules,original,Action::remove(id));check(isUnusedPart(part(original,id)),"Untouched pure grant could not be stored unused");
        for(const auto& prices:std::vector<Materials>{{4,4,4,5,6},{5,2,3,7,11}})check(upgradePartSaleValue(original,part(original,id),prices)==price(generatedBasis(row),prices),"Generated sale repriced from source cost or later history");
        snapshot(original);auto stored=original;next(rules,stored);check(exists(stored,id)&&part(stored,id).createdRound==prototype.createdRound,"Generated reserve storage refreshed age or lost part");
        for(const auto* copier:{"SH071","SH102","SH118"}){
            auto s=original;if(row.kind==Kind::Ammo){const auto fuse=granted(rules,s,"SH036");Action a;a.parts={id};activate(rules,s,fuse,a);}
            auto a=Action::craft(memory(s,copier));a.parts={id};const auto rarity=originRarity(creator);const bool allowed=std::string(copier)=="SH118"?rarity<=Rarity::Rare:rarity==Rarity::Common&&(row.kind==Kind::Ammo||std::string(copier)=="SH102");
            if(!allowed){reject(rules,s,a);continue;}act(rules,s,a);next(rules,s);std::vector<Id> clones;for(const auto& p:s.parts)if(p.creator==copier&&p.origin==PartOrigin::Copied)clones.push_back(p.id);check(clones.size()==(std::string(copier)=="SH071"?1U:2U),"Generated copy count/clock mismatch");
            check(std::set<Id>(clones.begin(),clones.end()).size()==clones.size(),"Generated copies reused a physical instance ID");
            for(auto clone:clones){const auto copied=part(s,clone);generatedIdentity(copied,row,creator);check(copied.id!=id&&copied.sourceRecipeCopy==prototype.sourceRecipeCopy&&copied.attachments.empty()&&copied.place==Place::Reserve&&!copied.everInstalled,"Generated copy retained history/bonus or lost source-copy provenance");check(upgradePartSaleValue(s,copied,{4,4,4,5,6})==price(generatedBasis(row),{4,4,4,5,6}),"Copied generated part changed resale");}
            const auto clone=clones.front();if(row.kind==Kind::Ammo)check(shotValue(rules,s,{clone})==row.value&&!exists(s,clone),"Copied plain Ammo inherited producer's side effect or amount");
            else{const auto heat=s.heat,hp=s.hp;const auto deliveries=s.deliveries.size();act(rules,s,Action::install(clone));check(part(s,clone).shield==row.value&&s.hp==hp&&s.heat==heat&&s.deliveries.size()==deliveries,"Copied pure Shield replayed original recipe payload/payment");}
        }
        generatedReturn(rules,original,id,row);
        if(row.kind==Kind::Ammo){check(shotValue(rules,original,{id})==row.value&&!exists(original,id),"Generated Ammo's original fixed damage/consumption changed");}
        else{act(rules,original,Action::install(id));Rules::spendShield(original,Rules::shield(original,true),true);act(rules,original,Action::remove(id));check(!isUnusedPart(part(original,id))&&upgradePartSaleValue(original,part(original,id),{4,4,4,5,6})==-1,"Depleted ordinary grant regained saleability");act(rules,original,Action::install(id));check(part(original,id).shield==0,"Generated Shield refilled on reinstall");next(rules,original);check(!exists(original,id),"Used ordinary grant survived reset");}
    }
    CampaignRules campaigns(rules,cinderwallUpgradeHooks(rules));auto c=campaignArena(campaigns);c.fight.hotBarrel=false;const auto generated=materializePlain(rules,c.fight,row);const auto origin=part(c.fight,generated);if(origin.place==Place::Installed)combat(campaigns,c,Action::remove(generated));const auto credits=c.fight.credits;const auto sale=command(c,CampaignActionType::SellPart,generated);campaignAct(campaigns,c,sale);check(!exists(c.fight,generated)&&c.fight.credits==credits+price(generatedBasis(row),{4,4,4,5,6}),"Actual generated-part sale used producer ingredients");const auto saved=serializeCampaign(c);const auto replay=campaigns.apply(c,sale);check(replay.ok&&replay.replayed&&serializeCampaign(c)==saved,"Generated sale receipt paid twice");
    generatedCleanup(rules,row);
}

// Additional source clauses. Fixtures are controlled unit states, not earned runs.
void campaignPartCleanup(const std::string& recipe,bool delivered,bool defeat){
    CampaignRules campaignRules(rules,cinderwallUpgradeHooks(rules));
    auto c=campaignRules.newGame(17,"mara-delivery-contract");
    c.mayorOffers={"MY1-01","MY1-02","MY1-03"};
    const auto run=[&](CampaignAction a){
        a.runId=c.runId;a.sequence=c.nextTransaction;
        auto preview=c;const auto predicted=campaignRules.apply(preview,a);
        const auto result=campaignRules.apply(c,a);check(result.ok,result.reason);
        check(predicted.ok&&serializeCampaign(c)==serializeCampaign(preview),"Campaign commit equals isolated prediction");
        check(result.events.size()==predicted.events.size(),"Campaign event count agrees");
        for(std::size_t i=0;i<result.events.size();++i)check(eventJson(result.events[i])==eventJson(predicted.events[i]),"Campaign ordered events agree");
        return result;
    };
    CampaignAction mayor;mayor.type=CampaignActionType::ChooseMayor;mayor.choice="MY1-02";run(mayor);
    CampaignAction enter;enter.type=CampaignActionType::EnterOffer;
    for(const auto& offer:routeOffers(c.route))if(offer.kind==EncounterKind::Regular){enter.subject=offer.id;break;}
    check(enter.subject!=0,"Actual regular offer exists");run(enter);
    const auto combat=[&](const Action& a){CampaignAction action;action.type=CampaignActionType::Combat;action.combat=a;return run(action);};
    combat(Action::collect(0));c.fight.materials={100,100,100,100,100};c.fight.hotBarrel=false;c.fight.heat=4;
    for(auto& e:c.fight.enemies){e.robotAction.clear();e.intent={Move::Recover,0,0};e.pattern={e.intent};e.hp=6;e.armor=e.shield=e.tiles=e.mesh=0;}
    const auto source=memory(c.fight,recipe);const auto spent=c.fight.spentThisRound;
    combat(Action::craft(source));check(delta(c.fight.spentThisRound,spent)==printedCosts.at(recipe),"Campaign delivery recipe pays printed materials");
    check(!c.fight.deliveries.empty(),"Actual recipe schedules future parts");
    if(delivered){combat(Action::endTurn());combat(Action::collect(0));check(c.fight.parts.size()==(recipe=="MA119"?4u:2u),"Actual future parts arrive before completion");}
    if(defeat){c.fight.hp=1;c.fight.enemies[0].robotAction.clear();attack(c.fight,0,1);combat(Action::endTurn());check(c.phase==CityPhase::Defeated,"Actual enemy attack ends campaign fight");}
    else{
        while(c.phase==CityPhase::Fight){
            Id target=0;for(const auto& e:c.fight.enemies)if(!e.dead&&!e.escaped){target=e.id;break;}
            check(target!=0,"A living target remains");const auto ammoCopy=memory(c.fight,"SH001");const auto first=c.fight.nextId;
            combat(Action::craft(ammoCopy));const auto ammo=created(c.fight,first);check(ammo.size()==1,"Paid finisher creates one part");
            combat(Action::load(ammo));combat(Action::fire(target));
        }
        check(c.phase==CityPhase::Rewards,"Actual finishing shots enter reward phase");
    }
    check(c.fight.parts.empty()&&c.fight.deliveries.empty()&&c.fight.materials==Materials{},"Campaign completion clears delivered/pending parts and resources");
}

void boundaryCases(){
    extend("MA002","earlier payload Heat cannot change Fire snapshot",[]{auto s=arena();s.heat=3;fire(s,{make(s,"MA023"),make(s,"MA002")});check(s.heat==5&&s.enemies[0].burn==0,"Rivet Heat arrives too late to enable Hot Cast");});
    extend("MA006","full-strip Heat is after hit",[]{auto s=arena();s.enemies[0].shield=6;const auto result=shot(s,"MA006");const auto hit=std::find_if(result.events.begin(),result.events.end(),[](const Event& e){return e.type=="hit";});const auto gain=std::find_if(result.events.begin(),result.events.end(),[](const Event& e){return e.type=="heat";});check(hit!=result.events.end()&&gain!=result.events.end()&&hit<gain,"Heat2 receipt follows actual main impact");});
    extend("MA011","earlier payload Heat does not upgrade this shot Burn",[]{auto s=arena();s.heat=5;fire(s,{make(s,"MA023"),make(s,"MA011")});check(s.heat==7&&s.enemies[0].burn==2,"Fire Heat5 retained despite earlier Rivet payload");});
    extend("MA024","shown Drive/Weaken count, dead attacker excluded",[]{auto s=arena();attack(s,0,8,2);s.enemies[0].drive=1;s.enemies[0].weaken=2;attack(s,1,1);s.enemies[2].dead=true;s.enemies[2].hp=0;attack(s,2,100);check(part(s,plate(s,"MA024")).shield==12,"Shown14 plus1, dead100 excluded");});
    extend("MA030","sacrificed attached Ammo cannot lend main-shot bonus",[]{auto s=arena();const auto p=make(s,"SH001");Action a;a.parts={p};use(s,"MA030",a);next(s);activate(s,make(s,"MA052"),a);check(damage(shot(s,"SH001"))==13,"Compactor7 only; Rack8 lost with sacrifice");});
    extend("MA034","Spread is not Ammo and fresh grant is exactly4",[]{auto s=arena();s.heat=4;plate(s,"MA034");const auto spread=make(s,"MA041");Action a;a.spreadTargets={{spread,s.enemies[1].id}};const auto result=fire(s,{make(s,"SH001"),spread},0,a);check(amount(result,"part_created")==4&&shield(s)==12,"One Ammo plus physical Modifier passes and grants regular4");});
    extend("MA041","killed extra receives no posthit Burn",[]{auto s=arena();s.heat=4;s.enemies[1].hp=3;const auto p=make(s,"MA041");Action a;a.spreadTargets={{p,s.enemies[1].id}};fire(s,{make(s,"SH001"),p},0,a);check(s.enemies[1].dead&&s.enemies[1].burn==0,"Dead spread target is not given Burn");});
    extend("MA044","Shield payment is not an enemy-attack break",[]{auto s=arena();plate(s,"MA044");use(s,"MA037");use(s,"MA037");check(shield(s)==0,"Two paid Utilities consume all10 Shield");const auto result=end(s);check(count(result,"part_created")==0,"No restoration from planning-time depletion");});
    extend("MA052","wrong-kind sacrifice fails atomically",[]{auto s=arena();const auto shieldPart=make(s,"SH002"),press=make(s,"MA052");Action a;a.type=ActionType::Activate;a.subject=press;a.parts={shieldPart};reject(s,a);});
    extend("MA053","low-Heat branch does not require a Mark victim",[]{auto s=arena();s.heat=0;activate(s,make(s,"MA053"));check(s.heat==2&&damage(shot(s,"SH001"))==9,"Low branch resolves with no target selection");});
    extend("MA060","printed impact6 precedes first Burn tick",[]{auto s=arena();check(damage(shot(s,"MA060"))==6&&s.enemies[0].hp==9994&&s.enemies[0].burn==3,"Flat6 then Burn3 without immediate tick");});
    extend("MA061","valid chosen payment must be available on installed parts",[]{auto s=arena();make(s,"SH002");Action a;a.type=ActionType::Activate;a.subject=make(s,"MA061");a.amount=4;reject(s,a);});
    extend("MA064","multiple positive counters and zero floor",[]{auto s=arena();const auto first=memory(s,"MA119"),second=memory(s,"MA114"),ready=memory(s,"SH001");copy(s,first).cooldown=3;copy(s,second).cooldown=1;s.heat=3;use(s,"MA064");check(copy(s,first).cooldown==2&&copy(s,second).cooldown==0&&copy(s,ready).cooldown==0,"Each live counter loses1");});
    extend("MA067","full4 loss and next-round unused-bonus expiry",[]{auto s=arena();s.heat=8;activate(s,make(s,"MA067"));shot(s,"SH001");check(s.heat==4,"Four Heat lost after impact when available");s=arena();activate(s,make(s,"MA067"));next(s);check(damage(shot(s,"SH001"))==6,"No later-round Weight damage");});
    extend("MA068","uninstalled Shield cannot satisfy12",[]{auto s=arena();make(s,"SH019");make(s,"SH019");check(damage(shot(s,"MA068"))==10,"Reserve protection is not active Shield");});
    extend("MA070","legal upgraded Heat14 produces22 Shield",[]{auto s=arena();check(rules.acquireUpgrade(s,"MAU-02").ok,"Acquire Heat cap");for(int i=0;i<5;++i)use(s,"MA001");check(part(s,plate(s,"MA070")).shield==22&&s.heat==14,"Heat14 is neither capped at10 nor consumed");});
    extend("MA073","direct Utility resource cost is not reserve sacrifice",[]{auto s=arena();use(s,"MA046");check(damage(shot(s,"MA073"))==12,"HP/material payment does not trigger replacement bonus");});
    extend("MA076","ordinary Heat loss cannot meet paid5 threshold",[]{auto s=arena();s.heat=10;s.enemies[0].shield=100;Action a;a.target=s.enemies[1].id;use(s,"MA026",a);use(s,"MA026",a);check(s.heat==4,"Two ordinary losses remove6 Heat");shot(s,"MA076");check(s.enemies[0].shield==90,"No extra10 strip for unpriced cooling");});
    extend("MA081","inclusive4 Shield payment threshold",[]{auto s=arena();use(s,"MA081");plate(s);Action a;a.amount=4;activate(s,make(s,"MA061"),a);check(s.heat==3,"Own floor4 Heat1 plus Boiler Heat2");});
    extend("MA088","all distinct cooling copies cleared",[]{auto s=arena();const auto first=memory(s,"MA119"),second=memory(s,"MA114");copy(s,first).cooldown=4;copy(s,second).cooldown=1;s.heat=5;use(s,"MA088");check(copy(s,first).cooldown==0&&copy(s,second).cooldown==0,"Reset is not limited to first cooling copy");});
    extend("MA089","used removed Shield cannot pay",[]{auto s=arena();const auto p=plate(s);use(s,"MA037");remove(s,p);Action a=Action::craft(memory(s,"MA089"));a.parts={p};reject(s,a);});
    extend("MA091","victim killed by earlier Ammo support receives no copied Burn",[]{auto s=arena();s.enemies[0].hp=16;s.enemies[0].burn=5;s.enemies[1].hp=3;const auto first=make(s,"MA012"),last=make(s,"MA091");Action a;a.partTargets={{first,s.enemies[1].id},{last,s.enemies[1].id}};fire(s,{first,last},0,a);check(s.enemies[0].dead&&s.enemies[1].dead&&s.enemies[1].burn==0,"Earlier support kills chosen victim before Burn transfer starts");});
    extend("MA094","below6 Heat cannot activate",[]{auto s=arena();s.heat=5;Action a;a.type=ActionType::Activate;a.subject=make(s,"MA094");reject(s,a);});
    extend("MA099","removed source does not schedule End Turn conditional",[]{auto s=arena();const auto p=plate(s,"MA099");remove(s,p);end(s);check(shield(s)==0&&s.deliveries.empty()&&part(s,p).shield==22,"Saved source preserves its old part, not an absent End Turn trigger");});
    extend("MA101","enemy HP damage never triggers planning dispenser",[]{auto s=arena();use(s,"MA101");attack(s,0,2);const auto result=end(s);check(s.hp==998&&count(result,"part_created")==0,"Real enemy HP loss has no Shield5 response");});
    extend("MA102","remembered floor cannot lower later higher Heat",[]{auto s=arena();s.heat=3;plate(s,"MA102");use(s,"MA001");use(s,"MA001");end(s);check(s.heat==7,"Later Heat9 decays to7 above remembered3");});
    extend("MA105","single living enemy cannot provide two victims",[]{auto s=arena(1);s.enemies[0].burn=1;Action a=Action::craft(memory(s,"MA105"));a.target=s.enemies[0].id;a.targets={999};reject(s,a);});
    extend("MA106","earned future bonus expires if next-round shot omitted",[]{auto s=arena();s.enemies[0].hp=10;activate(s,make(s,"MA106"));shot(s,"SH001");next(s);next(s);check(damage(fire(s,{make(s,"SH001")},s.enemies[1].id))==6,"No delayed use beyond exactly next round");});
    extend("MA107","production Campaign clears pending and delivered Ready Plates",[]{campaignPartCleanup("MA107",false,false);campaignPartCleanup("MA107",true,false);});
    extend("MA111","surviving spread victims give no Heat and keep ordinary Mark",[]{auto s=arena();s.heat=7;s.enemies[1].shield=2;s.enemies[1].mark=8;const auto p=make(s,"MA111");fire(s,{make(s,"SH001"),p});check(s.heat==0&&s.enemies[1].hp==9999&&s.enemies[1].mark==8&&s.enemies[2].hp==9997,"Floor60percent6=3, no kills or target Mark bonus");});
    extend("MA112","fight effect survives multiple shots and next round",[]{auto s=arena();use(s,"MA112");plate(s);check(damage(shot(s,"SH001"))==9&&damage(shot(s,"SH001"))==9,"Every main shot reads current Shield once");next(s);plate(s);check(damage(shot(s,"SH001"))==9,"Fight effect persists after first round");});
    extend("MA119","production Campaign clears delivered and pending Siege parts on victory/defeat",[]{campaignPartCleanup("MA119",false,false);campaignPartCleanup("MA119",true,false);campaignPartCleanup("MA119",true,true);});
    extend("MA120","echo ignores original Mark, applies no payload, floor-halves odd original damage",[]{auto s=arena();s.heat=4;s.enemies[0].mark=20;check(damage(fire(s,{make(s,"MA120"),make(s,"MA002")}))==24,"Original base24 excludes Mark20");check(s.enemies[0].hp==9956&&s.enemies[0].burn==2,"Original main gets Mark20 and one Burn2");end(s);check(s.enemies[0].hp==9942&&s.enemies[0].burn==1,"Tick2 then echo12 with no repeated Burn payload");s=arena();check(damage(fire(s,{make(s,"MA120"),make(s,"MA005")}))==27,"Odd original27");end(s);check(s.enemies[0].hp==9960,"Echo floor13");});
}

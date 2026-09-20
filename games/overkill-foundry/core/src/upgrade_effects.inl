// Engine members. Every branch is an authored source effect, not parsed prose.
    std::string upgradeSource;
    bool inRecipeUse=false,inEnemyPhase=false;
    struct PaidEffect { Amount hp=0,heat=0;bool recipe=false; };
    std::vector<PaidEffect> paidEffects;
    static Amount ucode(const std::string& id){
        if(id.substr(0,4)=="UGS-")return std::stoi(id.substr(4));
        if(id.substr(0,4)=="MAU-")return 1000+std::stoi(id.substr(4));
        if(id.substr(0,5)=="MY1-M")return 2100+std::stoi(id.substr(5));
        if(id.substr(0,4)=="MY1-")return 2000+std::stoi(id.substr(4));
        if(id.substr(0,4)=="MY2-")return 2200+std::stoi(id.substr(4));
        if(id.substr(0,4)=="MY3-")return 2300+std::stoi(id.substr(4));return -1;
    }
    OwnedUpgrade* up(const std::string& id){return ownedUpgrade(s,id);}
    Amount count(const OwnedUpgrade& u,const std::string& key)const{return upgradeCounter(u,key);}
    void setCount(OwnedUpgrade& u,const std::string& key,Amount value,UpgradeScope scope=UpgradeScope::Fight){for(auto& c:u.counters)if(c.key==key){c.value=value;c.scope=scope;return;}u.counters.push_back({key,scope,value});}
    Amount increase(OwnedUpgrade& u,const std::string& key,Amount n=1,UpgradeScope scope=UpgradeScope::Fight){const auto v=add(count(u,key),n);setCount(u,key,v,scope);return v;}
    bool claim(OwnedUpgrade& u,const std::string& key,Amount limit=1,UpgradeScope scope=UpgradeScope::Fight){if(count(u,key)>=limit)return false;increase(u,key,1,scope);return true;}
    bool claim(const std::string& id,const std::string& key,Amount limit=1,UpgradeScope scope=UpgradeScope::Fight){auto* u=up(id);return u&&claim(*u,key,limit,scope);}
    std::vector<std::string> upgradeOrder()const{auto owned=s.upgrades;std::sort(owned.begin(),owned.end(),[](const OwnedUpgrade&a,const OwnedUpgrade&b){return a.order!=b.order?a.order<b.order:a.id<b.id;});std::vector<std::string> ids;for(const auto& u:owned)ids.push_back(u.id);return ids;}
    void clearUpgradeScope(UpgradeScope scope){for(auto& u:s.upgrades)u.counters.erase(std::remove_if(u.counters.begin(),u.counters.end(),[scope](const UpgradeCounter& c){return c.scope==scope;}),u.counters.end());}
    Amount upgradeHeal(Amount n,const std::string& source){if(s.hp<=0)return 0;const auto actual=std::min(n,s.maxHp-s.hp);s.hp=add(s.hp,actual);emit("upgrade_heal:"+source,0,0,actual);return actual;}
    void grow(Amount n,const std::string& source){s.maxHp=add(s.maxHp,n);upgradeHeal(n,source);emit("max_hp:"+source,0,0,n);}
    void credits(Amount n,const std::string& source){s.credits=add(s.credits,n);emit("upgrade_credits:"+source,0,0,n);}
    void fightBonus(const std::string& source,Amount n){bonus(source,n);for(auto& b:s.shotBonuses)if(b.source==source)b.fightLifetime=true;}
    Id namedPart(const std::string& id,const std::string& source,bool installed=false){const auto* r=rules.recipe(id);require(r,"Upgrade names an unavailable recipe.");auto p=recipePart(*r,Action{});p.creator=source;p.origin=PartOrigin::Granted;return receive(p,installed);}
    void directShield(Amount n,const std::string& source){s.protection.push_back({s.nextId++,s.nextOrder++,n});emit("active_shield:"+source,0,0,n);}
    void allDamage(Amount n){const auto ids=living();for(Id id:ids){if(s.phase==Phase::Defeat)break;enemyDamage(id,n,true);}}
    UpgradeRequest& request(const std::string& source,UpgradeRequestKind kind){UpgradeRequest r;r.id=s.nextId++;r.source=source;r.kind=kind;s.upgradeRequests.push_back(r);emit("upgrade_request:"+source,r.id);return s.upgradeRequests.back();}
    void materialChoice(const std::string& source,std::vector<Amount> values){UpgradeChoice c;c.id=s.nextId++;c.source=source;c.kind=UpgradeChoiceKind::Material;c.values=std::move(values);s.upgradeChoices.push_back(c);emit("upgrade_choice:"+source,c.id);}
    void copyChoice(const std::string& source,Amount minimum,Amount maximum,Kind kind=Kind::Utility,bool filter=false,bool numeric=false){
        UpgradeChoice c;c.id=s.nextId++;c.source=source;c.kind=UpgradeChoiceKind::RecipeCopies;c.minimum=minimum;c.maximum=maximum;c.optional=minimum==0;
        for(const auto& copy:s.memory){if(copy.storage==MemoryKind::Borrowed)continue;const auto* r=rules.recipe(copy.recipe);if(!r)continue;if(filter&&r->kind!=kind)continue;if(numeric&&r->cooldown==0)continue;
            if(source=="MY1-21"&&r->cooldown!=0)continue;if((source=="MY1-23"||source=="UGS-119")&&(r->kind!=Kind::Ammo&&r->kind!=Kind::Shield))continue;c.objects.push_back(copy.id);}
        require(c.objects.size()>=static_cast<std::size_t>(minimum),"No legal recipe selection for this upgrade.");s.upgradeChoices.push_back(c);emit("upgrade_choice:"+source,c.id);
    }
    void packChoice(const std::string& source){UpgradeChoice c;c.id=s.nextId++;c.source=source;c.kind=UpgradeChoiceKind::Pack;c.options={"assault","defence"};s.upgradeChoices.push_back(c);emit("upgrade_choice:"+source,c.id);}
    void tag(RecipeCopy& copy,const OwnedUpgrade& u,RecipeTagKind kind,Amount amount=1,Amount material=0){for(const auto& t:copy.tags)if(t.source==u.id&&t.kind==kind)return;copy.tags.push_back({u.id,u.order,kind,amount,material,0});}
    void queueUpgradeEvent(UpgradeEvent e,UpgradeContinuation after=UpgradeContinuation::None){UpgradeResolution r;r.event=std::move(e);r.sources=r.event.snapshotListeners?r.event.listeners:upgradeOrder();if(r.event.kind==UpgradeEventKind::TurnStart)std::stable_partition(r.sources.begin(),r.sources.end(),[](const std::string& id){return id!="UGS-076";});r.after=after;if(r.event.kind==UpgradeEventKind::Acquire)s.upgradeResolution.insert(s.upgradeResolution.begin(),std::move(r));else s.upgradeResolution.push_back(std::move(r));}
    void acquirePayload(OwnedUpgrade& u){const auto id=u.id;switch(ucode(id)){
        case 2002:grow(10,id);break;case 2005:case 11:grow(18,id);break;case 64:grow(14,id);break;case 70:grow(12,id);break;case 71:grow(4,id);break;case 120:grow(6,id);break;
        case 2006:{auto& r=request(id,UpgradeRequestKind::RecipeOffer);r.rarity=2;r.sharedOnly=true;r.optional=false;break;}
        case 2012:u.charges=3;break;case 2015:packChoice(id);break;
        case 15:u.charges=3;break;case 24:case 35:u.charges=5;break;case 65:case 126:case 150:u.charges=1;break;
        case 32:request(id,UpgradeRequestKind::CopyRecipe);break;
        case 39:require(s.hp>8,"Risky Calibration needs at least 9 HP.");s.hp-=8;emit("acquisition_hp_cost",0,0,8);copyChoice(id,2,2);break;
        case 46:copyChoice(id,0,3,Kind::Ammo,true);break;
        case 85:credits(100,id);break;
        case 89:{auto& r=request(id,UpgradeRequestKind::RecipeOffer);r.offers=3;break;}
        case 102:copyChoice(id,1,1,Kind::Ammo,true);break;
        case 144:copyChoice(id,2,2,Kind::Utility,true,true);break;case 145:copyChoice(id,2,2,Kind::Ammo,true,true);break;
        default:break; // Other registered sources bind a later clock or a pure query.
    }}
    void fightStartPayload(OwnedUpgrade& u){const auto id=u.id;switch(ucode(id)){
        case 1001:heat(2);break;
        case 2003:receive(plain(Kind::Ammo,6,id));shieldPart(6,id,true);break;
        case 2007:if(s.encounterClass==EncounterClass::Officer){materialNow({4,0,0,0,0});shieldPart(8,id,true);}break;
        case 2008:case 2011:materialChoice(id,ucode(id)==2008?std::vector<Amount>{0,1,2}:std::vector<Amount>{0,1,2,3,4});break;
        case 2021:copyChoice(id,1,1);break;case 2022:packChoice(id);break;case 2023:copyChoice(id,1,1);break;
        case 2101:heat(4);break;
        case 1:fightBonus(id,6);break;
        case 6:{auto ids=living();if(!ids.empty()){Id best=ids.front();for(Id n:ids)if(byId(s.enemies,n)->maxHp>byId(s.enemies,best)->maxHp)best=n;status(best,2,2);}break;}
        case 7:namedPart("SH001",id);namedPart("SH003",id);break;
        case 13:upgradeHeal(2,id);break;
        case 15:if(u.charges>0){--u.charges;setCount(u,"active",1);}break;
        case 21:{UpgradeChoice c;c.id=s.nextId++;c.source=id;c.kind=UpgradeChoiceKind::Enemy;c.objects=living();if(!c.objects.empty())s.upgradeChoices.push_back(c);break;}
        case 24:if(u.charges>0){--u.charges;materialChoice(id,{0,1,2});}break;
        case 35:if(u.charges>0)setCount(u,"active",1);break;
        case 37:allDamage(4);break;
        case 45:if(u.campaignCount)fightBonus(id,2*u.campaignCount);break;
        case 70:if(s.encounterClass==EncounterClass::Boss)grow(1,id);break;
        case 90:if(s.encounterClass==EncounterClass::Boss)upgradeHeal(12,id);break;
        case 105:for(Id n:living())status(n,3,1);break;
        case 116:u.values={static_cast<Amount>(s.rng.below(Domain::Choice,5))};emit("discount_material",u.order,0,u.values[0]);break;
        case 119:copyChoice(id,2,2);break;
        case 125:if(count(u,"awakened"))fightBonus(id,7);break;
        case 126:if(u.charges>0)setCount(u,"loan",1);break;
        case 132:if(count(u,"shipment")){setCount(u,"shipment",0,UpgradeScope::Campaign);namedPart("SH001",id);namedPart("SH002",id,true);}break;
        case 133:{auto& r=request(id,UpgradeRequestKind::RecipeOffer);r.rarity=1;r.sharedOnly=true;r.optional=false;break;}
        case 141:if(count(u,"voucher")){setCount(u,"base_voucher",2);setCount(u,"voucher",0,UpgradeScope::Campaign);}break;
        case 142:if(count(u,"voucher")){materialNow({1,0,0,0,0});setCount(u,"voucher",0,UpgradeScope::Campaign);}break;
        case 146:materialNow(u.materials);u.materials={};break;
        default:break;
    }}
    void turnStartPayload(OwnedUpgrade& u){const auto id=u.id;switch(ucode(id)){
        case 2004:if(s.round==1)materialNow({3,3,0,0,0});break;
        case 2203:if(s.round==3)namedPart("SH003",id);break;
        case 2302:if(s.round==1)directShield(6,id);break;
        case 14:if(count(u,"heal_due")==s.round){setCount(u,"heal_due",0);upgradeHeal(3,id);}break;
        case 18:if(s.round>=2)materialNow({0,1,0,0,0});break;
        case 22:if(s.round==2)materialNow({2,1,0,0,0});break;
        case 50:if(s.round%3==0)materialNow({1,1,0,0,0});break;
        case 53:if(s.round==2)shieldPart(10,id,true);break;
        case 76:if(s.round>=2)allDamage(2);break;
        case 94:if(s.round%3==0){RecipeCopy* best=nullptr;for(auto& c:s.memory)if(c.cooldown>0&&(!best||c.cooldown>best->cooldown))best=&c;if(best){--best->cooldown;emit("cooled",best->id,0,1);}}break;
        case 99:if(s.round%4==0)materialChoice(id,{0,3});break;
        case 117:if(s.round==3){fightBonus(id,4);shieldPart(4,id,true);}break;
        default:break;
    }}
    void completionPayload(OwnedUpgrade& u,const UpgradeEvent& e){const auto id=u.id;const bool win=e.kind==UpgradeEventKind::Victory;
        if((win||e.kind==UpgradeEventKind::Escaped)&&ucode(id)==35&&count(u,"active")&&u.charges>0)--u.charges;
        if((win||e.kind==UpgradeEventKind::Escaped)&&ucode(id)==126&&count(u,"loan")){u.charges=0;setCount(u,"awakened",1,UpgradeScope::Campaign);}
        if(!win)return;
        switch(ucode(id)){
        case 2:if(e.baseValue>0)credits(8,id);break;
        case 2013:if(s.encounterClass==EncounterClass::Regular&&increase(u,"victories",1,UpgradeScope::Campaign)%3==0)request(id,UpgradeRequestKind::Subscription);break;
        case 2014:setCount(u,"reward_reroll",0);break;
        case 31:{auto& r=request(id,UpgradeRequestKind::AddRewardOption);r.count=1;r.sharedOnly=true;r.normalOffer=true;break;}
        case 33:increase(u,"entitlement",1,UpgradeScope::Campaign);break;
        case 34:if(increase(u,"victories",1,UpgradeScope::Campaign)%3==0){auto& r=request(id,UpgradeRequestKind::AddRewardOption);r.count=1;r.sharedOnly=true;r.filterKind=true;r.recipeKind=Kind::Utility;}break;
        case 36:upgradeHeal(std::min(4,static_cast<Amount>(s.memory.size()/5)),id);break;
        case 45:if(s.encounterClass==EncounterClass::Officer)u.campaignCount=std::min(3,u.campaignCount+1);break;
        case 62:if(increase(u,"victories",1,UpgradeScope::Campaign)%2==0){auto& r=request(id,UpgradeRequestKind::AddRewardOption);r.count=1;r.sharedOnly=true;r.filterKind=true;r.recipeKind=Kind::Modifier;}break;
        case 65:if(u.charges>0){--u.charges;upgradeHeal(10,id);}break;
        case 72:if(!count(u,"disabled"))credits(10,id);break;
        case 73:increase(u,"entitlement",1,UpgradeScope::Campaign);break;
        case 74:if(static_cast<std::int64_t>(s.hp)*2<=s.maxHp)upgradeHeal(8,id);break;
        case 101:if(s.encounterClass==EncounterClass::Regular){auto& r=request(id,UpgradeRequestKind::AddRewardOffer);r.normalOffer=true;}break;
        case 125:if(s.encounterClass==EncounterClass::Officer&&increase(u,"officers",1,UpgradeScope::Campaign)>=3)setCount(u,"awakened",1,UpgradeScope::Campaign);break;
        case 129:grow(1,id);break;
        case 141:if(s.precisionGoodFight)setCount(u,"voucher",1,UpgradeScope::Campaign);break;
        case 142:if(s.precisionFailedFight)setCount(u,"voucher",1,UpgradeScope::Campaign);break;
        case 146:materialChoice(id,{0,1,2,3,4});break;
        case 147:if(s.encounterClass==EncounterClass::Officer){auto& r=request(id,UpgradeRequestKind::AddRewardOffer);r.rarity=3;}break;
        case 150:if(u.charges>0&&increase(u,"victories",1,UpgradeScope::Campaign)==5){--u.charges;auto& r=request(id,UpgradeRequestKind::UpgradeOffer);r.rarity=2;r.sharedOnly=true;r.optional=false;}break;
        default:break;
    }}
    void acceptedRecipePayload(OwnedUpgrade& u,const UpgradeEvent& e){if(e.origin!=AcquisitionSource::Victory)return;auto* c=byId(s.memory,e.subject);require(c,"Accepted recipe copy is missing.");const auto* r=rules.recipe(c->recipe);require(r,"Accepted recipe definition is missing.");const auto id=u.id;switch(ucode(id)){
        case 12:if(claim(u,"copy_reward")){auto& q=request(id,UpgradeRequestKind::CopyRecipe);q.recipeCopy=c->id;q.definition=c->recipe;q.reward=e.reward;}break;
        case 16:if(increase(u,"acceptances",1,UpgradeScope::Campaign)%5==0)upgradeHeal(8,id);break;
        case 40:if(r->kind==Kind::Shield&&claim(u,"shield_reward"))tag(*c,u,RecipeTagKind::Shield,2);break;
        case 41:if(r->kind==Kind::Utility)tag(*c,u,RecipeTagKind::CopperCoupon);break;
        case 69:if(claim(u,"filing_reward"))credits(6,id);break;
        case 134:if(r->kind==Kind::Shield)tag(*c,u,RecipeTagKind::IronCoupon);break;
        case 148:if(e.normalOffer&&e.lightTouch)tag(*c,u,RecipeTagKind::LightTouch);break;
        default:break;
    }}
    void otherUpgradePayload(OwnedUpgrade& u,const UpgradeEvent& e){const auto id=u.id;const auto n=ucode(id);
        if(e.kind==UpgradeEventKind::RecipeAccepted){acceptedRecipePayload(u,e);return;}
        if(e.kind==UpgradeEventKind::Purchase){if(n==72)setCount(u,"disabled",1,UpgradeScope::Campaign);if(n==73&&count(u,"entitlement")){const auto earned=count(u,"entitlement");setCount(u,"entitlement",0,UpgradeScope::Campaign);upgradeHeal(checked(static_cast<std::int64_t>(earned)*3),id);}return;}
        if(e.kind==UpgradeEventKind::CoreSold&&n==33&&count(u,"entitlement")){const auto earned=count(u,"entitlement");setCount(u,"entitlement",0,UpgradeScope::Campaign);grow(earned,id);}
        if(e.kind==UpgradeEventKind::PartSold&&n==2016&&claim(u,"sale"))credits(std::min(12,e.baseValue),id);
        if(e.kind==UpgradeEventKind::ShopRestocked&&n==131)request(id,UpgradeRequestKind::RevealReward);
        if(e.kind==UpgradeEventKind::Noncombat){if(n==97)upgradeHeal(3,id);if(n==132)setCount(u,"shipment",1,UpgradeScope::Campaign);if(n==113&&claim(u,"survey",1,UpgradeScope::City)){auto& q=request(id,UpgradeRequestKind::UpgradeOffer);q.rarity=1;}}
    }
    void drainUpgrades(){
        while(!s.upgradeResolution.empty()&&s.upgradeChoices.empty()&&s.upgradeRequests.empty()){
            auto& f=s.upgradeResolution.front();if(s.phase==Phase::Defeat){s.upgradeResolution.clear();return;}
            if(f.cursor>=static_cast<Amount>(f.sources.size())){const auto after=f.after;s.upgradeResolution.erase(s.upgradeResolution.begin());if(after==UpgradeContinuation::FirstTurn){UpgradeEvent e;e.kind=UpgradeEventKind::TurnStart;queueUpgradeEvent(e,UpgradeContinuation::Collection);}if(after==UpgradeContinuation::Collection){terminal();if(s.phase!=Phase::Victory&&s.phase!=Phase::Defeat&&s.phase!=Phase::Escaped){s.phase=Phase::Collection;commitIntents();}}if(after==UpgradeContinuation::Preparation)s.phase=Phase::Preparation;continue;}
            const auto e=f.event;const auto id=f.sources[static_cast<std::size_t>(f.cursor++)];auto* u=up(id);if(!u)continue;upgradeSource=id;emit("upgrade_trigger:"+id,u->order,0,static_cast<Amount>(e.kind));
            switch(e.kind){case UpgradeEventKind::Acquire:acquirePayload(*u);break;case UpgradeEventKind::FightStart:fightStartPayload(*u);break;case UpgradeEventKind::TurnStart:turnStartPayload(*u);break;case UpgradeEventKind::Victory:case UpgradeEventKind::Escaped:completionPayload(*u,e);break;case UpgradeEventKind::Collect:collectUpgradePayload(*u,e);break;default:otherUpgradePayload(*u,e);break;}upgradeSource.clear();
        }
    }
    void answerUpgrade(const UpgradeAnswer& a){
        require(!s.upgradeChoices.empty()&&s.upgradeChoices.front().id==a.choice,"Choose the current saved upgrade decision.");const auto c=s.upgradeChoices.front();auto* u=up(c.source);require(u,"Choice source is no longer owned.");const auto n=ucode(c.source);ScopedSource sourceScope(upgradeSource,c.source);
        if(a.decline){require(c.optional,"This upgrade requires a choice.");}
        else if(c.kind==UpgradeChoiceKind::Material){require(a.values.size()==1&&std::find(c.values.begin(),c.values.end(),a.values[0])!=c.values.end(),"Choose an offered material.");u->values=a.values;if(n==24||n==99){Materials v{};v[static_cast<std::size_t>(a.values[0])]=2;materialNow(v);}if(n==146){u->materials={};u->materials[static_cast<std::size_t>(a.values[0])]=2;}}
        else if(c.kind==UpgradeChoiceKind::Pack){require(a.option=="assault"||a.option=="defence","Choose assault or defence.");u->values={a.option=="assault"?0:1};if(n==2022)for(int i=0;i<2;++i){if(u->values[0]==0)receive(plain(Kind::Ammo,4,u->id));else shieldPart(4,u->id,true);}}
        else if(c.kind==UpgradeChoiceKind::RecipeCopies){require(a.objects.size()>=static_cast<std::size_t>(c.minimum)&&a.objects.size()<=static_cast<std::size_t>(c.maximum),"Choose the stated number of recipe copies.");std::set<Id> seen;for(Id id:a.objects)require(std::find(c.objects.begin(),c.objects.end(),id)!=c.objects.end()&&seen.insert(id).second,"Choose distinct offered recipe copies.");
            if(n==39){const auto* x=rules.recipe(byId(s.memory,a.objects[0])->recipe);const auto* y=rules.recipe(byId(s.memory,a.objects[1])->recipe);require((x->kind==Kind::Ammo&&y->kind==Kind::Shield)||(y->kind==Kind::Ammo&&x->kind==Kind::Shield),"Choose one Ammo and one Shield copy.");}
            u->recipes=a.objects;u->recipeCopy=a.objects.empty()?0:a.objects.front();for(Id id:a.objects){auto* copy=byId(s.memory,id);const auto* r=rules.recipe(copy->recipe);if(n==39)tag(*copy,*u,r->kind==Kind::Ammo?RecipeTagKind::Damage:RecipeTagKind::Shield,2);if(n==46)tag(*copy,*u,RecipeTagKind::Damage,1);if(n==144||n==145)tag(*copy,*u,RecipeTagKind::Cooldown,1);}}
        else if(c.kind==UpgradeChoiceKind::Enemy){require(a.objects.size()==1&&std::find(c.objects.begin(),c.objects.end(),a.objects[0])!=c.objects.end(),"Choose an offered enemy.");chosen(a.objects[0]);u->target=a.objects[0];}
        else if(c.kind==UpgradeChoiceKind::HaulExchange){Amount take=0,give=0;for(std::size_t i=0;i<5;++i){require(a.removed[i]>=0&&a.removed[i]<=c.baseHaul[i]&&a.removed[i]<=s.materials[i]&&a.added[i]>=0&&!(a.removed[i]&&a.added[i]),"Exchange only this haul's base units for different materials.");take=add(take,a.removed[i]);give=add(give,a.added[i]);}require(take<=3&&take==give,"Exchange up to three units one for one.");for(std::size_t i=0;i<5;++i)s.materials[i]=add(s.materials[i]-a.removed[i],a.added[i]);}
        else if(c.kind==UpgradeChoiceKind::PrecisionRetry){require(a.values.size()==1&&a.values[0]>=0&&a.values[0]<=2,"Record the retry result.");if(a.values[0]>0){Materials v{};v[static_cast<std::size_t>(c.values[0])]=a.values[0];if(s.finitePile){auto& stock=s.pile[static_cast<std::size_t>(c.values[0])];v[static_cast<std::size_t>(c.values[0])]=std::min(stock,a.values[0]);stock-=v[static_cast<std::size_t>(c.values[0])];}materialNow(v);++s.precisionGoodFight;}else ++s.precisionFailedFight;}
        s.upgradeChoices.erase(s.upgradeChoices.begin());drainUpgrades();
    }
    Materials upgradeBaseHaul(){
        Materials v{3,2,1,1,1};Amount loss=0,extra=0;
        if(s.round==1){if(up("UGS-011"))loss+=2;if(up("UGS-018"))loss+=2;if(auto* u=up("UGS-126");u&&count(*u,"loan"))loss+=2;if(auto* u=up("UGS-141"))extra=count(*u,"base_voucher");}
        require(loss<=3&&extra==0,"The authored combined base-haul composition awaits the owner decision.");
        const std::array<Amount,3> order{{0,1,0}};for(Amount i=0;i<loss;++i)--v[static_cast<std::size_t>(order[static_cast<std::size_t>(i)])];return v;
    }
    void collectUpgradePayload(OwnedUpgrade& u,const UpgradeEvent& e){const auto n=ucode(u.id);Materials v{};
        if(n==2018&&s.round<=2)v[static_cast<std::size_t>(e.steering)]=2;
        if(n==51&&s.round%5==0)v[static_cast<std::size_t>(e.steering)]=1;
        if(n==61&&s.round==1)v[static_cast<std::size_t>(e.steering)]=1;
        if(n==125&&s.round==1&&count(u,"awakened"))v[static_cast<std::size_t>(e.steering)]=1;
        if(n==96&&e.precision==0&&claim(u,"failure"))v[static_cast<std::size_t>(e.steering)]=1;
        materialNow(v);
        if(n==42&&s.round==1&&claim(u,"sort")){UpgradeChoice c;c.id=s.nextId++;c.source=u.id;c.kind=UpgradeChoiceKind::HaulExchange;c.minimum=0;c.maximum=3;c.baseHaul=e.baseHaul;c.optional=true;s.upgradeChoices.push_back(c);}
        if(n==2017&&e.precision==0&&claim(u,"retry")){UpgradeChoice c;c.id=s.nextId++;c.source=u.id;c.kind=UpgradeChoiceKind::PrecisionRetry;c.values={e.steering};c.optional=true;s.upgradeChoices.push_back(c);}
    }
    bool extraRecipeUse(const RecipeCopy& c,const Recipe& r){const auto* u=up("MY1-21");return r.cooldown==0&&s.round==1&&u&&u->recipeCopy==c.id&&c.usesThisRound<2;}
    Materials discountedCost(RecipeCopy& copy,const Recipe& r,const Action& a){Materials cost=r.cost;
        struct Source {Id order;std::string id;RecipeTag* tag;};std::vector<Source> sources;for(auto& t:copy.tags)sources.push_back({t.order,t.source,&t});for(const auto& u:s.upgrades)sources.push_back({u.order,u.id,nullptr});
        std::sort(sources.begin(),sources.end(),[](const Source& x,const Source& y){return x.order<y.order;});bool allocated=false;
        for(auto& x:sources){if(x.tag){auto& t=*x.tag;if(t.usedFight==std::max(1,s.fightSerial))continue;Amount k=-1;
            if(t.kind==RecipeTagKind::CopperCoupon)k=1;if(t.kind==RecipeTagKind::IronCoupon)k=0;if(t.kind==RecipeTagKind::Subscription)k=t.material;
            if(t.kind==RecipeTagKind::LightTouch){Amount best=0;for(Amount i=0;i<5;++i)if(cost[static_cast<std::size_t>(i)]>best){best=cost[static_cast<std::size_t>(i)];k=i;}}
            if(k>=0&&cost[static_cast<std::size_t>(k)]>0){--cost[static_cast<std::size_t>(k)];t.usedFight=std::max(1,s.fightSerial);}continue;}
            auto* u=up(x.id);if(x.id=="UGS-116"&&!u->values.empty()){const auto k=static_cast<std::size_t>(u->values[0]);if(cost[k]>0&&claim(*u,"discount",1,UpgradeScope::Round))--cost[k];}
            if(x.id=="MY1-09"&&count(*u,"paid_uses")>=3&&!count(*u,"discount")){Amount total=0;for(std::size_t i=0;i<5;++i){require(a.discount[i]>=0&&a.discount[i]<=cost[i],"Allocate only printed available material discounts.");total=add(total,a.discount[i]);}require(total<=2,"Rotating Toolhead discounts at most two units.");for(std::size_t i=0;i<5;++i)cost[i]-=a.discount[i];setCount(*u,"discount",1,UpgradeScope::Round);allocated=true;}}
        if(!allocated)for(Amount n:a.discount)require(n==0,"No allocated material discount is available.");return cost;
    }
    Amount assignedCooldown(RecipeCopy& copy,Amount cooldown){if(cooldown==0)return 0;for(auto& t:copy.tags)if(t.kind==RecipeTagKind::Cooldown&&t.usedFight!=std::max(1,s.fightSerial)){cooldown=std::max(0,cooldown-t.amount);t.usedFight=std::max(1,s.fightSerial);}return cooldown;}
    void addOutput(Id id,Amount damage,Amount shield,const std::string& source){auto* p=byId(s.parts,id);if(!p)return;p->upgradeDamage=add(p->upgradeDamage,damage);p->upgradeShield=add(p->upgradeShield,shield);if(p->everInstalled){p->shield=add(p->shield,shield);p->originalValue=add(p->originalValue,shield);}emit("output_modified:"+source,id,0,damage,shield);}
    void producedUpgrades(const RecipeCopy& copy,const Recipe& r,const Materials& paid,const std::vector<Id>& outputs){if(outputs.empty())return;const auto total=std::accumulate(paid.begin(),paid.end(),Amount{0});const Id first=outputs.front();const auto* p=byId(s.parts,first);if(!p)return;const bool damage=p->kind==Kind::Ammo,shield=p->kind==Kind::Shield;
        struct Source{Id order;std::string id;const RecipeTag* tag;};std::vector<Source> sources;for(const auto& t:copy.tags)sources.push_back({t.order,t.source,&t});for(const auto& u:s.upgrades)sources.push_back({u.order,u.id,nullptr});std::sort(sources.begin(),sources.end(),[](const Source&a,const Source&b){return a.order<b.order;});
        for(const auto& x:sources){ScopedSource sourceScope(upgradeSource,x.id);if(s.phase==Phase::Defeat)return;if(x.tag){if(total>0){if(x.tag->kind==RecipeTagKind::Damage&&damage)addOutput(first,x.tag->amount,0,x.id);if(x.tag->kind==RecipeTagKind::Shield&&shield)addOutput(first,0,x.tag->amount,x.id);}continue;}auto* u=up(x.id);const auto n=ucode(x.id);Amount d=0,h=0;
            if(n==2001&&r.kind==Kind::Ammo&&damage&&claim(*u,"output",1,UpgradeScope::Round))d=2;
            if(n==2202&&r.kind==Kind::Shield&&shield&&claim(*u,"output",1,UpgradeScope::Round))h=3;
            if(n==2015&&!u->values.empty()){if(u->values[0]==0&&damage&&claim(*u,"output",1,UpgradeScope::Round))d=2;if(u->values[0]==1&&shield&&claim(*u,"output",1,UpgradeScope::Round))h=3;}
            if(n==2023&&u->recipeCopy==copy.id&&claim(*u,"output")){if(damage)d=5;if(shield)h=7;}
            if(n==2102&&r.kind==Kind::Ammo&&s.heat>=4&&claim(*u,"output",1,UpgradeScope::Round)){d=3;heat(1);}
            if(n==9&&s.round==1&&total>0&&(damage||shield)&&claim(*u,"output",3)){if(damage)d=2;else h=2;}
            if(n==119&&total>0&&std::find(u->recipes.begin(),u->recipes.end(),copy.id)!=u->recipes.end()&&claim(*u,"copy_"+std::to_string(copy.id))){if(damage)d=2;else if(shield)h=2;}
            if(n==121&&r.id=="SH001"&&total>0&&claim(*u,"output",1,UpgradeScope::Round))d=2;
            if(n==140&&shield&&total>0&&claim(*u,"output"))h=4;
            if(d||h){addOutput(first,d,h,x.id);if(n==121)attach(*byId(s.parts,first),x.id,0,0,0);}
            if(n==15&&count(*u,"active")&&r.kind==Kind::Ammo&&total>0&&claim(*u,"copy")){auto duplicated=*byId(s.parts,first);duplicated.origin=PartOrigin::Copied;duplicated.creator=x.id;receive(duplicated);}
        }
    }
    void resolvedUseUpgrades(const RecipeCopy& copy,const Recipe& r,const Materials& paid,Amount removedCooldown,bool reachedReady){auto payments=std::move(paidEffects);paidEffects.clear();const auto total=std::accumulate(paid.begin(),paid.end(),Amount{0});for(const auto& id:upgradeOrder()){ScopedSource sourceScope(upgradeSource,id);if(s.phase==Phase::Defeat)return;auto* u=up(id);const auto n=ucode(id);for(const auto& p:payments)paidUpgradeReward(*u,p);
        if(n==2008&&total>0&&claim(*u,"refund")&&!u->values.empty()){Materials refund{};const auto k=static_cast<std::size_t>(u->values[0]);refund[k]=std::min(3,paid[k]);materialNow(refund);}
        if(n==2009&&total>0)increase(*u,"paid_uses",1,UpgradeScope::Round);
        if(n==2011&&!u->values.empty()&&paid[static_cast<std::size_t>(u->values[0])]>0&&increase(*u,"uses")==3){Materials v{};v[static_cast<std::size_t>(u->values[0])]=5;materialNow(v);}
        if(n==2201&&r.kind==Kind::Utility&&removedCooldown>0&&claim(*u,"cool",1,UpgradeScope::Round))materialNow({0,1,0,0,0});
        if(n==55&&total>=5&&claim(*u,"heavy_use",2,UpgradeScope::Round))shieldPart(3,id,true);
        if(n==66&&r.kind==Kind::Utility&&total>0&&increase(*u,"utility",1,UpgradeScope::Round)==3)allDamage(3);
        if(n==95&&(r.kind==Kind::Modifier||r.kind==Kind::Spread)&&total>0&&claim(*u,"modifier"))shieldPart(7,id,true);
        if(n==107&&r.kind==Kind::Utility&&total>0&&claim(*u,"utility",1,UpgradeScope::Round)){bonus(id,3);setCount(*u,"shot_pending",1,UpgradeScope::Round);}
        if(n==127&&r.kind==Kind::Utility&&reachedReady&&claim(*u,"cool",1,UpgradeScope::Round))shieldPart(5,id,true);
        if(n==136&&r.kind==Kind::Utility&&total>0&&increase(*u,"utility")%4==0)shieldPart(5,id,true);
    }(void)copy;}
    void paidUpgradeReward(OwnedUpgrade& u,const PaidEffect& p){ScopedSource sourceScope(upgradeSource,u.id);const auto n=ucode(u.id);if(n==2101&&s.round==1&&p.heat>=2&&claim(u,"heat"))shieldPart(8,u.id,true);if(n==2103&&p.heat>0&&increase(u,"heat",p.heat)>=6&&claim(u,"heal"))upgradeHeal(6,u.id);if(n==1005&&p.recipe&&p.hp>0&&claim(u,"hp_use",1,UpgradeScope::Round)){const auto amount=std::min(2,std::max(0,6-count(u,"healed")));increase(u,"healed",upgradeHeal(amount,u.id));}}
    void finishPaidUpgrades(){auto payments=std::move(paidEffects);paidEffects.clear();for(const auto& p:payments){if(s.phase==Phase::Defeat)return;for(const auto& id:upgradeOrder())paidUpgradeReward(*up(id),p);}}
    void paidBeforeEffect(Amount hp,Amount heatCost,Amount shieldCost,bool partUse){struct Entry{Id order;std::string id;};std::vector<Entry> entries;if(partUse)for(const auto& b:s.bindings)if(b.source=="MA081"||b.source=="MA093"||b.source=="MA101")entries.push_back({b.order,b.source});if(auto* u=up("MAU-03"))entries.push_back({u->order,u->id});std::sort(entries.begin(),entries.end(),[](const Entry&a,const Entry&b){return a.order<b.order;});for(const auto& e:entries){ScopedSource sourceScope(upgradeSource,e.id);if(e.id=="MAU-03"){if(heatCost>=2&&claim(e.id,"heat",1,UpgradeScope::Round))shieldPart(3,e.id,true);continue;}const auto clock=e.id=="MA101"?BindingClock::Round:BindingClock::Fight;auto* b=binding(e.id,clock);if(!b)continue;if(b->round!=s.round){b->round=s.round;b->count=0;}if(e.id=="MA081"&&shieldCost>=4&&b->count<2){++b->count;heat(2);}if(e.id=="MA093"&&heatCost>=3&&b->count<2){++b->count;materialLater({1,0,0,0,0},e.id);}if(e.id=="MA101"&&hp>0&&b->count<3){++b->count;shieldPart(5,e.id,true);}}}
    void recipeHealed(Amount actual){ScopedSource sourceScope(upgradeSource,"UGS-106");if(actual>0&&claim("UGS-106","heal"))upgradeHeal(3,"UGS-106");}
    Amount fireUpgradeBonus(const std::vector<Part>& parts,Id target,Amount heatAtFire){Amount flat=0;const bool saved=std::any_of(parts.begin(),parts.end(),[&](const Part&p){return p.createdRound<s.round&&isUnusedPart(p);});for(const auto& id:upgradeOrder()){ScopedSource sourceScope(upgradeSource,id);auto* u=up(id);const auto n=ucode(id);
        const auto shot=increase(*u,"shots");const auto roundShot=increase(*u,"round_shots",1,UpgradeScope::Round);
        if(n==21&&u->target==target&&claim(*u,"shot"))flat=add(flat,4);
        if(n==35&&count(*u,"active")&&shot<=2)flat=add(flat,3);
        if(n==77&&saved&&claim(*u,"saved",1,UpgradeScope::Round))flat=add(flat,3);
        if(n==93&&shot%5==0)flat=add(flat,8);
        if(n==115&&s.encounterClass==EncounterClass::Officer&&roundShot==1)flat=add(flat,3);
        if(n==139&&roundShot==1)flat=add(flat,1);
        if(n==102&&std::any_of(parts.begin(),parts.end(),[&](const Part&p){return p.sourceRecipeCopy==u->recipeCopy;})&&claim(*u,"momentum_shot",1,UpgradeScope::Round)){const auto last=count(*u,"last_round");const auto level=last==s.round-1?std::min(3,count(*u,"streak")+1):1;setCount(*u,"streak",level);setCount(*u,"last_round",s.round);flat=add(flat,2*level);}
        if(n==1007&&heatAtFire>=8&&claim(*u,"armor",1,UpgradeScope::Round))setCount(*u,"armor_this_shot",4,UpgradeScope::Round);
    }return flat;}
    Amount fireArmorBypass(){auto* u=up("MAU-07");if(!u)return 0;const auto n=count(*u,"armor_this_shot");setCount(*u,"armor_this_shot",0,UpgradeScope::Round);return n;}
    void afterFireUpgrade(const std::string& id,const std::vector<Part>& parts,Id target){ScopedSource sourceScope(upgradeSource,id);if(s.phase==Phase::Defeat)return;auto* u=up(id);const auto n=ucode(id),shots=count(*u,"shots"),roundShots=count(*u,"round_shots");
        if(n==30&&roundShots<=3)shieldPart(2,id,true);
        if(n==44&&std::any_of(parts.begin(),parts.end(),[](const Part&p){return (p.canonicalRecipe.empty()?p.recipe:p.canonicalRecipe)=="SH001";})&&claim(*u,"slug",1,UpgradeScope::Round))materialNow({1,0,0,0,0});
        if(n==60&&roundShots==3){Id chosenTarget=0;for(Id enemy:living())if(enemy!=target){chosenTarget=enemy;break;}if(!chosenTarget){auto* e=byId(s.enemies,target);if(e&&alive(*e))chosenTarget=target;}if(chosenTarget)enemyDamage(chosenTarget,5,true);}
        if(n==83&&shots%5==0)materialNow({0,1,0,0,0});
        if(n==88&&roundShots==2)shieldPart(4,id,true);
        if(n==122&&parts.size()==1&&(parts[0].canonicalRecipe.empty()?parts[0].recipe:parts[0].canonicalRecipe)=="SH001"&&claim(*u,"slug"))namedPart("SH003",id);
    }
    void beforeEnemyHit(Amount& damage){for(const auto& id:upgradeOrder()){ScopedSource sourceScope(upgradeSource,id);auto* u=up(id);if(id=="MY1-20"&&static_cast<std::int64_t>(s.hp)*2<=s.maxHp&&claim(*u,"alarm"))shieldPart(10,id,true);if(id=="UGS-071"&&claim(*u,"first_hit"))damage=std::max(0,damage-1);}}
    void playerLostUpgrades(Id attacker,Amount hp,Amount shield,bool attack,bool enemyCause){if(s.phase==Phase::Defeat)return;for(const auto& id:upgradeOrder()){ScopedSource sourceScope(upgradeSource,id);if(s.phase==Phase::Defeat)return;auto* u=up(id);const auto n=ucode(id);
        if(n==14&&enemyCause&&attacker!=0&&hp>0&&claim(*u,"hit"))setCount(*u,"heal_due",s.round+1);
        if(n==25&&enemyCause&&hp>0&&claim(*u,"loss"))materialNow({1,1,0,0,0});
        if(n==19&&attack&&hp+shield>0){auto* e=byId(s.enemies,attacker);if(e&&alive(*e))enemyDamage(attacker,2,true);}
    }}
    void enemyHitUpgrades(Id enemy,Amount armorReduction,bool killed){if(s.phase==Phase::Defeat)return;for(const auto& id:upgradeOrder()){ScopedSource sourceScope(upgradeSource,id);auto* u=up(id);if(id=="UGS-049"&&armorReduction>0&&!killed&&claim(*u,"armor",1,UpgradeScope::Round))status(enemy,2,2);if(id=="UGS-048"&&killed&&!inEnemyPhase&&claim(*u,"kill",1,UpgradeScope::Round))materialNow({1,0,1,0,0});}}
    void enemyDebuffUpgrade(RobotStatus type){auto* u=up("UGS-029");if(!u)return;ScopedSource sourceScope(upgradeSource,u->id);const auto key=std::to_string(static_cast<Amount>(type));if(std::find(u->seen.begin(),u->seen.end(),key)!=u->seen.end())return;u->seen.push_back(key);if(claim(*u,"debuff",3))grow(1,u->id);}
    void sacrificedUpgrades(){for(const auto& id:upgradeOrder()){ScopedSource sourceScope(upgradeSource,id);if(s.phase==Phase::Defeat)return;auto* u=up(id);if(id=="UGS-038"&&claim(*u,"sacrifice",2,UpgradeScope::Round)){const auto ids=living();if(!ids.empty())enemyDamage(ids.front(),3,true);}if(id=="UGS-056"&&increase(*u,"sacrifices")%3==0)materialNow({0,0,0,1,0});}}
    bool keepLoadedUpgrade(){return !s.bullet.empty()&&claim("UGS-108","keep_loaded");}
    void endTurnUpgrade(const std::string& id){ScopedSource sourceScope(upgradeSource,id);auto* u=up(id);if(!u||s.phase==Phase::Defeat)return;const auto n=ucode(id),shots=count(*u,"round_shots");Amount reserve=0,materials=0;for(const auto& p:s.parts)if(p.place==Place::Reserve)++reserve;for(Amount v:s.materials)materials=add(materials,v);
        if(n==2019&&materials>=3){auto p=plain(Kind::Ammo,5,id);copyLater(p,1,id);}
        if(n==4&&claim(*u,"first_end")&&std::none_of(s.parts.begin(),s.parts.end(),[](const Part&p){return p.place==Place::Installed&&p.kind==Kind::Shield;}))shieldPart(4,id,true);
        if(n==5&&shots==0)materialLater({2,0,1,0,0},id);
        if(n==10&&reserve==0)shieldPart(5,id,true);
        if(n==28&&reserve>0)shieldPart(std::min(7,reserve),id,true);
        if(n==47&&s.round<=2)shieldPart(3,id,true);
        if(n==86&&Rules::shield(s)==0)shieldPart(5,id,true);
        if(n==87&&shots==0&&Rules::shield(s)==0)shieldPart(4,id,true);
        if(n==109&&shots==0)shieldPart(4,id,true);
        if(n==91&&Rules::shield(s)>=10){const auto ids=living();if(!ids.empty()){Id best=ids.front();for(Id candidate:ids)if(byId(s.enemies,candidate)->hp<byId(s.enemies,best)->hp)best=candidate;enemyDamage(best,5,true);}}
    }
    void equipmentAction(const Action& a){require(a.upgrade=="MY1-10"&&up(a.upgrade),"This upgrade has no preparation equipment action.");auto* c=byId(s.memory,a.subject);require(c&&c->cooldown>0,"Select a cooling recipe copy.");require(claim(a.upgrade,"cool"),"Cooling Authorization is already spent this fight.");const auto n=c->cooldown;c->cooldown=0;emit("cooled",c->id,0,n);}

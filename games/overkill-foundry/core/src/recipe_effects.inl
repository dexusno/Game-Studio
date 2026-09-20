// Member functions of Engine. Explicit source-bound behavior; no prose interpretation.
    static Amount code(const std::string& id) {
        if(id.size()!=5 || (id.substr(0,2)!="SH" && id.substr(0,2)!="MA"))return 0;
        Amount n=0;for(std::size_t i=2;i<5;++i){if(id[i]<'0'||id[i]>'9')return 0;n=n*10+id[i]-'0';}
        return n+(id[0]=='M'?1000:0);
    }
    static bool catalogue(const std::vector<Effect>& es){return std::any_of(es.begin(),es.end(),[](const Effect& f){return f.op==Op::Catalogue;});}
    Binding* binding(const std::string& source,BindingClock clock,Id part=0) {
        for(auto& b:s.bindings)if(b.source==source && b.clock==clock && b.part==part)return &b;return nullptr;
    }
    Binding& bind(const std::string& source,BindingClock clock,Amount amount=0,Id target=0,Id part=0) {
        if(auto* b=binding(source,clock,part)){b->amount=amount;b->target=target;if(clock!=BindingClock::Fight)b->round=s.round;return *b;}
        Binding b;b.id=s.nextId++;b.order=s.nextOrder++;b.source=source;b.clock=clock;b.amount=amount;b.target=target;b.part=part;b.round=s.round;s.bindings.push_back(b);return s.bindings.back();
    }
    bool active(const std::string& source,BindingClock clock=BindingClock::Fight){return binding(source,clock)!=nullptr;}
    std::vector<Id> living() const {std::vector<Id> ids;for(const auto& e:s.enemies)if(alive(e))ids.push_back(e.id);return ids;}
    Amount attackers()const {Amount n=0;for(const auto& e:s.enemies)if(alive(e)&&e.intent.move==Move::Attack)++n;return n;}
    Amount intentDamage()const {Amount n=0;for(const auto& e:s.enemies)if(alive(e)&&e.intent.move==Move::Attack)n=add(n,checked(static_cast<std::int64_t>(std::max(0,e.intent.damage+e.drive-e.weaken))*e.intent.hits));return n;}
    Enemy& chosen(Id id){auto* e=byId(s.enemies,id);require(e&&alive(*e),"Choose a living enemy.");return *e;}
    void status(Id id,Amount kind,Amount n) {
        if(n<=0)return;auto* e=byId(s.enemies,id);if(!e||!alive(*e))return;
        Amount* value=kind==0?&e->burn:kind==1?&e->corrosion:kind==2?&e->mark:&e->weaken;*value=add(*value,n);
        static const char* names[]={"burn_applied","corrosion_applied","mark_applied","weaken_applied"};emit(names[kind],0,id,n);
    }
    void loseHeat(Amount n){const Amount actual=std::min(s.heat,n);s.heat-=actual;emit("heat_lost",0,0,actual);}
    void cooling(Amount n){for(auto& c:s.memory){const auto take=std::min(c.cooldown,n);c.cooldown-=take;if(take)emit("cooled",c.id,0,take);}}
    void payment(Amount hp,Amount heatCost,Amount shieldCost,bool partUse,Part* part=nullptr) {
        require(s.hp>hp,"Own HP cost must leave at least 1 HP.");require(s.heat>=heatCost,"Not enough Heat.");require(Rules::shield(s,true)>=shieldCost,"Not enough installed Shield.");
        s.hp-=hp;s.hpPaidFight=add(s.hpPaidFight,hp);s.hpLostRound=add(s.hpLostRound,hp);s.heat-=heatCost;s.heatPaidRound=add(s.heatPaidRound,heatCost);
        if(part){part->paidHp=hp;part->paidHeat=heatCost;}
        if(hp)emit("hp_cost",part?part->id:0,0,hp);if(heatCost)emit("heat_cost",part?part->id:0,0,heatCost);
        if(shieldCost){Rules::spendShield(s,shieldCost,true);emit("shield_cost",part?part->id:0,0,shieldCost);}
        if(partUse&&shieldCost>=4)if(auto* b=binding("MA081",BindingClock::Fight)){if(b->round!=s.round){b->round=s.round;b->count=0;}if(b->count<2){++b->count;heat(2);}}
        if(partUse){s.partHpPaidFight=add(s.partHpPaidFight,hp);s.partHeatPaidRound=add(s.partHeatPaidRound,heatCost);}
        if(partUse&&heatCost>=3)if(auto* b=binding("MA093",BindingClock::Fight)){if(b->round!=s.round){b->round=s.round;b->count=0;}if(b->count<2){++b->count;materialLater({1,0,0,0,0},"MA093");}}
        if(partUse&&hp)if(auto* b=binding("MA101",BindingClock::Round)){if(b->count<3){++b->count;shieldPart(5,"MA101",true);}}
    }
    void materialNow(Materials values){for(std::size_t i=0;i<5;++i)if(values[i]){s.materials[i]=add(s.materials[i],values[i]);emit("material_granted",0,0,values[i],static_cast<Amount>(i));}}
    void materialLater(Materials values,const std::string& source){Delivery d;d.id=s.nextId++;d.source=source;d.dueRound=s.round+1;d.kind=DeliveryKind::Material;d.materials=values;s.deliveries.push_back(d);emit("delivery_scheduled",d.id);}
    void specialLater(const std::string& source,Amount amount=0,Id target=0,Amount detail=0){Delivery d;d.id=s.nextId++;d.source=source;d.dueRound=s.round+1;d.kind=DeliveryKind::Custom;d.amount=amount;d.target=target;d.detail=detail;s.deliveries.push_back(d);emit("delivery_scheduled",d.id);}
    Part plain(Kind kind,Amount value,const std::string& source,const std::string& output="") {
        Part p;p.recipe=source;p.kind=kind;p.output=output.empty()?(kind==Kind::Shield?"Shield":"Slug"):output;p.originalValue=value;
        p.resaleReference=std::string("generated:")+(kind==Kind::Shield?"shield:":"ammo:")+std::to_string(value);
        p.effects={{kind==Kind::Shield?Op::ShieldValue:Op::FlatDamage,kind==Kind::Shield?Timing::Install:Timing::Assembly,value,0}};
        if(const auto* r=rules.recipe(source))p.rarity=r->rarity;
        p.materialBasis[0]=checked((static_cast<std::int64_t>(value)+5)/6);if(kind==Kind::Shield&&value>0)p.materialBasis[1]=1;return p;
    }
    Id receive(Part p,bool installed=false){p.id=s.nextId++;p.createdRound=s.round;p.place=Place::Reserve;p.everInstalled=false;p.firstInstallRound=0;p.bindingOrder=0;p.installOrder=0;p.shield=0;p.paidHeat=p.paidHp=p.hookUses=0;p.attachments.clear();p.reservedBy=0;s.parts.push_back(p);emit("part_created",p.id,0,p.originalValue);if(installed)install(p.id);return p.id;}
    void copyLater(const Part& p,Amount count,const std::string& source){Delivery d;d.id=s.nextId++;d.source=source;d.dueRound=s.round+1;d.kind=DeliveryKind::PartCopy;for(Amount i=0;i<count;++i)d.parts.push_back(p);s.deliveries.push_back(d);emit("delivery_scheduled",d.id);}
    static bool unused(const Part& p){
        if(!p.everInstalled)return true;
        return p.kind==Kind::Shield&&p.effects.size()==1&&p.effects[0].op==Op::ShieldValue&&p.shield==p.originalValue&&p.paidHeat==0&&p.paidHp==0&&p.attachments.empty();
    }
    bool installedBinding(const Binding& b){const auto* p=byId(s.parts,b.part);return p&&p->place==Place::Installed&&p->firstInstallRound==s.round;}
    Part sacrifice(Id id,Kind kind=Kind::Utility){auto* p=byId(s.parts,id);require(p&&p->place==Place::Reserve&&unused(*p),"Choose an unused reserve part.");require(kind==Kind::Utility||p->kind==kind,"The sacrificed part has the wrong kind.");const auto copy=*p;s.parts.erase(std::remove_if(s.parts.begin(),s.parts.end(),[id](const Part& v){return v.id==id;}),s.parts.end());s.sacrificesRound=add(s.sacrificesRound,1);emit("part_sacrificed",id);return copy;}
    Part& reserveChoice(const Action& a,Kind kind=Kind::Utility){require(a.parts.size()==1,"Choose exactly one reserve part.");auto* p=byId(s.parts,a.parts[0]);require(p&&p->place==Place::Reserve&&unused(*p),"Choose an unused reserve part.");require(kind==Kind::Utility||p->kind==kind,"The selected part has the wrong kind.");return *p;}
    void attach(Part& p,const std::string& source,Amount damage,Amount heatGain,Amount round){auto it=std::find_if(p.attachments.begin(),p.attachments.end(),[&](const Attachment& b){return b.source==source;});Attachment b{source,damage,heatGain,round};if(it==p.attachments.end())p.attachments.push_back(b);else *it=b;}
    void bonus(const std::string& source,Amount flat=0,Amount percent=0){auto it=std::find_if(s.shotBonuses.begin(),s.shotBonuses.end(),[&](const ShotBonus& b){return b.source==source;});if(it==s.shotBonuses.end())s.shotBonuses.push_back({source,flat,percent});else *it={source,flat,percent};}
    void checkMaterials(const std::vector<Amount>& choices,std::size_t count,Amount max=4,bool different=false){require(choices.size()==count,"Choose the printed number of materials.");std::set<Amount> seen;for(Amount x:choices){require(x>=0&&x<=max,"Invalid material choice.");require(!different||seen.insert(x).second,"Choose different materials.");}}
    void craftOptions(const Recipe& r,const Action& a){
        switch(code(r.id)){
        case 80:case 119:checkMaterials(a.choices,2,4,true);break;
        case 122:checkMaterials(a.choices,2,3,true);break;
        case 106:case 108:checkMaterials(a.choices,1);break;
        case 121:checkMaterials(a.choices,1,3);break;
        default:break;
        }
    }
    Amount fixedShield(const Part& p){
        for(const auto& f:p.effects)if(f.op==Op::ShieldValue)return f.amount;
        switch(code(p.recipe)){
        case 20:return 6;case 22:return 6;case 25:return 5;case 27:return 4;case 58:return 9;case 60:return 16;case 62:return 6;case 63:return 8;case 91:return 24;case 92:return 15;case 94:return 16;case 96:return 12;case 97:return 14;case 113:return 38;case 114:return 25;
        case 1014:return 5;case 1015:return 8;case 1016:return 5;case 1018:return 6;case 1022:return 12;case 1034:return 8;case 1040:return 7;case 1044:return 10;case 1048:return 10;case 1054:return 18;case 1055:return 10;case 1057:return 8;case 1059:return 7;case 1065:return 10;case 1071:return 6;case 1072:return 9;case 1075:return 18;case 1079:return 12;case 1086:return 12;case 1099:return 22;case 1102:return 16;case 1108:return 18;case 1115:return 24;
        default: return -1;}
    }
    void catalogueUtility(const Recipe& r,const Action& a){
        const auto c=code(r.id);const auto source=r.id;
        switch(c){
        case 15:chosen(a.target);status(a.target,2,5);break;
        case 29:materialNow({0,1,0,0,0});break;case 30:materialNow({0,0,0,1,0});break;case 31:materialNow({1,0,0,0,0});break;
        case 32:{const auto p=reserveChoice(a);sacrifice(p.id);materialLater({1,0,0,1,0},source);break;}
        case 33:s.burn=std::max(0,s.burn-2);s.corrosion=std::max(0,s.corrosion-2);shieldPart(3,source,true);break;
        case 34:schedule(DeliveryKind::ShieldPart,8,source);break;
        case 49:chosen(a.target);for(Id id:living())status(id,2,id==a.target?4:2);break;
        case 57:shieldPart(8,source,true);if(s.materials[0]>=3)schedule(DeliveryKind::ShieldPart,5,source);break;
        case 65:{const auto p=reserveChoice(a,Kind::Ammo);sacrifice(p.id,Kind::Ammo);shieldPart(10,source,true);materialLater({1,0,0,0,0},source);break;}
        case 67:cooling(2);break;case 68:case 100:cooling(1);break;
        case 69:{checkMaterials(a.choices,1,3);require(a.choices[0]>0,"Choose Copper, Carbon or Glass.");Materials m{};m[static_cast<std::size_t>(a.choices[0])]=2;materialLater(m,source);break;}
        case 71:case 102:case 118:{const auto p=reserveChoice(a);require(p.kind==Kind::Ammo||(c!=71&&p.kind==Kind::Shield),"This recipe copies Ammo or eligible Shield only.");require(c==118?p.rarity<=Rarity::Rare:p.rarity==Rarity::Common,"The part rarity is not eligible.");copyLater(p,c==71?1:2,source);break;}
        case 73:s.weaken=std::max(0,s.weaken-6);s.mark=std::max(0,s.mark-6);bonus(source,3);break;
        case 76:{const auto p=reserveChoice(a,Kind::Shield);const Amount value=fixedShield(p);require(value>=0,"Choose a Shield part with a printed fixed Shield gain.");sacrifice(p.id,Kind::Shield);receive(plain(Kind::Ammo,std::min(14,value),source,"Recast Plate Slug"));break;}
        case 99:case 116:cooling(std::numeric_limits<Amount>::max());break;
        case 101:{require(a.parts.size()<=2,"Sacrifice at most two parts.");checkMaterials(a.choices,a.parts.size(),3);for(std::size_t i=0;i<a.parts.size();++i){sacrifice(a.parts[i]);Materials m{};m[static_cast<std::size_t>(a.choices[i])]=2;materialLater(m,source);}break;}
        case 104:{const Amount n=std::min({8,s.maxHp-s.hp,16-s.carefulHealing});s.hp+=n;s.carefulHealing+=n;emit("heal",0,0,n);if(attackers()==0)shieldPart(8,source,true);break;}
        case 115:{s.deliveries.erase(std::remove_if(s.deliveries.begin(),s.deliveries.end(),[&](const Delivery& d){return d.source==source;}),s.deliveries.end());for(Amount i=1;i<=3;++i){schedule(DeliveryKind::ShieldPart,18,source);s.deliveries.back().dueRound=s.round+i;}break;}
        case 117:materialLater({2,2,2,2,0},source);break;
        case 1021:materialLater({0,0,2,0,0},source);break;
        case 1026:chosen(a.target);loseHeat(3);status(a.target,3,5);break;
        case 1029:chosen(a.target);status(a.target,2,3);if(s.heat>=6)status(a.target,3,2);break;
        case 1030:{auto& p=reserveChoice(a,Kind::Ammo);attach(p,source,8,0,s.round+1);break;}
        case 1035:payment(0,2,0,false);cooling(1);break;
        case 1037:payment(0,0,5,false);heat(4);break;
        case 1043:{Amount count=0;for(Id id:living())if(byId(s.enemies,id)->burn>0){status(id,0,3);++count;}heat(std::min(3,count));break;}
        case 1046:payment(2,0,0,false);heat(2);materialLater({0,0,3,0,0},source);break;
        case 1049:{auto& e=chosen(a.target);require(e.burn>0,"Choose an enemy with Burn.");status(e.id,3,std::min(8,e.burn));break;}
        case 1050:{auto& e=chosen(a.target);if(e.intent.move==Move::Attack)status(e.id,3,6);else{status(e.id,2,7);status(e.id,0,2);}break;}
        case 1056:case 1081:case 1090:case 1093:case 1110:case 1112:bind(source,BindingClock::Fight);break;
        case 1058:payment(0,5,0,false);for(Id id:living())if(s.phase!=Phase::Defeat)enemyDamage(id,6,true);terminal();break;
        case 1064:payment(0,3,0,false);cooling(1);break;
        case 1066:{auto& p=reserveChoice(a);attach(p,source,0,4,s.round+1);break;}
        case 1069:{auto& b=bind(source,BindingClock::Round);b.seen.clear();break;}
        case 1074:chosen(a.target);payment(0,4,0,false);status(a.target,0,7);break;
        case 1080:payment(0,4,0,false);schedule(DeliveryKind::Heat,6,source);materialLater({0,1,0,0,0},source);break;
        case 1085:payment(0,6,0,false);for(Id id:living()){status(id,0,5);status(id,3,4);}break;
        case 1087:{chosen(a.target);const auto n=s.heat;require(n>=1,"Level Pressure requires Heat.");payment(0,n,0,false);status(a.target,3,std::min(16,n*2));status(a.target,2,std::min(8,n));break;}
        case 1088:payment(0,5,0,false);cooling(std::numeric_limits<Amount>::max());break;
        case 1089:{const auto p=reserveChoice(a,Kind::Shield);sacrifice(p.id,Kind::Shield);materialLater({3,0,0,1,0},source);break;}
        case 1092:payment(5,0,0,false);heat(10);schedule(DeliveryKind::ShieldPart,8,source);break;
        case 1095:chosen(a.target);require(s.heat==0,"Cold Sight requires zero Heat.");status(a.target,3,12);status(a.target,2,8);break;
        case 1100:{auto& e=chosen(a.target);require(e.intent.move==Move::Attack,"Choose an attacking enemy.");auto& b=bind(source,BindingClock::Round,0,e.id,e.id);(void)b;break;}
        case 1101:bind(source,BindingClock::Round);break;
        case 1105:{require(a.targets.size()==1,"Choose a second enemy.");auto& from=chosen(a.target);chosen(a.targets[0]);require(a.target!=a.targets[0]&&from.burn>0,"Transfer requires different enemies and Burn.");const auto n=std::min(8,from.burn);from.burn-=n;status(a.targets[0],0,2*n);break;}
        case 1107:payment(0,4,0,false);copyLater(plain(Kind::Shield,8,source,"Ready Plate"),2,source);break;
        case 1114:payment(0,6,0,false);cooling(2);break;
        case 1116:payment(6,0,0,false);heat(6);shieldPart(24,source,true);bonus(source,18);break;
        case 1117:{const auto& e=chosen(a.target);require(e.burn>0,"Choose an enemy with Burn.");status(e.id,0,std::min(12,e.burn));for(Id id:living())if(id!=a.target)status(id,0,4);break;}
        case 1119:copyLater(plain(Kind::Ammo,10,source,"Siege Slug"),2,source);copyLater(plain(Kind::Shield,10,source,"Siege Plate"),2,source);break;
        default:throw Invalid("No Utility behavior for this catalogue recipe.");
        }
    }
    Amount catalogueShield(Part& p,const Action& a){
        const auto c=code(p.recipe);Amount value=0;
        switch(c){
        case 19:value=7+(attackers()>=2?3:0);break;
        case 20:value=6;break;case 21:value=attackers()==1?12:7;break;case 22:value=6;break;
        case 23:value=std::min(10,5+s.spentThisRound[1]);break;case 25:value=5;break;case 27:value=4;break;
        case 28:value=std::min(12,checked(static_cast<std::int64_t>(living().size())*3));break;
        case 58:value=9;break;case 59:value=s.hp<=s.maxHp/2?13:7;break;case 60:value=16;break;
        case 61:{Amount n=0;for(const auto& e:s.enemies)if(alive(e)&&e.mark>0)++n;value=std::min(16,7+n*3);break;}
        case 62:value=6;break;case 63:value=8;break;case 91:value=24;break;case 92:value=15;break;
        case 93:value=std::min(30,intentDamage()/2);break;case 94:value=16;break;case 96:value=12;break;
        case 97:value=14;break;case 98:value=10+std::min(3,s.round-p.createdRound)*7;break;
        case 113:value=38;break;case 114:value=25;break;
        case 1013:{bool burning=false;for(const auto& e:s.enemies)if(alive(e)&&e.burn>0)burning=true;value=7+(burning?4:0);break;}
        case 1014:value=5;break;case 1015:value=8;break;case 1016:value=5;break;case 1018:value=6;break;
        case 1022:{require(a.sacrifices.size()==1,"Welding Clamp needs one unused Ammo part.");sacrifice(a.sacrifices[0],Kind::Ammo);value=12;break;}
        case 1024:value=9+(intentDamage()>=15?3:0);break;case 1028:value=5+std::min(3,s.firstInstallsRound)*2;break;
        case 1031:{Amount n=0;for(const auto& e:s.enemies)if(alive(e)&&e.burn>0)++n;value=std::min(12,4*n);break;}
        case 1034:value=8;break;case 1040:payment(0,4,0,true,&p);value=7;break;case 1044:value=10;break;case 1048:value=10;break;
        case 1054:{const bool lost=s.hpLostRound>0;payment(3,0,0,true,&p);if(lost)heat(2);value=18;break;}
        case 1055:value=10;break;case 1057:value=8;break;case 1059:value=7;break;
        case 1062:value=s.heat==0?16:6;if(s.heat>0)loseHeat(2);break;case 1065:value=10;break;case 1070:value=8+s.heat;break;
        case 1071:value=6;break;case 1072:value=9;break;case 1075:require(s.firstInstallsRound>0,"Install another Shield part first this round.");value=18;break;
        case 1079:value=12;break;case 1083:value=18;if(a.amount){require(a.amount==s.heat,"Flood Door pays all current Heat or none.");payment(0,a.amount,0,true,&p);value+=2*a.amount;}break;
        case 1086:{auto& e=chosen(a.target);require(e.intent.move==Move::Attack,"Officer Brace needs an attacking enemy.");value=12;break;}
        case 1096:require(a.amount>=3&&a.amount<=8,"Choose 3 to 8 Heat.");payment(0,a.amount,0,true,&p);value=6+3*a.amount;break;
        case 1099:value=22;break;case 1102:value=16;break;case 1104:value=12+(s.previousEnemyAttackHp>0?14:0);break;
        case 1108:value=18;break;case 1115:value=24;break;
        default:throw Invalid("No Shield behavior for this catalogue recipe.");
        }
        return value;
    }
    void catalogueInstalled(const Part& p,const Action& a){
        const auto c=code(p.recipe);
        switch(c){
        case 22:s.burn=std::max(0,s.burn-3);break;case 63:s.corrosion=std::max(0,s.corrosion-5);break;
        case 96:schedule(DeliveryKind::ShieldPart,12,p.recipe);break;case 1014:heat(2);break;case 1016:schedule(DeliveryKind::ShieldPart,5,p.recipe);break;
        case 1040:schedule(DeliveryKind::ShieldPart,7,p.recipe);break;
        case 1059:bind(p.recipe,BindingClock::Round,5,0,p.id);break;
        case 1079:specialLater(p.recipe,s.partHeatPaidRound);break;
        case 1086:{auto& b=bind(p.recipe,BindingClock::Round,50,a.target,p.id);(void)b;break;}
        case 1102:bind(p.recipe,BindingClock::Round,std::min(8,s.heat),0,p.id);break;
        default:break;
        }
    }
    void catalogueActivate(const Part& p,const Action& a){
        const auto c=code(p.recipe);const auto source=p.recipe;
        if(p.kind==Kind::Magnet){auto& b=bind(source,BindingClock::Collection);b.round=s.round+1;b.choices=p.choices;return;}
        switch(c){
        case 24:bind(source,BindingClock::Round,6);break;
        case 35:bind(source,BindingClock::Round,4);break;
        case 36:{auto& part=reserveChoice(a,Kind::Ammo);attach(part,source,4,0,0);break;}
        case 43:bind(source,BindingClock::Shot,12);break;
        case 64:bind(source,BindingClock::Round,1);shieldPart(5,source,true);break;
        case 66:bind(source,BindingClock::Round);break;
        case 70:specialLater(source);break;
        case 72:case 83:case 111:bind(source,BindingClock::Shot);if(c==111)bonus(source,0,100);break;
        case 85:{require(a.amount>=0&&a.amount<=15,"Choose 0 to 15 installed Shield.");payment(0,0,a.amount,true);bonus(source,2*a.amount);break;}
        case 95:{Amount n=0;for(Id id:living()){auto* e=byId(s.enemies,id);if(e->intent.move==Move::Attack)status(id,3,8);else ++n;}shieldPart(std::min(15,n*5),source,true);break;}
        case 103:bind(source,BindingClock::Round,8);break;
        case 1010:payment(0,3,0,true);bonus(source,10);break;
        case 1027:payment(0,0,6,true);bonus(source,12);break;
        case 1045:require(a.amount>=2&&a.amount<=5,"Choose 2 to 5 Heat.");payment(0,a.amount,0,true);bonus(source,3*a.amount);break;
        case 1052:{require(!a.parts.empty()&&a.parts.size()<=3,"Consume 1 to 3 unused Ammo parts.");for(Id id:a.parts)sacrifice(id,Kind::Ammo);bonus(source,static_cast<Amount>(a.parts.size())*7);break;}
        case 1053:if(s.heat>=6){chosen(a.target);bonus(source,8);status(a.target,2,4);}else{heat(2);bonus(source,3);}break;
        case 1061:require(a.amount>=4&&a.amount<=12,"Choose 4 to 12 installed Shield.");payment(0,0,a.amount,true);bonus(source,2*a.amount);heat(a.amount/4);break;
        case 1063:case 1078:case 1098:bind(source,BindingClock::Shot);break;
        case 1067:for(Id id:s.bullet){const auto* part=byId(s.parts,id);require(part&&part->kind!=Kind::Spread,"Barrel Weight cannot combine with spreading parts.");}bonus(source,18);bind(source,BindingClock::Shot);break;
        case 1094:payment(0,6,0,true);bind(source,BindingClock::Shot);break;
        case 1106:bonus(source,8);bind(source,BindingClock::Shot);break;
        case 1113:{require(s.heat>=8,"Winter Core Sleeve requires at least 8 Heat.");const auto cost=s.heat;payment(0,cost,0,true);bonus(source,30);shieldPart(20,source,true);break;}
        case 1118:payment(0,0,12,true);bonus(source,28);schedule(DeliveryKind::ShieldPart,24,source);break;
        default:throw Invalid("No planning behavior for this catalogue recipe.");
        }
    }
    Amount ammoFlat(const Part& p,const Enemy& target,Amount count,Amount distinct,Amount materialTypes,Amount heatAtFire){
        const auto c=code(p.recipe);const auto age=std::min(3,s.round-p.createdRound);
        switch(c){
        case 9:return 8+(count>=4?2:0);case 10:return target.shield>0?12:8;case 11:return 3;case 12:return 7+(target.hp<=target.maxHp/4?5:0);
        case 14:return 2;case 16:return 5;case 17:return 5;case 18:return std::min(12,2*(count-1));
        case 41:return 8;case 42:return 8;case 44:{Amount n=0;for(const auto& x:s.parts)if(x.place==Place::Installed)++n;return 8+(n>=3?5:0);}
        case 45:return 5;case 46:return 7;case 47:return 5;case 48:return 8;case 52:return 7+(age>0?8:0);
        case 53:return std::all_of(s.materials.begin(),s.materials.end(),[](Amount n){return n==0;})?17:5;
        case 54:return 8+std::min(3,s.sacrificesRound)*3;case 55:return target.hpLostFight==0?8:4;case 56:return 7;case 75:return 8;
        case 81:return 14;case 84:return 12+3*materialTypes;case 86:return 16;case 87:return 14;
        case 88:return 10+5*((target.burn>0?1:0)+(target.corrosion>0?1:0)+(target.mark>0?1:0)+(target.weaken>0?1:0));
        case 89:return 12;case 90:return 10+8*age;case 109:return 36;case 112:return 24;
        case 1005:return 9+(count==1?5:0);case 1006:return 7;case 1007:return 6;case 1008:return 5;case 1009:return 8+(heatAtFire<=2?6:0);
        case 1011:return 4;case 1012:return 5;case 1023:return 7;case 1025:return 9+(living().size()==1?5:0);case 1032:return 8;case 1033:return 12;case 1036:return 7;case 1038:return 4;case 1039:return 7;
        case 1042:return 9;case 1047:return 8;case 1051:return 6+std::min(6,s.recastConsumed)*2;case 1060:return 6;
        case 1068:return 10;case 1073:return 12+(s.sacrificesRound>0?10:0);case 1076:return 10;case 1077:return 7;
        case 1082:return 12;case 1084:return 10+std::min(9,s.partHpPaidFight)*2;case 1091:return 16;case 1097:return 10;
        case 1103:return 14;case 1109:return 24;case 1120:return 18;
        default:(void)distinct;throw Invalid("No Ammo behavior for this catalogue recipe.");
        }
    }
    Id additionalTarget(const Action& a,Id part,Id main,bool required=true){
        std::vector<Id> targets;for(const auto& pair:a.partTargets)if(pair.part==part)targets.push_back(pair.enemy);
        // Existing spreadTargets remains supported for source-bound support payloads.
        if(targets.empty())for(const auto& pair:a.spreadTargets)if(pair.part==part)targets.push_back(pair.enemy);
        if(targets.empty()&&(!required||living().size()<2))return 0;require(targets.size()==1,"Choose one extra target for this part.");chosen(targets[0]);require(targets[0]!=main,"Choose another enemy.");return targets[0];
    }
    Amount beforeAmmo(const Part& p,Id main,Amount& removedShield,Amount paidBeforeFire){
        auto& t=*byId(s.enemies,main);const auto c=code(p.recipe);Amount removal=0,bonusDamage=0;
        if(c==11||c==1006)removal=6;if(c==41)removal=t.shield/2;if(c==81)removal=t.shield;if(c==1076&&paidBeforeFire>=5)removal=10;if(c==1109)removal=20;
        removedShield=std::min(t.shield,removal);if(removedShield){t.shield-=removedShield;emit("shield_removed",p.id,main,removedShield);}
        if(c==1109)bonusDamage=removedShield/2;
        if(c==1042){const auto n=std::min(6,t.burn);t.burn-=n;bonusDamage=2*n;emit("burn_spent",p.id,main,n);}
        return bonusDamage;
    }
    void afterAmmo(const Part& p,const Action& a,const Enemy& before,const Enemy& fireStart,Amount heatAtFire,Amount ammoCount,Amount shot,Amount removedShield,bool killed){
        const auto c=code(p.recipe);const Id main=a.target;auto* t=byId(s.enemies,main);const auto source=p.recipe;
        const auto other=[&](){for(const auto& x:a.partTargets)if(x.part==p.id)return x.enemy;for(const auto& x:a.spreadTargets)if(x.part==p.id)return x.enemy;return Id{0};};
        switch(c){
        case 14:status(main,1,2);break;case 16:status(main,3,4);break;case 17:enemyDamage(other(),4,true,false,p.id);break;
        case 42:if(killed)enemyDamage(other(),10,true,false,p.id);break;
        case 45:shieldPart(std::min(12,2*ammoCount),source,true);break;
        case 46:for(Id id:living())if(id!=main)status(id,0,2);break;
        case 47:for(Id id:living())status(id,1,2);break;
        case 48:for(Id id:living())if(byId(s.enemies,id)->intent.move==Move::Attack)status(id,3,4);break;
        case 55:status(main,3,3);break;
        case 56:specialLater(source,8,main);break;
        case 75:if(killed)materialLater({0,2,0,0,0},source);break;
        case 86:if(fireStart.shield==0)status(main,1,6);break;
        case 87:{const Amount burn=t?t->burn:0;enemyDamage(other(),burn,true,false,p.id);if(s.phase==Phase::Defeat)return;t=byId(s.enemies,main);if(t)t->burn-=t->burn/2;break;}
        case 89:if(t&&alive(*t))enemyDamage(main,12,true,false,p.id);break;
        case 109:for(Id id:living())if(id!=main){auto* e=byId(s.enemies,id);const auto n=std::min(12,e->shield);e->shield-=n;emit("shield_removed",p.id,id,n);}break;
        case 112:if(killed)for(Id id:living())if(id!=main&&s.phase!=Phase::Defeat)enemyDamage(id,20,true,false,p.id);break;
        case 1006:if(removedShield==6)heat(2);break;case 1007:status(main,2,4);break;
        case 1008:{Amount n=0;for(Id id:living())if(id!=main){status(id,0,2);++n;}heat(std::min(3,n));break;}
        case 1011:status(main,0,heatAtFire>=6?5:2);break;
        case 1012:{const auto id=other();auto* e=byId(s.enemies,id);if(e&&alive(*e)){const bool burning=e->burn>0;enemyDamage(id,3,true,false,p.id);if(burning&&s.phase!=Phase::Defeat)heat(2);}break;}
        case 1023:heat(2);break;case 1032:specialLater(source,4,main);break;
        case 1033:if(before.intent.move==Move::GainShield||before.intent.move==Move::Brace)status(main,3,4);break;
        case 1036:if(t&&t->burn>=3)status(main,3,5);break;case 1038:heat(1);break;
        case 1039:status(main,3,before.intent.move==Move::Attack?6:3);break;case 1047:shieldPart(8,source,true);break;
        case 1060:status(main,0,3);t=byId(s.enemies,main);if(t&&alive(*t))t->burnHoldTicks=2;break;
        case 1077:status(other(),0,std::min(6,(t?t->burn:0)/2));break;
        case 1082:status(main,0,heatAtFire);break;
        case 1091:if(killed)status(other(),0,std::min(8,before.burn));break;
        case 1097:for(Id id:living())if(id!=main&&s.phase!=Phase::Defeat)enemyDamage(id,8,true,false,p.id);break;
        case 1103:if(t&&alive(*t)&&t->shield>0)enemyDamage(main,14,true,false,p.id);break;
        case 1120:{auto& b=bind(source,BindingClock::EndPhase,std::min(30,shot/2),main,p.id);(void)b;break;}
        default:break; // Flat-only Ammo have no payload.
        }
    }
    std::vector<Part> hooks(bool installed=true){std::vector<Part> out;for(const auto& p:s.parts)if(p.everInstalled&&p.firstInstallRound==s.round&&p.resaleReference.rfind("generated:",0)!=0&&(!installed||p.place==Place::Installed))out.push_back(p);std::sort(out.begin(),out.end(),[](const Part&a,const Part&b){return a.bindingOrder<b.bindingOrder;});return out;}
    bool claim(Id id,Amount maximum=1){auto* p=byId(s.parts,id);if(!p||p->hookUses>=maximum)return false;++p->hookUses;return true;}
    Amount attackReduction(Id enemy,Amount total){
        std::vector<Id> use;std::vector<Binding> ordered=s.bindings;std::sort(ordered.begin(),ordered.end(),[](const Binding&a,const Binding&b){return a.order<b.order;});
        for(const auto& b:ordered)if(b.clock==BindingClock::Round&&(b.source=="SH024"||(b.source=="MA059"&&installedBinding(b)))){total=std::max(0,total-b.amount);use.push_back(b.id);}
        bool halved=false;for(const auto& b:ordered)if(b.clock==BindingClock::Round&&b.source=="MA086"&&b.target==enemy&&installedBinding(b)){halved=true;use.push_back(b.id);}
        s.bindings.erase(std::remove_if(s.bindings.begin(),s.bindings.end(),[&](const Binding& b){return std::find(use.begin(),use.end(),b.id)!=use.end();}),s.bindings.end());
        return halved?total/2:total;
    }
    Amount hpPrevention(Amount incoming){for(const auto& p:hooks())if(code(p.recipe)==97&&incoming>0&&claim(p.id)){incoming=std::max(0,incoming-8);}return incoming;}
    void afterEnemyAttack(Id enemy,Amount lostHp,Amount removedShield,Amount shieldBefore,bool burned){
        s.enemyAttackHpRound=add(s.enemyAttackHpRound,lostHp);auto* attacker=byId(s.enemies,enemy);if(attacker)attackedThisRound.insert(enemy);
        const bool blocked=removedShield>0&&lostHp==0;const bool broke=shieldBefore>0&&Rules::shield(s)==0;
        for(const auto& p:hooks()){
            if(s.phase==Phase::Defeat)return;
            const auto c=code(p.recipe);
            switch(c){
            case 20:if(claim(p.id))status(enemy,3,2);break;
            case 25:if(blocked&&claim(p.id))materialLater({1,0,0,0,0},p.recipe);break;
            case 27:if(claim(p.id))enemyDamage(enemy,3,true,false,p.id);break;
            case 58:if(broke&&claim(p.id))for(Id id:living())status(id,3,3);break;
            case 92:if(removedShield>0&&claim(p.id))status(enemy,2,4);break;
            case 94:if(claim(p.id,2))enemyDamage(enemy,7,true,false,p.id);break;
            case 114:if(blocked&&claim(p.id,3))enemyDamage(enemy,10,true,false,p.id);break;
            case 1018:if(claim(p.id)){enemyDamage(enemy,3,true,false,p.id);if(s.phase!=Phase::Defeat&&s.heat>=6)status(enemy,0,2);}break;
            case 1044:if(broke&&claim(p.id))shieldPart(6,p.recipe,true);break;
            case 1065:{auto& b=bind(p.recipe,BindingClock::Round,0,0,p.id);const auto key=std::to_string(enemy);if(std::find(b.seen.begin(),b.seen.end(),key)==b.seen.end()){b.seen.push_back(key);status(enemy,0,2);}break;}
            case 1071:if(lostHp>0&&claim(p.id)){loseHeat(5);shieldPart(12,p.recipe,true);}break;
            case 1072:if(burned&&!active(p.recipe,BindingClock::Round)){bind(p.recipe,BindingClock::Round);materialLater({1,0,0,0,0},p.recipe);}break;
            case 1108:if(blocked&&claim(p.id)){enemyDamage(enemy,10,true,false,p.id);if(s.phase!=Phase::Defeat)status(enemy,0,3);}break;
            case 1115:{auto& b=bind(p.recipe,BindingClock::Round);const auto key=std::to_string(enemy);if(std::find(b.seen.begin(),b.seen.end(),key)!=b.seen.end())break;b.seen.push_back(key);const auto n=std::min(removedShield,24-b.count);b.count+=n;if(n)shieldPart(n,p.recipe,true);break;}
            default:break;
            }
        }
    }
    void afterShotHooks(Amount hpDamage,Amount count){
        for(const auto& p:hooks(false))if((code(p.recipe)==62||code(p.recipe)==1034)&&claim(p.id)){
            if(p.place==Place::Installed&&((code(p.recipe)==62&&hpDamage>0)||(code(p.recipe)==1034&&count<=1)))shieldPart(code(p.recipe)==62?7:4,p.recipe,true);
        }
    }
    void endTurnHooks(const Action& a){
        for(const auto& p:hooks())if(catalogue(p.effects)){
            switch(code(p.recipe)){
            case 92:schedule(DeliveryKind::ShieldPart,15,p.recipe);break;
            case 113:schedule(DeliveryKind::ShieldPart,20,p.recipe);break;
            case 1048:{Amount cost=0;for(const auto& choice:a.partChoices)if(choice.part==p.id){require(choice.enemy==0||choice.enemy==3,"Holdfast can pay 3 Heat or decline.");cost=static_cast<Amount>(choice.enemy);}require(cost==0||cost==3,"Holdfast can pay 3 Heat or decline.");if(cost)payment(0,3,0,true);schedule(DeliveryKind::ShieldPart,cost?12:4,p.recipe);break;}
            case 1099:specialLater(p.recipe,14);break;
            default:break;
            }
        }
    }
    void preReset(){
        struct Ordered{Id order,id;bool part;};std::vector<Ordered> sequence;
        for(const auto& p:hooks())sequence.push_back({p.bindingOrder,p.id,true});
        for(const auto& b:s.bindings)if(b.clock==BindingClock::EndPhase||(b.clock==BindingClock::Round&&(b.source=="SH066"||b.source=="MA069")))sequence.push_back({b.order,b.id,false});
        std::sort(sequence.begin(),sequence.end(),[](const Ordered&a,const Ordered&b){return a.order<b.order;});
        for(const auto& entry:sequence){
            if(s.phase==Phase::Defeat)return;
            if(entry.part){const auto* pointer=byId(s.parts,entry.id);if(!pointer||pointer->place!=Place::Installed)continue;const auto p=*pointer;
                switch(code(p.recipe)){
                case 60:for(auto& x:s.parts)if(x.place==Place::Installed)x.shield=0;for(auto& x:s.protection)x.amount=0;break;
                case 91:if(Rules::shield(s)>0)for(Id id:attackedThisRound)if(s.phase!=Phase::Defeat)enemyDamage(id,6,true,false,p.id);break;
                case 113:if(Rules::shield(s)>=10)specialLater(p.recipe,1);break;
                case 1055:materialLater({std::min(3,Rules::shield(s)/5),0,0,0,0},p.recipe);break;
                case 1057:if(s.enemyAttackHpRound==0)schedule(DeliveryKind::Heat,4,p.recipe);break;
                default:break;
                }
            }else{const auto* ptr=byId(s.bindings,entry.id);if(!ptr)continue;const auto b=*ptr;
                if(b.source=="SH066"){const auto n=std::min(3,Rules::shield(s)/3);Rules::spendShield(s,3*n);materialLater({n,0,0,0,0},b.source);}
                if(b.source=="MA069")materialLater({std::min(4,static_cast<Amount>(b.seen.size())),0,0,0,0},b.source);
                if(b.source=="MA120")enemyDamage(b.target,b.amount,true,false,b.part);
            }
        }
    }
    void extendedDelivery(const Delivery& d,Amount beforeStartHeat){
        switch(d.kind){
        case DeliveryKind::Material:materialNow(d.materials);break;
        case DeliveryKind::PartCopy:for(const auto& p:d.parts)receive(p);break;
        case DeliveryKind::Damage:enemyDamage(d.target,d.amount,true);break;
        case DeliveryKind::Burn:status(d.target,0,d.amount);break;
        case DeliveryKind::NextShot:bonus(d.source,d.amount);break;
        case DeliveryKind::Custom:
            switch(code(d.source)){
            case 56:enemyDamage(d.target,d.amount,true);break;
            case 70:bind(d.source,BindingClock::Round,1);break;
            case 113:cooling(d.amount);break;
            case 1032:status(d.target,0,d.amount);break;
            case 1079:if(d.amount==previousHeatPaid)heat(2);break;
            case 1099:if(beforeStartHeat==0)shieldPart(d.amount,d.source,true);break;
            case 1106:bonus(d.source,d.amount);break;
            default:throw Invalid("Unsupported scheduled recipe effect.");
            }break;
        default:break;
        }
    }
    void nextRoundBindings(){
        const auto oldRound=s.round-1;
        s.bindings.erase(std::remove_if(s.bindings.begin(),s.bindings.end(),[&](const Binding& b){return b.clock==BindingClock::EndPhase||(b.clock==BindingClock::Round&&b.round<=oldRound)||(b.clock==BindingClock::Collection&&b.round<s.round);}),s.bindings.end());
        if(active("MA056"))heat(1);
    }
    void collectionEffects(Materials& haul,const Action& a){
        auto ordered=s.bindings;std::sort(ordered.begin(),ordered.end(),[](const Binding& x,const Binding& y){return x.order<y.order;});
        const auto available=[&](Amount k){return !s.finitePile||s.pile[static_cast<std::size_t>(k)]>0;};
        const auto take=[&](Amount k,Amount n){const auto i=static_cast<std::size_t>(k);const Amount actual=s.finitePile?std::min(n,s.pile[i]):n;if(s.finitePile)s.pile[i]-=actual;haul[i]=add(haul[i],actual);return actual;};
        const std::array<Amount,8> bag{{0,0,0,1,1,2,3,4}};
        const auto randomExtra=[&](Amount n){for(Amount i=0;i<n;++i){std::vector<Amount> choices;for(Amount k:bag)if(available(k))choices.push_back(k);if(choices.empty())break;const auto choice=choices[s.rng.below(Domain::Collection,static_cast<std::uint32_t>(choices.size()))];take(choice,1);}};
        std::vector<std::vector<Amount>> preferred;
        for(const auto& b:ordered)if(b.clock==BindingClock::Collection&&b.round==s.round){switch(code(b.source)){
            case 80:for(int i=0;i<3;++i)preferred.push_back(b.choices);break;
            case 108:for(int i=0;i<4;++i)preferred.push_back(b.choices);break;
            case 120:for(Amount i=0;i<5;++i)preferred.push_back({i});break;
            default:break;}}
        // Preference slots are ordinary haul positions, never extra units. Binding order allocates them.
        std::vector<Amount> normal;for(std::size_t i=0;i<5;++i)for(Amount n=0;n<haul[i];++n)normal.push_back(static_cast<Amount>(i));haul={};
        for(std::size_t i=0;i<normal.size();++i){Amount choice=normal[i];if(i<preferred.size()){const auto& options=preferred[i];for(std::size_t n=0;n<options.size();++n){const auto proposed=options[(i+n)%options.size()];if(available(proposed)){choice=proposed;break;}}}
            if(!available(choice)){auto it=std::find_if(bag.begin(),bag.end(),available);require(it!=bag.end(),"The pile cannot supply the guaranteed baseline.");choice=*it;}take(choice,1);}
        Materials precisionBundle{};if(a.precision>0)precisionBundle[static_cast<std::size_t>(a.steering)]=take(a.steering,a.precision);
        randomExtra(s.haulBonus);s.haulBonus=0;Amount discard=0;
        for(const auto& b:ordered)if(b.clock==BindingClock::Collection&&b.round==s.round){
            if(b.amount>0){randomExtra(b.amount);continue;}
            const auto c=code(b.source);const auto& v=b.choices;
            switch(c){
            case 37:case 38:case 39:case 40:take(c-37,2);break;
            case 77:take(0,1);take(1,1);break;case 78:take(2,1);take(3,1);break;case 79:take(4,1);break;
            case 80:break;case 105:randomExtra(5);discard+=2;break;
            case 106:take(v[0],4);break;case 107:for(Amount i=0;i<4;++i)take(i,1);break;
            case 108:shieldPart(6,b.source,true);break;case 119:for(Amount i:v)take(i,3);break;case 120:randomExtra(4);break;
            case 121:take(v[0],1+(a.precision==2?2:0));break;
            case 122:take(v[0],1+(a.precision==2?1:0));if(a.precision==2)take(v[1],2);break;
            case 123:take(3,1);if(a.precision==2)take(4,2);break;
            case 124:take(1,1);if(a.precision==2)for(std::size_t i=0;i<5;++i)take(static_cast<Amount>(i),precisionBundle[i]);break;
            case 125:take(0,1);take(1,1);if(a.precision==2)for(Amount i=0;i<5;++i)take(i,1);break;
            case 126:take(0,1);if(a.precision==2){receive(plain(Kind::Ammo,10,b.source,"Salvage Slug"));receive(plain(Kind::Shield,10,b.source,"Salvage Shield Plate"));}break;
            default:throw Invalid("Unsupported gathering recipe.");
            }
        }
        Amount discarded=0;for(std::size_t i=0;i<5;++i){require(a.discarded[i]>=0&&a.discarded[i]<=haul[i],"Leave only units gathered by this collection.");discarded=add(discarded,a.discarded[i]);haul[i]-=a.discarded[i];if(s.finitePile)s.pile[i]=add(s.pile[i],a.discarded[i]);}require(discarded==discard,"Choose the exact Heavy Magnet Lift discard.");
        s.bindings.erase(std::remove_if(s.bindings.begin(),s.bindings.end(),[&](const Binding& b){return b.clock==BindingClock::Collection&&b.round<=s.round;}),s.bindings.end());
    }
    std::set<Id> attackedThisRound;
    Amount previousHeatPaid=0;
    void completeFire(const Action& a){
        require(!s.bullet.empty(),"Load a nonempty bullet first.");const auto original=chosen(a.target);const Amount heatAtFire=s.heat,paidBeforeFire=s.partHeatPaidRound;
        std::vector<Part> fired;std::set<std::string> represented;Materials types{};Amount ammoCount=0;
        for(Id id:s.bullet){auto* p=byId(s.parts,id);require(p&&p->place==Place::Loaded,"Loaded part is missing.");fired.push_back(*p);if(p->kind==Kind::Ammo){++ammoCount;represented.insert(p->recipe);for(std::size_t i=0;i<5;++i)if(p->materialBasis[i]>0)types[i]=1;}}
        const Amount distinct=static_cast<Amount>(represented.size()),materialTypes=static_cast<Amount>(std::count(types.begin(),types.end(),1));
        Amount base=s.nextFlat,percent=s.nextPercent,heatCost=0,hpCost=0;for(const auto& b:s.shotBonuses){base=add(base,b.flat);percent=add(percent,b.percent);}
        struct Spread{Id part,target;Amount percent,code;};std::vector<Spread> spreads;std::set<Id> consumedTargetPairs;std::set<Amount> uniqueSpreads;
        bool shoulder=false;
        for(const auto& p:fired){const auto c=code(p.recipe);
            for(const auto& f:p.effects)if(f.timing==Timing::Assembly){if(f.op==Op::FlatDamage)base=add(base,f.amount);if(f.op==Op::AttackIntentBonus&&original.intent.move==Move::Attack)base=add(base,f.amount);if(f.op==Op::PercentDamage)percent=add(percent,f.amount);if(f.op==Op::HeatCost)heatCost=add(heatCost,f.amount);if(f.op==Op::HpCost)hpCost=add(hpCost,f.amount);}
            if(catalogue(p.effects)&&p.kind==Kind::Ammo)base=add(base,ammoFlat(p,original,ammoCount,distinct,materialTypes,heatAtFire));
            for(const auto& b:p.attachments)if(b.dueRound==0||b.dueRound==s.round)base=add(base,b.damage);
            if(c==1068&&Rules::shield(s,true)>=12)shoulder=true;
            if(c==17||c==42||c==87||c==1012||c==1077||c==1091)additionalTarget(a,p.id,a.target,living().size()>1);
            if(p.kind!=Kind::Spread)continue;require(!active("MA067",BindingClock::Shot),"Barrel Weight cannot combine with spreading parts.");
            Amount factor=0,maxTargets=1;bool all=false,unique=false;
            for(const auto& f:p.effects)if(f.op==Op::SpreadPercent)factor=f.amount;
            switch(c){case 51:factor=40;maxTargets=2;break;case 82:factor=50;all=true;break;case 110:factor=100;all=true;break;case 1041:factor=50;heatCost=add(heatCost,4);unique=true;break;case 1111:factor=60;heatCost=add(heatCost,7);all=true;unique=true;break;default:break;}
            std::vector<Id> targets;if(all){for(Id id:living())if(id!=a.target)targets.push_back(id);}else{for(const auto& x:a.spreadTargets)if(x.part==p.id)targets.push_back(x.enemy);for(const auto& x:a.partTargets)if(x.part==p.id)targets.push_back(x.enemy);require(targets.size()<=static_cast<std::size_t>(maxTargets),"Too many targets for this spreading part.");if(maxTargets==1&&living().size()>1)require(targets.size()==1,"Choose another enemy for every spreading part.");}
            std::set<Id> seen;for(Id id:targets){chosen(id);require(id!=a.target&&seen.insert(id).second,"Spread targets must be distinct other living enemies.");}
            consumedTargetPairs.insert(p.id);if(unique&&!uniqueSpreads.insert(c).second)continue;for(Id id:targets)spreads.push_back({p.id,id,factor,c});
        }
        for(const auto& x:a.spreadTargets){const auto it=std::find_if(fired.begin(),fired.end(),[&](const Part&p){return p.id==x.part;});require(it!=fired.end(),"Unused spread target selection.");}
        for(const auto& x:a.partTargets){const auto it=std::find_if(fired.begin(),fired.end(),[&](const Part&p){return p.id==x.part;});require(it!=fired.end(),"Unused part target selection.");}
        if(auto* b=binding("SH072",BindingClock::Shot)){(void)b;require(a.partChoices.size()==1,"Return Delivery needs one Base or Common Ammo in this shot.");const Id selected=a.partChoices[0].enemy;const auto it=std::find_if(fired.begin(),fired.end(),[selected](const Part&p){return p.id==selected;});require(it!=fired.end()&&it->kind==Kind::Ammo&&it->rarity<=Rarity::Common,"Return Delivery needs Base or Common Ammo.");copyLater(*it,1,"SH072");}
        require(s.hp>hpCost,"Own HP cost must leave at least 1 HP.");require(s.heat>=heatCost,"Not enough Heat for the complete bullet.");
        // Full combined affordability is validated before any per-part payment trigger can grant supplies.
        for(const auto& p:fired){Amount hp=0,heatPayment=0;for(const auto& f:p.effects)if(f.timing==Timing::Assembly){if(f.op==Op::HpCost)hp=add(hp,f.amount);if(f.op==Op::HeatCost)heatPayment=add(heatPayment,f.amount);}if(code(p.recipe)==1041)heatPayment=add(heatPayment,4);if(code(p.recipe)==1111)heatPayment=add(heatPayment,7);payment(hp,heatPayment,0,true);}
        for(const auto& p:fired)if(code(p.recipe)==110){auto it=std::find_if(s.parts.begin(),s.parts.end(),[&](const Part& x){return x.place==Place::Payment&&x.reservedBy==p.id;});require(it!=s.parts.end(),"Full-Spread Outlet requires its reserved Shield payment.");const auto id=it->id;it->place=Place::Reserve;sacrifice(id,Kind::Shield);}
        if(s.hotBarrel)base=add(base,heatAtFire/2);
        if(active("MA112"))base=add(base,std::min(20,Rules::shield(s,true)/2));
        if(active("SH083",BindingClock::Shot))percent=add(percent,std::min(80,8*ammoCount));
        if(active("MA063",BindingClock::Shot))base=add(base,std::min(15,3*distinct));
        if(active("MA078",BindingClock::Shot))base=add(base,std::min(16,4*std::max(0,ammoCount-1)));
        if(active("MA098",BindingClock::Shot)){Amount n=0;for(const auto& p:s.parts)if(p.place==Place::Reserve)++n;base=add(base,std::min(18,3*n));}
        if(shoulder)percent=add(percent,25);
        std::vector<Amount> removed;for(const auto& p:fired){Amount n=0;if(catalogue(p.effects)&&p.kind==Kind::Ammo)base=add(base,beforeAmmo(p,a.target,n,paidBeforeFire));removed.push_back(n);}
        const Enemy before=*byId(s.enemies,a.target);
        const Amount shot=std::max(0,checked(static_cast<std::int64_t>(base)+static_cast<std::int64_t>(base)*percent/100)-s.weaken);
        const Amount main=add(shot,before.mark);Amount bypass=0;if(active("SH043",BindingClock::Shot))bypass=12;if(active("MA094",BindingClock::Shot))bypass=main;
        byId(s.enemies,a.target)->mark=0;
        s.parts.erase(std::remove_if(s.parts.begin(),s.parts.end(),[](const Part& p){return p.place==Place::Loaded;}),s.parts.end());s.bullet.clear();++s.shots;s.nextFlat=s.nextPercent=0;s.shotBonuses.clear();
        emit("fire",0,a.target,shot,heatAtFire);enemyDamage(a.target,main,true,false,0,bypass);if(s.phase==Phase::Defeat)return;
        const auto* mainAfter=byId(s.enemies,a.target);const bool killed=mainAfter&&mainAfter->dead;const Amount hpDamage=mainAfter?before.hp-mainAfter->hp:0;
        for(std::size_t i=0;i<fired.size()&&s.phase!=Phase::Defeat;++i){const auto& p=fired[i];effects(p.effects,Timing::AfterHit,a.target,heatAtFire,p.recipe);if(catalogue(p.effects)&&p.kind==Kind::Ammo)afterAmmo(p,a,before,original,heatAtFire,ammoCount,shot,removed[i],killed);if(s.phase!=Phase::Defeat)for(const auto& b:p.attachments)if(b.heat&&b.dueRound==s.round)heat(b.heat);}
        if(s.phase==Phase::Defeat)return;
        if(active("MA110"))status(a.target,0,heatAtFire>=8?6:3);
        Amount splitterKills=0;
        for(const auto& spread:spreads){if(s.phase==Phase::Defeat)break;auto* e=byId(s.enemies,spread.target);const bool wasAlive=e&&alive(*e);enemyDamage(spread.target,checked(static_cast<std::int64_t>(shot)*spread.percent/100),true,false,spread.part);if(s.phase==Phase::Defeat)break;e=byId(s.enemies,spread.target);if(wasAlive&&e&&e->dead&&spread.code==1111)++splitterKills;if(wasAlive&&spread.code==1041)status(spread.target,0,2);if(wasAlive&&spread.code==110)status(spread.target,3,3);}
        if(s.phase==Phase::Defeat)return;if(splitterKills)heat(std::min(6,2*splitterKills));
        if(active("SH111",BindingClock::Shot)&&ammoCount>=10)shieldPart(15,"SH111",true);
        if(active("MA067",BindingClock::Shot))loseHeat(4);
        if(active("MA078",BindingClock::Shot)&&ammoCount>=5){loseHeat(3);shieldPart(6,"MA078",true);}
        if(active("MA106",BindingClock::Shot)&&killed){s.deliveries.erase(std::remove_if(s.deliveries.begin(),s.deliveries.end(),[&](const Delivery& d){return d.source=="MA106"&&d.dueRound==s.round+1;}),s.deliveries.end());specialLater("MA106",16);}
        for(const auto& p:fired)if(code(p.recipe)==1051)++s.recastConsumed;
        afterShotHooks(hpDamage,ammoCount);
        s.bindings.erase(std::remove_if(s.bindings.begin(),s.bindings.end(),[](const Binding& b){return b.clock==BindingClock::Shot;}),s.bindings.end());terminal();
    }

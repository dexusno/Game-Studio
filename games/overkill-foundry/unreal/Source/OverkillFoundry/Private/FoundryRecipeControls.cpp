#include "FoundryRecipeControls.h"
#if FOUNDRY_WITH_CAMPAIGN
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <initializer_list>

namespace foundry_controls {
using namespace overkill;
namespace {
const std::vector<Id> Empty;
const Selections NoSelections;
auto key(const Action& a){return std::make_pair(static_cast<int>(a.type),a.subject);}
bool oneOf(const std::string& id,std::initializer_list<const char*> ids){for(const char* v:ids)if(id==v)return true;return false;}
const Part* part(const State& s,Id id){for(const auto& p:s.parts)if(p.id==id)return &p;return nullptr;}
const Binding* binding(const State& s,const std::string& id,BindingClock clock){for(const auto& b:s.bindings)if(b.source==id&&b.clock==clock)return &b;return nullptr;}
std::vector<Option> enemies(const State& s,Id except=0){
    std::vector<Option> result;for(const auto& e:s.enemies)if(!e.dead&&!e.escaped&&e.id!=except){
        std::string label=e.name+" ["+std::to_string(e.id)+"] / "+std::to_string(e.hp)+" HP";
        if(e.burn)label+=" / Burn "+std::to_string(e.burn);
        if(e.mark)label+=" / Mark "+std::to_string(e.mark);
        if(e.weaken)label+=" / Weaken "+std::to_string(e.weaken);
        label+=" / "+Rules::intentText(e,s.round);
        result.push_back({e.id,std::move(label),{}});
    }return result;
}
std::vector<Option> reserves(const State& s,Kind kind=Kind::Utility,Id except=0){std::vector<Option> result;for(const auto& p:s.parts)if(p.id!=except&&p.place==Place::Reserve&&isUnusedPart(p)&&(kind==Kind::Utility||p.kind==kind))result.push_back({p.id,p.output+" ["+std::to_string(p.id)+"]",p.recipe});return result;}
std::vector<Option> amounts(int first,int last){std::vector<Option> result;for(int i=first;i<=last;++i)result.push_back({static_cast<Id>(i),std::to_string(i),{}});return result;}
std::vector<Option> materials(int first=0,int last=4){static const char* names[]={"Iron","Copper","Carbon","Glass","Circuit"};std::vector<Option> result;for(int i=first;i<=last;++i)result.push_back({static_cast<Id>(i),names[i],{}});return result;}
}
const Selections& Drafts::selections(const Action& base)const{auto i=drafts.find(key(base));return i==drafts.end()?NoSelections:i->second;}
const std::vector<Id>& Drafts::selected(const Action& base,const std::string& name)const{const auto& s=selections(base);auto i=s.find(name);return i==s.end()?Empty:i->second;}
void Drafts::set(const Action& base,const std::string& name,std::vector<Id> values){drafts[key(base)][name]=std::move(values);}
void Drafts::choose(const Action& base,const Choice& choice,Id value){
    if(std::none_of(choice.options.begin(),choice.options.end(),[value](const Option& o){return o.value==value;}))return;
    auto& v=drafts[key(base)][choice.key];auto it=std::find(v.begin(),v.end(),value);
    if(choice.maximum==1){if(it!=v.end()&&choice.minimum==0)v.clear();else v={value};}
    else if(it!=v.end())v.erase(it);else if(v.size()<static_cast<std::size_t>(choice.maximum))v.push_back(value);
}
void Drafts::reset(const Action& base){drafts.erase(key(base));}
void Drafts::clear(){drafts.clear();}
std::vector<Choice> Drafts::choices(const Rules& rules,const State& s,const Action& a)const{
    std::vector<Choice> out;
    const auto add=[&](std::string name,std::string label,Field field,std::vector<Option> options,int min=1,int max=1,Id source=0,int index=0,std::string detail={}){
        out.push_back({std::move(name),std::move(label),std::move(detail),field,source,index,min,max,std::move(options)});
    };
    const auto target=[&](const char* label="Choose an enemy"){add("target",label,Field::Target,enemies(s));};
    const auto reserve=[&](const char* label,Kind kind=Kind::Utility,int min=1,int max=1){add("parts",label,Field::Parts,reserves(s,kind,a.subject),min,max);};
    std::string recipe;const Recipe* r=nullptr;const Part* p=nullptr;
    if(a.type==ActionType::Craft){for(const auto& c:s.memory)if(c.id==a.subject){recipe=c.recipe;r=rules.recipe(recipe);break;}}
    else if(a.type==ActionType::Install||a.type==ActionType::Activate){p=part(s,a.subject);if(p)recipe=p->recipe;}
    if(a.type==ActionType::Craft){
        if(oneOf(recipe,{"SH015","SH049","MA026","MA029","MA049","MA050","MA074","MA087","MA095","MA100","MA105","MA117"}))target(recipe=="MA105"?"Burn source enemy":"Choose an enemy");
        if(recipe=="MA105")add("transfer","Burn destination enemy",Field::ExtraTarget,enemies(s),1,1,0,0,"Choose a different living enemy to receive the source enemy's Burn.");
        if(oneOf(recipe,{"SH032","SH071","SH102","SH118","MA030","MA066"}))reserve(recipe=="SH032"?"Unused part to sacrifice":"Unused part to copy or attach to",oneOf(recipe,{"SH071","MA030"})?Kind::Ammo:Kind::Utility);
        if(recipe=="SH065")reserve("Unused Ammo to sacrifice",Kind::Ammo);
        if(oneOf(recipe,{"SH076","MA089"}))reserve("Unused Shield to sacrifice",Kind::Shield);
        if(recipe=="SH101")reserve("Unused parts to sacrifice (in selected order)",Kind::Utility,0,2);
        int count=0,first=0,last=4;
        if(oneOf(recipe,{"SH080","SH119"}))count=2;
        else if(recipe=="SH122"){count=2;last=3;}
        else if(oneOf(recipe,{"SH106","SH108"}))count=1;
        else if(recipe=="SH121"){count=1;last=3;}
        else if(recipe=="SH069"){count=1;first=1;last=3;}
        else if(recipe=="SH101"){count=static_cast<int>(selected(a,"parts").size());last=3;}
        for(int i=0;i<count;++i)add("material-"+std::to_string(i),"Material choice "+std::to_string(i+1),Field::Material,materials(first,last),1,1,0,i,count==2&&recipe!="SH101"?"Choose two different materials, in printed order.":"Order matches the selected parts when a sacrifice is involved.");
        const auto* mould=binding(s,"SH103",BindingClock::Round);
        if(r&&r->kind==Kind::Ammo&&r->outputCount>1&&mould&&mould->amount>0){
            auto options=amounts(0,r->outputCount-1);for(auto& o:options)o.label="Output "+std::to_string(o.value+1);
            add("amount","Fine Mould: enhanced output",Field::Amount,std::move(options));
        }
        if(ownedUpgrade(s,"MY1-09"))for(int i=0;i<5;++i)add("discount-"+std::to_string(i),"Rotating Toolhead: "+materials(i,i)[0].label+" discount",Field::Discount,amounts(0,2),1,1,0,i,"Allocate at most two units when the upgrade is ready. Pay now shows the resulting cost.");
    }
    if(a.type==ActionType::Install&&p&&!p->everInstalled){
        if(recipe=="MA022")add("sacrifice","Unused Ammo to sacrifice",Field::Sacrifice,reserves(s,Kind::Ammo));
        if(recipe=="MA083")add("amount","Flood Door: optional Heat payment",Field::Amount,{{0,"Pay no Heat",{}},{static_cast<Id>(s.heat),"Pay all current Heat ("+std::to_string(s.heat)+")",{}}});
        if(recipe=="MA086")target("Attacking enemy to brace against");
        if(recipe=="MA096")add("amount","Heat payment",Field::Amount,amounts(3,8));
    }
    if(a.type==ActionType::Activate){
        if(recipe=="SH036")reserve("Unused Ammo to attach the fuse to",Kind::Ammo);
        if(recipe=="SH085")add("amount","Installed Shield payment (optional)",Field::Amount,amounts(0,15));
        if(recipe=="MA045")add("amount","Heat payment",Field::Amount,amounts(2,5));
        if(recipe=="MA052")reserve("Unused Ammo to consume",Kind::Ammo,1,3);
        if(recipe=="MA053"&&s.heat>=6)target("Enemy to mark");
        if(recipe=="MA061")add("amount","Installed Shield payment",Field::Amount,amounts(4,12));
    }
    if(a.type==ActionType::Load)for(Id id:a.parts)if(const auto* item=part(s,id);item&&item->recipe=="SH110")
        add("sacrifice-"+std::to_string(id),"Shield reserved for "+item->output+" ["+std::to_string(id)+"]",Field::Sacrifice,reserves(s,Kind::Shield),1,1,id,0,"Reserved now, consumed only by Fire. Unload returns it.");
    if(a.type==ActionType::Fire){
        for(Id id:s.bullet)if(const auto* item=part(s,id)){
            const bool support=oneOf(item->recipe,{"SH017","SH042","SH087","MA012","MA077","MA091"});
            const bool spread=item->kind==Kind::Spread&&!oneOf(item->recipe,{"SH082","SH110","MA111"});
            if(support||spread){auto options=enemies(s,a.target);const int max=item->recipe=="SH051"?2:1;add("extra-"+std::to_string(id),item->output+" ["+std::to_string(id)+"] : additional target",Field::ExtraTarget,std::move(options),max==2?0:1,max,id,0,max==2?"Choose up to two different enemies; click order is hit order.":"Choose another living enemy when one remains.");}
        }
        if(binding(s,"SH072",BindingClock::Shot)){
            std::vector<Option> options;for(Id id:s.bullet)if(const auto* item=part(s,id);item&&item->kind==Kind::Ammo&&item->rarity<=Rarity::Common)options.push_back({id,item->output+" ["+std::to_string(id)+"]",item->recipe});
            add("return-delivery","Return Delivery: Ammo to copy after this shot",Field::PartChoice,std::move(options));
        }
    }
    if(a.type==ActionType::EndTurn)for(const auto& item:s.parts)if(item.recipe=="MA048"&&item.place==Place::Installed&&item.firstInstallRound==s.round)
        add("holdfast-"+std::to_string(item.id),"Holdfast ["+std::to_string(item.id)+"] : Heat payment",Field::PartChoice,{{0,"Decline extra payment",{}},{3,"Pay 3 Heat",{}}},1,1,item.id);
    return out;
}
Action Drafts::build(const Rules& rules,const State& s,Action a)const{
    for(const auto& c:choices(rules,s,a)){
        const auto& v=selected(a,c.key);const Id value=v.empty()?0:v.front();
        switch(c.field){
        case Field::Amount:a.amount=static_cast<Amount>(value);break;
        case Field::Target:a.target=value;break;
        case Field::Parts:a.parts=v;break;
        case Field::Material:a.choices.push_back(v.empty()?-1:static_cast<Amount>(value));break;
        case Field::Sacrifice:a.sacrifices.push_back(value);break;
        case Field::ExtraTarget:if(c.source==0){a.targets=v;}else for(Id id:v)a.partTargets.push_back({c.source,id});break;
        case Field::PartChoice:if(!v.empty())a.partChoices.push_back({c.source,value});break;
        case Field::Discount:a.discount[static_cast<std::size_t>(c.index)]=static_cast<Amount>(value);break;
        }
    }
    return a;
}
std::vector<Id> loadedDisplayOrder(const State& s){
    std::vector<Id> result;
    for(Id id:s.bullet){
        result.push_back(id);
        for(const auto& p:s.parts)if(p.place==Place::Payment&&p.reservedBy==id)result.push_back(p.id);
    }
    return result;
}
bool heavyLiftPending(const State& s){for(const auto& b:s.bindings)if(b.source=="SH105"&&b.clock==BindingClock::Collection&&b.round==s.round)return true;return false;}
std::vector<CollectionOption> collectionOptions(const Rules& rules,const State& state,int steering,int precision){
    std::vector<CollectionOption> result;
    const auto add=[&](Action a){auto p=rules.preview(state,a);if(p.result.ok)result.push_back({std::move(a),std::move(p)});};
    if(!heavyLiftPending(state)){add(Action::collect(steering,precision));return result;}
    for(std::size_t i=0;i<5;++i)for(std::size_t j=i;j<5;++j){auto a=Action::collect(steering,precision);++a.discarded[i];++a.discarded[j];add(std::move(a));}
    return result;
}
}
#endif

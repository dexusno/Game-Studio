#include "FoundryRecipeControls.h"
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>
using namespace overkill;
using namespace foundry_controls;
namespace {
int checks=0;
void check(bool ok,const std::string& message){++checks;if(!ok)throw std::runtime_error(message);}
void apply(const Rules& r,State& s,const Action& a){auto before=stateHash(s);const auto p=r.preview(s,a);check(stateHash(s)==before,"preview mutated live state");auto result=r.apply(s,a);check(result.ok,result.reason);check(p.result.ok&&serialize(p.state)==serialize(s),"configured preview/commit differ");}
Id memory(State& s,const std::string& recipe){RecipeCopy c;c.id=s.nextId++;c.recipe=recipe;s.memory.push_back(c);return c.id;}
Id grant(const Rules& r,State& s,const std::string& recipe){const auto before=s.nextId;auto result=r.grantPart(s,recipe);check(result.ok,result.reason);for(const auto& p:s.parts)if(p.id>=before&&p.recipe==recipe)return p.id;throw std::runtime_error("grant missing "+recipe);}
State fixture(const Rules& r){State s;s.encounter="recipe-ui-prepared-test";s.rng=Rng::seeded(9,s.encounter);s.phase=Phase::Preparation;s.hp=60;s.heat=8;s.hotBarrel=false;s.materials={100,100,100,100,100};
    for(int i=0;i<3;++i){Enemy e;e.id=s.nextId++;e.definition="TEST";e.name="Prepared target";e.hp=e.maxHp=1000;e.burn=4;e.intent={Move::Attack,10,1};e.pattern={e.intent};s.enemies.push_back(e);}
    check(r.grantPlainPart(s,Kind::Shield,80,"Prepared protection",true).ok,"protection");grant(r,s,"SH009");grant(r,s,"SH002");return s;}
void chooseAll(Drafts& d,const Rules& r,const State& s,const Action& base){
    // Deliberate test selections, not production defaults. The second pass
    // picks per-part material choices exposed by SH101's selected sacrifices.
    for(int pass=0;pass<2;++pass)for(const auto& c:d.choices(r,s,base)){
        if(!d.selected(base,c.key).empty()||c.options.empty()||c.field==Field::Discount)continue;
        if(c.key=="transfer"){d.set(base,c.key,{s.enemies[1].id});continue;}
        const auto index=c.field==Field::Material?static_cast<std::size_t>(c.index):std::size_t{0};
        if(c.maximum>1){std::vector<Id> ids;for(int i=0;i<c.maximum&&static_cast<std::size_t>(i)<c.options.size();++i)ids.push_back(c.options[static_cast<std::size_t>(i)].value);d.set(base,c.key,ids);}
        else d.set(base,c.key,{c.options[std::min(index,c.options.size()-1)].value});
    }
}
void configured(const Rules&r,State&s,Drafts&d,Action a){chooseAll(d,r,s,a);apply(r,s,d.build(r,s,a));}
}
int main(){try{Rules r;std::set<std::string> covered,choiceRecipes;
    for(const auto& recipe:r.content()){
        State s=fixture(r);Drafts d;if(recipe.id=="MA095")s.heat=0;
        const auto copy=memory(s,recipe.id);auto craft=Action::craft(copy);chooseAll(d,r,s,craft);
        if(!d.choices(r,s,craft).empty())choiceRecipes.insert(recipe.id);
        const Id first=s.nextId;apply(r,s,d.build(r,s,craft));
        if(recipe.kind!=Kind::Utility&&!recipe.automaticOutput){
            Id output=0;for(const auto& p:s.parts)if(p.id>=first&&p.recipe==recipe.id){output=p.id;break;}check(output!=0,"missing output "+recipe.id);
            if(recipe.kind==Kind::Shield){auto a=Action::install(output);if(!d.choices(r,s,a).empty())choiceRecipes.insert(recipe.id);configured(r,s,d,a);}
            else if(recipe.kind==Kind::Modifier||recipe.kind==Kind::Magnet){Action a;a.type=ActionType::Activate;a.subject=output;if(!d.choices(r,s,a).empty())choiceRecipes.insert(recipe.id);configured(r,s,d,a);
                if(recipe.kind==Kind::Magnet){apply(r,s,Action::endTurn());const auto before=stateHash(s);auto options=collectionOptions(r,s,0,2);check(stateHash(s)==before,"collection options changed RNG/state");check(!options.empty(),"no valid collection option "+recipe.id);apply(r,s,options.back().action);}
                else{auto ammo=grant(r,s,"SH001");configured(r,s,d,Action::load({ammo}));configured(r,s,d,Action::fire(s.enemies[0].id));}
            }
            else{std::vector<Id> bullet={output};if(recipe.kind==Kind::Spread)bullet.insert(bullet.begin(),grant(r,s,"SH001"));auto load=Action::load(bullet);if(!d.choices(r,s,load).empty())choiceRecipes.insert(recipe.id);configured(r,s,d,load);auto fire=Action::fire(s.enemies[0].id);if(!d.choices(r,s,fire).empty())choiceRecipes.insert(recipe.id);configured(r,s,d,fire);}
        }
        covered.insert(recipe.id);
    }
    check(covered.size()==246,"all 246 recipes need a configured action path");
    {
        auto s=fixture(r);Drafts d;const auto id=grant(r,s,"SH085");Action a;a.type=ActionType::Activate;a.subject=id;d.set(a,"amount",{0});const auto zero=d.build(r,s,a);check(zero.amount==0,"explicit zero payment lost");auto z=r.preview(s,zero);d.set(a,"amount",{15});const auto full=d.build(r,s,a);auto f=r.preview(s,full);check(z.result.ok&&f.result.ok,"optional Shield choices");check(Rules::shield(z.state,true)-Rules::shield(f.state,true)==15,"chosen Shield payment was ignored");
        d.reset(a);check(d.build(r,s,a).amount==0,"reset selection");
    }
    {
        auto s=fixture(r);Drafts d;const auto victim=grant(r,s,"SH001");const auto clamp=grant(r,s,"MA022");auto a=Action::install(clamp);d.set(a,"sacrifice",{victim});auto built=d.build(r,s,a);check(built.sacrifices==std::vector<Id>{victim},"wrong sacrifice field");apply(r,s,built);check(std::none_of(s.parts.begin(),s.parts.end(),[victim](const Part&p){return p.id==victim;}),"selected victim not consumed");
    }
    {
        auto s=fixture(r);Drafts d;const auto a=grant(r,s,"SH110"),b=grant(r,s,"SH110"),shieldA=grant(r,s,"SH002"),shieldB=grant(r,s,"SH002");auto load=Action::load({a,b});d.set(load,"sacrifice-"+std::to_string(a),{shieldA});d.set(load,"sacrifice-"+std::to_string(b),{shieldB});
        auto reversed=Action::load({b,a});check(d.build(r,s,reversed).sacrifices==std::vector<Id>({shieldB,shieldA}),"reordering changed payment-to-source association");apply(r,s,d.build(r,s,reversed));
        check(loadedDisplayOrder(s)==std::vector<Id>({b,shieldB,a,shieldA}),"loaded panel must show actual firing order with each physical payment beside its source");
        Action unload;unload.type=ActionType::Unload;apply(r,s,unload);check(std::none_of(s.parts.begin(),s.parts.end(),[](const Part&p){return p.place==Place::Payment;}),"unload failed to return reserved payments");
        check(loadedDisplayOrder(s).empty(),"unloaded panel retained stale loaded rows");
    }
    {
        auto s=fixture(r);Drafts d;const auto p=grant(r,s,"MA048");apply(r,s,Action::install(p));auto a=Action::endTurn();d.set(a,"holdfast-"+std::to_string(p),{3});auto built=d.build(r,s,a);check(built.partChoices.size()==1&&built.partChoices[0].part==p&&built.partChoices[0].enemy==3,"Holdfast field mapping");apply(r,s,built);check(std::any_of(s.parts.begin(),s.parts.end(),[&](const Part&v){return v.recipe=="MA048"&&v.originalValue==12&&v.createdRound==s.round;}),"Holdfast selected payment not delivered next round");
    }
    {
        auto s=fixture(r);Drafts d;check(r.acquireUpgrade(s,"MY1-09").ok,"Toolhead acquire");for(const char* id:{"SH013","SH004","SH019"})apply(r,s,Action::craft(memory(s,id)));auto a=Action::craft(memory(s,"SH002"));d.set(a,"discount-0",{1});d.set(a,"discount-1",{1});auto built=d.build(r,s,a);check(built.discount==Materials({1,1,0,0,0}),"discount material allocation");const auto before=s.materials;apply(r,s,built);check(s.materials==before,"eligible free recipe paid materials");auto rejected=Action::craft(memory(s,"SH001"));d.set(rejected,"discount-0",{1});auto hash=stateHash(s);check(!r.apply(s,d.build(r,s,rejected)).ok&&stateHash(s)==hash,"unavailable discount must be atomic rejection");
    }
    {
        auto s=fixture(r);Drafts d;auto mould=grant(r,s,"SH103");Action use;use.type=ActionType::Activate;use.subject=mould;apply(r,s,use);auto a=Action::craft(memory(s,"MA038"));d.set(a,"amount",{1});const auto first=s.nextId;apply(r,s,d.build(r,s,a));std::vector<const Part*> outputs;for(const auto&p:s.parts)if(p.id>=first&&p.recipe=="MA038")outputs.push_back(&p);check(outputs.size()==2&&outputs[0]->attachments.empty()&&!outputs[1]->attachments.empty(),"Fine Mould output index ignored");
        const auto next=Action::craft(memory(s,"MA038"));const auto choices=d.choices(r,s,next);
        check(std::none_of(choices.begin(),choices.end(),[](const Choice& c){return c.label=="Fine Mould: enhanced output";}),"spent Fine Mould still offers an enhanced output");
        const auto nextOutput=s.nextId;apply(r,s,d.build(r,s,next));
        check(std::none_of(s.parts.begin(),s.parts.end(),[nextOutput](const Part&p){return p.id>=nextOutput&&!p.attachments.empty();}),"spent Fine Mould applied a second enhancement");
    }
    {
        auto s=fixture(r);auto lift=grant(r,s,"SH105");Action fit;fit.type=ActionType::Activate;fit.subject=lift;apply(r,s,fit);apply(r,s,Action::endTurn());const auto before=stateHash(s);for(int precision:{-1,0,1,2}){const auto options=collectionOptions(r,s,3,precision);check(!options.empty(),"missing Heavy Lift options");for(const auto&o:options){int total=0;for(int n:o.action.discarded)total+=n;check(total==2&&o.action.precision==precision&&o.action.steering==3,"discard enumeration altered measured result/steering");auto copy=s;apply(r,copy,o.action);}}check(stateHash(s)==before,"options consumed RNG or Precision");
    }
    std::cout<<"RECIPE_CONTROL_TESTS_OK checks="<<checks<<" recipes="<<covered.size()<<" configured_recipe_ids=";
    for(const auto&id:choiceRecipes)std::cout<<id<<',';std::cout<<"\n";return 0;
}catch(const std::exception&e){std::cerr<<"RECIPE_CONTROL_TEST_FAILED checks="<<checks<<" "<<e.what()<<"\n";return 1;}}

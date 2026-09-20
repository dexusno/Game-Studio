// Links the archived pre-fix library; emits actual old-rule outputs, not edited saves.
#include "overkill/core.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace overkill;
int main(int argc,char** argv){
    if(argc!=2){std::cerr<<"Expected existing output directory\n";return 2;}
    try{
        const Rules rules;
        for(const bool jig:{false,true}){
            State s;s.hp=s.maxHp=80;s.hotBarrel=false;s.phase=Phase::Preparation;
            s.materials={100,100,100,100,100};s.encounter="legacy-copy-control";s.rng=Rng::seeded(37,s.encounter);
            Enemy enemy;enemy.id=s.nextId++;enemy.definition=enemy.name="Controlled target";enemy.hp=enemy.maxHp=1000;enemy.intent={Move::Recover,0,0};enemy.pattern={enemy.intent};s.enemies.push_back(enemy);
            auto ok=[](const Result& r){if(!r.ok)throw std::runtime_error(r.reason);};
            ok(rules.acquireUpgrade(s,jig?"UGS-121":"MY1-01"));ok(rules.acquireUpgrade(s,"UGS-015"));
            ok(rules.startUpgrades(s,EncounterClass::Regular));ok(rules.apply(s,Action::collect(0)));
            RecipeCopy recipe;recipe.id=s.nextId++;recipe.recipe="SH001";s.memory.push_back(recipe);
            ok(rules.apply(s,Action::craft(recipe.id)));
            if(s.parts.size()!=2)throw std::runtime_error("Expected old paid original and lease copy");
            for(const auto& part:s.parts)if(part.upgradeDamage!=2||!part.attachments.empty())throw std::runtime_error("Historical aggregate or attribution differs");
            const std::string name=jig?"jig.ofcore":"mayor.ofcore";
            std::ofstream out(std::string(argv[1])+"/"+name,std::ios::binary);out<<serialize(s);if(!out)throw std::runtime_error("Cannot write legacy fixture");
            std::cout<<"EMITTED "<<name<<" old actual paid original/copy bonus2 each, no attachments\n";
        }
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
    return 0;
}

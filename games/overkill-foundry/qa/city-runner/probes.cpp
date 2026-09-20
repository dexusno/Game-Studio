// Independent replay/parser and policy-observation probes. No production writes.
#include "city_runner.hpp"
#include "overkill/upgrades.hpp"
#include <algorithm>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace overkill;
using namespace overkill::runner;
namespace {
int passed=0,failed=0;
void require(bool yes,const std::string& why){if(!yes)throw std::runtime_error(why);}
void check(const char* name,const std::function<void()>& f){try{f();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cout<<"FAIL "<<name<<": "<<e.what()<<'\n';}}
std::string bytes(const std::string& s){require(s.size()%2==0,"odd hex");std::string out;for(std::size_t i=0;i<s.size();i+=2){const auto value=std::stoul(s.substr(i,2),nullptr,16);out.push_back(static_cast<char>(value));}return out;}
template<class T> void vector(std::istream& in,std::vector<T>& v){std::size_t size=0;in>>size;require(size<100000,"excessive fixture vector");v.resize(size);for(auto& item:v)in>>item;}
void materials(std::istream& in,Materials& m){std::size_t size=0;in>>size;require(size==5,"material count");for(auto& value:m)in>>value;}
void targets(std::istream& in,std::vector<SpreadTarget>& v){std::size_t size=0;in>>size;require(size<100000,"excessive fixture targets");v.resize(size);for(auto& item:v)in>>item.part>>item.enemy;}
// Deliberately does not call the runner's decodeAction or replayTrace.
CampaignAction command(const std::string& encoded){std::istringstream in(bytes(encoded));CampaignAction a;int outer=0,inner=0;auto& c=a.combat;
 in>>outer>>std::quoted(a.runId)>>a.sequence>>a.subject>>a.exchange>>a.target>>std::quoted(a.choice)>>a.quantity>>a.eventShop>>a.decline>>a.material;
 in>>inner>>c.subject>>c.target>>c.steering>>c.precision>>c.amount;a.type=static_cast<CampaignActionType>(outer);c.type=static_cast<ActionType>(inner);
 vector(in,c.parts);targets(in,c.spreadTargets);vector(in,c.choices);vector(in,c.targets);vector(in,c.sacrifices);targets(in,c.partTargets);targets(in,c.partChoices);materials(in,c.discarded);
 in>>c.upgradeChoice.choice;vector(in,c.upgradeChoice.objects);vector(in,c.upgradeChoice.values);in>>std::quoted(c.upgradeChoice.option);materials(in,c.upgradeChoice.removed);materials(in,c.upgradeChoice.added);in>>c.upgradeChoice.decline;materials(in,c.discount);in>>std::quoted(c.upgrade);
 require(static_cast<bool>(in),"incomplete independently decoded command");in>>std::ws;require(in.eof(),"extra command data");return a;
}
std::string decision(const Decision& d){return d.available?encodeAction(d.action):"unavailable:"+d.reason;}
CampaignAction action(const Campaign& c,CampaignActionType type,Id id=0,const std::string& choice={}){CampaignAction a;a.type=type;a.runId=c.runId;a.sequence=c.nextTransaction;a.subject=id;a.choice=choice;return a;}
void apply(const CampaignRules& rules,Campaign& c,const CampaignAction& a){const auto r=rules.apply(c,a);require(r.ok,r.reason);}
void inspectTrace(const std::string& path,const CampaignRules& rules){std::ifstream input(path,std::ios::binary);require(static_cast<bool>(input),"missing trace");std::string line;std::getline(input,line);require(line=="OFCITY2","expected current trace version");std::getline(input,line);require(line.rfind("META ",0)==0,"metadata absent");std::getline(input,line);require(line.rfind("START ",0)==0,"start absent");const auto start=bytes(line.substr(6));Campaign state;std::string error;require(deserializeCampaign(start,state,error),error);
 require(serializeCampaign(rules.newGame(state.seed,state.runId))==start,"runner start differs from production NewGame");require(state.fight.hp==80&&state.fight.maxHp==80&&state.fight.credits==100&&state.fight.memory.size()==12&&state.fight.parts.empty(),"baseline initial resources differ");
 std::size_t count=0,calibrations=0,recipeUses=0,collections=0;std::int64_t hpPaid=0;bool sawFinal=false;
 while(std::getline(input,line)){if(line.rfind("FINAL ",0)==0){std::istringstream final(line.substr(6));std::string hash,outcome,hexState;final>>hash>>outcome>>hexState;require(bytes(hexState)==serializeCampaign(state),"independent final campaign bytes differ");require(hash==campaignHash(state),"independent final hash differs");if(outcome=="city_complete")require(state.phase==CityPhase::Complete,"false win");if(outcome=="defeat")require(state.phase==CityPhase::Defeated,"false loss");std::cout<<"TRACE seed="<<state.seed<<" actions="<<count<<" recipe_uses="<<recipeUses<<" collections="<<collections<<" technician_payments="<<calibrations<<" known_hp_paid="<<hpPaid<<" outcome="<<outcome<<" hash="<<hash<<'\n';sawFinal=true;break;}
  require(line.rfind("A ",0)==0,"missing action");const auto a=command(line.substr(2));const auto before=state;const auto result=rules.apply(state,a);++count;
  std::getline(input,line);require(line.rfind("R ",0)==0,"missing result");std::istringstream row(line.substr(2));bool ok=false;std::string reason,hash;std::size_t n=0;row>>ok>>std::quoted(reason)>>hash>>n;require(ok==result.ok&&reason==result.reason&&hash==campaignHash(state)&&n==result.events.size(),"independent action result mismatch");
  if(result.ok&&a.type==CampaignActionType::AcceptCalibration){++calibrations;hpPaid+=8;std::cout<<"TECHNICIAN command="<<count<<" before_hp="<<before.fight.hp<<" after_hp="<<state.fight.hp<<" item="<<a.choice<<'\n';}
  for(const auto& event:result.events){std::getline(input,line);require(line=="E "+eventJson(event),"independent event mismatch");if(event.type=="hp_cost"||event.type=="acquisition_hp_cost")hpPaid+=event.amount;if(event.type=="recipe_used")++recipeUses;if(event.type=="collected")++collections;}
 }
 require(sawFinal,"trace lacks final");require(!std::getline(input,line),"extra trace lines");
}
}
int main(int argc,char** argv){Rules fights;CampaignRules rules(fights,cinderwallUpgradeHooks(fights));
 for(int i=1;i<argc;++i){const auto path=std::string(argv[i]);check(("R01 independent full-state replay "+std::to_string(i)).c_str(),[&]{inspectTrace(path,rules);});}
 check("R02 policy ignores unseen later route slots and all RNG streams",[&]{
  auto c=rules.newGame(31,"qa-policy-observation");apply(rules,c,action(c,CampaignActionType::ChooseMayor,0,c.mayorOffers.front()));while(!c.fight.upgradeChoices.empty()||!c.upgradeOffers.empty()){Options o;Policy p(fights,rules,o);const auto d=p.choose(c);require(d.available,"setup choice");apply(rules,c,d.action);}c.shop.clear();c.cores.clear();
  const auto original=serializeCampaign(c);Options options;options.policy="defensive";Policy p(fights,rules,options);auto first=p.choose(c);const auto expected=decision(first.action.type==CampaignActionType::OpenShop?p.choose(c):first);require(serializeCampaign(c)==original,"choose mutated caller");
  for(std::uint64_t seed=500;seed<520;++seed){auto hidden=c;hidden.seed=seed;hidden.rng=Rng::seeded(seed,"different-hidden");hidden.fight.seed=seed;hidden.fight.rng=Rng::seeded(seed,"different-fight");hidden.route.seed=seed;hidden.route.rng=Rng::seeded(seed,"different-route");for(std::size_t n=static_cast<std::size_t>(hidden.route.position);n<hidden.route.nodes.size();++n){hidden.route.officer[n]=!hidden.route.officer[n];hidden.route.mystery[n]=!hidden.route.mystery[n];for(auto& offer:hidden.route.nodes[n].offers){offer.encounterSeed=seed;offer.mystery="C1-M-PATROL";offer.formation="C1-F-GATEBREAKER";}}Policy other(fights,rules,options);auto x=other.choose(hidden);require(decision(x.action.type==CampaignActionType::OpenShop?other.choose(hidden):x)==expected,"future/seed altered route choice");}
 });
 check("R04 future enemy pattern changes cannot alter current combat decision",[&]{
  auto c=rules.newGame(2,"qa-observable-combat");Options options;Policy setup(fights,rules,options);for(int n=0;n<20&&c.phase!=CityPhase::Between;++n){const auto d=setup.choose(c);require(d.available,"ordinary setup stalled");apply(rules,c,d.action);}require(c.phase==CityPhase::Between,"Mayor setup unfinished");const auto& offers=routeOffers(c.route);const auto offer=std::find_if(offers.begin(),offers.end(),[](const RouteOffer& o){return o.kind==EncounterKind::Regular;});require(offer!=offers.end(),"no regular fixture offer");apply(rules,c,action(c,CampaignActionType::EnterOffer,offer->id));for(int n=0;n<20&&(!c.fight.upgradeChoices.empty()||!c.upgradeOffers.empty());++n){const auto d=setup.choose(c);require(d.available,"entry choice stalled");apply(rules,c,d.action);}auto collect=action(c,CampaignActionType::Combat);collect.combat=Action::collect(0);apply(rules,c,collect);const auto original=serializeCampaign(c);Policy first(fights,rules,options);const auto expected=decision(first.choose(c));require(serializeCampaign(c)==original,"combat preview modified state");
  for(std::uint64_t seed=11;seed<31;++seed){auto hidden=c;hidden.fight.rng=Rng::seeded(seed,"hidden-future");for(auto& e:hidden.fight.enemies){e.pattern={{Move::Attack,999,9},{Move::Escape,0,0}};e.patternRandom=seed;}Policy other(fights,rules,options);require(decision(other.choose(hidden))==expected,"uncommitted future intent changed action");}
 });
 check("R03 source inventory and price mutations remain production-validated",[&]{auto c=rules.newGame(7,"qa-no-freebies");auto before=serializeCampaign(c);auto illegal=action(c,CampaignActionType::Buy,999999);require(!rules.apply(c,illegal).ok&&serializeCampaign(c)==before,"bad purchase mutated baseline");CampaignAction afterCodec;std::string error;auto nested=action(c,CampaignActionType::Combat);nested.combat=Action::collect(3,2);nested.combat.discount={1,2,3,4,5};nested.combat.partChoices={{11,22}};nested.combat.sacrifices={333};nested.combat.upgrade="MY1-10";require(decodeAction(encodeAction(nested),afterCodec,error)&&encodeAction(afterCodec)==encodeAction(nested),"typed payload lost through trace codec");});
 std::cout<<"SUMMARY "<<passed<<" passed, "<<failed<<" failed\n";return failed?1:0;
}

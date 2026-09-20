#include "overkill/upgrades.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace overkill {
const OwnedUpgrade* ownedUpgrade(const State& s,const std::string& id){for(const auto& u:s.upgrades)if(u.id==id)return &u;return nullptr;}
OwnedUpgrade* ownedUpgrade(State& s,const std::string& id){for(auto& u:s.upgrades)if(u.id==id)return &u;return nullptr;}
Amount upgradeCounter(const OwnedUpgrade& u,const std::string& key){for(const auto& c:u.counters)if(c.key==key)return c.value;return 0;}
std::vector<std::string> eligibleUpgradeIds(){
    std::vector<std::string> out={"MAU-01","MAU-02","MAU-03","MAU-04","MAU-05","MAU-07","MY1-01","MY1-02","MY1-M1","MY1-M2","MY1-M3","MY2-01","MY2-02","MY2-03","MY3-02","MY3-03"};
    for(int n=3;n<=23;++n)out.push_back("MY1-"+std::string(n<10?"0":"")+std::to_string(n));
    for(int n:{1,2,4,5,6,7,9,10,11,12,13,14,15,16,17,18,19,21,22,24,25,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,44,45,46,47,48,49,50,51,53,55,56,57,58,60,61,62,64,65,66,69,70,71,72,73,74,75,76,77,83,85,86,87,88,89,90,91,93,94,95,96,97,99,100,101,102,105,106,107,108,109,113,115,116,117,119,120,121,122,123,125,126,127,129,131,132,133,134,136,139,140,141,142,144,145,146,147,148,149,150})
        out.push_back("UGS-"+std::string(n<10?"00":n<100?"0":"")+std::to_string(n));
    std::sort(out.begin(),out.end());return out;
}
bool upgradeEligible(const Rules& r,const State& s,const std::string& id){
    const auto ids=eligibleUpgradeIds();if(!std::binary_search(ids.begin(),ids.end(),id)||ownedUpgrade(s,id))return false;
    Amount ammo=0,shield=0,utilityCd=0,ammoCd=0,none=0,outputs=0,permanent=0;
    for(const auto& c:s.memory){if(c.storage==MemoryKind::Borrowed)continue;const auto* p=r.recipe(c.recipe);if(!p)continue;++permanent;if(p->kind==Kind::Ammo){++ammo;if(p->cooldown>0)++ammoCd;}if(p->kind==Kind::Shield)++shield;if(p->kind==Kind::Utility&&p->cooldown>0)++utilityCd;if(p->cooldown==0)++none;if(p->kind==Kind::Ammo||p->kind==Kind::Shield)++outputs;}
    if(id=="UGS-039")return s.hp>8&&ammo>0&&shield>0;
    if(id=="UGS-144")return utilityCd>=2;if(id=="UGS-145")return ammoCd>=2;
    if(id=="MY1-21")return none>0;if(id=="MY1-23")return outputs>0;
    if(id=="UGS-119")return outputs>=2;if(id=="UGS-102")return ammo>0;
    if(id=="UGS-032")return permanent>0;
    return true;
}
bool isUnusedPart(const Part& p){return !p.everInstalled||(p.kind==Kind::Shield&&p.effects.size()==1&&p.effects[0].op==Op::ShieldValue&&p.shield==p.originalValue&&p.paidHeat==0&&p.paidHp==0&&p.attachments.empty());}
Amount upgradeHeatCap(const State& s){return ownedUpgrade(s,"MAU-02")?14:10;}
Amount upgradeHeatDecay(const State& s){return ownedUpgrade(s,"MAU-04")?1:2;}
Amount upgradePrecisionWidthPercent(const State& s){const auto* u=ownedUpgrade(s,"UGS-126");return 100+(ownedUpgrade(s,"UGS-058")?20:0)+(u&&upgradeCounter(*u,"awakened")?10:0);}
Amount upgradeGeneralMemoryBonus(const State& s){return (ownedUpgrade(s,"MY1-06")?4:0)+(ownedUpgrade(s,"MY2-03")?4:0);}
Amount upgradeUtilityMemoryBonus(const State& s){return ownedUpgrade(s,"UGS-100")?2:0;}
Amount upgradeShopPrice(const State& s,PurchaseKind k,Amount price){if(price<0)throw std::runtime_error("Negative shop price.");if(!price)return 0;if(k!=PurchaseKind::Part&&ownedUpgrade(s,"UGS-075"))return std::max(1,static_cast<Amount>(static_cast<std::int64_t>(price)*80/100));return price;}
Amount upgradeCoreSaleValue(const State& s,Amount value){if(value<0)throw std::runtime_error("Negative core value.");const auto n=static_cast<std::int64_t>(value)+(ownedUpgrade(s,"UGS-017")?value/4:0);if(n>std::numeric_limits<Amount>::max())throw std::runtime_error("Core sale exceeds integer range.");return static_cast<Amount>(n);}
Amount upgradePartSaleValue(const State&,const Part& p,const Materials& prices){if(p.place!=Place::Reserve||!isUnusedPart(p))return -1;std::int64_t n=0;for(std::size_t i=0;i<5;++i){if(prices[i]<0||p.materialBasis[i]<0)throw std::runtime_error("Negative canonical resale ingredient.");n+=static_cast<std::int64_t>(prices[i])*p.materialBasis[i];}n/=2;if(n>std::numeric_limits<Amount>::max())throw std::runtime_error("Part sale exceeds integer range.");return static_cast<Amount>(n);}
bool acknowledgeUpgradeRequest(State& s,Id id){const auto i=std::find_if(s.upgradeRequests.begin(),s.upgradeRequests.end(),[id](const UpgradeRequest& r){return r.id==id;});if(i==s.upgradeRequests.end())return false;s.upgradeRequests.erase(i);return true;}
} // namespace overkill

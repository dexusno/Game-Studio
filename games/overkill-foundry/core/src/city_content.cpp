#include "overkill/city_content.hpp"
#include <algorithm>
#include <stdexcept>

namespace overkill {
CityContent cinderwallContent(){
    CityContent result;
#include "city_content.generated.inc"
    return result;
}
const CityItem* cityItem(const std::vector<CityItem>& items,const std::string& id){
    const auto it=std::find_if(items.begin(),items.end(),[&](const CityItem& x){return x.id==id;});
    return it==items.end()?nullptr:&*it;
}
std::vector<std::string> drawCityOffer(const std::vector<CityItem>& items,const std::vector<CityPool>& pools,
    const std::string& source,Rng& rng,Domain domain,Amount count,const std::vector<std::string>& excluded,
    const std::function<bool(const std::string&)>& eligible){
    if(count<0)throw std::invalid_argument("Negative offer count.");
    const auto pool=std::find_if(pools.begin(),pools.end(),[&](const CityPool& p){return p.name==source;});
    if(pool==pools.end())throw std::invalid_argument("Unknown offer source: "+source);
    std::array<std::vector<std::string>,5> byRarity;
    for(const auto& id:pool->ids){
        const auto* item=cityItem(items,id);
        if(!item || item->rarity<0 || item->rarity>4)throw std::invalid_argument("Broken content-pool reference: "+id);
        if(std::find(excluded.begin(),excluded.end(),id)!=excluded.end() || (eligible && !eligible(id)))continue;
        byRarity[static_cast<std::size_t>(item->rarity)].push_back(id);
    }
    auto copy=rng;std::vector<std::string> out;
    for(Amount slot=0;slot<count;++slot){
        std::uint32_t total=0;
        for(std::size_t i=0;i<5;++i){if(pool->weights[i]<0)throw std::invalid_argument("Negative rarity weight.");if(!byRarity[i].empty())total+=static_cast<std::uint32_t>(pool->weights[i]);}
        if(total==0)break;
        auto draw=copy.below(domain,total);std::size_t rarity=0;
        for(;rarity<5;++rarity){const auto weight=byRarity[rarity].empty()?0U:static_cast<std::uint32_t>(pool->weights[rarity]);if(draw<weight)break;draw-=weight;}
        auto& choices=byRarity[rarity];const auto index=copy.below(domain,static_cast<std::uint32_t>(choices.size()));
        out.push_back(choices[index]);choices.erase(choices.begin()+index);
    }
    rng=copy;return out;
}
} // namespace overkill

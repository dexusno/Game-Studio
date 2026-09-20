#include "overkill/core.hpp"
#include <algorithm>
namespace overkill {
std::vector<Recipe> Rules::completeContent() {
    std::vector<Recipe> result{
#include "catalogue_data.inc"
    };
    // Preserve the verified primitive definitions; attach the complete immutable metadata.
    for (const auto& primitive:starterContent()) {
        auto it=std::find_if(result.begin(),result.end(),[&](const Recipe& r){return r.id==primitive.id;});
        if(it!=result.end()) { it->effects=primitive.effects; it->automaticOutput=primitive.automaticOutput; }
    }
    return result;
}
std::vector<std::string> Rules::implementedRecipeIds() {
    // Runtime registration only. Independent semantic tests and release evidence remain separate.
    std::vector<std::string> ids;
    for(const auto& r:completeContent())ids.push_back(r.id);
    return ids;
}
}

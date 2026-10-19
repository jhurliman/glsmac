#include "game/frontend/SmacResourceArt.h"
#include <iostream>
#include <stdexcept>
int main() {
    unsigned count;std::cin>>count;
    for(unsigned c=0;c<count;++c) {
        int32_t x,y,pattern;unsigned band;uint32_t live,flags;bool automatic;int zoom;
        std::cin>>x>>y>>band>>live>>pattern>>automatic>>flags>>zoom;
        if(!std::cin) {throw std::runtime_error("Truncated resource art context");}
        const auto picks=game::frontend::smac_resource_art::Select(x,y,uint8_t(band),live,pattern,automatic,flags,zoom);
        std::cout<<picks.size();
        for(const auto& pick:picks) {
            std::cout<<' '<<static_cast<int>(pick.stage)<<' '<<pick.shadow;
            for(auto value:pick.crop) {std::cout<<' '<<value;}
        }
        std::cout<<'\n';
    }
}

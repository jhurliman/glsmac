#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>

namespace game::backend::faction {
// Original 0x5d17b0: effective query, not the raw possession grant. Numeric
// prerequisite sentinels are -1 (none), below -1 (disabled), otherwise IDs.
inline bool QuerySmacTechnology(int32_t faction,int32_t technology,uint8_t ownership,std::array<int32_t,2> prerequisites) {
    if(faction<=0) {return false;}
    if(faction>7) {throw std::invalid_argument("Technology query requires an original faction identity");}
    if(technology==-1) {return true;}
    if(technology<0 || technology>=88) {return false;}
    if(prerequisites[0]<-1 || (prerequisites[1]<-1 && prerequisites[0]!=-1)) {return false;}
    return ownership&(1u<<faction);
}
}

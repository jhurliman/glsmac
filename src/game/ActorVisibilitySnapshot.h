#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace game::actor_visibility {
struct Stack {int16_t id,next,previous;};
struct Unit {
    std::array<int32_t,2> tile;
    int32_t owner;
    std::optional<uint8_t> visible_to;
    std::optional<uint16_t> flags;
    std::optional<uint32_t> status;
    std::optional<Stack> stack;
    std::optional<uint8_t> order;
};
struct Base {
    std::array<int32_t,2> tile;
    int32_t owner;
    std::optional<uint8_t> known_by;
};
// Owned local presentation inputs. Enclosing frontend-response world tickets
// reject retired worlds; unavailable authority is not a reveal fallback.
struct Snapshot {
    bool available=false,horizontal_wrap=false;
    int32_t width=0,height=0,viewer=0;
    uint8_t observers=0,revealed_by=0;
    std::vector<uint8_t> explored_by;
    std::map<uint64_t,Unit> units;
    std::map<uint64_t,Base> bases;
    bool Known(int32_t x,int32_t y) const noexcept {
        if(!available || viewer<1 || viewer>7 || !(observers&(1u<<viewer))
            || width<=0 || (width&1) || height<=0 || x<0 || x>=width || y<0 || y>=height || ((x^y)&1)) {return false;}
        if(uint64_t(width/2)*uint64_t(height)!=explored_by.size()) {return false;}
        const auto index=uint64_t(y)*uint64_t(width/2)+uint64_t(x/2);
        return index<explored_by.size() && ((explored_by[index]|revealed_by)&(1u<<viewer));
    }
};
}

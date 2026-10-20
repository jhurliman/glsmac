#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace game::actor_visibility {
struct Occupancy {uint8_t tag,presence;};
struct Stack {int16_t id,next,previous;};
struct Unit {
    std::array<int32_t,2> tile;
    int32_t owner;
    std::optional<uint8_t> visible_to;
    std::optional<uint16_t> flags;
    std::optional<uint32_t> status;
    std::optional<Stack> stack;
    std::optional<uint8_t> order;
    bool working=false;
    std::optional<uint8_t> domain;
    std::optional<int32_t> chassis;
};
struct BaseVisibility {bool body=false,name=false;};
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
    std::vector<Occupancy> occupancy;
    std::map<uint64_t,Unit> units;
    std::map<uint64_t,Base> bases;
    bool HasViewer() const noexcept {
        return available && viewer>=1 && viewer<=7 && (observers&(1u<<viewer))
            && width>0 && !(width&1) && height>0
            && uint64_t(width/2)*uint64_t(height)==explored_by.size();
    }
    BaseVisibility BaseDisclosure(uint64_t id) const noexcept {
        if(!HasViewer()) {return {};}
        const auto found=bases.find(id);if(found==bases.end()) {return {};}
        const auto& base=found->second;const auto x=base.tile[0],y=base.tile[1];
        if(x<0 || x>=width || y<0 || y>=height || ((x^y)&1)) {return {};}
        const bool disclosed=base.owner==viewer || (base.known_by && (*base.known_by&(1u<<viewer)));
        return {disclosed && Known(x,y),disclosed};
    }
    bool Known(int32_t x,int32_t y) const noexcept {
        if(!available || viewer<1 || viewer>7 || !(observers&(1u<<viewer))
            || width<=0 || (width&1) || height<=0 || x<0 || x>=width || y<0 || y>=height || ((x^y)&1)) {return false;}
        if(uint64_t(width/2)*uint64_t(height)!=explored_by.size()) {return false;}
        const auto index=uint64_t(y)*uint64_t(width/2)+uint64_t(x/2);
        return index<explored_by.size() && ((explored_by[index]|revealed_by)&(1u<<viewer));
    }
};
}

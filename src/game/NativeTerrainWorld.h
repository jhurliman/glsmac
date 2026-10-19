#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace game::native_terrain {
// Original Base-specific owner/visibility gate after a matching live record.
inline bool AdmitsBase(int viewer,int owner,uint8_t known_by) {
    return viewer>0 && viewer<8 && (owner==viewer || (known_by&(1u<<viewer)));
}
// Owned rendering inputs copied on the backend thread. No Tile/Map pointers,
// gameplay RNG state, assets or serialization lifetime cross this boundary.
struct Tile {
    uint8_t band=0,moisture=0,rockiness=0,contour=0;
    uint32_t items=0,selected_items=0;
    bool admitted_base=false;
    bool developed_ready=false,selected_known=false,center_known=false,base_present=false;
    uint8_t cached_affiliation=15;
    int64_t nutrients=0;
    uint32_t landmarks=0;
    uint8_t region=0;
    int8_t owner=-1;
    bool ordinary=false,deformed=false;
};
struct World {
    static constexpr int WIDTH=80,HEIGHT=80;
    bool horizontal_wrap=true;
    int viewer=0;
    std::array<uint32_t,8> relations{};
    std::array<std::string,8> faction_files{};
    std::array<Tile,WIDTH*HEIGHT/2> tiles{};
};
}

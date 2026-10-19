#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace game::native_terrain {
// Owned rendering inputs copied on the backend thread. No Tile/Map pointers,
// gameplay RNG state, assets or serialization lifetime cross this boundary.
struct Tile {
    uint8_t band=0,moisture=0,rockiness=0,contour=0;
    uint32_t items=0;
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

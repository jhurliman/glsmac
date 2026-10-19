#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace game::frontend::smac_land_material {

using Point=std::array<int,2>;
using Crop=std::array<int,4>;
struct Tile {uint8_t band,moisture,rockiness;uint32_t items;};
// Caller owns topology and observer admission. Invalid/clipped coordinates
// return absence. No installed-world lookup or implicit map wrapping occurs.
using TileAt=std::function<std::optional<Tile>(int,int)>;
enum class Role {Base,Rock,Forest,Fungus};
struct Pass {
    Role role;
    int fan; // 0..3 for triangles, -1 for W/N/E/S quads.
    Crop crop;
    std::array<Point,4> uv;
    uint8_t vertices;
    std::optional<uint32_t> flags;
};

// The original variation seed is the tile-record address's low16 bits.
// The caller must supply an explicit compatibility value; this function never
// hashes coordinates or consumes a gameplay/presentation RNG stream.
std::vector<Pass> Select(Point center,const TileAt& tile_at,uint16_t variation_seed_low16);

using Rgb=std::array<uint8_t,3>;
using Palette=std::array<Rgb,256>;
// Preserves supplied active reserved/system entries; overwrites only10..245.
Palette CommonGamma100(const Palette& common,const Palette& active);
uint8_t LoadedIndex(uint8_t source);
std::optional<uint8_t> FinalIndex(uint8_t loaded,int shade,bool keyed);
std::vector<uint8_t> ResolveRgba(const std::vector<uint8_t>& indices,const Palette& palette);

}

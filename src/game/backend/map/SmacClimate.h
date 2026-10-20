#pragma once
#include "game/backend/SmacGameplayRandomState.h"
#include <array>
#include <functional>
#include <optional>
#include <vector>

namespace game::backend::map::smac_climate {
using Coordinate=std::array<int32_t,2>;
struct Tile {
    // Retained altitude uses all three bits (0..7); reconstruction classifies contours to0..6.
    int32_t elevation,temperature,moisture,contour,aux;
    uint32_t items,landmarks;
    int32_t cached_occupant=15,rockiness=0;
    std::array<uint32_t,7> remembered_items{};
};
// Detached evolving plane. Raw half-X addressing intentionally admits the
// intermediate odd coordinates used by the original rainfall influence pass.
struct World {
    int32_t width,height;bool horizontal_wrap;
    std::vector<Tile> tiles;
    uint32_t dirty_flags=0;
    std::array<int32_t,8> flood_counts{};
    std::array<int32_t,8> affected_bases{{-1,-1,-1,-1,-1,-1,-1,-1}};
    std::optional<Coordinate> mount_planet;
    void Validate() const;
    Tile* At(int32_t x,int32_t y);
    const Tile* At(int32_t x,int32_t y) const;
};
struct Rules {
    uint32_t pattern;
    int32_t extra,temperature,rainfall,latitude_divisor,band_divisor,jitter_divisor,warming_divisor;
    int32_t mountain_shadow,hill_shadow,shadow_step;
};
struct Streams {
    SmacGameplayRandomState& state;
    uint64_t shared_draws=0,crt_draws=0;
    // Optional bounded-control tracing; production callers can omit allocation.
    std::vector<uint32_t>* seed_requests=nullptr;
    std::vector<uint16_t>* crt_values=nullptr;
    // Synchronous observation only; callbacks must not mutate/reenter streams.
    std::function<void(uint32_t,uint32_t)> on_seed;
    std::function<void(uint32_t,uint32_t,int32_t,int32_t,int32_t)> on_shared;
    void Seed(uint32_t seed);
    int32_t Shared(int32_t low,int32_t high);
    uint16_t Crt();
};
struct Context {
    int32_t sea_offset,turn;
    bool terrain_only;
    std::function<bool(const World&,int32_t,int32_t)> resource_bonus;
    std::function<void(uint32_t,uint8_t)> invalidate_landmark;
    // Called after item/memory masking, before final class storage. The real
    // Base/Unit cleanup and stable compaction belong to this owning transaction.
    std::function<void(World&,int32_t,int32_t,int32_t,int32_t,Streams&)> entity_change;
};
int32_t ClassifyContour(int32_t contour,int32_t sea_offset);
void AssignSurfaceClass(World& world,int32_t x,int32_t y,int32_t category,Streams& random,const Context& context);
void ShapeHeight(World& world,int32_t x,int32_t y,int32_t category,Streams& random,const Context& context);
void RecalculateAltitudes(World& world,Streams& random,const Context& context);
void EcologyVolcano(World& world,int32_t x,int32_t y,Streams& random,const Context& context);
void Temperature(World& world,const Rules& rules,Streams& random,const std::function<void(int32_t)>& row_complete={});
void Rivers(World& world,Streams& random);
void Rainfall(World& world,const Rules& rules);
// First four passes only. Caller must still stage regions/coasts/territory,
// final clock reseed, memory/cache notifications and renderer publication.
void Recalculate(World& world,const Rules& rules,Streams& random,const Context& context);
}

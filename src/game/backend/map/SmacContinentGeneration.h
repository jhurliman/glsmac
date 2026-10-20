#pragma once
#include "SmacGenerationBootstrap.h"
#include <functional>

namespace game::backend::map::smac_generation {
using Coordinate=std::array<int32_t,2>;
struct Geometry {int32_t width,height;};
struct ContinentRules {
    int32_t base,modifier;
    std::array<int32_t,5> ratios;
    uint32_t primary;
};
enum class CrtUse {CandidateX,CandidateY,SmallSize,ConditionalSize,Direction,Contour};
// Optional synchronous observation/progress. Callbacks must not mutate or
// reenter the detached State. Omit this object to avoid trace allocation.
struct ContinentTrace {
    uint64_t writes=0,draws=0,paints=0,pumps=0,direction_corrections=0;
    std::function<void(uint32_t,uint8_t)> on_write;
    std::function<void(CrtUse,uint32_t,uint32_t,uint16_t)> on_draw;
    std::function<void(Coordinate)> on_candidate;
    std::function<void(Coordinate,int32_t,int32_t)> on_walk;
    std::function<void(Coordinate,int32_t)> on_paint;
    std::function<void(int32_t,int32_t,int32_t)> on_direction;
    std::function<void()> checkpoint;
};
// Turn-zero native 0x5da7f0/0x5da700/0x5da370. Ordinal and outer attempt
// counters belong to the caller; these callees mutate the existing raw plane.
void SeedContinent(State& state,Geometry geometry,const ContinentRules& rules,int32_t ordinal,ContinentTrace* trace=nullptr);
void GrowContinent(State& state,Geometry geometry,const ContinentRules& rules,Coordinate seed,int32_t category,int32_t target,ContinentTrace* trace=nullptr);
void PaintContinent(State& state,Geometry geometry,const ContinentRules& rules,Coordinate point,int32_t category,ContinentTrace* trace=nullptr);
}

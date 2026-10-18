#pragma once
#include "game/backend/map/SmacSiteQuality.h"
#include <vector>
#include <utility>
// Two original disposition/search cache misses, separate from direct query cases.
// RE-SMAC-SITE-QUALITY-001 source JSON SHA256: 427f4e3364a5add2619b9af2d793dc3f42b1e744ee11065a006d74a555cdf840
// Native post-loss terrain postpass supplies F1=960 (restart-input-lifetimes receipt).
namespace gse::tests::smac_restart_miss_fixture {
namespace site=game::backend::map::smac_site;
struct Case {const char* name;site::Geometry geometry;int32_t turn;site::Tile fallback;std::vector<std::pair<site::Coordinate,site::Tile>> overrides;site::Areas areas;site::Coordinate selected;uint32_t crt,final_crt;uint8_t before,after;};
inline const std::vector<Case> cases{
{"clear_rainy_cache_miss",{64,32,true},3,{3,3,0,1,false,false,false,false,false,false,false,false,false,false,false,false,false,0},{},{{0,1024,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},{{8,24}},2038257055u,2380015889u,0,1},
{"monolith_zero_cache_miss",{64,32,true},3,{3,3,0,1,false,false,false,false,false,false,false,false,false,false,false,false,false,0},{{{{8,24}},{3,3,0,1,false,true,false,false,false,false,false,false,false,false,false,false,false,0}}},{{0,1024,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},{{8,24}},2038257055u,2380015889u,0,0},
};
}

#pragma once
#include "game/backend/resource/SmacResourceBonus.h"
#include "game/backend/resource/SmacSiteSignal.h"
#include <array>
#include <vector>
#include <string_view>

namespace game::frontend::smac_resource_art {
enum class Stage { Resource, Pod, Monolith };
struct Pick {std::array<int,4> crop;bool shadow;Stage stage;};
inline bool ReplacesLegacySprite(std::string_view name) {
    for(const auto candidate:{"NutrientBonusLand","NutrientBonusSea","EnergyBonusLand","EnergyBonusSea",
        "MineralsBonusLand","MineralsBonusSea","UnityPodLand","UnityPodSea","Monolith"}) {
        if(name==candidate) {return true;}
    }
    return false;
}

// Live native item word, independent of remembered improvement authority.
// Resource and site queries are the existing source-backed gameplay resolvers.
inline std::vector<Pick> Select(int32_t x,int32_t y,uint8_t band,uint32_t live,
    int32_t pattern,bool automatic,uint32_t flags,int zoom=0) {
    if(!(flags&0x200u) || zoom<=-8) {return {};}
    namespace rules=backend::resource::smac;
    const auto altitude=static_cast<rules::AltitudeBand>(band);
    const auto kind=rules::ResolveResourceBonus({x,y,altitude,pattern,automatic,
        bool(live&0x400u),bool(live&0x20000000u),bool(live&0x10000u),bool(live&0x20000u)});
    const auto site=rules::ResolveSiteSignal({x,y,altitude,live,automatic,pattern,11,61});
    const bool sea=band<3;
    std::vector<Pick> result;
    if(kind!=rules::ResourceBonus::None) {
        const int parity=(x/2+y)&1;
        result.push_back({{{(sea?1:203)+101*parity,253+63*(static_cast<int>(kind)-1),100,62}},true,Stage::Resource});
    }
    if(site) {
        const int variant=int((int64_t(7)*x+int64_t(5)*y)%3);
        result.push_back({{{418+202*variant+(sea?0:101),379,100,62}},false,Stage::Pod});
    }
    if(live&0x2000u) {result.push_back({{{304,1,100,62}},true,Stage::Monolith});}
    return result;
}
}

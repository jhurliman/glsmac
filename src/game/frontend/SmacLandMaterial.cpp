#include "SmacLandMaterial.h"
#include "SmacLandMaterialTables.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace game::frontend::smac_land_material {
namespace {
void Validate(const Tile& tile) {
    if(tile.band>7 || tile.moisture>2 || tile.rockiness>2) {throw std::invalid_argument("Unsupported ordinary terrain fields");}
}
}

std::vector<Pass> Select(Point center,const TileAt& tile_at,uint16_t seed) {
    if(!tile_at || center[0]< -32765 || center[0]>32764 || center[1]< -32765 || center[1]>32764) {
        throw std::invalid_argument("Invalid ordinary terrain material lookup");
    }
    const auto own=tile_at(center[0],center[1]);
    if(!own || own->band<3) {throw std::invalid_argument("Ordinary land material requires admitted land");}
    Validate(*own);
    const auto neighbor=[&](Point offset) {
        const auto tile=tile_at(center[0]+offset[0],center[1]+offset[1]);
        if(tile) {Validate(*tile);}return tile;
    };
    unsigned moisture_mask=0,fungus_mask=0;
    for(unsigned bit=0;bit<8;++bit) {
        const auto tile=neighbor(tables::NEIGHBORS[(bit+7)&7]);
        if(!tile) {continue;}
        if(tile->band<3 || tile->moisture>=own->moisture) {moisture_mask|=1U<<bit;}
        if(tile->band>=2 && (tile->items&0x20)) {fungus_mask|=1U<<bit;}
    }
    const auto pattern=tables::PATTERN[moisture_mask];
    const auto variation=(((uint32_t(seed)*1664525U+1013904223U)&65535U)*2U)>>16;
    const bool farm=(own->items&0x8000)!=0;
    const auto crop=farm?tables::FARM[3*variation+own->moisture]:own->moisture==0?tables::ARID:
        (own->moisture==1?tables::MOIST:tables::RAINY)[static_cast<size_t>(pattern[0])];
    std::vector<Pass> result;result.reserve(10);
    for(int fan=0;fan<4;++fan) {
        const auto turn=own->moisture && !farm?pattern[1]:0;
        result.push_back({Role::Base,fan,crop,tables::CANONICAL_UV[static_cast<size_t>((fan+turn)&3)],3,0xffffffffU});
        if(own->rockiness) {
            result.push_back({Role::Rock,fan,tables::ROCKS[2*size_t(own->rockiness-1)+variation],
                tables::CANONICAL_UV[static_cast<size_t>(fan)],3,0xffffffffU});
        }
    }
    if(own->items&0x200000) {
        unsigned mask=0;
        for(unsigned bit=0;bit<4;++bit) {const auto tile=neighbor(tables::FOREST_NEIGHBORS[bit]);
            if(tile && (tile->items&0x200000)) {mask|=1U<<bit;}}
        result.push_back({Role::Forest,-1,tables::FOREST[mask],{{{0,0},{55,0},{55,55},{0,55}}},4,{}});
    }
    if(own->items&0x20) {
        const auto fungus=tables::PATTERN[fungus_mask];
        std::array<Point,4> uv{};
        for(size_t i=0;i<4;++i) {uv[i]=tables::CANONICAL_UV[(i+static_cast<size_t>(fungus[1]))&3][1];}
        result.push_back({Role::Fungus,-1,tables::FUNGUS[static_cast<size_t>(fungus[0])],uv,4,0xffffffffU});
    }
    return result;
}

Palette CommonGamma100(const Palette& common,const Palette& active) {
    auto result=active;
    for(size_t index=0;index<236;++index) {for(size_t channel=0;channel<3;++channel) {
        result[index+10][channel]=std::min<uint8_t>(common[index][channel],235);
    }}
    return result;
}
uint8_t LoadedIndex(uint8_t source) {return static_cast<uint8_t>(source+10);}
std::optional<uint8_t> FinalIndex(uint8_t loaded,int shade,bool keyed) {
    if(keyed && loaded==9) {return {};}
    return static_cast<uint8_t>(loaded+static_cast<uint8_t>(shade));
}
std::vector<uint8_t> ResolveRgba(const std::vector<uint8_t>& indices,const Palette& palette) {
    if(indices.size()>std::numeric_limits<size_t>::max()/4) {throw std::length_error("Terrain RGBA size overflow");}
    std::vector<uint8_t> result(indices.size()*4);
    for(size_t i=0;i<indices.size();++i) {
        const auto& color=palette[indices[i]];
        for(size_t channel=0;channel<3;++channel) {result[4*i+channel]=color[channel];}result[4*i+3]=255;
    }
    return result;
}
}

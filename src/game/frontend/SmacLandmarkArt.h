#pragma once
#include "SmacTerrainRaster.h"
#include "SmacLandmarkArtTables.h"
#include "SmacLandMaterialTables.h"
#include <stdexcept>
#include <optional>

namespace game::frontend::smac_landmark_art {
namespace raster=smac_terrain_raster;
struct Pass {
    std::array<int,4> crop;
    raster::Polygon polygon;
    raster::Shader shader;
    std::array<int32_t,2> shade_limits;
};
using Crop=std::array<int,4>;
struct Neighbor {uint8_t band;uint32_t landmarks;};
inline std::optional<Pass> Jungle(uint32_t word,bool land,
    const std::array<raster::Point,5>& points,const std::array<std::optional<Neighbor>,8>& neighbors) {
    if(!land || (word&0x80000004u)!=4u) {return {};}
    unsigned mask=0;
    for(unsigned bit=0;bit<8;++bit) {
        const auto& cell=neighbors[(bit+7)&7];
        if(cell && cell->band>=3 && (cell->landmarks&0x80000004u)==4u) {mask|=1u<<bit;}
    }
    const auto& pattern=smac_land_material::tables::PATTERN[mask];
    const int index=pattern[0];
    Pass pass{{526+57*(index%4),259+57*(index/4),56,56},{},raster::Shader::KeyedUnlit,{{-4,4}}};
    pass.polygon.count=4;
    for(unsigned v=0;v<4;++v) {
        pass.polygon.vertices[v]={points[v+1],smac_land_material::tables::CANONICAL_UV[(pattern[1]+v)&3][1],0};
    }
    return pass;
}
inline std::optional<Crop> Dunes(uint32_t word,bool land,uint32_t selected_items,uint32_t flags) {
    if(!land || !(word&0x40u) || ((flags&1u) && (selected_items&0x8000u))) {return {};}
    return Crop{{229,1,56,56}};
}
inline std::optional<Crop> EarlyMarker(uint32_t word,uint32_t flags) {
    if(!(flags&0x200u)) {return {};}
    if((word&0x80000008u)==8u) {return Crop{{822,253,100,62}};}
    if((word&0x80000400u)==0x400u) {return Crop{{822,190,100,62}};}
    return {};
}
inline std::optional<Crop> Nexus(uint32_t word,uint32_t flags) {
    if(!(flags&0x20u) || (word&0x80002000u)!=0x2000u) {return {};}
    const unsigned detail=word>>24;
    if(detail>8) {throw std::invalid_argument("Unsupported Nexus art detail");}
    constexpr Crop crops[]={{{579,695,100,62}},{{680,695,100,62}},{{0,0,0,0}},
        {{0,0,0,0}},{{0,0,0,0}},{{478,695,100,62}},{{478,632,100,62}},
        {{579,632,100,62}},{{680,632,100,62}}};
    return detail>=2 && detail<=4?std::optional<Crop>{}:crops[detail];
}

// Live retained word, already deformed C/W/N/E/S points and raw lighting.
// Mount, Mesa, Crater are independent branches, in original submission order.
inline std::vector<Pass> Quads(uint32_t word,bool land,
    const std::array<raster::Point,5>& points,const std::array<int,5>& lighting) {
    std::vector<Pass> result;
    if(!(word&0x103u) || (word>>24)>=128) {return result;}
    const unsigned detail=word>>24;
    if(detail>=49) {throw std::invalid_argument("Unsupported landmark art detail");}
    constexpr uint32_t bits[]={2,0x100,1};
    constexpr raster::Point mount_uv[]={{0,0},{55,0},{55,55},{0,55}};
    constexpr raster::Point other_uv[]={{0,55},{0,0},{55,0},{55,55}};
    for(unsigned branch=0;branch<3;++branch) {
        if(!(word&bits[branch])) {continue;}
        const int index=tables::QUADS[branch][2*detail];
        if(index<0) {continue;}
        const unsigned rotation=unsigned(tables::QUADS[branch][2*detail+1])&3u;
        const bool unlit=branch==0 && detail==0;
        Pass pass{};
        pass.crop=branch==0?std::array<int,4>{{1+57*index,628,56,56}}:
            branch==1?std::array<int,4>{{768+57*(index%4),15+57*(index/4),56,56}}:
            std::array<int,4>{{1+57*index,685,56,56}};
        pass.shader=unlit?raster::Shader::KeyedUnlit:raster::Shader::KeyedShaded;
        pass.shade_limits=land?std::array<int32_t,2>{{-4,4}}:std::array<int32_t,2>{{-256,256}};
        pass.polygon.count=4;
        for(unsigned v=0;v<4;++v) {
            const unsigned outer=(rotation+v)&3u;
            pass.polygon.vertices[v]={points[outer+1],branch==0?mount_uv[v]:other_uv[v],
                unlit?0:land?lighting[outer+1]:int(outer)};
        }
        result.push_back(pass);
    }
    return result;
}
}

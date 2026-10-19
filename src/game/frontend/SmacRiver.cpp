#include "SmacRiver.h"
namespace game::frontend::smac_river {
std::vector<smac_water_coast::Pass> Select(int x,int y,
    const std::array<smac_terrain_raster::Point,5>& points,
    const std::array<int,5>& lighting,const smac_water_coast::CellAt& live,uint32_t flags) {
    if(!(flags&0x400) || x<0 || x>=80 || y<0 || y>=80 || ((x^y)&1)) {return {};}
    const auto center=live(x,y);
    if(!center || center->band<3 || !(center->items&0x80)) {return {};}
    constexpr std::array<smac_terrain_raster::Point,4> offsets{{{{1,-1}},{{1,1}},{{-1,1}},{{-1,-1}}}};
    unsigned mask=0;
    for(unsigned i=0;i<4;++i) {
        const auto neighbor=live(x+offsets[i][0],y+offsets[i][1]);
        if(neighbor && (neighbor->items&0x80)) {mask|=1u<<i;}
    }
    const auto quad=[&](unsigned index) {
        smac_water_coast::Pass pass{{280+57*int(index&3),259+57*int(index>>2),56,56},{},smac_terrain_raster::Shader::KeyedShaded,{{-4,4}}};
        constexpr std::array<smac_terrain_raster::Point,4> uv{{{{0,0}},{{55,0}},{{55,55}},{{0,55}}}};
        pass.polygon.count=4;
        for(unsigned i=0;i<4;++i) {pass.polygon.vertices[i]={points[i+1],uv[i],lighting[i+1]};}
        return pass;
    };
    std::vector<smac_water_coast::Pass> result{quad(mask)};
    if(center->items&0x200) {result.push_back(quad(0));}
    return result;
}
}

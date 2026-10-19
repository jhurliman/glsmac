#include "SmacRoadTube.h"
namespace game::frontend::smac_road_tube {
std::vector<smac_water_coast::Pass> Select(int x,int y,
    const std::array<smac_terrain_raster::Point,5>& points,
    const std::array<int,5>& lighting,const CellAt& cells,uint32_t flags) {
    if(!(flags&0x20) || x<0 || x>=80 || y<0 || y>=80 || ((x^y)&1)) {return {};}
    const auto center=cells(x,y);
    if(!center || center->band<3 || (!(center->selected_items&4) && !center->admitted_base)) {return {};}
    const bool tube=(center->selected_items&8) || center->admitted_base;
    constexpr std::array<smac_terrain_raster::Point,8> offsets{{{{1,-1}},{{2,0}},{{1,1}},{{0,2}},{{-1,1}},{{-2,0}},{{-1,-1}},{{0,-2}}}};
    std::vector<smac_water_coast::Pass> result;
    const auto append=[&](bool tube_link,unsigned index) {
        smac_water_coast::Pass pass{{775+57*int(index%3),(tube_link?566:395)+57*int(index/3),56,56},{},smac_terrain_raster::Shader::KeyedShaded,{{-4,4}}};
        constexpr std::array<smac_terrain_raster::Point,4> uv{{{{0,0}},{{55,0}},{{55,55}},{{0,55}}}};
        pass.polygon.count=4;
        for(unsigned i=0;i<4;++i) {pass.polygon.vertices[i]={points[i+1],uv[i],lighting[i+1]};}
        result.push_back(pass);
    };
    bool connected=false,tube_connected=false;
    for(unsigned direction=0;direction<8;++direction) {
        const auto neighbor=cells(x+offsets[direction][0],y+offsets[direction][1]);
        if(!neighbor || neighbor->band<3 || (!(neighbor->selected_items&4) && !neighbor->admitted_base)) {continue;}
        const bool tube_link=tube && ((neighbor->selected_items&8) || neighbor->admitted_base);
        connected=true;tube_connected|=tube_link;append(tube_link,1+((direction+2)&7));
    }
    if(!center->admitted_base && ((tube && !tube_connected) || !connected)) {append(tube && !tube_connected,0);}
    return result;
}
}

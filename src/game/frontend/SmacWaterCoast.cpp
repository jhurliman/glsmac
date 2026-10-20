#include "SmacWaterCoast.h"
#include "SmacWaterCoastTables.h"
#include "SmacLandMaterialTables.h"
#include <algorithm>
#include <stdexcept>

namespace game::frontend::smac_water_coast {
namespace {
constexpr std::array<raster::Point,8> neighbors{{{{1,-1}},{{2,0}},{{1,1}},{{0,2}},{{-1,1}},{{-2,0}},{{-1,-1}},{{0,-2}}}};
bool Center(int x,int y) {return x>=0 && x<80 && y>=0 && y<80 && !((x^y)&1);}
int Contour(const std::optional<Cell>& cell) {return cell?std::min(79,int(cell->contour)):0;}
int Half(int64_t sum) {return static_cast<int>(sum<0?-((-sum+1)/2):sum/2);}
}
uint8_t Sample(int x,int y,unsigned sector,unsigned mode,const CellAt& at) {
    if(sector>3 || mode>3) {throw std::invalid_argument("Water sampler requires sector/mode0..3");}
    if(!Center(x,y)) {return 0;}
    const auto center=at(x,y);if(!center) {return 0;}
    const auto adjacent=[&](unsigned i){const auto& d=neighbors[i&7];return at(x+d[0],y+d[1]);};
    const auto corner=[&](unsigned i){return at(x+tables::sampler_corner_offsets_x[4*sector+i],y+tables::sampler_corner_offsets_y[4*sector+i]);};
    int value=0;
    if(!mode) {value=center->contour;}
    else if(mode&1) {value=(Contour(center)+Contour(adjacent(mode+2*sector+3)))/2;}
    else {for(unsigned i=0;i<4;++i) {value+=Contour(corner(i));}value/=4;}
    if(mode==2) {
        const auto first=corner(1),last=corner(3);
        if(first && first->band>=3) {value+=7;}
        if(last && last->band>=3) {value-=3;}
    } else {
        constexpr int sea_overrides[2][4]={{57,67,67,57},{67,67,57,57}};
        constexpr int land_overrides[2][4]={{67,57,57,67},{57,57,67,67}};
        for(unsigned i=0;i<3;++i) {
            const auto neighbor=adjacent(2*sector+4+i);
            const bool opposite=neighbor && ((neighbor->band<3)!=(center->band<3));
            if(opposite && ((mode==1 && i==0) || (mode==3 && i==2))) {
                return (center->band<3?sea_overrides:land_overrides)[mode==1?0:1][sector];
            }
        }
    }
    return static_cast<uint8_t>(std::clamp(value,0,79));
}
std::array<uint8_t,4> CoastPatterns(int x,int y,const CellAt& at) {
    std::array<uint8_t,4> result{};
    if(!Center(x,y)) {return result;}
    for(unsigned i=0;i<8;++i) {
        const auto& d=neighbors[i];const auto cell=at(x+d[0],y+d[1]);
        if(!cell || cell->band>=3) {continue;}
        if(i&1) {result[((i>>1)+2)&3]|=2;}
        else {result[((i>>1)+1)&3]|=4;result[((i>>1)+2)&3]|=1;}
    }
    return result;
}
Mapping CoastMapping(unsigned pattern,int x,int y) {
    if(pattern>7) {throw std::invalid_argument("Coast pattern requires0..7");}
    Mapping result{};
    for(size_t i=0;i<result.size();++i) {
        const auto value=tables::coast_classes[8*i+pattern];
        result[i]=static_cast<uint8_t>(value<=1?value:tables::coast_colors[2+value]);
    }
    if(pattern==7 && ((static_cast<uint32_t>(x)-static_cast<uint32_t>(y))&2)) {
        result[5]=0;std::fill(result.begin()+14,result.begin()+18,0);
        for(size_t start:{18u,22u,26u}) {for(size_t i=0;i<4;++i) {result[start+i]=tables::coast_colors[7-i];}}
    }
    return result;
}
std::vector<Pass> Select(int x,int y,const std::array<raster::Point,5>& points,const CellAt& at,std::optional<uint32_t> center_items) {
    if(!Center(x,y)) {return {};}
    auto own=at(x,y);if(!own) {return {};}
    if(center_items) {own->items=*center_items;}
    const bool sea=own->band<3;const auto patterns=CoastPatterns(x,y,at);
    std::vector<Pass> result;result.reserve(8);
    for(unsigned sector=0;sector<4;++sector) {
        if(!sea && !patterns[sector]) {continue;}
        const auto& outer=points[sector+1];
        const auto& previous=points[((sector+3)&3)+1];const auto& next=points[((sector+1)&3)+1];
        const std::array<raster::Point,4> vertices{{points[0],
            {{Half(int64_t(outer[0])+previous[0]),Half(int64_t(outer[1])+previous[1])}},outer,
            {{Half(int64_t(outer[0])+next[0]),Half(int64_t(outer[1])+next[1])}}}};
        std::array<int,4> depth{};
        Pass base{{280,79,56,56},{},sea?raster::Shader::WaterDepth16:raster::Shader::CoastMapped,sea?std::array<int32_t,2>{{-256,256}}:std::array<int32_t,2>{{-4,4}}};
        base.polygon.count=4;
        for(unsigned mode=0;mode<4;++mode) {
            depth[mode]=tables::depth_lookup[Sample(x,y,sector,mode,at)];
            const auto& uv=sea?tables::water_uv:tables::coast_uv;
            base.polygon.vertices[mode]={vertices[mode],{{uv[8*sector+2*mode],uv[8*sector+2*mode+1]}},depth[mode]};
        }
        if(sea && std::any_of(depth.begin(),depth.end(),[](int value){return value>=16;})) {
            auto overlay=base;overlay.crop={280,136,56,56};overlay.shader=raster::Shader::OpaqueShaded;
            for(auto& vertex:overlay.polygon.vertices) {vertex.light-=16;}
            result.push_back(overlay);
        }
        if(!sea) {base.mapping=CoastMapping(patterns[sector],x,y);}
        result.push_back(base);
    }
    if(sea && own->band==2 && (own->items&0x20)) {
        unsigned mask=0;
        for(unsigned bit=0;bit<8;++bit) {
            const auto& d=neighbors[(bit+7)&7];const auto neighbor=at(x+d[0],y+d[1]);
            if(neighbor && neighbor->band>=2 && (neighbor->items&0x20)) {mask|=1u<<bit;}
        }
        const auto& pattern=smac_land_material::tables::PATTERN[mask];
        Pass fungus{{508+57*(pattern[0]%4),516+57*(pattern[0]/4),56,56},{},raster::Shader::KeyedUnlit,{{-256,256}}};
        fungus.polygon.count=4;
        for(unsigned j=0;j<4;++j) {
            fungus.polygon.vertices[j]={points[j+1],smac_land_material::tables::CANONICAL_UV[(pattern[1]+j)&3][1],0};
        }
        result.push_back(fungus);
    }
    return result;
}
std::array<raster::Bitmap,2> CoastMasks(const raster::Bitmap& rainfall) {
    if(rainfall.width<115 || rainfall.height<389 || rainfall.width>=32767 || rainfall.height>=32767 ||
        uint64_t(rainfall.width)*static_cast<uint32_t>(rainfall.height)!=rainfall.pixels.size()) {
        throw std::invalid_argument("Unsupported indexed rainfall mask source");
    }
    std::array<uint8_t,256> classes{};
    for(size_t i=0;i<tables::coast_source_indices.size();++i) {classes[static_cast<uint8_t>(tables::coast_source_indices[i]+10)]=static_cast<uint8_t>(i);}
    std::array<raster::Bitmap,2> result;
    for(size_t which=0;which<2;++which) {
        auto& mask=result[which];mask={56,56,std::vector<uint8_t>(56*56)};
        for(size_t y=0;y<56;++y) {for(size_t x=0;x<56;++x) {
            const auto value=rainfall.pixels[(333+y)*static_cast<size_t>(rainfall.width)+2+57*which+x];
            mask.pixels[y*56+x]=(value==6 || value==7 || value==9)?0:classes[value];
        }}
    }
    return result;
}
}

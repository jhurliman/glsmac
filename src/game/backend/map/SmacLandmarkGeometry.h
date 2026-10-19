#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>
#include "SmacTerrainCorners.h"
#include "SmacLandmarkTables.h"

namespace game::backend::map::smac_terrain {

// Owns retained fields and the native shared-vertex cache. Reconstruct after
// terrain/profile/topology/relief changes; do not share across mutable worlds.
class LandmarkGeometry {
public:
    struct Cell { uint8_t band=0; uint32_t landmarks=0; };
    LandmarkGeometry(int width,int rows,bool wrap,std::vector<Cell> cells,bool low_relief=false)
        : width_(width),rows_(rows),wrap_(wrap),cells_(std::move(cells)),step_(low_relief?8:25) {
        if(width<=0 || width%2 || rows<=0 || cells_.size()!=size_t(width/2)*size_t(rows)) {
            throw std::invalid_argument("Invalid native landmark geometry dimensions");
        }
        cache_.resize(cells_.size());
        for(auto& entry:cache_) {entry.fill(std::nullopt);}
        profiles_=landmark_tables::HEIGHT;
        for(const auto& cell:cells_) {
            const unsigned detail=cell.landmarks>>24;
            if(detail<128 || !(cell.landmarks&3u)) {continue;}
            const unsigned row=255-detail;
            for(unsigned p=0;p<2;++p) {
                if(!(cell.landmarks&(p==0?2u:1u))) {continue;}
                if(row>=49) {prepared_[p]=false;continue;}
                for(unsigned v=0;v<5;++v) {profiles_[p][landmark_tables::MAPPING[row*5+v]]=0;}
            }
        }
    }

    std::optional<int> Height(int x,int y,unsigned vertex) {
        if(vertex>=5) {return std::nullopt;}
        const auto index=Index(x,y);
        if(!index || cells_[*index].band<3) {return 0;}
        auto& cached=cache_[*index][vertex];
        if(cached) {return cached;}
        const auto profile=Profile(cells_[*index].landmarks,vertex,false);
        if(!profile) {return std::nullopt;}
        int base=step_*(int(cells_[*index].band)-3);
        if(vertex) {
            int sum=0;bool land=true;
            for(const auto& offset:CORNERS[vertex-1]) {
                const auto neighbor=Index(x+offset[0],y+offset[1]);
                if(!neighbor || cells_[*neighbor].band<3) {land=false;break;}
            }
            if(land) {
                for(const auto& offset:CORNERS[vertex-1]) {
                    const auto height=Height(x+offset[0],y+offset[1],0);
                    if(!height) {return std::nullopt;}
                    sum+=*height;
                }
            }
            base=land?FloorQuarter(sum):0;
        }
        int value=base+step_*(*profile)/100;
        if(*profile<0) {value=std::max(-127,value);}
        value=std::min(127,value);
        if(vertex) {
            constexpr unsigned neighbor_vertex[]={3,4,1,2};
            for(unsigned n=0;n<4;++n) {
                const auto& offset=CORNERS[vertex-1][n];
                const auto neighbor=Index(x+offset[0],y+offset[1]);
                if(neighbor) {cache_[*neighbor][neighbor_vertex[n]]=value;}
            }
        } else {cached=value;}
        return value;
    }

    std::optional<int> ExtraShade(int x,int y,unsigned vertex) const {
        const auto index=Index(x,y);
        if(!index || vertex>=5) {return std::nullopt;}
        return Profile(cells_[*index].landmarks,vertex,true);
    }
    static int FloorQuarter(int value) {return value/4-(value%4<0?1:0);}

private:
    std::optional<size_t> Index(int x,int y) const {
        if(y<0 || y>=rows_) {return std::nullopt;}
        if(wrap_) {x%=width_;if(x<0) {x+=width_;}}
        if(x<0 || x>=width_ || ((x^y)&1)) {return std::nullopt;}
        return size_t(y)*size_t(width_/2)+size_t(x/2);
    }
    std::optional<int> Profile(uint32_t word,unsigned vertex,bool shade) const {
        const unsigned detail=word>>24;
        if(!(word&(shade?0x203u:0xb03u))) {return 0;}
        if(shade && detail>=128) {return 0;}
        if(detail>=49) {return std::nullopt;}
        const unsigned index=landmark_tables::MAPPING[detail*5+vertex];
        constexpr uint32_t priority[]={2,1,0x200,0x100,0x800};
        constexpr unsigned shade_priority[]={1,0,2};
        for(unsigned n=0;n<(shade?3u:5u);++n) {
            const unsigned p=shade?shade_priority[n]:n;
            if(!(word&priority[p])) {continue;}
            if(!shade && !prepared_[p]) {return std::nullopt;}
            const int value=shade?landmark_tables::SHADE[p][index]:profiles_[p][index];
            return !shade && p==0?value:value<128?value:value-256;
        }
        return 0;
    }
    int width_,rows_;bool wrap_;
    std::vector<Cell> cells_;
    int step_;
    std::vector<std::array<std::optional<int>,5>> cache_;
    std::array<std::array<uint8_t,113>,5> profiles_;
    std::array<bool,5> prepared_{{true,true,true,true,true}};
};
}

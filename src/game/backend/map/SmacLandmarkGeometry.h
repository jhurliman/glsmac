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
#include "SmacNegativeLandmarkTables.h"
#include "SmacTerrainLighting.h"

namespace game::backend::map::smac_terrain {

// Owns retained fields and the native shared-vertex cache. Reconstruct after
// terrain/profile/topology/relief changes; do not share across mutable worlds.
class LandmarkGeometry {
public:
    struct Cell { uint8_t band=0; uint32_t landmarks=0; };
    LandmarkGeometry(int width,int rows,bool wrap,std::vector<Cell> cells,bool low_relief=false,bool pinned_planet=false)
        : width_(width),rows_(rows),wrap_(wrap),cells_(std::move(cells)),step_(low_relief?8:25),pinned_planet_(pinned_planet) {
        if(width<=0 || width%2 || rows<=0 || cells_.size()!=size_t(width/2)*size_t(rows)) {
            throw std::invalid_argument("Invalid native landmark geometry dimensions");
        }
        cache_.resize(cells_.size());
        for(auto& entry:cache_) {entry.fill(std::nullopt);}
        face_cache_.resize(cells_.size());
        relief_=step_;
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
        if(cached && *cached!= -1) {return cached;}
        const auto profile=Profile(cells_[*index].landmarks,vertex,false);
        if(!profile) {return std::nullopt;}
        int base=step_*(int(cells_[*index].band)-3);
        if(vertex) {
            int sum=0;bool land=true;
            for(const auto& offset:CORNERS[vertex-1]) {
                const auto neighbor=Index(x+offset[0],y+offset[1]);
                if(!neighbor || cells_[*neighbor].band<3) {land=false;break;}
                // Native early sea exits preserve earlier center-cache writes.
                const auto height=Height(x+offset[0],y+offset[1],0);
                if(!height) {return std::nullopt;}
                sum+=*height;
            }
            base=land?FloorQuarter(sum):0;
        }
        int value=base+step_*(*profile)/100;
        if(*profile<0) {value=std::max(-127,value);}
        else if(*profile>0) {value=std::min(127,value);}
        // Profile-sign clamps precede publication. Both cold centers and cold
        // corners return value; their published shared bytes can be smaller.
        const int stored=std::min(127,value);
        if(!vertex) {cached=stored;return value;}
        if(vertex) {
            constexpr unsigned neighbor_vertex[]={3,4,1,2};
            for(unsigned n=0;n<4;++n) {
                const auto& offset=CORNERS[vertex-1][n];
                const auto neighbor=Index(x+offset[0],y+offset[1]);
                if(neighbor) {cache_[*neighbor][neighbor_vertex[n]]=stored;}
            }
        } else {cached=stored;}
        return value;
    }

    // Raw signed height byte after a query. Sea can return0 while leaving FF
    // in this cache; the original unknown outline reads that byte as minus1.
    std::optional<int> CachedHeight(int x,int y,unsigned vertex) const {
        const auto index=Index(x,y);if(!index || vertex>=5) {return {};}
        return cache_[*index][vertex].value_or(-1);
    }
    // Read-only semantic cache receipt: four biased shade bytes, five heights.
    std::vector<uint8_t> CacheBytes() const {
        std::vector<uint8_t> bytes;bytes.reserve(cache_.size()*9);
        for(size_t i=0;i<cache_.size();++i) {
            for(const auto value:face_cache_[i]) {bytes.push_back(static_cast<uint8_t>(value.value_or(127)+128));}
            for(const auto value:cache_[i]) {bytes.push_back(static_cast<uint8_t>(value.value_or(-1)));}
        }return bytes;
    }

    std::optional<int> ExtraShade(int x,int y,unsigned vertex) const {
        const auto index=Index(x,y);
        if(!index || vertex>=5) {return std::nullopt;}
        return Profile(cells_[*index].landmarks,vertex,true);
    }
    std::optional<int> ProjectorHeight(int x,int y) {
        int sum=0;
        for(unsigned v=4;v>0;--v) {const auto height=Height(x,y,v);if(!height) {return {};}sum+=*height;}
        return FloorQuarter(sum);
    }
    // Camera movement preserves both caches. Zoom changes invalidate both;
    // relief changes invalidate heights while retaining ordinary face shades.
    void Configure(int zoom,int relief) {
        if(zoom< -14 || zoom>16 || (relief!=8 && relief!=25)) {
            throw std::invalid_argument("Unsupported native geometry view configuration");
        }
        if(zoom!=zoom_) {
            for(auto& entry:face_cache_) {entry.fill(std::nullopt);}
        }
        if(zoom!=zoom_ || relief!=relief_) {
            for(auto& entry:cache_) {entry.fill(std::nullopt);}
        }
        zoom_=zoom;relief_=relief;step_=relief*(zoom+16)/16;
    }
    std::optional<int> Shade(int x,int y,unsigned vertex) {
        if(vertex>=5 || !Index(x,y)) {return {};}
        int sum=0;
        if(vertex==0) {
            for(unsigned f=0;f<4;++f) {const auto value=FaceShade(x,y,f);if(!value) {return {};}sum+=*value;}
        } else {
            for(unsigned i=0;i<4;++i) {
                const auto& offset=CORNERS[vertex-1][i];
                const auto first=FaceShade(x+offset[0],y+offset[1],(i+1)&3);
                const auto second=FaceShade(x+offset[0],y+offset[1],(i+2)&3);
                if(!first || !second) {return {};}sum+=*first+*second;
            }
        }
        const auto extra=ExtraShade(x,y,vertex);if(!extra) {return {};}
        const int count=vertex?8:4;
        return sum/count-(sum%count<0?1:0)+*extra;
    }
    static int FloorQuarter(int value) {return value/4-(value%4<0?1:0);}

private:
    std::optional<int> Slope(int x,int y,unsigned corner) const {
        const auto own=Index(x,y);if(!own || cells_[*own].band<3) {return 0;}
        if(cells_[*own].band>7) {return {};}
        int sum=0;
        for(const auto& offset:CORNERS[corner]) {
            const auto cell=Index(x+offset[0],y+offset[1]);
            if(!cell || cells_[*cell].band<3) {return 0;}
            if(cells_[*cell].band>7) {return {};}sum+=cells_[*cell].band;
        }
        return int(unsigned(sum-4*int(cells_[*own].band)+4)&15u)-4;
    }
    std::optional<int> FaceShade(int x,int y,unsigned face) {
        // Unlike height queries, native face shading rejects off-map X rather
        // than wrapping it to the opposite edge.
        if(x<0 || x>=width_) {return 0;}
        const auto own=Index(x,y);if(!own) {return 0;}
        auto& cached=face_cache_[*own][face];
        if(cached && *cached!=127) {return cached;}
        if(cells_[*own].band<3) {cached=0;return cached;}
        for(unsigned v=1;v<5;++v) {if(!Height(x,y,v)) {return {};}}
        const auto first=Slope(x,y,face),second=Slope(x,y,(face+1)&3);
        if(!first || !second) {return {};}
        cached=FaceLighting(*first,*second,face);return cached;
    }
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
        unsigned index=0;
        if(detail>=128 && !shade && pinned_planet_) {
            const int signed_detail=int(detail)-256;
            const auto found=std::find(landmark_tables::NEGATIVE_DETAILS.begin(),landmark_tables::NEGATIVE_DETAILS.end(),signed_detail);
            if(found==landmark_tables::NEGATIVE_DETAILS.end()) {return {};}
            index=landmark_tables::NEGATIVE_MAPPING[size_t(found-landmark_tables::NEGATIVE_DETAILS.begin())][vertex];
        } else {
            if(detail>=49) {return std::nullopt;}
            index=landmark_tables::MAPPING[detail*5+vertex];
        }
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
    int zoom_=0,relief_=25;
    bool pinned_planet_;
    std::vector<std::array<std::optional<int>,5>> cache_;
    std::vector<std::array<std::optional<int>,4>> face_cache_;
    std::array<std::array<uint8_t,113>,5> profiles_;
    std::array<bool,5> prepared_{{true,true,true,true,true}};
};
}

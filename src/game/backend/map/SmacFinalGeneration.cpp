#include "SmacFinalGeneration.h"
#include "SmacShapingOffsets.h"
#include <algorithm>
#include <cstdlib>
#include <optional>
#include <stdexcept>

namespace game::backend::map::smac_generation {
namespace {
constexpr std::array<int32_t,8> old_bounds{{0,15,32,45,60,75,80,100}};
constexpr std::array<int32_t,11> new_bounds{{0,20,40,60,80,100,120,140,160,180,200}};
uint32_t Read(const Record& r,size_t f) {return uint32_t(r[f])|(uint32_t(r[f+1])<<8)|(uint32_t(r[f+2])<<16)|(uint32_t(r[f+3])<<24);}
class Plane {
public:
    Plane(State& state,Geometry geometry,InitialClimateTrace* trace):s(state),g(geometry),t(trace) {
        if(g.width<8 || g.height<8 || g.width>65534 || g.height>65534 || (g.width&1)
            || uint64_t(g.width/2)*uint64_t(g.height)!=s.records.size() || s.records.size()>UINT32_MAX/44u) {
            throw std::invalid_argument("Final generation requires an admitted owned plane");
        }
    }
    size_t Index(Coordinate p) const {return size_t(p[1])*size_t(g.width/2)+size_t(p[0]/2);}
    int Band(size_t i) const {return s.records[i][0]>>5;}
    std::optional<Coordinate> Neighbor(Coordinate p,size_t k) const {
        p[0]+=smac_shaping::OFFSETS[k][0];p[1]+=smac_shaping::OFFSETS[k][1];
        if(s.topology==0) {if(p[0]<0) {p[0]+=g.width;}else if(p[0]>=g.width) {p[0]-=g.width;}}
        if(p[0]<0 || p[0]>=g.width || p[1]<0 || p[1]>=g.height) {return std::nullopt;}return p;
    }
    void Write(size_t index,size_t field,unsigned size,uint32_t value) {
        for(unsigned k=0;k<size;++k) {s.records[index][field+k]=uint8_t(value>>(8*k));}
        if(t) {++t->stores;if(t->on_store) {t->on_store(uint32_t(index*44+field),size,value);}}
    }
    void Pump() {if(t) {++t->pumps;if(t->checkpoint) {t->checkpoint();}}}
    uint16_t Draw() {
        const auto before=s.random.Crt();const auto value=s.random.NextCrtRaw15();
        if(t) {++t->crt_draws;if(t->on_crt) {t->on_crt(before,s.random.Crt(),value);}}return value;
    }
    State& s;Geometry g;InitialClimateTrace* t;
};
int Rockiness(int x,int y,int band,int height,int pattern) {
    if(y==0 || y==height-1) {return 2;}
    const auto u=(x+y)/2,v=x-u;const int64_t score=3*int64_t(v/2)+pattern+2*int64_t(u/2);
    const auto phase=unsigned(uint64_t(score-2*(uint32_t(v)&1)-(uint32_t(u)&1))&3);
    const auto distance=std::abs(band-3)-(band<3?1:0);
    const std::array<int,4> zero{{1,0,1+int(bool(uint64_t(score)&4)),0}},one{{0,1,1,2}},two{{0,1,2,2}},three{{1,1,2,2}};
    if(distance==0) {return zero[phase];}if(distance==1) {return one[phase];}
    if(distance==2) {return two[phase];}if(distance==3) {return three[phase];}return int((~phase)&1);
}
}
void NormalizeContours(State& s,Geometry g,InitialClimateTrace* t) {
    Plane plane(s,g,t);
    for(int y=0;y<g.height;++y) {
        for(int x=y&1;x<g.width;x+=2) {
            const auto i=plane.Index({{x,y}});const int64_t contour=s.records[i][1];int band=0;
            for(int k=6;k>=0;--k) {if(contour-s.reset_words[0]>=s.working_land_table[k]) {band=k;break;}}
            const auto high=new_bounds[band<6?band+1:band+4];
            const auto value=new_bounds[band]+int64_t(high-new_bounds[band])*(contour-old_bounds[band])/(old_bounds[band+1]-old_bounds[band]);
            plane.Write(i,1,1,uint8_t(value));
        }
        plane.Pump();
    }
    s.working_land_table=new_bounds;
}
void ClimatePostpass(State& s,Geometry g,Regions& regions,InitialClimateTrace* t) {
    Plane plane(s,g,t);
    const auto wet_count=[&](size_t region,uint32_t value){for(unsigned k=0;k<4;++k) {regions.records[region*28+4+k]=uint8_t(value>>(8*k));}};
    for(size_t r=0;r<128;++r) {wet_count(r,0);}
    for(int y=0;y<g.height;++y) {
        for(int x=y&1;x<g.width;x+=2) {
            const Coordinate p{{x,y}};const auto i=plane.Index(p);const auto region=s.records[i][3];
            if(region>=128) {throw std::invalid_argument("Invalid native region label");}
            if(plane.Band(i)>=3 && (s.records[i][0]&24) && (s.records[i][5]&192)<128 && !(Read(s.records[i],8)&32)) {
                const auto off=size_t(region)*28+4;
                const uint32_t value=uint32_t(regions.records[off])|(uint32_t(regions.records[off+1])<<8)|(uint32_t(regions.records[off+2])<<16)|(uint32_t(regions.records[off+3])<<24);
                wet_count(region,value+1u);
            }
            plane.Write(i,8,4,Read(s.records[i],8)&0xbfffbfffu);
            if(plane.Band(i)<3) {continue;}
            int previous=-1,inner=0,outer=0;
            for(size_t k=1;k<21;++k) {
                const auto n=plane.Neighbor(p,k);if(!n) {continue;}
                const auto j=plane.Index(*n);const int sea=plane.Band(j)<3;
                if(k<9) {
                    if(sea!=previous) {previous=sea;++inner;if(inner>=4 && regions.Area(region)>=80) {plane.Write(i,8,4,Read(s.records[i],8)|0x40000000u);}}
                } else {
                    if(sea!=previous || k==9) {previous=sea;++outer;}
                    const auto other=s.records[j][3];
                    if((*n)[1]>4 && (*n)[1]<g.height-4 && other<64 && other!=region && regions.Area(other)>=40 && regions.Area(region)>=40) {
                        plane.Write(i,8,4,Read(s.records[i],8)|0x4000u);
                    }
                }
            }
            if(outer<4) {plane.Write(i,8,4,Read(s.records[i],8)&0xbfffffffu);}
        }
        plane.Pump();
    }
    for(size_t i=0;i<s.records.size();++i) {plane.Write(i,2,1,s.records[i][2]&15);}
}
void FinalFeatures(State& s,Geometry g,InitialClimateTrace* t) {
    Plane plane(s,g,t);
    for(int y=0;y<g.height;++y) {
        for(int x=y&1;x<g.width;x+=2) {
            const auto i=plane.Index({{x,y}});const auto marks=Read(s.records[i],12);
            if((marks&0x80000001u)==1u && (marks&0xff000000u)<0x15000000u) {continue;}
            const auto band=plane.Band(i),rock=Rockiness(x,y,band,g.height,s.resource_pattern);
            plane.Write(i,5,1,uint8_t((s.records[i][5]&63)|(rock<<6)));plane.Write(i,12,4,marks|0x400000u);s.dirty_flags|=1;
            if((marks&128u) && band<3) {continue;}
            if(band<3 || ((marks&0x80000004u)==4u && plane.Draw()%2==0)) {plane.Write(i,8,4,Read(s.records[i],8)|0x10000u);}
            else if(band>4 || (s.records[i][0]&24)>8) {plane.Write(i,8,4,Read(s.records[i],8)|0x20000u);}
        }
        plane.Pump();
    }
}
}

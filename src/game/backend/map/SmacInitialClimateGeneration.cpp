#include "SmacInitialClimateGeneration.h"
#include "SmacClimate.h"
#include "SmacGenerationNoise.h"
#include "SmacShapingOffsets.h"
#include <algorithm>
#include <optional>
#include <stdexcept>

namespace game::backend::map::smac_generation {
namespace {
uint32_t Read(const Record& r,size_t f) {return uint32_t(r[f])|(uint32_t(r[f+1])<<8)|(uint32_t(r[f+2])<<16)|(uint32_t(r[f+3])<<24);}
class Plane {
public:
    Plane(State& state,Geometry geometry,InitialClimateTrace* trace):s(state),g(geometry),t(trace) {
        if(g.width<8 || g.height<8 || g.width>65534 || g.height>65534 || (g.width&1)
            || uint64_t(g.width/2)*uint64_t(g.height)!=s.records.size() || s.records.size()>UINT32_MAX/44u) {
            throw std::invalid_argument("Initial climate requires an admitted owned plane");
        }
    }
    size_t Index(Coordinate p) const {return size_t(p[1])*size_t(g.width/2)+size_t(p[0]/2);}
    int Band(size_t i) const {return s.records[i][0]>>5;}
    std::optional<size_t> Neighbor(Coordinate p,size_t k) const {
        p[0]+=smac_shaping::OFFSETS[k][0];p[1]+=smac_shaping::OFFSETS[k][1];
        if(s.topology==0) {if(p[0]<0) {p[0]+=g.width;}else if(p[0]>=g.width) {p[0]-=g.width;}}
        if(p[0]<0 || p[0]>=g.width || p[1]<0 || p[1]>=g.height) {return std::nullopt;}return Index(p);
    }
    void Write(size_t index,size_t field,unsigned size,uint32_t value) {
        for(unsigned k=0;k<size;++k) {s.records[index][field+k]=uint8_t(value>>(8*k));}
        if(t) {++t->stores;if(t->on_store) {t->on_store(uint32_t(index*44+field),size,value);}}
    }
    uint16_t Draw() {
        const auto before=s.random.Crt();const auto value=s.random.NextCrtRaw15();
        if(t) {++t->crt_draws;if(t->on_crt) {t->on_crt(before,s.random.Crt(),value);}}return value;
    }
    void Pump() {if(t) {++t->pumps;if(t->checkpoint) {t->checkpoint();}}}
    State& s;Geometry g;InitialClimateTrace* t;
};
}
void InitialTemperature(State& s,Geometry g,const WorldRules& rules,InitialClimateTrace* t) {
    Plane plane(s,g,t);
    // Reuse the established climate rule over an owned view; publish only its
    // temperature bits back into the raw records. Other fields remain raw-owned.
    smac_climate::World world{g.width,g.height,s.topology==0,{}};world.tiles.reserve(s.records.size());
    for(const auto& r:s.records) {world.tiles.push_back({r[0]>>5,0,0,r[1],r[6],Read(r,8),Read(r,12)});}
    smac_climate::Streams streams{s.random};
    if(t) {
        streams.on_seed=[&](uint32_t before,uint32_t after){++t->shared_events;if(t->on_shared) {t->on_shared(true,before,after,0,0,0);}};
        streams.on_shared=[&](uint32_t before,uint32_t after,int32_t low,int32_t high,int32_t value){
            ++t->shared_events;if(t->on_shared) {t->on_shared(false,before,after,low,high,value);}
        };
    }
    const smac_climate::Rules climate{uint32_t(s.resource_pattern),s.reset_words[1],s.categories[4],s.categories[5],rules[10],rules[11],rules[12],rules[13],0,0,0};
    smac_climate::Temperature(world,climate,streams,[&](int32_t y) {
        for(int32_t x=y&1;x<g.width;x+=2) {
            const auto i=plane.Index({{x,y}});const auto marks=Read(s.records[i],12);
            if((marks&0x80000001u)==1u && (marks&0xff000000u)<0x15000000u) {continue;}
            plane.Write(i,0,1,uint8_t((s.records[i][0]&~7)|world.tiles[i].temperature));
        }
        plane.Pump();
    });
}
RiverSources SeedRiverSources(State& s,Geometry g,const WorldRules& rules,InitialClimateTrace* t) {
    Plane plane(s,g,t);
    for(size_t i=0;i<s.records.size();++i) {plane.Write(i,8,4,Read(s.records[i],8)&~0x100u);}
    const auto quota=((int64_t(rules[8])+int64_t(rules[9])*s.categories[5])*(4-int64_t(s.categories[1]))/3)*int64_t(s.records.size())/3200;
    if(quota<INT32_MIN || quota>INT32_MAX) {throw std::overflow_error("River-source quota exceeds measured signed32 domain");}
    RiverSources result{int32_t(quota),0,0};
    while(result.placed<result.quota && result.attempts<4000) {
        Coordinate p{{plane.Draw()%g.width,plane.Draw()%g.height}};p[0]=(p[0]&~1)+(p[1]&1);
        const auto a=plane.Index(p);int highest=0;std::optional<size_t> selected;
        bool admitted=plane.Band(a)>=3 && !(Read(s.records[a],8)&0x100u);
        if(admitted) {
            for(size_t k=0;k<81;++k) {
                const auto n=plane.Neighbor(p,k);if(!n) {continue;}
                if(Read(s.records[*n],8)&0x100u) {admitted=false;break;}
                if(k<49 && plane.Band(*n)>highest) {highest=plane.Band(*n);selected=*n;}
            }
            if(admitted && selected) {plane.Write(*selected,8,4,Read(s.records[*selected],8)|0x100u);++result.placed;}
        }
        ++result.attempts;
    }
    return result;
}
void InitialFungus(State& s,Geometry g,const WorldRules& rules,uint32_t clear_mask,InitialClimateTrace* t) {
    Plane plane(s,g,t);
    Noise noise(s.random,[&](const Draw& draw){if(t) {++t->crt_draws;if(t->on_crt) {t->on_crt(draw.before,draw.after,uint16_t(draw.value));}}});
    const int64_t delta=(int64_t(s.categories[6])-1)*rules[24];
    for(int32_t y=0;y<g.height;++y) {
        for(int32_t x=y&1;x<g.width;x+=2) {
            const auto value=std::clamp(((5*noise.Evaluate(16*x,16*y)+2*noise.Evaluate(256*x,256*y))*7)/256,0,100);
            const auto a=plane.Index({{x,y}});const auto marks=Read(s.records[a],12);
            if((marks&0x80000002u)==2u && (marks&0xff000000u)<0x09000000u) {continue;}
            bool admitted=false;
            for(const int base:{20,40,60}) {if(base-delta<=value && value<=base+5+2*delta) {admitted=true;break;}}
            if(!admitted && ((y!=0 && y!=g.height-1) || plane.Draw()%2==0)) {continue;}
            if((s.records[a][5]&0xc0)>=0x80 && plane.Band(a)>=3) {continue;}
            const auto items=Read(s.records[a],8)|0x20u;plane.Write(a,8,4,items);plane.Write(a,8,4,items&~clear_mask);
        }
        plane.Pump();
    }
}
RiverSources GenerateInitialClimate(State& s,Geometry g,const WorldRules& rules,uint32_t clear_mask,InitialClimateTrace* t) {
    if(s.stage!=Stage::Pretemperature || s.generation_flags!=std::array<int32_t,2>{{1,1}}) {
        throw std::invalid_argument("Initial climate requires pretemperature continuation");
    }
    InitialTemperature(s,g,rules,t);const auto sources=SeedRiverSources(s,g,rules,t);
    InitialFungus(s,g,rules,clear_mask,t);s.stage=Stage::Prelandmarks;return sources;
}
}

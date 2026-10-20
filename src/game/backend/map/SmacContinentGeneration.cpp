#include "SmacContinentGeneration.h"
#include "SmacShapingOffsets.h"
#include "game/backend/resource/SmacResourceBonus.h"
#include <algorithm>
#include <optional>
#include <stdexcept>

namespace game::backend::map::smac_generation {
namespace {
constexpr std::array<Coordinate,8> directions{{{{1,-1}},{{2,0}},{{1,1}},{{0,2}},{{-1,1}},{{-2,0}},{{-1,-1}},{{0,-2}}}};
int32_t Checked(int64_t value) {
    if(value<INT32_MIN || value>INT32_MAX) {throw std::overflow_error("Continent arithmetic exceeds measured signed32 domain");}
    return int32_t(value);
}
class Worker {
public:
    Worker(State& state,Geometry geometry,const ContinentRules& rules,ContinentTrace* trace)
        :s(state),g(geometry),r(rules),t(trace),land(state.categories[2]),wrap(state.topology==0) {
        if(g.width<8 || g.height<8 || g.width>65534 || g.height>65534 || (g.width&1)
            || uint64_t(g.width/2)*uint64_t(g.height)!=s.records.size() || s.records.size()>UINT32_MAX/44u
            || land<0 || land>2 || s.stage!=Stage::ContinentLoop || s.generation_flags!=std::array<int32_t,2>{{1,1}}) {
            throw std::invalid_argument("Continent stage requires a complete admitted turn-zero plane");
        }
        for(size_t i=1;i<9;++i) {if(s.working_land_table[i]<s.working_land_table[i-1]) {
            throw std::invalid_argument("Continent shaping requires ordered working thresholds");
        }}
    }
    void Seed(int32_t ordinal) {
        for(size_t i=0;i<s.records.size();++i) {Write(i,3,0);}
        const auto effective=land && s.land_progress>=s.land_target?land-1:land;
        auto size=Checked(int64_t(r.base)+int64_t(r.modifier)*effective*effective);
        Coordinate seed{};
        for(int candidate=0;candidate<100;++candidate) {
            const auto xs=g.width-(wrap?0:8),ys=g.height-8;
            seed[0]=(xs>1?Draw(CrtUse::CandidateX)%xs:0)+(wrap?0:4);
            seed[1]=(ys>1?Draw(CrtUse::CandidateY)%ys:0)+4;
            seed[0]=(seed[0]&~1)+(seed[1]&1);
            if(t && t->on_candidate) {t->on_candidate(seed);}
            if(candidate==99 || Band(seed)<3) {break;}
        }
        if(Band(seed)>=3) {return;}
        const auto q=Checked(int64_t(ordinal)*3200)/int32_t(s.records.size());
        if(q>r.ratios[4] || s.land_progress>=s.land_target) {
            size/=4;
            if(s.land_progress>=s.land_target && q>r.ratios[4]) {size=Draw(CrtUse::SmallSize)%3+1;}
        } else if(q>r.ratios[3]) {size/=3;}
        else if(q>r.ratios[2]) {size/=2;}
        else if(q>r.ratios[1]) {
            const auto divisor=4-land;
            if(divisor>1 && Draw(CrtUse::ConditionalSize)%divisor) {size/=2;}
        } else if(q<r.ratios[0] && land>1) {size=Checked(int64_t(size)+size/2);}
        if(s.land_progress<s.land_target) {size=std::max(size,4);}
        Grow(seed,3,size);
        if(t) {++t->pumps;if(t->checkpoint) {t->checkpoint();}}
    }
    void Grow(Coordinate seed,int32_t category,int32_t target) {
        RequirePoint(seed,category);
        if(t && t->on_walk) {t->on_walk(seed,category,target);}
        s.painted=0;int32_t steps=0,streak=0,previous=-1;auto point=seed;
        while(true) {
            Paint(point,category);
            const int32_t raw=(Draw(CrtUse::Direction)%8)|1;auto direction=raw;
            if(raw==previous) {
                ++streak;
                if(streak>land*land+2) {direction=(raw-land*land+streak-2)&7;}
            } else {previous=raw;streak=0;}
            if(t) {
                t->direction_corrections+=direction!=raw;
                if(t->on_direction) {t->on_direction(direction,previous,streak);}
            }
            point[0]+=directions[direction][0];point[1]+=directions[direction][1];
            if(wrap) {if(point[0]<0) {point[0]+=g.width;}else if(point[0]>=g.width) {point[0]-=g.width;}}
            ++steps;
            if(steps<2000 && Inside(point) && s.painted<target) {continue;}
            ++steps;
            if(steps>=2000 || s.painted>=target) {break;}
            point=seed;streak=0;previous=-1;
        }
    }
    void Paint(Coordinate point,int32_t category) {
        RequirePoint(point,category);
        if(t) {++t->paints;if(t->on_paint) {t->on_paint(point,category);}}
        for(const auto j:{0,1,3}) {
            const auto q=Normalize({{point[0]+smac_shaping::OFFSETS[j][0],point[1]+smac_shaping::OFFSETS[j][1]}});
            if(!q || Band(*q)==category) {continue;}
            bool reject=false;
            for(const auto& d:directions) {
                const auto n=Normalize({{(*q)[0]+d[0],(*q)[1]+d[1]}});
                if(n && Band(*n)>=3 && s.records[Index(*n)][3]==0) {reject=true;break;}
            }
            if(reject) {continue;}
            if(category==3) {for(int k=0;k<8;k+=2) {
                const auto& d=directions[k+1];const auto n=Normalize({{(*q)[0]+d[0],(*q)[1]+d[1]}});
                if(!n || Band(*n)!=3) {continue;}
                const auto& a=directions[k];const auto& b=directions[(k+2)&7];
                const auto left=Normalize({{(*q)[0]+a[0],(*q)[1]+a[1]}}),right=Normalize({{(*q)[0]+b[0],(*q)[1]+b[1]}});
                if(left && right && Band(*left)<3 && Band(*right)<3) {reject=true;break;}
            }}
            if(reject) {continue;}
            Write(Index(*q),3,1);Shape(*q,category);
            s.painted=Checked(int64_t(s.painted)+1);s.land_progress=Checked(int64_t(s.land_progress)+1);
        }
    }
private:
    bool Inside(Coordinate p) const {return p[0]>=0 && p[0]<g.width && p[1]>=0 && p[1]<g.height;}
    std::optional<Coordinate> Normalize(Coordinate p) const {
        if(wrap) {if(p[0]<0) {p[0]+=g.width;}else if(p[0]>=g.width) {p[0]-=g.width;}}
        return Inside(p)?std::optional<Coordinate>{p}:std::nullopt;
    }
    void RequirePoint(Coordinate p,int32_t category) const {
        if(!Inside(p) || ((p[0]^p[1])&1) || category<0 || category>7) {
            throw std::invalid_argument("Continent center/category is outside measured physical domain");
        }
    }
    size_t Index(Coordinate p) const {return size_t(p[1])*size_t(g.width/2)+size_t(p[0])/2;}
    int32_t Band(Coordinate p) const {return s.records[Index(p)][0]>>5;}
    uint16_t Draw(CrtUse use) {
        const auto before=s.random.Crt();const auto value=s.random.NextCrtRaw15();
        if(t) {++t->draws;if(t->on_draw) {t->on_draw(use,before,s.random.Crt(),value);}}
        return value;
    }
    void Write(size_t index,size_t field,uint8_t value) {
        s.records[index][field]=value;
        if(t) {++t->writes;if(t->on_write) {t->on_write(uint32_t(index*44+field),value);}}
    }
    uint32_t Items(Coordinate p) const {
        const auto& record=s.records[Index(p)];
        return uint32_t(record[8])|(uint32_t(record[9])<<8)|(uint32_t(record[10])<<16)|(uint32_t(record[11])<<24);
    }
    bool Resource(Coordinate p) const {
        namespace rules=resource::smac;
        const auto items=Items(p);
        return rules::ResolveResourceBonus({p[0],p[1],static_cast<rules::AltitudeBand>(Band(p)),s.resource_pattern,
            bool(r.primary&0x2000u),bool(items&0x400u),bool(items&0x20000000u),bool(items&0x10000u),bool(items&0x20000u)})!=rules::ResourceBonus::None;
    }
    void Ensure(Coordinate point,int32_t category) {
        const auto index=Index(point);
        Write(index,0,uint8_t((s.records[index][0]&31)|(category<<5)));s.dirty_flags|=4;
        const auto contour=int64_t(s.records[index][1])-s.reset_words[0];int32_t current=0;
        for(int32_t c=6;c>=0;--c) {if(contour>=s.working_land_table[c]) {current=c;break;}}
        if(current!=category) {
            const auto span=Checked(int64_t(s.working_land_table[category+1])-s.working_land_table[category]);
            const auto residue=span>1?Draw(CrtUse::Contour)%span:0;
            Write(index,1,uint8_t(int64_t(s.working_land_table[category])+s.reset_words[0]+residue));
        }
    }
    void Shape(Coordinate center,int32_t category) {
        s.flood_counts.fill(0);s.affected_bases.fill(-1);Ensure(center,category);
        for(const int32_t direction:{-1,1}) {
            const auto limit=direction<0?category:8-category;
            for(int32_t radius=1;radius<limit;++radius) {
                const auto target=category+direction*radius;bool changed=false;
                for(size_t i=smac_shaping::RING_ENDS[radius-1];i<smac_shaping::RING_ENDS[radius];++i) {
                    const auto q=Normalize({{center[0]+smac_shaping::OFFSETS[i][0],center[1]+smac_shaping::OFFSETS[i][1]}});
                    if(!q || !(direction<0?Band(*q)<target:Band(*q)>target)) {continue;}
                    Ensure(*q,target);
                    const auto index=Index(*q);
                    if(!(Items(*q)&3u) || (s.records[index][2]&15)>=8) {
                        if(!Resource(*q)) {Write(index,2,uint8_t((s.records[index][2]&240)|15));}
                    }
                    changed=true;
                }
                if(!changed) {break;}
            }
        }
    }
    State& s;Geometry g;const ContinentRules& r;ContinentTrace* t;int32_t land;bool wrap;
};
}
void SeedContinent(State& s,Geometry g,const ContinentRules& r,int32_t ordinal,ContinentTrace* t) {Worker(s,g,r,t).Seed(ordinal);}
void GrowContinent(State& s,Geometry g,const ContinentRules& r,Coordinate p,int32_t category,int32_t target,ContinentTrace* t) {Worker(s,g,r,t).Grow(p,category,target);}
void PaintContinent(State& s,Geometry g,const ContinentRules& r,Coordinate p,int32_t category,ContinentTrace* t) {Worker(s,g,r,t).Paint(p,category);}
}

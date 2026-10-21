#include "SmacGenerationRegions.h"
#include "SmacShapingOffsets.h"
#include <algorithm>
#include <optional>
#include <stdexcept>

namespace game::backend::map::smac_generation {
namespace {
uint32_t Read(const uint8_t* p) {return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
class Builder {
public:
    Builder(State& state,Geometry geometry,Regions& regions,RegionTrace* trace):s(state),g(geometry),r(regions),t(trace) {
        if(g.width<8 || g.height<8 || g.width>65534 || g.height>65534 || (g.width&1)
            || uint64_t(g.width/2)*uint64_t(g.height)!=s.records.size() || s.records.size()>UINT32_MAX/44u
            || r.summary_width<0 || r.summary_height<0 || r.summary_width>65534 || r.summary_height>65534) {
            throw std::invalid_argument("Region rebuild requires an admitted owned plane and summary geometry");
        }
    }
    void Run() {
        for(size_t i=0;i<s.records.size();++i) {Label(i,0);}
        int32_t land=0,sea=64;
        std::vector<Coordinate> queue;queue.reserve(s.records.size());
        for(int32_t y=1;y<g.height-1;++y) {for(int32_t x=y&1;x<g.width;x+=2) {
            const Coordinate point{{x,y}};const auto index=Index(point);
            if(s.records[index][3]) {continue;}
            const bool ocean=Band(index)<3;
            const auto id=ocean?std::min(++sea,126):std::min(++land,62);
            const auto low=ocean?64:0,limit=ocean?126:62;
            Area(id,0);queue.clear();queue.push_back(point);Label(index,id);int32_t fresh=0;
            for(size_t head=0;head<queue.size();++head) {
                const auto p=queue[head];Area(id,int64_t(r.Area(id))+1);
                if(p[1]==0 || p[1]==g.height-1) {continue;}
                for(size_t k=1;k<=8;++k) {
                    const auto n=Neighbor(p,k);if(!n) {continue;}
                    const auto j=Index(*n);
                    if((Band(j)<3)!=ocean || s.records[j][3]) {continue;}
                    if(ocean && (s.records[j][12]&128)) {++fresh;}
                    Label(j,id);queue.push_back(*n);
                }
            }
            if(fresh) {
                const bool enabled=fresh>=int64_t(3)*r.Area(id)/4;
                for(size_t i=0;i<s.records.size();++i) {if(s.records[i][3]==id) {
                    const auto marks=Read(s.records[i].data()+12);
                    Tile(i,12,4,enabled?marks|128u:marks&~128u);
                }}
            }
            Pump();
            if(id==limit) {
                auto best=r.Area(limit);int32_t smallest_region=-1;
                for(int32_t k=low;k<limit;++k) {if(r.Area(k)<best) {best=r.Area(k);smallest_region=k;}}
                const auto overflow=limit+1;
                if(smallest_region>=0) {
                    Area(overflow,int64_t(r.Area(overflow))+r.Area(smallest_region));Area(smallest_region,0);Relabel(smallest_region,overflow);
                    Area(smallest_region,int64_t(r.Area(smallest_region))+r.Area(limit));Area(limit,0);Relabel(limit,smallest_region);
                } else {Area(overflow,int64_t(r.Area(overflow))+r.Area(limit));Area(limit,0);Relabel(limit,overflow);}
            }
        }}
        for(const auto y:{0,g.height-1}) {for(int32_t x=y&1;x<g.width;x+=2) {
            const auto index=Index({{x,y}});const auto id=Band(index)>=3?63:127;
            Label(index,id);Area(id,int64_t(r.Area(id))+1);
        }}
        int32_t largest=0;int64_t sum=0;
        for(size_t i=1;i<63;++i) {const auto area=r.Area(i);largest=std::max(largest,area);sum+=area;}
        if(largest>=4*sum/5) {r.secondary_flags|=256;}else {r.secondary_flags&=~256u;}
        for(size_t i=0;i<64;++i) {Region(i*28+20,4,0);Region(i*28+24,4,0);}
        for(int32_t y=0;y<g.height;++y) {for(int32_t x=y&1;x<g.width;x+=2) {
            const Coordinate p{{x,y}};const auto id=s.records[Index(p)][3];if(id>=64) {continue;}
            for(size_t k=0;k<8;++k) {
                const auto n=Neighbor(p,k+1);if(!n) {continue;}
                const auto other=s.records[Index(*n)][3];if(other<64) {continue;}
                const auto bit=other-64;const auto off=size_t(id)*28+20+bit/8;
                Region(off,1,r.records[off]|(1u<<(bit&7)));k+=2-(k&1);
            }
        }}
        const auto size=size_t((r.summary_width+1)/2)*size_t(r.summary_height);
        if(r.summary.size()!=size) {r.summary.assign(size,r.summary_initial);}
        for(int32_t y=0;y<r.summary_height;++y) {for(int32_t x=y&1;x<r.summary_width;x+=2) {
            uint8_t value=0;
            for(size_t k=0;k<9;++k) {
                const auto n=Neighbor({{5*x,5*y}},k);
                if(n && Band(Index(*n))>=3) {value=s.records[Index(*n)][3];break;}
            }
            // The original uses floor stride even for odd summary widths.
            r.summary[size_t(r.summary_width/2)*size_t(y)+size_t(x/2)]=value;
        }}
        Pump();
    }
private:
    size_t Index(Coordinate p) const {return size_t(p[1])*size_t(g.width/2)+size_t(p[0]/2);}
    int Band(size_t index) const {return s.records[index][0]>>5;}
    std::optional<Coordinate> Neighbor(Coordinate p,size_t offset) const {
        p[0]+=smac_shaping::OFFSETS[offset][0];p[1]+=smac_shaping::OFFSETS[offset][1];
        if(s.topology==0) {if(p[0]<0) {p[0]+=g.width;}else if(p[0]>=g.width) {p[0]-=g.width;}}
        if(p[0]<0 || p[0]>=g.width || p[1]<0 || p[1]>=g.height) {return std::nullopt;}
        return p;
    }
    void Tile(size_t index,size_t field,unsigned size,uint32_t value) {
        for(unsigned i=0;i<size;++i) {s.records[index][field+i]=uint8_t(value>>(8*i));}
        if(t) {++t->tile_stores;if(t->on_tile_store) {t->on_tile_store(uint32_t(index*44+field),size,value);}}
    }
    void Label(size_t index,int32_t value) {Tile(index,3,1,uint32_t(value));}
    void Relabel(int32_t from,int32_t to) {for(size_t i=0;i<s.records.size();++i) {if(s.records[i][3]==from) {Label(i,to);}}}
    void Region(size_t offset,unsigned size,uint32_t value) {
        for(unsigned i=0;i<size;++i) {r.records[offset+i]=uint8_t(value>>(8*i));}
        if(t) {++t->region_stores;if(t->on_region_store) {t->on_region_store(uint32_t(offset),size,value);}}
    }
    void Area(size_t id,int64_t value) {Region(id*28,4,uint32_t(value));}
    void Pump() {if(t) {++t->pumps;if(t->checkpoint) {t->checkpoint();}}}
    State& s;Geometry g;Regions& r;RegionTrace* t;
};
}
int32_t Regions::Area(size_t region) const {
    if(region>=128) {throw std::out_of_range("Native region index");}
    const auto value=Read(records.data()+region*28);
    return value<=INT32_MAX?int32_t(value):int32_t(int64_t(value)-4294967296LL);
}
void RebuildRegions(State& state,Geometry geometry,Regions& regions,RegionTrace* trace) {Builder(state,geometry,regions,trace).Run();}
}

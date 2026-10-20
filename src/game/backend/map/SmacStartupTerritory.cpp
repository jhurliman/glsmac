#include "SmacStartupTerritory.h"
#include "SmacRawSiteQuality.h"
#include "game/backend/faction/SmacComputerGoals.h"
#include "game/backend/resource/SmacSiteSignal.h"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace game::backend::map {
namespace {
constexpr size_t faction_base=0x95d1a8;
template<class Record> uint32_t Read(const Record& r,size_t off,unsigned size=4) {uint32_t value=0;for(unsigned k=0;k<size;++k) {value|=uint32_t(r.at(off+k))<<(8*k);}return value;}
template<class Record> void Write(Record& r,size_t off,uint32_t value,unsigned size=4) {for(unsigned k=0;k<size;++k) {r.at(off+k)=uint8_t(value>>(8*k));}}
int32_t Signed(uint32_t value) {return value<=INT32_MAX?int32_t(value):int32_t(int64_t(value)-4294967296LL);}
int16_t Short(uint32_t value) {return value<=INT16_MAX?int16_t(value):int16_t(int32_t(value)-65536);}
template<size_t N> void LoadGoals(std::array<faction::SmacComputerGoal,N>& goals,const faction::SmacStartupFaction& actor,size_t start) {
    for(size_t i=0;i<N;++i) {const auto off=start-faction_base+16*i;
        goals[i]={Short(Read(actor.record,off,2)),Short(Read(actor.record,off+2,2)),Signed(Read(actor.record,off+4)),Signed(Read(actor.record,off+8)),Signed(Read(actor.record,off+12))};}
}
template<size_t N> void StoreGoals(const std::array<faction::SmacComputerGoal,N>& goals,faction::SmacStartupFaction& actor,size_t start) {
    for(size_t i=0;i<N;++i) {const auto off=start-faction_base+16*i;const auto& g=goals[i];
        Write(actor.record,off,uint16_t(g.type),2);Write(actor.record,off+2,uint16_t(g.priority),2);Write(actor.record,off+4,uint32_t(g.x));
        Write(actor.record,off+8,uint32_t(g.y));Write(actor.record,off+12,uint32_t(g.auxiliary));}
}
}
SmacStartupTerritoryResult RebuildSmacStartupTerritory(std::vector<std::array<uint8_t,44>>& tiles,
    std::array<faction::SmacStartupFaction,8>& actors,const std::vector<base::SmacStartupBaseRecord>& bases,
    smac_generation::Regions& regions,const SmacStartupTerritoryOptions& o,const faction::SmacStartupStoreObserver& stores,
    const SmacStartupQualityObserver& queries) {
    const auto g=o.geometry;
    if(g.width<16 || g.width>512 || (g.width&1) || g.height<8 || g.height>256 || tiles.size()!=size_t(g.width/2)*size_t(g.height)
        || bases.size()>512 || o.border<0 || o.difficulty<0 || o.difficulty>5) {throw std::invalid_argument("Invalid native startup territory inputs");}
    const auto index=[&](smac_site::Coordinate p) {if(p[0]<0 || p[0]>=g.width || p[1]<0 || p[1]>=g.height || ((p[0]^p[1])&1)) {
            throw std::invalid_argument("Startup territory coordinate outside parity grid");}
        return size_t(p[1])*size_t(g.width/2)+size_t(p[0]/2);};
    for(const auto& t:tiles) {if(t[3]>=128) {throw std::invalid_argument("Startup territory region exceeds native table");}}
    struct Base {smac_site::Coordinate position;uint8_t owner;};std::vector<Base> centers;
    for(const auto& b:bases) {const smac_site::Coordinate p{{Short(Read(b,0,2)),Short(Read(b,2,2))}};(void)index(p);
        if(b[4]>7) {throw std::invalid_argument("Invalid startup territory Base owner");}centers.push_back({p,b[4]});}
    const auto distance=[&](smac_site::Coordinate p,smac_site::Coordinate q) {auto dx=std::abs(p[0]-q[0]);const auto dy=std::abs(p[1]-q[1]);
        if(g.wrap) {dx=std::min(dx,g.width-dx);}return std::max(dx,dy)-(std::abs(dx-dy)+2)/4;};
    SmacStartupTerritoryResult result{o.dirty,9999,0};
    const auto nearest=[&](smac_site::Coordinate p,uint8_t region,int owner=-1) {int chosen=-1,best=9999;
        for(size_t i=0;i<centers.size();++i) {const auto& b=centers[i];if(tiles[index(b.position)][3]!=region || (owner>=0 && b.owner!=owner)) {continue;}
            const auto d=distance(p,b.position);if(d<=best) {best=d;chosen=int(i);}}
        result.last_distance=best;return chosen;};
    std::array<faction::SmacComputerGoals,8> goals;
    for(size_t f=0;f<8;++f) {LoadGoals(goals[f].active,actors[f],0x95eba0);LoadGoals(goals[f].persistent,actors[f],0x95f050);
        std::fill(actors[f].record.begin()+(0x95e620-faction_base),actors[f].record.begin()+(0x95eb20-faction_base),0);}
    smac_site::Areas areas;for(size_t r=0;r<128;++r) {areas[r]=regions.Area(r);Write(regions.records,28*r+8,0);Write(regions.records,28*r+12,0);}
    const smac_site::RawQualityContext quality{g,o.pattern,o.rules,o.coefficient_u,o.coefficient_v};
    const auto increment=[&](size_t f,size_t address,unsigned size) {auto& raw=actors[f].record;const auto off=address-faction_base;Write(raw,off,Read(raw,off,size)+1u,size);};
    for(int32_t y=0;y<g.height;++y) {for(int32_t x=y&1;x<g.width;x+=2) {
        const smac_site::Coordinate p{{x,y}};const auto at=index(p);auto& tile=tiles[at];const auto region=tile[3];const auto items=Read(tile,8);const bool sea=(tile[0]>>5)<3;
        const auto store=[&](size_t off,unsigned size,uint32_t value,uint32_t pc) {Write(tile,off,value,size);if(stores) {stores({uint32_t(at*44+off),uint8_t(size),value,pc});}};
        for(unsigned f=1;f<8;++f) {const auto flags=Read(actors[f].record,0);
            const bool known=(flags&0x20000000)?bool(items&0x100000):bool((flags&0x200) || (tile[4]&(1u<<f)));
            if(known) {increment(f,0x95e720+2*region,2);}}
        const auto selected=nearest(p,region);int owner=-1;
        if(selected>=0 && result.last_distance<(sea?o.border/2:o.border)) {
            owner=centers[selected].owner;
            if(sea) {for(const auto& b:centers) {if(b.owner!=owner && (tiles[index(b.position)][0]>>5)>=3
                && distance(p,b.position)<=result.last_distance) {owner=-1;break;}}}
        }
        const int old=tile[7]<128?int(tile[7]):int(tile[7])-256;
        if(owner!=old) {store(12,4,Read(tile,12)|0x400000u,0x5390d6);result.dirty|=1;}
        store(7,1,uint8_t(owner),0x5390e7);const auto f=unsigned(std::max(owner,0));
        increment(f,0x95e620+2*region,2);
        if((tile[0]&0x18) && (tile[5]&0xc0)<0x80 && !(items&0x20)) {increment(f,0x95e820+2*region,2);if(items&0x1000) {increment(f,0x95e920+2*region,2);}}
        int32_t rating=(items&0x1000)?0:int32_t(tile[2]>>4);
        if(!(items&0x1000) && !rating) {rating=smac_site::RawQuality(tiles,areas,quality,p).rating;++result.uncached;
            if(queries) {queries(p,rating);}store(2,1,uint8_t((tile[2]&15)|((rating&15)<<4)),0x5a8fd1);}
        Write(regions.records,28*region+8,std::max(Read(regions.records,28*region+8),uint32_t(rating)));
        if(f) {auto& maximum=actors[f].record[0x95ea20-faction_base+region];maximum=std::max(maximum,uint8_t(rating));}
        const bool pod=resource::smac::ResolveSiteSignal({x,y,resource::smac::AltitudeBand(tile[0]>>5),items,bool(o.rules&0x2000),o.pattern,o.coefficient_u,o.coefficient_v})!=0;
        if(!pod) {continue;}
        Write(regions.records,28*region+12,Read(regions.records,28*region+12)+1u);increment(f,0x95eaa0+region,1);
        const auto admitted=[&](unsigned actor){return (tile[4]&(1u<<actor)) || (!(o.human_mask&(1u<<actor)) && o.difficulty>3);};
        if(f) {if(admitted(f)) {goals[f].InsertPersistent(3,1,x,y,g.width,g.height);}}
        else {for(unsigned actor=1;actor<8;++actor) {
            if(!actors[actor].record[0x95e320-faction_base+region] || !admitted(actor)) {continue;}
            const auto& persistent=goals[actor].persistent;
            if(std::any_of(persistent.begin(),persistent.end(),[&](const auto& goal){return goal.type==3 && goal.x==x && goal.y==y;})) {continue;}
            if(nearest(p,region,int(actor))>=0 && result.last_distance>=o.border && int64_t(result.last_distance)<=2*int64_t(o.border)) {
                goals[actor].InsertPersistent(3,1,x,y,g.width,g.height);}
        }}
    }}
    for(size_t f=0;f<8;++f) {StoreGoals(goals[f].active,actors[f],0x95eba0);StoreGoals(goals[f].persistent,actors[f],0x95f050);}
    return result;
}
}

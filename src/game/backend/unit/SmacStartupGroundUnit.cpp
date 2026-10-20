#include "SmacStartupGroundUnit.h"
#include "SmacUnitInitialization.h"
#include "game/backend/map/SmacOrdinaryLandExploration.h"
#include "game/backend/map/SmacShapingOffsets.h"
#include <algorithm>
#include <cstdlib>
#include <optional>
#include <set>
#include <stdexcept>

namespace game::backend::unit {
namespace {
template<class Record> uint32_t Read(const Record& r,size_t off,unsigned size=4) {uint32_t value=0;for(unsigned k=0;k<size;++k) {value|=uint32_t(r.at(off+k))<<(8*k);}return value;}
template<class Record> void Write(Record& r,size_t off,uint32_t value,unsigned size=4) {for(unsigned k=0;k<size;++k) {r.at(off+k)=uint8_t(value>>(8*k));}}
int16_t Short(uint32_t value) {return value<=INT16_MAX?int16_t(value):int16_t(int32_t(value)-65536);}
}
int32_t CreateSmacStartupGroundUnit(std::vector<std::array<uint8_t,44>>& tiles,std::vector<SmacNativeUnitRecord>& units,
    faction::SmacStartupFaction& actor,const std::vector<base::SmacStartupBaseRecord>& bases,
    SmacStartupUnitFactoryState& state,const SmacStartupGroundUnitOptions& o,const faction::SmacStartupStoreObserver& stores,const SmacStartupUnitMetadataObserver& metadata) {
    if(units.size()>=2048) {return -1;}
    const auto g=o.geometry;
    if(o.owner<1 || o.owner>7 || o.design<0 || o.design>=512 || (o.role!=2 && o.role!=3 && o.role!=8 && o.role!=9)
        || o.display_faction<0 || o.display_faction>7 || o.abilities || o.extended_sight || o.sensor_sight || bases.size()>512
        || g.width<16 || g.width>512 || (g.width&1) || g.height<8 || g.height>256 || tiles.size()!=size_t(g.width/2)*size_t(g.height)
        || (o.role<=2 && Read(actor.record,0x95d598-0x95d1a8)<0x80000000u)) {
        throw std::invalid_argument("Unsupported original ordinary ground factory admission");
    }
    const auto index=[&](map::smac_site::Coordinate p) {if(p[0]<0 || p[0]>=g.width || p[1]<0 || p[1]>=g.height || ((p[0]^p[1])&1)) {
            throw std::invalid_argument("Ground factory coordinate outside parity grid");}return size_t(p[1])*size_t(g.width/2)+size_t(p[0]/2);};
    const auto center=index(o.position);
    map::smac_exploration::Input sight{g.width,g.height,g.wrap,{o.position[0],o.position[1]},o.owner,{},{}};
    sight.sea.reserve(tiles.size());sight.explored.reserve(tiles.size());
    for(const auto& tile:tiles) {sight.sea.push_back((tile[0]>>5)<3);sight.explored.push_back(tile[4]);}
    const auto exposed=map::smac_exploration::Explore(sight);
    const auto resolve=[&](map::smac_site::Coordinate p)->std::optional<map::smac_site::Coordinate> {
        if(g.wrap) {if(p[0]<0) {p[0]+=g.width;}else if(p[0]>=g.width) {p[0]-=g.width;}}
        if(p[0]<0 || p[0]>=g.width || p[1]<0 || p[1]>=g.height) {return {};}return p;
    };
    std::set<size_t> footprint;
    for(size_t i=0;i<25;++i) {const auto& d=map::smac_shaping::OFFSETS[i];const auto p=resolve({{o.position[0]+d[0],o.position[1]+d[1]}});if(!p) {continue;}
        footprint.insert(index(*p));if(i<9) {continue;}
        for(size_t k=1;k<9;++k) {const auto& n=map::smac_shaping::OFFSETS[k];const auto q=resolve({{(*p)[0]+n[0],(*p)[1]+n[1]}});if(q) {footprint.insert(index(*q));}}
    }
    for(size_t i=0;i<units.size();++i) {const auto& unit=units[i];unit.Validate();
        if(unit.id!=i) {throw std::invalid_argument("Ground factory requires dense original identities");}
        if(unit.x>=0 && unit.y>=0 && unit.owner!=o.owner && footprint.count(index({{unit.x,unit.y}}))) {
            throw std::invalid_argument("Foreign Unit detection requires its visibility producer");}
    }
    const auto distance=[&](map::smac_site::Coordinate p) {auto dx=std::abs(o.position[0]-p[0]);const auto dy=std::abs(o.position[1]-p[1]);
        if(g.wrap) {dx=std::min(dx,g.width-dx);}return std::max(dx,dy)-(std::abs(dx-dy)+2)/4;};
    int nearest=-1,best=9999;
    for(size_t i=0;i<bases.size();++i) {const auto& b=bases[i];const map::smac_site::Coordinate p{{Short(Read(b,0,2)),Short(Read(b,2,2))}};
        const auto at=index(p);if(b[4]>7) {throw std::invalid_argument("Invalid original Base owner in ground factory");}
        if(footprint.count(at) && b[4]!=o.owner) {throw std::invalid_argument("Foreign Base disclosure requires its visibility producer");}
        const auto d=distance(p);if(d<=best) {best=d;nearest=int(i);}
    }
    const auto identity=units.size();auto fresh=InitializeSmacUnitRecord(int32_t(identity),o.design,o.owner,o.morale);
    units.push_back(fresh);
    constexpr size_t faction_base=0x95d1a8;
    if(o.role<=7) {const auto off=0x95e1fc-faction_base;Write(actor.record,off,Read(actor.record,off)+1u);}
    ++actor.record[0x95d9fc-faction_base+o.design];
    if(nearest>=0) {
        state.last_distance=best;
        if(metadata) {metadata({SmacStartupUnitMetadataStore::Space::Global,0x91871c,4,uint32_t(best),0x4f950d});}
        if(bases[nearest][4]==o.owner) {units.back().home_base=int16_t(nearest);
            if(metadata) {metadata({SmacStartupUnitMetadataStore::Space::Units,uint32_t(52*identity+46),2,uint16_t(nearest),0x5d8617});}
            if(nearest==state.cached_base) {state.cached_base=-1;
                if(metadata) {metadata({SmacStartupUnitMetadataStore::Space::Global,0x6aae1c,4,UINT32_MAX,0x5d8629});}}
        }
    }
    const bool had_base=Read(tiles[center],8)&1;
    SmacUnitPlacementWorld placement{units,tiles,g.width,g.height,state.placement};PlaceSmacUnit(placement,identity,o.position[0],o.position[1],0);
    const auto store=[&](size_t at,size_t off,unsigned size,uint32_t value,uint32_t pc) {
        Write(tiles[at],off,value,size);if(stores) {stores({uint32_t(at*44+off),uint8_t(size),value,pc});}};
    if(units.back().stack_next<0 && !had_base) {if(stores) {stores({uint32_t(center*44+2),1,tiles[center][2],0x5a8f93});}}
    if(stores) {stores({uint32_t(center*44+8),4,Read(tiles[center],8),0x5a91d2});}
    size_t direct=0;for(size_t i=0;i<9;++i) {const auto& delta=map::smac_shaping::OFFSETS[i];auto x=o.position[0]+delta[0];const auto y=o.position[1]+delta[1];
        if(g.wrap) {if(x<0) {x+=g.width;}else if(x>=g.width) {x-=g.width;}}if(x>=0 && x<g.width && y>=0 && y<g.height) {++direct;}}
    const auto bit=uint8_t(1u<<o.owner);const bool reveal=Read(actor.record,0)&0x200;
    for(size_t i=0;i<exposed.refresh_improvements.size();++i) {const auto p=exposed.refresh_improvements[i];const auto at=index({{p.x,p.y}});auto& tile=tiles[at];const bool outer=i>=direct;
        if(!(tile[4]&bit) && o.owner==o.display_faction && !o.editor && !reveal) {
            store(at,12,4,Read(tile,12)|0x400000u,outer?0x5ce401:0x5ce1e8);state.dirty|=1;}
        store(at,12+4*o.owner,4,Read(tile,8),outer?0x5ce41a:0x5ce22a);
        store(at,4,1,uint8_t(tile[4]|bit),outer?0x5ce431:0x5ce22e);
    }
    return int32_t(identity);
}
}

#include "SmacStartupBase.h"
#include "game/backend/faction/SmacTechnologyQuery.h"
#include "game/backend/resource/SmacMineralCostFactor.h"
#include "game/backend/map/SmacShapingOffsets.h"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace game::backend::base {
namespace {
constexpr size_t faction_base=0x95d1a8;
template<class Record> uint32_t Read(const Record& r,size_t off,unsigned size=4) {uint32_t value=0;for(unsigned k=0;k<size;++k) {value|=uint32_t(r.at(off+k))<<(8*k);}return value;}
template<class Record> void Write(Record& r,size_t off,uint32_t value,unsigned size=4) {for(unsigned k=0;k<size;++k) {r.at(off+k)=uint8_t(value>>(8*k));}}
int32_t Signed(uint32_t v) {return v<=INT32_MAX?int32_t(v):int32_t(int64_t(v)-4294967296LL);}
void Validate(const std::vector<std::array<uint8_t,44>>& tiles,map::smac_site::Geometry g,map::smac_site::Coordinate p,int32_t owner) {
    if(g.width<16 || g.width>512 || (g.width&1) || g.height<8 || g.height>256 || tiles.size()!=size_t(g.width/2)*size_t(g.height)
        || p[0]<0 || p[0]>=g.width || p[1]<0 || p[1]>=g.height || ((p[0]^p[1])&1) || owner<0 || owner>7) {
        throw std::invalid_argument("Invalid original startup Base geometry or owner");}
}
}
SmacBaseSeedResult SeedSmacStartupBase(SmacStartupBaseRecord& r,std::vector<std::array<uint8_t,44>>& tiles,
    faction::SmacStartupFaction& state,const SmacBaseSeedOptions& o,const SmacBaseSeedCatalog& catalog) {
    if(o.base_count>=512) {return {-1,o.base_count,{}};}
    Validate(tiles,o.geometry,o.position,o.owner);
    if(o.base_count<0 || o.reference_owner<0 || o.reference_owner>7 || o.map_category<0 || o.map_category>4
        || o.name.size()>=25 || o.name.find('\0')!=std::string::npos) {throw std::invalid_argument("Invalid original startup Base inputs");}
    auto& actor=state.record;const auto fp=[&](size_t address){return Signed(Read(actor,address-faction_base));};
    const auto fput=[&](size_t address,uint32_t value){Write(actor,address-faction_base,value);};
    const auto has=[&](int32_t technology) {
        if(technology==-1) {return faction::QuerySmacTechnology(o.owner,technology,0,{{-1,-1}});}
        if(technology<0 || technology>=88) {return false;}
        return faction::QuerySmacTechnology(o.owner,technology,catalog.ownership[technology],catalog.prerequisites[technology]);
    };
    const auto target=has(catalog.formers_prerequisite)?1:fp(0x95d1b0)>1?2:0;
    using namespace resource::smac;
    const auto size=o.map_category==0?MineralCostWorldSize::Tiny:o.map_category==1?MineralCostWorldSize::Small:MineralCostWorldSize::StandardOrLarger;
    const auto factor=CalculateMineralCostFactor({bool(o.human_mask&(1u<<o.owner)),o.difficulty,bool(o.human_mask&(1u<<o.reference_owner)),o.network,
        false,size,o.mineral_multiplier,fp(0x95d418)});
    const auto credit=int64_t(factor)*catalog.design_costs[target]/2;
    if(credit>INT32_MAX) {throw std::overflow_error("Startup Base mineral credit exceeds signed32");}
    const auto index=size_t(o.position[1])*size_t(o.geometry.width/2)+size_t(o.position[0]/2);auto& tile=tiles[index];
    const auto region=tile[3];if(region>=128) {throw std::invalid_argument("Invalid startup Base region");}
    const auto count=Read(actor,0x95e200-faction_base)+1u;
    auto population=o.previous_same_owner?3:1;
    if(count==1) {population=o.network?3+int(o.extra_population || fp(0x95d494)>0)-int(o.population_penalty):1;}
    const auto disclosure=uint8_t(tile[4]|(1u<<o.owner));
    Write(tile,8,Read(tile,8)|1u);tile[2]=uint8_t((tile[2]&240)|(o.owner&15));
    if(o.owner) {Write(tile,12+4*o.owner,Read(tile,8));}
    fput(0x95e200,count);++actor[0x95e320-faction_base+region];
    Write(r,0,uint32_t(o.position[0]),2);Write(r,2,uint32_t(o.position[1]),2);r[4]=r[5]=uint8_t(o.owner);
    for(const auto off:{7,8,9}) {r[off]=0;}r[10]=disclosure;
    std::copy(o.name.begin(),o.name.end(),r.begin()+19);r[19+o.name.size()]=0;
    for(const auto off:{48,52,60,64,72,76,120,124,128,292}) {Write(r,off,0);}
    Write(r,56,uint32_t(fp(0x95d378)));Write(r,268,UINT32_MAX);Write(r,272,UINT32_MAX,2);
    Write(r,80,uint32_t(target));Write(r,64,uint32_t(credit));Write(r,44,UINT32_MAX,2);Write(r,46,UINT32_MAX,2);Write(r,140,0);r[144]=0;
    for(const auto off:{288,292,296,300}) {Write(r,off,0);}
    const auto facility=[&](int32_t id){r[140+id/8]|=uint8_t(1u<<(id&7));};
    if(count==1) {facility(1);if(o.network) {facility(3);if(o.difficulty>=3) {facility(6);}if(fp(0x95d478)>0) {facility(7);}}}
    r[6]=uint8_t(population);if(tile[0]>>5<3) {facility(26);}
    for(unsigned viewer=0;viewer<8;++viewer) {r[11+viewer]=(viewer==unsigned(o.owner) || (disclosure&(1u<<viewer)))?uint8_t(population):0;}
    for(const auto id:catalog.facility_bonuses) {facility(std::clamp(id,1,38));}
    for(size_t i=0;i<38;++i) {if(has(catalog.free_facility_technologies[i])) {facility(int32_t(i+1));}}
    Write(r,68,uint32_t(target));Write(r,152,Read(r,64));Write(r,148,0);
    std::array<uint8_t,8> history{};history[1]=uint8_t(o.owner);Write(history,4,uint32_t(o.position[0]),2);Write(history,6,uint32_t(o.position[1]),2);
    return {o.base_count,o.base_count+1,history};
}
map::smac_site::Coordinate PrepareSmacStartupBaseRadius(std::vector<std::array<uint8_t,44>>& tiles,
    std::array<faction::SmacStartupFaction,8>& factions,map::smac_site::Geometry g,map::smac_site::Coordinate position,
    int32_t owner,const faction::SmacStartupStoreObserver& stores) {
    Validate(tiles,g,position,owner);map::smac_site::Coordinate last{};
    for(size_t i=0;i<49;++i) {
        auto x=position[0]+map::smac_shaping::OFFSETS[i][0];const auto y=position[1]+map::smac_shaping::OFFSETS[i][1];
        if(g.wrap) {if(x<0) {x+=g.width;}else if(x>=g.width) {x-=g.width;}}last={{x,y}};
        if(x<0 || x>=g.width || y<0 || y>=g.height) {continue;}
        const auto index=size_t(y)*size_t(g.width/2)+size_t(x/2);auto& tile=tiles[index];
        if(i<21) {
            const auto items=Read(tile,8)|0x1000u;Write(tile,8,items);if(stores) {stores({uint32_t(index*44+8),4,items,0x5a91d2});}
            tile[5]=uint8_t((tile[5]&248)|(owner&7));if(stores) {stores({uint32_t(index*44+5),1,tile[5],0x5a9093});}
        }else {tile[2]&=15;if(stores) {stores({uint32_t(index*44+2),1,tile[2],0x5a8fd1});}}
    }
    const auto distance=[&](int32_t x,int32_t y) {auto dx=std::abs(int64_t(last[0])-x);const auto dy=std::abs(int64_t(last[1])-y);
        if(g.wrap) {dx=std::min(dx,int64_t(g.width)-dx);}return std::max(dx,dy)-(std::abs(dx-dy)+2)/4;};
    for(auto& actor:factions) {auto& raw=actor.record;
        for(size_t j=0;j<25;++j) {const auto p=0x95f050-faction_base+16*j;
            if(Read(raw,p,2)!=8) {continue;}const auto x=Signed(Read(raw,p+4)),y=Signed(Read(raw,p+8));
            if(distance(x,y)>3) {continue;}Write(raw,p,UINT32_MAX,2);Write(raw,p+2,0,2);
            for(size_t k=0;k<75;++k) {const auto a=0x95eba0-faction_base+16*k;
                if(Read(raw,a,2)==8 && Signed(Read(raw,a+4))==x && Signed(Read(raw,a+8))==y) {Write(raw,a,UINT32_MAX,2);}}
        }
    }
    return last;
}
}

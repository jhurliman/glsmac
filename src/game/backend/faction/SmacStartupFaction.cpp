#include "SmacStartupFaction.h"
#include <algorithm>
#include <stdexcept>
#include <utility>
#include "game/backend/map/SmacShapingOffsets.h"

namespace game::backend::faction {
SmacFactionPrefixResult InitializeSmacFactionPrefix(SmacStartupFaction& state,
    const SmacFactionPrefixOptions& o,SmacGameplayRandomState& random,const SmacStartupObserver& observer) {
    if(o.faction<0 || o.faction>7 || o.difficulty<0 || o.difficulty>5) {throw std::invalid_argument("Invalid original faction startup options");}
    std::array<std::string,8> channels;
    for(size_t i=0;i<channels.size();++i) {
        channels[i]=o.channel_prefix+o.channel_separator+std::to_string(i+1);
        if(channels[i].size()>=24 || channels[i].find('\0')!=std::string::npos) {throw std::invalid_argument("Original faction channel exceeds 24-byte slot");}
    }
    constexpr uint32_t base=0x95d1a8;
    const auto put=[&](uint32_t address,uint32_t value,unsigned size=4) {
        for(unsigned i=0;i<size;++i) {state.record.at(address-base+i)=uint8_t(value>>(8*i));}
    };
    const auto zero=[&](uint32_t address,unsigned count,unsigned size=4) {for(unsigned i=0;i<count;++i) {put(address+size*i,0,size);}};
    const auto draw=[&](uint32_t caller) {const auto before=random.Crt();const auto value=random.NextCrtRaw15();
        if(observer) {observer({caller,before,random.Crt(),value});}return uint32_t(value);};
    for(const auto a:{0x95d59cu,0x95d598u,0x95d60cu}) {put(a,UINT32_MAX);}
    for(const auto table:{std::pair<uint32_t,unsigned>{0x95eba0,75},{0x95f050,25}}) {
        for(unsigned i=0;i<table.second;++i) {const auto a=table.first+16*i;
            put(a,UINT32_MAX,2);put(a+2,0,2);put(a+8,0);put(a+4,0);put(a+12,0);}
    }
    state.scratch.fill(255);
    put(0x95d1a8,0);put(0x95d378,o.difficulty==0?0xc0dfbe41u:0xdfbe41u);
    put(0x95d1b4,0);put(0x95d1b8,0);
    put(0x95d584,uint32_t(o.definition.fight));put(0x95d594,uint32_t(o.definition.tech));
    put(0x95d58c,uint32_t(o.definition.power));put(0x95d590,uint32_t(o.definition.growth));put(0x95d588,uint32_t(o.definition.wealth));
    put(0x95d610,0);
    for(size_t i=0;i<8;++i) {put(0x95d6d4+4*i,0);
        std::copy(channels[i].begin(),channels[i].end(),state.record.begin()+(0x95d614-base)+24*i);
        state.record[(0x95d614-base)+24*i+channels[i].size()]=0;
    }
    for(unsigned i=0;i<4;++i) {put(0x95d3e0+4*i,0);put(0x95d3d0+4*i,0);}
    zero(0x95d3f4,11);
    for(const auto a:{0x95d3f0u,0x95f200u,0x95f204u,0x95f208u,0x95f20cu,0x95f210u}) {put(a,0);}
    put(0x95d1b0,(o.human_mask&(1u<<o.faction))?uint32_t(o.difficulty):3u);
    for(const auto a:{0x95d2ccu,0x95d2d0u,0x95d374u,0x95d3c8u,0x95d37cu,0x95d380u,0x95d608u,0x95d268u}) {put(a,0);}
    put(0x95d5a4,uint32_t(-999));
    if(o.secondary&0x800000) {
        put(0x95d584,draw(0x5c8c54)%3-1);put(0x95d594,draw(0x5c8c68)%2);
        put(0x95d58c,draw(0x5c8c7f)%2);put(0x95d590,draw(0x5c8c96)%2);put(0x95d588,draw(0x5c8cad)%2);
    }
    if(o.rules&0x40) {put(0x95d584,1);}
    for(const auto a:{0x95e1fcu,0x95e200u,0x95e204u,0x95e208u,0x95e20cu,0x95e210u,0x95d384u}) {put(a,0);}
    for(unsigned i=0;i<8;++i) {put(0x95d388+4*i,0);put(0x95d3a8+4*i,0);}
    put(0x95d580,0);
    for(unsigned i=0;i<128;++i) {put(0x95e220+2*i,0,2);put(0x95e320+i,0,1);put(0x95e3a0+i,0,1);put(0x95e420+2*i,0,2);put(0x95e520+2*i,0,2);}
    for(unsigned i=0;i<11;++i) {put(0x95d5a8+4*i,0);put(0x95d5d4+4*i,0);}
    zero(0x95f214,10);
    for(const auto a:{0x95f1e0u,0x95f1e4u,0x95f1e8u,0x95f1ecu,0x95f1f0u,0x95f1f4u}) {put(a,0);}
    put(0x95f1f8,1);
    put(0x95d3c8,uint32_t(o.definition.energy)+uint32_t(o.network?100:o.starting_energy));
    put(0x95d3cc,0);put(0x95d578,UINT32_MAX);put(0x95d1bc,0);
    for(unsigned i=0;i<8;++i) {
        const auto a=0x95d240+4*i;put(a-0x80,0);put(a,0);
        const auto span=uint32_t(3*((o.rules&0x40)?5:o.difficulty)+8);
        put(a-0x40,std::clamp(draw(0x5c8e3f)%span,1u,16u));
        for(const auto address:{a+0x94,a+0xb4,a+0xd4,a+0x114,a+0xf4}) {put(address,0);}
        put(0x95d260+i,0,1);put(a-0x20,UINT32_MAX);
        for(const auto address:{a+0x6c,a+0x2c,a+0x4c}) {put(address,0);}
    }
    zero(0x95d9fc,512,1);zero(0x95dbfc,512,1);zero(0x95ddfc,512,2);
    zero(0x95d4d4,12);zero(0x95d514,22);put(0x95d56c,0,1);
    for(const auto a:{0x95d600u,0x95d9acu,0x95d9b0u,0x95d9b4u,0x95d9b8u,0x95d510u,0x95d504u,0x95d50cu,0x95d570u,0x95d57cu,0x95d508u,0x95f220u}) {put(a,0);}
    put(0x95d604,0,1);put(0x95d574,UINT32_MAX);
    for(const auto a:{0x95d5a0u,0x95d4a4u,0x95d4a8u,0x95d4b8u,0x95d4b4u,0x95d4acu,0x95d4bcu,0x95d4c0u,0x95d4c4u,0x95d4c8u,0x95d4b0u,0x95d9bcu,0x95d9c8u,0x95d9ccu,0x95d9c0u,0x95d9c4u,0x95d9d0u,0x95d9d4u,0x95d9d8u,0x95d9dcu,0x95d9e0u}) {put(a,0);}
    auto active=o.active_mask;
    const bool searching=o.faction!=0 && (active&(1u<<o.faction)) && o.mode>=-1;
    if(o.faction && (active&(1u<<o.faction)) && o.mode<-1) {active=uint8_t(active&~(1u<<o.faction));}
    return {active,searching};
}
void PrepareSmacStartingSite(SmacStartupFaction& faction,std::vector<std::array<uint8_t,44>>& tiles,
    const SmacSitePreparation& o,SmacGameplayRandomState& random,const SmacStartupObserver& observer,const SmacStartupStoreObserver& stores) {
    const auto& g=o.geometry;const auto& p=o.position;
    if(g.width<4 || g.width>512 || (g.width&1) || g.height<2 || g.height>256
        || tiles.size()!=size_t(g.width/2)*size_t(g.height) || p[0]<0 || p[0]>=g.width || p[1]<0 || p[1]>=g.height
        || ((p[0]^p[1])&1) || o.faction<0 || o.faction>7 || o.difficulty<0 || o.difficulty>5) {
        throw std::invalid_argument("Invalid original startup site preparation inputs");
    }
    constexpr uint32_t base=0x95d1a8;
    const auto get=[&](uint32_t address) {const auto off=address-base;
        return uint32_t(faction.record[off])|(uint32_t(faction.record[off+1])<<8)|(uint32_t(faction.record[off+2])<<16)|(uint32_t(faction.record[off+3])<<24);};
    const auto signedGet=[&](uint32_t address) {const auto v=get(address);return v<=INT32_MAX?int32_t(v):int32_t(int64_t(v)-4294967296LL);};
    const auto put=[&](uint32_t address,uint32_t value) {for(unsigned i=0;i<4;++i) {faction.record[address-base+i]=uint8_t(value>>(8*i));}};
    const auto mask=[&](uint32_t address,uint32_t value) {put(address,get(address)|value);};
    const auto draw=[&](uint32_t caller) {const auto before=random.Crt();const auto value=random.NextCrtRaw15();
        if(observer) {observer({caller,before,random.Crt(),value});}return uint32_t(value);};
    put(0x95d4d0,5);put(0x95d4cc,0);
    if(o.difficulty>=2) {
        if((get(0x95d590)!=0 || get(0x95d58c)!=0) && draw(0x5c9b25)%4==0) {mask(0x95f228,8);}
        if(o.region_area<250 && draw(0x5c9b68)%3==0) {mask(0x95f228,4);}
        if(signedGet(0x95d584)>=0 && draw(0x5c9b9d)%4==0) {mask(0x95f228,0x10);}
        if(draw(0x5c9bbe)%6==0) {mask(0x95f228,2);}
        if(draw(0x5c9bd5)%6==0) {mask(0x95f228,1);}
        if(draw(0x5c9bf4)%5==0) {mask(0x95f228,0x40);}
        if(((o.rules&0x40) || o.difficulty>=4) && signedGet(0x95d584)>((o.rules&0x40)?-1:0)) {
            if(draw(0x5c9c33)%4==0) {mask(0x95f228,0x20);}
            if(draw(0x5c9c74)%4==0) {mask(0x95d1a8,0x1000000);
                if(draw(0x5c9c91)%2==0) {mask(0x95d1a8,0x2000000);}}
        }
        if(get(0x95d594)!=0 || signedGet(0x95d584)>0) {
            if(o.region_area>=400 && draw(0x5c9cca)%4==0) {mask(0x95d1a8,0x4000000);}
            if(draw(0x5c9d03)%(o.region_area>250?6:4)==0) {mask(0x95d1a8,0x8000000);}
            if(draw(0x5c9d19)%4==0) {mask(0x95d1a8,0x10000000);}
        }
    }
    for(size_t i=0;i<21;++i) {
        auto x=p[0]+map::smac_shaping::OFFSETS[i][0];const auto y=p[1]+map::smac_shaping::OFFSETS[i][1];
        if(g.wrap) {if(x<0) {x+=g.width;}else if(x>=g.width) {x-=g.width;}}
        if(x<0 || x>=g.width || y<0 || y>=g.height) {continue;}
        const auto index=size_t(y)*size_t(g.width/2)+size_t(x/2);auto& tile=tiles[index];
        tile[4]|=uint8_t(1u<<o.faction);
        if(stores) {stores({uint32_t(44*index+4),1,tile[4],0x5c9da8});}
        if(o.faction) {
            const auto value=uint32_t(tile[8])|(uint32_t(tile[9])<<8)|(uint32_t(tile[10])<<16)|(uint32_t(tile[11])<<24);
            const auto off=12+4*o.faction;for(unsigned k=0;k<4;++k) {tile[off+k]=uint8_t(value>>(8*k));}
            if(stores) {stores({uint32_t(44*index+off),4,value,0x5a92bc});}
        }
    }
}
}

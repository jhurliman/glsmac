#include "SmacRegionalStrategy.h"

#include <stdexcept>

namespace game::backend::faction {
namespace {
uint8_t Decide(const SmacRegionalStrategyContext& context,const SmacRegionalStrategyRegion& region,
    uint8_t index,uint8_t old_code,const SmacRegionalStrategyRoute& route) {
    const auto& own=region.peers[context.owner];
    const int64_t bases=own.bases,units=own.units,attack=own.attack;
    if(!bases && (index==63 || index==64 || index==127)) {return 8;}
    if(region.stored_area<0) {throw std::invalid_argument("Regional strategy requires nonnegative retained area");}
    int64_t present=0;
    for(size_t peer=1;peer<=7;++peer) {if(region.peers[peer].bases) {++present;}}
    const int64_t q=3+int(context.leading_human_dominance)+(region.maximum_rating<4)
        -2*int(context.retained_land_region_concentration)+(context.total_bases>4)+present
        +(context.world_tiles>3200)+(context.turn>75);
    const int64_t uncovered=int64_t(region.owned_suitable)-region.covered_suitable;
    bool pressure=false;
    if(bases) {
        pressure=region.maximum_rating<2 || (region.flags&8) || bases>=20 || uncovered<4
            || (2*(int64_t(context.explore)-context.build+q)*bases>int64_t(region.neutral_suitable)+region.owned_suitable
                && uncovered/((bases+1)/2)+region.maximum_rating<2*q);
    }
    uint8_t code=pressure ? 7 : 8;
    int32_t wars=0,take0=0,take2=0;
    uint8_t pact=0;
    for(int32_t peer=1;peer<=7;++peer) {
        const auto& foreign=region.peers[peer];
        if(!foreign.bases && (!foreign.units || attack>5*int64_t(foreign.attack))) {continue;}
        if(peer==context.owner || (foreign.relation&1) || !(foreign.relation&8)) {continue;}
        const bool fresh=context.turn<=1 || (region.stored_area<500 && bases*foreign.bases<50)
            || (region.stored_area<1000 ? (int64_t(context.turn)+context.owner)%8==0
                : (int64_t(context.turn)+2*context.owner)%16==0);
        bool strict,broad;
        if(!fresh) {strict=broad=!foreign.bases || old_code<=2;}
        else if(!foreign.bases) {strict=broad=true;}
        else {
            if(!route) {throw std::invalid_argument("Regional strategy requires actual Base routing");}
            strict=route(peer,index,0);
            broad=strict || route(peer,index,1);
        }
        if(foreign.relation&(16|1024|2048)) {
            if(broad) {++wars;}
            if((attack<=foreign.units || attack<=foreign.attack) && units<=foreign.attack && bases>0 && broad) {++take2;}
            else {++take0;}
        } else if((foreign.relation&32) && bases>0 && foreign.attack>units) {
            if(attack>foreign.units && foreign.bases>0 && foreign.attack<2*attack) {pact|=2;}
            else {pact|=broad ? 1 : 2;}
        }
    }
    if(wars || !bases) {
        if(take0) {pact=0;code=0;}
        else if(pact&2) {code=0;}
        if(take2 || (pact&1)) {code=2;}
    }
    return index>=64 ? 7 : code;
}
}

SmacRegionalStrategyRow RefreshSmacRegionalStrategy(const SmacRegionalStrategyContext& context,
    const std::array<SmacRegionalStrategyRegion,128>& regions,const SmacRegionalStrategyRow& previous,
    const SmacRegionalStrategyRoute& route) {
    if(context.owner<1 || context.owner>7 || context.turn<0 || context.world_tiles<1 || context.world_tiles>65536) {
        throw std::invalid_argument("Regional strategy requires a positive owner, nonnegative turn and bounded world");
    }
    auto result=previous;
    for(size_t region=1;region<result.size();++region) {
        result[region]=Decide(context,regions[region],static_cast<uint8_t>(region),previous[region],route);
    }
    return result;
}
}

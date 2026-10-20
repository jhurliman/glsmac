#include "SmacStartupCapability.h"
#include "SmacTechnologyQuery.h"
#include "game/backend/SmacOriginalArithmetic.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend::faction {
void RefreshSmacStartupCapabilities(std::array<SmacStartupFaction,8>& actors,
    const SmacStartupTechnologyOwnership& ownership,const std::array<std::array<int32_t,2>,89>& prerequisites,
    const SmacCapabilityCatalog& catalog,const SmacCapabilityInputs& in,const SmacCapabilityObserver& observer) {
    if(in.owner<1 || in.owner>7 || in.psi_offense<=0 || in.psi_defense<=0) {
        throw std::invalid_argument("Capability refresh requires positive owner and PSI ratio");
    }
    const auto get=[&](int32_t f,uint32_t address){const auto& r=actors[f].record;const auto off=address-0x95d1a8;
        uint32_t value=0;for(unsigned k=0;k<4;++k) {value|=uint32_t(r[off+k])<<(8*k);}return SmacWrap32(value);};
    const auto put=[&](uint32_t address,int32_t value,uint32_t pc){auto& r=actors[in.owner].record;const auto off=address-0x95d1a8;
        for(unsigned k=0;k<4;++k) {r[off+k]=uint8_t(uint32_t(value)>>(8*k));}
        if(observer) {observer({uint32_t(in.owner*8380)+off,4,uint32_t(value),pc});}};
    const auto tech=[&](int32_t t){return QuerySmacTechnology(in.owner,t,t>=0 && t<89?ownership[t]:0,t>=0 && t<89?prerequisites[t]:std::array<int32_t,2>{{-1,-1}});};
    const auto checked=[](int64_t v){if(v<INT32_MIN || v>INT32_MAX) {throw std::overflow_error("Capability arithmetic outside measured signed32 domain");}return int32_t(v);};
    const auto old_attack=get(in.owner,0x95d9d0),old_defense=get(in.owner,0x95d9d4);
    const auto evaluate=[&](int32_t power,bool weapon){
        if(power>=0) {return power;}
        auto value=checked(int64_t(weapon?old_defense:old_attack)*(weapon?in.psi_offense:in.psi_defense))/(weapon?in.psi_defense:in.psi_offense);
        if(in.psi_percent) {value=checked(int64_t(value)*checked(int64_t(100)+in.psi_percent))/100;}
        if((weapon?in.dream_twister_owner:in.neural_amplifier_owner)==in.owner) {value=checked(int64_t(value)+value/2);}
        return value;
    };
    const bool eligible=(catalog.mind_worm_flags&1) && !(catalog.mind_worm_exclusion&(1u<<in.owner))
        && tech(catalog.mind_worm_prerequisite) && !(catalog.mind_worm_plan==8 && (in.rules&0x400000));
    put(0x95d9c0,eligible?evaluate(catalog.weapons[12].power,true):0,0x577181);put(0x95d9bc,1,0x577187);
    for(const auto& c:catalog.weapons) {
        if(!tech(c.prerequisite) || c.power>=99) {continue;}const auto value=evaluate(c.power,true);
        if(c.power<0 && value>=get(in.owner,0x95d9c0)) {put(0x95d9c0,value,0x5771c0);}
        if(value>=get(in.owner,0x95d9bc)) {put(0x95d9bc,value,0x5771ce);}
    }
    put(0x95d9c4,eligible?evaluate(catalog.armor[9].power,false):0,0x577200);put(0x95d9c8,1,0x577206);
    for(const auto& c:catalog.armor) {
        if(!tech(c.prerequisite)) {continue;}const auto value=evaluate(c.power,false);
        if(c.power<0 && value>=get(in.owner,0x95d9c4)) {put(0x95d9c4,value,0x577241);}
        if(value>=get(in.owner,0x95d9c8)) {put(0x95d9c8,value,0x57724f);}
    }
    put(0x95d9cc,1,0x577261);
    for(const auto& c:catalog.chassis) {if(tech(c.prerequisite) && c.domain==0 && c.speed>=get(in.owner,0x95d9cc)) {put(0x95d9cc,c.speed,0x577297);}}
    constexpr std::array<uint32_t,5> clear{{0x95d9d8,0x95d9d4,0x95d9d0,0x95d9e0,0x95d9dc}};
    for(size_t i=0;i<clear.size();++i) {put(clear[i],0,0x5772ad+uint32_t(i)*6);}
    constexpr std::array<uint32_t,5> own{{0x95d9bc,0x95d9c8,0x95d9cc,0x95d9c0,0x95d9c4}};
    constexpr std::array<uint32_t,5> foreign{{0x95d9d0,0x95d9d4,0x95d9d8,0x95d9dc,0x95d9e0}};
    constexpr std::array<uint32_t,5> pcs{{0x57733a,0x57734e,0x577363,0x577378,0x57738d}};
    for(int tier=0;tier<4 && get(in.owner,0x95d9d0)==0;++tier) {
        for(int f=1;f<8;++f) {
            if(f==in.owner) {continue;}const auto relation=uint32_t(get(in.owner,0x95d1a8+0x18+4*f));
            if((relation&1) || (tier==0 && !(relation&16)) || (tier<=1 && (relation&2)) || (tier<=2 && !(relation&8))) {continue;}
            for(size_t i=0;i<5;++i) {put(foreign[i],std::max(get(in.owner,foreign[i]),get(f,own[i])),pcs[i]);}
        }
    }
}
}

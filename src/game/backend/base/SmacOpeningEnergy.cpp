#include "SmacOpeningEnergy.h"
#include "SmacCitizens.h"
#include "SmacBaseEnergy.h"
#include "SmacEnergyArithmetic.h"
#include "game/backend/SmacOriginalArithmetic.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend::base {
namespace {
template<class Record> int32_t Read(const Record& r,size_t off) {uint32_t v=0;for(unsigned k=0;k<4;++k) {v|=uint32_t(r.at(off+k))<<(8*k);}return SmacWrap32(v);}
int32_t Checked(int64_t v) {if(v<INT32_MIN || v>INT32_MAX) {throw std::overflow_error("Opening energy/citizens exceed measured signed32 arithmetic");}return int32_t(v);}
uint8_t Admit(const std::vector<SmacStartupBaseRecord>& bases,const std::array<faction::SmacStartupFaction,8>& actors,const SmacOpeningCitizenInputs& in) {
    if(bases.empty() || bases.size()>512 || bases[0][4]<1 || bases[0][4]>7) {throw std::invalid_argument("Opening citizens require positive Base0 owner");}
    const auto& b=bases[0];const auto owner=b[4];const auto& a=actors[owner].record;
    if(b[6]<1 || b[6]>127 || b[7] || b[8] || Read(b,0x7c)<0 || Read(b,0x7c)>b[6]
        || in.definition_drone<0 || in.definition_drone>127 || in.definition_talent<0 || in.definition_talent>127
        || Read(a,0x95d3f8-0x95d1a8) || Read(a,0x95d400-0x95d1a8) || Read(a,0x95d408-0x95d1a8)
        || Read(a,0x95e200-0x95d1a8)!=1) {throw std::invalid_argument("Citizens require the settled neutral first-Base profile");}
    constexpr std::array<uint8_t,5> masks{{0x12,1,0,0,0}};
    for(size_t i=0;i<5;++i) {if(b[0x8c+i]&~masks[i]) {throw std::invalid_argument("Opening citizens exclude mood facilities");}}
    if(std::count_if(bases.begin(),bases.end(),[&](const auto& row){return row[4]==owner;})!=1) {throw std::invalid_argument("Opening citizens require one owned Base");}
    return owner;
}
struct Writer {
    std::vector<SmacStartupBaseRecord>& bases;SmacOpeningEnergyScratch& scratch;const SmacBaseAccountingObserver& observer;
    int32_t Base(uint32_t off,int64_t value,uint32_t pc) {const auto v=Checked(value);for(unsigned k=0;k<4;++k) {bases[0].at(off+k)=uint8_t(uint32_t(v)>>(8*k));}
        if(observer) {observer({SmacBaseAccountingStore::Space::Bases,off,4,uint32_t(v),pc});}return v;}
    void Global(uint32_t address,int32_t value,uint32_t pc) {scratch.at((address-0x9185f8)/4)=value;
        if(observer) {observer({SmacBaseAccountingStore::Space::Global,address,4,uint32_t(value),pc});}}
};
}
void CalculateSmacOpeningCitizens(std::vector<SmacStartupBaseRecord>& bases,const std::array<faction::SmacStartupFaction,8>& actors,
    SmacOpeningEnergyScratch& scratch,const SmacOpeningCitizenInputs& in,const SmacBaseAccountingObserver& observer) {
    const auto owner=Admit(bases,actors,in);const auto& b=bases[0];const int32_t population=b[6],excluded=Read(b,0x7c);
    const bool human=in.human_mask&(1u<<owner);
    const auto prefix=CalculateSmacCitizenPrefix(population,human,human?Read(actors[owner].record,8):3,in.definition_drone,in.definition_talent,0);
    Writer w{bases,scratch,observer};
    const auto get=[&](size_t off){return Read(bases[0],off);};
    const auto clamp=[&](int64_t value){return int32_t(std::clamp<int64_t>(value,0,population));};
    const auto normalize=[&](std::array<uint32_t,5> pcs){
        SmacCitizenCounts c{get(0x114),get(0x118),get(0x11c)};
        constexpr std::array<uint32_t,5> offsets{{0x114,0x11c,0x118,0x118,0x11c}};
        NormalizeSmacCitizenCounts(c,population-excluded,[&](uint8_t branch,int32_t value){w.Base(offsets[branch],value,pcs[branch]);});
    };
    const auto snapshot=[&](unsigned stage,std::array<uint32_t,3> pcs){w.Global(0x918660+4*stage,get(0x114),pcs[0]);w.Global(0x91864c+4*stage,get(0x118),pcs[1]);w.Global(0x918638+4*stage,get(0x11c),pcs[2]);};
    w.Base(0x118,prefix.after_faction,0x4ffe01);w.Base(0x114,prefix.faction_talents,0x4ffe10);w.Base(0x114,prefix.faction_talents,0x4ffe2f);
    w.Base(0x118,clamp(get(0x118)),0x4ffe5d);w.Base(0x11c,clamp(int64_t(get(0x118))-population),0x4ffe85);w.Base(0x118,clamp(get(0x118)),0x4ffead);
    normalize({{0x4ffee5,0x4ffefc,0x4fff0f,0x4fff1c,0x4fff6b}});snapshot(0,{{0x4fff7d,0x4fff89,0x4fff94}});
    w.Base(0x114,int64_t(get(0x114))+get(0x100)/2,0x4fffad);w.Base(0x118,clamp(get(0x118)),0x4fffd5);w.Base(0x114,clamp(get(0x114)),0x4ffffd);
    normalize({{0x500034,0x50004a,0x50005d,0x50006a,0x5000b8}});snapshot(1,{{0x5000c9,0x5000d5,0x5000e5}});
    w.Base(0x118,get(0x118),0x5000f3);w.Base(0x114,get(0x114),0x50010a);w.Base(0x118,clamp(get(0x118)),0x500132);w.Base(0x114,clamp(get(0x114)),0x50015a);
    normalize({{0x500192,0x5001a9,0x5001bc,0x5001c9,0x500218}});snapshot(2,{{0x50022c,0x500238,0x500243}});
    w.Base(0x118,clamp(get(0x118)),0x500437);w.Base(0x114,clamp(get(0x114)),0x50045f);
    normalize({{0x500496,0x5004ac,0x5004bf,0x5004cc,0x50051a}});snapshot(3,{{0x50052b,0x500537,0x500547}});
    w.Base(0x114,get(0x114),0x500555);w.Base(0x118,clamp(get(0x118)),0x5005fd);w.Base(0x114,clamp(get(0x114)),0x500625);
    normalize({{0x50065d,0x500674,0x500687,0x500694,0x5006e3}});snapshot(4,{{0x5006f9,0x500704,0x500712}});
}
void AccountSmacOpeningEnergy(std::vector<SmacStartupBaseRecord>& bases,const std::array<faction::SmacStartupFaction,8>& actors,
    SmacOpeningEnergyScratch& scratch,const SmacOpeningEnergyInputs& in,const SmacBaseAccountingObserver& observer) {
    const auto owner=Admit(bases,actors,in.citizens);const auto& a=actors[owner].record;
    if(!(bases[0][0x8c]&2) || Read(bases[0],0x7c)) {throw std::invalid_argument("Opening energy requires HQ without specialists");}
    for(size_t f=1;f<8;++f) {if(f!=owner && ((Read(a,0x18+4*f)&2) || (Read(actors[f].record,0x18+4*owner)&2))) {throw std::invalid_argument("Opening energy excludes trade-eligible contacts");}}
    const SmacEnergyRates rates{Read(a,0x95d4cc-0x95d1a8),Read(a,0x95d4d0-0x95d1a8)};rates.Validate();
    Writer w{bases,scratch,observer};const auto get=[&](size_t off){return Read(bases[0],off);};
    const auto gross=w.Base(0xc4,int64_t(get(0xc4))+in.incoming,0x50089e);w.Base(0xf4,in.outgoing,0x5008b2);w.Base(0xe4,0,0x5008de);
    const auto net=w.Base(0xd4,int64_t(gross)-in.outgoing,0x500901);
    for(unsigned i=0;i<7;++i) {w.Global(0x918618+4*i,0,0x500981);w.Global(0x9185f8+4*i,0,0x500987);}
    w.Base(0xe4,0,0x500b8d);w.Base(0xd4,net,0x500bae);
    const auto allocation=AllocateSmacEnergy(net,rates,{}).allocation;
    const auto psych=w.Base(0x100,allocation.psych,0x500c20);auto econ=w.Base(0xfc,allocation.economy,0x500c72);auto labs=w.Base(0x104,allocation.labs,0x500c91);
    const auto energy=std::clamp(net,0,9999);const auto shadow=w.Base(0x108,std::clamp((energy*(9-rates.psych-rates.labs)+4)/10,0,energy-psych),0x500ce1);
    labs=w.Base(0x104,int64_t(labs)+in.flat_labs,0x500e73);
    w.Base(0x100,MultiplySmacEnergyQuarters(psych,4,Checked),0x5011bf);labs=w.Base(0x104,MultiplySmacEnergyQuarters(labs,(bases[0][0x8d]&1)?6:4,Checked),0x5011e0);
    econ=w.Base(0xfc,MultiplySmacEnergyQuarters(econ,4,Checked),0x501244);w.Base(0x108,MultiplySmacEnergyQuarters(shadow,4,Checked),0x5012bd);
    const auto imbalance=std::abs(rates.psych+2*rates.labs-10)/2,coefficient=8*imbalance;
    const auto reduce=[&](int32_t value,int32_t percent){return ReduceSmacEnergyPercent(value,percent,Checked);};
    if(imbalance) {
        if(rates.labs>10-rates.psych-rates.labs) {w.Base(0x104,reduce(labs,std::min(coefficient,40)),0x501453);w.Base(0xfc,reduce(econ,std::min(2*coefficient,80)),0x501491);}
        else {w.Base(0x104,reduce(labs,std::min(2*coefficient,80)),0x5014de);w.Base(0xfc,reduce(econ,std::min(coefficient,40)),0x50151c);}
    }
    if(get(0x50)==-38 && get(0x44)==-38 && get(0xd0)>0) {w.Base(0xfc,int64_t(get(0xfc))+(int64_t(get(0xd0))+1)/2,0x50154f);}
    CalculateSmacOpeningCitizens(bases,actors,scratch,in.citizens,observer);
}
}

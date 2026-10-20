#include "SmacBaseAccounting.h"
#include "SmacNutrientStorage.h"
#include "game/backend/SmacOriginalArithmetic.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend::base {
namespace {
template<class Record> uint32_t Read(const Record& r,size_t off) {return uint32_t(r.at(off))|(uint32_t(r.at(off+1))<<8)|(uint32_t(r.at(off+2))<<16)|(uint32_t(r.at(off+3))<<24);}
template<class Record> void Write(Record& r,size_t off,uint32_t value) {for(unsigned k=0;k<4;++k) {r.at(off+k)=uint8_t(value>>(8*k));}}
uint8_t Owner(const std::vector<SmacStartupBaseRecord>& bases,int32_t selected) {
    if(bases.empty() || bases.size()>512 || selected<0 || size_t(selected)>=bases.size()) {throw std::invalid_argument("Accounting requires a valid selected Base");}
    const auto owner=bases[selected][4];if(owner<1 || owner>7) {throw std::invalid_argument("Accounting requires a positive original Base owner");}return owner;
}
}
int32_t AccountSmacBaseNutrients(std::vector<SmacStartupBaseRecord>& bases,std::array<faction::SmacStartupFaction,8>& actors,
    const SmacNutrientAccountingInputs& in,const SmacBaseAccountingObserver& observer) {
    const auto owner=Owner(bases,in.selected_base);
    if(in.facility_base<0 || size_t(in.facility_base)>=bases.size()) {throw std::invalid_argument("Accounting requires a separate valid facility Base context");}
    auto& base=bases[in.selected_base];auto& actor=actors[owner].record;
    const auto emit=[&](SmacBaseAccountingStore::Space space,uint32_t offset,int32_t value,uint32_t pc) {if(observer) {observer({space,offset,4,uint32_t(value),pc});}return value;};
    const auto put=[&](size_t off,int64_t value,uint32_t pc) {const auto v=SmacWrap32(value);Write(base,off,uint32_t(v));return emit(SmacBaseAccountingStore::Space::Bases,uint32_t(304*in.selected_base+off),v,pc);};
    auto growth=emit(SmacBaseAccountingStore::Space::Global,0x9185f0,SmacWrap32(Read(actor,0x95d40c-0x95d1a8)),0x4ff0da);
    if(bases[in.facility_base][0x8c]&4) {growth=emit(SmacBaseAccountingStore::Space::Global,0x9185f0,SmacWrap32(int64_t(growth)+2),0x4ff10b);}
    if(base[0x30]&4) {growth=emit(SmacBaseAccountingStore::Space::Global,0x9185f0,SmacWrap32(int64_t(growth)+2),0x4ff11d);}
    const auto intake=put(0xbc,int64_t(SmacWrap32(Read(base,0xbc)))+in.incoming,0x4ff132);
    const int32_t population=base[6]<128?int32_t(base[6]):int32_t(base[6])-256;
    const auto consumption=put(0xec,int64_t(population)*in.food_per_citizen+in.outgoing,0x4ff150);
    const auto surplus=put(0xcc,int64_t(intake)-consumption,0x4ff169);const auto stored=SmacWrap32(Read(base,0x3c));
    const auto normalized=NormalizeSmacNutrientStorage(surplus,stored);
    if(normalized!=stored) {put(0x3c,normalized,surplus>=0?0x4ff185:0x4ff192);}
    if(in.summary_mode==1) {constexpr size_t off=0x95d9a4-0x95d1a8;
        const auto total=SmacWrap32(int64_t(SmacWrap32(Read(actor,off)))+std::clamp(surplus,0,99));Write(actor,off,uint32_t(total));
        emit(SmacBaseAccountingStore::Space::Factions,uint32_t(owner*8380+off),total,surplus>99?0x4ff1c4:0x4ff1d1);}
    return growth;
}
void PrepareSmacOpeningSupport(const std::vector<SmacStartupBaseRecord>& bases,int32_t selected,
    const std::vector<unit::OriginalUnitBytes>& units,SmacOpeningSupportScratch& scratch,const SmacBaseAccountingObserver& observer) {
    const auto owner=Owner(bases,selected);if(units.size()>2048) {throw std::invalid_argument("Opening support exceeds original Unit pool");}
    for(const auto& unit:units) {if(unit[14]==owner) {throw std::invalid_argument("Owned Units require the complete support/convoy traversal");}}
    constexpr std::array<uint32_t,12> addresses{{0x918680,0x918690,0x918684,0x918694,0x918688,0x918698,0x91868c,0x91869c,0x9186a0,0x9186a4,0x9186a8,0x91867c}};
    constexpr std::array<uint32_t,12> pcs{{0x4feb3a,0x4feb40,0x4feb46,0x4feb4c,0x4feb54,0x4feb5a,0x4feb65,0x4feb6b,0x4feb71,0x4feb77,0x4feb7d,0x4feb83}};
    for(size_t i=0;i<12;++i) {scratch[(addresses[i]-0x91867c)/4]=0;if(observer) {observer({SmacBaseAccountingStore::Space::Global,addresses[i],4,0,pcs[i]});}}
}
}

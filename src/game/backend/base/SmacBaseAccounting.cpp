#include "SmacBaseAccounting.h"
#include "SmacNutrientStorage.h"
#include "SmacMineralArithmetic.h"
#include "SmacEcologyOriginal.h"
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
void AccountSmacBaseMinerals(std::vector<SmacStartupBaseRecord>& bases,const std::array<faction::SmacStartupFaction,8>& actors,
    const SmacMineralAccountingInputs& in,const SmacBaseAccountingObserver& observer) {
    const auto owner=Owner(bases,in.selected_base);
    if(in.facility_base<0 || size_t(in.facility_base)>=bases.size()) {throw std::invalid_argument("Mineral accounting requires an indexed facility Base");}
    auto& base=bases[in.selected_base];const auto& actor=actors[owner].record;
    const auto field=[&](size_t address){return SmacWrap32(Read(actor,address-0x95d1a8));};
    const auto facility=[&](unsigned id){return bool(bases[in.facility_base][0x8c+id/8]&(1u<<(id&7)));};
    const auto project=[&](int32_t host){if(host<0) {return false;}if(size_t(host)>=bases.size()) {throw std::invalid_argument("Mineral accounting project host outside Base pool");}return bases[host][4]==owner;};
    const bool singularity=project(in.projects.singularity),bulk=project(in.projects.bulk),pholus=project(in.projects.pholus),elevator=project(in.projects.elevator);
    const auto put=[&](size_t off,int64_t value,uint32_t pc){const auto v=SmacWrap32(value);Write(base,off,uint32_t(v));
        if(observer) {observer({SmacBaseAccountingStore::Space::Bases,uint32_t(304*in.selected_base+off),4,uint32_t(v),pc});}return v;};
    const auto incoming=put(0xc0,int64_t(SmacWrap32(Read(base,0xc0)))+in.incoming,0x4ff201);
    const int steps=int(facility(21)||singularity)+int(facility(19))+int(facility(22))+int(facility(20))+int(bulk);
    const auto demand=SmacWrap32(int64_t(in.support)+in.outgoing);
    const auto processed=CalculateSmacMineralAmounts(incoming,demand,steps,[](int64_t v){return SmacWrap32(v);});
    auto gross=put(0xc0,processed.intake,0x4ff2cc);put(0xf0,demand,0x4ff2e5);
    put(0xd0,processed.surplus,0x4ff2fe);put(0xe0,0,0x4ff309);put(0xd0,processed.surplus,0x4ff326);put(0x94,processed.surplus,0x4ff337);
    const int32_t population=base[6]<128?int32_t(base[6]):int32_t(base[6])-256;
    const auto burden=CalculateOriginalSmacEcologyBurden({field(0x95d608),SmacWrap32(Read(base,0x48)),gross,population,field(0x95d9b0),
        facility(20),facility(31),facility(32),singularity,pholus});
    put(0x48,burden.scaled_terraforming,0x4ff348);if(burden.scaled_terraforming>0) {put(0x48,burden.residual_terraforming,0x4ff380);}
    auto current=put(0x48,burden.burden,0x4ff4bb);const bool human=in.human_mask&(1u<<owner);
    const bool voice=in.projects.voice!=-1,transcendence=in.primary_rules&0x800,perihelion=in.secondary_rules&0x100000;
    const auto eco=CalculateOriginalSmacEcologyDamage(burden,{human,field(0x95d1b0),in.global_difficulty,field(0x95d380),in.sea_level,in.sea_rise_rule,
        in.native_life,field(0x95d504),field(0x95d510),field(0x95d410),voice,transcendence,perihelion});
    if(human) {current=put(0x48,eco.after_human_term,0x4ff51b);}if(current<0) {current=put(0x48,0,0x4ff52b);}
    if(voice && transcendence) {current=put(0x48,2*int64_t(current),0x4ff550);}if(perihelion) {put(0x48,2*int64_t(current),0x4ff56a);}
    put(0x48,eco.after_division_by_six,0x4ff61a);put(0x48,eco.damage,0x4ff637);
    const auto target=SmacWrap32(Read(base,0x50));if(elevator && target>=-37 && target<=-34) {
        gross=put(0xc0,2*int64_t(gross),0x4ff684);put(0xd0,int64_t(gross)-demand,0x4ff69d);}
}
}

#include "SmacBaseAccounting.h"
#include "fixtures/SmacBaseAccounting.h"
#include "task/gsetests/GSETests.h"
#include "util/SHA256.h"
#include <algorithm>

namespace gse::tests {
namespace {
template<class Record> std::string Bytes(const Record& r) {return std::string(reinterpret_cast<const char*>(r.data()),r.size());}
template<class Records> std::string PoolBytes(const Records& rows) {std::string s;for(const auto& row:rows) {s+=Bytes(row);}return s;}
std::string ActorBytes(const std::array<game::backend::faction::SmacStartupFaction,8>& actors) {std::string s;for(const auto& actor:actors) {s+=Bytes(actor.record);}return s;}
template<class Record> void Word(Record& r,size_t off,uint32_t value) {for(unsigned k=0;k<4;++k) {r[off+k]=uint8_t(value>>(8*k));}}
void Append(std::string& s,uint32_t v,unsigned size) {for(unsigned k=0;k<size;++k) {s+=char(uint8_t(v>>(8*k)));}}
void Trace(std::string& s,const game::backend::base::SmacBaseAccountingStore& v) {Append(s,uint8_t(v.space),1);Append(s,v.offset,4);Append(s,v.size,1);Append(s,v.value,4);Append(s,v.caller,4);}
}
void AddSmacBaseAccountingTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC complete nutrient accounting matches one hundred ninety two original record and store controls",GT(){
        using namespace game::backend;using namespace game::backend::base;size_t total=0;
        for(const auto& c:fixtures::base_accounting::Nutrients()) {
            std::vector<SmacStartupBaseRecord> bases(3);std::array<faction::SmacStartupFaction,8> actors;
            for(size_t b=0;b<3;++b) {for(size_t i=0;i<304;++i) {bases[b][i]=uint8_t((b*304+i)*29+7);}bases[b][4]=uint8_t(c.owner);bases[b][0x8c]&=251;bases[b][0x30]&=251;}
            for(size_t f=0;f<8;++f) {for(size_t i=0;i<8380;++i) {actors[f].record[i]=uint8_t((f*8380+i)*13+3);}}
            auto& selected=bases[c.inputs.selected_base];selected[6]=uint8_t(c.population);Word(selected,0xbc,uint32_t(c.intake));Word(selected,0xec,0x13572468);
            Word(selected,0xcc,uint32_t(-12345));Word(selected,0x3c,uint32_t(c.stored));
            if(c.creche) {bases[c.inputs.facility_base][0x8c]|=4;}if(c.golden) {selected[0x30]|=4;}
            if(c.other_creche) {bases[(c.inputs.facility_base+1)%3][0x8c]|=4;}if(c.other_golden) {bases[(c.inputs.selected_base+1)%3][0x30]|=4;}
            Word(actors[c.owner].record,0x95d40c-0x95d1a8,uint32_t(c.growth));Word(actors[c.owner].record,0x95d9a4-0x95d1a8,uint32_t(c.aggregate));
            GT_ASSERT(util::SHA256(PoolBytes(bases))==c.input_bases && util::SHA256(ActorBytes(actors))==c.input_actors,"Original nutrient input recipe differs");
            std::string trace;size_t count=0;const auto growth=AccountSmacBaseNutrients(bases,actors,c.inputs,[&](const auto& s){Trace(trace,s);++count;});
            GT_ASSERT(growth==c.output_growth && util::SHA256(PoolBytes(bases))==c.bases && util::SHA256(ActorBytes(actors))==c.actors
                && count==c.count && util::SHA256(trace)==c.stores,"Original full nutrient output or journal differs");total+=count;
        }
        GT_ASSERT(total==986,"Original nutrient coverage changed");return {};
    });
    task->AddTest("SMAC opening support matches thirty original foreign pool and scratch store controls",GT(){
        using namespace game::backend;using namespace game::backend::base;size_t total=0;
        constexpr std::array<uint32_t,12> order{{0x918680,0x918690,0x918684,0x918694,0x918688,0x918698,0x91868c,0x91869c,0x9186a0,0x9186a4,0x9186a8,0x91867c}};
        for(const auto& c:fixtures::base_accounting::Supports()) {
            std::vector<SmacStartupBaseRecord> bases(2);std::array<faction::SmacStartupFaction,8> actors;
            std::vector<unit::OriginalUnitBytes> units(c.count);
            for(size_t b=0;b<2;++b) {for(size_t i=0;i<304;++i) {bases[b][i]=uint8_t((b*304+i)*7+3);}}bases[c.selected][4]=uint8_t(c.owner);
            for(size_t f=0;f<8;++f) {for(size_t i=0;i<8380;++i) {actors[f].record[i]=uint8_t((f*8380+i)*17+11);}}
            for(size_t u=0;u<units.size();++u) {for(size_t i=0;i<52;++i) {units[u][i]=uint8_t((u*52+i)*19+5);}const auto owner=(c.owner+1+int(u))%8;units[u][14]=uint8_t(owner==c.owner?0:owner);}
            GT_ASSERT(util::SHA256(PoolBytes(bases))==c.bases && util::SHA256(ActorBytes(actors))==c.actors && util::SHA256(PoolBytes(units))==c.units,"Original opening support input recipe differs");
            SmacOpeningSupportScratch scratch;for(size_t i=0;i<12;++i) {scratch[(order[i]-0x91867c)/4]=int32_t(0x12340000+i);}
            std::string trace;size_t count=0;PrepareSmacOpeningSupport(bases,c.selected,units,scratch,[&](const auto& s){Trace(trace,s);++count;});
            GT_ASSERT(std::all_of(scratch.begin(),scratch.end(),[](auto v){return v==0;}) && util::SHA256(PoolBytes(bases))==c.bases
                && util::SHA256(ActorBytes(actors))==c.actors && util::SHA256(PoolBytes(units))==c.units && count==c.writes && util::SHA256(trace)==c.stores,"Original complete opening support output differs");total+=count;
        }
        GT_ASSERT(total==360,"Original opening support coverage changed");return {};
    });
}
}

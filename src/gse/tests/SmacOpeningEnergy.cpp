#include "SmacOpeningEnergy.h"
#include "fixtures/SmacOpeningEnergy.h"
#include "fixtures/SmacRelocatedOpeningEnergy.h"
#include "task/gsetests/GSETests.h"
#include "util/SHA256.h"
#include <algorithm>

namespace gse::tests {
namespace {
template<class Record> void Word(Record& record,size_t off,uint32_t v,unsigned size=4) {for(unsigned k=0;k<size;++k) {record[off+k]=uint8_t(v>>(8*k));}}
template<class Record> std::string Bytes(const Record& row) {return std::string(reinterpret_cast<const char*>(row.data()),row.size());}
std::string Bases(const std::vector<game::backend::base::SmacStartupBaseRecord>& pool) {std::string s;for(const auto& row:pool) {s+=Bytes(row);}return s;}
std::string Actors(const std::array<game::backend::faction::SmacStartupFaction,8>& pool) {std::string s;for(const auto& row:pool) {s+=Bytes(row.record);}return s;}
void Append(std::string& s,uint32_t v,unsigned size) {for(unsigned k=0;k<size;++k) {s+=char(uint8_t(v>>(8*k)));}}
}
void AddSmacOpeningEnergyTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC opening energy and citizens match complete original records scratch and ordered stores",GT(){
        using namespace game::backend::base;size_t citizens=0,energy=0;
        auto cases=fixtures::opening_energy::Cases();const auto relocated=fixtures::opening_energy::RelocatedCases();
        cases.insert(cases.end(),relocated.begin(),relocated.end());
        for(const auto& c:cases) {
            std::vector<SmacStartupBaseRecord> bases(3);std::array<game::backend::faction::SmacStartupFaction,8> actors;
            for(size_t b=0;b<3;++b) {for(size_t i=0;i<304;++i) {bases[b][i]=uint8_t((304*b+i)*7+5);}}
            for(size_t f=0;f<8;++f) {for(size_t i=0;i<8380;++i) {actors[f].record[i]=uint8_t((f*8380+i)*13+7);}}
            auto& base=bases[0];Word(base,0,16,2);Word(base,2,12,2);base[4]=uint8_t(c.owner);base[6]=uint8_t(c.population);base[7]=base[8]=0;
            std::fill_n(base.begin()+0x8c,5,0);for(const auto id:c.facilities) {base[0x8c+id/8]|=uint8_t(1u<<(id&7));}
            Word(base,0x100,uint32_t(c.psych));Word(base,0x7c,uint32_t(c.specialists));Word(base,0x114,12345);Word(base,0x118,uint32_t(-6789));Word(base,0x11c,4242);
            auto& actor=actors[c.owner].record;Word(actor,8,uint32_t(c.difficulty));
            for(const auto address:{0x95d3f8,0x95d400,0x95d408}) {Word(actor,address-0x95d1a8,0);}Word(actor,0x95e200-0x95d1a8,1);
            SmacOpeningEnergyScratch scratch;
            for(size_t i=0;i<scratch.size();++i) {scratch[i]=c.energy?int32_t(0x24680000+4*i):0x13572468;}
            if(c.energy) {
                for(size_t i=1;i<3;++i) {bases[i][4]=uint8_t(c.owner%7+1);}
                for(auto& a:actors) {for(size_t i=0;i<8;++i) {Word(a.record,0x18+4*i,0);}}
                Word(actor,0x95d4cc-0x95d1a8,uint32_t(c.psych_rate));Word(actor,0x95d4d0-0x95d1a8,uint32_t(c.labs_rate));
                constexpr std::array<uint32_t,10> offsets{{0xc4,0xe4,0xf4,0xd4,0xfc,0x104,0x108,0x50,0x44,0xd0}};
                const std::array<int32_t,10> values{{c.intake,0x13572468,12345,-23456,34567,-45678,56789,c.target,c.previous_target,c.mineral_surplus}};
                for(size_t i=0;i<10;++i) {Word(base,offsets[i],uint32_t(values[i]));}
            }
            if(c.inputs.citizens.selected_base) {
                const auto selected=bases[0];bases.resize(7);
                for(size_t i=0;i<bases.size();++i) {
                    for(size_t j=0;j<304;++j) {bases[i][j]=uint8_t((304*i+j)*7+5);}
                    bases[i][4]=uint8_t(c.owner%7+1);Word(bases[i],0,uint32_t(2+2*i),2);Word(bases[i],2,uint32_t(2+2*i),2);
                    std::fill_n(bases[i].begin()+0x8c,5,0);
                }
                bases.at(c.inputs.citizens.selected_base)=selected;
            }
            GT_ASSERT(util::SHA256(Bases(bases))==c.input_bases && util::SHA256(Actors(actors))==c.input_actors,"Opening energy/citizen input recipe differs");
            const auto before=actors;std::string trace;size_t count=0;
            const SmacBaseAccountingObserver observer=[&](const auto& s){Append(trace,uint8_t(s.space),1);Append(trace,s.offset,4);Append(trace,s.size,1);Append(trace,s.value,4);Append(trace,s.caller,4);++count;};
            if(c.inputs.citizens.selected_base==6 && c.owner==7 && c.population==7 && c.facilities.size()==3) {
                const auto untouched=bases;const auto untouched_scratch=scratch;
                for(const auto index:{-1,7,INT32_MAX}) {
                    auto invalid=c.inputs;invalid.citizens.selected_base=index;
                    bool rejected=false;
                    try {AccountSmacOpeningEnergy(bases,actors,scratch,invalid,observer);}catch(const std::invalid_argument&) {rejected=true;}
                    GT_ASSERT(rejected && bases==untouched && scratch==untouched_scratch && count==0,"Invalid Base context must reject before any write");
                    rejected=false;
                    try {CalculateSmacOpeningCitizens(bases,actors,scratch,invalid.citizens,observer);}catch(const std::invalid_argument&) {rejected=true;}
                    GT_ASSERT(rejected && bases==untouched && scratch==untouched_scratch && count==0,"Invalid citizen context must reject before any write");
                }
            }
            if(c.energy) {AccountSmacOpeningEnergy(bases,actors,scratch,c.inputs,observer);energy+=count;}
            else {CalculateSmacOpeningCitizens(bases,actors,scratch,c.inputs.citizens,observer);citizens+=count;}
            std::string scratch_bytes;for(const auto v:scratch) {Append(scratch_bytes,uint32_t(v),4);}
            GT_ASSERT(util::SHA256(Bases(bases))==c.bases && util::SHA256(Actors(actors))==c.actors && util::SHA256(scratch_bytes)==c.scratch
                && count==c.count && util::SHA256(trace)==c.trace,"Original complete opening energy/citizen output or store order differs");
            for(size_t f=0;f<8;++f) {GT_ASSERT(actors[f].scratch==before[f].scratch,"Energy/citizens touched startup scratch");}
        }
        GT_ASSERT(citizens==4510 && energy==18849+5208,"Opening energy/citizens coverage changed");return {};
    });
}
}

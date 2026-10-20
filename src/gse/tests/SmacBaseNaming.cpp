#include "SmacBaseNaming.h"
#include "game/backend/base/SmacBaseNaming.h"
#include "fixtures/SmacBaseNaming.h"
#include "task/gsetests/GSETests.h"
#include "util/SHA256.h"
#include <algorithm>

namespace gse::tests {
namespace {
void Word(game::backend::faction::SmacStartupFaction& a,size_t off,int32_t value) {for(unsigned k=0;k<4;++k) {a.record[off+k]=uint8_t(uint32_t(value)>>(8*k));}}
void Append(std::string& s,uint32_t value,unsigned size) {for(unsigned k=0;k<size;++k) {s+=char(uint8_t(value>>(8*k)));}}
}
void AddSmacBaseNamingTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC automatic Base naming matches native selection and complete cursor records",GT(){
        using namespace game::backend;size_t total=0;
        for(const auto& c:fixtures::base_naming::Cases()) {
            std::array<faction::SmacStartupFaction,8> actors{};auto& actor=actors[c.owner];Word(actor,12,c.land);Word(actor,16,c.sea_cursor);
            const auto counts=fixtures::base_naming::COUNTS[c.owner-1];base::SmacBaseNameCatalog catalog;catalog.noun="Faction";
            for(int i=0;i<counts.land;++i) {catalog.land.push_back("Land "+std::to_string(i));}
            for(int i=0;i<counts.sea;++i) {catalog.sea.push_back("Sea "+std::to_string(i));}
            for(int i=0;i<counts.generic;++i) {catalog.generic.push_back("Generic "+std::to_string(i));}
            const auto expected=c.kind==0?"Faction "+std::to_string(c.land+1):c.kind==1?"Land "+std::to_string(c.index):c.kind==2?"Sea "+std::to_string(c.index):"Generic "+std::to_string(c.index);
            std::array<uint8_t,128> output;output.fill(165);std::string trace;size_t count=0;
            const auto label=base::GenerateSmacBaseName(actor,output,catalog,c.owner,c.pattern,c.advance,c.sea,[&](const auto& v){Append(trace,v.offset,4);Append(trace,4,1);Append(trace,v.value,4);Append(trace,v.caller,4);++count;});
            GT_ASSERT(label==expected,"Automatic Base naming selected a different native source entry");
            for(size_t i=0;i<output.size();++i) {
                const auto value=i<expected.size()?uint8_t(expected[i]):i==expected.size() || (c.kind!=0 && i<24)?uint8_t(0):uint8_t(165);
                GT_ASSERT(output[i]==value,"Automatic Base naming symbolic buffer layout differs");
            }
            std::string bytes;for(const auto& a:actors) {bytes.append(reinterpret_cast<const char*>(a.record.data()),a.record.size());}
            GT_ASSERT(util::SHA256(bytes)==c.actors && count==c.count && util::SHA256(trace)==c.stores,"Automatic naming native faction pool or cursor stores differ");total+=count;
        }
        GT_ASSERT(total==694,"Automatic naming coverage changed");return {};
    });
    task->AddTest("SMAC automatic Base naming retains fixed copy truncation and fallback tail",GT(){
        using namespace game::backend;faction::SmacStartupFaction actor{};std::array<uint8_t,128> output;output.fill(165);
        base::SmacBaseNameCatalog catalog{"Faction",{"abcdefghijklmnopqrstuvwxyz"},{},{}};
        const auto label=base::GenerateSmacBaseName(actor,output,catalog,1,11446,false,false);
        GT_ASSERT(label=="abcdefghijklmnopqrstuvw" && output[23]==0 && output[24]==165 && actor.record[12]==0,"Native fixed24 copy or preview changed");
        Word(actor,12,1);output.fill(165);const auto fallback=base::GenerateSmacBaseName(actor,output,catalog,1,11446,true,false);
        GT_ASSERT(fallback=="Faction 2" && actor.record[12]==1 && output[9]==0 && output[10]==165,"Exhausted fallback padded or advanced the cursor");return {};
    });
}
}

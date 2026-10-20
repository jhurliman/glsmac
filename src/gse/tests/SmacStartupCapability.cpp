#include "SmacStartupCapability.h"
#include "fixtures/SmacStartupCapability.h"
#include "task/gsetests/GSETests.h"
#include "util/SHA256.h"

namespace gse::tests {
void AddSmacStartupCapabilityTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC startup capability refresh matches complete original records and store order",GT(){
        using namespace game::backend::faction;size_t total=0;
        const auto word=[](auto& r,size_t off,uint32_t v){for(unsigned k=0;k<4;++k) {r[off+k]=uint8_t(v>>(8*k));}};
        const auto bytes=[](const auto& actors){std::string s;for(const auto& a:actors) {s.append(reinterpret_cast<const char*>(a.record.data()),a.record.size());}return s;};
        const auto append=[](std::string& s,uint32_t v,unsigned size){for(unsigned k=0;k<size;++k) {s+=char(uint8_t(v>>(8*k)));}};
        for(const auto& c:fixtures::capability::Cases()) {
            std::array<SmacStartupFaction,8> actors;
            for(size_t f=0;f<8;++f) {
                for(size_t i=0;i<8380;++i) {actors[f].record[i]=uint8_t((f*8380+i)*37+11);}
                for(size_t i=0;i<8;++i) {word(actors[f].record,0x18+4*i,0);}
                for(size_t i=0;i<10;++i) {word(actors[f].record,0x95d9bc-0x95d1a8+4*i,0);}
            }
            word(actors[c.options.owner].record,0x95d9d0-0x95d1a8,uint32_t(c.previous[0]));
            word(actors[c.options.owner].record,0x95d9d4-0x95d1a8,uint32_t(c.previous[1]));
            constexpr std::array<uint32_t,5> addresses{{0x95d9bc,0x95d9c8,0x95d9cc,0x95d9c0,0x95d9c4}};
            for(const auto& p:c.peers) {
                for(size_t i=0;i<5;++i) {word(actors[p.owner].record,addresses[i]-0x95d1a8,uint32_t(p.values[i]));}
                word(actors[c.options.owner].record,0x18+4*p.owner,p.relation);
            }
            const auto before=actors;GT_ASSERT(util::SHA256(bytes(actors))==c.input,"Original capability input recipe differs");
            std::string trace;size_t count=0;
            RefreshSmacStartupCapabilities(actors,c.ownership,c.prerequisites,c.catalog,c.options,[&](const auto& v){
                append(trace,v.offset,4);append(trace,v.size,1);append(trace,v.value,4);append(trace,v.caller,4);++count;});
            GT_ASSERT(util::SHA256(bytes(actors))==c.output && count==c.count && util::SHA256(trace)==c.trace,"Original capability record or ordered stores differ");
            for(size_t f=0;f<8;++f) {GT_ASSERT(actors[f].scratch==before[f].scratch,"Capability refresh touched startup scratch");}
            total+=count;
        }
        GT_ASSERT(total==3350,"Capability original coverage changed");return {};
    });
}
}

#include "SmacOpeningDesigns.h"
#include "game/backend/unit/SmacOpeningDesigns.h"
#include "fixtures/SmacOpeningDesigns.h"
#include "fixtures/SmacStartupCapability.h"
#include "task/gsetests/GSETests.h"
#include "util/SHA256.h"

namespace gse::tests {
namespace {
using namespace game::backend::unit;
template<class Record> void Word(Record& record,size_t offset,uint32_t value,unsigned size=4) {for(unsigned k=0;k<size;++k) {record[offset+k]=uint8_t(value>>(8*k));}}
SmacOriginalDesignPool Input(int owner,int poison,int prerequisite) {
    SmacOriginalDesignPool pool{};
    for(size_t i=0;i<3;++i) {
        pool[i][36]=0;pool[i][37]=uint8_t(i==0?8:i==1?9:0);pool[i][38]=0;pool[i][39]=1;
        pool[i][41]=uint8_t(i==0?3:i==1?2:1);pool[i][42]=uint8_t(i==0?8:i==1?9:3);
        Word(pool[i],48,1,2);Word(pool[i],50,uint32_t(i==1?6:-1),2);
    }
    for(size_t i=64*owner;i<size_t(64*owner+64);++i) {pool[i][47]=uint8_t(poison);Word(pool[i],50,uint32_t(prerequisite),2);}
    return pool;
}
std::string Pool(const SmacOriginalDesignPool& pool) {std::string s;for(const auto& d:pool) {s.append(reinterpret_cast<const char*>(d.data()),d.size());}return s;}
void Append(std::string& s,uint32_t value,unsigned size) {for(unsigned k=0;k<size;++k) {s+=char(uint8_t(value>>(8*k)));}}
void Trace(std::string& s,const SmacDesignStore& v,bool space) {if(space) {Append(s,uint8_t(v.space),1);}Append(s,v.offset,4);Append(s,v.size,1);Append(s,v.value,4);Append(s,v.caller,4);}
}
void AddSmacOpeningDesignTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC fresh opening design constructor matches thirty six complete original pools",GT(){
        size_t total=0;
        for(const auto& c:fixtures::opening_designs::FreshCases()) {
            auto pool=Input(c.owner,c.poison,c.prerequisite);int32_t first=-1;std::string trace;size_t count=0;
            const auto result=ConstructSmacOpeningDesign(pool,c.owner,c.chassis,c.armor,c.difficulty,"Probe design",first,false,[&](const auto& v){
                if(v.space==SmacDesignStore::Space::Designs) {Trace(trace,v,false);++count;}});
            GT_ASSERT(result==c.result && first==c.first && util::SHA256(Pool(pool))==c.designs && count==c.count && util::SHA256(trace)==c.trace,
                "Original fresh opening design bytes or ordered stores differ");total+=count;
        }
        GT_ASSERT(total==480,"Fresh design coverage changed");return {};
    });
    task->AddTest("SMAC opening design generation and caller selection match one hundred two original controls",GT(){
        using namespace game::backend::faction;size_t total=0,marked=0;
        for(const auto& c:fixtures::opening_designs::EarlyCases()) {
            auto pool=Input(c.owner,165,4660);std::array<SmacStartupFaction,8> actors{};SmacStartupTechnologyOwnership ownership{};
            Word(actors[c.owner].record,8,uint32_t(c.difficulty));
            for(size_t i=0;i<5;++i) {Word(actors[c.owner].record,0x95d584-0x95d1a8+4*i,uint32_t(c.personality[i]));}
            for(const auto t:c.technologies) {ownership[t]|=uint8_t(1u<<c.owner);}
            constexpr std::array<uint32_t,5> addresses{{0x95d9bc,0x95d9c8,0x95d9cc,0x95d9c0,0x95d9c4}};
            for(size_t f=1;f<8;++f) {if(f==size_t(c.owner)) {continue;}for(size_t i=0;i<5;++i) {Word(actors[f].record,addresses[i]-0x95d1a8,uint32_t(c.peers[i]));}}
            int32_t first=123;std::string trace;size_t count=0,requests=0;std::vector<std::array<int32_t,2>> components;
            GenerateSmacOpeningDesigns(actors,pool,ownership,fixtures::opening_designs::Prerequisites(),fixtures::capability::StockCatalog(),c.owner,c.difficulty,first,
                [&](int32_t owner,int32_t chassis,int32_t armor){++requests;components.push_back({{chassis,armor}});return owner==c.owner?"Probe design":"WRONG OWNER";},
                [&](const auto& v){Trace(trace,v,true);++count;});
            std::string bytes;for(const auto& actor:actors) {bytes.append(reinterpret_cast<const char*>(actor.record.data()),actor.record.size());}
            GT_ASSERT(util::SHA256(Pool(pool))==c.designs && util::SHA256(bytes)==c.actors && first==c.first && count==c.count
                && requests==c.requests && util::SHA256(trace)==c.trace,"Original opening design generation or ordered stores differ");
            if(components.size()==2) {GT_ASSERT((components[0]==std::array<int32_t,2>{{0,1}} && components[1]==std::array<int32_t,2>{{1,0}}),"Opening constructor request order differs");}
            trace.clear();size_t mark_count=0;const auto scout=MarkAndSelectSmacStartingScout(pool,c.owner,[&](const auto& v){Trace(trace,v,true);++mark_count;});
            GT_ASSERT(scout==c.scout && util::SHA256(Pool(pool))==c.marked && mark_count==c.mark_count && util::SHA256(trace)==c.mark_trace,
                "Original prototype marking or Scout selection differs");total+=count;marked+=mark_count;
        }
        GT_ASSERT(total==5268 && marked==36,"Opening design coverage changed");return {};
    });
    task->AddTest("SMAC startup Scout selector retains last active Recon",GT(){
        auto pool=Input(1,165,4660);pool[65][42]=pool[67][42]=3;Word(pool[65],48,1,2);Word(pool[67],48,5,2);
        pool[66][42]=3; // inactive Recon does not qualify
        size_t count=0;const auto scout=MarkAndSelectSmacStartingScout(pool,1,[&](const auto&){++count;});
        GT_ASSERT(scout==67 && count==2 && pool[65][48]==5 && pool[67][48]==5,"Last active Recon or repeated prototype store differs");return {};
    });
}
}

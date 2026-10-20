#include "SmacStartupBase.h"
#include "fixtures/SmacStartupBase.h"
#include "task/gsetests/GSETests.h"
#include "util/SHA256.h"

namespace gse::tests {
namespace {
template<class Record> std::string Bytes(const Record& r) {return std::string(reinterpret_cast<const char*>(r.data()),r.size());}
template<class Record> void Word(Record& r,size_t off,uint32_t value,unsigned size=4) {for(unsigned k=0;k<size;++k) {r[off+k]=uint8_t(value>>(8*k));}}
std::string TileBytes(const std::vector<std::array<uint8_t,44>>& tiles) {std::string result;for(const auto& r:tiles) {result+=Bytes(r);}return result;}
}
void AddSmacStartupBaseTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC startup Base seeds match eighty three original complete records history and terrain controls",GT(){
        using namespace game::backend::base;using game::backend::faction::SmacStartupFaction;
        for(const auto& c:fixtures::startup_base::Seeds()) {
            SmacStartupBaseRecord base;base.fill(165);std::vector<std::array<uint8_t,44>> tiles(16*24);
            for(auto& tile:tiles) {tile.fill(165);tile[0]=c.sea?32:104;tile[3]=1;tile[4]=16;}
            SmacStartupFaction state;for(size_t i=0;i<state.record.size();++i) {state.record[i]=uint8_t(i*37+19);}
            const auto set=[&](size_t address,int32_t v){Word(state.record,address-0x95d1a8,uint32_t(v));};
            set(0x95e200,c.owner_bases);set(0x95d1b0,c.stored_difficulty);set(0x95d378,0xdfbe41);
            set(0x95d418,c.industry);set(0x95d478,c.morale);set(0x95d494,c.growth);state.record[0x95e320-0x95d1a8+1]=uint8_t(c.region_count);
            GT_ASSERT(util::SHA256(Bytes(base))==c.input_base && util::SHA256(TileBytes(tiles))==c.input_tiles
                && util::SHA256(Bytes(state.record))==c.input_faction,"Original Base input recipe differs: "+std::string(c.id));
            SmacBaseSeedCatalog catalog{};for(auto& pair:catalog.prerequisites) {pair={{-1,-1}};}
            catalog.design_costs={{3,2,1}};catalog.formers_prerequisite=6;catalog.free_facility_technologies.fill(-2);catalog.facility_bonuses=c.facilities;
            if(c.ecology) {catalog.ownership[6]=uint8_t(1u<<c.options.owner);}if(c.free) {catalog.free_facility_technologies[1]=6;}
            const auto result=SeedSmacStartupBase(base,tiles,state,c.options,catalog);
            GT_ASSERT(util::SHA256(Bytes(base))==c.base && util::SHA256(TileBytes(tiles))==c.tiles && util::SHA256(Bytes(state.record))==c.faction
                && result.index==c.result && result.count==c.count,"Original complete Base seed output differs: "+std::string(c.id));
            GT_ASSERT(c.result<0?!result.history:(result.history && *result.history==c.history),"Original Base founding history differs");
        }
        return {};
    });
    task->AddTest("SMAC startup Base radius matches sixteen original all faction goal and ordered terrain controls",GT(){
        using namespace game::backend::base;size_t total=0;
        for(const auto& c:fixtures::startup_base::Radii()) {
            std::vector<std::array<uint8_t,44>> tiles(16*24);for(size_t i=0;i<tiles.size();++i) {for(size_t k=0;k<44;++k) {tiles[i][k]=uint8_t((44*i+k)*13+7);}}
            std::array<game::backend::faction::SmacStartupFaction,8> actors;
            const auto x=c.position[0],y=c.position[1];auto west=x-6;if(!c.flat && west<0) {west+=32;}
            const std::array<std::array<int32_t,3>,4> goals{{{{8,x,y}},{{8,west,y}},{{8,(west+35)%32,y<23?y+1:y-1}},{{9,west,y}}}};
            for(size_t f=0;f<8;++f) {auto& r=actors[f].record;for(size_t i=0;i<r.size();++i) {r[i]=uint8_t((f*r.size()+i)*37+19);}
                const auto goal=[&](size_t off,int kind,int priority,int gx,int gy,int aux) {
                    Word(r,off,uint32_t(kind),2);Word(r,off+2,uint32_t(priority),2);Word(r,off+4,uint32_t(gx));Word(r,off+8,uint32_t(gy));Word(r,off+12,uint32_t(aux));};
                for(size_t j=0;j<75;++j) {goal(0x95eba0-0x95d1a8+16*j,-1,7,0,0,123);}
                for(size_t j=0;j<25;++j) {goal(0x95f050-0x95d1a8+16*j,-1,7,0,0,123);}
                for(size_t j=0;j<goals.size();++j) {const auto& g=goals[j];goal(0x95eba0-0x95d1a8+16*j,g[0],7,g[1],g[2],123);goal(0x95f050-0x95d1a8+16*j,g[0],7,g[1],g[2],123);}
                goal(0x95eba0-0x95d1a8+16*5,8,19,west+1,y,789);
            }
            const auto actorBytes=[&](){std::string result;for(const auto& a:actors) {result+=Bytes(a.record);}return result;};
            GT_ASSERT(util::SHA256(TileBytes(tiles))==c.input_tiles && util::SHA256(actorBytes())==c.input_factions,"Original radius input recipe differs");
            std::string trace;size_t count=0;const auto pack=[&](uint32_t v,unsigned size){for(unsigned k=0;k<size;++k) {trace+=char(uint8_t(v>>(8*k)));}};
            const auto last=PrepareSmacStartupBaseRadius(tiles,actors,{32,24,!c.flat},c.position,c.owner,[&](const auto& s){
                pack(s.offset,4);pack(s.size,1);pack(s.value,4);pack(s.caller,4);++count;});
            GT_ASSERT(util::SHA256(TileBytes(tiles))==c.tiles && util::SHA256(actorBytes())==c.factions
                && count==c.count && util::SHA256(trace)==c.stores && last[0]==west && last[1]==y,"Original radius/goals output differs");total+=count;
        }
        GT_ASSERT(total==752,"Original radius coverage changed");return {};
    });
}
}

#include "SmacGlobalSpread.h"
#include "game/backend/map/SmacGlobalSpread.h"
#include "task/gsetests/GSETests.h"
#include <yaml-cpp/yaml.h>
#include <cstdlib>
#include <functional>

namespace gse::tests {
namespace {
namespace spread=game::backend::map::smac_spread;
using Random=game::backend::SmacGameplayRandomState;
void Put(game::backend::map::SmacPlanetMap::Tile& tile,size_t offset,uint32_t value) {
    for(size_t i=0;i<4;++i) {tile.bytes[offset+i]=uint8_t(value>>(8*i));}
}
std::vector<game::backend::map::SmacPlanetMap::Tile> Tiles(const std::string& hex) {
    const auto nibble=[](char c)->uint8_t {
        if(c>='0' && c<='9') {return c-'0';}if(c>='a' && c<='f') {return c-'a'+10;}
        throw std::runtime_error("Invalid reference hex");
    };
    if(hex.size()%88) {throw std::runtime_error("Invalid reference terrain length");}
    std::vector<game::backend::map::SmacPlanetMap::Tile> result(hex.size()/88);
    for(size_t i=0;i<hex.size()/2;++i) {result[i/44].bytes[i%44]=uint8_t((nibble(hex[2*i])<<4)|nibble(hex[2*i+1]));}return result;
}
bool Equal(const std::vector<game::backend::map::SmacPlanetMap::Tile>& a,const std::vector<game::backend::map::SmacPlanetMap::Tile>& b) {
    if(a.size()!=b.size()) {return false;}for(size_t i=0;i<a.size();++i) {if(a[i].bytes!=b[i].bytes) {return false;}}return true;
}
}
void AddSmacGlobalSpreadTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC global spread matches nine native full-map checkpoints when supplied",GT(){
        const auto* path=std::getenv("GLSMAC_GLOBAL_ECOLOGY_REFERENCE");
        if(!path || !*path) {GT_OK();} // Original bytes stay local and opt-in.
        const auto receipt=YAML::LoadFile(path);
        for(const auto& row:receipt["cases"]) {
            const auto before=row["before"];spread::Input input{};
            input.geometry={before["width"].as<int32_t>(),before["height"].as<int32_t>(),!before["map_flat"].as<int32_t>()};
            input.tile_count=before["tile_count"].as<int32_t>();input.turn=before["turn"].as<int32_t>();
            input.tiles=Tiles(before["raw_map_hex"].as<std::string>());
            // Faction status is the first DWORD of each positive owner's row.
            const auto factions=before["raw_factions_hex"].as<std::string>();
            for(size_t owner=1;owner<=7;++owner) {
                const auto offset=(owner-1)*0x20bc*2;
                uint32_t value=0;
                for(size_t k=0;k<4;++k) {value|=uint32_t(std::stoul(factions.substr(offset+2*k,2),nullptr,16))<<(8*k);}
                input.observer_status[owner]=value;
            }
            const Random initial(before["shared"].as<uint32_t>(),before["crt"].as<uint32_t>());
            const auto result=spread::Prepare(input,initial);bool checkpoint=false;size_t sample=0;
            for(const auto& event:row["events"]) {
                if(event["pc"].as<std::string>()=="0x537bef") {
                    checkpoint=true;
                    GT_ASSERT(Equal(result.tiles,Tiles(event["state"]["raw_map_hex"].as<std::string>())) && result.random.Crt()==event["state"]["crt"].as<uint32_t>(),"Native complete pre-wildlife map or CRT differs");
                    GT_ASSERT(event["draw_count"].as<size_t>()==2*result.samples.size(),"Native spread draw budget differs");
                }
                if(event["pc"].as<std::string>()=="0x537845") {
                    const auto stack=event["stack_hex"].as<std::string>();
                    const auto read=[&](size_t offset) {uint32_t value=0;for(size_t k=0;k<4;++k) {value|=uint32_t(std::stoul(stack.substr(2*(offset+k),2),nullptr,16))<<(8*k);}return value;};
                    GT_ASSERT(sample<result.samples.size() && uint32_t(result.samples[sample][0])==read(0x10) && uint32_t(result.samples[sample][1])==read(0x18),"Native ordered sample differs");++sample;
                }
            }
            GT_ASSERT(checkpoint && sample==result.samples.size() && result.random.Shared()==initial.Shared(),"Missing native spread checkpoint or shared RNG mutation");
        }GT_OK();
    });
    task->AddTest("SMAC global spread preserves source observer gates first ties and unrelated bytes",GT(){
        spread::Input input{{8,8,true},32,1,{},std::vector<game::backend::map::SmacPlanetMap::Tile>(32)};
        for(auto& tile:input.tiles) {tile.bytes[0]=96;tile.bytes[1]=87;tile.bytes[7]=6;}
        auto& source=input.tiles[3*4];Put(source,8,0x200000);source.bytes[4]=2;
        Put(source,16,0x200000);Put(source,24,0x200000);input.observer_status[3]=0x200;
        input.observer_status[2]=0x200; // Omniscience alone does not invent known forest.
        auto& target=input.tiles[2*4+1];Put(target,8,0x24);Put(target,16,0x24);Put(target,20,0x24);Put(target,24,0x24);Put(target,28,0x24);target.bytes[4]=0x10;
        const auto original=input;const auto result=spread::Prepare(input,Random(123,1));
        GT_ASSERT(result.samples==std::vector<spread::Coordinate>({{1,3}}) && result.changes.size()==1 && result.changes[0].target==spread::Coordinate({2,2}),"Parity sampling or first neighbor tie differs");
        auto expected=original.tiles;Put(expected[9],8,0x200004);Put(expected[9],16,0x200004);Put(expected[9],24,0x200004);
        GT_ASSERT(Equal(result.tiles,expected) && Equal(input.tiles,original.tiles) && result.random.Shared()==123,"Observer source gates, fungus clearing or unrelated bytes differ");
        input.tiles[0].bytes[0]=64; // Independent copies never alias result terrain.
        GT_ASSERT(result.tiles[0].bytes[0]==96,"Result terrain aliases input");GT_OK();
    });
    task->AddTest("SMAC global spread propagates newly grown forest and kelp within the same ordered pass",GT(){
        for(const bool sea:{false,true}) {
            spread::Input input{{8,16,true},64,1,{},std::vector<game::backend::map::SmacPlanetMap::Tile>(64)};
            for(auto& tile:input.tiles) {tile.bytes[0]=sea ? 64:96;}
            const uint32_t feature=sea ? 0x8000:0x200000;
            Put(input.tiles[2*4+1],8,feature);
            const auto result=spread::Prepare(input,Random(1,28));
            GT_ASSERT(result.samples==std::vector<spread::Coordinate>({{2,2},{3,1}}) && result.changes.size()==2,"Newly grown feature was not considered by the later sample");
            GT_ASSERT(result.changes[0].target==spread::Coordinate({3,1}) && result.changes[1].target==spread::Coordinate({4,0})
                && result.tiles[2].Items()==feature,"Ordered land/sea propagation or native first-neighbor choice differs");
        }GT_OK();
    });
    task->AddTest("SMAC global spread budgets consume CRT even without growth and reject malformed geometry",GT(){
        spread::Input input{{80,80,true},3200,1,{},std::vector<game::backend::map::SmacPlanetMap::Tile>(3200)};
        const Random initial(9,1537134187u);auto expected=initial;
        for(size_t i=0;i<200;++i) {expected.NextCrtRaw15();}
        auto result=spread::Prepare(input,initial);
        GT_ASSERT(result.samples.size()==100 && result.changes.empty() && result.random==expected && Equal(result.tiles,input.tiles),"Empty sources suppressed mandatory samples");
        input.turn=4;result=spread::Prepare(input,initial);expected=initial;for(size_t i=0;i<192;++i) {expected.NextCrtRaw15();}
        GT_ASSERT(result.samples.size()==96 && result.random==expected,"Turn4 budget denominator differs");
        input.turn=INT32_MAX;result=spread::Prepare(input,initial);
        GT_ASSERT(result.samples.empty() && result.random==initial,"Large nonnegative turn overflowed budget");
        const auto rejects=[](const spread::Input& value) {try {(void)spread::Prepare(value,Random(1,1));}catch(const std::invalid_argument&) {return true;}return false;};
        input.geometry.width=79;GT_ASSERT(rejects(input),"Odd-width parity terrain admitted");
        input.geometry.width=80;input.tiles.pop_back();GT_ASSERT(rejects(input),"Truncated terrain admitted");GT_OK();
    });
}
}

#include "SmacGlobalWildlife.h"
#include "game/backend/map/SmacGlobalWildlife.h"
#include "task/gsetests/GSETests.h"
#include <stdexcept>

namespace gse::tests {
namespace {
namespace wildlife=game::backend::map::smac_wildlife;
using Tile=game::backend::map::SmacPlanetMap::Tile;
using Random=game::backend::SmacGameplayRandomState;
using Coord=wildlife::Coordinate;
void Items(Tile& tile,uint32_t flags) {for(size_t i=0;i<4;++i) {tile.bytes[8+i]=uint8_t(flags>>(8*i));}}
struct World {
    wildlife::Input input{};
    std::vector<Tile> tiles=std::vector<Tile>(3200);
    std::vector<int> designs,sentries,promotions;
    int capacity=2,tile_reads=0;bool constructor_draw=false;
    World() {
        input.geometry={80,80,true};input.turn=1;input.life=1;input.difficulty=3;
        input.voice_marker=-1;input.unit_count=13;
        for(auto& tile:tiles) {tile.bytes[0]=96;tile.bytes[2]=15;tile.bytes[3]=1;}
    }
    Tile& At(Coord p) {return tiles.at(size_t(p[1])*40+size_t(p[0])/2);}
    wildlife::Hooks Hooks() {
        return {
            [&](Coord p){++tile_reads;return At(p);},[](Coord){return 0;},
            [&](int design,Coord p,Random& rng){
                if(constructor_draw) {rng.NextCrtRaw15();}
                const int id=input.unit_count+int(designs.size());designs.push_back(design);
                auto& tile=At(p);if(!(tile.Items()&3u)) {tile.bytes[2]&=0xf0;}
                Items(tile,tile.Items()|2u);return id;
            },[&](int){return capacity;},
            [&](int id,Random&){sentries.push_back(id);},[&](int id,Random&){promotions.push_back(id);}
        };
    }
};
Coord Sample(Random& rng) {int x=int(rng.NextCrtRaw15())%80;int y=int(rng.NextCrtRaw15())%80;x+=(y&1)-(x&1);return {{x,y}};}
}
void AddSmacGlobalWildlifeTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC global wildlife gates do not consume RNG or invoke factories",GT(){
        for(int gate=0;gate<3;++gate) {
            World world;if(gate==0) {world.input.rules=0x200000;}if(gate==1) {world.input.voice_marker=0;}if(gate==2) {world.input.unit_count=1792;}
            const Random initial(7,1);const auto result=wildlife::Produce(world.input,initial,{});
            GT_ASSERT(result.random==initial && result.actions.empty() && result.land_samples.empty() && result.sea_samples.empty(),"Disabled fauna changed state");
        }GT_OK();
    });
    task->AddTest("SMAC global wildlife sea shortcut retains mixed owner and clamps actual capacity",GT(){
        World world;world.input.turn=80;world.input.unit_count=1791;world.input.locust_technology_masks[1]=1;
        const Random initial(7,1);auto expected=initial;const auto point=Sample(expected);
        auto& tile=world.At(point);Items(tile,0x22);tile.bytes[2]=2; // Occupied land fungus bypasses fallback filters.
        const auto result=wildlife::Produce(world.input,initial,world.Hooks());
        GT_ASSERT(world.designs==std::vector<int>({9,8,8,10}) && world.sentries==std::vector<int>({1792,1793}) && world.promotions==std::vector<int>({1791}),"Brood capacity/order/Locust sequence differs");
        GT_ASSERT(result.unit_count==1795 && result.sea_samples==std::vector<Coord>({point}) && world.At(point).CachedOccupant()==2,"Entry count was reapplied or positive occupancy lost");
        for(int i=0;i<4;++i) {Sample(expected);}
        GT_ASSERT(result.random==expected && result.actions.size()==7,"Native action count or CRT sample budget differs");GT_OK();
    });
    task->AddTest("SMAC global wildlife consumes untested thousandth sea pair and factory RNG",GT(){
        World world;world.input.turn=8;
        const Random initial(7,1);auto expected=initial;Coord last{};for(int i=0;i<1000;++i) {last=Sample(expected);}
        const auto result=wildlife::Produce(world.input,initial,world.Hooks());
        for(int i=0;i<4;++i) {Sample(expected);}
        GT_ASSERT(result.sea_samples.size()==1000 && result.sea_samples.back()==last && result.actions.empty() && result.random==expected && world.tile_reads==1003,"Sea exhaustion budget differs");
        World staged;staged.input.turn=8;staged.constructor_draw=true;expected=initial;const auto sea=Sample(expected);Items(staged.At(sea),0x20);
        // Carrier and one early passenger each own one additional constructor draw.
        expected.NextCrtRaw15();expected.NextCrtRaw15();std::vector<Coord> land;for(int i=0;i<4;++i) {land.push_back(Sample(expected));}
        const auto made=wildlife::Produce(staged.input,initial,staged.Hooks());
        GT_ASSERT(staged.designs==std::vector<int>({9,8}) && made.land_samples==land && made.random==expected,"Constructor-owned RNG was omitted from later decisions");GT_OK();
    });
    task->AddTest("SMAC global wildlife no Base permits unseen ambush but never empty land birth",GT(){
        const Random initial(7,1);auto expected=initial;const auto source=Sample(expected);
        World world;Items(world.At(source),0x22);world.At(source).bytes[2]=2;
        const auto result=wildlife::Produce(world.input,initial,world.Hooks());
        Coord target{{(source[0]+2)%80,source[1]-2}};
        GT_ASSERT(world.designs==std::vector<int>({8}) && result.actions[0].point==target,"No-Base sentinel incorrectly blocked ambush");
        World empty;Items(empty.At(source),0x20);const auto none=wildlife::Produce(empty.input,initial,empty.Hooks());
        GT_ASSERT(none.actions.empty(),"No Base invented an empty-land distance");
        World visible;Items(visible.At(source),0x22);visible.At(source).bytes[2]=2;visible.input.observer_status[2]=0x200;
        GT_ASSERT(wildlife::Produce(visible.input,initial,visible.Hooks()).actions.empty(),"Omniscient victim admitted an unseen ambush");GT_OK();
    });
}
}

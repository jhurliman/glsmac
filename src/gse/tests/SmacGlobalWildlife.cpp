#include "SmacGlobalWildlife.h"
#include "game/backend/map/SmacGlobalWildlife.h"
#include "task/gsetests/GSETests.h"
#include <stdexcept>
#include <cstdlib>
#include <map>
#include <yaml-cpp/yaml.h>
#include "game/backend/resource/SmacSiteSignal.h"

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
    task->AddTest("SMAC global wildlife matches thirty original policy returns when supplied",GT(){
        const auto* directory=std::getenv("GLSMAC_GLOBAL_WILDLIFE_REFERENCE");if(!directory || !*directory) {GT_OK();}
        const std::string root(directory);
        const auto fixtures=YAML::LoadFile("docs/behavior/global-wildlife-closure-cases.json");
        const auto bytes=[](const std::string& hex) {
            std::vector<uint8_t> out;out.reserve(hex.size()/2);
            if(hex.size()%2) {throw std::runtime_error("Odd reference hex");}
            for(size_t i=0;i<hex.size();i+=2) {out.push_back(uint8_t(std::stoul(hex.substr(i,2),nullptr,16)));}return out;
        };
        const auto word=[](const std::vector<uint8_t>& raw,size_t p) {uint32_t n=0;for(size_t k=0;k<4;++k) {n|=uint32_t(raw.at(p+k))<<(8*k);}return n;};
        size_t checked=0;std::map<std::string,YAML::Node> documents;
        for(const auto& fixture:fixtures["cases"]) {
            const auto filename=fixture["source_receipt"].as<std::string>();
            auto found=documents.find(filename);if(found==documents.end()) {found=documents.emplace(filename,YAML::LoadFile(root+"/"+filename)).first;}
            const auto document=found->second;
            YAML::Node row;for(size_t j=0;j<document["cases"].size();++j) {const auto candidate=document["cases"][j];if(candidate["variant"].as<std::string>()==fixture["variant"].as<std::string>()) {row=candidate;break;}}
            GT_ASSERT(row && row["pc"].as<std::string>()=="0x53a820","Missing complete native producer return");
            const auto a=row["before"];const auto b=row["after"];wildlife::Input input{};
            input.geometry={a["width"].as<int32_t>(),a["height"].as<int32_t>(),!a["map_flat"].as<int32_t>()};
            input.turn=a["turn"].as<int32_t>();input.life=a["life"].as<int32_t>();input.difficulty=a["difficulty"].as<int32_t>();
            input.rules=a["rules"].as<uint32_t>();input.secondary=a["secondary"].as<uint32_t>();input.voice_marker=a["Voice_marker"].as<int32_t>();input.unit_count=a["unit_count"].as<int32_t>();
            for(size_t i=0;i<128;++i) {input.region_areas[i]=a["region_areas"][i].as<int32_t>();}
            for(size_t i=0;i<2;++i) {input.locust_technology_masks[i]=a["technology_native_masks"][i].as<uint32_t>();}
            const auto factions=bytes(a["raw_factions_hex"].as<std::string>()),native=bytes(a["faction0_hex"].as<std::string>());
            input.observer_status[0]=word(native,0);
            for(size_t i=1;i<8;++i) {input.observer_status[i]=word(factions,(i-1)*0x20bc);}
            for(size_t i=0;i<8;++i) {input.native_diplomacy[i]=word(native,0x18+4*i);}
            for(const auto& base:a["bases"]) {input.bases.push_back({base["id"].as<int32_t>(),{{base["position"][0].as<int32_t>(),base["position"][1].as<int32_t>()}}});}
            const auto raw=bytes(a["raw_map_hex"].as<std::string>());std::vector<Tile> terrain(raw.size()/44);
            for(size_t i=0;i<raw.size();++i) {terrain[i/44].bytes[i%44]=raw[i];}
            game::backend::map::smac_spread::Input flora{input.geometry,a["tile_count"].as<int32_t>(),input.turn,input.observer_status,terrain};
            const Random initial(a["shared"].as<uint32_t>(),a["crt"].as<uint32_t>());
            const auto growth=game::backend::map::smac_spread::Prepare(flora,initial);terrain=growth.tiles;
            GT_ASSERT(growth.changes.empty(),"Wildlife fixture unexpectedly contains flora changes");
            const auto tile=[&](Coord p)->Tile& {return terrain.at(size_t(p[1])*(input.geometry.width/2)+size_t(p[0])/2);};
            int next=input.unit_count;
            wildlife::Hooks hooks;
            hooks.tile=[&](Coord p){return tile(p);};
            hooks.site=[&](Coord p){const auto t=tile(p);return int(game::backend::resource::smac::ResolveSiteSignal({p[0],p[1],game::backend::resource::smac::AltitudeBand(t.Climate()>>5),t.Items(),bool(input.rules&0x2000u),27067,11,61}));};
            // Structural stock-profile factory. Complete original constructor writes
            // are verified by the private native checker, not synthesized here.
            hooks.create=[&](int,Coord p,Random&){if(next>=2048) {return -1;}auto& t=tile(p);if(!(t.Items()&3u)) {t.bytes[2]&=0xf0;}Items(t,t.Items()|2u);return next++;};
            hooks.capacity=[](int){return 2;};hooks.sentry=[](int,Random&){};hooks.promote_stack=[](int,Random&){};
            const auto result=wildlife::Produce(input,growth.random,hooks);
            GT_ASSERT(result.random.Crt()==b["crt"].as<uint32_t>() && result.random.Shared()==initial.Shared() && result.unit_count==b["unit_count"].as<int32_t>(),"Native final RNG or birth count differs");
            GT_ASSERT(result.actions.size()==fixture["actions"].size(),"Native ordered action count differs");
            for(size_t i=0;i<result.actions.size();++i) {
                const auto& actual=result.actions[i];const auto expected=fixture["actions"][i];const auto op=expected["op"].as<std::string>();
                if(op=="create") {
                    GT_ASSERT(actual.kind==wildlife::Action::Kind::Create && actual.design==expected["design"].as<int>() && actual.unit==expected["result"].as<int>() && actual.point==Coord({expected["position"][0].as<int>(),expected["position"][1].as<int>()}),"Native constructor request differs");
                } else {GT_ASSERT(actual.unit==expected["unit"].as<int>() && actual.kind==(op=="sentry" ? wildlife::Action::Kind::Sentry:wildlife::Action::Kind::PromoteStack),"Native order/head action differs");}
            }
            std::vector<Coord> sea,land;
            for(const auto& event:row["details"]) {if(event["pc"].as<std::string>()=="0x537cbe") {sea.push_back({{event["registers"]["esi"].as<int>(),event["registers"]["edi"].as<int>()}});}}
            for(const auto& event:row["events"]) {if(event["pc"].as<std::string>()=="0x537e74") {const auto stack=bytes(event["stack_hex"].as<std::string>());land.push_back({{int(word(stack,0x10)),int(word(stack,0x18))}});}}
            GT_ASSERT(result.sea_samples==sea && result.land_samples==land,"Native coordinate stream differs");++checked;
        }
        GT_ASSERT(checked==30,"Incomplete original wildlife controls");GT_OK();
    });

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

#include "SmacStartingSite.h"
#include "game/backend/map/SmacStartingSite.h"
#include "game/backend/map/SmacSiteQuality.h"
#include "game/backend/resource/SmacResourceBonus.h"
#include "game/backend/resource/SmacSiteSignal.h"
#include "task/gsetests/GSETests.h"
#include <yaml-cpp/yaml.h>
#include <cstdlib>
#include <map>
#include <stdexcept>

namespace gse::tests {
namespace {
namespace start=game::backend::map::smac_start;namespace site=game::backend::map::smac_site;
namespace resource=game::backend::resource::smac;
void Check(bool value,const std::string& message) {if(!value) {throw std::runtime_error(message);}}
std::vector<uint8_t> Hex(const std::string& text) {
    Check(!(text.size()&1),"Odd native map hex");std::vector<uint8_t> out;out.reserve(text.size()/2);
    const auto digit=[](char c)->unsigned {if(c>='0' && c<='9') {return c-'0';}if(c>='a' && c<='f') {return c-'a'+10;}if(c>='A' && c<='F') {return c-'A'+10;}throw std::runtime_error("Invalid native hex");};
    for(size_t i=0;i<text.size();i+=2) {out.push_back(static_cast<uint8_t>(16*digit(text[i])+digit(text[i+1])));}return out;
}
start::Coordinate Point(const YAML::Node& value) {return {{value[0].as<int32_t>(),value[1].as<int32_t>()}};}
struct NativeWorld {
    YAML::Node source;start::Input input{};std::vector<uint8_t> raw;
    NativeWorld(YAML::Node world,int32_t owner):source(world),raw(Hex(world["map_hex"].as<std::string>())) {
        input.geometry={world["width"].as<int32_t>(),world["height"].as<int32_t>(),!(world["flags"].as<int32_t>()&1)};
        Check(raw.size()==size_t(input.geometry.width/2)*input.geometry.height*44,"Native startup map extent mismatch");
        input.scale=world["map_scale"].as<int32_t>();input.ocean=world["region_density"].as<int32_t>();input.difficulty=world["difficulty"].as<int32_t>();
        input.human=(world["state"]["human"].as<uint32_t>()&(1u<<owner))!=0;input.look_first=(world["state"]["rules"].as<uint32_t>()&0x10)!=0;
        for(size_t r=0;r<128;++r) {input.regions[r]={world["regions"][r][0].as<int32_t>(),world["regions"][r][1].as<int32_t>(),0};
            for(size_t f=0;f<7;++f) {if(world["presence"][f][r].as<int32_t>()) {input.regions[r].presence|=uint8_t(1u<<(f+1));}}}
        for(const auto& b:world["state"]["bases"]) {input.bases.push_back({b["id"].as<int64_t>(),Point(b["position"])});}
        for(const auto& u:world["state"]["units"]) {input.units.push_back({u["owner"].as<int32_t>(),Point(u["position"])});}
    }
    size_t At(start::Coordinate p) const {return 44*(size_t(p[1])*(input.geometry.width/2)+p[0]/2);}
    uint32_t Word(size_t a) const {return uint32_t(raw.at(a))|(uint32_t(raw.at(a+1))<<8)|(uint32_t(raw.at(a+2))<<16)|(uint32_t(raw.at(a+3))<<24);}
    start::Tile Tile(start::Coordinate p) const {const auto a=At(p);const auto flags=Word(a+8);return {uint8_t(raw[a]>>5),uint8_t((raw[a]>>3)&3),uint8_t(raw[a+5]>>6),raw[a+3],raw[a+15],bool(flags&0x20),bool((flags&3) && (raw[a+2]&15)<8)};}
    int32_t Quality(start::Coordinate p) const {
        site::Areas areas{};for(size_t r=0;r<128;++r) {areas[r]=input.regions[r].area;}
        return site::Quality(input.geometry,p,0,areas,[&](auto q) {
            const auto a=At(q);const auto t=Tile(q);const auto flags=Word(a+8),landmarks=Word(a+12);const auto band=static_cast<resource::AltitudeBand>(t.elevation);
            const auto pattern=source["pattern"].as<int32_t>();const bool automatic=(source["state"]["rules"].as<uint32_t>()&0x2000)!=0;
            const auto bonus=resource::ResolveResourceBonus({q[0],q[1],band,pattern,automatic,bool(flags&0x400),false,false,false});
            const auto signal=resource::ResolveSiteSignal({q[0],q[1],band,flags,automatic,pattern,source["coefficients"][1].as<int32_t>(),source["coefficients"][0].as<int32_t>()});
            return site::Tile{t.elevation,t.moisture,t.rockiness,t.region,t.fungus,bool(flags&0x2000),bool(flags&0x200000),bool(flags&0x80),bool(flags&0x1000),bool(flags&1),bonus!=resource::ResourceBonus::None,signal!=0,bool(flags&0x40004000),bool(landmarks&2),bool(landmarks&1),bool(landmarks&8),bool(landmarks&4),t.landmark_detail};
        }).rating;
    }
};
}
void AddSmacStartingSiteTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC startup site search matches all original candidates metrics refinements and cache writes",GT(){
        const auto* path=std::getenv("GLSMAC_STARTUP_SITE_REFERENCE");if(!path || !*path) {GT_OK();}
        const auto data=YAML::LoadFile(path);size_t total=0,misses=0,refined=0;
        NativeWorld preparation(data["cache_stages"][1]["world"],1);
        const auto expected_cache=Hex(data["actors"][0]["inputs"]["map_hex"].as<std::string>());
        std::array<int32_t,128> wet{};
        for(int32_t y=0;y<preparation.input.geometry.height;++y) {for(int32_t x=y&1;x<preparation.input.geometry.width;x+=2) {
            const start::Coordinate p{{x,y}};const auto a=preparation.At(p);const auto tile=preparation.Tile(p);
            Check(!(preparation.raw[a+2]>>4),"Preparation retained imported cached ratings");
            Check(preparation.Quality(p)==int32_t(expected_cache[a+2]>>4),"Initial native all-map site refill differs at "+std::to_string(x)+","+std::to_string(y));
            if(tile.elevation>=3 && tile.moisture && tile.rockiness<2 && !tile.fungus) {++wet[tile.region];}
        }}
        for(size_t r=0;r<128;++r) {Check(wet[r]==preparation.input.regions[r].wet,"Native preparation regional wet count differs");}
        const std::map<std::string,std::string> rejected{{"0x5c9440","elevation_low"},{"0x5c944f","fungus"},{"0x5c9460","rocky"},{"0x5c9499","initial_distance"},{"0x5c94a5","elevation_high"},{"0x5c94be","occupied"},{"0x5c94c7","polar"},{"0x5c94d8","polar"},{"0x5c94fe","good_tiles"},{"0x5c953b","relaxed_distance"},{"0x5c9570","early_region_area"},{"0x5c95eb","region_density"},{"0x5c9628","region_area"},{"0x5c9658","shared_region_area"},{"0x5c96b9","site_rating"},{"0x5c96ed","landmark_distance"},{"0x5c96f5","landmark_distance"}};
        for(const auto& actor:data["actors"]) {
            NativeWorld world(actor["inputs"],actor["faction"].as<int32_t>());size_t seen=0,queries=0,refinements=0;
            const game::backend::SmacGameplayRandomState initial(world.source["state"]["shared"].as<uint32_t>(),world.source["state"]["crt"].as<uint32_t>());
            const auto result=start::Select(world.input,initial,[&](auto p){return world.Tile(p);},[&](auto p) {
                const auto a=world.At(p);if(world.Word(a+8)&0x1000) {return 0;}
                if(world.raw[a+2]>>4) {return int32_t(world.raw[a+2]>>4);}
                const auto value=world.Quality(p);const auto expected=actor["queries"][queries];const auto write=actor["map_writes"][queries++];
                Check(Point(expected["position"])==p && expected["value"].as<int32_t>()==value,"Native uncached startup quality differs");
                const auto next=uint8_t((world.raw[a+2]&15)|(value<<4));
                Check(write["offset"].as<size_t>()==a+2 && Hex(write["before"].as<std::string>()).at(0)==world.raw[a+2] && write["value"].as<uint32_t>()==next,"Native startup rating setter differs");world.raw[a+2]=next;return value;
            },[&](const auto& a) {
                const auto e=actor["attempts"][seen++];
                Check(a.number==int32_t(seen) && a.point==Point(e["position"]) && a.crt==e["crt_after_pair"].template as<uint32_t>()
                    && a.good==e["good_tiles"].template as<int32_t>() && a.bucket==e["landmark_ring"].template as<int32_t>()
                    && a.nearest==e["nearest"].template as<int64_t>() && a.distance==e["distance"].template as<int32_t>()
                    && a.region==e["region"].template as<uint8_t>() && a.presence==e["presence"].template as<uint8_t>(),"Native startup candidate metrics differ at "+std::to_string(seen));
                const auto expected=e["rejected_after"] ? rejected.at(e["rejected_after"].template as<std::string>()):"admitted";
                Check(start::OutcomeName(a.outcome)==expected,"Native startup rejection differs at "+std::to_string(seen));
            },[&](const auto& r) {
                const auto e=actor["refinements"][refinements++];const auto f=e["frame"];
                Check(r.index==f["0x14"].template as<size_t>() && r.point==start::Coordinate{{f["0x20"].template as<int32_t>(),f["0x34"].template as<int32_t>()}}
                    && r.score==e["score"].template as<int32_t>() && r.previous==f["0x30"].template as<int32_t>(),"Native startup refinement differs");
            });
            const auto center=actor["accepted"]["frame"],final=actor["final"]["frame"];
            GT_ASSERT(seen==actor["attempts"].size() && refinements==actor["refinements"].size() && queries==actor["queries"].size()
                && result.center==std::optional<start::Coordinate>({{center["0x10"].as<int32_t>(),center["0x1c"].as<int32_t>()}})
                && result.site==std::optional<start::Coordinate>({{final["0x10"].as<int32_t>(),final["0x1c"].as<int32_t>()}})
                && result.random.Crt()==actor["final"]["crt"].as<uint32_t>() && result.random.Shared()==initial.Shared()
                && world.raw==Hex(actor["final"]["map_hex"].as<std::string>()),"Native startup final position RNG or entire map differs");
            total+=seen;misses+=queries;refined+=refinements;
        }GT_ASSERT(total==3747 && misses==3 && refined==65,"Native startup control coverage changed");GT_OK();
    });
    task->AddTest("SMAC startup computer budget preserves the human turn-zero bypass",GT(){
        for(const bool human:{false,true}) {
            start::Input input{};input.geometry={80,80,true};input.scale=56;input.ocean=1;input.difficulty=3;input.human=human;
            input.regions[1]={3200,3200,0};size_t count=0;const game::backend::SmacGameplayRandomState initial(17,3);
            const auto result=start::Select(input,initial,[&](auto){return start::Tile{uint8_t(count>=1024 ? 3:2),2,0,1,0,false,false};},[](auto){return 1;},[&](const auto& a){++count;if(count==1025) {Check(a.outcome==(human ? start::Outcome::Selected:start::Outcome::Limit),"Startup budget changed native gate priority");}});
            GT_ASSERT(result.attempts==1025 && result.site.has_value()==human && initial.Crt()==3 && initial.Shared()==17,"Human inherited computer search refusal or input RNG changed");
        }GT_OK();
    });
}
}

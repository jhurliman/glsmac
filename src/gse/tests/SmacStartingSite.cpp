#include "SmacStartingSite.h"
#include "game/backend/map/SmacStartingBalance.h"
#include "game/backend/map/SmacPlanetMap.h"
#include "game/backend/map/GlsmacResourceMapPolicy.h"
#include <fstream>
#include <iterator>
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
    task->AddTest("SMAC startup balance matches original scores deficits Pods and every terrain item",GT(){
        namespace balance=game::backend::map::smac_balance;
        const auto* path=std::getenv("GLSMAC_STARTUP_BALANCE_REFERENCE");if(!path || !*path) {GT_OK();}
        const auto data=YAML::LoadFile(path),cases=YAML::LoadFile("docs/behavior/startup-site-balance-cases.json");
        size_t total=0,items=0;
        for(size_t variant=0;variant<data["replays"].size();++variant) {
            const auto replay=data["replays"][variant],world=replay["before"],expected=cases["cases"][variant];
            Check(replay["variant"].as<std::string>()==expected["variant"].as<std::string>(),"Balance control order differs");
            balance::Input input{};input.geometry={world["width"].as<int32_t>(),world["height"].as<int32_t>(),!(world["map_flags"].as<int32_t>()&1)};
            input.difficulty=3;input.human_mask=world["state"]["human"].as<uint8_t>();
            auto raw=Hex(world["tiles_hex"].as<std::string>());
            const auto index=[&](auto p){return 44*(size_t(p[1])*(input.geometry.width/2)+p[0]/2);};
            const auto word=[&](size_t a){return uint32_t(raw.at(a))|(uint32_t(raw.at(a+1))<<8)|(uint32_t(raw.at(a+2))<<16)|(uint32_t(raw.at(a+3))<<24);};
            for(size_t r=0;r<128;++r) {input.areas[r]=world["regions"][7*r].as<int32_t>();}
            for(const auto& u:world["state"]["units"]) {input.units.push_back({u["id"].as<int64_t>(),u["owner"].as<int32_t>(),Point(u["position"])});}
            for(size_t f=1;f<=7;++f) {const auto bonus=world["starting_bonuses"][f-1];
                for(int32_t j=0;j<bonus[0].as<int32_t>();++j) {if(bonus[1+j].as<int32_t>()==1) {
                    input.configured_unit_roles[f].push_back(world["design_roles"][std::to_string(bonus[9+j].as<int32_t>())].as<uint8_t>());
                }}
            }
            const game::backend::SmacGameplayRandomState initial(world["state"]["shared"].as<uint32_t>(),world["state"]["crt"].as<uint32_t>());
            const auto result=balance::Prepare(input,initial,[&](auto p) {const auto a=index(p);
                return balance::Tile{uint8_t(raw[a]>>5),uint8_t((raw[a]>>3)&3),uint8_t(raw[a+5]>>6),raw[a+3],word(a+8)};
            },[&](auto p,uint32_t flags) {return resource::ResolveSiteSignal({p[0],p[1],static_cast<resource::AltitudeBand>(raw[index(p)]>>5),flags,
                bool(world["state"]["rules"].as<uint32_t>()&0x2000),world["resource_pattern"].as<int32_t>(),world["pattern_multipliers"][1].as<int32_t>(),world["pattern_multipliers"][0].as<int32_t>()});});
            GT_ASSERT(result.scores.size()==7 && result.deficits.size()==7,"Balance skipped native owners");
            for(size_t i=0;i<7;++i) {const auto& a=result.scores[i];const auto e=expected["components"][i];
                Check(a.owner==e["faction"].as<int32_t>() && a.point==Point(e["position"]) && a.region==e["region"].as<int32_t>()
                    && a.area==e["region_area"].as<int32_t>() && a.spacing==e["nearest_normalized_spacing"].as<int32_t>()
                    && a.space==e["spacing_component"].as<int32_t>() && a.terrain_count==e["terrain_counter"].as<int32_t>()
                    && a.terrain==e["terrain_component"].as<int32_t>() && a.region_term==e["region_component"].as<int32_t>() && a.value==e["score"].as<int32_t>(),"Native balance score differs");
                const auto& d=result.deficits[i];const auto n=expected["deficits"][i];
                Check(d.owner==n["faction"].as<int32_t>() && d.maximum==n["maximum"].as<int32_t>() && d.credit==n["unit_credit"].as<int32_t>() && d.value==n["deficit"].as<int32_t>(),"Native balance deficit differs");
                for(size_t f=1;f<=7;++f) {Check(d.scores[f]==std::optional<int32_t>(n["scores"][f-1].as<int32_t>()),"Native mutable balance score history differs");}
            }
            Check(result.pods.size()==expected["pod_owners"].size() && result.writes.size()==expected["terrain_writes"].size(),"Native balance package count differs");
            for(size_t i=0;i<result.pods.size();++i) {Check(result.pods[i].owner==expected["pod_owners"][i].as<int32_t>(),"Native balance Pod owner differs");}
            for(size_t i=0;i<result.writes.size();++i) {const auto& w=result.writes[i];const auto e=expected["terrain_writes"][i];const auto a=index(w.point)+8;
                Check(w.owner==e["faction"].as<int32_t>() && w.point==Point(e["position"]) && a==e["offset"].as<size_t>()
                    && w.mask==e["mask"].as<uint32_t>() && w.after==e["value"].as<uint32_t>() && word(a)==w.before,"Native ordered compensation write differs");
                for(size_t k=0;k<4;++k) {raw[a+k]=uint8_t(w.after>>(8*k));}
            }
            const auto final=Hex(replay["tiles_hex"].as<std::string>());
            for(size_t a=0;a<raw.size();a+=44) {Check(word(a+8)==(uint32_t(final[a+8])|(uint32_t(final[a+9])<<8)|(uint32_t(final[a+10])<<16)|(uint32_t(final[a+11])<<24)),"Final native compensation items differ");++items;}
            if(const auto* assets=std::getenv("GLSMAC_SCOUT_ASSETS")) {
                namespace map=game::backend::map;
                std::ifstream file(std::string(assets)+"/maps/planet.MP",std::ios::binary);
                const std::string installed((std::istreambuf_iterator<char>(file)),{});
                Check(installed.size()==map::SmacPlanetMap::FILE_BYTES,"Missing owned Planet publication container");
                auto container=installed;const auto begin=map::SmacPlanetMap::PREFIX_BYTES+map::SmacPlanetMap::HEADER_BYTES;
                const auto before=Hex(world["tiles_hex"].as<std::string>());
                container.replace(begin,before.size(),reinterpret_cast<const char*>(before.data()),before.size());
                const auto source=map::SmacPlanetMap::Decode(container),published=source.WithStartingBalance(result);
                auto expected_container=container;expected_container.replace(begin,raw.size(),reinterpret_cast<const char*>(raw.data()),raw.size());
                Check(published.Encode()==expected_container && source.Encode()==container,"Compensation publication changed unrelated source bytes or input authority");
                if(!result.writes.empty()) {
                    auto stale=result;stale.writes.front().before^=0x80;stale.writes.front().after=stale.writes.front().before|stale.writes.front().mask;
                    bool rejected=false;try {(void)source.WithStartingBalance(stale);}catch(const std::invalid_argument&){rejected=true;}
                    Check(rejected,"Stale compensation source terrain was accepted");
                }
                // Exercise the actual immutable policy and its saved container
                // separately from native live Unit-presence bytes above.
                auto policy=map::GlsmacResourceMapPolicy::InstalledPlanetV1(map::SmacPlanetMap::Decode(installed),true);
                auto staged=result;staged.writes.clear();const auto flags=policy.OriginalPlanet()->At(1,1).Items();
                staged.writes.push_back({1,{{1,1}},flags,0x04000000u,flags|0x04000000u});
                const auto updated=policy.WithStartingBalance(staged);
                Check(updated.OriginalPlanet()->At(1,1).Items()==(flags|0x04000000u) && policy.OriginalPlanet()->At(1,1).Items()==flags
                    && map::GlsmacResourceMapPolicy::Deserialize(updated.Serialize().ToString())==updated,"Compensation policy/save lost markers or mutated its source");
            }
            Check(result.random.Crt()==replay["crt"].as<uint32_t>() && result.random.Shared()==initial.Shared() && initial.Crt()==expected["crt_before"].as<uint32_t>(),"Balance RNG lifecycle differs");
            total+=result.writes.size();
        }
        GT_ASSERT(total==161 && items==12800,"Native compensation coverage changed");GT_OK();
    });
    task->AddTest("SMAC startup balance retains zero-deficit draw and high-difficulty human Pod",GT(){
        namespace balance=game::backend::map::smac_balance;
        balance::Input input{};input.geometry={80,80,true};input.areas[1]=3200;input.units.push_back({0,1,{{20,20}}});input.human_mask=2;
        const game::backend::SmacGameplayRandomState initial(17,531966586);
        for(int32_t difficulty=0;difficulty<=5;++difficulty) {input.difficulty=difficulty;
            const auto result=balance::Prepare(input,initial,[](auto){return balance::Tile{3,2,0,1,0};},[](auto,auto){return 0;});
            auto after=initial;after.NextCrtRaw15();
            GT_ASSERT(result.random==after && result.deficits.size()==1 && result.deficits[0].value==0 && result.writes.empty()
                && result.pods.size()==size_t(difficulty>=4),"Zero deficit suppressed mandatory draw or used computer Pod threshold");
        }GT_OK();
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

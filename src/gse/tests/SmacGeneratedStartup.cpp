#include "SmacGeneratedStartup.h"
#include "fixtures/SmacGeneratedStartup.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/unit/SmacUnitInitialization.h"
#include "game/backend/map/GlsmacResourceMapPolicy.h"
#include "game/backend/map/SmacTerrainAuthority.h"
#include "loader/txt/UnitRulesTXTLoader.h"
#include "util/SHA256.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

namespace gse::tests {
namespace {
template<class R> void Bytes(std::string& out,const R& r) {out.append(reinterpret_cast<const char*>(r.data()),r.size());}
void Append(std::string& s,uint32_t value,unsigned size) {for(unsigned k=0;k<size;++k) {s+=char(uint8_t(value>>(8*k)));}}
std::string Difference(const game::backend::faction::SmacGeneratedStartup& w,const fixtures::generated_startup::Boundary& e,bool owned) {
    std::string b,u,a,d,t,r,o;for(auto base:w.bases) {if(!owned) {std::fill(base.begin()+19,base.begin()+44,0);}Bytes(b,base);}
    for(const auto& unit:w.units) {Bytes(u,game::backend::unit::OriginalSmacUnitBytes(unit));}
    for(const auto& actor:w.actors) {Bytes(a,actor.record);}
    for(size_t i=0;i<w.designs.size();++i) {auto design=w.designs[i];
        if(!owned && i<14) {std::fill_n(design.begin(),fixtures::installed_designs::NAME_LENGTHS[i]+1,0);}
        else if(!owned && i>=64 && (design[48]&1)) {std::fill_n(design.begin(),32,0);}Bytes(d,design);
    }
    for(const auto& tile:w.terrain.records) {Bytes(t,tile);}Bytes(r,w.regions.records);Bytes(o,w.ownership);
    for(const auto& p:std::array<std::pair<const std::string*,const char*>,7>{{{&b,owned?e.full_base_hash:e.base_hash},{&u,e.unit_hash},{&a,e.actor_hash},
        {&d,owned?e.full_design_hash:e.design_hash},{&t,e.tile_hash},{&r,e.region_hash},{&o,e.ownership_hash}}}) {if(util::SHA256(*p.first)!=p.second) {
        return " buffers(base/unit/actor/design/tile/region/ownership)="+util::SHA256(b)+"/"+util::SHA256(u)+"/"+util::SHA256(a)+"/"+util::SHA256(d)+"/"+util::SHA256(t)+"/"+util::SHA256(r)+"/"+util::SHA256(o);
    }}
    if(w.bases.size()!=e.bases || w.units.size()!=e.units || w.terrain.random.Crt()!=e.crt || w.terrain.random.Shared()!=e.shared || w.terrain.dirty_flags!=e.dirty) {
        return " counts/RNG/dirty differ: "+std::to_string(w.bases.size())+"/"+std::to_string(w.units.size())+"/"+std::to_string(w.terrain.random.Crt())+"/"+std::to_string(w.terrain.dirty_flags);
    }
    return {};
}
std::string Trim(const std::string& s) {const auto a=s.find_first_not_of(" \t\r\n");if(a==std::string::npos) {return {};}return s.substr(a,s.find_last_not_of(" \t\r\n")-a+1);}
std::vector<std::string> Section(const std::string& source,const std::string& name) {
    std::istringstream stream(source);std::string line;std::vector<std::string> rows;bool active=false;
    while(std::getline(stream,line)) {line=Trim(line.substr(0,line.find(';')));if(line.empty()) {continue;}
        if(line[0]=='#') {if(active) {break;}active=line=="#"+name;continue;}if(active) {rows.push_back(line);}}
    if(rows.empty()) {throw std::runtime_error("Missing owned startup text section "+name);}return rows;
}
void OwnedText(game::backend::faction::SmacGeneratedStartupRules& rules,const char* directory) {
    // Read exact owned bytes, including original single-byte text. No converted
    // text, original tables, captures or temporary asset copies are written.
    const auto lower=[](std::string s){for(auto& c:s) {if(c>='A' && c<='Z') {c=char(c-'A'+'a');}}return s;};
    std::map<std::string,std::filesystem::path> paths;
    for(const auto& entry:std::filesystem::directory_iterator(directory)) {if(entry.is_regular_file()) {paths.emplace(lower(entry.path().filename().string()),entry.path());}}
    const auto read=[&](const std::string& name,const char* hash){std::ifstream stream(paths.at(lower(name)),std::ios::binary);
        if(!stream) {throw std::runtime_error("Cannot read owned startup source "+name);}std::ostringstream data;data<<stream.rdbuf();const auto value=data.str();
        if(util::SHA256(value)!=hash) {throw std::runtime_error("Owned startup source hash differs: "+name);}return value;};
    const auto alpha=read("alpha.txt","38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4");
    const auto units=loader::txt::UnitRulesTXTLoader::ParseUnits(alpha,"smac");
    const auto chassis=loader::txt::UnitRulesTXTLoader::ParseChassis(alpha,"smac");
    const auto armor=loader::txt::UnitRulesTXTLoader::ParseDefenses(alpha,"smac");
    if(units.units.size()!=rules.predefined.size()) {throw std::runtime_error("Owned predefined count differs");}
    for(size_t i=0;i<units.units.size();++i) {rules.predefined[i].name=units.units[i].name;}
    const auto labels=Section(read("labels.txt","b6b7057a9f96a29a77e89da143d38e34bec5d93d651ac798b3c27d4efdab2009"),"LABELS");
    rules.design_names={armor.defenses.at(1).short_name,chassis.chassis.at(0).names[2].name,chassis.chassis.at(0).names[3].name,
        labels.at(147),chassis.chassis.at(1).names[1].name,armor.defenses.at(1).rating};
    for(size_t i=0;i<7;++i) {auto& faction=rules.factions[i];const auto text=read(faction.identity+".txt",fixtures::generated_startup::FACTION_SOURCE_HASHES[i]);
        faction.names.land=Section(text,"BASES");faction.names.sea=Section(text,"WATERBASES");
        for(auto* list:{&faction.names.land,&faction.names.sea}) {for(auto& name:*list) {name=Trim(name.substr(0,name.find(',')));}}
    }
}
}
void AddSmacGeneratedStartupTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC complete generated seven faction startup matches original records workers and RNG",GT(){
        namespace generation=game::backend::map::smac_generation;namespace expected=fixtures::generated_startup;
        generation::State terrain{std::vector<generation::Record>(3200),{{2,1,1,1,1,1,1}},game::backend::SmacGameplayRandomState(1,1)};
        generation::Regions regions;regions.summary_width=16;regions.summary_height=16;regions.summary_initial=0;
        generation::GenerateTerrain(terrain,{80,80},generation::DEFAULT_WORLD_RULES,regions,{100,110});
        const auto original=terrain.records;const auto original_random=terrain.random;const auto original_regions=regions.records;
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");const bool owned=assets && *assets;
        auto rules=expected::Rules();if(owned) {OwnedText(rules,assets);}
        std::string random,stores,queries,scores;size_t random_count=0,boundaries=0,store_count=0,query_count=0,score_count=0;
        game::backend::faction::SmacGeneratedStartupObservers observer;
        observer.random=[&](const auto& d){Append(random,d.caller,4);Append(random,d.before,4);Append(random,d.after,4);Append(random,d.value,2);++random_count;};
        observer.worker_stores=[&](const auto& s){Append(stores,uint8_t(s.space),1);Append(stores,s.offset,4);Append(stores,s.size,1);Append(stores,s.value,4);Append(stores,s.caller,4);++store_count;};
        observer.worker_queries=[&](const auto& q){Append(queries,q.dispatcher?1:0,1);for(const auto value:{q.resource,q.x,q.y,q.value}) {Append(queries,uint32_t(value),4);}++query_count;};
        observer.worker_scores=[&](int32_t index,int32_t value){Append(scores,uint32_t(index),4);Append(scores,uint32_t(value),4);++score_count;};
        observer.founded=[&](int32_t owner,const auto& world) {
            const auto difference=Difference(world,expected::BOUNDARIES.at(owner-1),owned);
            if(!difference.empty()) {throw std::runtime_error("Generated founding "+std::to_string(owner)+difference);}
            const auto& worker=expected::WORKERS.at(owner-1);
            if(store_count!=worker.stores || query_count!=worker.queries || score_count!=worker.scores || util::SHA256(stores)!=worker.store_hash
                || util::SHA256(queries)!=worker.query_hash || util::SHA256(scores)!=worker.score_hash) {
                throw std::runtime_error("Generated worker journal differs for owner "+std::to_string(owner)+" stores="+std::to_string(store_count)+" hash="+util::SHA256(stores));
            }
            ++boundaries;stores.clear();queries.clear();scores.clear();store_count=query_count=score_count=0;
        };
        const auto world=game::backend::faction::CompleteSmacGeneratedStartup(terrain,regions,{80,80},rules,"Template",observer);
        const auto first=Difference(world,expected::BOUNDARIES[7],owned),last=Difference(world,expected::BOUNDARIES[8],owned);
        GT_ASSERT(first.empty() && last.empty(),"Original complete generated setup return differs"+first+last);
        GT_ASSERT(boundaries==7 && random_count==8339 && util::SHA256(random)==expected::RNG,"Complete original startup ordered RNG differs: "+std::to_string(random_count)+" hash="+util::SHA256(random));
        GT_ASSERT(terrain.records==original && terrain.random==original_random && regions.records==original_regions,"Startup changed caller-owned prestartup geography");
        using Authority=game::backend::map::SmacTerrainAuthority;using Policy=game::backend::map::GlsmacResourceMapPolicy;
        const auto policy=Policy::GeneratedTerrainV3(Authority::GeneratedPrestartup(world.terrain,{80,80}));
        const auto restored=Policy::Deserialize(policy.Serialize().ToString());std::string raw;
        for(const auto& tile:world.terrain.records) {Bytes(raw,tile);}
        GT_ASSERT(restored==policy && !restored.OriginalPlanet() && restored.NativeTerrainSource()->RecordBytes()==raw
            && restored.NativeTerrainSource()->Info().categories[0]==2,"Complete generated gameplay terrain lost records or provenance through policy persistence");
        return {};
    });
}
}

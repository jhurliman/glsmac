#include "SmacStartupFaction.h"
#include "game/backend/faction/SmacStartupFaction.h"
#include "fixtures/SmacFactionPrefix.h"
#include "fixtures/SmacSitePreparation.h"
#include "game/backend/map/SmacCompleteTerrainGeneration.h"
#include "game/backend/map/SmacGeneratedStartingSite.h"
#include "fixtures/SmacGeneratedFirstSite.h"
#include "game/backend/faction/SmacStartupTechnology.h"
#include "game/backend/faction/SmacTechnologyQuery.h"
#include "fixtures/SmacStartupTechnology.h"
#include "fixtures/SmacFactionStartupSource.h"
#include "loader/txt/FactionTXTLoader.h"
#include "game/backend/base/SmacStartupBase.h"
#include "fixtures/SmacStartupBase.h"
#include "game/backend/map/SmacStartupTerritory.h"
#include "fixtures/SmacStartupTerritory.h"
#include "game/backend/unit/SmacStartupGroundUnit.h"
#include "game/backend/unit/SmacUnitInitialization.h"
#include "fixtures/SmacStartupGroundUnit.h"
#include "task/gsetests/GSETests.h"
#include "util/SHA256.h"

namespace gse::tests {
void AddSmacStartupFactionTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC startup named technology grants match one hundred three original complete records",GT(){
        using namespace game::backend::faction;
        const auto bytes=[](const auto& a) {return std::string(reinterpret_cast<const char*>(a.data()),a.size());};
        for(const auto& c:fixtures::startup_technology::Grants()) {
            SmacStartupFaction state;SmacStartupTechnologyOwnership owned;
            for(size_t i=0;i<state.record.size();++i) {state.record[i]=uint8_t(i*37+19);}
            for(size_t i=0;i<owned.size();++i) {owned[i]=uint8_t(i*11+17);}
            for(unsigned k=0;k<4;++k) {state.record[0x95d574-0x95d1a8+k]=uint8_t(uint32_t(c.target)>>(8*k));}
            GT_ASSERT(util::SHA256(bytes(state.record))==c.input_record && util::SHA256(bytes(owned))==c.input_ownership,"Technology input recipe differs");
            GrantSmacStartupNamedTechnology(state,owned,c.faction,c.technology,c.source);
            GT_ASSERT(util::SHA256(bytes(state.record))==c.record && util::SHA256(bytes(owned))==c.ownership,
                "Original complete named technology grant differs");
        }
        return {};
    });
    task->AddTest("SMAC technology ownership query matches one hundred eighty original sentinel and definition controls",GT(){
        for(const auto& c:fixtures::startup_technology::Queries()) {
            GT_ASSERT(game::backend::faction::QuerySmacTechnology(c.faction,c.technology,c.ownership,c.parents)==c.value,
                "Original effective ownership query differs");
        }
        return {};
    });
    task->AddTest("SMAC generated startup derives first Scout map and Unit from zero storage",GT(){
        using namespace game::backend::faction;namespace generation=game::backend::map::smac_generation;
        namespace expected=fixtures::generated_first_site;
        generation::State terrain{std::vector<generation::Record>(3200),{{2,1,1,1,1,1,1}},game::backend::SmacGameplayRandomState(1,1)};
        generation::Regions regions;regions.summary_width=16;regions.summary_height=16;regions.summary_initial=0;
        generation::GenerateTerrain(terrain,{80,80},generation::DEFAULT_WORLD_RULES,regions,{100,110});
        SmacStartupFaction state;SmacFactionPrefixOptions options{1,3,2,255,0x1a0e,0,false,10,{-1,0,0,0,1,0},"Template"," "};
        InitializeSmacFactionPrefix(state,options,terrain.random);
        std::vector<generation::StartingCacheStore> stores;std::vector<uint32_t> pairs;
        const auto selected=generation::SelectGeneratedStartingSite(terrain,{80,80},regions,{3,true,false},
            [&](const auto& s){stores.push_back(s);},[&](const auto& a){pairs.push_back(a.crt);});
        GT_ASSERT(selected.site && (*selected.site)[0]==25 && (*selected.site)[1]==35 && selected.attempts==558 && stores.size()==37
            && selected.random.Crt()==789993329 && terrain.random==selected.random,"Original generated site search differs");
        GT_ASSERT(pairs.size()==expected::PAIR_CRT.size(),"Generated candidate count differs");
        for(size_t i=0;i<pairs.size();++i) {GT_ASSERT(pairs[i]==expected::PAIR_CRT[i],"Generated candidate pair RNG differs");}
        for(size_t i=0;i<stores.size();++i) {const auto& a=stores[i];const auto& b=expected::STORES[i];
            GT_ASSERT(a.offset==b.offset && a.before==b.before && a.after==b.after,"Original generated rating cache write differs");}
        const auto planeBytes=[&]() {std::string result;for(const auto& r:terrain.records) {result.append(reinterpret_cast<const char*>(r.data()),r.size());}return result;};
        const auto factionBytes=[&]() {return std::string(reinterpret_cast<const char*>(state.record.data()),state.record.size());};
        GT_ASSERT(util::SHA256(planeBytes())==expected::SEARCH_TILES_SHA256 && util::SHA256(factionBytes())==expected::SEARCH_RECORD_SHA256,
            "Complete original first-site plane/faction differs");
        const auto region=terrain.records[size_t((*selected.site)[1])*40+(*selected.site)[0]/2][3];
        size_t draws=0,writes=0;
        PrepareSmacStartingSite(state,terrain.records,{{80,80,true},*selected.site,1,regions.Area(region),3,0x1a0e},terrain.random,
            [&](const auto&){++draws;},[&](const auto&){++writes;});
        GT_ASSERT(util::SHA256(planeBytes())==expected::PREPARED_TILES_SHA256 && util::SHA256(factionBytes())==expected::PREPARED_RECORD_SHA256
            && terrain.random.Crt()==2011860586 && terrain.random.Shared()==110 && draws==3 && writes==42,
            "Original generated first site strategy/exploration differs");
        namespace source=smac_faction_startup_fixture;
        const auto technologies=source::Technologies();
        const auto profiles=loader::txt::FactionTXTLoader::ParseStartupRules(source::Alpha(),source::Source,technologies);
        const auto* gaia=profiles.Find("GAIANS");GT_ASSERT(gaia,"Missing source Gaia startup profile");
        SmacStartupTechnologyOwnership ownership{};GrantSmacStartupNamedTechnologies(state,ownership,1,*gaia);
        const std::string ownership_bytes(reinterpret_cast<const char*>(ownership.data()),ownership.size());
        GT_ASSERT(util::SHA256(factionBytes())==fixtures::startup_technology::GENERATED_RECORD
            && util::SHA256(ownership_bytes)==fixtures::startup_technology::GENERATED_OWNERSHIP
            && util::SHA256(planeBytes())==expected::PREPARED_TILES_SHA256 && terrain.random.Crt()==2011860586 && terrain.random.Shared()==110,
            "Generated first named grant differs from original complete faction/tile/ownership boundary");
        using namespace game::backend::base;
        SmacBaseSeedCatalog catalog{};catalog.ownership=ownership;catalog.design_costs={{3,2,1}};catalog.formers_prerequisite=6;
        catalog.free_facility_technologies.fill(-2); // stock loader disables all legacy Free-tech fields
        for(size_t i=0;i<89;++i) {for(size_t j=0;j<2;++j) {
            const auto ref=technologies.ResolveToken(technologies.technologies[i].prerequisites[j]);
            using Kind=game::backend::settings::SmacTechnologyReference::Kind;
            GT_ASSERT(ref.kind!=Kind::UNKNOWN,"Missing owned technology prerequisite input");
            catalog.prerequisites[i][j]=ref.kind==Kind::NONE?-1:ref.kind==Kind::DISABLED?-2:*ref.technology_id;
        }}
        for(const auto& bonus:gaia->Records()) {if(bonus.kind==game::backend::settings::SmacFactionStartupBonus::Kind::FACILITY) {catalog.facility_bonuses.push_back(bonus.parameter);}}
        SmacStartupBaseRecord base{};
        const auto seeded=SeedSmacStartupBase(base,terrain.records,state,{{80,80,true},*selected.site,1,0,3,2,2,10,0,false,false,false,false,"Gaia's Landing"},catalog);
        const auto baseBytes=[&](){return std::string(reinterpret_cast<const char*>(base.data()),base.size());};
        GT_ASSERT(seeded.index==0 && seeded.count==1 && util::SHA256(baseBytes())==fixtures::startup_base::SEEDED_BASES_HEX
            && util::SHA256(planeBytes())==fixtures::startup_base::SEEDED_TILES_HEX,"Original generated Base/tile seed projection differs");
        std::array<SmacStartupFaction,8> actors{};actors[1]=state;size_t radius_stores=0;
        const auto cleanup=PrepareSmacStartupBaseRadius(terrain.records,actors,{80,80,true},*selected.site,1,[&](const auto&){++radius_stores;});
        GT_ASSERT(util::SHA256(baseBytes())==fixtures::startup_base::RADIUS_BASES_HEX && util::SHA256(planeBytes())==fixtures::startup_base::RADIUS_TILES_HEX
            && cleanup[0]==19 && cleanup[1]==35 && radius_stores==70 && terrain.random.Crt()==2011860586 && terrain.random.Shared()==110,
            "Original generated Base/radius projection differs");
        std::string territory_stores,territory_queries;size_t store_count=0,query_count=0;
        const auto pack=[](std::string& out,uint32_t v,unsigned size){for(unsigned k=0;k<size;++k) {out+=char(uint8_t(v>>(8*k)));}};
        const auto rebuilt=game::backend::map::RebuildSmacStartupTerritory(terrain.records,actors,{base},regions,
            {{80,80,true},8,terrain.resource_pattern,11,61,terrain.primary_rules,2,3,terrain.dirty_flags},[&](const auto& s){
                pack(territory_stores,s.offset,4);pack(territory_stores,s.size,1);pack(territory_stores,s.value,4);pack(territory_stores,s.caller,4);++store_count;
            },[&](const auto& p,int32_t value){pack(territory_queries,uint32_t(p[0]),4);pack(territory_queries,uint32_t(p[1]),4);pack(territory_queries,uint32_t(value),4);++query_count;});
        const auto& owner=actors[1].record;
        const std::string owner_stats(reinterpret_cast<const char*>(owner.data()+(0x95e620-0x95d1a8)),0x500);
        const std::string owner_goals(reinterpret_cast<const char*>(owner.data()+(0x95eba0-0x95d1a8)),0x95f1e0-0x95eba0);
        const std::string region_bytes(reinterpret_cast<const char*>(regions.records.data()),regions.records.size());
        namespace territory_expected=fixtures::startup_territory;
        GT_ASSERT(util::SHA256(planeBytes())==territory_expected::GENERATED_TILES_HEX && util::SHA256(region_bytes)==territory_expected::GENERATED_REGIONS_HEX
            && util::SHA256(baseBytes())==territory_expected::GENERATED_BASES_HEX && util::SHA256(owner_stats)==territory_expected::GENERATED_STATS
            && util::SHA256(owner_goals)==territory_expected::GENERATED_GOALS && rebuilt.dirty==territory_expected::GENERATED_DIRTY
            && rebuilt.uncached==3155 && query_count==3155 && store_count==9555 && util::SHA256(territory_stores)==territory_expected::GENERATED_STORES
            && util::SHA256(territory_queries)==territory_expected::GENERATED_QUERIES && terrain.random.Crt()==2011860586 && terrain.random.Shared()==110,
            "Original generated full territory/region/Base and faction statistics/goals projection differs");
        using namespace game::backend::unit;
        SmacStartupUnitFactoryState factory;factory.dirty=rebuilt.dirty|2;factory.cached_base=0; // original Base-return dirty tail
        std::vector<SmacNativeUnitRecord> units;
        const auto scout=CreateSmacStartupGroundUnit(terrain.records,units,actors[1],{base},factory,{{80,80,true},*selected.site,1,2,3,0,1});
        GT_ASSERT(scout==0 && units.size()==1 && units[0].home_base==0,"Startup Scout nearest-home assignment differs");
        units[0].home_base=-1; // separate startup caller 0x5c9f5d, not the general factory
        const auto record=OriginalSmacUnitBytes(units[0]);const std::string unit_bytes(reinterpret_cast<const char*>(record.data()),record.size());
        const auto& factory_actor=actors[1].record;const size_t role_off=0x95e1fc-0x95d1a8;
        const auto role_count=uint32_t(factory_actor[role_off])|(uint32_t(factory_actor[role_off+1])<<8)|(uint32_t(factory_actor[role_off+2])<<16)|(uint32_t(factory_actor[role_off+3])<<24);
        namespace scout_expected=fixtures::startup_ground;
        GT_ASSERT(util::SHA256(unit_bytes)==scout_expected::GENERATED_UNITS_HEX && util::SHA256(planeBytes())==scout_expected::GENERATED_TILES_HEX
            && role_count==scout_expected::GENERATED_ROLE_COUNT && factory_actor[0x95d9fc-0x95d1a8+2]==scout_expected::GENERATED_DESIGN_COUNT
            && factory.dirty==scout_expected::GENERATED_DIRTY && terrain.random.Crt()==2011860586 && terrain.random.Shared()==110,
            "Original generated first Scout map/Unit and factory counters projection differs");return {};
    });
    task->AddTest("SMAC startup site strategy and exploration match eighty one original complete records and stores",GT(){
        using namespace game::backend::faction;
        const auto bytes=[](const auto& a) {return std::string(reinterpret_cast<const char*>(a.data()),a.size());};
        size_t total_draws=0,total_stores=0;
        for(const auto& c:fixtures::site_preparation::Cases()) {
            SmacStartupFaction state;for(size_t i=0;i<state.record.size();++i) {state.record[i]=uint8_t(i*37+19);}
            const auto word=[&](size_t address,uint32_t value) {for(unsigned k=0;k<4;++k) {state.record[address-0x95d1a8+k]=uint8_t(value>>(8*k));}};
            word(0x95d1a8,0);word(0x95f228,0);const std::array<size_t,4> addresses{{0x95d584,0x95d594,0x95d58c,0x95d590}};
            for(size_t i=0;i<4;++i) {word(addresses[i],uint32_t(c.personality[i]));}
            std::vector<std::array<uint8_t,44>> tiles(size_t(c.options.geometry.width/2)*size_t(c.options.geometry.height));
            for(size_t i=0;i<tiles.size();++i) {for(size_t k=0;k<44;++k) {tiles[i][k]=uint8_t((44*i+k)*13+7);}}
            const auto tileBytes=[&]() {std::string result;for(const auto& tile:tiles) {result+=bytes(tile);}return result;};
            GT_ASSERT(util::SHA256(bytes(state.record))==c.input_hash && util::SHA256(tileBytes())==c.input_tiles,"Site preparation input differs");
            game::backend::SmacGameplayRandomState random(110,c.input_crt);std::vector<SmacStartupDraw> draws;std::string stores;size_t count=0;
            const auto pack=[&](uint32_t v,unsigned size) {for(unsigned k=0;k<size;++k) {stores+=char(uint8_t(v>>(8*k)));}};
            PrepareSmacStartingSite(state,tiles,c.options,random,[&](const auto& d){draws.push_back(d);},[&](const auto& s){
                pack(s.offset,4);pack(s.size,1);pack(s.value,4);pack(s.caller,4);++count;});
            GT_ASSERT(util::SHA256(bytes(state.record))==c.output_hash && util::SHA256(tileBytes())==c.output_tiles
                && random.Crt()==c.crt && random.Shared()==110 && count==c.store_count && util::SHA256(stores)==c.stores,
                "Original site preparation records/stores differ: "+std::string(c.id));
            GT_ASSERT(draws.size()==c.draws.size(),"Original strategy draw count differs");
            for(size_t i=0;i<draws.size();++i) {const auto& a=draws[i];const auto& b=c.draws[i];
                GT_ASSERT(a.caller==b.caller && a.before==b.before && a.after==b.after && a.value==b.value,"Original strategy RNG differs");}
            total_draws+=draws.size();total_stores+=count;
        }
        GT_ASSERT(total_draws==560 && total_stores==3310,"Site preparation control coverage changed");return {};
    });
    task->AddTest("SMAC startup faction prefix composes with zero storage complete terrain generation",GT(){
        using namespace game::backend::faction;namespace generation=game::backend::map::smac_generation;
        generation::State terrain{std::vector<generation::Record>(3200),{{2,1,1,1,1,1,1}},game::backend::SmacGameplayRandomState(1,1)};
        generation::Regions regions;regions.summary_width=16;regions.summary_height=16;regions.summary_initial=0;
        generation::GenerateTerrain(terrain,{80,80},generation::DEFAULT_WORLD_RULES,regions,{100,110});
        const auto plane=terrain.records;
        SmacStartupFaction state;SmacFactionPrefixOptions options{1,3,2,255,0x1a0e,0,false,10,{-1,0,0,0,1,0},"Template"," "};
        const auto result=InitializeSmacFactionPrefix(state,options,terrain.random);
        const std::string record(reinterpret_cast<const char*>(state.record.data()),state.record.size());
        const std::string scratch(reinterpret_cast<const char*>(state.scratch.data()),state.scratch.size());
        GT_ASSERT(util::SHA256(record)=="d926d12652cad0cd24fa23c2429679bad79bc418af34395c3baa2179f75a83f3"
            && util::SHA256(scratch)=="9f56cda75fefeab90f6fa5d5ddc9601544b121732c5ecccab32e631060453a5d"
            && terrain.random.Crt()==692949669 && terrain.random.Shared()==110 && terrain.records==plane && result.searching && result.active_mask==255,
            "Actual generated first faction prefix differs");return {};
    });
    task->AddTest("SMAC startup faction prefix matches thirty five complete poisoned records and original draws",GT(){
        using namespace game::backend::faction;
        const auto bytes=[](const auto& a) {return std::string(reinterpret_cast<const char*>(a.data()),a.size());};
        for(const auto& c:fixtures::faction_prefix::Cases()) {
            SmacStartupFaction state;
            for(size_t i=0;i<state.record.size();++i) {state.record[i]=uint8_t(i*37+c.poison);}
            for(size_t i=0;i<state.scratch.size();++i) {state.scratch[i]=uint8_t(i*11+c.poison);}
            GT_ASSERT(util::SHA256(bytes(state.record))==c.input_hash && util::SHA256(bytes(state.scratch))==c.input_scratch,
                "Original startup prefix input differs: "+std::string(c.id));
            game::backend::SmacGameplayRandomState random(110,c.input_crt);std::vector<SmacStartupDraw> draws;
            const auto result=InitializeSmacFactionPrefix(state,c.options,random,[&](const auto& d){draws.push_back(d);});
            GT_ASSERT(util::SHA256(bytes(state.record))==c.output_hash && util::SHA256(bytes(state.scratch))==c.output_scratch
                && random.Crt()==c.crt && random.Shared()==110 && result.active_mask==c.active && result.searching==c.searching,
                "Original full faction prefix differs: "+std::string(c.id));
            GT_ASSERT(draws.size()==c.draws.size(),"Original prefix draw count differs");
            for(size_t i=0;i<draws.size();++i) {const auto& a=draws[i];const auto& b=c.draws[i];
                GT_ASSERT(a.caller==b.caller && a.before==b.before && a.after==b.after && a.value==b.value,
                    "Original ordered faction startup RNG differs: "+std::string(c.id));}
        }
        return {};
    });
}
}

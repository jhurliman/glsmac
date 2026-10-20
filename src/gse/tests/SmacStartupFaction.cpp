#include "SmacStartupFaction.h"
#include "game/backend/faction/SmacStartupFaction.h"
#include "fixtures/SmacFactionPrefix.h"
#include "fixtures/SmacSitePreparation.h"
#include "game/backend/map/SmacCompleteTerrainGeneration.h"
#include "game/backend/map/SmacGeneratedStartingSite.h"
#include "fixtures/SmacGeneratedFirstSite.h"
#include "task/gsetests/GSETests.h"
#include "util/SHA256.h"

namespace gse::tests {
void AddSmacStartupFactionTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC generated startup derives original first site cache and exploration from zero storage",GT(){
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
            "Original generated first site strategy/exploration differs");return {};
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

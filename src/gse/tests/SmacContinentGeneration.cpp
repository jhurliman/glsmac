#include "SmacContinentGeneration.h"
#include "fixtures/SmacContinentGeneration.h"
#include "game/backend/map/SmacContinentGeneration.h"
#include "game/backend/map/SmacGenerationNoise.h"
#include "fixtures/SmacGenerationNoise.h"
#include "fixtures/SmacHillAltitude.h"
#include "fixtures/SmacGenerationRegions.h"
#include "game/backend/map/SmacGenerationRegions.h"
#include "game/backend/map/SmacPretemperatureGeneration.h"
#include "fixtures/SmacPretemperatureGeneration.h"
#include "game/backend/map/SmacInitialClimateGeneration.h"
#include "fixtures/SmacInitialClimateGeneration.h"
#include "task/gsetests/GSETests.h"
#include "util/SHA256.h"
#include <string>

namespace gse::tests {
namespace {
namespace generation=game::backend::map::smac_generation;
namespace fixture=fixtures::continent_generation;
void Word(std::string& bytes,uint32_t value,unsigned size=4) {
    for(unsigned i=0;i<size;++i) {bytes.push_back(char(uint8_t(value>>(8*i))));}
}
std::string Bytes(const generation::State& state) {
    std::string result;result.reserve(state.records.size()*44);
    for(const auto& record:state.records) {result.append(reinterpret_cast<const char*>(record.data()),44);}
    return result;
}
uint32_t Caller(generation::CrtUse use) {
    constexpr uint32_t sites[]={0x5da872,0x5da8a2,0x5da9f2,0x5da99f,0x5da731,0x5a8ddb,
        0x5daa6e,0x5daa9b,0x5dd902,0x5dd935,0x5dd969,0x5dd9a3};
    return sites[size_t(use)];
}
void PrepareRegionInput(generation::State& state,generation::Geometry g,generation::Regions& storage,
    const std::string& profile,const std::string& areas,const std::string& fresh="none") {
    const std::array<int,8> thresholds{{0,15,32,45,60,75,80,100}};
    for(int y=0;y<g.height;++y) {for(int x=y&1;x<g.width;x+=2) {
        const auto i=size_t(y)*size_t(g.width/2)+size_t(x/2);auto& record=state.records[i];
        const int band=profile=="mosaic"?((3*x+5*y)/2)%7:profile=="coast"?(y<g.height/2?1:3)
            :profile=="islands"?(x%8==0 && y%4==0?3:1):profile=="lakes"?(x%8==0 && y%4==0?1:3):profile[0]-'0';
        record[0]=uint8_t(band*32+((i*7)&31));record[1]=uint8_t(thresholds[band]+(x+y)%(thresholds[band+1]-thresholds[band]));
        record[2]=163;record[3]=99;record[5]=17;record[6]=uint8_t(i*13);record[7]=uint8_t(i*11);
        const uint32_t marks=(fresh=="all" || (fresh=="sparse" && i%4==0))?0x12000080:0x12000000;
        for(unsigned k=0;k<4;++k) {record[12+k]=uint8_t(marks>>(8*k));}
    }}
    storage.secondary_flags=0x4100;storage.summary_width=(g.width+4)/5;storage.summary_height=(g.height+4)/5;
    storage.summary.assign(size_t((storage.summary_width+1)/2)*size_t(storage.summary_height),165);
    for(size_t i=0;i<storage.records.size();++i) {storage.records[i]=uint8_t(i*7+13);}
    for(size_t i=0;i<128;++i) {
        const auto area=uint32_t(areas=="high"?1000:areas=="stale"?5+(i*3)%29:0);
        for(unsigned k=0;k<4;++k) {storage.records[i*28+k]=uint8_t(area>>(8*k));}
    }
}
std::string RegionBytes(const generation::Regions& r) {return std::string(reinterpret_cast<const char*>(r.records.data()),r.records.size());}
std::string SummaryBytes(const generation::Regions& r) {return std::string(r.summary.begin(),r.summary.end());}
struct ClimateObservation {
    generation::InitialClimateTrace trace;
    std::string stores,rng,shared;
    uint64_t checkpoints=0;
    ClimateObservation() {
        trace.on_store=[this](uint32_t off,unsigned size,uint32_t value){Word(stores,off);Word(stores,size,1);Word(stores,value);};
        trace.on_crt=[this](uint32_t before,uint32_t after,uint16_t value){Word(rng,before);Word(rng,after);Word(rng,value,2);};
        trace.on_shared=[this](bool seed,uint32_t before,uint32_t after,int32_t low,int32_t high,int32_t value){
            Word(shared,seed?1:0,1);Word(shared,before);Word(shared,after);Word(shared,uint32_t(low));Word(shared,uint32_t(high));Word(shared,uint32_t(value));
        };
        trace.checkpoint=[this]{++checkpoints;};
    }
    bool Matches(const generation::State& s,const fixtures::initial_climate::Result& r) const {
        return trace.stores==r.stores && trace.crt_draws==r.draws && trace.shared_events==r.shared
            && util::SHA256(Bytes(s))==r.output_hash && util::SHA256(stores)==r.store_hash && util::SHA256(rng)==r.rng_hash
            && util::SHA256(shared)==r.shared_hash && s.random.Crt()==r.crt && s.random.Shared()==r.shared_state;
    }
};
}
void AddSmacContinentGenerationTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC initial climate matches original temperature sources fungus and actual setup",GT(){
        uint64_t stores=0,draws=0,shared=0;
        for(const auto& c:fixtures::initial_climate::Cases()) {
            const generation::Geometry g{c.width,c.height};
            generation::State s{std::vector<generation::Record>(size_t(g.width/2)*size_t(g.height)),{{2,c.ocean,1,1,c.temperature,c.rainfall,c.life}},
                game::backend::SmacGameplayRandomState(1234567,c.crt)};
            generation::Regions unused;PrepareRegionInput(s,g,unused,c.profile,"zero");
            s.topology=c.flat?1:0;s.resource_pattern=11446;s.reset_words[1]=c.extra;
            for(auto& record:s.records) {
                record[5]=uint8_t(17+(c.rockiness<<6));
                for(unsigned k=0;k<4;++k) {record[8+k]=uint8_t(c.items>>(8*k));}
            }
            for(const auto& p:c.patches) {s.records.at(size_t(p.y)*size_t(g.width/2)+size_t(p.x/2)).at(p.field)=p.value;}
            GT_ASSERT(util::SHA256(Bytes(s))==c.input_hash,"Original initial climate input differs: "+std::string(c.id));
            ClimateObservation observed;
            if(c.kind==0) {generation::InitialTemperature(s,g,generation::DEFAULT_WORLD_RULES,&observed.trace);}
            else if(c.kind==1) {generation::SeedRiverSources(s,g,generation::DEFAULT_WORLD_RULES,&observed.trace);}
            else {generation::InitialFungus(s,g,generation::DEFAULT_WORLD_RULES,0x288050,&observed.trace);}
            GT_ASSERT(observed.Matches(s,c.result) && observed.trace.pumps==c.result.pumps && observed.checkpoints==c.result.pumps,
                "Original initial climate trace/output differs: "+std::string(c.id));
            stores+=observed.trace.stores;draws+=observed.trace.crt_draws;shared+=observed.trace.shared_events;
        }
        GT_ASSERT(stores==15029 && draws==10584 && shared==5469,"Initial climate control coverage changed");
        generation::State s{std::vector<generation::Record>(3200),{{2,1,1,1,1,1,1}},game::backend::SmacGameplayRandomState(1,1)};
        generation::Regions r;r.summary_width=16;r.summary_height=16;r.summary_initial=0;
        generation::GeneratePretemperature(s,{80,80},generation::DEFAULT_WORLD_RULES,r);
        ClimateObservation observed;const auto boundaries=fixtures::initial_climate::Boundaries();const auto hashes=fixtures::initial_climate::BoundaryHashes();
        for(size_t i=0;i<3;++i) {
            if(i==0) {generation::InitialTemperature(s,{80,80},generation::DEFAULT_WORLD_RULES,&observed.trace);}
            else if(i==1) {generation::SeedRiverSources(s,{80,80},generation::DEFAULT_WORLD_RULES,&observed.trace);}
            else {generation::InitialFungus(s,{80,80},generation::DEFAULT_WORLD_RULES,0x288050,&observed.trace);}
            const std::array<uint32_t,5> actual{{s.random.Crt(),s.random.Shared(),uint32_t(observed.trace.stores),uint32_t(observed.trace.crt_draws),uint32_t(observed.trace.shared_events)}};
            GT_ASSERT(actual==boundaries[i] && util::SHA256(Bytes(s))==hashes[i],"Actual original initial climate return differs");
        }
        GT_ASSERT(observed.Matches(s,fixtures::initial_climate::Full()) && observed.checkpoints==160,"Actual original initial climate complete trace differs");
        generation::State composed{std::vector<generation::Record>(3200),{{2,1,1,1,1,1,1}},game::backend::SmacGameplayRandomState(1,1)};
        generation::Regions regions;regions.summary_width=16;regions.summary_height=16;regions.summary_initial=0;
        generation::GeneratePretemperature(composed,{80,80},generation::DEFAULT_WORLD_RULES,regions);
        generation::GenerateInitialClimate(composed,{80,80},generation::DEFAULT_WORLD_RULES);
        GT_ASSERT(Bytes(composed)==Bytes(s) && composed.random==s.random && composed.stage==generation::Stage::Prelandmarks,
            "Initial climate controller did not reach the original landmark boundary");GT_OK();
    });
    task->AddTest("SMAC contour morph matches original two passes stores RNG and region history",GT(){
        uint64_t tiles=0,regions=0,draws=0;
        for(const auto& c:fixtures::pretemperature::Morphs()) {
            const generation::Geometry g{c.width,c.height};
            generation::State state{std::vector<generation::Record>(size_t(g.width/2)*size_t(g.height)),{{2,c.ocean,c.land,c.erosion,1,1,1}},
                game::backend::SmacGameplayRandomState(1234567,c.crt)};
            generation::Regions storage;PrepareRegionInput(state,g,storage,c.profile,c.areas);
            state.stage=generation::Stage::ContinentLoop;state.generation_flags={{1,1}};state.topology=c.flat?1:0;
            state.resource_pattern=11446;state.dirty_flags=16;state.working_land_table={{0,15,32,45,60,75,80,100,100,100,100}};
            GT_ASSERT(util::SHA256(Bytes(state))==c.input_hash && util::SHA256(RegionBytes(storage))==c.input_regions_hash,"Original morph input differs");
            auto rules=generation::DEFAULT_WORLD_RULES;if(c.zero) {for(size_t k=18;k<24;++k) {rules[k]=0;}}
            std::string tile_trace,region_trace,rng;generation::ContinentTrace shaping;generation::RegionTrace region;
            const auto draw=[&](uint32_t before,uint32_t after,uint16_t value){Word(rng,before);Word(rng,after);Word(rng,value,2);++draws;};
            const auto tile=[&](uint32_t off,unsigned size,uint32_t value){Word(tile_trace,off);Word(tile_trace,size,1);Word(tile_trace,value);++tiles;};
            shaping.on_write=[&](uint32_t off,uint8_t value){tile(off,1,value);};region.on_tile_store=tile;
            shaping.on_draw=[&](generation::CrtUse,uint32_t before,uint32_t after,uint16_t value){draw(before,after,value);};
            region.on_region_store=[&](uint32_t off,unsigned size,uint32_t value){Word(region_trace,off);Word(region_trace,size,1);Word(region_trace,value);++regions;};
            uint64_t checkpoints=0;shaping.checkpoint=[&]{++checkpoints;};region.checkpoint=shaping.checkpoint;
            const generation::Observer observer=[&](const generation::Draw& d){draw(d.before,d.after,uint16_t(d.value));};
            generation::MorphContours(state,g,rules,storage,{&shaping,&region,observer});
            GT_ASSERT(util::SHA256(Bytes(state))==c.output_hash && util::SHA256(RegionBytes(storage))==c.output_regions_hash
                && util::SHA256(SummaryBytes(storage))==c.summary_hash && storage.secondary_flags==c.secondary
                && state.random.Crt()==c.crt_after && state.random.Shared()==1234567 && state.dirty_flags==20,
                "Original complete morph output differs: "+std::string(c.id));
            GT_ASSERT(util::SHA256(tile_trace)==c.tile_hash && util::SHA256(region_trace)==c.region_hash && util::SHA256(rng)==c.rng_hash
                && shaping.writes+region.tile_stores==c.tiles && region.region_stores==c.regions && shaping.draws+324==c.draws
                && shaping.pumps+region.pumps==c.pumps && checkpoints==c.pumps,"Original ordered morph trace differs: "+std::string(c.id));
        }
        GT_ASSERT(tiles==72093 && regions==27751 && draws==12580,"Original morph coverage changed");GT_OK();
    });
    task->AddTest("SMAC complete pretemperature pipeline matches original attempts and actual setup",GT(){
        uint64_t draws=0;size_t attempts=0;
        for(const auto& c:fixtures::pretemperature::Prefixes()) {
            const generation::Geometry g{c.width,c.height};
            generation::State s{std::vector<generation::Record>(size_t(g.width/2)*size_t(g.height)),c.settings,
                game::backend::SmacGameplayRandomState(c.shared,c.crt)};
            for(size_t i=0;i<s.records.size();++i) {for(size_t k=0;k<44;++k) {s.records[i][k]=uint8_t((i*44+k)*37+11);}}
            generation::Regions regions;generation::ContinentTrace shaping;generation::RegionTrace region;std::string rng;
            const auto draw=[&](uint32_t before,uint32_t after,uint16_t value){Word(rng,before);Word(rng,after);Word(rng,value,2);++draws;};
            shaping.on_draw=[&](generation::CrtUse,uint32_t before,uint32_t after,uint16_t value){draw(before,after,value);};
            const generation::Observer observer=[&](const generation::Draw& d){if(d.kind==generation::Draw::Kind::Crt) {draw(d.before,d.after,uint16_t(d.value));}};
            const auto result=generation::GeneratePretemperature(s,g,c.rules,regions,{&shaping,&region,observer});
            GT_ASSERT(util::SHA256(Bytes(s))==c.output_hash && util::SHA256(rng)==c.rng_hash && s.random.Crt()==c.crt_after
                && s.random.Shared()==c.shared_after && s.resource_pattern==c.pattern && s.land_progress==c.progress && s.land_target==c.target
                && s.painted==c.painted && s.continent_attempts==c.carried && result.retries==c.retries && s.dirty_flags==c.dirty
                && s.stage==generation::Stage::Pretemperature && shaping.pumps+region.pumps==c.pumps,"Original complete generation differs: "+std::string(c.id));
            GT_ASSERT(result.attempts.size()==c.attempts.size(),"Original retry count differs");
            for(size_t i=0;i<result.attempts.size();++i) {
                const auto& a=result.attempts[i];const std::array<int32_t,6> actual{{a.entry,a.carried,a.iterations,a.continent_calls,a.hills,a.rejected?1:0}};
                GT_ASSERT(actual==c.attempts[i],"Original generation attempt history differs: "+std::string(c.id));
            }
            for(size_t i=0;i<128;++i) {GT_ASSERT(regions.Area(i)==c.areas[i],"Original generation region history differs");}
            GT_ASSERT(result.accepted==!result.attempts.back().rejected,"Original rejection outcome differs");attempts+=result.attempts.size();
        }
        GT_ASSERT(draws==205014 && attempts==19,"Complete generation coverage changed");
        generation::State actual{std::vector<generation::Record>(3200),{{2,1,1,1,1,1,1}},game::backend::SmacGameplayRandomState(1,1)};
        generation::Regions storage;storage.summary_width=16;storage.summary_height=16;storage.summary_initial=0;
        const auto result=generation::GeneratePretemperature(actual,{80,80},generation::DEFAULT_WORLD_RULES,storage);
        GT_ASSERT(util::SHA256(Bytes(actual))=="bbc3bdc35928316e48e2fc1f5e98370f00f4a59e2356697e17359e6555c1e215"
            && util::SHA256(RegionBytes(storage))=="a173f98907300d2531ba565eeb4ba86041c4d32ce163f0445ec304e0042fda5b"
            && util::SHA256(SummaryBytes(storage))=="60a88af59ddf64f017c4530709020b1e834f5a0c9a2d76560cf6a3d2cf697cd4",
            "Actual original setup pretemperature bytes differ");
        GT_ASSERT(actual.random.Crt()==3828047505u && actual.random.Shared()==1586005467u && actual.resource_pattern==17133
            && actual.continent_attempts==103 && result.retries==1 && result.accepted && actual.land_progress==642 && actual.land_target==640
            && actual.painted==10 && actual.dirty_flags==4 && storage.secondary_flags==0,"Actual original setup continuation differs");GT_OK();
    });
    task->AddTest("SMAC generation regions match original history capacity adjacency and summary",GT(){
        uint64_t tiles=0,regions=0,pumps=0;
        for(const auto& c:fixtures::generation_regions::Cases()) {
            generation::State state{std::vector<generation::Record>(size_t(c.width/2)*size_t(c.height)),{{2,1,1,1,1,1,1}},
                game::backend::SmacGameplayRandomState(1234567,1)};
            const std::array<int,8> thresholds{{0,15,32,45,60,75,80,100}};
            const std::string profile=c.profile,areas=c.areas,fresh=c.fresh;
            state.topology=c.flat?1:0;state.dirty_flags=16;
            for(int y=0;y<c.height;++y) {for(int x=y&1;x<c.width;x+=2) {
                const auto i=size_t(y)*size_t(c.width/2)+size_t(x/2);auto& record=state.records[i];
                const int band=profile=="mosaic"?((3*x+5*y)/2)%7:profile=="coast"?(y<c.height/2?1:3)
                    :profile=="islands"?(x%8==0 && y%4==0?3:1):profile=="lakes"?(x%8==0 && y%4==0?1:3):profile[0]-'0';
                record[0]=uint8_t(band*32+((i*7)&31));record[1]=uint8_t(thresholds[band]+(x+y)%(thresholds[band+1]-thresholds[band]));
                record[2]=163;record[3]=99;record[5]=17;record[6]=uint8_t(i*13);record[7]=uint8_t(i*11);
                const uint32_t marks=(fresh=="all" || (fresh=="sparse" && i%4==0))?0x12000080:0x12000000;
                for(unsigned k=0;k<4;++k) {record[12+k]=uint8_t(marks>>(8*k));}
            }}
            generation::Regions storage;storage.secondary_flags=0x4100;
            storage.summary_width=(c.width+4)/5;storage.summary_height=(c.height+4)/5;
            storage.summary.assign(size_t((storage.summary_width+1)/2)*size_t(storage.summary_height),165);
            for(size_t i=0;i<storage.records.size();++i) {storage.records[i]=uint8_t(i*7+13);}
            for(size_t i=0;i<128;++i) {
                const auto area=uint32_t(areas=="high"?1000:areas=="stale"?5+(i*3)%29:0);
                for(unsigned k=0;k<4;++k) {storage.records[i*28+k]=uint8_t(area>>(8*k));}
            }
            const auto region_bytes=[&]{return std::string(reinterpret_cast<const char*>(storage.records.data()),storage.records.size());};
            GT_ASSERT(util::SHA256(Bytes(state))==c.input_hash && util::SHA256(region_bytes())==c.input_regions_hash,"Original region fixture inputs differ");
            std::string tile_trace,region_trace;uint64_t checkpoints=0;generation::RegionTrace trace;
            trace.on_tile_store=[&](uint32_t off,unsigned size,uint32_t value){Word(tile_trace,off);Word(tile_trace,size,1);Word(tile_trace,value);};
            trace.on_region_store=[&](uint32_t off,unsigned size,uint32_t value){Word(region_trace,off);Word(region_trace,size,1);Word(region_trace,value);};
            trace.checkpoint=[&]{++checkpoints;};
            for(int i=0;i<c.repeat;++i) {generation::RebuildRegions(state,{c.width,c.height},storage,&trace);}
            const std::string summary(reinterpret_cast<const char*>(storage.summary.data()),storage.summary.size());
            GT_ASSERT(util::SHA256(Bytes(state))==c.output_hash && util::SHA256(region_bytes())==c.output_regions_hash
                && util::SHA256(summary)==c.summary_hash && storage.secondary_flags==c.secondary,
                "Original full region/summary result differs: "+std::string(c.id));
            GT_ASSERT(util::SHA256(tile_trace)==c.tile_hash && util::SHA256(region_trace)==c.region_hash
                && trace.tile_stores==c.tiles && trace.region_stores==c.regions && trace.pumps==c.pumps && checkpoints==c.pumps,
                "Original ordered region writes differ: "+std::string(c.id));
            GT_ASSERT(state.dirty_flags==16 && state.random.Crt()==1 && state.random.Shared()==1234567,"Region rebuild changed unrelated state");
            tiles+=trace.tile_stores;regions+=trace.region_stores;pumps+=trace.pumps;
        }
        GT_ASSERT(tiles==67424 && regions==49794 && pumps==1458,"Original region coverage changed");GT_OK();
    });
    task->AddTest("SMAC hill polar and altitude stages match original ordered boundaries",GT(){
        uint64_t stores=0,draws=0;
        for(const auto& control:fixtures::hill_altitude::Cases()) {
            const auto count=size_t(control.width/2)*size_t(control.height);
            generation::State state{std::vector<generation::Record>(count),control.settings,
                game::backend::SmacGameplayRandomState(control.shared,control.crt)};
            for(size_t i=0;i<count;++i) {
                state.records[i]={{32,20,15,0,0,0}};
                for(size_t f=6;f<8;++f) {state.records[i][f]=uint8_t((i*44+f)*37+11);}
            }
            for(const auto& field:control.fields) {
                GT_ASSERT(field.first<44 && field.second.size()==count*2,"Invalid raw-plane fixture field");
                const auto nibble=[](char c){return c<='9'?c-'0':c-'a'+10;};
                for(size_t i=0;i<count;++i) {state.records[i][field.first]=uint8_t(nibble(field.second[2*i])*16+nibble(field.second[2*i+1]));}
            }
            GT_ASSERT(util::SHA256(Bytes(state))==control.input_hash,"Original hill/altitude input differs");
            state.resource_pattern=control.pattern;state.land_progress=control.progress;state.painted=control.painted;state.dirty_flags=control.dirty;
            state.stage=generation::Stage::ContinentLoop;state.generation_flags={{1,1}};
            for(size_t i=0;i<8;++i) {state.working_land_table[i]=control.thresholds[i];}
            for(size_t i=8;i<11;++i) {state.working_land_table[i]=control.thresholds[7];}
            std::string writes,rng;uint64_t pumps=0;generation::ContinentTrace trace;
            trace.on_write=[&](uint32_t offset,uint8_t value){Word(writes,offset);Word(writes,value,1);};
            trace.on_draw=[&](generation::CrtUse use,uint32_t before,uint32_t after,uint16_t value){
                Word(rng,before);Word(rng,after);Word(rng,value,2);Word(rng,Caller(use));
            };
            trace.checkpoint=[&]{++pumps;};
            const generation::Geometry geometry{control.width,control.height};
            const generation::ContinentRules rules{12,24,{{3,6,12,18,24}},0};
            if(control.kind==0) {generation::SeedHill(state,geometry,rules,control.base,control.modifier,control.category,&trace);}
            else if(control.kind==1) {generation::SeedPolarContours(state,geometry,rules,&trace);}
            else {generation::ReconstructAltitudes(state,geometry,rules,&trace);}
            GT_ASSERT(util::SHA256(Bytes(state))==control.output_hash && state.random.Crt()==control.crt_after
                && state.random.Shared()==control.shared && state.land_progress==control.progress_after
                && state.painted==control.painted_after && state.dirty_flags==control.dirty_after,
                "Original hill/altitude outgoing state differs: "+std::string(control.id));
            GT_ASSERT(trace.writes==control.writes && trace.draws==control.draws && trace.pumps==control.pumps && pumps==control.pumps
                && util::SHA256(writes)==control.store_hash && util::SHA256(rng)==control.rng_hash,
                "Original ordered hill/altitude stores or RNG differ: "+std::string(control.id));
            stores+=trace.writes;draws+=trace.draws;
        }
        GT_ASSERT(stores==27719 && draws==1652,"Hill/altitude original coverage changed");GT_OK();
    });
    task->AddTest("SMAC generation noise matches original grid samples and random continuation",GT(){
        size_t samples=0;
        for(const auto& control:fixtures::generation_noise::Cases()) {
            game::backend::SmacGameplayRandomState random(1234567,control.before);
            game::backend::SmacGameplayRandomState expected(1234567,control.before);
            size_t draws=0;bool ordered=true;
            generation::Noise noise(random,[&](const generation::Draw& draw){
                const auto before=expected.Crt();const auto value=expected.NextCrtRaw15();
                ordered=ordered && draw.kind==generation::Draw::Kind::Crt && draw.before==before
                    && draw.after==expected.Crt() && draw.value==value;++draws;
            });
            GT_ASSERT(draws==324 && ordered && random.Crt()==control.after && random.Shared()==1234567,
                "Original noise constructor random stream differs");
            for(size_t x=0;x<17;++x) {for(size_t y=0;y<17;++y) {
                GT_ASSERT(noise.Grid()[x][y]==control.grid[x*17+y],"Original periodic noise grid differs");
            }}
            for(const auto& sample:control.samples) {
                GT_ASSERT(noise.Evaluate(sample[0],sample[1])==sample[2],"Original noise interpolation differs");
                GT_ASSERT(noise.Evaluate(sample[0]+4096,sample[1]-4096)==sample[2],"Noise period differs");
                ++samples;
            }
            GT_ASSERT(random.Crt()==control.after && random.Shared()==1234567,"Noise evaluation consumed randomness");
        }
        GT_ASSERT(samples==135,"Original noise sample coverage changed");GT_OK();
    });
    task->AddTest("SMAC continent generation matches original ordered stores RNG and return boundaries",GT(){
        uint64_t writes=0,draws=0;
        for(const auto& control:fixture::Cases()) {
            const auto count=size_t(control.geometry.width/2)*size_t(control.geometry.height);
            generation::State state{std::vector<generation::Record>(count),{{2,1,control.land,1,1,1,1}},
                game::backend::SmacGameplayRandomState(control.shared,control.crt)};
            state.resource_pattern=control.pattern;state.reset_words[0]=control.sea_offset;
            state.topology=control.flat?1:0;state.land_progress=control.progress;state.land_target=control.target;
            state.painted=control.painted;state.dirty_flags=control.dirty;
            state.generation_flags={{1,1}};state.stage=generation::Stage::ContinentLoop;
            state.working_land_table={{0,15,32,45,60,75,80,100,100,100,100}};
            for(int j=0;j<8;++j) {state.flood_counts[j]=100+j;state.affected_bases[j]=200+j;}
            for(size_t i=0;i<count;++i) {
                auto& record=state.records[i];
                record[0]=uint8_t(control.band*32+control.low_climate);record[1]=control.contour;record[2]=control.occupant;record[3]=control.region;
                record[6]=uint8_t(i*7+3);record[7]=uint8_t(i*11+5);
            }
            if(control.bootstrap_poison>=0) {
                // Actual production prefix -> actual production seed, with the
                // pinned supplied seed1 boundary rather than injected outputs.
                for(size_t i=0;i<count;++i) {for(size_t k=0;k<44;++k) {state.records[i][k]=uint8_t((44*i+k)*37+control.bootstrap_poison);}}
                state.random=game::backend::SmacGameplayRandomState(1,1);
                generation::Bootstrap(state,{384,256,36},generation::Entry::First);
                GT_ASSERT(state.random.Shared()==control.shared && state.random.Crt()==control.crt && state.resource_pattern==control.pattern
                    && state.land_target==control.target && state.land_progress==control.progress,"Bootstrap-to-continent continuation differs");
            }
            for(const auto& patch:control.patches) {state.records.at(patch.index).at(patch.field)=patch.value;}
            GT_ASSERT(util::SHA256(Bytes(state))==control.input_hash,"Original continent input plane differs: "+std::string(control.id));
            std::string stores,rng,paint_trace,direction_trace;
            std::vector<generation::Coordinate> seeds;std::vector<std::array<int32_t,4>> walks;
            generation::ContinentTrace trace;uint64_t checkpoints=0;
            trace.checkpoint=[&]{++checkpoints;};
            trace.on_write=[&](uint32_t offset,uint8_t value){Word(stores,offset);Word(stores,value,1);};
            trace.on_draw=[&](generation::CrtUse use,uint32_t before,uint32_t after,uint16_t value){
                Word(rng,before);Word(rng,after);Word(rng,value,2);Word(rng,Caller(use));
            };
            trace.on_candidate=[&](generation::Coordinate point){seeds.push_back(point);};
            trace.on_walk=[&](generation::Coordinate point,int32_t category,int32_t target){walks.push_back({{point[0],point[1],category,target}});};
            trace.on_paint=[&](generation::Coordinate point,int32_t category){Word(paint_trace,uint32_t(point[0]));Word(paint_trace,uint32_t(point[1]));Word(paint_trace,uint32_t(category));};
            trace.on_direction=[&](int32_t actual,int32_t previous,int32_t streak){Word(direction_trace,uint32_t(actual));Word(direction_trace,uint32_t(previous));Word(direction_trace,uint32_t(streak));};
            for(const auto& call:control.calls) {
                const auto& args=call.arguments;
                if(control.kind==fixture::Kind::Seed) {
                    state.continent_attempts=args[0]; // supplied outer caller's ordinal, not callee-owned progress
                    generation::SeedContinent(state,control.geometry,control.rules,state.continent_attempts,&trace);
                    GT_ASSERT(state.continent_attempts==args[0],"Continent callee changed its caller-owned attempt count");
                } else if(control.kind==fixture::Kind::Grow) {generation::GrowContinent(state,control.geometry,control.rules,{{args[0],args[1]}},args[2],args[3],&trace);}
                else {generation::PaintContinent(state,control.geometry,control.rules,{{args[0],args[1]}},args[2],&trace);}
                GT_ASSERT(state.random.Crt()==call.crt && state.random.Shared()==call.shared && state.land_progress==call.progress
                    && state.painted==call.painted && util::SHA256(Bytes(state))==call.hash && trace.writes==call.writes && trace.draws==call.draws,
                    "Original continent return boundary differs: "+std::string(control.id));
            }
            GT_ASSERT(state.random.Crt()==control.crt_after && state.random.Shared()==control.shared_after && state.land_target==control.target
                && state.land_progress==control.progress_after && state.painted==control.painted_after && state.dirty_flags==control.dirty_after,
                "Original continent final counters differ: "+std::string(control.id));
            GT_ASSERT(state.flood_counts==control.flood && state.affected_bases==control.affected && seeds==control.seeds && walks==control.walks,
                "Original continent candidates/scratch differ: "+std::string(control.id));
            GT_ASSERT(trace.writes==control.writes && trace.draws==control.draws && trace.paints==control.paints && trace.pumps==control.pumps && checkpoints==control.pumps
                && trace.direction_corrections==control.corrections,"Original continent trace counts differ: "+std::string(control.id));
            GT_ASSERT(util::SHA256(Bytes(state))==control.output_hash && util::SHA256(stores)==control.write_hash && util::SHA256(rng)==control.rng_hash
                && util::SHA256(paint_trace)==control.paint_hash && util::SHA256(direction_trace)==control.direction_hash,
                "Original ordered continent stores/draws/walk states differ: "+std::string(control.id));
            writes+=trace.writes;draws+=trace.draws;
        }
        GT_ASSERT(writes==113022 && draws==7331,"Original continent coverage changed");GT_OK();
    });
}
}

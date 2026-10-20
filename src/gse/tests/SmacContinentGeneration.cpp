#include "SmacContinentGeneration.h"
#include "fixtures/SmacContinentGeneration.h"
#include "game/backend/map/SmacContinentGeneration.h"
#include "game/backend/map/SmacGenerationNoise.h"
#include "fixtures/SmacGenerationNoise.h"
#include "fixtures/SmacHillAltitude.h"
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
}
void AddSmacContinentGenerationTests(task::gsetests::GSETests* task) {
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

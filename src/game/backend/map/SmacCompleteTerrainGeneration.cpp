#include "SmacCompleteTerrainGeneration.h"
#include "SmacFinalGeneration.h"
#include "SmacClimate.h"
#include <stdexcept>
#include <utility>

namespace game::backend::map::smac_generation {
namespace {
uint32_t Read(const Record& r,size_t f) {return uint32_t(r[f])|(uint32_t(r[f+1])<<8)|(uint32_t(r[f+2])<<16)|(uint32_t(r[f+3])<<24);}
void Observe(InitialClimateTrace* t,bool seed,uint32_t before,uint32_t after,int32_t low=0,int32_t high=0,int32_t value=0) {
    if(t) {++t->shared_events;if(t->on_shared) {t->on_shared(seed,before,after,low,high,value);}}
}
smac_climate::World Export(const State& s,Geometry g) {
    smac_climate::World w{g.width,g.height,s.topology==0,{}};w.tiles.reserve(s.records.size());
    for(const auto& r:s.records) {
        smac_climate::Tile tile{r[0]>>5,r[0]&7,(r[0]>>3)&3,r[1],r[6],Read(r,8),Read(r,12),r[2]&15,r[5]>>6};
        for(size_t k=0;k<7;++k) {tile.remembered_items[k]=Read(r,16+4*k);}w.tiles.push_back(tile);
    }return w;
}
void Commit(State& s,const smac_climate::World& w) {
    if(w.tiles.size()!=s.records.size()) {throw std::invalid_argument("Climate publication geometry differs");}
    for(size_t i=0;i<s.records.size();++i) {
        auto& r=s.records[i];const auto& t=w.tiles[i];r[0]=uint8_t((t.elevation<<5)|(t.moisture<<3)|t.temperature);
        r[1]=uint8_t(t.contour);r[6]=uint8_t(t.aux);r[2]=uint8_t((r[2]&240)|t.cached_occupant);r[5]=uint8_t((r[5]&63)|(t.rockiness<<6));
        const auto word=[&](size_t f,uint32_t value){for(unsigned k=0;k<4;++k) {r[f+k]=uint8_t(value>>(8*k));}};
        word(8,t.items);word(12,t.landmarks);for(size_t k=0;k<7;++k) {word(16+4*k,t.remembered_items[k]);}
    }s.dirty_flags|=w.dirty_flags;
}
}
void FinalClimate(State& s,Geometry g,const WorldRules& r,Regions& regions,uint32_t clock,InitialClimateTrace* trace) {
    ContinentTrace shaping;
    shaping.on_write=[&](uint32_t off,uint8_t value){if(trace) {++trace->stores;if(trace->on_store) {trace->on_store(off,1,value);}}};
    shaping.on_draw=[&](CrtUse,uint32_t before,uint32_t after,uint16_t value){if(trace) {++trace->crt_draws;if(trace->on_crt) {trace->on_crt(before,after,value);}}};
    shaping.checkpoint=[&]{if(trace) {++trace->pumps;if(trace->checkpoint) {trace->checkpoint();}}};
    const ContinentRules continent{r[2],r[3],{{r[25],r[26],r[27],r[28],r[29]}},s.primary_rules};
    ReconstructAltitudes(s,g,continent,&shaping);InitialTemperature(s,g,r,trace);
    auto w=Export(s,g);smac_climate::Streams random{s.random};
    random.on_seed=[&](uint32_t before,uint32_t after){Observe(trace,true,before,after);};
    random.on_shared=[&](uint32_t before,uint32_t after,int32_t low,int32_t high,int32_t value){Observe(trace,false,before,after,low,high,value);};
    // The routing kernel owns its static new contour bounds, independently of
    // the working thresholds used by the reconstruction above.
    smac_climate::Rivers(w,random);Commit(s,w);
    const smac_climate::Rules climate{uint32_t(s.resource_pattern),s.reset_words[1],s.categories[4],s.categories[5],r[10],r[11],r[12],r[13],r[15],r[16],r[17]};
    smac_climate::Rainfall(w,climate);Commit(s,w);RebuildRegions(s,g,regions);ClimatePostpass(s,g,regions,trace);
    const auto before=s.random.ExchangeShared(clock);Observe(trace,true,before,clock);
}
GeneratedTerrain CompleteTerrain(State& s,Geometry g,const WorldRules& r,Regions& regions,GenerationClocks clocks,InitialClimateTrace* trace) {
    if(s.stage!=Stage::Prelandmarks || s.generation_flags!=std::array<int32_t,2>{{1,1}}) {throw std::invalid_argument("Terrain completion requires prelandmark continuation");}
    GeneratedTerrain result;
    for(size_t i=0;i<13;++i) {PlaceLandmark(s,g,r,regions,result.landmarks,Landmark(i),{{-1,-1}},clocks.boreholes,false,trace);}
    result.profiles=smac_terrain::landmark_tables::PrepareHeightProfiles(s.records.size(),[&](size_t i){return Read(s.records[i],12);});
    NormalizeContours(s,g,trace);s.generation_flags[0]=0;
    FinalClimate(s,g,r,regions,clocks.final_climate,trace);FinalFeatures(s,g,trace);s.generation_flags[1]=0;s.stage=Stage::Prestartup;
    return result;
}
GeneratedTerrain GenerateTerrain(State& s,Geometry g,const WorldRules& r,Regions& regions,GenerationClocks clocks,InitialClimateTrace* trace) {
    auto attempts=GeneratePretemperature(s,g,r,regions);GenerateInitialClimate(s,g,r);
    auto result=CompleteTerrain(s,g,r,regions,clocks,trace);result.attempts=std::move(attempts);return result;
}
}

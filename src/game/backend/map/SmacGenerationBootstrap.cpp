#include "SmacGenerationBootstrap.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend::map::smac_generation {
namespace {
int32_t Signed(uint32_t value) {
    return value<=INT32_MAX?int32_t(value):int32_t(int64_t(value)-0x100000000LL);
}
int32_t Add(int32_t a,int32_t b) {return Signed(uint32_t(a)+uint32_t(b));}
int32_t Sub(int32_t a,int32_t b) {return Signed(uint32_t(a)-uint32_t(b));}
int32_t Mul(int32_t a,int32_t b) {return Signed(uint32_t(a)*uint32_t(b));}
}

void ResetRecords(State& state,const Observer& observer) {
    state.stage=Stage::Allocated;
    state.reset_words={};
    const auto before=state.random.Shared();
    const auto value=int32_t((uint32_t(state.random.NextSharedLow16())*32767u)>>16);
    state.resource_pattern=value+1;
    if(observer) {observer({Draw::Kind::Shared,before,state.random.Shared(),value});}
    for(auto& record:state.records) {
        record[0]=0x20;record[1]=0x14;record[2]=0x0f;
        record[3]=record[4]=record[5]=0;
        std::fill(record.begin()+8,record.end(),0);
    }
}

void Bootstrap(State& state,const Rules& rules,Entry entry,const Observer& observer) {
    if(state.records.size()>size_t(INT32_MAX)) {throw std::invalid_argument("Native generation record count exceeds signed32");}
    state.stage=Stage::Allocated;
    if(entry==Entry::First) {state.continent_attempts=0;}
    state.working_land_table={{0,15,32,45,60,75,80,100,100,100,100}};
    state.generation_flags={{1,1}};
    for(auto& category:state.categories) {if(category<0) {
        const auto before=state.random.Crt();const auto value=state.random.NextCrtRaw15();
        if(observer) {observer({Draw::Kind::Crt,before,state.random.Crt(),value});}
        category=value%3;
    }}
    ResetRecords(state,observer);
    const auto count=int32_t(state.records.size());
    const auto factor=Add(rules.land_base,Mul(Sub(2,state.categories[1]),rules.land_modifier));
    state.land_target=Mul(factor,count)/3200;
    state.continent_minimum=Mul(rules.islands,count)/3200;
    state.topology=0;state.land_progress=0;state.attempt_iterations=0;
    state.stage=Stage::ContinentLoop;
}
}

#include "SmacGenerationBootstrap.h"
#include "fixtures/SmacGenerationBootstrap.h"
#include "game/backend/map/SmacGenerationBootstrap.h"
#include "task/gsetests/GSETests.h"
#include "util/SHA256.h"
#include <string>

namespace gse::tests {
namespace {
namespace generation=game::backend::map::smac_generation;
std::array<int32_t,9> Globals(const generation::State& state) {
    return {{state.resource_pattern,state.reset_words[0],state.reset_words[1],state.reset_words[2],
        state.topology,state.land_target,state.land_progress,state.generation_flags[0],state.generation_flags[1]}};
}
std::string RecordBytes(const generation::State& state) {
    std::string bytes;bytes.reserve(state.records.size()*44);
    for(const auto& record:state.records) {bytes.append(reinterpret_cast<const char*>(record.data()),record.size());}
    return bytes;
}
}

void AddSmacGenerationBootstrapTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC generation bootstrap matches original records RNG and continuation",GT(){
        size_t records=0,draws=0;
        for(const auto& control:fixtures::generation_bootstrap::Cases()) {
            generation::State state{std::vector<generation::Record>(control.count),control.categories,
                game::backend::SmacGameplayRandomState(control.shared,control.crt)};
            for(size_t i=0;i<state.records.size();++i) {for(size_t k=0;k<44;++k) {
                state.records[i][k]=uint8_t((44*i+k)*37+control.poison);
            }}
            state.resource_pattern=control.before[0];
            state.reset_words={{control.before[1],control.before[2],control.before[3]}};
            state.topology=control.before[4];state.land_target=control.before[5];state.land_progress=control.before[6];
            state.generation_flags={{control.before[7],control.before[8]}};
            state.working_land_table.fill(-1515870811);
            state.continent_attempts=control.retry?control.carried:91;state.attempt_iterations=23;state.continent_minimum=29;
            std::vector<generation::Draw> observed;
            const generation::Observer observer=[&](const generation::Draw& draw){observed.push_back(draw);};
            if(control.reset) {generation::ResetRecords(state,observer);}
            else {generation::Bootstrap(state,control.rules,control.retry?generation::Entry::Retry:generation::Entry::First,observer);}
            GT_ASSERT(state.categories==control.resolved && Globals(state)==control.after
                && state.working_land_table==control.table,"Original generation globals/categories differ: "+std::string(control.id));
            GT_ASSERT(state.random.Shared()==control.shared_after && state.random.Crt()==control.crt_after,
                "Original generation random continuation differs: "+std::string(control.id));
            GT_ASSERT(observed.size()==control.calls.size(),"Original generation draw count differs: "+std::string(control.id));
            for(size_t i=0;i<observed.size();++i) {
                const auto& actual=observed[i];const auto& expected=control.calls[i];
                GT_ASSERT(actual.kind==expected.kind && actual.before==expected.before && actual.after==expected.after && actual.value==expected.value,
                    "Original generation draw order/value differs: "+std::string(control.id));
            }
            GT_ASSERT(util::SHA256(RecordBytes(state))==control.record_hash,
                "Complete 44-byte generation reset differs from original: "+std::string(control.id));
            std::string preserved;for(const auto& record:state.records) {
                preserved.push_back(char(record[6]));preserved.push_back(char(record[7]));
            }
            GT_ASSERT(util::SHA256(preserved)==control.preserved_hash,
                "Generation reset discarded preserved auxiliary/owner bytes: "+std::string(control.id));
            if(control.reset) {
                GT_ASSERT(state.stage==generation::Stage::Allocated,"Direct reset invented a ready continent continuation");
            } else {
                GT_ASSERT(state.continent_attempts==control.attempts && state.attempt_iterations==control.iterations
                    && state.continent_minimum==control.minimum && state.stage==generation::Stage::ContinentLoop,
                    "Original continent boundary registers/budget differ: "+std::string(control.id));
            }
            records+=state.records.size();draws+=observed.size();
        }
        GT_ASSERT(records==70921 && draws==45,"Original generation control coverage changed");GT_OK();
    });
    task->AddTest("SMAC generation bootstrap retry carries attempts and replaces resource pattern",GT(){
        generation::State state{std::vector<generation::Record>(3200),{{2,1,1,1,1,1,1}},
            game::backend::SmacGameplayRandomState(1,1)};
        const generation::Rules rules{384,256,36};
        generation::Bootstrap(state,rules,generation::Entry::First);
        GT_ASSERT(state.resource_pattern==11446 && state.random.Shared()==1015568748u
            && state.random.Crt()==1 && state.land_target==640 && state.continent_minimum==36,
            "First original generation prefix differs");
        // Supplied continuation from later growth/rejection, not a fabricated
        // claim that the missing continent stage has already executed here.
        state.continent_attempts=17;state.attempt_iterations=99;state.land_progress=400;
        state.records[0][6]=0x3c;state.records[0][7]=0x59;
        generation::Bootstrap(state,rules,generation::Entry::Retry);
        GT_ASSERT(state.resource_pattern==17133 && state.random.Shared()==1586005467u && state.random.Crt()==1,
            "Retry retained the previous pattern or consumed CRT for resolved categories");
        GT_ASSERT(state.continent_attempts==17 && state.attempt_iterations==0 && state.land_progress==0
            && state.records[0][6]==0x3c && state.records[0][7]==0x59 && state.stage==generation::Stage::ContinentLoop,
            "Retry lost carried attempts, preserved storage or per-attempt reset");GT_OK();
    });
}
}

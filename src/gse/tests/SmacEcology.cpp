#include "SmacEcology.h"
#include "fixtures/SmacEcology.h"
#include "task/gsetests/GSETests.h"
#include <algorithm>
#include <stdexcept>
#include <tuple>

namespace gse::tests {
namespace {
namespace b=game::backend::base;
auto Values(const b::SmacEcologyBurden& v) {
    return std::make_tuple(v.scaled_terraforming,v.residual_terraforming,v.remaining_clean_allowance,v.divisor,v.mining_satellite_deduction,v.burden);
}
auto Values(const b::SmacEcologyDamage& v) {
    return std::make_tuple(v.after_human_term,v.after_global_doublings,v.difficulty_factor,v.after_division_by_six,v.damage);
}
bool Overflows(const std::function<void()>& call) {try {call();}catch(const std::overflow_error&) {return true;}return false;}
}
void AddSmacEcologyTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC ecology radius scan matches35 native literal feature and boundary cases",GT(){
        for(const auto& row:smac_ecology_fixture::terraform) {
            const auto result=b::CalculateSmacTerraformingEcology(row.map,row.tree,row.hybrid,[&](int32_t x,int32_t y) {
                const auto found=std::find_if(row.tiles.begin(),row.tiles.end(),[&](const auto& tile){return tile.x==x && tile.y==y;});
                return found==row.tiles.end() ? b::SmacEcologySquare{0,false} : b::SmacEcologySquare{found->improvements,found->worked};
            });
            GT_ASSERT(result.raw_points==row.raw && result.scaled==row.scaled,"Native terraforming points differ: "+std::string(row.id));
        }GT_OK();
    });
    task->AddTest("SMAC ecology burden matches16 native signed allowance divisor and satellite cases",GT(){
        for(const auto& row:smac_ecology_fixture::burdens) {
            GT_ASSERT(Values(b::CalculateSmacEcologyBurden(row.input))==Values(row.expected),"Native burden differs: "+std::string(row.id));
        }GT_OK();
    });
    task->AddTest("SMAC ecology damage matches34 native scaling and intermediate rounding cases",GT(){
        for(const auto& row:smac_ecology_fixture::damages) {
            const auto burden=b::CalculateSmacEcologyBurden(row.burden);
            GT_ASSERT(burden.burden==row.intermediate && Values(b::CalculateSmacEcologyDamage(burden,row.input))==Values(row.expected),
                "Native final ecology differs: "+std::string(row.id));
        }GT_OK();
    });
    task->AddTest("SMAC ecology contribution matches9 native final-damage owner-pass projections",GT(){
        for(const auto& row:smac_ecology_fixture::contributions) {
            int64_t total=0;
            for(size_t i=0;i<row.damage.size();++i) {
                total+=b::SmacEcologyResearchContribution(row.damage[i],row.planet);
                GT_ASSERT(total==row.cumulative[i],"Native aggregate projection differs: "+std::string(row.id));
            }
        }GT_OK();
    });
    task->AddTest("SMAC ecology rejects unmeasured intermediate overflow before signed C++ arithmetic",GT(){
        auto burden=smac_ecology_fixture::burdens.front().input;burden.history=INT32_MAX;
        GT_ASSERT(Overflows([&]{b::CalculateSmacEcologyBurden(burden);}),"Clean allowance overflow was accepted");
        auto input=smac_ecology_fixture::damages.front().input;input.technology_score=INT32_MAX;
        const auto ordinary=b::CalculateSmacEcologyBurden(smac_ecology_fixture::damages.front().burden);
        GT_ASSERT(Overflows([&]{b::CalculateSmacEcologyDamage(ordinary,input);}),"Scaling multiplication overflow was accepted");
        GT_ASSERT(Overflows([&]{b::SmacEcologyResearchContribution(INT32_MAX,4);}),"Aggregate multiplication overflow was accepted");GT_OK();
    });
}
}

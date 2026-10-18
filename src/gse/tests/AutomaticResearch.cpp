#include "AutomaticResearch.h"
#include "game/backend/faction/SmacAutomaticResearch.h"
#include "task/gsetests/GSETests.h"
#include <stdexcept>
#include <vector>

namespace gse::tests {
namespace {
struct Case {
    const char* name;
    int32_t faction;
    bool human, blind;
    std::array<int32_t,4> priorities;
    int32_t former;
    std::vector<std::pair<int32_t,std::array<int32_t,2>>> definitions;
    std::vector<int32_t> owned;
    std::vector<std::pair<int32_t,int32_t>> values;
    uint32_t shared, crt;
    int32_t selected;
    std::vector<int32_t> calls;
    uint32_t final_shared, final_crt;
};
#include "fixtures/SmacAutomaticResearch.h"
std::string CheckAutomaticResearch() {
    using namespace game::backend;
    for (const auto& c : cases) {
        faction::SmacAutomaticResearchInputs in{};
        in.faction_id=c.faction; in.human=c.human; in.blind=c.blind;
        in.priorities=c.priorities; in.former_prerequisite=c.former;
        for (auto& row : in.technologies) { row.prerequisites={-2,-2}; }
        for (const auto& [id,prerequisites] : c.definitions) { in.technologies.at(id).prerequisites=prerequisites; }
        for (const auto id : c.owned) { in.technologies.at(id).possession |= uint32_t(1)<<c.faction; }
        std::array<int32_t,89> values{};
        for (const auto& [id,value] : c.values) { values.at(id)=value; }
        std::vector<int32_t> calls;
        SmacGameplayRandomState random(c.shared,c.crt);
        const auto selected=faction::SelectSmacAutomaticResearch(in,random,[&](int32_t id) {
            calls.push_back(id); return values.at(id);
        });
        if (selected != c.selected || calls != c.calls || random.Shared()!=c.final_shared || random.Crt()!=c.final_crt) {
            return std::string(c.name)+": target="+std::to_string(selected)+" shared="+std::to_string(random.Shared())+" CRT="+std::to_string(random.Crt());
        }
    }
    return {};
}
}
void AddAutomaticResearchTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC automatic research matches 32 original target and RNG traces", GT() {
        try { return CheckAutomaticResearch(); } catch (const std::exception& error) { GT_FAIL(error.what()); }
    });
}
}

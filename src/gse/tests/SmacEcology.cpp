#include "SmacEcology.h"
#include "fixtures/SmacEcology.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/settings/Settings.h"
#include "game/backend/WorldSnapshotRules.h"
#include "fixtures/SmacRulesSource.h"
#include "fixtures/SmacPerihelion.h"
#include "game/backend/faction/SmacEcologyWorld.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <tuple>

namespace gse::tests {
namespace {
namespace b=game::backend::base;
using game::backend::settings::SmacEcologyRules;
bool Rejects(const std::function<void()>& call) {try {call();}catch(const std::exception&) {return true;}return false;}
std::string EcologySource(const std::string& value) {
    std::string result = "#WORLDBUILDER\n; skipped comment\n## translator comment\n\n";
    for (int i = 0; i < 14; ++i) { result += std::to_string(201 + i) + ", ; preceding row\n"; }
    return result + value + "\n999, ; following row\n#END\n";
}
auto Values(const b::SmacEcologyBurden& v) {
    return std::make_tuple(v.scaled_terraforming,v.residual_terraforming,v.remaining_clean_allowance,v.divisor,v.mining_satellite_deduction,v.burden);
}
auto Values(const b::SmacEcologyDamage& v) {
    return std::make_tuple(v.after_human_term,v.after_global_doublings,v.difficulty_factor,v.after_division_by_six,v.damage);
}
bool Overflows(const std::function<void()>& call) {try {call();}catch(const std::overflow_error&) {return true;}return false;}
}
void AddSmacEcologyTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC perihelion scheduling matches20 native named maintenance controls",GT(){
        using namespace game::backend::faction;
        for(const auto& row:smac_perihelion_fixture::cases) {
            auto active=row.active;
            for(size_t i=0;i<row.states.size();++i) {
                const auto value=AdvanceSmacPerihelion(active,row.turn,row.start+row.turn,row.maintenance);
                GT_ASSERT(value.active==row.states[i] && static_cast<int>(value.transition)==row.transitions[i],
                    "Native perihelion transition differs: "+std::string(row.id));active=value.active;
            }
        }
        GT_ASSERT(Rejects([]{AdvanceSmacPerihelion(false,-1,2190,true);})
            && Rejects([]{AdvanceSmacPerihelion(true,1,INT32_MIN,true);}), "Unmeasured perihelion arithmetic admitted");GT_OK();
    });
    task->AddTest("SMAC ecology rules read positional Sea Level Rises with native loader clamp", GT() {
        for (const auto& pair : std::vector<std::pair<std::string, int32_t>>{
            {"5, ; translated label",5}, {"+19,",19}, {"-2147483648,",1}, {"0",1}, {"100,",100}, {"2147483647,",100}}) {
            const auto rules = SmacEcologyRules::Read(EcologySource(pair.first));
            GT_ASSERT(rules && rules->sea_level_rises == pair.second, "Sea Level Rises used another row or missed loader clamp");
            const auto wire = rules->Serialize().ToString();
            GT_ASSERT(SmacEcologyRules::Deserialize(types::Buffer(wire)) == *rules, "Ecology rules failed wire round trip");
            for (size_t size = 0; size < wire.size(); ++size) {
                GT_ASSERT(Rejects([&]{SmacEcologyRules::Deserialize(types::Buffer(wire.substr(0,size)));}), "Truncated ecology rules admitted");
            }
            GT_ASSERT(Rejects([&]{SmacEcologyRules::Deserialize(types::Buffer(wire + "x"));}), "Trailing ecology fields admitted");
        }
        GT_ASSERT(!SmacEcologyRules::Read("#OTHER\n5,\n"), "Absent rules invented an installed default");
        for (const auto* token : {",", "nope,", "1.5,", "2147483648,", "-2147483649,", "1,2", "+,", "5x,"}) {
            GT_ASSERT(Rejects([&]{SmacEcologyRules::Read(EcologySource(token));}), "Malformed ecology token admitted");
        }
        GT_ASSERT(Rejects([]{SmacEcologyRules::Read("#WORLDBUILDER\n5,\n");})
            && Rejects([]{SmacEcologyRules::Read(EcologySource("5,") + EcologySource("6,"));}), "Incomplete or duplicate ecology authority admitted");
        for (const auto value : {0,101}) {
            types::Buffer invalid; invalid.WriteInt(1); invalid.WriteInt(value);
            GT_ASSERT(Rejects([&]{SmacEcologyRules::Deserialize(invalid);}), "Unclamped wire authority admitted");
        }
        GT_OK();
    });
    task->AddTest("SMAC ecology host settings freeze source preserve old absence and reject partial replacement", GT() {
        using game::backend::settings::GlobalSettings;
        const auto source = [](const std::string& filename) {
            return smac_rules_fixture::Source(filename) + (filename == "alpha.txt" ? EcologySource("19,") : "");
        };
        GlobalSettings host; host.SelectRuleset("smac"); host.InitializeHostRules(source); smac_rules_fixture::AcceptWorldSize(host);
        GT_ASSERT(host.GetSmacEcologyRules() && host.GetSmacEcologyRules()->sea_level_rises == 19, "Host ignored configured ecology rule");
        const auto wire = host.Serialize().ToString(); GlobalSettings restored; restored.Deserialize(types::Buffer(wire));
        GT_ASSERT(restored.Serialize().ToString() == wire && restored.GetSmacEcologyRules()->sea_level_rises == 19, "Host transfer lost ecology rule");
        GlobalSettings absent; absent.SelectRuleset("smac"); absent.InitializeHostRules(smac_rules_fixture::Source); smac_rules_fixture::AcceptWorldSize(absent);
        GT_ASSERT(Rejects([&]{host.Deserialize(absent.Serialize());}) && host.Serialize().ToString() == wire, "Frozen profile removal partially changed settings");
        GlobalSettings changed; changed.SelectRuleset("smac"); changed.InitializeHostRules([](const std::string& filename) {
            return smac_rules_fixture::Source(filename) + (filename == "alpha.txt" ? EcologySource("20,") : "");
        }); smac_rules_fixture::AcceptWorldSize(changed);
        changed.game_name = "Must not publish";
        GT_ASSERT(Rejects([&]{host.Deserialize(changed.Serialize());}) && host.Serialize().ToString() == wire, "Frozen replacement partially changed host settings");
        namespace world = game::backend::world_snapshot;
        types::Buffer matching; world::WriteHeader(matching,host); matching.WriteString("world body");
        world::ValidateHeader(matching,restored);
        GT_ASSERT(matching.ReadString()=="world body" && matching.lenr==matching.lenw, "Ecology header consumed world body");
        types::Buffer mismatched; world::WriteHeader(mismatched,changed);
        GT_ASSERT(Rejects([&]{world::ValidateHeader(mismatched,host);}) && host.Serialize().ToString()==wire, "World accepted changed ecology rule");
        types::Buffer missing; world::WriteHeader(missing,absent);
        GT_ASSERT(Rejects([&]{world::ValidateHeader(missing,host);}), "World omitted available ecology authority");
        // Reconstruct a V33 header without the V34 absent-ecology suffix.
        auto previous=missing; previous.lenr=0; previous.dr=previous.data;
        const auto old_marker=previous.ReadInt(); previous.ReadInt();
        types::Buffer old_header; old_header.WriteInt(old_marker); old_header.WriteInt(33);
        types::Buffer no_ecology; no_ecology.WriteBool(false);
        const auto v33=old_header.ToString()+previous.ToString().substr(previous.lenr,previous.lenw-previous.lenr-no_ecology.lenw);
        types::Buffer compatible(v33); world::ValidateHeader(compatible,absent);
        GT_ASSERT(compatible.lenr==compatible.lenw, "Legacy world with absent ecology profile was not consumed");
        types::Buffer unavailable(v33);
        GT_ASSERT(Rejects([&]{world::ValidateHeader(unavailable,host);}), "Legacy world invented current ecology authority");
        auto old = absent.Serialize(); old.ReadInt(); types::Buffer version; version.WriteInt(19); types::Buffer marker; marker.WriteBool(false);
        const auto v19 = version.ToString() + old.ToString().substr(old.lenr,old.lenw-old.lenr-marker.lenw);
        GlobalSettings legacy; legacy.Deserialize(types::Buffer(v19));
        GT_ASSERT(!legacy.GetSmacEcologyRules(), "V19 restore invented ecology authority");
        host.ResetRuleset(); GT_ASSERT(!host.GetSmacEcologyRules(), "Reset retained old ecology rules");
        GlobalSettings failed; failed.SelectRuleset("smac");
        GT_ASSERT(Rejects([&]{failed.InitializeHostRules([](const std::string& filename) {
            return smac_rules_fixture::Source(filename) + (filename == "alpha.txt" ? EcologySource("broken,") : "");
        });}) && !failed.GetSmacEcologyRules() && !failed.GetSmacResourceRules(), "Failed host admission partially published rules");
        GT_OK();
    });
    if (const char* asset = std::getenv("GLSMAC_TEST_ALPHA_TXT")) {
        const std::string path = asset;
        task->AddTest("SMAC installed WORLDBUILDER ecology rule matches original loader5", GT(path) {
            std::ifstream input(path); GT_ASSERT(input.good(), "Installed alpha.txt unavailable");
            const std::string source{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
            const auto rules = SmacEcologyRules::Read(source);
            GT_ASSERT(rules && rules->sea_level_rises == 5, "Installed loaded ecology rule differs"); GT_OK();
        });
    }
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

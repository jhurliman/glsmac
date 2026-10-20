#include "SmacNativeAI.h"
#include "game/backend/unit/SmacNativeAI.h"
#include "task/gsetests/GSETests.h"
#include <yaml-cpp/yaml.h>
#include <map>
#include <stdexcept>

namespace gse::tests {
namespace {
namespace ai=game::backend::unit::smac_native_ai;
ai::Coordinate Point(const YAML::Node& n) {return {{n[0].as<int32_t>(),n[1].as<int32_t>()}};}
ai::Base Base(const YAML::Node& n) {
    return {n["id"].as<int32_t>(),Point(n["position"]),n["region"].as<uint8_t>(),n["owner"].as<uint8_t>(),
        n["c0"].as<int32_t>(),n["c4"].as<int32_t>(),n["field48"].as<int32_t>(),n["role12_count"].as<int32_t>()};
}
bool Target(const YAML::Node& input,const YAML::Node& expected) {
    std::array<int32_t,8> planet{};for(size_t i=0;i<8;++i) {planet[i]=input["planet"][i].as<int32_t>();}
    std::vector<ai::Base> bases;for(const auto& b:input["bases"]) {bases.push_back(Base(b));}
    const auto actual=ai::ChooseBase({input["width"].as<int32_t>(),input["height"].as<int32_t>(),input["wrap"].as<bool>()},
        Point(input["point"]),input["region"].as<uint8_t>(),input["home"].as<int32_t>(),input["tag"].as<uint8_t>(),planet,bases);
    if(actual.base!=expected["Base"].as<int32_t>() || actual.metric!=expected["metric"].as<int32_t>()
        || actual.terms.size()!=expected["terms"].size()) {return false;}
    for(size_t i=0;i<actual.terms.size();++i) {
        const auto& a=actual.terms[i];const auto e=expected["terms"][i];
        if(a.base!=e["Base"].as<int32_t>() || a.distance!=e["distance"].as<int32_t>()
            || a.denominator!=e["denominator"].as<int32_t>() || a.metric!=e["metric"].as<int32_t>()) {return false;}
    }return true;
}
bool Competition(const YAML::Node& input,const YAML::Node& expected) {
    std::vector<ai::Candidate> candidates;
    for(const auto& c:input["candidates"]) {
        ai::Candidate q{c["direction"].as<int32_t>(),Point(c["point"]),c["in_bounds"].as<bool>(),false,0,0,{}};
        if(q.in_bounds) {
            q.sea=c["sea"].as<bool>();q.items=c["flags"].as<uint32_t>();q.affiliation=c["affiliation"].as<uint8_t>();
            if(c["head"] && !c["head"].IsNull()) {const auto h=c["head"];q.head=ai::Head{h["owner"].as<int32_t>(),h["design"].as<int32_t>(),h["weapon_offense"].as<int32_t>(),h["triad"].as<int32_t>()};}
        }candidates.push_back(q);
    }
    const game::backend::SmacGameplayRandomState initial(input["shared"].as<uint32_t>(),input["crt"].as<uint32_t>());
    const auto actual=ai::ChooseNeighbors(input["sea"].as<bool>(),input["triad"].as<int32_t>(),input["facing"].as<uint8_t>(),
        input["turn"].as<int32_t>(),input["difficulty"].as<int32_t>(),input["state"].as<uint32_t>(),initial,candidates);
    if(actual.random.Shared()!=initial.Shared() || actual.random.Crt()!=expected["seed"].as<uint32_t>()
        || actual.level!=expected["level"].as<int32_t>() || actual.best!=expected["best_score"].as<int32_t>()
        || actual.direction!=expected["direction"].as<int32_t>() || actual.terms.size()!=expected["terms"].size()) {return false;}
    const std::map<std::string,ai::Rejection> rejections{{"bounds",ai::Rejection::Bounds},{"land_to_water",ai::Rejection::LandToWater},
        {"sea_to_land",ai::Rejection::SeaToLand},{"beaching_into_positive_Unit",ai::Rejection::BeachingIntoPositiveUnit},
        {"negative_predefined_weapon",ai::Rejection::NegativePredefinedWeapon},{"air_mismatch",ai::Rejection::AirMismatch}};
    for(size_t i=0;i<actual.terms.size();++i) {
        const auto& a=actual.terms[i];const auto e=expected["terms"][i];
        if(a.direction!=e["direction"].as<int32_t>()) {return false;}
        if(e["rejected"]) {if(a.rejection!=rejections.at(e["rejected"].as<std::string>())) {return false;}continue;}
        if(a.rejection!=ai::Rejection::None || a.base_bonus!=e["base_bonus"].as<int32_t>() || a.features!=e["features"].as<int32_t>()
            || a.item_2000_bonus!=e["road"].as<int32_t>() || a.jitter!=e["jitter"].as<int32_t>() || a.heading!=e["heading"].as<int32_t>() || a.total!=e["score"].as<int32_t>()) {return false;}
    }return true;
}
bool Carrier(const YAML::Node& input,const YAML::Node& expected) {
    const ai::Geometry g{input["width"].as<int32_t>(),input["height"].as<int32_t>(),input["wrap"].as<bool>()};
    std::map<ai::Coordinate,ai::Tile> tiles;
    for(const auto& t:input["query_tiles"]) {tiles.emplace(Point(t["point"]),ai::Tile{t["sea"].as<bool>(),t["region"].as<uint8_t>(),t["affiliation"].as<uint8_t>(),t["explored"].as<uint8_t>(),t["flags"].as<uint32_t>()});}
    const auto tile=[&](ai::Coordinate p) {return tiles.at(p);};
    std::optional<ai::Base> target;for(const auto& b:input["bases"]) {if(b["id"].as<int32_t>()==expected["target"]["Base"].as<int32_t>()) {target=Base(b);}}
    if(!target) {return false;}
    const auto point=Point(input["point"]);const auto a=ai::CarrierStep(g,point,*target,input["work"].as<int32_t>(),input["land_count"].as<int32_t>(),input["capacity"].as<int32_t>(),tile);
    const auto e=expected["step"];
    const std::map<std::string,ai::CarrierAction> actions{{"normal_competition",ai::CarrierAction::NormalCompetition},{"native_disband",ai::CarrierAction::Disband},
        {"immediate_sea_attack",ai::CarrierAction::SeaAttack},{"activate_entire_stack_and_exhaust_carrier",ai::CarrierAction::ActivateStack},{"directive",ai::CarrierAction::Directive}};
    if(a.action!=actions.at(e["action"].as<std::string>())) {return false;}
    if(e["direction"] && a.direction!=e["direction"].as<int32_t>()) {return false;}
    if(e["position"] && (!a.point || *a.point!=Point(e["position"]))) {return false;}
    if(e["observation_or"] && a.observation!=e["observation_or"].as<uint8_t>()) {return false;}
    if(a.action==ai::CarrierAction::Directive) {
        const auto d=ai::CarrierDirective(g,point,target->point,tile);const auto de=e["directive"];
        if(d.direct!=de["direct"].as<bool>() || bool(d.waypoint)!=!de["waypoint"].IsNull() || d.terms.size()!=de["terms"].size()) {return false;}
        if(d.waypoint && *d.waypoint!=Point(de["waypoint"])) {return false;}
        for(size_t i=0;i<d.terms.size();++i) {if(d.terms[i].point!=Point(de["terms"][i]["position"]) || d.terms[i].count!=de["terms"][i]["shore_count"].as<int32_t>()) {return false;}}
    }return true;
}
}
void AddSmacNativeAITests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC native AI decisions match all 41 original caller recordings",GT(){
        const auto data=YAML::LoadFile("docs/behavior/native-wildlife-decisions-cases.json");size_t cases=0,decisions=0,planner_cases=0,planner_units=0;
        for(const auto& row:data["cases"]) {
            const auto name=row["variant"].as<std::string>();const auto kind=row["kind"].as<std::string>();++cases;
            if(kind=="planner") {
                ++planner_cases;
                for(const auto& u:row["units"]) {
                    ++planner_units;
                    const auto actual=ai::PlanUpkeep(u["triad"].as<int32_t>(),u["sea"].as<bool>(),u["ordinary_base"].as<bool>(),u["flags"].as<uint32_t>(),u["order"].as<int32_t>(),u["state"].as<uint32_t>(),u["transports"].as<int32_t>());
                    const auto e=u["expected"];GT_ASSERT(actual.order==e["order"].as<int32_t>() && actual.state==(u["state"].as<uint32_t>()&0xcfffffdbu) && actual.actions.size()==e["actions"].size(),"Native upkeep differs: "+name);
                    const std::map<std::string,ai::UpkeepAction> actions{{"clear_sentry",ai::UpkeepAction::ClearSentry},{"sentry",ai::UpkeepAction::Sentry},{"delete",ai::UpkeepAction::Delete}};
                    for(size_t i=0;i<actual.actions.size();++i) {GT_ASSERT(actual.actions[i]==actions.at(e["actions"][i].as<std::string>()),"Native upkeep action order differs: "+name);}
                }continue;
            }
            for(const auto& d:row["decisions"]) {
                ++decisions;const auto input=d["input"],e=d["expected"];
                GT_ASSERT(Target(input,e["target"]),"Native weighted target differs: "+name);
                if(kind=="carrier_command") {GT_ASSERT(Carrier(input,e),"Native carrier decision differs: "+name);}
                if(e["competition"]) {GT_ASSERT(Competition(input,e["competition"]),"Native competing scores/ties/RNG differ: "+name);}
            }
        }GT_ASSERT(cases==41 && decisions==42 && planner_cases==10 && planner_units==16,"Native caller fixture coverage changed");GT_OK();
    });
    task->AddTest("SMAC native command priorities retry threshold and carrier Base connectivity retain their own callers",GT(){
        GT_ASSERT(ai::CommandPassAdmits(1,7,1,0,0,false,1,0,false) && !ai::CommandPassAdmits(1,7,1,0,0,false,0,0,false),"Carrier priority ignored live cargo count");
        GT_ASSERT(!ai::CommandPassAdmits(8,7,1,0,0,false,1,0,false) && ai::CommandPassAdmits(8,7,1,0,1,false,1,0,false),"Owner0 entered positive carrier pass");
        GT_ASSERT(!ai::CommandPassAdmits(6,4,0,0,0,true,0,0,false) && ai::CommandPassAdmits(7,4,0,0,0,false,0,0,true),"Late pass exclusions differ");
        const auto retry=ai::CommandRepeat(0,15,15,24,1,32);GT_ASSERT(!retry.finish && retry.retries==33 && retry.exhaust,"Native retry guard is not strict greater-than32");
        GT_ASSERT(ai::CommandRepeat(0,15,14,0,0,0).finish && ai::CommandRepeat(1,15,15,0,0,0).finish,"Native command continued across changed roster or handled action");
        const ai::TileQuery tile=[](ai::Coordinate p) {return ai::Tile{p[0]!=4, uint8_t(p[0]==4?1:65),15,0,0};};
        bool rejected=false;try {(void)ai::CarrierDirective({8,8,true},{0,0},{4,4},tile,3);}catch(const std::invalid_argument&) {rejected=true;}
        GT_ASSERT(rejected,"Carrier Base invented ocean connectivity");
        size_t calls=0;const auto directive=ai::CarrierDirective({8,8,true},{0,0},{4,4},tile,3,[&](int32_t base,uint8_t region) {++calls;return base==3 && region==65;});
        GT_ASSERT(calls && directive.waypoint && !directive.direct,"Carrier Base did not consult authenticated water connection");GT_OK();
    });
}
}

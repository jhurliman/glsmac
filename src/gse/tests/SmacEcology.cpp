#include "SmacEcology.h"
#include "fixtures/SmacEcology.h"
#include "fixtures/SmacBloom.h"
#include "fixtures/SmacBloomTerrain.h"
#include "fixtures/SmacBloomClass.h"
#include <map>
#include "game/backend/SmacGameplayRandomState.h"
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

#include "game/backend/base/SmacEcologyNativeSpawn.h"
#include "fixtures/SmacEcologyNativeSpawn.h"

namespace gse::tests {
namespace {
namespace b=game::backend::base;
using SpawnCoordinate=std::array<int32_t,2>;
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
    task->AddTest("SMAC ecology native spawn plan matches six original ordered sweep fallback and capacity streams",GT(){
        for(const auto& c:smac_native_spawn_fixture::cases) {
            const b::SmacNativeSpawnInput input{{{40,40},80,80,true},1,c.history,9,c.population};
            const auto plan=b::PrepareSmacNativeSpawn(input,{{{16,16},1}},[&](int32_t x,int32_t y) {
                if(x<0 || x>=80 || y<0 || y>=80 || ((x^y)&1)) {throw std::runtime_error("Invalid spawn query coordinate");}
                return b::SmacNativeSpawnTile{c.region<64?3:2,!c.no_fungus,static_cast<uint8_t>(c.region),static_cast<uint8_t>(!c.absent),false,15};
            });
            GT_ASSERT(plan.Requests().size()==c.calls.size() && plan.SweepAttempts()==c.sweep_attempts && plan.SpecialCounter()==c.special_counter,"Original sweep attempt/special counters differ");
            size_t successful=0;
            for(size_t n=0;n<c.calls.size();++n) {
                const auto& expected=c.calls[n];const auto& actual=plan.Requests()[n];
                GT_ASSERT(actual.design==expected.design && actual.coordinate==SpawnCoordinate({expected.x,expected.y})
                    && actual.sweep_attempts==expected.sweep_attempts && actual.special_counter==expected.special_counter,"Original factory attempt order or metadata differs");
                GT_ASSERT(actual.ecology_owner.has_value()==(n<static_cast<size_t>(c.sweep_attempts)),"Fallback incorrectly writes ecology-owner attribution");
                if(expected.result>=0) {
                    const auto& born=c.born.at(successful++);
                    GT_ASSERT(actual.design==born.design && actual.coordinate==SpawnCoordinate({born.x,born.y}) && actual.ecology_owner.value_or(0)==born.tag,"Captured successful birth attribution differs from plan");
                }
            }
            GT_ASSERT(successful==c.born.size() && (c.count!=2048 || (successful==0 && plan.Requests().size()==9)),"Failed factory attempts were dropped or retried as successes");
        }GT_OK();
    });
    task->AddTest("SMAC ecology native spawn keeps nearest-owner ties cached occupancy and fallback gates distinct",GT(){
        b::SmacNativeSpawnInput input{{{20,20},80,80,true},1,9,9,0};
        b::SmacNativeSpawnTile center{3,true,1,1,false,15};
        const auto query=[&](int32_t x,int32_t y) {auto tile=center;if(x!=20 || y!=20) {tile.fungus=false;}return tile;};
        auto denied=b::PrepareSmacNativeSpawn(input,{{{18,20},1},{{22,20},2}},query);
        auto allowed=b::PrepareSmacNativeSpawn(input,{{{22,20},2},{{18,20},1}},query);
        GT_ASSERT(denied.Requests().empty() && allowed.SweepAttempts()==1,"Nearest Base tie did not use later record owner");
        GT_ASSERT(b::PrepareSmacNativeSpawn(input,{{{18,20},1},{{22,20},1}},query).SweepAttempts()==1,"Different same-owner nearest Base incorrectly blocks spawning");
        for(const auto owner:{0,1,7,15}) {
            center.unit_or_base=true;center.cached_owner=owner;
            const auto plan=b::PrepareSmacNativeSpawn(input,{{{20,20},1}},query);
            GT_ASSERT(plan.SweepAttempts()==(owner==0 || owner==15?1:0),"Native0 or invalid cached owner became positive ownership");
        }
        center.unit_or_base=false;center.cached_owner=1;
        GT_ASSERT(b::PrepareSmacNativeSpawn(input,{{{20,20},1}},query).SweepAttempts()==1,"Stale positive nibble blocked an empty square");
        center.fungus=false;input.history=INT32_MAX;input.population=127;
        const auto maximum=b::PrepareSmacNativeSpawn(input,{{{20,20},1}},query);
        GT_ASSERT(maximum.SweepAttempts()==0 && maximum.Requests().size()==127,"Fallback does not use signed current population independently of sweep");
        for(const auto& request:maximum.Requests()) {GT_ASSERT(!request.ecology_owner && request.coordinate==input.map.center && request.design==8,"Fallback changed selected-square design or wrote actor tag");}
        input.population=-1;
        GT_ASSERT(b::PrepareSmacNativeSpawn(input,{{{20,20},1}},query).Requests().empty(),"Negative signed population produced fallback births");
        input.history=9;input.population=0;input.actor=7;center.fungus=true;center.unit_or_base=true;center.cached_owner=0;
        const auto native=b::PrepareSmacNativeSpawn(input,{{{20,20},7}},query);
        GT_ASSERT(native.Requests().size()==1 && native.Requests()[0].ecology_owner==7,"Actor attribution borrowed native owner0 or ignored actor7");
        GT_OK();
    });
    task->AddTest("SMAC ecology native spawn preserves ordinal clipping and strict special-design limits",GT(){
        b::SmacNativeSpawnInput input{{{0,0},32,24,true},1,7,9,0};
        const auto query=[](int32_t x,int32_t y) {
            if(x<0 || x>=32 || y<0 || y>=24 || ((x^y)&1)) {throw std::runtime_error("Clipped spawn queried outside map");}
            return b::SmacNativeSpawnTile{3,true,1,1,false,15};
        };
        const auto wrapped=b::PrepareSmacNativeSpawn(input,{{{0,0},1}},query);input.map.horizontal_wrap=false;
        const auto flat=b::PrepareSmacNativeSpawn(input,{{{0,0},1}},query);
        GT_ASSERT(wrapped.Requests().size()==6 && flat.Requests().size()==4 && wrapped.Requests()[4].coordinate==SpawnCoordinate({31,1}),"Clipping normalized wrong ordinal or reordered valid attempts");
        input.map={{20,20},80,80,true};input.population=3;input.history=9;
        const auto special=b::PrepareSmacNativeSpawn(input,{{{20,20},1}},[](int32_t,int32_t){return b::SmacNativeSpawnTile{3,true,1,0,false,15};});
        GT_ASSERT(special.SweepAttempts()==2 && special.SpecialCounter()==9 && special.Requests().size()==3 && !special.Requests().back().ecology_owner,"Skipped non-Mind-Worm candidates lost special-counter increments or fallback");
        input.retained_base_count=1;
        const auto small=b::PrepareSmacNativeSpawn(input,{{{20,20},1}},[](int32_t,int32_t){return b::SmacNativeSpawnTile{2,true,65,1,false,15};});
        GT_ASSERT(small.SweepAttempts()==0 && small.SpecialCounter()==9 && small.Requests().size()==3 && small.Requests()[0].design==9,"Strict Base-count limit was applied to fallback or skipped special counters");
        input.history=6;int queries=0;
        GT_ASSERT(Rejects([&]{b::PrepareSmacNativeSpawn(input,{{{20,20},1}},[&](int32_t,int32_t){++queries;return b::SmacNativeSpawnTile{};});}) && queries==0,"Pre-hold history reached spawn query");
        GT_OK();
    });
    task->AddTest("SMAC bloom class-only setter matches11 native exact turn rockiness and landmark controls",GT(){
        for(const auto& row:smac_bloom_class_fixture::cases) {
            const b::SmacBloomTerrainCell cell{row.before,row.rockiness,0xa5a5a5a5u,row.landmarks,7};
            game::backend::SmacGameplayRandomState rng(1234,row.seed);size_t draws=0;
            const auto after=b::ChangeSmacBloomClassOnly(cell,row.after,row.turn!=0,[&]()->int32_t {
                const auto raw=rng.NextCrtRaw15();
                if(draws>=row.draws.size() || row.draws[draws]!=raw) {throw std::runtime_error("Native class CRT differs: "+std::string(row.id));}
                ++draws;return raw;
            });
            GT_ASSERT(after.cell.altitude_class==row.after && after.cell.rockiness==row.result_rockiness
                && after.cell.landmarks==row.result_landmarks && after.cell.items==cell.items && after.cell.affiliation==7
                && after.native_dirty_mask==row.dirty && draws==row.draws.size() && rng.Crt()==row.result_seed && rng.Shared()==1234,
                "Native class-only helper differs: "+std::string(row.id));
        }
        int draws=0;
        GT_ASSERT(Rejects([&]{b::ChangeSmacBloomClassOnly({2,1,0,0,15},3,true,[&]{++draws;return 0;});}) && draws==0,
            "Cross-category conversion silently used same-category helper");
        GT_ASSERT(Rejects([&]{b::PrepareSmacBloomTerrain({{8,6},16,12,true},INT32_MAX,2,true,
            [](int32_t,int32_t){return b::SmacBloomTerrainCell{2,1,0,0,15};},[&]{++draws;return 0;});}) && draws==0,
            "History overflow consumed RNG or invoked signed overflow");GT_OK();
    });

    task->AddTest("SMAC bloom terrain reproduces26 original selection class slope item and CRT controls",GT(){
        for(const auto& row:smac_bloom_terrain_fixture::cases) {
            using Coordinate=std::array<int32_t,2>;
            std::map<Coordinate,b::SmacBloomTerrainCell> source;
            for(const auto& cell:row.cells) {source.emplace(cell.coordinate,cell.value);}
            const auto original=source;
            const auto query=[&](int32_t x,int32_t y) {
                if(x<0 || x>=row.map.width || y<0 || y>=row.map.height || ((x+y)&1)) {throw std::runtime_error("Invalid terrain fixture query");}
                const auto found=source.find({x,y});return found==source.end() ? row.default_tile : found->second;
            };
            game::backend::SmacGameplayRandomState rng(1234,row.seed);size_t draws=0;
            const auto draw=[&]()->int32_t {
                const auto raw=rng.NextCrtRaw15();
                if(draws>=row.draws.size() || raw!=row.draws[draws].raw || rng.Crt()!=row.draws[draws].after) {
                    throw std::runtime_error("Native terrain CRT sequence differs: "+std::string(row.id));
                }++draws;return raw;
            };
            GT_ASSERT(b::AdmitSmacEcologyBloom(row.damage,row.perihelion,row.primary,row.secondary,draw),"Terrain fixture rejected native entry");
            const auto winner=b::SelectSmacEcologyBloom(row.map,row.base,row.bases,[&](int32_t x,int32_t y) {
                const auto cell=query(x,y);uint32_t improvements=0;
                for(const auto bit:std::array<std::pair<uint32_t,uint32_t>,5>{{
                    {0x10,b::SmacEcologySquare::Mine},{0x40,b::SmacEcologySquare::Solar},{0x8000,b::SmacEcologySquare::Farm},
                    {0x80000,b::SmacEcologySquare::SoilEnricher},{0x200000,b::SmacEcologySquare::Forest}}}) {
                    if(cell.items&bit.first) {improvements|=bit.second;}
                }
                return b::SmacBloomTile{cell.altitude_class,bool(cell.items&0x20u),bool((cell.items&1u) && cell.affiliation<=7),improvements};
            },draw);
            GT_ASSERT(winner && winner->coordinate==row.winner && winner->ordinal==row.ordinal,"Terrain fixture selected a different cell: "+std::string(row.id));
            auto region=row.map;region.center=winner->coordinate;
            const auto plan=b::PrepareSmacBloomTerrain(region,row.history,row.difficulty,true,query,draw);
            GT_ASSERT(plan.History()==row.after_history && plan.NominalCellBudget()==row.budget && plan.NativeDirtyMask()==row.dirty
                && plan.FungusCell()==row.fungus && draws==row.draws.size() && rng.Shared()==1234 && source==original,
                "Native terrain history/budget/dirty/direct mutation/RNG differs: "+std::string(row.id));
            std::map<Coordinate,std::pair<b::SmacBloomTerrainCell,b::SmacBloomTerrainCell>> expected,actual;
            for(const auto& delta:row.changes) {expected.emplace(delta.coordinate,std::make_pair(delta.before,delta.after));}
            for(const auto& delta:plan.Changes()) {
                GT_ASSERT(actual.emplace(delta.coordinate,std::make_pair(delta.before,delta.after)).second,"Duplicate terrain delta");
            }
            GT_ASSERT(actual==expected,"Native final terrain cells differ: "+std::string(row.id));
        }GT_OK();
    });

    task->AddTest("SMAC bloom selector replays5 original sparse terrain maps and exact CRT states",GT(){
        for(const auto& row:smac_bloom_fixture::cases) {
            game::backend::SmacGameplayRandomState rng(1234,row.seed);size_t draws=0;
            const auto draw=[&]()->int32_t {
                const auto raw=rng.NextCrtRaw15();
                if(draws>=row.draws.size() || raw!=row.draws[draws].raw || rng.Crt()!=row.draws[draws].after) {
                    throw std::runtime_error("Native bloom CRT sequence differs: "+std::string(row.id));
                }++draws;return raw;
            };
            GT_ASSERT(b::AdmitSmacEcologyBloom(row.damage,row.perihelion,0,0,draw),"Native selector fixture failed entry roll");
            const auto selected=b::SelectSmacEcologyBloom(row.map,row.base,row.bases,[&](int32_t x,int32_t y) {
                if(x<0 || x>=row.map.width || y<0 || y>=row.map.height || ((x+y)&1)) {throw std::runtime_error("Invalid native fixture query");}
                for(const auto& tile:row.cells) {if(tile.x==x && tile.y==y) {return tile.value;}}
                return row.default_tile;
            },draw);
            GT_ASSERT(selected && selected->coordinate==row.selected.coordinate && selected->ordinal==row.selected.ordinal
                && selected->score==row.selected.score && draws==row.draws.size() && rng.Shared()==1234,
                "Native sparse terrain winner or RNG count differs: "+std::string(row.id));
        }GT_OK();
    });

    task->AddTest("SMAC bloom threshold matches18 native admission controls and always draws once",GT(){
        struct Row {const char* id;int32_t damage;bool perihelion;uint32_t primary,secondary;int32_t raw;bool admitted;};
        for(const auto& row:std::vector<Row>{
            {"zero",0,false,0x0u,0x0u,100,false},
            {"below",49,false,0x0u,0x0u,250,false},
            {"equal",50,false,0x0u,0x0u,250,false},
            {"above_first_bloom",51,false,0x0u,0x0u,250,true},
            {"suppressed_rule",51,false,0x200000u,0x0u,250,false},
            {"editor_hold",51,false,0x0u,0x1u,250,false},
            {"editor_playtest",51,false,0x0u,0x9u,250,true},
            {"perihelion_equal",101,true,0x0u,0x100000u,250,false},
            {"perihelion_above",102,true,0x0u,0x100000u,250,true},
            {"no_eligible",51,false,0x0u,0x0u,250,true},
            {"protected_winner",51,false,0x0u,0x0u,250,true},
            {"improvements",51,false,0x0u,0x0u,250,true},
            {"signed_landmark",51,false,0x0u,0x0u,250,true},
            {"native_voice_hold",51,false,0x0u,0x0u,250,true},
            {"native_spawn",51,false,0x0u,0x0u,250,true},
            {"native_once_hold",51,false,0x0u,0x0u,250,true},
            {"once_equal_retained",50,false,0x0u,0x0u,250,false},
            {"once_first_bloom",51,false,0x0u,0x0u,250,true},
        }) {
            int draws=0;const auto admitted=b::AdmitSmacEcologyBloom(row.damage,row.perihelion,row.primary,row.secondary,[&]{++draws;return row.raw;});
            GT_ASSERT(admitted==row.admitted && draws==1,"Native bloom threshold or draw ordering differs: "+std::string(row.id));
        }
        int draws=0;
        GT_ASSERT(Rejects([&]{b::AdmitSmacEcologyBloom(-1,false,0,0,[&]{++draws;return 0;});}) && draws==0,"Invalid damage consumed RNG");
        GT_ASSERT(Rejects([]{b::AdmitSmacEcologyBloom(1,false,0,0,[]{return -1;});})
            && Rejects([]{b::AdmitSmacEcologyBloom(1,false,0,0,[]{return 32768;});}),"Non-CRT draw admitted");GT_OK();
    });
    task->AddTest("SMAC bloom selector preserves48 native offsets strict ties clipping and nearest Base order",GT(){
        const b::SmacWorkerRadiusMap world{{32,32},64,64,true};
        const std::vector<b::SmacBloomBase> bases{{17,{32,32}}};
        // Literal original scan order, independent of the production array.
        const std::array<std::array<int32_t,2>,48> offsets{{{1,-1},{2,0},{1,1},{0,2},{-1,1},{-2,0},{-1,-1},{0,-2},{2,-2},{2,2},{-2,2},{-2,-2},{1,-3},{3,-1},{3,1},{1,3},{-1,3},{-3,1},{-3,-1},{-1,-3},{4,0},{-4,0},{0,4},{0,-4},{1,-5},{2,-4},{3,-3},{4,-2},{5,-1},{5,1},{4,2},{3,3},{2,4},{1,5},{-1,5},{-2,4},{-3,3},{-4,2},{-5,1},{-5,-1},{-4,-2},{-3,-3},{-2,-4},{-1,-5},{0,6},{6,0},{0,-6},{-6,0}}};
        for(size_t i=0;i<offsets.size();++i) {
            const std::array<int32_t,2> target{32+offsets[i][0],32+offsets[i][1]};int draws=0;
            const auto selected=b::SelectSmacEcologyBloom(world,17,bases,[&](int32_t x,int32_t y) {
                return b::SmacBloomTile{3,std::array<int32_t,2>{x,y}!=target,false,0};
            },[&]{++draws;return 999;});
            GT_ASSERT(selected && selected->coordinate==target && selected->ordinal==int32_t(i+1) && draws==1,
                "Bloom candidate coordinate/order differs at native offset "+std::to_string(i+1));
        }
        int draws=0;
        const auto none=b::SelectSmacEcologyBloom(world,17,bases,[](int32_t,int32_t){return b::SmacBloomTile{3,true,false,0};},[&]{++draws;return 99;});
        GT_ASSERT(!none && draws==0,"All fungus consumed candidate draws or selected a cell");
        const auto ties=b::SelectSmacEcologyBloom(world,17,bases,[](int32_t x,int32_t y) {
            return b::SmacBloomTile{3,!(x==33 && (y==31 || y==33)),false,0};
        },[&]{++draws;return 10;});
        GT_ASSERT(ties && ties->ordinal==1 && ties->score==10 && draws==2,"Equal score replaced earlier candidate");
        draws=0;
        const auto zero=b::SelectSmacEcologyBloom(world,17,bases,[](int32_t x,int32_t y) {
            return b::SmacBloomTile{3,!(x==33 && y==31),false,0};
        },[&]{++draws;return 0;});
        GT_ASSERT(!zero && draws==1,"Eligible score zero selected a winner");
        // An isolated outer candidate has weight1 before scaling, so its five
        // removable improvements do not receive the weight>1 branch bonus.
        const auto isolated=[](int32_t x,int32_t y){return b::SmacBloomTile{3,false,!(x==33 && y==27),572u};};
        const std::vector<b::SmacBloomBase> peer_later{{17,{32,32}},{29,{34,22}}};
        const auto blocked=b::SelectSmacEcologyBloom(world,17,peer_later,isolated,[&]{++draws;return 19;});
        GT_ASSERT(!blocked && draws==1,"Nearest Base tie did not prefer later record");
        const auto own_later=b::SelectSmacEcologyBloom(world,17,{{29,{34,22}},{17,{32,32}}},isolated,[&]{++draws;return 19;});
        GT_ASSERT(own_later && own_later->ordinal==25 && own_later->score==9 && draws==2,"Outer weight1 bonus or later Base tie differs");
        const b::SmacWorkerRadiusMap edge{{0,0},64,64,true};
        const auto wrapped=b::SelectSmacEcologyBloom(edge,17,{{17,{0,0}}},[](int32_t x,int32_t y) {
            if(x<0 || x>=64 || y<0 || y>=64 || ((x+y)&1)) {throw std::runtime_error("invalid bloom query coordinate");}
            return b::SmacBloomTile{3,!(x==63 && y==1),false,0};
        },[]{return 9;});
        GT_ASSERT(wrapped && wrapped->ordinal==5 && wrapped->coordinate[0]==63,"X wrap or Y clip differs");
        auto flat=edge;flat.horizontal_wrap=false;draws=0;
        GT_ASSERT(!b::SelectSmacEcologyBloom(flat,17,{{17,{0,0}}},[](int32_t x,int32_t y){return b::SmacBloomTile{3,!(x==63 && y==1),false,0};},[&]{++draws;return 9;}) && draws==0,
            "Flat map wrapped candidate");GT_OK();
    });
    task->AddTest("SMAC bloom selector counts semantic improvements sea rounding and fungus altitude",GT(){
        const b::SmacWorkerRadiusMap world{{32,32},64,64,true};
        for(const auto& row:std::vector<std::tuple<int32_t,uint32_t,int32_t>>{{3,0,49},{3,572,139},{2,572,69},{1,572,69}}) {
            const auto selected=b::SelectSmacEcologyBloom(world,17,{{17,{32,32}}},[&](int32_t x,int32_t y) {
                const bool target=x==33 && y==31;
                return b::SmacBloomTile{target ? std::get<0>(row) : 3,true,!target,target ? std::get<1>(row) : 0u};
            },[]{return 139;});
            // Target fungus is eligible only below class2; clear it above that.
            if(std::get<0>(row)>=2) {GT_ASSERT(!selected,"Existing surface fungus remained eligible");}
            else {GT_ASSERT(selected && selected->score==std::get<2>(row),"Deep fungus or sea rounding differs");}
            const auto cleared=b::SelectSmacEcologyBloom(world,17,{{17,{32,32}}},[&](int32_t x,int32_t y) {
                const bool target=x==33 && y==31;
                return b::SmacBloomTile{target ? std::get<0>(row) : 3,!target,!target,target ? std::get<1>(row) : 0u};
            },[]{return 139;});
            GT_ASSERT(cleared && cleared->score==std::get<2>(row),"Improvement weight/first8 bonus/sea rounding differs");
        }GT_OK();
    });

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

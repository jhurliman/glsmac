#include "SmacNativeUnitRecord.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacStartupSource.h"
#include "game/backend/Game.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/SmacOrdinaryLandExploration.h"
#include <set>
#include "game/backend/map/SmacWorkerKnowledge.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/MoraleSet.h"
#include "game/backend/unit/StaticDef.h"
#include "game/backend/unit/SpriteRender.h"
#include "game/backend/unit/CvrScoutRender.h"
#include "game/backend/State.h"
#include "game/backend/slot/Slots.h"
#include "util/FinallyGuard.h"
#include <memory>
#include "fixtures/SmacNativeUnitRecord.h"
#include "fixtures/SmacDenseUnitIdentity.h"
#include "game/backend/unit/SmacNativeUnitRecord.h"
#include "task/gsetests/GSETests.h"
#include <limits>
#include <stdexcept>
#include <vector>

namespace gse::tests {
namespace {
using Record=game::backend::unit::SmacNativeUnitRecord;
bool Rejects(const std::function<void()>& call) {try {call();}catch(const std::exception&) {return true;}return false;}
std::string Replace(const Record& record,size_t index,int64_t value) {
    auto wire=record.Serialize();std::vector<int64_t> fields;
    while(wire.lenr<wire.lenw) fields.push_back(wire.ReadInt());
    fields.at(index)=value;types::Buffer result;
    for(const auto field:fields) result.WriteInt(field);
    return result.ToString();
}
}
void AddSmacNativeUnitRecordTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC native dense index repair matches five original mixed-owner erasures",GT(){
        const auto record=[](const smac_dense_identity_fixture::Row& row) {
            auto value=Record::InitializeMindWorm(row[0],0);value.owner=row[2];value.design=row[3];value.x=row[4];value.y=row[5];
            value.stack_next=row[6];value.stack_previous=row[7];value.order=row[8];value.waypoint_x[0]=row[9];value.ecology_owner_tag=row[10];
            value.aux_word=0x1200+row[1];value.automation_control=17+row[1];value.status_character=80+row[1];
            value.waypoint_y[3]=900+row[1];value.home_base=27;return value;
        };
        for(const auto& c:smac_dense_identity_fixture::cases) {
            size_t output=0;
            for(const auto& row:c.before) {
                if(row[0]==c.deleted) {continue;}
                const auto before=record(row);const auto expected=record(c.after.at(output++));
                GT_ASSERT(before.AfterUnlinkedIndexErasure(c.deleted)==expected,"Dense repair changed survivor order, links, order payload or opaque canaries");
            }
            GT_ASSERT(output==4,"Original dense survivor count differs");
        }
        auto linked=Record::InitializeMindWorm(2,0);linked.stack_next=1;
        GT_ASSERT(Rejects([&]{linked.AfterUnlinkedIndexErasure(1);}),"Dense repair accepted a still-linked removed neighbor");
        GT_ASSERT(Rejects([&]{linked.AfterUnlinkedIndexErasure(2);}),"Erased record treated as a survivor");
        GT_OK();
    });
    task->AddTest("SMAC native dense restore rejects stale indices dangling links cycles and split stacks",GT(){
        using namespace game::backend::unit;
        for(int scenario=0;scenario<4;++scenario) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(smac_startup_fixture::OrdinarySource,"GAIANS",80,40);
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* manager=fixture.Game()->GetUM();MoraleSet::morale_values_t names;for(int n=0;n<7;++n) {names.emplace_back("level");}
                manager->DefineMoraleSet(new MoraleSet("bad-native-stack",names));
                manager->DefineUnit(new StaticDef("MindWorms",manager->GetMoraleSet("bad-native-stack"),"Mind Worms",MT_LAND,999,new SpriteRender("fixture",0,0,1,1,0,0,1)));
                types::Buffer definitions;manager->Serialize(definitions);types::Buffer loaded;loaded.WriteString(definitions.ReadString());
                for(int group=0;group<2;++group) {const auto count=definitions.ReadInt();loaded.WriteInt(count);for(int64_t n=0;n<count;++n) {loaded.WriteString(definitions.ReadString());loaded.WriteString(definitions.ReadString());}}
                GT_ASSERT(definitions.ReadInt()==0,"Malformed stack fixture already contains units");loaded.WriteInt(2);
                for(int n=0;n<2;++n) {
                    auto record=Record::InitializeMindWorm(n,0);record.x=6;record.y=2;record.stack_previous=n==0?1:-1;record.stack_next=n==1?0:-1;
                    if(scenario==0 && n==0) {record.id=2;}
                    if(scenario==1 && n==0) {record.stack_previous=2047;}
                    if(scenario==2) {record.stack_previous=1-n;record.stack_next=1-n;}
                    if(scenario==3) {record.stack_previous=record.stack_next=-1;}
                    types::Buffer unit;unit.WriteInt(5);unit.WriteInt(2);unit.WriteInt(71+n);unit.WriteString("MindWorms");unit.WriteInt(-1);
                    unit.WriteInt(6);unit.WriteInt(2);unit.WriteString(record.Serialize().ToString());loaded.WriteString(unit.ToString());
                }
                loaded.WriteInt(73);manager->Clear();
                GT_ASSERT(Rejects([&]{manager->Deserialize(GSE_CALL,loaded);}),"Malformed native reference graph admitted");
                // The manager-level fixture owns this rejected, unpublished map.
                manager->Clear();fixture.Game()->GetMap()->GetTile(6,2)->units.clear();GT_OK();
            });GT_ASSERT(error.empty(),"Malformed native dense restore: "+error);
        }GT_OK();
    });
    task->AddTest("SMAC native dense survivors commit atomically with conventional removal and restore",GT(){
        using namespace game::backend;using namespace game::backend::unit;using Knowledge=map::SmacWorkerKnowledge;
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(smac_startup_fixture::OrdinarySource,"GAIANS",80,40);
        std::string snapshot,knowledge;std::array<Record,2> expected;
        faction::SmacNativeFactoryState counts{0,{{-1,-1}},UINT32_MAX,{}};counts.active_designs[8]=254;
        const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* manager=game->GetUM();
            for(size_t y=0;y<40;++y) {for(size_t x=y&1;x<80;x+=2) {auto* tile=game->GetMap()->GetTile(x,y);for(auto* corner:tile->elevation.corners) {*corner=100;}tile->Update();}}
            MoraleSet::morale_values_t names;for(int i=0;i<7;++i) {names.emplace_back("level"+std::to_string(i));}
            manager->DefineMoraleSet(new MoraleSet("native-dense",names));
            manager->DefineUnit(new StaticDef("MindWorms",manager->GetMoraleSet("native-dense"),"Mind Worms",MT_LAND,999,new SpriteRender("fixture",0,0,1,1,0,0,1)));
            manager->DefineUnit(new StaticDef("human-dense",manager->GetMoraleSet("native-dense"),"Human",MT_LAND,999,new CvrScoutRender(2,0,100,100,{1,1},{128,128})));
            types::Buffer definitions;manager->Serialize(definitions);types::Buffer loaded;loaded.WriteString(definitions.ReadString());
            for(int group=0;group<2;++group) {const auto count=definitions.ReadInt();loaded.WriteInt(count);for(int64_t i=0;i<count;++i) {loaded.WriteString(definitions.ReadString());loaded.WriteString(definitions.ReadString());}}
            GT_ASSERT(definitions.ReadInt()==0,"Dense fixture unexpectedly contains units");loaded.WriteInt(4);
            for(int index=0;index<4;++index) {
                types::Buffer unit;const bool native=index==1 || index==2;
                unit.WriteInt(native?5:4);unit.WriteInt(native?2:1);unit.WriteInt(71+index);unit.WriteString(native?"MindWorms":"human-dense");
                unit.WriteInt(native?-1:0);unit.WriteInt(native?6:index==0?2:8);unit.WriteInt(2);
                if(native) {
                    auto record=Record::InitializeMindWorm(index,0);record.x=6;record.y=2;record.ecology_owner_tag=1;
                    record.stack_previous=index==1?2:-1;record.stack_next=index==2?1:-1;
                    record.waypoint_x[0]=3;record.order=index==1?1:0;expected[index-1]=record;
                    unit.WriteString(record.Serialize().ToString());
                } else {ConventionalUnitState{2,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0}.Serialize(unit);}
                loaded.WriteString(unit.ToString());
            }
            loaded.WriteInt(75);manager->Clear();manager->Deserialize(GSE_CALL,loaded);
            Knowledge::Data data{80,40,true,true,0,{},{},{}};data.observers[1]=Knowledge::Observer{};data.observers[1]->pact_with=0;
            data.tiles.resize(1600,{2,std::nullopt});for(uint64_t id=71;id<75;++id) {data.units.push_back({id,0,false});}
            game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
            fixture.ResourceState([&](faction::SmacSimulationState& state) {state.native_factory=counts;});
            const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();
            {auto abandoned=manager->PrepareConventionalRemoval(GSE_CALL,{71,74});}
            GT_ASSERT(fixture.Snapshot()==before && fixture.GameplayRandom()==random,"Abandoned dense plan changed live records");
            GT_ASSERT(Rejects([&]{manager->PrepareConventionalRemoval(GSE_CALL,{71,71});}) && fixture.Snapshot()==before,"Failed duplicate removal leaked native repairs");
            auto prepared=manager->PrepareConventionalRemoval(GSE_CALL,{71,74});prepared.Commit();prepared.Notify(GSE_CALL);
            expected[0].id=0;expected[0].stack_previous=1;expected[0].order=0;expected[0].waypoint_x[0]=2;
            expected[1].id=1;expected[1].stack_next=0;
            GT_ASSERT(manager->GetUnitRecordOrder()==std::vector<size_t>({72,73}) && manager->GetUnit(72)->Native()==expected[0]
                && manager->GetUnit(73)->Native()==expected[1] && fixture.GameplayRandom()==random,"Live dense erasure lost native identity/reference/history or consumed RNG");
            bool retained=false;fixture.ResourceState([&](faction::SmacSimulationState& state) {retained=state.native_factory==counts;});
            GT_ASSERT(retained,"Conventional removal recounted or debited retained native counters");
            knowledge=game->GetMap()->GetSmacWorkerKnowledge()->Serialize();snapshot=fixture.Snapshot();GT_OK();
        });GT_ASSERT(error.empty(),"Native dense removal: "+error);
        ConventionalUnitFixture restored;restored.ConfigureProduction(smac_startup_fixture::OrdinarySource,"GAIANS",80,40);
        return restored.Run([&](GSE_CALLABLE)->std::string {
            restored.Restore(GSE_CALL,snapshot,true);auto* manager=restored.Game()->GetUM();
            bool retained=false;restored.ResourceState([&](faction::SmacSimulationState& state) {retained=state.native_factory==counts;});
            GT_ASSERT(retained,"Native counter history changed on dense world restore");
            GT_ASSERT(manager->GetUnitRecordOrder()==std::vector<size_t>({72,73}) && manager->GetUnit(72)->Native()==expected[0]
                && manager->GetUnit(73)->Native()==expected[1] && restored.Game()->GetMap()->GetSmacWorkerKnowledge()->Serialize()==knowledge,"Native compacted stack failed actual world restore");GT_OK();
        });
    });
    task->AddTest("SMAC native Unit live ZOC and stationary visibility match five original movement controls",GT(){
        using namespace game::backend;using namespace game::backend::unit;using Knowledge=map::SmacWorkerKnowledge;
        struct Case {const char* name;int32_t destination,count;std::array<std::array<int32_t,2>,2> native;uint8_t pacts,spent;std::array<uint8_t,3> masks;std::set<std::pair<int32_t,int32_t>> explored;};
        const std::array<Case,5> cases{{
{"source_only",30,1,{{{{33,15}},{{0,0}}}},0,3,{{0,2,0}},{{28,16},{29,15},{29,17},{30,14},{30,18}}},
{"both",34,1,{{{{33,15}},{{0,0}}}},0,0,{{1,2,0}},{}},
{"split_reveal",34,2,{{{{31,15}},{{35,15}}}},0,0,{{1,2,2}},{{35,15}}},
{"positive_Pacts_do_not_exempt_native",34,2,{{{{31,15}},{{35,15}}}},254,0,{{1,2,2}},{{35,15}}},
{"raw_native_slot_Pact_diagnostic",34,1,{{{{33,15}},{{0,0}}}},1,3,{{1,2,0}},{{34,14},{34,18},{35,15},{35,17},{36,16}}}
        }};
        for(const auto& c:cases) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(smac_startup_fixture::OrdinarySource,"GAIANS",80,40);
            std::string snapshot,knowledge;
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* manager=game->GetUM();
                for(size_t y=0;y<40;++y) {for(size_t x=y&1;x<80;x+=2) {
                    auto* tile=game->GetMap()->GetTile(x,y);for(auto* corner:tile->elevation.corners) {*corner=100;}
                    tile->features=0;tile->terraforming=0;tile->rockiness=map::tile::ROCKINESS_FLAT;tile->Update();
                }}
                MoraleSet::morale_values_t names;for(int i=0;i<7;++i) {names.emplace_back("level"+std::to_string(i));}
                manager->DefineMoraleSet(new MoraleSet("native-zoc",names));
                manager->DefineUnit(new StaticDef("MindWorms",manager->GetMoraleSet("native-zoc"),"Mind Worms",MT_LAND,999,new SpriteRender("fixture",0,0,1,1,0,0,1)));
                manager->DefineUnit(new StaticDef("human-zoc",manager->GetMoraleSet("native-zoc"),"Human",MT_LAND,999,new CvrScoutRender(2,0,100,100,{1,1},{128,128})));
                manager->SpawnConventionalUnit(GSE_CALL,71,manager->GetUnitDef("human-zoc"),&game->GetState()->m_slots->GetSlot(0),game->GetMap()->GetTile(32,16),
                    {2,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0});
                types::Buffer current;manager->Serialize(current);types::Buffer loaded;loaded.WriteString(current.ReadString());
                for(int group=0;group<2;++group) {const auto count=current.ReadInt();loaded.WriteInt(count);
                    for(int64_t i=0;i<count;++i) {loaded.WriteString(current.ReadString());loaded.WriteString(current.ReadString());}}
                GT_ASSERT(current.ReadInt()==1,"Native ZOC fixture has wrong actor count");
                loaded.WriteInt(1+c.count);loaded.WriteString(current.ReadString());
                for(int n=0;n<c.count;++n) {
                    auto record=Record::InitializeMindWorm(1+n,0);record.x=c.native[n][0];record.y=c.native[n][1];record.observation_mask=n==0?2:0;
                    types::Buffer native;native.WriteInt(5);native.WriteInt(2);native.WriteInt(72+n);native.WriteString("MindWorms");native.WriteInt(-1);
                    native.WriteInt(record.x);native.WriteInt(record.y);native.WriteString(record.Serialize().ToString());loaded.WriteString(native.ToString());
                }
                loaded.WriteInt(72+c.count);
                // This fixture replaces the manager on the same synthetic map;
                // detach its old actor membership before retiring that registry.
                game->GetMap()->GetTile(32,16)->units.erase(71);
                manager->Clear();manager->Deserialize(GSE_CALL,loaded);auto* actor=manager->GetUnit(71);
                Knowledge::Data data{80,40,true,true,0,{},{},{}};data.observers[1]=Knowledge::Observer{};data.observers[1]->pact_with=c.pacts;
                data.tiles.resize(1600,{0,std::nullopt});data.units.push_back({71,1,false});
                const auto initial=map::smac_exploration::Explore({80,40,true,{32,16},1,std::vector<uint8_t>(1600,0),std::vector<uint8_t>(1600,0)});
                for(size_t k=0;k<data.tiles.size();++k) {data.tiles[k].explored_by=initial.explored[k];}
                for(int n=0;n<c.count;++n) {data.units.push_back({uint64_t(72+n),uint8_t(n==0?2:0),false});}
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
                const auto random=fixture.GameplayRandom();const auto before=game->GetMap()->GetSmacWorkerKnowledge()->Serialize();const auto actor_wire=Unit::Serialize(actor).ToString();
                auto* destination=game->GetMap()->GetTile(c.destination,16);manager->ValidateConventionalMove(actor,destination,0);
                GT_ASSERT(before==game->GetMap()->GetSmacWorkerKnowledge()->Serialize() && Unit::Serialize(actor).ToString()==actor_wire && fixture.GameplayRandom()==random,"Native ZOC validation published refusal/reveal effects");
                game->Event(GSE_CALL,"move_unit",{{"unit",actor->Wrap(GSE_CALL)},{"tile",destination->Wrap(GSE_CALL)}});fixture.DrainEvents();
                const auto after=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
                GT_ASSERT(actor->GetTile()->coord.x==static_cast<size_t>(c.spent?c.destination:32) && actor->Conventional().spent==c.spent && fixture.GameplayRandom()==random,"Original native ZOC movement/spending/RNG differs");
                for(size_t n=0;n<after.units.size();++n) {
                    GT_ASSERT(after.units[n].visible_to==c.masks[n],"Native ZOC changed the wrong retained observation");
                    if(n) {GT_ASSERT(manager->GetUnit(after.units[n].id)->Native().observation_mask==c.masks[n],"Native record mask not committed with knowledge");}
                }
                std::set<std::pair<int32_t,int32_t>> explored;
                for(int32_t y=0;y<40;++y) {for(int32_t x=y&1;x<80;x+=2) {const auto k=size_t(y)*40+x/2;
                    if(after.tiles[k].explored_by!=data.tiles[k].explored_by) {explored.emplace(x,y);}
                }}
                GT_ASSERT(explored==c.explored,"Native refusal or movement explored different cells");
                if(c.count==2) {const auto k=size_t(15)*40+35/2;GT_ASSERT(after.remembered[1][k] && after.remembered[1][k]->unit_present,"Native refusal omitted remembered destination Unit");}
                if(std::string(c.name)=="source_only") {
                    for(const auto x:{28,30,32}) {
                        manager->ResetConventionalSpent(GSE_CALL,actor);
                        game->Event(GSE_CALL,"move_unit",{{"unit",actor->Wrap(GSE_CALL)},{"tile",game->GetMap()->GetTile(x,16)->Wrap(GSE_CALL)}});fixture.DrainEvents();
                        const auto next=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
                        GT_ASSERT(actor->GetTile()->coord.x==static_cast<size_t>(x) && next.units[0].visible_to==(x==32?1:0)
                            && next.units[1].visible_to==2 && manager->GetUnit(72)->Native().observation_mask==2 && fixture.GameplayRandom()==random,
                            "Observer departure/return globally reset stationary native visibility");
                    }
                }
                knowledge=game->GetMap()->GetSmacWorkerKnowledge()->Serialize();snapshot=fixture.Snapshot();
                GT_OK();
            });GT_ASSERT(error.empty(),"Native ZOC "+std::string(c.name)+": "+error);
            ConventionalUnitFixture restored;restored.ConfigureProduction(smac_startup_fixture::OrdinarySource,"GAIANS",80,40);
            const auto restore_error=restored.Run([&](GSE_CALLABLE)->std::string {
                restored.Restore(GSE_CALL,snapshot,true);
                GT_ASSERT(restored.Game()->GetMap()->GetSmacWorkerKnowledge()->Serialize()==knowledge,"Native movement/refusal knowledge changed on restore");
                for(int n=0;n<c.count;++n) {
                    const auto* native=restored.Game()->GetUM()->GetUnit(72+n);
                    GT_ASSERT(native && native->IsNative() && !native->m_owner && native->Native().observation_mask==c.masks[1+n],"Native move/refusal record lost owner0 or visibility after restore");
                }GT_OK();
            });GT_ASSERT(restore_error.empty(),"Native ZOC restore "+std::string(c.name)+": "+restore_error);
        }GT_OK();
    });
    task->AddTest("SMAC native Unit actual human placement detects adjacent eight across five terrain families",GT(){
        using namespace game::backend;
        using namespace game::backend::unit;
        using Knowledge=map::SmacWorkerKnowledge;
        // Original native detection family: eight adjacent and sixteen outer
        // offsets, both produced human designs, five supplied land terrains.
        const std::array<std::array<int32_t,2>,24> offsets{{
            {{-2,0}},{{-1,-1}},{{0,-2}},{{1,-1}},{{2,0}},{{1,1}},{{0,2}},{{-1,1}},
            {{-4,0}},{{-3,-1}},{{-2,-2}},{{-1,-3}},{{0,-4}},{{1,-3}},{{2,-2}},{{3,-1}},
            {{4,0}},{{3,1}},{{2,2}},{{1,3}},{{0,4}},{{-1,3}},{{-2,2}},{{-3,1}}
        }};
        for(const bool rover:{false,true}) {for(int terrain=0;terrain<5;++terrain) {for(size_t position=0;position<offsets.size();++position) {
            ConventionalUnitFixture fixture;
            fixture.ConfigureProduction(smac_startup_fixture::OrdinarySource,rover?"SPARTANS":"GAIANS",80,40);
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* manager=game->GetUM();
                for(size_t y=0;y<40;++y) {for(size_t x=y&1;x<80;x+=2) {
                    auto* tile=game->GetMap()->GetTile(x,y);for(auto* corner:tile->elevation.corners) {*corner=100;}
                    tile->features=terrain==4?map::tile::FEATURE_XENOFUNGUS:0;
                    tile->terraforming=terrain==3?map::tile::TERRAFORMING_FOREST:0;
                    tile->rockiness=terrain==1?map::tile::ROCKINESS_ROLLING:terrain==2?map::tile::ROCKINESS_ROCKY:map::tile::ROCKINESS_FLAT;
                    tile->Update();
                }}
                MoraleSet::morale_values_t names;for(int i=0;i<7;++i) {names.emplace_back("level"+std::to_string(i));}
                manager->DefineMoraleSet(new MoraleSet("native-sight",names));
                manager->DefineUnit(new StaticDef("MindWorms",manager->GetMoraleSet("native-sight"),"Mind Worms",MT_LAND,999,new SpriteRender("fixture",0,0,1,1,0,0,1)));
                manager->DefineUnit(new StaticDef("human-sight",manager->GetMoraleSet("native-sight"),"Human",MT_LAND,999,
                    new CvrScoutRender(2,0,100,100,{1,1},{128,128},rover?CvrScoutRender::Composition::SpartanRover:CvrScoutRender::Composition::Scout)));
                auto record=Record::InitializeMindWorm(0,0);record.x=32+offsets[position][0];record.y=16+offsets[position][1];
                types::Buffer native;native.WriteInt(5);native.WriteInt(2);native.WriteInt(71);native.WriteString("MindWorms");
                native.WriteInt(-1);native.WriteInt(record.x);native.WriteInt(record.y);native.WriteString(record.Serialize().ToString());
                types::Buffer current;manager->Serialize(current);types::Buffer loaded;loaded.WriteString(current.ReadString());
                for(int group=0;group<2;++group) {const auto count=current.ReadInt();loaded.WriteInt(count);
                    for(int64_t i=0;i<count;++i) {loaded.WriteString(current.ReadString());loaded.WriteString(current.ReadString());}}
                GT_ASSERT(current.ReadInt()==0,"Detection fixture unexpectedly contains Units");
                loaded.WriteInt(1);loaded.WriteString(native.ToString());loaded.WriteInt(72);manager->Clear();manager->Deserialize(GSE_CALL,loaded);
                Knowledge::Data data{80,40,true,true,0,{},{},{}};data.observers[1]=Knowledge::Observer{};data.observers[1]->pact_with=0;
                data.tiles.resize(1600,{0,std::nullopt});data.units.push_back({71,0,false});
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
                const auto random=fixture.GameplayRandom();const auto before=game->GetMap()->GetSmacWorkerKnowledge()->Serialize();
                auto prepared=manager->PrepareConventionalUnit(GSE_CALL,72,manager->GetUnitDef("human-sight"),&game->GetState()->m_slots->GetSlot(0),
                    game->GetMap()->GetTile(32,16),{rover?64:2,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0});
                GT_ASSERT(before==game->GetMap()->GetSmacWorkerKnowledge()->Serialize() && manager->GetUnit(71)->Native()==record && !manager->GetUnit(72),"Native sight preparation published entities or visibility");
                prepared.Commit();
                const auto after=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
                const auto detected=position<8;
                GT_ASSERT(after.units.size()==2 && after.units[0].visible_to==(detected?2:0) && after.units[1].visible_to==(detected?1:0)
                    && manager->GetUnit(71)->Native().observation_mask==(detected?2:0) && !manager->GetUnit(71)->m_owner
                    && fixture.GameplayRandom()==random,"Actual human placement differs from original native detection family or consumes RNG");
                GT_OK();
            });
            GT_ASSERT(error.empty(),"Native sight rover="+std::to_string(rover)+" terrain="+std::to_string(terrain)+" offset="+std::to_string(position)+": "+error);
        }}}
        GT_OK();
    });
    task->AddTest("SMAC native Unit world snapshot restores faction zero and retained observation authority",GT(){
        std::string snapshot,unit_wire,knowledge_wire;
        ConventionalUnitFixture fixture;
        fixture.ConfigureProduction(smac_startup_fixture::OrdinarySource,"GAIANS",80,40);
        const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
            using namespace game::backend::unit;
            using Knowledge=game::backend::map::SmacWorkerKnowledge;
            auto* game=fixture.Game();auto* manager=game->GetUM();
            for(size_t y=0;y<40;++y) {for(size_t x=y&1;x<80;x+=2) {
                auto* tile=game->GetMap()->GetTile(x,y);
                for(auto* corner:tile->elevation.corners) {*corner=100;}tile->Update();
            }}
            MoraleSet::morale_values_t names;
            for(const auto* name:{"Hatchling","Larval Mass","Pre-Boil","Boil","Mature Boil","Great Boil","Demon Boil"}) {names.emplace_back(name);}
            manager->DefineMoraleSet(new MoraleSet("NATIVE_MORALE",names));
            manager->DefineUnit(new StaticDef("MindWorms",manager->GetMoraleSet("NATIVE_MORALE"),"Mind Worms",MT_LAND,999,
                new SpriteRender("fixture",0,0,1,1,0,0,1)));
            // Supplied saved native record, not an ecology-birth or sight-producer claim.
            auto record=smac_native_record_fixture::records.front();record.observation_mask=2;
            types::Buffer unit;unit.WriteInt(5);unit.WriteInt(2);unit.WriteInt(71);unit.WriteString("MindWorms");
            unit.WriteInt(-1);unit.WriteInt(record.x);unit.WriteInt(record.y);unit.WriteString(record.Serialize().ToString());
            types::Buffer current;manager->Serialize(current);types::Buffer loaded;loaded.WriteString(current.ReadString());
            for(int group=0;group<2;++group) {
                const auto count=current.ReadInt();loaded.WriteInt(count);
                for(int64_t i=0;i<count;++i) {loaded.WriteString(current.ReadString());loaded.WriteString(current.ReadString());}
            }
            GT_ASSERT(current.ReadInt()==0,"Native fixture unexpectedly contains Units");
            loaded.WriteInt(1);loaded.WriteString(unit.ToString());loaded.WriteInt(std::max<int64_t>(72,current.ReadInt()));
            manager->Clear();manager->Deserialize(GSE_CALL,loaded);
            auto* native=manager->GetUnit(71);
            GT_ASSERT(native && native->Native()==record && native->SimulationOwner()==0 && !native->m_owner,
                "Production manager restore did not retain independent native ownership");
            Knowledge::Data data{80,40,true,true,0,{},{},{}};
            data.observers[1]=Knowledge::Observer{};data.observers[1]->pact_with=0;
            data.tiles.resize(1600,{2,std::nullopt});data.units.push_back({71,2,false});
            game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
            auto stale=data;stale.units[0].visible_to=0;
            GT_ASSERT(Rejects([&]{game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(stale));})
                && native->Native().observation_mask==2,"Mismatched native observation was silently accepted");
            fixture.CommitTechnologyReveal(Knowledge::Create(stale));
            GT_ASSERT(native->Native().observation_mask==0,"Sight publication left native record visibility stale");
            game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(stale));
            unit_wire=Unit::Serialize(native).ToString();knowledge_wire=game->GetMap()->GetSmacWorkerKnowledge()->Serialize();
            snapshot=fixture.Snapshot();GT_OK();
        });
        if(!error.empty()) {return error;}
        ConventionalUnitFixture restored;
        restored.ConfigureProduction(smac_startup_fixture::OrdinarySource,"GAIANS",80,40);
        return restored.Run([&](GSE_CALLABLE)->std::string {
            restored.Restore(GSE_CALL,snapshot,true);
            const auto* native=restored.Game()->GetUM()->GetUnit(71);
            GT_ASSERT(native && native->IsNative() && !native->m_owner && native->SimulationOwner()==0
                && game::backend::unit::Unit::Serialize(native).ToString()==unit_wire
                && restored.Game()->GetMap()->GetSmacWorkerKnowledge()->Serialize()==knowledge_wire,
                "World restore lost native state, identity or retained visibility");
            GT_OK();
        });
    });
    task->AddTest("SMAC native Unit live record roundtrip keeps owner zero separate from controllers and ecology",GT(){
        ConventionalUnitFixture fixture;
        fixture.ConfigureProduction(smac_startup_fixture::OrdinarySource,"GAIANS",80,40);
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            using namespace game::backend::unit;
            auto* game=fixture.Game();auto* manager=game->GetUM();
            MoraleSet::morale_values_t names;
            for(const auto* name:{"Hatchling","Larval Mass","Pre-Boil","Boil","Mature Boil","Great Boil","Demon Boil"}) {names.emplace_back(name);}
            manager->DefineMoraleSet(new MoraleSet("NATIVE_MORALE",names));
            manager->DefineUnit(new StaticDef("MindWorms",manager->GetMoraleSet("NATIVE_MORALE"),"Mind Worms",MT_LAND,999,
                new SpriteRender("fixture",0,0,1,1,0,0,1)));
            const auto random=fixture.GameplayRandom();
            for(const auto& literal:smac_native_record_fixture::records) {
                auto* tile=game->GetMap()->GetTile(literal.x,literal.y);
                for(auto* corner:tile->elevation.corners) {*corner=100;}tile->Update();
                types::Buffer wire;wire.WriteInt(5);wire.WriteInt(2);wire.WriteInt(71);wire.WriteString("MindWorms");
                wire.WriteInt(-1);wire.WriteInt(literal.x);wire.WriteInt(literal.y);wire.WriteString(literal.Serialize().ToString());
                auto decoded=wire;
                std::unique_ptr<Unit> unit(Unit::Deserialize(GSE_CALL,decoded,manager));
                util::FinallyGuard cleanup([&]{tile->units.erase(unit->m_id);});
                GT_ASSERT(unit->IsNative() && !unit->IsConventional() && !unit->m_owner && unit->Native()==literal,
                    "Native live state borrowed a controller, dropped fields or lost attribution");
                GT_ASSERT(Unit::Serialize(unit.get()).ToString()==wire.ToString(),"Live Unit record did not preserve complete native state");
                const auto presentation=unit->GetPresentation();
                GT_ASSERT(presentation.movement==1 && presentation.full_movement==1 && presentation.health==1
                    && presentation.morale==0 && unit->HasMovesLeft(),"Native display used stored floats or stored morale");
                const auto stats=manager->QueryNativeUnit(unit->Native());
                GT_ASSERT(stats.quantum==3 && stats.full_allowance==3 && stats.hit_points==10,
                    "Native stats did not resolve frozen produced design components");
                GT_ASSERT(Rejects([&]{unit->Legacy();}),"Native record exposed mutable legacy float state");
            }
            GT_ASSERT(fixture.GameplayRandom()==random,"Native record load/save/query consumed gameplay RNG");
            GT_OK();
        });
    });
    task->AddTest("SMAC native Unit complete record retains ten original ecology births and source ID zero",GT(){
        for(const auto& literal:smac_native_record_fixture::records) {
            auto initialized=Record::InitializeMindWorm(literal.id,0);
            GT_ASSERT(initialized.x==-4 && initialized.y==-4 && initialized.owner==0 && initialized.ecology_owner_tag==0,
                "Initializer conflated native owner with ecology attribution or placed early");
            initialized.x=literal.x;initialized.y=literal.y;initialized.ecology_owner_tag=literal.ecology_owner_tag;
            initialized.observation_mask=literal.observation_mask;
            GT_ASSERT(initialized==literal,"Complete original record differs after admitted placement/tag writes");
            GT_ASSERT(Record::Deserialize(literal.Serialize().ToString())==literal,"Native record codec dropped a field or sentinel");
        }
        GT_ASSERT(smac_native_record_fixture::records.front().id==0 && smac_native_record_fixture::records.front().owner==0,
            "Native identity zero was treated as absence");
        for(const auto& sample:std::vector<std::pair<int32_t,uint8_t>>{{-2,255},{-1,0},{0,1},{256,1}}) {
            const auto record=Record::InitializeMindWorm(0,sample.first);
            GT_ASSERT(record.stored_morale==sample.second,"Native morale byte wrapping changed");
        }
        GT_OK();
    });
    task->AddTest("SMAC wild Mind Worm lifecycle matches original existing Unit counter queries",GT(){
        using game::backend::unit::SmacWildMindWormLevel;
        auto record=smac_native_record_fixture::records.front();const auto before=record;
        for(const auto& row:smac_native_record_fixture::levels) {
            GT_ASSERT(SmacWildMindWormLevel(record,row.turn,0,3)==row.neutral
                && SmacWildMindWormLevel(record,row.turn,1,3)==row.opposed,"Native global-turn level/context differs");
        }
        for(const auto& row:smac_native_record_fixture::base_levels)
            GT_ASSERT(SmacWildMindWormLevel(record,row.first,0,0)==row.second,"Initialized native lifecycle boundary differs");
        record.stored_morale=255;record.ecology_owner_tag=7;
        GT_ASSERT(SmacWildMindWormLevel(record,90,0,0)==2 && SmacWildMindWormLevel(record,90,1,0)==2,
            "Stored morale or ecology tag changed native level");
        record=before;record.state=1;
        GT_ASSERT(Rejects([&]{SmacWildMindWormLevel(record,90,0,0);}),"Unmeasured native state marker silently used baseline morale");
        record=before;record.owner=1;
        GT_ASSERT(Rejects([&]{SmacWildMindWormLevel(record,90,0,0);}),"Captured faction-owned native used wild lifecycle");
        GT_OK();
    });
    task->AddTest("SMAC native Unit codec preserves nonzero controls and rejects narrowing and malformed records",GT(){
        auto record=Record::InitializeMindWorm(2047,0);
        record.x=32767;record.y=32767;record.state=UINT32_MAX;record.flags=UINT16_MAX;record.aux_word=0x1234;
        record.owner=7;record.aux_owner_control=0xa5;record.damage=9;record.order=0xfe;record.automation_control=0x81;
        record.waypoint_count=4;record.waypoint_x={{-32768,-1,0,32767}};record.waypoint_y={{32767,0,-1,-32768}};
        record.stored_morale=255;record.terraforming_progress=201;record.ecology_owner_tag=6;record.observation_mask=0xff;
        record.movement_spent=254;record.facing=7;record.movement_iteration_control=0x80;record.status_character=0x7f;
        record.probe_action_control=0x34;record.probe_target_control=0x56;record.home_base=0;record.stack_next=0;record.stack_previous=2046;
        const auto wire=record.Serialize().ToString();
        GT_ASSERT(wire.size()<=Record::MAX_WIRE_BYTES && Record::Deserialize(wire)==record,
            "Nonzero auxiliary/waypoint/identity controls were normalized on reload");
        GT_ASSERT(Rejects([&]{Record::Deserialize(wire+"x");}) && Rejects([&]{Record::Deserialize(wire.substr(0,wire.size()-1));})
            && Rejects([&]{Record::Deserialize(std::string(Record::MAX_WIRE_BYTES+1,'x'));}),"Malformed record boundary accepted");
        // Framed integer replacement keeps checksums valid and reaches our field admission.
        for(const auto& sample:std::vector<std::pair<size_t,int64_t>>{{0,2},{1,-1},{4,int64_t(UINT32_MAX)+1},
            {5,65536},{8,8},{10,256},{26,2048},{27,32768}}) {
            GT_ASSERT(Rejects([&]{Record::Deserialize(Replace(record,sample.first,sample.second));}),"Out-of-range scalar narrowed or invalid identity accepted");
        }
        GT_OK();
    });
}
}

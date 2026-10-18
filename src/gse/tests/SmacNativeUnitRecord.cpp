#include "SmacNativeUnitRecord.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacStartupSource.h"
#include "game/backend/Game.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/SmacWorkerKnowledge.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/MoraleSet.h"
#include "game/backend/unit/StaticDef.h"
#include "game/backend/unit/SpriteRender.h"
#include "util/FinallyGuard.h"
#include <memory>
#include "fixtures/SmacNativeUnitRecord.h"
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

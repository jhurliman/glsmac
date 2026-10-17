#include "ConventionalWork.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacStartupSource.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/Random.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tile.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/Base.h"
#include "game/backend/resource/Resource.h"
#include "game/backend/resource/ResourceManager.h"
#include "gse/value/Callable.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/StaticDef.h"
#include "game/backend/unit/MoraleSet.h"
#include "game/backend/unit/FormerMarkerRender.h"
#include "game/backend/unit/CvrScoutRender.h"
#include "gse/value/Int.h"
#include <sstream>

namespace gse::tests {
namespace {
using namespace game::backend;
using namespace game::backend::unit;
using Order=ConventionalUnitState::WorkOrder;
bool Rejects(const std::function<void()>& call){try{call();}catch(const std::exception&){return true;}return false;}
std::string Source(const std::string& name) {
    const auto source=smac_startup_fixture::Source(name);if(name!="alpha.txt"){return source;}
    std::istringstream input(source);std::string line,section,result;
    while(std::getline(input,line)) {
        if(!line.empty()&&line[0]=='#'){section=line;}
        else if(section=="#RESOURCEINFO"&&!line.empty()){line="Fixture,1,1,1";}
        else if(section=="#TERRAIN"&&!line.empty()) {
            std::istringstream row(line);std::string field;std::vector<std::string> fields;
            while(std::getline(row,field,',')){fields.push_back(field);}
            if(fields.size()>=8){fields[1]="None";fields[4]="4";line.clear();for(size_t i=0;i<fields.size();++i){if(i){line+=",";}line+=fields[i];}}
        }
        result+=line+"\n";
    }return result;
}
Unit* Populate(GSE_CALLABLE,ConventionalUnitFixture& fixture,size_t helpers=0,bool with_base=false) {
    auto* game=fixture.Game();auto* map=game->GetMap();auto* um=game->GetUM();
    for(size_t y=0;y<map->GetHeight();++y){for(size_t x=y&1;x<map->GetWidth();x+=2){
        auto* tile=map->GetTile(x,y);for(auto* corner:tile->elevation.corners){*corner=100;}
        tile->features=0;tile->terraforming=0;tile->rockiness=map::tile::ROCKINESS_FLAT;tile->moisture=map::tile::MOISTURE_ARID;tile->bonus=map::tile::BONUS_NONE;tile->Update();
    }}
    MoraleSet::morale_values_t names;for(int i=0;i<7;++i){names.emplace_back("level"+std::to_string(i));}
    um->DefineMoraleSet(new MoraleSet("work-morale",names));
    auto* def=new StaticDef("work-former",um->GetMoraleSet("work-morale"),"Former",MT_LAND,1,new FormerMarkerRender());um->DefineUnit(def);
    auto* owner=&game->GetState()->m_slots->GetSlot(0);auto* tile=map->GetTile(16,16);
    ConventionalUnitState record{1,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0};
    auto* actor=um->SpawnConventionalUnit(GSE_CALL,71,def,owner,tile,record);
    for(size_t i=0;i<helpers;++i){um->SpawnConventionalUnit(GSE_CALL,72+i,def,owner,tile,record);}
    std::array<std::string,8> roles;roles[0]="work-worker";
    game->GetBM()->DefinePop(new base::PopDef(roles[0],"Worker",{},{},base::PopDef::PF_TILE_WORKER));
    for(uint8_t i=0;i<7;++i){roles[i+1]="work-specialist"+std::to_string(i);game->GetBM()->DefinePop(new base::PopDef(roles[i+1],"Specialist",{},{},base::PopDef::PF_NONE,i));}
    fixture.BasePresentationRoles(roles);
    if(with_base){
        for(const auto* id:{"NUTRIENTS","MINERALS","ENERGY"}){game->GetRM()->DefineResource(new game::backend::resource::Resource(id,id,{}));}
        fixture.ResourcePolicy(true);
        base::Base::pops_t pops;pops.emplace(1,base::Pop(nullptr,1,game->GetBM()->GetPopDef(roles[0]),0,tile));
        base::SmacBaseAllocationState allocation{5,{{},0}};
        auto* base=new base::Base(game,901,owner,owner->GetPlayer()->GetFaction(),map->GetTile(14,16),"Work consumer",{},pops,2,0,{},base::SmacBaseResourceContext{1,false,0,0,0,false},allocation);
        game->GetBM()->SpawnBase(GSE_CALL,base);
    }
    map::SmacWorkerKnowledge::Data data{int32_t(map->GetWidth()),int32_t(map->GetHeight()),true,true,0,{},{},{}};
    data.tiles.resize(map->GetWidth()*map->GetHeight()/2,{2,std::nullopt});data.observers[1]=map::SmacWorkerKnowledge::Observer{};data.observers[1]->pact_with=0;
    data.units={{71,0,false}};for(size_t i=0;i<helpers;++i){data.units.push_back({72+i,0,false});}
    data.remembered[1].resize(data.tiles.size(),map::SmacWorkerKnowledge::Improvements{0,0,false,false});
    game->PublishSmacWorkerKnowledge(GSE_CALL,map::SmacWorkerKnowledge::Create(data));fixture.Updates();return actor;
}
void Event(GSE_CALLABLE,ConventionalUnitFixture& fixture,Unit* actor,Order order,bool dispatch=false) {
    auto* game=fixture.Game();
    if(dispatch){game->Event(GSE_CALL,"continue_unit_work",{{"unit",actor->Wrap(GSE_CALL)},{"turn_id",VALUE(value::Int,,game->GetTurnId())}});}
    else if(order==Order::None){game->Event(GSE_CALL,"cancel_unit_work",{{"unit",actor->Wrap(GSE_CALL)}});}
    else{game->Event(GSE_CALL,"work_unit",{{"unit",actor->Wrap(GSE_CALL)},{"action",VALUE(value::Int,,static_cast<int32_t>(order))}});}
    fixture.DrainEvents();
}
}
void AddConventionalWorkTests(task::gsetests::GSETests* task) {
    task->AddTest("Former selected controller latches actual work and retires queued visits",GT(){
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(Source,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string{
            auto* actor=Populate(GSE_CALL,fixture);Event(GSE_CALL,fixture,actor,Order::Farm);
            return fixture.CheckSelectedWorkController(GSE_CALL,actor);
        });
    });
    task->AddTest("Former selected controller turn stamp rejects stale work after real upkeep",GT(){
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(Source,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string{
            auto* actor=Populate(GSE_CALL,fixture);auto* game=fixture.Game();Event(GSE_CALL,fixture,actor,Order::Farm);
            const auto old_turn=game->GetTurnId();
            game->Event(GSE_CALL,"complete_turn",{});
            GT_ASSERT(game->QueueSelectedConventionalWork(GSE_CALL,actor->m_id,old_turn),"current selected work was not queued behind upkeep");
            fixture.DrainEvents();
            GT_ASSERT(game->GetTurnId()==old_turn+1&&actor->Conventional().spent==0&&actor->Conventional().work_progress==0,
                "old controller visit spent the next turn's allowance");
            GT_ASSERT(!game->QueueSelectedConventionalWork(GSE_CALL,actor->m_id,old_turn),"stale frontend turn queued work");
            GT_ASSERT(Rejects([&]{game->Event(GSE_CALL,"continue_unit_work",{{"unit",actor->Wrap(GSE_CALL)}});}),"unstamped continuation admitted");
            GT_ASSERT(game->QueueSelectedConventionalWork(GSE_CALL,actor->m_id,game->GetTurnId()),"fresh selected work did not queue");fixture.DrainEvents();
            GT_ASSERT(actor->Conventional().spent==3&&actor->Conventional().work_progress==2,"fresh controller visit failed");
            GT_ASSERT(!game->QueueSelectedConventionalWork(GSE_CALL,actor->m_id,game->GetTurnId())
                &&!game->QueueSelectedConventionalWork(GSE_CALL,9999,game->GetTurnId()),"exhausted or missing Unit queued work");GT_OK();
        });
    });
    task->AddTest("Former work actual command dispatch upkeep and completion preserve repeat guard",GT(){
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(Source,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string{
            auto* actor=Populate(GSE_CALL,fixture);auto* game=fixture.Game();auto* tile=actor->GetTile();
            auto state=actor->Conventional();state.spent=1;game->GetUM()->UpdateConventionalUnit(GSE_CALL,actor,state);fixture.Updates();
            const auto random=fixture.GameplayRandom();const auto jsf=game->GetRandom()->GetStateString();
            Event(GSE_CALL,fixture,actor,Order::Farm);
            GT_ASSERT(actor->Conventional().work_order==Order::Farm&&actor->Conventional().work_progress==0&&actor->Conventional().spent==1,"request advanced work or changed spent");
            for(int step=1;step<=4;++step){
                Event(GSE_CALL,fixture,actor,Order::None,true);
                GT_ASSERT(actor->Conventional().spent==3&&actor->Conventional().work_progress==(step==4?0:step*2),"actual dispatch did not grant one full ordinary contribution");
                fixture.Updates();const auto before=fixture.Snapshot();Event(GSE_CALL,fixture,actor,Order::None,true);
                GT_ASSERT(fixture.Snapshot()==before&&fixture.Updates().empty(),"same-turn repeat changed authority or presentation");
                if(step!=4){game->Event(GSE_CALL,"complete_turn",{});fixture.DrainEvents();GT_ASSERT(actor->Conventional().spent==0&&actor->Conventional().work_progress==step*2,"upkeep advanced work or discarded progress");}
            }
            const auto memory=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();const auto index=size_t(tile->coord.y)*(memory.width/2)+tile->coord.x/2;
            GT_ASSERT(actor->Conventional().work_order==Order::None&&(tile->terraforming&map::tile::TERRAFORMING_FARM)
                &&memory.remembered[1][index]->terraforming==tile->terraforming,"completion lost terrain or acting-owner memory");
            GT_ASSERT(fixture.GameplayRandom()==random&&game->GetRandom()->GetStateString()==jsf,"work spent an unrelated random stream");GT_OK();
        });
    });
    task->AddTest("Former work five installed action times reach actual terrain completion",GT(){
        const std::array<Order,5> actions{Order::Farm,Order::Road,Order::Forest,Order::Solar,Order::Mine};
        const std::array<int,5> turns{4,1,4,4,8};
        const std::array<uint16_t,5> bits{map::tile::TERRAFORMING_FARM,map::tile::TERRAFORMING_ROAD,map::tile::TERRAFORMING_FOREST,map::tile::TERRAFORMING_SOLAR,map::tile::TERRAFORMING_MINE};
        for(size_t i=0;i<actions.size();++i){
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(smac_startup_fixture::Source,"GAIANS");
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string{
                auto* actor=Populate(GSE_CALL,fixture);Event(GSE_CALL,fixture,actor,actions[i]);
                for(int step=1;step<=turns[i];++step){
                    if(step>1){fixture.Game()->GetUM()->ResetConventionalSpent(GSE_CALL,actor);}
                    Event(GSE_CALL,fixture,actor,Order::None,true);
                    GT_ASSERT(actor->Conventional().spent==3&&actor->Conventional().work_progress==(step==turns[i]?0:step*2),"active source time did not control work threshold");
                    GT_ASSERT(actor->GetTile()->terraforming==(step==turns[i]?bits[i]:0),"terrain appeared at the wrong work boundary");
                }
                GT_ASSERT(actor->Conventional().work_order==Order::None,"completion left an active order");GT_OK();
            });if(!error.empty()){return error;}
        }GT_OK();
    });
    task->AddTest("Former work cancellation reassignment movement and saved continuation retain bytes",GT(){
        ConventionalUnitFixture source;source.ConfigureProduction(Source,"GAIANS");std::string snapshot;
        const auto first=source.Run([&](GSE_CALLABLE)->std::string{
            auto* actor=Populate(GSE_CALL,source);Event(GSE_CALL,source,actor,Order::Farm);Event(GSE_CALL,source,actor,Order::None,true);
            Event(GSE_CALL,source,actor,Order::None);
            GT_ASSERT(actor->Conventional().work_order==Order::None&&actor->Conventional().work_progress==2&&actor->Conventional().spent==3,"menu cancellation refunded or removed work");
            source.Game()->Event(GSE_CALL,"complete_turn",{});source.DrainEvents();source.MoveEvent(GSE_CALL,actor,actor->GetTile()->E);
            Event(GSE_CALL,source,actor,Order::Solar);Event(GSE_CALL,source,actor,Order::None,true);
            GT_ASSERT(actor->Conventional().work_order==Order::Solar&&actor->Conventional().work_progress==2&&actor->Conventional().spent==3,"move or same-turn reassignment changed work");
            snapshot=source.Snapshot();GT_OK();
        });if(!first.empty()){return first;}
        ConventionalUnitFixture restored;restored.ConfigureProduction(Source,"GAIANS");
        return restored.Run([&](GSE_CALLABLE)->std::string{
            restored.Restore(GSE_CALL,snapshot);restored.Updates();auto* game=restored.Game();auto* actor=game->GetUM()->GetUnit(71);
            GT_ASSERT(actor->Conventional().work_progress==2&&actor->Conventional().work_order==Order::Solar,"restore replayed or lost active work");
            for(int step=0;step<3;++step){game->GetUM()->ResetConventionalSpent(GSE_CALL,actor);Event(GSE_CALL,restored,actor,Order::None,true);}
            GT_ASSERT(actor->GetTile()==game->GetMap()->GetTile(18,16)&&actor->GetTile()->terraforming==map::tile::TERRAFORMING_SOLAR
                &&game->GetMap()->GetTile(16,16)->terraforming==0,"restored work completed at its old coordinate");
            game->GetUM()->ResetConventionalSpent(GSE_CALL,actor);restored.MoveEvent(GSE_CALL,actor,actor->GetTile()->E);
            GT_ASSERT(actor->GetTile()==game->GetMap()->GetTile(20,16),"completed ordinary improvement trapped Former movement");GT_OK();
        });
    });
    task->AddTest("Former work matching stack progress wraps and completion clears helpers only",GT(){
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(Source,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string{
            auto* actor=Populate(GSE_CALL,fixture,true);auto* game=fixture.Game();auto* helper=game->GetUM()->GetUnit(72);
            auto a=actor->Conventional(),b=helper->Conventional();a.work_order=b.work_order=Order::Farm;a.work_progress=254;b.work_progress=2;b.spent=1;
            game->GetUM()->UpdateConventionalUnit(GSE_CALL,actor,a);game->GetUM()->UpdateConventionalUnit(GSE_CALL,helper,b);
            Event(GSE_CALL,fixture,actor,Order::None,true);
            GT_ASSERT(actor->Conventional().work_progress==2&&helper->Conventional().work_progress==0&&helper->Conventional().spent==1&&helper->Conventional().work_order==Order::Farm,"pooling was not byte-wrapped or consumed helper movement");
            a=actor->Conventional();a.work_progress=6;a.spent=0;game->GetUM()->UpdateConventionalUnit(GSE_CALL,actor,a);
            Event(GSE_CALL,fixture,actor,Order::None,true);
            GT_ASSERT(actor->Conventional().work_order==Order::None&&helper->Conventional().work_order==Order::None&&helper->Conventional().spent==1,"completion did not clear matching orders without helper movement");GT_OK();
        });
    });
    task->AddTest("Former work captured three-worker collaboration preserves unused teammates",GT(){
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(smac_startup_fixture::Source,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string{
            auto* first=Populate(GSE_CALL,fixture,2);auto* game=fixture.Game();
            const std::array<Unit*,3> actors{first,game->GetUM()->GetUnit(72),game->GetUM()->GetUnit(73)};
            for(auto* actor:actors){Event(GSE_CALL,fixture,actor,Order::Farm);}
            const std::array<std::array<int,3>,3> captured{{{2,0,0},{0,4,0},{0,0,6}}};
            for(size_t i=0;i<actors.size();++i){
                Event(GSE_CALL,fixture,actors[i],Order::None,true);
                for(size_t j=0;j<actors.size();++j){GT_ASSERT(actors[j]->Conventional().work_progress==captured[i][j],"captured three-worker progress differs");}
            }
            game->Event(GSE_CALL,"complete_turn",{});fixture.DrainEvents();
            Event(GSE_CALL,fixture,first,Order::None,true);
            GT_ASSERT(first->GetTile()->terraforming==map::tile::TERRAFORMING_FARM&&first->Conventional().spent==3
                &&actors[1]->Conventional().spent==0&&actors[2]->Conventional().spent==0,"early completion spent unused helpers");
            for(auto* actor:actors){GT_ASSERT(actor->Conventional().work_order==Order::None&&actor->Conventional().work_progress==0,"completion left matching work active");}
            Event(GSE_CALL,fixture,actors[1],Order::Road);Event(GSE_CALL,fixture,actors[1],Order::None,true);
            Event(GSE_CALL,fixture,actors[2],Order::Solar);Event(GSE_CALL,fixture,actors[2],Order::None,true);
            GT_ASSERT(first->GetTile()->terraforming==(map::tile::TERRAFORMING_FARM|map::tile::TERRAFORMING_ROAD)
                &&actors[2]->Conventional().work_order==Order::Solar&&actors[2]->Conventional().work_progress==2,"unused helpers failed captured same-turn new work");GT_OK();
        });
    });
    task->AddTest("Former work completion changes the actual assigned Base intake",GT(){
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(Source,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string{
            auto* actor=Populate(GSE_CALL,fixture,false,true);auto* game=fixture.Game();auto* base=game->GetBM()->GetBase(901);
            fixture.PrepareBaseCallbacks();static_cast<value::Callable*>(fixture.Script(GSE_CALL,"./GLSMAC_data/tests/_base_growth"))->Run(GSE_CALL,{game->Wrap(GSE_CALL)});
            const auto read=[&](){auto* wrapper=static_cast<value::Object*>(base->Wrap(GSE_CALL));auto* result=static_cast<value::Object*>(static_cast<value::Callable*>(wrapper->Get("get_intake"))->Run(GSE_CALL,{}));return static_cast<value::Int*>(result->Get("NUTRIENTS"))->value;};
            const auto before=read();const auto population=base->m_pops.size();const auto selected=base->GetSmacAllocation();
            Event(GSE_CALL,fixture,actor,Order::Farm);
            for(int step=0;step<4;++step){if(step){game->GetUM()->ResetConventionalSpent(GSE_CALL,actor);}Event(GSE_CALL,fixture,actor,Order::None,true);}
            GT_ASSERT(read()==before+1&&read()==before+1,"actual canonical intake did not consume completed Farm");
            GT_ASSERT(base->m_pops.size()==population&&base->GetSmacAllocation()==selected,"terraforming altered worker allocation");GT_OK();
        });
    });
    task->AddTest("Former work rejection and stored-order bounds preserve actual world",GT(){
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(Source,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string{
            auto* actor=Populate(GSE_CALL,fixture);auto* game=fixture.Game();auto* tile=actor->GetTile();
            for(int scenario=0;scenario<4;++scenario){
                const auto old=tile->features;const auto terraform=tile->terraforming;
                if(scenario==0){tile->features=map::tile::FEATURE_XENOFUNGUS;}
                if(scenario==1){tile->terraforming=map::tile::TERRAFORMING_FOREST;}
                if(scenario==2){tile->terraforming=map::tile::TERRAFORMING_FARM;}
                const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();
                GT_ASSERT(Rejects([&]{game->GetUM()->ValidateConventionalWork(actor,Order::Farm,scenario==3?6:0,false);}),"unsupported work admitted");
                GT_ASSERT(fixture.Snapshot()==before&&fixture.GameplayRandom()==random&&fixture.Updates().empty(),"failed work changed authority");
                tile->features=old;tile->terraforming=terraform;
            }
            auto state=actor->Conventional();state.work_order=Order::None;state.work_progress=255;types::Buffer wire;state.Serialize(wire);
            GT_ASSERT(ConventionalUnitState::Deserialize(wire)==state,"unassigned saved progress was normalized");
            state.work_order=static_cast<Order>(255);GT_ASSERT(Rejects([&]{state.Validate();}),"unknown saved order admitted");
            state=actor->Conventional();state.design_id=2;state.work_progress=1;
            GT_ASSERT(Rejects([&]{game->GetUM()->UpdateConventionalUnit(GSE_CALL,actor,state);}),"Scout retained Former progress");GT_OK();
        });
    });
}
}

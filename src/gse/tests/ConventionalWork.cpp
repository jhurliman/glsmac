#include "ConventionalWork.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacStartupSource.h"
#include "fixtures/SignedSelectionCursor.h"
#include "game/backend/unit/SmacSelectionDistance.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/FrontendRequest.h"
#include "game/backend/Player.h"
#include "game/backend/Random.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tile.h"
#include "game/backend/map/tile/Tiles.h"
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
#include "gse/value/String.h"
#include <sstream>
#include <array>

namespace gse::tests {
namespace {
using namespace game::backend;
using namespace game::backend::unit;
using Order=ConventionalUnitState::WorkOrder;
bool Rejects(const std::function<void()>& call){try{call();}catch(const std::exception&){return true;}return false;}
std::string Source(const std::string& name) {
    const auto source=smac_startup_fixture::OrdinarySource(name);if(name!="alpha.txt"){return source;}
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
Unit* Populate(GSE_CALLABLE,ConventionalUnitFixture& fixture,size_t helpers=0,bool with_base=false,
    const std::vector<types::Vec2<size_t>>& positions={}) {
    auto* game=fixture.Game();auto* map=game->GetMap();auto* um=game->GetUM();
    for(size_t y=0;y<map->GetHeight();++y){for(size_t x=y&1;x<map->GetWidth();x+=2){
        auto* tile=map->GetTile(x,y);for(auto* corner:tile->elevation.corners){*corner=100;}
        tile->features=0;tile->terraforming=0;tile->rockiness=map::tile::ROCKINESS_FLAT;tile->moisture=map::tile::MOISTURE_ARID;tile->bonus=map::tile::BONUS_NONE;tile->Update();
    }}
    MoraleSet::morale_values_t names;for(int i=0;i<7;++i){names.emplace_back("level"+std::to_string(i));}
    um->DefineMoraleSet(new MoraleSet("work-morale",names));
    auto* def=new StaticDef("work-former",um->GetMoraleSet("work-morale"),"Former",MT_LAND,1,new FormerMarkerRender());um->DefineUnit(def);
    auto* owner=&game->GetState()->m_slots->GetSlot(0);auto* tile=positions.empty()?map->GetTile(16,16):map->GetTile(positions[0].x,positions[0].y);
    ConventionalUnitState record{1,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0};
    auto* actor=um->SpawnConventionalUnit(GSE_CALL,71,def,owner,tile,record);
    for(size_t i=0;i<helpers;++i){auto* position=positions.empty()?tile:map->GetTile(positions[i+1].x,positions[i+1].y);um->SpawnConventionalUnit(GSE_CALL,72+i,def,owner,position,record);}
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
    task->AddTest("Conventional visible order glyph uses actual work and hides foreign Hold",GT(){
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(Source,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string{
            auto* actor=Populate(GSE_CALL,fixture);auto* game=fixture.Game();
            const std::pair<Order,char> cases[]{{Order::None,'-'},{Order::Farm,'f'},{Order::Road,'R'},{Order::Forest,'F'},{Order::Solar,'S'},{Order::Mine,'M'}};
            for(const auto& value:cases) {
                Event(GSE_CALL,fixture,actor,value.first);
                GT_ASSERT(actor->VisibleOrderGlyph(0)==value.second && actor->VisibleOrderGlyph(1)=='-',"Renderer substituted port wire ID or exposed foreign work");
            }
            game->Event(GSE_CALL,"unit_hold",{{"unit",actor->Wrap(GSE_CALL)},{"turn_id",VALUE(value::Int,,game->GetTurnId())}});fixture.DrainEvents();
            GT_ASSERT(actor->VisibleOrderGlyph(0)=='H' && actor->VisibleOrderGlyph(1)=='-',"Hold glyph leaked to another viewer");
            auto state=actor->Conventional();state.retained_state_word=ConventionalUnitState::PAID_SUPPORT;game->GetUM()->UpdateConventionalUnit(GSE_CALL,actor,state);
            GT_ASSERT(actor->VisibleOrderGlyph(0)=='H',"Accounting bits replaced ordinary Hold");
            state.retained_state_word=0x4000;game->GetUM()->UpdateConventionalUnit(GSE_CALL,actor,state);
            GT_ASSERT(actor->VisibleOrderGlyph(0)==0,"Unimplemented status override was presented as ordinary Hold");GT_OK();
        });
    });
    task->AddTest("Human order queued behind actual turn end cannot mutate next turn",GT(){
        for(const char* name:{"unit_hold","unit_hold_ten","unit_skip_turn","unit_activate"}) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(Source,"GAIANS");
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string{
                auto* actor=Populate(GSE_CALL,fixture);auto* game=fixture.Game();
                if(std::string(name)=="unit_activate") {
                    game->Event(GSE_CALL,"unit_hold",{{"unit",actor->Wrap(GSE_CALL)},{"turn_id",VALUE(value::Int,,game->GetTurnId())}});fixture.DrainEvents();
                }
                const auto old_turn=game->GetTurnId();
                game->Event(GSE_CALL,"complete_turn",{});
                game->Event(GSE_CALL,name,{{"unit",actor->Wrap(GSE_CALL)},{"turn_id",VALUE(value::Int,,old_turn)}});fixture.DrainEvents();
                GT_ASSERT(game->GetTurnId()==old_turn+1 && actor->Conventional().spent==0 && actor->Conventional().order_countdown==0
                    && actor->Conventional().IsHeld()==(std::string(name)=="unit_activate"),"Old queued input consumed next-turn allowance or changed its order");
                GT_OK();
            });if(!error.empty()){return error;}
        }
        GT_OK();
    });
    task->AddTest("Human Hold codec preserves signed countdown and rejects unsupported orders",GT(){
        ConventionalUnitState plain{2,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0};
        for(const int count:std::array<int,6>{INT16_MIN,-1,0,1,10,INT16_MAX}) {
            auto state=plain;state.order=ConventionalUnitState::Order::Hold;state.order_countdown=static_cast<int16_t>(count);
            state.retained_state_word=0x80000000u;types::Buffer wire;state.Serialize(wire,true,true);
            const auto copy=ConventionalUnitState::Deserialize(wire,true,true);
            GT_ASSERT(copy==state && wire.lenr==wire.lenw,"Order codec lost signed countdown or opaque state");
            types::Buffer downgrade;GT_ASSERT(Rejects([&]{state.Serialize(downgrade,true,false);}) && downgrade.lenw==0,"Legacy downgrade silently discarded Hold");
        }
        for(const auto invalid:std::vector<std::pair<int,int>>{{1,0},{3,0},{2,32768},{2,-32769}}) {
            types::Buffer wire;plain.Serialize(wire,false,false);wire.WriteInt(invalid.first);wire.WriteInt(invalid.second);
            GT_ASSERT(Rejects([&]{ConventionalUnitState::Deserialize(wire,false,true);}),"Unsupported order/countdown admitted");
        }
        GT_OK();
    });
    task->AddTest("Human Hold retains Former progress movement and selected controller through restore",GT(){
        ConventionalUnitFixture source;source.ConfigureProduction(Source,"GAIANS");std::string snapshot;
        auto first=source.Run([&](GSE_CALLABLE)->std::string{
            auto* actor=Populate(GSE_CALL,source,1);auto* game=source.Game();auto* um=game->GetUM();
            const auto command=[&](const char* name){game->Event(GSE_CALL,name,{{"unit",actor->Wrap(GSE_CALL)},{"turn_id",VALUE(value::Int,,game->GetTurnId())}});source.DrainEvents();};
            Event(GSE_CALL,source,actor,Order::Farm);Event(GSE_CALL,source,actor,Order::None,true);
            const auto random=source.GameplayRandom();command("unit_hold");
            bool held_update=false;for(const auto& update:source.Updates()) {
                if(update.type==game::FrontendRequest::FR_UNIT_UPDATE && update.data.unit_update.unit_id==actor->m_id) {held_update=update.data.unit_update.held && update.data.unit_update.movement==0;}
            }
            GT_ASSERT(held_update,"Committed Hold did not publish its actual held/spent presentation");
            GT_ASSERT(actor->Conventional().IsHeld() && actor->Conventional().order_countdown==0 && actor->Conventional().spent==3
                && actor->Conventional().work_progress==2 && actor->Conventional().work_order==Order::None,"Hold refunded spent movement or rolled back work");
            game->Event(GSE_CALL,"complete_turn",{});source.DrainEvents();
            GT_ASSERT(actor->Conventional().IsHeld() && actor->Conventional().spent==0 && actor->HasMovesLeft(),"Hold faked exhaustion or woke on upkeep");
            GT_ASSERT(game->SelectLocalConventionalUnit(game->GetTurnId(),actor->m_id,actor->m_id,actor->GetTile()->coord,true)==72,"Held current unit defeated next ready selection");
            const auto before=source.Snapshot();
            GT_ASSERT(Rejects([&]{source.MoveEvent(GSE_CALL,actor,actor->GetTile()->E);}) && source.Snapshot()==before,"Directional movement implicitly activated Hold");
            GT_ASSERT(!game->QueueSelectedConventionalWork(GSE_CALL,actor->m_id,game->GetTurnId()),"Held Former continued work");
            GT_ASSERT(source.GameplayRandom()==random,"Hold/upkeep consumed gameplay random stream");
            snapshot=source.Snapshot();GT_OK();
        });if(!first.empty()){return first;}
        ConventionalUnitFixture restored;restored.ConfigureProduction(Source,"GAIANS");
        return restored.Run([&](GSE_CALLABLE)->std::string{
            restored.Restore(GSE_CALL,snapshot);restored.Updates();auto* game=restored.Game();auto* actor=game->GetUM()->GetUnit(71);
            std::array<std::string,8> roles;roles[0]="work-worker";for(uint8_t i=0;i<7;++i){roles[i+1]="work-specialist"+std::to_string(i);}restored.BasePresentationRoles(roles);
            GT_ASSERT(actor->Conventional().IsHeld() && actor->Conventional().work_progress==2 && actor->Conventional().spent==0,"Unit7 restore lost Hold/work");
            auto* initialize=static_cast<value::Callable*>(restored.Script(GSE_CALL,"./GLSMAC_data/tests/_unit_commands"));
            auto* ui=static_cast<value::Callable*>(initialize->Run(GSE_CALL,{game->Wrap(GSE_CALL)}));
            auto* wrapper=actor->Wrap(GSE_CALL);game->GetState()->TriggerObject(game,"unit_select",ARGS_F(&wrapper){{"unit",wrapper}};});
            ui->Run(GSE_CALL,{VALUE(value::String,,"check"),VALUE(value::String,,"Activate unit"),VALUE(value::String,,"Holding position — stored work: 2")});
            ui->Run(GSE_CALL,{VALUE(value::String,,"click"),VALUE(value::String,,"Activate unit"),VALUE(value::String,,"")});restored.DrainEvents();
            GT_ASSERT(!actor->Conventional().IsHeld() && actor->Conventional().work_progress==2,"Activate removed retained work");
            restored.MoveEvent(GSE_CALL,actor,actor->GetTile()->E);
            game->Event(GSE_CALL,"complete_turn",{});restored.DrainEvents();
            Event(GSE_CALL,restored,actor,Order::Solar);Event(GSE_CALL,restored,actor,Order::None,true);
            GT_ASSERT(actor->Conventional().work_progress==4 && actor->Conventional().spent==3 && actor->GetTile()->coord.x==18,"Hold/Activate/move broke continuous Former work");GT_OK();
        });
    });
    task->AddTest("Human Hold ten expires on tenth actual owning turn and menu preserves countdown",GT(){
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(Source,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string{
            auto* actor=Populate(GSE_CALL,fixture);auto* game=fixture.Game();
            const auto command=[&](const char* name){game->Event(GSE_CALL,name,{{"unit",actor->Wrap(GSE_CALL)},{"turn_id",VALUE(value::Int,,game->GetTurnId())}});fixture.DrainEvents();};
            command("unit_hold_ten");
            for(int turn=1;turn<=10;++turn) {
                game->Event(GSE_CALL,"complete_turn",{});fixture.DrainEvents();
                GT_ASSERT(actor->Conventional().order_countdown==10-turn && actor->Conventional().IsHeld()==(turn!=10),"Timed Hold expired on the wrong owner turn");
            }
            auto state=actor->Conventional();state.order_countdown=-1;game->GetUM()->UpdateConventionalUnit(GSE_CALL,actor,state);
            command("unit_menu_hold");game->Event(GSE_CALL,"complete_turn",{});fixture.DrainEvents();
            GT_ASSERT(actor->Conventional().IsHeld() && actor->Conventional().order_countdown==-2,"Menu Hold normalized retained countdown");
            command("unit_hold");game->Event(GSE_CALL,"complete_turn",{});fixture.DrainEvents();
            GT_ASSERT(actor->Conventional().IsHeld() && actor->Conventional().order_countdown==0,"Keyboard Hold was not indefinite");
            state=actor->Conventional();state.order_countdown=INT16_MIN;state.AdvanceOrderUpkeep();
            GT_ASSERT(state.order_countdown==INT16_MAX && state.IsHeld(),"Countdown word arithmetic invoked signed overflow");GT_OK();
        });
    });
    task->AddTest("Human Skip keeps active work while Hold authenticates actual owner and tile",GT(){
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(Source,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string{
            auto* actor=Populate(GSE_CALL,fixture);auto* game=fixture.Game();auto* um=game->GetUM();
            Event(GSE_CALL,fixture,actor,Order::Farm);
            game->Event(GSE_CALL,"unit_skip_turn",{{"unit",actor->Wrap(GSE_CALL)},{"turn_id",VALUE(value::Int,,game->GetTurnId())}});fixture.DrainEvents();
            GT_ASSERT(actor->Conventional().work_order==Order::Farm && !actor->Conventional().IsHeld() && actor->Conventional().spent==3,"Space became Hold or cancelled work");
            const auto before=fixture.Snapshot();using Command=UnitManager::HumanOrderCommand;
            GT_ASSERT(Rejects([&]{um->PrepareHumanOrder(actor,Command::Hold,1);}) && fixture.Snapshot()==before,"Foreign caller assigned Hold");
            game->Event(GSE_CALL,"unit_hold",{{"unit",actor->Wrap(GSE_CALL)},{"turn_id",VALUE(value::Int,,game->GetTurnId())}});fixture.DrainEvents();
            GT_ASSERT(actor->Conventional().IsHeld() && actor->Conventional().spent==3,"Exhausted unit could not Hold");
            const auto held=fixture.Snapshot();
            GT_ASSERT(Rejects([&]{um->PrepareHumanOrder(actor,Command::Skip,0);}) && fixture.Snapshot()==held,"Skip acted on held unit");
            const auto actions=um->QueryConventionalCommands(actor).actions;
            GT_ASSERT(actions==std::vector<int32_t>{7},"Held unit lacks sole Activate command");
            types::Buffer old;auto plain=actor->Conventional();plain.facing.reset();plain.stack.reset(); // fields absent from the authored Unit4 migration control
            plain.native_flags.reset();plain.retained_state_word.reset();plain.accounting_markers.reset();plain.order=ConventionalUnitState::Order::None;plain.order_countdown=0;plain.Serialize(old,false,false);
            const auto migrated=ConventionalUnitState::Deserialize(old,false,false);
            GT_ASSERT(migrated==plain && old.lenr==old.lenw,"Unit4 migration invented an order");GT_OK();
        });
    });
    task->AddTest("Former command panel refreshes after actual frontend work and cancel updates",GT(){
        return ConventionalUnitFixture::WithControllerViewport([&]()->std::string{
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(Source,"GAIANS");
            Unit* actor=nullptr;
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string{actor=Populate(GSE_CALL,fixture,1);GT_OK();});
            if(!error.empty()){return error;}
            return fixture.CheckUnitCommandCompletion(actor,fixture.Game()->GetUM()->GetUnit(72));
        });
    });
    task->AddTest("Former command panel displays saved work and rejects stale selection callbacks",GT(){
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(Source,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string{
            auto* actor=Populate(GSE_CALL,fixture,1);auto* game=fixture.Game();auto* um=game->GetUM();
            auto* helper=um->GetUnit(72);
            auto* initialize=static_cast<value::Callable*>(fixture.Script(GSE_CALL,"./GLSMAC_data/tests/_unit_commands"));
            auto* ui=static_cast<value::Callable*>(initialize->Run(GSE_CALL,{game->Wrap(GSE_CALL)}));
            const auto call=[&](const char* operation,const char* expected="",const char* heading=""){
                ui->Run(GSE_CALL,{VALUE(value::String,,operation),VALUE(value::String,,expected),VALUE(value::String,,heading)});
            };
            const auto refresh=[&](Unit* unit,const char* event){
                auto* wrapper=unit->Wrap(GSE_CALL);
                if(std::string(event)=="unit_select") {game->GetState()->TriggerObject(game,event,ARGS_F(&wrapper){{"unit",wrapper}};});}
                else {game->GetState()->TriggerObject(game->GetMap(),event,ARGS_F(&wrapper){{"unit",wrapper}};});}
            };
            const auto random=fixture.GameplayRandom();const auto jsf=game->GetRandom()->GetStateString();
            const auto initial=fixture.Snapshot();refresh(actor,"unit_select");
            call("check","Farm [F]|Road [R]|Forest [O]|Solar [S]|Mine [M]","Former — Order: None  |  Stored work: 0");
            GT_ASSERT(fixture.Snapshot()==initial&&fixture.Updates().empty(),"command query mutated authority or queued updates");
            call("remember","Farm [F]");call("click","Farm [F]");fixture.DrainEvents();
            GT_ASSERT(actor->Conventional().work_order==Order::Farm&&actor->Conventional().work_progress==0,"button bypassed or failed native order event");
            Event(GSE_CALL,fixture,actor,Order::None,true);fixture.Updates();refresh(actor,"unit_preview");
            call("check","Farm [F]|Road [R]|Forest [O]|Solar [S]|Mine [M]|Cancel [C]","Former — Order: Farm  |  Stored work: 2");
            call("click","Cancel [C]");fixture.DrainEvents();fixture.Updates();refresh(actor,"unit_preview");
            call("check","Farm [F]|Road [R]|Forest [O]|Solar [S]|Mine [M]","Former — Order: None  |  Stored work: 2");
            GT_ASSERT(actor->Conventional().work_order==Order::None&&actor->Conventional().work_progress==2&&actor->Conventional().spent==3,"cancel button refunded movement or discarded stored work");
            refresh(helper,"unit_select");const auto before=fixture.Snapshot();call("remembered");fixture.DrainEvents();
            GT_ASSERT(fixture.Snapshot()==before&&fixture.Updates().empty(),"old selected Unit callback commanded another Unit");
            refresh(actor,"unit_select");call("remember","Farm [F]");actor->GetTile()->features=map::tile::FEATURE_XENOFUNGUS;
            refresh(actor,"unit_preview");call("check","","Former — Order: None  |  Stored work: 2");
            const auto blocked=fixture.Snapshot();call("remembered");fixture.DrainEvents();
            GT_ASSERT(fixture.Snapshot()==blocked&&fixture.Updates().empty(),"stale offered action ignored current admission");
            actor->GetTile()->features=0;
            game->GetState()->TriggerObject(game,"unit_despawn",ARGS_F(){{"unit_id",VALUE(value::Int,,71)}};});call("hidden");
            GT_ASSERT(fixture.GameplayRandom()==random&&game->GetRandom()->GetStateString()==jsf,"command projection or clicks spent unrelated RNG");
            fixture.Early();GT_ASSERT(um->QueryConventionalCommands(actor).actions.empty(),"unavailable turn exposed commands");GT_OK();
        });
    });
    task->AddTest("Conventional native selection uses original wrap ties and current record order",GT(){
        // Literal positions/answers from LOCAL-UNIT-SELECTION: distance_tie_first,
        // distance_tie_current, reference_unit_origin, wrapped_nearest and equal_metric_different_axes.
        const std::vector<std::vector<types::Vec2<size_t>>> positions{ {{16,16},{18,16},{14,16}}, {{16,16},{62,16},{6,16}}, {{16,16},{20,16},{19,19}} };
        for(size_t scenario=0;scenario<positions.size();++scenario){
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(Source,"GAIANS");
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string{
                auto* game=fixture.Game();game->GetState()->m_settings.global.SelectSmacCustomSize(64,32);
                auto* tiles=game->GetMap()->GetTilesPtr();tiles->Resize(64,32);tiles->Clear();
                auto* first=Populate(GSE_CALL,fixture,2,false,positions[scenario]);
                auto state=first->Conventional();state.spent=3;game->GetUM()->UpdateConventionalUnit(GSE_CALL,first,state);
                const types::Vec2<size_t> focus{scenario==1?0u:16u,16};
                const auto select=[&](std::optional<size_t> current={},std::optional<size_t> reference={}){
                    return game->SelectLocalConventionalUnit(game->GetTurnId(),current,reference,focus,false);
                };
                GT_ASSERT(select()==72,"literal original distance/order selection differs");
                if(scenario==0){
                    GT_ASSERT(select(73)==73&&select(73,72)==72,"current tie or independent reference origin differs");
                    auto partial=game->GetUM()->GetUnit(72)->Conventional();partial.spent=1;game->GetUM()->UpdateConventionalUnit(GSE_CALL,game->GetUM()->GetUnit(72),partial);
                    GT_ASSERT(select()==72,"partial movement was ineligible");
                }
                GT_OK();
            });if(!error.empty()){return error;}
        }GT_OK();
    });
    task->AddTest("Conventional native selection matches forty original signed metric controls",GT(){
        for(const auto& control:fixtures::signed_selection::Cases) {
            std::optional<int> selected;int32_t best=9999;
            for(int i=0;i<2;++i) {
                const auto distance=selection_distance::Metric(control.cursor,{{20+4*i,40}},80,!control.flat);
                GT_ASSERT(distance==control.distances[i],"signed metric differs from original x86 return: "+std::string(control.id));
                if(distance<best) {best=distance;selected=i;}
            }
            GT_ASSERT(selected.value_or(-1)==control.selected,"original cutoff/order result differs: "+std::string(control.id));
        }GT_OK();
    });
    task->AddTest("Conventional native selection keeps raw saved cursor separate from fallback camera",GT(){
        return ConventionalUnitFixture::WithControllerViewport([&]()->std::string{
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(Source,"GAIANS");
            const auto setup=fixture.Run([&](GSE_CALLABLE)->std::string{
                auto* game=fixture.Game();game->GetState()->m_settings.global.SelectSmacCustomSize(80,80);
                auto* tiles=game->GetMap()->GetTilesPtr();tiles->Resize(80,80);tiles->Clear();
                auto* exhausted=Populate(GSE_CALL,fixture,2,false,{{16,16},{20,40},{24,40}});
                auto state=exhausted->Conventional();state.spent=3;game->GetUM()->UpdateConventionalUnit(GSE_CALL,exhausted,state);
                const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();
                for(const auto& control:fixtures::signed_selection::Cases) {if(!control.flat) {
                    const auto selected=game->SelectLocalConventionalUnitFromCursor(game->GetTurnId(),{},{},control.cursor,false);
                    const auto expected=control.selected<0?std::optional<size_t>{}:std::optional<size_t>{72+size_t(control.selected)};
                    GT_ASSERT(selected==expected,"actual selector differs from original signed-origin result: "+std::string(control.id));
                }}
                GT_ASSERT(game->SelectLocalConventionalUnitFromCursor(game->GetTurnId(),{},{},{{80,38}},false)==72,
                    "Original invalid saved cursor did not select first ready record");
                GT_ASSERT(game->SelectLocalConventionalUnitFromCursor(game->GetTurnId(),{},{},{{40,40}},false)==73,
                    "Midpoint camera control did not distinguish raw cursor origin");
                GT_ASSERT(game->SelectLocalConventionalUnitFromCursor(game->GetTurnId(),{},73,{{80,38}},false)==73,
                    "Owned reference Unit did not override independent cursor origin");
                // Source524e63..524e71 admits the current ID at an exact9999
                // tie, even without an earlier selected candidate. This derived
                // origin gives distances10002/9999 for the two ready records.
                GT_ASSERT(!game->SelectLocalConventionalUnitFromCursor(game->GetTurnId(),{},{},{{13436,40}},false),
                    "Unselected candidate at initial9999 cutoff was admitted");
                GT_ASSERT(game->SelectLocalConventionalUnitFromCursor(game->GetTurnId(),73,{},{{13436,40}},false)==73,
                    "Current selection lost the source-proven exact9999 equality branch");
                GT_ASSERT(!game->SelectLocalConventionalUnitFromCursor(game->GetTurnId()+1,{},{},{{80,38}},false),
                    "Stale turn admitted a raw-cursor selection");
                GT_ASSERT(fixture.Snapshot()==before && fixture.GameplayRandom()==random,
                    "Cursor selection mutated simulation or consumed random draws");GT_OK();
            });
            if(!setup.empty()){return setup;}
            const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();
            const auto error=fixture.CheckRawCursorSelection();if(!error.empty()){return error;}
            GT_ASSERT(fixture.Snapshot()==before && fixture.GameplayRandom()==random,"Queued cursor selector mutated authority");GT_OK();
        });
    });
    task->AddTest("Conventional native selection retains working Former when Scout resets ready",GT(){
        return ConventionalUnitFixture::WithControllerViewport([&]()->std::string{
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(Source,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string{
            auto* actor=Populate(GSE_CALL,fixture);auto* game=fixture.Game();auto* um=game->GetUM();
            auto* def=new StaticDef("selection-scout",um->GetMoraleSet("work-morale"),"Scout",MT_LAND,1,
                new CvrScoutRender(2,0,1,1,{1,1},{0,0},CvrScoutRender::Composition::InitializedScout));um->DefineUnit(def);
            auto state=actor->Conventional();state.design_id=2;state.spent=3;
            auto* scout=um->SpawnConventionalUnit(GSE_CALL,7,def,actor->m_owner,actor->GetTile(),state);
            Event(GSE_CALL,fixture,actor,Order::Farm);
            return fixture.CheckRetainedWorkSelection(GSE_CALL,actor,scout);
        });
        });
    });
    task->AddTest("Former selected controller latches actual work and retires queued visits",GT(){
        return ConventionalUnitFixture::WithControllerViewport([&]()->std::string{
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(Source,"GAIANS");
            Unit* actor=nullptr;
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string{
                actor=Populate(GSE_CALL,fixture);Event(GSE_CALL,fixture,actor,Order::Farm);GT_OK();
            });
            if(!error.empty()){return error;}
            return fixture.CheckSelectedWorkController(actor);
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
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(smac_startup_fixture::OrdinarySource,"GAIANS");
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string{
                auto* actor=Populate(GSE_CALL,fixture);auto* map=fixture.Game()->GetMap();
                GT_ASSERT(map->GetTerritoryTopology()->SiteRatings(),"ordinary work fixture lacks initialized site history");
                const std::string ratings(map->GetWidth()*map->GetHeight()/2,char(13));fixture.SiteRatings(ratings);
                Event(GSE_CALL,fixture,actor,actions[i]);
                for(int step=1;step<=turns[i];++step){
                    if(step>1){fixture.Game()->GetUM()->ResetConventionalSpent(GSE_CALL,actor);}
                    Event(GSE_CALL,fixture,actor,Order::None,true);
                    GT_ASSERT(actor->Conventional().spent==3&&actor->Conventional().work_progress==(step==turns[i]?0:step*2),"active source time did not control work threshold");
                    GT_ASSERT(actor->GetTile()->terraforming==(step==turns[i]?bits[i]:0),"terrain appeared at the wrong work boundary");
                    GT_ASSERT(map->GetTerritoryTopology()->SiteRatings()->Serialize()==ratings,"ordinary Former completion invalidated or refreshed a nonzero site cache");
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
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(smac_startup_fixture::OrdinarySource,"GAIANS");
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
            auto state=actor->Conventional();state.work_order=Order::None;state.work_progress=255;types::Buffer wire;state.Serialize(wire,true,true,true,true,true);
            GT_ASSERT(ConventionalUnitState::Deserialize(wire,true,true,true,true,true)==state,"unassigned saved progress was normalized");
            state.work_order=static_cast<Order>(255);GT_ASSERT(Rejects([&]{state.Validate();}),"unknown saved order admitted");
            state=actor->Conventional();state.design_id=2;state.work_progress=1;
            GT_ASSERT(Rejects([&]{game->GetUM()->UpdateConventionalUnit(GSE_CALL,actor,state);}),"Scout retained Former progress");GT_OK();
        });
    });
}
}

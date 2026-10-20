#include "gse/value/Callable.h"
#include "gse/value/Object.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"
#include "game/backend/base/SmacEcologyNativeSpawn.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/Player.h"
#include "fixtures/SmacNativeBirthSight.h"
#include "fixtures/SmacEcologyNativeSpawn.h"
#include "SmacNativeUnitRecord.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacStartupSource.h"
#include "game/backend/Game.h"
#include "game/backend/Random.h"
#include "game/FrontendRequest.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/SmacOrdinaryLandExploration.h"
#include <set>
#include <sstream>
#include <algorithm>
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
#include "fixtures/SmacSeaNativeRecord.h"
#include "game/backend/unit/SmacConventionalStats.h"
#include "game/backend/unit/ColonyMarkerRender.h"
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
namespace {

std::string NativeComponentSource(const std::string& filename) {
            const auto source=smac_startup_fixture::OrdinarySource(filename);if(filename!="alpha.txt") {return source;}
            std::istringstream input(source);std::string line,section,result;int index=-1;
            while(std::getline(input,line)) {
                if(!line.empty() && line[0]=='#') {section=line;index=-1;}
                else if(section=="#UNITS") {
                    if(index>=0 && (index==9 || index==10)) {
                        std::istringstream row(line);std::vector<std::string> fields;std::string field;
                        while(std::getline(row,field,',')) {fields.push_back(field);}
                        fields.at(5)=index==9?"8":"10";fields.at(6)=index==9?"4":"0";
                        line.clear();for(const auto& value:fields) {if(!line.empty()) {line+=',';}line+=value;}
                    }
                    ++index;
                }
                result+=line+'\n';
            }
            return result;

}

void BirthDefinitions(game::backend::Game* game) {
    using namespace game::backend::unit;auto* manager=game->GetUM();
    MoraleSet::morale_values_t names;for(int n=0;n<7;++n) {names.emplace_back("level"+std::to_string(n));}
    manager->DefineMoraleSet(new MoraleSet("birth-morale",names));
    for(const auto design:{8,9,10}) {
        const auto* name=design==8?"MindWorms":design==9?"IsleOfTheDeep":"LocustsOfChiron";
        manager->DefineUnit(new StaticDef(name,manager->GetMoraleSet("birth-morale"),name,design==8?MT_LAND:design==9?MT_WATER:MT_AIR,999,new SpriteRender("fixture",0,0,1,1,0,0,1)));
    }
    manager->DefineUnit(new StaticDef("birth-human",manager->GetMoraleSet("birth-morale"),"Scout",MT_LAND,999,new CvrScoutRender(2,0,100,100,{1,1},{128,128})));
    for(size_t y=0;y<game->GetMap()->GetHeight();++y) {for(size_t x=y&1;x<game->GetMap()->GetWidth();x+=2) {
        auto* tile=game->GetMap()->GetTile(x,y);for(auto* corner:tile->elevation.corners) {*corner=100;}
        tile->features=0;tile->terraforming=0;tile->rockiness=game::backend::map::tile::ROCKINESS_FLAT;tile->Update();
    }}
}
game::backend::base::Base* BirthBase(GSE_CALLABLE, game::backend::Game* game) {
    using namespace game::backend;auto* bm=game->GetBM();base::Base::SmacAllocationRoles roles;roles.worker="birth-worker";
    bm->DefinePop(new base::PopDef(roles.worker,"Worker",{},{},base::PopDef::PF_TILE_WORKER));
    for(uint8_t n=0;n<7;++n) {roles.specialists[n]="birth-specialist"+std::to_string(n);bm->DefinePop(new base::PopDef(roles.specialists[n],"Specialist",{},{},base::PopDef::PF_NONE,n));}
    auto* owner=&game->GetState()->m_slots->GetSlot(0);base::Base::pops_t pops;
    pops.emplace(1,base::Pop(nullptr,1,bm->GetPopDef(roles.worker),0,game->GetMap()->GetTile(17,15)));
    auto* result=new base::Base(game,901,owner,owner->GetPlayer()->GetFaction(),game->GetMap()->GetTile(16,16),"Sight Base",{},pops,2,0,{},base::SmacBaseResourceContext{1,false,0,0,0,false});
    bm->SpawnBase(GSE_CALL,result);result->PublishSmacAllocation(GSE_CALL,{3,{{},0}},roles);return result;
}
void ExploreNine(game::backend::map::SmacWorkerKnowledge::Data& data,int32_t x,int32_t y,uint8_t bit) {
    for(int dy=-2;dy<=2;++dy) {for(int dx=-2;dx<=2;++dx) {
        if(std::abs(dx)+std::abs(dy)>2 || ((dx^dy)&1)) {continue;}
        data.tiles[size_t(y+dy)*(data.width/2)+(x+dx)/2].explored_by|=bit;
    }}
}

}
void AddSmacNativeUnitRecordTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC native factory publishes mixed Scout brood observation and restores bound departure",GT(){
        using namespace game::backend;using namespace game::backend::unit;using Knowledge=map::SmacWorkerKnowledge;using Stack=ConventionalUnitState::Stack;
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(NativeComponentSource,"GAIANS",80,80);std::string saved,knowledge_wire;
        const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* manager=game->GetUM();BirthDefinitions(game);BirthBase(GSE_CALL,game);
            auto* tile=game->GetMap()->GetTile(40,40);tile->features|=map::tile::FEATURE_XENOFUNGUS;tile->Update();
            types::Buffer definitions;manager->Serialize(definitions);types::Buffer loaded;loaded.WriteString(definitions.ReadString());
            for(int group=0;group<2;++group) {const auto count=definitions.ReadInt();loaded.WriteInt(count);for(int64_t i=0;i<count;++i) {loaded.WriteString(definitions.ReadString());loaded.WriteString(definitions.ReadString());}}
            GT_ASSERT(definitions.ReadInt()==0,"Mixed constructor fixture already has Units");loaded.WriteInt(1);
            types::Buffer scout;scout.WriteInt(4);scout.WriteInt(1);scout.WriteInt(71);scout.WriteString("birth-human");scout.WriteInt(0);scout.WriteInt(40);scout.WriteInt(40);
            ConventionalUnitState human{2,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0};human.Serialize(scout,false,false);
            loaded.WriteString(scout.ToString());loaded.WriteInt(72);manager->Clear();manager->Deserialize(GSE_CALL,loaded);
            Knowledge::Data data{80,80,true,true,0,{},{},{}};data.observers[1]=Knowledge::Observer{};data.observers[1]->pact_with=0;data.tiles.resize(3200,{0,std::nullopt});
            Knowledge::BaseObservation base{901,2,{}};base.population[1]=1;data.bases.push_back(base);ExploreNine(data,16,16,2);data.tiles[40*40+20].explored_by=2;
            data.units={{71,0,false}};data.occupancy=map::SmacOccupancyCache::Blank(80,80,true);data.occupancy->FoundBase({16,16},1);data.occupancy->PlaceUnit({40,40},1,true);
            game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();
            const auto produce=[&](const UnitManager::NativeBirthFactory& factory) {
                const auto isle=factory.create(9,{40,40},{});const auto worm=factory.create(8,{40,40},{});factory.sentry(worm);factory.promote_stack(isle);
                const auto& live=factory.knowledge();
                const auto observed=std::find_if(live.units.begin(),live.units.end(),[](const auto& row){return row.id==71;});
                if(isle!=1 || worm!=2 || factory.record(isle).stack_next!=worm || factory.record(worm).stack_next!=0
                    || observed==live.units.end() || observed->visible_to!=2 || live.tiles[40*40+20].explored_by!=3
                    || live.occupancy->At({40,40}).affiliation!=1) {throw std::runtime_error("Staged mixed birth differs from owner-translated native discriminator");}
            };
            // The captured owner2 is translated to the fixture's admitted owner1;
            // source dense indices are translated13/14/15 ->0/1/2. No byte-exact
            // original whole-Unit equivalence is claimed for the conventional body.
            GT_ASSERT(Rejects([&]{(void)manager->PrepareNativeBirths(GSE_CALL,80,80,true,[&](const auto& factory){produce(factory);throw std::runtime_error("Injected mixed producer failure");});})
                && fixture.Snapshot()==before,"Failed mixed birth bound the live Scout or published sight");
            {auto discard=manager->PrepareNativeBirths(GSE_CALL,80,80,true,produce);}
            GT_ASSERT(fixture.Snapshot()==before && !manager->GetUnit(71)->Conventional().stack,"Discarded mixed birth changed legacy source authority");
            auto birth=manager->PrepareNativeBirths(GSE_CALL,80,80,true,produce);birth.Commit();birth.Notify(GSE_CALL);
            auto actual=manager->GetUnit(71)->Conventional();GT_ASSERT((actual.stack==Stack{0,-1,2}),"Scout tail link was lost");actual.stack.reset();
            GT_ASSERT(actual==human && fixture.GameplayRandom()==random && manager->GetUnit(72)->Native().stack_previous==-1
                && manager->GetUnit(73)->Native().stack_previous==1,"Mixed constructor changed Scout body/RNG or final head");
            saved=fixture.Snapshot();knowledge_wire=game->GetMap()->GetSmacWorkerKnowledge()->Serialize();GT_OK();
        });GT_ASSERT(error.empty(),"Mixed factory: "+error);
        for(const bool fungal_exit:{false,true}) {
        ConventionalUnitFixture restored;restored.ConfigureProduction(NativeComponentSource,"GAIANS",80,80);
        const auto restore_error=restored.Run([&](GSE_CALLABLE)->std::string {
            restored.Restore(GSE_CALL,saved,true);auto* game=restored.Game();auto* manager=game->GetUM();auto* scout=manager->GetUnit(71);
            GT_ASSERT((scout->Conventional().stack==Stack{0,-1,2}),"Full world restore lost Scout source links");
            GT_ASSERT(game->GetMap()->GetSmacWorkerKnowledge()->Serialize()==knowledge_wire,"Full world restore changed mixed birth sight/occupancy authority");
            // Repeat after identical full restore, retaining actual source
            // fungus in the second case. Destination remains clear in both.
            auto* source=scout->GetTile();
            if(!fungal_exit) {source->features&=~map::tile::FEATURE_XENOFUNGUS;source->Update();}
            const auto source_features=source->features;
            auto* destination=game->GetMap()->GetTile(41,39);
            const auto before_move=restored.Snapshot();const auto random=restored.GameplayRandom();
            manager->ValidateConventionalMove(scout,destination,0);
            GT_ASSERT(restored.Snapshot()==before_move,"Mixed departure validation changed live state");
            manager->MoveConventionalUnit(GSE_CALL,scout,destination,0);
            GT_ASSERT((scout->GetTile()==destination && scout->Conventional().stack==Stack{0,-1,-1}
                && scout->Conventional().spent==3 && scout->Conventional().facing==0
                && source->features==source_features && restored.GameplayRandom()==random),"Mixed departure lost singleton, budget or RNG");
            GT_ASSERT(source->units.size()==2 && destination->units.size()==1
                && manager->GetUnit(72)->GetTile()==source && manager->GetUnit(73)->GetTile()==source
                && manager->GetUnit(72)->Native().stack_previous==-1 && manager->GetUnit(72)->Native().stack_next==2
                && manager->GetUnit(73)->Native().stack_previous==1 && manager->GetUnit(73)->Native().stack_next==-1,
                "Scout departure moved natives or broke their source chain");
            const auto& sight=*game->GetMap()->GetSmacWorkerKnowledge();
            const auto after_sight=sight.CopyData();
            const auto visible=[&](uint64_t id) {return std::find_if(after_sight.units.begin(),after_sight.units.end(),[&](const auto& row){return row.id==id;})->visible_to;};
            GT_ASSERT(visible(71)==1 && visible(72)==2 && visible(73)==2
                && after_sight.occupancy->At({40,40}).affiliation==1
                && after_sight.occupancy->At({41,39}).affiliation==1,
                "Mixed departure lost native detection or recomputed source affiliation");
            manager->ResetConventionalSpent(GSE_CALL,scout);
            const auto departed=restored.Snapshot();
            GT_ASSERT(Rejects([&]{manager->ValidateConventionalMove(scout,source,0);}) && restored.Snapshot()==departed,
                "Mixed departure relaxed hostile destination admission");
            auto casualties=manager->PrepareWorldUnitRemoval(GSE_CALL,{72,73});casualties.Commit();casualties.Notify(GSE_CALL);
            auto* target=game->GetMap()->GetTile(43,39);const auto buddy_id=Unit::GetNextId();
            ConventionalUnitState human{2,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0};
            auto* buddy=manager->SpawnConventionalUnit(GSE_CALL,buddy_id,manager->GetUnitDef("birth-human"),scout->m_owner,target,human);
            manager->ResetConventionalSpent(GSE_CALL,scout);manager->MoveConventionalUnit(GSE_CALL,scout,target,0);
            GT_ASSERT((scout->Conventional().stack==Stack{0,1,-1} && buddy->Conventional().stack==Stack{1,-1,0}),"Friendly entry lost retained head or singleton binding");
            const auto third_id=Unit::GetNextId();const auto before_creation=restored.Snapshot();
            {auto discard=manager->PrepareConventionalUnit(GSE_CALL,third_id,manager->GetUnitDef("birth-human"),scout->m_owner,target,human);}
            GT_ASSERT(restored.Snapshot()==before_creation && Unit::GetNextId()==third_id,"Discarded conventional insertion changed live source links");
            auto creation=manager->PrepareConventionalUnit(GSE_CALL,third_id,manager->GetUnitDef("birth-human"),scout->m_owner,target,human);
            auto retire=manager->PrepareConventionalRemoval(GSE_CALL,{buddy_id},&creation);
            auto* third=creation.Commit();
            GT_ASSERT((third->Conventional().stack==Stack{2,0,-1} && scout->Conventional().stack==Stack{0,1,2}),"Prepared conventional factory did not insert at retained head");
            retire.Commit();creation.Notify(GSE_CALL);retire.Notify(GSE_CALL);
            GT_ASSERT((scout->Conventional().stack==Stack{0,-1,1} && third->Conventional().stack==Stack{1,0,-1}),"Combined conventional creation/casualty lost source links");
            types::Buffer roundtrip;manager->Serialize(roundtrip);GT_OK();
        });GT_ASSERT(restore_error.empty(),"Mixed factory restore/departure: "+restore_error);
        }GT_OK();
    });

    task->AddTest("SMAC native occupied Base factory retains affiliation sight and restored mixed links",GT(){
        using namespace game::backend;using namespace game::backend::unit;using Knowledge=map::SmacWorkerKnowledge;using Stack=ConventionalUnitState::Stack;
        for(const int design:{8,9}) {for(const bool occupied:{false,true}) {for(const bool remove_scout:{false,true}) {
            if(remove_scout && !occupied) {continue;}
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(NativeComponentSource,"GAIANS",80,80);std::string saved;
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* manager=game->GetUM();BirthDefinitions(game);auto* base=BirthBase(GSE_CALL,game);
                auto* tile=base->GetTile();const auto base_wire=base::Base::Serialize(base).ToString();
                manager->DefineUnit(new StaticDef("birth-pod",manager->GetMoraleSet("birth-morale"),"Pod",MT_LAND,999,new ColonyMarkerRender()));
                if(occupied) {
                    types::Buffer definitions;manager->Serialize(definitions);types::Buffer loaded;loaded.WriteString(definitions.ReadString());
                    for(int group=0;group<2;++group) {const auto count=definitions.ReadInt();loaded.WriteInt(count);for(int64_t n=0;n<count;++n) {loaded.WriteString(definitions.ReadString());loaded.WriteString(definitions.ReadString());}}
                    GT_ASSERT(definitions.ReadInt()==0,"Base constructor fixture already contains Units");loaded.WriteInt(2);
                    // Retained initial order Pod1 -> Scout0, not ID sorting.
                    for(int n=0;n<2;++n) {
                        types::Buffer unit;unit.WriteInt(8);unit.WriteInt(1);unit.WriteInt(71+n);unit.WriteString(n?"birth-pod":"birth-human");unit.WriteInt(0);unit.WriteInt(16);unit.WriteInt(16);
                        ConventionalUnitState state{n?0:2,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0};
                        state.stack=n?Stack{1,0,-1}:Stack{0,-1,1};state.Serialize(unit,true,true,true);loaded.WriteString(unit.ToString());
                    }
                    loaded.WriteInt(73);manager->Clear();manager->Deserialize(GSE_CALL,loaded);
                }
                Knowledge::Data data{80,80,true,true,0,{},{},{}};data.observers[1]=Knowledge::Observer{};data.observers[1]->pact_with=0;
                data.tiles.resize(3200,{0,std::nullopt});Knowledge::BaseObservation known{901,2,{}};known.population[1]=1;data.bases.push_back(known);ExploreNine(data,16,16,2);
                data.occupancy=map::SmacOccupancyCache::Blank(80,80,true);data.occupancy->FoundBase({16,16},1);
                if(occupied) {data.units={{71,0,false},{72,0,false}};data.occupancy->PlaceUnit({16,16},1,true);}
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();
                const auto next=Unit::GetNextId();const int born_index=occupied?2:0;
                const auto produce=[&](const auto& factory) {
                    if(factory.create(design,{16,16},{})!=born_index) {throw std::runtime_error("Base constructor dense result differs");}
                    const auto& view=factory.knowledge();const auto point=16*40+8;
                    if(!(view.occupancy->At({16,16})==map::SmacOccupancyCache::Cell{1,3})
                        || view.tiles[point].explored_by!=3 || !view.remembered[1][point]
                        || !view.remembered[1][point]->base_present || !view.remembered[1][point]->unit_present) {
                        throw std::runtime_error("Prepared Base constructor lost affiliation, presence or memory");
                    }
                    for(const auto& unit:view.units) {if(unit.visible_to!=2) {throw std::runtime_error("Base center sight omitted an occupant");}}
                };
                GT_ASSERT(Rejects([&]{(void)manager->PrepareNativeBirths(GSE_CALL,80,80,true,[&](const auto& f){produce(f);throw std::runtime_error("Injected Base birth failure");});})
                    && fixture.Snapshot()==before,"Failed Base construction published state");
                {auto discard=manager->PrepareNativeBirths(GSE_CALL,80,80,true,produce);}
                GT_ASSERT(fixture.Snapshot()==before && Unit::GetNextId()==next,"Discarded Base construction changed world or allocator");
                auto birth=manager->PrepareNativeBirths(GSE_CALL,80,80,true,produce);birth.Commit();birth.Notify(GSE_CALL);
                GT_ASSERT(manager->GetUnit(next)->Native().stack_previous==-1
                    && manager->GetUnit(next)->Native().stack_next==(occupied?1:-1)
                    && (!occupied || (manager->GetUnit(72)->Conventional().stack==Stack{1,0,2}
                        && manager->GetUnit(71)->Conventional().stack==Stack{0,-1,1})),"Base constructor did not prepend to retained order");
                GT_ASSERT(base::Base::Serialize(base).ToString()==base_wire && fixture.GameplayRandom()==random,"Base birth captured, damaged or randomized the Base");
                saved=fixture.Snapshot();GT_OK();
            });GT_ASSERT(error.empty(),"Occupied Base constructor: "+error);
            ConventionalUnitFixture restored;restored.ConfigureProduction(NativeComponentSource,"GAIANS",80,80);
            const auto roundtrip=restored.Run([&](GSE_CALLABLE)->std::string {
                restored.Restore(GSE_CALL,saved,true);auto* game=restored.Game();auto* manager=game->GetUM();auto* tile=game->GetMap()->GetTile(16,16);
                const auto knowledge=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();const auto base_wire=base::Base::Serialize(tile->base).ToString();
                const auto victim=remove_scout?size_t(71):manager->GetUnitRecordOrder().back();
                auto removal=manager->PrepareWorldUnitRemoval(GSE_CALL,{victim});removal.Commit();removal.Notify(GSE_CALL);
                const auto after=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
                GT_ASSERT(after.tiles==knowledge.tiles && after.remembered==knowledge.remembered
                    && after.occupancy->At({16,16}).affiliation==1 && base::Base::Serialize(tile->base).ToString()==base_wire,
                    "Base casualty rewound sight, affiliation or Base state");
                for(const auto& observation:after.units) {GT_ASSERT(observation.visible_to==2,"Base casualty rewound surviving observation");}
                if(remove_scout) {
                    GT_ASSERT((manager->GetUnit(72)->Conventional().stack==Stack{0,-1,1}
                        && manager->GetUnit(manager->GetUnitRecordOrder().back())->Native().stack_next==0),"Scout casualty failed dense mixed repair");
                } else if(occupied) {
                    GT_ASSERT((manager->GetUnit(72)->Conventional().stack==Stack{1,0,-1}
                        && manager->GetUnit(71)->Conventional().stack==Stack{0,-1,1}),"Newborn casualty did not restore original Base stack");
                } else {GT_ASSERT(tile->units.empty() && after.occupancy->At({16,16}).presence==1,"Last Base Unit casualty lost Base presence");}
                GT_OK();
            });GT_ASSERT(roundtrip.empty(),"Occupied Base restore/casualty: "+roundtrip);
        }}}GT_OK();
    });

    task->AddTest("SMAC mixed owner source stacks unlink conventional and native casualties before compaction",GT(){
        using namespace game::backend;using namespace game::backend::unit;using Knowledge=map::SmacWorkerKnowledge;using Stack=ConventionalUnitState::Stack;
        for(const bool staged:{false,true}) {for(int removed=0;removed<3;++removed) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(NativeComponentSource,"GAIANS",80,80);
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* manager=game->GetUM();BirthDefinitions(game);if(staged) {BirthBase(GSE_CALL,game);}
                types::Buffer definitions;manager->Serialize(definitions);types::Buffer loaded;loaded.WriteString(definitions.ReadString());
                for(int group=0;group<2;++group) {const auto count=definitions.ReadInt();loaded.WriteInt(count);for(int64_t i=0;i<count;++i) {loaded.WriteString(definitions.ReadString());loaded.WriteString(definitions.ReadString());}}
                GT_ASSERT(definitions.ReadInt()==0,"Mixed stack fixture already has Units");loaded.WriteInt(3);
                // Explicit retained state: source stack Isle1 -> Worm2 -> Scout0.
                // This fixture checks lifecycle consumers, not constructor production.
                for(int i=0;i<3;++i) {
                    types::Buffer unit;unit.WriteInt(i==0?8:5);unit.WriteInt(i==0?1:2);unit.WriteInt(71+i);unit.WriteString(i==0?"birth-human":i==1?"IsleOfTheDeep":"MindWorms");unit.WriteInt(i==0?0:-1);unit.WriteInt(40);unit.WriteInt(40);
                    if(i==0) {ConventionalUnitState human{2,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0};human.stack=Stack{0,-1,2};human.Serialize(unit,true,true,true);}
                    else {auto record=Record::InitializeWild(i,i==1?9:8,0);record.x=40;record.y=40;record.stack_next=i==1?2:0;record.stack_previous=i==1?-1:1;unit.WriteString(record.Serialize().ToString());}
                    loaded.WriteString(unit.ToString());
                }
                loaded.WriteInt(74);manager->Clear();manager->Deserialize(GSE_CALL,loaded);
                if(staged) {
                    Knowledge::Data data{80,80,true,true,0,{},{},{}};data.observers[1]=Knowledge::Observer{};data.tiles.resize(3200,{0,std::nullopt});
                    Knowledge::BaseObservation base{901,2,{}};base.population[1]=1;data.bases.push_back(base);ExploreNine(data,16,16,2);
                    data.units={{71,0,false},{72,0,false},{73,0,false}};game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
                }
                const auto next=Unit::GetNextId();const auto random=fixture.GameplayRandom();
                if(staged) {
                    const auto before=fixture.Snapshot();{auto discard=manager->PrepareWorldUnitRemoval(GSE_CALL,{size_t(71+removed)});}
                    GT_ASSERT(fixture.Snapshot()==before,"Discarded mixed stack casualty changed live state");
                    auto removal=manager->PrepareWorldUnitRemoval(GSE_CALL,{size_t(71+removed)});removal.Commit();removal.Notify(GSE_CALL);
                } else {manager->DespawnUnit(GSE_CALL,71+removed);}
                std::vector<int> chain;for(const auto n:{1,2,0}) {if(n!=removed) {chain.push_back(n);}}
                for(size_t i=0;i<chain.size();++i) {
                    auto* unit=manager->GetUnit(71+chain[i]);const auto& n=unit->IsNative()?unit->Native():Record{};
                    const auto actual=unit->IsNative()?Stack{int16_t(n.id),n.stack_next,n.stack_previous}:*unit->Conventional().stack;
                    const auto compact=[&](int n){return int16_t(n-(n>removed));};
                    GT_ASSERT((actual==Stack{compact(chain[i]),i+1<chain.size()?compact(chain[i+1]):int16_t(-1),i?compact(chain[i-1]):int16_t(-1)}),"Mixed stack unlink/reindex differs");
                }
                GT_ASSERT(manager->GetUnits().size()==2 && Unit::GetNextId()==next && fixture.GameplayRandom()==random,"Mixed casualty changed allocator/RNG or retained victim");
                types::Buffer saved;manager->Serialize(saved);std::vector<std::string> wires;for(auto id:manager->GetUnitRecordOrder()) {wires.push_back(Unit::Serialize(manager->GetUnit(id)).ToString());}
                manager->Clear();game->GetMap()->GetTile(40,40)->units.clear();manager->Deserialize(GSE_CALL,saved);
                for(size_t i=0;i<wires.size();++i) {GT_ASSERT(Unit::Serialize(manager->GetUnit(manager->GetUnitRecordOrder()[i])).ToString()==wires[i],"Mixed casualty stack failed complete manager restore");}GT_OK();
            });GT_ASSERT(error.empty(),"Mixed source stack lifecycle: "+error);
        }}GT_OK();
    });

    task->AddTest("SMAC native factory interleaves brood sentry promotion and restores transaction",GT(){
        using namespace game::backend;using namespace game::backend::unit;using Knowledge=map::SmacWorkerKnowledge;
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(NativeComponentSource,"GAIANS",80,80);
        std::string saved;std::vector<Record> expected;
        const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* manager=game->GetUM();BirthDefinitions(game);BirthBase(GSE_CALL,game);
            Knowledge::Data data{80,80,true,true,0,{},{},{}};data.observers[1]=Knowledge::Observer{};data.observers[1]->pact_with=0;
            data.tiles.resize(3200,{0,std::nullopt});Knowledge::BaseObservation base{901,2,{}};base.population[1]=1;data.bases.push_back(base);ExploreNine(data,16,16,2);
            game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
            const auto before=fixture.Snapshot();const auto next=Unit::GetNextId();const auto random=fixture.GameplayRandom();
            const auto produce=[&](const UnitManager::NativeBirthFactory& factory) {
                const auto carrier=factory.create(9,{40,40},{});
                if(carrier!=0 || factory.capacity(carrier)!=2) {throw std::runtime_error("Actual Isle capacity differs");}
                for(int i=0;i<2;++i) {const auto passenger=factory.create(8,{40,40},{});factory.sentry(passenger);}
                factory.promote_stack(carrier);factory.promote_stack(carrier); // Already-head promotion is a no-op.
                const auto head=factory.record(carrier);
                if(head.stack_previous!=-1 || head.stack_next!=2 || factory.record(2).stack_next!=1
                    || factory.record(1).stack_next!=-1 || factory.record(1).order!=1 || factory.record(2).waypoint_x[0]!=-1
                    || factory.record(2).waypoint_y[0]!=0) {throw std::runtime_error("Staged brood order or head differs");}
                factory.create(10,{40,40},{});
            };
            GT_ASSERT(Rejects([&]{(void)manager->PrepareNativeBirths(GSE_CALL,80,80,true,[&](const auto& factory){produce(factory);throw std::runtime_error("Injected producer failure");});})
                && fixture.Snapshot()==before && Unit::GetNextId()==next && fixture.GameplayRandom()==random,"Failed producer published staged authority");
            {auto discarded=manager->PrepareNativeBirths(GSE_CALL,80,80,true,produce);GT_ASSERT(discarded.Size()==4,"Discarded brood missing bodies");}
            GT_ASSERT(fixture.Snapshot()==before && Unit::GetNextId()==next,"Discarded brood published state");
            auto prepared=manager->PrepareNativeBirths(GSE_CALL,80,80,true,produce);
            GT_ASSERT(prepared.Size()==4 && fixture.Snapshot()==before,"Brood published before commit");
            const int next_links[]{2,-1,1,0},previous_links[]{3,2,0,-1};
            for(size_t i=0;i<4;++i) {const auto value=prepared.Candidate(i).Native();
                GT_ASSERT(value.id==i && value.stack_next==next_links[i] && value.stack_previous==previous_links[i],"Prepared stack differs after Locust constructor");expected.push_back(value);}
            prepared.Commit();prepared.Notify(GSE_CALL);
            GT_ASSERT(Unit::GetNextId()==next+4 && fixture.GameplayRandom()==random,"Brood allocation or RNG differs");
            saved=fixture.Snapshot();GT_OK();
        });GT_ASSERT(error.empty(),"Native producer: "+error);
        ConventionalUnitFixture restored;restored.ConfigureProduction(NativeComponentSource,"GAIANS",80,80);
        const auto restore_error=restored.Run([&](GSE_CALLABLE)->std::string {
            restored.Restore(GSE_CALL,saved,true);auto* manager=restored.Game()->GetUM();
            GT_ASSERT(manager->GetUnitRecordOrder().size()==expected.size(),"Restored brood count differs");
            for(size_t i=0;i<expected.size();++i) {GT_ASSERT(manager->GetUnit(manager->GetUnitRecordOrder()[i])->Native()==expected[i],"Restore lost brood order/stack record");}GT_OK();
        });GT_ASSERT(restore_error.empty(),"Native producer restore: "+restore_error);GT_OK();
    });

    task->AddTest("SMAC native birth batches publish six original streams and restore complete sea stacks",GT(){
        using namespace game::backend;using namespace game::backend::unit;using Knowledge=map::SmacWorkerKnowledge;
        for(const auto& c:smac_native_spawn_fixture::cases) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(NativeComponentSource,"GAIANS",80,80);
            std::string snapshot,knowledge,factory_wire;std::vector<Record> records;
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* manager=game->GetUM();BirthDefinitions(game);BirthBase(GSE_CALL,game);
                Knowledge::Data data{80,80,true,true,0,{},{},{}};data.observers[1]=Knowledge::Observer{};data.observers[1]->pact_with=0;
                data.tiles.resize(3200,{0,std::nullopt});Knowledge::BaseObservation base{901,2,{}};base.population[1]=1;data.bases.push_back(base);ExploreNine(data,16,16,2);
                if(c.count) {
                    types::Buffer definitions;manager->Serialize(definitions);types::Buffer loaded;loaded.WriteString(definitions.ReadString());
                    for(int group=0;group<2;++group) {const auto count=definitions.ReadInt();loaded.WriteInt(count);for(int64_t n=0;n<count;++n) {loaded.WriteString(definitions.ReadString());loaded.WriteString(definitions.ReadString());}}
                    GT_ASSERT(definitions.ReadInt()==0,"Capacity fixture already contains Units");loaded.WriteInt(c.count);
                    for(int n=0;n<c.count;++n) {
                        const auto position=n>=648?n+1:n; // leave the positive Base's square empty
                        const int y=position/40,x=2*(position%40)+(y&1);
                        auto record=Record::InitializeMindWorm(n,0);record.x=x;record.y=y;
                        types::Buffer wire;wire.WriteInt(5);wire.WriteInt(2);wire.WriteInt(71+n);wire.WriteString("MindWorms");wire.WriteInt(-1);
                        wire.WriteInt(x);wire.WriteInt(y);wire.WriteString(record.Serialize().ToString());loaded.WriteString(wire.ToString());data.units.push_back({uint64_t(71+n),0,false});
                    }
                    loaded.WriteInt(71+c.count);manager->Clear();manager->Deserialize(GSE_CALL,loaded);
                }
                fixture.ResourceState([&](auto& state){state.native_factory->active_designs[8]=254;state.native_factory->active_plan_tally=UINT32_MAX;
                    if(c.count) {state.native_factory->assignment_reference={{0,0}};} // full pool must not enter the uninitialized assignment tail
                });
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
                const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();const auto next=Unit::GetNextId();
                const auto plan=base::PrepareSmacNativeSpawn({{{40,40},80,80,true},1,c.history,9,c.population},{{{16,16},1}},[&](int32_t,int32_t) {
                    return base::SmacNativeSpawnTile{c.region<64?3:2,!c.no_fungus,static_cast<uint8_t>(c.region),static_cast<uint8_t>(!c.absent),false,15};
                });
                auto prepared=manager->PrepareEcologyNativeBirths(GSE_CALL,plan);
                GT_ASSERT(prepared.Size()==c.born.size() && fixture.Snapshot()==before && Unit::GetNextId()==next,"Native batch published early or allocated on source capacity failure");
                prepared.Commit();prepared.Notify(GSE_CALL);
                GT_ASSERT(manager->GetUnits().size()==size_t(c.count)+c.born.size() && Unit::GetNextId()==next+c.born.size()
                    && fixture.GameplayRandom()==random,"Native batch count IDs or initialized-factory RNG differs");
                int counts[3]={254,0,0};for(const auto& born:c.born) {counts[born.design-8]=(counts[born.design-8]+1)&255;}
                bool counters=false;fixture.ResourceState([&](auto& state){counters=state.native_factory->active_designs[8]==counts[0]
                    && state.native_factory->active_designs[9]==counts[1] && state.native_factory->active_designs[10]==counts[2]
                    && state.native_factory->active_plan_tally==(c.born.empty()?UINT32_MAX:uint32_t(c.born.size()-1));factory_wire=state.native_factory->Serialize().ToString();});
                GT_ASSERT(counters,"Successful birth counters were recounted, lost or advanced on failure");
                if(c.count) {GT_ASSERT(fixture.Snapshot()==before,"Capacity-only batch changed the full world");GT_OK();}
                size_t i=0;for(const auto& born:c.born) {
                    const auto* unit=manager->GetUnit(next+i);GT_ASSERT(unit && unit->IsNative() && !unit->m_owner,"Native body did not reach the live manager");
                    const auto& value=unit->Native();GT_ASSERT(value.id==i && value.design==born.design && value.x==born.x && value.y==born.y
                        && value.ecology_owner_tag==born.tag && value.observation_mask==0,"Native batch differs from captured factory/caller stream");
                    records.push_back(value);++i;
                }
                const std::string name=c.name;
                if(name=="sea_region_present_H9" || name=="sea_region_absent_H9") {
                    const auto& literal=smac_sea_native_fixture::profiles[name=="sea_region_present_H9"?0:1].placed;
                    GT_ASSERT(records==literal,"Actual birth batch differs from six complete original final sea/air records");
                }
                types::Buffer persisted;manager->Serialize(persisted);knowledge=game->GetMap()->GetSmacWorkerKnowledge()->Serialize();snapshot=fixture.Snapshot();GT_OK();
            });GT_ASSERT(error.empty(),"Native batch "+std::string(c.name)+": "+error);
            if(c.count) {continue;}
            ConventionalUnitFixture restored;restored.ConfigureProduction(NativeComponentSource,"GAIANS",80,80);
            const auto restore_error=restored.Run([&](GSE_CALLABLE)->std::string {
                restored.Restore(GSE_CALL,snapshot,true);auto* manager=restored.Game()->GetUM();
                GT_ASSERT(manager->GetUnits().size()==records.size() && restored.Game()->GetMap()->GetSmacWorkerKnowledge()->Serialize()==knowledge,"Restored born world changed native count or sight authority");
                for(size_t i=0;i<records.size();++i) {GT_ASSERT(manager->GetUnit(manager->GetUnitRecordOrder().at(i))->Native()==records[i],"World restore changed a complete born native stack record");}
                std::string retained;restored.ResourceState([&](auto& state){retained=state.native_factory->Serialize().ToString();});GT_ASSERT(retained==factory_wire,"World restore recounted native factory state");GT_OK();
            });GT_ASSERT(restore_error.empty(),"Native batch restore "+std::string(c.name)+": "+restore_error);
        }GT_OK();
    });
    task->AddTest("SMAC native birth level retains original state and flag modifier consumers",GT(){
        for(const auto& c:smac_native_birth_fixture::levels) {
            auto record=Record::InitializeWild(0,c.design,0);record.state=c.state;record.flags=c.flags;const auto before=record;
            GT_ASSERT(game::backend::unit::SmacWildNativeLevel(record,c.turn,c.opponent,c.atrocities)==c.expected && record==before,
                "Original native marker input changed level arithmetic or record bytes");
        }GT_OK();
    });
    task->AddTest("SMAC native birth transaction matches fourteen original placement sight Base sensor and stack controls",GT(){
        using namespace game::backend;using namespace game::backend::unit;using Knowledge=map::SmacWorkerKnowledge;
        for(const auto& c:smac_native_birth_fixture::cases) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(NativeComponentSource,"GAIANS",80,80);
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* manager=game->GetUM();BirthDefinitions(game);BirthBase(GSE_CALL,game);
                if(c.coast) {for(size_t y=0;y<80;++y) {for(size_t x=y&1;x<80;x+=2) {
                    auto* tile=game->GetMap()->GetTile(x,y);if(x>32) {for(auto* corner:tile->elevation.corners) {*corner=-1000;}}
                }}for(size_t y=0;y<80;++y) {for(size_t x=y&1;x<80;x+=2) {game->GetMap()->GetTile(x,y)->Update();}}}
                if(c.sea) {auto* tile=game->GetMap()->GetTile(c.x,c.y);for(auto* corner:tile->elevation.corners) {*corner=-1000;}tile->Update();}
                types::Buffer definitions;manager->Serialize(definitions);types::Buffer loaded;loaded.WriteString(definitions.ReadString());
                for(int group=0;group<2;++group) {const auto count=definitions.ReadInt();loaded.WriteInt(count);for(int64_t n=0;n<count;++n) {loaded.WriteString(definitions.ReadString());loaded.WriteString(definitions.ReadString());}}
                GT_ASSERT(definitions.ReadInt()==0,"Birth fixture already contains Units");loaded.WriteInt(c.before.size());
                Knowledge::Data data{80,80,true,true,0,{},{},{}};data.observers[1]=Knowledge::Observer{};data.observers[1]->pact_with=0;
                data.tiles.resize(3200,{0,std::nullopt});data.remembered[1].resize(3200,Knowledge::Improvements{0,0,false,false});
                Knowledge::BaseObservation base{901,2,{}};base.population[1]=1;data.bases.push_back(base);ExploreNine(data,16,16,2);
                for(const auto& u:c.before) {
                    types::Buffer wire;wire.WriteInt(u.owner==0?5:4);wire.WriteInt(u.owner==0?2:1);wire.WriteInt(71+u.id);
                    wire.WriteString(u.owner==0?(u.design==9?"IsleOfTheDeep":"LocustsOfChiron"):"birth-human");wire.WriteInt(u.owner==0?-1:0);wire.WriteInt(u.x);wire.WriteInt(u.y);
                    if(u.owner==0) {
                        auto record=Record::InitializeWild(u.id,u.design,0);record.x=u.x;record.y=u.y;record.observation_mask=u.visible;
                        record.flags=u.flags;record.state=u.state;record.stack_next=u.next;record.stack_previous=u.previous;
                        wire.WriteString(record.Serialize().ToString());ExploreNine(data,u.x,u.y,1);
                    } else {
                        // Explicit legacy Unit4 conventional body, independent of later optional-history codecs.
                        ExploreNine(data,u.x,u.y,2);
                        wire.WriteInt(u.design);wire.WriteInt(u.owner);wire.WriteInt(1);wire.WriteInt(0);wire.WriteInt(0);wire.WriteBool(false);
                        wire.WriteInt(0);wire.WriteInt(0);wire.WriteInt(0);wire.WriteInt(0);
                    }
                    loaded.WriteString(wire.ToString());data.units.push_back({uint64_t(71+u.id),static_cast<uint8_t>(u.visible),false});
                }
                loaded.WriteInt(71+c.before.size());manager->Clear();manager->Deserialize(GSE_CALL,loaded);
                if(c.sensor_x>=0) {auto* sensor=game->GetMap()->GetTile(c.sensor_x,c.sensor_y);sensor->terraforming|=map::tile::TERRAFORMING_SENSOR;data.tiles[size_t(c.sensor_y)*40+c.sensor_x/2].claimant=1;}
                for(const auto& cell:c.cells) {
                    const auto index=size_t(cell.y)*40+cell.x/2;data.tiles[index].explored_by=cell.before;
                    if(cell.memory_before) {data.remembered[1][index]=Knowledge::Improvements{0,map::tile::TERRAFORMING_FOREST,false,false};}
                }
                if(c.stack) {const auto* tile=game->GetMap()->GetTile(c.x,c.y);data.remembered[1][size_t(c.y)*40+c.x/2]=Knowledge::Improvements{tile->features,tile->terraforming,false,true};}
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
                auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();const auto next=Unit::GetNextId();
                const auto plan=base::PrepareSmacNativeSpawn({{{c.x,c.y},80,80,true},1,7,9,1},{{{16,16},1}},[&](int32_t,int32_t) {
                    return base::SmacNativeSpawnTile{c.design==8?3:2,false,static_cast<uint8_t>(c.design==9?65:1),static_cast<uint8_t>(c.design==10?0:1),false,15};
                });
                if(std::string(c.name)=="Isle_coast_adjacent") {
                    manager->UndefineUnit("LocustsOfChiron");const auto failed_before=fixture.Snapshot();const auto requests=fixture.QueuedFrontendRequests().size();
                    const auto partial=base::PrepareSmacNativeSpawn({{{c.x,c.y},80,80,true},1,9,9,1},{{{16,16},1}},[&](int32_t x,int32_t y) {
                        return base::SmacNativeSpawnTile{3,true,static_cast<uint8_t>(x==c.x && y==c.y?1:65),1,false,15};
                    });
                    GT_ASSERT(partial.Requests().size()>1 && partial.Requests()[0].design==8 && partial.Requests()[1].design==10,"Partial failure did not reach a successful first body");
                    GT_ASSERT(Rejects([&]{(void)manager->PrepareEcologyNativeBirths(GSE_CALL,partial);}) && fixture.Snapshot()==failed_before
                        && Unit::GetNextId()==next && fixture.GameplayRandom()==random && fixture.QueuedFrontendRequests().size()==requests,
                        "Later native body failure leaked its earlier candidate, sight, counters or queues");
                    manager->DefineUnit(new StaticDef("LocustsOfChiron",manager->GetMoraleSet("birth-morale"),"LocustsOfChiron",MT_AIR,999,new SpriteRender("fixture",0,0,1,1,0,0,1)));
                    before=fixture.Snapshot(); // explicit fixture catalog replacement may change its serialized entry order
                }
                {auto discarded=manager->PrepareEcologyNativeBirths(GSE_CALL,plan);GT_ASSERT(discarded.Size()==1,"Native preparation did not stage its body");}
                GT_ASSERT(fixture.Snapshot()==before && Unit::GetNextId()==next && fixture.GameplayRandom()==random,"Discarded native birth published authority or ID/RNG");
                auto prepared=manager->PrepareEcologyNativeBirths(GSE_CALL,plan);
                const auto& candidate=prepared.Candidate(0);GT_ASSERT(candidate.Native().id==c.before.size() && !candidate.m_owner,"Native birth borrowed a controller or wrong dense index");
                GT_ASSERT(fixture.Snapshot()==before && Unit::GetNextId()==next,"Prepared native birth changed the live world");
                prepared.Commit();prepared.Notify(GSE_CALL);
                const auto after=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
                for(const auto& u:c.after) {
                    const auto* actual=manager->GetUnit(71+u.id);GT_ASSERT(actual && actual->SimulationOwner()==u.owner,"Birth source/port identity mapping differs");
                    const auto seen=std::find_if(after.units.begin(),after.units.end(),[&](const auto& row){return row.id==actual->m_id;});
                    GT_ASSERT(seen!=after.units.end() && seen->visible_to==u.visible,"Actual birth sight mask differs from original");
                    if(u.owner==0) {const auto& record=actual->Native();GT_ASSERT(record.id==u.id && record.design==u.design && record.x==u.x && record.y==u.y
                        && record.flags==u.flags && record.state==u.state && record.stack_next==u.next && record.stack_previous==u.previous
                        && record.observation_mask==u.visible && record.ecology_owner_tag==0,"Native birth changed retained links flags state or fallback attribution");}
                }
                GT_ASSERT(after.bases.size()==1 && after.bases[0].known_by==c.base_visible
                    && after.bases[0].population[0].value_or(0)==c.base_native_population,"Native Base visibility/population memory differs");
                for(const auto& cell:c.cells) {
                    const auto index=size_t(cell.y)*40+cell.x/2;const auto* tile=game->GetMap()->GetTile(cell.x,cell.y);
                    GT_ASSERT(after.tiles[index].explored_by==cell.after,"Native nine-cell or positive disclosure exploration differs");
                    if(cell.memory_before==cell.memory_after) {GT_ASSERT(after.remembered[1][index]==data.remembered[1][index],"Native exploration rewrote unrelated human memory");}
                    else {const Knowledge::Improvements expected{tile->features,tile->terraforming,tile->base!=nullptr,!tile->units.empty()};GT_ASSERT(after.remembered[1][index]==expected,"Native disclosure did not copy current birth occupancy/improvements");}
                }
                if(c.cells.empty()) {GT_ASSERT(after.tiles==data.tiles,"Stack insertion redundantly changed exploration");}
                GT_ASSERT(after.remembered[0].empty() && fixture.GameplayRandom()==random && Unit::GetNextId()==next+1,"Native birth invented native memory or consumed RNG");
                types::Buffer persisted;manager->Serialize(persisted); // authenticates complete native stack graph after publication
                int count=-1;fixture.ResourceState([&](auto& state){count=state.native_factory->active_designs[c.design];});GT_ASSERT(count==1,"Native factory count did not commit");
                GT_OK();
            });GT_ASSERT(error.empty(),"Native birth "+std::string(c.name)+": "+error);
        }GT_OK();
    });

    task->AddTest("SMAC native Unit bottom bar consumes checked birth queries without legacy floats",GT(){
        using namespace game::backend;
        for(const auto design:{8,9,10}) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(NativeComponentSource,"GAIANS",80,80);
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* manager=game->GetUM();BirthDefinitions(game);BirthBase(GSE_CALL,game);
                using Knowledge=map::SmacWorkerKnowledge;
                Knowledge::Data data{80,80,true,true,0,{},{},{}};data.observers[1]=Knowledge::Observer{};data.observers[1]->pact_with=0;
                data.tiles.resize(3200,{0,std::nullopt});Knowledge::BaseObservation base_observation{901,2,{}};base_observation.population[1]=1;data.bases.push_back(base_observation);ExploreNine(data,16,16,2);
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
                const auto plan=base::PrepareSmacNativeSpawn({{{24,24},80,80,true},1,7,9,1},{{{16,16},1}},[&](int32_t,int32_t) {
                    return base::SmacNativeSpawnTile{design==9?2:3,false,static_cast<uint8_t>(design==9?65:1),static_cast<uint8_t>(design==10?0:1),false,15};
                });
                auto birth=manager->PrepareEcologyNativeBirths(GSE_CALL,plan);GT_ASSERT(birth.Size()==1,"Preview fixture requires one actual birth");
                const auto id=birth.Candidate(0).m_id;birth.Commit();birth.Notify(GSE_CALL);auto* unit=manager->GetUnit(id);
                auto* initialize=static_cast<value::Callable*>(fixture.Script(GSE_CALL,"./GLSMAC_data/tests/_conventional_unit_preview"));
                auto* check=static_cast<value::Callable*>(initialize->Run(GSE_CALL,{game->Wrap(GSE_CALL)}));
                const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();const auto shared=game->GetRandom()->GetStateString();
                const auto next=unit::Unit::GetNextId();const auto queued=fixture.QueuedFrontendRequests().size();auto* wrapper=unit->Wrap(GSE_CALL);
                auto* object=static_cast<value::Object*>(wrapper);
                const auto query=[&](int64_t viewer) {return static_cast<value::Callable*>(object->Get("query_native_stats"))->Run(GSE_CALL,{VALUE(value::Int,,viewer)});};
                auto* copied=static_cast<value::Object*>(query(0));copied->Assign("remaining_allowance",VALUE(value::Int,,255));
                GT_ASSERT(static_cast<value::Int*>(static_cast<value::Object*>(query(0))->Get("remaining_allowance"))->value!=255,"Native copied query exposed authority");
                bool invalid=false;try {query(INT64_MAX);}catch(const std::exception&) {invalid=true;}
                GT_ASSERT(invalid,"Native script viewer narrowed before validation");
                for(int repeat=0;repeat<2;++repeat) {
                    auto* tile_wrapper=unit->GetTile()->Wrap(GSE_CALL);
                    game->GetState()->TriggerObject(game->GetMap(),"tile_preview",ARGS_F(&tile_wrapper){{"tile",tile_wrapper}};});
                    game->GetState()->TriggerObject(game->GetMap(),"unit_preview",ARGS_F(&wrapper){{"unit",wrapper}};});
                    game->GetState()->TriggerObject(game,"unit_select",ARGS_F(&wrapper){{"unit",wrapper}};});
                    check->Run(GSE_CALL,{wrapper,new value::Object(GSE_CALL,{
                        {"stats",VALUE(value::String,,design==8?"?-?-1":design==9?"?-?-4":"?-?-8")},
                        {"morale",VALUE(value::String,,design==10?"level1":"level0")},
                        {"moves",VALUE(value::String,,design==8?"Moves: 1":design==9?"Moves: 4":"Moves: 8")}
                    })});
                }
                GT_ASSERT(fixture.Snapshot()==before && fixture.GameplayRandom()==random && game->GetRandom()->GetStateString()==shared
                    && unit::Unit::GetNextId()==next && fixture.QueuedFrontendRequests().size()==queued,"Native preview changed world RNG IDs or frontend queue");
                const auto& updates=fixture.Updates();const auto spawn=std::find_if(updates.begin(),updates.end(),[&](const auto& fr){return fr.type==game::FrontendRequest::FR_UNIT_SPAWN && fr.data.unit_spawn.unit_id==id;});
                GT_ASSERT(spawn!=updates.end() && spawn->data.unit_spawn.native && !spawn->data.unit_spawn.conventional
                    && spawn->data.unit_spawn.morale==(design==10?1:0) && !unit->m_owner
                    && spawn->data.unit_spawn.has_facing && spawn->data.unit_spawn.facing==unit->Native().facing,"Native publication borrowed human ownership or lost its identity");
                game::FrontendRequest copy(*spawn);GT_ASSERT(copy.data.unit_spawn.native && copy.data.unit_spawn.has_facing && copy.data.unit_spawn.facing==unit->Native().facing
                    && copy.data.unit_spawn.morale_string!=spawn->data.unit_spawn.morale_string,"Native spawn copy lost its owned description");
                fixture.ResourceState([](auto& state){state.ecology_history->major_atrocities[1]=1;});
                manager->RefreshUnit(GSE_CALL,unit);const auto& refreshed=fixture.Updates();
                const auto refresh=std::find_if(refreshed.begin(),refreshed.end(),[&](const auto& fr){return fr.type==game::FrontendRequest::FR_UNIT_UPDATE && fr.data.unit_update.unit_id==id;});
                GT_ASSERT(refresh!=refreshed.end() && refresh->data.unit_update.has_facing && refresh->data.unit_update.facing==unit->Native().facing
                    && refresh->data.unit_update.morale==(design==10?2:1)
                    && *refresh->data.unit_update.morale_string==(design==10?"level1":"level0"),"Native frontend badge did not use viewer context independently of neutral description");GT_OK();
            });GT_ASSERT(error.empty(),"Native preview "+std::to_string(design)+": "+error);
        }GT_OK();
    });
    task->AddTest("SMAC sea native complete factory and final records retain separate stack and caller writes",GT(){
        for(const auto& profile:smac_sea_native_fixture::profiles) {
            for(size_t i=0;i<profile.returned.size();++i) {
                const auto& literal=profile.returned[i];
                auto initialized=Record::InitializeWild(literal.id,literal.design,0);
                GT_ASSERT(initialized.x==-4 && initialized.y==-4 && initialized.ecology_owner_tag==0,"Sea native initializer placed or tagged before caller");
                initialized.x=literal.x;initialized.y=literal.y;
                initialized.stack_next=literal.stack_next;initialized.stack_previous=literal.stack_previous;
                GT_ASSERT(initialized==literal,"Sea native initializer differs from actual full factory return");
                GT_ASSERT(Record::Deserialize(literal.Serialize().ToString())==literal
                    && Record::Deserialize(profile.placed[i].Serialize().ToString())==profile.placed[i],"Sea native codec lost retained record fields");
            }
            GT_ASSERT(profile.placed[0].stack_previous==2 && profile.placed[2].stack_next==0
                && profile.placed[0].ecology_owner_tag==1 && profile.placed[1].ecology_owner_tag==1
                && profile.placed[2].ecology_owner_tag==0,"Returned and final stack/tag snapshots conflated");
        }
        GT_ASSERT(Rejects([]{Record::InitializeWild(0,7,0);}) && Rejects([]{Record::InitializeWild(2048,9,0);}),"Invalid wild design or exhausted pool admitted");
        GT_OK();
    });
    task->AddTest("SMAC sea native queries match sixty original level boundaries and sixteen damaged allowances",GT(){
        ConventionalUnitFixture fixture;
        // The shared synthetic catalog intentionally leaves carry/cost zero.
        // Supply the two original-loaded native component rows for this test.
        fixture.ConfigureProduction(NativeComponentSource,"GAIANS",80,80);
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            using namespace game::backend::unit;
            auto* manager=fixture.Game()->GetUM();const auto random=fixture.GameplayRandom();
            for(const auto& q:smac_sea_native_fixture::queries) {
                const auto& record=smac_sea_native_fixture::profiles.at(q.profile).placed.at(q.unit);const auto before=record;
                GT_ASSERT(SmacWildNativeLevel(record,q.turn,0,3)==q.neutral && SmacWildNativeLevel(record,q.turn,1,3)==q.opposed,"Native intrinsic/opposing level differs from original");
                const auto stats=manager->QueryNativeUnit(record);
                GT_ASSERT(stats.quantum==3 && stats.basic_allowance==q.basic && stats.full_allowance==q.full
                    && stats.hit_points==10 && record==before,"Native chassis/component query differs or mutates record");
            }
            for(const auto& c:smac_sea_native_fixture::damage) {
                auto record=Record::InitializeWild(0,c.design,0);record.damage=c.damage;
                const auto domain=c.design==9?SmacMovementDomain::Sea:SmacMovementDomain::Air;
                GT_ASSERT(SmacWildNativeLevel(record,330,0,0)==c.level
                    && SmacConventionalAllowance({c.basic,3,domain,1,record.damage,c.level,false,true})==c.full,"Damaged level6 Psi allowance differs from original");
                for(const auto spent:{0,1,c.full,c.full+1,255}) {
                    record.movement_spent=static_cast<uint8_t>(spent);const auto before=record;
                    const auto stats=manager->QueryNativeUnit(record);
                    GT_ASSERT(stats.basic_allowance==c.basic && stats.full_allowance==c.full && stats.hit_points==c.health
                        && stats.maximum_hit_points==10 && stats.remaining_allowance==std::max(0,c.full-spent) && record==before,
                        "Live damaged native query scaled air or rounded sea after Q");
                }
            }
            MoraleSet::morale_values_t names;
            for(const auto* name:{"Hatchling","Larval Mass","Pre-Boil","Boil","Mature Boil","Great Boil","Demon Boil"}) {names.emplace_back(name);}
            manager->DefineMoraleSet(new MoraleSet("SEA_NATIVE_MORALE",names));
            for(const auto design:{9,10}) {
                const auto* name=design==9?"IsleOfTheDeep":"LocustsOfChiron";
                manager->DefineUnit(new StaticDef(name,manager->GetMoraleSet("SEA_NATIVE_MORALE"),name,design==9?MT_WATER:MT_AIR,999,new SpriteRender("fixture",0,0,1,1,0,0,1)));
            }
            fixture.ResourceState([](auto& state){if(!state.ecology_history) {state.ecology_history.emplace();} state.ecology_history->major_atrocities[1]=3;});
            for(const auto& profile:smac_sea_native_fixture::profiles) {
                for(const auto& literal:profile.returned) {
                    const auto* name=literal.design==9?"IsleOfTheDeep":"LocustsOfChiron";
                    types::Buffer wire;wire.WriteInt(5);wire.WriteInt(2);wire.WriteInt(71);wire.WriteString(name);
                    wire.WriteInt(-1);wire.WriteInt(literal.x);wire.WriteInt(literal.y);wire.WriteString(literal.Serialize().ToString());
                    auto decoded=wire;std::unique_ptr<Unit> unit(Unit::Deserialize(GSE_CALL,decoded,manager));
                    util::FinallyGuard cleanup([&]{unit->GetTile()->units.erase(unit->m_id);});
                    const auto presentation=unit->GetPresentation();const auto expected=literal.design==9?4:8;
                    const auto viewer=unit->GetPresentation(1);
                    const auto neutral_level=SmacWildNativeLevel(literal,static_cast<int32_t>(fixture.Game()->GetTurnId()),0,0);
                    const auto viewer_level=SmacWildNativeLevel(literal,static_cast<int32_t>(fixture.Game()->GetTurnId()),1,3);
                    GT_ASSERT(presentation.morale==neutral_level && viewer.morale==viewer_level
                        && unit->GetMoraleString()==names.at(neutral_level).m_name
                        && viewer.movement==presentation.movement && viewer.full_movement==presentation.full_movement
                        && viewer.health==presentation.health && unit->Native()==literal,
                        "Viewer badge context changed native description, movement, health or retained record");
                    GT_ASSERT(Rejects([&]{unit->GetPresentation(8);}),"Invalid native viewing faction admitted");
                    GT_ASSERT(unit->IsNative() && !unit->m_owner && unit->Native()==literal && Unit::Serialize(unit.get()).ToString()==wire.ToString()
                        && presentation.full_movement==expected && presentation.movement==expected && presentation.health==1,
                        "Native sea/air live Unit restore or presentation borrowed legacy movement");
                }
            }
            GT_ASSERT(fixture.GameplayRandom()==random,"Native immutable query/load consumed gameplay RNG");
            GT_OK();
        });
    });
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
    task->AddTest("SMAC native direct despawn unlinks head middle and tail before dense compaction",GT(){
        using namespace game::backend::unit;
        for(int removed=0;removed<3;++removed) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(smac_startup_fixture::OrdinarySource,"GAIANS",80,40);
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();BirthDefinitions(game);auto* manager=game->GetUM();
                types::Buffer definitions;manager->Serialize(definitions);types::Buffer loaded;loaded.WriteString(definitions.ReadString());
                for(int group=0;group<2;++group) {const auto count=definitions.ReadInt();loaded.WriteInt(count);for(int64_t n=0;n<count;++n) {loaded.WriteString(definitions.ReadString());loaded.WriteString(definitions.ReadString());}}
                GT_ASSERT(definitions.ReadInt()==0,"Despawn fixture already contains Units");loaded.WriteInt(3);
                std::vector<Record> records;
                for(int n=0;n<3;++n) {
                    auto record=Record::InitializeMindWorm(n,2);record.x=6;record.y=2;record.stack_previous=n==0?-1:n-1;record.stack_next=n==2?-1:n+1;
                    record.state=0x400100;record.flags=0x104+8*n;record.movement_spent=5;record.aux_word=17+n;
                    record.order=1;record.waypoint_x[0]=2;record.waypoint_y[0]=37;record.ecology_owner_tag=3;
                    records.push_back(record);types::Buffer unit;unit.WriteInt(5);unit.WriteInt(2);unit.WriteInt(71+n);unit.WriteString("MindWorms");unit.WriteInt(-1);
                    unit.WriteInt(6);unit.WriteInt(2);unit.WriteString(record.Serialize().ToString());loaded.WriteString(unit.ToString());
                }
                loaded.WriteInt(74);manager->Clear();manager->Deserialize(GSE_CALL,loaded);
                auto* wrapper=manager->GetUnit(71+removed)->Wrap(GSE_CALL);const auto next=Unit::GetNextId();const auto random=fixture.GameplayRandom();
                fixture.ResourceState([](auto& state){state.native_factory=game::backend::faction::SmacNativeFactoryState{0,{{-1,-1}},0,{}};});
                manager->DespawnUnit(GSE_CALL,71+removed);
                bool debit=false;fixture.ResourceState([&](const auto& state){debit=state.native_factory->active_plan_tally==UINT32_MAX && state.native_factory->active_designs[8]==255;});
                GT_ASSERT(debit,"Legacy native despawn omitted known factory accounting");
                GT_ASSERT(!manager->GetUnit(71+removed) && wrapper->type==VT_UNDEFINED && manager->GetUnits().size()==2
                    && game->GetMap()->GetTile(6,2)->units.size()==2,"Native despawn retained the retired record, wrapper or physical occupant");
                std::vector<size_t> order;
                for(int n=0;n<3;++n) {
                    if(n==removed) {continue;}
                    const int index=order.size();order.push_back(71+n);auto expected=records[n];expected.id=index;
                    expected.stack_previous=index==0?-1:0;expected.stack_next=index==0?1:-1;
                    if(removed==2) {expected.order=0;}else {expected.waypoint_x[0]=1;}
                    GT_ASSERT(manager->GetUnit(71+n)->Native()==expected,"Native despawn lost retained fields or repaired links after compaction");
                }
                GT_ASSERT(manager->GetUnitRecordOrder()==order && Unit::GetNextId()==next && fixture.GameplayRandom()==random,
                    "Native despawn changed stable IDs, record order or RNG");
                // Reusing the ordinary manager codec validates reciprocal links
                // and complete same-tile stack membership after removal.
                std::vector<std::string> wires;for(const auto id:order) {wires.push_back(Unit::Serialize(manager->GetUnit(id)).ToString());}
                types::Buffer saved;manager->Serialize(saved);manager->Clear();game->GetMap()->GetTile(6,2)->units.clear();manager->Deserialize(GSE_CALL,saved);
                GT_ASSERT(manager->GetUnitRecordOrder()==order && Unit::GetNextId()==next,"Despawned native stack lost IDs during ordinary manager restore");
                for(size_t n=0;n<order.size();++n) {GT_ASSERT(Unit::Serialize(manager->GetUnit(order[n])).ToString()==wires[n],"Despawned native stack lost complete records during ordinary manager restore");}GT_OK();
            });GT_ASSERT(error.empty(),"Native direct despawn: "+error);
        }GT_OK();
    });
    task->AddTest("SMAC native prepared world removal unlinks atomically and retains occupancy history",GT(){
        using namespace game::backend::unit;
        for(const int design:{8,9,10}) {for(const bool zero:{false,true}) {for(int removed=0;removed<3;++removed) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(NativeComponentSource,"GAIANS",80,40);
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();BirthDefinitions(game);auto* manager=game->GetUM();
                types::Buffer definitions;manager->Serialize(definitions);types::Buffer loaded;loaded.WriteString(definitions.ReadString());
                for(int group=0;group<2;++group) {const auto count=definitions.ReadInt();loaded.WriteInt(count);for(int64_t n=0;n<count;++n) {loaded.WriteString(definitions.ReadString());loaded.WriteString(definitions.ReadString());}}
                GT_ASSERT(definitions.ReadInt()==0,"Despawn fixture already contains Units");loaded.WriteInt(3);
                std::vector<Record> records;
                for(int n=0;n<3;++n) {
                    auto record=Record::InitializeWild(n,design,2);record.x=6;record.y=2;record.stack_previous=n==0?-1:n-1;record.stack_next=n==2?-1:n+1;
                    record.state=0x400100;record.flags=0x104+8*n;record.movement_spent=5;record.aux_word=17+n;
                    record.order=1;record.waypoint_x[0]=2;record.waypoint_y[0]=37;record.ecology_owner_tag=3;
                    records.push_back(record);types::Buffer unit;unit.WriteInt(5);unit.WriteInt(2);unit.WriteInt(71+n);unit.WriteString(design==8?"MindWorms":design==9?"IsleOfTheDeep":"LocustsOfChiron");unit.WriteInt(-1);
                    unit.WriteInt(6);unit.WriteInt(2);unit.WriteString(record.Serialize().ToString());loaded.WriteString(unit.ToString());
                }
                loaded.WriteInt(74);manager->Clear();manager->Deserialize(GSE_CALL,loaded);
                auto* wrapper=manager->GetUnit(71+removed)->Wrap(GSE_CALL);const auto next=Unit::GetNextId();const auto random=fixture.GameplayRandom();
                using Knowledge=game::backend::map::SmacWorkerKnowledge;
                Knowledge::Data data{80,40,true,true,0,{},{},{}};data.observers[1]=Knowledge::Observer{};data.observers[1]->pact_with=0;
                data.tiles.resize(1600,{2,std::nullopt});for(uint64_t id=71;id<74;++id) {data.units.push_back({id,0,false});}
                data.occupancy=game::backend::map::SmacOccupancyCache::Blank(80,40,true);data.occupancy->PlaceUnit({6,2},0,true);data.occupancy->SetAffiliation({0,0},7);
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
                fixture.ResourceState([&](auto& state) {state.native_factory=game::backend::faction::SmacNativeFactoryState{4,{{17,19}},zero?0u:23u,{}};state.native_factory->active_designs[design]=zero?0:201;state.native_factory->active_designs[7]=88;state.native_factory->active_designs[11]=99;});
                const auto before=fixture.Snapshot();
                {auto abandoned=manager->PrepareWorldUnitRemoval(GSE_CALL,{71+size_t(removed)});}
                GT_ASSERT(fixture.Snapshot()==before,"Abandoned world removal published native repairs");
                GT_ASSERT(Rejects([&]{manager->PrepareWorldUnitRemoval(GSE_CALL,{71+size_t(removed),71+size_t(removed)});}) && fixture.Snapshot()==before,"Duplicate world casualty leaked staged records or accounting");
                GT_ASSERT(Rejects([&]{manager->PrepareConventionalRemoval(GSE_CALL,{71+size_t(removed)});}) && fixture.Snapshot()==before,"Conventional-only removal admitted a native casualty");
                auto removal=manager->PrepareWorldUnitRemoval(GSE_CALL,{71+size_t(removed)});removal.Commit();removal.Notify(GSE_CALL);
                bool debited=false;fixture.ResourceState([&](const auto& state){debited=state.native_factory->active_designs[design]==(zero?255:200) && state.native_factory->active_plan_tally==(zero?UINT32_MAX:22) && state.native_factory->active_designs[7]==88 && state.native_factory->active_designs[11]==99 && state.native_factory->assignment_reference==std::array<int32_t,2>{{17,19}} && state.native_factory->definition_morale==4;});
                GT_ASSERT(debited,"World native casualty failed one retained-counter debit");
                const auto cache=game->GetMap()->GetSmacWorkerKnowledge()->CopyData().occupancy;
                GT_ASSERT(cache->At({6,2}).presence==2 && cache->At({6,2}).affiliation==0 && cache->At({0,0}).affiliation==7,"World removal recounted cached affiliation or cleared a surviving stack");
                GT_ASSERT(!manager->GetUnit(71+removed) && wrapper->type==VT_UNDEFINED && manager->GetUnits().size()==2
                    && game->GetMap()->GetTile(6,2)->units.size()==2,"Native despawn retained the retired record, wrapper or physical occupant");
                std::vector<size_t> order;
                for(int n=0;n<3;++n) {
                    if(n==removed) {continue;}
                    const int index=order.size();order.push_back(71+n);auto expected=records[n];expected.id=index;
                    expected.stack_previous=index==0?-1:0;expected.stack_next=index==0?1:-1;
                    if(removed==2) {expected.order=0;}else {expected.waypoint_x[0]=1;}
                    GT_ASSERT(manager->GetUnit(71+n)->Native()==expected,"Native despawn lost retained fields or repaired links after compaction");
                }
                GT_ASSERT(manager->GetUnitRecordOrder()==order && Unit::GetNextId()==next && fixture.GameplayRandom()==random,
                    "Native despawn changed stable IDs, record order or RNG");
                // Reusing the ordinary manager codec validates reciprocal links
                // and complete same-tile stack membership after removal.
                std::vector<std::string> wires;for(const auto id:order) {wires.push_back(Unit::Serialize(manager->GetUnit(id)).ToString());}
                types::Buffer saved;manager->Serialize(saved);manager->Clear();game->GetMap()->GetTile(6,2)->units.clear();manager->Deserialize(GSE_CALL,saved);
                GT_ASSERT(manager->GetUnitRecordOrder()==order && Unit::GetNextId()==next,"Despawned native stack lost IDs during ordinary manager restore");
                for(size_t n=0;n<order.size();++n) {GT_ASSERT(Unit::Serialize(manager->GetUnit(order[n])).ToString()==wires[n],"Despawned native stack lost complete records during ordinary manager restore");}
                auto rest=manager->PrepareWorldUnitRemoval(GSE_CALL,order);rest.Commit();rest.Notify(GSE_CALL);
                fixture.ResourceState([&](const auto& state){debited=state.native_factory->active_designs[design]==(zero?253:198) && state.native_factory->active_plan_tally==(zero?UINT32_MAX-2:20);});
                GT_ASSERT(debited,"Last native stack casualties double-debited or recounted factory history");
                const auto empty=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
                GT_ASSERT(manager->GetUnits().empty() && manager->GetUnitRecordOrder().empty() && empty.units.empty()
                    && empty.occupancy->At({6,2}).presence==0 && empty.occupancy->At({6,2}).affiliation==0,"Last world casualties lost empty-cache history or retained registry/observations");GT_OK();
            });GT_ASSERT(error.empty(),"Native direct despawn: "+error);
        }}}GT_OK();
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
                } else {ConventionalUnitState{2,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0}.Serialize(unit,false);}
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
        GT_ASSERT(SmacWildMindWormLevel(record,90,0,0)==2,"Unrelated native state bit changed lifecycle level");
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

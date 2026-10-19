#include "SmacLiveWorkerKnowledge.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacLiveZoc.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/Random.h"
#include "game/backend/WorldSnapshotWorkerKnowledge.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/faction/Faction.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/SmacOrdinaryLandExploration.h"
#include "game/backend/map/tile/Tiles.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/StaticDef.h"
#include "game/backend/unit/MoraleSet.h"
#include "game/backend/unit/CvrScoutRender.h"
#include "game/FrontendRequest.h"
#include "gse/value/Callable.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"
#include "gse/value/Undefined.h"
#include "gse/callable/Native.h"
#include "util/FinallyGuard.h"
#include <functional>
#include <memory>

namespace gse::tests {
namespace {
using namespace game::backend;
using namespace game::backend::base;
using namespace game::backend::unit;
using Knowledge = map::SmacWorkerKnowledge;
using Frame = map::SmacWorkerMapFrame;

bool Rejects( const std::function< void() >& call ) { try { call(); } catch ( const std::exception& ) { return true; } return false; }
bool KnowledgeFence( const std::function< void() >& call ) {
	try { call(); } catch ( const std::exception& e ) { return std::string( e.what() ).find( "worker sight/territory producer" ) != std::string::npos; } return false;
}
Value* Call( GSE_CALLABLE, value::Object* object, const std::string& name, const value::function_arguments_t& args = {} ) {
	return static_cast< value::Callable* >( object->Get( name ) )->Run( GSE_CALL, args );
}
Base::SmacAllocationRoles Roles() {
	Base::SmacAllocationRoles roles; roles.worker = "knowledge-worker";
	for ( size_t i = 0; i < 7; ++i ) { roles.specialists[ i ] = "knowledge-specialist-" + std::to_string( i ); }
	return roles;
}
void Definitions( Game* game ) {
	auto* bm = game->GetBM(); const auto roles = Roles();
	bm->DefinePop( new PopDef( roles.worker, "Worker", {}, {}, PopDef::PF_TILE_WORKER ) );
	for ( uint8_t i = 0; i < 7; ++i ) { bm->DefinePop( new PopDef( roles.specialists[ i ], "Specialist", {}, {}, PopDef::PF_NONE, i ) ); }
	MoraleSet::morale_values_t names; for ( int i = 0; i < 7; ++i ) { names.emplace_back( "level" + std::to_string( i ) ); }
	auto* um = game->GetUM(); um->DefineMoraleSet( new MoraleSet( "knowledge-morale", names ) );
	um->DefineUnit( new StaticDef( "knowledge-scout", um->GetMoraleSet( "knowledge-morale" ), "Scout", MT_LAND, 999,
		new CvrScoutRender( 2, 0, 100, 100, { 1, 1 }, { 128, 128 } ) ) );
}
Base* MakeBase( GSE_CALLABLE, Game* game, size_t id, size_t slot, int32_t simulation, size_t x, size_t y, uint32_t selected ) {
	auto* bm = game->GetBM(); auto* owner = &game->GetState()->m_slots->GetSlot( slot );
	Base::pops_t pops;
	pops.emplace( 4, Pop( nullptr, 4, bm->GetPopDef( Roles().specialists[ 1 ] ), 1 ) );
	pops.emplace( 91, Pop( nullptr, 91, bm->GetPopDef( Roles().specialists[ 1 ] ), 0 ) );
	auto* base = new Base( game, id, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( x, y ), "Knowledge Base", {}, pops, 100, 19, {},
		SmacBaseResourceContext{ simulation, false, 0, 0, 0x80000040u, false } );
	bm->SpawnBase( GSE_CALL, base );
	SmacBaseAllocationState allocation{ selected, { {}, 1 } };
	for ( size_t i = 0; i < 16; ++i ) { allocation.specialists.types[ i ] = ( i + 1 ) % 7; }
	base->PublishSmacAllocation( GSE_CALL, allocation, Roles() ); return base;
}
Unit* Spawn( GSE_CALLABLE, Game* game, size_t id, size_t slot, int32_t simulation, size_t x, size_t y ) {
	return game->GetUM()->SpawnConventionalUnit( GSE_CALL, id, game->GetUM()->GetUnitDef( "knowledge-scout" ), &game->GetState()->m_slots->GetSlot( slot ),
		game->GetMap()->GetTile( x, y ), { 2, simulation, 1, 0, 0, false, ConventionalUnitState::Home::None, SmacMoraleMode::Mode0 } );
}
struct World { Base* local; Base* foreign; Unit* local_unit; Unit* foreign_unit; };
World Populate( GSE_CALLABLE, Game* game, bool sea ) {
	Definitions( game );
	// Explicit synthetic terrain supplied to the actual Tile authority; no map
	// generator, visibility producer or original identity mapping is inferred.
	for ( size_t y = 0; y < game->GetMap()->GetHeight(); ++y ) { for ( size_t x = y & 1; x < game->GetMap()->GetWidth(); x += 2 ) {
		auto* tile = game->GetMap()->GetTile( x, y );
		for ( auto* corner : tile->elevation.corners ) { *corner = 100; }
	} }
	if ( sea ) { for ( auto* corner : game->GetMap()->GetTile( 4, 2 )->elevation.corners ) { *corner = -1000; } }
	for ( size_t y = 0; y < 8; ++y ) { for ( size_t x = y & 1; x < 8; x += 2 ) { game->GetMap()->GetTile( x, y )->Update(); } }
	return { MakeBase( GSE_CALL, game, 901, 0, 1, 2, 2, 0x80000003u ), MakeBase( GSE_CALL, game, 800, 6, 2, 4, 4, 0x40000101u ),
		Spawn( GSE_CALL, game, 91, 0, 1, 6, 6 ), Spawn( GSE_CALL, game, 17, 6, 2, 3, 3 ) };
}
size_t Index( size_t x, size_t y ) { return y * 4 + x / 2; }
Knowledge::Data Data() {
	Knowledge::Data data{ 8, 8, true, true, 0, {}, {}, {} };
	data.observers[ 1 ] = Knowledge::Observer{}; data.observers[ 2 ] = Knowledge::Observer{};
	data.tiles.resize( 32, { 6, std::nullopt } );
	data.tiles[ Index( 0, 2 ) ].explored_by = 4;
	data.tiles[ Index( 1, 3 ) ].claimant = 2;
	data.tiles[ Index( 1, 1 ) ].claimant = 0;
	// Deliberately opposite the actual manager's ascending ID order.
	data.units = { { 91, 0, false }, { 17, 2, false } }; return data;
}
struct Saved {
	std::string snapshot, base, other, unit, other_unit, knowledge, jsf;
	SmacGameplayRandomState pair;
	size_t requests, unit_id, base_id;
};
Saved Save( ConventionalUnitFixture& fixture, const World& world ) {
	const auto& knowledge = fixture.Game()->GetMap()->GetSmacWorkerKnowledge();
	return { fixture.Snapshot(), Base::Serialize( world.local ).ToString(), Base::Serialize( world.foreign ).ToString(), Unit::Serialize( world.local_unit ).ToString(),
		Unit::Serialize( world.foreign_unit ).ToString(), knowledge ? knowledge->Serialize() : "", fixture.Game()->GetRandom()->GetStateString(), fixture.GameplayRandom(),
		fixture.QueuedFrontendRequests().size(), Unit::GetNextId(), Base::GetNextId() };
}
bool Unchanged( ConventionalUnitFixture& fixture, const World& world, const Saved& before ) {
	const auto now = Save( fixture, world );
	return now.snapshot == before.snapshot && now.base == before.base && now.other == before.other && now.unit == before.unit && now.other_unit == before.other_unit
		&& now.knowledge == before.knowledge && now.jsf == before.jsf && now.pair == before.pair && now.requests == before.requests
		&& now.unit_id == before.unit_id && now.base_id == before.base_id;
}
}

void AddSmacLiveWorkerKnowledgeTests( task::gsetests::GSETests* task ) {
    task->AddTest("SMAC occupancy cache retains source history and repairs Units before Bases",GT() {
        using Cache=map::SmacOccupancyCache;
        auto cache=Cache::Blank(8,8,true);
        GT_ASSERT((cache.At({0,0})==Cache::Cell{15,0}),"Blank cache inferred an owner");
        cache.PlaceUnit({0,0},1,true);cache.PlaceUnit({0,0},0,false);cache.RemoveUnit({0,0},false);
        GT_ASSERT((cache.At({0,0})==Cache::Cell{1,2}),"Mixed stack recounted surviving native affiliation");
        cache.RemoveUnit({0,0},true);
        GT_ASSERT((cache.At({0,0})==Cache::Cell{1,0}),"Last departure cleared cached affiliation");
        cache.FoundBase({2,2},2);cache.PlaceUnit({2,2},1,true);cache.SetAffiliation({2,2},7);
        cache.PlaceUnit({4,4},1,true);cache.SetAffiliation({6,6},3);
        cache.Repair({{{2,2},1},{{4,4},0}},{{{2,2},2}});
        GT_ASSERT((cache.At({2,2})==Cache::Cell{2,3} && cache.At({4,4})==Cache::Cell{0,2} && cache.At({6,6})==Cache::Cell{3,0}),"Load repair lost incoming Base marker, native owner, or empty history");
        auto incoming=Cache::Blank(8,8,true);
        incoming.FoundBase({0,0},2);incoming.SetAffiliation({0,0},7);incoming.Repair({{{0,0},1}},{});
        GT_ASSERT((incoming.At({0,0})==Cache::Cell{7,2}),"Unit repair ignored incoming Base flag before Base repair cleared it");
        cache.RemoveBase({2,2});
        GT_ASSERT((cache.At({2,2})==Cache::Cell{2,2} && cache.At({-4,4})==cache.At({4,4})),"Base removal cleared affiliation or wrapping changed cell");
        const auto before=cache.Serialize();
        GT_ASSERT(Rejects([&]{cache.Repair({{{4,4},1},{{1,0},1}},{});}) && cache.Serialize()==before,"Invalid repair partially changed history");
        GT_ASSERT(Cache::Deserialize(before)==cache,"Cache wire changed semantic cells");
        for(size_t i=0;i<before.size();++i) {GT_ASSERT(Rejects([&]{Cache::Deserialize(before.substr(0,i));}),"Truncated cache accepted");}
        auto bad=before;bad[17]=64;
        GT_ASSERT(Rejects([&]{Cache::Deserialize(bad);}) && Rejects([&]{Cache::Deserialize(before+"x");}),"Malformed or trailing cache accepted");
        Knowledge::Data data{8,8,true,true,0,{},{},{}};data.tiles.resize(32);data.observers[1]=Knowledge::Observer{};
        const auto legacy=Knowledge::Create(data).Serialize();
        GT_ASSERT(!Knowledge::Deserialize(legacy).CopyData().occupancy && Knowledge::Deserialize(legacy).Serialize()==legacy,"V4 restore invented occupancy history");
        data.occupancy=cache;const auto current=Knowledge::Create(data).Serialize();
        GT_ASSERT(Knowledge::Deserialize(current).CopyData()==data,"V5 restore lost occupancy or neighboring knowledge");
        data.occupancy=Cache::Blank(16,8,true);
        GT_ASSERT(Rejects([&]{Knowledge::Create(data);}),"Knowledge accepted mismatched occupancy geometry");GT_OK();
    });

	task->AddTest("SMAC technology reveal stages actual map and Base snapshots and restores exactly",GT() {
		ConventionalUnitFixture fixture; fixture.Configure(); std::string snapshot, knowledge;
		const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game(); const auto world=Populate(GSE_CALL,game,true);
			auto data=Data(); data.all_map_revealed=4;
			data.remembered[1].resize(32,Knowledge::Improvements{0,map::tile::TERRAFORMING_AIRBASE,false,false});
			data.remembered[2]=data.remembered[1];
			Knowledge::BaseObservation prior{901,4,{}}; prior.population[2]=99; data.bases.push_back(prior);
			auto* tile=game->GetMap()->GetTile(4,2); tile->terraforming=map::tile::TERRAFORMING_MINE|map::tile::TERRAFORMING_ROAD;
			game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
			const auto before=Save(fixture,world);
			auto prepared=fixture.PrepareTechnologyReveal(1);
			GT_ASSERT(prepared && !fixture.HasTechnologyReveal(1) && Unchanged(fixture,world,before),"reveal preparation published authority");
			const auto next=prepared->CopyData();
			GT_ASSERT(next.all_map_revealed==6 && next.units==data.units && next.observers==data.observers && next.remembered[2]==data.remembered[2],"revelation changed visibility, observers or other memory");
			for(size_t y=0;y<8;++y){for(size_t x=y&1;x<8;x+=2){
				const auto i=Index(x,y);const auto* actual=game->GetMap()->GetTile(x,y);
				const Knowledge::Improvements expected{actual->features,actual->terraforming,actual->base!=nullptr,!actual->units.empty()};
				GT_ASSERT(next.tiles[i].explored_by==(data.tiles[i].explored_by|2) && next.tiles[i].claimant==data.tiles[i].claimant && next.remembered[1][i]==expected,"reveal did not copy complete current land/sea improvements");
			}}
			GT_ASSERT(next.bases.size()==2,"reveal omitted a foreign Base");
			for(const auto& base:next.bases){GT_ASSERT(base.population[1]==2 && (base.known_by&2),"Base known/population snapshot differs");if(base.id==901){GT_ASSERT(base.population[2]==99 && (base.known_by&4),"other Base memory changed");}}
			fixture.CommitTechnologyReveal(std::move(prepared));
			GT_ASSERT(fixture.HasTechnologyReveal(1) && Base::Serialize(world.local).ToString()==before.base && Base::Serialize(world.foreign).ToString()==before.other
				&& Unit::Serialize(world.local_unit).ToString()==before.unit && Unit::Serialize(world.foreign_unit).ToString()==before.other_unit
				&& fixture.GameplayRandom()==before.pair && game->GetRandom()->GetStateString()==before.jsf && fixture.QueuedFrontendRequests().size()==before.requests,"reveal changed entities, RNG or queues");
			knowledge=game->GetMap()->GetSmacWorkerKnowledge()->Serialize();
			tile->terraforming=map::tile::TERRAFORMING_FARM;
			GT_ASSERT(!fixture.PrepareTechnologyReveal(1) && game->GetMap()->GetSmacWorkerKnowledge()->Serialize()==knowledge,"repeat reveal refreshed the retained snapshot");
			snapshot=fixture.Snapshot();GT_OK();
		});
		if(!error.empty()){return error;}
		ConventionalUnitFixture restored;restored.Configure();
		return restored.Run([&](GSE_CALLABLE)->std::string {
			restored.Restore(GSE_CALL,snapshot,true);
			GT_ASSERT(restored.HasTechnologyReveal(1) && !restored.PrepareTechnologyReveal(1)
				&& restored.Game()->GetMap()->GetSmacWorkerKnowledge()->Serialize()==knowledge,"restore replayed reveal or lost memory");
			// Actual Base retirement removes only its acquired memory record.
			restored.Game()->GetBM()->DespawnBase(GSE_CALL,901);
			const auto after=restored.Game()->GetMap()->GetSmacWorkerKnowledge()->CopyData();
			GT_ASSERT(after.bases.size()==1 && after.bases[0].id==800 && after.bases[0].population[1]==2,"Base retirement retained stale identity or lost survivor memory");GT_OK();
		});
	});
	task->AddTest("SMAC technology reveal rejects invalid bindings before mutation",GT() {
		ConventionalUnitFixture fixture;fixture.Configure();
		return fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();const auto world=Populate(GSE_CALL,game,false);
			GT_ASSERT(Rejects([&]{fixture.PrepareTechnologyReveal(1);}),"absent knowledge inferred");
			game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(Data()));const auto before=Save(fixture,world);
			for(const auto owner:{0,3,8,-1}){GT_ASSERT(Rejects([&]{fixture.PrepareTechnologyReveal(owner);}) && Unchanged(fixture,world,before),"invalid reveal recipient changed world");}
			auto bad=Data();bad.bases.push_back({999,2,{}});
			GT_ASSERT(Rejects([&]{game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(bad));}) && Unchanged(fixture,world,before),"unregistered Base memory admitted");
			GT_OK();
		});
	});

	task->AddTest( "SMAC live ordinary ZOC preserves directed Pact and unknown distinction", GT() {
		auto data=Data(); data.observers[1]->pact_with=4; data.observers[2]->pact_with=0;
		const auto value=Knowledge::Create(data); const auto wire=value.Serialize();
		GT_ASSERT(Knowledge::Deserialize(wire)==value && *value.CopyData().observers[2]->pact_with==0, "Pact direction or explicit zero lost");
		data.observers[2]->pact_with.reset();
		GT_ASSERT(!(Knowledge::Create(data)==value), "unknown Pact state collapsed into no Pacts");
		for(int scenario=0;scenario<4;++scenario) {
			auto bad=wire;
			if(scenario==0){bad[bad.size()-13]|=1;}
			if(scenario==1){bad[bad.size()-5]=1;}
			if(scenario==2){bad.pop_back();}
			if(scenario==3){bad[4]=2;}
			GT_ASSERT(Rejects([&]{Knowledge::Deserialize(bad);}), "invalid directed Pact suffix admitted");
		} GT_OK();
	});
	task->AddTest( "SMAC live ordinary ZOC matches ten original admission and refusal projections", GT() {
		for(const auto& c:zoc_fixture::CASES) {
			ConventionalUnitFixture fixture;fixture.ConfigureMovementCampaign(c.rover);std::string restore_snapshot,restore_knowledge;
			const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
				auto* game=fixture.Game();Definitions(game);
				for(size_t y=0;y<64;++y){for(size_t x=y&1;x<128;x+=2){auto* tile=game->GetMap()->GetTile(x,y);for(auto* corner:tile->elevation.corners){*corner=100;}tile->features=0;tile->terraforming=0;tile->rockiness=map::tile::ROCKINESS_FLAT;tile->Update();}}
				auto* source=game->GetMap()->GetTile(c.sx,c.sy);auto* target=game->GetMap()->GetTile(c.dx,c.dy);
				if(c.road){source->terraforming=target->terraforming=map::tile::TERRAFORMING_ROAD;}
				if(c.base){MakeBase(GSE_CALL,game,901,0,1,c.sx,c.sy,3);}
				Unit* actor=nullptr;
				if(c.rover){
					auto* um=game->GetUM();um->DefineUnit(new StaticDef("zoc-rover",um->GetMoraleSet("knowledge-morale"),"Rover",MT_LAND,999,new CvrScoutRender(2,0,100,100,{1,1},{128,128},CvrScoutRender::Composition::SpartanRover)));
					auto* owner=&game->GetState()->m_slots->GetSlot(0);
					auto* view=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"get_startup_designs",{owner->GetPlayer()->Wrap(GSE_CALL)}));
					const auto design=static_cast<int32_t>(static_cast<value::Int*>(view->Get("starting_scout"))->value);
					actor=um->SpawnConventionalUnit(GSE_CALL,71,um->GetUnitDef("zoc-rover"),owner,source,{design,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0});
				}else{actor=Spawn(GSE_CALL,game,71,0,1,c.sx,c.sy);}
				Knowledge::Data data{128,64,true,true,0,{},{},{}};data.tiles.resize(4096,{6,std::nullopt});data.observers[1]=Knowledge::Observer{};data.observers[2]=Knowledge::Observer{};
				data.observers[1]->pact_with=c.pact?4:0;data.observers[2]->pact_with=2; // opposite direction must not exempt actor
				data.observers[1]->relations[2].foreign_unit_exemption=c.pact||c.treaty;
				data.units.push_back({71,4,false});
				for(int i=0;i<c.enemies;++i){Spawn(GSE_CALL,game,72+i,1,2,c.ex[i],c.ey[i]);const bool unseen=c.newly_revealed&(1u<<i);data.units.push_back({uint64_t(72+i),uint8_t(unseen?0:2),false});if(unseen){data.tiles[c.ey[i]*64+c.ex[i]/2].explored_by=4;}}
				if(c.claim>=0){data.tiles[c.sy*64+c.sx/2].claimant=c.claim;data.tiles[c.dy*64+c.dx/2].claimant=c.claim;}
				if(std::string(c.name)=="both_endpoints"){
					auto unknown=data;unknown.observers[1]->pact_with.reset();game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(unknown));
					const auto before=game->GetMap()->GetSmacWorkerKnowledge()->Serialize();const auto requests=fixture.QueuedFrontendRequests().size();
					GT_ASSERT(Rejects([&]{game->GetUM()->ValidateConventionalMove(actor,target,0);}) && before==game->GetMap()->GetSmacWorkerKnowledge()->Serialize() && requests==fixture.QueuedFrontendRequests().size(),"unknown Pact state silently became neutral or changed publication");
				}
				game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
				const auto before=game->GetMap()->GetSmacWorkerKnowledge()->Serialize();const auto pair=fixture.GameplayRandom();const auto bytes=Unit::Serialize(actor).ToString();const auto requests=fixture.QueuedFrontendRequests().size();
				game->GetUM()->ValidateConventionalMove(actor,target,0);
				GT_ASSERT(before==game->GetMap()->GetSmacWorkerKnowledge()->Serialize() && pair==fixture.GameplayRandom() && bytes==Unit::Serialize(actor).ToString() && requests==fixture.QueuedFrontendRequests().size(),"ZOC validation published refusal side effects");
				game->Event(GSE_CALL,"move_unit",{{"unit",actor->Wrap(GSE_CALL)},{"tile",target->Wrap(GSE_CALL)}});fixture.DrainEvents();
				const auto after=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
				GT_ASSERT(actor->GetTile()==(c.moved?target:source) && actor->Conventional().spent==(c.moved?3:0) && fixture.GameplayRandom()==pair,"original ZOC outcome/spending/RNG differs");
				for(int i=0;i<c.enemies;++i){GT_ASSERT(after.units[i+1].visible_to==2,"actual refusal/sight did not reveal foreign unit");}
				GT_ASSERT(after.observers==data.observers,"movement changed directed diplomatic state");
				if(!c.moved){
					GT_ASSERT(Unit::Serialize(actor).ToString()==bytes && after.units[0].visible_to==4,"ZOC refusal entered normal mover clear/spending");
					for(size_t i=0;i<data.tiles.size();++i){const bool reveal=c.enemies==2 && i==static_cast<size_t>(c.ey[1]*64+c.ex[1]/2);GT_ASSERT(after.tiles[i].explored_by==(reveal?6:data.tiles[i].explored_by),"refusal explored outside unseen destination stack");}
					if(c.enemies==2){const auto index=c.ey[1]*64+c.ex[1]/2;GT_ASSERT(after.remembered[1][index] && after.remembered[1][index]->unit_present,"refusal omitted remembered improvements");}
				}
				if(c.enemies==2){restore_knowledge=game->GetMap()->GetSmacWorkerKnowledge()->Serialize();restore_snapshot=fixture.Snapshot();}
				if(std::string(c.name)=="source_only"){
					for(int x:{28,30,32}){
						game->GetUM()->ResetConventionalSpent(GSE_CALL,actor);
						game->Event(GSE_CALL,"move_unit",{{"unit",actor->Wrap(GSE_CALL)},{"tile",game->GetMap()->GetTile(x,16)->Wrap(GSE_CALL)}});fixture.DrainEvents();
						const auto next=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
						GT_ASSERT(actor->GetTile()->coord.x==static_cast<size_t>(x) && next.units[0].visible_to==(x==32?4:0) && next.units[1].visible_to==2 && fixture.GameplayRandom()==pair,"continuous departure/return recomputed stationary visibility or failed mutual return");
					}
				}
				GT_OK();
			});GT_ASSERT(result.empty(),"ZOC case " + std::string(c.name)+": "+result);
			if(!restore_snapshot.empty()){
				ConventionalUnitFixture restored;restored.ConfigureMovementCampaign(false);
				const auto error=restored.Run([&](GSE_CALLABLE)->std::string{
					restored.RestoreThroughStart(GSE_CALL,restore_snapshot,[](GSE_CALLABLE){});
					GT_ASSERT(restored.Game()->GetMap()->GetSmacWorkerKnowledge()->Serialize()==restore_knowledge && restored.Game()->GetUM()->GetUnit(71)->GetTile()->coord.x==32,"refusal knowledge or directed Pact changed on actual restore");GT_OK();
				});GT_ASSERT(error.empty(),"ZOC restore: " + error);
			}
		}GT_OK();
	});
	task->AddTest( "SMAC live ordinary sight preserves unknown memory and exact remembered copies", GT() {
		auto data=Data(); data.remembered[1].resize(data.tiles.size());
		data.remembered[1][0]=Knowledge::Improvements{0x7fff,0x1fff,true,false};
		data.remembered[7].resize(data.tiles.size()); data.remembered[7].back()=Knowledge::Improvements{0,0,false,true};
		const auto value=Knowledge::Create(data); const auto wire=value.Serialize();
		GT_ASSERT(Knowledge::Deserialize(wire)==value, "remembered fields or unknown entries lost in codec");
		data.remembered[1][0]->features=0;
		GT_ASSERT(value.CopyData().remembered[1][0]->features==0x7fff && value.CopyData().remembered[2].empty(), "memory aliases caller or fabricates absent observer");
		for(int scenario=0;scenario<4;++scenario) {
			auto malformed=wire;
			if(scenario==0) {malformed.pop_back();}
			if(scenario==1) {malformed.push_back('x');}
			if(scenario==2) {malformed.back()=4;}
			if(scenario==3) {malformed[4]=1;}
			GT_ASSERT(Rejects([&]{Knowledge::Deserialize(malformed);}), "malformed remembered suffix admitted");
		}
		GT_OK();
	} );
	task->AddTest( "SMAC live ordinary sight factory publishes mutual visibility and worker filtering", GT() {
		for(int relation=0;relation<3;++relation) {
			ConventionalUnitFixture fixture; fixture.ConfigureMovementCampaign(false);
			const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
				auto* game=fixture.Game(); Definitions(game);
				for(size_t y=0;y<64;++y) {for(size_t x=y&1;x<128;x+=2) {auto* tile=game->GetMap()->GetTile(x,y); for(auto* corner:tile->elevation.corners){*corner=100;} tile->features=0;tile->terraforming=0;tile->rockiness=map::tile::ROCKINESS_FLAT;tile->Update();}}
				auto* base=MakeBase(GSE_CALL,game,901,0,1,16,16,3);
				Knowledge::Data data{128,64,true,true,0,{},{},{}}; data.tiles.resize(4096,{2,std::nullopt}); data.observers[1]=Knowledge::Observer{};data.observers[2]=Knowledge::Observer{};
				data.observers[1]->relations[2].foreign_unit_exemption=relation!=0;
				game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
				auto* foreign=Spawn(GSE_CALL,game,71,1,2,17,15);
				const auto actual=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
				GT_ASSERT(actual.units.size()==1 && actual.units[0].id==71 && actual.units[0].visible_to==2, "base-only factory failed to reveal foreign unit");
				GT_ASSERT(actual.tiles[15*64+17/2].explored_by==6 && actual.tiles[0].explored_by==2, "factory did not publish bounded exploration while preserving other observer bits");
				GT_ASSERT(game->GetSmacWorkerAvailability(*base).unavailable==(relation==0 ? 3u:1u), "stored visibility confused with worker diplomatic exemption");
				GT_ASSERT(actual.remembered[2][15*64+17/2]->unit_present && actual.remembered[2][16*64+16/2]->base_present, "factory memory omitted post-placement Unit or actual Base");
				// A second, adjacent observer updates both masks without changing the
				// already saved diplomatic worker predicate.
				auto* local=Spawn(GSE_CALL,game,72,0,1,18,16);
				const auto seen=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
				GT_ASSERT(seen.units[0].visible_to==2 && seen.units[1].visible_to==4 && local->GetTile()->units.at(72)==local && foreign->GetTile()->units.at(71)==foreign,
					"factory did not publish mutual adjacent visibility with identity");
				GT_OK();
			}); GT_ASSERT(result.empty(), "sight factory fixture: "+result);
		}
		GT_OK();
	} );
	task->AddTest( "SMAC live ordinary sight actual movement clears only mover and restores without replay", GT() {
		ConventionalUnitFixture source; source.ConfigureMovementCampaign(false); std::string snapshot,knowledge;
		const auto result=source.Run([&](GSE_CALLABLE)->std::string {
			auto* game=source.Game(); Definitions(game);
			for(size_t y=0;y<64;++y) {for(size_t x=y&1;x<128;x+=2) {auto* tile=game->GetMap()->GetTile(x,y);for(auto* corner:tile->elevation.corners){*corner=100;}tile->features=0;tile->terraforming=0;tile->rockiness=map::tile::ROCKINESS_FLAT;tile->Update();}}
			auto* local=Spawn(GSE_CALL,game,71,0,1,32,16); Spawn(GSE_CALL,game,72,1,2,48,16);
			auto partial=local->Conventional(); partial.spent=2; game->GetUM()->UpdateConventionalUnit(GSE_CALL,local,partial);
			source.GameplayRandom({0,123});
			Knowledge::Data data{128,64,true,true,0,{},{},{}};data.tiles.resize(4096,{6,std::nullopt});data.observers[1]=Knowledge::Observer{};data.observers[2]=Knowledge::Observer{};
			// Retained supplied history: neither stationary flag is reconstructed
			// from the current separated positions before the actual move event.
			data.units={{71,4,false},{72,2,false}};data.remembered[1].resize(4096);data.remembered[1][0]=Knowledge::Improvements{1,2,true,true};
            data.occupancy=map::SmacOccupancyCache::Blank(128,64,true);data.occupancy->PlaceUnit({32,16},1,true);data.occupancy->PlaceUnit({48,16},2,true);data.occupancy->SetAffiliation({0,0},7);
			game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
			const auto before_attempt=game->GetMap()->GetSmacWorkerKnowledge()->Serialize();
			auto random=source.GameplayRandom();
			GT_ASSERT(random.NextSharedLow16()==62303, "supplied failed-attempt draw changed");
			auto* destination=game->GetMap()->GetTile(30,16);
			auto* args=VALUE(value::Object,,GSE_CALL_NOGC,value::object_properties_t({{"unit",local->Wrap(GSE_CALL)},{"tile",destination->Wrap(GSE_CALL)}}));
			Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"event",{VALUE(value::String,,"move_unit"),args});source.DrainEvents();
			GT_ASSERT(local->GetTile()->coord.x==32 && local->Conventional().spent==3 && local->Conventional().failed_move
				&& source.GameplayRandom()==random && game->GetMap()->GetSmacWorkerKnowledge()->Serialize()==before_attempt,
				"accepted failed lottery changed sight or failed to retain its actual draw/spent marker");
			game->GetUM()->ResetConventionalSpent(GSE_CALL,local);
			Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"event",{VALUE(value::String,,"move_unit"),args});source.DrainEvents();
			const auto actual=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
			GT_ASSERT(local->GetTile()==destination && local->Conventional().spent==3 && actual.units[0].visible_to==0 && actual.units[1].visible_to==2, "actual movement globally rebuilt visibility or omitted mover clear");
			GT_ASSERT(actual.remembered[1][16*64+30/2]->unit_present && !actual.remembered[1][16*64+32/2]->unit_present && actual.remembered[1][0]==data.remembered[1][0], "movement memory missed membership timing or refreshed unrelated old memory");
            GT_ASSERT((actual.occupancy && actual.occupancy->At({32,16})==map::SmacOccupancyCache::Cell{1,0} && actual.occupancy->At({30,16})==map::SmacOccupancyCache::Cell{1,2} && actual.occupancy->At({0,0})==map::SmacOccupancyCache::Cell{7,0}),"Actual move lost cached affiliation history or membership");
			GT_ASSERT(source.GameplayRandom()==random, "ordinary full move consumed a sight RNG draw");
			knowledge=game->GetMap()->GetSmacWorkerKnowledge()->Serialize();snapshot=source.Snapshot();
			GT_ASSERT(Rejects([&]{game->GetUM()->MoveConventionalUnit(GSE_CALL,local,game->GetMap()->GetTile(28,16),0);}) && game->GetMap()->GetSmacWorkerKnowledge()->Serialize()==knowledge, "exhausted command changed sight");
			GT_OK();
		});GT_ASSERT(result.empty(),"sight move fixture: "+result);
		ConventionalUnitFixture destination;destination.ConfigureMovementCampaign(false);
		return destination.Run([&](GSE_CALLABLE)->std::string {
			destination.RestoreThroughStart(GSE_CALL,snapshot,[](GSE_CALLABLE){});
			GT_ASSERT(destination.Game()->GetMap()->GetSmacWorkerKnowledge()->Serialize()==knowledge && destination.Game()->GetUM()->GetUnit(71)->GetTile()->coord.x==30, "restore replayed sight or lost remembered state");GT_OK();
		});
	} );
	task->AddTest( "SMAC live ordinary sight rejects unsupported detection before publication", GT() {
		ConventionalUnitFixture fixture;fixture.ConfigureMovementCampaign(false);
		return fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();Definitions(game);
			for(size_t y=0;y<64;++y){for(size_t x=y&1;x<128;x+=2){auto* tile=game->GetMap()->GetTile(x,y);for(auto* corner:tile->elevation.corners){*corner=100;}tile->features=0;tile->terraforming=0;tile->rockiness=map::tile::ROCKINESS_FLAT;tile->Update();}}
			Spawn(GSE_CALL,game,71,0,1,32,16);
			Knowledge::Data data{128,64,true,true,0,{},{},{}};data.tiles.resize(4096,{2,std::nullopt});data.observers[1]=Knowledge::Observer{};data.observers[2]=Knowledge::Observer{};data.units={{71,0,false}};
			game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));const auto bytes=game->GetMap()->GetSmacWorkerKnowledge()->Serialize();const auto next=Unit::GetNextId();const auto queued=fixture.QueuedFrontendRequests().size();
			auto* tile=game->GetMap()->GetTile(33,15);tile->features=map::tile::FEATURE_XENOFUNGUS;
			GT_ASSERT(Rejects([&]{Spawn(GSE_CALL,game,72,1,2,33,15);}) && game->GetUM()->GetUnits().size()==1 && tile->units.empty() && Unit::GetNextId()==next && fixture.QueuedFrontendRequests().size()==queued && game->GetMap()->GetSmacWorkerKnowledge()->Serialize()==bytes,
				"unsupported detection partly published identity, membership, queues or sight");GT_OK();
		});
	} );

	task->AddTest( "SMAC live worker knowledge joins actual entities and terrain without query effects", []( GSE*, context::Context* ) -> std::string {
		for ( bool sea : { false, true } ) {
			ConventionalUnitFixture fixture; fixture.Configure();
			const auto result = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); const auto world = Populate( GSE_CALL, game, sea ); auto data = Data();
				for ( int scenario = 0; scenario < 4; ++scenario ) {
					data.units[ 1 ].visible_to = scenario == 1 ? 0 : 2;
					data.observers[ 1 ]->relations[ 2 ] = { scenario == 2, scenario == 3 };
					game->PublishSmacWorkerKnowledge( GSE_CALL, Knowledge::Create( data ) ); const auto before = Save( fixture, world );
					const auto actual = game->GetSmacWorkerAvailability( *world.local );
					uint32_t expected = 1u | ( 1u << 6 ) | ( 1u << 10 ) | ( 1u << 13 ) | ( 1u << 20 );
					if ( sea ) { expected |= 1u << 2; }
					if ( scenario != 1 && scenario != 2 ) { expected |= 1u << 3; }
					if ( scenario == 3 ) { expected |= 1u << 5; }
					GT_ASSERT( actual.unavailable == expected && actual.selected_radius == 3, "live IDs/terrain/foreign allocation or visibility projected incorrectly" );
					GT_ASSERT( actual.squares[ 5 ]->claimant == 2 && actual.squares[ 5 ]->recognized_owner == ( scenario == 3 ? std::optional< int32_t >( 2 ) : std::nullopt )
						&& actual.squares[ 7 ]->claimant == 0 && !actual.squares[ 7 ]->recognized_owner, "raw/native claims conflated with recognition" );
					GT_ASSERT( world.local->GetSmacAllocation()->selected == 0x80000003u && world.foreign->GetSmacAllocation()->selected == 0x40000101u,
						"consumer projection truncated saved selected32" );
					(void)game->GetSmacWorkerAvailability( *world.foreign );
					GT_ASSERT( Unchanged( fixture, world, before ), "observational query changed world, queues, IDs or either RNG" );
				}
				const auto installed = game->GetMap()->GetSmacWorkerKnowledge()->Serialize();
				auto replacement = *world.foreign->GetSmacAllocation(); replacement.selected = 0x40000001u; replacement.specialists.total_count = 2;
				world.foreign->PublishSmacAllocation( GSE_CALL, replacement, Roles() );
				GT_ASSERT( !( game->GetSmacWorkerAvailability( *world.local ).unavailable & ( 1u << 2 ) )
					&& game->GetMap()->GetSmacWorkerKnowledge()->Serialize() == installed, "live allocation replacement was hidden behind a copied Base list" );
				GT_OK();
			} ); GT_ASSERT( result.empty(), "live fixture failed: " + result );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC live worker knowledge rejects replacement before publishing any state", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); const auto world = Populate( GSE_CALL, game, false );
			game->PublishSmacWorkerKnowledge( GSE_CALL, Knowledge::Create( Data() ) ); const auto before = Save( fixture, world );
			for ( int scenario = 0; scenario < 7; ++scenario ) {
				auto data = Data();
				if ( scenario == 0 ) { data.width = 16; data.tiles.resize( 64 ); }
				if ( scenario == 1 ) { data.horizontal_wrap = false; }
				if ( scenario == 2 ) { data.units.pop_back(); }
				if ( scenario == 3 ) { data.units.push_back( { 99, 0, false } ); }
				if ( scenario == 4 ) { data.units[ 1 ].id = 99; }
				if ( scenario == 5 ) { data.units[ 1 ].active_convoy = true; }
				if ( scenario == 6 ) { data.observers[ 7 ] = Knowledge::Observer{}; }
				const auto candidate = Knowledge::Create( data );
				GT_ASSERT( Rejects( [ & ] { game->PublishSmacWorkerKnowledge( GSE_CALL, candidate ); } ) && Unchanged( fixture, world, before ),
					"invalid replacement changed installed knowledge/entities/queues" );
			}
			// Presence for another admitted observer never manufactures this Base's row.
			auto data = Data(); data.observers[ 1 ].reset(); game->PublishSmacWorkerKnowledge( GSE_CALL, Knowledge::Create( data ) );
			GT_ASSERT( Rejects( [ & ] { game->GetSmacWorkerAvailability( *world.local ); } ), "missing observer row silently inherited another participant" );
			(void)game->GetSmacWorkerAvailability( *world.foreign ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live worker knowledge authenticates forward and reverse entity membership", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); const auto world = Populate( GSE_CALL, game, false ); const auto knowledge = Knowledge::Create( Data() );
			game->PublishSmacWorkerKnowledge( GSE_CALL, knowledge ); const auto before = Save( fixture, world );
			for ( int scenario = 0; scenario < 5; ++scenario ) {
				auto* tile = world.foreign_unit->GetTile(); auto* spare = game->GetMap()->GetTile( 6, 4 );
				const auto center = world.foreign->GetTile()->base; auto* owner = world.foreign_unit->m_owner;
				if ( scenario == 0 ) { tile->units.erase( world.foreign_unit->m_id ); }
				if ( scenario == 1 ) { spare->units.emplace( 17, world.foreign_unit ); }
				if ( scenario == 2 ) { world.foreign->GetTile()->base = nullptr; }
				if ( scenario == 3 ) { spare->base = world.foreign; }
				if ( scenario == 4 ) { world.foreign_unit->m_owner = world.local_unit->m_owner; }
				const bool query_rejected = Rejects( [ & ] { game->GetSmacWorkerAvailability( *world.local ); } );
				const bool publish_rejected = Rejects( [ & ] { game->PublishSmacWorkerKnowledge( GSE_CALL, knowledge ); } );
				tile->units[ 17 ] = world.foreign_unit; spare->units.clear(); spare->base = nullptr; world.foreign->GetTile()->base = center; world.foreign_unit->m_owner = owner;
				GT_ASSERT( query_rejected && publish_rejected && Unchanged( fixture, world, before ), "unbound live entity admitted or rejection changed stored state" );
			}
			GT_OK();
		} );
	} );
	task->AddTest( "SMAC live worker knowledge requires canonical allocation and conventional unit authority", []( GSE*, context::Context* ) -> std::string {
		for ( bool legacy_unit : { false, true } ) {
			ConventionalUnitFixture fixture; fixture.Configure();
			const auto result = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); const auto world = Populate( GSE_CALL, game, false ); auto data = Data();
				auto* owner = world.local->m_owner; auto* tile = game->GetMap()->GetTile( 7, 5 );
				if ( legacy_unit ) {
					auto* legacy = new Unit( GSE_CALL, game->GetUM(), 57, world.local_unit->m_def, owner, tile, 1, 1, 1, false );
					game->GetUM()->SpawnUnit( GSE_CALL, legacy ); data.units.push_back( { 57, 0, false } );
				}
				else {
					auto* unallocated = new Base( game, 902, owner, owner->GetPlayer()->GetFaction(), tile, "No allocation", {}, {}, 1, 0, {},
						SmacBaseResourceContext{ 1, false, 0, 0, 0, false } ); game->GetBM()->SpawnBase( GSE_CALL, unallocated );
				}
				const auto snapshot = fixture.Snapshot(); const auto next_unit = Unit::GetNextId(), next_base = Base::GetNextId(); const auto queued = fixture.QueuedFrontendRequests().size();
				GT_ASSERT( Rejects( [ & ] { game->PublishSmacWorkerKnowledge( GSE_CALL, Knowledge::Create( data ) ); } ) && !game->GetMap()->GetSmacWorkerKnowledge()
					&& fixture.Snapshot() == snapshot && Unit::GetNextId() == next_unit && Base::GetNextId() == next_base && fixture.QueuedFrontendRequests().size() == queued,
					"legacy Unit or missing full Base allocation admitted by inventing canonical state" ); GT_OK();
			} ); GT_ASSERT( result.empty(), "authority fixture failed: " + result );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC live worker knowledge restores after both actual manager queues", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture source; source.Configure(); std::string snapshot, knowledge, base, unit; Frame::Evaluation expected{};
		auto result = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = source.Game(); const auto world = Populate( GSE_CALL, game, true );
			game->PublishSmacWorkerKnowledge( GSE_CALL, Knowledge::Create( Data() ) );
			snapshot = source.Snapshot(); knowledge = game->GetMap()->GetSmacWorkerKnowledge()->Serialize(); base = Base::Serialize( world.local ).ToString(); unit = Unit::Serialize( world.foreign_unit ).ToString();
			expected = game->GetSmacWorkerAvailability( *world.local ); GT_OK();
		} ); GT_ASSERT( result.empty(), "source snapshot failed: " + result );
		for ( bool no_current : { false, true } ) {
			ConventionalUnitFixture destination; destination.Configure(); destination.GameplayRandom( { 99, 101 } );
			result = destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				destination.Restore( GSE_CALL, snapshot, no_current ); auto* game = destination.Game();
				auto* restored_base = game->GetBM()->GetBase( 901 ); auto* restored_unit = game->GetUM()->GetUnit( 17 );
				GT_ASSERT( restored_base && restored_unit && game->GetMap()->GetSmacWorkerKnowledge()->Serialize() == knowledge
					&& Base::Serialize( restored_base ).ToString() == base && Unit::Serialize( restored_unit ).ToString() == unit
					&& game->GetSmacWorkerAvailability( *restored_base ) == expected, "body restore lost knowledge/entity authority or published before pending registries" );
				GT_ASSERT( restored_base->GetSmacAllocation()->selected == 0x80000003u && restored_base->GetSmacAllocation()->specialists.types[ 15 ] == 2,
					"restore recreated active-only allocation instead of full saved authority" );
				GT_ASSERT( destination.GameplayRandom() == source.GameplayRandom(), "restore changed paired RNG" ); GT_OK();
			} ); GT_ASSERT( result.empty(), "restored fixture failed: " + result );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC live worker knowledge fences entity terrain and turn mutation before effects", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); const auto world = Populate( GSE_CALL, game, false );
			game->PublishSmacWorkerKnowledge( GSE_CALL, Knowledge::Create( Data() ) ); const auto before = Save( fixture, world );
			auto* target = game->GetMap()->GetTile( 7, 5 ); auto* um = game->GetUM(); auto* bm = game->GetBM(); bool called = false;
			std::vector< std::function< void() > > mutations{
				[ & ] { world.local_unit->SetTile( GSE_CALL, target ); },
				[ & ] { um->ValidateConventionalMove( world.local_unit, target, 0 ); },
				[ & ] { um->MoveConventionalUnit( GSE_CALL, world.local_unit, target, 0 ); },
				[ & ] { std::unique_ptr< const std::string > error( um->MoveUnitToTile( GSE_CALL, world.local_unit, target, [ & ] { called = true; } ) ); },
				[ & ] { Spawn( GSE_CALL, game, Unit::GetNextId(), 0, 1, 7, 5 ); },
				[ & ] { um->DespawnUnit( GSE_CALL, world.local_unit->m_id ); },
				[ & ] { um->SpawnUnit( GSE_CALL, world.local_unit ); },
				[ & ] { bm->DespawnBase( GSE_CALL, 999999 ); },
				[ & ] { bm->SpawnBase( GSE_CALL, world.local ); },
				[ & ] { std::unique_ptr< Base > rejected( new Base( game, Base::GetNextId(), world.local->m_owner, world.local->m_faction, target, "Rejected", {}, {} ) ); },
				[ & ] { target->Clear(); }, [ & ] { target->Deserialize( target->Serialize() ); },
				[ & ] { game->GetMap()->GetTilesPtr()->Clear(); }, [ & ] { game->GetMap()->GetTilesPtr()->Resize( 8, 8 ); },
				[ & ] { game->GetMap()->GetTilesPtr()->FixTopBottomRows( game->GetRandom() ); },
				[ & ] { game->Event( GSE_CALL, "", {} ); }, [ & ] { game->CompleteTurn( GSE_CALL, 0 ); },
				[ & ] { game->AdvanceTurn( 2 ); },
				[ & ] { auto tiles = game->GetMap()->GetTilesPtr()->Serialize(); (void)game->GetMap()->LoadFromBuffer( tiles ); },
				[ & ] { Call( GSE_CALL, static_cast< value::Object* >( um->Wrap( GSE_CALL ) ), "despawn_unit", { world.local_unit->Wrap( GSE_CALL ) } ); },
				[ & ] { Call( GSE_CALL, static_cast< value::Object* >( bm->Wrap( GSE_CALL ) ), "despawn_base", { VALUE(value::Int,,999999) } ); }
			};
			for ( size_t index=0; index<mutations.size(); ++index ) {
				const auto& mutation=mutations[index];
				// Canonical turn admission now rejects AdvanceTurn before the generic knowledge fence.
				GT_ASSERT( ( ( index==1 || index==2 || index==4 || index==7 || index==8 || index==9 || index==17 || index==20 ) ? Rejects(mutation) : KnowledgeFence(mutation) ) && !called && !target->IsLocked() && !world.local_unit->m_animation_id && Unchanged( fixture, world, before ),
					"guarded mutation changed IDs/membership/queues/terrain/turn/RNG or ran callback at index " + std::to_string(index) );
			}
			GT_OK();
		} );
	} );
	task->AddTest( "SMAC live worker knowledge fences already queued event callbacks", []( GSE*, context::Context* ) -> std::string {
		for ( bool installed : { false, true } ) {
			ConventionalUnitFixture fixture; fixture.Configure(); auto calls = std::make_shared< size_t >( 0 );
			const auto result = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); const auto world = Populate( GSE_CALL, game, false );
				fixture.QueueProbeEvent( GSE_CALL, [ calls, game ] { ++*calls; game->GetRandom()->GetUInt(); } );
				if ( installed ) { game->PublishSmacWorkerKnowledge( GSE_CALL, Knowledge::Create( Data() ) ); }
				const auto before = Save( fixture, world );
				if ( installed ) {
					GT_ASSERT( KnowledgeFence( [ & ] { fixture.DrainEvents(); } ) && *calls == 0 && Unchanged( fixture, world, before ),
						"captured event ran validation/resolution/apply or spent RNG after knowledge publication" );
				}
				else { fixture.DrainEvents(); GT_ASSERT( *calls == 3 && game->GetRandom()->GetStateString() != before.jsf, "absent-knowledge control did not execute all real handler phases" ); }
				GT_OK();
			} ); GT_ASSERT( result.empty(), "event fixture failed: " + result );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC live worker knowledge remains unavailable during actual restored Start callback", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture source; source.Configure(); std::string snapshot; Base* original = nullptr; Frame::Evaluation expected{};
		auto result = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			const auto world = Populate( GSE_CALL, source.Game(), true ); original = world.local;
			source.Game()->PublishSmacWorkerKnowledge( GSE_CALL, Knowledge::Create( Data() ) );
			snapshot = source.Snapshot(); expected = source.Game()->GetSmacWorkerAvailability( *original ); GT_OK();
		} ); GT_ASSERT( result.empty(), "source setup failed: " + result );
		ConventionalUnitFixture destination; destination.Configure();
		return destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = destination.Game(); size_t starts = 0; bool pending = false, query_blocked = false, spawn_blocked = false, base_blocked = false;
			destination.RestoreThroughStart( GSE_CALL, snapshot, [ & ]( GSE_CALLABLE ) {
				++starts; pending = game->GetMap()->GetSmacWorkerKnowledge().has_value() && game->GetUM()->GetUnits().empty() && game->GetBM()->GetBases().empty();
				// No destination Base has been published yet. The readiness gate
				// must reject before inspecting the supplied Base reference.
				try { (void)game->GetSmacWorkerAvailability( *original ); } catch ( const std::runtime_error& e ) { query_blocked = std::string( e.what() ) == "Live worker knowledge is unavailable"; }
				const auto next_unit = Unit::GetNextId(), next_base = Base::GetNextId(); const auto requests = destination.QueuedFrontendRequests().size();
				spawn_blocked = Rejects( [ & ] { Spawn( GSE_CALL, game, next_unit, 0, 1, 7, 5 ); } );
				auto* owner = &game->GetState()->m_slots->GetSlot( 0 );
				base_blocked = Rejects( [ & ] { std::unique_ptr< Base > rejected( new Base( game, next_base, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( 7, 5 ), "Start rejection", {}, {} ) ); } );
				if ( Unit::GetNextId() != next_unit || Base::GetNextId() != next_base || destination.QueuedFrontendRequests().size() != requests ) { throw std::runtime_error( "start rejection changed IDs or queues" ); }
			} );
			GT_ASSERT( starts == 1 && pending && query_blocked && spawn_blocked && base_blocked, "restored start escaped knowledge/producer readiness gate" );
			GT_ASSERT( game->GetUM()->GetUnits().size() == 2 && game->GetBM()->GetBases().size() == 2
				&& game->GetSmacWorkerAvailability( *game->GetBM()->GetBase( 901 ) ) == expected, "actual InitComplete did not publish knowledge after both manager drains" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live worker knowledge world prefix bounds before copying and map admission", []( GSE*, context::Context* ) -> std::string {
		using namespace game::backend::world_snapshot;
		const auto knowledge = Knowledge::Create( Data() ); types::Buffer valid;
		WriteWorkerKnowledge( valid, knowledge, true ); valid.WriteInt( 713 );
		GT_ASSERT( ReadWorkerKnowledge( valid, true ) == std::optional< Knowledge >( knowledge ) && valid.ReadInt() == 713, "bounded prefix did not preserve following body" );
		for ( size_t length : { Knowledge::MAX_WIRE_BYTES, Knowledge::MAX_WIRE_BYTES + 1, Knowledge::MAX_WIRE_BYTES + 4096 } ) {
			types::Buffer input; input.WriteBool( true ); const auto boundary = input.lenw; input.WriteString( std::string( length, 'x' ) );
			GT_ASSERT( Rejects( [ & ] { ReadWorkerKnowledge( input, true ); } ), "invalid knowledge frame admitted" );
			GT_ASSERT( ( length > Knowledge::MAX_WIRE_BYTES && input.lenr == boundary ) || ( length == Knowledge::MAX_WIRE_BYTES && input.lenr == input.lenw ),
				"outer length rejection conflated with bounded nested codec rejection" );
		}
		types::Buffer non_smac; non_smac.WriteBool( true ); const auto boundary = non_smac.lenw; non_smac.WriteString( knowledge.Serialize() );
		GT_ASSERT( Rejects( [ & ] { ReadWorkerKnowledge( non_smac, false ); } ) && non_smac.lenr == boundary, "non-SMAC presence consumed the payload" );
		types::Buffer output; output.WriteInt( 91 ); const auto before = output.ToString();
		GT_ASSERT( Rejects( [ & ] { WriteWorkerKnowledge( output, knowledge, false ); } ) && output.ToString() == before, "rejected writer appended a partial prefix" );
		types::Buffer map; map.WriteInt( 8 ); map.WriteInt( 8 ); ValidateWorkerKnowledgeGeometry( knowledge, map.ToString() );
		for ( const auto dimensions : { std::pair< int, int >{ 6, 8 }, { 8, 6 }, { -1, 8 } } ) {
			types::Buffer bad; bad.WriteInt( dimensions.first ); bad.WriteInt( dimensions.second );
			GT_ASSERT( Rejects( [ & ] { ValidateWorkerKnowledgeGeometry( knowledge, bad.ToString() ); } ), "knowledge/map geometry mismatch reached map decoding" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC live worker knowledge absence preserves existing explicit entity lifecycle", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); const auto world = Populate( GSE_CALL, game, false );
			GT_ASSERT( !game->GetMap()->GetSmacWorkerKnowledge() && Rejects( [ & ] { game->GetSmacWorkerAvailability( *world.local ); } ), "absence invented worker knowledge" );
			auto* target = game->GetMap()->GetTile( 7, 5 ); world.local_unit->SetTile( GSE_CALL, target );
			GT_ASSERT( target->units.at( 91 ) == world.local_unit && !game->GetMap()->GetTile( 6, 6 )->units.count( 91 ), "absence changed existing relocation" );
			game->GetUM()->DespawnUnit( GSE_CALL, 91 ); game->GetBM()->DespawnBase( GSE_CALL, 901 );
			GT_ASSERT( !game->GetUM()->GetUnit( 91 ) && !game->GetBM()->GetBase( 901 ) && !target->units.count( 91 ), "absence changed manager removal" );
			target->Clear(); GT_OK();
		} );
	} );
}
}

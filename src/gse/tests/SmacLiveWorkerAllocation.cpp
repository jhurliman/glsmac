#include "SmacLiveWorkerAllocation.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacStartupSource.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/Random.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/faction/Faction.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tiles.h"
#include "game/backend/resource/Resource.h"
#include "game/backend/resource/ResourceManager.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/StaticDef.h"
#include "game/backend/unit/MoraleSet.h"
#include "game/backend/unit/CvrScoutRender.h"
#include "game/FrontendRequest.h"
#include "gse/value/Array.h"
#include "gse/value/Callable.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"
#include "gse/value/Undefined.h"
#include <sstream>

namespace gse::tests {
namespace {
using namespace game::backend;
using namespace game::backend::base;
using namespace game::backend::map::tile;
using Knowledge = map::SmacWorkerKnowledge;

bool Rejects( const std::function< void() >& call ) { try { call(); } catch ( const std::exception& ) { return true; } return false; }
Value* Call( GSE_CALLABLE, value::Object* object, const std::string& name, const value::function_arguments_t& args = {} ) {
	return static_cast< value::Callable* >( object->Get( name ) )->Run( GSE_CALL, args );
}
std::string Source( const std::string& filename ) {
	auto source = smac_startup_fixture::Source( filename );
	if ( filename != "alpha.txt" ) { return source; }
	const auto replace = [ & ]( const std::string& name, const std::string& text ) {
		const auto begin = source.find( name ) + name.size(); const auto end = source.find( '#', begin ); source.replace( begin, end - begin, text );
	};
	replace( "#RESOURCEINFO\n", "Ocean,1,0,0\nCenter,2,1,1\nBonus,2,2,2\nForest,1,2,1\nRecycling,1,1,1\nLand,1,0,0\nSea,1,0,0\nMonolith,2,2,2\nBorehole,0,6,6\n" );
	std::string citizens;
	for ( int i = 0; i < 7; ++i ) { citizens += "Same name,Same plurals," + std::string( i < 2 ? "None" : "Disable" ) + ",Disable,"
		+ ( i == 0 ? "3,0,0" : i == 1 ? "0,2,0" : "0,0,0" ) + ",opaque\n"; }
	replace( "#CITIZENS\n", citizens + "A,As,\nB,Bs,\nC,Cs,\n" );
	std::istringstream input( source ); std::string result, line, section; size_t row = 0;
	while ( std::getline( input, line ) ) {
		if ( !line.empty() && line.front() == '#' ) { section = line; row = 0; }
		else {
			if ( section == "#RULES" && ( row == 1 || row == 10 ) ) { line.replace( 0, line.find( ',' ), row == 1 ? "2" : "5" ); }
			++row;
		}
		result += line + '\n';
	}
	return result;
}
Base::SmacAllocationRoles Roles() {
	Base::SmacAllocationRoles result{ "live-worker", {} };
	for ( size_t i = 0; i < 7; ++i ) { result.specialists[ i ] = "live-role-" + std::to_string( i ); } return result;
}
ProductionState Production( ProductionItemKind kind = ProductionItemKind::Unit ) {
	const ProductionItem item{ kind, kind == ProductionItemKind::Stockpile ? "" : "opaque-target" };
	return { item, 17, { ProductionItemKind::Stockpile, "" }, 5 };
}
Base* Populate( GSE_CALLABLE, ConventionalUnitFixture& fixture, bool doctor, bool homed = false ) {
	auto* game = fixture.Game(); auto* bm = game->GetBM(); const auto roles = Roles();
	for ( const auto* id : { "NUTRIENTS", "MINERALS", "ENERGY" } ) { game->GetRM()->DefineResource( new game::backend::resource::Resource( id, id, {} ) ); }
	bm->DefinePop( new PopDef( roles.worker, "Display only", {}, {}, PopDef::PF_TILE_WORKER ) );
	for ( uint8_t i = 0; i < 7; ++i ) { bm->DefinePop( new PopDef( roles.specialists[ i ], "Display only", {}, {}, PopDef::PF_NONE, i ) ); }
	for ( size_t y = 0; y < 8; ++y ) { for ( size_t x = y & 1; x < 8; x += 2 ) {
		auto* tile = game->GetMap()->GetTile( x, y );
		for ( auto* corner : tile->elevation.corners ) { *corner = 0; }
		tile->Update(); tile->moisture = MOISTURE_ARID; tile->rockiness = ROCKINESS_FLAT; tile->features = 0; tile->terraforming = 0; tile->bonus = BONUS_NONE;
	} }
	game->GetMap()->GetTile( 4, 2 )->terraforming = TERRAFORMING_FOREST;
	auto* owner = &game->GetState()->m_slots->GetSlot( 0 );
	auto* base = new Base( game, 701, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( 2, 2 ), "Live allocation", {},
		{ { 19, Pop( nullptr, 19, bm->GetPopDef( roles.specialists[ 1 ] ), 1 ) } }, 41, 13, Production(), SmacBaseResourceContext{ 1, false, 0, 0, 0x40 } );
	bm->SpawnBase( GSE_CALL, base );
	SmacBaseAllocationState allocation{ doctor ? 1u : 3u, { {}, doctor ? 1 : 0 } };
	for ( size_t i = 0; i < 16; ++i ) { allocation.specialists.types[ i ] = i ? ( i + 2 ) % 7 : 1; }
	base->PublishSmacAllocation( GSE_CALL, allocation, roles );
	// An actual conventional Unit and optional saved home, not an injected support total.
	using namespace game::backend::unit;
	MoraleSet::morale_values_t names; for ( int i = 0; i < 7; ++i ) { names.emplace_back( "level" + std::to_string( i ) ); }
	auto* um = game->GetUM(); um->DefineMoraleSet( new MoraleSet( "live-morale", names ) );
	um->DefineUnit( new StaticDef( "live-scout", um->GetMoraleSet( "live-morale" ), "Scout", MT_LAND, 999,
		new CvrScoutRender( 2, 0, 100, 100, { 1, 1 }, { 128, 128 } ) ) );
	ConventionalUnitState record{ 2, 1, 1, 0, 0, false, ConventionalUnitState::Home::None, SmacMoraleMode::Mode0 };
	if ( homed ) { record.home = ConventionalUnitState::Home::Base; record.home_base_id = base->m_id; }
	um->SpawnConventionalUnit( GSE_CALL, 91, um->GetUnitDef( "live-scout" ), owner, game->GetMap()->GetTile( 6, 6 ), record );
	fixture.PrepareBaseCallbacks();
	auto* setup = static_cast< value::Callable* >( fixture.Script( GSE_CALL, "./GLSMAC_data/tests/_base_growth" ) );
	setup->Run( GSE_CALL, { game->Wrap( GSE_CALL ) } );
	return base;
}
Knowledge KnowledgeValue( bool old_available = false, bool candidate_available = true ) {
	Knowledge::Data data{ 8, 8, true, true, 0, {}, {}, { { 91, 0, false } } };
	data.observers[ 1 ] = Knowledge::Observer{ false, {} }; data.tiles.resize( 32, { 0, {} } );
	for ( const auto xy : { std::array< int, 2 >{ 2, 2 }, { 3, 1 }, { 4, 2 } } ) {
		auto& tile = data.tiles[ xy[ 1 ] * 4 + xy[ 0 ] / 2 ]; tile.claimant = 1;
		tile.explored_by = xy[ 0 ] == 3 ? old_available ? 2 : 0 : xy[ 0 ] == 4 ? candidate_available ? 2 : 0 : 2;
	}
	return Knowledge::Create( data );
}
void RecalculateViaWrapper( GSE_CALLABLE, Base* base ) {
	value::array_elements_t specialists; for ( const auto& role : Roles().specialists ) { specialists.push_back( VALUE( value::String,, role ) ); }
	Call( GSE_CALL, static_cast< value::Object* >( base->Wrap( GSE_CALL ) ), "recalculate_smac_workers",
		{ VALUE( value::String,, Roles().worker ), VALUE( value::Array,, specialists ) } );
}
}

void AddSmacLiveWorkerAllocationTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC live worker recalculation repairs actual unavailable assignments", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false );
			game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue() );
			auto* pop = &base->m_pops.at( 19 ); auto* wrapper = static_cast< value::Object* >( pop->Wrap( GSE_CALL ) );
			const auto prior = *base->GetSmacAllocation(); const auto knowledge = game->GetMap()->GetSmacWorkerKnowledge()->Serialize();
			const auto rng = game->GetRandom()->GetStateString(); const auto pair = fixture.GameplayRandom(); const auto production = base->GetProductionState();
			RecalculateViaWrapper( GSE_CALL, base );
			const auto actual = *base->GetSmacAllocation();
			GT_ASSERT( actual.selected == 5 && actual.specialists.total_count == 0 && actual.specialists.types == prior.specialists.types, "actual unavailable worker was not repaired onto the explored forest" );
			GT_ASSERT( &base->m_pops.at( 19 ) == pop && Pop::Unwrap( wrapper ) == pop && pop->m_variant == 1
				&& !game->GetBM()->GetWorkingPop( { 3, 1 } ) && game->GetBM()->GetWorkingPop( { 4, 2 } ) == pop, "live publication lost stable Pop or reverse membership" );
			auto* intake = static_cast< value::Object* >( Call( GSE_CALL, static_cast< value::Object* >( base->Wrap( GSE_CALL ) ), "get_intake" ) );
			GT_ASSERT( static_cast< value::Int* >( intake->Get( "NUTRIENTS" ) )->value == 3
				&& static_cast< value::Int* >( intake->Get( "MINERALS" ) )->value == 3, "actual intake callback did not consume the repaired live worker" );
			GT_ASSERT( game->GetMap()->GetSmacWorkerKnowledge()->Serialize() == knowledge && game->GetRandom()->GetStateString() == rng
				&& fixture.GameplayRandom() == pair && base->GetProductionState() == production && base->GetAccumulatedNutrients() == 13, "allocation changed unrelated authority" );
			bool refresh = false; for ( const auto& update : fixture.Updates() ) { if ( update.type == game::FrontendRequest::FR_BASE_UPDATE && update.data.base_update.base_id == 701 ) { refresh = true; } }
			GT_ASSERT( refresh, "repaired live allocation did not enqueue player presentation" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live worker recalculation retains valid Doctors and typed production", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, true ); game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true ) );
			const auto allocation = base->GetSmacAllocation(); auto* pop = &base->m_pops.at( 19 );
			for ( const auto kind : { ProductionItemKind::Unit, ProductionItemKind::Facility, ProductionItemKind::Stockpile } ) {
				base->SetProductionState( GSE_CALL, Production( kind ) ); const auto before = Base::Serialize( base ).ToString();
				base->RecalculateSmacWorkers( GSE_CALL, Roles() );
				GT_ASSERT( Base::Serialize( base ).ToString() == before && base->GetSmacAllocation() == allocation && &base->m_pops.at( 19 ) == pop
					&& !pop->GetWorkedTileCoords() && pop->m_def->m_smac_specialist_id == 1, "ordinary recalculation optimized a complete valid Doctor allocation" );
				base->RecalculateSmacWorkers( GSE_CALL, Roles(), SmacBaseResourcePhase::Upkeep );
				GT_ASSERT( Base::Serialize( base ).ToString() == before, "unmanaged upkeep changed a complete valid Doctor allocation" );
				GT_ASSERT( Rejects( [ & ] { base->RecalculateSmacWorkers( GSE_CALL, Roles(), static_cast< SmacBaseResourcePhase >( 99 ) ); } )
					&& Base::Serialize( base ).ToString() == before, "unknown allocation phase changed Base" );
			}
			GT_OK();
		} );
	} );
	task->AddTest( "SMAC live worker recalculation rejects before atomic publication", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false ); game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue() );
			for ( int scenario = 0; scenario < 6; ++scenario ) {
				auto roles = Roles(); auto context = base->GetSmacResourceContext(); auto* candidate = game->GetMap()->GetTile( 4, 2 );
				if ( scenario == 0 ) { context.governor_options |= SmacBaseResourceContext::GOVERNOR_ACTIVE; base->SetSmacResourceContext( GSE_CALL, context ); }
				if ( scenario == 1 ) { base->SetProductionState( GSE_CALL, std::nullopt ); }
				if ( scenario == 2 ) { base->SetProductionState( GSE_CALL, Production( ProductionItemKind::Project ) ); }
				if ( scenario == 3 ) { base->SetProductionState( GSE_CALL, Production( ProductionItemKind::Orbital ) ); }
				if ( scenario == 4 ) { candidate->terraforming |= TERRAFORMING_MIRROR; }
				if ( scenario == 5 ) { roles.specialists[ 6 ] = roles.specialists[ 0 ]; } // late publication preflight
				const auto before = fixture.Snapshot(); const auto requests = fixture.QueuedFrontendRequests().size();
				GT_ASSERT( Rejects( [ & ] { base->RecalculateSmacWorkers( GSE_CALL, roles ); } ), "unsupported live allocation context was accepted" );
				GT_ASSERT( fixture.Snapshot() == before && fixture.QueuedFrontendRequests().size() == requests
					&& game->GetBM()->GetWorkingPop( { 3, 1 } ) == &base->m_pops.at( 19 ) && !game->GetBM()->GetWorkingPop( { 4, 2 } ), "rejection published partial worker/world state" );
				context.governor_options = 0x40; base->SetSmacResourceContext( GSE_CALL, context ); base->SetProductionState( GSE_CALL, Production() ); candidate->terraforming = TERRAFORMING_FOREST;
			}
			GT_OK();
		} );
	} );
	task->AddTest( "SMAC live worker recalculation resumes actual saved world authority", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture source; source.ConfigureResources( Source, "GAIANS" );
		std::string saved, knowledge, result; auto pair = source.GameplayRandom();
		const auto error = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = source.Game(); auto* base = Populate( GSE_CALL, source, false ); game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue() );
			saved = source.Snapshot(); knowledge = game->GetMap()->GetSmacWorkerKnowledge()->Serialize(); pair = source.GameplayRandom();
			base->RecalculateSmacWorkers( GSE_CALL, Roles() ); result = Base::Serialize( base ).ToString(); GT_OK();
		} ); GT_ASSERT( error.empty(), "source continuation failed: " + error );
		// A separate actual decoder destination: replacing a live known world
		// requires its normal owner retirement, not the fixture's UM-only clear.
		ConventionalUnitFixture destination; destination.ConfigureResources( Source, "GAIANS" );
		return destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			destination.Restore( GSE_CALL, saved, true ); auto* game = destination.Game(); auto* base = game->GetBM()->GetBase( 701 );
			GT_ASSERT( base->GetSmacAllocation()->selected == 3 && game->GetMap()->GetSmacWorkerKnowledge()->Serialize() == knowledge, "restore replayed allocation or knowledge producers" );
			RecalculateViaWrapper( GSE_CALL, base );
			GT_ASSERT( Base::Serialize( base ).ToString() == result && destination.GameplayRandom() == pair, "restored next live allocation differs" ); GT_OK();
		} );
	} );

	task->AddTest( "SMAC manual worker events publish Pop index and presentation atomically", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false );
			auto context = base->GetSmacResourceContext(); context.governor_options = 0x1000040; base->SetSmacResourceContext( GSE_CALL, context );
			game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true ) );
			auto* pop = &base->m_pops.at( 19 ); auto* wrapper = pop->Wrap( GSE_CALL );
			const auto padding = base->GetSmacAllocation()->specialists.types; const auto pair = fixture.GameplayRandom();
			const auto rng = game->GetRandom()->GetStateString(); const auto knowledge = game->GetMap()->GetSmacWorkerKnowledge()->Serialize();
			auto* initialize = static_cast< value::Callable* >( fixture.Script( GSE_CALL, "./GLSMAC_data/tests/_canonical_worker_click" ) );
			auto* click = static_cast< value::Callable* >( initialize->Run( GSE_CALL, { game->Wrap( GSE_CALL ), base->Wrap( GSE_CALL ) } ) );
			click->Run( GSE_CALL, { VALUE( value::Int,, 250 ), VALUE( value::Int,, 75 ) } ); fixture.DrainEvents();
			GT_ASSERT( base->GetSmacAllocation()->selected == 1 && base->GetSmacAllocation()->specialists.total_count == 1
				&& pop->m_def->m_smac_specialist_id == 1 && !game->GetBM()->GetWorkingPop( { 3, 1 } ), "event did not create and retain the actual automatic Doctor" );
			GT_ASSERT( base->GetSmacResourceContext().governor_options == 0x1000000, "manual command cleared more than citizen management" );
			click->Run( GSE_CALL, { VALUE( value::Int,, 300 ), VALUE( value::Int,, 100 ) } ); fixture.DrainEvents();
			GT_ASSERT( base->GetSmacAllocation()->selected == 5 && base->GetSmacAllocation()->specialists.total_count == 0
				&& game->GetBM()->GetWorkingPop( { 4, 2 } ) == pop && &base->m_pops.at( 19 ) == pop && Pop::Unwrap( wrapper ) == pop
				&& base->GetSmacAllocation()->specialists.types == padding, "work event lost selection, stable Pop or unused specialist padding" );
			GT_ASSERT( fixture.GameplayRandom() == pair && game->GetRandom()->GetStateString() == rng
				&& game->GetMap()->GetSmacWorkerKnowledge()->Serialize() == knowledge, "manual event changed RNG or knowledge" );
			bool refresh = false; for ( const auto& update : fixture.Updates() ) { if ( update.type == game::FrontendRequest::FR_BASE_UPDATE && update.data.base_update.base_id == 701 ) { refresh = true; } }
			GT_ASSERT( refresh, "manual command did not publish Base presentation" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC manual worker click replacement center and unavailable semantics", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false );
			game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true ) );
			const auto click = [ & ]( size_t x, size_t y ) {
				game->Event( GSE_CALL, "work_base_tile", { { "base", base->Wrap( GSE_CALL ) }, { "tile", game->GetMap()->GetTile( x, y )->Wrap( GSE_CALL ) } } ); fixture.DrainEvents();
			};
			click( 4, 2 ); GT_ASSERT( base->GetSmacAllocation()->selected == 5, "manual replacement did not replace an existing complete worker allocation" );
			click( 3, 1 ); GT_ASSERT( base->GetSmacAllocation()->selected == 3, "manual replacement incorrectly optimized back to the better forest" );
			click( 2, 2 ); GT_ASSERT( base->GetSmacAllocation()->selected == 5, "center click did not clear selection and perform new allocation" );
			auto context = base->GetSmacResourceContext(); context.governor_options |= SmacBaseResourceContext::MANAGE_CITIZENS; base->SetSmacResourceContext( GSE_CALL, context );
			const auto allocation = base->GetSmacAllocation(); click( 4, 0 );
			GT_ASSERT( base->GetSmacAllocation() == allocation && !base->GetSmacResourceContext().ManageCitizens(), "unavailable click changed allocation or failed to clear citizen management" );
			game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue() );
			auto stale = *base->GetSmacAllocation(); stale.selected = 3; base->PublishSmacAllocation( GSE_CALL, stale, Roles() );
			click( 4, 2 );
			GT_ASSERT( base->GetSmacAllocation()->selected == 1 && base->GetSmacAllocation()->specialists.total_count == 1
				&& base->m_pops.at( 19 ).m_def->m_smac_specialist_id == 1, "click did not inspect selection after preliminary repair" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC manual worker events consume actual homed support and frozen priorities", []( GSE*, context::Context* ) -> std::string {
		for ( const bool homed : { false, true } ) {
			ConventionalUnitFixture fixture; fixture.ConfigureResources( []( const std::string& filename ) {
				auto source = Source( filename );
				if ( filename != "alpha.txt" ) {
					const std::string original = "Leader,F,-1,0,0,1,1,";
					source.replace( source.find( original ), original.size(), "Leader,F,500,0,0,0,0," );
				}
				return source;
			}, "GAIANS" );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false, homed );
				auto allocation = *base->GetSmacAllocation(); allocation.selected = 5;
				base->PublishSmacAllocation( GSE_CALL, allocation, Roles() );
				fixture.ResourceState( []( auto& state ) { state.social.factions.front().retained->at( 2 ) = -4; } );
				auto* unit = game->GetUM()->GetUnit( 91 );
				auto context = base->GetSmacResourceContext(); context.drone_riots = true; base->SetSmacResourceContext( GSE_CALL, context );
				game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true ) );
				GT_ASSERT( game->GetCanonicalMineralSupport( *base ) == ( homed ? 2 : 0 ), "actual home did not produce measured support" );
				const auto record = unit->Conventional(); const auto pair = fixture.GameplayRandom(); const auto rng = game->GetRandom()->GetStateString();
				const auto knowledge = game->GetMap()->GetSmacWorkerKnowledge()->Serialize();
				RecalculateViaWrapper( GSE_CALL, base );
				game->Event( GSE_CALL, "work_base_tile", { { "base", base->Wrap( GSE_CALL ) }, { "tile", game->GetMap()->GetTile( 3, 1 )->Wrap( GSE_CALL ) } } );
				fixture.DrainEvents();
				// Forest M=2, total M=3, F=500: mineral score is 336 without
				// support, 1008 with support2. Only the former is below999.
				GT_ASSERT( base->GetSmacAllocation()->selected == ( homed ? 5u : 3u ), "manual event ignored home support or current frozen aggression" );
				GT_ASSERT( base->GetSmacResourceContext().drone_riots && !base->GetSmacResourceContext().ManageCitizens()
					&& unit->Conventional() == record && fixture.GameplayRandom() == pair && game->GetRandom()->GetStateString() == rng
					&& game->GetMap()->GetSmacWorkerKnowledge()->Serialize() == knowledge, "worker publication changed home, riots, RNG or knowledge" ); GT_OK();
			} );
			if ( !error.empty() ) { return error; }
		}
		GT_OK();
	} );

	task->AddTest( "SMAC manual worker event failures preserve authority and broad fences", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, true ); game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true ) );
			const auto before = fixture.Snapshot();
			GT_ASSERT( Rejects( [ & ] { game->Event( GSE_CALL, "unrelated", {} ); } ), "worker command opened arbitrary event dispatch" );
			GT_ASSERT( Rejects( [ & ] { game->Event( GSE_CALL, "work_base_tile", { { "base", base->Wrap( GSE_CALL ) }, { "tile", game->GetMap()->GetTile( 4, 2 )->Wrap( GSE_CALL ) }, { "pop", base->m_pops.at( 19 ).Wrap( GSE_CALL ) } } ); } ), "canonical click accepted legacy Pop override" );
			GT_ASSERT( fixture.Snapshot() == before, "early command rejection changed state" );
			auto* target = game->GetMap()->GetTile( 4, 2 ); target->terraforming |= TERRAFORMING_MIRROR;
			const auto rejected = fixture.Snapshot(); const auto requests = fixture.QueuedFrontendRequests().size();
			game->Event( GSE_CALL, "work_base_tile", { { "base", base->Wrap( GSE_CALL ) }, { "tile", target->Wrap( GSE_CALL ) } } );
			GT_ASSERT( Rejects( [ & ] { fixture.DrainEvents(); } ), "unsupported final tile query did not reject" );
			GT_ASSERT( fixture.Snapshot() == rejected && fixture.QueuedFrontendRequests().size() == requests && base->GetSmacResourceContext().ManageCitizens()
				&& !game->GetBM()->GetWorkingPop( { 4, 2 } ), "late command failure published allocation, context or reverse index" ); GT_OK();
		} );
	} );
}
}

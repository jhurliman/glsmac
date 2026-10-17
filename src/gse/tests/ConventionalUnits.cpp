#include "ConventionalUnits.h"
#include "fixtures/FriendlyHQMovement.h"
#include "fixtures/SpartanHQMovement.h"
#include "fixtures/GaianHQMovement.h"
#include "fixtures/RemoteHQMovement.h"
#include "fixtures/RemoteOrdinaryGround.h"
#include "fixtures/OwnedBaseTerrainMovement.h"
#include "fixtures/NearestAnyBase.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/WorldSnapshotRules.h"
#include "game/backend/WorldSnapshotWorkerKnowledge.h"
#include "game/backend/WorldSnapshotRandom.h"
#include "game/backend/WorldSnapshotTurn.h"
#include "game/backend/Player.h"
#include "game/backend/Random.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tile.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/FacilityDef.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/StaticDef.h"
#include "game/backend/unit/MoraleSet.h"
#include "game/backend/unit/CvrScoutRender.h"
#include "gse/value/Object.h"
#include "gse/value/Callable.h"
#include "gse/value/Int.h"
#include "gse/value/Bool.h"
#include "gse/value/String.h"
#include "gse/value/Float.h"
#include "gse/value/Undefined.h"
#include "gse/callable/Native.h"
#include "util/FinallyGuard.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace gse::tests {
namespace {
using namespace game::backend;
using namespace game::backend::unit;

ConventionalUnitState Record() { return { 2, 1, 1, 0, 0, false, ConventionalUnitState::Home::None, SmacMoraleMode::Mode0 }; }

Def* Define( UnitManager* manager, bool rover = false ) {
	MoraleSet::morale_values_t names;
	for ( int i = 0; i < 7; ++i ) { names.emplace_back( "level" + std::to_string( i ) ); }
	if ( !manager->GetMoraleSet( "CONVENTIONAL_MORALE" ) ) { manager->DefineMoraleSet( new MoraleSet( "CONVENTIONAL_MORALE", names ) ); }
	auto* def = new StaticDef( rover ? "EXPLICIT_ROVER" : "EXPLICIT_SCOUT", manager->GetMoraleSet( "CONVENTIONAL_MORALE" ), "Fixture Scout", MT_LAND, 999.0f,
		new CvrScoutRender( 2, 0, 100, 100, { 1.0f, 1.0f }, { 128.0f, 128.0f }, rover ? CvrScoutRender::Composition::SpartanRover : CvrScoutRender::Composition::Scout ) );
	manager->DefineUnit( def ); return def;
}

Value* Call( GSE_CALLABLE, value::Object* object, const std::string& method, const value::function_arguments_t& arguments = {} ) {
	return static_cast< value::Callable* >( object->Get( method ) )->Run( GSE_CALL, arguments );
}

Unit* Spawn( GSE_CALLABLE, ConventionalUnitFixture& fixture, const ConventionalUnitState& state, size_t id = 71, size_t slot = 0, const char* def = "EXPLICIT_SCOUT" ) {
	auto* game = fixture.Game();
	auto* data = VALUEEXT( value::Object, GSE_CALL, ( value::object_properties_t{
		{ "id", VALUE( value::Int,, id ) }, { "def", VALUE( value::String,, def ) },
		{ "owner", game->GetState()->m_slots->GetSlot( slot ).GetPlayer()->Wrap( GSE_CALL ) },
		{ "tile", game->GetMap()->GetTile( 2, 2 )->Wrap( GSE_CALL ) },
		{ "design_id", VALUE( value::Int,, state.design_id ) }, { "owner_simulation_id", VALUE( value::Int,, state.owner_simulation_id ) },
		{ "stored_morale", VALUE( value::Int,, state.stored_morale ) }, { "damage", VALUE( value::Int,, state.damage ) },
		{ "spent", VALUE( value::Int,, state.spent ) }, { "failed_move", VALUE( value::Bool,, state.failed_move ) },
		{ "home", VALUE( value::String,, state.home_base_id ? "base" : "none" ) },
		{ "home_base_id", VALUE( value::Int,, state.home_base_id ? static_cast< int64_t >( *state.home_base_id ) : 0 ) }, { "render_morale_mode", VALUE( value::Int,, static_cast< int >( state.render_morale_mode ) ) }
	} ) );
	const auto* result = static_cast< value::Object* >( Call( GSE_CALL, static_cast< value::Object* >( game->GetUM()->Wrap( GSE_CALL ) ), "spawn_conventional_unit", { data } ) );
	return static_cast< Unit* >( result->wrapobj );
}

void Land( Game* game ) {
	using namespace map::tile;
	for ( size_t y = 0; y < game->GetMap()->GetHeight(); ++y ) {
		for ( size_t x = y & 1; x < game->GetMap()->GetWidth(); x += 2 ) {
			auto* tile = game->GetMap()->GetTile( x, y );
			for ( auto* corner : tile->elevation.corners ) { *corner = 100; }
			tile->rockiness = ROCKINESS_FLAT; tile->features = FEATURE_NONE; tile->terraforming = TERRAFORMING_NONE;
			tile->Update();
		}
	}
}

base::Base* HQ( GSE_CALLABLE, Game* game, Unit* unit, map::tile::Tile* tile, size_t id = 31, size_t population = 1, bool has_hq = true ) {
	auto* manager = game->GetBM();
	if ( !manager->GetFacilityDef( "HEADQUARTERS" ) ) {
		manager->DefineFacility( new base::FacilityDef( "HEADQUARTERS", "Fixture HQ", "Fixture", 1, { "fixture", 0, 0, 1, 1 } ) );
	}
	if ( !manager->GetPopDef( "HQ_WORKER" ) ) { manager->DefinePop( new base::PopDef( "HQ_WORKER", "Worker", {}, {}, base::PopDef::PF_NONE ) ); }
	base::Base::pops_t pops;
	for ( size_t i = 1; i <= population; ++i ) { pops.emplace( i, base::Pop( nullptr, i, manager->GetPopDef( "HQ_WORKER" ) ) ); }
	base::Base::facilities_t facilities;
	if ( has_hq ) { facilities.emplace( "HEADQUARTERS", manager->GetFacilityDef( "HEADQUARTERS" ) ); }
	auto* result = new base::Base( game, id, unit->m_owner, unit->m_owner->GetPlayer()->GetFaction(), tile, "HQ", facilities, pops, population + 1 );
	manager->SpawnBase( GSE_CALL, result ); return result;
}

void OriginalFeatures( map::tile::Tile* tile, int flags ) {
	using namespace map::tile;
	if ( flags != 0 && flags != 4 && flags != 8 && flags != 32 ) { throw std::runtime_error( "unsupported original HQ fixture feature" ); }
	tile->features = flags == 32 ? FEATURE_XENOFUNGUS : FEATURE_NONE;
	tile->terraforming = flags == 4 ? TERRAFORMING_ROAD : flags == 8 ? TERRAFORMING_MAG_TUBE : TERRAFORMING_NONE;
}

// Build real canonical participants first, then owned units/bases. Source IDs
// are taken from detached runtime queries, never inferred from transport slots.
struct RemoteHQWorld { Unit* local; base::Base* hq; std::vector< Unit* > remote; };
RemoteHQWorld PopulateRemoteHQ( GSE_CALLABLE, ConventionalUnitFixture& fixture, const remote_hq_fixture::Case& c, bool rover = true ) {
	auto* game = fixture.Game(); auto* manager = game->GetUM(); Land( game ); Define( manager ); if ( rover ) { Define( manager, true ); }
	const auto read = [ & ]( size_t slot ) {
		auto* view = static_cast< value::Object* >( Call( GSE_CALL, static_cast< value::Object* >( game->Wrap( GSE_CALL ) ), "get_startup_designs",
			{ game->GetState()->m_slots->GetSlot( slot ).GetPlayer()->Wrap( GSE_CALL ) } ) );
		auto record = Record(); record.owner_simulation_id = static_cast< int32_t >( static_cast< value::Int* >( view->Get( "simulation_id" ) )->value );
		if ( slot == 0 && rover ) { record.design_id = static_cast< int32_t >( static_cast< value::Int* >( view->Get( "starting_scout" ) )->value ); }
		return record;
	};
	auto initial = read( 0 ); initial.spent = static_cast< uint8_t >( c.step.spent );
	auto* local = Spawn( GSE_CALL, fixture, initial, c.reordered ? 77 : 71, 0, rover ? "EXPLICIT_ROVER" : "EXPLICIT_SCOUT" );
	auto* source = game->GetMap()->GetTile( 16, 16 ); auto* destination = game->GetMap()->GetTile( 17, 17 ); local->SetTile( GSE_CALL, source );
	auto* hq = HQ( GSE_CALL, game, local, c.step.endpoint == 1 ? source : destination, c.reordered ? 37 : 31 );
	RemoteHQWorld result{ local, hq, {} };
	for ( size_t i = 1; i <= 6; ++i ) {
		const auto slot = c.same_owner && i == 1 ? 0 : i;
		const auto index = c.reordered && i == 6 ? 0 : i;
		auto record = read( slot ); record.design_id = 2;
		auto* remote = Spawn( GSE_CALL, fixture, record, 71 + index, slot );
		auto* tile = game->GetMap()->GetTile( 40 + 12 * i, 40 ); remote->SetTile( GSE_CALL, tile );
		auto* remote_base = HQ( GSE_CALL, game, remote, c.tie && i == 1 ? game->GetMap()->GetTile( 18, 18 ) : tile, 31 + index );
		if ( c.same_owner && i == 1 ) { remote_base->m_facilities.clear(); }
		result.remote.push_back( remote );
	}
	OriginalFeatures( source, c.step.source_flags ); OriginalFeatures( destination, c.step.destination_flags );
	fixture.GameplayRandom( { c.step.seed, 0xa1b2c3d4 } ); fixture.Updates();
	return result;
}

std::map< size_t, std::string > BaseBytes( Game* game ) {
	std::map< size_t, std::string > result;
	for ( const auto& entry : game->GetBM()->GetBases() ) { result.emplace( entry.first, base::Base::Serialize( entry.second ).ToString() ); }
	return result;
}

RemoteHQWorld PopulateRemoteGround( GSE_CALLABLE, ConventionalUnitFixture& fixture, const remote_ground_fixture::Case& c ) {
	auto* game = fixture.Game(); auto* manager = game->GetUM(); Land( game ); Define( manager ); if ( c.rover ) { Define( manager, true ); }
	const auto read = [ & ]( size_t slot ) {
		auto* view = static_cast< value::Object* >( Call( GSE_CALL, static_cast< value::Object* >( game->Wrap( GSE_CALL ) ), "get_startup_designs",
			{ game->GetState()->m_slots->GetSlot( slot ).GetPlayer()->Wrap( GSE_CALL ) } ) );
		auto record = Record(); record.owner_simulation_id = static_cast< int32_t >( static_cast< value::Int* >( view->Get( "simulation_id" ) )->value );
		if ( slot == 0 && c.rover ) { record.design_id = static_cast< int32_t >( static_cast< value::Int* >( view->Get( "starting_scout" ) )->value ); }
		return record;
	};
	auto record = read( 0 ); record.spent = static_cast< uint8_t >( c.spent );
	auto* local = Spawn( GSE_CALL, fixture, record, 71, 0, c.rover ? "EXPLICIT_ROVER" : "EXPLICIT_SCOUT" );
	auto* source = game->GetMap()->GetTile( 16, 16 ); auto* destination = game->GetMap()->GetTile( 17, 17 ); local->SetTile( GSE_CALL, source );
	RemoteHQWorld result{ local, nullptr, {} };
	if ( c.remote ) {
		result.hq = HQ( GSE_CALL, game, local, game->GetMap()->GetTile( 40, 16 ) );
		for ( size_t slot = 1; slot <= 6; ++slot ) {
			auto remote_record = read( slot ); remote_record.design_id = 2;
			auto* remote = Spawn( GSE_CALL, fixture, remote_record, 71 + slot, slot );
			auto* tile = game->GetMap()->GetTile( 40 + 12 * slot, 40 ); remote->SetTile( GSE_CALL, tile );
			HQ( GSE_CALL, game, remote, tile, 31 + slot ); result.remote.push_back( remote );
		}
	}
	OriginalFeatures( source, c.source_flags ); OriginalFeatures( destination, c.destination_flags );
	fixture.GameplayRandom( { c.seed, c.crt_after } ); fixture.Updates(); return result;
}

// UnitManager serializes its two named definition tables in unordered-map
// iteration order. Compare every byte of the actual world after sorting only
// these name/value pairs; definitions, Unit records and every other frame remain
// exact. This neither changes the production codec nor asserts raw wire order.
std::string CanonicalMovementSnapshot( Game* game, const std::string& bytes ) {
	types::Buffer world( bytes ); const auto* state = game->GetState();
	world_snapshot::ValidateHeader( world, state->m_settings.global );
	(void)world_snapshot::ReadRandomState( world );
	(void)world_snapshot::ReadTurnState( world, state->m_slots->GetSlots() );
	if ( world.ReadBool() ) { (void)world.ReadString(); } // exact simulation payload retained in prefix
	(void)map::GlsmacResourceMapPolicy::ReadOptional( world, state->m_settings.global.GetRulesetId() == "smac" );
	(void)world_snapshot::ReadWorkerKnowledge( world, state->m_settings.global.GetRulesetId() == "smac" );
	(void)world.ReadString(); (void)world.ReadString(); // map and resources, likewise unchanged
	const auto prefix = bytes.substr( 0, world.lenr ); types::Buffer units( world.ReadString() ), canonical;
	canonical.WriteString( units.ReadString() );
	for ( int table = 0; table < 2; ++table ) {
		const auto count = units.ReadInt(); if ( count < 0 || count > 1024 ) { throw std::runtime_error( "invalid test definition count" ); }
		std::map< std::string, std::string > entries;
		for ( int64_t i = 0; i < count; ++i ) {
			const auto name = units.ReadString(), value = units.ReadString();
			if ( !entries.emplace( name, value ).second ) { throw std::runtime_error( "duplicate test definition name" ); }
		}
		canonical.WriteInt( count );
		for ( const auto& entry : entries ) { canonical.WriteString( entry.first ); canonical.WriteString( entry.second ); }
	}
	types::Buffer result; result.WriteString( prefix ); result.WriteString( canonical.ToString() );
	result.WriteString( units.ToString().substr( units.lenr ) ); result.WriteString( bytes.substr( world.lenr ) );
	return result.ToString();
}

// Rewrite one existing unit's trailing home ID while preserving every other
// actual snapshot frame, including checksum framing and the saved Base body.
std::string WithLastHome( const std::string& bytes, Game* game, int64_t home ) {
	types::Buffer world( bytes ); world_snapshot::ValidateHeader( world, game->GetState()->m_settings.global );
	(void)world_snapshot::ReadRandomState( world ); (void)world_snapshot::ReadTurnState( world, game->GetState()->m_slots->GetSlots() );
	if ( world.ReadBool() ) { (void)world.ReadString(); } (void)map::GlsmacResourceMapPolicy::ReadOptional( world, true );
	(void)world_snapshot::ReadWorkerKnowledge( world, true );
	(void)world.ReadString(); (void)world.ReadString();
	const auto prefix = bytes.substr( 0, world.lenr ); types::Buffer units( world.ReadString() ), replacement;
	replacement.WriteString( units.ReadString() );
	for ( int table = 0; table < 2; ++table ) {
		const auto count = units.ReadInt(); replacement.WriteInt( count );
		for ( int64_t i = 0; i < count; ++i ) { replacement.WriteString( units.ReadString() ); replacement.WriteString( units.ReadString() ); }
	}
	const auto count = units.ReadInt(); replacement.WriteInt( count );
	for ( int64_t i = 0; i < count; ++i ) {
		auto wire = units.ReadString();
		if ( i + 1 == count ) { types::Buffer changed( wire.substr( 0, wire.size() - 14 ) ); changed.WriteInt( home ); wire = changed.ToString(); }
		replacement.WriteString( wire );
	}
	replacement.WriteInt( units.ReadInt() ); types::Buffer framed; framed.WriteString( replacement.ToString() );
	return prefix + framed.ToString() + bytes.substr( world.lenr );
}

uint32_t SharedNext( uint32_t value ) { return static_cast< uint32_t >( UINT64_C( 1664525 ) * value + UINT64_C( 1013904223 ) ); }
uint32_t SeedForBin( uint32_t bin ) {
	for ( uint32_t seed = 0; seed < 65536; ++seed ) { if ( uint64_t( SharedNext( seed ) & 65535 ) * 9 / 65536 == bin ) { return seed; } }
	throw std::runtime_error( "missing fixture lottery bin" );
}

int64_t Int( Value* value, const std::string& key ) { return static_cast< value::Int* >( static_cast< value::Object* >( value )->Get( key ) )->value; }

struct WireRecord {
	int64_t version = 3, kind = 1, id = 71, owner = 0, x = 2, y = 2;
	std::string definition = "EXPLICIT_SCOUT";
	int64_t design = 2, simulation = 1, morale = 255, damage = 4, spent = 255, home = 0, mode = 1;
	bool failed = true;
	std::string Encode() const {
		types::Buffer bytes;
		bytes.WriteInt( version ); bytes.WriteInt( kind ); bytes.WriteInt( id ); bytes.WriteString( definition );
		bytes.WriteInt( owner ); bytes.WriteInt( x ); bytes.WriteInt( y );
		for ( auto field : { design, simulation, morale, damage, spent } ) { bytes.WriteInt( field ); }
		bytes.WriteBool( failed ); bytes.WriteInt( home ); bytes.WriteInt( mode ); return bytes.ToString();
	}
};
}

void AddConventionalUnitTests( task::gsetests::GSETests* task ) {
	task->AddTest( "Conventional home codec preserves explicit IDs and rejects malformed shapes", GT() {
		for ( uint64_t id : { uint64_t( 1 ), uint64_t( 701 ), uint64_t( INT64_MAX - 1 ) } ) {
			auto record = Record(); record.home = ConventionalUnitState::Home::Base; record.home_base_id = id;
			types::Buffer wire; record.Serialize( wire ); auto copy = ConventionalUnitState::Deserialize( wire );
			GT_ASSERT( copy == record && wire.lenr == wire.lenw, "home ID lost or normalized" );
		}
		for ( int scenario = 0; scenario < 5; ++scenario ) {
			auto record = Record(); if ( scenario != 0 ) { record.home = ConventionalUnitState::Home::Base; }
			if ( scenario == 0 || scenario == 2 ) { record.home_base_id = 0; }
			if ( scenario == 3 ) { record.home_base_id = uint64_t( INT64_MAX ); }
			if ( scenario == 4 ) { record.home = static_cast< ConventionalUnitState::Home >( 2 ); }
			bool rejected = false; try { types::Buffer wire; record.Serialize( wire ); } catch ( const std::runtime_error& ) { rejected = true; }
			GT_ASSERT( rejected, "malformed home shape accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "Conventional home live creation checks ownership and uses Base morale status", GT() {
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* um = game->GetUM(); auto* bm = game->GetBM(); Define( um ); Land( game );
			auto* owner = &game->GetState()->m_slots->GetSlot( 0 );
			auto* home = new base::Base( game, 701, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( 0, 0 ), "Home", {}, {}, 1, 0, {}, base::SmacBaseResourceContext{ 1, false, 0, 0, 0, false } );
			bm->SpawnBase( GSE_CALL, home );
			auto record = Record(); record.home = ConventionalUnitState::Home::Base; record.home_base_id = 999;
			const auto next = Unit::GetNextId(), queued = fixture.QueuedFrontendRequests().size();
			bool rejected = false; try { Spawn( GSE_CALL, fixture, record ); } catch ( const std::exception& ) { rejected = true; }
			GT_ASSERT( rejected && Unit::GetNextId() == next && game->GetMap()->GetTile( 2, 2 )->units.empty() && fixture.QueuedFrontendRequests().size() == queued, "missing home changed constructor effects" );
			auto* foreign_owner = &game->GetState()->m_slots->GetSlot( 6 );
			auto* foreign = new base::Base( game, 702, foreign_owner, foreign_owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( 2, 0 ), "Foreign", {}, {}, 1, 0, {}, base::SmacBaseResourceContext{ 2, false, 0, 0, 0, false } ); bm->SpawnBase( GSE_CALL, foreign );
			auto* legacy = new base::Base( game, 703, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( 4, 0 ), "Legacy", {}, {} ); bm->SpawnBase( GSE_CALL, legacy );
			for ( const auto id : { 702u, 703u } ) {
				record.home_base_id = id; bool failed = false; const auto count = fixture.QueuedFrontendRequests().size();
				try { Spawn( GSE_CALL, fixture, record ); } catch ( const std::exception& ) { failed = true; }
				GT_ASSERT( failed && Unit::GetNextId() == next && game->GetMap()->GetTile( 2, 2 )->units.empty() && count == fixture.QueuedFrontendRequests().size(), "foreign or unavailable home admitted or mutated effects" );
			}
			record.home_base_id = 701; record.stored_morale = 4; record.render_morale_mode = SmacMoraleMode::Mode1;
			auto* unit = Spawn( GSE_CALL, fixture, record );
			fixture.RetainedMorale( -4, 4 );
			GT_ASSERT( unit->QueryStats( SmacMoraleMode::Mode0 ).effective_morale == 1, "homed ordinary morale ignored retained rating" );
			bm->DefineFacility( new base::FacilityDef( "NOT_CRECHE_TEXT", "Misleading", "", 0, {}, 2 ) );
			home->m_facilities.emplace( "NOT_CRECHE_TEXT", bm->GetFacilityDef( "NOT_CRECHE_TEXT" ) );
			GT_ASSERT( unit->QueryStats( SmacMoraleMode::Mode0 ).effective_morale == 3, "actual Creche role omitted" );
			Spawn( GSE_CALL, fixture, Record(), 72 ); fixture.Updates();
			auto context = home->GetSmacResourceContext(); context.governor_options ^= 1; home->SetSmacResourceContext( GSE_CALL, context );
			const auto& unrelated = fixture.Updates();
			GT_ASSERT( std::none_of( unrelated.begin(), unrelated.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_UPDATE; } ), "governor-only change refreshed Unit stats" );
			context.drone_riots = true; home->SetSmacResourceContext( GSE_CALL, context );
			const auto& refreshed = fixture.Updates();
			const auto update = std::find_if( refreshed.begin(), refreshed.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_UPDATE; } );
			GT_ASSERT( update != refreshed.end() && update->data.unit_update.unit_id == unit->m_id && update->data.unit_update.morale == 4
				&& std::count_if( refreshed.begin(), refreshed.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_UPDATE; } ) == 1,
				"riots status did not refresh only its homed Unit with fresh stats" );
			GT_ASSERT( unit->QueryStats( SmacMoraleMode::Mode1 ).effective_morale == 4 && unit->QueryStats( SmacMoraleMode::Mode0 ).effective_morale == 3, "saved riots lost mode distinction" );
			auto* detached = static_cast< value::Object* >( Call( GSE_CALL, static_cast< value::Object* >( unit->Wrap( GSE_CALL ) ), "get_conventional_state" ) );
			GT_ASSERT( static_cast< value::Int* >( detached->Get( "home_base_id" ) )->value == 701, "wrapper lost stable home ID" );
			rejected = false; try { bm->DespawnBase( GSE_CALL, 701 ); } catch ( const std::exception& ) { rejected = true; }
			GT_ASSERT( rejected && bm->GetBase( 701 ) == home, "Base removal orphaned a saved home" );
			GT_OK();
		} );
	} );
	task->AddTest( "Conventional support query consumes actual homes retained rating and current population", GT() {
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* um = game->GetUM(); auto* bm = game->GetBM(); Define( um ); Land( game );
			const auto make = [ & ]( size_t id, size_t slot, int32_t sim, size_t x ) {
				auto* owner = &game->GetState()->m_slots->GetSlot( slot );
				auto* b = new base::Base( game, id, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( x, 0 ), "Support" + std::to_string( id ), {}, {}, 1, 0, {}, base::SmacBaseResourceContext{ sim, false, 0, 0, 0, false } );
				bm->SpawnBase( GSE_CALL, b ); return b;
			};
			auto* home = make( 701, 0, 1, 0 ); auto* other = make( 702, 0, 1, 2 ); auto* foreign = make( 703, 6, 2, 4 );
			for ( size_t i = 0; i < 6; ++i ) {
				auto record = Record(); if ( i != 3 ) { record.home = ConventionalUnitState::Home::Base; record.home_base_id = i == 4 ? 702 : i == 5 ? 703 : 701; }
				if ( i == 5 ) { record.owner_simulation_id = 2; }
				Spawn( GSE_CALL, fixture, record, 90 - i, i == 5 ? 6 : 0 );
			}
			const auto pair = fixture.GameplayRandom(); const auto bytes = base::Base::Serialize( home ).ToString();
			const std::array< int32_t, 8 > expected{ 6, 3, 2, 2, 1, 0, 0, 0 };
			for ( int rating = -4; rating <= 3; ++rating ) {
				fixture.ResourceState( [ & ]( auto& state ) { state.social.factions.front().retained->at( 2 ) = rating; state.social.factions.front().current[ 2 ] = 3; } );
				GT_ASSERT( game->GetCanonicalMineralSupport( *home ) == expected[ rating + 4 ], "actual aggregate ignored retained rating/home filtering" );
				auto* support = Call( GSE_CALL, static_cast< value::Object* >( home->Wrap( GSE_CALL ) ), "get_smac_mineral_support" );
				GT_ASSERT( support->type == VT_INT && static_cast< value::Int* >( support )->value == expected[ rating + 4 ], "Base wrapper omitted actual canonical support" );
			}
			GT_ASSERT( game->GetCanonicalMineralSupport( *other ) == 0 && game->GetCanonicalMineralSupport( *foreign ) == 0
				&& fixture.GameplayRandom() == pair && base::Base::Serialize( home ).ToString() == bytes, "support consumed RNG or changed Base authority" );
			GT_OK();
		} );
	} );
	task->AddTest( "Conventional home restore rejects missing and foreign references before any readiness seal", GT() {
		ConventionalUnitFixture source; source.Configure(); std::string saved;
		const auto prepared = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = source.Game(); Define( game->GetUM() ); Land( game );
			for ( int i = 0; i < 2; ++i ) {
				auto* owner = &game->GetState()->m_slots->GetSlot( i ? 6 : 0 );
				auto* base = new base::Base( game, 701 + i, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( 2 * i, 0 ), "Reference" + std::to_string( i ), {}, {}, 1, 0, {}, base::SmacBaseResourceContext{ 1 + i, false, 0, 0, 0, false } );
				game->GetBM()->SpawnBase( GSE_CALL, base );
			}
			auto record = Record(); record.home = ConventionalUnitState::Home::Base; record.home_base_id = 701;
			Spawn( GSE_CALL, source, record, 71 ); Spawn( GSE_CALL, source, record, 72 ); saved = source.Snapshot(); GT_OK();
		} ); if ( !prepared.empty() ) { return prepared; }
		for ( const auto home : { 702, 999 } ) {
			ConventionalUnitFixture destination; destination.Configure();
			const auto error = destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				std::string diagnostic; bool rejected = false; try { destination.Restore( GSE_CALL, WithLastHome( saved, source.Game(), home ), true ); } catch ( const std::exception& error ) { rejected = true; diagnostic = error.what(); }
				GT_ASSERT( rejected && destination.Game()->GetBM()->GetBase( 701 ) && destination.Game()->GetBM()->GetBase( 702 ), "home rejection did not reach complete Base-reference validation: " + diagnostic );
				for ( size_t id : { 71u, 72u } ) {
					auto* unit = destination.Game()->GetUM()->GetUnit( id ); GT_ASSERT( unit, "restored Unit absent before home validation" );
					bool blocked = false; try { unit->RequireConventionalReady(); } catch ( const std::runtime_error& ) { blocked = true; }
					GT_ASSERT( blocked, "failed all-home preflight partly released Unit readiness" );
				}
				GT_OK();
			} ); if ( !error.empty() ) { return error; }
		}
		GT_OK();
	} );
	task->AddTest( "Conventional home actual Start restore binds only after Bases and preserves saved status", GT() {
		ConventionalUnitFixture source; source.Configure(); std::string saved;
		const auto prepared = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = source.Game(); Define( game->GetUM() ); Land( game ); auto* owner = &game->GetState()->m_slots->GetSlot( 0 );
			auto* base = new base::Base( game, 701, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( 0, 0 ), "Saved home", {}, {}, 1, 0, {}, base::SmacBaseResourceContext{ 1, false, 0, 0, 0, true } );
			game->GetBM()->SpawnBase( GSE_CALL, base ); auto record = Record(); record.home = ConventionalUnitState::Home::Base; record.home_base_id = 701;
			Spawn( GSE_CALL, source, record ); source.ResourceState( []( auto& state ) { state.social.factions.front().retained->at( 2 ) = -4; } ); saved = source.Snapshot(); GT_OK();
		} ); if ( !prepared.empty() ) { return prepared; }
		ConventionalUnitFixture destination; destination.Configure();
		return destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			bool pending_observed = false;
			destination.RestoreThroughStart( GSE_CALL, saved, [ & ]( GSE_CALLABLE ) {
				auto* game = destination.Game(); auto* unit = game->GetMap()->GetTile( 2, 2 )->units.at( 71 );
				if ( game->GetBM()->GetBase( 701 ) ) { throw std::runtime_error( "home Base loaded before Start checkpoint" ); }
				try { unit->QueryStats( SmacMoraleMode::Mode1 ); } catch ( const std::runtime_error& ) { pending_observed = true; }
				if ( !pending_observed ) { throw std::runtime_error( "unresolved home exposed stats" ); }
			} );
			auto* game = destination.Game(); auto* unit = game->GetUM()->GetUnit( 71 ); auto* base = game->GetBM()->GetBase( 701 );
			GT_ASSERT( pending_observed && unit && base && unit->Conventional().home_base_id == 701 && base->GetSmacResourceContext().drone_riots
				&& game->GetCanonicalMineralSupport( *base ) == 2, "actual Start lost saved link/status/support" );
			GT_ASSERT( unit->QueryStats( SmacMoraleMode::Mode1 ).effective_morale == 1, "restored home stats unavailable or recomputed status" ); GT_OK();
		} );
	} );

	task->AddTest( "movement history nearest base preserves original metric ties and empty output", GT() {
		for ( size_t i = 0; i < nearest_any_fixture::cases.size(); ++i ) {
			const auto& row = nearest_any_fixture::cases[ i ];
			const auto result = SmacMovementHistory::NearestAnyBase( row.x, row.y, row.width, row.wrap, row.bases, 777 );
			GT_ASSERT( result.base == ( row.expected < 0 ? std::optional< uint64_t >{} : std::optional< uint64_t >( row.expected ) )
				&& result.distance == row.distance, "original nearest-base output differs at case " + std::to_string( i + 1 ) );
		}
		GT_ASSERT( !SmacMovementHistory::NearestAnyBase( 0, 0, 128, true, {}, {} ).distance, "empty world invented zero distance" );
		for ( int invalid = 0; invalid < 5; ++invalid ) {
			bool rejected = false;
			try { SmacMovementHistory::NearestAnyBase( invalid == 0 ? -1 : 0, 0, invalid == 1 ? 0 : 128, true,
				invalid == 2 ? std::vector< SmacMovementHistory::BasePosition >{{2,0,0},{1,2,0}} : std::vector< SmacMovementHistory::BasePosition >{},
				invalid == 3 ? -1 : invalid == 4 ? 10000 : 777 ); } catch ( const std::runtime_error& ) { rejected = true; }
			GT_ASSERT( rejected, "invalid nearest-base input admitted" );
		}
		GT_OK();
	} );
	task->AddTest( "movement history codec bounds dimensions packed bits and optional nearest result", GT() {
		auto value = SmacMovementHistory::NewGeneratedWorld( 6, 6 );
		GT_ASSERT( !value.NearestDistance() && !value.WasPlaced( 5, 5 ), "generated policy did not use explicit clean placement and unknown distance" );
		value.RetainNearest( 9999 );
		const auto valid = value.Serialize();
		GT_ASSERT( SmacMovementHistory::Deserialize( valid ) == value, "movement history roundtrip changed authority" );
		const auto largest = SmacMovementHistory::NewGeneratedWorld( 512, 256 ).Serialize();
		GT_ASSERT( largest.size() == SmacMovementHistory::MAX_WIRE_BYTES && SmacMovementHistory::Deserialize( largest ).Serialize() == largest, "exact map history budget rejected" );
		for ( int scenario = 0; scenario < 12; ++scenario ) {
			auto wire = valid;
			if ( scenario == 0 ) { wire[0] = 'X'; }
			if ( scenario == 1 ) { wire[4] = 2; }
			if ( scenario == 2 ) { wire[8] = 0; }
			if ( scenario == 3 ) { wire[12] = 5; }
			if ( scenario == 4 ) { wire[15] = 127; }
			if ( scenario == 5 ) { wire[16] = 0; }
			if ( scenario == 6 ) { wire[20] = 2; }
			if ( scenario == 7 ) { wire[20] = 0; } // absent must carry canonical zero, not9999
			if ( scenario == 8 ) { wire[27] = 127; }
			if ( scenario == 9 ) { wire.back() = char( 128 ); }
			if ( scenario == 10 ) { wire.pop_back(); }
			if ( scenario == 11 ) { wire.resize( SmacMovementHistory::MAX_WIRE_BYTES + 1 ); }
			bool rejected = false; try { (void)SmacMovementHistory::Deserialize( wire ); } catch ( const std::runtime_error& ) { rejected = true; }
			GT_ASSERT( rejected && value.Serialize() == valid, "invalid movement history changed source or decoded at case " + std::to_string( scenario ) );
		}
		GT_OK();
	} );
	task->AddTest( "movement history actual placement departure and ordinary command preserve visits", GT() {
		ConventionalUnitFixture fixture; fixture.Configure( true, 3, true );
		GT_ASSERT( !fixture.MovementHistory(), "synthetic non-generated simulation invented clean movement history" );
		fixture.MovementHistory( SmacMovementHistory::NewGeneratedWorld( 8, 8 ) );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* manager = game->GetUM(); auto* def = Define( manager ); Land( game );
			auto* unit = Spawn( GSE_CALL, fixture, Record() ); auto* source = unit->GetTile(); auto* destination = game->GetMap()->GetTile( 3, 3 );
			GT_ASSERT( fixture.MovementHistory()->WasPlaced( 2, 2 ) && !fixture.MovementHistory()->WasPlaced( 3, 3 ), "actual initial placement missing" );
			fixture.MoveEvent( GSE_CALL, unit, destination );
			GT_ASSERT( unit->GetTile() == destination && fixture.MovementHistory()->WasPlaced( 2, 2 ) && fixture.MovementHistory()->WasPlaced( 3, 3 )
				&& !fixture.MovementHistory()->NearestDistance(), "ordinary command lost source visit or invented nearest query" );
			manager->ResetConventionalSpent( GSE_CALL, unit ); fixture.MoveEvent( GSE_CALL, unit, source );
			GT_ASSERT( fixture.MovementHistory()->WasPlaced( 3, 3 ) && destination->units.empty(), "departure cleared the placement marker" );
			// The same placement producer covers legacy non-air units, but air
			// occupancy must not create the original non-air history marker.
			auto* airdef = new StaticDef( "HISTORY_AIR", def->m_moraleset, "air", MT_AIR, 1,
				new CvrScoutRender( 2, 0, 0, 1, {1,1}, {128,128} ) ); manager->DefineUnit( airdef );
			auto* air = new Unit( GSE_CALL, manager, 72, airdef, unit->m_owner, game->GetMap()->GetTile( 4, 4 ), 1, 1, 1, false ); manager->SpawnUnit( GSE_CALL, air );
			auto* land = new Unit( GSE_CALL, manager, 73, def, unit->m_owner, game->GetMap()->GetTile( 6, 6 ), 1, 1, 1, false ); manager->SpawnUnit( GSE_CALL, land );
			GT_ASSERT( !fixture.MovementHistory()->WasPlaced( 4, 4 ) && fixture.MovementHistory()->WasPlaced( 6, 6 ), "air/non-air placement classification differs" );
			manager->DespawnUnit( GSE_CALL, land->m_id );
			GT_ASSERT( fixture.MovementHistory()->WasPlaced( 6, 6 ), "despawn cleared placement history" ); GT_OK();
		} );
	} );
	task->AddTest( "movement history HQ failed attempt commits nearest but unsupported requests remain atomic", GT() {
		ConventionalUnitFixture fixture; fixture.Configure( true, 3, true );
		fixture.MovementHistory( SmacMovementHistory::NewGeneratedWorld( 8, 8 ) );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* manager = game->GetUM(); Define( manager ); Land( game );
			auto raw = Record(); raw.spent = 1; auto* unit = Spawn( GSE_CALL, fixture, raw ); auto* destination = game->GetMap()->GetTile( 3, 3 );
			HQ( GSE_CALL, game, unit, unit->GetTile() ); destination->features = map::tile::FEATURE_XENOFUNGUS;
			fixture.GameplayRandom( { 0, 0xa1b2c3d4 } ); // remaining2, captured shared draw2 refuses
			const auto before = fixture.MovementHistory(); manager->ValidateConventionalMove( unit, destination, 0 );
			GT_ASSERT( fixture.MovementHistory() == before, "event validation mutated history" );
			fixture.MoveEvent( GSE_CALL, unit, destination );
			GT_ASSERT( unit->GetTile() != destination && unit->Conventional().failed_move && unit->Conventional().spent == 3
				&& fixture.MovementHistory()->NearestDistance() == 1 && !fixture.MovementHistory()->WasPlaced( 3, 3 )
				&& fixture.GameplayRandom().Crt() == 0xa1b2c3d4, "accepted failed fungus attempt lost nearest result or replayed placement/CRT" );
			const auto saved = fixture.MovementHistory(); const auto random = fixture.GameplayRandom();
			bool rejected = false; try { manager->ValidateConventionalMove( unit, destination, 0 ); } catch ( const std::exception& ) { rejected = true; }
			fixture.MoveEvent( GSE_CALL, unit, destination );
			GT_ASSERT( rejected && fixture.MovementHistory() == saved && fixture.GameplayRandom() == random, "exhausted human request entered lower encounter wrapper" );
			manager->ResetConventionalSpent( GSE_CALL, unit ); destination->terraforming = map::tile::TERRAFORMING_FOREST;
			rejected = false; try { manager->ValidateConventionalMove( unit, destination, 0 ); } catch ( const std::exception& ) { rejected = true; }
			fixture.MoveEvent( GSE_CALL, unit, destination );
			GT_ASSERT( rejected && fixture.MovementHistory() == saved && fixture.GameplayRandom() == random, "unsupported terrain mutated caller history" ); GT_OK();
		} );
	} );
	task->AddTest( "movement history actual world restore never replays placement and retains late-failure authority", GT() {
		ConventionalUnitFixture source, restored, failed; source.Configure( true, 3, true ); restored.Configure( true, 3, true ); failed.Configure( true, 3, true );
		std::string bytes; std::optional< SmacMovementHistory > expected;
		const auto setup = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = source.Game(); Define( game->GetUM() ); Land( game ); auto* unit = Spawn( GSE_CALL, source, Record() );
			// Explicit opt-in AFTER an existing unit. A restore must preserve the
			// saved false bit, rather than reconstructing it from occupancy.
			source.MovementHistory( SmacMovementHistory::NewGeneratedWorld( 8, 8 ) );
			unit->SetTile( GSE_CALL, game->GetMap()->GetTile( 3, 3 ) ); unit->SetTile( GSE_CALL, game->GetMap()->GetTile( 4, 4 ) );
			auto history = *source.MovementHistory(); history.RetainNearest( 777 ); source.MovementHistory( history );
			// Saved independent bitmap with no occupied-square bit is legal port
			// state and isolates restore replay from saved current occupancy.
			auto wire = history.Serialize(); const auto index = history.PlacementIndex( 4, 4 ); wire[28 + index/8] &= char( ~( 1 << ( index%8 ) ) );
			source.MovementHistory( SmacMovementHistory::Deserialize( wire ) ); expected = source.MovementHistory(); bytes = source.Snapshot(); GT_OK();
		} ); if ( !setup.empty() ) { return setup; }
		const auto success = restored.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			restored.MovementHistory( SmacMovementHistory::NewGeneratedWorld( 8, 8 ) ); restored.Restore( GSE_CALL, bytes );
			GT_ASSERT( restored.MovementHistory() == expected && !restored.MovementHistory()->WasPlaced( 4, 4 ) && restored.Snapshot() == bytes, "restore replayed placement or changed history" );
			auto* unit = restored.Game()->GetUM()->GetUnit( 71 ); restored.MoveEvent( GSE_CALL, unit, restored.Game()->GetMap()->GetTile( 5, 5 ) );
			GT_ASSERT( restored.MovementHistory()->WasPlaced( 5, 5 ) && restored.MovementHistory()->NearestDistance() == 777, "restored ordinary continuation reset nearest history" ); GT_OK();
		} ); if ( !success.empty() ) { return success; }
		return failed.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto prior = SmacMovementHistory::NewGeneratedWorld( 8, 8 ); prior.RetainNearest( 23 ); failed.MovementHistory( prior );
			bool rejected = false; try { failed.Restore( GSE_CALL, bytes.substr( 0, bytes.size() - 1 ) ); } catch ( const std::exception& ) { rejected = true; }
			GT_ASSERT( rejected && failed.MovementHistory() == prior, "late body failure published or mutated staged history" );
			// This asserts only movement-history retention; it does not claim
			// rollback of the already decoded map/manager body.
			GT_OK();
		} );
	} );
	for ( const bool rover : { false, true } ) {
		task->AddTest( rover ? "conventional bottom bar refreshes selected Rover checked stats" : "conventional bottom bar preserves native previews and refreshes checked Infantry stats", GT( rover ) {
			ConventionalUnitFixture fixture; fixture.Configure( true, 3, false, rover );
			const auto next = Unit::GetNextId(); util::FinallyGuard restore_id( [ = ] { Unit::SetNextId( next ); } );
			return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); auto* manager = game->GetUM(); Define( manager, rover );
				fixture.RetainedMorale( 0, 7 ); // current rating deliberately differs from authoritative retained
				auto record = Record(); if ( rover ) { record.design_id = 128; record.owner_simulation_id = 2; }
				auto* unit = Spawn( GSE_CALL, fixture, record, 71, rover ? 2 : 0, rover ? "EXPLICIT_ROVER" : "EXPLICIT_SCOUT" );
				MoraleSet::morale_values_t names;
				for ( int i = 0; i < 7; ++i ) { names.emplace_back( "native" + std::to_string( i ) ); }
				manager->DefineMoraleSet( new MoraleSet( "NATIVE", names ) );
				auto* legacy_def = new StaticDef( "SporeLauncher", manager->GetMoraleSet( "NATIVE" ), "Native fixture", MT_LAND, 2.0f,
					new CvrScoutRender( 2, 0, 0, 1, { 1.0f, 1.0f }, { 128.0f, 128.0f } ) );
				manager->DefineUnit( legacy_def );
				auto* legacy = new Unit( GSE_CALL, manager, 72, legacy_def, &game->GetState()->m_slots->GetSlot( 0 ), unit->GetTile(), 1.25f, 4, 1.0f, false );
				manager->SpawnUnit( GSE_CALL, legacy );
				auto* initialize = static_cast< value::Callable* >( fixture.Script( GSE_CALL, "./GLSMAC_data/tests/_conventional_unit_preview" ) );
				auto* check = static_cast< value::Callable* >( initialize->Run( GSE_CALL, { game->Wrap( GSE_CALL ) } ) );
				const auto inspect = [ & ]( Unit* target, const char* stats, const char* morale, const char* moves ) -> std::string {
					fixture.Updates(); const auto before = fixture.Snapshot(); const auto jsf = game->GetRandom()->GetStateString();
					const auto random = fixture.GameplayRandom(); const auto id = Unit::GetNextId();
					auto* wrapper = target->Wrap( GSE_CALL );
					// These are the production frontend's published event names and actual
					// native wrapper identities; DOM.listen registered the real callbacks.
					game->GetState()->TriggerObject( game->GetMap(), "unit_preview", ARGS_F( &wrapper ) { { "unit", wrapper } }; } );
					game->GetState()->TriggerObject( game, "unit_select", ARGS_F( &wrapper ) { { "unit", wrapper } }; } );
					check->Run( GSE_CALL, { wrapper, new value::Object( GSE_CALL, {
						{ "stats", VALUE( value::String,, stats ) }, { "morale", VALUE( value::String,, morale ) }, { "moves", VALUE( value::String,, moves ) }
					} ) } );
					GT_ASSERT( fixture.Snapshot() == before && game->GetRandom()->GetStateString() == jsf && fixture.GameplayRandom() == random
						&& Unit::GetNextId() == id && fixture.Updates().empty(), "bottom bar changed authoritative world, RNG, ID or presentation queue" ); GT_OK();
				};
				auto error = inspect( legacy, "(?) - ? - 2", "native4", "Moves: 1.25" ); if ( !error.empty() ) { return error; }
				error = inspect( unit, rover ? "? - ? - 2.0" : "? - ? - 1.0", "level1", rover ? "Moves: 2.0" : "Moves: 1.0" ); if ( !error.empty() ) { return error; }
				auto* copied_stats = static_cast< value::Object* >( Call( GSE_CALL, static_cast< value::Object* >( unit->Wrap( GSE_CALL ) ),
					"query_conventional_stats", { VALUE( value::Int,, 0 ) } ) );
				record.spent = 1; manager->UpdateConventionalUnit( GSE_CALL, unit, record );
				error = inspect( unit, rover ? "? - ? - 2.0" : "? - ? - 1.0", "level1", rover ? "Moves: 1.67" : "Moves: 0.67" ); if ( !error.empty() ) { return error; }
				record.stored_morale = 6; manager->UpdateConventionalUnit( GSE_CALL, unit, record );
				error = inspect( unit, rover ? "? - ? - 3.0" : "? - ? - 2.0", "level6", rover ? "Moves: 2.67" : "Moves: 1.67" ); if ( !error.empty() ) { return error; }
				GT_ASSERT( static_cast< value::String* >( copied_stats->Get( "morale_name" ) )->value == "level1", "query label did not remain an owned snapshot" );
				record.damage = 5; manager->UpdateConventionalUnit( GSE_CALL, unit, record );
				error = inspect( unit, rover ? "? - ? - 2.0" : "? - ? - 1.0", "level6", rover ? "Moves: 1.67" : "Moves: 0.67" ); if ( !error.empty() ) { return error; }
				record.spent = 255; manager->UpdateConventionalUnit( GSE_CALL, unit, record );
				error = inspect( unit, rover ? "? - ? - 2.0" : "? - ? - 1.0", "level6", "Moves: 0.0" ); if ( !error.empty() ) { return error; }
				if ( !rover ) {
					record.stored_morale = 1; record.damage = 0; record.spent = 0; fixture.RetainedMorale( -4, 7 );
					manager->UpdateConventionalUnit( GSE_CALL, unit, record );
					error = inspect( unit, "? - ? - 1.0", "level0", "Moves: 1.0" ); if ( !error.empty() ) { return error; }
					// Both explicit modes are accepted. They coincide for the admitted
					// homeless context; no rioting-home distinction is invented here.
					record.render_morale_mode = SmacMoraleMode::Mode1;
					auto* alternate = Spawn( GSE_CALL, fixture, record, 73 );
					error = inspect( alternate, "? - ? - 1.0", "level0", "Moves: 1.0" ); if ( !error.empty() ) { return error; }
				}
				legacy->Legacy().movement = 0.5f; legacy->Legacy().morale = 6;
				error = inspect( legacy, "(?) - ? - 2", "native6", "Moves: 0.5" ); if ( !error.empty() ) { return error; }
				auto* tower_def = new StaticDef( "FungalTower", manager->GetMoraleSet( "NATIVE" ), "Immovable fixture", MT_IMMOVABLE, 0.0f,
					new CvrScoutRender( 2, 0, 0, 1, { 1.0f, 1.0f }, { 128.0f, 128.0f } ) );
				manager->DefineUnit( tower_def );
				auto* tower = new Unit( GSE_CALL, manager, 74, tower_def, legacy->m_owner, unit->GetTile(), 0.0f, 2, 1.0f, false );
				manager->SpawnUnit( GSE_CALL, tower );
				error = inspect( tower, "? - ? - 0", "native2", "unused" ); if ( !error.empty() ) { return error; }
				// Losing an admitted query context must fail afresh rather than render
				// cached values or invent retained morale. No full-world rollback claim.
				fixture.Early(); const auto stored = unit->Conventional(); const auto random = fixture.GameplayRandom();
				auto* wrapper = unit->Wrap( GSE_CALL ); bool rejected = false;
				try { game->GetState()->TriggerObject( game->GetMap(), "unit_preview", ARGS_F( &wrapper ) { { "unit", wrapper } }; } ); }
				catch ( const std::exception& ) { rejected = true; }
				GT_ASSERT( rejected && unit->Conventional() == stored && fixture.GameplayRandom() == random && fixture.Updates().empty(),
					"preview reused stale query state or changed authority after context rejection" ); GT_OK();
			} );
		} );
	}
	for ( const bool remote : { false, true } ) {
		task->AddTest( remote ? "conventional remote ordinary ground matches12 literal remote outcomes" : "conventional remote ordinary ground matches12 literal empty controls", GT( remote ) {
			const auto unit_next = Unit::GetNextId(), base_next = base::Base::GetNextId();
			util::FinallyGuard restore_ids( [ = ] { Unit::SetNextId( unit_next ); base::Base::SetNextId( base_next ); } );
			for ( const auto& c : remote_ground_fixture::CASES ) {
				if ( c.remote != remote ) { continue; }
				ConventionalUnitFixture fixture; fixture.ConfigureMovementCampaign( c.rover );
				const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
					auto* game = fixture.Game(); const auto world = PopulateRemoteGround( GSE_CALL, fixture, c ); auto* unit = world.local;
					auto* source = unit->GetTile(); auto* destination = game->GetMap()->GetTile( 17, 17 );
					const auto stats = unit->QueryStats( SmacMoraleMode::Mode1 );
					GT_ASSERT( stats.full_allowance == c.allowance && stats.effective_morale == ( c.rover ? 2 : 0 ) && !source->base && !destination->base
						&& game->GetUM()->GetUnits().size() == ( remote ? 7u : 1u ) && game->GetBM()->GetBases().size() == ( remote ? 7u : 0u ), "remote ground producer or endpoint setup differs" );
					const auto bases = BaseBytes( game ); std::vector< std::string > remotes;
					for ( const auto* other : world.remote ) { remotes.push_back( Unit::Serialize( other ).ToString() ); }
					const auto jsf = game->GetRandom()->GetStateString(); const auto next = Unit::GetNextId();
					auto expected = unit->Conventional(); expected.spent = static_cast< uint8_t >( c.spent_after ); expected.failed_move = c.failed;
					fixture.MoveEvent( GSE_CALL, unit, destination );
					GT_ASSERT( unit->Conventional() == expected && unit->GetTile() == ( c.moved ? destination : source )
						&& source->units.count( unit->m_id ) == ( c.moved ? 0u : 1u ) && destination->units.count( unit->m_id ) == ( c.moved ? 1u : 0u )
						&& fixture.GameplayRandom() == SmacGameplayRandomState( c.shared_after, c.crt_after ), "case " + std::string( c.id ) + " differs from literal remote ground outcome" );
					GT_ASSERT( BaseBytes( game ) == bases && game->GetRandom()->GetStateString() == jsf && Unit::GetNextId() == next
						&& !source->base && !destination->base, "remote ground changed Base, endpoint, ID or JSF state" );
					for ( size_t i = 0; i < world.remote.size(); ++i ) {
						const auto* other = world.remote[ i ];
						GT_ASSERT( Unit::Serialize( other ).ToString() == remotes[ i ] && other->GetTile()->units.at( other->m_id ) == other, "remote ground changed remote Unit bytes or membership" );
					}
					const auto& updates = fixture.Updates();
					GT_ASSERT( std::count_if( updates.begin(), updates.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_UPDATE; } ) == ( c.spent < c.allowance ? 1 : 0 ), "remote ground presentation differs" );
					GT_OK();
				} ); if ( !error.empty() ) { return error; }
			}
			GT_OK();
		} );
	}
	task->AddTest( "conventional owned base nonfungus terrain matches18 original traces", []( GSE*, context::Context* ) -> std::string {
		using namespace map::tile;
		const auto unit_next = Unit::GetNextId(), base_next = base::Base::GetNextId();
		util::FinallyGuard restore_ids( [ = ] { Unit::SetNextId( unit_next ); base::Base::SetNextId( base_next ); } );
		for ( const auto& c : owned_base_terrain_fixture::CASES ) {
			ConventionalUnitFixture fixture; fixture.ConfigureMovementCampaign( c.rover );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); Land( game ); Define( game->GetUM(), c.rover );
				auto* player = game->GetState()->m_slots->GetSlot( 0 ).GetPlayer();
				auto* view = Call( GSE_CALL, static_cast< value::Object* >( game->Wrap( GSE_CALL ) ), "get_startup_designs", { player->Wrap( GSE_CALL ) } );
				auto record = Record(); record.owner_simulation_id = static_cast< int32_t >( Int( view, "simulation_id" ) );
				record.design_id = static_cast< int32_t >( Int( view, "starting_scout" ) );
				auto history = SmacMovementHistory::NewGeneratedWorld( 128, 64 ); history.RetainNearest( 777 ); fixture.MovementHistory( history );
				auto* unit = Spawn( GSE_CALL, fixture, record, 71, 0, c.rover ? "EXPLICIT_ROVER" : "EXPLICIT_SCOUT" );
				auto* source = game->GetMap()->GetTile( c.sx, c.sy ); auto* destination = game->GetMap()->GetTile( c.dx, c.dy );
				unit->SetTile( GSE_CALL, source );
				auto* base = HQ( GSE_CALL, game, unit, game->GetMap()->GetTile( c.bx, c.by ), 31, c.population, c.hq );
				const auto terrain = []( Tile* tile, int flags ) {
					if ( flags != 0 && flags != 4 && flags != 128 && flags != 2097152 ) { throw std::runtime_error( "unsupported original ordinary fixture flags" ); }
					tile->features = flags == 128 ? FEATURE_RIVER : FEATURE_NONE;
					tile->terraforming = flags == 4 ? TERRAFORMING_ROAD : flags == 2097152 ? TERRAFORMING_FOREST : TERRAFORMING_NONE;
				};
				terrain( source, c.source_flags ); terrain( destination, c.destination_flags );
				destination->rockiness = static_cast< rockiness_t >( ROCKINESS_FLAT + c.rockiness );
				// The receipt observes no nearest-base query. Preserve a nonzero old
				// result while actual placement still marks the newly occupied tile.
				history = *fixture.MovementHistory();
				fixture.GameplayRandom( { 12345, 0xa1b2c3d4 } ); fixture.Updates();
				GT_ASSERT( unit->QueryStats( SmacMoraleMode::Mode1 ).full_allowance == c.allowance && base->m_pops.size() == static_cast< size_t >( c.population )
					&& base->m_facilities.empty() == !c.hq, "original owned-Base setup differs" );
				const auto base_bytes = BaseBytes( game ); const auto jsf = game->GetRandom()->GetStateString(); const auto next = Unit::GetNextId();
				const auto sf = source->features, df = destination->features, st = source->terraforming, dt = destination->terraforming;
				game->GetUM()->ValidateConventionalMove( unit, destination, 0 );
				GT_ASSERT( fixture.MovementHistory() == history && fixture.GameplayRandom() == SmacGameplayRandomState( 12345, 0xa1b2c3d4 ), "ordinary validation changed authority" );
				fixture.MoveEvent( GSE_CALL, unit, destination );
				auto expected = record; expected.spent = static_cast< uint8_t >( c.spent );
				GT_ASSERT( unit->GetTile() == destination && unit->Conventional() == expected && source->units.empty()
					&& destination->units.size() == 1 && destination->units.at( unit->m_id ) == unit
					&& fixture.GameplayRandom() == SmacGameplayRandomState( c.shared_after, 0xa1b2c3d4 ), "original owned-Base movement differs at " + std::string( c.id ) );
				GT_ASSERT( BaseBytes( game ) == base_bytes && base->GetTile()->base == base && source->features == sf && destination->features == df
					&& source->terraforming == st && destination->terraforming == dt && fixture.MovementHistory()->NearestDistance() == 777
					&& fixture.MovementHistory()->WasPlaced( c.sx, c.sy ) && fixture.MovementHistory()->WasPlaced( c.dx, c.dy ) && game->GetRandom()->GetStateString() == jsf
					&& Unit::GetNextId() == next, "ordinary movement changed Base, nearest result, JSF or ID authority" );
				const auto& updates = fixture.Updates();
				GT_ASSERT( std::count_if( updates.begin(), updates.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_UPDATE; } ) == 1, "ordinary move did not publish one refreshed unit" );
				GT_OK();
			} ); if ( !error.empty() ) { return error; }
		}
		GT_OK();
	} );
	task->AddTest( "conventional remote ordinary ground rejects unsupported contexts atomically", []( GSE*, context::Context* ) -> std::string {
		using namespace map::tile;
		const auto unit_next = Unit::GetNextId(), base_next = base::Base::GetNextId();
		util::FinallyGuard restore_ids( [ = ] { Unit::SetNextId( unit_next ); base::Base::SetNextId( base_next ); } );
		// Former terrain/equal-infrastructure/owned-proximity rejections are
		// superseded by the complete ordinary movement receipt above.
		for ( const int scenario : { 0, 1, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 } ) {
			ConventionalUnitFixture fixture; fixture.ConfigureMovementCampaign();
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); auto* manager = game->GetUM(); const auto world = PopulateRemoteGround( GSE_CALL, fixture, remote_ground_fixture::CASES[ 0 ] );
				auto* unit = world.local; auto* source = unit->GetTile(); auto* destination = game->GetMap()->GetTile( 17, 17 ); size_t caller = 0;
				if ( scenario == 0 ) { source->features = FEATURE_XENOFUNGUS; }
				if ( scenario == 1 ) { destination->features = FEATURE_XENOFUNGUS; }
				if ( scenario == 12 ) { auto state = unit->Conventional(); state.damage = 1; manager->UpdateConventionalUnit( GSE_CALL, unit, state ); }
				if ( scenario == 13 ) { auto state = unit->Conventional(); state.stored_morale = 2; manager->UpdateConventionalUnit( GSE_CALL, unit, state ); }
				if ( scenario == 14 ) { world.remote[ 0 ]->SetTile( GSE_CALL, source ); }
				if ( scenario == 15 ) { world.remote[ 0 ]->SetTile( GSE_CALL, destination ); }
				if ( scenario == 16 ) { world.remote[ 0 ]->SetTile( GSE_CALL, source->W ); }
				if ( scenario == 17 ) { world.remote[ 0 ]->SetTile( GSE_CALL, destination->E ); }
				if ( scenario == 18 ) { caller = 1; }
				if ( scenario == 19 ) { fixture.MovementCapability( faction::SmacSimulationState::MovementCapability::Unavailable ); }
				if ( scenario == 20 ) { game->GetState()->m_settings.local.network_role = settings::LocalSettings::NR_SERVER; }
				if ( scenario == 21 ) { source->Lock( 0 ); }
				util::FinallyGuard unlock( [ & ] { if ( source->IsLocked() ) { source->Unlock(); } } );
				if ( scenario == 22 ) { unit->m_animation_id = 7; }
				util::FinallyGuard animation( [ & ] { unit->m_animation_id = 0; } );
				if ( scenario == 23 ) { destination->is_water_tile = true; }
				if ( scenario == 24 ) { HQ( GSE_CALL, game, world.remote[ 0 ], source->W, 38 ); }
				fixture.Updates(); const auto before = fixture.Snapshot(); const auto state = unit->Conventional(); const auto members = source->units;
				const auto others = destination->units; const auto random = fixture.GameplayRandom(); const auto jsf = game->GetRandom()->GetStateString(); const auto next = Unit::GetNextId();
				bool rejected = false; try { manager->ValidateConventionalMove( unit, destination, caller ); } catch ( const std::exception& ) { rejected = true; }
				GT_ASSERT( rejected, "unsupported remote ground context accepted at " + std::to_string( scenario ) );
				fixture.MoveEvent( GSE_CALL, unit, destination, caller );
				GT_ASSERT( fixture.Snapshot() == before && unit->Conventional() == state && unit->GetTile() == source && source->units == members && destination->units == others
					&& fixture.GameplayRandom() == random && game->GetRandom()->GetStateString() == jsf && Unit::GetNextId() == next, "unsupported remote ground context changed authority" );
				const auto& updates = fixture.Updates();
				GT_ASSERT( std::none_of( updates.begin(), updates.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_UPDATE || r.type == game::FrontendRequest::FR_UNIT_MOVE; } ), "rejected remote ground queued movement" ); GT_OK();
			} ); if ( !error.empty() ) { return error; }
		}
		GT_OK();
	} );
	task->AddTest( "conventional remote ordinary ground restores complete state and continues next command", []( GSE*, context::Context* ) -> std::string {
		const auto unit_next = Unit::GetNextId(), base_next = base::Base::GetNextId();
		util::FinallyGuard restore_ids( [ = ] { Unit::SetNextId( unit_next ); base::Base::SetNextId( base_next ); } );
		for ( const bool rover : { false, true } ) {
			ConventionalUnitFixture source, destination; source.ConfigureMovementCampaign( rover ); destination.ConfigureMovementCampaign( rover );
			std::string snapshot, canonical; std::map< size_t, std::string > bases;
			const auto setup = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				const auto& c = remote_ground_fixture::CASES[ rover ? 2 : 14 ]; const auto world = PopulateRemoteGround( GSE_CALL, source, c );
				source.MoveEvent( GSE_CALL, world.local, source.Game()->GetMap()->GetTile( 17, 17 ) );
				GT_ASSERT( world.local->Conventional().failed_move && world.local->Conventional().spent == c.allowance, "remote ground restore setup did not preserve failed attempt" );
				bases = BaseBytes( source.Game() ); snapshot = source.Snapshot(); canonical = CanonicalMovementSnapshot( source.Game(), snapshot ); GT_OK();
			} ); if ( !setup.empty() ) { return setup; }
			const auto restored = destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				destination.Restore( GSE_CALL, snapshot ); destination.Game()->GetBM()->ProcessUnprocessed( GSE_CALL );
				GT_ASSERT( CanonicalMovementSnapshot( destination.Game(), destination.Snapshot() ) == canonical && BaseBytes( destination.Game() ) == bases, "remote ground restore changed complete canonical world" ); GT_OK();
			} ); if ( !restored.empty() ) { return restored; }
			std::string continued;
			for ( auto* fixture : { &source, &destination } ) {
				const auto error = fixture->Run( [ & ]( GSE_CALLABLE ) -> std::string {
					auto* game = fixture->Game(); game->AdvanceTurn( 2 ); auto* unit = game->GetUM()->GetUnit( 71 ); auto* target = game->GetMap()->GetTile( 17, 17 );
					GT_ASSERT( unit->Conventional().spent == 0 && unit->Conventional().failed_move, "remote ground upkeep discarded failure history" );
					fixture->MoveEvent( GSE_CALL, unit, target );
					GT_ASSERT( unit->GetTile() == target && unit->Conventional().spent == 3 && !unit->Conventional().failed_move && BaseBytes( game ) == bases, "remote ground continuation failed or changed bases" );
					const auto value = CanonicalMovementSnapshot( game, fixture->Snapshot() );
					if ( continued.empty() ) { continued = value; } else { GT_ASSERT( value == continued, "remote ground restored continuation diverged" ); } GT_OK();
				} ); if ( !error.empty() ) { return error; }
			}
		}
		GT_OK();
	} );
	const std::array< const char*, 8 > early_names = {{
		"Early conventional claimed queued Start preserves raw order then seals before upkeep",
		"Early conventional claimed immediate Start preserves raw order then seals before upkeep",
		"Early conventional missing first turn retires raw identities without publication",
		"Early conventional all-unit seal preflight rejects incompatible later selection atomically",
		"Early conventional Pending Start cancellation creates no raw units",
		"Early conventional Running reset waits then retires raw identities",
		"Early conventional throwing raw event cleans identity and frontend ownership",
		"Early conventional unclaimed spawn rejects before ID and tile effects"
	}};
	for ( int scenario = 0; scenario < static_cast< int >( early_names.size() ); ++scenario ) {
		task->AddTest( early_names[ scenario ], GT( scenario ) { ConventionalUnitFixture fixture; return fixture.CheckEarlyStartup( scenario ); } );
	}
	task->AddTest( "selected Spartan Rover actual spawn and staged restore preserve presentation authority", GT() {
		ConventionalUnitFixture source, destination; source.Configure( true, 3, false, true ); destination.Configure( true, 3, false, true );
		const auto next = Unit::GetNextId(); util::FinallyGuard restore_id( [ = ] { Unit::SetNextId( next ); } );
		ConventionalUnitState record{ 128, 2, 1, 0, 1, true, ConventionalUnitState::Home::None, SmacMoraleMode::Mode0 };
		std::string bytes;
		const auto setup = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = source.Game(); auto* manager = game->GetUM(); Define( manager, true );
			auto* unit = Spawn( GSE_CALL, source, record, 91, 2, "EXPLICIT_ROVER" );
			GT_ASSERT( unit->m_owner->GetIndex() == 2 && unit->Conventional() == record
				&& static_cast< StaticDef* >( unit->m_def )->m_render->m_type == Render::RT_CVR_SPARTAN_ROVER,
				"selected Rover lost canonical owner or presentation" );
			const auto stats = unit->QueryStats( SmacMoraleMode::Mode0 );
			GT_ASSERT( stats.effective_morale == 2 && stats.full_allowance == 6 && stats.remaining_allowance == 5, "selected Rover used Infantry or unrefreshed stats" );
			const auto& updates = source.Updates();
			const auto spawn = std::find_if( updates.begin(), updates.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_SPAWN; } );
			GT_ASSERT( spawn != updates.end() && spawn->data.unit_spawn.slot_index == 2 && spawn->data.unit_spawn.morale == 2
				&& spawn->data.unit_spawn.full_movement == 2.0f && spawn->data.unit_spawn.movement == 5.0f / 3.0f,
				"selected Rover spawn FR lost its actual derived stats" );
			bytes = source.Snapshot(); GT_OK();
		} ); if ( !setup.empty() ) { return setup; }
		return destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			// Current context is valid but deliberately selects the Infantry fallback.
			// Only the saved private context can authorize this Rover during UM decode.
			destination.SelectStartup( 2, 2 );
			destination.Restore( GSE_CALL, bytes );
			auto* game = destination.Game(); auto* unit = game->GetUM()->GetUnit( 91 );
			GT_ASSERT( unit && unit->Conventional() == record && unit->m_def == game->GetUM()->GetUnitDef( "EXPLICIT_ROVER" )
				&& unit->m_owner == &game->GetState()->m_slots->GetSlot( 2 ) && unit->GetTile() == game->GetMap()->GetTile( 2, 2 )
				&& unit->QueryStats( SmacMoraleMode::Mode0 ).full_allowance == 6 && destination.Snapshot() == bytes,
				"saved selected Rover was rebound to current selection or lost exact destination state" ); GT_OK();
		} );
	} );
	task->AddTest( "selected Spartan Rover rejects other owners selections and mismatched renderers before effects", GT() {
		ConventionalUnitFixture fixture; fixture.Configure( true, 3, false, true );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* manager = game->GetUM(); Define( manager ); Define( manager, true ); fixture.Updates();
			// Structurally valid owned rows isolate faction/selection admission from
			// component legality. This does not claim the startup producer makes them.
			fixture.CopyRoverDesign( 2, 129 ); fixture.CopyRoverDesign( 3, 192 ); fixture.SelectStartup( 3, 192 );
			const auto next = Unit::GetNextId(); const auto before = fixture.Snapshot(); size_t hooks = 0;
			manager->On( GSE_CALL, "unit_spawn", NATIVE_CALL( &hooks ) { ++hooks; return VALUE( value::Undefined ); } ) );
			for ( int scenario = 0; scenario < 6; ++scenario ) {
				ConventionalUnitState state{ 128, 2, 1, 0, 0, false, ConventionalUnitState::Home::None, SmacMoraleMode::Mode0 };
				size_t owner = 2; const char* def = "EXPLICIT_ROVER";
				if ( scenario == 0 ) { def = "EXPLICIT_SCOUT"; }
				if ( scenario == 1 ) { state.design_id = 129; }
				if ( scenario == 2 ) { state.design_id = 192; state.owner_simulation_id = 3; owner = 6; }
				if ( scenario == 3 ) { state.design_id = 2; }
				if ( scenario == 4 ) { state.design_id = 11; }
				if ( scenario == 5 ) { state.owner_simulation_id = 3; owner = 6; }
				bool rejected = false; try { Spawn( GSE_CALL, fixture, state, next, owner, def ); } catch ( const std::exception& ) { rejected = true; }
				GT_ASSERT( rejected && Unit::GetNextId() == next && manager->GetUnits().empty() && game->GetMap()->GetTile( 2, 2 )->units.empty()
					&& hooks == 0 && fixture.Updates().empty(), "invalid selected Rover produced effects at scenario " + std::to_string( scenario ) );
			}
			GT_ASSERT( fixture.Snapshot() == before, "rejected selected Rover changed paired RNG or saved world" ); GT_OK();
		} );
	} );
	task->AddTest( "conventional selected Rover HQ movement matches21 original startup outcomes", []( GSE*, context::Context* ) -> std::string {
		const auto unit_next = Unit::GetNextId(), base_next = base::Base::GetNextId();
		util::FinallyGuard restore_ids( [ = ] { Unit::SetNextId( unit_next ); base::Base::SetNextId( base_next ); } );
		for ( int endpoint = 1; endpoint < 3; ++endpoint ) {
			ConventionalUnitFixture fixture; fixture.Configure( true, 3, true, true );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); auto* manager = game->GetUM(); Land( game ); Define( manager, true );
				fixture.RetainedPlanet( 0, 7 ); // deliberately disagreeing current value must not replace retained
				auto* view = Call( GSE_CALL, static_cast< value::Object* >( game->Wrap( GSE_CALL ) ), "get_startup_designs", { game->GetState()->m_slots->GetSlot( 0 ).GetPlayer()->Wrap( GSE_CALL ) } );
				auto initial = Record(); initial.design_id = static_cast< int32_t >( Int( view, "starting_scout" ) );
				auto* unit = Spawn( GSE_CALL, fixture, initial, 71, 0, "EXPLICIT_ROVER" );
				const auto stats = unit->QueryStats( SmacMoraleMode::Mode0 );
				GT_ASSERT( initial.design_id == 64 && stats.effective_morale == 2 && stats.basic_allowance == 6 && stats.full_allowance == 6, "real selected Rover producer or refreshed context differs" );
				auto* source = game->GetMap()->GetTile( 2, 2 ); auto* destination = game->GetMap()->GetTile( 3, 3 );
				auto* hq = endpoint ? HQ( GSE_CALL, game, unit, endpoint == 1 ? source : destination ) : nullptr;
				for ( const auto& c : spartan_hq_fixture::CASES ) {
					if ( c.endpoint != endpoint ) { continue; }
					Land( game ); unit->SetTile( GSE_CALL, source ); auto record = initial; record.spent = static_cast< uint8_t >( c.spent );
					manager->UpdateConventionalUnit( GSE_CALL, unit, record );
					OriginalFeatures( source, c.source_flags ); OriginalFeatures( destination, c.destination_flags );
					fixture.GameplayRandom( { c.seed, 0xabcdef01 } ); fixture.Updates();
					const auto base_bytes = hq ? base::Base::Serialize( hq ).ToString() : std::string{};
					const auto jsf = game->GetRandom()->GetStateString(); const auto next = Unit::GetNextId();
					const auto sf = source->features, df = destination->features, st = source->terraforming, dt = destination->terraforming;
					fixture.MoveEvent( GSE_CALL, unit, destination, 0 );
					auto expected = record; expected.spent = static_cast< uint8_t >( c.spent_after ); expected.failed_move = c.failed;
					GT_ASSERT( unit->Conventional() == expected && unit->GetTile() == ( c.moved ? destination : source )
						&& source->units.count( unit->m_id ) == ( c.moved ? 0u : 1u ) && destination->units.count( unit->m_id ) == ( c.moved ? 1u : 0u )
						&& fixture.GameplayRandom() == SmacGameplayRandomState( c.shared_after, 0xabcdef01 ), "case " + std::string( c.id ) + " differs from literal original movement outcome" );
					GT_ASSERT( game->GetRandom()->GetStateString() == jsf && Unit::GetNextId() == next && source->features == sf && destination->features == df
						&& source->terraforming == st && destination->terraforming == dt && ( !hq || base::Base::Serialize( hq ).ToString() == base_bytes ),
						"case " + std::string( c.id ) + " changed unrelated terrain/base/ID/JSF state" );
					const auto& updates = fixture.Updates();
					GT_ASSERT( std::count_if( updates.begin(), updates.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_UPDATE; } ) == ( c.spent < 6 ? 1 : 0 ),
						"case " + std::string( c.id ) + " presented an exhausted command or lost an admitted attempt" );
				}
				GT_OK();
			} ); if ( !error.empty() ) { return error; }
		}
		GT_OK();
	} );
	task->AddTest( "conventional Gaian HQ movement composes8 actual produced nonneutral outcomes", []( GSE*, context::Context* ) -> std::string {
		const auto unit_next = Unit::GetNextId(), base_next = base::Base::GetNextId();
		util::FinallyGuard restore_ids( [ = ] { Unit::SetNextId( unit_next ); base::Base::SetNextId( base_next ); } );
		for ( int endpoint = 1; endpoint < 3; ++endpoint ) {
			ConventionalUnitFixture fixture; fixture.Configure( true, 3, true );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); auto* manager = game->GetUM(); Land( game ); Define( manager );
				// Use actual frozen Gaian profile and first-command refresh; no injected ratings.
				auto* unit = Spawn( GSE_CALL, fixture, Record() );
				const auto stats = unit->QueryStats( SmacMoraleMode::Mode0 );
				GT_ASSERT( stats.effective_morale == 0 && stats.full_allowance == 3, "Gaian producer did not supply its actual refreshed context" );
				auto* source = game->GetMap()->GetTile( 2, 2 ); auto* destination = game->GetMap()->GetTile( 3, 3 );
				auto* hq = endpoint ? HQ( GSE_CALL, game, unit, endpoint == 1 ? source : destination ) : nullptr;
				for ( const auto& c : gaian_hq_fixture::CASES ) {
					if ( c.endpoint != endpoint ) { continue; }
					Land( game ); unit->SetTile( GSE_CALL, source ); auto record = Record(); record.spent = static_cast< uint8_t >( c.spent );
					manager->UpdateConventionalUnit( GSE_CALL, unit, record );
					OriginalFeatures( source, c.source_flags ); OriginalFeatures( destination, c.destination_flags );
					fixture.GameplayRandom( { c.seed, 0xabcdef01 } ); fixture.Updates();
					const auto base_bytes = hq ? base::Base::Serialize( hq ).ToString() : std::string{};
					const auto jsf = game->GetRandom()->GetStateString(); const auto next = Unit::GetNextId();
					const auto sf = source->features, df = destination->features, st = source->terraforming, dt = destination->terraforming;
					fixture.MoveEvent( GSE_CALL, unit, destination, 0 );
					auto expected = record; expected.spent = static_cast< uint8_t >( c.spent_after ); expected.failed_move = c.failed;
					GT_ASSERT( unit->Conventional() == expected && unit->GetTile() == ( c.moved ? destination : source )
						&& source->units.count( unit->m_id ) == ( c.moved ? 0u : 1u ) && destination->units.count( unit->m_id ) == ( c.moved ? 1u : 0u )
						&& fixture.GameplayRandom() == SmacGameplayRandomState( c.shared_after, 0xabcdef01 ), "case " + std::string( c.id ) + " differs from literal original movement outcome" );
					GT_ASSERT( game->GetRandom()->GetStateString() == jsf && Unit::GetNextId() == next && source->features == sf && destination->features == df
						&& source->terraforming == st && destination->terraforming == dt && ( !hq || base::Base::Serialize( hq ).ToString() == base_bytes ),
						"case " + std::string( c.id ) + " changed unrelated terrain/base/ID/JSF state" );
					const auto& updates = fixture.Updates();
					GT_ASSERT( std::count_if( updates.begin(), updates.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_UPDATE; } ) == ( c.spent < 3 ? 1 : 0 ),
						"case " + std::string( c.id ) + " presented an exhausted command or lost an admitted attempt" );
				}
				GT_OK();
			} ); if ( !error.empty() ) { return error; }
		}
		GT_OK();
	} );
	task->AddTest( "conventional remote HQ movement matches21 literal cardinality order and tie outcomes", []( GSE*, context::Context* ) -> std::string {
		const auto unit_next = Unit::GetNextId(), base_next = base::Base::GetNextId();
		util::FinallyGuard restore_ids( [ = ] { Unit::SetNextId( unit_next ); base::Base::SetNextId( base_next ); } );
		for ( const auto& c : remote_hq_fixture::CASES ) {
			ConventionalUnitFixture fixture; fixture.ConfigureMovementCampaign();
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); const auto world = PopulateRemoteHQ( GSE_CALL, fixture, c ); auto* unit = world.local;
				auto* source = unit->GetTile(); auto* destination = game->GetMap()->GetTile( 17, 17 );
				const auto stats = unit->QueryStats( SmacMoraleMode::Mode1 );
				GT_ASSERT( stats.full_allowance == 6 && stats.effective_morale == 2 && game->GetUM()->GetUnits().size() == 7 && game->GetBM()->GetBases().size() == 7,
					"remote setup did not produce selected Rover and seven canonical records" );
				const auto bases = BaseBytes( game ); std::vector< std::string > remotes;
				for ( auto* remote : world.remote ) { remotes.push_back( Unit::Serialize( remote ).ToString() ); }
				const auto jsf = game->GetRandom()->GetStateString(); auto expected = unit->Conventional();
				expected.spent = static_cast< uint8_t >( c.step.spent_after ); expected.failed_move = c.step.failed;
				fixture.MoveEvent( GSE_CALL, unit, destination );
				GT_ASSERT( unit->Conventional() == expected && unit->GetTile() == ( c.step.moved ? destination : source )
					&& source->units.count( unit->m_id ) == ( c.step.moved ? 0u : 1u ) && destination->units.count( unit->m_id ) == ( c.step.moved ? 1u : 0u )
					&& fixture.GameplayRandom() == SmacGameplayRandomState( c.step.shared_after, 0xa1b2c3d4 ),
					"case " + std::string( c.step.id ) + " differs from literal original outcome" );
				GT_ASSERT( BaseBytes( game ) == bases && game->GetRandom()->GetStateString() == jsf, "remote command changed base records or JSF" );
				for ( size_t i = 0; i < world.remote.size(); ++i ) {
					const auto* remote = world.remote[ i ];
					GT_ASSERT( Unit::Serialize( remote ).ToString() == remotes[ i ] && remote->GetTile()->units.at( remote->m_id ) == remote, "remote Unit bytes or membership changed" );
				}
				const auto& updates = fixture.Updates();
				GT_ASSERT( std::count_if( updates.begin(), updates.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_UPDATE; } ) == ( c.step.spent < 6 ? 1 : 0 ),
					"remote command refreshed remote units or lost local presentation" ); GT_OK();
			} ); if ( !error.empty() ) { return error; }
		}
		GT_OK();
	} );
	task->AddTest( "conventional multi participant HQ commands retain local authority and interaction fences", []( GSE*, context::Context* ) -> std::string {
		const auto unit_next = Unit::GetNextId(), base_next = base::Base::GetNextId();
		util::FinallyGuard restore_ids( [ = ] { Unit::SetNextId( unit_next ); base::Base::SetNextId( base_next ); } );
		for ( int scenario = 0; scenario < 9; ++scenario ) {
			ConventionalUnitFixture fixture; fixture.ConfigureMovementCampaign();
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); const auto world = PopulateRemoteHQ( GSE_CALL, fixture, remote_hq_fixture::CASES.back() );
				auto* unit = world.local; auto* source = unit->GetTile(); auto* destination = game->GetMap()->GetTile( 17, 17 ); size_t caller = 0;
				if ( scenario == 0 ) { caller = 1; }
				if ( scenario == 1 ) { unit = world.remote[ 1 ]; source = unit->GetTile(); destination = source->E; caller = 2; }
				if ( scenario == 2 ) { world.remote[ 0 ]->SetTile( GSE_CALL, source ); }
				if ( scenario == 3 ) { world.remote[ 1 ]->SetTile( GSE_CALL, source->W ); }
				if ( scenario == 4 ) { world.remote[ 1 ]->SetTile( GSE_CALL, destination->E ); }
				if ( scenario == 5 ) { world.remote[ 0 ]->SetTile( GSE_CALL, destination ); }
				if ( scenario == 6 ) { fixture.MovementCapability( faction::SmacSimulationState::MovementCapability::Unavailable ); }
				if ( scenario == 7 ) { game->GetState()->m_settings.local.network_role = settings::LocalSettings::NR_SERVER; }
				// Ordinary forest is admitted; distant destination fungus still
				// requires a separate encounter caller instead of the HQ bypass.
				if ( scenario == 8 ) { unit->SetTile( GSE_CALL, game->GetMap()->GetTile( 20, 20 ) ); source = unit->GetTile(); destination = source->E; destination->features = map::tile::FEATURE_XENOFUNGUS; }
				fixture.Updates(); const auto before = unit->Conventional(); const auto bases = BaseBytes( game );
				const auto random = fixture.GameplayRandom(); const auto members = source->units; const auto jsf = game->GetRandom()->GetStateString();
				bool rejected = false; try { game->GetUM()->ValidateConventionalMove( unit, destination, caller ); } catch ( const std::exception& ) { rejected = true; }
				GT_ASSERT( rejected, "remote-world unsupported command accepted at " + std::to_string( scenario ) );
				fixture.MoveEvent( GSE_CALL, unit, destination, caller );
				GT_ASSERT( unit->Conventional() == before && unit->GetTile() == source && source->units == members && BaseBytes( game ) == bases
					&& fixture.GameplayRandom() == random && game->GetRandom()->GetStateString() == jsf, "rejected remote command mutated authority" );
				const auto& updates = fixture.Updates();
				GT_ASSERT( std::none_of( updates.begin(), updates.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_UPDATE || r.type == game::FrontendRequest::FR_UNIT_MOVE; } ),
					"rejected remote command emitted unit presentation" ); GT_OK();
			} ); if ( !error.empty() ) { return error; }
		}
		GT_OK();
	} );
	task->AddTest( "conventional remote HQ actual world restore preserves roster and next command", []( GSE*, context::Context* ) -> std::string {
		const auto unit_next = Unit::GetNextId(), base_next = base::Base::GetNextId();
		util::FinallyGuard restore_ids( [ = ] { Unit::SetNextId( unit_next ); base::Base::SetNextId( base_next ); } );
		for ( const bool rover : { false, true } ) {
			ConventionalUnitFixture source, destination; source.ConfigureMovementCampaign( rover ); destination.ConfigureMovementCampaign( rover );
			std::string snapshot, canonical_snapshot; std::map< size_t, std::string > bases;
			const auto setup = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				const auto world = PopulateRemoteHQ( GSE_CALL, source, remote_hq_fixture::CASES.back(), rover );
				// Fail one actual command, retaining the saved marker and paired RNG.
				auto record = world.local->Conventional(); record.spent = rover ? 5 : 2;
				source.Game()->GetUM()->UpdateConventionalUnit( GSE_CALL, world.local, record );
				source.Game()->GetMap()->GetTile( 17, 17 )->features = map::tile::FEATURE_NONE; source.GameplayRandom( { 1, 77 } );
				source.MoveEvent( GSE_CALL, world.local, source.Game()->GetMap()->GetTile( 17, 17 ) );
				GT_ASSERT( world.local->Conventional().failed_move, "remote continuation setup did not fail" );
				bases = BaseBytes( source.Game() ); snapshot = source.Snapshot(); canonical_snapshot = CanonicalMovementSnapshot( source.Game(), snapshot ); GT_OK();
			} ); if ( !setup.empty() ) { return setup; }
			const auto restored = destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				destination.Restore( GSE_CALL, snapshot ); destination.Game()->GetBM()->ProcessUnprocessed( GSE_CALL );
				GT_ASSERT( CanonicalMovementSnapshot( destination.Game(), destination.Snapshot() ) == canonical_snapshot && BaseBytes( destination.Game() ) == bases,
					"remote restore changed complete canonical saved state" ); GT_OK();
			} ); if ( !restored.empty() ) { return restored; }
			std::string continued;
			for ( auto* fixture : { &source, &destination } ) {
				const auto error = fixture->Run( [ & ]( GSE_CALLABLE ) -> std::string {
					auto* game = fixture->Game(); game->AdvanceTurn( 2 ); auto* unit = game->GetUM()->GetUnit( 77 ); auto* target = game->GetMap()->GetTile( 17, 17 );
					GT_ASSERT( unit->Conventional().spent == 0 && unit->Conventional().failed_move, "upkeep changed saved failure history" );
					fixture->MoveEvent( GSE_CALL, unit, target );
					GT_ASSERT( unit->GetTile() == target && unit->Conventional().spent == 3 && !unit->Conventional().failed_move && BaseBytes( game ) == bases,
						"remote continuation did not preserve bases or perform the local next command" );
					const auto canonical = CanonicalMovementSnapshot( game, fixture->Snapshot() );
					if ( continued.empty() ) { continued = canonical; } else { GT_ASSERT( canonical == continued, "remote restored continuation diverged" ); }
					GT_OK();
				} ); if ( !error.empty() ) { return error; }
			}
		}
		GT_OK();
	} );
	task->AddTest( "conventional friendly HQ movement matches70 original homeless outcomes", []( GSE*, context::Context* ) -> std::string {
		const auto unit_next = Unit::GetNextId(), base_next = base::Base::GetNextId();
		util::FinallyGuard restore_ids( [ = ] { Unit::SetNextId( unit_next ); base::Base::SetNextId( base_next ); } );
		for ( int endpoint = 0; endpoint < 3; ++endpoint ) {
			ConventionalUnitFixture fixture; fixture.Configure( true, 3, true );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); auto* manager = game->GetUM(); Land( game ); Define( manager );
				fixture.RetainedPlanet( 0, 7 ); fixture.RetainedMorale( 0, 7 );
				auto* unit = Spawn( GSE_CALL, fixture, Record() );
				auto* source = game->GetMap()->GetTile( 2, 2 ); auto* destination = game->GetMap()->GetTile( 3, 3 );
				auto* hq = endpoint ? HQ( GSE_CALL, game, unit, endpoint == 1 ? source : destination ) : nullptr;
				for ( const auto& c : friendly_hq_fixture::CASES ) {
					if ( c.endpoint != endpoint ) { continue; }
					Land( game ); unit->SetTile( GSE_CALL, source ); auto record = Record(); record.spent = static_cast< uint8_t >( c.spent );
					manager->UpdateConventionalUnit( GSE_CALL, unit, record );
					OriginalFeatures( source, c.source_flags ); OriginalFeatures( destination, c.destination_flags );
					fixture.GameplayRandom( { c.seed, 0xabcdef01 } ); fixture.Updates();
					const auto base_bytes = hq ? base::Base::Serialize( hq ).ToString() : std::string{};
					const auto jsf = game->GetRandom()->GetStateString(); const auto next = Unit::GetNextId();
					const auto sf = source->features, df = destination->features, st = source->terraforming, dt = destination->terraforming;
					fixture.MoveEvent( GSE_CALL, unit, destination, 0 );
					auto expected = record; expected.spent = static_cast< uint8_t >( c.spent_after ); expected.failed_move = c.failed;
					GT_ASSERT( unit->Conventional() == expected && unit->GetTile() == ( c.moved ? destination : source )
						&& source->units.count( unit->m_id ) == ( c.moved ? 0u : 1u ) && destination->units.count( unit->m_id ) == ( c.moved ? 1u : 0u )
						&& fixture.GameplayRandom() == SmacGameplayRandomState( c.shared_after, 0xabcdef01 ), "case " + std::string( c.id ) + " differs from literal original movement outcome" );
					GT_ASSERT( game->GetRandom()->GetStateString() == jsf && Unit::GetNextId() == next && source->features == sf && destination->features == df
						&& source->terraforming == st && destination->terraforming == dt && ( !hq || base::Base::Serialize( hq ).ToString() == base_bytes ),
						"case " + std::string( c.id ) + " changed unrelated terrain/base/ID/JSF state" );
					const auto& updates = fixture.Updates();
					GT_ASSERT( std::count_if( updates.begin(), updates.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_UPDATE; } ) == ( c.spent < 3 ? 1 : 0 ),
						"case " + std::string( c.id ) + " presented an exhausted command or lost an admitted attempt" );
				}
				GT_OK();
			} ); if ( !error.empty() ) { return error; }
		}
		GT_OK();
	} );
	task->AddTest( "conventional selected Rover HQ rejects unmeasured or no-longer-selected contexts", []( GSE*, context::Context* ) -> std::string {
		const auto unit_next = Unit::GetNextId(), base_next = base::Base::GetNextId();
		util::FinallyGuard restore_ids( [ = ] { Unit::SetNextId( unit_next ); base::Base::SetNextId( base_next ); } );
		for ( int scenario = 0; scenario < 8; ++scenario ) {
			ConventionalUnitFixture fixture; fixture.Configure( true, scenario == 4 ? 6 : 3, true, true );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); auto* manager = game->GetUM(); Land( game ); Define( manager, true );
				auto record = Record(); record.design_id = 64;
				if ( scenario == 0 ) { record.damage = 1; } if ( scenario == 1 ) { record.stored_morale = 2; }
				auto* unit = Spawn( GSE_CALL, fixture, record, 71, 0, "EXPLICIT_ROVER" );
				auto* source = unit->GetTile(); auto* destination = game->GetMap()->GetTile( 3, 3 ); auto* hq = HQ( GSE_CALL, game, unit, source );
				fixture.Updates();
				if ( scenario == 2 ) { fixture.MovementCapability( faction::SmacSimulationState::MovementCapability::Unavailable ); }
				if ( scenario == 3 ) { source->Lock( 0 ); }
				util::FinallyGuard unlock( [ & ] { if ( source->IsLocked() ) { source->Unlock(); } } );
				if ( scenario == 5 ) { fixture.SelectStartup( 1, 2 ); }
				if ( scenario == 6 ) { fixture.CopyRoverDesign( 1, 65, 1 ); fixture.SelectStartup( 1, 65 ); }
				if ( scenario == 7 ) { auto* other = Spawn( GSE_CALL, fixture, record, 72, 0, "EXPLICIT_ROVER" ); other->SetTile( GSE_CALL, source ); fixture.Updates(); }
				const auto base_bytes = base::Base::Serialize( hq ).ToString();
				fixture.GameplayRandom( { 0, 77 } ); const auto jsf = game->GetRandom()->GetStateString(); const auto members = source->units;
				const auto before = unit->Conventional(); const auto next = Unit::GetNextId(); bool rejected = false;
				try { manager->ValidateConventionalMove( unit, destination, 0 ); } catch ( const std::exception& ) { rejected = true; }
				GT_ASSERT( rejected, "unsupported Rover HQ context admitted at " + std::to_string( scenario ) );
				fixture.MoveEvent( GSE_CALL, unit, destination );
				GT_ASSERT( unit->Conventional() == before && unit->GetTile() == source && source->units == members && destination->units.empty()
					&& fixture.GameplayRandom() == SmacGameplayRandomState( 0, 77 ) && game->GetRandom()->GetStateString() == jsf
					&& Unit::GetNextId() == next && base::Base::Serialize( hq ).ToString() == base_bytes, "rejected Rover HQ command changed authority" );
				const auto& updates = fixture.Updates();
				GT_ASSERT( std::none_of( updates.begin(), updates.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_UPDATE || r.type == game::FrontendRequest::FR_UNIT_MOVE; } ), "rejected Rover HQ command published unit movement" );
				GT_OK();
			} ); if ( !error.empty() ) { return error; }
		}
		GT_OK();
	} );
	task->AddTest( "conventional friendly HQ rejects unmeasured base and unit contexts atomically", []( GSE*, context::Context* ) -> std::string {
		using namespace map::tile;
		const auto unit_next = Unit::GetNextId(), base_next = base::Base::GetNextId();
		util::FinallyGuard restore_ids( [ = ] { Unit::SetNextId( unit_next ); base::Base::SetNextId( base_next ); } );
		// Ordinary terrain and larger/non-HQ Bases are now admitted. Preserve
		// the independent fungus-only population/facility boundary below.
		for ( const int scenario : { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 14, 18, 19, 20, 21 } ) {
			ConventionalUnitFixture fixture; fixture.Configure( true, scenario == 4 ? 6 : 3, true );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); auto* manager = game->GetUM(); Land( game ); Define( manager );
				fixture.RetainedPlanet( 0, 0 ); fixture.RetainedMorale( 0, 0 );
				auto record = Record(); if ( scenario == 0 ) { record.damage = 1; } if ( scenario == 1 ) { record.stored_morale = 2; }
				auto* unit = Spawn( GSE_CALL, fixture, record ); auto* source = unit->GetTile(); auto* destination = game->GetMap()->GetTile( 3, 3 );
				auto* hq = HQ( GSE_CALL, game, unit, source );
				fixture.Updates(); // retire valid spawn presentation before deliberate native corruption
				auto* original_owner = hq->m_owner; auto* original_faction = hq->m_faction;
				util::FinallyGuard restore_base( [ & ] { hq->m_owner = original_owner; hq->m_faction = original_faction; hq->GetTile()->base = hq; } );
				if ( scenario == 2 ) { fixture.MovementCapability( faction::SmacSimulationState::MovementCapability::Unavailable ); }
				if ( scenario == 3 ) { source->Lock( 0 ); }
				util::FinallyGuard unlock( [ & ] { if ( source->IsLocked() ) { source->Unlock(); } } );
				if ( scenario == 5 ) { hq->m_pops.clear(); }
				if ( scenario == 6 ) { hq->m_facilities.clear(); }
				if ( scenario == 7 ) { hq->m_facilities.emplace( "UNPROVED", game->GetBM()->GetFacilityDef( "HEADQUARTERS" ) ); }
				if ( scenario == 5 || scenario == 6 || scenario == 7 || scenario == 21 ) { destination->features = FEATURE_XENOFUNGUS; }
				if ( scenario == 8 ) { hq->m_owner = &game->GetState()->m_slots->GetSlot( 6 ); }
				if ( scenario == 9 ) { hq->m_faction = nullptr; }
				if ( scenario == 14 ) { source->features = FEATURE_XENOFUNGUS; }
				if ( scenario == 18 ) { destination->features = FEATURE_XENOFUNGUS; destination->terraforming = TERRAFORMING_ROAD; }
				if ( scenario == 19 ) { source->base = nullptr; }
				if ( scenario == 20 ) { auto* other = Spawn( GSE_CALL, fixture, record, 72 ); other->SetTile( GSE_CALL, source ); fixture.Updates(); }
				base::FacilityDef unregistered( "HEADQUARTERS", "Unregistered", "Fixture", 1, { "fixture", 0, 0, 1, 1 } );
				util::FinallyGuard restore_facility( [ & ] { if ( scenario == 21 ) { hq->m_facilities[ "HEADQUARTERS" ] = game->GetBM()->GetFacilityDef( "HEADQUARTERS" ); } } );
				if ( scenario == 21 ) { hq->m_facilities[ "HEADQUARTERS" ] = &unregistered; }
				fixture.GameplayRandom( { 3, 9 } ); const auto jsf = game->GetRandom()->GetStateString();
				const auto before = unit->Conventional(); const auto members = source->units; const auto next = Unit::GetNextId();
				std::string rejection; try { manager->ValidateConventionalMove( unit, destination, 0 ); } catch ( const std::exception& e ) { rejection = e.what(); }
				GT_ASSERT( !rejection.empty(), "unmeasured HQ context admitted at " + std::to_string( scenario ) );
				fixture.MoveEvent( GSE_CALL, unit, destination, 0 );
				GT_ASSERT( unit->Conventional() == before && unit->GetTile() == source && source->units == members && destination->units.empty()
					&& fixture.GameplayRandom() == SmacGameplayRandomState( 3, 9 ) && game->GetRandom()->GetStateString() == jsf && Unit::GetNextId() == next,
					"unmeasured HQ context mutated authority at " + std::to_string( scenario ) );
				const auto& updates = fixture.Updates();
				GT_ASSERT( std::none_of( updates.begin(), updates.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_UPDATE || r.type == game::FrontendRequest::FR_UNIT_MOVE; } ), "rejected HQ move queued Unit presentation" );
				GT_OK();
			} ); if ( !error.empty() ) { return error; }
		}
		GT_OK();
	} );
	task->AddTest( "conventional selected Rover HQ actual restore preserves failure and next command", []( GSE*, context::Context* ) -> std::string {
		const auto unit_next = Unit::GetNextId(), base_next = base::Base::GetNextId();
		util::FinallyGuard restore_ids( [ = ] { Unit::SetNextId( unit_next ); base::Base::SetNextId( base_next ); } );
		ConventionalUnitFixture source, destination; source.Configure( true, 3, true, true ); destination.Configure( true, 3, true, true );
		std::string snapshot, base_bytes;
		const auto setup = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = source.Game(); Land( game ); Define( game->GetUM(), true ); source.RetainedPlanet( 0, 9 );
			auto record = Record(); record.design_id = 64; record.spent = 5; auto* unit = Spawn( GSE_CALL, source, record, 71, 0, "EXPLICIT_ROVER" ); auto* hq = HQ( GSE_CALL, game, unit, unit->GetTile() );
			base_bytes = base::Base::Serialize( hq ).ToString(); source.GameplayRandom( { 0, 77 } );
			source.MoveEvent( GSE_CALL, unit, game->GetMap()->GetTile( 3, 3 ) );
			GT_ASSERT( unit->GetTile() == hq->GetTile() && unit->Conventional().failed_move && unit->Conventional().spent == 6, "saved HQ attempt did not fail at strict draw boundary" );
			snapshot = source.Snapshot(); GT_OK();
		} ); if ( !setup.empty() ) { return setup; }
		const auto restored = destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			destination.GameplayRandom( { 99, 88 } ); destination.MovementCapability( faction::SmacSimulationState::MovementCapability::Unavailable );
			destination.Restore( GSE_CALL, snapshot ); auto* game = destination.Game(); game->GetBM()->ProcessUnprocessed( GSE_CALL );
			auto* hq = game->GetBM()->GetBase( 31 ); auto* unit = game->GetUM()->GetUnit( 71 );
			GT_ASSERT( hq && hq->GetTile() == unit->GetTile() && hq->GetTile()->base == hq
				&& hq->m_facilities.at( "HEADQUARTERS" ) == game->GetBM()->GetFacilityDef( "HEADQUARTERS" )
				&& base::Base::Serialize( hq ).ToString() == base_bytes && destination.Snapshot() == snapshot,
				"world restore lost canonical HQ references or exact continuation" ); GT_OK();
		} ); if ( !restored.empty() ) { return restored; }
		std::string continued;
		for ( auto* fixture : { &source, &destination } ) {
			const auto error = fixture->Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture->Game(); game->AdvanceTurn( 2 ); auto* unit = game->GetUM()->GetUnit( 71 ); auto* target = game->GetMap()->GetTile( 3, 3 );
				GT_ASSERT( unit->Conventional().spent == 0 && unit->Conventional().failed_move, "upkeep lost saved HQ failure history" );
				fixture->MoveEvent( GSE_CALL, unit, target );
				GT_ASSERT( unit->GetTile() == target && unit->Conventional().spent == 3 && !unit->Conventional().failed_move
					&& base::Base::Serialize( game->GetBM()->GetBase( 31 ) ).ToString() == base_bytes, "continued HQ command changed base or failure history" );
				if ( continued.empty() ) { continued = fixture->Snapshot(); }
				else { GT_ASSERT( fixture->Snapshot() == continued, "restored HQ command diverged from uninterrupted whole world" ); }
				GT_OK();
			} ); if ( !error.empty() ) { return error; }
		}
		GT_OK();
	} );
	task->AddTest( "conventional friendly HQ actual world restore preserves canonical base and next command", []( GSE*, context::Context* ) -> std::string {
		const auto unit_next = Unit::GetNextId(), base_next = base::Base::GetNextId();
		util::FinallyGuard restore_ids( [ = ] { Unit::SetNextId( unit_next ); base::Base::SetNextId( base_next ); } );
		ConventionalUnitFixture source, destination; source.Configure( true, 3, true ); destination.Configure( true, 3, true );
		std::string snapshot, base_bytes;
		const auto setup = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = source.Game(); Land( game ); Define( game->GetUM() ); source.RetainedPlanet( 0, 9 ); source.RetainedMorale( 0, 9 );
			auto record = Record(); record.spent = 1; auto* unit = Spawn( GSE_CALL, source, record ); auto* hq = HQ( GSE_CALL, game, unit, unit->GetTile() );
			base_bytes = base::Base::Serialize( hq ).ToString(); source.GameplayRandom( { 0, 77 } );
			source.MoveEvent( GSE_CALL, unit, game->GetMap()->GetTile( 3, 3 ) );
			GT_ASSERT( unit->GetTile() == hq->GetTile() && unit->Conventional().failed_move && unit->Conventional().spent == 3, "saved HQ attempt did not fail at strict draw boundary" );
			snapshot = source.Snapshot(); GT_OK();
		} ); if ( !setup.empty() ) { return setup; }
		const auto restored = destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			destination.GameplayRandom( { 99, 88 } ); destination.MovementCapability( faction::SmacSimulationState::MovementCapability::Unavailable );
			destination.Restore( GSE_CALL, snapshot ); auto* game = destination.Game(); game->GetBM()->ProcessUnprocessed( GSE_CALL );
			auto* hq = game->GetBM()->GetBase( 31 ); auto* unit = game->GetUM()->GetUnit( 71 );
			GT_ASSERT( hq && hq->GetTile() == unit->GetTile() && hq->GetTile()->base == hq
				&& hq->m_facilities.at( "HEADQUARTERS" ) == game->GetBM()->GetFacilityDef( "HEADQUARTERS" )
				&& base::Base::Serialize( hq ).ToString() == base_bytes && destination.Snapshot() == snapshot,
				"world restore lost canonical HQ references or exact continuation" ); GT_OK();
		} ); if ( !restored.empty() ) { return restored; }
		std::string continued;
		for ( auto* fixture : { &source, &destination } ) {
			const auto error = fixture->Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture->Game(); game->AdvanceTurn( 2 ); auto* unit = game->GetUM()->GetUnit( 71 ); auto* target = game->GetMap()->GetTile( 3, 3 );
				GT_ASSERT( unit->Conventional().spent == 0 && unit->Conventional().failed_move, "upkeep lost saved HQ failure history" );
				fixture->MoveEvent( GSE_CALL, unit, target );
				GT_ASSERT( unit->GetTile() == target && unit->Conventional().spent == 3 && !unit->Conventional().failed_move
					&& base::Base::Serialize( game->GetBM()->GetBase( 31 ) ).ToString() == base_bytes, "continued HQ command changed base or failure history" );
				if ( continued.empty() ) { continued = fixture->Snapshot(); }
				else { GT_ASSERT( fixture->Snapshot() == continued, "restored HQ command diverged from uninterrupted whole world" ); }
				GT_OK();
			} ); if ( !error.empty() ) { return error; }
		}
		GT_OK();
	} );
	task->AddTest( "conventional movement real event commits terrain road tube river and wrap edges", []( GSE*, context::Context* ) -> std::string {
		using namespace map::tile;
		const auto initial_id = Unit::GetNextId(); util::FinallyGuard restore_id( [ = ] { Unit::SetNextId( initial_id ); } );
		for ( int scenario = 0; scenario < 7; ++scenario ) {
			ConventionalUnitFixture fixture; fixture.Configure( true, scenario == 6 ? 100 : 3, true );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); Land( game ); Define( game->GetUM() ); fixture.RetainedPlanet( 0, 9 );
				auto record = Record(); if ( scenario == 6 ) { record.spent = 99; }
				auto* unit = Spawn( GSE_CALL, fixture, record ); auto* source = unit->GetTile(); auto* destination = source->E;
				int expected_cost = 3;
				if ( scenario == 1 || scenario == 6 ) { source->terraforming = destination->terraforming = TERRAFORMING_ROAD; expected_cost = 1; }
				if ( scenario == 2 ) { source->terraforming = destination->terraforming = TERRAFORMING_MAG_TUBE; expected_cost = 0; }
				if ( scenario == 3 ) { destination->rockiness = ROCKINESS_ROCKY; destination->terraforming = TERRAFORMING_FOREST; expected_cost = 9; }
				if ( scenario == 4 ) { destination = source->NE; source->features = destination->features = FEATURE_RIVER; expected_cost = 1; }
				if ( scenario == 5 ) { unit->SetTile( GSE_CALL, game->GetMap()->GetTile( 0, 2 ) ); source = unit->GetTile(); destination = source->W; }
				fixture.GameplayRandom( { 0, UINT32_MAX } ); fixture.Updates();
				const auto jsf = game->GetRandom()->GetStateString(); const auto next = Unit::GetNextId();
				fixture.MoveEvent( GSE_CALL, unit, destination );
				GT_ASSERT( unit->GetTile() == destination && !source->units.count( unit->m_id ) && destination->units.at( unit->m_id ) == unit,
					"actual event did not atomically relocate membership at terrain scenario " + std::to_string( scenario ) );
				GT_ASSERT( unit->Conventional().spent == record.spent + expected_cost && !unit->Conventional().failed_move
					&& !unit->m_animation_id && Unit::GetNextId() == next && fixture.GameplayRandom() == SmacGameplayRandomState( 0, UINT32_MAX )
					&& game->GetRandom()->GetStateString() == jsf, "deterministic edge spent wrong amount or touched RNG/animation/ID" );
				const auto& updates = fixture.Updates();
				const auto found = std::find_if( updates.begin(), updates.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_UPDATE; } );
				GT_ASSERT( found != updates.end() && std::count_if( updates.begin(), updates.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_UPDATE; } ) == 1
					&& std::none_of( updates.begin(), updates.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_MOVE; } ), "move used animation or omitted committed refresh" );
				const auto& update = found->data.unit_update;
				GT_ASSERT( update.tile_coords.x == destination->coord.x && update.tile_coords.y == destination->coord.y
					&& update.conventional && update.has_moves == ( unit->Conventional().spent < unit->QueryStats( SmacMoraleMode::Mode1 ).full_allowance ),
					"refresh did not publish integer-authoritative actionability and position" ); GT_OK();
			} ); if ( !error.empty() ) { return error; }
		}
		GT_OK();
	} );
	task->AddTest( "conventional HQ fungus lottery and upkeep preserve exact paired RNG and failure marker", []( GSE*, context::Context* ) -> std::string {
		using namespace map::tile;
		const auto initial_id = Unit::GetNextId(), base_id = base::Base::GetNextId();
		util::FinallyGuard restore_id( [ = ] { Unit::SetNextId( initial_id ); base::Base::SetNextId( base_id ); } );
		for ( int scenario = 0; scenario < 3; ++scenario ) {
			ConventionalUnitFixture fixture; fixture.Configure( true, 3, true );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); Land( game ); Define( game->GetUM() ); fixture.RetainedPlanet( scenario == 2 ? 1 : 0, scenario == 2 ? -9 : 9 );
				auto record = Record(); auto* unit = Spawn( GSE_CALL, fixture, record ); auto* source = unit->GetTile(); auto* destination = source->E;
				HQ( GSE_CALL, game, unit, source ); destination->features = FEATURE_XENOFUNGUS;
				const uint32_t seed = SeedForBin( scenario == 0 ? 0 : 8 ); fixture.GameplayRandom( { seed, 0xabcdef01 } ); fixture.Updates();
				const auto jsf = game->GetRandom()->GetStateString(); fixture.MoveEvent( GSE_CALL, unit, destination );
				const bool failed = scenario == 1;
				GT_ASSERT( unit->GetTile() == ( failed ? source : destination ) && unit->Conventional().failed_move == failed
					&& unit->Conventional().spent == ( scenario == 0 ? 9 : 3 ) && unit->Conventional().damage == 0,
					"fungus result did not use retained PLANET or the admitted lottery" );
				GT_ASSERT( fixture.GameplayRandom() == SmacGameplayRandomState( scenario == 2 ? seed : SharedNext( seed ), 0xabcdef01 )
					&& game->GetRandom()->GetStateString() == jsf, "lottery advanced the wrong stream or wrong draw count" );
				if ( failed ) {
					game->AdvanceTurn( 2 );
					GT_ASSERT( unit->Conventional().spent == 0 && unit->Conventional().failed_move && unit->Conventional().damage == 0,
						"genuine upkeep erased retry marker or healed stored damage" );
					const auto retry_seed = SeedForBin( 4 ); fixture.GameplayRandom( { retry_seed, 7 } );
					fixture.MoveEvent( GSE_CALL, unit, destination );
					GT_ASSERT( unit->GetTile() == destination && unit->Conventional().spent == 9 && !unit->Conventional().failed_move
						&& fixture.GameplayRandom() == SmacGameplayRandomState( SharedNext( retry_seed ), 7 ), "retry omitted Q bonus or spent a second draw" );
				}
				GT_OK();
			} ); if ( !error.empty() ) { return error; }
		}
		GT_OK();
	} );
	task->AddTest( "conventional destination fungus without HQ rejects atomically while ordinary exit remains", []( GSE*, context::Context* ) -> std::string {
		using namespace map::tile;
		const auto initial_id = Unit::GetNextId(); util::FinallyGuard restore_id( [ = ] { Unit::SetNextId( initial_id ); } );
		for ( int scenario = 0; scenario < 4; ++scenario ) {
			ConventionalUnitFixture fixture; fixture.Configure( true, 3, true );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); auto* manager = game->GetUM(); Land( game ); Define( manager );
				fixture.RetainedPlanet( scenario == 1 ? 1 : 0, 9 ); auto record = Record();
				if ( scenario == 2 ) { record.spent = 1; record.failed_move = true; }
				auto* unit = Spawn( GSE_CALL, fixture, record ); auto* source = unit->GetTile(); auto* destination = source->E;
				destination->features = FEATURE_XENOFUNGUS; if ( scenario == 3 ) { source->features = FEATURE_XENOFUNGUS; }
				fixture.GameplayRandom( { 0, 0xdeadbeef } ); fixture.Updates();
				const auto snapshot = fixture.Snapshot(); const auto members = source->units; const auto next = Unit::GetNextId();
				std::string rejection; try { manager->ValidateConventionalMove( unit, destination, 0 ); } catch ( const std::exception& e ) { rejection = e.what(); }
				GT_ASSERT( rejection.find( "nearest-base" ) != std::string::npos, "base-free fungus lacked explicit caller-context rejection at " + std::to_string( scenario ) );
				fixture.MoveEvent( GSE_CALL, unit, destination );
				GT_ASSERT( fixture.Snapshot() == snapshot && unit->Conventional() == record && unit->GetTile() == source && source->units == members
					&& destination->units.empty() && Unit::GetNextId() == next, "unsupported fungus changed saved authority or membership" );
				const auto& updates = fixture.Updates();
				GT_ASSERT( std::none_of( updates.begin(), updates.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_UPDATE || r.type == game::FrontendRequest::FR_UNIT_MOVE; } ),
					"unsupported fungus published unit effects" );
				// The encounter prelude tests destination fungus. A source-only fungus
				// flag does not require that unimplemented native-life context.
				source->features = FEATURE_XENOFUNGUS; destination->features = FEATURE_NONE;
				manager->UpdateConventionalUnit( GSE_CALL, unit, Record() ); fixture.Updates();
				fixture.MoveEvent( GSE_CALL, unit, destination );
				GT_ASSERT( unit->GetTile() == destination && unit->Conventional().spent == 3 && !unit->Conventional().failed_move
					&& fixture.GameplayRandom() == SmacGameplayRandomState( 0, 0xdeadbeef ), "ordinary exit incorrectly inherited destination-fungus fence" ); GT_OK();
			} ); if ( !error.empty() ) { return error; }
		}
		GT_OK();
	} );
	task->AddTest( "conventional movement rejects unsupported commands before any authority or RNG effects", []( GSE*, context::Context* ) -> std::string {
		using namespace map::tile;
		const auto initial_id = Unit::GetNextId(); util::FinallyGuard restore_id( [ = ] { Unit::SetNextId( initial_id ); } );
		for ( const int scenario : { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 18, 19 } ) {
			ConventionalUnitFixture fixture; fixture.Configure( true, scenario == 14 ? 100 : 3, true );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); auto* manager = game->GetUM(); Land( game ); Define( manager ); fixture.RetainedPlanet( 0, 0 );
				auto record = Record(); if ( scenario == 2 ) { record.spent = 3; }
				auto* unit = Spawn( GSE_CALL, fixture, record ); auto* source = unit->GetTile(); auto* destination = source->E; size_t caller = 0;
				if ( scenario == 0 ) { fixture.MovementCapability( faction::SmacSimulationState::MovementCapability::Unavailable ); }
				if ( scenario == 1 ) { caller = 6; }
				if ( scenario == 2 ) { source->terraforming = destination->terraforming = TERRAFORMING_MAG_TUBE; }
				if ( scenario == 3 ) { for ( auto* c : destination->elevation.corners ) { *c = -100; } destination->Update(); }
				if ( scenario == 4 ) { destination->rockiness = ROCKINESS_NONE; }
				if ( scenario == 5 ) { destination->features = FEATURE_UNITY_POD; }
				if ( scenario == 6 ) { destination->terraforming = TERRAFORMING_FARM; }
				if ( scenario == 7 ) { auto* other = Spawn( GSE_CALL, fixture, record, 72 ); other->SetTile( GSE_CALL, destination ); }
				if ( scenario == 8 ) { unit->m_animation_id = 99; }
				if ( scenario == 9 ) { destination->Lock( 0 ); }
				if ( scenario == 10 ) { destination = source; }
				if ( scenario == 11 ) { destination = game->GetMap()->GetTile( 6, 6 ); }
				if ( scenario == 12 ) { game->GetState()->m_settings.local.game_mode = settings::LocalSettings::GM_MULTIPLAYER; }
				if ( scenario == 13 ) { game->GetState()->m_settings.local.network_role = settings::LocalSettings::NR_CLIENT; }
				if ( scenario == 14 ) { destination->rockiness = ROCKINESS_ROCKY; destination->terraforming = TERRAFORMING_FOREST; }
				const auto base_next = base::Base::GetNextId(); util::FinallyGuard restore_base_id( [ = ] { base::Base::SetNextId( base_next ); } );
				if ( scenario == 19 ) {
					game->GetBM()->SpawnBase( GSE_CALL, new base::Base( game, 31, unit->m_owner, game->GetFaction( "GAIANS" ), game->GetMap()->GetTile( 6, 6 ), "Admission fence", {}, {} ) );
				}
				if ( scenario == 18 ) { fixture.MovementCapability( static_cast< faction::SmacSimulationState::MovementCapability >( 2 ) ); }
				if ( scenario == 19 ) { destination->features = FEATURE_XENOFUNGUS; }
				fixture.GameplayRandom( { 0xdeadbeef, 0x12345678 } ); fixture.Updates();
				const auto before = unit->Conventional(); const auto source_members = source->units, destination_members = destination->units;
				const auto random = fixture.GameplayRandom(); const auto jsf = game->GetRandom()->GetStateString(); const auto next = Unit::GetNextId();
				std::string rejection;
				try { manager->ValidateConventionalMove( unit, destination, caller ); } catch ( const std::exception& error ) { rejection = error.what(); }
				GT_ASSERT( !rejection.empty(), "unsupported command was admitted at " + std::to_string( scenario ) );
				if ( scenario == 2 ) { GT_ASSERT( rejection.find( "exhausted" ) != std::string::npos, "zero-cost pricing preceded actionability" ); }
				fixture.MoveEvent( GSE_CALL, unit, destination, caller );
				GT_ASSERT( unit->Conventional() == before && unit->GetTile() == source && source->units == source_members && destination->units == destination_members
					&& fixture.GameplayRandom() == random && game->GetRandom()->GetStateString() == jsf && Unit::GetNextId() == next,
					"rejected event partially committed movement at " + std::to_string( scenario ) );
				const auto& updates = fixture.Updates();
				GT_ASSERT( std::none_of( updates.begin(), updates.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_UPDATE || r.type == game::FrontendRequest::FR_UNIT_MOVE; } ), "rejected move queued a unit effect" );
				unit->m_animation_id = 0; if ( destination->IsLocked() ) { destination->Unlock(); } GT_OK();
			} ); if ( !error.empty() ) { return error; }
		}
		GT_OK();
	} );
	task->AddTest( "conventional movement actual world restore continues failed attempt without reseeding or policy inference", []( GSE*, context::Context* ) -> std::string {
		using namespace map::tile;
		ConventionalUnitFixture source, destination; source.Configure( true, 3, true ); destination.Configure( true, 3, true );
		const auto initial_id = Unit::GetNextId(); util::FinallyGuard restore_id( [ = ] { Unit::SetNextId( initial_id ); } );
		std::string bytes; SmacGameplayRandomState saved( 0, 0 ); ConventionalUnitState record = Record();
		const auto setup = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = source.Game(); Land( game ); Define( game->GetUM() ); source.RetainedPlanet( 0, 9 );
			record.spent = 2; auto* unit = Spawn( GSE_CALL, source, record );
			source.GameplayRandom( { 1, 77 } ); source.MoveEvent( GSE_CALL, unit, unit->GetTile()->E );
			GT_ASSERT( unit->Conventional().failed_move && unit->Conventional().spent == 3, "ordinary partial command did not preserve a genuine failed attempt" );
			record = unit->Conventional(); saved = source.GameplayRandom(); bytes = source.Snapshot(); GT_OK();
		} ); if ( !setup.empty() ) { return setup; }
		const auto restored = destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			destination.MovementCapability( faction::SmacSimulationState::MovementCapability::Unavailable ); destination.GameplayRandom( { 1, 2 } );
			destination.Restore( GSE_CALL, bytes ); auto* unit = destination.Game()->GetUM()->GetUnit( 71 );
			GT_ASSERT( unit->Conventional() == record && destination.GameplayRandom() == saved && destination.Snapshot() == bytes,
				"world restoration changed continuation or inferred movement capability" ); GT_OK();
		} ); if ( !restored.empty() ) { return restored; }
		std::string continued;
		for ( auto* fixture : { &source, &destination } ) {
			const auto error = fixture->Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture->Game(); game->AdvanceTurn( 2 ); auto* unit = game->GetUM()->GetUnit( 71 );
				auto* target = unit->GetTile()->E; fixture->MoveEvent( GSE_CALL, unit, target );
				GT_ASSERT( unit->GetTile() == target && unit->Conventional().spent == 3 && !unit->Conventional().failed_move && fixture->GameplayRandom() == saved, "continued ordinary command changed saved stream or failed to move" );
				if ( continued.empty() ) { continued = fixture->Snapshot(); }
				else { GT_ASSERT( fixture->Snapshot() == continued, "restored command did not continue exact record tile and paired RNG" ); }
				GT_OK();
			} ); if ( !error.empty() ) { return error; }
		}
		const auto unavailable = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			source.MovementCapability( faction::SmacSimulationState::MovementCapability::Unavailable ); bytes = source.Snapshot(); GT_OK();
		} ); if ( !unavailable.empty() ) { return unavailable; }
		return destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			destination.Restore( GSE_CALL, bytes );
			GT_ASSERT( destination.Snapshot() == bytes, "Unavailable capability was replaced while restoring actual world" );
			auto* game = destination.Game(); auto* unit = game->GetUM()->GetUnit( 71 );
			std::string rejection; try { game->GetUM()->ValidateConventionalMove( unit, unit->GetTile()->E, 0 ); }
			catch ( const std::exception& error ) { rejection = error.what(); }
			GT_ASSERT( rejection.find( "campaign admission" ) != std::string::npos, "restored absence inferred no-project support" ); GT_OK();
		} );
	} );
	task->AddTest( "conventional unit record rejects malformed fields before publication", GT() {
		ConventionalUnitFixture fixture; fixture.Configure();
		const auto initial_id = Unit::GetNextId(); util::FinallyGuard restore_id( [ = ] { Unit::SetNextId( initial_id ); } );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* manager = game->GetUM(); Define( manager ); fixture.Updates();
			const auto next = Unit::GetNextId(); auto* tile = game->GetMap()->GetTile( 2, 2 );
			const ConventionalUnitResolver resolver = []( const auto& state, const auto* owner ) {
				if ( owner->GetIndex() != 0 || state.owner_simulation_id != 1 ) { throw std::runtime_error( "wrong owner" ); }
				return ConventionalUnitContext{ game::backend::settings::SmacMovementRules::GroundChassis::Infantry, 3, 3, -4, 1 };
			};
			const auto rejected = [ & ]( const std::string& bytes ) {
				types::Buffer input( bytes );
				try {
					auto* unexpected = Unit::Deserialize( GSE_CALL, input, manager, resolver );
					manager->SpawnUnit( GSE_CALL, unexpected ); // retain ownership even in a negative control
					return false;
				} catch ( const std::exception& ) { return true; }
			};
			const auto valid = WireRecord{}.Encode();
			for ( size_t size = 0; size < valid.size(); ++size ) {
				GT_ASSERT( rejected( valid.substr( 0, size ) ), "truncated record published a unit" );
			}
			GT_ASSERT( rejected( valid + "x" ), "trailing record bytes were ignored" );
			const std::array< std::function< void( WireRecord& ) >, 22 > malformed = {{
				[]( auto& r ) { r.version = 1; }, []( auto& r ) { r.kind = 2; }, []( auto& r ) { r.id = 0; },
				[]( auto& r ) { r.id = INT64_MAX; }, []( auto& r ) { r.definition = "missing"; },
				[]( auto& r ) { r.owner = 7; }, []( auto& r ) { r.x = -1; }, []( auto& r ) { r.y = 8; },
				[]( auto& r ) { r.x = 3; }, []( auto& r ) { r.design = -1; }, []( auto& r ) { r.design = 512; },
				[]( auto& r ) { r.simulation = 0; }, []( auto& r ) { r.simulation = 8; },
				[]( auto& r ) { r.morale = -1; }, []( auto& r ) { r.morale = 256; },
				[]( auto& r ) { r.damage = -1; }, []( auto& r ) { r.damage = 256; },
				[]( auto& r ) { r.spent = -1; }, []( auto& r ) { r.spent = 256; },
				[]( auto& r ) { r.home = 1; }, []( auto& r ) { r.mode = 2; }, []( auto& r ) { r.damage = 10; }
			}};
			for ( const auto& mutate : malformed ) { WireRecord record; mutate( record ); GT_ASSERT( rejected( record.Encode() ), "malformed field published a unit" ); }
			GT_ASSERT( manager->GetUnits().empty() && tile->units.empty() && Unit::GetNextId() == next && fixture.Updates().empty(),
				"rejected record changed canonical state or ID allocator" );
			types::Buffer input( valid ); auto* restored = Unit::Deserialize( GSE_CALL, input, manager, resolver ); manager->SpawnUnit( GSE_CALL, restored );
			GT_ASSERT( Unit::Serialize( restored ).ToString() == valid && restored->Conventional().stored_morale == 255
				&& restored->Conventional().spent == 255 && restored->Conventional().failed_move, "valid byte endpoints did not restore exactly" );
			manager->DespawnUnit( GSE_CALL, restored->m_id ); GT_OK();
		} );
	} );
	task->AddTest( "conventional unit actual spawn owns bytes and publishes fresh integer-derived stats", []( GSE* unused_gse, context::Context* unused_ctx ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		const auto next = Unit::GetNextId(); util::FinallyGuard restore_id( [ = ]() { Unit::SetNextId( next ); } );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* manager = game->GetUM(); auto* def = Define( manager );
			size_t hooks = 0; manager->On( GSE_CALL, "unit_spawn", NATIVE_CALL( &hooks ) { ++hooks; return VALUE( value::Undefined ); } ) );
			const auto random = game->GetRandom()->GetStateString();
			auto* unit = Spawn( GSE_CALL, fixture, Record() );
			GT_ASSERT( hooks == 1 && unit == manager->GetUnit( 71 ) && unit->m_def == def && unit->m_owner == &game->GetState()->m_slots->GetSlot( 0 )
				&& unit->GetTile()->units.at( 71 ) == unit && unit->Conventional() == Record(), "actual spawn lost canonical ownership or stored bytes" );
			auto* wrapper = static_cast< value::Object* >( unit->Wrap( GSE_CALL ) );
			auto* owned_state = static_cast< value::Object* >( Call( GSE_CALL, wrapper, "get_conventional_state" ) );
			owned_state->Assign( "spent", VALUE( value::Int,, 255 ) );
			GT_ASSERT( unit->Conventional() == Record(), "copied state exposed a mutable native scalar" );
			GT_ASSERT( wrapper->Get( "movement" )->type == VT_UNDEFINED && wrapper->Get( "health" )->type == VT_UNDEFINED, "conventional wrapper exposes mutable float authority" );
			auto* before = Call( GSE_CALL, wrapper, "query_conventional_stats", { VALUE( value::Int,, 0 ) } );
			GT_ASSERT( Int( before, "effective_morale" ) == 0 && Int( before, "full_allowance" ) == 3 && Int( before, "hit_points" ) == 10,
				"spawn ignored retained Gaia MORALE or used presentation Def movement" );
			const auto& spawned = fixture.Updates();
			const auto spawn = std::find_if( spawned.begin(), spawned.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_SPAWN; } );
			GT_ASSERT( spawn != spawned.end() && spawn->data.unit_spawn.movement == 1.0f && spawn->data.unit_spawn.morale == 0
				&& spawn->data.unit_spawn.health == 1.0f && spawn->data.unit_spawn.conventional && spawn->data.unit_spawn.has_moves
				&& spawn->data.unit_spawn.full_movement == 1.0f, "spawn FR did not derive exact stats" );
			fixture.RetainedMorale( 0, -4 );
			auto changed = Record(); changed.stored_morale = 6; changed.damage = 5; changed.spent = 2; changed.failed_move = true;
			manager->UpdateConventionalUnit( GSE_CALL, unit, changed );
			auto* after = Call( GSE_CALL, wrapper, "query_conventional_stats", { VALUE( value::Int,, 1 ) } );
			GT_ASSERT( Int( before, "effective_morale" ) == 0 && Int( after, "effective_morale" ) == 6 && Int( after, "full_allowance" ) == 3
				&& Int( after, "remaining_allowance" ) == 1 && Int( after, "hit_points" ) == 5, "retained wrapper query cached stats or used current social" );
			const auto& updates = fixture.Updates();
			const auto update = std::find_if( updates.begin(), updates.end(), []( const auto& r ) { return r.type == game::FrontendRequest::FR_UNIT_UPDATE; } );
			GT_ASSERT( update != updates.end() && update->data.unit_update.morale == 6 && *update->data.unit_update.morale_string == "level6"
				&& update->data.unit_update.movement == 1.0f / 3.0f && update->data.unit_update.health == 0.5f && update->data.unit_update.has_moves
				&& update->data.unit_update.full_movement == 1.0f, "refresh omitted fresh morale/allowance/health" );
			const game::FrontendRequest copy( *update );
			GT_ASSERT( copy.data.unit_update.morale_string != update->data.unit_update.morale_string && *copy.data.unit_update.morale_string == "level6", "FR morale copy borrowed text" );
			GT_ASSERT( unit->Conventional() == changed && game->GetRandom()->GetStateString() == random, "stat update changed stored record or RNG" );
			for ( const int scenario : { 0, 1, 2 } ) {
				auto invalid = changed;
				if ( scenario == 0 ) { invalid.damage = 10; } else if ( scenario == 1 ) { invalid.owner_simulation_id = 2; }
				else { invalid.render_morale_mode = SmacMoraleMode::Mode1; }
				bool rejected = false; try { manager->UpdateConventionalUnit( GSE_CALL, unit, invalid ); } catch ( const std::exception& ) { rejected = true; }
				GT_ASSERT( rejected && unit->Conventional() == changed && fixture.Updates().empty(), "invalid update partially changed authority or queued a refresh" );
			}
			GT_OK();
		} );
	} );
	task->AddTest( "conventional unit preflight rejects before IDs tiles hooks or frontend effects", []( GSE* unused_gse, context::Context* unused_ctx ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* manager = game->GetUM(); auto* def = Define( manager ); fixture.Updates();
			const auto next = Unit::GetNextId(), count = manager->GetUnits().size(), tiles = game->GetMap()->GetTile( 2, 2 )->units.size();
			const auto rng = game->GetRandom()->GetStateString(); size_t hooks = 0;
			manager->On( GSE_CALL, "unit_spawn", NATIVE_CALL( &hooks ) { ++hooks; return VALUE( value::Undefined ); } ) );
			for ( int scenario = 0; scenario < 9; ++scenario ) {
				auto record = Record(); auto* tile = game->GetMap()->GetTile( 2, 2 ); auto* presentation = def;
				if ( scenario == 0 ) { record.owner_simulation_id = 2; }
				if ( scenario == 1 ) { record.design_id = 0; }
				if ( scenario == 2 ) { record.design_id = 64; }
				if ( scenario == 3 ) { record.design_id = 11; } // measured Speeder, incompatible Infantry renderer
				if ( scenario == 4 ) { record.damage = 10; }
				if ( scenario == 5 ) { record.home = static_cast< ConventionalUnitState::Home >( 1 ); }
				if ( scenario == 6 ) { record.render_morale_mode = static_cast< SmacMoraleMode >( 2 ); }
				if ( scenario == 7 ) { presentation = nullptr; }
				if ( scenario == 8 ) { tile = nullptr; }
				bool rejected = false;
				try { manager->SpawnConventionalUnit( GSE_CALL, next, presentation, &game->GetState()->m_slots->GetSlot( 0 ), tile, record ); }
				catch ( const std::exception& ) { rejected = true; }
				GT_ASSERT( rejected && Unit::GetNextId() == next && manager->GetUnits().size() == count && game->GetMap()->GetTile( 2, 2 )->units.size() == tiles,
					"invalid preflight published a unit at scenario " + std::to_string( scenario ) );
			}
			fixture.Early(); bool rejected = false;
			try { Spawn( GSE_CALL, fixture, Record(), next ); } catch ( const std::exception& ) { rejected = true; }
			GT_ASSERT( rejected && hooks == 0 && Unit::GetNextId() == next && fixture.Updates().empty() && game->GetRandom()->GetStateString() == rng,
				"Early spawn invented query state or changed native state" ); GT_OK();
		} );
	} );
	task->AddTest( "conventional unit snapshots resolve private downloaded context and destination identities", []( GSE* unused_gse, context::Context* unused_ctx ) -> std::string {
		ConventionalUnitFixture source, destination; source.Configure(); destination.Configure();
		const auto next = Unit::GetNextId(); util::FinallyGuard restore_id( [ = ]() { Unit::SetNextId( next ); } );
		std::string bytes; ConventionalUnitState saved = Record(); saved.stored_morale = 255; saved.damage = 4; saved.spent = 5; saved.failed_move = true;
		const auto setup = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			Define( source.Game()->GetUM() ); source.RetainedMorale( -4, 4 ); Spawn( GSE_CALL, source, saved ); bytes = source.Snapshot(); GT_OK();
		} ); if ( !setup.empty() ) { return setup; }
		return destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = destination.Game(); size_t hooks = 0;
			game->GetUM()->On( GSE_CALL, "unit_spawn", NATIVE_CALL( &hooks ) { ++hooks; return VALUE( value::Undefined ); } ) );
			// Wrong current context is deliberately Early; only the staged decoded
			// Refreshed context can admit this unit during the actual body reader.
			destination.Early(); destination.Restore( GSE_CALL, bytes );
			auto* unit = game->GetUM()->GetUnit( 71 );
			GT_ASSERT( unit && unit->Conventional() == saved && unit->m_def == game->GetUM()->GetUnitDef( "EXPLICIT_SCOUT" )
				&& unit->m_owner == &game->GetState()->m_slots->GetSlot( 0 ) && unit->GetTile() == game->GetMap()->GetTile( 2, 2 )
				&& unit->GetTile()->units.at( 71 ) == unit && hooks == 0, "snapshot lost variant, destination identity, or replayed creation" );
			const auto stats = unit->QueryStats( SmacMoraleMode::Mode0 );
			GT_ASSERT( stats.effective_morale == 6 && stats.full_allowance == 6 && stats.remaining_allowance == 1 && stats.hit_points == 6,
				"restored stats were derived before saved context publication" );
			GT_ASSERT( destination.Snapshot() == bytes, "world body reserialization changed conventional stored bytes" ); GT_OK();
		} );
	} );
	task->AddTest( "conventional unit actual legacy handlers cannot spend heal animate or roll back bytes", []( GSE* unused_gse, context::Context* unused_ctx ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		const auto next = Unit::GetNextId(); util::FinallyGuard restore_id( [ = ]() { Unit::SetNextId( next ); } );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); Define( game->GetUM() ); auto record = Record(); record.damage = 4; record.spent = 2; record.failed_move = true;
			auto* unit = Spawn( GSE_CALL, fixture, record ); auto* wrapper = static_cast< value::Object* >( unit->Wrap( GSE_CALL ) );
			const auto rng = game->GetRandom()->GetStateString();
			auto* data = VALUEEXT( value::Object, GSE_CALL, ( value::object_properties_t{ { "unit", wrapper }, { "attacker", wrapper }, { "defender", wrapper },
				{ "tile", game->GetMap()->GetTile( 4, 2 )->Wrap( GSE_CALL ) } } ) );
			auto* event = VALUEEXT( value::Object, GSE_CALL, ( value::object_properties_t{ { "data", data }, { "caller", VALUE( value::Int,, 0 ) }, { "game", game->Wrap( GSE_CALL ) } } ) );
			for ( const auto* name : { "move_unit", "attack_unit", "despawn_unit", "unit_skip_turn" } ) {
				auto* handlers = static_cast< value::Object* >( fixture.Script( GSE_CALL, std::string( "./GLSMAC_data/default/game/event/" ) + name ) );
				const auto* result = static_cast< value::String* >( Call( GSE_CALL, handlers, "validate", { event } ) );
				GT_ASSERT( result->value.find( "Conventional" ) != std::string::npos, "actual event validation admitted conventional legacy action" );
				for ( const auto* stage : { "resolve", "apply", "rollback" } ) {
					// Rejected conventional despawn has no applied backup. Legacy
					// rollback reads that backup after its old wrapper was invalidated.
					if ( std::string( name ) == "despawn_unit" && std::string( stage ) == "rollback" ) { continue; }
					if ( handlers->Get( stage )->type == VT_UNDEFINED || ( std::string( name ) == "move_unit" && std::string( stage ) == "resolve" ) ) { continue; }
					bool rejected = false; try { Call( GSE_CALL, handlers, stage, { event } ); }
					catch ( const std::exception& error ) { rejected = std::string( error.what() ).find( "Conventional" ) != std::string::npos; }
					GT_ASSERT( rejected, "actual event body bypassed conventional guard" );
				}
			}
			bool rejected = false; try { wrapper->Set( "health", VALUE( value::Float,, 0.2f ), GSE_CALL ); } catch ( const std::exception& ) { rejected = true; }
			GT_ASSERT( rejected, "legacy setter overwrote conventional state" );
			auto* turns = static_cast< value::Object* >( fixture.Script( GSE_CALL, "./GLSMAC_data/default/units/turns" ) );
			Call( GSE_CALL, turns, "configure", { game->Wrap( GSE_CALL ) } );
			game->GetState()->TriggerObject( game->GetUM(), "unit_turn", ARGS_F( &wrapper ) { { "unit", wrapper } }; } );
			game->AdvanceTurn( 2 );
			record.spent = 0;
			GT_ASSERT( unit->Conventional() == record && unit->GetTile() == game->GetMap()->GetTile( 2, 2 ) && !unit->m_animation_id
				&& game->GetRandom()->GetStateString() == rng, "legacy handler or turn changed state, tile, animation or RNG" ); GT_OK();
		} );
	} );
	task->AddTest( "conventional unit fine quantum frontend actionability does not use float threshold", []( GSE* unused_gse, context::Context* unused_ctx ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure( true, 100 );
		const auto next = Unit::GetNextId(); util::FinallyGuard restore_id( [ = ]() { Unit::SetNextId( next ); } );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			Define( fixture.Game()->GetUM() ); auto record = Record(); record.spent = 99; auto* unit = Spawn( GSE_CALL, fixture, record );
			const auto& updates = fixture.Updates(); const auto& spawn = updates.front().data.unit_spawn;
			GT_ASSERT( updates.front().type == game::FrontendRequest::FR_UNIT_SPAWN && spawn.movement == 0.01f && spawn.has_moves && unit->HasMovesLeft(),
				"one quantum was lost to legacy float actionability" );
			record.spent = 100; fixture.Game()->GetUM()->UpdateConventionalUnit( GSE_CALL, unit, record );
			const auto& refreshed = fixture.Updates();
			GT_ASSERT( refreshed.front().type == game::FrontendRequest::FR_UNIT_UPDATE && !refreshed.front().data.unit_update.has_moves && !unit->HasMovesLeft(),
				"exhausted unit stayed actionable" ); GT_OK();
		} );
	} );
}
}

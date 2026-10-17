#include "ConventionalUnits.h"
#include "fixtures/FriendlyHQMovement.h"
#include "fixtures/SpartanHQMovement.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
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
		{ "home", VALUE( value::String,, "none" ) }, { "render_morale_mode", VALUE( value::Int,, static_cast< int >( state.render_morale_mode ) ) }
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

base::Base* HQ( GSE_CALLABLE, Game* game, Unit* unit, map::tile::Tile* tile, size_t id = 31 ) {
	auto* manager = game->GetBM();
	if ( !manager->GetFacilityDef( "HEADQUARTERS" ) ) {
		manager->DefineFacility( new base::FacilityDef( "HEADQUARTERS", "Fixture HQ", "Fixture", 1, { "fixture", 0, 0, 1, 1 } ) );
	}
	if ( !manager->GetPopDef( "HQ_WORKER" ) ) { manager->DefinePop( new base::PopDef( "HQ_WORKER", "Worker", {}, {}, base::PopDef::PF_NONE ) ); }
	auto* result = new base::Base( game, id, unit->m_owner, unit->m_owner->GetPlayer()->GetFaction(), tile, "HQ",
		{ { "HEADQUARTERS", manager->GetFacilityDef( "HEADQUARTERS" ) } }, { { 1, base::Pop( nullptr, 1, manager->GetPopDef( "HQ_WORKER" ) ) } }, 2 );
	manager->SpawnBase( GSE_CALL, result ); return result;
}

void OriginalFeatures( map::tile::Tile* tile, int flags ) {
	using namespace map::tile;
	if ( flags != 0 && flags != 4 && flags != 8 && flags != 32 ) { throw std::runtime_error( "unsupported original HQ fixture feature" ); }
	tile->features = flags == 32 ? FEATURE_XENOFUNGUS : FEATURE_NONE;
	tile->terraforming = flags == 4 ? TERRAFORMING_ROAD : flags == 8 ? TERRAFORMING_MAG_TUBE : TERRAFORMING_NONE;
}

uint32_t SharedNext( uint32_t value ) { return static_cast< uint32_t >( UINT64_C( 1664525 ) * value + UINT64_C( 1013904223 ) ); }
uint32_t SeedForBin( uint32_t bin ) {
	for ( uint32_t seed = 0; seed < 65536; ++seed ) { if ( uint64_t( SharedNext( seed ) & 65535 ) * 9 / 65536 == bin ) { return seed; } }
	throw std::runtime_error( "missing fixture lottery bin" );
}

int64_t Int( Value* value, const std::string& key ) { return static_cast< value::Int* >( static_cast< value::Object* >( value )->Get( key ) )->value; }

struct WireRecord {
	int64_t version = 2, kind = 1, id = 71, owner = 0, x = 2, y = 2;
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
				if ( scenario == 2 ) { fixture.RetainedPlanet( 1, 0 ); }
				if ( scenario == 3 ) { fixture.RetainedMorale( 0, 2 ); }
				if ( scenario == 5 ) { fixture.SelectStartup( 1, 2 ); }
				if ( scenario == 6 ) { fixture.CopyRoverDesign( 1, 65, 1 ); fixture.SelectStartup( 1, 65 ); }
				if ( scenario == 7 ) { auto* other = Spawn( GSE_CALL, fixture, record, 72, 0, "EXPLICIT_ROVER" ); other->SetTile( GSE_CALL, game->GetMap()->GetTile( 6, 6 ) ); fixture.Updates(); }
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
		for ( int scenario = 0; scenario < 22; ++scenario ) {
			ConventionalUnitFixture fixture; fixture.Configure( true, scenario == 4 ? 6 : 3, true );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); auto* manager = game->GetUM(); Land( game ); Define( manager );
				fixture.RetainedPlanet( scenario == 2 ? 1 : 0, 0 ); fixture.RetainedMorale( scenario == 3 ? 1 : 0, 0 );
				auto record = Record(); if ( scenario == 0 ) { record.damage = 1; } if ( scenario == 1 ) { record.stored_morale = 2; }
				auto* unit = Spawn( GSE_CALL, fixture, record ); auto* source = unit->GetTile(); auto* destination = game->GetMap()->GetTile( 3, 3 );
				auto* hq = HQ( GSE_CALL, game, unit, scenario == 10 ? game->GetMap()->GetTile( 6, 6 ) : source );
				fixture.Updates(); // retire valid spawn presentation before deliberate native corruption
				auto* original_owner = hq->m_owner; auto* original_faction = hq->m_faction;
				util::FinallyGuard restore_base( [ & ] { hq->m_owner = original_owner; hq->m_faction = original_faction; hq->GetTile()->base = hq; } );
				if ( scenario == 5 ) { hq->m_pops.clear(); }
				if ( scenario == 6 ) { hq->m_facilities.clear(); }
				if ( scenario == 7 ) { hq->m_facilities.emplace( "UNPROVED", game->GetBM()->GetFacilityDef( "HEADQUARTERS" ) ); }
				if ( scenario == 8 ) { hq->m_owner = &game->GetState()->m_slots->GetSlot( 6 ); }
				if ( scenario == 9 ) { hq->m_faction = nullptr; }
				if ( scenario == 11 ) { HQ( GSE_CALL, game, unit, game->GetMap()->GetTile( 6, 6 ), 32 ); fixture.Updates(); }
				if ( scenario == 12 ) { source->rockiness = ROCKINESS_ROLLING; }
				if ( scenario == 13 ) { destination->rockiness = ROCKINESS_ROLLING; }
				if ( scenario == 14 ) { source->features = FEATURE_XENOFUNGUS; }
				if ( scenario == 15 ) { destination->features = FEATURE_RIVER; }
				if ( scenario == 16 ) { destination->terraforming = TERRAFORMING_FOREST; }
				if ( scenario == 17 ) { source->terraforming = TERRAFORMING_ROAD; }
				if ( scenario == 18 ) { destination->features = FEATURE_XENOFUNGUS; destination->terraforming = TERRAFORMING_ROAD; }
				if ( scenario == 19 ) { source->base = nullptr; }
				if ( scenario == 20 ) { auto* other = Spawn( GSE_CALL, fixture, record, 72 ); other->SetTile( GSE_CALL, game->GetMap()->GetTile( 6, 6 ) ); fixture.Updates(); }
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
	task->AddTest( "conventional movement fungus lottery and upkeep preserve exact paired RNG and failure marker", []( GSE*, context::Context* ) -> std::string {
		using namespace map::tile;
		const auto initial_id = Unit::GetNextId(); util::FinallyGuard restore_id( [ = ] { Unit::SetNextId( initial_id ); } );
		for ( int scenario = 0; scenario < 3; ++scenario ) {
			ConventionalUnitFixture fixture; fixture.Configure( true, 3, true );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); Land( game ); Define( game->GetUM() ); fixture.RetainedPlanet( scenario == 2 ? 1 : 0, scenario == 2 ? -9 : 9 );
				auto record = Record(); record.damage = 2; auto* unit = Spawn( GSE_CALL, fixture, record ); auto* source = unit->GetTile(); auto* destination = source->E;
				destination->features = FEATURE_XENOFUNGUS;
				const uint32_t seed = SeedForBin( scenario == 0 ? 0 : 8 ); fixture.GameplayRandom( { seed, 0xabcdef01 } ); fixture.Updates();
				const auto jsf = game->GetRandom()->GetStateString(); fixture.MoveEvent( GSE_CALL, unit, destination );
				const bool failed = scenario == 1;
				GT_ASSERT( unit->GetTile() == ( failed ? source : destination ) && unit->Conventional().failed_move == failed
					&& unit->Conventional().spent == ( scenario == 0 ? 9 : 3 ) && unit->Conventional().damage == 2,
					"fungus result did not use retained PLANET or the admitted lottery" );
				GT_ASSERT( fixture.GameplayRandom() == SmacGameplayRandomState( scenario == 2 ? seed : SharedNext( seed ), 0xabcdef01 )
					&& game->GetRandom()->GetStateString() == jsf, "lottery advanced the wrong stream or wrong draw count" );
				if ( failed ) {
					game->AdvanceTurn( 2 );
					GT_ASSERT( unit->Conventional().spent == 0 && unit->Conventional().failed_move && unit->Conventional().damage == 2,
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
	task->AddTest( "conventional movement rejects unsupported commands before any authority or RNG effects", []( GSE*, context::Context* ) -> std::string {
		using namespace map::tile;
		const auto initial_id = Unit::GetNextId(); util::FinallyGuard restore_id( [ = ] { Unit::SetNextId( initial_id ); } );
		for ( int scenario = 0; scenario < 20; ++scenario ) {
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
				if ( ( scenario >= 15 && scenario <= 17 ) || scenario == 19 ) {
					base::Base::facilities_t facilities;
					if ( scenario == 17 ) {
						auto* facility = new base::FacilityDef( "HEADQUARTERS", "Headquarters", "Fixture", 1, { "fixture", 0, 0, 1, 1 } );
						game->GetBM()->DefineFacility( facility ); facilities.emplace( facility->m_id, facility );
					}
					auto* site = scenario == 15 ? source : scenario == 16 ? destination : game->GetMap()->GetTile( 6, 6 );
					game->GetBM()->SpawnBase( GSE_CALL, new base::Base( game, 31, unit->m_owner, game->GetFaction( "GAIANS" ), site, "Admission fence", facilities, {} ) );
				}
				if ( scenario == 18 ) { fixture.MovementCapability( static_cast< faction::SmacSimulationState::MovementCapability >( 2 ) ); }
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
			auto* unit = Spawn( GSE_CALL, source, record ); unit->GetTile()->E->features = FEATURE_XENOFUNGUS;
			source.GameplayRandom( { SeedForBin( 8 ), 77 } ); source.MoveEvent( GSE_CALL, unit, unit->GetTile()->E );
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
				fixture->MoveEvent( GSE_CALL, unit, unit->GetTile()->E );
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

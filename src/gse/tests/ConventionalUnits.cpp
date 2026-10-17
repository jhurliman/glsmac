#include "ConventionalUnits.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/Random.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/map/Map.h"
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

Def* Define( UnitManager* manager ) {
	MoraleSet::morale_values_t names;
	for ( int i = 0; i < 7; ++i ) { names.emplace_back( "level" + std::to_string( i ) ); }
	manager->DefineMoraleSet( new MoraleSet( "CONVENTIONAL_MORALE", names ) );
	auto* def = new StaticDef( "EXPLICIT_SCOUT", manager->GetMoraleSet( "CONVENTIONAL_MORALE" ), "Fixture Scout", MT_LAND, 999.0f,
		new CvrScoutRender( 0, 0, 100, 100, { 1.0f, 1.0f }, { 128.0f, 128.0f } ) );
	manager->DefineUnit( def ); return def;
}

Value* Call( GSE_CALLABLE, value::Object* object, const std::string& method, const value::function_arguments_t& arguments = {} ) {
	return static_cast< value::Callable* >( object->Get( method ) )->Run( GSE_CALL, arguments );
}

Unit* Spawn( GSE_CALLABLE, ConventionalUnitFixture& fixture, const ConventionalUnitState& state, size_t id = 71 ) {
	auto* game = fixture.Game();
	auto* data = VALUEEXT( value::Object, GSE_CALL, ( value::object_properties_t{
		{ "id", VALUE( value::Int,, id ) }, { "def", VALUE( value::String,, "EXPLICIT_SCOUT" ) },
		{ "owner", game->GetState()->m_slots->GetSlot( 0 ).GetPlayer()->Wrap( GSE_CALL ) },
		{ "tile", game->GetMap()->GetTile( 2, 2 )->Wrap( GSE_CALL ) },
		{ "design_id", VALUE( value::Int,, state.design_id ) }, { "owner_simulation_id", VALUE( value::Int,, state.owner_simulation_id ) },
		{ "stored_morale", VALUE( value::Int,, state.stored_morale ) }, { "damage", VALUE( value::Int,, state.damage ) },
		{ "spent", VALUE( value::Int,, state.spent ) }, { "failed_move", VALUE( value::Bool,, state.failed_move ) },
		{ "home", VALUE( value::String,, "none" ) }, { "render_morale_mode", VALUE( value::Int,, static_cast< int >( state.render_morale_mode ) ) }
	} ) );
	const auto* result = static_cast< value::Object* >( Call( GSE_CALL, static_cast< value::Object* >( game->GetUM()->Wrap( GSE_CALL ) ), "spawn_conventional_unit", { data } ) );
	return static_cast< Unit* >( result->wrapobj );
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
					if ( handlers->Get( stage )->type == VT_UNDEFINED ) { continue; }
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

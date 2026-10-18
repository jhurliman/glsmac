#include "Unit.h"

#include "gse/context/Context.h"
#include "gse/value/Object.h"
#include "gse/value/Int.h"
#include "gse/value/Float.h"
#include "gse/value/Bool.h"
#include "gse/value/String.h"
#include "gse/value/Ptr.h"
#include "gse/value/Array.h"
#include "gse/callable/Native.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/slot/Slot.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/map/Map.h"
#include "MoraleSet.h"
#include "StaticDef.h"
#include "UnitManager.h"
#include "gse/ExecutionPointer.h"
#include <cmath>
#include <limits>

namespace game {
namespace backend {
namespace unit {

static size_t next_id = 1;
const size_t Unit::GetNextId() {
	return next_id;
}
const void Unit::SetNextId( const size_t id ) {
	next_id = id;
}

Unit::Unit(
	GSE_CALLABLE,
	UnitManager* um,
	const size_t id,
	Def* def,
	slot::Slot* owner,
	map::tile::Tile* tile,
	const movement_t movement,
	const morale_t morale,
	const health_t health,
	const bool moved_this_turn,
	const bool record_placement
)
	: MapObject( um->GetMap(), nullptr )
	, m_id( id )
	, m_def( def )
	, m_owner( owner )
	, m_state( LegacyState{ movement, morale, health, moved_this_turn } )
	, m_um( um ) {
	Publish( GSE_CALL, tile, record_placement );
}

Unit::Unit( GSE_CALLABLE, UnitManager* um, size_t id, Def* def, slot::Slot* owner, map::tile::Tile* tile, ConventionalUnitState state, bool early_startup, bool record_placement, bool pending_home )
	: MapObject( um->GetMap(), nullptr ), m_id( id ), m_def( def ), m_owner( owner ), m_state( state ), m_um( um ), m_early_startup( early_startup ), m_pending_home( pending_home ) {
	// The manager has already resolved all context and presentation admission.
	Publish( GSE_CALL, tile, record_placement );
}

Unit::Unit( Unplaced, UnitManager* um, size_t id, Def* def, slot::Slot* owner, ConventionalUnitState state )
	: MapObject(um->GetMap(),nullptr), m_id(id), m_def(def), m_owner(owner), m_state(state), m_um(um) {}
Unit::Unit( Unplaced, UnitManager* um, size_t id, Def* def, map::tile::Tile* tile, SmacNativeUnitRecord state )
    : MapObject(um->GetMap(),tile), m_id(id), m_def(def), m_owner(nullptr), m_state(state), m_um(um) {}
Unit::Unit( GSE_CALLABLE, UnitManager* um, size_t id, Def* def, map::tile::Tile* tile, SmacNativeUnitRecord state )
    : MapObject(um->GetMap(),nullptr), m_id(id), m_def(def), m_owner(nullptr), m_state(state), m_um(um) {
    Publish(GSE_CALL,tile,false);
}
void Unit::CommitInitialPlacement( map::tile::Tile* tile ) noexcept {
	m_tile=tile; if ( next_id<=m_id ) { next_id=m_id+1; }
}

void Unit::Publish( GSE_CALLABLE, map::tile::Tile* tile, bool record_placement ) {
	if ( m_id == 0 || m_id >= static_cast< uint64_t >( std::numeric_limits< int64_t >::max() ) ||
		m_id >= std::numeric_limits< size_t >::max() || m_um->HasUnitId( m_id ) || !tile || tile->units.count( m_id ) ) {
		THROW( "invalid or duplicate unit ID" );
	}
	if ( !m_def || m_def != m_um->GetUnitDef( m_def->m_id ) ) {
		THROW( "unit definition is not owned by its manager" );
	}
	Place( GSE_CALL, tile, record_placement );
	if ( next_id <= m_id ) {
		next_id = m_id + 1;
	}
}

bool Unit::IsNative() const { return std::holds_alternative<SmacNativeUnitRecord>(m_state); }
const SmacNativeUnitRecord& Unit::Native() const {
    if(!IsNative()) { THROW("Unit is not native"); }
    return std::get<SmacNativeUnitRecord>(m_state);
}
int32_t Unit::SimulationOwner() const {
    return IsNative() ? Native().owner : Conventional().owner_simulation_id;
}
bool Unit::IsConventional() const { return std::holds_alternative< ConventionalUnitState >( m_state ); }
bool Unit::IsConventionalPending() const { return m_early_startup; }
void Unit::RequireConventionalReady() const {
	if ( m_early_startup ) { THROW( "Conventional startup unit has no derived state before first-turn seal" ); }
	if ( m_pending_home ) { THROW( "Conventional home awaits complete Base restore" ); }
}
Unit::LegacyState& Unit::Legacy() {
	if ( IsConventional() || IsNative() ) { THROW( "Canonical units do not have mutable legacy float state" ); }
	return std::get< LegacyState >( m_state );
}
const Unit::LegacyState& Unit::Legacy() const {
	if ( IsConventional() || IsNative() ) { THROW( "Canonical units do not have legacy float state" ); }
	return std::get< LegacyState >( m_state );
}
const ConventionalUnitState& Unit::Conventional() const {
	if ( !IsConventional() ) { THROW( "Unit is not conventional" ); }
	return std::get< ConventionalUnitState >( m_state );
}
ConventionalUnitStats Unit::QueryStats( const SmacMoraleMode mode ) const {
	RequireConventionalReady();
	const auto& state = Conventional();
	return QueryConventionalUnit( state, m_um->ResolveConventional( state, m_owner ), mode );
}
Unit::Presentation Unit::GetPresentation() const {
    if(IsNative()) {
        const auto stats=m_um->QueryNativeUnit(Native());
        return {float(stats.remaining_allowance)/stats.quantum,static_cast<morale_t>(stats.level),
            float(stats.hit_points)/stats.maximum_hit_points,float(stats.full_allowance)/stats.quantum};
    }
	if ( !IsConventional() ) { const auto& state = Legacy(); return { state.movement, state.morale, state.health }; }
	const auto stats = QueryStats( Conventional().render_morale_mode );
	return { static_cast< float >( stats.remaining_allowance ) / stats.quantum, stats.effective_morale,
		static_cast< float >( stats.hit_points ) / stats.maximum_hit_points, static_cast< float >( stats.full_allowance ) / stats.quantum };
}

const movement_t Unit::MINIMUM_MOVEMENT_TO_KEEP = 0.025f;
const movement_t Unit::MINIMUM_HEALTH_TO_KEEP = 0.025f;

const bool Unit::HasMovesLeft() const {
    if(IsNative()) {return m_um->QueryNativeUnit(Native()).remaining_allowance>0;}
	return IsConventional() ? QueryStats( SmacMoraleMode::Mode1 ).remaining_allowance > 0 : Legacy().movement >= unit::Unit::MINIMUM_MOVEMENT_TO_KEEP;
}

const std::string& Unit::GetMoraleString() const {
	return m_def->m_moraleset->m_morale_values.at( GetPresentation().morale ).m_name;
}

void Unit::CommitConventionalMove( const ConventionalUnitState& state, map::tile::Tile* tile ) noexcept {
	std::get< ConventionalUnitState >( m_state ) = state;
	m_tile = tile;
}

void Unit::SetTile( GSE_CALLABLE, map::tile::Tile* tile ) {
	Place( GSE_CALL, tile, true );
}

void Unit::Place( GSE_CALLABLE, map::tile::Tile* tile, bool record_placement ) {
	// Decoded snapshot records alone suppress placement; script rollback uses
	// the ordinary constructor and must not bypass supplied knowledge fences.
	if ( record_placement ) { m_um->m_game->RequireWorkerKnowledgeMutation(); }
	const auto placement = record_placement ? m_um->PreparePlacementHistory( *this, tile ) : std::pair< SmacMovementHistory*, size_t >{ nullptr, 0 };
	if ( m_tile ) {
		m_tile->units.erase( m_id );
	}
	ASSERT( tile->units.find( m_id ) == tile->units.end(), "duplicate unit id in tile" );
	tile->units.insert(
		{
			m_id,
			this
		}
	);
	m_tile = tile;
	if ( placement.first ) { placement.first->MarkPlaced( placement.second ); }
}

const types::Buffer Unit::Serialize( const Unit* unit ) {
	if ( unit->IsConventional() ) { unit->QueryStats( unit->Conventional().render_morale_mode ); }
	types::Buffer buf;
	buf.WriteInt( unit->IsNative() ? 5 : 4 );
	buf.WriteInt( unit->IsNative() ? 2 : unit->IsConventional() ? 1 : 0 );
	buf.WriteInt( unit->m_id );
	buf.WriteString( unit->m_def->m_id );
	buf.WriteInt( unit->IsNative() ? -1 : static_cast<int64_t>(unit->m_owner->GetIndex()) );
	buf.WriteInt( unit->m_tile->coord.x );
	buf.WriteInt( unit->m_tile->coord.y );
	if ( unit->IsNative() ) { buf.WriteString(unit->Native().Serialize().ToString()); }
	else if ( unit->IsConventional() ) { unit->Conventional().Serialize( buf ); }
	else {
		const auto& state = unit->Legacy();
		buf.WriteFloat( state.movement ); buf.WriteInt( state.morale );
		buf.WriteFloat( state.health ); buf.WriteBool( state.moved_this_turn );
	}
	return buf;
}

Unit* Unit::Deserialize( GSE_CALLABLE, types::Buffer& buf, UnitManager* um, const ConventionalUnitResolver& resolver, bool defer_home ) {
	if ( !um || !um->GetMap() ) { THROW( "unit snapshot requires a manager and map" ); }
	const auto version=buf.ReadInt();
	if ( version!=4 && version!=5 ) { THROW( "unsupported unit record version" ); }
	const auto kind = buf.ReadInt();
	if ( kind != 0 && kind != 1 && !(kind==2 && version==5) ) { THROW( "unsupported unit record kind" ); }
	const auto id = buf.ReadInt();
	const auto def_id = buf.ReadString();
	const auto owner = buf.ReadInt();
	const auto pos_x = buf.ReadInt();
	const auto pos_y = buf.ReadInt();
	std::optional< ConventionalUnitState > conventional;
	std::optional< SmacNativeUnitRecord > native;
	LegacyState legacy{};
	if ( kind == 2 ) {native=SmacNativeUnitRecord::Deserialize(buf.ReadString());}
	else if ( kind == 1 ) { conventional = ConventionalUnitState::Deserialize( buf ); }
	else {
		legacy.movement = static_cast< movement_t >( buf.ReadFloat() ); legacy.morale = buf.ReadInt();
		legacy.health = static_cast< health_t >( buf.ReadFloat() ); legacy.moved_this_turn = buf.ReadBool();
	}
	// Decode and validate the entire record before claiming a tile or advancing
	// the ID allocator. Finite signed values are intentional: current setters
	// permit them, and combat can leave negative health until its death callback.
	if ( buf.lenr != buf.lenw || id <= 0 || static_cast< uint64_t >( id ) >= std::numeric_limits< size_t >::max() ) {
		THROW( "invalid unit record or ID" );
	}
	auto* def = um->GetUnitDef( def_id );
	if ( !def ) { THROW( "unit definition not found: " + def_id ); }
	if ( native ? owner!=-1 : (owner < 0 || static_cast< uint64_t >( owner ) >= um->m_game->GetState()->m_slots->GetCount()) ) {
		THROW( "invalid unit owner" );
	}
	if ( pos_x < 0 || pos_y < 0 || static_cast< uint64_t >( pos_x ) >= um->GetMap()->GetWidth() ||
		static_cast< uint64_t >( pos_y ) >= um->GetMap()->GetHeight() || ( pos_x & 1 ) != ( pos_y & 1 ) ) {
		THROW( "invalid unit coordinates" );
	}
	auto* tile=um->GetMap()->GetTile(pos_x,pos_y);
    if(native) {
        um->ValidateNativeCreation(static_cast<size_t>(id),def,tile,*native);
        return new Unit(GSE_CALL,um,static_cast<size_t>(id),def,tile,*native);
    }
    auto* slot=um->GetSlot(owner);
	if ( conventional ) {
		um->ValidateConventionalCreation( static_cast< size_t >( id ), def, slot, tile, *conventional, resolver, false, defer_home );
		return new Unit( GSE_CALL, um, static_cast< size_t >( id ), def, slot, tile, *conventional, false, false, defer_home && conventional->home == ConventionalUnitState::Home::Base );
	}
	if ( legacy.morale < MORALE_MIN || legacy.morale > MORALE_MAX || !std::isfinite( legacy.movement ) || !std::isfinite( legacy.health ) ) {
		THROW( "invalid unit morale, movement or health" );
	}
	return new Unit( GSE_CALL, um, static_cast< size_t >( id ), def, slot, tile,
		legacy.movement, legacy.morale, legacy.health, legacy.moved_this_turn, false );
}

WRAPIMPL_SERIALIZE( Unit )
	buf->WriteInt( obj->m_id );
}

WRAPIMPL_DESERIALIZE( Unit )
	const auto id = buf->ReadInt();
	const auto& unit = game->GetUM()->GetUnit( id );
	ASSERT( unit, "base id not found: " + std::to_string( id ) );
	return unit->Wrap( GSE_CALL );
}

WRAPIMPL_DYNAMIC_GETTERS( Unit )
	WRAPIMPL_GET_CUSTOM( "is_conventional", Bool, IsConventional() )
	WRAPIMPL_GET_CUSTOM( "is_native", Bool, IsNative() )
	WRAPIMPL_GET_CUSTOM( "id", Int, m_id )
	WRAPIMPL_GET_CUSTOM( "def", String, m_def->m_id )
	{ "owner", IsNative() ? static_cast<gse::Value*>(VALUE(gse::value::Undefined)) : VALUE(gse::value::Int,,m_owner->GetIndex()) },
	WRAPIMPL_GET_CUSTOM( "tile", Object, GSE_CALL_NOGC, {
		{ "x", VALUE( gse::value::Int,, m_tile->coord.x ) },
		{ "y", VALUE( gse::value::Int,, m_tile->coord.y ) },
	} )
	// Legacy Ptr behavior is unchanged. Conventional callers must request a
	// fresh copied query; retaining a wrapper must not cache derived floats.
	{ "movement", (IsConventional() || IsNative()) ? static_cast< gse::Value* >( VALUE( gse::value::Undefined ) ) : VALUE( gse::value::Ptr,, Legacy().movement ) },
	{ "morale", (IsConventional() || IsNative()) ? static_cast< gse::Value* >( VALUE( gse::value::Undefined ) ) : VALUE( gse::value::Ptr,, Legacy().morale ) },
	{ "health", (IsConventional() || IsNative()) ? static_cast< gse::Value* >( VALUE( gse::value::Undefined ) ) : VALUE( gse::value::Ptr,, Legacy().health ) },
	{ "moved_this_turn", (IsConventional() || IsNative()) ? static_cast< gse::Value* >( VALUE( gse::value::Undefined ) ) : VALUE( gse::value::Ptr,, Legacy().moved_this_turn ) },
	{ "get_conventional_state", NATIVE_CALL( this ) {
		N_EXPECT_ARGS( 0 ); const auto& state = Conventional();
		return VALUEEXT( gse::value::Object, GSE_CALL, ( gse::value::object_properties_t{
			{ "design_id", VALUE( gse::value::Int,, state.design_id ) }, { "owner_simulation_id", VALUE( gse::value::Int,, state.owner_simulation_id ) },
			{ "stored_morale", VALUE( gse::value::Int,, state.stored_morale ) }, { "damage", VALUE( gse::value::Int,, state.damage ) },
			{ "spent", VALUE( gse::value::Int,, state.spent ) }, { "failed_move", VALUE( gse::value::Bool,, state.failed_move ) },
			{ "home", VALUE( gse::value::String,, state.home_base_id ? "base" : "none" ) },
			{ "home_base_id", state.home_base_id ? static_cast< gse::Value* >( VALUE( gse::value::Int,, static_cast< int64_t >( *state.home_base_id ) ) ) : VALUE( gse::value::Undefined ) }, { "render_morale_mode", VALUE( gse::value::Int,, static_cast< int32_t >( state.render_morale_mode ) ) }
		} ) );
	} ) },
	{ "validate_conventional_move", NATIVE_CALL( this ) {
		N_EXPECT_ARGS( 2 ); N_GETVALUE( caller, 0, Int ); N_GETVALUE_UNWRAP( tile, 1, map::tile::Tile );
		if ( caller < 0 || static_cast< uint64_t >( caller ) > SIZE_MAX ) { return VALUE( gse::value::String,, "Conventional movement caller is invalid" ); }
		try { m_um->ValidateConventionalMove( this, tile, static_cast< size_t >( caller ) ); }
		catch ( const std::runtime_error& error ) { return VALUE( gse::value::String,, error.what() ); }
		catch ( const std::invalid_argument& error ) { return VALUE( gse::value::String,, error.what() ); }
		return VALUE( gse::value::Undefined );
	} ) },
	{ "move_conventional", NATIVE_CALL( this ) {
		N_EXPECT_ARGS( 2 ); N_GETVALUE( caller, 0, Int ); N_GETVALUE_UNWRAP( tile, 1, map::tile::Tile );
		if ( caller < 0 || static_cast< uint64_t >( caller ) > SIZE_MAX ) { GSE_ERROR( gse::EC.INVALID_CALL, "Conventional movement caller is invalid" ); }
		auto* result = VALUE( gse::value::Undefined ); // allocate before authority commit
		m_um->MoveConventionalUnit( GSE_CALL, this, tile, static_cast< size_t >( caller ) ); return result;
	} ) },
	{ "query_conventional_stats", NATIVE_CALL( this ) {
		N_EXPECT_ARGS( 1 ); N_GETVALUE( mode, 0, Int );
		if ( mode < 0 || mode > 1 ) { GSE_ERROR( gse::EC.INVALID_CALL, "Conventional morale mode must be explicit 0 or 1" ); }
		const auto stats = QueryStats( static_cast< SmacMoraleMode >( mode ) );
		return VALUEEXT( gse::value::Object, GSE_CALL, ( gse::value::object_properties_t{
			{ "effective_morale", VALUE( gse::value::Int,, stats.effective_morale ) }, { "allowance_morale", VALUE( gse::value::Int,, stats.allowance_morale ) },
			{ "morale_name", VALUE( gse::value::String,, m_def->m_moraleset->m_morale_values.at( stats.effective_morale ).m_name ) },
			{ "quantum", VALUE( gse::value::Int,, stats.quantum ) }, { "basic_allowance", VALUE( gse::value::Int,, stats.basic_allowance ) },
			{ "full_allowance", VALUE( gse::value::Int,, stats.full_allowance ) }, { "remaining_allowance", VALUE( gse::value::Int,, stats.remaining_allowance ) },
			{ "hit_points", VALUE( gse::value::Int,, stats.hit_points ) }, { "maximum_hit_points", VALUE( gse::value::Int,, stats.maximum_hit_points ) },
			{ "work_order", VALUE( gse::value::Int,, static_cast< int32_t >( Conventional().work_order ) ) },
			{ "work_progress", VALUE( gse::value::Int,, Conventional().work_progress ) }
		} ) );
	} ) },
	{ "query_conventional_commands", NATIVE_CALL( this ) {
		N_EXPECT_ARGS( 0 );
		const auto commands = m_um->QueryConventionalCommands( this );
		gse::value::array_elements_t actions;
		for ( const auto action : commands.actions ) { actions.push_back( VALUE( gse::value::Int,, action ) ); }
		const auto kind = commands.package == ConventionalUnitContext::Package::Former ? "former"
			: commands.package == ConventionalUnitContext::Package::Colony ? "colony" : "none";
		return VALUEEXT( gse::value::Object, GSE_CALL, ( gse::value::object_properties_t{
			{ "kind", VALUE( gse::value::String,, kind ) },
			{ "actions", VALUE( gse::value::Array,, actions ) },
			{ "work_order", VALUE( gse::value::Int,, IsConventional() ? static_cast<int32_t>(Conventional().work_order) : 0 ) },
			{ "work_progress", VALUE( gse::value::Int,, IsConventional() ? Conventional().work_progress : 0 ) }
		} ) );
	} ) },
	WRAPIMPL_GET_CUSTOM( "is_immovable", Bool, m_def->GetMovementType() == MT_IMMOVABLE )
	WRAPIMPL_GET_CUSTOM( "is_land", Bool, m_def->GetMovementType() == MT_LAND )
	WRAPIMPL_GET_CUSTOM( "is_water", Bool, m_def->GetMovementType() == MT_WATER )
	WRAPIMPL_GET_CUSTOM( "is_air", Bool, m_def->GetMovementType() == MT_AIR )
	WRAPIMPL_LINK( "get_def", m_def )
	{ "get_owner", NATIVE_CALL(this) {N_EXPECT_ARGS(0);return IsNative() ? static_cast<gse::Value*>(VALUE(gse::value::Undefined)) : m_owner->Wrap(GSE_CALL);} ) },
	WRAPIMPL_LINK( "get_tile", m_tile )
	{
		"move_to_tile",
		NATIVE_CALL( this ) {

			m_um->m_game->CheckGameplayRW( GSE_CALL );
			if ( IsConventional() || IsNative() ) { GSE_ERROR( gse::EC.GAME_ERROR, "Canonical movement requires its native command producer" ); }

			N_EXPECT_ARGS( 2 );
			N_GETVALUE_UNWRAP( tile, 0, map::tile::Tile );
			N_GET_CALLABLE( on_complete, 1 );
			m_um->m_game->RequireWorkerKnowledgeMutation();
			m_um->Persist( on_complete );
			const auto* errmsg = m_um->MoveUnitToTile( GSE_CALL, this, tile, [ this, on_complete, gc_space, ctx, si, ep ]() {
				auto ep2 = ep;
				on_complete->Run( gc_space, ctx, si, ep2, {} );
				m_um->Unpersist( on_complete );
			});
			if ( errmsg ) {
				GSE_ERROR( gse::EC.GAME_ERROR, *errmsg );
				m_um->Unpersist( on_complete );
				delete errmsg;
			}
			return VALUE( gse::value::Undefined );
		} )
	},
WRAPIMPL_DYNAMIC_SETTERS( Unit )
	if ( obj->IsConventional() || obj->IsNative() ) { GSE_ERROR( gse::EC.INVALID_ASSIGNMENT, "Conventional unit fields are read-only; float state is not authoritative" ); }
	WRAPIMPL_SET_PTR( "movement", Float, Legacy().movement )
	WRAPIMPL_SET_PTR( "morale", Float, Legacy().morale )
	WRAPIMPL_SET_PTR( "health", Float, Legacy().health )
	WRAPIMPL_SET_PTR( "moved_this_turn", Bool, Legacy().moved_this_turn )
WRAPIMPL_DYNAMIC_ON_SET( Unit )
	// this is potentially risky because if it gets zero health it will be despawned without script's awareness, how to handle it?
	// maybe despawn unit from within script? but then it would be script's responsibility to ensure there are no zero-health units walking around
	m_um->RefreshUnit( GSE_CALL, this );
WRAPIMPL_DYNAMIC_END()

UNWRAPIMPL_PTR( Unit )

}
}
}

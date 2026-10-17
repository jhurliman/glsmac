#include "Base.h"
#include <set>
#include <utility>

#include "gse/context/Context.h"
#include "gse/value/Object.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"
#include "gse/value/Undefined.h"
#include "gse/value/Array.h"
#include "gse/value/Bool.h"
#include "gse/callable/Native.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/slot/Slot.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tile.h"
#include "game/backend/map/tile/Tiles.h"
#include "game/backend/faction/Faction.h"
#include "game/backend/base/BaseManager.h"
#include "Pop.h"
#include "PopDef.h"
#include "FacilityDef.h"
#include "game/backend/Random.h"
#include "game/backend/resource/ResourceManager.h"

namespace game {
namespace backend {
namespace base {

static size_t next_id = 1;
const size_t Base::GetNextId() {
	return next_id;
}
const void Base::SetNextId( const size_t id ) {
	next_id = id;
}

Base::Base(
	Game* game,
	const size_t id,
	slot::Slot* owner,
	faction::Faction* faction,
	map::tile::Tile* tile,
	const std::string& name,
	const facilities_t& facilities,
	const pops_t& pops,
	const size_t next_pop_id,
	const int64_t accumulated_nutrients,
	const std::optional< ProductionState >& production
)
	: MapObject( game->GetMap(), tile )
	, m_game( game )
	, m_id( id )
	, m_owner( owner )
	, m_faction( faction )
	, m_name( name )
	, m_facilities( facilities )
	, m_pops( pops )
	, m_next_pop_id( next_pop_id )
	, m_accumulated_nutrients( accumulated_nutrients )
	, m_production( production ) {
	if ( m_production ) { m_production->Validate(); }
	// Validate the complete record before claiming the center or advancing IDs.
	if ( !tile || tile->tiles->GetMap() != game->GetMap() || tile->base || game->GetBM()->GetBase( id ) ) {
		THROW( "invalid or occupied base location/ID" );
	}
	std::set< std::pair< size_t, size_t > > assigned;
	for ( const auto& entry : m_pops ) {
		const auto& pop = entry.second;
		if ( entry.first != pop.m_id || pop.m_base ) {
			THROW( "invalid population identity in base record" );
		}
		const auto& pos = pop.GetWorkedTileCoords();
		const auto error = game->GetBM()->ValidateAssignment( id, tile->coord, pop.m_id, pop.m_def, pos );
		if ( !error.empty() ) { THROW( error ); }
		if ( pos && !assigned.emplace( pos->x, pos->y ).second ) {
			THROW( "duplicate worked tile in base record" );
		}
	}
	if ( !m_pops.empty() && m_next_pop_id <= m_pops.rbegin()->first ) {
		THROW( "next population ID must follow existing IDs" );
	}
	if ( next_id <= id ) {
		next_id = id + 1;
	}
	ASSERT( !tile->base, "tile already has base" );
	tile->base = this;
	m_tile = tile;
	for ( auto& it : m_pops ) {
		it.second.SetBase( this );
	}
}

const Game* const Base::GetGame() const {
	return m_game;
}

Pop* const Base::AddPop( const Pop& pop ) {
	if ( m_pops.find( pop.m_id ) != m_pops.end() ) { THROW( "population ID already exists" ); }
	if ( pop.m_base != this ) { THROW( "population belongs to another base" ); }
	const auto error = m_game->GetBM()->ValidateAssignment( m_id, m_tile->coord, pop.m_id, pop.m_def, pop.m_worked_tile );
	if ( !error.empty() ) { THROW( error ); }
	if ( pop.m_worked_tile && m_game->GetBM()->GetBase( m_id ) != this ) {
		THROW( "worker assignment requires a registered base" );
	}
	m_pops.insert_or_assign( pop.m_id, pop );
	m_game->GetBM()->ChangeAssignment( m_id, pop.m_id, {}, pop.m_worked_tile );
	m_game->GetBM()->RefreshBase( this );
	TriggerUpdate();
	return &m_pops.at( pop.m_id );
}

void Base::RemovePop( const size_t pop_id ) {
	ASSERT( m_pops.find( pop_id ) != m_pops.end(), "pop id not found" );
	m_game->GetBM()->ChangeAssignment( m_id, pop_id, m_pops.at( pop_id ).m_worked_tile, {} );
	m_pops.erase( pop_id );
	m_game->GetBM()->RefreshBase( this );
	TriggerUpdate();
}

void Base::ChangePopType( GSE_CALLABLE, const size_t pop_id, const std::string& def_id ) {
	ASSERT( m_pops.find( pop_id ) != m_pops.end(), "pop id " + std::to_string( pop_id ) + " not found" );
	auto& pop = m_pops.at( pop_id );
	const auto* def = GetPopDef( GSE_CALL, def_id );
	SetPopAssignment( GSE_CALL, &pop, def_id, ( def->m_flags & PopDef::PF_TILE_WORKER ) ? pop.GetWorkedTile() : nullptr );
}

void Base::SetPopAssignment( GSE_CALLABLE, Pop* pop, const std::string& def_id, map::tile::Tile* tile ) {
	m_game->CheckRW( GSE_CALL );
	const auto member = pop ? m_pops.find( pop->m_id ) : m_pops.end();
	if ( member == m_pops.end() || &member->second != pop || pop->m_base != this ) {
		GSE_ERROR( gse::EC.GAME_ERROR, "Population does not belong to this base" );
	}
	if ( tile && tile->tiles->GetMap() != m_game->GetMap() ) {
		GSE_ERROR( gse::EC.GAME_ERROR, "Worked tile belongs to another map" );
	}
	const auto* def = GetPopDef( GSE_CALL, def_id );
	const Pop::worked_tile_t position = tile ? Pop::worked_tile_t( tile->coord ) : std::nullopt;
	const auto error = m_game->GetBM()->ValidateAssignment( m_id, m_tile->coord, pop->m_id, def, position );
	if ( !error.empty() ) { GSE_ERROR( gse::EC.GAME_ERROR, error ); }
	if ( position && m_game->GetBM()->GetBase( m_id ) != this ) {
		GSE_ERROR( gse::EC.GAME_ERROR, "Worker assignment requires a registered base" );
	}
	if ( pop->m_def == def && pop->m_worked_tile == position ) { return; }
	// All logical validation precedes changes to type, coordinates, or the index.
	m_game->GetBM()->ChangeAssignment( m_id, pop->m_id, pop->m_worked_tile, position );
	pop->m_worked_tile = position;
	pop->m_def = def;
	m_game->GetBM()->RefreshBase( this );
	TriggerUpdate();
}

int64_t Base::GetAccumulatedNutrients() const {
	return m_accumulated_nutrients;
}

void Base::SetAccumulatedNutrients( const int64_t nutrients ) {
	if ( m_accumulated_nutrients != nutrients ) {
		m_accumulated_nutrients = nutrients;
		TriggerUpdate();
	}
}

const std::optional< ProductionState >& Base::GetProductionState() const {
	return m_production;
}

void Base::SetProductionState( GSE_CALLABLE, const std::optional< ProductionState >& production ) {
	m_game->CheckRW( GSE_CALL );
	if ( production ) { production->Validate(); }
	// Copy completely before swapping: even an allocation failure preserves the
	// prior record. Neither switching nor baseline arithmetic belongs here.
	if ( m_production == production ) { return; }
	auto replacement = production;
	m_production.swap( replacement );
	TriggerUpdate();
}

const types::Buffer Base::Serialize( const Base* base ) {
	types::Buffer buf;
	buf.WriteInt( base->m_id );
	buf.WriteInt( base->m_owner->GetIndex() );
	buf.WriteString( base->m_faction->m_id );
	buf.WriteInt( base->m_tile->coord.x );
	buf.WriteInt( base->m_tile->coord.y );
	buf.WriteString( base->m_name );
	buf.WriteInt( base->m_facilities.size() );
	for ( const auto& it : base->m_facilities ) {
		buf.WriteString( it.first );
	}
	buf.WriteInt( base->m_pops.size() );
	for ( const auto& it : base->m_pops ) {
		buf.WriteInt( it.first );
		it.second.Serialize( buf );
	}
	buf.WriteInt( base->m_next_pop_id );
	buf.WriteInt( base->m_accumulated_nutrients );
	buf.WriteString( SerializeProductionState( base->m_production ).ToString() );
	return buf;
}

Base* Base::Deserialize( types::Buffer& buf, Game* game ) {
	ASSERT( game, "game is null" );
	const auto id = buf.ReadInt();
	auto* slot = &game->GetState()->m_slots->GetSlot( buf.ReadInt() );
	const auto faction_id = buf.ReadString();
	auto* faction = game->GetFaction( faction_id );
	const auto pos_x = buf.ReadInt();
	const auto pos_y = buf.ReadInt();
	const auto* tiles = game->GetMap()->GetTilesPtr();
	if ( pos_x < 0 || pos_y < 0 || pos_x >= tiles->GetWidth() || pos_y >= tiles->GetHeight() || ( pos_x & 1 ) != ( pos_y & 1 ) ) {
		THROW( "invalid base coordinates" );
	}
	auto* tile = game->GetMap()->GetTile( pos_x, pos_y );
	const auto name = buf.ReadString();
	facilities_t facilities = {};
	const auto facilities_count = buf.ReadInt();
	if ( facilities_count > 0 ) {
		const auto& bm = game->GetBM();
		for ( size_t i = 0 ; i < facilities_count ; i++ ) {
			const auto facility_id = buf.ReadString();
			const auto* def = bm->GetFacilityDef( facility_id );
			if ( !def ) {
				THROW( "can't deserialize base - facility def not found: " + facility_id );
			}
			facilities.insert({ facility_id, def });
		}
	}
	pops_t pops = {};
	const auto pops_count = buf.ReadInt();
	for ( size_t i = 0 ; i < pops_count ; i++ ) {
		const auto pop_id = buf.ReadInt();
		Pop pop = {};
		pop.Deserialize( buf, game );
		if ( !pops.emplace( pop_id, pop ).second ) {
			THROW( "duplicate population ID in base record" );
		}
	}
	const auto next_pop_id = buf.ReadInt();
	const auto accumulated_nutrients = buf.ReadInt();
	types::Buffer production_buffer( buf.ReadString() );
	const auto production = DeserializeProductionState( production_buffer );
	return new Base( game, id, slot, faction, tile, name, facilities, pops, next_pop_id, accumulated_nutrients, production );
}

WRAPIMPL_SERIALIZE( Base )
	buf->WriteInt( obj->m_id );
}

WRAPIMPL_DESERIALIZE( Base )
	const auto id = buf->ReadInt();
	const auto& base = game->GetBM()->GetBase( id );
	ASSERT( base, "base id not found: " + std::to_string( id ) );
	return base->Wrap( GSE_CALL );
}

namespace {

const char* ProductionKindName( const ProductionItemKind kind ) {
	switch ( kind ) {
		case ProductionItemKind::Unit: return "unit";
		case ProductionItemKind::Facility: return "facility";
		case ProductionItemKind::Orbital: return "orbital";
		case ProductionItemKind::Project: return "project";
		case ProductionItemKind::Stockpile: return "stockpile";
	}
	THROW( "Unknown production item kind" );
}

gse::Value* ProductionItemValue( GSE_CALLABLE, const ProductionItem& item ) {
	gse::value::object_properties_t fields = { { "kind", VALUE( gse::value::String,, ProductionKindName( item.kind ) ) } };
	if ( item.kind != ProductionItemKind::Stockpile ) { fields.emplace( "id", VALUE( gse::value::String,, item.id ) ); }
	return new gse::value::Object( GSE_CALL, fields );
}

ProductionItem ReadProductionItem( GSE_CALLABLE, const gse::value::object_properties_t& fields ) {
	const auto kind_field = fields.find( "kind" );
	if ( kind_field == fields.end() || kind_field->second->Deref()->type != gse::VT_STRING ) {
		GSE_ERROR( gse::EC.INVALID_CALL, "Production item requires a string kind" );
	}
	const auto& name = static_cast< gse::value::String* >( kind_field->second->Deref() )->value;
	ProductionItemKind kind;
	if ( name == "unit" ) { kind = ProductionItemKind::Unit; }
	else if ( name == "facility" ) { kind = ProductionItemKind::Facility; }
	else if ( name == "orbital" ) { kind = ProductionItemKind::Orbital; }
	else if ( name == "project" ) { kind = ProductionItemKind::Project; }
	else if ( name == "stockpile" ) { kind = ProductionItemKind::Stockpile; }
	else { GSE_ERROR( gse::EC.INVALID_CALL, "Unknown production item kind: " + name ); }
	std::string id;
	if ( kind == ProductionItemKind::Stockpile ) {
		if ( fields.size() != 1 ) { GSE_ERROR( gse::EC.INVALID_CALL, "Stockpile only accepts kind" ); }
	}
	else {
		const auto id_field = fields.find( "id" );
		if ( fields.size() != 2 || id_field == fields.end() || id_field->second->Deref()->type != gse::VT_STRING ) {
			GSE_ERROR( gse::EC.INVALID_CALL, "Production item requires only kind and string id" );
		}
		id = static_cast< gse::value::String* >( id_field->second->Deref() )->value;
		if ( id.empty() ) { GSE_ERROR( gse::EC.INVALID_CALL, "Production item requires a nonempty identity" ); }
	}
	return { kind, std::move( id ) };
}

}

WRAPIMPL_DYNAMIC_GETTERS( Base )
	WRAPIMPL_GET_CUSTOM( "id", Int, m_id )
	WRAPIMPL_GET_CUSTOM( "name", String, m_name )
	WRAPIMPL_LINK( "get_owner", m_owner )
	WRAPIMPL_LINK( "get_tile", m_tile )
	WRAPIMPL_CUSTOM_SETTERS
	{
		"get_production_state",
		NATIVE_CALL( this ) {
			N_EXPECT_ARGS( 0 );
			if ( !m_production ) { return VALUE( gse::value::Undefined ); }
			return new gse::value::Object( GSE_CALL, {
				{ "current_item", ProductionItemValue( GSE_CALL, m_production->current_item ) },
				{ "minerals", VALUE( gse::value::Int,, m_production->minerals ) },
				{ "baseline_item", ProductionItemValue( GSE_CALL, m_production->baseline_item ) },
				{ "baseline_minerals", VALUE( gse::value::Int,, m_production->baseline_minerals ) },
			} );
		} )
	},
	{
		"set_production_state",
		NATIVE_CALL( this ) {
			m_game->CheckRW( GSE_CALL );
			N_EXPECT_ARGS( 1 );
			if ( arguments.at( 0 )->Deref()->type == gse::VT_UNDEFINED ) {
				SetProductionState( GSE_CALL, std::nullopt );
			}
			else {
				N_GETVALUE( data, 0, Object );
				if ( data.size() != 4 ) { GSE_ERROR( gse::EC.INVALID_CALL, "Production state requires exactly four fields" ); }
				N_GETPROP( current_fields, data, "current_item", Object );
				N_GETPROP( minerals, data, "minerals", Int );
				N_GETPROP( baseline_fields, data, "baseline_item", Object );
				N_GETPROP( baseline_minerals, data, "baseline_minerals", Int );
				const auto current = ReadProductionItem( GSE_CALL, current_fields );
				const auto baseline = ReadProductionItem( GSE_CALL, baseline_fields );
				SetProductionState( GSE_CALL, ProductionState{ current, minerals, baseline, baseline_minerals } );
			}
			return VALUE( gse::value::Undefined );
		} )
	},
	{
		"get_accumulated_nutrients",
		NATIVE_CALL( this ) {
			N_EXPECT_ARGS( 0 );
			return VALUE( gse::value::Int,, GetAccumulatedNutrients() );
		} )
	},
	{
		"set_accumulated_nutrients",
		NATIVE_CALL( this ) {
			m_game->CheckRW( GSE_CALL );
			N_EXPECT_ARGS( 1 );
			N_GETVALUE( nutrients, 0, Int );
			SetAccumulatedNutrients( nutrients );
			return VALUE( gse::value::Undefined );
		} )
	},
	{
		"create_pop",
		NATIVE_CALL( this ) {

			m_game->CheckRW( GSE_CALL );

			N_EXPECT_ARGS( 1 );
			N_GETVALUE( data, 0, Object );
			N_GETPROP( def_id, data, "type", String );
			N_GETPROP_UNWRAP_OPT( worked_tile, data, "worked_tile", map::tile::Tile );
			auto* def = GetPopDef( GSE_CALL, def_id );
			const auto max_variants = (m_faction->m_flags & faction::Faction::FF_PROGENITOR)
				? 1 // aliens have 1 gender
				: 2; // humans have 2
			ASSERT( max_variants > 0, "no variants found for pop type: " + def_id );

			if ( worked_tile && worked_tile->tiles->GetMap() != m_game->GetMap() ) {
				GSE_ERROR( gse::EC.GAME_ERROR, "Worked tile belongs to another map" );
			}
			if ( m_pops.find( m_next_pop_id ) != m_pops.end() ) {
				GSE_ERROR( gse::EC.GAME_ERROR, "Next population ID already exists" );
			}
			Pop candidate( this, m_next_pop_id, def, 0, worked_tile );
			const auto error = m_game->GetBM()->ValidateAssignment( m_id, m_tile->coord, candidate.m_id, def, candidate.m_worked_tile );
			if ( !error.empty() ) { GSE_ERROR( gse::EC.GAME_ERROR, error ); }
			if ( worked_tile && m_game->GetBM()->GetBase( m_id ) != this ) {
				GSE_ERROR( gse::EC.GAME_ERROR, "Worker assignment requires a registered base" );
			}
			candidate.m_variant = m_game->GetRandom()->GetUInt( 0, max_variants - 1 );
			auto* const pop = AddPop( candidate );
			m_next_pop_id++;

			return pop->Wrap( GSE_CALL );
		} )
	},
	{
		"destroy_pop",
		NATIVE_CALL( this ) {

			m_game->CheckRW( GSE_CALL );

			N_EXPECT_ARGS( 1 );
			N_GETVALUE_UNWRAP( pop, 0, Pop );

			const auto member = m_pops.find( pop->m_id );
			if ( member == m_pops.end() || &member->second != pop ) {
				GSE_ERROR( gse::EC.GAME_ERROR, "Base does not have pop " + std::to_string( pop->m_id ) );
			}

			RemovePop( pop->m_id );

			return VALUE( gse::value::Undefined );
		} )
	},
	{
		"set_pop_assignment",
		NATIVE_CALL( this ) {
			N_EXPECT_ARGS( 3 );
			N_GETVALUE_UNWRAP( pop, 0, Pop );
			N_GETVALUE( def_id, 1, String );
			map::tile::Tile* tile = nullptr;
			const auto type = arguments.at( 2 )->Deref()->type;
			if ( type != gse::VT_UNDEFINED && type != gse::VT_NULL ) {
				N_GETVALUE_UNWRAP( assigned_tile, 2, map::tile::Tile );
				tile = assigned_tile;
			}
			SetPopAssignment( GSE_CALL, pop, def_id, tile );
			return VALUE( gse::value::Undefined );
		} )
	},
	{
		"is_tile_worked",
		NATIVE_CALL( this ) {
			N_EXPECT_ARGS( 1 );
			N_GETVALUE_UNWRAP( tile, 0, map::tile::Tile );
			auto* pop = tile->tiles->GetMap() == m_game->GetMap() ? m_game->GetBM()->GetWorkingPop( tile->coord ) : nullptr;
			return VALUE( gse::value::Bool,, pop && pop->m_base == this );
		} )
	},
	{
		"get_pops",
		NATIVE_CALL( this ) {
			N_EXPECT_ARGS( 0 );

			gse::value::array_elements_t elements = {};

			for ( auto& it : m_pops ) {
				elements.push_back( it.second.Wrap( GSE_CALL ) );
			}

			return VALUE( gse::value::Array,, elements );
		} ),
	},
	{
		"get_facilities",
		NATIVE_CALL( this ) {
			N_EXPECT_ARGS( 0 );

			gse::value::object_properties_t result = {};

			for ( auto& it : m_facilities ) {
				result.insert( { it.first, VALUEEXT( gse::value::Object, GSE_CALL, {
					{ "name", VALUE( gse::value::String,, it.second->m_name ) },
					{ "description", VALUE( gse::value::String,, it.second->m_description ) },
					{ "cost", VALUE( gse::value::Int,, it.second->m_cost ) },
				} ) } );
			}

			return VALUEEXT( gse::value::Object, GSE_CALL, result );
		} ),
	},
	{
		"has_facility",
		NATIVE_CALL( this ) {
			N_EXPECT_ARGS( 1 );

			N_GETVALUE( id, 0, String );

			return VALUE( gse::value::Bool,, m_facilities.find( id ) != m_facilities.end() );
		} ),
	},
	{
		"get_size",
		NATIVE_CALL( this ) {
			N_EXPECT_ARGS( 0 );
			return VALUE( gse::value::Int,, m_pops.size() );
		} ),
	},
	{
		"get_workable_tiles",
		NATIVE_CALL( this ) {
			N_EXPECT_ARGS( 0 );
			return GetWorkableTiles( GSE_CALL );
		} ),
	},
	{
		"get_worked_tiles",
		NATIVE_CALL( this ) {
			N_EXPECT_ARGS( 0 );
			return GetWorkedTiles( GSE_CALL );
		} ),
	},
	{
		"get_unworked_tiles",
		NATIVE_CALL( this ) {
			N_EXPECT_ARGS( 0 );
			return GetUnworkedTiles( GSE_CALL );
		} ),
	},
	{
		"get_intake",
		NATIVE_CALL( this ) {
			N_EXPECT_ARGS( 0 );
			return GetIntake( GSE_CALL );
		} ),
	},
	{
		"get_consumption",
		NATIVE_CALL( this ) {
			N_EXPECT_ARGS( 0 );
			return GetConsumption( GSE_CALL );
		} ),
	},
WRAPIMPL_DYNAMIC_SETTERS( Base )
WRAPIMPL_DYNAMIC_ON_SET( Base )
WRAPIMPL_DYNAMIC_END()

UNWRAPIMPL_PTR( Base )

void Base::GetReachableObjects( std::unordered_set< gc::Object* >& reachable_objects ) {
	gse::Wrappable::GetReachableObjects( reachable_objects );

	GC_DEBUG_BEGIN( "pops" );
	for ( auto& it : m_pops ) {
		it.second.GetReachableObjects( reachable_objects );
	}
	GC_DEBUG_END();
}

const PopDef* const Base::GetPopDef( GSE_CALLABLE, const std::string& id ) const {
	auto* def = m_game->GetBM()->GetPopDef( id );
	if ( !def ) {
		GSE_ERROR( gse::EC.INVALID_DEFINITION, "Unknown pop type: " + id );
	}
	return def;
}

gse::value::Array* const Base::GetWorkableTiles( GSE_CALLABLE ) {
	auto* const result = m_game->GetBM()->Trigger( GSE_CALL, "get_base_workable_tiles", ARGS_F( this ) {
		{
			"base",
			Wrap( GSE_CALL )
		},
	}; } );
	ASSERT( result, "GetWorkableTiles result is null" );
	if ( result->type != gse::VT_ARRAY ) {
		GSE_ERROR( gse::EC.GAME_ERROR, "get_base_workable_tiles must return array, got: " + result->ToString() );
	}
	for ( const auto& v : ((gse::value::Array*)result)->value ) {
		if ( v->type != gse::VT_OBJECT || ((gse::value::Object*)v)->object_class != "Tile" ) {
			GSE_ERROR( gse::EC.GAME_ERROR, "get_base_workable_tiles elements must be objects of type Tile, got: " + v->ToString() );
		}
	}
	return (gse::value::Array*)result;
}

gse::value::Array* const Base::GetWorkedTiles( GSE_CALLABLE ) {
	gse::value::array_elements_t result = {};
	for ( const auto& pop : m_pops ) {
		if ( auto* tile = pop.second.GetWorkedTile() ) {
			result.push_back( tile->Wrap( GSE_CALL ) );
		}
	}
	return VALUE( gse::value::Array,, result );
}

gse::value::Array* const Base::GetUnworkedTiles( GSE_CALLABLE ) {
	gse::value::array_elements_t result = {};
	const auto* workable_tiles_v = GetWorkableTiles( GSE_CALL );
	for ( const auto& v : workable_tiles_v->value ) {
		ASSERT( v->type == gse::VT_OBJECT && ((gse::value::Object*)v)->object_class == "Tile", "invalid tile object" );
		auto* tile = (map::tile::Tile*)((gse::value::Object*)v)->wrapobj;
		if ( !m_game->GetBM()->GetWorkingPop( tile->coord ) ) {
			result.push_back( v );
		}
	}
	return VALUE( gse::value::Array,, result );
}

gse::value::Object* const Base::GetIntake( GSE_CALLABLE ) {
	return GetResourcesFromCallback( GSE_CALL, m_game->GetBM(), m_game->GetRM(), "get_base_intake", ARGS_F( this ) {
		{
			"base",
			Wrap( GSE_CALL )
		},
		{
			"player",
			m_owner->Wrap( GSE_CALL )
		},
	}; } );
}

gse::value::Object* const Base::GetConsumption( GSE_CALLABLE ) {
	return GetResourcesFromCallback( GSE_CALL, m_game->GetBM(), m_game->GetRM(), "get_base_consumption", ARGS_F( this ) {
		{
			"base",
			Wrap( GSE_CALL )
		},
		{
			"player",
			m_owner->Wrap( GSE_CALL )
		},
	}; } );
}

void Base::TriggerUpdate() {
	m_game->GetBM()->AddUpdateTrigger( this );
}

}
}
}

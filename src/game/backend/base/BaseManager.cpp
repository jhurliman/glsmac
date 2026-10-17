#include "BaseManager.h"
#include "game/backend/unit/UnitManager.h"

#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/Bindings.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/Player.h"
#include "game/backend/faction/Faction.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tiles.h"

#include "Base.h"
#include "PopDef.h"
#include "FacilityDef.h"

#include "gse/context/Context.h"
#include "gse/callable/Native.h"
#include "gse/value/Bool.h"
#include "gse/value/Array.h"
#include <memory>
#include <algorithm>
#include <set>
#include "util/FinallyGuard.h"

namespace game {
namespace backend {
namespace base {

// A typed string also makes old readers, which expect an integer count first,
// reject this format before interpreting any definitions or base instances.
static const std::string s_snapshot_format = "GLSMAC_BASES_V9";

BaseManager::BaseManager( Game* game )
	: gse::GCWrappable( game->GetGCSpace() )
	, m_game( game ) {
	//
}

BaseManager::~BaseManager() {
	Clear();
}

void BaseManager::Clear() {
	m_work_assignments.clear();
	for ( auto& it : m_base_popdefs ) {
		delete it.second;
	}
	m_base_popdefs.clear();
	for ( auto& it : m_bases ) {
		delete it.second;
	}
	m_bases.clear();
	m_base_order.clear();
	for ( auto& it : m_base_facilitydefs ) {
		delete it.second;
	}
	m_base_facilitydefs.clear();

	m_base_updates.clear();
}

PopDef* BaseManager::GetPopDef( const std::string& id ) const {
	const auto& it = m_base_popdefs.find( id );
	if ( it != m_base_popdefs.end() ) {
		return it->second;
	}
	else {
		return nullptr;
	}
}

Base* BaseManager::GetBase( const size_t id ) const {
	const auto& it = m_bases.find( id );
	if ( it != m_bases.end() ) {
		return it->second;
	}
	else {
		return nullptr;
	}
}

Pop* BaseManager::GetWorkingPop( const map::tile::coords_t& coord ) const {
	const auto claim = m_work_assignments.find( { coord.x, coord.y } );
	if ( claim == m_work_assignments.end() ) {
		return nullptr;
	}
	auto* base = GetBase( claim->second.first );
	if ( !base ) {
		return nullptr;
	}
	const auto pop = base->m_pops.find( claim->second.second );
	return pop == base->m_pops.end() ? nullptr : &pop->second;
}

std::string BaseManager::ValidateAssignment( const size_t base_id, const map::tile::coords_t& center, const size_t pop_id, const PopDef* def, const Pop::worked_tile_t& tile ) const {
	if ( !tile ) {
		return "";
	}
	const auto* tiles = m_game->GetMap()->GetTilesPtr();
	if ( tile->x >= tiles->GetWidth() || tile->y >= tiles->GetHeight() || ( tile->x & 1 ) != ( tile->y & 1 ) ) {
		return "Invalid worked tile coordinates";
	}
	if ( *tile == center ) {
		return "The base center does not need a worker";
	}
	if ( !def || !( def->m_flags & PopDef::PF_TILE_WORKER ) ) {
		return "This population type cannot work tiles";
	}
	const auto claim = m_work_assignments.find( { tile->x, tile->y } );
	if ( claim != m_work_assignments.end() && claim->second != pop_key_t( base_id, pop_id ) ) {
		return "This tile is already worked";
	}
	return "";
}

void BaseManager::ChangeAssignment( const size_t base_id, const size_t pop_id, const Pop::worked_tile_t& old_tile, const Pop::worked_tile_t& new_tile ) {
	if ( old_tile == new_tile ) {
		return;
	}
	if ( new_tile ) {
		m_work_assignments.emplace( tile_key_t( new_tile->x, new_tile->y ), pop_key_t( base_id, pop_id ) );
	}
	if ( old_tile ) {
		m_work_assignments.erase( { old_tile->x, old_tile->y } );
	}
}

void BaseManager::RemoveAssignments( const Base* base ) {
	for ( const auto& pop : base->m_pops ) {
		ChangeAssignment( base->m_id, pop.first, pop.second.GetWorkedTileCoords(), {} );
	}
}

void BaseManager::DefinePop( PopDef* pop_def ) {
	ValidateSmacPopDef( pop_def );
	Log( "Defining base pop ('" + pop_def->m_id + "')" );

	// The caller keeps ownership until this method returns successfully. Build
	// the owned notification before publication, then roll back the exact entry
	// if the frontend queue cannot copy it. Its vector append has no callbacks.
	auto fr = FrontendRequest( FrontendRequest::FR_BASE_POP_DEFINE );
	NEW( fr.data.base_pop_define.serialized_popdef, std::string, PopDef::Serialize( pop_def ).ToString() );
	const auto inserted = m_base_popdefs.emplace( pop_def->m_id, pop_def );
	if ( !inserted.second ) { THROW( "Base pop def '" + pop_def->m_id + "' already exists" ); }
	try { m_game->AddFrontendRequest( fr ); }
	catch ( ... ) { m_base_popdefs.erase( inserted.first ); throw; }
}

void BaseManager::ValidateSmacPopDef( const PopDef* def ) const {
	if ( !def || !def->m_smac_specialist_id ) { return; }
	const auto& settings = m_game->GetState()->m_settings.global;
	const auto& rules = settings.GetSmacSpecialistRules();
	const auto& technology = settings.GetSmacTechnologyRules();
	if ( settings.GetRulesetId() != "smac" || !settings.IsRulesetLocked() || !rules || !technology
		|| *def->m_smac_specialist_id >= rules->specialists.size() || ( def->m_flags & PopDef::PF_TILE_WORKER ) ) {
		THROW( "SMAC specialist role requires a frozen positional catalog" );
	}
	rules->Validate();
	rules->TechnologyReferences( *technology );
}

void BaseManager::ValidateSmacResourceContext( const SmacBaseResourceContext& context, const slot::Slot* owner, const faction::Faction* faction ) const {
	context.Validate();
	m_game->ValidateCurrentSimulation();
	if ( !m_game->m_simulation ) { THROW( "SMAC base context requires an admitted simulation" ); }
	const auto& global = m_game->GetState()->m_settings.global;
	if ( !global.GetSmacResourceRules() || !global.GetSmacSpecialistRules() ) { THROW( "SMAC base context requires frozen resource and specialist profiles" ); }
	for ( const auto& participant : m_game->m_simulation->value.roster ) {
		if ( participant.simulation_id == context.owner_simulation_id
			&& &m_game->GetState()->m_slots->GetSlot( participant.slot ) == owner
			&& faction && participant.faction_id == faction->m_id ) { return; }
	}
	THROW( "SMAC base context owner/faction does not match its bound participant" );
}

SmacBaseResourceContext BaseManager::PrepareStartingResourceContext( const slot::Slot* owner ) const {
	// Reuse the exact claimed new-world/turn-zero admission, not a script flag
	// or a running-state approximation. This produces context only, not a full
	// population, production, facility or citizen-allocation package.
	m_game->RequireConventionalStartupClaim();
	m_game->ValidateCurrentSimulation();
	if ( m_game->m_simulation->value.designs.global_difficulty != 3 ) { THROW( "Starting base governor producer is admitted only at original difficulty 3" ); }
	for ( const auto& item : m_bases ) {
		if ( item.second->m_owner == owner ) { THROW( "Starting base resource producer requires the participant's first Base" ); }
	}
	for ( const auto& participant : m_game->m_simulation->value.roster ) {
		if ( &m_game->GetState()->m_slots->GetSlot( participant.slot ) == owner ) {
			// Drone riots begin false by explicit GLSMAC new-campaign policy.
			// Later citizen calculation owns changes; restore never replays this.
			SmacBaseResourceContext result{ participant.simulation_id, false, 0, 0, 0x00dfbe41u, false };
			ValidateSmacResourceContext( result, owner, owner->GetPlayer()->GetFaction() );
			return result;
		}
	}
	THROW( "Starting base owner is not an admitted participant" );
}

void BaseManager::UndefinePop( const std::string& id ) {
	Log( "Undefining base pop ('" + id + "')" );

	ASSERT( m_base_popdefs.find( id ) != m_base_popdefs.end(), "Base pop def '" + id + "' does not exist" );
	for ( const auto& item : m_bases ) {
		if ( item.second->HasSmacResourceContext() ) {
			for ( const auto& pop : item.second->m_pops ) {
				if ( pop.second.m_def->m_id == id ) { THROW( "Cannot undefine a canonical population role in use" ); }
			}
		}
	}

	m_base_popdefs.erase( id );

	auto fr = FrontendRequest( FrontendRequest::FR_BASE_POP_UNDEFINE );
	NEW( fr.data.base_pop_undefine.id, std::string, id );
	m_game->AddFrontendRequest( fr );
}

FacilityDef* BaseManager::GetFacilityDef( const std::string& id ) const {
	const auto& it = m_base_facilitydefs.find( id );
	if ( it != m_base_facilitydefs.end() ) {
		return it->second;
	}
	else {
		return nullptr;
	}
}

void BaseManager::DefineFacility( FacilityDef* facility_def ) {
	if ( !facility_def ) { THROW( "Missing facility definition" ); }
	ValidateSmacFacilityDef( facility_def );
	Log( "Defining base facility ('" + facility_def->m_id + "')" );

	if ( facility_def->m_smac_source_id ) {
		for ( const auto& entry : m_base_facilitydefs ) {
			if ( entry.second->m_smac_source_id == facility_def->m_smac_source_id ) { THROW( "SMAC facility catalog position is already bound" ); }
		}
	}
	if ( !m_base_facilitydefs.emplace( facility_def->m_id, facility_def ).second ) { THROW( "Facility ID is already defined" ); }

	// TODO
	/*auto fr = FrontendRequest( FrontendRequest::FR_BASE_FACILITY_DEFINE );
	NEW( fr.data.base_facility_define.serialized_facilitydef, std::string, FacilityDef::Serialize( facility_def ).ToString() );
	m_game->AddFrontendRequest( fr );*/
}

void BaseManager::ValidateSmacFacilityDef( const FacilityDef* def ) const {
	if ( !def || !def->m_smac_source_id ) { return; }
	const auto& global = m_game->GetState()->m_settings.global;
	const auto& rules = global.GetSmacProductionRules();
	if ( global.GetRulesetId() != "smac" || !global.IsRulesetLocked() || !rules
		|| *def->m_smac_source_id < 1 || *def->m_smac_source_id > 71 ) {
		THROW( "SMAC facility identity requires the frozen production catalog" );
	}
	rules->Validate();
	if ( rules->facilities.at( *def->m_smac_source_id - 1 ).source_id != *def->m_smac_source_id ) { THROW( "Mismatched SMAC facility catalog identity" ); }
}

void BaseManager::UndefineFacility( const std::string& id ) {
	Log( "Undefining base facility ('" + id + "')" );

	const auto found = m_base_facilitydefs.find( id );
	if ( found == m_base_facilitydefs.end() ) { THROW( "Unknown facility definition" ); }
	for ( const auto& entry : m_bases ) {
		for ( const auto& installed : entry.second->m_facilities ) {
			if ( installed.second == found->second ) { THROW( "Cannot undefine an installed facility" ); }
		}
	}
	delete found->second;
	m_base_facilitydefs.erase( found );

	/* TODO
	auto fr = FrontendRequest( FrontendRequest::FR_BASE_FACILITY_UNDEFINE );
	NEW( fr.data.base_facility_undefine.id, std::string, id );
	m_game->AddFrontendRequest( fr );*/
}

void BaseManager::SpawnBase( GSE_CALLABLE, Base* base ) {
	m_game->CheckRW( GSE_CALL );
	auto territory = m_game->PrepareBaseTerritory( base, nullptr, true );
	if ( GetBase( base->m_id ) ) { GSE_ERROR( gse::EC.GAME_ERROR, "Base ID already exists" ); }

	auto* tile = base->GetTile();

	// validate and fix name if needed (or assign if empty)
	std::vector< std::string > names_to_try = {};
	if ( base->m_name.empty() ) {
		const auto& names = base->m_owner->GetPlayer()->GetFaction()->m_base_names;
		names_to_try = tile->is_water_tile
			? names.water
			: names.land;
	}
	else if ( m_registered_base_names.find( base->m_name ) != m_registered_base_names.end() ) {
		names_to_try = { base->m_name };
	}
	if ( !names_to_try.empty() ) {
		size_t cycle = 0;
		bool found = false;
		while ( !found ) {
			cycle++;
			for ( const auto& name_to_try : names_to_try ) {
				base->m_name = cycle == 1
					? name_to_try
					: name_to_try + " " + std::to_string( cycle );
				if ( m_registered_base_names.find( base->m_name ) == m_registered_base_names.end() ) {
					found = true;
					break;
				}
			}
		}
	}
	RegisterBase( base );
	m_game->CommitBaseTerritory( territory );

	auto* state = m_game->GetState();
	if ( state->IsMaster() ) {
		state->TriggerObject( this, "base_spawn", ARGS_F( &base ) {
			{
				"base",
				base->Wrap( GSE_CALL )
			},
		}; } );
	}
}

void BaseManager::RegisterBase( Base* base ) {
	Log( "Registering base #" + std::to_string( base->m_id ) + " ( " + base->m_name + " ) at " + base->GetTile()->ToString() );
	if ( m_bases.find( base->m_id ) != m_bases.end() ) { THROW( "duplicate base ID" ); }
	auto next_order = m_base_order;
	next_order.push_back( base->m_id );
	for ( const auto& pop : base->m_pops ) {
		const auto error = ValidateAssignment( base->m_id, base->GetTile()->coord, pop.first, pop.second.m_def, pop.second.GetWorkedTileCoords() );
		if ( !error.empty() ) { THROW( error ); }
	}
	m_bases.insert_or_assign( base->m_id, base );
	m_base_order.swap( next_order );
	for ( const auto& pop : base->m_pops ) {
		ChangeAssignment( base->m_id, pop.first, {}, pop.second.GetWorkedTileCoords() );
	}
	m_registered_base_names.insert( base->m_name );
	QueueBaseUpdate( base, BUO_SPAWN );
	RefreshBase( base );
}

void BaseManager::DespawnBase( GSE_CALLABLE, const size_t base_id ) {
	m_game->CheckRW( GSE_CALL );

	const auto& it = m_bases.find( base_id );
	if ( it == m_bases.end() ) {
		GSE_ERROR( gse::EC.GAME_ERROR, "Base id " + std::to_string( base_id ) + " not found" );
	}

	auto* base = it->second;
	const auto ordered = std::find( m_base_order.begin(), m_base_order.end(), base_id );
	if ( ordered == m_base_order.end() ) { THROW( "Base is missing its record order" ); }
	if ( m_game->GetUM()->HasConventionalHome( base_id ) ) { THROW( "Cannot remove a conventional home Base before explicit reassignment" ); }
	auto territory = m_game->PrepareBaseTerritory( nullptr, base, false );

	Log( "Despawning base #" + std::to_string( base->m_id ) + " at " + base->GetTile()->ToString() );

	{
		std::lock_guard guard( m_updated_bases_mutex );
		m_updated_bases.erase( base );
	}

	QueueBaseUpdate( base, BUO_DESPAWN );

	auto* tile = base->GetTile();
	ASSERT( tile, "base tile not set" );
	ASSERT( tile->base, "base not found in tile" );
	ASSERT( tile->base == base, "tile base mismatch" );
	tile->base = nullptr;
	RemoveAssignments( base );

	m_bases.erase( it );
	m_base_order.erase( ordered );
	m_game->CommitBaseTerritory( territory );

	auto* state = m_game->GetState();
	if ( state->IsMaster() ) {
		state->TriggerObject( this, "base_despawn", ARGS_F( &base ) {
			{
				"base",
				base->Wrap( GSE_CALL )
			}
		}; } );
	}

	delete base;
}

const std::map< size_t, Base* >& BaseManager::GetBases() const {
	return m_bases;
}

const BaseManager::popdefs_t& BaseManager::GetBasePopDefs() const {
	return m_base_popdefs;
}

void BaseManager::ProcessUnprocessed( GSE_CALLABLE ) {
	for ( auto& it : m_unprocessed_bases ) {
		RegisterBase( Base::Deserialize( it, m_game ) );
	}
	m_unprocessed_bases.clear();
}

void BaseManager::PushUpdates() {
	if ( m_game->IsRunning() && !m_base_updates.empty() ) {
		for ( const auto& it : m_base_updates ) {
			const auto base_id = it.first;
			const auto& bu = it.second;
			const auto& base = bu.base;
			if ( !( bu.ops & BUO_DESPAWN ) ) { base->ValidateSmacAllocation(); }
			if ( bu.ops & BUO_SPAWN ) {
				auto fr = FrontendRequest( FrontendRequest::FR_BASE_SPAWN );
				fr.data.base_spawn.base_id = base->m_id;
				fr.data.base_spawn.slot_index = base->m_owner->GetIndex();
				const auto* tile = base->GetTile();
				fr.data.base_spawn.tile_coords = {
					tile->coord.x,
					tile->coord.y
				};
				const auto c = base->GetRenderCoords();
				fr.data.base_spawn.render_coords = {
					c.x,
					c.y,
					c.z
				};
				NEW( fr.data.base_spawn.name, std::string, base->m_name );
				m_game->AddFrontendRequest( fr );
			}
			if ( bu.ops & BUO_REFRESH ) {
				auto fr = FrontendRequest( FrontendRequest::FR_BASE_UPDATE );
				fr.data.base_update.base_id = base->m_id;
				fr.data.base_update.slot_index = base->m_owner->GetIndex();
				NEW( fr.data.base_update.name, std::string, base->m_name );
				NEW( fr.data.base_update.faction_id, std::string, base->m_faction->m_id );
				NEW( fr.data.base_update.pops, FrontendRequest::base_pops_t, {} );
				for ( const auto& it : base->m_pops ) {
					fr.data.base_update.pops->push_back(
						{
							it.second.m_def->m_id,
							it.second.m_variant
						}
					);
				}
				m_game->AddFrontendRequest( fr );
			}
			if ( bu.ops & BUO_DESPAWN ) {
				auto fr = FrontendRequest( FrontendRequest::FR_BASE_DESPAWN );
				fr.data.base_despawn.base_id = base_id;
				m_game->AddFrontendRequest( fr );
			}
		}
		m_base_updates.clear();
	}
}

WRAPIMPL_BEGIN( BaseManager )
	WRAPIMPL_PROPS
	WRAPIMPL_TRIGGERS
		{
			"define_pop",
			NATIVE_CALL( this ) {

				m_game->CheckRW( GSE_CALL );

				N_EXPECT_ARGS( 2 );
				N_GETVALUE( id, 0, String );
				N_GETVALUE( def, 1, Object );

				N_GETPROP( name, def, "name", String );

				render_infos_t rh = {};
				render_infos_t rp = {};
				const auto& f_read_renders = [ &def, &arg, &gc_space, &ctx, &si, &ep, &getprop_val, &obj_it ]( const std::string& key, render_infos_t& out ) {
					N_GETPROP( renders, def, key, Array );
					out.reserve( renders.size() );
					for ( const auto& v : renders ) {
						if ( v->type != gse::VT_OBJECT ) {
							GSE_ERROR( gse::EC.INVALID_CALL, "Pop render elements must be objects" );
						}
						const auto* obj = (gse::value::Object*)v;
						const auto& ov = obj->value;
						N_GETPROP( type, ov, "type", String );
						if ( type == "sprite" ) {
							N_GETPROP( file, ov, "file", String );
							N_GETPROP( x, ov, "x", Int );
							N_GETPROP( y, ov, "y", Int );
							N_GETPROP( w, ov, "w", Int );
							N_GETPROP( h, ov, "h", Int );
							out.push_back({
								file,
								(uint16_t)x,
								(uint16_t)y,
								(uint16_t)w,
								(uint16_t)h
							});
						}
						else {
							GSE_ERROR( gse::EC.INVALID_CALL, "Only sprite pops are supported for now" );
						}

					}
				};
				f_read_renders( "renders_human", rh );
				f_read_renders( "renders_progenitor", rp );

				PopDef::pop_flags_t flags = PopDef::PF_NONE;
				N_GETPROP_OPT( bool, can_work_tiles, def, "tile_worker", Bool, false );
				if ( can_work_tiles ) {
					flags |= PopDef::PF_TILE_WORKER;
				}

				std::optional< uint8_t > specialist;
				if ( def.count( "smac_specialist_id" ) ) {
					N_GETPROP( position, def, "smac_specialist_id", Int );
					if ( position < 0 || position > 6 ) { GSE_ERROR( gse::EC.GAME_ERROR, "Specialist ID must be a CITIZENS position 0..6" ); }
					specialist = static_cast< uint8_t >( position );
				}
				auto candidate = std::make_unique< PopDef >( id, name, rh, rp, flags, specialist );
				DefinePop( candidate.get() ); candidate.release();

				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"undefine_pop",
			NATIVE_CALL( this ) {

				m_game->CheckRW( GSE_CALL );

				N_EXPECT_ARGS( 1 );
				N_GETVALUE( id, 0, String );

				UndefinePop( id );

				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"get_pop_renders",
			NATIVE_CALL( this ) {

				N_EXPECT_ARGS( 1 );
				N_GETVALUE_UNWRAP( player, 0, Player );

				gse::value::object_properties_t defs = {};

				const auto& faction = player->GetFaction();

				for ( const auto& it : m_base_popdefs ) {
					gse::value::array_elements_t variants = {};
					const auto& renders = ( faction->m_flags & faction::Faction::FF_PROGENITOR )
						? it.second->m_renders_progenitor
						: it.second->m_renders_human
					;
					for ( const auto& render : renders ) {
						variants.push_back( VALUE( gse::value::String,, render.file + ":crop(" + std::to_string( render.x ) + "," + std::to_string( render.y ) + "," + std::to_string( render.x + render.width - 1 ) + "," + std::to_string( render.y + render.height - 1 ) + ")" ) );
					}
					defs.insert({ it.first, VALUE( gse::value::Array,, variants ) } );
				}

				return VALUE( gse::value::Object,, GSE_CALL_NOGC, defs );
			} )
		},
		{
			"define_facility",
			NATIVE_CALL( this ) {

				m_game->CheckRW( GSE_CALL );

				N_EXPECT_ARGS( 2 );
				N_GETVALUE( id, 0, String );
				N_GETVALUE( def, 1, Object );

				N_GETPROP( name, def, "name", String );
				N_GETPROP( description, def, "description", String );
				N_GETPROP( cost, def, "cost", Int );

				N_GETPROP( render, def, "render", Object );

				RenderInfo r = {};
				N_GETPROP( type, render, "type", String );
				if ( type == "sprite" ) {
					N_GETPROP( file, render, "file", String );
					N_GETPROP( x, render, "x", Int );
					N_GETPROP( y, render, "y", Int );
					N_GETPROP( w, render, "w", Int );
					N_GETPROP( h, render, "h", Int );
					r = {
						file,
						(uint16_t)x,
						(uint16_t)y,
						(uint16_t)w,
						(uint16_t)h
					};
				}
				else {
					GSE_ERROR( gse::EC.INVALID_CALL, "Only sprite facilities are supported for now" );
				}

				std::optional< int32_t > source;
				if ( def.count( "smac_source_id" ) ) {
					N_GETPROP( source_id, def, "smac_source_id", Int );
					if ( source_id < 1 || source_id > 71 ) { GSE_ERROR( gse::EC.GAME_ERROR, "Facility source ID must be a FACILITIES position 1..71" ); }
					source = static_cast< int32_t >( source_id );
				}
				auto candidate = std::make_unique< FacilityDef >( id, name, description, cost, r, source );
				DefineFacility( candidate.get() ); candidate.release();

				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"undefine_facility",
			NATIVE_CALL( this ) {

				m_game->CheckRW( GSE_CALL );

				N_EXPECT_ARGS( 1 );
				N_GETVALUE( id, 0, String );

				UndefineFacility( id );

				return VALUE( gse::value::Undefined );
			} )
		},

		{
			"spawn_base",
			NATIVE_CALL( this ) {

				m_game->CheckRW( GSE_CALL );

				N_EXPECT_ARGS_MIN_MAX( 3, 4 );
				N_GETVALUE_UNWRAP( owner, 0, Player );
				N_GETVALUE_UNWRAP( tile, 1, map::tile::Tile );

				N_GETVALUE( info, 2, Object );
				N_GETPROP_OPT( std::string, name, info, "name", String, "" );
				N_GETPROP_OPT( bool, starting_context, info, "smac_starting_resource_context", Bool, false );

				std::map< std::string, const FacilityDef* > facilities = {};
				{
					const auto& it = info.find( "facilities" );
					if ( it != info.end() ) {
						if ( it->second->type != gse::VT_ARRAY ) {
							GSE_ERROR( gse::EC.GAME_ERROR, "Facilities must be array, found: " + it->second->GetTypeString() );
						}
						for ( const auto& f : ((gse::value::Array*)it->second)->value ) {
							if ( f->type != gse::VT_STRING ) {
								GSE_ERROR( gse::EC.GAME_ERROR, "Facilities elements must be string IDs, found: " + f->GetTypeString() );
							}
							const auto& id = ((gse::value::String*)f)->value;
							if ( facilities.find( id ) != facilities.end() ) {
								GSE_ERROR( gse::EC.GAME_ERROR, "Duplicate base facility: " + id );
							}
							const auto& f_it = m_base_facilitydefs.find( id );
							if ( f_it == m_base_facilitydefs.end() ) {
								GSE_ERROR( gse::EC.GAME_ERROR, "Unknown base facility: " + id );
							}
							facilities.insert({ id, f_it->second } );
						}
					}
				}

				const auto resource_context = starting_context ? std::optional< SmacBaseResourceContext >( PrepareStartingResourceContext( owner->GetSlot() ) ) : std::nullopt;
				auto* base = new Base(
					m_game,
					Base::GetNextId(),
					owner->GetSlot(),
					owner->GetFaction(),
					tile,
					m_name,
					facilities,
					{}, 1, 0, std::nullopt, resource_context
				);

				SpawnBase( GSE_CALL, base );

				return base->Wrap( GSE_CALL );
			} )
		},
		{
			"despawn_base",
			NATIVE_CALL( this ) {

				m_game->CheckRW( GSE_CALL );

				N_EXPECT_ARGS( 1 );

				if ( arguments.at( 0 )->type == gse::VT_INT ) {
					N_GETVALUE( base_id, 0, Int );
					DespawnBase( GSE_CALL, base_id );
				}
				else {
					N_GETVALUE_UNWRAP( base, 0, Base );
					DespawnBase( GSE_CALL, base->m_id );
				}

				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"get_bases",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				gse::value::array_elements_t arr = {};
				arr.reserve( m_bases.size() );
				for ( const auto& it : m_bases ) {
					arr.push_back( it.second->Wrap( GSE_CALL ) );
				}

				return VALUE( gse::value::Array,, arr );
			} )
		},
	};
WRAPIMPL_END_PTR()

UNWRAPIMPL_PTR( BaseManager )

void BaseManager::TriggerUpdates( GSE_CALLABLE ) {
	std::lock_guard guard( m_updated_bases_mutex );
	m_dispatching_updates = true;
	util::FinallyGuard dispatch( [ this ] { m_dispatching_updates = false; } );
	for ( const auto& base : m_updated_bases ) {
		m_game->Trigger( GSE_CALL, "update_base", ARGS_F( base ) {
			{
				"base",
				base->Wrap( GSE_CALL )
			},
		}; } );
	}
	m_updated_bases.clear();
}

void BaseManager::Serialize( types::Buffer& buf ) const {
	buf.WriteString( s_snapshot_format );

	Log( "Serializing " + std::to_string( m_base_popdefs.size() ) + " base pop defs" );
	buf.WriteInt( m_base_popdefs.size() );
	for ( const auto& it : m_base_popdefs ) {
		buf.WriteString( it.first );
		buf.WriteString( PopDef::Serialize( it.second ).ToString() );
	}

	Log( "Serializing " + std::to_string( m_base_facilitydefs.size() ) + " base facility defs" );
	buf.WriteInt( m_base_facilitydefs.size() );
	for ( const auto& it : m_base_facilitydefs ) {
		buf.WriteString( FacilityDef::Serialize( it.second ).ToString() );
	}

	Log( "Serializing " + std::to_string( m_bases.size() ) + " bases" );
	if ( m_base_order.size() != m_bases.size() ) { THROW( "Base record order differs from registry" ); }
	std::set< size_t > ordered_ids;
	for ( const auto id : m_base_order ) {
		if ( !m_bases.count( id ) || !ordered_ids.insert( id ).second ) { THROW( "Invalid Base record order" ); }
	}
	buf.WriteInt( m_bases.size() );
	for ( const auto id : m_base_order ) {
		buf.WriteString( Base::Serialize( m_bases.at( id ) ).ToString() );
	}
	buf.WriteInt( Base::GetNextId() );

	Log( "Saved next base id: " + std::to_string( Base::GetNextId() ) );
}

void BaseManager::Deserialize( GSE_CALLABLE, types::Buffer& buf ) {
	ASSERT( m_base_popdefs.empty(), "base pop defs not empty" );
	ASSERT( m_base_facilitydefs.empty(), "base facility defs not empty" );
	ASSERT( m_bases.empty(), "bases not empty" );
	ASSERT( m_base_order.empty(), "Base record order not empty" );
	ASSERT( m_unprocessed_bases.empty(), "unprocessed bases not empty" );

	// Unversioned snapshots start with an integer and fail the typed read;
	// unknown versions fail here, before mutating any manager state.
	if ( buf.ReadString() != s_snapshot_format ) {
		THROW( "unsupported base snapshot format (expected " + s_snapshot_format + ")" );
	}

	size_t sz = buf.ReadInt();
	m_base_popdefs.reserve( sz );
	Log( "Unserializing " + std::to_string( sz ) + " base pop defs" );
	for ( size_t i = 0 ; i < sz ; i++ ) {
		const auto name = buf.ReadString();
		auto b = types::Buffer( buf.ReadString() );
		auto definition = std::unique_ptr< PopDef >( PopDef::Deserialize( b ) );
		DefinePop( definition.get() ); definition.release();
	}

	sz = buf.ReadInt();
	m_base_facilitydefs.reserve( sz );
	Log( "Unserializing " + std::to_string( sz ) + " base facility defs" );
	for ( size_t i = 0 ; i < sz ; i++ ) {
		auto b = types::Buffer( buf.ReadString() );
		auto definition = std::unique_ptr< FacilityDef >( FacilityDef::Deserialize( b ) );
		DefineFacility( definition.get() ); definition.release();
	}

	sz = buf.ReadInt();
	Log( "Unserializing " + std::to_string( sz ) + " bases" );
	if ( !m_game->IsRunning() ) {
		m_unprocessed_bases.reserve( sz );
	}
	for ( size_t i = 0 ; i < sz ; i++ ) {
		auto b = types::Buffer( buf.ReadString() );
		if ( m_game->IsRunning() ) {
			RegisterBase( Base::Deserialize( b, m_game ) );
		}
		else {
			m_unprocessed_bases.push_back( b );
		}
	}

	Base::SetNextId( buf.ReadInt() );
	Log( "Restored next base id: " + std::to_string( Base::GetNextId() ) );
}

void BaseManager::RefreshBase( const Base* base ) {
	QueueBaseUpdate( base, BUO_REFRESH );
}

void BaseManager::AddUpdateTrigger( Base* base ) {
	std::lock_guard guard( m_updated_bases_mutex );
	m_updated_bases.insert( base );
}

void BaseManager::GetReachableObjects( std::unordered_set< Object* >& reachable_objects ) {
	gse::GCWrappable::GetReachableObjects( reachable_objects );

	GC_DEBUG_BEGIN( "BaseManager" );

	GC_DEBUG_BEGIN( "bases" );
	for ( const auto& it : m_bases ) {
		it.second->GetReachableObjects( reachable_objects );
	}
	GC_DEBUG_END();

	GC_DEBUG_END();
}

void BaseManager::QueueBaseUpdate( const Base* base, const base_update_op_t op ) {
	auto it = m_base_updates.find( base->m_id );
	if ( it == m_base_updates.end() ) {
		it = m_base_updates.insert(
			{
				base->m_id,
				{
					{},
					base,
				}
			}
		).first;
	}
	auto& update = it->second;
	if ( op == BUO_DESPAWN ) {
		if ( update.ops & BUO_SPAWN ) {
			// if base is despawned immediately after spawning - frontend doesn't need to know
			m_base_updates.erase( it );
			return;
		}
		update.ops = BUO_NONE; // clear other actions if base was despawned
	}
	if ( op == BUO_SPAWN || op == BUO_REFRESH ) {
		if ( update.ops & BUO_DESPAWN ) {
			// do not despawn if it needs to spawn or refresh, i.e. if event was rolled back
			update.ops = (base_update_op_t)( (uint8_t)update.ops & ~BUO_DESPAWN);
			if ( op == BUO_SPAWN ) {
				// if there's pending despawn event it means unit was already spawned, nothing to do
				m_base_updates.erase( it );
				return;
			}
		}
	}
	// add to operations list
	update.ops = (base_update_op_t)( (uint8_t)update.ops | (uint8_t)op );
}

}
}
}

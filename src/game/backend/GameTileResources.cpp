#include "Game.h"

#include <limits>
#include <stdexcept>

#include "base/Base.h"
#include "base/BaseManager.h"
#include "base/FacilityDef.h"
#include "base/SmacBaseResourceContext.h"
#include "map/Map.h"
#include "map/tile/Tiles.h"
#include "resource/ResourceManager.h"
#include "resource/SmacBaseSquareYield.h"
#include "resource/SmacTileNutrients.h"
#include "resource/SmacTileMinerals.h"
#include "resource/SmacTileEnergy.h"
#include "slot/Slot.h"
#include "Player.h"

namespace game::backend {
namespace {
using namespace resource::smac;
using Reference = settings::SmacTechnologyReference::Kind;

// This is the ordinary owned-and-available helper's sign/ownership boundary,
// not recursive research eligibility or a silent Unknown -> Disable conversion.
bool HasConfiguredTechnology( const settings::SmacTechnologyReference& reference,
	const settings::SmacTechnologyRules& rules, const faction::SmacFactionTechnologyState& owner ) {
	if ( reference.kind == Reference::NONE ) { return true; }
	if ( reference.kind == Reference::DISABLED ) { return false; }
	if ( reference.kind != Reference::TECHNOLOGY_ID || !reference.technology_id ) {
		throw std::runtime_error( "Unknown configured SMAC resource technology" );
	}
	const auto id = *reference.technology_id;
	if ( id < 0 || id >= 89 ) { throw std::runtime_error( "Invalid configured SMAC resource technology" ); }
	if ( id == 88 || !owner.owned[ id ] ) { return false; }
	const auto& row = rules.technologies[ id ];
	const auto first = rules.ResolveToken( row.prerequisites[ 0 ] );
	const auto second = rules.ResolveToken( row.prerequisites[ 1 ] );
	if ( first.kind == Reference::UNKNOWN || second.kind == Reference::UNKNOWN ) {
		throw std::runtime_error( "Unknown owned SMAC resource technology prerequisite" );
	}
	return first.kind != Reference::DISABLED && ( second.kind != Reference::DISABLED || first.kind == Reference::NONE );
}

struct FacilityRoles {
	bool headquarters = false, recycling = false, tree_farm = false, hybrid_forest = false, pressure_dome = false, aerospace = false;
};

FacilityRoles ReadFacilityRoles( const base::Base& base, const base::BaseManager& manager ) {
	FacilityRoles result;
	for ( const auto& item : base.m_facilities ) {
		// Compare registered identity before reading a potentially retired Def.
		if ( !item.second || manager.GetFacilityDef( item.first ) != item.second ) {
			throw std::runtime_error( "Canonical resource facility is not registered" );
		}
		manager.ValidateSmacFacilityDef( item.second );
		if ( !item.second->m_smac_source_id ) { throw std::runtime_error( "Canonical resource facility has no positional role" ); }
		const auto id = *item.second->m_smac_source_id;
		// This consumer admits local facilities only. Orbitals, Stockpile and
		// projects are not inferred absent merely from missing query branches.
		if ( id < 1 || id > 33 ) { throw std::runtime_error( "Canonical resource query does not admit orbital/project facilities" ); }
		switch ( id ) {
			case 1: result.headquarters = true; break;
			case 3: result.recycling = true; break;
			case 13: result.tree_farm = true; break;
			case 14: result.hybrid_forest = true; break;
			case 26: result.pressure_dome = true; break;
			case 29: result.aerospace = true; break;
			default: break; // no queried role in these measured resource routines
		}
	}
	return result;
}

void RequireResourceDefinitions( const resource::ResourceManager& manager ) {
	const auto& definitions = manager.GetDefinedResources();
	for ( const auto* id : { "NUTRIENTS", "MINERALS", "ENERGY" } ) {
		if ( !definitions.count( id ) ) { throw std::runtime_error( "Canonical resources require the explicit NUTRIENTS/MINERALS/ENERGY port IDs" ); }
	}
}
}

std::array< int32_t, 3 > Game::GetCanonicalOrbitalIntake( const base::Base& evaluating_base ) const {
	ValidateCurrentSimulation();
	const auto found = m_bm->GetBases().find( evaluating_base.m_id );
	if ( evaluating_base.GetGame() != this || found == m_bm->GetBases().end() || found->second != &evaluating_base ) {
		throw std::runtime_error( "Canonical orbital intake requires a registered Base in this world" );
	}
	const auto context = evaluating_base.GetSmacResourceContext();
	if ( !m_simulation || m_simulation->value.movement_capability != faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain ) {
		throw std::runtime_error( "Canonical orbital intake requires the explicit no-project campaign" );
	}
	bool aerospace = false;
	for ( const auto& entry : m_bm->GetBases() ) {
		if ( !entry.second || entry.second->m_id != entry.first ) { throw std::runtime_error( "Invalid canonical base registry" ); }
		const auto roles = ReadFacilityRoles( *entry.second, *m_bm );
		if ( entry.second == &evaluating_base ) { aerospace = roles.aerospace; }
	}
	// No Space Elevator follows from the saved campaign admission, not from
	// missing project metadata. Registered project/orbital facility rows reject.
	evaluating_base.ValidateSmacAllocation();
	if ( evaluating_base.m_pops.size() > 127 ) { throw std::runtime_error( "Orbital intake population outside 0..127" ); }
	return m_simulation->value.GetSatelliteCounts( context.owner_simulation_id ).Intake( static_cast< int32_t >( evaluating_base.m_pops.size() ), aerospace, false );
}

std::array< int64_t, 3 > Game::GetCanonicalTileResources( const map::tile::Tile& tile, const base::Base& evaluating_base, ResourceQueryPhase phase ) const {
	using namespace map::tile;
	const auto context = evaluating_base.GetSmacResourceContext();
	ValidateCurrentSimulation();
	if ( !m_simulation || !m_map || !m_map->GetResourcePolicy()
		|| m_simulation->value.movement_capability != faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain ) {
		throw std::runtime_error( "Canonical resource query requires saved map semantics and the supported no-project campaign" );
	}
	if ( evaluating_base.GetGame() != this || tile.tiles != m_map->GetTilesPtr()
		|| tile.coord.x >= m_map->GetWidth() || tile.coord.y >= m_map->GetHeight()
		|| ( ( tile.coord.x ^ tile.coord.y ) & 1 ) || m_map->GetTile( tile.coord.x, tile.coord.y ) != &tile ) {
		throw std::runtime_error( "Canonical resource tile belongs to another world" );
	}
	// The original dispatches from the queried tile's center marker. Cross-base
	// context is not part of this adapter's admitted center-query contract.
	if ( tile.base && tile.base != &evaluating_base ) {
		throw std::runtime_error( "Canonical resource query cannot evaluate another Base center" );
	}
	RequireResourceDefinitions( *m_rm );
	const auto& settings = m_state->m_settings.global;
	const auto& rules = settings.GetSmacResourceRules();
	const auto& technologies = settings.GetSmacTechnologyRules();
	const auto& profiles = settings.GetSmacFactionStartupRules();
	if ( !rules || !technologies || !profiles ) { throw std::runtime_error( "Canonical resource profiles are unavailable" ); }
	rules->Validate( *technologies );
	const auto* social = m_simulation->value.social.Find( context.owner_simulation_id );
	const auto* owned = m_simulation->value.technology.Find( context.owner_simulation_id );
	if ( !social || !owned || social->phase != faction::SmacFactionSocialState::Phase::Refreshed ) {
		throw std::runtime_error( "Canonical resource query requires refreshed current faction ratings" );
	}

	// Read only current ratings. A query never performs the first-turn refresh
	// or substitutes retained movement/combat ratings for resource inputs.
	const auto economy = social->current[ 0 ];
	const auto planet = social->current[ 7 ];
	const auto fungus = m_simulation->value.technology.FungalCapabilities( context.owner_simulation_id, *technologies, *profiles );
	const auto refs = rules->TechnologyReferences( *technologies );
	const auto& global = m_simulation->value.resource_modifiers;
	const auto facilities = ReadFacilityRoles( evaluating_base, *m_bm );
	uint32_t base_count = 0;
	for ( const auto& entry : m_bm->GetBases() ) {
		const auto* candidate = entry.second;
		if ( !candidate || candidate->m_id != entry.first ) { throw std::runtime_error( "Invalid canonical base registry" ); }
		const auto candidate_context = candidate->GetSmacResourceContext();
		(void)ReadFacilityRoles( *candidate, *m_bm ); // no hidden remote project authority
		if ( candidate_context.owner_simulation_id == context.owner_simulation_id ) {
			if ( base_count == UINT32_MAX ) { throw std::runtime_error( "Canonical base count exceeds unsigned32" ); }
			++base_count;
		}
	}
	if ( !base_count ) { throw std::runtime_error( "Canonical resource owner has no registered base" ); }

	constexpr feature_t supported_features = FEATURE_RIVER | FEATURE_MONOLITH | FEATURE_XENOFUNGUS | FEATURE_JUNGLE;
	constexpr terraforming_t supported_terraforming = TERRAFORMING_ROAD | TERRAFORMING_MAG_TUBE | TERRAFORMING_FOREST
		| TERRAFORMING_FARM | TERRAFORMING_SOIL_ENRICHER | TERRAFORMING_SOLAR | TERRAFORMING_MINE
		| TERRAFORMING_CONDENSER | TERRAFORMING_BOREHOLE | TERRAFORMING_SENSOR | TERRAFORMING_BUNKER | TERRAFORMING_AIRBASE;
	if ( ( tile.features & ~supported_features ) || ( tile.terraforming & ~supported_terraforming ) ) {
		throw std::runtime_error( "Canonical resource tile has unsupported landmark/ownership metadata" );
	}
	if ( tile.moisture < MOISTURE_ARID || tile.moisture > MOISTURE_RAINY
		|| tile.rockiness < ROCKINESS_FLAT || tile.rockiness > ROCKINESS_ROCKY || !tile.elevation.center ) {
		throw std::runtime_error( "Canonical resource terrain has unknown moisture/rockiness/elevation" );
	}
	const auto& policy = *m_map->GetResourcePolicy();
	const auto altitude = policy.Altitude( tile.is_water_tile, *tile.elevation.center );
	const auto bonus = policy.Bonus( tile );
	const auto feature = [ & ]( feature_t bit ) { return ( tile.features & bit ) != 0; };
	const auto improved = [ & ]( terraforming_t bit ) { return ( tile.terraforming & bit ) != 0; };
	const bool nutrient_landmark = !tile.is_water_tile && feature( FEATURE_JUNGLE );
	using Events = base::SmacBaseResourceContext;
	const bool dust = global.atmospheric_dust_countdown != 0;
	// Ordinary display/intake evaluation is explicit. Upkeep's separate solar
	// flag is not selected by whether the query happens to run during a turn.
	bool solar;
	switch ( phase ) {
		case ResourceQueryPhase::Ordinary: solar = global.solar_ordinary; break;
		case ResourceQueryPhase::Upkeep: solar = global.solar_upkeep; break;
		default: throw std::runtime_error( "Unknown canonical resource query phase" );
	}
	if ( evaluating_base.GetTile() == &tile ) {
		return CalculateBaseSquareYield( rules->BaseSquare(), {
			{ !tile.is_water_tile, bonus, { nutrient_landmark, false, false }, feature( FEATURE_RIVER ) },
			{ facilities.headquarters, facilities.recycling, facilities.pressure_dome },
			{ economy, context.golden_age, base_count, global.planetary_governor == context.owner_simulation_id, false },
			{ { context.HasEvent( Events::BumperCrop ), context.HasEvent( Events::IndustrialBoom ), context.HasEvent( Events::HeatWave ) },
			  { context.HasEvent( Events::Famine ), context.HasEvent( Events::IndustrialBust ), context.HasEvent( Events::CloudCover ) }, dust, solar }
		} );
	}
	// Mirror affiliation is not represented by the port Tile. Refuse any
	// relevant neighboring mirror instead of manufacturing an owner/count.
	if ( improved( TERRAFORMING_SOLAR ) ) {
		for ( const auto* neighbor : tile.neighbours ) {
			if ( !neighbor || ( neighbor->terraforming & TERRAFORMING_MIRROR ) ) {
				throw std::runtime_error( "Canonical collector requires resolved mirror affiliation" );
			}
		}
	}
	const bool forest = improved( TERRAFORMING_FOREST ), fungal = feature( FEATURE_XENOFUNGUS );
	const bool borehole = improved( TERRAFORMING_BOREHOLE ), monolith = feature( FEATURE_MONOLITH );
	const auto n = CalculateTileNutrients( rules->TileNutrients(), {
		{ altitude, static_cast< Moisture >( tile.moisture - MOISTURE_ARID ), static_cast< Rockiness >( tile.rockiness - ROCKINESS_FLAT ), bonus, nutrient_landmark, forest, fungal, borehole, monolith },
		{ improved( TERRAFORMING_FARM ), improved( TERRAFORMING_MINE ), improved( TERRAFORMING_SOIL_ENRICHER ), improved( TERRAFORMING_CONDENSER ) },
		{ facilities.tree_farm, facilities.hybrid_forest, HasConfiguredTechnology( refs[ 0 ], *technologies, *owned ), fungus.nutrients, planet,
			context.HasEvent( Events::BumperCrop ), context.HasEvent( Events::Famine ) }
	} );
	const auto m = CalculateTileMinerals( rules->TileMinerals(), {
		{ altitude, tile.rockiness - ROCKINESS_FLAT, bonus, false, forest, fungal, borehole, monolith },
		{ improved( TERRAFORMING_MINE ), improved( TERRAFORMING_ROAD ) },
		{ HasConfiguredTechnology( refs[ 1 ], *technologies, *owned ), HasConfiguredTechnology( refs[ 3 ], *technologies, *owned ), fungus.minerals, planet,
			context.HasEvent( Events::IndustrialBoom ), context.HasEvent( Events::IndustrialBust ) }
	} );
	const auto e = CalculateTileEnergy( rules->TileEnergy(), {
		{ altitude, bonus, false, forest, fungal, monolith, borehole, feature( FEATURE_RIVER ) },
		{ improved( TERRAFORMING_SOLAR ), false, 0 },
		{ facilities.hybrid_forest, HasConfiguredTechnology( refs[ 2 ], *technologies, *owned ), fungus.energy, planet, economy, context.golden_age, false,
			context.HasEvent( Events::HeatWave ), context.HasEvent( Events::CloudCover ), dust, solar }
	} );
	return { n.nutrients, m.minerals, e.energy };
}

int64_t Game::GetCanonicalNutrientDemand( const base::Base& base ) const {
	(void)base.GetSmacResourceContext();
	ValidateCurrentSimulation();
	const auto& rules = m_state->m_settings.global.GetSmacResourceRules();
	if ( base.GetGame() != this || !rules ) { throw std::runtime_error( "Canonical nutrient demand requires this world's frozen resource profile" ); }
	const int64_t rate = rules->nutrients_per_citizen;
	if ( !rate ) { return 0; }
	const uint64_t magnitude = rate < 0 ? static_cast< uint64_t >( -rate ) : static_cast< uint64_t >( rate );
	if ( magnitude && base.m_pops.size() > static_cast< uint64_t >( INT64_MAX ) / magnitude ) {
		throw std::overflow_error( "Canonical nutrient demand exceeds signed64" );
	}
	return rate * static_cast< int64_t >( base.m_pops.size() );
}

}

#include "UnitDesignResolver.h"

#include <stdexcept>
#include <unordered_map>

namespace loader::txt {
namespace {

using Index = std::unordered_map< std::string, size_t >;

template< typename Component, typename Key >
Index BuildIndex( const std::vector< Component >& components, const Key& key, const std::string& section ) {
	Index result;
	for ( size_t i = 0 ; i < components.size() ; ++i ) {
		const auto& name = key( components[ i ] );
		const auto location = "#" + section + " row " + std::to_string( i + 1 );
		if ( name.empty() || name.find( '\0' ) != std::string::npos ) {
			throw std::invalid_argument( location + ": missing or invalid component key" );
		}
		if ( !result.emplace( name, i ).second ) {
			throw std::invalid_argument( location + ": ambiguous duplicate component key " + name );
		}
	}
	return result;
}

size_t Lookup( const Index& index, const std::string& key, const size_t source_index, const std::string& field ) {
	const auto found = index.find( key );
	if ( found == index.end() ) {
		// The count occupies data row 1 in the original #UNITS section.
		throw std::invalid_argument( "#UNITS row " + std::to_string( source_index + 2 ) + ": unresolved " + field + " reference " + key );
	}
	return found->second;
}

}

ResolvedUnitTable UnitDesignResolver::Resolve( const UnitTable& units, const ChassisTable& chassis,
	const WeaponTable& weapons, const DefenseTable& defenses ) {
	if ( units.ruleset_id != "smac" && units.ruleset_id != "smax" ) {
		throw std::invalid_argument( "Unit resolution requires an explicit smac or smax ruleset" );
	}
	if ( chassis.ruleset_id != units.ruleset_id || weapons.ruleset_id != units.ruleset_id || defenses.ruleset_id != units.ruleset_id ) {
		throw std::invalid_argument( "Unit and component tables must have matching ruleset identities" );
	}
	const auto chassis_index = BuildIndex( chassis.chassis, []( const ChassisComponent& component ) -> const std::string& { return component.names[ 0 ].name; }, "CHASSIS" );
	const auto weapon_index = BuildIndex( weapons.weapons, []( const WeaponComponent& component ) -> const std::string& { return component.short_name; }, "WEAPONS" );
	const auto defense_index = BuildIndex( defenses.defenses, []( const DefenseComponent& component ) -> const std::string& { return component.short_name; }, "DEFENSES" );
	ResolvedUnitTable result{ units.ruleset_id, {} };
	for ( size_t i = 0 ; i < units.units.size() ; ++i ) {
		const auto& unit = units.units[ i ];
		const auto& resolved_chassis = chassis.chassis[ Lookup( chassis_index, unit.chassis, i, "chassis" ) ];
		const auto& resolved_weapon = weapons.weapons[ Lookup( weapon_index, unit.weapon, i, "weapon" ) ];
		const auto& resolved_defense = defenses.defenses[ Lookup( defense_index, unit.defense, i, "defense" ) ];
		result.units.push_back( { i, unit, resolved_chassis, resolved_weapon, resolved_defense } );
	}
	return result;
}

}

// Opt-in Release probe. Installed rules and original reports stay outside Git.
#include "game/backend/unit/SmacPredefinedUnitCatalog.h"
#include "loader/txt/SmacRulesTXTLoader.h"
#include "loader/txt/UnitRulesTXTLoader.h"

#include <charconv>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>

using namespace loader::txt;
using Catalog = game::backend::unit::SmacPredefinedUnitCatalog;

int32_t Integer( const std::string& text ) {
	int32_t value = 0; const auto result = std::from_chars( text.data(), text.data() + text.size(), value );
	if ( result.ec != std::errc() || result.ptr != text.data() + text.size() ) { throw std::invalid_argument( "Invalid probe integer" ); }
	return value;
}

template< typename T, typename Key >
size_t Find( const std::vector< T >& rows, const std::string& name, const Key& key ) {
	for ( size_t i = 0; i < rows.size(); ++i ) { if ( key( rows[ i ] ) == name ) { return i; } }
	throw std::invalid_argument( "Unknown probe component" );
}

int main( const int argc, const char** argv ) {
	if ( argc != 3 ) { std::cerr << "usage: probe catalog|cost|capacity ALPHA\n"; return 2; }
	try {
		std::ifstream file( argv[ 2 ], std::ios::binary ); if ( !file ) { throw std::runtime_error( "Cannot read explicit alpha file" ); }
		const std::string source{ std::istreambuf_iterator< char >( file ), {} };
		auto units = UnitRulesTXTLoader::ParseUnits( source, "smac" );
		auto chassis = UnitRulesTXTLoader::ParseChassis( source, "smac" );
		auto weapons = UnitRulesTXTLoader::ParseWeapons( source, "smac" );
		auto defenses = UnitRulesTXTLoader::ParseDefenses( source, "smac" );
		auto reactors = UnitRulesTXTLoader::ParseReactors( source, "smac" );
		auto abilities = UnitRulesTXTLoader::ParseAbilities( source, "smac" );
		const auto technologies = SmacRulesTXTLoader::ParseTechnologyRules( source );
		const std::string mode = argv[ 1 ]; std::ostringstream output;
		if ( mode == "catalog" ) {
			size_t count = 0;
			if ( !( std::cin >> count ) || count > 1024 ) { throw std::invalid_argument( "Invalid probe change count" ); }
			for ( size_t i = 0; i < count; ++i ) {
				std::string kind, key, field, value;
				if ( !( std::cin >> kind >> std::quoted( key ) >> field >> std::quoted( value ) ) ) { throw std::invalid_argument( "Incomplete probe change" ); }
				if ( kind == "unit" ) {
					const auto index = Integer( key ); if ( index < 0 || static_cast< size_t >( index ) >= units.units.size() ) { throw std::invalid_argument( "Invalid unit index" ); }
					auto& row = units.units[ index ];
					if ( field == "plan" ) { row.plan = static_cast< UnitPlan >( Integer( value ) ); }
					else if ( field == "cost" ) { row.cost = Integer( value ); }
					else if ( field == "carry" ) { row.carry = Integer( value ); }
					else if ( field == "icon" ) { row.icon = Integer( value ); }
					else if ( field == "prerequisite" ) { row.prerequisite = value; }
					else if ( field == "ability_binary" ) { row.ability_bits = value; }
					else if ( field == "weapon" ) { row.weapon = value; }
					else { throw std::invalid_argument( "Unsupported unit probe field" ); }
				} else if ( kind == "capacity" && field == "cargo" ) {
					const auto index = Find( chassis.chassis, key, []( const auto& row ) { return row.names[ 0 ].name; } );
					chassis.chassis[ index ].cargo = Integer( value );
				} else { throw std::invalid_argument( "Unsupported probe change" ); }
			}
			const auto result = Catalog::Build( units, chassis, weapons, defenses, reactors, abilities, technologies );
			output << '[';
			for ( size_t i = 0; i < result.units.size(); ++i ) {
				const auto& row = result.units[ i ]; using Kind = game::backend::settings::SmacTechnologyReference::Kind;
				const int32_t prerequisite = row.prerequisite.kind == Kind::NONE ? -1 : row.prerequisite.kind == Kind::DISABLED ? -2
					: row.prerequisite.kind == Kind::UNKNOWN ? -3 : *row.prerequisite.technology_id;
				if ( i ) { output << ','; }
				output << '[' << row.source_id << ',' << row.chassis_id << ',' << row.weapon_id << ',' << row.defense_id << ',' << unsigned( row.reactor )
					<< ',' << row.abilities << ',' << unsigned( row.automatic_plan ) << ',' << unsigned( row.calculated_cost ) << ',' << unsigned( row.calculated_carry )
					<< ',' << unsigned( row.plan ) << ',' << unsigned( row.cost ) << ',' << unsigned( row.carry ) << ',' << unsigned( row.icon )
					<< ',' << prerequisite << ',' << row.active << ',' << row.prototyped << ',' << row.faction_exclusion << ']';
			}
			output << "]\n";
		} else if ( mode == "cost" || mode == "capacity" ) {
			std::string name; uint32_t reactor = 0, mask = 0;
			if ( !( std::cin >> std::quoted( name ) >> reactor >> mask ) ) { throw std::invalid_argument( "Incomplete helper inputs" ); }
			auto c = chassis.chassis[ Find( chassis.chassis, name, []( const auto& row ) { return row.names[ 0 ].name; } ) ];
			if ( mode == "capacity" ) {
				int32_t cargo = -1; if ( !( std::cin >> cargo ) ) { throw std::invalid_argument( "Missing cargo input" ); }
				if ( cargo >= 0 ) { c.cargo = cargo; }
				output << Catalog::CalculateCapacity( c, reactor, mask ) << '\n';
			} else {
				std::string weapon_name, defense_name; int32_t ability_index, ability_cost, land_flag;
				if ( !( std::cin >> std::quoted( weapon_name ) >> std::quoted( defense_name ) >> ability_index >> ability_cost >> land_flag ) ) { throw std::invalid_argument( "Incomplete cost inputs" ); }
				if ( ability_index >= 0 ) {
					if ( ability_index >= 24 || ( land_flag != -1 && land_flag != 0 && land_flag != 1 ) ) { throw std::invalid_argument( "Invalid ability override" ); }
					auto& ability = abilities.abilities[ ability_index ];
					if ( ability_cost != INT32_MIN ) { ability.cost = ability_cost; }
					if ( land_flag >= 0 ) {
						if ( ability.flags.size() != 11 ) { throw std::invalid_argument( "Flag override requires 11 source bits" ); }
						ability.flags[ 0 ] = land_flag ? '1' : '0';
					}
				}
				const auto& w = weapons.weapons[ Find( weapons.weapons, weapon_name, []( const auto& row ) { return row.short_name; } ) ];
				const auto& d = defenses.defenses[ Find( defenses.defenses, defense_name, []( const auto& row ) { return row.short_name; } ) ];
				output << Catalog::CalculateCost( c, w, d, reactor, mask, abilities ) << '\n';
			}
		} else { return 2; }
		std::cin >> std::ws; if ( !std::cin.eof() ) { throw std::invalid_argument( "Unexpected trailing probe input" ); }
		std::cout << output.str(); return 0;
	} catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
}

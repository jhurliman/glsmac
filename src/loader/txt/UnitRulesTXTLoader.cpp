#include "UnitRulesTXTLoader.h"

#include "TXTLoader.h"
#include "util/String.h"

#include <charconv>
#include <stdexcept>
#include <unordered_set>

namespace loader::txt {
namespace {

std::string Location( const std::string& section, const size_t row, const size_t column ) {
	return "#" + section + " row " + std::to_string( row ) + " column " + std::to_string( column );
}

int32_t Integer( const std::string& field, const std::string& section, const size_t row, const size_t column ) {
	const auto* begin = field.data();
	const auto* end = begin + field.size();
	if ( begin != end && *begin == '+' ) {
		++begin;
		if ( begin == end || *begin < '0' || *begin > '9' ) {
			throw std::invalid_argument( Location( section, row, column ) + ": invalid signed int32" );
		}
	}
	int32_t result = 0;
	const auto parsed = std::from_chars( begin, end, result );
	if ( parsed.ec != std::errc() || parsed.ptr != end ) {
		throw std::invalid_argument( Location( section, row, column ) + ": invalid signed int32" );
	}
	return result;
}

void RequiredText( const std::string& field, const std::string& section, const size_t row, const size_t column ) {
	if ( field.empty() || field.find( '\0' ) != std::string::npos ) {
		throw std::invalid_argument( Location( section, row, column ) + ": missing or invalid text" );
	}
}

std::vector< std::string > Rows( const std::string& source, const std::string& ruleset_id, const std::string& section ) {
	if ( ruleset_id != "smac" && ruleset_id != "smax" ) {
		throw std::invalid_argument( "Component table requires an explicit smac or smax ruleset" );
	}
	auto rows = TXTLoader::ReadUniqueSection( source, section );
	if ( rows.empty() ) { throw std::invalid_argument( "#" + section + " must not be empty" ); }
	return rows;
}

std::vector< std::string > Fields( const std::string& source, const std::string& section, const size_t row, const size_t count ) {
	const auto line = util::String::TrimCopy( source.substr( 0, source.find( ';' ) ) );
	auto fields = util::String::Split( line, ',' );
	// Split omits the final empty field; retain it so exactly one optional
	// trailing delimiter is accepted without hiding extra empty columns.
	if ( !line.empty() && line.back() == ',' ) { fields.emplace_back(); }
	if ( fields.size() == count + 1 && fields.back().empty() ) { fields.pop_back(); }
	if ( fields.size() != count ) {
		throw std::invalid_argument( "#" + section + " row " + std::to_string( row ) + ": expected " + std::to_string( count ) + " fields" );
	}
	for ( const size_t column : { size_t( 0 ), size_t( 1 ), count - 1 } ) { RequiredText( fields[ column ], section, row, column + 1 ); }
	return fields;
}

void UniqueShortName( std::unordered_set< std::string >& names, const std::string& name, const std::string& section, const size_t row ) {
	if ( !names.insert( name ).second ) {
		throw std::invalid_argument( Location( section, row, 2 ) + ": duplicate short name " + name );
	}
}

}

WeaponTable UnitRulesTXTLoader::ParseWeapons( const std::string& source, const std::string& ruleset_id ) {
	const std::string section = "WEAPONS";
	const auto rows = Rows( source, ruleset_id, section );
	WeaponTable result{ ruleset_id, {} };
	std::unordered_set< std::string > short_names;
	for ( size_t i = 0 ; i < rows.size() ; ++i ) {
		const auto row = i + 1;
		const auto fields = Fields( rows[ i ], section, row, 7 );
		const auto offense = Integer( fields[ 2 ], section, row, 3 );
		const auto mode = Integer( fields[ 3 ], section, row, 4 );
		const auto cost = Integer( fields[ 4 ], section, row, 5 );
		const auto icon = Integer( fields[ 5 ], section, row, 6 );
		if ( !( ( mode >= 0 && mode <= 2 ) || ( mode >= 7 && mode <= 12 ) ||
			( ruleset_id == "smax" && ( mode == 13 || mode == 14 ) ) ) ) {
			throw std::invalid_argument( Location( section, row, 4 ) + ": unsupported mode for " + ruleset_id );
		}
		UniqueShortName( short_names, fields[ 1 ], section, row );
		result.weapons.push_back( { fields[ 0 ], fields[ 1 ], offense, static_cast< WeaponMode >( mode ), cost, icon, fields[ 6 ] } );
	}
	return result;
}

DefenseTable UnitRulesTXTLoader::ParseDefenses( const std::string& source, const std::string& ruleset_id ) {
	const std::string section = "DEFENSES";
	const auto rows = Rows( source, ruleset_id, section );
	DefenseTable result{ ruleset_id, {} };
	std::unordered_set< std::string > short_names;
	for ( size_t i = 0 ; i < rows.size() ; ++i ) {
		const auto row = i + 1;
		const auto fields = Fields( rows[ i ], section, row, 6 );
		const auto rating = Integer( fields[ 2 ], section, row, 3 );
		const auto mode = Integer( fields[ 3 ], section, row, 4 );
		const auto cost = Integer( fields[ 4 ], section, row, 5 );
		if ( mode < 0 || mode > 2 ) {
			throw std::invalid_argument( Location( section, row, 4 ) + ": unsupported defense mode" );
		}
		UniqueShortName( short_names, fields[ 1 ], section, row );
		result.defenses.push_back( { fields[ 0 ], fields[ 1 ], rating, static_cast< DefenseMode >( mode ), cost, fields[ 5 ] } );
	}
	return result;
}

ReactorTable UnitRulesTXTLoader::ParseReactors( const std::string& source, const std::string& ruleset_id ) {
	const std::string section = "REACTORS";
	const auto rows = Rows( source, ruleset_id, section );
	ReactorTable result{ ruleset_id, {} };
	std::unordered_set< std::string > short_names;
	for ( size_t i = 0 ; i < rows.size() ; ++i ) {
		const auto row = i + 1;
		const auto fields = Fields( rows[ i ], section, row, 4 );
		const auto power = Integer( fields[ 2 ], section, row, 3 );
		UniqueShortName( short_names, fields[ 1 ], section, row );
		result.reactors.push_back( { fields[ 0 ], fields[ 1 ], power, fields[ 3 ] } );
	}
	return result;
}

ChassisTable UnitRulesTXTLoader::ParseChassis( const std::string& source, const std::string& ruleset_id ) {
	const std::string section = "CHASSIS";
	const auto rows = Rows( source, ruleset_id, section );
	ChassisTable result{ ruleset_id, {} };
	std::unordered_set< std::string > primary_names;
	for ( size_t i = 0 ; i < rows.size() ; ++i ) {
		const auto row = i + 1;
		const auto fields = Fields( rows[ i ], section, row, 19 );
		std::array< ChassisName, 6 > names;
		for ( size_t pair = 0 ; pair < names.size() ; ++pair ) {
			const size_t column = pair < 4 ? pair * 2 : 15 + ( pair - 4 ) * 2;
			RequiredText( fields[ column ], section, row, column + 1 );
			const auto& grammar = fields[ column + 1 ];
			if ( grammar.size() != 2 || ( grammar[ 0 ] != 'M' && grammar[ 0 ] != 'F' && grammar[ 0 ] != 'N' ) ||
				( grammar[ 1 ] != '1' && grammar[ 1 ] != '2' ) ) {
				throw std::invalid_argument( Location( section, row, column + 2 ) + ": unsupported name grammar" );
			}
			names[ pair ] = { fields[ column ], grammar };
		}
		RequiredText( fields[ 14 ], section, row, 15 );
		const auto speed = Integer( fields[ 8 ], section, row, 9 );
		const auto triad = Integer( fields[ 9 ], section, row, 10 );
		const auto range = Integer( fields[ 10 ], section, row, 11 );
		const auto missile = Integer( fields[ 11 ], section, row, 12 );
		const auto cargo = Integer( fields[ 12 ], section, row, 13 );
		const auto cost = Integer( fields[ 13 ], section, row, 14 );
		if ( triad < 0 || triad > 2 ) {
			throw std::invalid_argument( Location( section, row, 10 ) + ": unsupported chassis triad" );
		}
		if ( missile != 0 && missile != 1 ) {
			throw std::invalid_argument( Location( section, row, 12 ) + ": missile flag must be 0 or 1" );
		}
		if ( !primary_names.insert( names[ 0 ].name ).second ) {
			throw std::invalid_argument( Location( section, row, 1 ) + ": duplicate primary chassis name" );
		}
		result.chassis.push_back( { names, speed, static_cast< ChassisTriad >( triad ), range, missile == 1, cargo, cost, fields[ 14 ] } );
	}
	return result;
}

}

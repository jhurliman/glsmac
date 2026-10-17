#include "UnitRulesTXTLoader.h"

#include "TXTLoader.h"
#include "util/String.h"

#include <charconv>
#include <stdexcept>
#include <unordered_set>

namespace loader::txt {
namespace {

std::string Location( const size_t row, const size_t column ) {
	return "#WEAPONS row " + std::to_string( row ) + " column " + std::to_string( column );
}

int32_t Integer( const std::string& field, const size_t row, const size_t column ) {
	const auto* begin = field.data();
	const auto* end = begin + field.size();
	if ( begin != end && *begin == '+' ) {
		++begin;
		if ( begin == end || *begin < '0' || *begin > '9' ) {
			throw std::invalid_argument( Location( row, column ) + ": invalid signed int32" );
		}
	}
	int32_t result = 0;
	const auto parsed = std::from_chars( begin, end, result );
	if ( parsed.ec != std::errc() || parsed.ptr != end ) {
		throw std::invalid_argument( Location( row, column ) + ": invalid signed int32" );
	}
	return result;
}

void RequiredText( const std::string& field, const size_t row, const size_t column ) {
	if ( field.empty() || field.find( '\0' ) != std::string::npos ) {
		throw std::invalid_argument( Location( row, column ) + ": missing or invalid text" );
	}
}

}

WeaponTable UnitRulesTXTLoader::ParseWeapons( const std::string& source, const std::string& ruleset_id ) {
	if ( ruleset_id != "smac" && ruleset_id != "smax" ) {
		throw std::invalid_argument( "Weapon table requires an explicit smac or smax ruleset" );
	}
	const auto rows = TXTLoader::ReadUniqueSection( source, "WEAPONS" );
	if ( rows.empty() ) { throw std::invalid_argument( "#WEAPONS must not be empty" ); }
	WeaponTable result{ ruleset_id, {} };
	std::unordered_set< std::string > short_names;
	for ( size_t i = 0 ; i < rows.size() ; ++i ) {
		const auto row = i + 1;
		const auto line = util::String::TrimCopy( rows[ i ].substr( 0, rows[ i ].find( ';' ) ) );
		auto fields = util::String::Split( line, ',' );
		// Split omits the final empty field; retain it so exactly one optional
		// trailing delimiter is accepted without hiding extra empty columns.
		if ( !line.empty() && line.back() == ',' ) { fields.emplace_back(); }
		if ( fields.size() == 8 && fields.back().empty() ) { fields.pop_back(); }
		if ( fields.size() != 7 ) {
			throw std::invalid_argument( "#WEAPONS row " + std::to_string( row ) + ": expected seven fields" );
		}
		for ( const size_t column : { 0, 1, 6 } ) { RequiredText( fields[ column ], row, column + 1 ); }
		const auto offense = Integer( fields[ 2 ], row, 3 );
		const auto mode = Integer( fields[ 3 ], row, 4 );
		const auto cost = Integer( fields[ 4 ], row, 5 );
		const auto icon = Integer( fields[ 5 ], row, 6 );
		if ( !( ( mode >= 0 && mode <= 2 ) || ( mode >= 7 && mode <= 12 ) ||
			( ruleset_id == "smax" && ( mode == 13 || mode == 14 ) ) ) ) {
			throw std::invalid_argument( Location( row, 4 ) + ": unsupported mode for " + ruleset_id );
		}
		if ( !short_names.insert( fields[ 1 ] ).second ) {
			throw std::invalid_argument( Location( row, 2 ) + ": duplicate short name " + fields[ 1 ] );
		}
		result.weapons.push_back( { fields[ 0 ], fields[ 1 ], offense, static_cast< WeaponMode >( mode ), cost, icon, fields[ 6 ] } );
	}
	return result;
}

}

#include "SmacSpecialists.h"

#include <stdexcept>

namespace game::backend::base {
namespace {
using References = std::array< std::array< settings::SmacTechnologyReference, 2 >, 7 >;

References Validate( const settings::SmacSpecialistRules& rules, const settings::SmacTechnologyRules& technologies,
	uint32_t population ) {
	if ( population > 127 ) { throw std::invalid_argument( "SMAC specialist query population exceeds admitted 0..127 domain" ); }
	auto result = rules.TechnologyReferences( technologies );
	for ( const auto& row : result ) {
		for ( const auto& reference : row ) {
			if ( reference.kind == settings::SmacTechnologyReference::Kind::UNKNOWN ) {
				throw std::invalid_argument( "Unknown SMAC specialist technology token" );
			}
		}
	}
	return result;
}

bool Available( const settings::SmacTechnologyReference& reference, const std::array< bool, 89 >& owned, const settings::SmacTechnologyRules& technologies ) {
	using Kind = settings::SmacTechnologyReference::Kind;
	switch ( reference.kind ) {
		case Kind::NONE: return true;
		case Kind::DISABLED: return false;
		case Kind::TECHNOLOGY_ID: {
			const auto id = *reference.technology_id;
			// The specialist callers use the ordinary owned-and-available helper,
			// not raw possession. Future resolves but is not an ordinary reference.
			if ( id < 0 || id >= 88 ) { return false; }
			const auto& definition = technologies.technologies[ id ];
			const auto first = technologies.ResolveToken( definition.prerequisites[ 0 ] );
			const auto second = technologies.ResolveToken( definition.prerequisites[ 1 ] );
			if ( first.kind == Kind::UNKNOWN || second.kind == Kind::UNKNOWN ) { throw std::invalid_argument( "Unknown SMAC specialist technology definition prerequisite" ); }
			if ( first.kind == Kind::DISABLED || ( second.kind == Kind::DISABLED && first.kind != Kind::NONE ) ) { return false; }
			return owned.at( id ); // parent possession is not recursively required
		}
		default: throw std::invalid_argument( "Unknown SMAC specialist technology token" );
	}
}

int32_t Checked( int64_t value ) {
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::overflow_error( "SMAC automatic specialist score exceeds signed32" ); }
	return static_cast< int32_t >( value );
}
}

SmacSpecialistMenu QuerySmacSpecialistMenu( const settings::SmacSpecialistRules& rules,
	const settings::SmacTechnologyRules& technologies, const std::array< bool, 89 >& owned,
	uint32_t population, std::optional< uint8_t > current ) {
	const auto references = Validate( rules, technologies, population );
	if ( current && *current >= 7 ) { throw std::invalid_argument( "SMAC specialist current type exceeds positional catalog" ); }
	SmacSpecialistMenu result{ current, {}, false };
	for ( uint8_t id = 0; id < 7; ++id ) {
		if ( !Available( references[ id ][ 0 ], owned, technologies ) || ( Available( references[ id ][ 1 ], owned, technologies ) && current != id ) ) { continue; }
		if ( int64_t( population ) >= rules.minimum_base_size || rules.specialists[ id ].psych >= 2 ) { result.eligible[ id ] = true; }
		else { result.size_explanation_requested = true; }
	}
	return result;
}

uint8_t SelectAutomaticSmacSpecialist( const settings::SmacSpecialistRules& rules,
	const settings::SmacTechnologyRules& technologies, const std::array< bool, 89 >& owned, uint32_t population ) {
	const auto references = Validate( rules, technologies, population );
	int32_t best_score = 0;
	std::optional< uint8_t > best;
	for ( uint8_t id = 0; id < 7; ++id ) {
		if ( !Available( references[ id ][ 0 ], owned, technologies ) ) { continue; }
		const auto& row = rules.specialists[ id ];
		auto score = Checked( int64_t( row.psych ) * 3 );
		if ( int64_t( population ) >= rules.minimum_base_size ) {
			score = Checked( int64_t( score ) + row.economy );
			score = Checked( int64_t( score ) + row.labs );
		}
		// Obsolescence is intentionally absent: the later replacement pass is
		// separate, and equal scores retain the earlier positional record.
		if ( score > best_score ) { best_score = score; best = id; }
	}
	if ( !best ) { throw std::invalid_argument( "SMAC automatic specialist requires an available positive score" ); }
	return *best;
}

}

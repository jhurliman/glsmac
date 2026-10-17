#include "SmacWorkerAvailability.h"

#include <set>
#include <stdexcept>

namespace game::backend::base {
namespace {
void PlayerOwner( int32_t owner ) {
	if ( owner < 1 || owner > 7 ) { throw std::invalid_argument( "SMAC worker owner must be a playable simulation identity" ); }
}
}

bool SmacWorkerUnitPresence::operator==( const SmacWorkerUnitPresence& other ) const noexcept {
	return owner == other.owner && visible_to_evaluator == other.visible_to_evaluator && active_convoy == other.active_convoy;
}

bool SmacWorkerAvailabilitySquare::operator==( const SmacWorkerAvailabilitySquare& other ) const noexcept {
	return explored == other.explored && base_marker == other.base_marker && sea == other.sea
		&& recognized_owner == other.recognized_owner && units == other.units;
}

uint32_t SmacWorkerAvailability::Build( const SmacWorkerAvailabilityContext& c, const Squares& squares,
	const std::vector< SmacWorkerOtherBase >& other_bases ) {
	PlayerOwner( c.owner );
	if ( c.pact[ 0 ] || c.treaty[ 0 ] ) {
		throw std::invalid_argument( "Native units cannot be SMAC diplomacy partners" );
	}
	std::array< std::optional< std::array< int32_t, 2 > >, 21 > coordinates;
	for ( int32_t i = 0; i <= 20; ++i ) {
		coordinates[ i ] = SmacWorkedSquareCollection::Resolve( c.map, i );
		if ( bool( coordinates[ i ] ) != bool( squares[ i ] ) ) {
			throw std::invalid_argument( "SMAC worker square does not match radius clipping" );
		}
		if ( !squares[ i ] ) { continue; }
		const auto& square = *squares[ i ];
		if ( square.recognized_owner ) { PlayerOwner( *square.recognized_owner ); }
		for ( const auto& unit : square.units ) {
			if ( unit.owner < 0 || unit.owner > 7 ) { throw std::invalid_argument( "Invalid SMAC worker unit owner" ); }
		}
		for ( int32_t previous = 0; previous < i; ++previous ) {
			if ( coordinates[ previous ] == coordinates[ i ] && !( *squares[ previous ] == square ) ) {
				throw std::invalid_argument( "Conflicting SMAC worker coordinate aliases" );
			}
		}
	}
	std::set< uint64_t > ids;
	for ( const auto& base : other_bases ) {
		PlayerOwner( base.owner );
		if ( !ids.insert( base.id ).second ) { throw std::invalid_argument( "Duplicate SMAC worker base identity" ); }
		(void)SmacWorkedSquareCollection::Resolve( { base.center, c.map.width, c.map.height, c.map.horizontal_wrap }, 0 );
		if ( base.id == c.base_id && ( base.owner != c.owner || base.center != c.map.center ) ) {
			throw std::invalid_argument( "Conflicting SMAC evaluating base identity" );
		}
	}

	uint32_t unavailable = 0;
	for ( int32_t i = 0; i <= 20; ++i ) {
		const auto bit = uint32_t( 1 ) << i;
		if ( !squares[ i ] ) { unavailable |= bit; continue; }
		const auto& square = *squares[ i ];
		if ( ( !square.explored && !c.all_map_revealed ) || square.base_marker
			|| ( square.recognized_owner && *square.recognized_owner != c.owner ) ) { unavailable |= bit; }
		for ( const auto& unit : square.units ) {
			if ( unit.active_convoy || ( unit.owner != c.owner && unit.visible_to_evaluator
				&& !c.pact[ unit.owner ] && !c.treaty[ unit.owner ] ) ) { unavailable |= bit; }
		}
	}
	for ( const auto& base : other_bases ) {
		if ( base.id == c.base_id ) { continue; }
		const SmacWorkerRadiusMap map{ base.center, c.map.width, c.map.height, c.map.horizontal_wrap };
		for ( int32_t occupied = 0; occupied <= 20; ++occupied ) {
			if ( occupied && !( base.selected & ( uint32_t( 1 ) << occupied ) ) ) { continue; }
			const auto coordinate = SmacWorkedSquareCollection::Resolve( map, occupied );
			if ( !coordinate ) { continue; }
			for ( int32_t i = 0; i <= 20; ++i ) {
				if ( coordinates[ i ] == coordinate && ( base.owner == c.owner || squares[ i ]->sea ) ) {
					unavailable |= uint32_t( 1 ) << i;
				}
			}
		}
	}
	return unavailable;
}

}

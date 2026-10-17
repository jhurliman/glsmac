#include "SmacUnitSupport.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace game::backend::base {
namespace {
void Validate( const SmacSupportUnit& unit ) {
	switch ( unit.home ) {
		case SmacSupportHome::None: case SmacSupportHome::OtherBase: case SmacSupportHome::EvaluatingBase: break;
		default: throw std::invalid_argument( "Unsupported SMAC support home relationship" );
	}
	switch ( unit.package ) {
		case SmacSupportPackage::Combat: case SmacSupportPackage::Colony: case SmacSupportPackage::Former:
		case SmacSupportPackage::Transport: case SmacSupportPackage::Supply: case SmacSupportPackage::Probe:
		case SmacSupportPackage::Artifact: break;
		default: throw std::invalid_argument( "Unsupported SMAC support weapon package" );
	}
	switch ( unit.design ) {
		case SmacSupportDesign::BuiltIn: case SmacSupportDesign::PlayerDesigned: break;
		default: throw std::invalid_argument( "Unsupported SMAC support design origin" );
	}
	switch ( unit.terrain ) {
		case SmacSupportTerrain::Land: case SmacSupportTerrain::Shelf: case SmacSupportTerrain::DeepOcean: case SmacSupportTerrain::Trench: break;
		default: throw std::invalid_argument( "Unsupported SMAC support terrain band" );
	}
}

bool Exempt( const SmacSupportUnit& unit ) {
	return unit.package == SmacSupportPackage::Supply || unit.package == SmacSupportPackage::Probe ||
		unit.package == SmacSupportPackage::Artifact || unit.clean_reactor ||
		( unit.design == SmacSupportDesign::BuiltIn && unit.psi_weapon && unit.fungus &&
			( unit.terrain == SmacSupportTerrain::Land || unit.terrain == SmacSupportTerrain::Shelf ) );
}
}

SmacUnitSupportResult SmacUnitSupport::Recalculate( int32_t rating, int32_t population, const std::vector< SmacSupportUnit >& ordered_units ) {
	if ( population < 0 || population > 127 || rating > std::numeric_limits< int32_t >::max() - 4 ||
		ordered_units.size() > MAX_ORDERED_UNITS ) {
		throw std::invalid_argument( "SMAC support requires bounded population and signed32 arithmetic" );
	}
	std::unordered_set< uint64_t > identities;
	for ( const auto& unit : ordered_units ) {
		Validate( unit );
		if ( !identities.insert( unit.id ).second ) { throw std::invalid_argument( "Duplicate ordered SMAC support unit" ); }
	}
	const int32_t allowances[] = { 0, 0, 1, 1, 2, 3, 4, std::max( 4, population ) };
	SmacUnitSupportResult result{ allowances[ std::clamp( rating + 4, 0, 7 ) ], rating <= -4 ? 2 : 1, 0, 0, 0, {} };
	result.units.reserve( ordered_units.size() );
	for ( const auto& unit : ordered_units ) {
		auto disposition = SmacSupportDisposition::Ignored;
		if ( unit.owned_by_base_faction && unit.home == SmacSupportHome::EvaluatingBase ) {
			disposition = SmacSupportDisposition::Exempt;
			if ( !Exempt( unit ) ) {
				++result.eligible_count;
				disposition = SmacSupportDisposition::Free;
				if ( result.eligible_count > result.free_allowance ) {
					disposition = SmacSupportDisposition::Paid;
					++result.paid_count;
					result.mineral_support += result.minerals_per_paid_unit;
				}
			}
		}
		result.units.push_back( { unit.id, disposition } );
	}
	return result;
}

}

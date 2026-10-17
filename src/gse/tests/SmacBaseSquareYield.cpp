#include "SmacBaseSquareYield.h"

#include "fixtures/SmacBaseSquareInteger.h"
#include "task/gsetests/GSETests.h"

#include <limits>
#include <stdexcept>

namespace gse::tests {
namespace {

using namespace game::backend::resource::smac;

std::string Describe( const Yield& yield ) {
	return std::to_string( yield[ 0 ] ) + "/" + std::to_string( yield[ 1 ] ) + "/" + std::to_string( yield[ 2 ] );
}

}

void AddSmacBaseSquareYieldTests( task::gsetests::GSETests* task ) {
	for ( const auto& test : smac_fixture::cases ) {
		task->AddTest( std::string( "SMAC " ) + test.id + ": " + test.name, GT( test ) {
			const auto actual = CalculateBaseSquareYield( test.rules, test.inputs );
			GT_ASSERT( actual == test.expected, "original-x86 checked expectation " + Describe( test.expected ) + ", got " + Describe( actual ) );
			GT_OK();
		} );
	}
	task->AddTest( "SMAC center calculator rejects invalid resolved inputs", GT() {
		const auto& neutral = smac_fixture::cases.front();
		for ( const bool invalid_bonus : { false, true } ) {
			auto input = neutral.inputs;
			if ( invalid_bonus ) { input.terrain.resource_bonus = static_cast< ResourceBonus >( 99 ); }
			else { input.faction.base_count = 0; }
			bool rejected = false;
			try { CalculateBaseSquareYield( neutral.rules, input ); }
			catch ( const std::invalid_argument& ) { rejected = true; }
			GT_ASSERT( rejected, "invalid SMAC center context was accepted" );
		}
		GT_OK();
	} );
	for ( const auto& test : smac_fixture::integer_cases ) {
		task->AddTest( std::string( "SMAC " ) + test.id + ": " + test.name, GT( test ) {
			const auto actual = CalculateBaseSquareYield( test.rules, test.inputs );
			GT_ASSERT( actual == test.expected, "original-x86 signed32 expectation " + Describe( test.expected ) + ", got " + Describe( actual ) );
			GT_OK();
		} );
	}
	task->AddTest( "SMAC center calculator accepts representable signed boundaries", GT() {
		const auto maximum = std::numeric_limits< int32_t >::max();
		const auto minimum = std::numeric_limits< int32_t >::min();
		auto rules = smac_fixture::cases.front().rules;
		auto input = smac_fixture::cases.front().inputs;
		rules.base_square = { maximum, minimum, maximum };
		const Yield limits = { maximum, minimum, 99 };
		GT_ASSERT( CalculateBaseSquareYield( rules, input ) == limits, "representable nutrient/mineral endpoints or energy clamp changed" );
		rules = smac_fixture::cases.front().rules;
		input.faction.economy = maximum;
		const Yield high_economy = { 2, 1, 6 };
		GT_ASSERT( CalculateBaseSquareYield( rules, input ) == high_economy, "high economy did not saturate its base term" );
		input.faction.economy = minimum;
		const Yield low_economy = { 2, 1, 0 };
		GT_ASSERT( CalculateBaseSquareYield( rules, input ) == low_economy, "low economy did not clamp energy safely" );
		input.faction.economy = -1;
		input.faction.base_count = std::numeric_limits< uint32_t >::max();
		input.facilities.headquarters = true;
		const Yield multiple_bases = { 2, 1, 1 };
		GT_ASSERT( CalculateBaseSquareYield( rules, input ) == multiple_bases, "large faction base count was treated as one base" );
		GT_OK();
	} );
}

}

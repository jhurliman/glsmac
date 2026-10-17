#include "SmacMineralCostFactor.h"

#include "fixtures/SmacMineralCostFactors.h"
#include "task/gsetests/GSETests.h"

#include <limits>
#include <stdexcept>

namespace gse::tests {

using namespace game::backend::resource::smac;

void AddSmacMineralCostFactorTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC mineral factor named arithmetic expectations", GT() {
		for ( const auto& test : smac_mineral_cost_fixture::named ) {
			GT_ASSERT( CalculateMineralCostFactor( test.inputs ) == test.expected, "factor differs from original arithmetic evidence: " + std::string( test.id ) );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC mineral factor original dominance integration expectations", GT() {
		// These are the factor outputs of original integration worlds. This port
		// supplies their resolved dominance result; it does not execute ranking.
		for ( const auto& test : smac_mineral_cost_fixture::integrations ) {
			for ( size_t owner = 0; owner < 2; ++owner ) {
				auto inputs = test.inputs;
				inputs.human = owner == 0;
				GT_ASSERT( CalculateMineralCostFactor( inputs ) == test.expected[ owner ], "factor differs from original integrated output: " + std::string( test.id ) );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC mineral factor rounding and out-of-range discontinuity", GT() {
		auto input = smac_mineral_cost_fixture::named.front().inputs;
		input.industry = -7;
		GT_ASSERT( CalculateMineralCostFactor( input ) == 13, "negative INDUSTRY penalty changed" );
		input.industry = -8;
		GT_ASSERT( CalculateMineralCostFactor( input ) == 5, "original -8 discontinuity was clamped away" );
		input.world_size = MineralCostWorldSize::Tiny; input.industry = 1;
		GT_ASSERT( CalculateMineralCostFactor( input ) == 8, "world truncation then industry ceiling changed" );
		input.mineral_multiplier = 1;
		GT_ASSERT( CalculateMineralCostFactor( input ) == 0, "early truncation acquired a minimum-one floor" );
		input.mineral_multiplier = 0;
		for ( const auto industry : { std::numeric_limits< int32_t >::min(), -7, -2, -1, 0, 1, 2, 3, 4, std::numeric_limits< int32_t >::max() } ) {
			input.industry = industry;
			GT_ASSERT( CalculateMineralCostFactor( input ) == 0, "zero multiplier was not preserved" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC mineral factor human and independent adaptive inputs", GT() {
		auto input = smac_mineral_cost_fixture::named.front().inputs;
		for ( int32_t difficulty = 0; difficulty < 6; ++difficulty ) {
			for ( int mask = 0; mask < 8; ++mask ) {
				input.global_difficulty = difficulty; input.leader_human = mask & 1;
				input.network_session = mask & 2; input.leading_human_dominance = mask & 4;
				GT_ASSERT( CalculateMineralCostFactor( input ) == 10, "human factor used adaptive AI metadata" );
			}
		}
		input.human = false; input.global_difficulty = 5;
		input.leader_human = true; input.network_session = true; input.leading_human_dominance = true;
		GT_ASSERT( CalculateMineralCostFactor( input ) == 6, "network mode incorrectly suppressed dominance deduction" );
		input.leader_human = false; input.network_session = false;
		GT_ASSERT( CalculateMineralCostFactor( input ) == 6, "resolved dominance result was implicitly recomputed" );
		GT_OK();
	} );
	task->AddTest( "SMAC mineral factor rejects invalid resolved metadata", GT() {
		for ( const bool human : { false, true } ) {
			for ( int variant = 0; variant < 4; ++variant ) {
				auto input = smac_mineral_cost_fixture::named.front().inputs; input.human = human;
				if ( variant == 0 ) { input.global_difficulty = -1; }
				else if ( variant == 1 ) { input.global_difficulty = 6; }
				else if ( variant == 2 ) { input.world_size = static_cast< MineralCostWorldSize >( 99 ); }
				else { input.mineral_multiplier = -1; }
				bool rejected = false;
				try { CalculateMineralCostFactor( input ); }
				catch ( const std::invalid_argument& ) { rejected = true; }
				GT_ASSERT( rejected, "invalid resolved metadata was accepted" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC mineral factor signed intermediate boundaries", GT() {
		const auto maximum = std::numeric_limits< int32_t >::max();
		auto input = smac_mineral_cost_fixture::named.front().inputs;
		for ( const auto industry : { -7, -1, 0 } ) {
			input.industry = industry;
			input.mineral_multiplier = industry == -7 ? ( maximum - 9 ) / 13 : industry == -1 ? ( maximum - 9 ) / 11 : maximum / 10;
			GT_ASSERT( CalculateMineralCostFactor( input ) == 214748364, "last admitted arithmetic boundary changed" );
			++input.mineral_multiplier;
			bool rejected = false;
			try { CalculateMineralCostFactor( input ); }
			catch ( const std::overflow_error& ) { rejected = true; }
			GT_ASSERT( rejected, "out-of-domain numerator/product was accepted" );
		}
		input.mineral_multiplier = maximum / 10; input.industry = -2;
		GT_ASSERT( CalculateMineralCostFactor( input ) == 257698037, "reduced 6*S+4 numerator was rejected or changed" );
		for ( const auto industry : { std::numeric_limits< int32_t >::min(), maximum } ) {
			input.industry = industry;
			GT_ASSERT( CalculateMineralCostFactor( input ) == 107374182, "extreme INDUSTRY did not use the half-factor branch" );
		}
		input.human = false; input.global_difficulty = 0; input.industry = 0;
		input.mineral_multiplier = maximum / 13;
		GT_ASSERT( CalculateMineralCostFactor( input ) == 214748363, "AI product boundary changed" );
		++input.mineral_multiplier;
		bool rejected = false;
		try { CalculateMineralCostFactor( input ); }
		catch ( const std::overflow_error& ) { rejected = true; }
		GT_ASSERT( rejected, "AI product overflow was accepted" );
		GT_OK();
	} );
}

}

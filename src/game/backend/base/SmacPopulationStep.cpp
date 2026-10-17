#include "SmacPopulationStep.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace game::backend::base {
namespace {
int32_t Checked( const int64_t value ) {
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::overflow_error( "SMAC population arithmetic exceeds signed32" ); }
	return static_cast< int32_t >( value );
}
int32_t Normalize( const int32_t stored, const int32_t surplus ) {
	if ( surplus < 0 && stored == 0 ) { return -1; }
	if ( surplus >= 0 && stored < 0 ) { return 0; }
	return stored;
}
}

int32_t CalculateSmacNutrientCostFactor( const SmacNutrientCostInputs& input ) {
	if ( input.global_difficulty < 0 || input.global_difficulty > 5 || input.world_size_category < 0
		|| input.world_size_category > 4 || input.nutrient_multiplier < 0 ) {
		throw std::invalid_argument( "Unsupported SMAC nutrient cost inputs" );
	}
	static constexpr std::array< int32_t, 6 > ai_factors{ 13, 12, 11, 10, 8, 7 };
	int32_t value = 10;
	if ( !input.human ) {
		value = ai_factors[ input.global_difficulty ];
		if ( input.leader_human && !input.network_session ) { --value; }
		if ( input.leading_human_dominance ) { --value; }
	}
	value = Checked( int64_t( value ) * input.nutrient_multiplier ) / 10;
	if ( input.world_size_category == 0 ) { value = Checked( int64_t( value ) * 8 ) / 10; }
	else if ( input.world_size_category == 1 ) { value = Checked( int64_t( value ) * 9 ) / 10; }
	const auto growth = std::clamp( input.effective_growth, -2, 5 );
	return Checked( int64_t( value ) * ( 10 - growth ) + 9 ) / 10;
}

bool SmacHousingAllowsGrowth( const int32_t population, const int32_t penalty,
	const int32_t complex_limit, const int32_t dome_limit, const bool complex, const bool dome, const bool ascetic ) {
	if ( population < 1 || population > 127 ) { throw std::invalid_argument( "Unsupported SMAC housing population" ); }
	const auto effective = Checked( int64_t( Checked( int64_t( population ) + penalty ) ) - ( ascetic ? 2 : 0 ) );
	return ( effective < complex_limit || complex ) && ( effective < dome_limit || dome );
}

bool SmacPopulationPlan::NeedsRecalculation() const { return branch == Branch::Growth || branch == Branch::Boom; }
bool SmacPopulationPlan::CollectAfterRecalculation() const { return branch == Branch::Growth; }
int32_t SmacPopulationPlan::Finish( const int32_t surplus ) const {
	if ( !NeedsRecalculation() || !stored_nutrients ) { throw std::logic_error( "Population branch has no internal resource recalculation" ); }
	const auto normalized = Normalize( *stored_nutrients, surplus );
	return CollectAfterRecalculation() ? Checked( int64_t( normalized ) + surplus ) : std::min( normalized, old_threshold );
}

SmacPopulationPlan SmacPopulationStep::Prepare( const SmacPopulationRules& rules, const int32_t population,
	const int64_t stored_nutrients, const int32_t surplus, const bool protected_base ) {
	if ( population < 1 || population > 127 || rules.cost_per_step < 0 || rules.nutrients_per_citizen < 0 ) {
		throw std::invalid_argument( "Unsupported SMAC population step inputs" );
	}
	using Branch = SmacPopulationPlan::Branch;
	const auto threshold = Checked( int64_t( rules.cost_per_step ) * ( population + 1 ) );
	const auto stored = Normalize( Checked( stored_nutrients ), surplus );
	if ( stored < 0 ) {
		if ( population == 1 && !protected_base ) { return { Branch::Removal, 0, std::nullopt, threshold }; }
		return { Branch::Starvation, population == 1 ? 1 : population - 1, surplus, threshold };
	}
	if ( ( rules.cloning_vats || rules.effective_growth >= 6 ) && rules.nutrients_per_citizen != 0
		&& surplus >= rules.nutrients_per_citizen && rules.housing_allows_growth ) {
		// The original still recalculates at 127, although the increase is skipped.
		return { Branch::Boom, population < 127 ? population + 1 : population, stored, threshold };
	}
	if ( stored < threshold ) { return { Branch::Collection, population, Checked( int64_t( stored ) + surplus ), threshold }; }
	if ( rules.effective_growth <= -3 ) { return { Branch::Stagnation, population, threshold, threshold }; }
	if ( !rules.housing_allows_growth ) { return { Branch::Housing, population, Checked( int64_t( threshold / 2 ) + surplus ), threshold }; }
	// The original ordinary-growth signed-byte edge is not established. Reject
	// that unsupported overflow instead of reporting a made-up population128.
	if ( population == 127 ) { throw std::overflow_error( "Ordinary SMAC population growth exceeds 127" ); }
	return { Branch::Growth, population + 1, 0, threshold };
}

}

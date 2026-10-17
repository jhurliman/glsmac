#include "SmacPopulationStep.h"

#include "fixtures/SmacPopulationSteps.h"
#include "task/gsetests/GSETests.h"
#include <stdexcept>

namespace gse::tests {
using namespace game::backend::base;
namespace {
template< typename F > bool Rejects( F&& operation ) {
	try { operation(); } catch ( const std::exception& ) { return true; }
	return false;
}
}

void AddSmacPopulationStepTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC population nutrient factor matches original human and AI cases", GT() {
		for ( const auto& test : smac_population_fixture::costs ) {
			GT_ASSERT( CalculateSmacNutrientCostFactor( test.input ) == test.expected, "nutrient cost differs: " + std::string( test.id ) );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC population plans match original growth starvation and housing steps", GT() {
		for ( const auto& test : smac_population_fixture::cases ) {
			const auto growth = test.growth + ( test.creche ? 2 : 0 ) + ( test.golden ? 2 : 0 );
			const SmacPopulationRules rules{
				CalculateSmacNutrientCostFactor( { true, 3, false, false, false, test.size, test.multiplier, growth } ),
				growth, test.demand, SmacHousingAllowsGrowth( test.population, test.penalty, test.complex_limit,
					test.dome_limit, test.complex, test.dome, test.ascetic ), test.cloning
			};
			const auto plan = SmacPopulationStep::Prepare( rules, test.population, test.stored, test.surplus, test.objective );
			const auto stored = plan.NeedsRecalculation() ? std::optional< int32_t >( plan.Finish( test.next_surplus ) ) : plan.stored_nutrients;
			GT_ASSERT( plan.population == test.expected_population && stored == test.expected_stored,
				"population or food differs from original: " + std::string( test.id ) );
			GT_ASSERT( plan.NeedsRecalculation() == test.recalculated, "internal resource call differs: " + std::string( test.id ) );
			GT_ASSERT( ( plan.branch == SmacPopulationPlan::Branch::Removal ) == !test.expected_stored,
				"Base removal request differs: " + std::string( test.id ) );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC population staging preserves recalculation order and checked arithmetic", GT() {
		const SmacPopulationRules boom{ 0, 6, 2, true, false };
		const auto plan = SmacPopulationStep::Prepare( boom, 2, 5, 2, false );
		GT_ASSERT( plan.stored_nutrients == 5 && plan.Finish( -1 ) == 0, "boom cap occurred before resource normalization" );
		const SmacPopulationRules ordinary{ 10, 0, 2, true, false };
		GT_ASSERT( CalculateSmacNutrientCostFactor( { true, 3, false, false, false, 99, 10, 0 } ) == 10,
			"accepted custom category acquired preset size scaling" );
		GT_ASSERT( Rejects( [ & ] { SmacPopulationStep::Prepare( ordinary, 2, INT64_MAX, 1, false ); } ), "stored food narrowed" );
		GT_ASSERT( Rejects( [ & ] { SmacPopulationStep::Prepare( ordinary, 127, 1280, 1, false ); } ), "unobserved ordinary population overflow accepted" );
		GT_ASSERT( Rejects( [ & ] { SmacPopulationStep::Prepare( ordinary, 2, 1, INT32_MAX, false ); } ), "collection overflow accepted" );
		GT_ASSERT( Rejects( [ & ] { CalculateSmacNutrientCostFactor( { true, 3, false, false, false, 2, INT32_MAX, 0 } ); } ), "cost intermediate overflow accepted" );
		GT_ASSERT( Rejects( [ & ] { SmacHousingAllowsGrowth( 2, INT32_MAX, 7, 14, false, false, true ); } ), "housing intermediate overflow accepted" );
		const auto collect = SmacPopulationStep::Prepare( ordinary, 2, 0, 1, false );
		GT_ASSERT( Rejects( [ & ] { collect.Finish( 1 ); } ), "ordinary collection was recalculated as growth" );
		GT_OK();
	} );
}
}

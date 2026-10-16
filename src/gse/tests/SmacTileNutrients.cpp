#include "SmacTileNutrients.h"

#include "fixtures/SmacTileNutrients.h"
#include "task/gsetests/GSETests.h"

#include <limits>
#include <stdexcept>

namespace gse::tests {

void AddSmacTileNutrientTests( task::gsetests::GSETests* task ) {
	using namespace game::backend::resource::smac;
	for ( const auto& test : smac_nutrient_fixture::cases ) {
		task->AddTest( std::string( "SMAC " ) + test.id + ": " + test.name, GT( test ) {
			const auto result = CalculateTileNutrients( test.rules, test.inputs );
			GT_ASSERT( result.nutrients == test.expected.nutrients, "original-x86 checked nutrient expectation changed" );
			GT_ASSERT( result.technology_capped_excess == test.expected.technology_capped_excess, "technology-capped excess changed" );
			GT_OK();
		} );
	}
	task->AddTest( "SMAC nutrient resource resolution and sea improvements respect every altitude band", GT() {
		auto test = smac_nutrient_fixture::cases[ 0 ];
		test.inputs.terrain.moisture = Moisture::Rainy;
		test.inputs.improvements = { true, true, false, false };
		test.inputs.context.nutrient_technology = true;
		for ( uint8_t band = 0 ; band < 8 ; ++band ) {
			test.inputs.terrain.altitude = static_cast< AltitudeBand >( band );
			test.inputs.terrain.resource_bonus = ResolveResourceBonus( { 0, 0, test.inputs.terrain.altitude, 0, false, true, true, false, false } );
			const auto result = CalculateTileNutrients( test.rules, test.inputs );
			GT_ASSERT( result.nutrients == ( band < 2 ? 1 : 4 ), "resolved nutrient resource or shelf/land improvements changed" );
			GT_ASSERT( result.technology_capped_excess == 0, "technology ownership unexpectedly capped nutrients" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC nutrient multipliers round each signed half independently", GT() {
		auto test = smac_nutrient_fixture::cases[ 0 ];
		test.inputs.terrain.forest = true;
		test.inputs.improvements = { false, false, true, true };
		struct Row { int32_t before; int64_t after; };
		for ( const auto row : { Row{ -5, -10 }, Row{ -3, -6 }, Row{ -1, -1 }, Row{ 0, 0 }, Row{ 1, 1 }, Row{ 3, 6 }, Row{ 5, 10 } } ) {
			test.rules.forest = row.before;
			const auto result = CalculateTileNutrients( test.rules, test.inputs );
			GT_ASSERT( result.nutrients == row.after && result.technology_capped_excess == 0, "separately rounded halves were combined or rounded down" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC nutrient special branches win before ordinary improvements and cap", GT() {
		auto test = smac_nutrient_fixture::cases[ 0 ];
		test.rules.borehole = 5;
		test.inputs.improvements = { true, true, true, true };
		test.inputs.context = { true, true, false, 9, 0, false, false };
		test.inputs.terrain.forest = true;
		for ( const auto band : { AltitudeBand::Trench, AltitudeBand::DeepOcean, AltitudeBand::Shelf, AltitudeBand::Land3 } ) {
			test.inputs.terrain.altitude = band;
			for ( int flags = 0 ; flags < 8 ; ++flags ) {
				test.inputs.terrain.borehole = flags & 1;
				test.inputs.terrain.monolith = flags & 2;
				test.inputs.terrain.fungus = flags & 4;
				const int64_t expected = flags & 1 ? 7 : flags & 2 ? 2 : ( flags & 4 ) && band >= AltitudeBand::Shelf ? 9 : band < AltitudeBand::Shelf ? 1 : band == AltitudeBand::Shelf ? 4 : 6;
				GT_ASSERT( CalculateTileNutrients( test.rules, test.inputs ).nutrients == expected, "special-tile branch precedence changed" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC nutrient facilities stack independently and other resources do not exempt the cap", GT() {
		auto test = smac_nutrient_fixture::cases[ 0 ];
		test.inputs.terrain.forest = true;
		for ( const auto resource : { ResourceBonus::None, ResourceBonus::Minerals, ResourceBonus::Energy } ) {
			test.inputs.terrain.resource_bonus = resource;
			for ( int facilities = 0 ; facilities < 4 ; ++facilities ) {
				test.inputs.context.tree_farm = facilities & 1;
				test.inputs.context.hybrid_forest = facilities & 2;
				const auto result = CalculateTileNutrients( test.rules, test.inputs );
				GT_ASSERT( result.nutrients == ( facilities == 0 ? 1 : 2 ), "facility or unrelated resource changed yield" );
				GT_ASSERT( result.technology_capped_excess == ( facilities == 3 ? 1 : 0 ), "independent facility excess changed" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC nutrient events preserve signed values and run after capping", GT() {
		auto test = smac_nutrient_fixture::cases[ 0 ];
		test.inputs.terrain.monolith = true;
		struct Row { int32_t before; bool increase, decrease; int64_t after; };
		for ( const auto row : { Row{ 0, false, true, 0 }, Row{ 0, true, true, 0 }, Row{ -1, true, true, 0 }, Row{ -3, false, true, -4 }, Row{ -3, true, true, -3 }, Row{ 100, true, false, 101 } } ) {
			test.rules.monolith = row.before;
			test.inputs.context.bumper_crop = row.increase;
			test.inputs.context.famine = row.decrease;
			GT_ASSERT( CalculateTileNutrients( test.rules, test.inputs ).nutrients == row.after, "event order, zero guard or signed result changed" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC nutrients retain rule endpoints and large cap excess", GT() {
		const auto minimum = std::numeric_limits< int32_t >::min();
		const auto maximum = std::numeric_limits< int32_t >::max();
		auto test = smac_nutrient_fixture::cases[ 0 ];
		test.inputs.terrain.monolith = true;
		for ( const auto value : { minimum, maximum } ) {
			test.rules.monolith = value;
			GT_ASSERT( CalculateTileNutrients( test.rules, test.inputs ).nutrients == value, "representable rule endpoint was clamped" );
		}
		test.inputs.terrain.monolith = false;
		test.inputs.terrain.forest = true;
		test.rules.forest = maximum;
		const auto capped = CalculateTileNutrients( test.rules, test.inputs );
		GT_ASSERT( capped.nutrients == 2 && capped.technology_capped_excess == int64_t( maximum ) - 2, "large cap excess was lost" );
		test.inputs.terrain.fungus = true;
		test.inputs.context.fungal_capability = maximum;
		for ( const auto planet : { minimum, maximum } ) {
			test.inputs.context.planet = planet;
			GT_ASSERT( CalculateTileNutrients( test.rules, test.inputs ).nutrients == 99, "PLANET endpoint or final fungal cap changed" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC nutrients reject invalid metadata instead of assuming neutral values", GT() {
		for ( int field = 0 ; field < 7 ; ++field ) {
			auto test = smac_nutrient_fixture::cases[ 0 ];
			switch ( field ) {
				case 0: test.inputs.terrain.altitude = static_cast< AltitudeBand >( 8 ); break;
				case 1: test.inputs.terrain.moisture = static_cast< Moisture >( -1 ); break;
				case 2: test.inputs.terrain.moisture = static_cast< Moisture >( 3 ); break;
				case 3: test.inputs.terrain.rockiness = static_cast< Rockiness >( 3 ); break;
				case 4: test.inputs.terrain.resource_bonus = static_cast< ResourceBonus >( 4 ); break;
				case 5: test.inputs.terrain.altitude = AltitudeBand::DeepOcean; test.inputs.terrain.resource_bonus = ResourceBonus::Nutrients; break;
				case 6: test.inputs.context.fungal_capability = -1; break;
			}
			bool rejected = false;
			try { CalculateTileNutrients( test.rules, test.inputs ); }
			catch ( const std::invalid_argument& ) { rejected = true; }
			GT_ASSERT( rejected, "invalid or incoherent resolved metadata was accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC nutrients reject each evaluated overflow before later clamp or cancellation", GT() {
		const auto minimum = std::numeric_limits< int32_t >::min();
		const auto maximum = std::numeric_limits< int32_t >::max();
		for ( int operation = 0 ; operation < 10 ; ++operation ) {
			auto test = smac_nutrient_fixture::cases[ 0 ];
			switch ( operation ) {
				case 0: test.inputs.terrain.borehole = true; test.rules.borehole = minimum; test.rules.bonus = -1; break;
				case 1: test.inputs.terrain.monolith = true; test.rules.monolith = maximum; test.inputs.terrain.resource_bonus = ResourceBonus::Nutrients; break;
				case 2: test.inputs.terrain.forest = true; test.rules.forest = maximum; test.inputs.context.tree_farm = true; break;
				case 3: test.inputs.terrain.forest = true; test.rules.forest = maximum; test.inputs.improvements.soil_enricher = true; break;
				case 4: test.inputs.terrain.altitude = AltitudeBand::Shelf; test.rules.ocean = minimum; test.inputs.improvements.condenser = true; break;
				case 5: test.inputs.terrain.moisture = Moisture::Rainy; test.inputs.improvements.farm = true; test.rules.improved_land = maximum; break;
				case 6: test.inputs.terrain.moisture = Moisture::Rainy; test.inputs.improvements.mine = true; test.rules.mine_effect = maximum; break;
				case 7: test.inputs.terrain.resource_bonus = ResourceBonus::Nutrients; test.rules.bonus = maximum; test.inputs.terrain.landmark_bonus = true; break;
				case 8: test.inputs.terrain.monolith = true; test.rules.monolith = maximum; test.inputs.context.bumper_crop = true; test.inputs.context.famine = true; break;
				case 9: test.inputs.terrain.monolith = true; test.rules.monolith = minimum; test.inputs.context.famine = true; break;
			}
			bool rejected = false;
			try { CalculateTileNutrients( test.rules, test.inputs ); }
			catch ( const std::overflow_error& ) { rejected = true; }
			GT_ASSERT( rejected, "unverified signed overflow was silently accepted" );
		}
		GT_OK();
	} );
}

}

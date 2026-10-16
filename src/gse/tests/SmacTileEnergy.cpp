#include "SmacTileEnergy.h"

#include "fixtures/SmacTileEnergy.h"
#include "task/gsetests/GSETests.h"

#include <limits>
#include <stdexcept>

namespace gse::tests {

void AddSmacTileEnergyTests( task::gsetests::GSETests* task ) {
	using namespace game::backend::resource::smac;
	for ( const auto& test : smac_energy_fixture::cases ) {
		task->AddTest( std::string( "SMAC " ) + test.id + ": " + test.name, GT( test ) {
			const auto result = CalculateTileEnergy( test.rules, test.inputs );
			GT_ASSERT( result.energy == test.expected.energy, "original-x86 checked energy expectation changed" );
			GT_ASSERT( result.technology_capped_excess == test.expected.technology_capped_excess, "energy technology removed amount changed" );
			GT_OK();
		} );
	}
	task->AddTest( "SMAC energy resolved mirrors affect only ordinary land collectors", GT() {
		auto test = smac_energy_fixture::cases[ 0 ];
		test.inputs.context.energy_technology = true;
		for ( uint8_t band = 0 ; band < 8 ; ++band ) {
			test.inputs.terrain.altitude = static_cast< AltitudeBand >( band );
			for ( int flags = 0 ; flags < 8 ; ++flags ) {
				test.inputs.terrain.forest = flags & 1;
				test.inputs.improvements.solar_collector = flags & 2;
				test.inputs.improvements.echelon_mirror = flags & 4;
				for ( int count = 0 ; count <= 8 ; ++count ) {
					test.inputs.improvements.matching_adjacent_mirrors = count;
					int64_t expected;
					if ( band <= 2 ) { expected = ( band == 2 && ( flags & 2 ) ? 3 : 0 ) - ( flags & 1 ? 1 : 0 ); }
					else if ( flags & 1 ) { expected = 1; }
					else { expected = ( flags & 6 ? band - 2 : 0 ) + ( flags & 2 ? count : 0 ); }
					const auto result = CalculateTileEnergy( test.rules, test.inputs );
					GT_ASSERT( result.energy == expected && result.technology_capped_excess == 0, "resolved mirror, altitude or forest arithmetic changed" );
				}
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC energy resources resolve before common additions and caps", GT() {
		auto test = smac_energy_fixture::cases[ 0 ];
		test.inputs.terrain.borehole = true;
		for ( uint8_t band = 0 ; band < 8 ; ++band ) {
			test.inputs.terrain.altitude = static_cast< AltitudeBand >( band );
			test.inputs.terrain.resource_bonus = ResolveResourceBonus( { 0, 0, test.inputs.terrain.altitude, 0, false, true, true, true, false } );
			const auto result = CalculateTileEnergy( test.rules, test.inputs );
			GT_ASSERT( result.energy == ( band < 2 ? 2 : 8 ) && result.technology_capped_excess == ( band < 2 ? 4 : 0 ), "effective resource did not control addition and cap exemption" );
		}
		for ( const auto resource : { ResourceBonus::None, ResourceBonus::Nutrients, ResourceBonus::Minerals } ) {
			test.inputs.terrain.resource_bonus = resource;
			const auto result = CalculateTileEnergy( test.rules, test.inputs );
			GT_ASSERT( result.energy == 2 && result.technology_capped_excess == 4, "unrelated resource added energy or exempted the cap" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC energy special branches preserve precedence and fungal exclusions", GT() {
		auto test = smac_energy_fixture::cases[ 0 ];
		test.inputs.terrain.landmark_bonus = true;
		test.inputs.terrain.river = true;
		test.inputs.terrain.forest = true;
		test.inputs.improvements = { true, true, 8 };
		test.inputs.context.hybrid_forest = true;
		test.inputs.context.merchant_exchange_here = true;
		test.inputs.context.economy = 2;
		test.inputs.context.fungal_capability = 9;
		for ( const auto band : { AltitudeBand::Trench, AltitudeBand::DeepOcean, AltitudeBand::Shelf, AltitudeBand::Land3 } ) {
			test.inputs.terrain.altitude = band;
			for ( int flags = 0 ; flags < 8 ; ++flags ) {
				test.inputs.terrain.monolith = flags & 1;
				test.inputs.terrain.borehole = flags & 2;
				test.inputs.terrain.fungus = flags & 4;
				const bool fungal_branch = ( flags & 4 ) && !( flags & 3 ) && band >= AltitudeBand::Shelf;
				const int64_t before = ( flags & 1 ? 2 : flags & 2 ? 6 : band < AltitudeBand::Shelf ? -1 : 2 ) + 3 + ( band > AltitudeBand::Shelf ? 1 : 0 );
				const auto result = CalculateTileEnergy( test.rules, test.inputs );
				GT_ASSERT( result.energy == ( fungal_branch ? 9 : 2 ) && result.technology_capped_excess == ( fungal_branch ? 0 : before - 2 ), "special energy branch or fungal bonus exclusion changed" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC energy signed events preserve order after the technology cap", GT() {
		auto test = smac_energy_fixture::cases[ 0 ];
		test.inputs.terrain.monolith = true;
		test.inputs.context.energy_technology = true;
		struct Row { int32_t initial; bool heat, cloud, dust, solar; int64_t expected; };
		for ( const auto row : { Row{ 0, false, true, true, true, 0 }, Row{ -1, true, true, true, true, 0 },
			Row{ 1, false, true, true, true, 0 }, Row{ 1, true, true, true, true, 0 },
			Row{ -3, false, true, true, true, -15 }, Row{ 120, true, true, false, true, 360 } } ) {
			test.rules.monolith = row.initial;
			test.inputs.context.heat_wave = row.heat;
			test.inputs.context.cloud_cover = row.cloud;
			test.inputs.context.atmospheric_dust = row.dust;
			test.inputs.context.solar_multiplier_active = row.solar;
			GT_ASSERT( CalculateTileEnergy( test.rules, test.inputs ).energy == row.expected, "signed event zero guards, order or ordinary unclamped output changed" );
		}
		test.rules.monolith = 120;
		test.inputs.context.energy_technology = false;
		const auto capped = CalculateTileEnergy( test.rules, test.inputs );
		GT_ASSERT( capped.energy == 6 && capped.technology_capped_excess == 118, "events changed the pre-event capped excess" );
		GT_OK();
	} );
	task->AddTest( "SMAC energy endpoints retain signed rules and social ratings", GT() {
		const auto minimum = std::numeric_limits< int32_t >::min(), maximum = std::numeric_limits< int32_t >::max();
		auto test = smac_energy_fixture::cases[ 0 ];
		test.inputs.terrain.monolith = true;
		test.inputs.context.energy_technology = true;
		for ( const auto endpoint : { minimum, maximum } ) {
			test.rules.monolith = endpoint;
			GT_ASSERT( CalculateTileEnergy( test.rules, test.inputs ).energy == endpoint, "ordinary energy endpoint was clamped" );
		}
		test.inputs.context.energy_technology = false;
		const auto capped = CalculateTileEnergy( test.rules, test.inputs );
		GT_ASSERT( capped.energy == 2 && capped.technology_capped_excess == int64_t( maximum ) - 2, "maximum cap removal changed" );
		test.rules.monolith = 0;
		for ( const auto endpoint : { minimum, maximum } ) {
			test.inputs.context.economy = endpoint;
			GT_ASSERT( CalculateTileEnergy( test.rules, test.inputs ).energy == ( endpoint > 0 ? 1 : 0 ), "effective economy endpoint changed" );
		}
		test.inputs.terrain.monolith = false;
		test.inputs.terrain.fungus = true;
		test.inputs.context.fungal_capability = 5;
		for ( const auto endpoint : { minimum, maximum } ) {
			test.inputs.context.planet = endpoint;
			GT_ASSERT( CalculateTileEnergy( test.rules, test.inputs ).energy == ( endpoint < 0 ? 2 : 5 ), "PLANET penalty was not clamped before adding capability" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC energy rejects invalid resolved inputs before calculation", GT() {
		for ( int mode = 0 ; mode < 10 ; ++mode ) {
			auto test = smac_energy_fixture::cases[ 0 ];
			switch ( mode ) {
				case 0: test.inputs.terrain.altitude = static_cast< AltitudeBand >( 8 ); break;
				case 1: test.inputs.terrain.altitude = static_cast< AltitudeBand >( 255 ); break;
				case 2: test.inputs.terrain.resource_bonus = static_cast< ResourceBonus >( -1 ); break;
				case 3: test.inputs.terrain.resource_bonus = static_cast< ResourceBonus >( 4 ); break;
				case 4: test.inputs.terrain.altitude = AltitudeBand::Trench; test.inputs.terrain.resource_bonus = ResourceBonus::Energy; break;
				case 5: test.inputs.terrain.altitude = AltitudeBand::DeepOcean; test.inputs.terrain.resource_bonus = ResourceBonus::Nutrients; break;
				case 6: test.inputs.context.fungal_capability = -1; break;
				case 7: test.inputs.improvements.matching_adjacent_mirrors = -1; break;
				case 8: test.inputs.improvements.matching_adjacent_mirrors = 9; break;
				case 9: test.inputs.improvements.matching_adjacent_mirrors = std::numeric_limits< int32_t >::max(); break;
			}
			bool rejected = false;
			try { CalculateTileEnergy( test.rules, test.inputs ); }
			catch ( const std::invalid_argument& ) { rejected = true; }
			GT_ASSERT( rejected, "invalid energy input accepted: " + std::to_string( mode ) );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC energy rejects evaluated signed overflow before later caps", GT() {
		const auto minimum = std::numeric_limits< int32_t >::min(), maximum = std::numeric_limits< int32_t >::max();
		for ( int mode = 0 ; mode < 13 ; ++mode ) {
			auto test = smac_energy_fixture::cases[ 0 ];
			test.inputs.terrain.monolith = true;
			test.rules.monolith = maximum;
			switch ( mode ) {
				case 0: test.inputs.terrain.river = true; break;
				case 1: test.inputs.terrain.resource_bonus = ResourceBonus::Energy; test.rules.bonus = 1; break;
				case 2: test.inputs.terrain.landmark_bonus = true; break;
				case 3: test.inputs.context.merchant_exchange_here = true; break;
				case 4: test.inputs.context.economy = 2; break;
				case 5: test.inputs.context.economy = maximum; test.inputs.context.golden_age = true; break;
				case 6: test.inputs.context.energy_technology = true; test.inputs.context.heat_wave = true; break;
				case 7: test.rules.monolith = minimum; test.inputs.context.cloud_cover = true; break;
				case 8: test.rules.monolith = minimum; test.inputs.context.atmospheric_dust = true; break;
				case 9: test.inputs.context.energy_technology = true; test.inputs.context.solar_multiplier_active = true; break;
				case 10: test.inputs.terrain.monolith = false; test.inputs.terrain.forest = true; test.rules.forest = maximum; test.inputs.context.hybrid_forest = true; break;
				case 11: test.inputs.terrain.monolith = false; test.inputs.terrain.altitude = AltitudeBand::Shelf; test.rules.ocean = maximum; test.inputs.improvements.solar_collector = true; break;
				case 12: test.inputs.terrain.monolith = false; test.inputs.terrain.altitude = AltitudeBand::Shelf; test.inputs.terrain.forest = true; test.rules.ocean = minimum; break;
			}
			bool rejected = false;
			try { CalculateTileEnergy( test.rules, test.inputs ); }
			catch ( const std::overflow_error& ) { rejected = true; }
			GT_ASSERT( rejected, "evaluated energy overflow accepted: " + std::to_string( mode ) );
		}
		GT_OK();
	} );
}

}

#include "SmacTileMinerals.h"

#include "fixtures/SmacTileMinerals.h"
#include "task/gsetests/GSETests.h"

#include <limits>
#include <stdexcept>

namespace gse::tests {

void AddSmacTileMineralTests( task::gsetests::GSETests* task ) {
	using namespace game::backend::resource::smac;
	for ( const auto& test : smac_mineral_fixture::cases ) {
		task->AddTest( std::string( "SMAC " ) + test.id + ": " + test.name, GT( test ) {
			const auto result = CalculateTileMinerals( test.rules, test.inputs );
			GT_ASSERT( result.minerals == test.expected.minerals, "original-x86 checked mineral expectation changed" );
			GT_ASSERT( result.removed_minerals == test.expected.removed_minerals, "road/technology removed amount changed" );
			GT_OK();
		} );
	}
	task->AddTest( "SMAC mineral resolution and platforms respect all altitude bands", GT() {
		auto test = smac_mineral_fixture::cases[ 0 ];
		test.inputs.terrain.rockiness = 3;
		test.inputs.improvements = { true, true };
		test.inputs.context.advanced_platform_technology = true;
		for ( uint8_t band = 0 ; band < 8 ; ++band ) {
			test.inputs.terrain.altitude = static_cast< AltitudeBand >( band );
			test.inputs.terrain.resource_bonus = ResolveResourceBonus( { 0, 0, test.inputs.terrain.altitude, 0, false, true, true, false, true } );
			const auto result = CalculateTileMinerals( test.rules, test.inputs );
			GT_ASSERT( result.minerals == ( band < 2 ? 0 : band == 2 ? 4 : 9 ), "altitude, platform or encoded rockiness boundary changed" );
			GT_ASSERT( result.removed_minerals == 0, "mineral resource did not exempt technology cap or road did not exempt mine limit" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC mineral special branches keep their own precedence and cap behavior", GT() {
		auto test = smac_mineral_fixture::cases[ 0 ];
		test.inputs.terrain.landmark_bonus = true;
		test.inputs.terrain.forest = true;
		test.inputs.improvements = { true, true };
		test.inputs.context.advanced_platform_technology = true;
		test.inputs.context.fungal_capability = 9;
		for ( const auto band : { AltitudeBand::Trench, AltitudeBand::DeepOcean, AltitudeBand::Shelf, AltitudeBand::Land3 } ) {
			test.inputs.terrain.altitude = band;
			for ( int flags = 0 ; flags < 8 ; ++flags ) {
				test.inputs.terrain.monolith = flags & 1;
				test.inputs.terrain.borehole = flags & 2;
				test.inputs.terrain.fungus = flags & 4;
				const auto result = CalculateTileMinerals( test.rules, test.inputs );
				const int64_t expected = flags & 1 ? 2 : flags & 2 ? 2 : ( flags & 4 ) && band >= AltitudeBand::Shelf ? 9 : band < AltitudeBand::Shelf ? 1 : 2;
				const int64_t removed = flags & 1 ? 0 : flags & 2 ? 4 : ( flags & 4 ) && band >= AltitudeBand::Shelf ? 0 : band < AltitudeBand::Shelf ? 0 : 1;
				GT_ASSERT( result.minerals == expected && result.removed_minerals == removed, "mineral special branch precedence or cap changed" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC mineral road limit affects only mine contribution and adds to technology removal", GT() {
		auto test = smac_mineral_fixture::cases[ 0 ];
		test.inputs.terrain.rockiness = 2;
		test.inputs.improvements.mine = true;
		struct Row { bool resource, landmark, road, technology; int64_t minerals, removed; };
		for ( const auto row : { Row{ false, false, false, false, 2, 2 }, Row{ false, false, false, true, 3, 1 }, Row{ true, false, false, false, 5, 2 }, Row{ true, false, true, false, 7, 0 }, Row{ true, true, true, false, 8, 0 }, Row{ false, true, false, false, 2, 4 }, Row{ false, true, true, false, 2, 4 } } ) {
			test.inputs.terrain.resource_bonus = row.resource ? ResourceBonus::Minerals : ResourceBonus::None;
			test.inputs.terrain.landmark_bonus = row.landmark;
			test.inputs.improvements.road = row.road;
			test.inputs.context.mineral_technology = row.technology;
			const auto result = CalculateTileMinerals( test.rules, test.inputs );
			GT_ASSERT( result.minerals == row.minerals && result.removed_minerals == row.removed, "road restriction scope, resource exemption or combined removed amount changed" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC other resources do not add minerals or exempt caps", GT() {
		auto test = smac_mineral_fixture::cases[ 0 ];
		test.inputs.terrain.borehole = true;
		for ( const auto resource : { ResourceBonus::None, ResourceBonus::Nutrients, ResourceBonus::Energy } ) {
			test.inputs.terrain.resource_bonus = resource;
			const auto result = CalculateTileMinerals( test.rules, test.inputs );
			GT_ASSERT( result.minerals == 2 && result.removed_minerals == 4, "unrelated resource changed borehole yield or cap" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC mineral events preserve signed values and test zero after increase", GT() {
		auto test = smac_mineral_fixture::cases[ 0 ];
		test.inputs.terrain.monolith = true;
		struct Row { int32_t before; bool increase, decrease; int64_t after; };
		for ( const auto row : { Row{ 0, false, true, 0 }, Row{ 0, true, true, 0 }, Row{ -1, true, true, 0 }, Row{ -3, false, true, -4 }, Row{ -3, true, true, -3 }, Row{ 100, true, false, 101 } } ) {
			test.rules.monolith = row.before;
			test.inputs.context.industrial_boom = row.increase;
			test.inputs.context.industrial_bust = row.decrease;
			GT_ASSERT( CalculateTileMinerals( test.rules, test.inputs ).minerals == row.after, "signed event order or zero guard changed" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC mineral endpoints preserve signed rules and removed amount", GT() {
		const auto minimum = std::numeric_limits< int32_t >::min(), maximum = std::numeric_limits< int32_t >::max();
		auto test = smac_mineral_fixture::cases[ 0 ];
		test.inputs.terrain.monolith = true;
		for ( const auto endpoint : { minimum, maximum } ) {
			test.rules.monolith = endpoint;
			GT_ASSERT( CalculateTileMinerals( test.rules, test.inputs ).minerals == endpoint, "representable monolith endpoint clamped" );
		}
		test.inputs.terrain.monolith = false;
		test.inputs.improvements.mine = true;
		test.rules.unroaded_mine_limit = 1 - maximum;
		const auto limited = CalculateTileMinerals( test.rules, test.inputs );
		GT_ASSERT( limited.minerals == 1 - int64_t( maximum ) && limited.removed_minerals == maximum, "signed limit or maximum removed amount changed" );
		test.inputs.terrain.borehole = true;
		test.rules.borehole = maximum;
		const auto capped = CalculateTileMinerals( test.rules, test.inputs );
		GT_ASSERT( capped.minerals == 2 && capped.removed_minerals == int64_t( maximum ) - 2, "large technology removed amount changed" );
		test.inputs.terrain.borehole = false;
		test.inputs.terrain.fungus = true;
		test.inputs.context.fungal_capability = maximum;
		for ( const auto planet : { minimum, maximum } ) {
			test.inputs.context.planet = planet;
			GT_ASSERT( CalculateTileMinerals( test.rules, test.inputs ).minerals == 99, "PLANET endpoint or fungal clamp changed" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC mineral calculator rejects invalid resolved metadata", GT() {
		for ( int field = 0 ; field < 6 ; ++field ) {
			auto test = smac_mineral_fixture::cases[ 0 ];
			switch ( field ) {
				case 0: test.inputs.terrain.altitude = static_cast< AltitudeBand >( 8 ); break;
				case 1: test.inputs.terrain.rockiness = -1; break;
				case 2: test.inputs.terrain.rockiness = 4; break;
				case 3: test.inputs.terrain.resource_bonus = static_cast< ResourceBonus >( 4 ); break;
				case 4: test.inputs.terrain.altitude = AltitudeBand::DeepOcean; test.inputs.terrain.resource_bonus = ResourceBonus::Minerals; break;
				case 5: test.inputs.context.fungal_capability = -1; break;
			}
			bool rejected = false;
			try { CalculateTileMinerals( test.rules, test.inputs ); } catch ( const std::invalid_argument& ) { rejected = true; }
			GT_ASSERT( rejected, "invalid or incoherent mineral metadata accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC mineral calculator rejects evaluated overflow before replacement cap or cancellation", GT() {
		const auto minimum = std::numeric_limits< int32_t >::min(), maximum = std::numeric_limits< int32_t >::max();
		for ( int operation = 0 ; operation < 10 ; ++operation ) {
			auto test = smac_mineral_fixture::cases[ 0 ];
			switch ( operation ) {
				case 0: test.inputs.terrain.monolith = true; test.inputs.terrain.landmark_bonus = true; test.inputs.terrain.resource_bonus = ResourceBonus::Minerals; test.rules.bonus = maximum; test.rules.monolith = minimum; break;
				case 1: test.inputs.terrain.monolith = true; test.rules.monolith = maximum; test.inputs.terrain.resource_bonus = ResourceBonus::Minerals; break;
				case 2: test.inputs.terrain.borehole = true; test.rules.borehole = minimum; test.inputs.terrain.resource_bonus = ResourceBonus::Minerals; test.rules.bonus = -1; break;
				case 3: test.inputs.terrain.altitude = AltitudeBand::Shelf; test.rules.ocean = maximum; test.inputs.terrain.landmark_bonus = true; break;
				case 4: test.inputs.terrain.altitude = AltitudeBand::Shelf; test.inputs.improvements.mine = true; test.rules.improved_sea = maximum; test.inputs.context.advanced_platform_technology = true; break;
				case 5: test.inputs.terrain.forest = true; test.rules.forest = maximum; test.inputs.terrain.landmark_bonus = true; break;
				case 6: test.inputs.improvements.mine = true; test.rules.unroaded_mine_limit = minimum; break;
				case 7: test.inputs.improvements = { true, true }; test.inputs.terrain.resource_bonus = ResourceBonus::Minerals; test.rules.bonus = maximum; break;
				case 8: test.inputs.terrain.monolith = true; test.rules.monolith = maximum; test.inputs.context.industrial_boom = true; test.inputs.context.industrial_bust = true; break;
				case 9: test.inputs.terrain.monolith = true; test.rules.monolith = minimum; test.inputs.context.industrial_bust = true; break;
			}
			bool rejected = false;
			try { CalculateTileMinerals( test.rules, test.inputs ); } catch ( const std::overflow_error& ) { rejected = true; }
			GT_ASSERT( rejected, "unverified evaluated overflow accepted" );
		}
		GT_OK();
	} );
}

}

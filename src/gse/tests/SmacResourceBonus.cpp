#include "SmacResourceBonus.h"

#include "fixtures/SmacResourceBonus.h"
#include "task/gsetests/GSETests.h"

#include <limits>
#include <stdexcept>

namespace gse::tests {

void AddSmacResourceBonusTests( task::gsetests::GSETests* task ) {
	using namespace game::backend::resource::smac;
	for ( const auto& test : smac_resource_fixture::cases ) {
		task->AddTest( std::string( "SMAC " ) + test.id + ": " + test.name, GT( test ) {
			GT_ASSERT( ResolveResourceBonus( test.inputs ) == test.expected, "original-x86 checked resource expectation changed" );
			GT_OK();
		} );
	}
	task->AddTest( "SMAC resource resolution uses floor divisions and nonnegative phases", GT() {
		// These map coordinates are nonnegative, but v=(x-y)/2 is negative.
		// Truncating v/4 toward zero would suppress both automatic resources.
		ResourceBonusInputs input( 0, 2, AltitudeBand::Land3, 10, true, false, false, false, false );
		GT_ASSERT( ResolveResourceBonus( input ) == ResourceBonus::Energy, "negative-v floor/residue changed on land" );
		input.altitude_band = AltitudeBand::Shelf;
		GT_ASSERT( ResolveResourceBonus( input ) == ResourceBonus::Minerals, "negative-v shelf phase changed" );
		input.x = 0; input.y = 6; input.pattern_offset = 4;
		GT_ASSERT( ResolveResourceBonus( input ) == ResourceBonus::Minerals, "negative-v phase wrap changed" );
		GT_OK();
	} );
	task->AddTest( "SMAC resource resolution distinguishes offset zero from nonzero multiples of sixteen", GT() {
		ResourceBonusInputs input( 0, 0, AltitudeBand::Land3, 0, true, false, false, false, false );
		for ( const int32_t offset : { 0, 16, -16, std::numeric_limits< int32_t >::min() } ) {
			input.pattern_offset = offset;
			GT_ASSERT( ResolveResourceBonus( input ) == ( offset == 0 ? ResourceBonus::None : ResourceBonus::Nutrients ), "offset zero gate was replaced by modulo equivalence" );
		}
		input.pattern_offset = std::numeric_limits< int32_t >::max();
		GT_ASSERT( ResolveResourceBonus( input ) == ResourceBonus::None, "representable maximum offset phase changed" );
		GT_OK();
	} );
	task->AddTest( "SMAC resource coordinate types cover every phase and altitude band", GT() {
		const std::array< ResourceBonus, 16 > land = {
			ResourceBonus::Nutrients, ResourceBonus::Nutrients, ResourceBonus::Minerals, ResourceBonus::Energy, ResourceBonus::Nutrients,
			ResourceBonus::Nutrients, ResourceBonus::Nutrients, ResourceBonus::Minerals, ResourceBonus::Energy, ResourceBonus::Nutrients,
			ResourceBonus::Nutrients, ResourceBonus::Nutrients, ResourceBonus::Minerals, ResourceBonus::Energy, ResourceBonus::Nutrients, ResourceBonus::Nutrients
		};
		const std::array< ResourceBonus, 3 > shelf = { ResourceBonus::Nutrients, ResourceBonus::Minerals, ResourceBonus::Energy };
		for ( int32_t phase = 0 ; phase < 16 ; ++phase ) {
			const auto u = 4 + phase % 4, v = phase / 4;
			for ( uint8_t band = 0 ; band < 8 ; ++band ) {
				const auto expected = band < 2 ? ResourceBonus::None : band == 2 ? shelf[ phase % 3 ] : land[ phase ];
				ResourceBonusInputs input( u + v, u - v, static_cast< AltitudeBand >( band ), 0, false, true, false, false, false );
				GT_ASSERT( ResolveResourceBonus( input ) == expected, "coordinate type table or altitude gate changed" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC resource resolution rejects invalid logical coordinates and altitude bands", GT() {
		for ( const auto input : {
			ResourceBonusInputs( -2, 0, AltitudeBand::Land3, 0, true, true, false, false, false ),
			ResourceBonusInputs( 0, -2, AltitudeBand::Land3, 0, true, true, false, false, false ),
			ResourceBonusInputs( 1, 0, AltitudeBand::Land3, 0, true, true, false, false, false ),
			ResourceBonusInputs( 0, 1, AltitudeBand::Land3, 0, true, true, false, false, false ),
			ResourceBonusInputs( 0, 0, static_cast< AltitudeBand >( 8 ), 0, true, true, false, false, false ),
			ResourceBonusInputs( 0, 0, static_cast< AltitudeBand >( 255 ), 0, true, true, false, false, false ),
		} ) {
			bool rejected = false;
			try { ResolveResourceBonus( input ); }
			catch ( const std::invalid_argument& ) { rejected = true; }
			GT_ASSERT( rejected, "invalid decoded resource inputs were accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC resource resolution rejects evaluated signed overflow without clamping inputs", GT() {
		const auto maximum = std::numeric_limits< int32_t >::max();
		const auto minimum = std::numeric_limits< int32_t >::min();
		for ( const auto input : {
			ResourceBonusInputs( maximum, 1, AltitudeBand::Land3, 0, true, true, false, false, false ),
			ResourceBonusInputs( maximum, 1, AltitudeBand::DeepOcean, 0, true, true, false, false, false ),
			ResourceBonusInputs( maximum, 1, AltitudeBand::Trench, 0, true, true, false, false, false ),
			ResourceBonusInputs( 0, 2, AltitudeBand::Land3, maximum, true, false, false, false, false ),
			ResourceBonusInputs( 8, 0, AltitudeBand::Land3, minimum, true, false, false, false, false ),
		} ) {
			bool rejected = false;
			try { ResolveResourceBonus( input ); }
			catch ( const std::overflow_error& ) { rejected = true; }
			GT_ASSERT( rejected, "unverified signed overflow was accepted" );
		}
		ResourceBonusInputs input( maximum - 1, 0, AltitudeBand::Land7, 0, true, true, false, false, false );
		GT_ASSERT( ResolveResourceBonus( input ) == ResourceBonus::Nutrients, "representable large coordinate result changed" );
		input.x = 8; input.pattern_offset = minimum;
		GT_ASSERT( ResolveResourceBonus( input ) == ResourceBonus::Nutrients, "forced presence evaluated an unused automatic-pattern expression" );
		GT_OK();
	} );
}

}

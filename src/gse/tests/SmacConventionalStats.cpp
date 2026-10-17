#include "SmacConventionalStats.h"
#include "game/backend/unit/SmacConventionalStats.h"
#include "task/gsetests/GSETests.h"

#include <array>
#include <functional>
#include <stdexcept>

namespace gse::tests {
namespace {
using namespace game::backend::unit;
using Mode = SmacMoraleMode;
using Domain = SmacMovementDomain;
template< typename E > bool Rejects( const std::function< void() >& fn ) {
	try { fn(); } catch ( const E& ) { return true; } return false;
}
}

void AddSmacConventionalStatsTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC conventional morale preserves retained mapping and ordered home effects", GT() {
		const std::array< int32_t, 13 > expected{ 0, 0, 0, 1, 2, 2, 3, 4, 4, 5, 6, 6, 6 };
		for ( int32_t retained = -6; retained <= 6; ++retained ) {
			GT_ASSERT( SmacConventionalMorale( { 3, retained, 0, false, {}, Mode::Mode0 } ) == expected[ retained + 6 ], "retained rating mapping differs" );
		}
		struct Case { SmacConventionalMoraleInput input; int32_t expected; };
		const Case cases[] = {
			{ { 1, -2, 0, false, SmacMoraleHome{ false, true }, Mode::Mode0 }, 0 },
			{ { 1, -2, 0, false, SmacMoraleHome{ false, true }, Mode::Mode1 }, 1 },
			{ { 3, -3, -1, false, SmacMoraleHome{ true, false }, Mode::Mode0 }, 2 },
			{ { 3, -3, 1, false, SmacMoraleHome{ true, false }, Mode::Mode0 }, 3 },
			{ { 3, -4, 1, false, SmacMoraleHome{ true, false }, Mode::Mode0 }, 3 },
			{ { 3, -4, -1, false, SmacMoraleHome{ true, false }, Mode::Mode0 }, 1 },
			{ { 3, -4, -1, true, SmacMoraleHome{ true, true }, Mode::Mode1 }, 3 },
			{ { 1, 4, 1, false, SmacMoraleHome{ false, true }, Mode::Mode1 }, 4 },
			{ { 1, 4, 1, true, SmacMoraleHome{ false, true }, Mode::Mode1 }, 5 },
			{ { 6, INT32_MIN, 0, false, {}, Mode::Mode1 }, 3 },
			{ { 0, INT32_MAX, 0, false, {}, Mode::Mode0 }, 3 },
			{ { 7, -4, 0, false, {}, Mode::Mode0 }, 4 },
			{ { 255, -4, 0, false, {}, Mode::Mode0 }, 6 },
		};
		for ( const auto& c : cases ) { GT_ASSERT( SmacConventionalMorale( c.input ) == c.expected, "ordered morale interaction differs" ); }
		GT_OK();
	} );
	task->AddTest( "SMAC conventional allowance applies elite maritime and damage in original order", GT() {
		struct Case { SmacConventionalAllowanceInput input; int32_t expected; };
		const Case cases[] = {
			{ { 3, 3, Domain::Land, 1, 0, 1, false, false }, 3 },
			{ { 3, 3, Domain::Land, 1, 9, 1, false, false }, 3 },
			{ { 3, 3, Domain::Land, 1, 4, 6, false, false }, 6 },
			{ { 3, 3, Domain::Land, 1, 5, 6, false, false }, 3 },
			{ { 3, 3, Domain::Land, 1, 4, 6, false, true }, 3 },
			{ { 12, 3, Domain::Sea, 1, 2, 1, false, false }, 12 },
			{ { 12, 3, Domain::Sea, 1, 3, 1, false, false }, 9 },
			{ { 12, 3, Domain::Sea, 1, 5, 1, false, false }, 6 },
			{ { 12, 3, Domain::Sea, 1, 3, 6, true, false }, 15 },
			{ { 12, 3, Domain::Sea, 1, 3, 6, true, true }, 15 },
			{ { 3, 3, Domain::Land, 0, 9, 1, true, false }, 3 },
			{ { 3, 3, Domain::Land, 255, 1, 1, false, false }, 33 },
			{ { 3, 3, Domain::Land, 10, 1, 1, false, false }, 3 },
			{ { 3, 3, Domain::Land, 10, 0, 1, false, false }, 3 },
			{ { 6, 3, Domain::Land, 10, 1, 1, false, false }, 6 },
			{ { 3, 3, Domain::Land, 1, 255, 1, false, false }, 3 },
			{ { 12, 3, Domain::Sea, 1, 255, 6, true, false }, 6 },
			{ { 30, 3, Domain::Air, 1, 255, 6, true, false }, 33 },
			{ { 30, 3, Domain::Air, 1, 255, 6, true, true }, 30 },
			{ { 5, 3, Domain::Land, 1, 1, 1, false, false }, 3 },
			{ { 30000, 3, Domain::Land, 100, 1, 1, false, false }, 2997 },
			{ { 0, 3, Domain::Land, 1, 0, 1, false, false }, 0 },
			{ { 0, 3, Domain::Land, 1, 1, 1, false, false }, 3 },
		};
		for ( const auto& c : cases ) { GT_ASSERT( SmacConventionalAllowance( c.input ) == c.expected, "ordered allowance interaction differs" ); }
		GT_OK();
	} );
	task->AddTest( "SMAC conventional stat metadata is explicit and validated", GT() {
		for ( const auto value : { INT32_MIN, -1, 7, INT32_MAX } ) {
			GT_ASSERT( Rejects< std::invalid_argument >( [ & ]() { SmacConventionalAllowance( { 3, 3, Domain::Land, 1, 0, value, false, false } ); } ), "invalid effective morale accepted" );
		}
		for ( const auto value : { INT32_MIN, -1, 256, INT32_MAX } ) {
			GT_ASSERT( Rejects< std::invalid_argument >( [ & ]() { SmacConventionalMorale( { value, 0, 0, false, {}, Mode::Mode0 } ); } ), "invalid stored morale accepted" );
		}
		GT_ASSERT( Rejects< std::invalid_argument >( []() { SmacConventionalMorale( { 1, 0, 0, false, {}, static_cast< Mode >( 2 ) } ); } ), "invalid query mode accepted" );
		GT_ASSERT( Rejects< std::invalid_argument >( []() { SmacConventionalAllowance( { 3, 3, static_cast< Domain >( 3 ), 1, 0, 1, false, false } ); } ), "invalid domain accepted" );
		for ( const auto q : { INT32_MIN, -1, 0 } ) { GT_ASSERT( Rejects< std::invalid_argument >( [ & ]() { SmacConventionalAllowance( { 3, q, Domain::Land, 1, 0, 1, false, false } ); } ), "invalid quantum accepted" ); }
		GT_ASSERT( Rejects< std::invalid_argument >( []() { SmacConventionalAllowance( { -1, 3, Domain::Land, 1, 0, 1, false, false } ); } ), "negative basic allowance accepted" );
		GT_OK();
	} );
	task->AddTest( "SMAC conventional stats check only evaluated signed arithmetic before clamps", GT() {
		GT_ASSERT( Rejects< std::overflow_error >( []() { SmacConventionalMorale( { 1, -4, INT32_MIN, true, SmacMoraleHome{ true, true }, Mode::Mode1 } ); } ), "early negative overflow hidden by later immunity" );
		GT_ASSERT( Rejects< std::overflow_error >( []() { SmacConventionalMorale( { 0, 4, INT32_MAX, false, {}, Mode::Mode0 } ); } ), "positive modifier overflow hidden" );
		GT_ASSERT( Rejects< std::overflow_error >( []() { SmacConventionalMorale( { 0, 0, INT32_MIN, false, SmacMoraleHome{ false, true }, Mode::Mode1 } ); } ), "riot subtraction overflow hidden" );
		GT_ASSERT( Rejects< std::overflow_error >( []() { SmacConventionalMorale( { 6, 0, INT32_MAX, false, {}, Mode::Mode0 } ); } ), "final sum overflow hidden by clamp" );
		GT_ASSERT( SmacConventionalMorale( { 0, 0, INT32_MAX, false, {}, Mode::Mode0 } ) == 6, "representable extreme rejected" );
		GT_ASSERT( SmacConventionalAllowance( { 0, INT32_MAX, Domain::Land, 1, 0, 1, true, false } ) == 0, "unused maritime multiplication evaluated" );
		GT_ASSERT( SmacConventionalAllowance( { INT32_MAX, INT32_MAX, Domain::Land, 1, 1, 1, false, false } ) == INT32_MAX, "representable ceil/result rejected" );
		GT_ASSERT( SmacConventionalAllowance( { INT32_MAX, 1, Domain::Air, 255, 255, 1, false, false } ) == INT32_MAX, "unused air damage arithmetic evaluated" );
		GT_ASSERT( Rejects< std::overflow_error >( []() { SmacConventionalAllowance( { 0, INT32_MAX, Domain::Sea, 1, 0, 1, true, false } ); } ), "maritime overflow hidden" );
		GT_ASSERT( Rejects< std::overflow_error >( []() { SmacConventionalAllowance( { INT32_MAX, 1, Domain::Land, 1, 255, 6, false, false } ); } ), "elite overflow hidden by damage" );
		GT_ASSERT( Rejects< std::overflow_error >( []() { SmacConventionalAllowance( { INT32_MAX, 1, Domain::Land, 100, 1, 1, false, false } ); } ), "damage product overflow hidden by cap" );
		GT_ASSERT( Rejects< std::overflow_error >( []() { SmacConventionalAllowance( { 0, INT32_MAX, Domain::Sea, 1, 255, 1, false, false } ); } ), "minimum allowance result overflow hidden" );
		GT_OK();
	} );
}
}

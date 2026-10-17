#include "SmacEmptyGroundMovement.h"

#include "game/backend/unit/SmacEmptyGroundMovement.h"
#include "fixtures/SmacEmptyGroundMovement.h"
#include "task/gsetests/GSETests.h"

#include <limits>
#include <stdexcept>

namespace gse::tests {
namespace {
namespace move = game::backend::unit::smac_move;
namespace fixture = fixtures::smac_empty_ground;
using Random = game::backend::SmacGameplayRandomState;
constexpr uint64_t ID = 0x8000000012345678ull;
constexpr uint32_t CRT = 0xfedcba98u;

std::string CheckCase( const fixture::Case& c ) {
	const move::MovementState unit( ID, c.spent, c.marker );
	const Random random( c.seed, CRT );
	const auto result = move::ResolveEmptyGroundStep( unit, 3, c.allowance, c.cost, c.fungus, random );
	const auto expected_outcome = c.moved ? move::Outcome::Moved : c.wrapper_return == 0 ? move::Outcome::Exhausted : move::Outcome::FailedAttempt;
	GT_ASSERT( result.outcome == expected_outcome && result.original_wrapper_return == c.wrapper_return, "original outcome/return differs: " + std::string( c.name ) );
	GT_ASSERT( result.unit.identity == ID && result.unit.spent == c.spent_after && result.unit.prior_failure_marker == c.marker_after, "original unit state differs: " + std::string( c.name ) );
	GT_ASSERT( result.position == ( c.moved ? move::Position::Destination : move::Position::Source ) && result.unit_present[ 0 ] == c.source_present && result.unit_present[ 1 ] == c.destination_present, "original position/membership differs: " + std::string( c.name ) );
	GT_ASSERT( result.random.Shared() == c.after && result.random.Crt() == CRT, "original RNG or CRT isolation differs: " + std::string( c.name ) );
	GT_ASSERT( result.draw.has_value() == ( c.draw >= 0 ), "original draw suppression differs: " + std::string( c.name ) );
	if ( result.draw ) {
		GT_ASSERT( result.draw->state_before == c.seed && result.draw->state_after == c.after && result.draw->scaled == c.draw, "original draw differs: " + std::string( c.name ) );
	}
	if ( !c.cost_queried ) {
		GT_ASSERT( result.outcome == move::Outcome::Exhausted && !result.draw, "unqueried original cost affected exhaustion" );
	}
	GT_ASSERT( unit.spent == c.spent && unit.prior_failure_marker == c.marker && random.Shared() == c.seed && random.Crt() == CRT, "pure query changed input" );
	GT_OK();
}
template< typename F > bool Rejects( F f ) {
	try { f(); } catch ( const std::invalid_argument& ) { return true; }
	return false;
}
}

void AddSmacEmptyGroundMovementTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC empty ground movement matches retained ground admission captures", GT() {
		for ( const auto& c : fixture::ADMISSION ) { const auto error = CheckCase( c ); GT_ASSERT( error.empty(), "" + error ); }
		GT_OK();
	} );
	task->AddTest( "SMAC empty ground movement keeps cost relief separate from fungus admission", GT() {
		for ( const auto& c : fixture::FUNGUS ) { const auto error = CheckCase( c ); GT_ASSERT( error.empty(), "" + error ); }
		GT_OK();
	} );
	task->AddTest( "SMAC empty ground movement preserves shared state through failed retry and upkeep", GT() {
		// Actual retained immediate-retry sequence: seed1 gives D1 at C3.
		const auto failed = move::ResolveEmptyGroundStep( { ID, 2, false }, 3, 3, 3, false, Random( 1, CRT ) );
		GT_ASSERT( failed.outcome == move::Outcome::FailedAttempt && failed.unit.spent == 3 && failed.unit.prior_failure_marker && failed.random.Shared() == 1015568748u, "first retained retry request differs" );
		const auto retry = move::ResolveEmptyGroundStep( failed.unit, 3, 3, 3, false, failed.random );
		GT_ASSERT( retry.outcome == move::Outcome::Exhausted && retry.original_wrapper_return == 0 && !retry.draw && retry.random == failed.random && retry.unit.prior_failure_marker, "immediate retry drew or lost its failure marker" );
		for ( const auto& c : fixture::UPKEEP ) {
			const auto first = move::ResolveEmptyGroundStep( { ID, 2, false }, 3, 3, c.first_cost, true, Random( c.seed, CRT ) );
			GT_ASSERT( first.outcome == move::Outcome::FailedAttempt && first.draw && first.draw->scaled == c.first_draw && first.random.Shared() == c.failed_seed && first.unit.spent == c.failed_spent && first.unit.prior_failure_marker == c.failed_marker, "original pre-upkeep failure differs" );
			const auto reset = move::ResetSpentForCheckedUnitUpkeep( first.unit );
			GT_ASSERT( reset.identity == ID && reset.spent == c.reset_spent && reset.prior_failure_marker == c.reset_marker && first.random.Shared() == c.reset_seed, "checked upkeep changed identity/marker or stream" );
			const auto second = move::ResolveEmptyGroundStep( reset, 3, 3, c.next_cost, true, first.random );
			GT_ASSERT( second.draw && second.draw->scaled == c.next_draw && second.random.Shared() == c.final_seed && second.random.Crt() == CRT, "continuous original post-upkeep draw differs" );
			GT_ASSERT( ( second.outcome == move::Outcome::Moved ) == c.moved && second.unit.spent == c.spent && second.unit.prior_failure_marker == c.marker && second.original_wrapper_return == c.wrapper_return, "original post-upkeep result differs" );
			GT_ASSERT( second.position == ( c.moved ? move::Position::Destination : move::Position::Source ) && second.unit_present[ 0 ] == !c.moved && second.unit_present[ 1 ] == c.moved, "post-upkeep membership differs" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC empty ground movement preserves exhaustion and owned stable identities", GT() {
		for ( const auto id : { uint64_t( 0 ), std::numeric_limits< uint64_t >::max() } ) {
			for ( const auto cost : { 0, 3, 9 } ) {
				for ( const auto spent : { 3, 4 } ) {
					for ( const auto marker : { false, true } ) {
						const Random random( 0xffffffffu, 0x80000000u );
						const auto result = move::ResolveEmptyGroundStep( { id, spent, marker }, 3, 3, cost, true, random );
						GT_ASSERT( result.outcome == move::Outcome::Exhausted && result.unit.identity == id && result.unit.spent == spent && result.unit.prior_failure_marker == marker && result.random == random && !result.draw && result.original_wrapper_return == 0, "exhaustion changed state or admitted zero cost" );
					}
				}
			}
		}
		move::MovementState source( ID, 2, true ); Random random( 3, CRT );
		const auto zero_cost = move::ResolveEmptyGroundStep( source, 3, 3, 0, false, random );
		source.identity = 0; source.spent = 9; random.ExchangeShared( 0 );
		GT_ASSERT( zero_cost.outcome == move::Outcome::Moved && zero_cost.unit.identity == ID && zero_cost.unit.spent == 2 && !zero_cost.unit.prior_failure_marker && zero_cost.random.Shared() == 3 && !zero_cost.draw, "owned zero-cost result aliases input or fails to clear marker" );
		GT_OK();
	} );
	task->AddTest( "SMAC empty ground movement admits explicit bounded port arithmetic", GT() {
		const Random random( 0xffffffffu, CRT );
		const auto upper = move::ResolveEmptyGroundStep( { ID, 0, true }, 100, 254, 255, true, random );
		GT_ASSERT( upper.outcome == move::Outcome::Moved && upper.unit.spent == 255 && upper.draw && upper.draw->raw == 36178 && upper.draw->scaled == 140 && upper.random.Shared() == 1012239698u && upper.random.Crt() == CRT, "upper domain draw/scaled cost/threshold differs" );
		const auto lower = move::ResolveEmptyGroundStep( { ID, 0, false }, 1, 1, 2, false, random );
		GT_ASSERT( lower.outcome == move::Outcome::Moved && lower.unit.spent == 2 && !lower.draw && lower.random == random, "one full quantum should bypass ordinary over-cost lottery" );
		const auto zero = move::ResolveEmptyGroundStep( { ID, 0, true }, 1, 0, 0, false, random );
		GT_ASSERT( zero.outcome == move::Outcome::Exhausted && zero.unit.prior_failure_marker && zero.random == random, "zero allowance moved on zero edge" );
		GT_OK();
	} );
	task->AddTest( "SMAC empty ground movement rejects unproved byte wraps and malformed inputs", GT() {
		using Inputs = std::array< int32_t, 4 >; // Q, allowance, cost, spent
		for ( const auto& c : { Inputs{ 0, 3, 3, 0 }, Inputs{ 101, 3, 3, 0 }, Inputs{ -1, 3, 3, 0 }, Inputs{ 3, -1, 3, 0 }, Inputs{ 3, 256, 3, 0 }, Inputs{ 3, 3, -1, 0 }, Inputs{ 3, 3, 256, 0 }, Inputs{ 3, 3, 3, -1 }, Inputs{ 3, 3, 3, 256 }, Inputs{ 3, 255, 2, 254 }, Inputs{ 3, 0, 1, 255 }, Inputs{ std::numeric_limits< int32_t >::max(), 3, 3, 0 }, Inputs{ 3, 3, std::numeric_limits< int32_t >::max(), 0 } } ) {
			const move::MovementState unit( ID, c[ 3 ], true ); const Random random( 3, CRT );
			GT_ASSERT( Rejects( [ & ]() { move::ResolveEmptyGroundStep( unit, c[ 0 ], c[ 1 ], c[ 2 ], false, random ); } ), "unsupported port domain accepted" );
			GT_ASSERT( unit.spent == c[ 3 ] && unit.prior_failure_marker && random.Shared() == 3 && random.Crt() == CRT, "rejection changed input" );
		}
		GT_ASSERT( Rejects( []() { move::ResetSpentForCheckedUnitUpkeep( { ID, -1, true } ); } ) && Rejects( []() { move::ResetSpentForCheckedUnitUpkeep( { ID, 256, true } ); } ), "upkeep accepted invalid byte state" );
		GT_OK();
	} );
}

}

#include "SmacAutomaticWorkerAllocation.h"
#include "game/backend/base/SmacAutomaticWorkerAllocation.h"
#include "fixtures/SmacAutomaticWorkerAllocation.h"
#include "task/gsetests/GSETests.h"

#include <stdexcept>
#include <vector>

namespace gse::tests {
namespace {
using namespace game::backend::base;
namespace fixture = smac_automatic_worker_fixture;

std::string CheckFamily( int32_t family, size_t expected_count ) {
	size_t count = 0;
	for ( const auto& row : fixture::cases ) {
		if ( row.family != family ) { continue; }
		++count;
		const auto input = row.input;
		const auto result = SmacAutomaticWorkerAllocation::Allocate( input,
			[ & ]( int32_t index, const SmacWorkerStepState& ) -> std::optional< SmacWorkerTileYield > {
				// The original full-routine fixtures independently expose exactly
				// these three usable land squares. Return fresh supplied yields on
				// every invocation; no scores or allocation decisions are supplied.
				if ( index < 0 || index > 2 ) { throw std::runtime_error( "Unavailable original candidate was queried" ); }
				return SmacWorkerTileYield{ row.yields[ index ], false };
			} );
		const auto label = std::string( row.id );
		if ( result.workers.visited != row.visited || result.workers.intake != row.intake
			|| result.workers.remaining_slots != row.count || result.specialists.total_count != row.count ) {
			return "Original final workers/intake/count differ: " + label;
		}
		for ( size_t i = 0; i < row.reported_types; ++i ) {
			if ( result.specialists.types[ i ] != row.types[ i ] ) { return "Original stored specialist prefix differs: " + label; }
		}
		// Only finalization and the new observation-only report record this
		// boundary. Earlier cleanup observations are not relabeled as pre-fill.
		if ( row.before_fill && ( result.before_fill.retained != ( *row.before_fill )[ 0 ]
			|| result.before_fill.provisional != ( *row.before_fill )[ 1 ] ) ) {
			return "Observed original pre-fill counters differ: " + label;
		}
		if ( input.selected != row.input.selected || input.prepared_intake != row.input.prepared_intake
			|| input.specialists.types != row.input.specialists.types || input.specialists.total_count != row.input.specialists.total_count ) {
			return "Detached input was changed: " + label;
		}
	}
	return count == expected_count ? "" : "Original full-routine family count differs";
}

bool SameState( const fixture::Query& a, const fixture::Query& b ) {
	return a.visited == b.visited && a.slots == b.slots && a.intake == b.intake;
}

std::string CheckObservedQueries() {
	size_t count = 0, scans = 0, repairs = 0;
	for ( const auto& row : fixture::cases ) {
		if ( row.family != 4 ) { continue; }
		++count;
		std::vector< fixture::Query > queries;
		SmacAutomaticWorkerAllocation::Allocate( row.input,
			[ & ]( int32_t index, const SmacWorkerStepState& state ) -> std::optional< SmacWorkerTileYield > {
				if ( index < 0 || index > 2 ) { throw std::runtime_error( "Excluded original candidate was queried" ); }
				queries.push_back( { index, state.visited, state.remaining_slots, state.intake } );
				return SmacWorkerTileYield{ row.yields[ index ], false };
			} );
		const auto label = std::string( row.id );
		std::vector< fixture::Query > observed_scans;
		for ( const auto& actual : queries ) {
			for ( const auto& expected : row.scan_queries ) {
				if ( SameState( actual, expected ) ) { observed_scans.push_back( actual ); break; }
			}
		}
		// These exact states are recorded before scoring any admitted tile.
		// Collection/addition/swap callbacks have different staged states in
		// this corpus. Do not invent an original trace for those other calls.
		if ( observed_scans.size() != row.scan_queries.size() ) { return "Original candidate query count/state differs: " + label; }
		for ( size_t i = 0; i < observed_scans.size(); ++i ) {
			if ( observed_scans[ i ].index != row.scan_queries[ i ].index || !SameState( observed_scans[ i ], row.scan_queries[ i ] ) ) {
				return "Original candidate query order/state differs: " + label;
			}
		}
		scans += observed_scans.size();
		if ( row.repair_center ) {
			size_t matching = 0;
			for ( const auto& actual : queries ) {
				if ( actual.index == 0 && SameState( actual, *row.repair_center ) ) { ++matching; }
			}
			// Original selectors0..3 share visited/slots; selector0 observes
			// the intake before addition. The port has ONE whole-tile query.
			if ( matching != 1 ) { return "Missing center was not queried before marking/spending: " + label; }
			++repairs;
		}
	}
	return count == 20 && scans == 26 && repairs == 8 ? "" : "Original orchestration trace coverage differs";
}
}

void AddSmacAutomaticWorkerAllocationTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC automatic worker allocation matches ten original complete intake returns", GT() {
		return CheckFamily( 0, 10 );
	} );
	task->AddTest( "SMAC automatic worker allocation preserves seven original cleanup completion paths", GT() {
		return CheckFamily( 1, 7 );
	} );
	task->AddTest( "SMAC automatic worker allocation matches seven observed specialist fill histories", GT() {
		return CheckFamily( 2, 7 );
	} );
	task->AddTest( "SMAC automatic worker allocation matches seven original governor priority choices", GT() {
		return CheckFamily( 3, 7 );
	} );
	task->AddTest( "SMAC automatic worker allocation matches twenty complete original orchestration returns", GT() {
		return CheckFamily( 4, 20 );
	} );
	task->AddTest( "SMAC automatic worker allocation preserves observed scan and repair query states", GT() {
		return CheckObservedQueries();
	} );
}

}

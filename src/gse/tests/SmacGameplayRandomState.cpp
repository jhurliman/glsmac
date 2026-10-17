#include "SmacGameplayRandomState.h"

#include "game/backend/SmacGameplayRandomState.h"
#include "fixtures/SmacGameplayRandomState.h"
#include "task/gsetests/GSETests.h"

#include <array>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace gse::tests {
namespace {
using State = game::backend::SmacGameplayRandomState;
using namespace fixtures::smac_gameplay_random;
static_assert( !std::is_default_constructible_v< State >, "Both gameplay states must be supplied" );

bool Rejects( const std::string& wire ) {
	try { State::Deserialize( wire ); } catch ( const std::invalid_argument& ) { return true; }
	return false;
}

std::array< uint16_t, 4 > Draw( State& state, const Replay& c ) {
	std::array< uint16_t, 4 > result{};
	for ( auto& value : result ) {
		// This is the original fixture caller's interval 0..8, not a production
		// interval API. A draw here always advances its chosen stream exactly once.
		value = c.shared ? static_cast< uint16_t >( uint64_t( state.NextSharedLow16() ) * c.width / 65536 ) : state.NextCrtRaw15();
	}
	return result;
}
}

void AddSmacGameplayRandomStateTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC gameplay RNG matches original supplied-state draw and replay records", GT() {
		for ( const auto& c : REPLAYS ) {
			State state( c.shared ? c.prior : c.companion, c.shared ? c.companion : c.prior );
			if ( c.shared ) {
				GT_ASSERT( state.ExchangeShared( c.supplied ) == c.prior, "exchange lost prior state: " + std::string( c.id ) );
				GT_ASSERT( state.Shared() == c.supplied, "shared snapshot changed supplied bits: " + std::string( c.id ) );
			} else {
				state.SetCrt( c.supplied );
				GT_ASSERT( state.Crt() == c.supplied, "CRT setter changed supplied bits: " + std::string( c.id ) );
			}
			GT_ASSERT( Draw( state, c ) == c.draws, "original draws differ: " + std::string( c.id ) );
			GT_ASSERT( ( c.shared ? state.Shared() : state.Crt() ) == c.final_state, "original final state differs: " + std::string( c.id ) );
			if ( c.shared ) {
				GT_ASSERT( state.ExchangeShared( c.supplied ) == c.final_state, "restore lost previous state: " + std::string( c.id ) );
			} else { state.SetCrt( c.supplied ); }
			GT_ASSERT( Draw( state, c ) == c.draws, "restored draws differ: " + std::string( c.id ) );
			GT_ASSERT( ( c.shared ? state.Shared() : state.Crt() ) == c.final_state, "replayed final state differs: " + std::string( c.id ) );
			GT_ASSERT( ( c.shared ? state.Crt() : state.Shared() ) == c.companion, "independent stream changed: " + std::string( c.id ) );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC gameplay RNG supplied writes do not imply lifecycle defaults", GT() {
		State state( RESET.prior_shared, RESET.crt );
		GT_ASSERT( state.Shared() == RESET.prior_shared && state.Crt() == RESET.crt, "construction seeded or drew" );
		state.ExchangeShared( 0 );
		GT_ASSERT( state.Shared() == RESET.after_first, "explicit first reset write differs" );
		state.ExchangeShared( RESET.before_second_write );
		state.ExchangeShared( 0 );
		GT_ASSERT( state.Shared() == RESET.after_second && state.Crt() == RESET.final_crt, "explicit reset affected CRT" );
		// These reproduce supplied setter effects only. No module registration,
		// time service, new-game initialization or original save loader executes.
		for ( const auto& c : SAMPLES ) {
			State supplied( c.previous_shared, c.previous_crt );
			supplied.SetCrt( c.first ); supplied.ExchangeShared( c.second );
			GT_ASSERT( supplied.Shared() == c.expected_shared && supplied.Crt() == c.expected_crt, "supplied writes differ: " + std::string( c.id ) );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC gameplay RNG codec matches independent bytes and full-width words", GT() {
		const std::string expected( "SMGR\x01\x00\x00\x00\x78\x56\x34\x12\xf0\xde\xbc\x9a", 16 );
		const State original( 0x12345678u, 0x9abcdef0u );
		GT_ASSERT( original.Serialize() == expected && State::Deserialize( expected ) == original, "version, endian or stream order changed" );
		for ( const auto shared : { 0u, 1u, 0x7fffffffu, 0x80000000u, 0xffffffffu } ) {
			for ( const auto crt : { 0u, 1u, 0x7fffffffu, 0x80000000u, 0xffffffffu } ) {
				const State source( shared, crt );
				GT_ASSERT( State::Deserialize( source.Serialize() ) == source, "full-width word was normalized or rejected" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC gameplay RNG restore owns state and interleaved continuation", GT() {
		State source( 0xffffffffu, 0x80000000u );
		source.NextCrtRaw15(); source.NextSharedLow16(); source.NextCrtRaw15();
		auto bytes = source.Serialize(); auto restored = State::Deserialize( bytes ); auto copied = restored; auto moved = std::move( copied );
		bytes.assign( bytes.size(), 'x' ); restored.ExchangeShared( 42 ); restored.SetCrt( 81 );
		GT_ASSERT( moved == source, "restored/copy state aliases mutated data" );
		for ( unsigned i = 0; i < 64; ++i ) {
			const auto old_shared = moved.Shared();
			GT_ASSERT( moved.NextCrtRaw15() == source.NextCrtRaw15() && moved.Shared() == old_shared, "CRT continuation or isolation changed" );
			const auto old_crt = moved.Crt();
			GT_ASSERT( moved.NextSharedLow16() == source.NextSharedLow16() && moved.Crt() == old_crt, "shared continuation or isolation changed" );
			GT_ASSERT( moved == source, "continued states differ" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC gameplay RNG codec rejects malformed records without state mutation", GT() {
		const State original( 0x12345678u, 0x9abcdef0u ); const auto wire = original.Serialize();
		for ( size_t size = 0; size < wire.size(); ++size ) {
			GT_ASSERT( Rejects( wire.substr( 0, size ) ), "truncated fixed record accepted" );
		}
		GT_ASSERT( Rejects( wire + '\0' ) && Rejects( wire + wire ), "trailing record bytes accepted" );
		for ( size_t i = 0; i < 4; ++i ) {
			auto bad = wire; bad[ i ] = '\0'; GT_ASSERT( Rejects( bad ), "wrong record marker accepted" );
		}
		for ( const auto version : { 0u, 2u, 0xffffffffu } ) {
			auto bad = wire;
			for ( unsigned i = 0; i < 4; ++i ) { bad[ 4 + i ] = static_cast< char >( static_cast< unsigned char >( version >> ( i * 8 ) ) ); }
			GT_ASSERT( Rejects( bad ), "unknown version accepted" );
		}
		GT_ASSERT( original.Serialize() == wire, "decode failure changed existing state" );
		GT_OK();
	} );
}

}

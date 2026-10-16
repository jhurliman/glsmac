#include "CvrTransformCalculator.h"

#include "loader/cvr/CvrTransformCalculator.h"
#include "task/gsetests/GSETests.h"

#include <cfenv>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace gse::tests {
namespace {

using loader::cvr::CvrTransformCalculator;
using loader::cvr::CvrTransformInput;
using loader::cvr::CvrTransformResult;

uint32_t Bits( const float value ) {
	uint32_t bits;
	static_assert( sizeof( bits ) == sizeof( value ) );
	std::memcpy( &bits, &value, sizeof( bits ) );
	return bits;
}

template< std::size_t N >
std::array< uint32_t, N > Bits( const std::array< float, N >& values ) {
	std::array< uint32_t, N > bits;
	for ( std::size_t i = 0; i < N; ++i ) { bits[ i ] = Bits( values[ i ] ); }
	return bits;
}

CvrTransformInput Identity() {
	CvrTransformInput input{};
	input.transform_bits = input.frame_matrix_bits = { Bits( 1 ), 0, 0, 0, Bits( 1 ), 0, 0, 0, Bits( 1 ) };
	input.scalar_bits = Bits( 1 );
	return input;
}

bool Rejected( const CvrTransformInput& input ) {
	try { CvrTransformCalculator::Calculate( input ); }
	catch ( const std::invalid_argument& ) { return true; }
	return false;
}

class RoundingGuard {
public:
	RoundingGuard() : m_original( std::fegetround() ) {}
	~RoundingGuard() { if ( m_original != -1 ) { std::fesetround( m_original ); } }
private:
	const int m_original;
};

}

void AddCvrTransformCalculatorTests( task::gsetests::GSETests* task ) {
	task->AddTest( "CVR transform composes noncommuting inputs and geometry origin", GT() {
		auto input = Identity();
		input.transform_bits = Bits< 9 >( { .5F, .125F, -.25F, -.375F, .75F, .125F, .125F, -.25F, .5F } );
		input.frame_matrix_bits = Bits< 9 >( { 1, .25F, -.5F, -.125F, 1.5F, .75F, .625F, -.25F, 2 } );
		input.frame_location_bits = Bits< 3 >( { 3, 5, -7 } );
		input.translation_bits = Bits< 3 >( { 2048, 2048, 1024 } );
		input.scalar_bits = Bits( 2 );
		input.header_vector = { 7, -11, 13 };
		const auto before = input;
		const auto result = CvrTransformCalculator::Calculate( input );
		GT_ASSERT( result.matrix_bits == Bits< 9 >( { .65625F, .75F, -1.3125F, -.78125F, 2, 2, .9375F, -.9375F, 1.5F } ), "matrix order/scalar changed" );
		GT_ASSERT( result.location_bits == Bits< 3 >( { 2051.875F, 2049.75F, 1019.625F } ), "location transform/translation changed" );
		GT_ASSERT( result.origin_bits == Bits< 3 >( { 2031.15625F, 2048.28125F, 1056 } ), "geometry header origin changed" );
		const std::array< int32_t, 3 > origin{ 519976, 524360, 270336 };
		const std::array< std::array< int32_t, 3 >, 3 > basis{ { { 168, 192, -336 }, { -200, 512, 512 }, { 240, -240, 384 } } };
		GT_ASSERT( result.origin == origin && result.basis == basis, "fixed outputs changed" );
		GT_ASSERT( input.transform_bits == before.transform_bits && input.translation_bits == before.translation_bits &&
			input.frame_matrix_bits == before.frame_matrix_bits && input.frame_location_bits == before.frame_location_bits &&
			input.scalar_bits == before.scalar_bits && input.header_vector == before.header_vector, "calculator mutated input" );
		GT_OK();
	} );
	task->AddTest( "CVR transform preserves each binary32 store before later arithmetic", GT() {
		auto input = Identity();
		input.transform_bits = { Bits( 1 ), 0x33800000, 0, 0, 0, 0, 0, 0, 0 }; // 1 + 2^-24 tie
		input.frame_matrix_bits = { Bits( 1 ), 0, 0, Bits( 1 ), 0, 0, 0, 0, 0 };
		input.frame_location_bits = { Bits( 1 ), Bits( 1 ), 0 };
		input.translation_bits = { Bits( -1 ), 0, 0 };
		input.scalar_bits = Bits( 3 );
		const auto result = CvrTransformCalculator::Calculate( input );
		GT_ASSERT( result.matrix_bits[ 0 ] == Bits( 3 ), "matrix store skipped before scalar multiplication" );
		GT_ASSERT( result.location_bits[ 0 ] == 0 && result.origin[ 0 ] == 0, "location store skipped before cancellation" );
		// Odd halfway significand rounds upward; the next store then cancels 1.
		input.frame_matrix_bits[ 0 ] = input.frame_location_bits[ 0 ] = 0x3f800001;
		input.scalar_bits = Bits( 1 );
		const auto odd = CvrTransformCalculator::Calculate( input );
		GT_ASSERT( odd.matrix_bits[ 0 ] == 0x3f800002 && odd.location_bits[ 0 ] == 0x34800000, "nearest-even odd tie rounded incorrectly" );
		input.frame_matrix_bits[ 0 ] = input.frame_location_bits[ 0 ] = 0x3fffffff;
		GT_ASSERT( CvrTransformCalculator::Calculate( input ).matrix_bits[ 0 ] == Bits( 2 ), "rounding carry into next exponent changed" );
		input.transform_bits[ 0 ] |= 0x80000000U;
		input.transform_bits[ 1 ] |= 0x80000000U;
		input.frame_matrix_bits[ 0 ] = input.frame_location_bits[ 0 ] = Bits( 1 );
		input.translation_bits[ 0 ] = Bits( 1 );
		input.scalar_bits = Bits( 3 );
		const auto negative = CvrTransformCalculator::Calculate( input );
		GT_ASSERT( negative.matrix_bits[ 0 ] == Bits( -3 ) && negative.location_bits[ 0 ] == 0, "negative tie/cancellation store changed" );
		GT_OK();
	} );
	task->AddTest( "CVR transform admits exact 64-bit carry borrow and cancellation", GT() {
		auto input = Identity();
		input.frame_matrix_bits = {};
		input.transform_bits = { 0x5f800000, Bits( 1 ), 0, 0, 0, 0, 0, 0, 0 }; // 2^64
		input.frame_location_bits = { Bits( 1 ), Bits( -1 ), 0 };
		input.translation_bits = { 0xdf800000, 0, 0 };
		GT_ASSERT( !Rejected( input ), "exact65-bit temporary borrow was incorrectly rejected" );
		GT_ASSERT( CvrTransformCalculator::Calculate( input ).location_bits[ 0 ] == 0, "2^64-1 exact borrow was rejected or rounded incorrectly" );
		input.frame_location_bits[ 1 ] = Bits( 1 );
		GT_ASSERT( Rejected( input ), "2^64+1 inexact odd significand accepted" );
		input.frame_location_bits[ 1 ] = Bits( -1 );
		input.transform_bits[ 0 ] = 0x60000000; // 2^65: low term cannot cancel to64 bits
		GT_ASSERT( Rejected( input ), "large exponent separation accepted" );
		// Products are (2^32-1)*2^32, (2^32-1), 1. All pairs exact;
		// the first sum is UINT64_MAX and the final sum carries to2^64.
		input.transform_bits = Bits< 9 >( { 281470681743360.0F, 65535, 1, 0, 0, 0, 0, 0, 0 } );
		input.frame_location_bits = Bits< 3 >( { 65537, 65537, 1 } );
		GT_ASSERT( CvrTransformCalculator::Calculate( input ).location_bits[ 0 ] == 0, "65th carry bit was lost before normalization" );
		input.frame_location_bits[ 2 ] = Bits( -1 );
		GT_ASSERT( CvrTransformCalculator::Calculate( input ).location_bits[ 0 ] == 0, "final signed subtraction changed rounding" );
		GT_OK();
	} );
	task->AddTest( "CVR transform rejects exceptional input in every binary32 field", GT() {
		for ( const uint32_t invalid : { 0x80000000U, 0x7f800000U, 0xff800000U, 0x7fc00001U, 0x7f800001U } ) {
			for ( std::size_t field = 0; field < 25; ++field ) {
				auto input = Identity();
				if ( field < 9 ) { input.transform_bits[ field ] = invalid; }
				else if ( field < 12 ) { input.translation_bits[ field - 9 ] = invalid; }
				else if ( field < 21 ) { input.frame_matrix_bits[ field - 12 ] = invalid; }
				else if ( field < 24 ) { input.frame_location_bits[ field - 21 ] = invalid; }
				else { input.scalar_bits = invalid; }
				GT_ASSERT( Rejected( input ), "exceptional field was accepted" );
			}
		}
		auto input = Identity();
		for ( const uint32_t invalid : { 0U, 0xbf800000U } ) { input.scalar_bits = invalid; GT_ASSERT( Rejected( input ), "nonpositive scalar accepted" ); }
		input = Identity(); input.transform_bits[ 0 ] = 1;
		GT_ASSERT( Rejected( input ), "nonnormal nonzero store accepted" );
		input.frame_matrix_bits[ 0 ] = 0x4b000000;
		GT_ASSERT( CvrTransformCalculator::Calculate( input ).matrix_bits[ 0 ] == 0x00800000, "subnormal input with normal product rejected" );
		input = Identity(); input.transform_bits[ 1 ] = 0x17800000; // 2^-80
		input.frame_location_bits = Bits< 3 >( { 1, 1, 0 } );
		GT_ASSERT( Rejected( input ), "inexact extended sum accepted" );
		input = Identity(); input.transform_bits[ 0 ] = 0x7f7fffff; input.frame_location_bits[ 0 ] = Bits( 2 );
		GT_ASSERT( Rejected( input ), "binary32 overflow accepted" );
		input = Identity(); input.transform_bits = Bits< 9 >( { -1, -1, -1, 0, 0, 0, 0, 0, 0 } );
		GT_ASSERT( Rejected( input ), "generated negative zero was silently normalized" );
		GT_OK();
	} );
	task->AddTest( "CVR transform keeps conversion limits separate from raster limits", GT() {
		auto input = Identity();
		input.header_vector = { std::numeric_limits< int16_t >::min(), std::numeric_limits< int16_t >::max(), -1 };
		const auto header_result = CvrTransformCalculator::Calculate( input );
		const std::array< int32_t, 3 > header_origin{ -8388608, 8388352, -256 };
		GT_ASSERT( header_result.origin == header_origin, "signed16 header endpoints changed" );
		input.header_vector = {};
		input.translation_bits[ 2 ] = Bits( 40000 );
		GT_ASSERT( CvrTransformCalculator::Calculate( input ).origin[ 2 ] == 10240000, "calculator silently applied later raster depth policy" );
		for ( std::size_t i = 0; i < 3; ++i ) {
			input = Identity(); input.translation_bits[ i ] = 0xcb000000;
			GT_ASSERT( CvrTransformCalculator::Calculate( input ).origin[ i ] == std::numeric_limits< int32_t >::min(), "negative origin endpoint rejected" );
			input.translation_bits[ i ] = 0x4affffff;
			GT_ASSERT( CvrTransformCalculator::Calculate( input ).origin[ i ] == 2147483520, "positive origin endpoint changed" );
			input.translation_bits[ i ] = 0x4b000000;
			GT_ASSERT( Rejected( input ), "out-of-width origin narrowed" );
		}
		input = Identity(); input.scalar_bits = Bits( 128 );
		GT_ASSERT( Rejected( input ), "signed16 basis limit bypassed" );
		for ( const uint32_t sign : { 0U, 0x80000000U } ) {
			input = Identity(); input.translation_bits[ 0 ] = sign | 0x1b800000; // 2^-72
			GT_ASSERT( CvrTransformCalculator::Calculate( input ).origin[ 0 ] == 0, "exact64 origin half-shift threshold rejected" );
			input.translation_bits[ 0 ] = sign | 0x1b000000; // 2^-73
			GT_ASSERT( Rejected( input ), "inexact origin half-shift accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "CVR transform results survive input lifetime and ignore host rounding", GT() {
		const auto calculate = []() {
			auto input = Identity();
			input.translation_bits = { 0x3b000000, 0xbb000000, 0x3c200000 };
			return CvrTransformCalculator::Calculate( input );
		};
		const auto owned = calculate();
		const std::array< int32_t, 3 > expected{ 1, -1, 3 };
		GT_ASSERT( owned.origin == expected, "half-away origin or owned result changed" );
		RoundingGuard restore;
		for ( const int mode : { FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO } ) {
			GT_ASSERT( std::fesetround( mode ) == 0, "test platform cannot set rounding mode" );
			const auto result = calculate();
			GT_ASSERT( result.origin == owned.origin && result.matrix_bits == owned.matrix_bits && result.origin_bits == owned.origin_bits,
				"host rounding changed calculator output" );
			GT_ASSERT( std::fegetround() == mode, "calculator changed caller rounding mode" );
		}
		GT_OK();
	} );
}

}

#include "CvrBasisConverter.h"

#include "loader/cvr/CvrBasisConverter.h"
#include "task/gsetests/GSETests.h"

#include <cfenv>
#include <limits>
#include <stdexcept>
#include <utility>

namespace gse::tests {
namespace {

using loader::cvr::CvrBasisConverter;
using Input = std::array< uint32_t, 9 >;
using Basis = std::array< std::array< int32_t, 3 >, 3 >;

bool Rejected( const Input& input ) {
	try { CvrBasisConverter::Convert( input ); }
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

void AddCvrBasisConverterTests( task::gsetests::GSETests* task ) {
	task->AddTest( "CVR basis preserves original supplied component order", GT() {
		const Input identity{ 0x3f800000, 0, 0, 0, 0x3f800000, 0, 0, 0, 0x3f800000 };
		const Basis expected_identity{ { { 256, 0, 0 }, { 0, 256, 0 }, { 0, 0, 256 } } };
		GT_ASSERT( CvrBasisConverter::Convert( identity ) == expected_identity, "identity conversion differs from original" );
		const Input distinct{ 0x3fa00000, 0xc0200000, 0x40480000, 0xbe800000, 0x3f400000, 0xc0900000, 0x457a0400, 0xc5fa0400, 0x41890000 };
		const Basis expected_distinct{ { { 320, -640, 800 }, { -64, 192, -1152 }, { 1024064, -2048128, 4384 } } };
		GT_ASSERT( CvrBasisConverter::Convert( distinct ) == expected_distinct, "nonidentity order or scaling differs from original" );
		GT_OK();
	} );
	task->AddTest( "CVR basis rounds original halfway neighbors away from zero", GT() {
		for ( const auto& threshold : std::array< std::pair< uint32_t, int32_t >, 4 >{ {
			{ 0x3b000000, 0 }, { 0x3bc00000, 1 }, { 0x3c200000, 2 }, { 0x42fffd00, 32766 }
		} } ) {
			for ( size_t index = 0; index < 9; ++index ) {
				for ( int offset = -1; offset <= 1; ++offset ) {
					for ( const bool negative : { false, true } ) {
						Input input{};
						input[ index ] = static_cast< uint32_t >( static_cast< int64_t >( threshold.first ) + offset ) | ( negative ? 0x80000000U : 0U );
						Basis expected{};
						const auto magnitude = threshold.second + ( offset >= 0 ? 1 : 0 );
						expected[ index / 3 ][ index % 3 ] = negative ? -magnitude : magnitude;
						GT_ASSERT( CvrBasisConverter::Convert( input ) == expected, "halfway neighbor differs from original rounding" );
					}
				}
			}
		}
		GT_OK();
	} );
	task->AddTest( "CVR basis handles signed zero subnormals and host rounding independently", GT() {
		const Input small{ 0, 0x80000000, 1, 0x80000001, 0x007fffff, 0x807fffff, 0x00800000, 0x80800000, 0x33800000 };
		GT_ASSERT( CvrBasisConverter::Convert( small ) == Basis{}, "small or signed-zero input acquired a nonzero result" );
		const Input halfway{ 0x3c200000, 0xbc200000, 0x3bc00000, 0xbbc00000, 0x3b000000, 0xbb000000, 0x3c200000, 0xbc200000, 0 };
		const Basis expected{ { { 3, -3, 2 }, { -2, 1, -1 }, { 3, -3, 0 } } };
		RoundingGuard restore;
		for ( const int mode : { FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO } ) {
			GT_ASSERT( std::fesetround( mode ) == 0, "test platform could not set rounding mode" );
			GT_ASSERT( CvrBasisConverter::Convert( halfway ) == expected, "host rounding mode changed conversion" );
			GT_ASSERT( CvrBasisConverter::Convert( small ) == Basis{}, "host rounding mode changed small values" );
			GT_ASSERT( std::fegetround() == mode, "converter changed caller rounding mode" );
		}
		GT_OK();
	} );
	task->AddTest( "CVR basis validates every destination width before returning", GT() {
		for ( size_t index = 0; index < 9; ++index ) {
			const bool narrow = index < 6;
			for ( const bool negative : { false, true } ) {
				Input input{};
				input[ index ] = narrow ? ( negative ? 0xc3000000U : 0x42fffeffU ) : ( negative ? 0xcb000000U : 0x4affffffU );
				Basis expected{};
				expected[ index / 3 ][ index % 3 ] = narrow ? ( negative ? -32768 : 32767 ) :
					( negative ? std::numeric_limits< int32_t >::min() : 2147483520 );
				GT_ASSERT( CvrBasisConverter::Convert( input ) == expected, "last supported destination value changed" );
				input[ index ] = narrow ? ( negative ? 0xc3000080U : 0x42ffff00U ) : ( negative ? 0xcb000001U : 0x4b000000U );
				const auto before = input;
				GT_ASSERT( Rejected( input ), "out-of-width value was silently narrowed" );
				GT_ASSERT( input == before, "rejection changed supplied bits" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "CVR basis rejects nonfinite and large finite bits in every position", GT() {
		for ( const uint32_t invalid : { 0x7f800000U, 0xff800000U, 0x7fc00001U, 0x7f800001U, 0xffc00001U, 0x7f7fffffU, 0xff7fffffU } ) {
			for ( size_t index = 0; index < 9; ++index ) {
				Input input{}; input[ index ] = invalid;
				GT_ASSERT( Rejected( input ), "nonfinite or oversized input accepted" );
			}
		}
		GT_OK();
	} );
}

}

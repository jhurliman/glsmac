#pragma once

#include "loader/cvr/CvrFrameAssembler.h"

#include <cfenv>
#include <cstring>
#include <stdexcept>
#include <string>

#if defined( __SSE__ ) || defined( _M_X64 ) || ( defined( _M_IX86_FP ) && _M_IX86_FP >= 1 )
#include <xmmintrin.h>
#define GLSMAC_TEST_CVR_SSE 1
#endif

namespace gse::tests::cvr_frame_arithmetic {

// Save both units explicitly: a platform's fenv interface need not report an
// independently changed SSE rounding mode. Restore on assertion/exception exits.
class Environment {
public:
	Environment() {
#ifdef GLSMAC_TEST_CVR_SSE
		m_mxcsr = _mm_getcsr();
#endif
		if ( std::feholdexcept( &m_saved ) != 0 ) { throw std::runtime_error( "cannot save floating-point environment" ); }
	}
	~Environment() {
		std::fesetenv( &m_saved );
#ifdef GLSMAC_TEST_CVR_SSE
		_mm_setcsr( m_mxcsr );
#endif
	}
	Environment( const Environment& ) = delete;
	Environment& operator=( const Environment& ) = delete;
private:
	std::fenv_t m_saved;
#ifdef GLSMAC_TEST_CVR_SSE
	unsigned m_mxcsr;
#endif
};

inline float Float( const uint32_t bits ) { float value; std::memcpy( &value, &bits, sizeof( value ) ); return value; }
inline uint32_t Bits( const float value ) { uint32_t bits; std::memcpy( &bits, &value, sizeof( bits ) ); return bits; }

inline std::string CheckModes() {
	using namespace loader::cvr;
	Environment restore;
	if ( std::fesetround( FE_TONEAREST ) != 0 ) { return "cannot select nearest rounding"; }
	CvrFrame frame{ 0, { Float( 0x3f800001 ), Float( 0x00800001 ), Float( 0x80000000 ) }, {} };
	frame.matrix = { Float( 1 ), Float( 0x80000001 ), Float( 0x80000000 ), 0, 0, 0, 0, 0, 0 };
	const auto source = CvrFrameSource::Create( { CvrScene{ "arithmetic", 1, { CvrSceneObject{ "", std::nullopt, { frame } } } } } );
	const std::vector< CvrFrameSelection > selection{ { 0, 0, { Float( 0x33800000 ), 0, Float( 0x80000000 ) } } };
	const auto nearest = [ & ]() -> std::string {
		const auto result = CvrFrameAssembler::Assemble( source, selection );
		const auto& actual = result.records.at( 0 ).value;
		if ( Bits( actual.location[ 0 ] ) != 0x3f800002 || Bits( actual.location[ 1 ] ) != 0x00800001 ||
			Bits( actual.location[ 2 ] ) != 0x80000000 ) { return "nearest-even/normal/signed-zero result changed"; }
		for ( size_t i = 0 ; i < frame.matrix.size() ; ++i ) {
			if ( Bits( actual.matrix[ i ] ) != Bits( frame.matrix[ i ] ) ) { return "opaque matrix bits changed with denormal controls"; }
		}
		for ( const auto offset : { Float( 1 ), Float( 0x80800000 ) } ) {
			auto invalid = selection;
			invalid[ 0 ].offset[ 1 ] = offset; // subnormal operand, or a subnormal cancellation result
			bool rejected = false;
			try { (void)CvrFrameAssembler::Assemble( source, invalid ); }
			catch ( const std::invalid_argument& error ) { rejected = std::string( error.what() ).find( "CVR assembly:" ) == 0; }
			if ( !rejected ) { return "denormal controls concealed an unsupported operand/result"; }
		}
		return {};
	};
	if ( const auto error = nearest(); !error.empty() ) { return error; }
#ifdef GLSMAC_TEST_CVR_SSE
	const auto baseline = _mm_getcsr();
	for ( const unsigned mode : { _MM_ROUND_DOWN, _MM_ROUND_UP, _MM_ROUND_TOWARD_ZERO } ) {
		// This instruction changes MXCSR only; x87 stays nearest. On glibc,
		// fegetround() still returns FE_TONEAREST, reproducing the review defect.
		_mm_setcsr( ( baseline & ~_MM_ROUND_MASK ) | mode );
		try {
			const auto result = CvrFrameAssembler::Assemble( source, selection );
			return "accepted independently changed SSE rounding; mode=" + std::to_string( mode ) +
				", fegetround=" + std::to_string( std::fegetround() ) + ", tie bits=" +
				std::to_string( Bits( result.records.at( 0 ).value.location[ 0 ] ) );
		}
		catch ( const std::invalid_argument& error ) {
			if ( std::string( error.what() ).find( "CVR assembly:" ) != 0 ) { return "unexpected SSE rounding rejection"; }
		}
	}
	// FTZ (bit 15) and DAZ (bit 6) cannot change the admitted normal/zero
	// float inputs, normal double intermediates or normal/zero float outputs.
	for ( const unsigned denormals : { 0u, 0x8000u, 0x40u, 0x8040u } ) {
		_mm_setcsr( ( baseline & ~( _MM_ROUND_MASK | 0x8040u ) ) | denormals );
		if ( const auto error = nearest(); !error.empty() ) { return error; }
	}
#endif
	return {};
}

inline std::string Check() {
	const auto rounding = std::fegetround();
#ifdef GLSMAC_TEST_CVR_SSE
	const auto mxcsr = _mm_getcsr();
#endif
	const auto error = CheckModes();
	if ( std::fegetround() != rounding ) { return "test did not restore standard rounding"; }
#ifdef GLSMAC_TEST_CVR_SSE
	if ( _mm_getcsr() != mxcsr ) { return "test did not restore exact MXCSR"; }
#endif
	return error;
}

}

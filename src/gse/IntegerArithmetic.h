#pragma once

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace gse::integer {

// Callers translate this arithmetic failure into a GSEMathError with their
// execution context. Every check precedes the potentially undefined operation.
class Error final : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

inline constexpr int64_t minimum = std::numeric_limits< int64_t >::min();
inline constexpr int64_t maximum = std::numeric_limits< int64_t >::max();

inline int64_t Add( const int64_t a, const int64_t b ) {
	if ( ( b > 0 && a > maximum - b ) || ( b < 0 && a < minimum - b ) ) {
		throw Error( "Integer overflow" );
	}
	return a + b;
}

inline int64_t Subtract( const int64_t a, const int64_t b ) {
	if ( ( b > 0 && a < minimum + b ) || ( b < 0 && a > maximum + b ) ) {
		throw Error( "Integer overflow" );
	}
	return a - b;
}

inline int64_t Multiply( const int64_t a, const int64_t b ) {
	// These divisions have nonzero divisors and never divide minimum by -1.
	if ( a > 0 ? ( b > 0 ? a > maximum / b : b < minimum / a )
		: a < 0 && ( b > 0 ? a < minimum / b : b < 0 && a < maximum / b ) ) {
		throw Error( "Integer overflow" );
	}
	return a * b;
}

inline void CheckDivision( const int64_t a, const int64_t b ) {
	if ( b == 0 ) { throw Error( "Division by zero" ); }
	// C++ division and remainder both require a representable quotient.
	if ( a == minimum && b == -1 ) { throw Error( "Integer overflow" ); }
}

inline int64_t Divide( const int64_t a, const int64_t b ) {
	CheckDivision( a, b );
	return a / b;
}

inline int64_t Modulo( const int64_t a, const int64_t b ) {
	CheckDivision( a, b );
	return a % b;
}

inline int64_t Abs( const int64_t a ) {
	if ( a == minimum ) { throw Error( "Integer overflow" ); }
	return a < 0 ? -a : a;
}

}

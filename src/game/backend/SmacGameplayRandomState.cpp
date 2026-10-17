#include "SmacGameplayRandomState.h"

#include <stdexcept>
#include <utility>

namespace game::backend {
namespace {

uint32_t ReadWord( const std::string& wire, const size_t offset ) {
	uint32_t result = 0;
	for ( unsigned i = 0; i < 4; ++i ) {
		result |= uint32_t( static_cast< unsigned char >( wire[ offset + i ] ) ) << ( i * 8 );
	}
	return result;
}

void WriteWord( std::string& wire, const size_t offset, const uint32_t value ) {
	for ( unsigned i = 0; i < 4; ++i ) {
		wire[ offset + i ] = static_cast< char >( static_cast< unsigned char >( value >> ( i * 8 ) ) );
	}
}

}

SmacGameplayRandomState::SmacGameplayRandomState( const uint32_t shared, const uint32_t crt ) noexcept
	: m_shared( shared ), m_crt( crt ) {}

uint32_t SmacGameplayRandomState::Shared() const noexcept { return m_shared; }
uint32_t SmacGameplayRandomState::Crt() const noexcept { return m_crt; }
uint32_t SmacGameplayRandomState::ExchangeShared( const uint32_t state ) noexcept { return std::exchange( m_shared, state ); }
void SmacGameplayRandomState::SetCrt( const uint32_t state ) noexcept { m_crt = state; }

uint16_t SmacGameplayRandomState::NextSharedLow16() noexcept {
	// Widen first, then the defined unsigned conversion performs modulo 2^32.
	m_shared = static_cast< uint32_t >( UINT64_C( 1664525 ) * m_shared + UINT64_C( 1013904223 ) );
	return static_cast< uint16_t >( m_shared & UINT32_C( 65535 ) );
}

uint16_t SmacGameplayRandomState::NextCrtRaw15() noexcept {
	m_crt = static_cast< uint32_t >( UINT64_C( 214013 ) * m_crt + UINT64_C( 2531011 ) );
	return static_cast< uint16_t >( ( m_crt >> 16 ) & UINT32_C( 32767 ) );
}

bool SmacGameplayRandomState::operator==( const SmacGameplayRandomState& other ) const noexcept {
	return m_shared == other.m_shared && m_crt == other.m_crt;
}

std::string SmacGameplayRandomState::Serialize() const {
	std::string wire( "SMGR" );
	wire.resize( WIRE_SIZE );
	WriteWord( wire, 4, 1 );
	WriteWord( wire, 8, m_shared );
	WriteWord( wire, 12, m_crt );
	return wire;
}

SmacGameplayRandomState SmacGameplayRandomState::Deserialize( const std::string& wire ) {
	// Fixed extent is checked before any field access or materialization.
	if ( wire.size() != WIRE_SIZE || wire.compare( 0, 4, "SMGR" ) != 0 || ReadWord( wire, 4 ) != 1 ) {
		throw std::invalid_argument( "Unsupported SMAC gameplay random-state record" );
	}
	return { ReadWord( wire, 8 ), ReadWord( wire, 12 ) };
}

}

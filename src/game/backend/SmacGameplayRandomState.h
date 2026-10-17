#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace game::backend {

// Two independent supplied SMAC gameplay states. Neither is the map's JSF32
// generator. Construction/restoration never chooses a seed or makes a draw.
class SmacGameplayRandomState {
public:
	static constexpr size_t WIRE_SIZE = 16;
	SmacGameplayRandomState( uint32_t shared, uint32_t crt ) noexcept;

	uint32_t Shared() const noexcept;
	uint32_t Crt() const noexcept;
	uint32_t ExchangeShared( uint32_t state ) noexcept;
	void SetCrt( uint32_t state ) noexcept;

	// Primitive outputs, not interval selection or caller draw suppression.
	uint16_t NextSharedLow16() noexcept;
	uint16_t NextCrtRaw15() noexcept;

	bool operator==( const SmacGameplayRandomState& other ) const noexcept;
	// Owned port record: SMGR, LE32 version1, shared word, CRT word. This is
	// not an original save-file format or a new-game/load lifecycle claim.
	std::string Serialize() const;
	static SmacGameplayRandomState Deserialize( const std::string& wire );

private:
	uint32_t m_shared;
	uint32_t m_crt;
};

}

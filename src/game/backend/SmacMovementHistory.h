#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace game::backend {
namespace unit { class Unit; class UnitManager; }

// Port world history, not visibility/exploration or an original save format.
// Absence of this value means unavailable. Only an explicitly generated
// canonical world uses the policy that initially clears its placement bits.
class SmacMovementHistory {
public:
	static constexpr size_t MAX_TILE_COUNT = 512 * 256 / 2;
	static constexpr size_t MAX_WIRE_BYTES = 28 + MAX_TILE_COUNT / 8;
	static SmacMovementHistory NewGeneratedWorld( int32_t width, int32_t height );
	void ValidateDimensions( size_t width, size_t height ) const;
	size_t PlacementIndex( int32_t x, int32_t y ) const;
	bool WasPlaced( int32_t x, int32_t y ) const;
	const std::optional< int32_t >& NearestDistance() const noexcept;
	void RetainNearest( std::optional< int32_t > distance );
	std::string Serialize() const;
	static SmacMovementHistory Deserialize( const std::string& wire );
	bool operator==( const SmacMovementHistory& other ) const noexcept;

	struct BasePosition { uint64_t id; int32_t x, y; };
	struct NearestResult { std::optional< uint64_t > base; std::optional< int32_t > distance; };
	// Bases are supplied in canonical ascending record/ID order. The caller
	// validates live membership. Diagnostic geometry is intentionally separate
	// from the smaller admitted generated-world bitmap dimensions.
	static NearestResult NearestAnyBase( int32_t x, int32_t y, int32_t width, bool wrap,
		const std::vector< BasePosition >& bases, std::optional< int32_t > prior );

private:
	friend class unit::Unit;
	friend class unit::UnitManager;
	// Index was checked before any native authority mutation. No allocation.
	void MarkPlaced( size_t index ) noexcept;
	SmacMovementHistory( int32_t width, int32_t height );
	int32_t m_width, m_height;
	std::vector< uint8_t > m_placed;
	std::optional< int32_t > m_nearest_distance;
};

}

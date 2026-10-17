#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace game::backend::map {

// Saved GLSMAC topology over actual displayed land/sea classification. The
// region traversal follows REGION-FRESH-001 below its unimplemented merging
// paths. This owns no Freshwater markers or original region-count table.
class GlsmacTerritoryTopology {
public:
	static constexpr size_t MAX_WIRE_BYTES = 16 + 512 * 256 / 2;
	static GlsmacTerritoryTopology Build( int32_t width, int32_t height, bool wrap, const std::vector< bool >& sea );
	int32_t Width() const { return m_width; }
	int32_t Height() const { return m_height; }
	bool Wraps() const { return m_wrap; }
	const std::vector< uint8_t >& Regions() const { return m_regions; }
	// Rebuild and compare against actual terrain; restore cannot substitute a
	// different topology with individually valid region bytes.
	void ValidateTerrain( const std::vector< bool >& sea ) const;
	bool operator==( const GlsmacTerritoryTopology& other ) const;
	std::string Serialize() const;
	static GlsmacTerritoryTopology Deserialize( const std::string& wire );

private:
	GlsmacTerritoryTopology( int32_t width, int32_t height, bool wrap, std::vector< uint8_t > regions );
	int32_t m_width, m_height;
	bool m_wrap;
	std::vector< uint8_t > m_regions;
};

}

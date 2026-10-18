#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <optional>
#include "SmacSiteRatingCache.h"

namespace game::backend::map {

// Saved GLSMAC topology over actual displayed land/sea classification. The
// region traversal follows REGION-FRESH-001 below its unimplemented merging
// paths. Stored area and postpass data retain their own producer lifetimes;
// this does not own Freshwater markers or an original world generator.
class GlsmacTerritoryTopology {
public:
	using RegionValues = std::array<int32_t,128>;
	struct TerrainPostpass {
		RegionValues fertility{};
		// Bit0: coastal complexity; bit1: nearby distinct land region.
		std::vector<uint8_t> coastal;
		bool operator==(const TerrainPostpass& other) const {return fertility==other.fertility && coastal==other.coastal;}
	};
	static constexpr size_t MAX_WIRE_BYTES = 16 + 3 * (512 * 256 / 2) + 2 * 128 * 4;
	static GlsmacTerritoryTopology Build( int32_t width, int32_t height, bool wrap, const std::vector< bool >& sea, RegionValues initial_areas = {} );
	GlsmacTerritoryTopology Rebuild(const std::vector<bool>& sea) const;
	GlsmacTerritoryTopology WithTerrainPostpass(const std::vector<bool>& sea,const std::vector<bool>& fertile) const;
	GlsmacTerritoryTopology WithSiteRatings(smac_site::RatingCache sites) const;
	int32_t Width() const { return m_width; }
	int32_t Height() const { return m_height; }
	bool Wraps() const { return m_wrap; }
	const std::vector< uint8_t >& Regions() const { return m_regions; }
	const std::optional<RegionValues>& Areas() const {return m_areas;}
	const std::optional<TerrainPostpass>& Postpass() const {return m_postpass;}
	const std::optional<smac_site::RatingCache>& SiteRatings() const {return m_sites;}
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
	// Absent for V1 port saves: final labels cannot reconstruct prior areas.
	std::optional<RegionValues> m_areas;
	std::optional<TerrainPostpass> m_postpass;
	// V1/V2 restores cannot reconstruct stale ratings or stray exclusions.
	std::optional<smac_site::RatingCache> m_sites;
	static GlsmacTerritoryTopology BuildWithAreas(int32_t width,int32_t height,bool wrap,const std::vector<bool>& sea,RegionValues areas);
};

}

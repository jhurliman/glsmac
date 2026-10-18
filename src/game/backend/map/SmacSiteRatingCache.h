#pragma once

#include "SmacSiteGeometry.h"
#include <functional>
#include <string>
#include <vector>

namespace game::backend::map::smac_site {

// Owned rating/exclusion history only. Occupying affiliation belongs to the
// world and is never changed by a rating write. Packed bytes are a port save
// format: low4 bits rating, bit4 Base-radius exclusion, high3 reserved.
class RatingCache {
public:
	using RatingQuery = std::function<uint8_t(Coordinate)>;
	static RatingCache Fresh(Geometry geometry);
	static RatingCache Restore(Geometry geometry,const std::string& packed);
	std::string Serialize() const;
	uint8_t Rating(Coordinate point) const;
	bool Excluded(Coordinate point) const;
	const Geometry& GetGeometry() const {return m_geometry;}
	RatingCache ClearRatings() const;
	RatingCache InvalidateLocal(Coordinate center) const;
	RatingCache FoundBase(Coordinate center) const;
	RatingCache RemoveBase(Coordinate center,const std::vector<Coordinate>& surviving_centers) const;
	RatingCache Refill(const RatingQuery& query) const;
	bool operator==(const RatingCache& other) const;
private:
	RatingCache(Geometry geometry,std::vector<uint8_t> values);
	size_t Index(Coordinate point) const;
	void BaseMark(Coordinate center);
	Geometry m_geometry;
	std::vector<uint8_t> m_values;
};
}

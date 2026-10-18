#include "Map.h"
#include "SmacSiteQuality.h"
#include "tile/Tile.h"
#include <stdexcept>

namespace game::backend::map {
GlsmacTerritoryTopology Map::PrepareSiteRatingRefill(const GlsmacTerritoryTopology& topology,
	const std::vector<smac_site::Coordinate>& bases,size_t turn) const {
	return PrepareSiteRatings(topology,bases,turn,{});
}
GlsmacTerritoryTopology Map::PrepareSiteRatingQuery(const GlsmacTerritoryTopology& topology,
	const std::vector<smac_site::Coordinate>& bases,size_t turn,smac_site::Coordinate point) const {
	if(!topology.SiteRatings()) {throw std::runtime_error("Direct site query lacks saved rating history");}
	return PrepareSiteRatings(topology,bases,turn,point);
}
GlsmacTerritoryTopology Map::PrepareSiteRatings(const GlsmacTerritoryTopology& topology,
	const std::vector<smac_site::Coordinate>& bases,size_t turn,std::optional<smac_site::Coordinate> point) const {
	// Legacy absence is history unavailable, not a request to reconstruct it.
	if(!topology.SiteRatings()) {return topology;}
	if(topology.Width()!=GetWidth() || topology.Height()!=GetHeight() || !topology.Wraps()) {
		throw std::runtime_error("Site-rating topology differs from current map geometry");
	}
	const auto width=topology.Width(),height=topology.Height();
	const auto index=[&](smac_site::Coordinate point) {return size_t(point[1])*(width/2)+point[0]/2;};
	std::vector<bool> markers(size_t(width)*height/2,false);
	for(const auto point:bases) {
		if(point[0]<0 || point[0]>=width || point[1]<0 || point[1]>=height || ((point[0]^point[1])&1)) {
			throw std::runtime_error("Site-rating future Base has invalid geometry");
		}
		markers[index(point)]=true;
	}
	std::vector<std::optional<smac_site::Tile>> resolved;
	const smac_site::Query query=[&](auto point) {
		if(resolved.empty()) {resolved.resize(markers.size());}
		const auto at=index(point);if(resolved[at]) {return *resolved[at];}
		const auto x=point[0],y=point[1];const auto* square=GetTile(x,y);
		if(!square->elevation.center || (!square->is_water_tile && (square->moisture<tile::MOISTURE_ARID || square->moisture>tile::MOISTURE_RAINY
			|| square->rockiness<tile::ROCKINESS_FLAT || square->rockiness>tile::ROCKINESS_ROCKY))) {
			throw std::runtime_error("Site-rating terrain lacks decoded moisture rockiness or elevation");
		}
		if(!topology.LandmarkDetails() && (square->features&(tile::FEATURE_VOLCANO|tile::FEATURE_GARLAND_CRATER))) {
			throw std::runtime_error("Site-rating terrain lacks shared Planet/Crater landmark detail");
		}
		const auto feature=[&](tile::feature_t flag) {return bool(square->features&flag);};
		// DisplayedTerrainV1 uses explicit visible bonuses/site markers and
		// active legacy Jungle/Uranium flags. When owned, consume the actual raw
		// detail byte, including suppression; never infer it from membership.
		resolved[at]=smac_site::Tile{static_cast<uint8_t>(m_resource_policy->Altitude(*square)),
			static_cast<uint8_t>(square->is_water_tile ? 0 : square->moisture-tile::MOISTURE_ARID),
			static_cast<uint8_t>(square->is_water_tile ? 0 : square->rockiness-tile::ROCKINESS_FLAT),topology.Regions()[at],
			feature(tile::FEATURE_XENOFUNGUS),feature(tile::FEATURE_MONOLITH),bool(square->terraforming&tile::TERRAFORMING_FOREST),
			feature(tile::FEATURE_RIVER),topology.SiteRatings()->Excluded(point),markers[at],
			m_resource_policy->Bonus(*square)!=resource::smac::ResourceBonus::None,feature(tile::FEATURE_UNITY_POD),bool(topology.Postpass()->coastal[at]),
			feature(tile::FEATURE_VOLCANO),feature(tile::FEATURE_GARLAND_CRATER),feature(tile::FEATURE_URANIUM),feature(tile::FEATURE_JUNGLE),
			topology.LandmarkDetails() ? topology.LandmarkDetails()->at(at) : uint8_t(0)};
		return *resolved[at];
	};
	const smac_site::RatingCache::RatingQuery score=[&](auto square) {
		if(!m_resource_policy || !topology.Areas() || !topology.Postpass() || turn>INT32_MAX) {
			throw std::runtime_error("Uncached site query requires owned terrain semantics and geography");
		}
		return static_cast<uint8_t>(smac_site::Quality({width,height,true},square,static_cast<int32_t>(turn),*topology.Areas(),query).rating);
	};
	auto sites=point ? topology.SiteRatings()->Resolve(*point,score) : topology.SiteRatings()->Refill(score);
	return topology.WithSiteRatings(std::move(sites));
}
}

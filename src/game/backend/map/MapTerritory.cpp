#include "Map.h"
#include "tile/Tiles.h"

namespace game::backend::map {
std::vector< bool > Map::TerritorySea() const {
	std::vector< bool > sea;
	sea.reserve( GetWidth() * GetHeight() / 2 );
	for ( size_t y = 0; y < GetHeight(); ++y ) {
		for ( size_t x = y & 1; x < GetWidth(); x += 2 ) { sea.push_back( GetTile( x, y )->is_water_tile ); }
	}
	return sea;
}

GlsmacTerritoryTopology Map::PrepareFreshTerritoryTopology() const {
	const auto sea=TerritorySea();
	auto topology=GlsmacTerritoryTopology::Build(GetWidth(),GetHeight(),true,sea);
	if(!m_resource_policy) {return topology;}
	std::vector<bool> fertile;fertile.reserve(sea.size());
	for(size_t y=0;y<GetHeight();++y) {for(size_t x=y&1;x<GetWidth();x+=2) {
		const auto* tile=GetTile(x,y);
		if(tile->is_water_tile) {fertile.push_back(false);continue;}
		if(tile->moisture<tile::MOISTURE_ARID || tile->moisture>tile::MOISTURE_RAINY
			|| tile->rockiness<tile::ROCKINESS_FLAT || tile->rockiness>tile::ROCKINESS_ROCKY) {
			return topology; // partial legacy/diagnostic terrain cannot establish F
		}
		// DisplayedTerrainV1 maps ARID/FLAT to original ordinal zero, just as
		// the live resource consumer does. Zero moisture does not qualify for F.
		fertile.push_back(tile->moisture>tile::MOISTURE_ARID && tile->rockiness<=tile::ROCKINESS_ROLLING
			&& !(tile->features&tile::FEATURE_XENOFUNGUS));
	}}
	return topology.WithTerrainPostpass(sea,fertile).WithSiteRatings(smac_site::RatingCache::Fresh({
		static_cast<int32_t>(GetWidth()),static_cast<int32_t>(GetHeight()),true}));
}
}

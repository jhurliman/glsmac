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
}

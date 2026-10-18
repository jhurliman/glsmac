#include "WaterSurfacePP.h"

#include "game/backend/map/Map.h"
#include "game/backend/map/Consts.h"
#include "game/backend/map/tile/Tile.h"
#include "game/backend/map/tile/TileState.h"

namespace game {
namespace backend {
namespace map {
namespace module {

void WaterSurfacePP::GenerateTile( const tile::Tile* tile, tile::TileState* ts, MapState* ms ) {

	if ( ts->has_water ) {
        m_map->EnsureWaterFungusRecipe();

		// it's here instead of WaterSurface because it needs to be drawn on top of coastline river fix redraw
		if ( m_map->GetRenderFeatures(tile) & tile::FEATURE_XENOFUNGUS ) {
			auto txinfo = m_map->GetTileTextureInfo( Map::TVT_TILES, tile, Map::TG_FEATURE, tile::FEATURE_XENOFUNGUS );
			m_map->AddWaterFungus(
				s_consts.tc.texture_pcx.fungus_water[ txinfo.texture_variant ],
				types::texture::AM_MERGE | txinfo.texture_flags | types::texture::AM_KEEP_TRANSPARENCY | types::texture::AM_INVERT,
				txinfo.rotate_direction
			);
		}
	}
}

}
}
}
}

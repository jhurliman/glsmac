#pragma once

#include "MapGenerator.h"
#include "game/backend/map/GlsmacResourceMapPolicy.h"

namespace game {
namespace backend {
namespace map {
namespace generator {

CLASS( SimplePerlin, MapGenerator )

	SimplePerlin( Game* game, util::random::Random* random, std::optional< GlsmacResourceMapPolicy > resource_policy = {} )
		: MapGenerator( game, random ), m_resource_policy( resource_policy ) {}

	void GenerateElevations( tile::Tiles* tiles, const settings::MapSettings* map_settings, MT_CANCELABLE ) override;
	void GenerateDetails( tile::Tiles* tiles, const settings::MapSettings* map_settings, MT_CANCELABLE ) override;

private:
	const std::optional< GlsmacResourceMapPolicy > m_resource_policy;
	void GenerateRiver( tile::Tiles* tiles, tile::Tile* tile, uint8_t length, uint8_t direction, int8_t direction_diagonal, MT_CANCELABLE );
	bool HasRiversNearby( tile::Tile* current_tile, tile::Tile* tile );

};

}
}
}
}

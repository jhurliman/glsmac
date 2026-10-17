#pragma once
#include <optional>
#include "map/GlsmacTerritoryTopology.h"
#include "types/Buffer.h"

namespace game::backend::world_snapshot {
void WriteTerritoryTopology( types::Buffer& output, const std::optional< map::GlsmacTerritoryTopology >& topology, bool smac );
std::optional< map::GlsmacTerritoryTopology > ReadTerritoryTopology( types::Buffer& input, bool smac );
void ValidateTerritoryGeometry( const std::optional< map::GlsmacTerritoryTopology >& topology, const std::string& map_payload, bool has_knowledge );
}

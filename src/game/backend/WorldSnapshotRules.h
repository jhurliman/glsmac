#pragma once

#include <cstdint>
#include <string>

namespace types { class Buffer; }
namespace game::backend::settings { class GlobalSettings; }

namespace game::backend::world_snapshot {

// Independent of the lobby settings and map/body formats. The typed prefix
// also rejects legacy snapshots that began directly with a map string.
inline constexpr int64_t rules_marker = 0x474c53574f524c44; // GLSWORLD
// V18 additionally carries the optional owned GLSMAC map-resource policy
// before the map body. Its absence remains explicitly unavailable.
// V1..V17 downloads are unsupported.
inline constexpr int64_t rules_version = 18;

void WriteHeader( types::Buffer& buffer, const settings::GlobalSettings& settings );
// Compare only: never changes settings, reads local files, or consumes the body.
void ValidateHeader( types::Buffer& buffer, const settings::GlobalSettings& settings );
// Check the existing Tiles dimension prefix before any map/tiles allocation.
// This does not validate the remaining tile payload.
void ValidateMapDimensions( const std::string& payload, const settings::GlobalSettings& settings );

}

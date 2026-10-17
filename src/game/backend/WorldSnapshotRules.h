#pragma once

#include <cstdint>
#include <string>

namespace types { class Buffer; }
namespace game::backend::settings { class GlobalSettings; }

namespace game::backend::world_snapshot {

// Independent of the lobby settings and map/body formats. The typed prefix
// also rejects legacy snapshots that began directly with a map string.
inline constexpr int64_t rules_marker = 0x474c53574f524c44; // GLSWORLD
// V7 also binds the frozen faction source identities and startup metadata.
// V1..V6 downloads are unsupported. JSF32, turn and body codecs are unchanged.
inline constexpr int64_t rules_version = 7;

void WriteHeader( types::Buffer& buffer, const settings::GlobalSettings& settings );
// Compare only: never changes settings, reads local files, or consumes the body.
void ValidateHeader( types::Buffer& buffer, const settings::GlobalSettings& settings );
// Check the existing Tiles dimension prefix before any map/tiles allocation.
// This does not validate the remaining tile payload.
void ValidateMapDimensions( const std::string& payload, const settings::GlobalSettings& settings );

}

#pragma once

#include <cstdint>

namespace types { class Buffer; }
namespace game::backend::settings { class GlobalSettings; }

namespace game::backend::world_snapshot {

// Independent of the lobby settings and map/body formats. The typed prefix
// also rejects legacy snapshots that began directly with a map string.
inline constexpr int64_t rules_marker = 0x474c53574f524c44; // GLSWORLD
// V5 binds the frozen technology catalog after the yield/production profiles,
// before unchanged JSF32 and turn blocks. V1..V4 downloads are unsupported.
inline constexpr int64_t rules_version = 5;

void WriteHeader( types::Buffer& buffer, const settings::GlobalSettings& settings );
// Compare only: never changes settings, reads local files, or consumes the body.
void ValidateHeader( types::Buffer& buffer, const settings::GlobalSettings& settings );

}

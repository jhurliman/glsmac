#pragma once

#include <cstdint>
#include <string>

namespace types { class Buffer; }
namespace game::backend::settings { class GlobalSettings; }

namespace game::backend::world_snapshot {

// Independent of the lobby settings and map/body formats. The typed prefix
// also rejects legacy snapshots that began directly with a map string.
inline constexpr int64_t rules_marker = 0x474c53574f524c44; // GLSWORLD
// V32 binds frozen combat percentages and Base defender-loss aftermath.
// V31 retains Base upkeep caches and an explicit Colony question continuation.
// V30 binds the frozen TERRAIN action catalog. V29 reserves research state.
// V28 requires saved faction economy state (Simulation9). V27 preserves Unit
// record order (Unit manager V4) for support and casualties.
// V26 adds separately supplied directed Pact state to worker knowledge.
// V25 binds frozen territory border rules, saved region topology and Base
// record order (BM9). V24 adds remembered-improvement copies to knowledge.
// V23 binds frozen faction header worker priorities. V22 added conventional
// home Base links (Unit manager V3) and saved Base riots (Base manager V8).
// Satellite assets and movement caller history remain unchanged.
// The World18 optional map-resource policy remains before the map body.
// V1..V31 downloads are unsupported.
inline constexpr int64_t rules_version = 32;

void WriteHeader( types::Buffer& buffer, const settings::GlobalSettings& settings );
// Compare only: never changes settings, reads local files, or consumes the body.
void ValidateHeader( types::Buffer& buffer, const settings::GlobalSettings& settings );
// Check the existing Tiles dimension prefix before any map/tiles allocation.
// This does not validate the remaining tile payload.
void ValidateMapDimensions( const std::string& payload, const settings::GlobalSettings& settings );

}

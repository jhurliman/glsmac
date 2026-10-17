#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "types/Buffer.h"
#include "game/backend/resource/SmacResourceBonus.h"

namespace game::backend::map {
namespace tile { class Tile; }

// GLSMAC map semantics, not an original generator or height reconstruction.
// The raw Tile fields remain authoritative; this saved tag specifies how the
// admitted resource consumer interprets them. Absence is explicitly unavailable.
class GlsmacResourceMapPolicy {
public:
	static constexpr uint32_t MAX_WIRE_BYTES = 28; // two framed int64 values
	static GlsmacResourceMapPolicy DisplayedTerrainV1();
	bool operator==( const GlsmacResourceMapPolicy& ) const { return true; }

	// Water classification comes from Tile::Update's five-sample majority.
	// Depth labels use the same strict integer boundaries as tile_preview.
	resource::smac::AltitudeBand Altitude( bool water, int64_t center_elevation ) const;
	bool AllowsGeneratedBonus( bool water, int64_t center_elevation ) const;
	resource::smac::ResourceBonus Bonus( const tile::Tile& tile ) const;

	types::Buffer Serialize() const;
	static GlsmacResourceMapPolicy Deserialize( const std::string& wire );
	static void WriteOptional( types::Buffer& output, const std::optional< GlsmacResourceMapPolicy >& policy, bool smac );
	static std::optional< GlsmacResourceMapPolicy > ReadOptional( types::Buffer& input, bool smac );

private:
	GlsmacResourceMapPolicy() = default;
};

}

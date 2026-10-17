#pragma once

#include "Render.h"
#include "loader/cvr/CvrScoutRenderer.h"
#include "types/Vec2.h"

namespace game::backend::unit {

// A logical reactor-1 flat-idle Scout composition, never serialized file paths.
// Placement is explicit port world units; it does not assert original screen scale.
class CvrScoutRender final : public Render {
public:
	static constexpr size_t MAX_WIRE_BYTES = 256;
	CvrScoutRender( int32_t facing, int32_t zoom, int32_t numerator, int32_t denominator,
		const types::Vec2< float >& size, const types::Vec2< float >& center );
	const loader::cvr::CvrScoutViewInput view;
	const types::Vec2< float > size;
	const types::Vec2< float > center;
	types::Vec2< float > Offset() const;
	std::string PixelKey() const;
	std::string PlacementKey() const;
	std::string Encode() const;
	static CvrScoutRender Decode( const std::string& bytes );
	const std::string ToString( const std::string& prefix ) const override;
};

}

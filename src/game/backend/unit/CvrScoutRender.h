#pragma once

#include "Render.h"
#include "loader/cvr/CvrScoutRenderer.h"
#include "types/Vec2.h"

namespace game::backend::unit {

// Fixed reactor-1 flat-idle compositions, never serialized file paths.
// The Scout name is retained for its existing source API and wire profile.
// Placement is explicit port world units; it does not assert original screen scale.
class CvrScoutRender final : public Render {
public:
	enum class Composition : int32_t { Scout = 0, SpartanRover = 1 };
	static constexpr size_t MAX_WIRE_BYTES = 256;
	CvrScoutRender( int32_t facing, int32_t zoom, int32_t numerator, int32_t denominator,
		const types::Vec2< float >& size, const types::Vec2< float >& center, Composition composition = Composition::Scout );
	const Composition composition;
	const loader::cvr::CvrScoutViewInput view;
	const types::Vec2< float > size;
	const types::Vec2< float > center;
	types::Vec2< float > Offset() const;
	std::string PixelKey() const;
	std::string PlacementKey() const;
	std::string Encode() const;
	static CvrScoutRender Decode( const std::string& bytes, render_type_t expected_type = RT_CVR_SCOUT );
	const std::string ToString( const std::string& prefix ) const override;
};

}

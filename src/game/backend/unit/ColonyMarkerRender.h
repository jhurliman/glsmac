#pragma once

#include "Render.h"

namespace game::backend::unit {

// Explicit GLSMAC marker, never an original Colony model or Combat render.
class ColonyMarkerRender final : public Render {
public:
	ColonyMarkerRender() : Render( RT_PORT_COLONY_MARKER ) {}
	const std::string ToString( const std::string& prefix ) const override { return prefix + "GLSMAC Colony marker"; }
};

}

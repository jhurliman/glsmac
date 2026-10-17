#pragma once

#include "Render.h"

namespace game::backend::unit {

// Explicit GLSMAC placeholder, not an original-game model or Scout variant.
// It has no asset path, camera, facing or inferred composition.
class FormerMarkerRender final : public Render {
public:
	FormerMarkerRender() : Render( RT_PORT_FORMER_MARKER ) {}
	const std::string ToString( const std::string& prefix ) const override { return prefix + "GLSMAC Former marker"; }
};

}

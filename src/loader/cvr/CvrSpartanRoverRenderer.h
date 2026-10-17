#pragma once

#include "CvrScoutRenderer.h"
#include "CvrActorMaterial.h"

#include <functional>
#include <string>

namespace loader::cvr {

// Fixed reactor1/Gun/Scout-armor/no-ability Speeder, idle animation0. This
// identifies an asset composition, not a live Unit or arbitrary serialized paths.
class CvrSpartanRoverRenderer {
public:
	using AssetReader = std::function< std::string( const std::string& logical_name ) >;
	// Resolve the named installed assets using the application's existing resolver.
	// All source bytes and decoded outputs become owned before this call returns.
	static CvrSpartanRoverRenderer Load( const AssetReader& read );
	CvrScoutImage Render( const CvrScoutViewInput& view ) const;
	const CvrActorMaterial& Material() const;
	static const std::array< const char*, 10 >& PartNames();

private:
	CvrSpartanRoverRenderer( CvrScoutRenderer renderer, CvrActorMaterial material );
	CvrScoutRenderer m_renderer;
	CvrActorMaterial m_material;
};

}

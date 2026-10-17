#pragma once

#include <string>

#include "types/Buffer.h"

namespace game {
namespace backend {

namespace map {
namespace tile {
class Tile;
class TileState;
}
}

namespace unit {

class Render {
public:

	enum render_type_t {
		RT_SPRITE,
		RT_CVR_SCOUT,
		RT_CVR_SPARTAN_ROVER,
		RT_CVR_INITIALIZED_SCOUT,
		RT_PORT_FORMER_MARKER,
	};

	Render( const render_type_t type );
	virtual ~Render() = default;

	virtual const std::string ToString( const std::string& prefix ) const = 0;

	const render_type_t m_type;

private:
	friend class StaticDef;

	static void Serialize( types::Buffer& buf, const Render* render );
	static Render* Deserialize( types::Buffer& buf );

};

}
}
}

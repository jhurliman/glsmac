#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "Types.h"
#include "types/Vec3.h"
#include "types/texture/Types.h"
#include "util/random/Types.h"

namespace types::texture { class Texture; }
namespace util { class Perlin; }

namespace game::backend::map {

// Owned CPU data only. Applied to the frontend-owned atlas/actors on its thread.
struct TerrainRenderUpdate {
	struct Patch { size_t x, y; std::shared_ptr< const types::texture::Texture > pixels; };
	struct Instance { size_t id; std::string actor; types::Vec3 coords; };
	std::vector< Patch > patches;
	std::map< std::string, sprite_actor_t > actors;
	std::vector< std::pair< size_t, std::string > > removed;
	std::vector< Instance > added;
	void ApplyTexture( types::texture::Texture& atlas ) const;
};

// Presentation-only recipes retained while building an atlas. A land reference
// identifies the small source canvas; all other inputs own their pixel bytes.
// Perlin instances belong to this Map's modules and outlive recipe evaluation.
class TerrainCoastRecipes {
public:
	using Key = std::pair< size_t, size_t >;
	using Pixels = std::shared_ptr< const types::texture::Texture >;
	using Land = std::map< Key, Pixels >;
	struct Paint {
		Pixels pixels;
		std::optional< Key > land;
		types::texture::add_flag_t flags;
		uint8_t rotate;
		float alpha;
		util::random::state_t random;
		util::Perlin* perlin;
	};
	void Record( Key destination, Paint paint );
	std::vector< TerrainRenderUpdate::Patch > Prepare( const Land& replacements ) const;
	void Commit( const Land& replacements ) noexcept;
	void Clear();
private:
	std::map< Key, std::vector< Paint > > m_recipes;
	Land m_land;
};

}

#pragma once

#include <memory>

#include "Actor.h"

#include "Types.h"
#include "types/mesh/Types.h"

namespace types {
namespace texture {
class Texture;
}
namespace mesh {
class Render;
}
}

namespace scene {
namespace actor {

CLASS( Sprite, Actor )

	Sprite(
		const std::string& name,
		const scene::actor::sprite_coords_t dimensions,
		types::texture::Texture* texture,
		const types::mesh::tex_coords_t tex_coords,
		const types::Vec2< types::mesh::tex_coord_t > dst_offsets
	);
	Sprite( Sprite* orig );

	const scene::actor::sprite_coords_t& GetDimensions() const;
	types::texture::Texture* GetTexture() const;
	const types::mesh::tex_coords_t& GetTexCoords() const;
	const types::Vec2< types::mesh::tex_coord_t >& GetDstOffsets() const;
	struct MeshDeleter {
		void operator()( const types::mesh::Render* mesh ) const;
	};
	using mesh_ptr_t = std::unique_ptr< const types::mesh::Render, MeshDeleter >;
	// Fresh geometry belongs to the caller; sprite textures remain borrowed.
	mesh_ptr_t GenerateMesh() const;

	const types::Vec3 NormalizePosition( const types::Vec3& position ) const override;

protected:
	const types::Vec2< float > m_dimensions;
	types::texture::Texture* m_texture = nullptr;
	const types::mesh::tex_coords_t m_tex_coords;
	const types::Vec2< types::mesh::tex_coord_t > m_dst_offsets;

};

}
}

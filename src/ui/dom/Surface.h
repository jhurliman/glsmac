#pragma once

#include "Area.h"
#include <memory>
#include <cstdint>
#include <vector>

namespace task::intro { class Movie; }
namespace loader::texture {struct SmacTurnFrames;struct SmacGraphPalette;}

namespace types {
namespace mesh {
class Rectangle;
}
namespace texture {
class Texture;
}
}

namespace scene::actor {
class Mesh;
}

namespace ui {

class Geometry;
class UI;

namespace dom {

class Surface : public Area {
public:

	Surface( DOM_ARGS_T = "surface" );
	void Destroy( GSE_CALLABLE ) override;
	// Main-thread RGBA upload; updates both the texture and cached actor revision.
	void PresentMovieFrame(int width, int height, const std::vector<uint8_t>& rgba);
    void PresentTurnFrame(const std::vector<uint8_t>& rgba);

protected:
	virtual ~Surface();

	struct {
		types::texture::Texture* texture = nullptr;
		bool is_texture_owned = false;
	} m_background = {};

	void CreateTexture();

private:
	types::mesh::Rectangle* m_mesh = nullptr;
	scene::actor::Mesh* m_actor = nullptr;

	void ClearTexture();
	std::unique_ptr<task::intro::Movie> m_movie;
    std::shared_ptr<const loader::texture::SmacTurnFrames> m_turn_frames;
    std::shared_ptr<const loader::texture::SmacGraphPalette> m_graph_palette;
	void StopMovie();

private:
	friend class Panel;
	types::texture::Texture* GetOwnedTexturePtr();

};

}
}

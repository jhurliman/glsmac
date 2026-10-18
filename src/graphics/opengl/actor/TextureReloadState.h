#pragma once

#include <cstddef>

namespace graphics::opengl {

// Pointer and per-texture update counts can both repeat after an owned UI
// texture is replaced several times between draws. The actor revision retains
// those transitions even when the allocator reuses the first texture address.
class TextureReloadState {
public:
	bool Update( const void* texture, size_t updates, size_t revision ) {
		if ( texture == m_texture && updates == m_updates && revision == m_revision ) { return false; }
		m_texture = texture;
		m_updates = updates;
		m_revision = revision;
		return true;
	}
private:
	const void* m_texture = nullptr;
	size_t m_updates = 0;
	size_t m_revision = 0;
};

}

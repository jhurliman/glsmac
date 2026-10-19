#pragma once

#include "common/Common.h"

#include "game/frontend/Types.h"

#include "types/Color.h"
#include <map>

namespace types {
class Font;
}

namespace scene {
class Scene;
}

namespace game {
namespace frontend {

namespace sprite {
class InstancedSpriteManager;
}

namespace text {

class InstancedFont;
class InstancedText;

CLASS( InstancedTextManager, common::Class )
public:

	InstancedTextManager( sprite::InstancedSpriteManager* ism );
	~InstancedTextManager();

	InstancedFont* GetInstancedFont( const types::Font* font, bool pixel_base_names = false );

	InstancedText* CreateInstancedText(
		const std::string& text,
		InstancedFont* font,
		const types::Color& color,
		const types::Color& shadow_color
	);

private:

	sprite::InstancedSpriteManager* m_ism = nullptr;

	std::map< std::pair<const types::Font*,bool>, InstancedFont* > m_instanced_fonts = {};
	std::unordered_map< const types::Font*, InstancedText* > m_instanced_texts = {};

};

}
}
}

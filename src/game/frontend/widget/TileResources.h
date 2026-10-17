#pragma once

#include "Widget.h"
#include "game/backend/map/tile/Tile.h"

namespace game {
namespace frontend {
namespace widget {

CLASS( TileResources, Widget )
public:
	TileResources( Game* const game, ui::UI* const ui );

	void Update( ui::dom::Widget* const widget, const void* const data ) override;

	// The same checked query is used for widget redraws and resource previews.
	// A supplied base selects its owner; absent base preserves the active player.
	static backend::map::tile::Tile::resources_t QueryResources( GSE_CALLABLE, backend::Game* const game, gse::value::Object* const data );

protected:
	void Register( ui::dom::Widget* const widget ) override;

private:
	void DrawResources( ui::dom::Widget* const widget, const backend::map::tile::Tile::resources_t& resources );

};

}
}
}

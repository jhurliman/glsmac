#include "TilePreview.h"

#include "ui/UI.h"
#include "ui/dom/Widget.h"
#include "game/frontend/tile/Tile.h"
#include "types/mesh/Render.h"
#include "ui/geometry/Rectangle.h"
#include "scene/actor/Mesh.h"
#include "game/frontend/Game.h"
#include "game/backend/map/tile/Tile.h"
#include "game/frontend/tile/TileManager.h"
#include "game/frontend/sprite/InstancedSpriteManager.h"
#include "scene/actor/Instanced.h"
#include "scene/actor/Sprite.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/Player.h"
#include "types/texture/Texture.h"

namespace game {
namespace frontend {
namespace widget {

TilePreview::TilePreview( Game* const game, ui::UI* const ui )
	: Widget(
	game, ui, ui::WT_TILE_PREVIEW, "tile-preview", {
		{ "tile", { true, gse::VT_OBJECT, ::game::backend::map::tile::Tile::WRAP_CLASS } },
	}
) {}

void TilePreview::Register( ui::dom::Widget* const widget ) {
	const std::weak_ptr<const bool> lifetime=m_lifetime;
	widget->OnUpdate(
		F_WIDGET_UPDATE( this, lifetime ) {
			if(lifetime.expired()) {return;}
			if ( data ) {
				auto* const t = ::game::backend::map::tile::Tile::Unwrap( data->value.at( "tile" ) );
				ASSERT( t, "invalid tile ptr" );
				const auto* const tile = m_game->GetTM()->GetTile( t->coord.x, t->coord.y );
				ASSERT( tile, "invalid tile" );
				Update( widget, tile );
			}
		}
	);
}

void TilePreview::Unregister(ui::dom::Widget* const widget) {
    m_coordinates.erase(widget);Widget::Unregister(widget);
}

void TilePreview::Refresh() {
    WithWidget(F_WITH_WIDGET(this) {
        const auto found=m_coordinates.find(widget);
        if(found!=m_coordinates.end()) {Update(widget,m_game->GetTM()->GetTile(found->second.x,found->second.y));}
    });
}

void TilePreview::Update( ui::dom::Widget* const widget, const void* const data ) {
	widget->Clear();

	const auto* tile = (const tile::Tile*)data;
    if(!tile) {m_coordinates.erase(widget);return;}
    m_coordinates[widget]=tile->GetCoords();
    // Unknown native selections must remain empty, including during the brief
    // interval before the script receives the refreshed knowledge event.
    if(!m_game->AllowsNativeTileInspection(tile->GetCoords().x,tile->GetCoords().y)) {return;}
    if(const auto texture=m_game->GetNativeTilePreview(tile->GetCoords().x,tile->GetCoords().y)) {
        widget->RetainTexture(texture);AddMeshAndTexture(widget,0,nullptr,texture.get());return;
    }

	const auto& render = tile->GetRenderData();

	size_t index = 0;
    // The native work surface occupies the full89x117 clip. Keep the legacy
    // fallback in its former80x40 portrait rectangle, independent of that clip.
    const auto& area=widget->GetGeometry()->AsRectangle()->m_area;
    if(area.width<=0 || area.height<=0) {return;}
    const types::Vec2<float> scale{80/area.width,40/area.height},offset{2/area.width,1/area.height};

	// add tile texture
	auto* const texture = m_game->GetTerrainTexture();
	types::Color tint = { 1.5f, 1.5f, 1.5f, 1.0f }; // to compensate for lack of lighting
	for ( auto& preview_mesh : render.preview_meshes ) {
		auto* const actor = AddMeshAndTexture( widget, index++, preview_mesh, texture, false, scale, offset );
		actor->SetTintColor( tint );
	}

	// add tile sprites
	for ( auto& s : render.sprites ) {
		auto* actor = m_game->GetISM()->GetInstancedSpriteByKey( s )->actor;
		auto sprite = actor->GetSpriteActor();
		auto mesh = sprite->GenerateMesh();
		AddMeshAndTexture( widget, index++, mesh.get(), sprite->GetTexture(), false, scale, offset );
	}

}

}
}
}

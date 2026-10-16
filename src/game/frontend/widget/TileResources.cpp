#include "TileResources.h"

#include <vector>
#include <algorithm>

#include "ui/UI.h"
#include "ui/dom/Widget.h"
#include "ui/geometry/Geometry.h"
#include "types/texture/Texture.h"
#include "game/frontend/text/InstancedTextManager.h"
#include "engine/Engine.h"
#include "loader/font/FontLoader.h"
#include "game/frontend/text/InstancedFont.h"
#include "game/frontend/resource/ResourceDisplay.h"
#include "scene/actor/Mesh.h"
#include "game/frontend/Game.h"
#include "game/backend/map/tile/Tile.h"
#include "game/frontend/sprite/InstancedSprite.h"
#include "scene/actor/Instanced.h"
#include "scene/actor/Sprite.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/Player.h"
#include "game/backend/base/Base.h"
#include "game/backend/map/tile/Tiles.h"
#include "game/backend/map/Map.h"
#include "game/backend/resource/ResourceManager.h"
#include "game/frontend/resource/ResourceManager.h"

namespace game {
namespace frontend {
namespace widget {

TileResources::TileResources( Game* const game, ui::UI* const ui )
	: Widget(
	game, ui, ui::WT_TILE_RESOURCES, "tile-resources", {
		{ "tile", { true, gse::VT_OBJECT, ::game::backend::map::tile::Tile::WRAP_CLASS } },
		{ "base", { false, gse::VT_OBJECT, backend::base::Base::WRAP_CLASS } },
	}
) {}

backend::map::tile::Tile::resources_t TileResources::QueryResources( GSE_CALLABLE, backend::Game* const game, gse::value::Object* const data ) {
	// Widget data can outlive a wrapped base. Validate the current value on
	// every query, before Unwrap (which assumes a live, correctly typed object).
	if ( !data ) { GSE_ERROR( gse::EC.INVALID_CALL, "Tile resource widget requires data" ); }
	const auto unwrap = [ & ]( const std::string& key, const std::string& cls, const bool required ) -> gse::value::Object* {
		const auto it = data->value.find( key );
		if ( it == data->value.end() && !required ) { return nullptr; }
		if ( it == data->value.end() || it->second->Deref()->type != gse::VT_OBJECT ) {
			GSE_ERROR( gse::EC.INVALID_CALL, "Tile resource widget requires a live " + key );
		}
		auto* object = static_cast< gse::value::Object* >( it->second->Deref() );
		if ( object->object_class != cls || !object->wrapobj ) {
			GSE_ERROR( gse::EC.INVALID_CALL, "Tile resource widget requires a live " + key );
		}
		return object;
	};
	auto* const tile = backend::map::tile::Tile::Unwrap( unwrap( "tile", backend::map::tile::Tile::WRAP_CLASS, true ) );
	auto* const base_value = unwrap( "base", backend::base::Base::WRAP_CLASS, false );
	if ( tile->tiles->GetMap() != game->GetMap() ) {
		GSE_ERROR( gse::EC.GAME_ERROR, "Tile resource widget tile belongs to another world" );
	}
	return tile->GetResources( GSE_CALL, base_value ? nullptr : game->GetPlayer()->GetSlot(), base_value ? backend::base::Base::Unwrap( base_value ) : nullptr );
}

void TileResources::Register( ui::dom::Widget* const widget ) {
	widget->OnUpdate(
		F_WIDGET_UPDATE( this ) {
			if ( data ) {
				auto* const backend = m_game->GetBackend();
				backend::map::tile::Tile::resources_t resources;
				backend->GetState()->WithGSE( backend, [ & ]( GSE_CALLABLE ) {
					resources = QueryResources( GSE_CALL, backend, data );
				}, nullptr, true );
				DrawResources( widget, resources );
			}
		}
	);
}

void TileResources::Update( ui::dom::Widget* const widget, const void* const data ) {
	auto* const tile = (backend::map::tile::Tile*)data;
	auto* const backend = m_game->GetBackend();
	backend::map::tile::Tile::resources_t resources;
	backend->GetState()->WithGSE( backend, [ & ]( GSE_CALLABLE ) {
		resources = tile->GetResources( GSE_CALL, backend->GetPlayer()->GetSlot() );
	}, nullptr, true );
	DrawResources( widget, resources );
}

void TileResources::DrawResources( ui::dom::Widget* const widget, const backend::map::tile::Tile::resources_t& resources ) {
	widget->Clear();
	auto* const backend = m_game->GetBackend();

	size_t count = 0;
	auto* const rm = m_game->GetRM();
	struct displayed_resource_t {
		std::string id;
		resource::ResourceDisplay display;
	};
	std::vector< displayed_resource_t > displayed;
	bool needs_numbers = false;
	for ( const auto& id : backend->GetRM()->GetDefinedResourcesOrder() ) {
		const auto it = resources.find( id );
		if ( it == resources.end() || it->second == 0 ) { continue; }
		auto display = resource::ResourceDisplay::For( it->second, rm->GetResourceSpriteCount( id ) );
		needs_numbers |= !display.label.empty();
		displayed.push_back( { id, display } );
	}
	if ( needs_numbers ) {
		// Keep exceptional values readable without allocating one icon per unit.
		// Ordinary values still use their exact stacked icon in this mixed view.
		const auto& area = widget->GetGeometry()->m_area;
		const auto aspect = std::max( 1.0f, area.width ) / std::max( 1.0f, area.height );
		const auto row_height = 0.9f / displayed.size();
		auto* font = m_game->GetITM()->GetInstancedFont( g_engine->GetFontLoader()->GetBuiltinFont( 16 ) );
		size_t index = 0;
		for ( size_t row = 0; row < displayed.size(); row++ ) {
			const auto& entry = displayed.at( row );
			const auto center_y = 0.05f + row_height * ( row + 0.5f );
			if ( entry.display.sprite_value ) {
				const auto* icon = rm->GetResourceSprite( entry.id, entry.display.sprite_value );
				auto* sprite = icon->actor->GetSpriteActor();
				auto h = row_height * 0.8f;
				auto w = h * icon->wh.x / icon->wh.y / aspect;
				if ( w > 0.22f ) { h *= 0.22f / w; w = 0.22f; }
				AddMeshAndTexture( widget, index++, sprite->GenerateMesh(), sprite->GetTexture(), false,
					{ w, h }, { 0.03f + ( 0.22f - w ) / 2, center_y - h / 2 } );
			}
			if ( !entry.display.label.empty() ) {
				const auto text = entry.display.sprite_value ? entry.display.label : entry.id + " " + entry.display.label;
				// GetTextTexture returns a fresh owned texture, not a cached font atlas.
				auto* texture = font->GetTextTexture(
					text, { 0, 0, 0, 0 }, { 1, 1, 1, 1 }, { 0, 0, 0, 1 }, 1
				);
				auto h = row_height * 0.8f;
				auto w = h * texture->GetWidth() / texture->GetHeight() / aspect;
				const auto left = entry.display.sprite_value ? 0.3f : 0.03f;
				const auto max_width = 0.97f - left;
				if ( w > max_width ) { h *= max_width / w; w = max_width; }
				widget->SetTexture( texture, index, false );
				AddMeshAndTexture( widget, index++, nullptr, texture, false, { w, h }, { left, center_y - h / 2 } );
			}
		}
		return;
	}
	for ( const auto& it : resources ) {
		if ( it.second != 0 ) {
			count++;
		}
	}

	const auto x_scale = 0.4f;
	const auto y_scale = 0.8f;

	types::Vec2< size_t > max_wh = { 0, 0 };

	for ( const auto& id : backend->GetRM()->GetDefinedResourcesOrder() ) {
		const auto& r_max_wh = rm->GetResourceMaxWH( id );
		if ( r_max_wh.x > max_wh.x ) {
			max_wh.x = r_max_wh.x;
		}
		if ( r_max_wh.y > max_wh.y ) {
			max_wh.y = r_max_wh.y;
		}
	}

	if ( count > 0 ) {
		std::vector< const types::mesh::Render* > meshes = {};
		std::vector< types::texture::Texture* > textures = {};
		std::vector< backend::map::pcx_texture_coordinates_t > whs = {};
		size_t index = 0;
		size_t total_width = 0;
		size_t max_height = 0;
		size_t total_possible_width = 0;
		for ( const auto& id : backend->GetRM()->GetDefinedResourcesOrder() ) {
			total_possible_width += max_wh.x;
			const auto& resource_it = resources.find( id );
			if ( resource_it == resources.end() ) {
				continue;
			}
			const auto& value = resource_it->second;
			if ( value == 0 ) {
				continue;
			}
			auto* instanced_sprite = rm->GetResourceSprite( id, value );
			ASSERT( instanced_sprite, "resource sprite is null" );
			auto* actor = instanced_sprite->actor;
			auto sprite = actor->GetSpriteActor();
			const auto& wh = instanced_sprite->wh;
			if ( wh.y > max_height ) {
				max_height = wh.y;
			}
			total_width += wh.x;
			meshes.push_back( sprite->GenerateMesh() );
			textures.push_back( sprite->GetTexture() );
			whs.push_back( wh );
		}
		ASSERT( meshes.size() == count, "meshes size mismatch" );
		ASSERT( textures.size() == count, "textures size mismatch" );
		ASSERT( whs.size() == count, "whs size mismatch" );
		ASSERT( max_wh.x > 0, "max_wh x is zero" );
		ASSERT( max_wh.y > 0, "max_wh y is zero" );
		auto offset = 0.5f - (float)total_width / total_possible_width / 2;
		for ( size_t i = 0 ; i < count ; i++ ) {
			const auto& wh = whs.at( i );
			const auto w = (float)wh.x / max_wh.x * x_scale;
			const auto h = (float)wh.y / max_wh.y * y_scale;
			AddMeshAndTexture(
				widget,
				index++,
				meshes.at( i ),
				textures.at( i ),
				false,
				{
					w,
					h,
				},
				{
					offset,
					0.5f - h / 2,
				}
			);
			offset += w;
		}
	}
	else {
		auto* instanced_sprite = rm->GetNoResourceSprite();
		ASSERT( instanced_sprite, "no-resource sprite is null" );
		auto* actor = instanced_sprite->actor;
		auto sprite = actor->GetSpriteActor();
		const auto& wh = instanced_sprite->wh;
		const auto w = (float)wh.x / max_wh.x * x_scale;
		const auto h = (float)wh.y / max_wh.y * y_scale;
		AddMeshAndTexture(
			widget,
			0,
			sprite->GenerateMesh(),
			sprite->GetTexture(),
			false,
			{
				w,
				h,
			},
			{
				0.5f - w / 2,
				0.5f - h / 2,
			}
		);
	}

}

}
}
}

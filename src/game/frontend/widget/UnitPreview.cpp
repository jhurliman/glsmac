#include "UnitPreview.h"

#include "ui/UI.h"
#include "ui/dom/Widget.h"
#include "game/frontend/unit/Unit.h"
#include "types/mesh/Rectangle.h"
#include "ui/geometry/Rectangle.h"
#include "scene/actor/Mesh.h"
#include "game/frontend/Game.h"
#include "game/backend/unit/Unit.h"
#include "game/frontend/unit/UnitManager.h"
#include "types/texture/Texture.h"
#include <cstring>
#include <memory>

namespace game {
namespace frontend {
namespace widget {

UnitPreview::UnitPreview( Game* const game, ui::UI* const ui )
	: Widget(
	game, ui, ui::WT_UNIT_PREVIEW, "unit-preview", {
		{ "unit", { true, gse::VT_OBJECT, ::game::backend::unit::Unit::WRAP_CLASS } },
	}
) {}

void UnitPreview::Register( ui::dom::Widget* const widget ) {
	const std::weak_ptr< const bool > lifetime = m_lifetime;
	widget->OnUpdate(
		F_WIDGET_UPDATE( this, lifetime ) {
			if ( lifetime.expired() ) { return; }
			ASSERT( data->value.find( "unit" ) != data->value.end(), "unit property missing" );
			const auto* const u = ::game::backend::unit::Unit::Unwrap( data->value.at( "unit" ) );
			ASSERT( u, "invalid unit ptr" );
			m_game->SetWidgetRelation( m_type, u->m_id, widget );
		}
	);
	widget->OnRemove(
		F_WIDGET_REMOVE( this, lifetime ) {
			if ( lifetime.expired() ) { return; }
			m_game->ClearWidgetRelation( m_type, widget );
		}
	);
}

void UnitPreview::Update( ui::dom::Widget* const widget, const void* const data ) {
	const auto* const unit = (const unit::Unit*)data;

	widget->Clear();

	if ( !unit ) {
		return;
	}

	const auto& render = unit->GetRenderData();

	size_t index = 0;

	ASSERT( render.unit.mesh->GetType() == types::mesh::Mesh::MT_RENDER, "unit mesh not render" );
	// Establish the owner before creating any actor that borrows its texture.
	widget->RetainTexture( render.unit.texture_owner );

	const auto preview_texture = [ & ]( types::texture::Texture* source, const size_t texture_index ) {
		if ( !render.unit.texture_owner ) { return source; }
		// A retained generated preview can outlive the scene cache. Its badges
		// have independent manager owners, so copy those two bitmaps.
		const auto destroy = []( types::texture::Texture* texture ) { DELETE( texture ); };
		NEWV( copy, types::texture::Texture, source->GetWidth(), source->GetHeight() );
		std::unique_ptr< types::texture::Texture, decltype( destroy ) > owner( copy, destroy );
		std::memcpy( copy->GetBitmap(), source->GetBitmap(), source->GetBitmapSize() );
		widget->SetTexture( copy, texture_index, false );
		return owner.release();
	};
	AddMeshAndTexture( widget, index++, render.unit.mesh, render.unit.texture, true );
	auto* badge = preview_texture( render.badge.texture, index );
	AddMeshAndTexture( widget, index++, render.badge.mesh, badge, true, { 0.23f, 0.4f }, { 0.06f, 0.08f } );
	auto* healthbar = preview_texture( render.healthbar.texture, index );
	AddMeshAndTexture( widget, index++, render.healthbar.mesh, healthbar, true, { 0.03f, 0.2933f }, { 0.12f, 0.16f } );
}

}
}
}

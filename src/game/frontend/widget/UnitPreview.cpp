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
#include "gse/value/Bool.h"
#include "game/frontend/sprite/InstancedSprite.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>

namespace game {
namespace frontend {
namespace widget {

UnitPreview::UnitPreview( Game* const game, ui::UI* const ui )
	: Widget(
	game, ui, ui::WT_UNIT_PREVIEW, "unit-preview", {
		{ "unit", { true, gse::VT_OBJECT, ::game::backend::unit::Unit::WRAP_CLASS } },
		{ "roster", { false, gse::VT_BOOL } },
        { "native_console", { false, gse::VT_BOOL } },
	}
) {}

void UnitPreview::Register( ui::dom::Widget* const widget ) {
	const std::weak_ptr< const bool > lifetime = m_lifetime;
    const auto old=m_area_handlers.find(widget);
    if(old!=m_area_handlers.end()) {widget->GetGeometry()->RemoveHandler(old->second);m_area_handlers.erase(old);}
    const auto identity=std::make_shared<std::optional<size_t>>();
    const auto area_handler=widget->GetGeometry()->AddHandler(ui::GH_ON_AREA_UPDATE,[this,lifetime,widget,identity]() {
        if(lifetime.expired() || !identity->has_value() || !m_game->GetUM()) {return;}
        Update(widget,m_game->GetUM()->GetUnitById(**identity));
    });
    m_area_handlers.emplace(widget,area_handler);
	widget->OnUpdate(
		F_WIDGET_UPDATE( this, lifetime, identity ) {
			if ( lifetime.expired() ) { return; }
			ASSERT( data->value.find( "unit" ) != data->value.end(), "unit property missing" );
			const auto* const u = ::game::backend::unit::Unit::Unwrap( data->value.at( "unit" ) );
			ASSERT( u, "invalid unit ptr" );
            *identity=u->m_id;
			m_game->SetWidgetRelation( m_type, u->m_id, widget );
		}
	);
	widget->OnRemove(
		F_WIDGET_REMOVE( this, lifetime, identity, area_handler ) {
			if ( lifetime.expired() ) { return; }
            identity->reset();widget->GetGeometry()->RemoveHandler(area_handler);m_area_handlers.erase(widget);
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
	const auto& properties = widget->GetData();
	const auto mode = properties.find( "roster" );
	const bool roster = mode != properties.end() && static_cast< gse::value::Bool* >( mode->second )->value;
	const auto* card = unit->GetCardSprite( roster );
	if ( card ) {
		// Keep the native work surface at 1:1 pixels; only its visible ink lies
		// inside the clipped card. Never fit the transparent 256-square canvas.
		widget->RetainTexture( card->texture_owner );
        const auto& area = widget->GetGeometry()->AsRectangle()->m_area;
        if (area.width<=0 || area.height<=0) {return;}
        const auto native=properties.find("native_console");
        std::shared_ptr<types::texture::Texture> composed;
        if(native!=properties.end() && static_cast<gse::value::Bool*>(native->second)->value) {
            const std::array<int,2> origin{int(std::lround(area.left)),int(std::lround(area.top))-511};
            if(origin[1]==(roster?196:66)) {composed=unit->GetConsoleCard(roster,origin);}
        }
        if(composed) {
            widget->RetainTexture(composed);
            AddMeshAndTexture(widget,index++,nullptr,composed.get(),false,
                {composed->GetWidth()/area.width,composed->GetHeight()/area.height});
        }else {
            const float cx=roster?38:57,cy=roster?26:37;
            AddMeshAndTexture(widget,index++,nullptr,card->texture_owner.get(),false,
                {256/area.width,256/area.height},{(cx-128)/area.width,(cy-128)/area.height});
        }
		const float bx = roster ? 2 : 0, by = roster ? 1 : 0;
        if(auto overlay=unit->GetCardOverlay()) {
            widget->RetainTexture(overlay);
            AddMeshAndTexture(widget,index++,nullptr,overlay.get(),false,
                {23/area.width,30/area.height},{bx/area.width,by/area.height});
            return;
        }
		auto* badge = preview_texture( render.badge.texture, index );
		AddMeshAndTexture( widget, index++, render.badge.mesh, badge, true,
			{ 23 / area.width, 30 / area.height }, { bx / area.width, by / area.height } );
		auto* bar = preview_texture( render.healthbar.texture, index );
		AddMeshAndTexture( widget, index++, render.healthbar.mesh, bar, true,
			{ 2 / area.width, 21 / area.height }, { ( bx + 6 ) / area.width, ( by + 6 ) / area.height } );
		return;
	}
	// Static/native sprites and explicit port markers retain their compact
	// fitted portrait path. The selected card's tall native clip is not a
	// request to stretch those fallback images across its text rows.
	const auto& area = widget->GetGeometry()->AsRectangle()->m_area;
	if ( area.height <= 0 ) { return; }
	const float portrait_ratio = std::min( area.height, 68.0f ) / area.height;
	AddMeshAndTexture( widget, index++, render.unit.mesh, render.unit.texture, true, { 1, portrait_ratio } );
    if(unit->IsNative() && area.width>0) {
        if(auto overlay=unit->GetCardOverlay()) {
            widget->RetainTexture(overlay);
            AddMeshAndTexture(widget,index++,nullptr,overlay.get(),false,
                {23/area.width,30/area.height},{(roster?2.0f:0)/area.width,(roster?1.0f:0)/area.height});
            return;
        }
    }
	auto* badge = preview_texture( render.badge.texture, index );
	AddMeshAndTexture( widget, index++, render.badge.mesh, badge, true, { 0.23f, 0.4f * portrait_ratio }, { 0.06f, 0.08f * portrait_ratio } );
	auto* healthbar = preview_texture( render.healthbar.texture, index );
	AddMeshAndTexture( widget, index++, render.healthbar.mesh, healthbar, true, { 0.03f, 0.2933f * portrait_ratio }, { 0.12f, 0.16f * portrait_ratio } );
}

}
}
}

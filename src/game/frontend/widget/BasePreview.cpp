#include "BasePreview.h"

#include "ui/UI.h"
#include "ui/dom/Widget.h"
#include "game/frontend/base/Base.h"
#include "types/mesh/Rectangle.h"
#include "ui/geometry/Rectangle.h"
#include "scene/actor/Mesh.h"
#include "game/frontend/Game.h"
#include "game/backend/base/Base.h"
#include "game/frontend/base/BaseManager.h"
#include "gse/value/Bool.h"
#include "game/frontend/faction/Faction.h"
#include "game/frontend/sprite/Sprite.h"
#include "game/frontend/sprite/InstancedSprite.h"
#include <algorithm>
#include <cstring>
#include <cctype>
#include "engine/Engine.h"
#include "loader/texture/TextureLoader.h"

namespace game {
namespace frontend {
namespace widget {

BasePreview::BasePreview( Game* const game, ui::UI* const ui )
	: Widget(
	game, ui, ui::WT_BASE_PREVIEW, "base-preview", {
		{ "base",     { true,  gse::VT_OBJECT, ::game::backend::base::Base::WRAP_CLASS } },
		{ "no_badge", { false, gse::VT_BOOL } },
		{ "roster", { false, gse::VT_BOOL } },
        { "card", { false, gse::VT_BOOL } },
	}
) {}

void BasePreview::Register( ui::dom::Widget* const widget ) {
	const std::weak_ptr<const bool> lifetime=m_lifetime;
	widget->OnUpdate(
		F_WIDGET_UPDATE( this, lifetime ) {
			if(lifetime.expired()) {return;}
			const auto* const b = ::game::backend::base::Base::Unwrap( data->value.at( "base" ) );
			ASSERT( b, "invalid base ptr" );
			m_game->SetWidgetRelation( m_type, b->m_id, widget );
		}
	);
	widget->OnRemove(
		F_WIDGET_REMOVE( this, lifetime ) {
			if(lifetime.expired()) {return;}
			m_game->ClearWidgetRelation( m_type, widget );
		}
	);
}

void BasePreview::Update( ui::dom::Widget* const widget, const void* const data ) {
	const auto* const base = (const base::Base*)data;

	widget->Clear();

	if ( !base ) {
		return;
	}

	const auto& render = base->GetRenderData();

	size_t index = 0;

	ASSERT( render.base.mesh->GetType() == types::mesh::Mesh::MT_RENDER, "base mesh not render" );

	const auto& d = widget->GetData();

    const auto flag=[&](const char* name) {const auto i=d.find(name);return i!=d.end() && static_cast<gse::value::Bool*>(i->second)->value;};
    const bool roster=flag("roster"),no_badge=flag("no_badge");
    auto filename=base->GetFaction()->GetBaseFilename();
    std::transform(filename.begin(),filename.end(),filename.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    const auto* source=base->GetSprite()->instanced_sprite;
    // The current original receipt admits University stock land/sea artwork.
    // Other content retains the working fitted fallback without a parity claim.
    if(flag("card") && filename=="univ.pcx" && source->wh.x==100 && source->wh.y==75
        && (no_badge || (render.badge.texture->GetHeight()==16 && render.badge.texture->GetWidth()<=64))) {
        const auto key=source->key+(roster?"_roster":"_selected");
        auto found=m_cards.find(key);
        if(found==m_cards.end()) {
            const auto native=g_engine->GetTextureLoader()->GetSmacNativeBaseCard(filename,source->xy.x,source->xy.y,roster);
            auto card=native?std::optional<SmacBaseCard>(*native):MakeSmacBaseCard(*render.base.texture,source->xy.x,source->xy.y,roster);
            if(card) {found=m_cards.emplace(key,std::move(*card)).first;}
        }
        if(found!=m_cards.end()) {
            const auto& area=widget->GetGeometry()->AsRectangle()->m_area;
            if(area.width<=0 || area.height<=0) {return;}
            const auto& card=found->second;
            widget->RetainTexture(card.texture);
            const float anchor_x=roster?13:20,anchor_y=roster?1:0;
            AddMeshAndTexture(widget,index++,nullptr,card.texture.get(),false,
                {card.texture->GetWidth()/area.width,card.texture->GetHeight()/area.height},
                {(anchor_x+card.offset[0])/area.width,(anchor_y+card.offset[1])/area.height});
            if(!no_badge) {
                auto badge=std::make_shared<types::texture::Texture>("SMAC Base card population",render.badge.texture->GetWidth(),16);
                std::memcpy(badge->GetBitmap(),render.badge.texture->GetBitmap(),badge->GetBitmapSize());
                widget->RetainTexture(badge);
                AddMeshAndTexture(widget,index++,nullptr,badge.get(),false,
                    {badge->GetWidth()/area.width,16/area.height},{(roster?2.0f:20.0f)/area.width,(roster?1.0f:0.0f)/area.height});
            }
            return;
        }
    }
	AddMeshAndTexture( widget, index++, render.base.mesh, render.base.texture, true );
	
	auto it = d.find( "no_badge" );
	if ( it == d.end() || !( (gse::value::Bool*)it->second )->value ) {
		AddMeshAndTexture( widget, index++, render.badge.mesh, render.badge.texture, true, { 0.16f, 0.27f }, { 0.04f, 0.04f } );
	}
}

}
}
}

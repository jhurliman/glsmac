#include "Minimap.h"
#include "SmacMinimapGeometry.h"
#include "SmacMinimapRaster.h"
#include "game/frontend/SmacDevelopedOverlay.h"
#include "engine/Engine.h"
#include "loader/texture/TextureLoader.h"
#include "loader/texture/SmacGraphPalette.h"
#include "loader/font/FontLoader.h"
#include "types/Font.h"
#include "gse/value/Array.h"

#include <memory>
#include <stdexcept>

#include "ui/dom/Widget.h"
#include "types/mesh/Rectangle.h"
#include "ui/geometry/Rectangle.h"
#include "scene/actor/Mesh.h"
#include "types/texture/Texture.h"
#include "ui/UI.h"
#include "input/Event.h"
#include "game/frontend/Game.h"
#include "gse/value/Object.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"
#include "gse/value/Bool.h"
#include "types/texture/Texture.h"

namespace game {
namespace frontend {
namespace widget {

Minimap::Minimap( Game* const game, ui::UI* const ui )
	: Widget( game, ui, ui::WT_MINIMAP, "minimap", {
		{ "overview", { false, gse::VT_OBJECT, {} } },
        {"detail",{false,gse::VT_OBJECT,{}}},
        {"mode",{false,gse::VT_STRING,{}}},
	} ) {}

Minimap::~Minimap() {
	if ( m_last_texture ) {
		delete m_last_texture;
	}
}

void Minimap::ReadDetail(gse::value::Object* data) {
    namespace detail=::game::smac_detail;
    const auto found=data->value.find("detail");
    if(found!=data->value.end()) {
        if(!found->second || found->second->type!=gse::VT_OBJECT) {throw std::invalid_argument("Invalid Detail frame");}
        const auto& fields=static_cast<gse::value::Object*>(found->second)->value;
        m_detail.reset();
        if(!fields.empty()) {
            const auto field=[&](const char* name,gse::value_type_t type)->gse::Value* {
                const auto it=fields.find(name);if(it==fields.end() || !it->second || it->second->type!=type) {throw std::invalid_argument("Incomplete Detail frame");}return it->second;
            };
            if(static_cast<gse::value::String*>(field("kind",gse::VT_STRING))->value!="smac-detail-ordinary-v2") {throw std::invalid_argument("Unknown Detail profile");}
            const auto& packed=static_cast<gse::value::String*>(field("tiles",gse::VT_STRING))->value;
            if(packed.size()!=3200*12) {throw std::invalid_argument("Invalid Detail world extent");}
            detail::World next;next.wrap=static_cast<gse::value::Bool*>(field("horizontal_wrap",gse::VT_BOOL))->value;
            for(size_t i=0;i<3200;++i) {
                auto& tile=next.tiles[i];tile.band=static_cast<uint8_t>(packed[i*12]);const auto flags=static_cast<uint8_t>(packed[i*12+1]);
                if(tile.band>7 || (flags&~7u)) {throw std::invalid_argument("Invalid Detail tile flags");}
                tile.known=flags&1u;tile.base_known=flags&2u;tile.supported=flags&4u;
                for(size_t v=0;v<5;++v) {const unsigned word=static_cast<uint8_t>(packed[i*12+2+v*2])|(unsigned(static_cast<uint8_t>(packed[i*12+3+v*2]))<<8);
                    tile.heights[v]=word&0x8000u?int(word)-65536:int(word);if(tile.heights[v]<0 || tile.heights[v]>127) {throw std::invalid_argument("Unclosed Detail deformation");}}
            }
            const auto& bases=static_cast<gse::value::Array*>(field("bases",gse::VT_ARRAY))->value;
            if(bases.size()>3200) {throw std::invalid_argument("Invalid Detail Base count");}
            for(const auto* value:bases) {
                if(!value || value->type!=gse::VT_OBJECT) {throw std::invalid_argument("Invalid Detail Base");}
                const auto& b=static_cast<const gse::value::Object*>(value)->value;
                const auto item=[&](const char* name,gse::value_type_t type)->gse::Value* {const auto it=b.find(name);if(it==b.end() || !it->second || it->second->type!=type) {throw std::invalid_argument("Incomplete Detail Base");}return it->second;};
                const auto x=static_cast<gse::value::Int*>(item("x",gse::VT_INT))->value,y=static_cast<gse::value::Int*>(item("y",gse::VT_INT))->value;
                if(x<0 || x>=80 || y<0 || y>=80 || ((x^y)&1)) {throw std::invalid_argument("Invalid Detail Base tile");}
                const auto& tile=next.tiles[size_t(y)*40+x/2];if(!tile.base_known || !tile.supported) {throw std::invalid_argument("Unadmitted Detail Base");}
                const auto file=static_cast<gse::value::String*>(item("file",gse::VT_STRING))->value;
                if(file.empty() || file.size()>1024) {throw std::invalid_argument("Invalid Detail Base source");}
                const auto population=static_cast<gse::value::Int*>(item("population",gse::VT_INT))->value;
                const auto column=static_cast<gse::value::Int*>(item("column",gse::VT_INT))->value,row=static_cast<gse::value::Int*>(item("row",gse::VT_INT))->value;
                const auto owner=static_cast<gse::value::Int*>(item("native_owner",gse::VT_INT))->value;
                if(population<0 || population>127 || column<0 || column>3 || row<0 || row>5 || owner<0 || owner>7) {throw std::invalid_argument("Invalid Detail Base selector");}
                if(owner==0 && static_cast<gse::value::Bool*>(item("style_known",gse::VT_BOOL))->value) {throw std::invalid_argument("Unknown Detail population palette owner");}
                next.bases.push_back({{int(x),int(y)},file,int(population),static_cast<gse::value::Bool*>(item("owned",gse::VT_BOOL))->value,1+76*int(row),int(column),int(owner),
                    static_cast<gse::value::Bool*>(item("filled",gse::VT_BOOL))->value,static_cast<gse::value::Bool*>(item("spartans",gse::VT_BOOL))->value,
                    static_cast<gse::value::Bool*>(item("style_known",gse::VT_BOOL))->value});
            }
            m_detail=std::move(next);m_using_overview=true;
        }
        if(!m_detail) {m_detail_mode=false;}
    }
    const auto mode=data->value.find("mode");
    if(mode!=data->value.end()) {
        if(!mode->second || mode->second->type!=gse::VT_STRING) {throw std::invalid_argument("Invalid minimap mode");}
        const auto& choice=static_cast<gse::value::String*>(mode->second)->value;
        if(choice!="world" && choice!="detail") {throw std::invalid_argument("Unknown minimap mode");}
        if(choice=="world") {m_detail_mode=false;}
        else if(m_detail) {
            if(!m_detail_initialized && m_game) {if(const auto center=m_game->GetActiveMapTileCenter()) {m_detail_center={int(center->x),int(center->y)};}}
            m_detail_initialized=m_detail_mode=true;
        }
    }
}

void Minimap::DrawDetail(ui::dom::Widget* widget) {
    if(!m_detail) {return;}
    namespace detail=::game::smac_detail;
    auto* preview=widget->GetTexture(LI_PREVIEW);
    if(preview->GetWidth()!=239 || preview->GetHeight()!=119) {preview->Clear();preview->FullUpdate();return;}
    const auto palette=g_engine->GetTextureLoader()->GetSmacGraphPalette();if(!palette) {throw std::runtime_error("Detail palette unavailable");}
    const auto shadow=smac_developed_overlay::ShadowLookup(palette->colors);
    auto frame=detail::Terrain(*m_detail,m_detail_center);
    const auto* font=m_detail->bases.empty()?nullptr:g_engine->GetFontLoader()->LoadCustomFont("arialbd.ttf",9,loader::font::RasterMode::MONOCHROME);
    for(const auto& entry:detail::Entries(*m_detail,m_detail_center)) {
        const auto base=std::find_if(m_detail->bases.begin(),m_detail->bases.end(),[&](const auto& b){return b.tile==entry.tile;});
        if(base==m_detail->bases.end()) {continue;}
        const detail::Point anchor{{entry.anchor[0],entry.anchor[1]-14}};
        const auto source=g_engine->GetTextureLoader()->GetSmacDetailBaseSprite(base->file,base->crop_y,base->column);
        if(!source) {continue;}
        detail::Composite(frame,anchor,*source,shadow,[](int,int,int,int,uint8_t){});
        if(!base->style_known || !palette->text_available) {continue;}
        const auto style=smac_base_body::Colors(base->filled,base->spartans,palette->normal.at(base->native_owner),palette->alternate.at(base->native_owner),palette->text.at(base->native_owner));
        const auto layout=smac_base_body::PopulationLayout(base->population,font->m_symbols['8'].ax,7);
        detail::PopulationBox(frame,anchor,layout,style);
        int pen=0;
        for(const auto ch:std::to_string(base->population)) {
            const auto& glyph=font->m_symbols[static_cast<uint8_t>(ch)];
            for(size_t y=0;y<glyph.height;++y) {for(size_t x=0;x<glyph.width;++x) {if(glyph.data[y*glyph.width+x]) {
                const int px=anchor[0]+3+pen+glyph.left+int(x),py=anchor[1]+layout.baseline-glyph.top+int(y);
                if(px>=0 && px<239 && py>=0 && py<119) {frame.pixels[py*240+px]=style.text;}
            }}}
            pen+=glyph.ax;
        }
    }
    for(int y=0;y<119;++y) {for(int x=0;x<239;++x) {
        const auto& rgb=palette->colors[frame.pixels[y*240+x]];preview->SetPixel(x,y,types::Color::RGBA(rgb[0],rgb[1],rgb[2],255));
    }}
    preview->FullUpdate();auto* selection=widget->GetTexture(LI_SELECTION);selection->Clear();selection->FullUpdate();
}

void Minimap::DrawDetailOutline(ui::dom::Widget* widget) {
    if(!m_detail || !m_overview || m_detail_mode) {return;}
    auto* texture=widget->GetTexture(LI_SELECTION);texture->Clear();
    namespace navigation=smac_minimap;
    const auto layout=navigation::OverviewLayout({m_overview->width,m_overview->height},{int(texture->GetWidth()),int(texture->GetHeight())});
    const auto view=::game::smac_detail::View(m_detail_center,m_detail->wrap);
    const auto a=navigation::TileToPixel(view.first,layout,!m_detail->wrap),b=navigation::TileToPixel({view.first[0]+10,view.first[1]+10},layout,!m_detail->wrap);
    if(a && b) {
        const auto color=types::Color::FromRGBA(types::Color::RGB(255,255,0));
        const auto horizontal=[&](int left,int right,int y) {for(int x=left;x<=right;++x) {texture->SetPixelMaybe(x,y,color);}};
        const auto vertical=[&](int x,int top,int bottom) {for(int y=top;y<=bottom;++y) {texture->SetPixelMaybe(x,y,color);}};
        const auto x1=(*a)[0],y1=(*a)[1],x2=(*b)[0],y2=(*b)[1];
        if(x1<=x2) {horizontal(x1,x2-1,y1);horizontal(x1,x2-1,y2-1);vertical(x1,y1,y2-1);vertical(x2-1,y1,y2-1);}
        else {const int left=layout.offset[0],right=(left?left+layout.image[0]:layout.window[0])-1;
            horizontal(left,x2-1,y1);horizontal(left,x2-2,y2-1);vertical(x2-1,y1+1,y2-1);
            horizontal(x1+1,right,y1);horizontal(x1,right,y2-1);vertical(x1,y1,y2-2);}
    }texture->FullUpdate();
}

void Minimap::ReadOverview( ui::dom::Widget* widget, gse::value::Object* data ) {
	if ( !data ) { return; }
    ReadDetail(data);
	const auto found = data->value.find( "overview" );
	if ( found == data->value.end() ) {if(m_using_overview) {DrawOverview(widget);} return;} // mode-only or legacy capture-only caller
	if ( found->second->type != gse::VT_OBJECT ) { throw std::runtime_error( "Invalid minimap overview payload" ); }
	const auto* frame = static_cast<gse::value::Object*>( found->second );
	std::optional< Overview > next;
	if ( !frame->value.empty() ) {
		const auto field = [ frame ]( const char* name, gse::value_type_t type ) -> gse::Value* {
			const auto item = frame->value.find( name );
			if ( item == frame->value.end() || item->second->type != type ) { throw std::runtime_error( "Incomplete minimap overview payload" ); }
			return item->second;
		};
		const auto width = static_cast<gse::value::Int*>( field( "width", gse::VT_INT ) )->value;
		const auto height = static_cast<gse::value::Int*>( field( "height", gse::VT_INT ) )->value;
		const auto& rgba = static_cast<gse::value::String*>( field( "rgba", gse::VT_STRING ) )->value;
		if ( static_cast<gse::value::String*>( field( "kind", gse::VT_STRING ) )->value != "smac-coarse-physical-v1"
			|| width < 2 || width > 512 || ( width & 1 ) || height <= 4 || height > 256 || ( height & 1 )
			|| rgba.size() != static_cast<size_t>( width * height * 2 ) ) { throw std::runtime_error( "Unsupported minimap overview payload" ); }
		next = Overview{ static_cast<int32_t>( width ), static_cast<int32_t>( height ),
			static_cast<gse::value::Bool*>( field( "horizontal_wrap", gse::VT_BOOL ) )->value, rgba };
	}
	m_overview = std::move( next );
	m_using_overview = true;
	m_is_dragging = false;
	DrawOverview( widget );
}

void Minimap::DrawOverview( ui::dom::Widget* widget ) {
	// The native overview is supplied by the observer-filtered snapshot. Never
	// use a full-world GPU capture to fill unavailable survey/knowledge fields.
    if(m_detail_mode && m_detail) {DrawDetail(widget);return;}
	auto* preview = widget->GetTexture( LI_PREVIEW );
	preview->Clear();
	if ( m_overview && preview->GetWidth() && preview->GetHeight() ) {
		const auto raster = smac_minimap::OverviewTarget( {{ m_overview->width, m_overview->height }},
			{{ static_cast<int32_t>( preview->GetWidth() ), static_cast<int32_t>( preview->GetHeight() ) }}, m_overview->rgba, m_overview->horizontal_wrap );
		std::memcpy( preview->GetBitmap(), raster.rgba.data(), raster.rgba.size() );
	}
	preview->FullUpdate();
	auto* selection = widget->GetTexture( LI_SELECTION );
	selection->Clear();
	selection->FullUpdate();
    DrawDetailOutline(widget);
}

void Minimap::SetMinimapSelection( const types::Vec2< float > position_percents, const types::Vec2< float > zoom ) {

	// save for future redraws
	m_last_position_percents = position_percents;
	m_last_zoom = zoom;

	WithWidget(
		F_WITH_WIDGET( this, &position_percents, &zoom ) {
			SetMinimapSelectionImpl( widget, position_percents, zoom );
		}
	);
}

void Minimap::Register( ui::dom::Widget* const widget ) {
	AddMeshAndTexture( widget, LI_PREVIEW );
	AddMeshAndTexture( widget, LI_SELECTION );
	const std::weak_ptr< const bool > lifetime = m_lifetime;
	widget->OnUpdate( F_WIDGET_UPDATE( this, lifetime ) {
		if ( lifetime.expired() ) { return; }
		ReadOverview( widget, data );
	} );

	const auto* a = &widget->GetGeometry()->m_area;

	const auto f_scrollto = [ this, a, lifetime ]( const types::Vec2< ui::coord_t > coords ) -> void {
		if ( lifetime.expired() || !m_game ) { return; }
        if(m_detail_mode) {return;}
		if ( m_using_overview ) {
			if ( !m_overview || a->width < 1 || a->height < 1 ) { return; }
			const auto layout = smac_minimap::OverviewLayout( {{ m_overview->width, m_overview->height }},
				{{ static_cast<int32_t>( a->width ), static_cast<int32_t>( a->height ) }} );
			const auto tile = smac_minimap::PixelToTile( {{ static_cast<int32_t>( coords.x ), static_cast<int32_t>( coords.y ) }}, layout );
            if(tile && m_shift_down && m_detail) {
                if(const auto center=m_game->GetActiveMapTileCenter()) {
                    m_detail_center={static_cast<int>(center->x),static_cast<int>(center->y)};
                    m_detail_initialized=m_detail_mode=true;m_is_dragging=false;
                    WithWidget(F_WITH_WIDGET(this) {DrawDetail(widget);});
                }
            } else if(tile) {m_game->CenterAtMapTile({static_cast<size_t>((*tile)[0]),static_cast<size_t>((*tile)[1])});}
			return;
		}
		//Log( "Minimap select at " + coords.ToString() );
		auto x = coords.x;
		if ( x < 0 ) {
			x = 0;
		}
		if ( x >= a->width ) {
			x = a->width - 1;
		}
		x -= a->width / 2;
		if ( x < 0 ) {
			x += a->width - 4; // TODO: where does this 4 come from?
		}
		m_game->CenterAtCoordinatePercents(
			{
				x / a->width,
				coords.y / a->height
			}
		);
	};

    const auto key=[this,lifetime](const input::Event& event) {
        if(lifetime.expired()) {return false;}
        m_shift_down=(event.data.key.modifiers&input::KM_SHIFT)!=0;
        if(event.data.key.code==input::K_SHIFT) {m_shift_down=event.type==input::EV_KEY_DOWN;}
        return false;
    };
    widget->SetGlobalEventHandler(input::EV_KEY_DOWN,key);widget->SetGlobalEventHandler(input::EV_KEY_UP,key);
	widget->SetEventHandler(
		input::EV_MOUSE_DOWN, F_EVENT_HANDLER( this, a, f_scrollto, lifetime ) {
			if ( lifetime.expired() ) { return false; }
            if(m_detail_mode) {return false;}
			if ( event.data.mouse.button == input::MB_LEFT ) {
				// Retain the port's drag gesture; only its tile mapping is native-
				// verified here, not the original drag cadence or wheel policy.
				m_is_dragging = true;
				f_scrollto(
					{
						event.data.mouse.x - a->left,
						event.data.mouse.y - a->top
					}
				);
				return true;
			}
			return false;
		}
	);
	widget->SetGlobalEventHandler(
		input::EV_MOUSE_MOVE, F_EVENT_HANDLER( this, a, f_scrollto, lifetime ) {
			if ( lifetime.expired() ) { return false; }
			if ( m_is_dragging ) {
				f_scrollto(
					{
						event.data.mouse.x - a->left,
						event.data.mouse.y - a->top
					}
				);
				return true;
			}
			return false;
		}
	);
	widget->SetGlobalEventHandler(
		input::EV_MOUSE_UP, F_EVENT_HANDLER( this, lifetime ) {
			if ( lifetime.expired() ) { return false; }
			if ( event.data.mouse.button == input::MB_LEFT ) {
				if ( m_is_dragging ) {
					m_is_dragging = false;
					return true;
				}
			}
			return false;
		}
	);
	widget->SetEventHandler(
		input::EV_MOUSE_SCROLL, F_EVENT_HANDLER( this, a, f_scrollto, lifetime ) {
			if ( lifetime.expired() || !m_game ) { return false; }
			f_scrollto(
				{
					event.data.mouse.x - a->left,
					event.data.mouse.y - a->top
				}
			);
			m_game->SmoothScroll( event.data.mouse.scroll_y );
			return true;
		}
	);

	if ( m_using_overview ) { DrawOverview( widget ); }
	else if ( m_last_texture ) {
		UpdateImpl( widget, m_last_texture );
		SetMinimapSelectionImpl( widget, m_last_position_percents, m_last_zoom );
	}
}

const types::Vec2< ui::coord_t > Minimap::FindLargestArea() const {
	types::Vec2< ui::coord_t > result = { 0.0f, 0.0f };
	WITH_WIDGET( &result ) {
		const auto& area = widget->GetGeometry()->m_area;
		if ( area.width > result.x ) {
			result.x = area.width;
		}
		if ( area.height > result.y ) {
			result.y = area.height;
		}
	} );
	return result;
}

void Minimap::UpdateCapture( types::texture::Texture* texture ) {
	const auto release = []( types::texture::Texture* value ) { DELETE( value ); };
	std::unique_ptr< types::texture::Texture, decltype( release ) > capture( texture, release );
	Update( nullptr, capture.get() );
}

void Minimap::Update( ui::dom::Widget* const widget, const void* const data ) {
	const auto* const minimap_texture = (const types::texture::Texture*)data;
	const auto w = minimap_texture->GetWidth();
	const auto h = minimap_texture->GetHeight();

	// save last texture for redraws later
	if ( !m_last_texture ) {
		m_last_texture = new types::texture::Texture();
	}
	m_last_texture->Resize( w, h );
	m_last_texture->AddFrom( minimap_texture, types::texture::AM_DEFAULT, 0, 0, w - 1, h - 1 );

	WITH_WIDGET( this, &minimap_texture, &w, &h ) { // TODO: refactor
		UpdateImpl( widget, minimap_texture );
	} );
}

void Minimap::UpdateImpl( ui::dom::Widget* const widget, const types::texture::Texture* const texture ) {
	if ( m_using_overview ) { DrawOverview( widget ); return; }
	// TODO: downscale in case of multiple minimaps of different sizes
	auto* const wtexture = widget->GetTexture( LI_PREVIEW );
	const auto w = texture->GetWidth();
	const auto h = texture->GetHeight();
	if ( w == wtexture->GetWidth() && h == wtexture->GetHeight() ) { // update only if matches exactly (won't match for example during resize)
		wtexture->AddFrom( texture, types::texture::AM_MIRROR_Y, 0, 0, w - 1, h - 1 );
		wtexture->FullUpdate();
	}
}

void Minimap::SetMinimapSelectionImpl( ui::dom::Widget* const widget, const types::Vec2< float > position_percents, const types::Vec2< float > zoom ) {
	// The native yellow outline belongs to the separate detail-view geometry.
	// Do not relabel the legacy active-camera white rectangle as that outline.
	if ( m_using_overview ) {if(!m_detail_mode) {DrawDetailOutline(widget);} return;}
	const size_t w = widget->GetGeometry()->m_area.width;
	const size_t h = widget->GetGeometry()->m_area.height;

	const types::Vec2< ssize_t > ss = { // selection size
		(ssize_t)( w * zoom.x ),
		(ssize_t)( h * zoom.y ),
	};
	if ( ss.x <= 0 || ss.y <= 0 ) {
		return;
	}

	// final rectangle coords
	const types::Vec2< ssize_t > top_left = {
		(ssize_t)( w * position_percents.x - ss.x / 2 ),
		(ssize_t)( h * position_percents.y - ss.y / 2 ),
	};
	const types::Vec2< ssize_t > bottom_right = {
		top_left.x + ss.x,
		top_left.y + ss.y,
	};

	const auto& texture = widget->GetTexture( LI_SELECTION );

	// redraw selection texture
	// TODO: add 'lines' shader program and do it without texture
	// TODO: optimize?

	const types::Color c( 1.0f, 1.0f, 1.0f, 0.6f );

	texture->Resize( w, h );
	texture->Clear();
	for ( char instance = -1 ; instance <= 1 ; instance += 2 ) {
		const auto shift = w / 2 * instance;
		for ( auto y = top_left.y ; y < bottom_right.y ; y++ ) {
			texture->SetPixelMaybe( shift + top_left.x, y, c );
			texture->SetPixelMaybe( shift + bottom_right.x, y, c );
		}
		for ( auto x = top_left.x ; x < bottom_right.x ; x++ ) {
			texture->SetPixelMaybe( shift + x, top_left.y, c );
			texture->SetPixelMaybe( shift + x, bottom_right.y, c );
		}
	}
	texture->FullUpdate();
}

}
}
}

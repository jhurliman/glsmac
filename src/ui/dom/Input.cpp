#include "Input.h"

#include "Text.h"
#include "Surface.h"
#include "SmacEditState.h"
#include "ui/geometry/Geometry.h"
#include "engine/Engine.h"
#include "input/Input.h"
#include "input/Event.h"
#include "util/String.h"
#include "ui/UI.h"
#include "gse/value/Int.h"
#include "gse/value/Bool.h"
#include "gse/value/Undefined.h"
#include "gse/callable/Native.h"
#include <limits>
#include <algorithm>

namespace ui {
namespace dom {

Input::Input( DOM_ARGS )
	: Panel( DOM_ARGS_PASS, "Input", false )
	, Focusable() {

	m_text = new Text( GSE_CALL, ui, this, {} );
	m_text->GetGeometry()->SetAlign( geometry::Geometry::ALIGN_LEFT_CENTER );
	m_text->GetGeometry()->SetLeft( 4 ); // TODO: configurable?
	m_text->GetGeometry()->SetRight( 4 ); // TODO: configurable?
	Embed( m_text );
    m_selection_surface=new Surface(GSE_CALL,ui,this,{});Embed(m_selection_surface);
    m_selection_text=new Text(GSE_CALL,ui,this,{});Embed(m_selection_text);
    m_caret_surface=new Surface(GSE_CALL,ui,this,{});Embed(m_caret_surface);
    for(auto* node:{static_cast<Drawable*>(m_selection_surface),static_cast<Drawable*>(m_selection_text),static_cast<Drawable*>(m_caret_surface)}) {
        node->GetGeometry()->SetAlign(geometry::Geometry::ALIGN_LEFT_TOP);node->Hide();
    }

	m_geometry->SetOverflowMode( geometry::Geometry::OM_HIDDEN );

	m_value = "";
	m_is_blink_cursor_visible = false;
	UpdateText();

	Property(
		GSE_CALL, "value", gse::VT_STRING, VALUE( gse::value::String, , "" ), PF_NONE,
		[ this ]( GSE_CALLABLE, gse::Value* const v ) {
			SetValue( GSE_CALL, ( (gse::value::String*)v )->value );
			return nullptr;
		}
	);

    Property(GSE_CALL,"max_bytes",gse::VT_INT,VALUE(gse::value::Int,,0),PF_NONE,
        [this](GSE_CALLABLE,gse::Value* const v) {
            const auto limit=static_cast<gse::value::Int*>(v)->value;
            if(limit<0 || static_cast<uint64_t>(limit)>std::numeric_limits<size_t>::max()) {
                GSE_ERROR(gse::EC.UI_ERROR,"Input max_bytes must be a nonnegative byte count");
            }
            m_max_bytes=static_cast<size_t>(limit);
            SetValue(GSE_CALL,m_value);
        });
	ForwardProperty( GSE_CALL, "color", m_text );
    Property(GSE_CALL,"font",gse::VT_STRING,nullptr,PF_NONE,[this](GSE_CALLABLE,gse::Value* const value) {
        WrapSetStatic(m_text,"font",value,GSE_CALL);WrapSetStatic(m_selection_text,"font",value,GSE_CALL);UpdateText();
    });
    Property(GSE_CALL,"textbaseline",gse::VT_INT,nullptr,PF_NONE,[this](GSE_CALLABLE,gse::Value* const value) {
        WrapSetStatic(m_text,"baseline",value,GSE_CALL);WrapSetStatic(m_selection_text,"baseline",value,GSE_CALL);
    });
    Property(GSE_CALL,"selection_color",gse::VT_STRING,nullptr,PF_NONE,[this](GSE_CALLABLE,gse::Value* const value) {
        ParseColor(GSE_CALL,static_cast<gse::value::String*>(value)->value,m_selection_color);
        WrapSetStatic(m_selection_text,"color",value,GSE_CALL);UpdateText();
    });
    ForwardProperty(GSE_CALL,"selection_background","background",m_selection_surface);
    Property(GSE_CALL,"caret_color",gse::VT_STRING,nullptr,PF_NONE,[this](GSE_CALLABLE,gse::Value* const value) {
        ParseColor(GSE_CALL,static_cast<gse::value::String*>(value)->value,m_caret_color);UpdateText();
    });
    Property(GSE_CALL,"native_edit",gse::VT_BOOL,nullptr,PF_NONE,[this](GSE_CALLABLE,gse::Value* const value) {
        m_native_edit=static_cast<gse::value::Bool*>(value)->value;m_cursor=m_value.size();m_anchor=m_native_edit ? 0:m_cursor;FixAlign();UpdateText();
    });
    Method(GSE_CALL,"select_all",NATIVE_CALL(this) {
        N_EXPECT_ARGS(0);m_anchor=0;m_cursor=m_value.size();UpdateText();return VALUE(gse::value::Undefined);
    }));
    Method(GSE_CALL,"focus",NATIVE_CALL(this) {
        N_EXPECT_ARGS(0);
        if(!m_ui || !m_is_visible) {GSE_ERROR(gse::EC.UI_ERROR,"Cannot focus an inactive input");}
        m_ui->Focus(this);
        return VALUE(gse::value::Undefined);
    }));

	Events(
		{
			input::EV_CHANGE,
			input::EV_INPUT,
		}
	);

	Iterable(
		[ this ]() {
            if(m_native_edit) {
                if(m_is_focused) {
                    const auto now=std::chrono::steady_clock::now();
                    if(now>=m_native_blink_at) {
                        const auto interval=std::chrono::milliseconds(500);
                        m_native_blink_at+=interval*((now-m_native_blink_at)/interval+1);
                        m_is_blink_cursor_visible=!m_is_blink_cursor_visible;UpdateText();
                    }
                }return;
            }
			while ( m_blinker.HasTicked() ) {
				m_is_blink_cursor_visible = !m_is_blink_cursor_visible;
				UpdateText();
			}
		}
	);

	GeometryHandler(
		GH_ON_AREA_UPDATE, [ this ]() {
			FixAlign();if(m_native_edit) {UpdateNativeText();}
		}
	);
}

void Input::OnFocus() {
	if ( !m_is_focused ) {
		m_is_focused = true;
		if(m_native_edit) {m_native_blink_at=std::chrono::steady_clock::now()+std::chrono::milliseconds(500);UpdateText();return;}
		m_blinker.SetInterval(250);
		if ( !m_is_blink_cursor_visible ) {
			m_is_blink_cursor_visible = true;
			UpdateText();
		}
	}
}
void Input::OnDefocus() {
	if ( m_is_focused ) {
		m_is_focused = false;
		m_blinker.Stop();
		if ( m_is_blink_cursor_visible ) {
			m_is_blink_cursor_visible = false;
			UpdateText();
		}
	}
}

void Input::Show() {
	Panel::Show();
	if ( m_ui ) {
		m_ui->AddFocusable( this );
	}
}

void Input::Hide() {
	if ( m_ui ) {
		m_ui->RemoveFocusable( this );
	}
	Panel::Hide();
}

const bool Input::ProcessEventImpl( GSE_CALLABLE, const input::Event& event ) {
	switch ( event.type ) {
		case input::EV_KEY_DOWN: {
			if ( m_is_focused ) {
                if(m_native_edit && NativeKey(GSE_CALL,event)) {return true;}
				switch ( event.data.key.code ) {
					case input::K_BACKSPACE: {
						if ( !m_value.empty() ) {
							SetValue( GSE_CALL, m_value.substr( 0, m_value.size() - 1 ) );
						}
						return true;
					}
					case input::K_ENTER: {
						input::Event e;
						e.SetType( input::EV_INPUT );
						e.data.value.change_select.text = new std::string( m_value );
						ProcessEvent( GSE_CALL, e );
						delete e.data.value.change_select.text;
						return true;
					}
					case input::K_INSERT: {
						if ( event.data.key.modifiers & input::KM_SHIFT ) {
							SetValue( GSE_CALL, m_value + util::String::FilterPrintable( g_engine->GetInput()->GetClipboardText() ) );
							return true;
						}
					}
					default: {
						if ( event.data.key.is_printable ) {
							SetValue( GSE_CALL, m_value + std::string( 1, event.data.key.key ) );
							return true;
						}
					}
				}
			}
			break;
		}
		case input::EV_MOUSE_DOWN: {
			if ( event.data.mouse.button == input::MB_LEFT && m_native_edit ) {
                if(m_ui && !m_is_focused) {m_ui->Focus(this);}
                const auto x=event.data.mouse.x-m_geometry->GetEffectiveArea().left;size_t cursor=0;
                while(cursor<m_value.size()) {
                    const auto left=m_text->MeasureWidth(m_value.substr(0,cursor)),right=m_text->MeasureWidth(m_value.substr(0,cursor+1));
                    if(x<static_cast<coord_t>((left+right)/2)) {break;}++cursor;
                }
                m_cursor=m_anchor=cursor;UpdateNativeText();return true;
            }
			if ( event.data.mouse.button == input::MB_LEFT && !m_is_focused ) {
				if ( m_ui ) {
					m_ui->Focus( this );
				}
				return true;
			}
			break;
		}
		default: {
		}
	}
	return Panel::ProcessEventImpl( GSE_CALL, event );
}

void Input::WrapEvent( GSE_CALLABLE, const input::Event& e, gse::value::object_properties_t& obj ) const {
	switch ( e.type ) {
		case input::EV_CHANGE:
		case input::EV_INPUT: {
			ASSERT( e.data.value.change_select.text, "change text not set" );
			obj.insert(
				{
					"value",
					VALUE( gse::value::String, , *e.data.value.change_select.text )
				}
			);
			break;
		}
		default: {
			Panel::WrapEvent( GSE_CALL, e, obj );
		}
	}
}

void Input::SetValue( GSE_CALLABLE, const std::string& value ) {
    // Original narrow edit controls trim defaults before the first user edit.
    // Keep the unrestricted path unchanged for unrelated inputs.
    const auto accepted=m_max_bytes ? value.substr(0,std::min(m_max_bytes,value.find('\0'))) : value;
    const bool changed=accepted!=m_value;
    // A repeated overlong assignment still needs its public property repaired,
    // even when clamping leaves the actual control text unchanged.
    UpdateProperty("value",VALUE(gse::value::String,,accepted));
	if ( changed ) {
		m_value = accepted;
        m_cursor=m_value.size();m_anchor=m_native_edit ? 0:m_cursor;
		UpdateText();
		input::Event e;
		e.SetType( input::EV_CHANGE );
		e.data.value.change_select.text = new std::string( m_value );
		ProcessEvent( GSE_CALL, e );
		delete e.data.value.change_select.text;
		FixAlign();
	}
}

void Input::FixAlign() {
	auto* g = m_text->GetGeometry();
    if(m_native_edit) {g->SetLeft(0);g->SetRight(0);g->SetAlign(geometry::Geometry::ALIGN_LEFT_TOP);return;}
	if ( g->m_area.width < m_geometry->m_area.width ) {
		g->SetAlign( geometry::Geometry::ALIGN_LEFT_CENTER );
	}
	else {
		g->SetAlign( geometry::Geometry::ALIGN_RIGHT_CENTER );
	}
}

void Input::UpdateText() {
    if(m_native_edit) {UpdateNativeText();return;}
    m_selection_surface->Hide();m_selection_text->Hide();m_caret_surface->Hide();
	m_text->SetText(
		m_value + ( m_is_blink_cursor_visible
			? "_"
			: " "
		)
	);
}

void Input::UpdateNativeText() {
    m_anchor=std::min(m_anchor,m_value.size());
    m_text->SetText(m_value);
    const auto first=std::min(m_cursor,m_anchor),last=std::max(m_cursor,m_anchor);
    const auto left=m_text->MeasureWidth(m_value.substr(0,std::min(first,m_value.size())));
    if(first!=last) {
        const auto text=m_value.substr(std::min(first,m_value.size()),last-first);auto* geometry=m_selection_surface->GetGeometry();
        geometry->SetLeft(left);geometry->SetTop(0);geometry->SetWidth(m_text->MeasureWidth(text));geometry->SetHeight(m_geometry->m_area.height);
        m_selection_text->SetText(text);m_selection_text->GetGeometry()->SetLeft(left);
        m_selection_surface->Show();m_selection_text->Show();
    }else {m_selection_surface->Hide();m_selection_text->Hide();}
    if(m_is_focused && m_is_blink_cursor_visible) {
        m_caret_surface->PresentSolidColor(first!=last && m_cursor>=first && m_cursor<last ? m_selection_color:m_caret_color);
        auto* caret=m_caret_surface->GetGeometry();caret->SetLeft(m_text->MeasureWidth(m_value.substr(0,m_cursor)));
        caret->SetTop(0);caret->SetWidth(1);caret->SetHeight(m_geometry->m_area.height);m_caret_surface->Show();
    }else {m_caret_surface->Hide();}
}
bool Input::NativeKey(GSE_CALLABLE,const input::Event& event) {
    const auto& key=event.data.key;
    auto result=StepSmacInput({m_value,m_anchor,m_cursor},key.code,key.key,key.is_printable,key.modifiers,m_max_bytes);
    if(!result.handled) {return false;}
    SetValue(GSE_CALL,result.state.text);m_cursor=result.state.caret;m_anchor=result.state.anchor;UpdateNativeText();return true;
}

}
}

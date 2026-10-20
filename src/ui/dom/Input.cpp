#include "Input.h"

#include "Text.h"
#include "ui/geometry/Geometry.h"
#include "engine/Engine.h"
#include "input/Input.h"
#include "input/Event.h"
#include "util/String.h"
#include "ui/UI.h"
#include "gse/value/Int.h"
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
	ForwardProperty( GSE_CALL, "font", m_text );
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
			while ( m_blinker.HasTicked() ) {
				m_is_blink_cursor_visible = !m_is_blink_cursor_visible;
				UpdateText();
			}
		}
	);

	GeometryHandler(
		GH_ON_AREA_UPDATE, [ this ]() {
			FixAlign();
		}
	);
}

void Input::OnFocus() {
	if ( !m_is_focused ) {
		m_is_focused = true;
		m_blinker.SetInterval( 250 ); // TODO: config?
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
	if ( g->m_area.width < m_geometry->m_area.width ) {
		g->SetAlign( geometry::Geometry::ALIGN_LEFT_CENTER );
	}
	else {
		g->SetAlign( geometry::Geometry::ALIGN_RIGHT_CENTER );
	}
}

void Input::UpdateText() {
	m_text->SetText(
		m_value + ( m_is_blink_cursor_visible
			? "_"
			: " "
		)
	);
}

}
}

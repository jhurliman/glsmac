#pragma once

#include "Panel.h"
#include "Focusable.h"

#include "util/Timer.h"
#include "types/Color.h"

namespace ui {
namespace dom {

class Text;
class Surface;

class Input : public Panel, public Focusable {
public:
	Input( DOM_ARGS );

	void OnFocus() override;
	void OnDefocus() override;

	void Show() override;
	void Hide() override;

protected:
	virtual const bool ProcessEventImpl( GSE_CALLABLE, const input::Event& event ) override;

	virtual void WrapEvent( GSE_CALLABLE, const input::Event& e, gse::value::object_properties_t& obj ) const override;

private:

	bool m_is_focused = false;

	std::string m_value = "";
	size_t m_max_bytes = 0; // zero preserves the unrestricted legacy control

	Text* m_text;
    bool m_native_edit=false;
    size_t m_cursor=0,m_anchor=0;
    std::chrono::steady_clock::time_point m_native_blink_at{};
    Surface* m_selection_surface;
    Text* m_selection_text;
    Surface* m_caret_surface;
    types::Color m_caret_color{},m_selection_color{};
    void UpdateNativeText();
    bool NativeKey(GSE_CALLABLE,const input::Event& event);

	bool m_is_blink_cursor_visible = false;
	util::Timer m_blinker;

	void SetValue( GSE_CALLABLE, const std::string& value );
	void FixAlign();

	void UpdateText();

};

}
}

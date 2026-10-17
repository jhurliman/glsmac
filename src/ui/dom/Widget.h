#pragma once

#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "Area.h"

#include "ui/Types.h"

namespace game::frontend::widget {
class Widget;
}

namespace types {
namespace texture {
class Texture;
}
}

namespace scene::actor {
class Actor;
}

namespace ui {
namespace dom {

class Widget : public Area {
public:

	Widget( DOM_ARGS );
	~Widget() override;

	void Show() override;
	void Hide() override;
	void Destroy( GSE_CALLABLE ) override;
	const bool ProcessEvent( GSE_CALLABLE, const input::Event& event ) override;

	// Takes ownership. Fixed-size textures retain their pixels when the widget resizes.
	void SetTexture( types::texture::Texture* const texture, const size_t index = 0, const bool resize_with_widget = true );
	void SetActor( scene::actor::Actor* const actor, const size_t index = 0 );
	// Retain a generated source texture through this widget's borrowed actors.
	void RetainTexture( const std::shared_ptr< types::texture::Texture >& texture );
	void Clear();

	types::texture::Texture* const GetTexture( const size_t index = 0 ) const;

	void OnUpdate( const f_widget_update_t& on_widget_update );
	void OnRemove( const f_widget_remove_t& on_widget_remove );

	typedef std::function< bool( const input::Event& ) > f_on_event_t;
#define F_EVENT_HANDLER( ... ) [ __VA_ARGS__ ]( const input::Event& event )
	void SetEventHandler( const input::event_type_t type, const f_on_event_t& f );
	void SetGlobalEventHandler( const input::event_type_t type, const f_on_event_t& f );

	const gse::value::object_properties_t& GetData() const;

private:

	widget_type_t m_type = WT_NONE;
	void Enable( const widget_type_t type );
	void Disable();
	void SetData( gse::value::Object* const data );

	bool m_is_attached = false;
	void Attach();
	void Detach();

	gse::value::Object* m_data = nullptr;
	bool m_data_update_needed = false;

	std::unordered_map< size_t, types::texture::Texture* > m_textures = {};
	std::unordered_set< size_t > m_fixed_size_textures;
	std::unordered_map< size_t, scene::actor::Actor* > m_actors = {};
	std::vector< std::shared_ptr< types::texture::Texture > > m_retained_textures;

	f_widget_update_t m_on_widget_update = nullptr;
	f_widget_remove_t m_on_widget_remove = nullptr;
	void UpdateWidget();

	std::unordered_map< input::event_type_t, f_on_event_t > m_event_handlers = {};
	std::unordered_map< input::event_type_t, f_on_event_t > m_global_event_handlers;
	size_t m_global_handler_id = 0;
};

}
}

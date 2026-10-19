#pragma once

#include "Widget.h"
#include "game/SmacDetailMinimap.h"

#include "types/Vec2.h"
#include <memory>
#include <optional>
#include <string>

namespace types::texture {
class Texture;
}

namespace game {
namespace frontend {
namespace widget {

CLASS( Minimap, Widget )
public:

	Minimap( Game* const game, ui::UI* const ui );
	virtual ~Minimap();

	const types::Vec2< ui::coord_t > FindLargestArea() const;

	void Update( ui::dom::Widget* const widget, const void* const data ) override;
	// Consumes a fresh capture response; Update continues to borrow cached data.
	void UpdateCapture( types::texture::Texture* texture );

	void SetMinimapSelection( const types::Vec2< float > position_percents, const types::Vec2< float > zoom );

protected:
	void Register( ui::dom::Widget* const widget ) override;

private:
	struct Overview {
		int32_t width, height;
		bool horizontal_wrap;
		std::string rgba;
	};
	std::optional< Overview > m_overview;
	bool m_using_overview = false;
    std::optional<::game::smac_detail::World> m_detail;
    ::game::smac_detail::Point m_detail_center{{40,40}};
    bool m_detail_mode=false,m_detail_initialized=false,m_shift_down=false;
    void ReadDetail(gse::value::Object* data);
    void DrawDetail(ui::dom::Widget* widget);
    void DrawDetailOutline(ui::dom::Widget* widget);
	std::shared_ptr< const bool > m_lifetime = std::make_shared< const bool >( true );
	void ReadOverview( ui::dom::Widget* widget, gse::value::Object* data );
	void DrawOverview( ui::dom::Widget* widget );
	enum layer_index_t {
		LI_PREVIEW = 0,
		LI_SELECTION = 1,
	};

	bool m_is_dragging = false;

	types::texture::Texture* m_last_texture = nullptr;
	types::Vec2< float > m_last_position_percents = {};
	types::Vec2< float > m_last_zoom = {};

	void UpdateImpl( ui::dom::Widget* const widget, const types::texture::Texture* const texture );
	void SetMinimapSelectionImpl( ui::dom::Widget* const widget, const types::Vec2< float > position_percents, const types::Vec2< float > zoom );

};

}
}
}

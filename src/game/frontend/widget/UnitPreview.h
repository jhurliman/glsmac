#pragma once

#include "Widget.h"
#include <memory>

namespace game {
namespace frontend {

namespace widget {

CLASS( UnitPreview, Widget )
public:
	UnitPreview( Game* const game, ui::UI* const ui );

	void Update( ui::dom::Widget* const widget, const void* const data ) override;

protected:
	void Register( ui::dom::Widget* const widget ) override;

private:
	// Expires before the base presenter unregisters. Retained DOM callbacks
	// must check this before accessing the presenter or its Game.
	std::shared_ptr< const bool > m_lifetime = std::make_shared< const bool >( true );

};

}
}
}

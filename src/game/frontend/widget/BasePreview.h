#pragma once

#include "Widget.h"
#include <memory>
#include <unordered_map>
#include "SmacBaseCard.h"

namespace game {
namespace frontend {

namespace widget {

CLASS( BasePreview, Widget )
public:
	BasePreview( Game* const game, ui::UI* const ui );

	void Update( ui::dom::Widget* const widget, const void* const data ) override;

protected:
	void Register( ui::dom::Widget* const widget ) override;

private:
	std::unordered_map<std::string,SmacBaseCard> m_cards;
	std::shared_ptr<const bool> m_lifetime=std::make_shared<const bool>(true);

};

}
}
}

#pragma once

#include "Geometry.h"
#include <optional>

namespace scene::actor {
class Text;
}

namespace ui {
namespace geometry {

class Text : public Geometry {
public:
	Text( const UI* const ui, Geometry* const parent );

	void SetActor( scene::actor::Text* const actor );
    // Optional baseline measured from the parent control top, in logical pixels.
    void SetParentBaseline( std::optional<coord_t> baseline );

protected:
	void UpdateImpl() override;

private:
	scene::actor::Text* m_actor = nullptr;
    std::optional<coord_t> m_parent_baseline;

};

}
}

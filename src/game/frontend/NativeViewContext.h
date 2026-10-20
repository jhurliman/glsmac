#pragma once
#include "NativeTerrainView.h"
#include "SmacKnownGrid.h"
#include "game/NativeTerrainWorld.h"

namespace game::frontend::native_terrain {
// UI-owned current primary view. Receiving a world/worker snapshot must not
// initialize this again. Observer identity remains in the owned world snapshot.
struct ViewContext {
    int index=0,active=1;
    uint32_t flags=0x60010ee3u;
    int zoom=0;
    Point camera{{0,0}};
    // Captured missing-profile default: non-laptop, difficulty below2. A
    // profile owner may supply complete active words; this is not an INI loader.
    smac_known_grid::Preferences preferences{0xbbe1ddb6u,0x003a7168u};
    int relief=25;
    static constexpr int SurveyLight=2;
    void ToggleFlattenedTerrain() {
        preferences.more^=0x10000u;
        relief=(preferences.more&0x10000u)?8:25;
    }
    ViewLayout Layout(const ::game::native_terrain::World& world) const {
        return native_terrain::Layout(camera[0],camera[1],zoom,world.horizontal_wrap,
            {{world.width,world.height}});
    }
};
}

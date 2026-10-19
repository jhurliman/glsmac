#pragma once
#include "NativeTerrainRenderer.h"
namespace game::frontend::native_terrain {
struct Session {
    static bool Admits(const ::game::native_terrain::World& world) {
        if(!world.retained_geometry || !world.horizontal_wrap) {return false;}
        for(const auto& cell:world.tiles) {
            if(cell.band>7) {return false;}
            if(cell.band<3 || !(cell.landmarks&0xb03u)) {continue;}
            const unsigned detail=cell.landmarks>>24;
            if(detail<49) {continue;}
            const auto& negative=backend::map::smac_terrain::landmark_tables::NEGATIVE_DETAILS;
            if(detail<128 || std::find(negative.begin(),negative.end(),int(detail)-256)==negative.end()) {return false;}
        }return true;
    }
    explicit Session(const ::game::native_terrain::World& world):geometry(world) {
        for(size_t i=0;i<3200;++i) {cells[i]={world.tiles[i].band,world.tiles[i].landmarks};}
    }
    bool Matches(const ::game::native_terrain::World& world) const {
        if(!Admits(world)) {return false;}
        for(size_t i=0;i<3200;++i) {
            if(cells[i].first!=world.tiles[i].band || cells[i].second!=world.tiles[i].landmarks) {return false;}
        }return true;
    }
    ViewGeometry geometry;
    std::optional<Frame> frame;
    std::optional<ViewLayout> layout;
    std::array<std::pair<uint8_t,uint32_t>,3200> cells{};
};
}

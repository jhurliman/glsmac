#pragma once
#include "NativeTerrainRenderer.h"
#include "NativeViewContext.h"
namespace game::frontend::native_terrain {
struct Session {
    static bool Admits(const ::game::native_terrain::World& world) {
        if(!world.ValidDimensions() || world.width!=80 || world.height!=80 || !world.retained_geometry || !world.horizontal_wrap) {return false;}
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
        if(!Admits(world)) {throw std::invalid_argument("Unsupported native view session world");}
        for(size_t i=0;i<3200;++i) {cells[i]={world.tiles[i].band,world.tiles[i].landmarks};}
    }
    bool Matches(const ::game::native_terrain::World& world) const {
        if(!Admits(world)) {return false;}
        for(size_t i=0;i<3200;++i) {
            if(cells[i].first!=world.tiles[i].band || cells[i].second!=world.tiles[i].landmarks) {return false;}
        }return true;
    }
    enum class Observation {Observer,RevealedInspector};
    void InvalidateAppearance() {frame.reset();layout.reset();pending_observer_.reset();}
    std::optional<ViewGeometry::Plan> Prepare(const ::game::native_terrain::World& world,
        const ViewContext& context,Observation observation=Observation::Observer,const ViewGeometry::Observer& observer={}) {
        if(!context.active || !Matches(world)) {InvalidateAppearance();return {};}
        const bool inspection=observation==Observation::RevealedInspector;
        ViewGeometry::Knowledge known;
        std::optional<int> survey;
        if(!inspection) {
            known.reserve(world.tiles.size());
            for(size_t i=0;i<world.tiles.size();++i) {
                const auto value=world.KnowledgeAt(i);
                if(!value) {InvalidateAppearance();return {};}
                known.push_back(*value==::game::native_terrain::World::Knowledge::Known);
                if(*value==::game::native_terrain::World::Knowledge::Survey) {survey=ViewContext::SurveyLight;}
            }
        }
        const auto next=context.Layout(world);
        const int viewer=inspection?-1:world.viewer;
        const bool incremental=!pending_observer_ && frame && layout && observer_==viewer &&
            frame->display_flags==context.flags && frame->grid_preferences==context.preferences &&
            bool(HorizontalPan(*layout,next));
        auto result=geometry.Prepare(next,context.zoom,context.relief,incremental,true,observer,
            inspection?nullptr:&known,survey);
        pending_observer_=viewer;return result;
    }
    // Publish bitmap/layout and observer identity together only after rendering
    // succeeds. An uncommitted preparation cannot authorize retained pixels.
    void Commit(Frame next,const ViewLayout& view) {
        if(!pending_observer_) {throw std::logic_error("Native view commit without preparation");}
        frame=std::move(next);layout=view;observer_=*pending_observer_;pending_observer_.reset();
    }
    ViewGeometry geometry;
    std::optional<Frame> frame;
    std::optional<ViewLayout> layout;
    std::array<std::pair<uint8_t,uint32_t>,3200> cells{};
private:
    int observer_=0;
    std::optional<int> pending_observer_;
};
}

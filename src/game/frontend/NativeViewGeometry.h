#pragma once
#include "NativeTerrainView.h"
#include "game/NativeTerrainWorld.h"
#include "game/backend/map/SmacLandmarkGeometry.h"
#include <functional>
#include <memory>
#include <utility>

namespace game::frontend::native_terrain {
// Owned view geometry, independent of backend preparation or borrowed map state.
class ViewGeometry {
public:
    struct Query {
        enum class Kind {Height,Shade,Projector};
        Kind kind;Point tile;unsigned vertex;Point result;
    };
    using Observer=std::function<void(const Query&)>;
    using Knowledge=std::vector<bool>; // synchronous owned-caller input; never retained
    struct Terrain {
        Placement placement;
        Point projector,flat_center;
        std::array<int,5> heights{},lighting{};
        bool unknown=false,survey=false;
        std::array<Point,4> outline{};
    };
    struct Object {Point tile,projector,dispatcher;};
    struct Plan {
        std::optional<ViewRegions> regions;
        Point scroll{{0,0}};
        std::array<int,4> clip{{0,0,1024,511}};
        std::vector<Terrain> terrain;
        std::vector<Object> objects;
        bool opaque_background=false;
        Point world{{80,80}},viewport{{1024,511}};
        int unit=12;
    };
    using Cell=backend::map::smac_terrain::LandmarkGeometry::Cell;
    ViewGeometry(int width,int height,bool wrap,std::vector<Cell> cells,bool pinned_planet=false)
        : bands_(Bands(cells)),geometry_(width,height,wrap,std::move(cells),false,pinned_planet),
          world_{{width,height}},wrap_(wrap) {}
    explicit ViewGeometry(const ::game::native_terrain::World& world)
        : ViewGeometry(world.width,world.height,world.horizontal_wrap,Cells(world),world.width==80 && world.height==80) {}
    Plan Prepare(const ViewLayout& view,int zoom,int relief,bool incremental,
        bool paint=true,const Observer& observer={},const Knowledge* known=nullptr,std::optional<int> survey_light=std::nullopt) {
        if(view.world!=world_ || view.horizontal_wrap!=wrap_ || view.viewport[0]<=0 || view.viewport[1]<=0) {
            throw std::invalid_argument("Native view and geometry dimensions/topology differ");
        }
        if(known && known->size()!=bands_.size()) {throw std::invalid_argument("Native knowledge dimensions differ");}
        if(survey_light && !known) {throw std::invalid_argument("Survey requires explicit observer knowledge");}
        geometry_.Configure(zoom,relief);Plan result;result.opaque_background=known!=nullptr;
        result.world=world_;result.viewport=view.viewport;result.unit=view.unit;
        result.clip={{0,0,view.viewport[0],view.viewport[1]}};
        if(!paint) {return result;}
        if(incremental && previous_ && zoom==zoom_ && relief==relief_) {
            result.regions=HorizontalPan(*previous_,view);
            if(!result.regions) {throw std::invalid_argument("Unmeasured incremental native view");}
            result.scroll={{(previous_->first[0]-view.first[0])*4*view.unit,0}};
            const int dx=result.scroll[0];
            result.clip=dx<0?std::array<int,4>{{view.viewport[0]+dx,0,view.viewport[0],view.viewport[1]}}:std::array<int,4>{{0,0,dx,view.viewport[1]}};
            for(const auto& tile:{view.first,previous_->first}) {
                Projector(view,tile,observer);
                for(unsigned v=4;v>0;--v) {Height(tile,v,observer);}
            }
        }
        for(const auto& placement:SelectView(view,result.regions)) {
            const auto tile=placement.tile;Terrain output{};output.placement=placement;
            output.projector=Projector(view,tile,observer);
            int sum=0;for(unsigned v=4;v>0;--v) {sum+=Height(tile,v,observer);}
            output.flat_center={{output.projector[0]+4*view.unit,
                output.projector[1]+Geometry::FloorQuarter(sum)+2*view.unit}};
            const auto index=size_t(tile[1])*size_t(world_[0]/2)+size_t(tile[0])/2;
            output.survey=known && !known->at(index) && survey_light.has_value();
            output.unknown=known && !known->at(index) && !output.survey;
            if(output.unknown) {
                constexpr unsigned triangles[4][3]={{0,1,2},{0,2,3},{0,3,4},{0,4,1}};
                const Point delta[5]={{{0,0}},{{-4*view.unit,0}},{{0,-2*view.unit}},{{4*view.unit,0}},{{0,2*view.unit}}};
                for(unsigned t=0;t<4;++t) {for(unsigned j=0;j<3;++j) {
                    const auto v=triangles[t][j];Height(tile,v,observer);
                    const auto cached=Require(geometry_.CachedHeight(tile[0],tile[1],v));
                    if(j==1) {output.outline[t]={{output.flat_center[0]+delta[v][0],output.flat_center[1]+delta[v][1]-cached}};}
                }}
            } else if(bands_[index]>=3) {
                constexpr unsigned triangles[4][3]={{0,1,2},{0,2,3},{0,3,4},{0,4,1}};
                for(const auto& triangle:triangles) {for(const auto v:triangle) {
                    output.heights[v]=Height(tile,v,observer);
                    if(output.survey) {output.lighting[v]=*survey_light;}
                    else {
                        const auto shade=Require(geometry_.Shade(tile[0],tile[1],v));output.lighting[v]=shade;
                        if(observer) {observer({Query::Kind::Shade,tile,v,{{shade,0}}});}
                    }
                }}
            }
            result.terrain.push_back(output);
        }
        for(const auto& placement:SelectView(view,result.regions,true)) {
            const auto p=Projector(view,placement.tile,observer);
            result.objects.push_back({placement.tile,p,{{p[0],p[1]-2*view.unit}}});
        }
        previous_=view;zoom_=zoom;relief_=relief;return result;
    }
    std::vector<uint8_t> CacheBytes() const {return geometry_.CacheBytes();}
private:
    using Geometry=backend::map::smac_terrain::LandmarkGeometry;
    static std::vector<Geometry::Cell> Cells(const ::game::native_terrain::World& world) {
        std::vector<Geometry::Cell> result;result.reserve(world.tiles.size());
        for(size_t i=0;i<world.tiles.size();++i) {
            const auto& cell=world.tiles[i];
            result.push_back({cell.band,cell.landmarks});
        }
        return result;
    }
    static int Require(std::optional<int> value) {
        if(!value) {throw std::invalid_argument("Unsupported native view geometry profile");}return *value;
    }
    int Height(Point tile,unsigned vertex,const Observer& observer) {
        const int value=Require(geometry_.Height(tile[0],tile[1],vertex));
        if(observer) {observer({Query::Kind::Height,tile,vertex,{{value,0}}});}return value;
    }
public:
    // Also used for actors outside the terrain traversal (including offscreen
    // anchors). Preserve the original flat-map out-of-view displacement rule.
    Point Projector(const ViewLayout& view,Point tile,const Observer& observer={}) {
        if(view.world!=world_ || view.horizontal_wrap!=wrap_) {throw std::invalid_argument("Native projector topology differs");}
        const int w=world_[0];int candidate=tile[0];
        const bool inside_y=view.first[1]<=tile[1] && tile[1]<view.first[1]+view.span[1]+view.span[3];
        if(inside_y && wrap_) {
            if(candidate<view.first[0]) {candidate+=w;}
            if(candidate>=view.first[0]+view.span[0]+view.span[2]) {candidate-=w;}
        }
        int dx=tile[0]-view.first[0];
        if(inside_y && view.first[0]<=candidate && candidate<view.first[0]+view.span[0]+view.span[2]) {
            if(wrap_) {if(dx<0) {dx+=w;}else if(dx>=w) {dx-=w;}}
        } else {if(dx>w/2) {dx-=w;}if(dx< -w/2) {dx+=w;}}
        const Point value{{view.offset[0]+(dx-1)*4*view.unit,view.offset[1]+(tile[1]-view.first[1]-1)*2*view.unit-
            Require(geometry_.ProjectorHeight(tile[0],tile[1]))}};
        if(observer) {observer({Query::Kind::Projector,tile,0,value});}return value;
    }
private:
    static std::vector<uint8_t> Bands(const std::vector<Cell>& cells) {
        std::vector<uint8_t> result;result.reserve(cells.size());
        for(const auto& cell:cells) {result.push_back(cell.band);}return result;
    }
    // Own the appearance bands independently of the geometry cache.
    std::vector<uint8_t> bands_;
    Geometry geometry_;
    Point world_;bool wrap_;
    std::optional<ViewLayout> previous_;
    int zoom_=0,relief_=25;
};
}

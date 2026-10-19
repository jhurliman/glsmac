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
    struct Terrain {
        Placement placement;
        Point projector,flat_center;
        std::array<int,5> heights{},lighting{};
    };
    struct Object {Point tile,projector,dispatcher;};
    struct Plan {
        std::optional<ViewRegions> regions;
        Point scroll{{0,0}};
        std::array<int,4> clip{{0,0,1024,511}};
        std::vector<Terrain> terrain;
        std::vector<Object> objects;
    };
    explicit ViewGeometry(const ::game::native_terrain::World& world)
        : geometry_(80,80,world.horizontal_wrap,Cells(world),false,true) {
        if(!world.horizontal_wrap) {throw std::invalid_argument("Native view schedule requires wrapped Planet");}
    }
    Plan Prepare(const ViewLayout& view,int zoom,int relief,bool incremental,
        bool paint=true,const Observer& observer={}) {
        if(!view.horizontal_wrap) {throw std::invalid_argument("Native view schedule requires wrapped Planet");}
        geometry_.Configure(zoom,relief);Plan result;
        if(!paint) {return result;}
        if(incremental && previous_ && zoom==zoom_ && relief==relief_) {
            result.regions=HorizontalPan(*previous_,view);
            if(!result.regions) {throw std::invalid_argument("Unmeasured incremental native view");}
            result.scroll={{(previous_->first[0]-view.first[0])*4*view.unit,0}};
            const int dx=result.scroll[0];
            result.clip=dx<0?std::array<int,4>{{1024+dx,0,1024,511}}:std::array<int,4>{{0,0,dx,511}};
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
            const auto index=size_t(tile[1])*40+size_t(tile[0])/2;
            if(bands_[index]>=3) {
                constexpr unsigned triangles[4][3]={{0,1,2},{0,2,3},{0,3,4},{0,4,1}};
                for(const auto& triangle:triangles) {for(const auto v:triangle) {
                    output.heights[v]=Height(tile,v,observer);
                    const auto shade=Require(geometry_.Shade(tile[0],tile[1],v));output.lighting[v]=shade;
                    if(observer) {observer({Query::Kind::Shade,tile,v,{{shade,0}}});}
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
private:
    using Geometry=backend::map::smac_terrain::LandmarkGeometry;
    std::vector<Geometry::Cell> Cells(const ::game::native_terrain::World& world) {
        std::vector<Geometry::Cell> result;result.reserve(world.tiles.size());
        for(size_t i=0;i<world.tiles.size();++i) {
            const auto& cell=world.tiles[i];bands_[i]=cell.band;
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
    Point Projector(const ViewLayout& view,Point tile,const Observer& observer) {
        int dx=tile[0]-view.first[0];const int wrapped=(dx%80+80)%80;
        if(view.first[1]<=tile[1] && tile[1]<view.first[1]+view.span[1]+view.span[3] && wrapped<view.span[0]+view.span[2]) {dx=wrapped;}
        else {if(dx>40) {dx-=80;}if(dx< -40) {dx+=80;}}
        const Point value{{(dx-1)*4*view.unit,(tile[1]-view.first[1]-1)*2*view.unit-
            Require(geometry_.ProjectorHeight(tile[0],tile[1]))}};
        if(observer) {observer({Query::Kind::Projector,tile,0,value});}return value;
    }
    // Declared first: constructor's Cells() fills these before geometry exists.
    std::array<uint8_t,3200> bands_{};
    Geometry geometry_;
    std::optional<ViewLayout> previous_;
    int zoom_=0,relief_=25;
};
}

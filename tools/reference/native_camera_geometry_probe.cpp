#include "game/frontend/NativeViewGeometry.h"
#include <iostream>
#include <stdexcept>
namespace native=game::frontend::native_terrain;
using Geometry=game::backend::map::smac_terrain::LandmarkGeometry;
int main() {
    int count;std::cin>>count;
    if(!std::cin || count<0 || count>100) {throw std::runtime_error("Invalid context count");}
    for(int c=0;c<count;++c) {
        int w,h,vw,vh,x,y,z,flat,relief,n;std::cin>>w>>h>>vw>>vh>>x>>y>>z>>flat>>relief>>n;
        if(!std::cin || n<0 || n>1000) {throw std::runtime_error("Invalid context");}
        const auto view=native::Layout(x,y,z,!flat,{{w,h}},{{vw,vh}});
        std::cout<<view.unit<<' '<<view.first[0]<<' '<<view.first[1];
        for(int value:view.span) {std::cout<<' '<<value;}
        std::cout<<' '<<view.offset[0]<<' '<<view.offset[1]<<'\n';
        const std::vector<Geometry::Cell> cells(size_t(w/2)*h,{4,0});
        native::ViewGeometry geometry(w,h,!flat,cells);Geometry heights(w,h,!flat,cells);
        geometry.Prepare(view,z,relief,false,false);heights.Configure(z,relief);
        for(int i=0;i<n;++i) {
            int tx,ty;std::cin>>tx>>ty;const auto p=geometry.Projector(view,{{tx,ty}});
            std::cout<<p[0]<<' '<<p[1];
            for(unsigned v=0;v<5;++v) {std::cout<<' '<<heights.Height(tx,ty,v).value();}
            std::cout<<'\n';
        }
        // Exercise the production traversal with a dimensioned all-unknown
        // mask. It must neither shade nor index using a Planet-only stride.
        native::ViewGeometry unknown(w,h,!flat,cells);
        const native::ViewGeometry::Knowledge known(cells.size(),false);
        size_t shades=0;
        const auto plan=unknown.Prepare(view,z,relief,false,true,[&](const auto& q) {
            if(q.kind==native::ViewGeometry::Query::Kind::Shade) {++shades;}
        },&known);
        if(shades || !plan.opaque_background || plan.world!=view.world || plan.viewport!=view.viewport ||
            plan.clip!=std::array<int,4>{{0,0,vw,vh}} || plan.terrain.empty()) {throw std::runtime_error("Unknown traversal metadata");}
        for(const auto& t:plan.terrain) {
            if(!t.unknown || t.placement.record_offset/44>=cells.size()) {throw std::runtime_error("Unknown traversal stride");}
        }
        bool rejected=false;
        try {const native::ViewGeometry::Knowledge bad(1,false);unknown.Prepare(view,z,relief,false,true,{},&bad);}
        catch(const std::invalid_argument&) {rejected=true;}
        if(!rejected) {throw std::runtime_error("Wrong-size mask accepted");}
    }
    // Large admitted record offsets exceed signed int even when the visible
    // selection is tiny. This is a bounds check, not a native fidelity claim.
    const auto large=native::Layout(65533,2899,16,false,{{65534,2900}},{{1,1}});
    for(const auto& p:native::SelectView(large)) {
        const auto expected=44ull*(uint64_t(p.tile[1])*32767u+uint64_t(p.tile[0])/2u);
        if(p.record_offset!=expected) {throw std::runtime_error("Record offset overflow");}
    }
    int edges;std::cin>>edges;
    for(int i=0;i<edges;++i) {
        int zoom,band;std::cin>>zoom>>band;
        Geometry g(80,80,true,std::vector<Geometry::Cell>(3200,{uint8_t(band),0}));g.Configure(zoom,25);
        std::cout<<g.Height(40,40,0).value()<<' '<<g.Height(40,40,0).value()<<'\n';
        g.Configure(zoom,8);g.Configure(zoom,25);
        std::cout<<g.Height(40,40,0).value()<<' '<<g.CachedHeight(40,40,0).value()<<'\n';
    }
    if(!std::cin) {throw std::runtime_error("Truncated probe input");}
}

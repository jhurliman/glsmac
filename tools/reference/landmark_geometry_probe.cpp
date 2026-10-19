#include "game/backend/map/SmacLandmarkGeometry.h"
#include <iostream>
#include <stdexcept>
using game::backend::map::smac_terrain::LandmarkGeometry;
int main() {
    // Unsupported detail must remain explicit absence; a negative detail on
    // unrelated bookkeeping must not invalidate ordinary geometry elsewhere.
    for(uint32_t word:{0x31000001u,0xff000001u,0xff000000u}) {
        std::vector<LandmarkGeometry::Cell> cells(3200,{4,0});
        cells[40*40+20].landmarks=word;
        LandmarkGeometry model(80,80,true,std::move(cells));
        if(bool(model.Height(40,40,0))!=(!(word&1u))) {
            throw std::runtime_error("Invalid detail admission");
        }
    }
    for(int value=-129;value<=129;++value) {
        const int quarter=LandmarkGeometry::FloorQuarter(value);
        if(quarter*4>value || quarter*4+4<=value) {throw std::runtime_error("Signed floor division");}
    }
    unsigned count;
    if(!(std::cin>>count)) {throw std::runtime_error("Missing context count");}
    for(unsigned c=0;c<count;++c) {
        int band,coast,step;unsigned marked,requests;
        std::cin>>band>>coast>>step>>marked;
        std::vector<LandmarkGeometry::Cell> cells(3200,{uint8_t(band),0});
        if(coast) {cells[39*40+39/2].band=2;}
        for(unsigned i=0;i<marked;++i) {
            int x,y;uint32_t word;std::cin>>x>>y>>word;
            cells.at(size_t(y)*40+size_t(x/2)).landmarks=word;
        }
        LandmarkGeometry model(80,80,true,std::move(cells),step==8);
        std::cin>>requests;
        for(unsigned i=0;i<requests;++i) {
            int x,y;unsigned vertex;std::cin>>x>>y>>vertex;
            const auto height=model.Height(x,y,vertex),shade=model.ExtraShade(x,y,vertex);
            std::cout<<(height?*height:9999)<<' '<<(shade?*shade:9999)<<'\n';
        }
        if(!std::cin) {throw std::runtime_error("Truncated geometry context");}
    }
}
